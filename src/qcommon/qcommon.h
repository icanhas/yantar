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

#include "../qcommon/cm_public.h"

/* Ignore __attribute__ on non-gcc platforms */
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

/*
 * msg.c
 */
typedef struct msg_s msg_t;
struct usercmd_s;
struct entityState_s;
struct playerState_s;

struct msg_s {
	qbool		allowoverflow;	/* if false, do a Q_Error */
	qbool		overflowed;	/* set to true if the buffer size failed (with allowoverflow set) */
	qbool		oob;		/* set to true if the buffer size failed (with allowoverflow set) */
	byte		*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	int		bit;	/* for bitwise reads and writes */
};

void MSG_Init(msg_t *buf, byte *data, int length);
void MSG_InitOOB(msg_t *buf, byte *data, int length);
void MSG_Clear(msg_t *buf);
void MSG_WriteData(msg_t *buf, const void *data, int length);
void MSG_Bitstream(msg_t *buf);

/* TTimo
 * copy a msg_t in case we need to store it as is for a bit
 * (as I needed this to keep an msg_t from a static var for later use)
 * sets data buffer as MSG_Init does prior to do the copy */
void MSG_Copy(msg_t *buf, byte *data, int length, msg_t *src);
void MSG_WriteBits(msg_t *msg, int value, int bits);

void MSG_WriteChar(msg_t *sb, int c);
void MSG_WriteByte(msg_t *sb, int c);
void MSG_WriteShort(msg_t *sb, int c);
void MSG_WriteLong(msg_t *sb, int c);
void MSG_WriteFloat(msg_t *sb, float f);
void MSG_WriteString(msg_t *sb, const char *s);
void MSG_WriteBigString(msg_t *sb, const char *s);
void MSG_WriteAngle16(msg_t *sb, float f);
int MSG_HashKey(const char *string, int maxlen);

void MSG_BeginReading(msg_t *sb);
void MSG_BeginReadingOOB(msg_t *sb);

int MSG_ReadBits(msg_t *msg, int bits);

int MSG_ReadChar(msg_t *sb);
int MSG_ReadByte(msg_t *sb);
int MSG_ReadShort(msg_t *sb);
int MSG_ReadLong(msg_t *sb);
float MSG_ReadFloat(msg_t *sb);
char* MSG_ReadString(msg_t *sb);
char* MSG_ReadBigString(msg_t *sb);
char* MSG_ReadStringLine(msg_t *sb);
float MSG_ReadAngle16(msg_t *sb);
void MSG_ReadData(msg_t *sb, void *buffer, int size);
int MSG_LookaheadByte(msg_t *msg);

void MSG_WriteDeltaUsercmd(msg_t *msg, struct usercmd_s *from,
			 struct usercmd_s *to);
void MSG_ReadDeltaUsercmd(msg_t *msg, struct usercmd_s *from,
			 struct usercmd_s *to);

void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from,
			 usercmd_t *to);
void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);

void MSG_WriteDeltaEntity(msg_t *msg, struct entityState_s *from,
			 struct entityState_s *to, qbool force);
void MSG_ReadDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to,
			 int number);

void MSG_WriteDeltaPlayerstate(msg_t *msg, struct playerState_s *from,
			 struct playerState_s *to);
void MSG_ReadDeltaPlayerstate(msg_t *msg, struct playerState_s *from,
			 struct playerState_s *to);

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

	MAX_PACKET_USERCMDS	= 32,	/* max number of usercmd_t in a packet */

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

typedef enum netadrtype_e netadrtype_t;
typedef enum netsrc_e netsrc_t;
typedef struct netadr_s netadr_t;
typedef struct netchan_s netchan_t;

enum netadrtype_e {
	NA_BAD = 0,	/* an address lookup failed */
	NA_BOT,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IP6,
	NA_MULTICAST6,
	NA_UNSPEC
};

enum netsrc_e {
	NS_CLIENT,
	NS_SERVER
};

