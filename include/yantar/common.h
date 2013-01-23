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

void MSG_Init(Bitmsg *buf, byte *data, int length);
void MSG_InitOOB(Bitmsg *buf, byte *data, int length);
void MSG_Clear(Bitmsg *buf);
void MSG_WriteData(Bitmsg *buf, const void *data, int length);
void MSG_Bitstream(Bitmsg *buf);

/* TTimo
 * copy a Bitmsg in case we need to store it as is for a bit
 * (as I needed this to keep an Bitmsg from a static var for later use)
 * sets data buffer as MSG_Init does prior to do the copy */
void MSG_Copy(Bitmsg *buf, byte *data, int length, Bitmsg *src);
void MSG_WriteBits(Bitmsg *msg, int value, int bits);

void MSG_WriteChar(Bitmsg *sb, int c);
void MSG_WriteByte(Bitmsg *sb, int c);
void MSG_WriteShort(Bitmsg *sb, int c);
void MSG_WriteLong(Bitmsg *sb, int c);
void MSG_WriteFloat(Bitmsg *sb, float f);
void MSG_WriteString(Bitmsg *sb, const char *s);
void MSG_WriteBigString(Bitmsg *sb, const char *s);
void MSG_WriteAngle16(Bitmsg *sb, float f);
int MSG_HashKey(const char *string, int maxlen);

void MSG_BeginReading(Bitmsg *sb);
void MSG_BeginReadingOOB(Bitmsg *sb);

int MSG_ReadBits(Bitmsg *msg, int bits);

int MSG_ReadChar(Bitmsg *sb);
int MSG_ReadByte(Bitmsg *sb);
int MSG_ReadShort(Bitmsg *sb);
int MSG_ReadLong(Bitmsg *sb);
float MSG_ReadFloat(Bitmsg *sb);
char* MSG_ReadString(Bitmsg *sb);
char* MSG_ReadBigString(Bitmsg *sb);
char* MSG_ReadStringLine(Bitmsg *sb);
float MSG_ReadAngle16(Bitmsg *sb);
void MSG_ReadData(Bitmsg *sb, void *buffer, int size);
int MSG_LookaheadByte(Bitmsg *msg);

void MSG_WriteDeltaUsercmd(Bitmsg *msg, struct Usrcmd *from,
			 struct Usrcmd *to);
void MSG_ReadDeltaUsercmd(Bitmsg *msg, struct Usrcmd *from,
			 struct Usrcmd *to);

void MSG_WriteDeltaUsercmdKey(Bitmsg *msg, int key, Usrcmd *from,
			 Usrcmd *to);
void MSG_ReadDeltaUsercmdKey(Bitmsg *msg, int key, Usrcmd *from, Usrcmd *to);

void MSG_WriteDeltaEntity(Bitmsg *msg, struct Entstate *from,
			 struct Entstate *to, qbool force);
void MSG_ReadDeltaEntity(Bitmsg *msg, Entstate *from, Entstate *to,
			 int number);

void MSG_WriteDeltaPlayerstate(Bitmsg *msg, struct Playerstate *from,
			 struct Playerstate *to);
