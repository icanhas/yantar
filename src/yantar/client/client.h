/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "shared.h"
#include "common.h"
#include "ref.h"
#include "paths.h"
#include "snd.h"
#include "cgame.h"
#include "bg.h"

#ifdef USE_CURL
#include "clcurl.h"
#endif	/* USE_CURL */

#ifdef USE_VOIP
#include "speex/speex.h"
#include "speex/speex_preprocess.h"
#endif

#define QKEY_FILE	"qkey"	/* file full of random crap that gets
						 * used to create cl_guid */
enum {
	QKEY_SIZE	= 2048,
	/* time between connection packet retransmits */
	RETRANSMIT_TIMEOUT	= 3000,
	/*
	 * the parseEntities array must be large enough to hold PACKET_BACKUP frames of
	 * entities, so that when a delta compressed message arives from the server
	 * it can be un-delta'd from the original
	 */
	MAX_PARSE_ENTITIES	= 2048,

	MAX_TIMEDEMO_DURATIONS	= 4096
};

/*
 * The Clientactive structure is wiped completely at every
 * new Gamestate, potentially several times during an established connection.
 *
 * The Clientconn structure is wiped when disconnecting from a server,
 * either to go to a full screen console, play a demo, or connect to a different server.
 * A connection can be to either a server through the network layer or a
 * demo through a file.
 *
 * The Clientstatic structure is never wiped, and is used even when
 * no client connection is active at all.
 */
typedef struct Clsnapshot Clsnapshot;
typedef struct Outpacket Outpacket;
typedef struct Clientactive Clientactive;
typedef struct Clientconn Clientconn;
typedef struct Ping Ping;
typedef struct Clientstatic Clientstatic;
extern Clientactive cl;
extern Clientconn clc;
extern Clientstatic cls;

extern int g_console_field_width;
 
/* snapshots are a view of the server at a given time */
struct Clsnapshot {
	qbool	valid;	/* cleared if delta parsing was invalid */
	int		snapFlags;	/* rate delayed and dropped commands */

	int		serverTime;	/* server time the message is valid for (in msec) */

	int		messageNum;	/* copied from netchan->incoming_sequence */
	int		deltaNum;	/* messageNum the delta is from */
	int		ping;			/* time from when cmdNum-1 was sent to time packet was received */
	byte		areamask[MAX_MAP_AREA_BYTES];	/* portalarea visibility bits */

	int		cmdNum;	/* the next cmdNum the server is expecting */
	Playerstate	ps;	/* complete information about the current player at this time */

	int		numEntities;		/* all of the entities that need to be presented */
	int		parseEntitiesNum;	/* at the time of this snapshot */

	int		serverCommandNum;	/* execute all commands up to this before
								 * making the snapshot current */
};

struct Outpacket {
	int	p_cmdNumber;	/* cl.cmdNumber when packet was sent */
	int	p_serverTime;	/* usercmd->serverTime when packet was sent */
	int	p_simtime;	/* cls.simtime when packet was sent */
};

struct Clientactive {
	int timeoutcount;	/* it requres several frames in a timeout condition */
	/* to disconnect, preventing debugging breaks from
	 * causing immediate disconnects on continue */
	Clsnapshot	snap;	/* latest received from server */

	int		serverTime;		/* may be paused during play */
	int		oldServerTime;		/* to prevent time from flowing bakcwards */
	int		oldFrameServerTime;	/* to check tournament restarts */
	int		serverTimeDelta;	/* cl.serverTime = cls.simtime + cl.serverTimeDelta */
	/* this value changes as net lag varies */
	qbool		extrapolatedSnapshot;	/* set if any cgame frame has been forced to extrapolate */
	/* cleared when CL_AdjustTimeDelta looks at it */
	qbool		newSnapshots;	/* set on parse of any valid packet */

	Gamestate	gameState;		/* configstrings */
	char		mapname[MAX_QPATH];	/* extracted from CS_SERVERINFO */

	int		parseEntitiesNum;	/* index (not anded off) into cl_parse_entities[] */

