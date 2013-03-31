/* definitions common between client and server, but not game or ref modules */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _QCOMMON_H_
#define _QCOMMON_H_

#include "cm.h"

/* Ignore __attribute__ on non-gcc platforms */
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

/*
 * msg.c
 */
typedef struct Bitmsg Bitmsg;
struct Usrcmd;
struct Entstate;
struct Playerstate;

struct Bitmsg {
	qbool		allowoverflow;	/* if false, do a comerrorf */
	qbool		overflowed;	/* set to true if the buffer size failed (with allowoverflow set) */
	qbool		oob;		/* set to true if the buffer size failed (with allowoverflow set) */
	byte		*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	int		bit;	/* for bitwise reads and writes */
};

void bminit(Bitmsg *buf, byte *data, int length);
void bminitOOB(Bitmsg *buf, byte *data, int length);
void bmclear(Bitmsg *buf);
void bmwrite(Bitmsg *buf, const void *data, int length);
void bmbitstream(Bitmsg *buf);

/* TTimo
 * copy a Bitmsg in case we need to store it as is for a bit
 * (as I needed this to keep an Bitmsg from a static var for later use)
 * sets data buffer as bminit does prior to do the copy */
void bmcopy(Bitmsg *buf, byte *data, int length, Bitmsg *src);
void bmwritebits(Bitmsg *msg, int value, int bits);

void bmwritec(Bitmsg *sb, int c);
void bmwriteb(Bitmsg *sb, int c);
void bmwrites(Bitmsg *sb, int c);
void bmwritel(Bitmsg *sb, int c);
void bmwritef(Bitmsg *sb, float f);
void bmwritestr(Bitmsg *sb, const char *s);
void bmwritebigstr(Bitmsg *sb, const char *s);
void bmwriteangle16(Bitmsg *sb, float f);
int bmhashkey(const char *string, int maxlen);

void bmstartreading(Bitmsg *sb);
void bmstartreadingOOB(Bitmsg *sb);

int bmreadbits(Bitmsg *msg, int bits);

int bmreadc(Bitmsg *sb);
int bmreadb(Bitmsg *sb);
int bmreads(Bitmsg *sb);
int bmreadl(Bitmsg *sb);
float bmreadf(Bitmsg *sb);
char* bmreadstr(Bitmsg *sb);
char* bmreadbigstr(Bitmsg *sb);
char* bmreadstrline(Bitmsg *sb);
float bmreadangle16(Bitmsg *sb);
void bmread(Bitmsg *sb, void *buffer, int size);
int bmlookaheadbyte(Bitmsg *msg);

void bmwritedeltaUsrcmdkey(Bitmsg *msg, int key, Usrcmd *from,
			 Usrcmd *to);
void bmreaddeltaUsrcmdkey(Bitmsg *msg, int key, Usrcmd *from, Usrcmd *to);

void bmwritedeltaEntstate(Bitmsg *msg, struct Entstate *from,
			 struct Entstate *to, qbool force);
void bmreaddeltaEntstate(Bitmsg *msg, Entstate *from, Entstate *to,
			 int number);

void bmwritedeltaPlayerstate(Bitmsg *msg, struct Playerstate *from,
			 struct Playerstate *to);
void bmreaddeltaPlayerstate(Bitmsg *msg, struct Playerstate *from,
			 struct Playerstate *to);

/*
 * NET
 */

enum {
	NET_ENABLEV4		= 0x01,
	NET_ENABLEV6		= 0x02,
	/* if this flag is set, always attempt ipv6 connections instead of ipv4 
	 * if a v6 address is found. */
	NET_PRIOV6			= 0x04,
	/* disables ipv6 multicast support if set. */
	NET_DISABLEMCAST	= 0x08,


	PACKET_BACKUP		= 32,	/* number of old messages that must be kept 
								 * on client and server for delta comrpession 
								 * and ping estimation */
	PACKET_MASK		 	= (PACKET_BACKUP-1),

	MAX_PACKET_USERCMDS	= 32,	/* max number of Usrcmd in a packet */

	#define PORT_ANY		-1	/* (#def'd because sign is implementation-defined) */

