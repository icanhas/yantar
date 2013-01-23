#include "shared.h"
#include "common.h"
 
enum {
	Zoneid		= /* Winnipeg as mandatory */ 0x5ca7feed,
	Minfrag		= 64,
	Hunkmagic	= 0x000b7e55,
	Hunkfreemagic	= 0xfeeee717,

	Minhunkmegs	= 56,
	Mindedhunkmegs	= 1,
	Defhunkmegs	= 128,
	Defzonemegs	= 24
};

typedef struct Zonedebug	Zonedebug;
typedef struct Memblk		Memblk;
typedef struct Memzone		Memzone;
typedef struct Memstatic	Memstatic;
typedef struct Hunkhdr		Hunkhdr;
typedef struct Hunkused		Hunkused;
typedef struct Hunkblk		Hunkblk;

struct Zonedebug {
	char	*label;
	char	*file;
	int	line;
	int	allocSize;
};

struct Memblk {
	int	size;	/* including the header and possibly tiny fragments */
	int	tag;	/* a tag of 0 is a free block */
	Memblk	*next;
	Memblk	*prev;
	int	id;	/* should be Zoneid */
#ifdef ZONE_DEBUG
	Zonedebug	d;
#endif
};

struct Memzone {
	int	size;		/* total bytes malloced, including header */
	int	used;		/* total bytes used */
	Memblk	blocklist;	/* start / end cap for linked list */
	Memblk	*rover;
};

/* static mem blocks to reduce a lot of small zone overhead */
struct Memstatic {
	Memblk	b;
	byte	mem[2];
};

struct Hunkhdr {
	uint	magic;
	uint	size;
};

struct Hunkused {
	int	mark;
	int	permanent;
	int	temp;
	int	tempHighwater;
};

struct Hunkblk {
	int	size;
	byte	printed;
	Hunkblk	*next;
	char	*label;
	char	*file;
	int	line;
};

extern Fhandle logfile;	/* in common.c */

/* Main zone for all dynamic memory allocation */
static Memzone *mainzone;
/* 
 * We also have a small zone for small allocations that would only
 * fragment the main zone (think of cvar and cmd strings) 
 */
static Memzone *smallzone;
static Hunkblk *hunkblocks;
static Hunkused hunk_low, hunk_high;
static Hunkused *hunk_permanent, *hunk_temp;
static byte *s_hunkData = nil;
static int s_hunkTotal;
static int s_zoneTotal;
static int s_smallZoneTotal;

/*
 * Zone memory allocation
 *
 * There is never any space between memblocks, and there will never be two
 * contiguous free memblocks.
 *
 * The rover can be left pointing at a non-empty block.
 *
 * The zone calls are only used for small strings and structures;
 * all big things are allocated on the hunk.
 */

void
Z_ClearZone(Memzone *zone, int size)
{
	Memblk *block;

	/* set the entire zone to one free block */

	zone->blocklist.next = zone->blocklist.prev = block = (Memblk*)((byte*)zone + sizeof(Memzone));
	zone->blocklist.tag = 1;	/* in use block */
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;
	zone->size = size;
	zone->used = 0;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;	/* free block */
	block->id = Zoneid;
	block->size = size - sizeof(Memzone);
}

int
Z_AvailableZoneMemory(Memzone *zone)
{
	return zone->size - zone->used;
}

int
zmemavailable(void)
{
	return Z_AvailableZoneMemory(mainzone);
}