	int		mouseDx[2], mouseDy[2];	/* added to by mouse events */
	int		mouseIndex;
	int		joystickAxis[MAX_JOYSTICK_AXIS];	/* set by joystick events */

	/* cgame communicates a few values to the client system */
	int	cgameweapsel[WSnumslots];	/* current weapons to add to Usrcmd */
	float	cgameSensitivity;

	/* cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	 * properly generated command */
	Usrcmd	cmds[CMD_BACKUP];	/* each mesage will send several old cmds */
	int		cmdNumber;		/* incremented each frame, because multiple */
	/* frames may need to be packed into a single packet */

	Outpacket outPackets[PACKET_BACKUP];	/* information about each packet we have sent out */

	/* the client maintains its own idea of view angles, which are
	 * sent to the server each frame.  It is cleared to 0 upon entering each level.
	 * the server sends a delta each frame which is added to the locally
	 * tracked view angles to account for standing on rotating objects,
	 * and teleport direction changes */
	Vec3	viewangles;

	int	serverId;	/* included in each client message so the server */
	/* can tell if it is for a prior map_restart
	 * big stuff at end of structure so most offsets are 15 bits or less */
	Clsnapshot	snapshots[PACKET_BACKUP];

	Entstate	entityBaselines[MAX_GENTITIES];	/* for delta compression when not in previous frame */

	Entstate	parseEntities[MAX_PARSE_ENTITIES];
};

struct Clientconn {
	Connstate	state;	/* connection status */

	int		clientNum;
	int		lastPacketSentTime;	/* for retransmits during connection */
	int		lastPacketTime;		/* for timeouts */

	char		servername[MAX_OSPATH];	/* name of server from original connect (used by reconnect) */
	Netaddr	serverAddress;
	int		connectTime;				/* for connection retransmits */
	int		connectPacketCount;			/* for display on connection dialog */
	char		serverMessage[MAX_STRING_TOKENS];	/* for display on connection dialog */

	int		challenge;	/* from the server to use for connecting */
	int		checksumFeed;	/* from the server for checksum calculations */

	/* these are our reliable messages that go to the server */
	int	reliableSequence;
	int	reliableAcknowledge;	/* the last one the server has executed */
	char	reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	/* server message (unreliable) and command (reliable) sequence
	 * numbers are NOT cleared at level changes, but continue to
	 * increase as long as the connection is valid */

	/* message sequence is used by both the network layer and the
	 * delta compression layer */
	int serverMessageSequence;

	/* reliable messages received from server */
	int	serverCommandSequence;
	int	lastExecutedServerCommand;	/* last server command grabbed or executed with CL_GetServerCommand */
	char	serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	/* file transfer from server */
	Fhandle	download;
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];
#ifdef USE_CURL
	qbool		cURLEnabled;
	qbool		cURLUsed;
	qbool		cURLDisconnected;
	char		downloadURL[MAX_OSPATH];
	CURL		*downloadCURL;
	CURLM		*downloadCURLM;
#endif	/* USE_CURL */
	int		sv_allowDownload;
	char		sv_dlURL[MAX_CVAR_VALUE_STRING];
	int		downloadNumber;
	int		downloadBlock;			/* block we are waiting for */
	int		downloadCount;			/* how many bytes we got */
	int		downloadSize;			/* how many bytes we got */
	char		downloadList[MAX_INFO_STRING];	/* list of paks we need to download */
	qbool		downloadRestart;		/* if true, we need to do another fsrestart because we downloaded a pak */

	/* demo information */
	char		demoName[MAX_QPATH];
	qbool		spDemoRecording;
	qbool		demorecording;
	qbool		demoplaying;
	qbool		demowaiting;	/* don't record until a non-delta message is received */
	qbool		firstDemoFrameSkipped;
	Fhandle	demofile;

	int		timeDemoFrames;					/* counter of rendered frames */
	int		timeDemoStart;					/* cls.simtime before first frame */
	int		timeDemoBaseTime;				/* each frame will be at this time + frameNum * 50 */
	int		timeDemoLastFrame;				/* time the last frame was rendered */
	int		timeDemoMinDuration;				/* minimum frame duration */
	int		timeDemoMaxDuration;				/* maximum frame duration */
	unsigned char	timeDemoDurations[ MAX_TIMEDEMO_DURATIONS ];	/* log of frame durations */

