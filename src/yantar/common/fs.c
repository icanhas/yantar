/* Handle-based filesystem */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
 /* FIXME: magic numbers everywhere */
 
#include "shared.h"
#include "common.h"
#include "unzip.h"

/*
 * The Virtual Filesystem
 * FIXME: documentation is not code.
 *
 * All of Yantar's data access is through a hierarchical file system, but the contents of
 * the file system can be transparently merged from several sources.
 *
 * A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
 * a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
 * references outside the quake directory system.
 *
 * The "base path" is the path to the directory holding all the game directories and usually
 * the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
 * command line to allow code debugging in a different directory.  Basepath cannot
 * be modified at all after startup.  Any files that are created (demos, screenshots,
 * etc) will be created reletive to the base path, so base path should usually be writable.
 *
 * The "home path" is the path used for all write access. On win32 systems we have "base path"
 * == "home path", but on *nix systems the base installation is usually readonly, and
 * "home path" points to ~/.q3a or similar
 *
 * The user can also install custom mods and content in "home path", so it should be searched
 * along with "home path" and "cd path" for game content.
 *
 *
 * The "base game" is the directory under the paths where data comes from by default, and
 * can be either "baseq3" or "demoq3".
 *
 * The "current game" may be the same as the base game, or it may be the name of another
 * directory under the paths that should be searched for files before looking in the base game.
 * This is the basis for addons.
 *
 * Clients automatically set the game directory after receiving a gamestate from a server,
 * so only servers need to worry about +set fs_game.
 *
 * No other directories outside of the base game and current game will ever be referenced by
 * filesystem functions.
 *
 * To save disk space and speed loading, directory trees can be collapsed into zip files.
 * The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
 * otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
 * zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
 * from the highest number to the lowest, and will always take precedence over the filesystem.
 * This allows a pk3 distributed as a patch to override all existing data.
 *
 * File search order: when fsopenr gets called it will go through the fs_searchpaths
 * structure and stop on the first successful hit. fs_searchpaths is built with successive
 * calls to fsaddgamedir
 *
 * Additionally, we search in several subdirectories:
 * current game is the current mode
 * base game is a variable to allow mods based on other mods
 * (such as baseq3 content combination in a mod for instance)
 * BASEGAME is the hardcoded base game ("baseq3")
 *
 * e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:
 *
 * home path + current game's zip files
 * home path + current game's directory
 * base path + current game's zip files
 * base path + current game's directory
 * cd path + current game's zip files
 * cd path + current game's directory
 *
 * home path + base game's zip file
 * home path + base game's directory
 * base path + base game's zip file
 * base path + base game's directory
 * cd path + base game's zip file
 * cd path + base game's directory
 *
 * home path + BASEGAME's zip file
 * home path + BASEGAME's directory
 * base path + BASEGAME's zip file
 * base path + BASEGAME's directory
 * cd path + BASEGAME's zip file
 * cd path + BASEGAME's directory
 *
 * server download, to be written to home path + current game's directory
 *
 *
 * The filesystem can be safely shutdown and reinitialized with different
 * basedir / cddir / game combinations, but all other subsystems that rely on it
 * (sound, video) must also be forced to restart.
 *
 * Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
 * subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
 * load the file with a request to cache.  Only one file will be kept cached at a time,
 * so any models that are going to be referenced by both subsystems should alternate
 * between the CM_ load function and the ref load function.
 *
 * TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
 * game is currently active.  This allows character models, skins, and sounds to be downloaded
 * to a common directory no matter which game is active.
 *
 * How to prevent downloading zip files?
 * Pass pk3 file names in systeminfo, and download before fsrestart()?
 *
 * Aborting a download disconnects the client from the server.
 *
 * How to mark files as downloadable?  Commercial add-ons won't be downloadable.
 *
 * Non-commercial downloads will want to download the entire zip file.
 * the game would have to be reset to actually read the zip in
 *
 * Auto-update information
 *
 * Path separators
 *
 * Casing
 *
 * separate server gamedir and client gamedir, so if the user starts
 * a local game after having connected to a network game, it won't stick
 * with the network game.
 *
 * todo:
 *
 * downloading (outside fs?)
 * game directory passing and restarting
 */

/* 
 * every time a new demo pk3 file is built, this checksum must be updated.
 * the easiest way to get it is to just run the game and see what it spits out 
 */
#define DEMO_PAK0_CHECKSUM 2985612116u
static const unsigned int	pak_checksums[] = 
{
	1566731103u,
	298122907u,
	412165236u,
	2991495316u,
	1197932710u,
	4087071573u,
	3709064859u,
	908855077u,
	977125798u
};

#define MAX_ZPATH		256
#define MAX_SEARCH_PATHS	4096
#define MAX_FILEHASH_SIZE	1024

typedef struct Pakchild	Pakchild;
typedef struct Pak		Pak;
typedef struct Dir		Dir;
typedef struct Searchpath	Searchpath;
typedef union qfile_gut		qfile_gut;
typedef struct qfile_ut		qfile_ut;
typedef struct Fhandledata	Fhandledata;

/* a file in a pak */
struct Pakchild {
	char			*name;	/* name of the file */
	unsigned long		pos;	/* file info position in zip */
	unsigned long		len;	/* uncompress file size */
	Pakchild		*next;
};

struct Pak {
	char		pakPathname[MAX_OSPATH];	/* c:\quake3\baseq3 */
	char		pakFilename[MAX_OSPATH];	/* c:\quake3\baseq3\pak0.pk3 */
	char		pakBasename[MAX_OSPATH];	/* pak0 */
	char		pakGamename[MAX_OSPATH];	/* baseq3 */
	unzFile		handle;				/* handle to zip file */
	int		checksum;			/* regular checksum */
	int		pure_checksum;			/* checksum for pure */
	int		numfiles;			/* number of files in pk3 */
	int		referenced;			/* referenced file flags */
	int		hashSize;			/* (power of 2) */
	Pakchild	**hashTable;
	Pakchild	* buildBuffer;			/* buffer with the filenames etc. */
};

struct Dir {
	char	path[MAX_OSPATH];	/* c:\quake3 */
	char	fullpath[MAX_OSPATH];	/* c:\quake3\baseq3 */
	char	gamedir[MAX_OSPATH];	/* baseq3 */
};

struct Searchpath {
	Searchpath	*next;
	Pak		*pack;	/* only one of pack / dir will be non NULL */
	Dir	*dir;
};

union qfile_gut {
	FILE	* o;
	unzFile z;
};

struct qfile_ut {
	qfile_gut	file;
	qbool		unique;
};

struct Fhandledata {
	qfile_ut	handleFiles;
	qbool		handleSync;
	int		baseOffset;
	int		fileSize;
	int		zipFilePos;
	qbool		zipFile;
	qbool		streamed;
	char		name[MAX_ZPATH];
};

static char fs_gamedir[MAX_OSPATH];	/* this will be a single file name with no separators */
static Cvar	*fs_debug;
static Cvar	*fs_homepath;

#ifdef MACOS_X
/* Also search the .app bundle for .pk3 files */
static Cvar *fs_apppath;
#endif

static Cvar *fs_basepath;
static Cvar *fs_basegame;
static Cvar	*fs_gamedirvar;
static Searchpath *fs_searchpaths;
static int	fs_readCount;		/* total bytes read */
static int	fs_loadCount;		/* total files read */
static int	fs_loadStack;		/* total files in memory */
static int	fs_packFiles = 0;	/* total number of files in packs */

static int	fs_checksumFeed;

static Fhandledata fsh[MAX_FILE_HANDLES];

/* whether we did a reorder on the current search path when joining the server */
static qbool fs_reordered;

/* never load anything from pk3 files that are not present at the server when pure */
static int	fs_numServerPaks = 0;
static int	fs_serverPaks[MAX_SEARCH_PATHS];	/* checksums */
static char *fs_serverPakNames[MAX_SEARCH_PATHS];	/* pk3 names */

/* 
 * only used for autodownload, to make sure the client has at least
 * all the pk3 files that are referenced at the server side 
 */
static int	fs_numServerReferencedPaks;
static int	fs_serverReferencedPaks[MAX_SEARCH_PATHS];	/* checksums */
static char *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];	/* pk3 names */

/* last valid game folder used */
char	lastValidBase[MAX_OSPATH];
char	lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
FILE *missingFiles = nil;
#endif

/* C99 defines __func__ */
#ifndef __func__
#define __func__ "(unknown)"
#endif

qbool
fsisinitialized(void)
{
	return (fs_searchpaths != NULL);
}

qbool
FS_PakIsPure(Pak *pack)
{
	int i;

	if(fs_numServerPaks){
		for(i = 0; i < fs_numServerPaks; i++)
			/* FIXME: also use hashed file names
			 * NOTE TTimo: a pk3 with same checksum but different name would be validated too
			 *   I don't see this as allowing for any exploit, it would only happen if the client does manips of its file names 'not a bug' */
			if(pack->checksum == fs_serverPaks[i])
				return qtrue;	/* on the aproved list */
		return qfalse;			/* not on the pure server pak list */
	}
	return qtrue;
}

/* return load stack */
int
fsloadstack(void)
{
	return fs_loadStack;
}

/* return a hash value for the filename */
static long
FS_HashFileName(const char *fname, int hashSize)
{
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while(fname[i] != '\0'){
		letter = tolower(fname[i]);
		if(letter =='.') break;			/* don't include extension */
		if(letter =='\\') letter = '/';
		if(letter == PATH_SEP) letter = '/';
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize-1);
	return hash;
}

static Fhandle
FS_HandleForFile(void)
{
	int i;

	for(i = 1; i < MAX_FILE_HANDLES; i++)
		if(fsh[i].handleFiles.file.o == NULL)
			return i;
	comerrorf(ERR_DROP, "FS_HandleForFile: none free");
	return 0;
}

static FILE*
FS_FileForHandle(Fhandle f)
{
	if(f < 1 || f > MAX_FILE_HANDLES)
		comerrorf(ERR_DROP, "FS_FileForHandle: out of range");
	if(fsh[f].zipFile == qtrue)
		comerrorf(ERR_DROP,
			"FS_FileForHandle: can't get FILE on zip file");
	if(!fsh[f].handleFiles.file.o)
		comerrorf(ERR_DROP, "FS_FileForHandle: NULL");

	return fsh[f].handleFiles.file.o;
}

/* forces flush on files we're writing to. */
void
fsforceflush(Fhandle f)
{
	FILE *file;

	file = FS_FileForHandle(f);
	setvbuf(file, NULL, _IONBF, 0);
}

long
FS_fplength(FILE *h)
{
	long	pos;
	long	end;

	pos = ftell(h);
	fseek(h, 0, SEEK_END);
	end = ftell(h);
	fseek(h, pos, SEEK_SET);

	return end;
}

/*
 * If this is called on a non-unique FILE (from a pak file),
 * it will return the size of the pak file, not the expected
 * size of the file.
 */
long
fsfilelen(Fhandle f)
{
	FILE *h;

	h = FS_FileForHandle(f);

	if(h == NULL)
		return -1;
	else
		return FS_fplength(h);
}

/* Fix things up differently for win/unix/mac */
static void
FS_ReplaceSeparators(char *path)
{
	char *s;
	qbool lastCharWasSep = qfalse;

	for(s = path; *s; s++){
		if(*s == '/' || *s == '\\'){
			if(!lastCharWasSep){
				*s = PATH_SEP;
				lastCharWasSep = qtrue;
			}else
				memmove (s, s + 1, strlen (s));
		}else
			lastCharWasSep = qfalse;
	}
}