	MAX_RELIABLE_COMMANDS	= 64,	/* max string commands buffered for restransmit */
	
	NET_ADDRSTRMAXLEN	= 48,	/* maximum length of an IPv6 address
							 	 * string including trailing null */
	MAX_MSGLEN 			=16384,	/* max length of a message, which may */
								/* be fragmented into multiple packets */
	MAX_DOWNLOAD_WINDOW= 48,	/* ACK window of 48 download chunks. 
								 * Cannot set this higher, or clients
					 			 * will overflow the reliable commands buffer */
	MAX_DOWNLOAD_BLKSIZE= 1024	/* 896 byte block chunks */
};

typedef enum Netaddrtype Netaddrtype;
typedef enum Netsrc Netsrc;
typedef struct Netaddr Netaddr;
typedef struct Netchan Netchan;

enum Netaddrtype {
	NA_BAD = 0,	/* an address lookup failed */
	NA_BOT,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IP6,
	NA_MULTICAST6,
	NA_UNSPEC
};

enum Netsrc {
	NS_CLIENT,
	NS_SERVER
};

struct Netaddr {
	Netaddrtype	type;

	byte		ip[4];
	byte		ip6[16];

	unsigned short	port;
	unsigned long	scope_id;	/* Needed for IPv6 link-local addresses */
};

void netinit(void);
void netshutdown(void);
void netrestart(void);
void netconfig(qbool enableNetworking);
void netflushpacketqueue(void);
void netsendpacket(Netsrc sock, int length, const void *data,
			 Netaddr to);
void QDECL netprintOOB(Netsrc net_socket, Netaddr adr,
			 const char *format,
			 ...) __attribute__ ((format (printf, 3, 4)));
void QDECL netdataOOB(Netsrc sock, Netaddr adr, byte *format, int len);

qbool equaladdr(Netaddr a, Netaddr b);
qbool equalbaseaddrmask(Netaddr a, Netaddr b, int netmask);
qbool equalbaseaddr(Netaddr a, Netaddr b);
qbool islocaladdr(Netaddr adr);
const char* addrtostr(Netaddr a);
const char* addrporttostr(Netaddr a);
int strtoaddr(const char *s, Netaddr *a, Netaddrtype family);
qbool netgetlooppacket(Netsrc sock, Netaddr *net_from,
			Bitmsg *net_message);
void netjoinmulticast6(void);
void netleavemulticast6(void);
void netsleep(int msec);

#define NETCHAN_GENCHECKSUM(challenge, sequence) ((challenge) ^	\
						 ((sequence) * (challenge)))

/*
 * Netchan handles packet fragmentation and out of order / duplicate suppression
 */

struct Netchan {
	Netsrc	sock;

	int		dropped;	/* between last packet and previous */

	Netaddr	remoteAddress;
	int		qport;	/* qport value to write when transmitting */

	/* sequencing variables */
	int	incomingSequence;
	int	outgoingSequence;

	/* incoming fragment assembly buffer */
	int	fragmentSequence;
	int	fragmentLength;
	byte	fragmentBuffer[MAX_MSGLEN];

	/* outgoing fragment buffer
	 * we need to space out the sending of large fragmented messages */
	qbool		unsentFragments;
	int		unsentFragmentStart;
	int		unsentLength;
	byte		unsentBuffer[MAX_MSGLEN];

	int		challenge;
	int		lastSentTime;
	int		lastSentSize;

};

void ncinit(int qport);
void ncsetup(Netsrc sock, Netchan *chan, Netaddr adr, int qport,
		 int challenge,
		 qbool compat);
void ncsend(Netchan *chan, int length, const byte *data);
void ncsendnextfrag(Netchan *chan);
qbool ncprocess(Netchan *chan, Bitmsg *msg);


/*
 *
 * PROTOCOL
 *
 */

enum {
	PROTOCOL_VERSION	= 71,
	PROTOCOL_LEGACY_VERSION	= 68,
	/* 1.31 - 67 */

	PORT_MASTER			= 27950,
	PORT_UPDATE			= 27951,
	PORT_SERVER			= 27960,
	NUM_SERVER_PORTS	= 4	/* broadcast scan this many ports after
							 * PORT_SERVER so a single machine can
							 * run multiple servers */
};