void
zfree(void *ptr)
{
	Memblk *block, *other;
	Memzone *zone;

	if(!ptr)
		comerrorf(ERR_DROP, "zfree: nil pointer");

	block = (Memblk*)((byte*)ptr - sizeof(Memblk));
	if(block->id != Zoneid)
		comerrorf(ERR_FATAL, "zfree: freed a pointer without Zoneid");
	if(block->tag == 0)
		comerrorf(ERR_FATAL, "zfree: freed a freed pointer");
	/* if static memory */
	if(block->tag == MTstatic)
		return;

	/* check the memory trash tester */
	if(*(int*)((byte*)block + block->size - 4) != Zoneid)
		comerrorf(ERR_FATAL, "zfree: memory block wrote past end");

	if(block->tag == MTsmall)
		zone = smallzone;
	else
		zone = mainzone;

	zone->used -= block->size;
	/* 
	 * set the block to something that should cause problems
	 * if it is referenced... 
	 */
	Q_Memset(ptr, 0xaa, block->size - sizeof(*block));

	block->tag = 0;	/* mark as free */

	other = block->prev;
	if(!other->tag){
		/* merge with previous free block */
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if(block == zone->rover)
			zone->rover = other;
		block = other;
	}

	zone->rover = block;

	other = block->next;
	if(!other->tag){
		/* merge the next free block onto the end */
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if(other == zone->rover)
			zone->rover = block;
	}
}

void
zfreetags(int tag)
{
	int count;
	Memzone *zone;

	if(tag == MTsmall)
		zone = smallzone;
	else
		zone = mainzone;
	count = 0;
	/* 
	 * use the rover as our pointer, because
	 * zfree automatically adjusts it 
	 */
	zone->rover = zone->blocklist.next;
	do {
		if(zone->rover->tag == tag){
			count++;
			zfree((void*)(zone->rover + 1));
			continue;
		}
		zone->rover = zone->rover->next;
	} while(zone->rover != &zone->blocklist);
}