/* Qpath may have either forward or backwards slashes */
char*
fsbuildospath(const char *base, const char *game, const char *qpath)
{
	char temp[MAX_OSPATH];
	static char	ospath[2][MAX_OSPATH];
	static int	toggle;

	toggle ^= 1;	/* flip-flop to allow two returns without clash */

	if(!game || !game[0])
		game = fs_gamedir;

	Q_sprintf(temp, sizeof(temp), "/%s/%s", game, qpath);
	FS_ReplaceSeparators(temp);
	Q_sprintf(ospath[toggle], sizeof(ospath[0]), "%s%s", base, temp);

	return ospath[toggle];
}

/* Creates any directories needed to store the given filename */
qbool
fscreatepath(char *OSPath)
{
	char	*ofs;
	char	path[MAX_OSPATH];

	/* make absolutely sure that it can't back up the path
	 * FIXME: is c: allowed??? */
	if(strstr(OSPath, "..") || strstr(OSPath, "::")){
		comprintf("WARNING: refusing to create relative path \"%s\"\n",
			OSPath);
		return qtrue;
	}

	Q_strncpyz(path, OSPath, sizeof(path));
	FS_ReplaceSeparators(path);

	/* Skip creation of the root directory as it will always be there */
	ofs = strchr(path, PATH_SEP);
	ofs++;

	for(; ofs != NULL && *ofs; ofs++)
		if(*ofs == PATH_SEP){
			/* create the directory */
			*ofs = 0;
			if(!sysmkdir (path))
				comerrorf(ERR_FATAL,
					"fscreatepath: failed to create path \"%s\"",
					path);
			*ofs = PATH_SEP;
		}

	return qfalse;
}

/* ERR_FATAL if trying to manipulate a file with the platform library extension */
static void
FS_CheckFilenameIsNotExecutable(const char *filename,
				const char *function)
{
	/* Check if the filename ends with the library extension */
	if(Q_cmpext(filename, DLL_EXT))
		comerrorf(
			ERR_FATAL, "%s: Not allowed to manipulate '%s' due "
				   "to %s extension", function, filename,
			DLL_EXT);
}

void
fsremove(const char *osPath)
{
	FS_CheckFilenameIsNotExecutable(osPath, __func__);

	remove(osPath);
}

void
fshomeremove(const char *homePath)
{
	FS_CheckFilenameIsNotExecutable(homePath, __func__);

	remove(fsbuildospath(fs_homepath->string,
			fs_gamedir, homePath));
}

/* Tests if path and file exists */
qbool
FS_FileInPathExists(const char *testpath)
{
	FILE *filep;

	filep = fopen(testpath, "rb");

	if(filep){
		fclose(filep);
		return qtrue;
	}

	return qfalse;
}

/*
 * Tests if the file exists in the current gamedir, this DOES NOT
 * search the paths.  This is to determine if opening a file to write
 * (which always goes into the current gamedir) will cause any overwrites.
 * NOTE TTimo: this goes with fsopenw for opening the file afterwards
 */
qbool
fsfileexists(const char *file)
{
	return FS_FileInPathExists(fsbuildospath(fs_homepath->string,
			fs_gamedir, file));
}

qbool
FS_SV_FileExists(const char *file)
{
	char *testpath;

	testpath = fsbuildospath(fs_homepath->string, file, "");
	testpath[strlen(testpath)-1] = '\0';

	return FS_FileInPathExists(testpath);
}

Fhandle
fssvopenw(const char *filename)
{
	char *ospath;
	Fhandle f;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	ospath = fsbuildospath(fs_homepath->string, filename, "");
	ospath[strlen(ospath)-1] = '\0';

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	if(fs_debug->integer)
		comprintf("fssvopenw: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	if(fscreatepath(ospath))
		return 0;

	comdprintf("writing to: %s\n", ospath);
	fsh[f].handleFiles.file.o = fopen(ospath, "wb");

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o)
		f = 0;
	return f;
}

/*
 * Search for a file somewhere below the home path then base path
 * in that order
 *
 * if uniqueFILE is true, then a new FILE will be fopened even if the file
 * is found in an already open pak file. If uniqueFILE is false, you must call
 * fsclose instead of fclose, otherwise the pak FILE would be improperly closed
 * It is generally safe to always set uniqueFILE to true, because the majority of
 * file IO goes through fsreadfile, which Does The Right Thing already.
 */
long
fssvopenr(const char *filename, Fhandle *fp)
{
	char *ospath;
	Fhandle f = 0;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	/* don't let sound stutter */
	sndclearbuf();

	/* search homepath */
	ospath = fsbuildospath(fs_homepath->string, filename, "");
	/* remove trailing slash */
	ospath[strlen(ospath)-1] = '\0';

	if(fs_debug->integer)
		comprintf("fssvopenr (fs_homepath): %s\n", ospath);

	fsh[f].handleFiles.file.o = fopen(ospath, "rb");
	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o){
		/* If fs_homepath == fs_basepath, don't bother */
		if(Q_stricmp(fs_homepath->string,fs_basepath->string)){
			/* search basepath */
			ospath = fsbuildospath(fs_basepath->string, filename,
				"");
			ospath[strlen(ospath)-1] = '\0';

			if(fs_debug->integer)
				comprintf(
					"fssvopenr (fs_basepath): %s\n",
					ospath);

			fsh[f].handleFiles.file.o = fopen(ospath, "rb");
			fsh[f].handleSync = qfalse;
		}

		if(!fsh[f].handleFiles.file.o)
			f = 0;
	}

	*fp = f;
	if(f)
		return fsfilelen(f);

	return -1;
}

void
fssvrename(const char *from, const char *to)
{
	char *from_ospath, *to_ospath;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	/* don't let sound stutter */
	sndclearbuf();

	from_ospath = fsbuildospath(fs_homepath->string, from, "");
	to_ospath = fsbuildospath(fs_homepath->string, to, "");
	from_ospath[strlen(from_ospath)-1] = '\0';
	to_ospath[strlen(to_ospath)-1] = '\0';

	if(fs_debug->integer)
		comprintf("fssvrename: %s --> %s\n", from_ospath, to_ospath);

	FS_CheckFilenameIsNotExecutable(to_ospath, __func__);

	rename(from_ospath, to_ospath);
}

void
fsrename(const char *from, const char *to)
{
	char *from_ospath, *to_ospath;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	/* don't let sound stutter */
	sndclearbuf();

	from_ospath = fsbuildospath(fs_homepath->string, fs_gamedir, from);
	to_ospath = fsbuildospath(fs_homepath->string, fs_gamedir, to);

	if(fs_debug->integer)
		comprintf("fsrename: %s --> %s\n", from_ospath, to_ospath);

	FS_CheckFilenameIsNotExecutable(to_ospath, __func__);

	rename(from_ospath, to_ospath);
}

/*
 * If the FILE pointer is an open pak file, leave it open.
 *
 * For some reason, other dll's can't just call fclose()
 * on files returned by fsopen...
 */
void
fsclose(Fhandle f)
{
	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(fsh[f].zipFile == qtrue){
		unzCloseCurrentFile(fsh[f].handleFiles.file.z);
		if(fsh[f].handleFiles.unique)
			unzClose(fsh[f].handleFiles.file.z);
		Q_Memset(&fsh[f], 0, sizeof(fsh[f]));
		return;
	}

	/* we didn't find it as a pak, so close it as a unique file */
	if(fsh[f].handleFiles.file.o)
		fclose (fsh[f].handleFiles.file.o);
	Q_Memset(&fsh[f], 0, sizeof(fsh[f]));
}

Fhandle
fsopenw(const char *filename)
{
	char *ospath;
	Fhandle f;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = fsbuildospath(fs_homepath->string, fs_gamedir, filename);

	if(fs_debug->integer)
		comprintf("fsopenw: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	if(fscreatepath(ospath))
		return 0;

	/* enabling the following line causes a recursive function call loop
	 * when running with +set logfile 1 +set developer 1
	 * comdprintf( "writing to: %s\n", ospath ); */
	fsh[f].handleFiles.file.o = fopen(ospath, "wb");

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o)
		f = 0;
	return f;
}

Fhandle
fsopena(const char *filename)
{
	char *ospath;
	Fhandle f;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	/* don't let sound stutter */
	sndclearbuf();

	ospath = fsbuildospath(fs_homepath->string, fs_gamedir, filename);

	if(fs_debug->integer)
		comprintf("fsopena: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	if(fscreatepath(ospath))
		return 0;

	fsh[f].handleFiles.file.o = fopen(ospath, "ab");
	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o)
		f = 0;
	return f;
}

Fhandle
fscreatepipefile(const char *filename)
{
	char	*ospath;
	FILE	*fifo;
	Fhandle f;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	/* don't let sound stutter */
	sndclearbuf();

	ospath = fsbuildospath(fs_homepath->string, fs_gamedir, filename);

	if(fs_debug->integer)
		comprintf("fscreatepipefile: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	fifo = sysmkfifo(ospath);
	if(fifo){
		fsh[f].handleFiles.file.o = fifo;
		fsh[f].handleSync = qfalse;
	}else{
		comprintf(
			S_COLOR_YELLOW
			"WARNING: Could not create new com_pipefile at %s. "
			"com_pipefile will not be used.\n",
			ospath);
		f = 0;
	}

	return f;
}

/* Ignore case and seprator char distinctions */
qbool
fscomparefname(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
		if(c2 >= 'a' && c2 <= 'z')
			c2 -= ('a' - 'A');

		if(c1 == '\\' || c1 == ':')
			c1 = '/';
		if(c2 == '\\' || c2 == ':')
			c2 = '/';

		if(c1 != c2)
			return qtrue;	/* strings not equal */
	} while(c1);

	return qfalse;	/* strings are equal */
}

/* Return qtrue if ext matches file extension filename */
qbool
FS_IsExt(const char *filename, const char *ext, int namelen)
{
	int extlen;

	extlen = strlen(ext);

	if(extlen > namelen)
		return qfalse;

	filename += namelen - extlen;

	return !Q_stricmp(filename, ext);
}

qbool
FS_IsDemoExt(const char *filename, int namelen)
{
	char	*ext_test;
	int	index, protocol;

	ext_test = strrchr(filename, '.');
	if(ext_test &&
	   !Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1)){
		protocol = atoi(ext_test + ARRAY_LEN(DEMOEXT));

		if(protocol == com_protocol->integer)
			return qtrue;


		for(index = 0; demo_protocols[index]; index++)
			if(demo_protocols[index] == protocol)
				return qtrue;
	}

	return qfalse;
}

/*
 * Tries opening file "filename" in searchpath "search"
 * Returns filesize and an open FILE pointer.
 */
extern qbool com_fullyInitialized;

