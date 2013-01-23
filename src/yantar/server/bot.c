/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* sv_bot.c */

#include "server.h"
#include "botlib.h"

typedef struct bot_debugpoly_s {
	int	inuse;
	int	color;
	int	numPoints;
	Vec3	points[128];
} bot_debugpoly_t;

static bot_debugpoly_t *debugpolygons;
int bot_maxdebugpolys;

extern botlib_export_t *botlib_export;
int bot_enable;


/*
 * SV_BotAllocateClient
 */
int
SV_BotAllocateClient(void)
{
	int i;
	Client *cl;

	/* find a client slot */
	for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		if(cl->state == CS_FREE)
			break;

	if(i == sv_maxclients->integer)
		return -1;

	cl->gentity = SV_GentityNum(i);
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
 * SV_BotFreeClient
 */
void
SV_BotFreeClient(int clientNum)
{
	Client *cl;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
		Com_Errorf(ERR_DROP, "SV_BotFreeClient: bad clientNum: %i",
			clientNum);
	cl = &svs.clients[clientNum];
	cl->state	= CS_FREE;
	cl->name[0]	= 0;
	if(cl->gentity)
		cl->gentity->r.svFlags &= ~SVF_BOT;
}

/*
 * BotDrawDebugPolygons
 */
void
BotDrawDebugPolygons(void (*drawPoly)(int color, int numPoints,
				      float *points), int value)
{
	static Cvar *bot_debug, *bot_groundonly, *bot_reachability,
	*bot_highlightarea;
	bot_debugpoly_t *poly;
	int i, parm0;

	if(!debugpolygons)
		return;
	/* bot debugging */
	if(!bot_debug) bot_debug = cvarget("bot_debug", "0", 0);
	if(bot_enable && bot_debug->integer){
		/* show reachabilities */
		if(!bot_reachability) bot_reachability = cvarget(
				"bot_reachability", "0", 0);
		/* show ground faces only */
		if(!bot_groundonly) bot_groundonly =
				cvarget("bot_groundonly", "1",
					0);
		/* get the hightlight area */
		if(!bot_highlightarea) bot_highlightarea = cvarget(
				"bot_highlightarea", "0", 0);
		parm0 = 0;
		if(svs.clients[0].lastUsercmd.buttons & BUTTON_PRIATTACK) parm0 |=
				1;
		if(bot_reachability->integer) parm0 |= 2;
		if(bot_groundonly->integer) parm0 |= 4;
		botlib_export->BotLibVarSet("bot_highlightarea",
			bot_highlightarea->string);
		botlib_export->Test(parm0, NULL,
			svs.clients[0].gentity->r.currentOrigin,
			svs.clients[0].gentity->r.currentAngles);
	}
	/* draw all debug polys */
	for(i = 0; i < bot_maxdebugpolys; i++){
		poly = &debugpolygons[i];
		if(!poly->inuse) continue;
		drawPoly(poly->color, poly->numPoints, (float*)poly->points);
		/* Com_Printf("poly %i, numpoints = %d\n", i, poly->numPoints); */
	}
}

/*
 * BotImport_Print
 */
static __attribute__ ((format (printf, 2, 3))) void QDECL
BotImport_Print(int type, char *fmt, ...)
{
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type){
	case PRT_MESSAGE: {
		Com_Printf("%s", str);
		break;
	}
	case PRT_WARNING: {
		Com_Printf(S_COLOR_YELLOW "Warning: %s", str);
		break;
	}
	case PRT_ERROR: {
		Com_Printf(S_COLOR_RED "Error: %s", str);
		break;
	}
	case PRT_FATAL: {
		Com_Printf(S_COLOR_RED "Fatal: %s", str);
		break;
	}
	case PRT_EXIT: {
		Com_Errorf(ERR_DROP, S_COLOR_RED "Exit: %s", str);
		break;
	}
	default: {
		Com_Printf("unknown print type\n");
		break;
	}
	}
}

/*
 * BotImport_Trace
 */