struct netadr_s {
	netadrtype_t	type;

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
void NET_SendPacket(netsrc_t sock, int length, const void *data,
			 netadr_t to);
void QDECL NET_OutOfBandPrint(netsrc_t net_socket, netadr_t adr,
			 const char *format,
			 ...) __attribute__ ((format (printf, 3, 4)));
void QDECL NET_OutOfBandData(netsrc_t sock, netadr_t adr, byte *format, int len);

qbool NET_CompareAdr(netadr_t a, netadr_t b);
qbool NET_CompareBaseAdrMask(netadr_t a, netadr_t b, int netmask);
qbool NET_CompareBaseAdr(netadr_t a, netadr_t b);
qbool NET_IsLocalAddress(netadr_t adr);
const char* NET_AdrToString(netadr_t a);
const char* NET_AdrToStringwPort(netadr_t a);
int NET_StringToAdr(const char *s, netadr_t *a, netadrtype_t family);
qbool NET_GetLoopPacket(netsrc_t sock, netadr_t *net_from,
			msg_t *net_message);
void NET_JoinMulticast6(void);
void NET_LeaveMulticast6(void);
void NET_Sleep(int msec);

#define NETCHAN_GENCHECKSUM(challenge, sequence) ((challenge) ^	\
						 ((sequence) * (challenge)))

/*
 * Netchan handles packet fragmentation and out of order / duplicate suppression
 */

struct netchan_s {
	netsrc_t	sock;

	int		dropped;	/* between last packet and previous */

	netadr_t	remoteAddress;
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

#ifdef LEGACY_PROTOCOL
	qbool compat;
#endif
};

void Netchan_Init(int qport);
void Netchan_Setup(netsrc_t sock, netchan_t *chan, netadr_t adr, int qport,
		 int challenge,
		 qbool compat);
void Netchan_Transmit(netchan_t *chan, int length, const byte *data);
void Netchan_TransmitNextFragment(netchan_t *chan);
qbool Netchan_Process(netchan_t *chan, msg_t *msg);


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
#define MASTER_SERVER_NAME "master.quake3arena.com"
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
	clc_move,		/* [[usercmd_t] */
	clc_moveNoDelta,	/* [[usercmd_t] */
	clc_clientCommand,	/* [string] message */
	clc_EOF,
	/* new commands, supported only by ioquake3 protocol but not legacy */
	clc_voip,	/* not wrapped in USE_VOIP, so this value is reserved. */
} clc_ops_e;


/*
 * VIRTUAL MACHINE
 */

typedef struct vm_s vm_t;
typedef enum vmInterpret_e vmInterpret_t;
typedef enum sharedTraps_e sharedTraps_t;

enum vmInterpret_e {
	VMI_NATIVE,
	VMI_BYTECODE,
	VMI_COMPILED
};

enum sharedTraps_e {
	TRAP_MEMSET = 100,
	TRAP_MEMCPY,
	TRAP_STRNCPY,
	TRAP_SIN,
	TRAP_COS,
	TRAP_ATAN2,
	TRAP_SQRT,
	TRAP_MATRIXMULTIPLY,
	TRAP_ANGLEVECTORS,
	TRAP_PERPENDICULARVECTOR,
	TRAP_FLOOR,
	TRAP_CEIL,

	TRAP_TESTPRINTINT,
	TRAP_TESTPRINTFLOAT
};

void VM_Init(void);
/* module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm" */
vm_t* VM_Create(const char *module, intptr_t (*systemCalls)(intptr_t *),
 		vmInterpret_t interpret);
void VM_Free(vm_t *vm);
void VM_Clear(void);
void VM_Forced_Unload_Start(void);
void VM_Forced_Unload_Done(void);
vm_t* VM_Restart(vm_t *vm, qbool unpure);
intptr_t QDECL VM_Call(vm_t *vm, int callNum, ...);
void VM_Debug(int level);
void* VM_ArgPtr(intptr_t intValue);
void* VM_ExplicitArgPtr(vm_t *vm, intptr_t intValue);