long
fsopenrDir(const char *filename, Searchpath *search,
		    Fhandle *file, qbool uniqueFILE,
		    qbool unpure)
{
	long hash;
	Pak *pak;
	Pakchild	*pakFile;
	Dir	*dir;
	char	*netpath;
	FILE    *filep;
	int	len;

	if(filename == NULL)
		comerrorf(ERR_FATAL,
			"fsopenr: NULL 'filename' parameter passed");

	/* qpaths are not supposed to have a leading slash */
	if(filename[0] == '/' || filename[0] == '\\')
		filename++;

	/* 
	 * Make absolutely sure that it can't back up the path.
	 * The searchpaths do guarantee that something will always
	 * be prepended, so we don't need to worry about "c:" or "//limbo" 
	 */
	if(strstr(filename, "..") || strstr(filename, "::")){
		if(file == NULL)
			return qfalse;

		*file = 0;
		return -1;
	}

	/* make sure the q3key file is only readable by the quake3.exe at initialization
	 * any other time the key should only be accessed in memory using the provided functions */
	if(com_fullyInitialized && strstr(filename, "q3key")){
		if(file == NULL)
			return qfalse;

		*file = 0;
		return -1;
	}

	if(file == NULL){
		/* just wants to see if file is there */

		/* is the element a pak file? */
		if(search->pack){
			hash = FS_HashFileName(filename, search->pack->hashSize);

			if(search->pack->hashTable[hash]){
				/* look through all the pak file elements */
				pak = search->pack;
				pakFile = pak->hashTable[hash];

				do {
					/* case and separator insensitive comparisons */
					if(!fscomparefname(pakFile->name,
						   filename)){
						/* found it! */
						if(pakFile->len)
							return pakFile->len;
						else
							/* It's not nice, but legacy code depends
							 * on positive value if file exists no matter
							 * what size */
							return 1;
					}

					pakFile = pakFile->next;
				} while(pakFile != NULL);
			}
		}else if(search->dir){
			dir = search->dir;

			netpath = fsbuildospath(dir->path, dir->gamedir,
				filename);
			filep = fopen (netpath, "rb");

			if(filep){
				len = FS_fplength(filep);
				fclose(filep);

				if(len)
					return len;
				else
					return 1;
			}
		}

		return 0;
	}

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	/* is the element a pak file? */
	if(search->pack){
		hash = FS_HashFileName(filename, search->pack->hashSize);

		if(search->pack->hashTable[hash]){
			/* disregard if it doesn't match one of the allowed pure pak files */
			if(!unpure && !FS_PakIsPure(search->pack)){
				*file = 0;
				return -1;
			}

			/* look through all the pak file elements */
			pak = search->pack;
			pakFile = pak->hashTable[hash];

			do {
				/* case and separator insensitive comparisons */
				if(!fscomparefname(pakFile->name,
					   filename)){
					/* found it! */

					/* 
					 * mark the pak as having been referenced and mark specifics on cgame and ui
					 * shaders, txt, arena files  by themselves do not count as a reference as
					 * these are loaded from all pk3s
					 * from every pk3 file.. 
					 */
					len = strlen(filename);

					if(!(pak->referenced & FS_GENERAL_REF)){
						if(!FS_IsExt(filename, ".shader", len)
						   && !FS_IsExt(filename, ".txt", len)
						   && !FS_IsExt(filename, ".cfg", len)
						   && !FS_IsExt(filename, ".config", len)
						   && !FS_IsExt(filename, ".bot", len)
						   && !FS_IsExt(filename, ".arena", len)
						   && !FS_IsExt(filename, ".menu", len)
						   && Q_stricmp(filename,  "game.qvm") != 0
						   && !strstr(filename, "levelshots"))
						then{
							pak->referenced |= FS_GENERAL_REF;
						}
					}

					if(strstr(filename, "cgame.qvm"))
						pak->referenced |= FS_CGAME_REF;
					if(strstr(filename, "ui.qvm"))
						pak->referenced |= FS_UI_REF;

					if(uniqueFILE){
						/* open a new file on the pakfile */
						fsh[*file].handleFiles.file.z =
							unzOpen(pak->pakFilename);

						if(fsh[*file].handleFiles.file.z
						   == NULL)
							comerrorf(
								ERR_FATAL,
								"Couldn't open %s",
								pak->pakFilename);
					}else
						fsh[*file].handleFiles.file.z =
							pak->handle;

					Q_strncpyz(fsh[*file].name, filename,
						sizeof(fsh[*file].name));
					fsh[*file].zipFile = qtrue;

					/* set the file position in the zip file (also sets the current file info) */
					unzSetOffset(
						fsh[*file].handleFiles.file.z,
						pakFile->pos);

					/* open the file in the zip */
					unzOpenCurrentFile(
						fsh[*file].handleFiles.file.z);
					fsh[*file].zipFilePos = pakFile->pos;

					if(fs_debug->integer)
						comprintf(
							"fsopenr: %s (found in '%s')\n",
							filename,
							pak->pakFilename);

					return pakFile->len;
				}

				pakFile = pakFile->next;
			} while(pakFile != NULL);
		}
	}else if(search->dir){
		/* check a file in the directory tree */

		/* 
		 * if we are running restricted, the only files we
		 * will allow to come from the directory are .cfg files 
		 */
		len = strlen(filename);
		/* FIXME TTimo I'm not sure about the fs_numServerPaks test
		 * if you are using fsreadfile to find out if a file exists,
		 *   this test can make the search fail although the file is in the directory
		 * I had the problem on https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=8
		 * turned out I used fsfileexists instead 
		 */
		if(!unpure && fs_numServerPaks)
			if(!FS_IsExt(filename, ".cfg", len) &&	/* for config files */
			   !FS_IsExt(filename, ".menu", len) &&	/* menu files */
			   !FS_IsExt(filename, ".game", len) &&	/* menu files */
			   !FS_IsExt(filename, ".dat", len) &&	/* for journal files */
			   !FS_IsDemoExt(filename, len))	/* demos */
			then{
				*file = 0;
				return -1;
			}

		dir = search->dir;

		netpath = fsbuildospath(dir->path, dir->gamedir, filename);
		filep	= fopen(netpath, "rb");

		if(filep == NULL){
			*file = 0;
			return -1;
		}

		Q_strncpyz(fsh[*file].name, filename, sizeof(fsh[*file].name));
		fsh[*file].zipFile = qfalse;

		if(fs_debug->integer)
			comprintf("fsopenr: %s (found in '%s/%s')\n",
				filename,
				dir->path,
				dir->gamedir);

		fsh[*file].handleFiles.file.o = filep;
		return FS_fplength(filep);
	}

	return -1;
}

/*
 * Finds the file in the search path.
 * Returns filesize and an open FILE pointer.
 * Used for streaming data out of either a
 * separate file or a ZIP file.
 */
long
fsopenr(const char *filename, Fhandle *file, qbool uniqueFILE)
{
	Searchpath *search;
	long len;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	for(search = fs_searchpaths; search; search = search->next){
		len = fsopenrDir(filename, search, file, uniqueFILE,
			qfalse);

		if(file == NULL){
			if(len > 0)
				return len;
		}else if(len >= 0 && *file)
			return len;

	}

#ifdef FS_MISSING
	if(missingFiles)
		fprintf(missingFiles, "%s\n", filename);
#endif

	if(file){
		*file = 0;
		return -1;
	}
	return 0;	/* File doesn't exist. */
}

/*
 * Find a suitable VM file in search path order.
 *
 * In each searchpath try:
 * - open DLL file if DLL loading enabled
 * - open QVM file
 *
 * Enable search for DLL by setting enableDll to FSVM_ENABLEDLL
 *
 * write found DLL or QVM to "found" and return VMnative if DLL, VMcompiled if QVM
 * Return the searchpath in "startSearch".
 */
Vmmode
fsfindvm(void **startSearch, char *found, int foundlen, const char *name,
	  int enableDll)
{
	Searchpath *search, *lastSearch;
	Dir	*dir;
	Pak		*pack;
	char dllName[MAX_OSPATH], qvmName[MAX_OSPATH];
	char		*netpath;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(enableDll)
		Q_sprintf(dllName, sizeof(dllName), "%s-" ARCH_STRING DLL_EXT,
			name);

	Q_sprintf(qvmName, sizeof(qvmName), Pvmfiles "/%s.qvm", name);

	lastSearch = *startSearch;
	if(*startSearch == NULL)
		search = fs_searchpaths;
	else
		search = lastSearch->next;

	while(search){
		if(search->dir && !fs_numServerPaks){
			dir = search->dir;

			if(enableDll){
				netpath = fsbuildospath(dir->path, dir->gamedir,
					dllName);

				if(FS_FileInPathExists(netpath)){
					Q_strncpyz(found, netpath, foundlen);
					*startSearch = search;

					return VMnative;
				}
			}

			if(fsopenrDir(qvmName, search, NULL, qfalse,
				   qfalse) > 0){
				*startSearch = search;
				return VMcompiled;
			}
		}else if(search->pack){
			pack = search->pack;

			if(lastSearch && lastSearch->pack)
				/* make sure we only try loading one VM file per game dir
				 * i.e. if VM from pak7.pk3 fails we won't try one from pak6.pk3 */

				if(!fscomparefname(lastSearch->pack->
					   pakPathname, pack->pakPathname)){
					search = search->next;
					continue;
				}

			if(fsopenrDir(qvmName, search, NULL, qfalse,
				   qfalse) > 0){
				*startSearch = search;

				return VMcompiled;
			}
		}

		search = search->next;
	}

	return -1;
}

/* Properly handles partial reads */
int
fsread2(void *buffer, int len, Fhandle f)
{
	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!f)
		return 0;
	if(fsh[f].streamed){
		int r;
		fsh[f].streamed = qfalse;
		r = fsread(buffer, len, f);
		fsh[f].streamed = qtrue;
		return r;
	}else
		return fsread(buffer, len, f);
}

int
fsread(void *buffer, int len, Fhandle f)
{
	int	block, remaining;
	int	read;
	byte *buf;
	int	tries;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!f)
		return 0;

	buf = (byte*)buffer;
	fs_readCount += len;

	if(fsh[f].zipFile == qfalse){
		remaining = len;
		tries = 0;
		while(remaining){
			block	= remaining;
			read	= fread (buf, 1, block,
				fsh[f].handleFiles.file.o);
			if(read == 0){
				/* 
				 * we might have been trying to read from a CD, which
				 * sometimes returns a 0 read on windows 
				 */
				if(!tries)
					tries = 1;
				else
					return len-remaining;	/* comerrorf (ERR_FATAL, "fsread: 0 bytes read"); */
			}

			if(read == -1)
				comerrorf (ERR_FATAL, "fsread: -1 bytes read");

			remaining -= read;
			buf += read;
		}
		return len;
	}else
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
}

/* Properly handles partial writes */
int
fswrite(const void *buffer, int len, Fhandle h)
{
	int	block, remaining;
	int	written;
	byte *buf;
	int	tries;
	FILE *f;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!h)
		return 0;

	f = FS_FileForHandle(h);
	buf = (byte*)buffer;

	remaining = len;
	tries = 0;
	while(remaining){
		block	= remaining;
		written = fwrite (buf, 1, block, f);
		if(written == 0){
			if(!tries)
				tries = 1;
			else{
				comprintf("fswrite: 0 bytes written\n");
				return 0;
			}
		}

		if(written == -1){
			comprintf("fswrite: -1 bytes written\n");
			return 0;
		}

		remaining -= written;
		buf += written;
	}
	if(fsh[h].handleSync)
		fflush(f);
	return len;
}

void QDECL
fsprintf(Fhandle h, const char *fmt, ...)
{
	va_list argptr;
	char	msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	fswrite(msg, strlen(msg), h);
}

#define PK3_SEEK_BUFFER_SIZE 65536