/* maintain a list of compatible protocols for demo playing
 * NOTE: that stuff only works with two digits protocols */
extern int demo_protocols[];

#if !defined UPDATE_SERVER_NAME && !defined STANDALONE
#define UPDATE_SERVER_NAME "update.quake3arena.com" /* FIXME */
#endif
/* override on command line, config files etc. */
#ifndef MASTER_SERVER_NAME
#define MASTER_SERVER_NAME "master.ioquake3.org"
#endif

#ifndef STANDALONE
 #ifndef AUTHORIZE_SERVER_NAME
 #define AUTHORIZE_SERVER_NAME "authorize.quake3arena.com"
 #endif
 #ifndef PORT_AUTHORIZE
 #define PORT_AUTHORIZE 27952
 #endif
#endif

/* the svc_strings[] array in cl_parse.c should mirror this */
/*
 * server to client
 */
enum {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,	/* [short] [string] only in gamestate messages */
	svc_baseline,		/* only in gamestate messages */
	svc_serverCommand,	/* [string] to be executed by client game module */
	svc_download,		/* [short] size [size bytes] */
	svc_snapshot,
	svc_EOF,
	/* new commands, supported only by ioquake3 protocol but not legacy */
	svc_voip,	/* not wrapped in USE_VOIP, so this value is reserved. */
} svc_ops_e;

/*
 * client to server
 */
enum {
	clc_bad,
	clc_nop,
	clc_move,		/* [[Usrcmd] */
	clc_moveNoDelta,	/* [[Usrcmd] */
	clc_clientCommand,	/* [string] message */
	clc_EOF,
	/* new commands, supported only by ioquake3 protocol but not legacy */
	clc_voip,	/* not wrapped in USE_VOIP, so this value is reserved. */
} clc_ops_e;


/*
 * VIRTUAL MACHINE
 */

typedef struct Vm Vm;
typedef enum Vmmode Vmmode;
typedef enum Sharedtraps Sharedtraps;

enum Vmmode {
	VMnative,
	VMbytecode,
	VMcompiled
};

void vminit(void);
/* module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm" */
Vm* vmcreate(const char *module, intptr_t (*systemCalls)(intptr_t *),
 		Vmmode interpret);
void vmfree(Vm *vm);
void vmclear(void);
void vmsetforceunload(void);
void vmclearforceunload(void);
Vm* vmrestart(Vm *vm, qbool unpure);
intptr_t QDECL vmcall(Vm *vm, int callNum, ...);
void vmdebug(int level);
void* vmargptr(intptr_t intValue);
void* vmexplicitargptr(Vm *vm, intptr_t intValue);

#define VMA(x) vmargptr(args[x])
static ID_INLINE float
_vmf(intptr_t x)
{
	Flint fi;
	fi.i = (int)x;
	return fi.f;
}
#define VMF(x) _vmf(args[x])


/*
 * CMD
 *
 * Command text buffering and command execution
 */

/*
 * Any number of commands can be added in a frame, from several different sources.
 * Most commands come from either keybindings or console line input, but entire text
 * files can be execed.
 */

void cbufinit(void);
void cbufaddstr(const char *text);
void cbufexecstr(int exec_when, const char *text);
void cbufflush(void);

/*
 *
 * Command execution takes a null terminated string, breaks it into tokens,
 * then searches for a command or variable that matches the first token.
 *
 */

typedef void (*xcommand_t)(void);

void cmdinit(void);
void cmdadd(const char *cmd_name, xcommand_t function);
void cmdremove(const char *cmd_name);

typedef void (*Completionfunc)(char *args, int argNum);

/* don't allow VMs to remove system commands */
void cmdsaferemove(const char *cmd_name);
void cmdcompletion(void (*callback)(const char *s));
void cmdsetcompletion(const char *command,
				 Completionfunc complete);
void cmdcompletearg(const char *command, char *args, int argNum);
void cmdcompletecfgname(char *args, int argNum);