#define VMA(x) VM_ArgPtr(args[x])
static ID_INLINE float
_vmf(intptr_t x)
{
	floatint_t fi;
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

void Cbuf_Init(void);
void Cbuf_AddText(const char *text);
void Cbuf_ExecuteText(int exec_when, const char *text);
void Cbuf_Execute(void);

/*
 *
 * Command execution takes a null terminated string, breaks it into tokens,
 * then searches for a command or variable that matches the first token.
 *
 */

typedef void (*xcommand_t)(void);

void Cmd_Init(void);
void Cmd_AddCommand(const char *cmd_name, xcommand_t function);
void Cmd_RemoveCommand(const char *cmd_name);

typedef void (*completionFunc_t)(char *args, int argNum);

/* don't allow VMs to remove system commands */
void Cmd_RemoveCommandSafe(const char *cmd_name);
void Cmd_CommandCompletion(void (*callback)(const char *s));
void Cmd_SetCommandCompletionFunc(const char *command,
				 completionFunc_t complete);
void Cmd_CompleteArgument(const char *command, char *args, int argNum);
void Cmd_CompleteCfgName(char *args, int argNum);

int Cmd_Argc(void);
char* Cmd_Argv(int arg);
void Cmd_ArgvBuffer(int arg, char *buffer, int bufferLength);
char* Cmd_Args(void);
char* Cmd_ArgsFrom(int arg);
void Cmd_ArgsBuffer(char *buffer, int bufferLength);
char* Cmd_Cmd(void);
void Cmd_Args_Sanitize(void);
void Cmd_TokenizeString(const char *text);
void Cmd_TokenizeStringIgnoreQuotes(const char *text_in);
void Cmd_ExecuteString(const char *text);


/*
 * CVAR
 */

cvar_t* Cvar_Get(const char *var_name, const char *value, int flags);
/* basically a slightly modified Cvar_Get for the interpreted modules */
void Cvar_Register(vmCvar_t *vmCvar, const char *varName,
 		const char *defaultValue,
 		int flags);
void Cvar_Update(vmCvar_t *vmCvar);
void Cvar_SetDesc(const char *name, const char *desc);
void Cvar_Set(const char *var_name, const char *value);
cvar_t* Cvar_Set2(const char *var_name, const char *value, qbool force);
void Cvar_SetSafe(const char *var_name, const char *value);
void Cvar_SetLatched(const char *var_name, const char *value);
void Cvar_SetValue(const char *var_name, float value);
void Cvar_SetValueSafe(const char *var_name, float value);
float Cvar_VariableValue(const char *var_name);
int Cvar_VariableIntegerValue(const char *var_name);
char* Cvar_VariableString(const char *var_name);
void Cvar_VariableStringBuffer(const char *var_name, char *buffer,
 				int bufsize);
int Cvar_Flags(const char *var_name);
/* callback with each valid string */
void Cvar_CommandCompletion(void (*callback)(const char *s));
void Cvar_Reset(const char *var_name);
void Cvar_ForceReset(const char *var_name);
void Cvar_SetCheatState(void);
qbool Cvar_Command(void);
void Cvar_WriteVariables(fileHandle_t f);
void Cvar_Init(void);
char* Cvar_InfoString(int bit);
char* Cvar_InfoString_Big(int bit);
void Cvar_InfoStringBuffer(int bit, char *buff, int buffsize);
void Cvar_CheckRange(cvar_t *cv, float minVal, float maxVal,
 qbool shouldBeIntegral);
void Cvar_Restart(qbool unsetVM);
void Cvar_Restart_f(void);
void Cvar_CompleteCvarName(char *args, int argNum);

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
	NUM_TA_PAKS		= 4,

	MAX_FILE_HANDLES	= 64
};

#ifdef DEDICATED
# define Q3CONFIG_CFG	"q3config_server.cfg"
#else
# define Q3CONFIG_CFG	"q3config.cfg"
#endif

qbool FS_Initialized(void);
void FS_InitFilesystem(void);
void FS_Shutdown(qbool closemfp);
qbool FS_ConditionalRestart(int checksumFeed, qbool disconnect);
void FS_Restart(int checksumFeed);
void FS_AddGameDirectory(const char *path, const char *dir);
char** FS_ListFiles(const char *directory, const char *extension,
		int *numfiles);
void FS_FreeFileList(char **list);
qbool FS_FileExists(const char *file);
qbool FS_CreatePath(char *OSPath);
vmInterpret_t FS_FindVM(void **startSearch, char *found, int foundlen,
		const char *name,
		int enableDll);