#ifdef USE_VOIP
	qbool		voipEnabled;
	qbool		speexInitialized;
	int		speexFrameSize;
	int		speexSampleRate;

	/* incoming data...
	 * !!! FIXME: convert from parallel arrays to array of a struct. */
	SpeexBits	speexDecoderBits[MAX_CLIENTS];
	void		*speexDecoder[MAX_CLIENTS];
	byte		voipIncomingGeneration[MAX_CLIENTS];
	int		voipIncomingSequence[MAX_CLIENTS];
	float		voipGain[MAX_CLIENTS];
	qbool		voipIgnore[MAX_CLIENTS];
	qbool		voipMuteAll;

	/* outgoing data...
	 * if voipTargets[i / 8] & (1 << (i % 8)),
	 * then we are sending to clientnum i. */
	uint8_t			voipTargets[(MAX_CLIENTS + 7) / 8];
	uint8_t			voipFlags;
	SpeexPreprocessState	*speexPreprocessor;
	SpeexBits		speexEncoderBits;
	void			*speexEncoder;
	int			voipOutgoingDataSize;
	int			voipOutgoingDataFrames;
	int			voipOutgoingSequence;
	byte			voipOutgoingGeneration;
	byte			voipOutgoingData[1024];
	float			voipPower;
#endif
	/* big stuff at end of structure so most offsets are 15 bits or less */
	Netchan netchan;
};

struct Ping {
	Netaddr	adr;
	int		start;
	int		time;
	char		info[MAX_INFO_STRING];
};

typedef struct {
	Netaddr	adr;
	char		hostName[MAX_NAME_LENGTH];
	char		mapName[MAX_NAME_LENGTH];
	char		game[MAX_NAME_LENGTH];
	int		netType;
	int		gameType;
	int		clients;
	int		maxClients;
	int		minPing;
	int		maxPing;
	int		ping;
	qbool		visible;
	int		g_humanplayers;
	int		g_needpass;
} Servinfo;

struct Clientstatic {
	/* when the server clears the hunk, all of these must be restarted */
	qbool		rendererStarted;
	qbool		soundStarted;
	qbool		soundRegistered;
	qbool		uiStarted;
	qbool		cgameStarted;

	int		framecount;
	int		realtime;	/* IRL time */
	int		realframetime;	/* msec since last frame */

	int		simtime;	/* may be different from realtime due to timescale, ignores pause */
	int		simframetime;	/* ignoring pause, so console always works */

	int		numlocalservers;
	Servinfo	localServers[MAX_OTHER_SERVERS];

	int		numglobalservers;
	Servinfo	globalServers[MAX_GLOBAL_SERVERS];
	/* additional global servers */
	int		numGlobalServerAddresses;
	Netaddr	globalServerAddresses[MAX_GLOBAL_SERVERS];

	int		numfavoriteservers;
	Servinfo	favoriteServers[MAX_OTHER_SERVERS];

	int		pingUpdateSource;	/* source currently pinging or updating */

	char		oldGame[MAX_QPATH];
	qbool		oldGameSet;

	/* update server info */
	Netaddr	updateServer;
	char		updateChallenge[MAX_TOKEN_CHARS];
	char		updateInfoString[MAX_INFO_STRING];

	Netaddr	authorizeServer;

	/* rendering info */
	Glconfig	glconfig;
	Handle		charSetShader;
	Fontinfo	consolefont;
	Handle		whiteShader;
	Handle		consoleShader;
};

extern Vm *cgvm;		/* interface to cgame dll or vm */
extern Vm *uivm;		/* interface to ui dll or vm */
extern Refexport re;	/* interface to refresh .dll */

extern Cvar *cl_nodelta;
extern Cvar *cl_debugMove;
extern Cvar *cl_noprint;
extern Cvar *cl_timegraph;
extern Cvar *cl_maxpackets;
extern Cvar *cl_packetdup;
extern Cvar *cl_shownet;
extern Cvar *cl_showSend;
extern Cvar *cl_timeNudge;
extern Cvar *cl_showTimeDelta;
extern Cvar *cl_freezeDemo;

