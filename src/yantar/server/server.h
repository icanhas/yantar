/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* server.h */

#include "shared.h"
#include "common.h"
#include "game.h"
#include "bg.h"

/* ============================================================================= */

#define PERS_SCORE 0	/* !!! MUST NOT CHANGE, SERVER AND */
/* GAME BOTH REFERENCE !!! */

#define MAX_ENT_CLUSTERS 16

#ifdef USE_VOIP
#define VOIP_QUEUE_LENGTH 64

typedef struct voipServerPacket_s {
	int	generation;
	int	sequence;
	int	frames;
	int	len;
	int	sender;
	int	flags;
	byte	data[1024];
} voipServerPacket_t;
#endif

typedef struct Svent {
	struct worldSector_s	*worldSector;
	struct Svent	*nextEntityInWorldSector;

	Entstate		baseline;	/* for delta compression of initial sighting */
	int			numClusters;	/* if -1, use headnode instead */
	int			clusternums[MAX_ENT_CLUSTERS];
	int			lastCluster;	/* if all the clusters don't fit in clusternums */
	int			areanum, areanum2;
	int			snapshotCounter;	/* used to prevent double adding from portal views */
} Svent;

typedef enum {
	SS_DEAD,	/* no map loaded */
	SS_LOADING,	/* spawning level entities */
	SS_GAME		/* actively running */
} serverState_t;

typedef struct {
	serverState_t	state;
	qbool		restarting;		/* if true, send configstring changes during SS_LOADING */
	int		serverId;		/* changes each server start */
	int		restartedServerId;	/* serverId before a map_restart */
	int		checksumFeed;		/* the feed key that we use to compute the pure checksum strings */
	/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
	 * the serverId associated with the current checksumFeed (always <= serverId) */
	int		checksumFeedServerId;
	int		snapshotCounter;	/* incremented for each snapshot built */
	int		timeResidual;		/* <= 1000 / sv_frame->value */
	int		nextFrameTime;		/* when time > nextFrameTime, process world */
	struct Cmodel *models[MAX_MODELS];
	char		*configstrings[MAX_CONFIGSTRINGS];
	Svent				svEntities[MAX_GENTITIES];

	char		*entityParsePoint;	/* used during game VM init */

	/* the game virtual machine will update these on init and changes */
	Sharedent	*gentities;
	int		gentitySize;
	int		num_entities;	/* current number, <= MAX_GENTITIES */

	Playerstate	*gameClients;
	int		gameClientSize;	/* will be > sizeof(Playerstate) due to game private data */

	int		restartTime;
	int		time;
} Server;





typedef struct {
	int		areabytes;
	byte		areabits[MAX_MAP_AREA_BYTES];	/* portalarea visibility bits */
	Playerstate	ps;
	int		num_entities;
	int		first_entity;	/* into the circular sv_packet_entities[] */
	/* the entities MUST be in increasing state number
	 * order, otherwise the delta compression will fail */
	int	messageSent;	/* time the message was transmitted */
	int	messageAcked;	/* time the message was acked */
	int	messageSize;	/* used to rate drop packets */
} clientSnapshot_t;

typedef enum {
	CS_FREE,	/* can be reused for a new connection */
	CS_ZOMBIE,	/* client has been disconnected, but don't reuse */
	/* connection for a couple seconds */
	CS_CONNECTED,	/* has been assigned to a Client, but no gamestate yet */
	CS_PRIMED,	/* gamestate has been sent, but client hasn't sent a usercmd */
	CS_ACTIVE	/* client is fully in game */
} clientState_t;

typedef struct netchan_buffer_s {
	Bitmsg	msg;
	byte	msgBuffer[MAX_MSGLEN];
	struct netchan_buffer_s *next;
} netchan_buffer_t;