char*FS_BuildOSPath(const char *base, const char *game, const char *qpath);
qbool FS_CompareZipChecksum(const char *zipfile);
int FS_LoadStack(void);
int FS_GetFileList(const char *path, const char *extension,
		char *listbuf,
		int bufsize);
int FS_GetModList(char *listbuf, int bufsize);
fileHandle_t FS_FOpenFileWrite(const char *qpath);
fileHandle_t FS_FOpenFileAppend(const char *filename);
fileHandle_t FS_FCreateOpenPipeFile(const char *filename);
fileHandle_t FS_SV_FOpenFileWrite(const char *filename);
long FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp);
void FS_SV_Rename(const char *from, const char *to);
long FS_FOpenFileRead(const char *qpath, fileHandle_t *file,
		qbool uniqueFILE);
int FS_FileIsInPAK(const char *filename, int *pChecksum);
int FS_Write(const void *buffer, int len, fileHandle_t f);
int FS_Read2(void *buffer, int len, fileHandle_t f);
int FS_Read(void *buffer, int len, fileHandle_t f);
void FS_FCloseFile(fileHandle_t f);
long FS_ReadFileDir(const char *qpath, void *searchPath, qbool unpure,
		void **buffer);
long FS_ReadFile(const char *qpath, void **buffer);
void FS_ForceFlush(fileHandle_t f);
void FS_FreeFile(void *buffer);
void FS_WriteFile(const char *qpath, const void *buffer, int size);
long FS_filelength(fileHandle_t f);
int FS_FTell(fileHandle_t f);
void FS_Flush(fileHandle_t f);
void QDECL FS_Printf(fileHandle_t f, const char *fmt,
			...) __attribute__ ((format (printf, 2, 3)));
int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f,
 			 fsMode_t mode);
int FS_Seek(fileHandle_t f, long offset, int origin);
qbool FS_FilenameCompare(const char *s1, const char *s2);
const char* FS_LoadedPakNames(void);
const char* FS_LoadedPakChecksums(void);
const char* FS_LoadedPakPureChecksums(void);
const char* FS_ReferencedPakNames(void);
const char* FS_ReferencedPakChecksums(void);
const char* FS_ReferencedPakPureChecksums(void);
void 	FS_ClearPakReferences(int flags);
void 	FS_PureServerSetReferencedPaks(const char *pakSums, const char *pakNames);
void 	FS_PureServerSetLoadedPaks(const char *pakSums, const char *pakNames);
qbool FS_CheckDirTraversal(const char *checkdir);
qbool FS_idPak(char *pak, char *base, int numPaks);
qbool FS_ComparePaks(char *neededpaks, int len, qbool dlstring);
void 	FS_Rename(const char *from, const char *to);
void 	FS_Remove(const char *osPath);
void 	FS_HomeRemove(const char *homePath);
void 	FS_FilenameCompletion(const char *dir, const char *ext,
		qbool stripExt, void (*callback)(const char *s),
		qbool allowNonPureFilesOnDisk);
const char* FS_GetCurrentGameDir(void);
qbool FS_Which(const char *filename, void *searchPath);

/*
 * Edit fields and command line history/completion
 */
 
 typedef struct field_s field_t;

enum { MAX_EDIT_LINE = 256 };
struct field_s {
	int	cursor;
	int	scroll;
	int	widthInChars;
	char	buffer[MAX_EDIT_LINE];
};

void Field_Clear(field_t *edit);
void Field_AutoComplete(field_t *edit);
void Field_CompleteKeyname(void);
void Field_CompleteFilename(const char *dir,
	 const char *ext, qbool stripExt,
			 qbool allowNonPureFilesOnDisk);
void Field_CompleteCommand(char *cmd,
	 qbool doCommands, qbool doCvars);


/*
 * MISC
 */

/* centralizing the declarations for cl_cdkey
 * https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=470 */
extern char cl_cdkey[34];

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
} cpuFeatures_t;

/* centralized and cleaned, that's the max string you can send to a Q_Printf / Q_DPrintf (above gets truncated) */
#define MAXPRINTMSG 4096