/* seek on a file (doesn't work for zip files! */
int
fsseek(Fhandle f, long offset, int origin)
{
	int _origin;

	if(!fs_searchpaths){
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");
		return -1;
	}

	if(fsh[f].streamed){
		fsh[f].streamed = qfalse;
		fsseek(f, offset, origin);
		fsh[f].streamed = qtrue;
	}

	if(fsh[f].zipFile == qtrue){
		/* FIXME: this is incomplete and really, really
		 * crappy (but better than what was here before) */
		byte	buffer[PK3_SEEK_BUFFER_SIZE];
		int	remainder = offset;

		if(offset < 0 || origin == FS_SEEK_END){
			comerrorf(
				ERR_FATAL,
				"Negative offsets and FS_SEEK_END not implemented "
				"for fsseek on pk3 file contents");
			return -1;
		}

		switch(origin){
		case FS_SEEK_SET:
			unzSetOffset(fsh[f].handleFiles.file.z,
				fsh[f].zipFilePos);
			unzOpenCurrentFile(fsh[f].handleFiles.file.z);
		/* fallthrough */

		case FS_SEEK_CUR:
			while(remainder > PK3_SEEK_BUFFER_SIZE){
				fsread(buffer, PK3_SEEK_BUFFER_SIZE, f);
				remainder -= PK3_SEEK_BUFFER_SIZE;
			}
			fsread(buffer, remainder, f);
			return offset;
			break;

		default:
			comerrorf(ERR_FATAL, "Bad origin in fsseek");
			return -1;
			break;
		}
	}else{
		FILE *file;
		file = FS_FileForHandle(f);
		switch(origin){
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			comerrorf(ERR_FATAL, "Bad origin in fsseek");
			break;
		}

		return fseek(file, offset, _origin);
	}
}

/*
 * Convenience functions for entire files
 */

int
fsfileisinpak(const char *filename, int *pChecksum)
{
	Searchpath *search;
	Pak	*pak;
	Pakchild    *pakFile;
	long	hash = 0;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!filename)
		comerrorf(ERR_FATAL,
			"fsopenr: NULL 'filename' parameter passed");

	/* qpaths are not supposed to have a leading slash */
	if(filename[0] == '/' || filename[0] == '\\')
		filename++;

	/* make absolutely sure that it can't back up the path.
	 * The searchpaths do guarantee that something will always
	 * be prepended, so we don't need to worry about "c:" or "//limbo" */
	if(strstr(filename, "..") || strstr(filename, "::"))
		return -1;

	/*
	 * search through the path, one element at a time
	 *  */

	for(search = fs_searchpaths; search; search = search->next){
		if(search->pack)
			hash = FS_HashFileName(filename, search->pack->hashSize);
		/* is the element a pak file? */
		if(search->pack && search->pack->hashTable[hash]){
			/* disregard if it doesn't match one of the allowed pure pak files */
			if(!FS_PakIsPure(search->pack))
				continue;

			/* look through all the pak file elements */
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				/* case and separator insensitive comparisons */
				if(!fscomparefname(pakFile->name,
					   filename)){
					if(pChecksum)
						*pChecksum = pak->pure_checksum;
					return 1;
				}
				pakFile = pakFile->next;
			} while(pakFile != NULL);
		}
	}
	return -1;
}

/*
 * Filename are relative to the quake search path
 * a null buffer will just return the file length without loading
 * If searchPath is non-NULL search only in that specific search path
 *
 * returns the length of the file
 * a null buffer will just return the file length without loading
 * as a quick check for existance. -1 length == not present
 * A 0 byte will always be appended at the end, so string ops are safe.
 * the buffer should be considered read-only, because it may be cached
 * for other uses.
 */
long
fsreadfiledir(const char *qpath, void *searchPath, qbool unpure,
	       void **buffer)
{
	Fhandle	h;
	Searchpath	*search;
	byte	* buf;
	qbool isConfig;
	long	len;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!qpath || !qpath[0])
		comerrorf(ERR_FATAL, "fsreadfile with empty name");

	buf = NULL;	/* quiet compiler warning */

	/* if this is a .cfg file and we are playing back a journal, read
	 * it from the journal file */
	if(strstr(qpath, ".cfg")){
		isConfig = qtrue;
		if(com_journal && com_journal->integer == 2){
			int r;

			comdprintf("Loading %s from journal file.\n", qpath);
			r = fsread(&len, sizeof(len), com_journalDataFile);
			if(r != sizeof(len)){
				if(buffer != NULL) *buffer = NULL;
				return -1;
			}
			/* if the file didn't exist when the journal was created */
			if(!len){
				if(buffer == NULL)
					return 1;	/* hack for old journal files */
				*buffer = NULL;
				return -1;
			}
			if(buffer == NULL)
				return len;

			buf = hunkalloctemp(len+1);
			*buffer = buf;

			r = fsread(buf, len, com_journalDataFile);
			if(r != len)
				comerrorf(ERR_FATAL,
					"Read from journalDataFile failed");

			fs_loadCount++;
			fs_loadStack++;

			/* guarantee that it will have a trailing 0 for string operations */
			buf[len] = 0;

			return len;
		}
	}else
		isConfig = qfalse;

	search = searchPath;

	if(search == NULL)
		/* look for it in the filesystem or pack files */
		len = fsopenr(qpath, &h, qfalse);
	else
		/* look for it in a specific search path only */
		len = fsopenrDir(qpath, search, &h, qfalse, unpure);

	if(h == 0){
		if(buffer)
			*buffer = NULL;
		/* if we are journalling and it is a config file, write a zero to the journal file */
		if(isConfig && com_journal && com_journal->integer == 1){
			comdprintf("Writing zero for %s to journal file.\n",
				qpath);
			len = 0;
			fswrite(&len, sizeof(len), com_journalDataFile);
			fsflush(com_journalDataFile);
		}
		return -1;
	}

	if(!buffer){
		if(isConfig && com_journal && com_journal->integer == 1){
			comdprintf("Writing len for %s to journal file.\n",
				qpath);
			fswrite(&len, sizeof(len), com_journalDataFile);
			fsflush(com_journalDataFile);
		}
		fsclose(h);
		return len;
	}

	fs_loadCount++;
	fs_loadStack++;

	buf = hunkalloctemp(len+1);
	*buffer = buf;

	fsread (buf, len, h);

	/* guarantee that it will have a trailing 0 for string operations */
	buf[len] = 0;
	fsclose(h);

	/* if we are journalling and it is a config file, write it to the journal file */
	if(isConfig && com_journal && com_journal->integer == 1){
		comdprintf("Writing %s to journal file.\n", qpath);
		fswrite(&len, sizeof(len), com_journalDataFile);
		fswrite(buf, len, com_journalDataFile);
		fsflush(com_journalDataFile);
	}
	return len;
}

/*
 * Filename are relative to the quake search path
 * a null buffer will just return the file length without loading
 */
long
fsreadfile(const char *qpath, void **buffer)
{
	return fsreadfiledir(qpath, NULL, qfalse, buffer);
}

/* frees the memory returned by fsreadfile */
void
fsfreefile(void *buffer)
{
	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");
	if(!buffer)
		comerrorf(ERR_FATAL, "fsfreefile( NULL )");
	fs_loadStack--;

	hunkfreetemp(buffer);

	/* if all of our temp files are free, clear all of our space */
	if(fs_loadStack == 0)
		hunkcleartemp();
}

/* writes a complete file, creating any subdirectories needed */
/* Filenames are relative to the quake search path */
void
fswritefile(const char *qpath, const void *buffer, int size)
{
	Fhandle f;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!qpath || !buffer)
		comerrorf(ERR_FATAL, "fswritefile: NULL parameter");

	f = fsopenw(qpath);
	if(!f){
		comprintf("Failed to open %s\n", qpath);
		return;
	}

	fswrite(buffer, size, f);

	fsclose(f);
}


/*
 * pk3 file loading
 */

/*
 * Creates a new Pak in the search chain for the contents
 * of a zip file.
 */
