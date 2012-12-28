/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

enum {
	CGAME_IMPORT_API_VERSION	= 0,
	/*
	 * allow a lot of command backups for very fast systems multiple
	 * commands may be combined into a single packet, so this needs
	 * to be larger than PACKET_BACKUP
	 */
	MAX_ENTITIES_IN_SNAPSHOT	= 256,
	CMD_BACKUP			= 64,
	CMD_MASK			= (CMD_BACKUP - 1)
};

enum {
	CGAME_EVENT_NONE,
	CGAME_EVENT_TEAMMENU,
	CGAME_EVENT_SCOREBOARD,
	CGAME_EVENT_EDITHUD
};

/*
 * Snapshots are a view of the server at a given time.  They are
 * generated at regular time intervals by the server, but they may not be
 * sent if a client's rate level is exceeded, or they may be dropped by
 * the network.
 */
typedef struct {
	int		snapFlags;	/* SNAPFLAG_RATE_DELAYED, etc */
	int		ping;
	int		serverTime;	/* server time the message is valid for (in msec) */
	byte		areamask[MAX_MAP_AREA_BYTES];	/* portalarea visibility bits */
	playerState_t	ps;		/* complete info about the current player at this time */
	int		numEntities;	/* all of the entities that need to be presented at the time of this snapshot */
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];
	int		numServerCommands;	/* text based server cmds to execute when this snapshot becomes current */
	int		serverCommandSequence;
} snapshot_t;

/*
 * functions imported from the main executable
 */
typedef enum {
	CG_PRINT,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_VARIABLESTRINGBUFFER,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,
	CG_FS_SEEK,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_REMOVECOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_LOADMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_MARKFRAGMENTS,
	CG_S_STARTSOUND,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_ADDREALLOOPINGSOUND,
	CG_S_STOPLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STOPBACKGROUNDTRACK,
	CG_S_STARTBACKGROUNDTRACK,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERFONT,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_REGISTERSHADERNOMIP,
	CG_R_REMAP_SHADER,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDPOLYSTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_ADDADDITIVELIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_R_LIGHTFORPOINT,
	CG_R_INPVS,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_MEMORY_REMAINING,
	CG_KEY_ISDOWN,
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,
	CG_KEY_GETKEY,
	CG_REAL_TIME,
	CG_SNAPVECTOR,
	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,

	CG_CM_TEMPCAPSULEMODEL,
	CG_CM_CAPSULETRACE,
	CG_CM_TRANSFORMEDCAPSULETRACE,
	CG_GET_ENTITY_TOKEN,
	
	CG_MEMSET,
	CG_MEMCPY,
	CG_STRNCPY,
	CG_SIN,
	CG_COS,
	CG_ATAN2,
	CG_SQRT,
	CG_FLOOR,
	CG_CEIL,
	CG_ACOS,
	CG_ASIN,
	CG_ATAN,
	CG_MATRIXMULTIPLY,
	CG_ANGLEVECTORS,
	CG_PERPENDICULARVECTOR,

	CG_TESTPRINTINT,
	CG_TESTPRINTFLOAT
} cgameImport_t;

/*
 * functions exported to the main executable
 */
typedef enum {
	/* 
	 * called when the level loads or when the renderer is restarted
	 * all media should be registered at this time
	 * cgame will display loading status by calling SCR_Update, which
	 * will call CG_DrawInformation during the loading process
	 * reliableCommandSequence will be 0 on fresh loads, but higher for
	 * demos, tourney restarts, or vid_restarts 
	 '*/
	CG_INIT,
	/* opportunity to flush and close any open files */
	CG_SHUTDOWN,
	/*
	 * a console command has been issued locally that is not recognized by the
	 * main game system.
	 * use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	 * command is not known to the game 
	 */
	CG_CONSOLE_COMMAND,
	/*
	 * Generates and draws a game scene and status information at the given time.
	 * If demoPlayback is set, local movement prediction will not be enabled 
	 */
	CG_DRAW_ACTIVE_FRAME,
	CG_CROSSHAIR_PLAYER,
	CG_LAST_ATTACKER,
	CG_KEY_EVENT,
	CG_MOUSE_EVENT,
	CG_EVENT_HANDLING
} cgameExport_t;