int cmdargc(void);
char* cmdargv(int arg);
void cmdargvbuf(int arg, char *buffer, int bufferLength);
char* cmdargs(void);
char* cmdargsfrom(int arg);
void cmdargsbuf(char *buffer, int bufferLength);
char* cmdcmd(void);
void cmdsanitizeargs(void);
void cmdstrtok(const char *text);
void cmdstrtokignorequotes(const char *text_in);
void cmdexecstr(const char *text);


/*
 * CVAR
 */

Cvar* cvarget(const char *var_name, const char *value, int flags);
/* basically a slightly modified cvarget for the interpreted modules */
void cvarregister(Vmcvar *vmCvar, const char *varName,
 		const char *defaultValue,
 		int flags);
void cvarupdate(Vmcvar *vmCvar);
void cvarsetdesc(const char *name, const char *desc);
void cvarsetstr(const char *var_name, const char *value);
Cvar* cvarsetstr2(const char *var_name, const char *value, qbool force);
void cvarsetstrsafe(const char *var_name, const char *value);
void cvarsetstrlatched(const char *var_name, const char *value);
void cvarsetf(const char *var_name, float value);
void cvarsetfsafe(const char *var_name, float value);
float cvargetf(const char *var_name);
int cvargeti(const char *var_name);
char* cvargetstr(const char *var_name);
void cvargetstrbuf(const char *var_name, char *buffer,
 				int bufsize);
int cvarflags(const char *var_name);
/* callback with each valid string */
void cvarcmdcompletion(void (*callback)(const char *s));
void cvarreset(const char *var_name);
void cvarforcereset(const char *var_name);
void cvarsetcheatstate(void);
qbool cvariscmd(void);
void cvarwritevars(Fhandle f);
void cvarinit(void);
char* cvargetinfostr(int bit);
char* cvargetbiginfostr(int bit);
void cvargetinfostrbuf(int bit, char *buff, int buffsize);
void cvarcheckrange(Cvar *cv, float minVal, float maxVal,
 qbool shouldBeIntegral);
void cvarrestart(qbool unsetVM);
void cvarrestart_f(void);
void cvarcompletename(char *args, int argNum);

/* whenever a cvar is modifed, its flags will be OR'd into this, so
 * a single check can determine if any CVAR_USERINFO, CVAR_SERVERINFO,
 * etc, variables have been modified since the last check. The bit
 * can then be cleared to allow another change detection. */
extern int cvar_modifiedFlags;


/*
 * FILESYSTEM
 *
 * No stdio calls should be used by any part of the game, because
 * we need to deal with all sorts of directory and separator char
 * issues.
 */

enum {
	/* referenced flags */
	/* these are in loop specific order so don't change the order */
	FS_GENERAL_REF	= 0x01,
	FS_UI_REF			= 0x02,
	FS_CGAME_REF	= 0x04,
	/* number of id paks that will never be autodownloaded from baseq3/missionpack */
	NUM_ID_PAKS		= 9,

	MAX_FILE_HANDLES	= 64
};

#ifdef DEDICATED
# define Q3CONFIG_CFG	"server.cfg"
#else
# define Q3CONFIG_CFG	"user.cfg"
#endif

qbool fsisinitialized(void);
void fsinit(void);
void fsshutdown(qbool closemfp);
qbool fscondrestart(int checksumFeed, qbool disconnect);
void fsrestart(int checksumFeed);
void fsaddgamedir(const char *path, const char *dir);
char** fslistfiles(const char *directory, const char *extension,
		int *numfiles);
void fsfreefilelist(char **list);
qbool fsfileexists(const char *file);
qbool fscreatepath(char *OSPath);
Vmmode fsfindvm(void **startSearch, char *found, int foundlen,
		const char *name,
		int enableDll);
char*fsbuildospath(const char *base, const char *game, const char *qpath);
qbool fscompzipchecksum(const char *zipfile);
int fsloadstack(void);
int fsgetfilelist(const char *path, const char *extension,
		char *listbuf,
		int bufsize);