extern Cvar *cl_yawspeed;
extern Cvar *cl_pitchspeed;
extern Cvar *cl_rollspeed;
extern Cvar *cl_anglespeedkey;

extern Cvar *cl_sensitivity;

extern Cvar *cl_mouseAccel;
extern Cvar *cl_mouseAccelOffset;
extern Cvar *cl_mouseAccelStyle;
extern Cvar *cl_showMouseRate;

extern Cvar *m_pitch;
extern Cvar *m_yaw;
extern Cvar *m_forward;
extern Cvar *m_side;
extern Cvar *m_filter;

extern Cvar *j_pitch;
extern Cvar *j_yaw;
extern Cvar *j_forward;
extern Cvar *j_side;
extern Cvar *j_up;
extern Cvar *j_pitch_axis;
extern Cvar *j_yaw_axis;
extern Cvar *j_forward_axis;
extern Cvar *j_side_axis;
extern Cvar *j_up_axis;

extern Cvar *cl_timedemo;
extern Cvar *cl_aviFrameRate;
extern Cvar *cl_aviMotionJpeg;

extern Cvar *cl_activeAction;

extern Cvar *cl_allowDownload;
extern Cvar *cl_downloadMethod;
extern Cvar *cl_conXOffset;
extern Cvar *cl_inGameVideo;

extern Cvar *cl_lanForcePackets;
extern Cvar *cl_autoRecordDemo;

extern Cvar *cl_consoleKeys;

#ifdef USE_MUMBLE
extern Cvar *cl_useMumble;
extern Cvar *cl_mumbleScale;
#endif

#ifdef USE_VOIP
/* cl_voipSendTarget is a string: "all" to broadcast to everyone, "none" to
 *  send to no one, or a comma-separated list of client numbers:
 *  "0,7,2,23" ... an empty string is treated like "all". */
extern Cvar *cl_voipUseVAD;
extern Cvar *cl_voipVADThreshold;
extern Cvar *cl_voipSend;
extern Cvar *cl_voipSendTarget;
extern Cvar *cl_voipGainDuringCapture;
extern Cvar *cl_voipCaptureMult;
extern Cvar *cl_voipShowMeter;
extern Cvar *cl_voip;
#endif

/*
 * cl_main.c
 */
void	clinit(void);
void	CL_AddReliableCommand(const char *cmd, qbool isDisconnectCmd);
void	clstarthunkusers(qbool rendererOnly);
void	cldisconnect_f(void);
void	CL_GetChallengePacket(void);
void	CL_Vid_Restart_f(void);
void	CL_Snd_Restart_f(void);
void	CL_StartDemoLoop(void);
void	CL_NextDemo(void);
void	CL_ReadDemoMessage(void);
void	CL_StopRecord_f(void);
void	clinitDownloads(void);
void	CL_NextDownload(void);
void	CL_GetPing(int n, char *buf, int buflen, int *pingtime);
void	CL_GetPingInfo(int n, char *buf, int buflen);
void	CL_ClearPing(int n);
int	CL_GetPingQueueCount(void);
void	clshutdownRef(void);
void	clinitref(void);
int	CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen);
qbool	CL_CheckPaused(void);

/*
 * cl_input.c
 */
typedef struct {
	int		down[2];	/* key nums holding it down */
	unsigned	downtime;	/* msec timestamp */
	unsigned	msec;		/* msec down this frame if both a down and up happened */
	qbool	active;		/* current state */
	qbool	wasPressed;	/* set when down, not cleared when up */
} Kbutton;

void	clinitInput(void);
void	clshutdownInput(void);
void	CL_SendCmd(void);
void	CL_ClearState(void);
void	CL_WritePacket(void);

/*
 * cl_keys.c
 */
int		Key_StringToKeynum(char *str);
char*	Key_KeynumToString(int keynum);

/*
 * cl_parse.c
 */
extern int	cl_connectedToPureServer;
extern int	cl_connectedToCheatServer;

#ifdef USE_VOIP
void	CL_Voip_f(void);
#endif