static void
BotImport_Trace(bsp_Trace *bsptrace, Vec3 start, Vec3 mins, Vec3 maxs,
		Vec3 end, int passent,
		int contentmask)
{
	Trace trace;

	SV_Trace(&trace, start, mins, maxs, end, passent, contentmask, qfalse);
	/* copy the trace information */
	bsptrace->allsolid	= trace.allsolid;
	bsptrace->startsolid	= trace.startsolid;
	bsptrace->fraction	= trace.fraction;
	copyv3(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	copyv3(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist	= 0;
	bsptrace->sidenum	= 0;
	bsptrace->contents	= 0;
}

/*
 * BotImport_EntityTrace
 */
static void
BotImport_EntityTrace(bsp_Trace *bsptrace, Vec3 start, Vec3 mins,
		      Vec3 maxs, Vec3 end, int entnum,
		      int contentmask)
{
	Trace trace;

	SV_ClipToEntity(&trace, start, mins, maxs, end, entnum, contentmask,
		qfalse);
	/* copy the trace information */
	bsptrace->allsolid	= trace.allsolid;
	bsptrace->startsolid	= trace.startsolid;
	bsptrace->fraction	= trace.fraction;
	copyv3(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	copyv3(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist	= 0;
	bsptrace->sidenum	= 0;
	bsptrace->contents	= 0;
}


/*
 * BotImport_PointContents
 */
static int
BotImport_PointContents(Vec3 point)
{
	return SV_PointContents(point, -1);
}

/*
 * BotImport_inPVS
 */
static int
BotImport_inPVS(Vec3 p1, Vec3 p2)
{
	return SV_inPVS (p1, p2);
}

/*
 * BotImport_BSPEntityData
 */
static char *
BotImport_BSPEntityData(void)
{
	return CM_EntityString();
}

/*
 * BotImport_BSPModelMinsMaxsOrigin
 */
static void
BotImport_BSPModelMinsMaxsOrigin(int modelnum, Vec3 angles, Vec3 outmins,
				 Vec3 outmaxs,
				 Vec3 origin)
{
	Cliphandle h;
	Vec3	mins, maxs;
	float	max;
	int	i;

	h = CM_InlineModel(modelnum);
	CM_ModelBounds(h, mins, maxs);
	/* if the model is rotated */
	if((angles[0] || angles[1] || angles[2])){
		/* expand for rotation */

		max = RadiusFromBounds(mins, maxs);
		for(i = 0; i < 3; i++){
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if(outmins) copyv3(mins, outmins);
	if(outmaxs) copyv3(maxs, outmaxs);
	if(origin) clearv3(origin);
}

/*
 * BotImport_GetMemory
 */
static void *
BotImport_GetMemory(int size)
{
	void *ptr;

	ptr = ztagalloc(size, MTbotlib);
	return ptr;
}

/*
 * BotImport_FreeMemory
 */
static void
BotImport_FreeMemory(void *ptr)
{
	zfree(ptr);
}

/*
 * BotImport_HunkAlloc
 */
static void *
BotImport_HunkAlloc(int size)
{
	if(hunkcheckmark())
		Com_Errorf(ERR_DROP,
			"SV_Bot_HunkAlloc: Alloc with marks already set");
	return hunkalloc(size, h_high);
}

/*
 * BotImport_DebugPolygonCreate
 */
int
BotImport_DebugPolygonCreate(int color, int numPoints, Vec3 *points)
{
	bot_debugpoly_t *poly;
	int i;

	if(!debugpolygons)
		return 0;

	for(i = 1; i < bot_maxdebugpolys; i++)
		if(!debugpolygons[i].inuse)
			break;
	if(i >= bot_maxdebugpolys)
		return 0;
	poly = &debugpolygons[i];
	poly->inuse	= qtrue;
	poly->color	= color;
	poly->numPoints = numPoints;
	Q_Memcpy(poly->points, points, numPoints * sizeof(Vec3));
	return i;
}

/*
 * BotImport_DebugPolygonShow
 */
static void
BotImport_DebugPolygonShow(int id, int color, int numPoints, Vec3 *points)
{
	bot_debugpoly_t *poly;

	if(!debugpolygons) return;
	poly = &debugpolygons[id];
	poly->inuse	= qtrue;
	poly->color	= color;
	poly->numPoints = numPoints;
	Q_Memcpy(poly->points, points, numPoints * sizeof(Vec3));
}

/*
 * BotImport_DebugPolygonDelete
 */
void
BotImport_DebugPolygonDelete(int id)
{
	if(!debugpolygons) return;
	debugpolygons[id].inuse = qfalse;
}

/*
 * BotImport_DebugLineCreate
 */
static int
BotImport_DebugLineCreate(void)
{
	Vec3 points[1];
	return BotImport_DebugPolygonCreate(0, 0, points);
}

/*
 * BotImport_DebugLineDelete
 */
static void
BotImport_DebugLineDelete(int line)
{
	BotImport_DebugPolygonDelete(line);
}

/*
 * BotImport_DebugLineShow
 */
static void
BotImport_DebugLineShow(int line, Vec3 start, Vec3 end, int color)
{
	Vec3	points[4], dir, cross, up = {0, 0, 1};
	float	dot;

	copyv3(start, points[0]);
	copyv3(start, points[1]);
	/* points[1][2] -= 2; */
	copyv3(end, points[2]);
	/* points[2][2] -= 2; */
	copyv3(end, points[3]);


	subv3(end, start, dir);
	normv3(dir);
	dot = dotv3(dir, up);
	if(dot > 0.99 || dot < -0.99) setv3(cross, 1, 0, 0);
	else crossv3(dir, up, cross);

	normv3(cross);

	saddv3(points[0], 2, cross, points[0]);
	saddv3(points[1], -2, cross, points[1]);
	saddv3(points[2], -2, cross, points[2]);
	saddv3(points[3], 2, cross, points[3]);

	BotImport_DebugPolygonShow(line, color, 4, points);
}

/*
 * SV_BotClientCommand
 */
static void
BotClientCommand(int client, char *command)
{
	SV_ExecuteClientCommand(&svs.clients[client], command, qtrue);
}

/*
 * SV_BotFrame
 */
void
SV_BotFrame(int time)
{
	if(!bot_enable) return;
	/* NOTE: maybe the game is already shutdown */
	if(!gvm) return;
	vmcall(gvm, BOTAI_START_FRAME, time);
}

/*
 * SV_BotLibSetup
 */
int
SV_BotLibSetup(void)
{
	if(!bot_enable)
		return 0;

	if(!botlib_export){
		Com_Printf(
			S_COLOR_RED
			"Error: SV_BotLibSetup without SV_BotInitBotLib\n");
		return -1;
	}

	return botlib_export->BotLibSetup();
}

/*
 * SV_ShutdownBotLib
 *
 * Called when either the entire server is being killed, or
 * it is changing to a different game directory.
 */
int
SV_BotLibShutdown(void)
{

	if(!botlib_export)
		return -1;

	return botlib_export->BotLibShutdown();
}

/*
 * SV_BotInitCvars
 */
void
SV_BotInitCvars(void)
{

	cvarget("bot_enable", "1", 0);				/* enable the bot */
	cvarget("bot_developer", "0", CVAR_CHEAT);		/* bot developer mode */
	cvarget("bot_debug", "0", CVAR_CHEAT);			/* enable bot debugging */
	cvarget("bot_maxdebugpolys", "2", 0);			/* maximum number of debug polys */
	cvarget("bot_groundonly", "1", 0);			/* only show ground faces of areas */
	cvarget("bot_reachability", "0", 0);			/* show all reachabilities to other areas */
	cvarget("bot_visualizejumppads", "0", CVAR_CHEAT);	/* show jumppads */
	cvarget("bot_forceclustering", "0", 0);		/* force cluster calculations */
	cvarget("bot_forcereachability", "0", 0);		/* force reachability calculations */
	cvarget("bot_forcewrite", "0", 0);			/* force writing aas file */
	cvarget("bot_aasoptimize", "0", 0);			/* no aas file optimisation */
	cvarget("bot_saveroutingcache", "0", 0);		/* save routing cache */
	cvarget("bot_thinktime", "100", CVAR_CHEAT);		/* msec the bots thinks */
	cvarget("bot_reloadcharacters", "0", 0);		/* reload the bot characters each time */
	cvarget("bot_testichat", "0", 0);			/* test ichats */
	cvarget("bot_testrchat", "0", 0);			/* test rchats */
	cvarget("bot_testsolid", "0", CVAR_CHEAT);		/* test for solid areas */
	cvarget("bot_testclusters", "0", CVAR_CHEAT);		/* test the AAS clusters */
	cvarget("bot_fastchat", "0", 0);			/* fast chatting bots */
	cvarget("bot_nochat", "0", 0);				/* disable chats */
	cvarget("bot_pause", "0", CVAR_CHEAT);			/* pause the bots thinking */
	cvarget("bot_report", "0", CVAR_CHEAT);		/* get a full report in ctf */
	cvarget("bot_grapple", "0", 0);			/* enable grapple */
	cvarget("bot_rocketjump", "1", 0);			/* enable rocket jumping */
	cvarget("bot_challenge", "0", 0);			/* challenging bot */
	cvarget("bot_minplayers", "0", 0);			/* minimum players in a team or the game */
	cvarget("bot_interbreedchar", "", CVAR_CHEAT);		/* bot character used for interbreeding */
	cvarget("bot_interbreedbots", "10", CVAR_CHEAT);	/* number of bots used for interbreeding */
	cvarget("bot_interbreedcycle", "20", CVAR_CHEAT);	/* bot interbreeding cycle */
	cvarget("bot_interbreedwrite", "", CVAR_CHEAT);	/* write interbreeded bots to this file */
}

/*
 * SV_BotInitBotLib
 */
void
SV_BotInitBotLib(void)
{
	botlib_import_t botlib_import;

	if(debugpolygons) zfree(debugpolygons);
	bot_maxdebugpolys = cvargeti("bot_maxdebugpolys");
	debugpolygons = zalloc(sizeof(bot_debugpoly_t) * bot_maxdebugpolys);

	botlib_import.Print	= BotImport_Print;
	botlib_import.Trace	= BotImport_Trace;
	botlib_import.EntityTrace	= BotImport_EntityTrace;
	botlib_import.PointContents	= BotImport_PointContents;
	botlib_import.inPVS = BotImport_inPVS;
	botlib_import.BSPEntityData = BotImport_BSPEntityData;
	botlib_import.BSPModelMinsMaxsOrigin = BotImport_BSPModelMinsMaxsOrigin;
	botlib_import.BotClientCommand = BotClientCommand;

	/* memory management */
	botlib_import.GetMemory		= BotImport_GetMemory;
	botlib_import.FreeMemory	= BotImport_FreeMemory;
	botlib_import.AvailableMemory = zmemavailable;
	botlib_import.HunkAlloc = BotImport_HunkAlloc;

	/* file system access */
	botlib_import.FS_FOpenFile = FS_FOpenFileByMode;
	botlib_import.FS_Read	= FS_Read2;
	botlib_import.FS_Write	= FS_Write;
	botlib_import.FS_FCloseFile = FS_FCloseFile;
	botlib_import.FS_Seek = FS_Seek;

	/* debug lines */
	botlib_import.DebugLineCreate	= BotImport_DebugLineCreate;
	botlib_import.DebugLineDelete	= BotImport_DebugLineDelete;
	botlib_import.DebugLineShow	= BotImport_DebugLineShow;

	/* debug polygons */
	botlib_import.DebugPolygonCreate	= BotImport_DebugPolygonCreate;
	botlib_import.DebugPolygonDelete	= BotImport_DebugPolygonDelete;

	botlib_export = (botlib_export_t*)GetBotLibAPI(BOTLIB_API_VERSION,
		&botlib_import);
	assert(botlib_export);	/* somehow we end up with a zero import. */
}


/*
 *  * * * BOT AI CODE IS BELOW THIS POINT * * *
 *  */

/*
 * SV_BotGetConsoleMessage
 */
int
SV_BotGetConsoleMessage(int client, char *buf, int size)
{
	Client *cl;
	int index;

	cl = &svs.clients[client];
	cl->lastPacketTime = svs.time;

	if(cl->reliableAcknowledge == cl->reliableSequence)
		return qfalse;

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1);

	if(!cl->reliableCommands[index][0])
		return qfalse;

	Q_strncpyz(buf, cl->reliableCommands[index], size);
	return qtrue;
}

#if 0
/*
 * EntityInPVS
 */
int
EntityInPVS(int client, int entityNum)
{
	Client *cl;
	clientSnapshot_t *frame;
	int i;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	for(i = 0; i < frame->num_entities; i++)
		if(svs.snapshotEntities[(frame->first_entity +
					 i) %
					svs.numSnapshotEntities].number ==
		   entityNum)
			return qtrue;
	return qfalse;
}
#endif

/*
 * SV_BotGetSnapshotEntity
 */
int
SV_BotGetSnapshotEntity(int client, int sequence)
{
	Client *cl;
	clientSnapshot_t *frame;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	if(sequence < 0 || sequence >= frame->num_entities)
		return -1;
	return svs.snapshotEntities[(frame->first_entity +
				     sequence) % svs.numSnapshotEntities].number;
}