int fsgetmodlist(char *listbuf, int bufsize);
Fhandle fsopenw(const char *qpath);
Fhandle fsopena(const char *filename);
Fhandle fscreatepipefile(const char *filename);
Fhandle fssvopenw(const char *filename);
long fssvopenr(const char *filename, Fhandle *fp);
void fssvrename(const char *from, const char *to);
long fsopenr(const char *qpath, Fhandle *file,
		qbool uniqueFILE);
int fsfileisinpak(const char *filename, int *pChecksum);
int fswrite(const void *buffer, int len, Fhandle f);
int fsread2(void *buffer, int len, Fhandle f);
int fsread(void *buffer, int len, Fhandle f);
void fsclose(Fhandle f);
long fsreadfiledir(const char *qpath, void *searchPath, qbool unpure,
		void **buffer);
long fsreadfile(const char *qpath, void **buffer);
void fsforceflush(Fhandle f);
void fsfreefile(void *buffer);
void fswritefile(const char *qpath, const void *buffer, int size);
long fsfilelen(Fhandle f);
int fsftell(Fhandle f);
void fsflush(Fhandle f);
void QDECL fsprintf(Fhandle f, const char *fmt,
			...) __attribute__ ((format (printf, 2, 3)));
int fsopenmode(const char *qpath, Fhandle *f,
 			 Fsmode mode);
int fsseek(Fhandle f, long offset, int origin);
qbool fscomparefname(const char *s1, const char *s2);
const char* fsloadedpaknames(void);
const char* fsloadedpakchecksums(void);
const char* fsloadedpakpurechecksums(void);
const char* fsreferencedpaknames(void);
const char* fsreferencedpakchecksums(void);
const char* fsreferencedpakpurechecksums(void);
void 	fsclearpakrefs(int flags);
void 	fspureservsetreferencedpaks(const char *pakSums, const char *pakNames);
void 	fspureservsetloadedpaks(const char *pakSums, const char *pakNames);
qbool fscheckdirtraversal(const char *checkdir);
qbool fsispak(char *pak, char *base, int numPaks);
qbool fscomparepaks(char *neededpaks, int len, qbool dlstring);
void 	fsrename(const char *from, const char *to);
void 	fsremove(const char *osPath);
void 	fshomeremove(const char *homePath);
void 	fsfnamecompletion(const char *dir, const char *ext,
		qbool stripExt, void (*callback)(const char *s),
		qbool allowNonPureFilesOnDisk);
const char* fsgetcurrentgamedir(void);
qbool fswhich(const char *filename, void *searchPath);

/*
 * Edit fields and command line history/completion
 */
 
 typedef struct Field Field;

enum { MAX_EDIT_LINE = 256 };
struct Field {
	int	cursor;
	int	scroll;
	int	widthInChars;
	char	buffer[MAX_EDIT_LINE];
};

void fieldclear(Field *edit);
void fieldautocomplete(Field *edit);
void fieldcompletekeyname(void);
void fieldcompletefilename(const char *dir,
	 const char *ext, qbool stripExt,
			 qbool allowNonPureFilesOnDisk);
void fieldcompletecmd(char *cmd,
	 qbool doCommands, qbool doCvars);


/*
 * MISC
 */

/* returned by sysgetprocessorfeatures */
typedef enum {
	CF_RDTSC		= 1 << 0,
	CF_MMX			= 1 << 1,
	CF_MMX_EXT		= 1 << 2,
	CF_3DNOW		= 1 << 3,
	CF_3DNOW_EXT	= 1 << 4,
	CF_SSE			= 1 << 5,
	CF_SSE2			= 1 << 6,
	CF_ALTIVEC		= 1 << 7
} CPUfeatures;

/* centralized and cleaned, that's the max string you can send to a comprintf / comdprintf (above gets truncated) */
#define MAXPRINTMSG 4096


typedef enum {
	/* SE_NONE must be zero */
	SE_NONE = 0,		/* evTime is still valid */
	SE_KEY,			/* evValue is a key code, evValue2 is the down flag */
	SE_CHAR,		/* evValue is an ascii char */
	SE_MOUSE,		/* evValue and evValue2 are reletive signed x / y moves */
	SE_JOYSTICK_AXIS,	/* evValue is an axis number and evValue2 is the current state (-127 to 127) */
	SE_CONSOLE		/* evPtr is a char* */
} Syseventtype;