void	CL_SystemInfoChanged(void);
void	CL_ParseServerMessage(Bitmsg	*msg);

void	CL_ServerInfoPacket(Netaddr	from, Bitmsg *msg);
void	CL_LocalServers_f(void);
void	CL_GlobalServers_f(void);
void	CL_FavoriteServers_f(void);
void	CL_Ping_f(void);
qbool	CL_UpdateVisiblePings_f(int source);

/*
 * console
 */
void	Con_DrawCharacter(int cx, int line, int num);
void	Con_CheckResize(void);
void	Con_Init(void);
void	Con_Shutdown(void);
void	Con_Clear_f(void);
void	Con_ToggleConsole_f(void);
void	Con_DrawNotify(void);
void	Con_ClearNotify(void);
void	Con_RunConsole(void);
void	Con_DrawConsole(void);
void	Con_PageUp(void);
void	Con_PageDown(void);
void	Con_Top(void);
void	Con_Bottom(void);
void	Con_Close(void);
void	CL_LoadConsoleHistory(void);
void	CL_SaveConsoleHistory(void);

/*
 * cl_scrn.c
 */
void	SCR_Init(void);
void	SCR_UpdateScreen(void);
void	scrdebuggraph(float value);
int	SCR_GetBigStringWidth(const	char *str);	/* returns in virtual 640x480 coordinates */
void	SCR_AdjustFrom640(float *x, float *y, float *w, float *h);
void	SCR_FillRect(float x, float y, float width, float height,
			const float *color);
void	SCR_DrawPic(float x, float y, float width, float height,
			Handle hShader);
void	SCR_DrawNamedPic(float x, float y, float width, float height,
			const char *picname);
void	SCR_DrawBigString(int x, int y, const char *s, float alpha,
			qbool noColorEscape);	/* draws a string with embedded color control characters with fade */
void	SCR_DrawBigStringColor(int x, int y, const char *s, Vec4 color,
			qbool noColorEscape);	/* ignores embedded color control characters */
void	SCR_DrawSmallStringExt(int x, int y, const char *string, float *setColor,
			qbool forceColor, qbool noColorEscape);
void	SCR_DrawSmallChar(int x, int y, int ch);

/*
 * cl_cin.c
 */
void	CL_PlayCinematic_f(void);
void	SCR_DrawCinematic(void);
void	SCR_RunCinematic(void);
void	SCR_StopCinematic(void);
int	CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width,
		int height, int bits);
Cinstatus	CIN_StopCinematic(int handle);
Cinstatus	CIN_RunCinematic(int handle);
void	CIN_DrawCinematic(int handle);
void	CIN_SetExtents(int handle, int x, int y, int w, int h);
void	CIN_SetLooping(int handle, qbool loop);
void	CIN_UploadCinematic(int handle);
void	CIN_CloseAllVideos(void);

/*
 * cl_cgame.c
 */
void clinitCGame(void);
void clshutdownCGame(void);
qbool clgamecmd(void);
void CL_CGameRendering(Stereoframe stereo);
void CL_SetCGameTime(void);
void CL_FirstSnapshot(void);
void CL_ShaderStateChanged(void);

/*
 * cl_ui.c
 */
void clinitUI(void);
void clshutdownUI(void);
int Key_GetCatcher(void);
void Key_SetCatcher(int catcher);
void LAN_LoadCachedServers(void);
void LAN_SaveServersToCache(void);

/*
 * cl_net_chan.c
 */
void		CL_Netchan_Transmit(Netchan *chan, Bitmsg* msg);	/* int length, const byte *data ); */
qbool	CL_Netchan_Process(Netchan *chan, Bitmsg *msg);

/*
 * cl_avi.c
 */
qbool CL_OpenAVIForWriting(const char *filename);
void CL_TakeVideoFrame(void);
void CL_WriteAVIVideoFrame(const byte *imageBuffer, int size);
void CL_WriteAVIAudioFrame(const byte *pcmBuffer, int size);
qbool CL_CloseAVI(void);
qbool CL_VideoRecording(void);

/*
 * cl_main.c
 */
void CL_WriteDemoMessage(Bitmsg *msg, int headerBytes);