static Pak *
FS_LoadZipFile(const char *zipfile, const char *basename)
{
	Pakchild *buildBuffer;
	Pak	*pack;
	unzFile uf;
	int	err;
	unz_global_info gi;
	char	filename_inzip[MAX_ZPATH];
	unz_file_info file_info;
	int	i, len;
	long	hash;
	int	fs_numHeaderLongs;
	int *fs_headerLongs;
	char *namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo (uf,&gi);

	if(err != UNZ_OK)
		return NULL;

	len = 0;
	unzGoToFirstFile(uf);
	for(i = 0; i < gi.number_entry; i++){
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip,
				sizeof(filename_inzip), NULL, 0, NULL,
				0);
		if(err != UNZ_OK)
			break;
		len += strlen(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = zalloc((gi.number_entry * sizeof(Pakchild)) + len);
	namePtr = ((char*)buildBuffer) + gi.number_entry * sizeof(Pakchild);
	fs_headerLongs = zalloc((gi.number_entry + 1) * sizeof(int));
	fs_headerLongs[ fs_numHeaderLongs++ ] = LittleLong(fs_checksumFeed);

	/* 
	 * get the hash table size from the number of files in the zip
	 * because lots of custom pk3 files have less than 32 or 64 files 
	 */
	for(i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1)
		if(i > gi.number_entry)
			break;

	pack = zalloc(sizeof(Pak) + i * sizeof(Pakchild *));
	pack->hashSize	= i;
	pack->hashTable = (Pakchild**)(((char*)pack) + sizeof(Pak));
	for(i = 0; i < pack->hashSize; i++)
		pack->hashTable[i] = NULL;

	Q_strncpyz(pack->pakFilename, zipfile, sizeof(pack->pakFilename));
	Q_strncpyz(pack->pakBasename, basename, sizeof(pack->pakBasename));

	/* strip .pk3 if needed */
	if(strlen(pack->pakBasename) > 4 &&
	   !Q_stricmp(pack->pakBasename + strlen(pack->pakBasename) - 4, ".pk3"))
		pack->pakBasename[strlen(pack->pakBasename) - 4] = 0;

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for(i = 0; i < gi.number_entry; i++){
		err =
			unzGetCurrentFileInfo(uf, &file_info, filename_inzip,
				sizeof(filename_inzip), NULL, 0, NULL,
				0);
		if(err != UNZ_OK)
			break;
		if(file_info.uncompressed_size > 0)
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(
				file_info.crc);
		Q_strlwr(filename_inzip);
		hash = FS_HashFileName(filename_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		strcpy(buildBuffer[i].name, filename_inzip);
		namePtr += strlen(filename_inzip) + 1;
		/* store the file position in the zip */
		buildBuffer[i].pos = unzGetOffset(uf);
		buildBuffer[i].len = file_info.uncompressed_size;
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = blockchecksum(
			&fs_headerLongs[1], sizeof(*fs_headerLongs) *
			(fs_numHeaderLongs - 1));
	pack->pure_checksum =
		blockchecksum(fs_headerLongs,
			sizeof(*fs_headerLongs) * fs_numHeaderLongs);
	pack->checksum = LittleLong(pack->checksum);
	pack->pure_checksum = LittleLong(pack->pure_checksum);

	zfree(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}

/* Frees a pak structure and releases all associated resources */
static void
FS_FreePak(Pak *thepak)
{
	unzClose(thepak->handle);
	zfree(thepak->buildBuffer);
	zfree(thepak);
}

/* Compares whether the given pak file matches a referenced checksum */
qbool
fscompzipchecksum(const char *zipfile)
{
	Pak	*thepak;
	int	index, checksum;

	thepak = FS_LoadZipFile(zipfile, "");

	if(!thepak)
		return qfalse;

	checksum = thepak->checksum;
	FS_FreePak(thepak);

	for(index = 0; index < fs_numServerReferencedPaks; index++)
		if(checksum == fs_serverReferencedPaks[index])
			return qtrue;

	return qfalse;
}


/*
 * Directory scanning routines
 */

#define MAX_FOUND_FILES 0x1000

static int
FS_ReturnPath(const char *zname, char *zpath, int *depth)
{
	int len, at, newdep;

	newdep = 0;
	zpath[0] = 0;
	len = 0;
	at = 0;

	while(zname[at] != 0){
		if(zname[at]=='/' || zname[at]=='\\'){
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy(zpath, zname);
	zpath[len] = 0;
	*depth = newdep;
	return len;
}

static int
FS_AddFileToList(char *name, char *list[MAX_FOUND_FILES], int nfiles)
{
	int i;

	if(nfiles == MAX_FOUND_FILES - 1)
		return nfiles;
	for(i = 0; i < nfiles; i++)
		if(!Q_stricmp(name, list[i]))
			return nfiles;	/* allready in list */
	list[nfiles] = copystr(name);
	nfiles++;

	return nfiles;
}

/*
 * Returns a uniqued list of files that match the given criteria
 * from all search paths
 */
char**
FS_ListFilteredFiles(const char *path, const char *extension, char *filter,
		     int *numfiles,
		     qbool allowNonPureFilesOnDisk)
{
	int nfiles;
	char **listCopy;
	char	*list[MAX_FOUND_FILES];
	Searchpath *search;
	int	i;
	int	pathLength;
	int	extensionLength;
	int	length, pathDepth, temp;
	Pak	*pak;
	Pakchild    *buildBuffer;
	char	zpath[MAX_ZPATH];

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!path){
		*numfiles = 0;
		return NULL;
	}
	if(!extension)
		extension = "";

	pathLength = strlen(path);
	if(path[pathLength-1] == '\\' || path[pathLength-1] == '/')
		pathLength--;
	extensionLength = strlen(extension);
	nfiles = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	/*
	 * search through the path, one element at a time, adding to list
	 */
	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(search->pack){
			if(!FS_PakIsPure(search->pack)){
				/* 
				 * If we are pure, don't search for files on paks that
				 * aren't on the pure list 
				 */
				continue;
			}

			/* look through all the pak file elements */
			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for(i = 0; i < pak->numfiles; i++){
				char *name;
				int zpathLen, depth;

				/* check for directory match */
				name = buildBuffer[i].name;
				if(filter){
					/* case insensitive */
					if(!filterpath(filter, name, qfalse))
						continue;
					/* unique the match */
					nfiles = FS_AddFileToList(name, list,
						nfiles);
				}else{

					zpathLen = FS_ReturnPath(name, zpath,
						&depth);

					if((depth-pathDepth)>2 || pathLength >
					   zpathLen ||
					   Q_stricmpn(name, path, pathLength))
						continue;

					/* check for extension match */
					length = strlen(name);
					if(length < extensionLength)
						continue;

					if(Q_stricmp(name + length -
						   extensionLength, extension))
						continue;
					/* unique the match */

					temp = pathLength;
					if(pathLength)
						temp++;		/* include the '/' */
					nfiles = FS_AddFileToList(name + temp,
							list,
							nfiles);
				}
			}
		}else if(search->dir){	/* scan for files in the filesystem */
			char	*netpath;
			int	numSysFiles;
			char **sysFiles;
			char	*name;

			/* don't scan directories for files if we are pure or restricted */
			if(fs_numServerPaks && !allowNonPureFilesOnDisk)
				continue;
			else{
				netpath = fsbuildospath(
					search->dir->path, search->dir->gamedir,
					path);
				sysFiles =
					syslistfiles(netpath, extension, filter,
						&numSysFiles,
						qfalse);
				for(i = 0; i < numSysFiles; i++){
					/* unique the match */
					name = sysFiles[i];
					nfiles = FS_AddFileToList(name, list,
						nfiles);
				}
				sysfreefilelist(sysFiles);
			}
		}
	}

	/* return a copy of the list */
	*numfiles = nfiles;

	if(!nfiles)
		return NULL;

	listCopy = zalloc((nfiles + 1) * sizeof(*listCopy));
	for(i = 0; i < nfiles; i++)
		listCopy[i] = list[i];
	listCopy[i] = NULL;

	return listCopy;
}

/* 
 * directory should not have either a leading or trailing /
 * if extension is "/", only subdirectories will be returned
 * the returned files will not include any directories or / 
 */
char**
fslistfiles(const char *path, const char *extension, int *numfiles)
{
	return FS_ListFilteredFiles(path, extension, NULL, numfiles, qfalse);
}

void
fsfreefilelist(char **list)
{
	int i;

	if(!fs_searchpaths)
		comerrorf(ERR_FATAL,
			"Filesystem call made without initialization");
	if(!list)
		return;
	for(i = 0; list[i]; i++)
		zfree(list[i]);
	zfree(list);
}

int
fsgetfilelist(const char *path, const char *extension, char *listbuf,
	       int bufsize)
{
	int nFiles, i, nTotal, nLen;
	char **pFiles = NULL;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if(Q_stricmp(path, "$modlist") == 0)
		return fsgetmodlist(listbuf, bufsize);

	pFiles = fslistfiles(path, extension, &nFiles);

	for(i =0; i < nFiles; i++){
		nLen = strlen(pFiles[i]) + 1;
		if(nTotal + nLen + 1 < bufsize){
			strcpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal += nLen;
		}else{
			nFiles = i;
			break;
		}
	}
	fsfreefilelist(pFiles);
	return nFiles;
}

/*
 * Naive implementation. Concatenates three lists into a
 * new list, and frees the old lists from the heap.
 *
 * FIXME TTimo those two should move to common.c next to syslistfiles
 */
static unsigned int
Sys_CountFileList(char **list)
{
	int i = 0;

	if(list)
		while(*list){
			list++;
			i++;
		}
	return i;
}

static char**
Sys_ConcatenateFileLists(char **list0, char **list1)
{
	int totalLength = 0;
	char ** cat = NULL, **dst, **src;

	totalLength += Sys_CountFileList(list0);
	totalLength += Sys_CountFileList(list1);

	/* Create new list. */
	dst = cat = zalloc((totalLength + 1) * sizeof(char*));

	/* Copy over lists. */
	if(list0)
		for(src = list0; *src; src++, dst++)
			*dst = *src;
	if(list1)
		for(src = list1; *src; src++, dst++)
			*dst = *src;

	/* Terminate the list */
	*dst = NULL;

	/* Free our old lists.
	 * NOTE: not freeing their content, it's been merged in dst and still being used */
	if(list0) zfree(list0);
	if(list1) zfree(list1);

	return cat;
}

/*
 * Returns a list of mod directory names
 * A mod directory is a peer to baseq3 with a pk3 in it
 * The directories are searched in base path, cd path and home path
 */
int
fsgetmodlist(char *listbuf, int bufsize)
{
	int nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
	char **pFiles = NULL;
	char	**pPaks = NULL;
	char    *name, *path;
	char	descPath[MAX_OSPATH];
	Fhandle descHandle;

	int dummy;
	char **pFiles0	= NULL;
	char **pFiles1	= NULL;
	qbool bDrop	= qfalse;

	*listbuf = 0;
	nMods = nTotal = 0;

	pFiles0 = syslistfiles(fs_homepath->string, NULL, NULL, &dummy, qtrue);
	pFiles1 = syslistfiles(fs_basepath->string, NULL, NULL, &dummy, qtrue);
	/* we searched for mods in the three paths
	 * it is likely that we have duplicate names now, which we will cleanup below */
	pFiles = Sys_ConcatenateFileLists(pFiles0, pFiles1);
	nPotential = Sys_CountFileList(pFiles);

	for(i = 0; i < nPotential; i++){
		name = pFiles[i];
		/* NOTE: cleaner would involve more changes
		 * ignore duplicate mod directories */
		if(i!=0){
			bDrop = qfalse;
			for(j=0; j<i; j++)
				if(Q_stricmp(pFiles[j],name)==0){
					/* this one can be dropped */
					bDrop = qtrue;
					break;
				}
		}
		if(bDrop)
			continue;
		/* we drop "baseq3" "." and ".." */
		if(Q_stricmp(name,
			   com_basegame->string) && Q_stricmpn(name, ".", 1)){
			/* now we need to find some .pk3 files to validate the mod
			 * NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			 * we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			 *   so it could be in base path, cd path or home path
			 *   we will try each three of them here (yes, it's a bit messy) */
			path = fsbuildospath(fs_basepath->string, name, "");
			nPaks	= 0;
			pPaks	= syslistfiles(path, ".pk3", NULL, &nPaks,
				qfalse);
			sysfreefilelist(pPaks);	/* we only use syslistfiles to check wether .pk3 files are present */

			/* try on home path */
			if(nPaks <= 0){
				path =
					fsbuildospath(fs_homepath->string, name,
						"");
				nPaks	= 0;
				pPaks	=
					syslistfiles(path, ".pk3", NULL, &nPaks,
						qfalse);
				sysfreefilelist(pPaks);
			}

			if(nPaks > 0){
				nLen = strlen(name) + 1;
				/* nLen is the length of the mod path
				 * we need to see if there is a description available */
				descPath[0] = '\0';
				strcpy(descPath, name);
				strcat(descPath, "/description.txt");
				nDescLen = fssvopenr(descPath,
					&descHandle);
				if(nDescLen > 0 && descHandle){
					FILE *file;
					file = FS_FileForHandle(descHandle);
					Q_Memset(descPath, 0, sizeof(descPath));
					nDescLen = fread(descPath, 1, 48, file);
					if(nDescLen >= 0)
						descPath[nDescLen] = '\0';
					fsclose(descHandle);
				}else
					strcpy(descPath, name);
				nDescLen = strlen(descPath) + 1;

				if(nTotal + nLen + 1 + nDescLen + 1 < bufsize){
					strcpy(listbuf, name);
					listbuf += nLen;
					strcpy(listbuf, descPath);
					listbuf += nDescLen;
					nTotal	+= nLen + nDescLen;
					nMods++;
				}else
					break;
			}
		}
	}
	sysfreefilelist(pFiles);

	return nMods;
}

void
FS_Dir_f(void)
{
	char *path;
	char	*extension;
	char **dirnames;
	int	ndirs;
	int	i;

	if(cmdargc() < 2 || cmdargc() > 3){
		comprintf("usage: dir <directory> [extension]\n");
		return;
	}

	if(cmdargc() == 2){
		path = cmdargv(1);
		extension = "";
	}else{
		path = cmdargv(1);
		extension = cmdargv(2);
	}

	comprintf("Directory of %s %s\n", path, extension);
	comprintf("---------------\n");

	dirnames = fslistfiles(path, extension, &ndirs);

	for(i = 0; i < ndirs; i++)
		comprintf("%s\n", dirnames[i]);
	fsfreefilelist(dirnames);
}

void
FS_ConvertPath(char *s)
{
	while(*s){
		if(*s == '\\' || *s == ':')
			*s = '/';
		s++;
	}
}

/* Ignore case and seprator char distinctions */
int
FS_PathCmp(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
		if(c2 >= 'a' && c2 <= 'z')
			c2 -= ('a' - 'A');

		if(c1 == '\\' || c1 == ':')
			c1 = '/';
		if(c2 == '\\' || c2 == ':')
			c2 = '/';

		if(c1 < c2)
			return -1;	/* strings not equal */
		if(c1 > c2)
			return 1;
	} while(c1);

	return 0;	/* strings are equal */
}

void
FS_SortFileList(char **filelist, int numfiles)
{
	int i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist = zalloc((numfiles + 1) * sizeof(*sortedlist));
	sortedlist[0]	= NULL;
	numsortedfiles	= 0;
	for(i = 0; i < numfiles; i++){
		for(j = 0; j < numsortedfiles; j++)
			if(FS_PathCmp(filelist[i], sortedlist[j]) < 0)
				break;
		for(k = numsortedfiles; k > j; k--)
			sortedlist[k] = sortedlist[k-1];
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Q_Memcpy(filelist, sortedlist, numfiles * sizeof(*filelist));
	zfree(sortedlist);
}

void
FS_NewDir_f(void)
{
	char	*filter;
	char **dirnames;
	int	ndirs;
	int	i;

	if(cmdargc() < 2){
		comprintf("usage: fdir <filter>\n");
		comprintf("example: fdir *q3dm*.bsp\n");
		return;
	}

	filter = cmdargv(1);

	comprintf("---------------\n");

	dirnames = FS_ListFilteredFiles("", "", filter, &ndirs, qfalse);

	FS_SortFileList(dirnames, ndirs);

	for(i = 0; i < ndirs; i++){
		FS_ConvertPath(dirnames[i]);
		comprintf("%s\n", dirnames[i]);
	}
	comprintf("%d files listed\n", ndirs);
	fsfreefilelist(dirnames);
}

void
FS_Path_f(void)
{
	Searchpath *s;
	int i;

	comprintf ("Current search path:\n");
	for(s = fs_searchpaths; s; s = s->next){
		if(s->pack){
			comprintf ("%s (%i files)\n", s->pack->pakFilename,
				s->pack->numfiles);
			if(fs_numServerPaks){
				if(!FS_PakIsPure(s->pack))
					comprintf("    not on the pure list\n");
				else
					comprintf("    on the pure list\n");
			}
		}else
			comprintf ("%s%c%s\n", s->dir->path, PATH_SEP,
				s->dir->gamedir);
	}


	comprintf("\n");
	for(i = 1; i < MAX_FILE_HANDLES; i++)
		if(fsh[i].handleFiles.file.o)
			comprintf("handle %i: %s\n", i, fsh[i].name);
}

void
FS_TouchFile_f(void)
{
	Fhandle f;

	if(cmdargc() != 2){
		comprintf("Usage: touchFile <file>\n");
		return;
	}

	fsopenr(cmdargv(1), &f, qfalse);
	if(f)
		fsclose(f);
}

qbool
fswhich(const char *filename, void *searchPath)
{
	Searchpath *search = searchPath;

	if(fsopenrDir(filename, search, NULL, qfalse, qfalse) > 0){
		if(search->pack){
			comprintf("File \"%s\" found in \"%s\"\n", filename,
				search->pack->pakFilename);
			return qtrue;
		}else if(search->dir){
			comprintf("File \"%s\" found at \"%s\"\n", filename,
				search->dir->fullpath);
			return qtrue;
		}
	}

	return qfalse;
}

void
fswhich_f(void)
{
	Searchpath *search;
	char *filename;

	filename = cmdargv(1);

	if(!filename[0]){
		comprintf("Usage: which <file>\n");
		return;
	}

	/* qpaths are not supposed to have a leading slash */
	if(filename[0] == '/' || filename[0] == '\\')
		filename++;

	/* just wants to see if file is there */
	for(search = fs_searchpaths; search; search = search->next)
		if(fswhich(filename, search))
			return;

	comprintf("File not found: \"%s\"\n", filename);
	return;
}


static int QDECL
paksort(const void *a, const void *b)
{
	char *aa, *bb;

	aa = *(char**)a;
	bb = *(char**)b;

	return FS_PathCmp(aa, bb);
}

/*
 * Sets fs_gamedir, adds the directory to the head of the
 * path, then loads the zip headers
 */
void
fsaddgamedir(const char *path, const char *dir)
{
	int i;	/* index into pakfiles */
	int j;	/* index into pakdirs */
	int nfiles, ndirs;
	char **pakfiles, **pakdirs;
	char *pakfile, curpath[MAX_OSPATH + 1];
	enum {Pakfile, Pakdir} paktype;
	Pak *pak;
	Searchpath *sp;

	for(sp = fs_searchpaths; sp != nil; sp = sp->next)
		if(sp->dir && Q_stricmp(sp->dir->gamedir, dir)
		&& Q_stricmp(sp->dir->path, path))
		then{
			return;	/* ignore; we've already got this one */
		}

	Q_strncpyz(fs_gamedir, dir, sizeof(fs_gamedir));
	/*
	 * find all pakfiles in this directory, and get _all_ dirs to later
	 * filter down to pakdirs
	 */
	Q_strncpyz(curpath, fsbuildospath(path, dir, ""), sizeof(curpath));
	curpath[strlen(curpath) - 1] = '\0';	/* strip trailing slash */
	pakfiles = syslistfiles(curpath, ".pk3", NULL, &nfiles, qfalse);
	qsort(pakfiles, nfiles, sizeof(char *), paksort);
	
	if(fs_numServerPaks){
		ndirs = 0;
		pakdirs = nil;
	}else{
		/* 
		 * Get top level directories (we'll filter them later
		 * since the syslistfiles filtering is terrible).
		 */
		pakdirs = syslistfiles(curpath, "/", NULL, &ndirs, qfalse);
		qsort(pakdirs, ndirs, sizeof(char *), paksort);
	}

	for(i = 0, j = 0; ((i + j) < (nfiles + ndirs)); ){
		if(i >= nfiles)
			paktype = Pakdir;	/* out of files; must be a pakdir */
		else if(j >= ndirs)
			paktype = Pakfile;	/* vice versa */
		else{
			/* could be either; let's see... */
			if(paksort(&pakfiles[i], &pakdirs[j]) < 0)
				paktype = Pakdir;
			else
				paktype = Pakfile;
		}

		/* add pakfile or add pakdir */
		if(paktype == Pakfile){
			pakfile = fsbuildospath(path, dir, pakfiles[i]);
			if((pak = FS_LoadZipFile(pakfile, pakfiles[i])) == 0)
				continue;
			Q_strncpyz(pak->pakPathname, curpath,
				sizeof(pak->pakPathname));
			/* store the game name for downloading */
			Q_strncpyz(pak->pakGamename, dir,
				sizeof(pak->pakGamename));

			fs_packFiles += pak->numfiles;

			sp = zalloc(sizeof(*sp));
			sp->pack = pak;
			sp->next = fs_searchpaths;
			fs_searchpaths = sp;

			++i;
		}else{
			int len;
			Dir *p;

			len = strlen(pakdirs[j]);
			/* filter out anything that doesn't end with .dir3 */
			if(!FS_IsExt(pakdirs[j], ".dir3", len)){
				++j;
				continue;
			}
			pakfile = fsbuildospath(path, dir, pakdirs[j]);
			/* FIXME: increase fs_packFiles */
			/* add to the search path */
			sp = zalloc(sizeof(*sp));
			sp->dir = zalloc(sizeof(*sp->dir));
			p = sp->dir;
			Q_strncpyz(p->path, curpath, sizeof(p->path));
			Q_strncpyz(p->fullpath, pakfile, sizeof(p->fullpath));
			Q_strncpyz(p->gamedir, pakdirs[j], sizeof(p->gamedir));

			++j;
		}
	}
	sysfreefilelist(pakfiles);
	sysfreefilelist(pakdirs);

	/* add the directory to the search path */
	sp = zalloc(sizeof(*sp));
	sp->dir = zalloc(sizeof(*sp->dir));
	Q_strncpyz(sp->dir->path, path, sizeof(sp->dir->path));
	Q_strncpyz(sp->dir->fullpath, curpath, sizeof(sp->dir->fullpath));
	Q_strncpyz(sp->dir->gamedir, dir, sizeof(sp->dir->gamedir));
	sp->next = fs_searchpaths;
	fs_searchpaths = sp;
}

qbool
fsispak(char *pak, char *base, int numPaks)
{
	int i;

	for(i = 0; i < NUM_ID_PAKS; i++)
		if(!fscomparefname(pak, va("%s/pak%d", base, i)))
			break;
	if(i < numPaks)
		return qtrue;
	return qfalse;
}

/*
 * Check whether the string contains stuff like "../" to prevent directory traversal bugs
 * and return qtrue if it does.
 */

qbool
fscheckdirtraversal(const char *checkdir)
{
	if(strstr(checkdir, "../") || strstr(checkdir, "..\\"))
		return qtrue;

	return qfalse;
}

/*
 * ----------------
 * dlstring == qtrue
 *
 * Returns a list of pak files that we should download from the server. They all get stored
 * in the current gamedir and an fsrestart will be fired up after we download them all.
 *
 * The string is the format:
 *
 * @remotename@localname [repeat]
 *
 * static int		fs_numServerReferencedPaks;
 * static int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];
 * static char		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];
 *
 * ----------------
 * dlstring == qfalse
 *
 * we are not interested in a download string format, we want something human-readable
 * (this is used for diagnostics while connecting to a pure server)
 *
 */
qbool
fscomparepaks(char *neededpaks, int len, qbool dlstring)
{
	Searchpath *sp;
	qbool havepak;
	char *origpos = neededpaks;
	int i;

	if(!fs_numServerReferencedPaks)
		return qfalse;	/* Server didn't send any pack information along */

	*neededpaks = 0;

	for(i = 0; i < fs_numServerReferencedPaks; i++){
		/* Ok, see if we have this pak file */
		havepak = qfalse;

		/* never autodownload any of the id paks */
		if(fsispak(fs_serverReferencedPakNames[i], BASEGAME,
			   NUM_ID_PAKS))
			continue;

		/* Make sure the server cannot make us write to non-quake3 directories. */
		if(fscheckdirtraversal(fs_serverReferencedPakNames[i])){
			comprintf("WARNING: Invalid download name %s\n",
				fs_serverReferencedPakNames[i]);
			continue;
		}

		for(sp = fs_searchpaths; sp; sp = sp->next)
			if(sp->pack && sp->pack->checksum ==
			   fs_serverReferencedPaks[i]){
				havepak = qtrue;	/* This is it! */
				break;
			}

		if(!havepak && fs_serverReferencedPakNames[i] &&
		   *fs_serverReferencedPakNames[i]){
			/* Don't got it */

			if(dlstring){
				/* We need this to make sure we won't hit the end of the buffer or the server could
				 * overwrite non-pk3 files on clients by writing so much crap into neededpaks that
				 * Q_strcat cuts off the .pk3 extension. */

				origpos += strlen(origpos);

				/* Remote name */
				Q_strcat(neededpaks, len, "@");
				Q_strcat(neededpaks, len,
					fs_serverReferencedPakNames[i]);
				Q_strcat(neededpaks, len, ".pk3");

				/* Local name */
				Q_strcat(neededpaks, len, "@");
				/* Do we have one with the same name? */
				if(FS_SV_FileExists(va("%s.pk3",
						   fs_serverReferencedPakNames[
							   i]))){
					char st[MAX_ZPATH];
					/* We already have one called this, we need to download it to another name
					 * Make something up with the checksum in it */
					Q_sprintf(
						st, sizeof(st), "%s.%08x.pk3",
						fs_serverReferencedPakNames[i],
						fs_serverReferencedPaks[i]);
					Q_strcat(neededpaks, len, st);
				}else{
					Q_strcat(neededpaks, len,
						fs_serverReferencedPakNames[i]);
					Q_strcat(neededpaks, len, ".pk3");
				}

				/* Find out whether it might have overflowed the buffer and don't add this file to the
				 * list if that is the case. */
				if(strlen(origpos) + (origpos - neededpaks) >=
				   len - 1){
					*origpos = '\0';
					break;
				}
			}else{
				Q_strcat(neededpaks, len,
					fs_serverReferencedPakNames[i]);
				Q_strcat(neededpaks, len, ".pk3");
				/* Do we have one with the same name? */
				if(FS_SV_FileExists(va("%s.pk3",
						   fs_serverReferencedPakNames[
							   i])))
					Q_strcat(
						neededpaks, len,
						" (local file exists with wrong checksum)");
				Q_strcat(neededpaks, len, "\n");
			}
		}
	}
	if(*neededpaks)
		return qtrue;
	return qfalse;	/* We have them all */
}

/* frees all resources */
void
fsshutdown(qbool closemfp)
{
	Searchpath *p, *next;
	int i;

	for(i = 0; i < MAX_FILE_HANDLES; i++)
		if(fsh[i].fileSize)
			fsclose(i);

	/* free everything */
	for(p = fs_searchpaths; p; p = next){
		next = p->next;

		if(p->pack)
			FS_FreePak(p->pack);
		if(p->dir)
			zfree(p->dir);

		zfree(p);
	}

	/* any FS_ calls will now be an error until reinitialized */
	fs_searchpaths = NULL;

	cmdremove("path");
	cmdremove("dir");
	cmdremove("fdir");
	cmdremove("touchFile");
	cmdremove("which");

#ifdef FS_MISSING
	if(closemfp)
		fclose(missingFiles);

#endif
}

/*
 * NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
 * this can lead to misleading situations, see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
 */
static void
FS_ReorderPurePaks(void)
{
	Searchpath *s;
	int i;
	Searchpath **p_insert_index,	/* for linked list reordering */
	**p_previous;			/* when doing the scan */

	fs_reordered = qfalse;

	/* only relevant when connected to pure server */
	if(!fs_numServerPaks)
		return;

	p_insert_index = &fs_searchpaths;	/* we insert in order at the beginning of the list */
	for(i = 0; i < fs_numServerPaks; i++){
		p_previous = p_insert_index;	/* track the pointer-to-current-item */
		for(s = *p_insert_index; s; s = s->next){
			/* the part of the list before p_insert_index has been sorted already */
			if(s->pack && fs_serverPaks[i] == s->pack->checksum){
				fs_reordered = qtrue;
				/* move this element to the insert list */
				*p_previous = s->next;
				s->next = *p_insert_index;
				*p_insert_index = s;
				/* increment insert list */
				p_insert_index = &s->next;
				break;	/* iterate to next server pack */
			}
			p_previous = &s->next;
		}
	}
}

static void
FS_Startup(const char *gameName)
{
	const char *homePath;

	comprintf("----- FS_Startup -----\n");
	fs_packFiles = 0;
	fs_debug = cvarget("fs_debug", "0", 0);
	fs_basepath = cvarget("fs_basepath",
		sysgetdefaultinstallpath(), CVAR_INIT|CVAR_PROTECTED);
	fs_basegame = cvarget("fs_basegame", "", CVAR_INIT);
#ifdef _WIN32
	/*
	 * Don't use home on windows. It would be cleaner to allow the user to
	 * decide whether to use the home dir via an archived cvar such as
	 * "fs_usehome"; but, of course, the filesystem isn't initialized at this
	 * point, so we cannot get cvars from the config.
	 */
	homePath = fs_basepath->string;
#else
	homePath = sysgetdefaulthomepath();
	if(!homePath || !homePath[0])
		homePath = fs_basepath->string;		/* fallback */

#endif
	fs_homepath = cvarget("fs_homepath", homePath,
		CVAR_INIT|CVAR_PROTECTED);
	fs_gamedirvar = cvarget("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO);

	/* add search path elements in reverse priority order */
	if(fs_basepath->string[0])
		fsaddgamedir(fs_basepath->string, gameName);
	/* fs_homepath is somewhat particular to *nix systems, only add if relevant */

	#ifdef MACOS_X
	fs_apppath = cvarget("fs_apppath", sysgetdefaultapppath(),
		CVAR_INIT|CVAR_PROTECTED);
	/* Make MacOSX also include the base path included with the .app bundle */
	if(fs_apppath->string[0])
		fsaddgamedir(fs_apppath->string, gameName);
	#endif

	/* NOTE: same filtering below for mods and basegame */
	if(fs_homepath->string[0] && Q_stricmp(fs_homepath->string,
		   fs_basepath->string)){
		fscreatepath(fs_homepath->string);
		fsaddgamedir(fs_homepath->string, gameName);
	}

	/* check for additional base game so mods can be based upon other mods */
	if(fs_basegame->string[0] && Q_stricmp(fs_basegame->string, gameName)){
		if(fs_basepath->string[0])
			fsaddgamedir(fs_basepath->string,
				fs_basegame->string);
		if(fs_homepath->string[0] &&
		   Q_stricmp(fs_homepath->string,fs_basepath->string))
			fsaddgamedir(fs_homepath->string,
				fs_basegame->string);
	}

	/* check for additional game folder for mods */
	if(fs_gamedirvar->string[0] &&
	   Q_stricmp(fs_gamedirvar->string, gameName)){
		if(fs_basepath->string[0])
			fsaddgamedir(fs_basepath->string,
				fs_gamedirvar->string);
		if(fs_homepath->string[0] &&
		   Q_stricmp(fs_homepath->string,fs_basepath->string))
			fsaddgamedir(fs_homepath->string,
				fs_gamedirvar->string);
	}

	/* add our commands */
	cmdadd("path", FS_Path_f);
	cmdadd("dir", FS_Dir_f);
	cmdadd("fdir", FS_NewDir_f);
	cmdadd("touchFile", FS_TouchFile_f);
	cmdadd("which", fswhich_f);

	/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=506 */
	/* reorder the pure pk3 files according to server order */
	FS_ReorderPurePaks();

	/* print the current search paths */
	FS_Path_f();

	fs_gamedirvar->modified = qfalse;	/* We just loaded, it's not modified */

	comprintf("----------------------\n");

#ifdef FS_MISSING
	if(missingFiles == NULL)
		missingFiles = fopen("\\missing.txt", "ab");
#endif
	comprintf("%d files in pk3 files\n", fs_packFiles);
}

#ifndef STANDALONE
/*
 * Check whether any of the original id pak files is present,
 * and start up in standalone mode, if there are none and a
 * different com_basegame was set.
 * Note: If you're building a game that doesn't depend on the
 * Q3 media pak0.pk3, you'll want to remove this by defining
 * STANDALONE in q_shared.h
 */
static void
FS_CheckPak0(void)
{
	Searchpath *path;
	Pak *curpack;
	qbool founddemo = qfalse;
	unsigned int foundPak = 0;

	for(path = fs_searchpaths; path; path = path->next){
		const char * pakBasename = path->pack->pakBasename;

		if(!path->pack)
			continue;

		curpack = path->pack;

		if(!Q_stricmpn(curpack->pakGamename, "demoq3", MAX_OSPATH)
		   && !Q_stricmpn(pakBasename, "pak0", MAX_OSPATH)){
			if(curpack->checksum == DEMO_PAK0_CHECKSUM)
				founddemo = qtrue;
		}else if(!Q_stricmpn(curpack->pakGamename, BASEGAME, MAX_OSPATH)
			 && strlen(pakBasename) == 4 &&
			 !Q_stricmpn(pakBasename, "pak", 3)
			 && pakBasename[3] >= '0' && pakBasename[3] <= '0' +
			 NUM_ID_PAKS - 1){
			if(curpack->checksum !=
			   pak_checksums[pakBasename[3]-'0']){
				if(pakBasename[3] == '0')
					comprintf(
						"\n\n"
						"**************************************************\n"
						"WARNING: " BASEGAME
						"/pak0.pk3 is present but its checksum (%u)\n"
						"is not correct. Please re-copy pak0.pk3 from your\n"
						"legitimate Q3 CDROM.\n"
						"**************************************************\n\n\n",
						curpack->checksum);
				else
					comprintf(
						"\n\n"
						"**************************************************\n"
						"WARNING: " BASEGAME
						"/pak%d.pk3 is present but its checksum (%u)\n"
						"is not correct. Please re-install the point release\n"
						"**************************************************\n\n\n",
						pakBasename[3]-'0',
						curpack->checksum);
			}

			foundPak |= 1<<(pakBasename[3]-'0');
		}else{
			int index;

			/* Finally check whether this pak's checksum is listed because the user tried
			 * to trick us by renaming the file, and set foundPak's highest bit to indicate this case. */

			for(index = 0; index < ARRAY_LEN(pak_checksums); index++){
				if(curpack->checksum == pak_checksums[index]){
					comprintf(
						"\n\n"
						"**************************************************\n"
						"WARNING: %s is renamed pak file %s%cpak%d.pk3\n"
						"Running in standalone mode won't work\n"
						"Please rename, or remove this file\n"
						"**************************************************\n\n\n",
						curpack->pakFilename, BASEGAME,
						PATH_SEP, index);


					foundPak |= 0x80000000;
				}
			}
		}
	}

	if(!foundPak && Q_stricmp(com_basegame->string, BASEGAME))
		cvarsetstr("com_standalone", "1");
	else
		cvarsetstr("com_standalone", "0");

	if(!com_standalone->integer){
		if(!(foundPak & 0x01))
			if(founddemo){
				comprintf(
					"\n\n"
					"**************************************************\n"
					"WARNING: It looks like you're using pak0.pk3\n"
					"from the demo. This may work fine, but it is not\n"
					"guaranteed or supported.\n"
					"**************************************************\n\n\n");

				foundPak |= 0x01;
			}
	}


	if(!com_standalone->integer && (foundPak & 0x1ff) != 0x1ff){
		char errorText[MAX_STRING_CHARS] = "";

		if((foundPak & 0x01) != 0x01)
			Q_strcat(errorText, sizeof(errorText),
				"\"pak0.pk3\" is missing. Please copy it "
				"from your legitimate Q3 CDROM. ");

		if((foundPak & 0x1fe) != 0x1fe)
			Q_strcat(errorText, sizeof(errorText),
				"Point Release files are missing. Please "
				"re-install the 1.32 point release. ");

		Q_strcat(errorText, sizeof(errorText),
			va("Also check that your ioq3 executable is in "
			   "the correct place and that every file "
			   "in the \"%s\" directory is present and readable",
				BASEGAME));

		comerrorf(ERR_FATAL, "%s", errorText);
	}
}
#endif

/*
 * Returns a space separated string containing the checksums of all loaded pk3 files.
 * Servers with sv_pure set will get this string and pass it to clients.
 */
const char*
fsloadedpakchecksums(void)
{
	static char info[BIG_INFO_STRING];
	Searchpath *search;

	info[0] = 0;

	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(!search->pack)
			continue;

		Q_strcat(info, sizeof(info), va("%i ", search->pack->checksum));
	}

	return info;
}

/*
 * Returns a space separated string containing the names of all loaded pk3 files.
 * Servers with sv_pure set will get this string and pass it to clients.
 */
const char *
fsloadedpaknames(void)
{
	static char info[BIG_INFO_STRING];
	Searchpath *search;

	info[0] = 0;

	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(!search->pack)
			continue;

		if(*info)
			Q_strcat(info, sizeof(info), " ");
		Q_strcat(info, sizeof(info), search->pack->pakBasename);
	}

	return info;
}

/*
 * Returns a space separated string containing the pure checksums of all loaded pk3 files.
 * Servers with sv_pure use these checksums to compare with the checksums the clients send
 * back to the server.
 */
const char *
fsloadedpakpurechecksums(void)
{
	static char info[BIG_INFO_STRING];
	Searchpath *search;

	info[0] = 0;

	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(!search->pack)
			continue;

		Q_strcat(info, sizeof(info),
			va("%i ", search->pack->pure_checksum));
	}

	return info;
}

/*
 * Returns a space separated string containing the checksums of all referenced pk3 files.
 * The server will send this to the clients so they can check which files should be auto-downloaded.
 */
const char*
fsreferencedpakchecksums(void)
{
	static char info[BIG_INFO_STRING];
	Searchpath *search;

	info[0] = 0;
	for(search = fs_searchpaths; search; search = search->next)
		/* is the element a pak file? */
		if(search->pack)
			if(search->pack->referenced ||
			   Q_stricmpn(search->pack->pakGamename,
				   com_basegame->string,
				   strlen(com_basegame->string)))
				Q_strcat(info, sizeof(info),
					va("%i ", search->pack->checksum));
	return info;
}

/*
 * Returns a space separated string containing the pure checksums of all referenced pk3 files.
 * Servers with sv_pure set will get this string back from clients for pure validation
 *
 * The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
 */
const char*
fsreferencedpakpurechecksums(void)
{
	static char info[BIG_INFO_STRING];
	Searchpath *search;
	int nFlags, numPaks, checksum;

	info[0] = 0;

	checksum = fs_checksumFeed;
	numPaks = 0;
	for(nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1){
		if(nFlags & FS_GENERAL_REF){
			/* add a delimter between must haves and general refs
			 * Q_strcat(info, sizeof(info), "@ "); */
			info[strlen(info)+1] = '\0';
			info[strlen(info)+2] = '\0';
			info[strlen(info)] = '@';
			info[strlen(info)] = ' ';
		}
		for(search = fs_searchpaths; search; search = search->next)
			/* is the element a pak file and has it been referenced based on flag? */
			if(search->pack &&
			   (search->pack->referenced & nFlags)){
				Q_strcat(info, sizeof(info),
					va("%i ", search->pack->pure_checksum));
				if(nFlags & (FS_CGAME_REF | FS_UI_REF))
					break;
				checksum ^= search->pack->pure_checksum;
				numPaks++;
			}
	}
	/* last checksum is the encoded number of referenced pk3s */
	checksum ^= numPaks;
	Q_strcat(info, sizeof(info), va("%i ", checksum));

	return info;
}

/*
 * Returns a space separated string containing the names of all referenced pk3 files.
 * The server will send this to the clients so they can check which files should be auto-downloaded.
 */
const char *
fsreferencedpaknames(void)
{
	static char info[BIG_INFO_STRING];
	Searchpath *search;

	info[0] = 0;

	/* we want to return ALL pk3's from the fs_game path
	 * and referenced one's from baseq3 */
	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(search->pack)
			if(search->pack->referenced ||
			   Q_stricmpn(search->pack->pakGamename,
			   com_basegame->string,
			   strlen(com_basegame->string)))
			then{
				if(*info)
					Q_strcat(info, sizeof(info), " ");
				Q_strcat(info, sizeof(info),
					search->pack->pakGamename);
				Q_strcat(info, sizeof(info), "/");
				Q_strcat(info, sizeof(info),
					search->pack->pakBasename);
			}
	}

	return info;
}

/* clears referenced booleans on loaded pk3s */
void
fsclearpakrefs(int flags)
{
	Searchpath *search;

	if(!flags)
		flags = -1;
	for(search = fs_searchpaths; search; search = search->next)
		/* is the element a pak file and has it been referenced? */
		if(search->pack)
			search->pack->referenced &= ~flags;
}

/*
 * If the string is empty, all data sources will be allowed.
 * If not empty, only pk3 files that match one of the space
 * separated checksums will be checked for files, with the
 * exception of .cfg and .dat files.
 */
void
fspureservsetloadedpaks(const char *pakSums, const char *pakNames)
{
	int i, c, d;

	cmdstrtok(pakSums);

	c = cmdargc();
	if(c > MAX_SEARCH_PATHS)
		c = MAX_SEARCH_PATHS;

	fs_numServerPaks = c;

	for(i = 0; i < c; i++)
		fs_serverPaks[i] = atoi(cmdargv(i));

	if(fs_numServerPaks)
		comdprintf("Connected to a pure server.\n");
	else if(fs_reordered){
		/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
		 * force a restart to make sure the search order will be correct */
		comdprintf("FS search reorder is required\n");
		fsrestart(fs_checksumFeed);
		return;
	}

	for(i = 0; i < c; i++){
		if(fs_serverPakNames[i])
			zfree(fs_serverPakNames[i]);
		fs_serverPakNames[i] = NULL;
	}
	if(pakNames && *pakNames){
		cmdstrtok(pakNames);

		d = cmdargc();
		if(d > MAX_SEARCH_PATHS)
			d = MAX_SEARCH_PATHS;

		for(i = 0; i < d; i++)
			fs_serverPakNames[i] = copystr(cmdargv(i));
	}
}

/*
 * The checksums and names of the pk3 files referenced at the server
 * are sent to the client and stored here. The client will use these
 * checksums to see if any pk3 files need to be auto-downloaded.
 */
void
fspureservsetreferencedpaks(const char *pakSums, const char *pakNames)
{
	int i, c, d = 0;

	cmdstrtok(pakSums);

	c = cmdargc();
	if(c > MAX_SEARCH_PATHS)
		c = MAX_SEARCH_PATHS;

	for(i = 0; i < c; i++)
		fs_serverReferencedPaks[i] = atoi(cmdargv(i));

	for(i = 0; i < ARRAY_LEN(fs_serverReferencedPakNames); i++){
		if(fs_serverReferencedPakNames[i])
			zfree(fs_serverReferencedPakNames[i]);

		fs_serverReferencedPakNames[i] = NULL;
	}

	if(pakNames && *pakNames){
		cmdstrtok(pakNames);
		d = cmdargc();
		if(d > c)
			d = c;
		for(i = 0; i < d; i++)
			fs_serverReferencedPakNames[i] = copystr(cmdargv(i));
	}
	/* ensure that there are as many checksums as there are pak names. */
	if(d < c)
		c = d;
	fs_numServerReferencedPaks = c;
}

/*
 * Called only at inital startup, not when the filesystem
 * is resetting due to a game change
 */
void
fsinit(void)
{
	/* allow command line parms to override our defaults
	 * we have to specially handle this, because normal command
	 * line variable sets don't happen until after the filesystem
	 * has already been initialized */
	comstartupvar("fs_basepath");
	comstartupvar("fs_homepath");
	comstartupvar("fs_game");

	if(!fscomparefname(cvargetstr("fs_game"),
		   com_basegame->string))
		cvarsetstr("fs_game", "");

	/* try to start up normally */
	FS_Startup(com_basegame->string);

#ifndef STANDALONE
	FS_CheckPak0( );
#endif

	/* if we can't find default.cfg, assume that the paths are
	 * busted and error out now, rather than getting an unreadable
	 * graphics screen when the font fails to load */
	if(fsreadfile("default.cfg", NULL) <= 0)
		comerrorf(ERR_FATAL, "Couldn't load default.cfg");

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
}

/* shutdown and restart the filesystem so changes to fs_gamedir can take effect */
void
fsrestart(int checksumFeed)
{
	fsshutdown(qfalse);	/* free anything we currently have loaded */
	fs_checksumFeed = checksumFeed;
	fsclearpakrefs(0);
	FS_Startup(com_basegame->string);	/* try to start up normally */
#ifndef STANDALONE
	FS_CheckPak0( );
#endif
	/* 
	 * if we can't find default.cfg, assume that the paths are
	 * busted and error out now, rather than getting an unreadable
	 * graphics screen when the font fails to load 
	 */
	if(fsreadfile("default.cfg", NULL) <= 0){
		/* 
		 * this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		 * (for instance a TA demo server) 
		 */
		if(lastValidBase[0]){
			fspureservsetloadedpaks("", "");
			cvarsetstr("fs_basepath", lastValidBase);
			cvarsetstr("fs_gamedirvar", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			fsrestart(checksumFeed);
			comerrorf(ERR_DROP, "Invalid game folder");
			return;
		}
		comerrorf(ERR_FATAL, "Couldn't load default.cfg");
	}
	if(Q_stricmp(fs_gamedirvar->string, lastValidGame))
		/* skip the user.cfg if "safe" is on the command line */
		if(!cominsafemode())
			cbufaddstr ("exec " Q3CONFIG_CFG "\n");
	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
}

/*
 * Restart if necessary
 * Return qtrue if restarting due to game directory changed, qfalse otherwise
 */
qbool
fscondrestart(int checksumFeed, qbool disconnect)
{
	if(fs_gamedirvar->modified){
		if(fscomparefname(lastValidGame, fs_gamedirvar->string) &&
		   (*lastValidGame ||
		    fscomparefname(fs_gamedirvar->string,
			    com_basegame->string)) &&
		   (*fs_gamedirvar->string ||
		    fscomparefname(lastValidGame, com_basegame->string)))
		then{
			comgamerestart(checksumFeed, disconnect);
			return qtrue;
		}else
			fs_gamedirvar->modified = qfalse;
	}

	if(checksumFeed != fs_checksumFeed)
		fsrestart(checksumFeed);
	else if(fs_numServerPaks && !fs_reordered)
		FS_ReorderPurePaks();

	return qfalse;
}

/*
 * Handle-based file calls for virtual machines
 */

int
fsopenmode(const char *qpath, Fhandle *f, Fsmode mode)
{
	int r;
	qbool sync;

	sync = qfalse;

	switch(mode){
	case FS_READ:
		r = fsopenr(qpath, f, qtrue);
		break;
	case FS_WRITE:
		*f = fsopenw(qpath);
		r = 0;
		if(*f == 0)
			r = -1;
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	case FS_APPEND:
		*f = fsopena(qpath);
		r = 0;
		if(*f == 0)
			r = -1;
		break;
	default:
		comerrorf(ERR_FATAL, "fsopenmode: bad mode");
		return -1;
	}

	if(!f)
		return r;

	if(*f){
		if(fsh[*f].zipFile == qtrue)
			fsh[*f].baseOffset = unztell(fsh[*f].handleFiles.file.z);
		else
			fsh[*f].baseOffset = ftell(fsh[*f].handleFiles.file.o);
		fsh[*f].fileSize = r;
		fsh[*f].streamed = qfalse;

		if(mode == FS_READ)
			fsh[*f].streamed = qtrue;
	}
	fsh[*f].handleSync = sync;

	return r;
}

int
fsftell(Fhandle f)
{
	int pos;
	if(fsh[f].zipFile == qtrue)
		pos = unztell(fsh[f].handleFiles.file.z);
	else
		pos = ftell(fsh[f].handleFiles.file.o);
	return pos;
}

void
fsflush(Fhandle f)
{
	fflush(fsh[f].handleFiles.file.o);
}

void
fsfnamecompletion(const char *dir, const char *ext,
		      qbool stripExt, void (*callback)(
			      const char *s), qbool allowNonPureFilesOnDisk)
{
	char	**filenames;
	int	nfiles;
	int	i;
	char	filename[ MAX_STRING_CHARS ];

	filenames = FS_ListFilteredFiles(dir, ext, NULL, &nfiles,
		allowNonPureFilesOnDisk);

	FS_SortFileList(filenames, nfiles);

	for(i = 0; i < nfiles; i++){
		FS_ConvertPath(filenames[ i ]);
		Q_strncpyz(filename, filenames[ i ], MAX_STRING_CHARS);

		if(stripExt)
			Q_stripext(filename, filename, sizeof(filename));

		callback(filename);
	}
	fsfreefilelist(filenames);
}

const char *
fsgetcurrentgamedir(void)
{
	if(fs_gamedirvar->string[0])
		return fs_gamedirvar->string;

	return com_basegame->string;
}