typedef struct {
	int		evTime;
	Syseventtype	evType;
	int		evValue, evValue2;
	int		evPtrLength;	/* bytes of data pointed to by evPtr, for journaling */
	void		*evPtr;		/* this must be manually freed if not NULL */
} Sysevent;

void	cominit(char *commandLine);
void	comtouchmem(void);
void	comshutdown(void);
void	comframe(void);
void	comqueueevent(int time, Syseventtype type, int value,
		int value2, int ptrLength,
		void *ptr);
int	comflushevents(void);
Sysevent	comgetsysevent(void);
char*	copystr(const char *in);
void	infoprint(const char *s);
void	comstartredirect(char *buffer, int buffersize, void (*flush)(char *));
void	comendredirect(void);
void QDECL	comprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void QDECL	comdprintf(const char *fmt,
			...) __attribute__ ((format (printf, 1, 2)));
void QDECL	comerrorf(int code, const char *fmt,
			...) __attribute__ ((noreturn, format(printf, 2, 3)));
void 	Com_Quit_f(void) __attribute__ ((noreturn));
void 	comgamerestart(int checksumFeed, qbool disconnect);
int 	commillisecs(void);	/* will be journaled properly */
uint	blockchecksum(const void *buffer, int length);
char*	md5file(const char *filename, int length,
		 const char *prefix,
		 int prefix_len);
int 	filterstr(char *filter, char *name, int casesensitive);
int 	filterpath(char *filter, char *name,
		int casesensitive);
int	comrealtime(Qtime *qtime);
qbool	cominsafemode(void);
void	comrunservpacket(Netaddr *evFrom, Bitmsg *buf);
qbool	comisvoiptarget(uint8_t *voipTargets, int voipTargetsSize,
			int clientNum);
void	comstartupvar(const char *match);
void	comrandbytes(byte *string, int len);

extern Cvar	*com_developer;
extern Cvar	*com_dedicated;
extern Cvar	*com_speeds;
extern Cvar	*com_timescale;
extern Cvar	*com_sv_running;
extern Cvar	*com_cl_running;
extern Cvar	*com_version;
extern Cvar	*com_blood;
extern Cvar	*com_buildScript;	/* for building release pak files */
extern Cvar	*com_journal;
extern Cvar	*com_cameraMode;
extern Cvar	*com_ansiColor;
extern Cvar	*com_unfocused;
extern Cvar	*com_maxfpsUnfocused;
extern Cvar	*com_minimized;
extern Cvar	*com_maxfpsMinimized;
extern Cvar	*com_altivec;
extern Cvar	*com_standalone;
extern Cvar	*com_basegame;
extern Cvar	*com_homepath;

/* both client and server must agree to pause */
extern Cvar	*cl_paused;
extern Cvar	*sv_paused;

extern Cvar	*cl_packetdelay;
extern Cvar	*sv_packetdelay;

extern Cvar	*com_gamename;
extern Cvar	*com_protocol;

/* com_speeds times */
extern int	time_game;
extern int	time_frontend;
extern int	time_backend;	/* renderer backend time */

extern int	com_frameTime;

extern qbool com_errorEntered;
extern qbool com_fullyInitialized;

extern Fhandle	com_journalFile;
extern Fhandle	com_journalDataFile;

typedef enum {
	MTfree,
	MTgeneral,
	MTbotlib,
	MTrenderer,
	MTsmall,
	MTstatic
} Memtag;

#if defined(_DEBUG) && !defined(BSPC)
	#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define ztagalloc(size, tag)	ztagallocdebug(size, tag, # size, __FILE__, \
	__LINE__)
#define zalloc(size)		zallocdebug(size, # size, __FILE__, __LINE__)
#define salloc(size)		sallocdebug(size, # size, __FILE__, __LINE__)
void*	ztagallocdebug(int size, int tag, char *label, char *file, int line);	/* NOT 0 filled memory */
void*	zallocdebug(int size, char *label, char *file, int line);		/* returns 0 filled memory */
void*	sallocdebug(int size, char *label, char *file, int line);		/* returns 0 filled memory */
#else
void*	ztagalloc(int size, int tag);	/* NOT 0 filled memory */
void*	zalloc(int size);		/* returns 0 filled memory */
void*	salloc(int size);		/* NOT 0 filled memory only for small allocations */
#endif
void 	zfree(void *ptr);
void 	zfreetags(int tag);
int 	zmemavailable(void);
void 	zlogheap(void);