void MSG_ReadDeltaPlayerstate(Bitmsg *msg, struct Playerstate *from,
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

void NET_Init(void);
void NET_Shutdown(void);
void NET_Restart_f(void);
void NET_Config(qbool enableNetworking);
void NET_FlushPacketQueue(void);
void NET_SendPacket(Netsrc sock, int length, const void *data,
			 Netaddr to);
void QDECL NET_OutOfBandPrint(Netsrc net_socket, Netaddr adr,
			 const char *format,
			 ...) __attribute__ ((format (printf, 3, 4)));
void QDECL NET_OutOfBandData(Netsrc sock, Netaddr adr, byte *format, int len);

qbool NET_CompareAdr(Netaddr a, Netaddr b);
qbool NET_CompareBaseAdrMask(Netaddr a, Netaddr b, int netmask);
qbool NET_CompareBaseAdr(Netaddr a, Netaddr b);
qbool NET_IsLocalAddress(Netaddr adr);
const char* NET_AdrToString(Netaddr a);
const char* NET_AdrToStringwPort(Netaddr a);
int NET_StringToAdr(const char *s, Netaddr *a, Netaddrtype family);
qbool NET_GetLoopPacket(Netsrc sock, Netaddr *net_from,
			Bitmsg *net_message);
void NET_JoinMulticast6(void);
void NET_LeaveMulticast6(void);
void NET_Sleep(int msec);

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

void Netchan_Init(int qport);
void Netchan_Setup(Netsrc sock, Netchan *chan, Netaddr adr, int qport,
		 int challenge,
		 qbool compat);
void Netchan_Transmit(Netchan *chan, int length, const byte *data);
void Netchan_TransmitNextFragment(Netchan *chan);
qbool Netchan_Process(Netchan *chan, Bitmsg *msg);


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
	svc_numsvc,
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

/* returned by Sys_GetProcessorFeatures */
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

void 	comqueueevent(int time, Syseventtype type, int value,
		int value2, int ptrLength,
		void *ptr);
int 	comflushevents(void);
Sysevent comgetsysevent(void);
char* copystr(const char *in);
void 	infoprint(const char *s);

void 	comstartredirect(char *buffer, int buffersize, void (*flush)(char *));
void 	comendredirect(void);
void QDECL comprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void QDECL comdprintf(const char *fmt,
			...) __attribute__ ((format (printf, 1, 2)));
void QDECL comerrorf(int code, const char *fmt,
			...) __attribute__ ((noreturn, format(printf, 2, 3)));
void 	Com_Quit_f(void) __attribute__ ((noreturn));
void 	comgamerestart(int checksumFeed, qbool disconnect);
int 	commillisecs(void);	/* will be journaled properly */
unsigned blockchecksum(const void *buffer, int length);
char* md5file(const char *filename, int length,
		 const char *prefix,
		 int prefix_len);
int 	filterstr(char *filter, char *name, int casesensitive);
int 	filterpath(char *filter, char *name,
					int casesensitive);
int 	comrealtime(Qtime *qtime);
qbool cominsafemode(void);
void 	comrunservpacket(Netaddr *evFrom, Bitmsg *buf);
qbool comisvoiptarget(uint8_t *voipTargets, int voipTargetsSize,
			int clientNum);
void 	comstartupvar(const char *match);

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

void 	Com_Touchmem(void);

void	Com_Init(char *commandLine);
void	Com_Frame(void);
void	Com_Shutdown(void);

/*
 * CLIENT / SERVER SYSTEMS
 */

/*
 * client interface
 */
/* the keyboard binding interface must be setup before execing
 * config files, but the rest of client startup will happen later */
void CL_InitKeyCommands(void);
void CL_Init(void);
void CL_Disconnect(qbool showMainMenu);
void CL_Shutdown(char *finalmsg, qbool disconnect, qbool quit);
void CL_Frame(int msec);
qbool CL_GameCommand(void);
void CL_KeyEvent(int key, qbool down, unsigned time);
/* char events are for field typing, not game control */
void CL_CharEvent(int key);
void CL_MouseEvent(int dx, int dy, int time);
void CL_JoystickEvent(int axis, int value, int time);
void CL_PacketEvent(Netaddr from, Bitmsg *msg);
void CL_ConsolePrint(char *text);
/* do a screen update before starting to load a map
 * when the server is going to load a new map, the entire hunk
 * will be cleared, so the client must shutdown cgame, ui, and
 * the renderer */
void CL_MapLoading(void);
/* adds the current command line as a clc_clientCommand to the client message.
* things like godmode, noclip, etc, are commands directed to the server,
* so when they are typed in at the console, they will need to be forwarded. */
void CL_ForwardCommandToServer(const char *string);
/* dump all memory on an error */
void CL_FlushMemory(void);
/* shutdown client */
void CL_ShutdownAll(qbool shutdownRef);
/* initialize renderer interface */
void CL_InitRef(void);
/* start all the client stuff using the hunk */
void CL_StartHunkUsers(qbool rendererOnly);
/* Restart sound subsystem */
void CL_Snd_Shutdown(void);
/* for keyname autocompletion */
void Key_KeynameCompletion(void (*callback)(const char *s));
/* for writing the config files */
void Key_WriteBindings(Fhandle f);
/* call before filesystem access */
void S_ClearSoundBuffer(void);

void SCR_DebugGraph(float value); /* FIXME: move logging to common? */

/* AVI files have the start of pixel lines 4 byte-aligned */
enum { AVI_LINE_PADDING = 4 };

/*
 * server interface
 */
void SV_Init(void);
void SV_Shutdown(char *finalmsg);
void SV_Frame(int msec);
void SV_PacketEvent(Netaddr from, Bitmsg *msg);
int SV_FrameMsec(void);
qbool SV_GameCommand(void);
int SV_SendQueuedPackets(void);

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

void Sys_Init(void);

/* general development dll loading for virtual machine testing */
void* QDECL Sys_LoadGameDll(const char *name, 
			intptr_t (QDECL **entryPoint)(int, ...),
			intptr_t (QDECL *systemcalls)(intptr_t, ...));
void 	Sys_UnloadDll(void *dllHandle);
void 	Sys_UnloadGame(void);
void* Sys_GetGameAPI(void *parms);
void 	Sys_UnloadCGame(void);
void* Sys_GetCGameAPI(void);
void 	Sys_UnloadUI(void);
void* Sys_GetUIAPI(void);

/* bot libraries */
void 	Sys_UnloadBotLib(void);
void* Sys_GetBotLibAPI(void *parms);
char* Sys_GetCurrentUser(void);
void QDECL Sys_Error(const char *error,
			...) __attribute__ ((noreturn, format (printf, 1, 2)));
void 	Sys_Quit(void) __attribute__ ((noreturn));
char* Sys_GetClipboardData(void);	/* note that this isn't journaled... */
void 	Sys_Print(const char *msg);
/* Sys_Milliseconds should only be used for profiling purposes,
 * any game related timing information should come from event timestamps */
int 	Sys_Milliseconds(void);
void 	Sys_snapv3(float *v);
qbool Sys_RandomBytes(byte *string, int len);
/* the system console is shown when a dedicated server is running */
void 	Sys_DisplaySystemConsole(qbool show);
CPUfeatures Sys_GetProcessorFeatures(void);
void 	Sys_SetErrorText(const char *text);
void 	Sys_SendPacket(int length, const void *data, Netaddr to);
/* Does NOT parse port numbers, only base addresses. */
qbool Sys_StringToAdr(const char *s, Netaddr *a, Netaddrtype family);
qbool Sys_IsLANAddress(Netaddr adr);
void 	Sys_ShowIP(void);
qbool Sys_Mkdir(const char *path);
FILE* Sys_Mkfifo(const char *ospath);
char* Sys_Cwd(void);
void 	Sys_SetDefaultInstallPath(const char *path);
char* Sys_DefaultInstallPath(void);
#ifdef MACOS_X
char* Sys_DefaultAppPath(void);
#endif
void 	Sys_SetDefaultHomePath(const char *path);
char* Sys_DefaultHomePath(void);
const char* Sys_TempPath(void);
const char* Sys_Dirname(char *path);
const char* Sys_Basename(char *path);
char* Sys_ConsoleInput(void);
char** Sys_ListFiles(const char *directory, const char *extension, char *filter,
			int *numfiles,
			qbool wantsubs);
void 	Sys_FreeFileList(char **list);
void 	Sys_Sleep(int msec);
qbool Sys_LowPhysicalMemory(void);
void 	Sys_SetEnv(const char *name, const char *value);
dialogResult_t Sys_Dialog(dialogType_t type, const char *message,
				const char *title);

/*
 * Huffman coding
 *
 * This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book. The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list
 */

#define NYT		HMAX	/* NYT = Not Yet Transmitted */
#define INTERNAL_NODE	(HMAX+1)

typedef struct nodetype {
	struct nodetype	*left, *right, *parent;	/* tree structure */
	struct nodetype	*next, *prev;		/* doubly-linked list */
	struct nodetype	**head;			/* highest ranked node in block */
	int			weight;
	int			symbol;
} Node;

#define HMAX 256	/* Maximum symbol */

typedef struct {
	int	blocNode;
	int	blocPtrs;

	Node	* tree;
	Node	* lhead;
	Node	* ltail;
	Node	* loc[HMAX+1];
	Node	** freelist;

	Node	nodeList[768];
	Node	* nodePtrs[768];
} Huff;

typedef struct {
	Huff	compressor;
	Huff	decompressor;
} Huffman;

void Huff_Compress(Bitmsg *buf, int offset);
void Huff_Decompress(Bitmsg *buf, int offset);
void Huff_Init(Huffman *huff);
void Huff_addRef(Huff* huff, byte ch);
int Huff_Receive(Node *node, int *ch, byte *fin);
void Huff_transmit(Huff *huff, int ch, byte *fout);
void Huff_offsetReceive(Node *node, int *ch, byte *fin, int *offset);
void Huff_offsetTransmit(Huff *huff, int ch, byte *fout, int *offset);
void Huff_putBit(int bit, byte *fout, int *offset);
int Huff_getBit(byte *fout, int *offset);

/* don't use if you don't know what you're doing. */
int Huff_getBloc(void);
void Huff_setBloc(int _bloc);

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