typedef enum {
	/* SE_NONE must be zero */
	SE_NONE = 0,		/* evTime is still valid */
	SE_KEY,			/* evValue is a key code, evValue2 is the down flag */
	SE_CHAR,		/* evValue is an ascii char */
	SE_MOUSE,		/* evValue and evValue2 are reletive signed x / y moves */
	SE_JOYSTICK_AXIS,	/* evValue is an axis number and evValue2 is the current state (-127 to 127) */
	SE_CONSOLE		/* evPtr is a char* */
} sysEventType_t;

typedef struct {
	int		evTime;
	sysEventType_t	evType;
	int		evValue, evValue2;
	int		evPtrLength;	/* bytes of data pointed to by evPtr, for journaling */
	void		*evPtr;		/* this must be manually freed if not NULL */
} sysEvent_t;

void 	Q_QueueEvent(int time, sysEventType_t type, int value,
		int value2, int ptrLength,
		void *ptr);
int 	Q_EventLoop(void);
sysEvent_t Q_GetSystemEvent(void);
char* CopyString(const char *in);
void 	Info_Print(const char *s);

void 	Q_BeginRedirect(char *buffer, int buffersize, void (*flush)(char *));
void 	Q_EndRedirect(void);
void QDECL Q_Printf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void QDECL Q_DPrintf(const char *fmt,
			...) __attribute__ ((format (printf, 1, 2)));
void QDECL Q_Error(int code, const char *fmt,
			...) __attribute__ ((noreturn, format(printf, 2, 3)));
void 	Q_Quit_f(void) __attribute__ ((noreturn));
void 	Q_GameRestart(int checksumFeed, qbool disconnect);
int 	Q_Milliseconds(void);	/* will be journaled properly */
unsigned Q_BlockChecksum(const void *buffer, int length);
char* Q_MD5File(const char *filename, int length,
		 const char *prefix,
		 int prefix_len);
int 	Q_Filter(char *filter, char *name, int casesensitive);
int 	Q_FilterPath(char *filter, char *name,
					int casesensitive);
int 	Q_RealTime(qtime_t *qtime);
qbool Q_SafeMode(void);
void 	Q_RunAndTimeServerPacket(netadr_t *evFrom, msg_t *buf);
qbool Q_IsVoipTarget(uint8_t *voipTargets, int voipTargetsSize,
			int clientNum);
void 	Q_StartupVariable(const char *match);

extern cvar_t	*com_developer;
extern cvar_t	*com_dedicated;
extern cvar_t	*com_speeds;
extern cvar_t	*com_timescale;
extern cvar_t	*com_sv_running;
extern cvar_t	*com_cl_running;
extern cvar_t	*com_version;
extern cvar_t	*com_blood;
extern cvar_t	*com_buildScript;	/* for building release pak files */
extern cvar_t	*com_journal;
extern cvar_t	*com_cameraMode;
extern cvar_t	*com_ansiColor;
extern cvar_t	*com_unfocused;
extern cvar_t	*com_maxfpsUnfocused;
extern cvar_t	*com_minimized;
extern cvar_t	*com_maxfpsMinimized;
extern cvar_t	*com_altivec;
extern cvar_t	*com_standalone;
extern cvar_t	*com_basegame;
extern cvar_t	*com_homepath;

/* both client and server must agree to pause */
extern cvar_t	*cl_paused;
extern cvar_t	*sv_paused;

extern cvar_t	*cl_packetdelay;
extern cvar_t	*sv_packetdelay;

extern cvar_t	*com_gamename;
extern cvar_t	*com_protocol;
#ifdef LEGACY_PROTOCOL
extern cvar_t	*com_legacyprotocol;
#endif

/* com_speeds times */
extern int	time_game;
extern int	time_frontend;
extern int	time_backend;	/* renderer backend time */

extern int	com_frameTime;

extern qbool com_errorEntered;
extern qbool com_fullyInitialized;

extern fileHandle_t	com_journalFile;
extern fileHandle_t	com_journalDataFile;

typedef enum {
	TAG_FREE,
	TAG_GENERAL,
	TAG_BOTLIB,
	TAG_RENDERER,
	TAG_SMALL,
	TAG_STATIC
} memtag_t;