void 	hunkclear(void);
void 	hunkcleartomark(void);
void 	hunksetmark(void);
qbool	hunkcheckmark(void);
void 	hunkcleartemp(void);
void*	hunkalloctemp(int size);
void 	hunkfreetemp(void *buf);
int 	hunkmemremaining(void);
void 	hunklog(void);
void 	hunktrash(void);

/*
 * CLIENT / SERVER SYSTEMS
 */

/*
 * client interface
 */
/* the keyboard binding interface must be setup before execing
 * config files, but the rest of client startup will happen later */
void clinitkeycmds(void);
void clinit(void);
void cldisconnect(qbool showMainMenu);
void clshutdown(char *finalmsg, qbool disconnect, qbool quit);
void clframe(int msec);
qbool clgamecmd(void);
void clkeyevent(int key, qbool down, unsigned time);
/* char events are for field typing, not game control */
void clcharevent(int key);
void clmouseevent(int dx, int dy, int time);
void cljoystickevent(int axis, int value, int time);
void clpacketevent(Netaddr from, Bitmsg *msg);
void clconsoleprint(char *text);
/* do a screen update before starting to load a map
 * when the server is going to load a new map, the entire hunk
 * will be cleared, so the client must shutdown cgame, ui, and
 * the renderer */
void clmaploading(void);
/* adds the current command line as a clc_clientCommand to the client message.
* things like godmode, noclip, etc, are commands directed to the server,
* so when they are typed in at the console, they will need to be forwarded. */
void clforwardcmdtoserver(const char *string);
/* dump all memory on an error */
void clflushmem(void);
/* shutdown client */
void clshutdownall(qbool shutdownRef);
/* initialize renderer interface */
void clinitref(void);
/* start all the client stuff using the hunk */
void clstarthunkusers(qbool rendererOnly);
/* Restart sound subsystem */
void clsndshutdown(void);
/* for keyname autocompletion */
void keykeynamecompletion(void (*callback)(const char *s));
/* for writing the config files */
void keywritebindings(Fhandle f);
/* call before filesystem access */
void sndclearbuf(void);

void scrdebuggraph(float value); /* FIXME: move logging to common? */

/* AVI files have the start of pixel lines 4 byte-aligned */
enum { AVI_LINE_PADDING = 4 };

/*
 * server interface
 */
void svinit(void);
void svshutdown(char *finalmsg);
void svframe(int msec);
void svpacketevent(Netaddr from, Bitmsg *msg);
int svframemsec(void);
qbool svgamecmd(void);
int svsendqueuedpackets(void);

/*
 * UI interface
 */
qbool UI_GameCommand(void);


/*
 * NON-PORTABLE SYSTEM SERVICES
 */

enum { MAX_JOYSTICK_AXIS = 16 };

typedef enum {
	DR_YES	= 0,
	DR_NO	= 1,
	DR_OK	= 0,
	DR_CANCEL = 1
} dialogResult_t;

typedef enum {
	DT_INFO,
	DT_WARNING,
	DT_ERROR,
	DT_YES_NO,
	DT_OK_CANCEL
} dialogType_t;

void sysinit(void);

/* general development dll loading for virtual machine testing */
void* QDECL sysloadgamedll(const char *name, 
			intptr_t (QDECL **entryPoint)(int, ...),
			intptr_t (QDECL *systemcalls)(intptr_t, ...));
void 	sysunloaddll(void *dllHandle);
void 	sysunloadgame(void);
void* sysgetgameapi(void *parms);
void 	sysunloadcgame(void);
void* sysgetcgameapi(void);
void 	sysunloadui(void);
void* sysgetuiapi(void);