#ifdef ZONE_DEBUG
void *ztagallocdebug(int size, int tag, char *label, char *file, int line)
{
	int allocSize;
#else
void* ztagalloc(int size, int tag)
{
#endif
	int extra;
	Memblk *start, *rover, *new, *base;
	Memzone *zone;

	if(!tag)
		comerrorf(ERR_FATAL, "ztagalloc: tried to use a 0 tag");

	if(tag == MTsmall)
		zone = smallzone;
	else
		zone = mainzone;

#ifdef ZONE_DEBUG
	allocSize = size;
#endif
	/* scan through the block list looking for the first free block
	 * of sufficient size */
	size += sizeof(Memblk);		/* account for size of block header */
	size += 4;				/* space for memory trash tester */
	size = PAD(size, sizeof(intptr_t));	/* align to 32/64 bit boundary */

	base = rover = zone->rover;
	start = base->prev;

	do {
		if(rover == start){
			/* scaned all the way around the list */
#ifdef ZONE_DEBUG
			zlogheap();
			comerrorf(
				ERR_FATAL, "zalloc: failed on allocation of"
					   " %i bytes from the %s zone: %s, line: %d (%s)",
				size, zone == smallzone ? "small" : "main",
				file, line, label);
#else
			comerrorf(ERR_FATAL, "zalloc: failed on allocation of"
					     " %i bytes from the %s zone",
				size, zone == smallzone ? "small" : "main");
#endif
			return nil;
		}
		if(rover->tag)
			base = rover = rover->next;
		else
			rover = rover->next;
	}while(base->tag || base->size < size);

	/* found a block big enough */
	extra = base->size - size;
	if(extra > Minfrag){
		/* there will be a free fragment after the allocated block */
		new = (Memblk*)((byte*)base + size);
		new->size = extra;
		new->tag = 0;	/* free block */
		new->prev = base;
		new->id = Zoneid;
		new->next = base->next;
		new->next->prev = new;
		base->next = new;
		base->size = size;
	}

	base->tag = tag;	/* no longer a free block */

	zone->rover = base->next;	/* next allocation will start looking here */
	zone->used += base->size;

	base->id = Zoneid;
#ifdef ZONE_DEBUG
	base->d.label = label;
	base->d.file = file;
	base->d.line = line;
	base->d.allocSize = allocSize;
#endif

	/* marker for memory trash testing */
	*(int*)((byte*)base + base->size - 4) = Zoneid;

	return (void*)((byte*)base + sizeof(Memblk));
}

#ifdef ZONE_DEBUG
void*
zallocdebug(int size, char *label, char *file, int line)
{
#else
void*
zalloc(int size)
{
#endif
	void *buf;

	/* Z_CheckHeap ();	// DEBUG */

#ifdef ZONE_DEBUG
	buf = ztagallocdebug(size, MTgeneral, label, file, line);
#else
	buf = ztagalloc(size, MTgeneral);
#endif
	Q_Memset(buf, 0, size);

	return buf;
}

#ifdef ZONE_DEBUG
void*
sallocdebug(int size, char *label, char *file, int line)
{
	return ztagallocdebug(size, MTsmall, label, file, line);
}

#else

void*
salloc(int size)
{
	return ztagalloc(size, MTsmall);
}
#endif

void
Z_CheckHeap(void)
{
	Memblk *block;

	for(block = mainzone->blocklist.next;; block = block->next){
		if(block->next == &mainzone->blocklist)
			break;	/* all blocks have been hit */
		if((byte*)block + block->size != (byte*)block->next)
			comerrorf(ERR_FATAL,
				"Z_CheckHeap: block size does not touch the next block");
		if(block->next->prev != block)
			comerrorf(ERR_FATAL,
				"Z_CheckHeap: next block doesn't have proper back link");
		if(!block->tag && !block->next->tag)
			comerrorf(ERR_FATAL,
				"Z_CheckHeap: two consecutive free blocks");
	}
}

void
Z_LogZoneHeap(Memzone *zone, char *name)
{
#ifdef ZONE_DEBUG
	char	dump[32], *ptr;
	int	i, j;
#endif
	Memblk *block;
	char	buf[4096];
	int	size, allocSize, numBlocks;

	if(!logfile || !fsisinitialized())
		return;
	size = numBlocks = 0;
#ifdef ZONE_DEBUG
	allocSize = 0;
#endif
	Q_sprintf(buf, sizeof(buf),
		"\r\n================\r\n%s log\r\n================\r\n",
		name);
	fswrite(buf, strlen(buf), logfile);
	for(block = zone->blocklist.next; block->next != &zone->blocklist;
	    block = block->next){
		if(block->tag){
#ifdef ZONE_DEBUG
			ptr = ((char*)block) + sizeof(Memblk);
			j = 0;
			for(i = 0; i < 20 && i < block->d.allocSize; i++){
				if(ptr[i] >= 32 && ptr[i] < 127)
					dump[j++] = ptr[i];
				else
					dump[j++] = '_';
			}
			dump[j] = '\0';
			Q_sprintf(buf, sizeof(buf),
				"size = %8d: %s, line: %d (%s) [%s]\r\n",
				block->d.allocSize, block->d.file, block->d.line,
				block->d.label,
				dump);
			fswrite(buf, strlen(buf), logfile);
			allocSize += block->d.allocSize;
#endif
			size += block->size;
			numBlocks++;
		}
	}
#ifdef ZONE_DEBUG
	/* subtract debug memory */
	size -= numBlocks * sizeof(Zonedebug);
#else
	allocSize = numBlocks * sizeof(Memblk);	/* + 32 bit alignment */
#endif
	Q_sprintf(buf, sizeof(buf), "%d %s memory in %d blocks\r\n", size,
		name,
		numBlocks);
	fswrite(buf, strlen(buf), logfile);
	Q_sprintf(buf, sizeof(buf), "%d %s memory overhead\r\n", size -
		allocSize,
		name);
	fswrite(buf, strlen(buf), logfile);
}

void
zlogheap(void)
{
	Z_LogZoneHeap(mainzone, "MAIN");
	Z_LogZoneHeap(smallzone, "SMALL");
}

Memstatic emptystring = {
	{(sizeof(Memblk)+2 + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'\0', '\0'} 
};
Memstatic numberstring[] = {
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'0', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'1', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'2', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'3', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'4', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'5', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'6', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'7', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'8', '\0'} },
	{ {(sizeof(Memstatic) + 3) & ~3, MTstatic, nil, nil, Zoneid}, {'9', '\0'} }
};

/*
 * NOTE:	never write over the memory copystr returns because
 *              memory from a Memstatic might be returned
 */
char*
copystr(const char *in)
{
	char *out;

	if(!in[0])
		return ((char*)&emptystring) + sizeof(Memblk);
	else if(!in[1])
		if(in[0] >= '0' && in[0] <= '9')
			return ((char*)&numberstring[in[0]-'0']) + sizeof(Memblk);
	out = salloc (strlen(in)+1);
	strcpy (out, in);
	return out;
}

/*
 * Hunk memory allocation
 *
 * Goals:
 *      reproducable without history effects -- no out of memory errors on weird map to map changes
 *      allow restarting of the client without fragmentation
 *      minimize total pages in use at run time
 *      minimize total pages needed during load time
 *
 * Single block of memory with stack allocators coming from both ends towards the middle.
 *
 * One side is designated the temporary memory allocator.
 *
 * Temporary memory can be allocated and freed in any order.
 *
 * A highwater mark is kept of the most in use at any time.
 *
 * When there is no temporary memory allocated, the permanent and temp sides
 * can be switched, allowing the already touched temp memory to be used for
 * permanent storage.
 *
 * Temp memory must never be allocated on two ends at once, or fragmentation
 * could occur.
 *
 * If we have any in-use temp memory, additional temp allocations must come from
 * that side.
 *
 * If not, we can choose to make either side the new temp side and push future
 * permanent allocations to the other side.  Permanent allocations should be
 * kept on the side that has the current greatest wasted highwater mark.
 *
 *
 * --- low memory ----
 * server vm
 * server clipmap
 * ---mark---
 * renderer initialization (shaders, etc)
 * UI vm
 * cgame vm
 * renderer map
 * renderer models
 *
 * ---free---
 *
 * temp file loading
 * --- high memory ---
 *
 */

void
Q_Meminfo_f(void)
{
	Memblk *block;
	int zoneBytes, zoneBlocks;
	int smallZoneBytes, smallZoneBlocks;
	int botlibBytes, rendererBytes;
	int unused;

	zoneBytes = 0;
	botlibBytes = 0;
	rendererBytes	= 0;
	zoneBlocks	= 0;
	for(block = mainzone->blocklist.next;; block = block->next){
		if(cmdargc() != 1)
			comprintf ("block:%p    size:%7i    tag:%3i\n",
				(void*)block, block->size, block->tag);
		if(block->tag){
			zoneBytes += block->size;
			zoneBlocks++;
			if(block->tag == MTbotlib)
				botlibBytes += block->size;
			else if(block->tag == MTrenderer)
				rendererBytes += block->size;
		}

		if(block->next == &mainzone->blocklist)
			break;	/* all blocks have been hit */
		if((byte*)block + block->size != (byte*)block->next)
			comprintf ("ERROR: block size does not touch the next block\n");
			/* FIXME: shouldn't this do ERR_FATAL? */
		if(block->next->prev != block)
			comprintf ("ERROR: next block doesn't have proper back link\n");
		if(!block->tag && !block->next->tag)
			comprintf ("ERROR: two consecutive free blocks\n");
	}

	smallZoneBytes	= 0;
	smallZoneBlocks = 0;
	for(block = smallzone->blocklist.next;; block = block->next){
		if(block->tag){
			smallZoneBytes += block->size;
			smallZoneBlocks++;
		}

		if(block->next == &smallzone->blocklist)
			break;	/* all blocks have been hit */
	}

	comprintf("%8i bytes total hunk\n", s_hunkTotal);
	comprintf("%8i bytes total zone\n", s_zoneTotal);
	comprintf("\n");
	comprintf("%8i low mark\n", hunk_low.mark);
	comprintf("%8i low permanent\n", hunk_low.permanent);
	if(hunk_low.temp != hunk_low.permanent)
		comprintf("%8i low temp\n", hunk_low.temp);
	comprintf("%8i low tempHighwater\n", hunk_low.tempHighwater);
	comprintf("\n");
	comprintf("%8i high mark\n", hunk_high.mark);
	comprintf("%8i high permanent\n", hunk_high.permanent);
	if(hunk_high.temp != hunk_high.permanent)
		comprintf("%8i high temp\n", hunk_high.temp);
	comprintf("%8i high tempHighwater\n", hunk_high.tempHighwater);
	comprintf("\n");
	comprintf("%8i total hunk in use\n",
		hunk_low.permanent + hunk_high.permanent);
	unused = 0;
	if(hunk_low.tempHighwater > hunk_low.permanent)
		unused += hunk_low.tempHighwater - hunk_low.permanent;
	if(hunk_high.tempHighwater > hunk_high.permanent)
		unused += hunk_high.tempHighwater - hunk_high.permanent;
	comprintf("%8i unused highwater\n", unused);
	comprintf("\n");
	comprintf("%8i bytes in %i zone blocks\n", zoneBytes, zoneBlocks);
	comprintf("        %8i bytes in dynamic botlib\n", botlibBytes);
	comprintf("        %8i bytes in dynamic renderer\n", rendererBytes);
	comprintf("        %8i bytes in dynamic other\n", zoneBytes -
		(botlibBytes + rendererBytes));
	comprintf("        %8i bytes in small Zone memory\n", smallZoneBytes);
}

/* Touch all known used data to make sure it is paged in */
void
Com_Touchmem(void)
{
	int	start, end;
	int	i, j;
	int	sum;
	Memblk *block;

	Z_CheckHeap();

	start	= Sys_Milliseconds();
	sum	= 0;

	j = hunk_low.permanent >> 2;
	for(i = 0; i < j; i += 64)	/* only need to touch each page */
		sum += ((int*)s_hunkData)[i];

	i = (s_hunkTotal - hunk_high.permanent) >> 2;
	j = hunk_high.permanent >> 2;
	for(; i < j; i += 64)	/* only need to touch each page */
		sum += ((int*)s_hunkData)[i];

	for(block = mainzone->blocklist.next;; block = block->next){
		if(block->tag){
			j = block->size >> 2;
			for(i = 0; i < j; i += 64)	/* ...each page... */
				sum += ((int*)block)[i];
		}
		if(block->next == &mainzone->blocklist)
			break;	/* all blocks have been hit */
	}

	end = Sys_Milliseconds();
	comprintf("Com_Touchmem: %i msec\n", end - start);
}

void
Com_Initsmallzone(void)
{
	s_smallZoneTotal = 512 * 1024;
	smallzone = calloc(s_smallZoneTotal, 1);
	if(smallzone == nil)
		comerrorf(ERR_FATAL, "Small zone data failed to allocate %1.1f"
				     "megs", (float)s_smallZoneTotal /
			(1024*1024));
	Z_ClearZone(smallzone, s_smallZoneTotal);
}

/*
 * N.B.: com_zoneMegs can only be set on the command line, and not in
 * user.cfg or comstartupvar, as they haven't been executed by this
 * point.  It's a chicken and egg problem.  We need the memory manager
 * configured to handle those places where you would configure the memory
 * manager.
 */
void
Com_Initzone(void)
{
	Cvar *cv;

	/* allocate the random block zone */
	cv = cvarget("com_zoneMegs", XSTRING(Defzonemegs),
		CVAR_LATCH | CVAR_ARCHIVE);
	if(cv->integer < Defzonemegs)
		s_zoneTotal = 1024 * 1024 * Defzonemegs;
	else
		s_zoneTotal = cv->integer * 1024 * 1024;

	mainzone = calloc(s_zoneTotal, 1);
	if(!mainzone)
		comerrorf(ERR_FATAL, "Zone data failed to allocate %i megs"
			, s_zoneTotal / (1024*1024));
	Z_ClearZone(mainzone, s_zoneTotal);
}

void
hunklog(void)
{
	Hunkblk *block;
	char	buf[4096];
	int	size, numBlocks;

	if(!logfile || !fsisinitialized())
		return;
	size = 0;
	numBlocks = 0;
	Q_sprintf(buf, sizeof(buf),
		"\r\n================\r\nHunk log\r\n================\r\n");
	fswrite(buf, strlen(buf), logfile);
	for(block = hunkblocks; block; block = block->next){
#ifdef HUNK_DEBUG
		Q_sprintf(buf, sizeof(buf),
			"size = %8d: %s, line: %d (%s)\r\n", block->size,
			block->file,
			block->line,
			block->label);
		fswrite(buf, strlen(buf), logfile);
#endif
		size += block->size;
		numBlocks++;
	}
	Q_sprintf(buf, sizeof(buf), "%d Hunk memory\r\n", size);
	fswrite(buf, strlen(buf), logfile);
	Q_sprintf(buf, sizeof(buf), "%d hunk blocks\r\n", numBlocks);
	fswrite(buf, strlen(buf), logfile);
}

void
Hunk_SmallLog(void)
{
	Hunkblk *block, *block2;
	char	buf[4096];
	int	size, locsize, numBlocks;

	if(!logfile || !fsisinitialized())
		return;
	for(block = hunkblocks; block; block = block->next)
		block->printed = qfalse;
	size = 0;
	numBlocks = 0;
	Q_sprintf(
		buf, sizeof(buf),
		"\r\n================\r\nHunk Small log\r\n================\r\n");
	fswrite(buf, strlen(buf), logfile);
	for(block = hunkblocks; block; block = block->next){
		if(block->printed)
			continue;
		locsize = block->size;
		for(block2 = block->next; block2; block2 = block2->next){
			if(block->line != block2->line)
				continue;
			if(Q_stricmp(block->file, block2->file))
				continue;
			size += block2->size;
			locsize += block2->size;
			block2->printed = qtrue;
		}
#ifdef HUNK_DEBUG
		Q_sprintf(buf, sizeof(buf),
			"size = %8d: %s, line: %d (%s)\r\n", locsize,
			block->file,
			block->line,
			block->label);
		fswrite(buf, strlen(buf), logfile);
#endif
		size += block->size;
		numBlocks++;
	}
	Q_sprintf(buf, sizeof(buf), "%d Hunk memory\r\n", size);
	fswrite(buf, strlen(buf), logfile);
	Q_sprintf(buf, sizeof(buf), "%d hunk blocks\r\n", numBlocks);
	fswrite(buf, strlen(buf), logfile);
}

void
Com_Inithunk(void)
{
	Cvar	*cv;
	int	nMinAlloc;
	char *pMsg = nil;

	/* make sure the file system has allocated and "not" freed any temp blocks
	 * this allows the config and product id files (journal files too) to be loaded
	 * by the file system without redunant routines in the file system utilizing different
	 * memory systems */
	if(fsloadstack() != 0)
		comerrorf(ERR_FATAL,
			"Hunk initialization failed. File system load stack not zero");

	/* allocate the stack based hunk allocator */
	cv = cvarget("com_hunkMegs", XSTRING(Defhunkmegs),
		CVAR_LATCH | CVAR_ARCHIVE);

	/* if we are not dedicated min allocation is 56, otherwise min is 1 */
	if(com_dedicated && com_dedicated->integer){
		nMinAlloc = Mindedhunkmegs;
		pMsg =
			"Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n";
	}else{
		nMinAlloc = Minhunkmegs;
		pMsg = "Minimum com_hunkMegs is %i, allocating %i megs.\n";
	}

	if(cv->integer < nMinAlloc){
		s_hunkTotal = 1024 * 1024 * nMinAlloc;
		comprintf(pMsg, nMinAlloc, s_hunkTotal / (1024 * 1024));
	}else
		s_hunkTotal = cv->integer * 1024 * 1024;

	s_hunkData = calloc(s_hunkTotal + 31, 1);
	if(!s_hunkData)
		comerrorf(ERR_FATAL, "Hunk data failed to allocate %i megs",
			s_hunkTotal / (1024*1024));
	/* cacheline align */
	s_hunkData = (byte*)(((intptr_t)s_hunkData + 31) & ~31);
	hunkclear();

	cmdadd("meminfo", Q_Meminfo_f);
#ifdef ZONE_DEBUG
	cmdadd("zonelog", zlogheap);
#endif
#ifdef HUNK_DEBUG
	cmdadd("hunklog", hunklog);
	cmdadd("hunksmalllog", Hunk_SmallLog);
#endif
}

int
hunkmemremaining(void)
{
	int low, high;

	low = hunk_low.permanent >
	      hunk_low.temp ? hunk_low.permanent : hunk_low.temp;
	high = hunk_high.permanent >
	       hunk_high.temp ? hunk_high.permanent : hunk_high.temp;

	return s_hunkTotal - (low + high);
}

/* The server calls this after the level and game VM have been loaded */
void
hunksetmark(void)
{
	hunk_low.mark = hunk_low.permanent;
	hunk_high.mark = hunk_high.permanent;
}

/* The client calls this before starting a vid_restart or snd_restart */
void
hunkcleartomark(void)
{
	hunk_low.permanent = hunk_low.temp = hunk_low.mark;
	hunk_high.permanent = hunk_high.temp = hunk_high.mark;
}

qbool
hunkcheckmark(void)
{
	if(hunk_low.mark || hunk_high.mark)
		return qtrue;
	return qfalse;
}

/* The server calls this before shutting down or loading a new map */
void
hunkclear(void)
{
	hunk_low.mark = 0;
	hunk_low.permanent = 0;
	hunk_low.temp = 0;
	hunk_low.tempHighwater = 0;

	hunk_high.mark = 0;
	hunk_high.permanent = 0;
	hunk_high.temp = 0;
	hunk_high.tempHighwater = 0;

	hunk_permanent = &hunk_low;
	hunk_temp = &hunk_high;

	comprintf("hunkclear: reset the hunk ok\n");
	vmclear();
#ifdef HUNK_DEBUG
	hunkblocks = nil;
#endif
}

static void
Hunk_SwapBanks(void)
{
	Hunkused *swap;

	/* can't swap banks if there is any temp already allocated */
	if(hunk_temp->temp != hunk_temp->permanent)
		return;

	/* if we have a larger highwater mark on this side, start making
	 * our permanent allocations here and use the other side for temp */
	if(hunk_temp->tempHighwater - hunk_temp->permanent >
	   hunk_permanent->tempHighwater - hunk_permanent->permanent){
		swap = hunk_temp;
		hunk_temp = hunk_permanent;
		hunk_permanent = swap;
	}
}

/* Allocate permanent (until the hunk is cleared) memory */
#ifdef HUNK_DEBUG
void* hunkallocdebug(int size, ha_pref preference, char *label, char *file, int line)
#else
void* hunkalloc(int size, ha_pref preference)
#endif
{
	void *buf;

	if(s_hunkData == nil)
		comerrorf(ERR_FATAL,
			"hunkalloc: Hunk memory system not initialized");
	/* can't do preference if there is any temp allocated */
	if(preference == h_dontcare || hunk_temp->temp != hunk_temp->permanent)
		Hunk_SwapBanks();
	else{
		if(preference == h_low && hunk_permanent != &hunk_low)
			Hunk_SwapBanks();
		else if(preference == h_high && hunk_permanent != &hunk_high)
			Hunk_SwapBanks();
	}
#ifdef HUNK_DEBUG
	size += sizeof(Hunkblk);
#endif
	/* round to cacheline */
	size = (size+31)&~31;
	
	if(hunk_low.temp + hunk_high.temp + size > s_hunkTotal){
#ifdef HUNK_DEBUG
		hunklog();
		Hunk_SmallLog();

		comerrorf(ERR_DROP, "hunkalloc failed on %i: %s, line: %d (%s)"
			, size, file, line, label);
#else
		comerrorf(ERR_DROP, "hunkalloc failed on %i", size);
#endif
	}

	if(hunk_permanent == &hunk_low){
		buf = (void*)(s_hunkData + hunk_permanent->permanent);
		hunk_permanent->permanent += size;
	}else{
		hunk_permanent->permanent += size;
		buf =
			(void*)(s_hunkData + s_hunkTotal -
				hunk_permanent->permanent);
	}

	hunk_permanent->temp = hunk_permanent->permanent;

	Q_Memset(buf, 0, size);

#ifdef HUNK_DEBUG
	{
		Hunkblk *block;

		block = (Hunkblk*)buf;
		block->size = size - sizeof(Hunkblk);
		block->file = file;
		block->label = label;
		block->line = line;
		block->next = hunkblocks;
		hunkblocks = block;
		buf = ((byte*)buf) + sizeof(Hunkblk);
	}
#endif
	return buf;
}

/*
 * This is used by the file loading system.
 * Multiple files can be loaded in temporary memory.
 * When the files-in-use count reaches zero, all temp memory will be deleted
 */
void *
hunkalloctemp(int size)
{
	void *buf;
	Hunkhdr *hdr;

	/* return a zalloc'd block if the hunk has not been initialized
	 * this allows the config and product id files ( journal files too ) to be loaded
	 * by the file system without redunant routines in the file system utilizing different
	 * memory systems */
	if(s_hunkData == nil)
		return zalloc(size);

	Hunk_SwapBanks();

	size = PAD(size, sizeof(intptr_t)) + sizeof(Hunkhdr);

	if(hunk_temp->temp + hunk_permanent->permanent + size > s_hunkTotal)
		comerrorf(ERR_DROP, "hunkalloctemp: failed on %i",
			size);

	if(hunk_temp == &hunk_low){
		buf = (void*)(s_hunkData + hunk_temp->temp);
		hunk_temp->temp += size;
	}else{
		hunk_temp->temp += size;
		buf = (void*)(s_hunkData + s_hunkTotal - hunk_temp->temp);
	}

	if(hunk_temp->temp > hunk_temp->tempHighwater)
		hunk_temp->tempHighwater = hunk_temp->temp;

	hdr = (Hunkhdr*)buf;
	buf = (void*)(hdr+1);

	hdr->magic = Hunkmagic;
	hdr->size = size;

	/* don't bother clearing, because we are going to load a file over it */
	return buf;
}

void
hunkfreetemp(void *buf)
{
	Hunkhdr *hdr;

	/* free with zfree if the hunk has not been initialized
	 * this allows the config and product id files ( journal files too ) to be loaded
	 * by the file system without redunant routines in the file system utilizing different
	 * memory systems */
	if(s_hunkData == nil){
		zfree(buf);
		return;
	}
	hdr = ((Hunkhdr*)buf) - 1;
	if(hdr->magic != Hunkmagic)
		comerrorf(ERR_FATAL, "hunkfreetemp: bad magic");
	hdr->magic = Hunkfreemagic;

	/* this only works if the files are freed in stack order,
	 * otherwise the memory will stay around until hunkcleartemp */
	if(hunk_temp == &hunk_low){
		if(hdr == (void*)(s_hunkData + hunk_temp->temp - hdr->size))
			hunk_temp->temp -= hdr->size;
		else
			comprintf("hunkfreetemp: not the final block\n");
	}else{
		if(hdr == (void*)(s_hunkData + s_hunkTotal - hunk_temp->temp))
			hunk_temp->temp -= hdr->size;
		else
			comprintf("hunkfreetemp: not the final block\n");
	}
}

/*
 * The temp space is no longer needed.  If we have left more
 * touched but unused memory on this side, have future
 * permanent allocs use this side.
 */
void
hunkcleartemp(void)
{
	if(s_hunkData != nil)
		hunk_temp->temp = hunk_temp->permanent;
}

void
hunktrash(void)
{
	int length, i, rnd;
	char *buf, value;

	return;

	if(s_hunkData == nil)
		return;
#ifdef _DEBUG
	comerrorf(ERR_DROP, "hunk trashed");
	return;
#endif
	cvarsetstr("com_jp", "1");
	Hunk_SwapBanks();

	if(hunk_permanent == &hunk_low)
		buf = (void*)(s_hunkData + hunk_permanent->permanent);
	else
		buf =
			(void*)(s_hunkData + s_hunkTotal -
				hunk_permanent->permanent);
	length = hunk_permanent->permanent;

	if(length > 0x7FFFF){
		/* randomly trash data within buf */
		rnd = random() * (length - 0x7FFFF);
		value = 31;
		for(i = 0; i < 0x7FFFF; i++){
			value *= 109;
			buf[rnd+i] ^= value;
		}
	}
}