#if defined(_DEBUG) && !defined(BSPC)
	#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define	Z_TagMalloc(size, tag)	Z_TagMallocDebug(size, tag, # size, __FILE__, \
	__LINE__)
#define	Z_Malloc(size)		Z_MallocDebug(size, # size, __FILE__, __LINE__)
#define	S_Malloc(size)		S_MallocDebug(size, # size, __FILE__, __LINE__)
void* Z_TagMallocDebug(int size, int tag, char *label, char *file, int line);	/* NOT 0 filled memory */
void* Z_MallocDebug(int size, char *label, char *file, int line);		/* returns 0 filled memory */
void* S_MallocDebug(int size, char *label, char *file, int line);		/* returns 0 filled memory */
#else
void* Z_TagMalloc(int size, int tag);	/* NOT 0 filled memory */
void* Z_Malloc(int size);		/* returns 0 filled memory */
void* S_Malloc(int size);		/* NOT 0 filled memory only for small allocations */
#endif
void 	Z_Free(void *ptr);
void 	Z_FreeTags(int tag);
int 	Z_AvailableMemory(void);
void 	Z_LogHeap(void);

void 	Hunk_Clear(void);
void 	Hunk_ClearToMark(void);
void 	Hunk_SetMark(void);
qbool Hunk_CheckMark(void);
void 	Hunk_ClearTempMemory(void);
void* Hunk_AllocateTempMemory(int size);
void 	Hunk_FreeTempMemory(void *buf);
int 	Hunk_MemoryRemaining(void);
void 	Hunk_Log(void);
void 	Hunk_Trash(void);

void 	Q_TouchMemory(void);

void Q_Init(char *commandLine);
void Q_Frame(void);
void Q_Shutdown(void);


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
void CL_PacketEvent(netadr_t from, msg_t *msg);
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
/* bring up the "need a cd to play" dialog */
void CL_CDDialog(void);
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
void Key_WriteBindings(fileHandle_t f);
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
void SV_PacketEvent(netadr_t from, msg_t *msg);
int SV_FrameMsec(void);
qbool SV_GameCommand(void);
int SV_SendQueuedPackets(void);

/*
 * UI interface
 */
qbool UI_GameCommand(void);
qbool UI_usesUniqueCDKey(void);


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
void 	Sys_SnapVector(float *v);
qbool Sys_RandomBytes(byte *string, int len);
/* the system console is shown when a dedicated server is running */
void 	Sys_DisplaySystemConsole(qbool show);
cpuFeatures_t Sys_GetProcessorFeatures(void);
void 	Sys_SetErrorText(const char *text);
void 	Sys_SendPacket(int length, const void *data, netadr_t to);
/* Does NOT parse port numbers, only base addresses. */
qbool Sys_StringToAdr(const char *s, netadr_t *a, netadrtype_t family);
qbool Sys_IsLANAddress(netadr_t adr);
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
qbool Sys_WritePIDFile(void);


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
} node_t;

#define HMAX 256	/* Maximum symbol */

typedef struct {
	int	blocNode;
	int	blocPtrs;

	node_t	* tree;
	node_t	* lhead;
	node_t	* ltail;
	node_t	* loc[HMAX+1];
	node_t	** freelist;

	node_t	nodeList[768];
	node_t	* nodePtrs[768];
} huff_t;

typedef struct {
	huff_t	compressor;
	huff_t	decompressor;
} huffman_t;

void Huff_Compress(msg_t *buf, int offset);
void Huff_Decompress(msg_t *buf, int offset);
void Huff_Init(huffman_t *huff);
void Huff_addRef(huff_t* huff, byte ch);
int Huff_Receive(node_t *node, int *ch, byte *fin);
void Huff_transmit(huff_t *huff, int ch, byte *fout);
void Huff_offsetReceive(node_t *node, int *ch, byte *fin, int *offset);
void Huff_offsetTransmit(huff_t *huff, int ch, byte *fout, int *offset);
void Huff_putBit(int bit, byte *fout, int *offset);
int Huff_getBit(byte *fout, int *offset);

/* don't use if you don't know what you're doing. */
int Huff_getBloc(void);
void Huff_setBloc(int _bloc);

extern huffman_t clientHuffTables;

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