/* bot libraries */
void 	sysunloadbotlib(void);
void* sysgetbotlibapi(void *parms);
char* sysgetcurrentuser(void);
void QDECL syserrorf(const char *error,
			...) __attribute__ ((noreturn, format (printf, 1, 2)));
void 	sysquit(void) __attribute__ ((noreturn));
char* sysgetclipboarddata(void);	/* note that this isn't journaled... */
void 	sysprint(const char *msg);
/* sysmillisecs should only be used for profiling purposes,
 * any game related timing information should come from event timestamps */
int 	sysmillisecs(void);
void 	syssnapv3(float *v);
qbool sysrandbytes(byte *string, int len);
/* the system console is shown when a dedicated server is running */
void 	sysdisplaysysconsole(qbool show);
CPUfeatures sysgetprocessorfeatures(void);
void 	sysseterrortext(const char *text);
void 	syssendpacket(int length, const void *data, Netaddr to);
/* Does NOT parse port numbers, only base addresses. */
qbool sysstrtoaddr(const char *s, Netaddr *a, Netaddrtype family);
qbool sysisLANaddr(Netaddr adr);
void 	sysshowIP(void);
qbool sysmkdir(const char *path);
FILE* sysmkfifo(const char *ospath);
char* syspwd(void);
void 	syssetdefaultinstallpath(const char *path);
char* sysgetdefaultinstallpath(void);
#ifdef MACOS_X
char* sysgetdefaultapppath(void);
#endif
void 	syssetdefaulthomepath(const char *path);
char* sysgetdefaulthomepath(void);
const char* systemppath(void);
const char* sysdirname(char *path);
const char* sysbasename(char *path);
char* sysconsoleinput(void);
char** syslistfiles(const char *directory, const char *extension, char *filter,
			int *numfiles,
			qbool wantsubs);
void 	sysfreefilelist(char **list);
void 	syssleep(int msec);
qbool syslowmem(void);
void 	syssetenv(const char *name, const char *value);
dialogResult_t sysmkdialog(dialogType_t type, const char *message,
				const char *title);

/*
 * Huffman coding
 *
 * This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book. The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list
 */

enum {
	HMAX		= 256,	/* Maximum symbol */
	NYT		= HMAX,	/* Not Yet Transmitted */
	INTERNAL_NODE	= (HMAX+1)
};

typedef struct Node	Node;
typedef struct Huff	Huff;
typedef struct Huffman	Huffman;

struct Node {
	Node	*left, *right, *parent;	/* tree structure */
	Node	*next, *prev;		/* doubly-linked list */
	Node	**head;			/* highest ranked node in block */
	int	weight;
	int	symbol;
};

struct Huff {
	int	blocNode;
	int	blocPtrs;
	Node	*tree;
	Node	*lhead;
	Node	*ltail;
	Node	*loc[HMAX+1];
	Node	**freelist;
	Node	nodeList[768];
	Node	*nodePtrs[768];
};

struct Huffman {
	Huff	compressor;
	Huff	decompressor;
};

void huffcompress(Bitmsg *buf, int offset);
void huffdecompress(Bitmsg *buf, int offset);
void huffinit(Huffman *huff);
void huffaddref(Huff* huff, byte ch);
int huffrecv(Node *node, int *ch, byte *fin);
void hufftransmit(Huff *huff, int ch, byte *fout);
void huffoffsetrecv(Node *node, int *ch, byte *fin, int *offset);
void huffoffsettransmit(Huff *huff, int ch, byte *fout, int *offset);
void huffputbit(int bit, byte *fout, int *offset);
int huffgetbit(byte *fout, int *offset);

/* don't use if you don't know what you're doing. */
int huffgetbloc(void);
void huffsetbloc(int _bloc);

extern Huffman clientHuffTables;

enum {
	SV_ENCODE_START 	= 4,
	SV_DECODE_START	= 12,
	CL_ENCODE_START	= 12,
	CL_DECODE_START	= 4,
	
	/* flags for sv_allowDownload and cl_allowDownload */
	DLF_ENABLE			= 1,
	DLF_NO_REDIRECT		= 2,
	DLF_NO_UDP			= 4,
	DLF_NO_DISCONNECT	= 8
};

#endif	/* _QCOMMON_H_ */