typedef struct Client {
	clientState_t	state;
	char		userinfo[MAX_INFO_STRING];	/* name, etc */

	char		reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS
	];
	int		reliableSequence;	/* last added reliable message, not necesarily sent or acknowledged yet */
	int		reliableAcknowledge;	/* last acknowledged reliable message */
	int		reliableSent;		/* last sent reliable message, not necesarily acknowledged yet */
	int		messageAcknowledge;

	int		gamestateMessageNum;	/* netchan->outgoingSequence of gamestate */
	int		challenge;

	Usrcmd	lastUsercmd;
	int		lastMessageNum;		/* for delta compression */
	int		lastClientCommand;	/* reliable client message sequence */
	char		lastClientCommandString[MAX_STRING_CHARS];
	Sharedent	*gentity;		/* SV_GentityNum(clientnum) */
	char		name[MAX_NAME_LENGTH];	/* extracted from userinfo, high bits masked */

	/* downloading */
	char			downloadName[MAX_QPATH];		/* if not empty string, we are downloading */
	Fhandle		download;				/* file being downloaded */
	int			downloadSize;				/* total bytes (can't use EOF because of paks) */
	int			downloadCount;				/* bytes sent */
	int			downloadClientBlock;			/* last block we sent to the client, awaiting ack */
	int			downloadCurrentBlock;			/* current block number */
	int			downloadXmitBlock;			/* last block we xmited */
	unsigned char		*downloadBlocks[MAX_DOWNLOAD_WINDOW];	/* the buffers for the download blocks */
	int			downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	qbool			downloadEOF;		/* We have sent the EOF block */
	int			downloadSendTime;	/* time we last got an ack from the client */

	int			deltaMessage;		/* frame last client usercmd message */
	int			nextReliableTime;	/* svs.time when another reliable command will be allowed */
	int			lastPacketTime;		/* svs.time when packet was last received */
	int			lastConnectTime;	/* svs.time when connection started */
	int			lastSnapshotTime;	/* svs.time of last sent snapshot */
	qbool			rateDelayed;		/* true if nextSnapshotTime was set based on rate instead of snapshotMsec */
	int			timeoutCount;		/* must timeout a few frames in a row so debugging doesn't break */
	clientSnapshot_t	frames[PACKET_BACKUP];	/* updates can be delta'd from here */
	int			ping;
	int			rate;		/* bytes / second */
	int			snapshotMsec;	/* requests a snapshot every snapshotMsec unless rate choked */
	int			pureAuthentic;
	qbool			gotCP;	/* TTimo - additional flag to distinguish between a bad pure checksum, and no cp command at all */
	Netchan		netchan;
	/* TTimo
	 * queuing outgoing fragmented messages to send them properly, without udp packet bursts
	 * in case large fragmented messages are stacking up
	 * buffer them into this queue, and hand them out to netchan as needed */
	netchan_buffer_t	*netchan_start_queue;
	netchan_buffer_t	**netchan_end_queue;

#ifdef USE_VOIP
	qbool		hasVoip;
	qbool		muteAllVoip;
	qbool		ignoreVoipFromClient[MAX_CLIENTS];
	voipServerPacket_t      *voipPacket[VOIP_QUEUE_LENGTH];
	int		queuedVoipPackets;
	int		queuedVoipIndex;
#endif

	int		oldServerTime;
	qbool		csUpdated[MAX_CONFIGSTRINGS+1];

} Client;

/* ============================================================================= */


/* MAX_CHALLENGES is made large to prevent a denial
 * of service attack that could cycle all of them
 * out before legitimate users connected */
#define MAX_CHALLENGES		2048
/* Allow a certain amount of challenges to have the same IP address
* to make it a bit harder to DOS one single IP address from connecting
* while not allowing a single ip to grab all challenge resources */
#define MAX_CHALLENGES_MULTI	(MAX_CHALLENGES / 2)

#define AUTHORIZE_TIMEOUT	5000

typedef struct {
	Netaddr	adr;
	int		challenge;
	int		clientChallenge;	/* challenge number coming from the client */
	int		time;			/* time the last packet was sent to the autherize server */
	int		pingTime;		/* time the challenge response was sent to client */
	int		firstTime;		/* time the adr was first used, for authorize timeout checks */
	qbool		wasrefused;
	qbool		connected;
} challenge_t;

/* this structure will be cleared only when the game dll changes */
typedef struct {
	qbool		initialized;	/* sv_init has completed */

	int		time;	/* will be strictly increasing across level changes */

	int		snapFlagServerBit;	/* ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer() */

	Client	*clients;		/* [sv_maxclients->integer]; */
	int		numSnapshotEntities;	/* sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES */
	int		nextSnapshotEntities;	/* next snapshotEntities to use */
	Entstate	*snapshotEntities;	/* [numSnapshotEntities] */
	int		nextHeartbeatTime;
	challenge_t	challenges[MAX_CHALLENGES];	/* to prevent invalid IPs from connecting */
	Netaddr	redirectAddress;		/* for rcon return messages */

	Netaddr	authorizeAddress;	/* for rcon return messages */
} serverStatic_t;

#define SERVER_MAXBANS 1024
/* Structure for managing bans */
typedef struct {
	Netaddr	ip;
	/* For a CIDR-Notation type suffix */
	int		subnet;

	qbool		isexception;
} serverBan_t;

/* ============================================================================= */

extern serverStatic_t svs;	/* persistant server info across maps */
extern Server sv;		/* cleared each map */
extern Vm	*gvm;		/* game virtual machine */

extern Cvar	*sv_fps;
extern Cvar	*sv_timeout;
extern Cvar	*sv_zombietime;
extern Cvar	*sv_rconPassword;
extern Cvar	*sv_privatePassword;
extern Cvar	*sv_allowDownload;
extern Cvar	*sv_maxclients;

extern Cvar	*sv_privateClients;
extern Cvar	*sv_hostname;
extern Cvar	*sv_master[MAX_MASTER_SERVERS];
extern Cvar	*sv_reconnectlimit;
extern Cvar	*sv_showloss;
extern Cvar	*sv_padPackets;
extern Cvar	*sv_killserver;
extern Cvar	*sv_mapname;
extern Cvar	*sv_mapChecksum;
extern Cvar	*sv_serverid;
extern Cvar	*sv_minRate;
extern Cvar	*sv_maxRate;
extern Cvar	*sv_dlRate;
extern Cvar	*sv_minPing;
extern Cvar	*sv_maxPing;
extern Cvar	*sv_gametype;
extern Cvar	*sv_pure;
extern Cvar	*sv_floodProtect;
extern Cvar	*sv_lanForceRate;
#ifndef STANDALONE
extern Cvar	*sv_strictAuth;
#endif
extern Cvar	*sv_banFile;

extern serverBan_t serverBans[SERVER_MAXBANS];
extern uint serverBansCount;

#ifdef USE_VOIP
extern Cvar *sv_voip;
#endif


/* =========================================================== */

/*
 * sv_main.c
 *  */
void SV_FinalMessage(char *message);
void QDECL SV_SendServerCommand(Client *cl, const char *fmt,
				...) __attribute__ ((format (printf, 2, 3)));


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterShutdown(void);
int SV_RateMsec(Client *client);



/*
 * sv_init.c
 *  */
void SV_SetConfigstring(int index, const char *val);
void SV_GetConfigstring(int index, char *buffer, int bufferSize);
void SV_UpdateConfigstrings(Client *client);

void SV_SetUserinfo(int index, const char *val);
void SV_GetUserinfo(int index, char *buffer, int bufferSize);

void SV_ChangeMaxClients(void);
void SV_SpawnServer(char *server, qbool killBots);



/*
 * sv_client.c
 *  */
void SV_GetChallenge(Netaddr from);

void SV_DirectConnect(Netaddr from);

#ifndef STANDALONE
void SV_AuthorizeIpPacket(Netaddr from);
#endif

void SV_ExecuteClientMessage(Client *cl, Bitmsg *msg);
void SV_UserinfoChanged(Client *cl);

void SV_ClientEnterWorld(Client *client, Usrcmd *cmd);
void SV_FreeClient(Client *client);
void SV_DropClient(Client *drop, const char *reason);

void SV_ExecuteClientCommand(Client *cl, const char *s, qbool clientOK);
void SV_ClientThink(Client *cl, Usrcmd *cmd);

int SV_WriteDownloadToClient(Client *cl, Bitmsg *msg);
int SV_SendDownloadMessages(void);
int SV_SendQueuedMessages(void);


/*
 * sv_ccmds.c
 *  */
void SV_Heartbeat_f(void);

/*
 * sv_snapshot.c
 *  */
void SV_AddServerCommand(Client *client, const char *cmd);
void SV_UpdateServerCommandsToClient(Client *client, Bitmsg *msg);
void SV_WriteFrameToClient(Client *client, Bitmsg *msg);
void SV_SendMessageToClient(Bitmsg *msg, Client *client);
void SV_SendClientMessages(void);
void SV_SendClientSnapshot(Client *client);

/*
 * sv_game.c
 *  */
int     SV_NumForGentity(Sharedent *ent);
Sharedent*SV_GentityNum(int num);
Playerstate*SV_GameClientNum(int num);
Svent*SV_SvEntityForGentity(Sharedent *gEnt);
Sharedent*SV_GEntityForSvEntity(Svent *svEnt);
void            svinitGameProgs(void);
void            svshutdownGameProgs(void);
void            SV_RestartGameProgs(void);
qbool        SV_inPVS(const Vec3 p1, const Vec3 p2);

/*
 * sv_bot.c
 *  */
void            SV_BotFrame(int time);
int                     SV_BotAllocateClient(void);
void            SV_BotFreeClient(int clientNum);

void            SV_BotInitCvars(void);
int                     SV_BotLibSetup(void);
int                     SV_BotLibShutdown(void);
int                     SV_BotGetSnapshotEntity(int client, int ent);
int                     SV_BotGetConsoleMessage(int client, char *buf, int size);

int BotImport_DebugPolygonCreate(int color, int numPoints, Vec3 *points);
void BotImport_DebugPolygonDelete(int id);

void SV_BotInitBotLib(void);

/* ============================================================
 *
 * high level object sorting to reduce interaction tests
 *  */

void SV_ClearWorld(void);
/* called after the world model has been loaded, before linking any entities */

void SV_UnlinkEntity(Sharedent *ent);
/* call before removing an entity, and before trying to move one,
 * so it doesn't clip against itself */

void SV_LinkEntity(Sharedent *ent);
/* Needs to be called any time an entity changes origin, mins, maxs,
 * or solid.  Automatically unlinks if needed.
 * sets ent->v.absmin and ent->v.absmax
 * sets ent->leafnums[] for pvs determination even if the entity
 * is not solid */


Cliphandle SV_ClipHandleForEntity(const Sharedent *ent);


void SV_SectorList_f(void);


int SV_AreaEntities(const Vec3 mins, const Vec3 maxs, int *entityList,
		    int maxcount);
/* fills in a table of entity numbers with entities that have bounding boxes
 * that intersect the given area.  It is possible for a non-axial bmodel
 * to be returned that doesn't actually intersect the area on an exact
 * test.
 * returns the number of pointers filled in
 * The world entity is never returned in this list. */


int SV_PointContents(const Vec3 p, int passEntityNum);
/* returns the CONTENTS_* value from the world and all entities at the given point. */


void SV_Trace(Trace *results, const Vec3 start, Vec3 mins, Vec3 maxs,
	      const Vec3 end, int passEntityNum, int contentmask,
	      int capsule);
/* mins and maxs are relative */

/* if the entire move stays in a solid volume, trace.allsolid will be set,
 * trace.startsolid will be set, and trace.fraction will be 0 */

/* if the starting point is in a solid, it will be allowed to move out
 * to an open area */

/* passEntityNum is explicitly excluded from clipping checks (normally ENTITYNUM_NONE) */


void SV_ClipToEntity(Trace *trace, const Vec3 start, const Vec3 mins,
		     const Vec3 maxs, const Vec3 end, int entityNum,
		     int contentmask,
		     int capsule);
/* clip to a specific entity */

/*
 * sv_net_chan.c
 *  */
void SV_Netchan_Transmit(Client *client, Bitmsg *msg);
int SV_Netchan_TransmitNextFragment(Client *client);
qbool SV_Netchan_Process(Client *client, Bitmsg *msg);
void SV_Netchan_FreeQueue(Client *client);
