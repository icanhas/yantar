/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* sv_game.c -- interface to the game dll */

#include "server.h"

#include "botlib.h"

botlib_export_t *botlib_export;

/* these functions must be used instead of pointer arithmetic, because
 * the game allocates gentities with private information after the server shared part */
int
SV_NumForGentity(Sharedent *ent)
{
	int num;

	num = ((byte*)ent - (byte*)sv.gentities) / sv.gentitySize;

	return num;
}

Sharedent *
SV_GentityNum(int num)
{
	Sharedent *ent;

	ent = (Sharedent*)((byte*)sv.gentities + sv.gentitySize*(num));

	return ent;
}

Playerstate *
SV_GameClientNum(int num)
{
	Playerstate *ps;

	ps = (Playerstate*)((byte*)sv.gameClients + sv.gameClientSize*(num));

	return ps;
}

Svent      *
SV_SvEntityForGentity(Sharedent *gEnt)
{
	if(!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES)
		comerrorf(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
	return &sv.svEntities[ gEnt->s.number ];
}

Sharedent *
SV_GEntityForSvEntity(Svent *svEnt)
{
	int num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum(num);
}

/*
 * SV_GameSendServerCommand
 *
 * Sends a command string to a client
 */
void
SV_GameSendServerCommand(int clientNum, const char *text)
{
	if(clientNum == -1)
		SV_SendServerCommand(NULL, "%s", text);
	else{
		if(clientNum < 0 || clientNum >= sv_maxclients->integer)
			return;
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}


/*
 * SV_GameDropClient
 *
 * Disconnects the client with a message
 */
void
SV_GameDropClient(int clientNum, const char *reason)
{
	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
		return;
	SV_DropClient(svs.clients + clientNum, reason);
}


/*
 * SV_SetBrushModel
 *
 * sets mins and maxs for inline bmodels
 */
void
SV_SetBrushModel(Sharedent *ent, const char *name)
{
	Cliphandle h;
	Vec3 mins, maxs;

	if(!name)
		comerrorf(ERR_DROP, "SV_SetBrushModel: NULL");

	if(name[0] != '*')
		comerrorf(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model",
			name);


	ent->s.modelindex = atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	copyv3 (mins, ent->r.mins);
	copyv3 (maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;	/* we don't know exactly what is in the brushes */

	SV_LinkEntity(ent);	/* FIXME: remove */
}



/*
 * SV_inPVS
 *
 * Also checks portalareas so that doors block sight
 */
qbool
SV_inPVS(const Vec3 p1, const Vec3 p2)
{
	int	leafnum;
	int	cluster;
	int	area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1	= CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2	= CM_LeafArea (leafnum);
	if(mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
		return qfalse;
	if(!CM_AreasConnected (area1, area2))
		return qfalse;	/* a door blocks sight */
	return qtrue;
}


/*
 * SV_inPVSIgnorePortals
 *
 * Does NOT check portalareas
 */
qbool
SV_inPVSIgnorePortals(const Vec3 p1, const Vec3 p2)
{
	int	leafnum;
	int	cluster;
	byte *mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);

	if(mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
		return qfalse;

	return qtrue;
}


/*
 * SV_AdjustAreaPortalState
 */
void
SV_AdjustAreaPortalState(Sharedent *ent, qbool open)
{
	Svent *svEnt;

	svEnt = SV_SvEntityForGentity(ent);
	if(svEnt->areanum2 == -1)
		return;
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
 * SV_EntityContact
 */
qbool
SV_EntityContact(Vec3 mins, Vec3 maxs, const Sharedent *gEnt,
		 int capsule)
{
	const float	*origin, *angles;
	Cliphandle	ch;
	Trace trace;

	/* check for exact collision */
	origin	= gEnt->r.currentOrigin;
	angles	= gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTrace (&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return trace.startsolid;
}


/*
 * SV_GetServerinfo
 *
 */
void
SV_GetServerinfo(char *buffer, int bufferSize)
{
	if(bufferSize < 1)
		comerrorf(ERR_DROP, "SV_GetServerinfo: bufferSize == %i",
			bufferSize);
	Q_strncpyz(buffer, cvargetinfostr(CVAR_SERVERINFO), bufferSize);
}

/*
 * SV_LocateGameData
 *
 */
void
SV_LocateGameData(Sharedent *gEnts, int numGEntities, int sizeofGEntity_t,
		  Playerstate *clients, int sizeofGameClient)
{
	sv.gentities = gEnts;
	sv.gentitySize	= sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}


/*
 * SV_GetUsercmd
 *
 */
void
SV_GetUsercmd(int clientNum, Usrcmd *cmd)
{
	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
		comerrorf(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	*cmd = svs.clients[clientNum].lastUsercmd;
}

/* ============================================== */

static int
FloatAsInt(float f)
{
	Flint fi;
	fi.f = f;
	return fi.i;
}

/*
 * SV_GameSystemCalls
 *
 * The module is making a system call
 */
intptr_t
SV_GameSystemCalls(intptr_t *args)
{
	switch(args[0]){
	case G_PRINT:
		comprintf("%s", (const char*)VMA(1));
		return 0;
	case G_ERROR:
		comerrorf(ERR_DROP, "%s", (const char*)VMA(1));
		return 0;
	case G_MILLISECONDS:
		return sysmillisecs();
	case G_CVAR_REGISTER:
		cvarregister(VMA(1), VMA(2), VMA(3), args[4]);
		return 0;
	case G_CVAR_UPDATE:
		cvarupdate(VMA(1));
		return 0;
	case G_CVAR_SET:
		cvarsetstrsafe((const char*)VMA(1), (const char*)VMA(2));
		return 0;
	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return cvargeti((const char*)VMA(1));
	case G_CVAR_VARIABLE_STRING_BUFFER:
		cvargetstrbuf(VMA(1), VMA(2), args[3]);
		return 0;
	case G_ARGC:
		return cmdargc();
	case G_ARGV:
		cmdargvbuf(args[1], VMA(2), args[3]);
		return 0;
	case G_SEND_CONSOLE_COMMAND:
		cbufexecstr(args[1], VMA(2));
		return 0;

	case G_FS_FOPEN_FILE:
		return fsopenmode(VMA(1), VMA(2), args[3]);
	case G_FS_READ:
		fsread2(VMA(1), args[2], args[3]);
		return 0;
	case G_FS_WRITE:
		fswrite(VMA(1), args[2], args[3]);
		return 0;
	case G_FS_FCLOSE_FILE:
		fsclose(args[1]);
		return 0;
	case G_FS_GETFILELIST:
		return fsgetfilelist(VMA(1), VMA(2), VMA(3), args[4]);
	case G_FS_SEEK:
		return fsseek(args[1], args[2], args[3]);

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData(VMA(1), args[2], args[3], VMA(4), args[5]);
		return 0;
	case G_DROP_CLIENT:
		SV_GameDropClient(args[1], VMA(2));
		return 0;
	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], VMA(2));
		return 0;
	case G_LINKENTITY:
		SV_LinkEntity(VMA(1));
		return 0;
	case G_UNLINKENTITY:
		SV_UnlinkEntity(VMA(1));
		return 0;
	case G_ENTITIES_IN_BOX:
		return SV_AreaEntities(VMA(1), VMA(2), VMA(3), args[4]);
	case G_ENTITY_CONTACT:
		return SV_EntityContact(VMA(1), VMA(2), VMA(
				3), /*int capsule*/ qfalse);
	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact(VMA(1), VMA(2), VMA(
				3), /*int capsule*/ qtrue);
	case G_TRACE:
		SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(
				5), args[6], args[7], /*int capsule*/ qfalse);
		return 0;
	case G_TRACECAPSULE:
		SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(
				5), args[6], args[7], /*int capsule*/ qtrue);
		return 0;
	case G_POINT_CONTENTS:
		return SV_PointContents(VMA(1), args[2]);
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel(VMA(1), VMA(2));
		return 0;
	case G_IN_PVS:
		return SV_inPVS(VMA(1), VMA(2));
	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals(VMA(1), VMA(2));

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], VMA(2));
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], VMA(2), args[3]);
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo(args[1], VMA(2));
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo(args[1], VMA(2), args[3]);
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo(VMA(1), args[2]);
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState(VMA(1), args[2]);
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected(args[1], args[2]);

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient(args[1]);
		return 0;

	case G_GET_USERCMD:
		SV_GetUsercmd(args[1], VMA(2));
		return 0;
	case G_GET_ENTITY_TOKEN:
	{
		const char *s;

		s = Q_readtok(&sv.entityParsePoint);
		Q_strncpyz(VMA(1), s, args[2]);
		if(!sv.entityParsePoint && !s[0])
			return qfalse;
		else
			return qtrue;
	}

	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate(args[1], args[2], VMA(3));
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete(args[1]);
		return 0;
	case G_REAL_TIME:
		return comrealtime(VMA(1));
	case G_SNAPVECTOR:
		Q_snapv3(VMA(1));
		return 0;

	/* ==================================== */

	case BOTLIB_SETUP:
		return SV_BotLibSetup();
	case BOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
	case BOTLIB_LIBVAR_SET:
		return botlib_export->BotLibVarSet(VMA(1), VMA(2));
	case BOTLIB_LIBVAR_GET:
		return botlib_export->BotLibVarGet(VMA(1), VMA(2), args[3]);

	case BOTLIB_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine(VMA(1));
	case BOTLIB_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle(VMA(1));
	case BOTLIB_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle(args[1]);
	case BOTLIB_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
	case BOTLIB_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine(args[1], VMA(2),
			VMA(3));

	case BOTLIB_START_FRAME:
		return botlib_export->BotLibStartFrame(VMF(1));
	case BOTLIB_LOAD_MAP:
		return botlib_export->BotLibLoadMap(VMA(1));
	case BOTLIB_UPDATENTITY:
		return botlib_export->BotLibUpdateEntity(args[1], VMA(2));
	case BOTLIB_TEST:
		return botlib_export->Test(args[1], VMA(2), VMA(3), VMA(4));

	case BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity(args[1], args[2]);
	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage(args[1], VMA(2), args[3]);
	case BOTLIB_USER_COMMAND:
		SV_ClientThink(&svs.clients[args[1]], VMA(2));
		return 0;

	case BOTLIB_AAS_BBOX_AREAS:
		return botlib_export->aas.AAS_BBoxAreas(VMA(1), VMA(2), VMA(
				3), args[4]);
	case BOTLIB_AAS_AREA_INFO:
		return botlib_export->aas.AAS_AreaInfo(args[1], VMA(2));
	case BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL:
		return botlib_export->aas.AAS_AlternativeRouteGoals(VMA(
				1), args[2], VMA(3), args[4], args[5], VMA(
				6), args[7], args[8]);
	case BOTLIB_AAS_ENTITY_INFO:
		botlib_export->aas.AAS_EntityInfo(args[1], VMA(2));
		return 0;

	case BOTLIB_AAS_INITIALIZED:
		return botlib_export->aas.AAS_Initialized();
	case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		botlib_export->aas.AAS_PresenceTypeBoundingBox(args[1], VMA(
				2), VMA(3));
		return 0;
	case BOTLIB_AAS_TIME:
		return FloatAsInt(botlib_export->aas.AAS_Time());

	case BOTLIB_AAS_POINT_AREA_NUM:
		return botlib_export->aas.AAS_PointAreaNum(VMA(1));
	case BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX:
		return botlib_export->aas.AAS_PointReachabilityAreaIndex(VMA(1));
	case BOTLIB_AAS_TRACE_AREAS:
		return botlib_export->aas.AAS_TraceAreas(VMA(1), VMA(2), VMA(
				3), VMA(4), args[5]);

	case BOTLIB_AAS_POINT_CONTENTS:
		return botlib_export->aas.AAS_PointContents(VMA(1));
	case BOTLIB_AAS_NEXT_BSP_ENTITY:
		return botlib_export->aas.AAS_NextBSPEntity(args[1]);
	case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_ValueForBSPEpairKey(args[1], VMA(
				2), VMA(3), args[4]);
	case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_VectorForBSPEpairKey(args[1],
			VMA(2), VMA(3));
	case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_FloatForBSPEpairKey(args[1], VMA(
				2), VMA(3));
	case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_IntForBSPEpairKey(args[1], VMA(
				2), VMA(3));

	case BOTLIB_AAS_AREA_REACHABILITY:
		return botlib_export->aas.AAS_AreaReachability(args[1]);

	case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(args[1],
			VMA(
				2), args[3], args[4]);
	case BOTLIB_AAS_ENABLE_ROUTING_AREA:
		return botlib_export->aas.AAS_EnableRoutingArea(args[1], args[2]);
	case BOTLIB_AAS_PREDICT_ROUTE:
		return botlib_export->aas.AAS_PredictRoute(VMA(1), args[2],
			VMA(
				3), args[4], args[5], args[6], args[7], args[8],
			args[9],
			args[10], args[11]);

	case BOTLIB_AAS_SWIMMING:
		return botlib_export->aas.AAS_Swimming(VMA(1));
	case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return botlib_export->aas.AAS_PredictClientMovement(VMA(
				1), args[2], VMA(3), args[4], args[5],
			VMA(6), VMA(7), args[8], args[9], VMF(
				10), args[11], args[12], args[13]);

	case BOTLIB_EA_SAY:
		botlib_export->ea.EA_Say(args[1], VMA(2));
		return 0;
	case BOTLIB_EA_SAY_TEAM:
		botlib_export->ea.EA_SayTeam(args[1], VMA(2));
		return 0;
	case BOTLIB_EA_COMMAND:
		botlib_export->ea.EA_Command(args[1], VMA(2));
		return 0;

	case BOTLIB_EA_ACTION:
		botlib_export->ea.EA_Action(args[1], args[2]);
		return 0;
	case BOTLIB_EA_GESTURE:
		botlib_export->ea.EA_Gesture(args[1]);
		return 0;
	case BOTLIB_EA_TALK:
		botlib_export->ea.EA_Talk(args[1]);
		return 0;
	case BOTLIB_EA_ATTACK:
		botlib_export->ea.EA_Attack(args[1]);
		return 0;
	case BOTLIB_EA_USE:
		botlib_export->ea.EA_Use(args[1]);
		return 0;
	case BOTLIB_EA_RESPAWN:
		botlib_export->ea.EA_Respawn(args[1]);
		return 0;
	case BOTLIB_EA_CROUCH:
		botlib_export->ea.EA_Crouch(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_UP:
		botlib_export->ea.EA_MoveUp(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_DOWN:
		botlib_export->ea.EA_MoveDown(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_FORWARD:
		botlib_export->ea.EA_MoveForward(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_BACK:
		botlib_export->ea.EA_MoveBack(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_LEFT:
		botlib_export->ea.EA_MoveLeft(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_RIGHT:
		botlib_export->ea.EA_MoveRight(args[1]);
		return 0;

	case BOTLIB_EA_SELECT_WEAPON:
		botlib_export->ea.EA_SelectWeapon(args[1], args[2]);
		return 0;
	case BOTLIB_EA_JUMP:
		botlib_export->ea.EA_Jump(args[1]);
		return 0;
	case BOTLIB_EA_DELAYED_JUMP:
		botlib_export->ea.EA_DelayedJump(args[1]);
		return 0;
	case BOTLIB_EA_MOVE:
		botlib_export->ea.EA_Move(args[1], VMA(2), VMF(3));
		return 0;
	case BOTLIB_EA_VIEW:
		botlib_export->ea.EA_View(args[1], VMA(2));
		return 0;

	case BOTLIB_EA_END_REGULAR:
		botlib_export->ea.EA_EndRegular(args[1], VMF(2));
		return 0;
	case BOTLIB_EA_GET_INPUT:
		botlib_export->ea.EA_GetInput(args[1], VMF(2), VMA(3));
		return 0;
	case BOTLIB_EA_RESET_INPUT:
		botlib_export->ea.EA_ResetInput(args[1]);
		return 0;

	case BOTLIB_AI_LOAD_CHARACTER:
		return botlib_export->ai.BotLoadCharacter(VMA(1), VMF(2));
	case BOTLIB_AI_FREE_CHARACTER:
		botlib_export->ai.BotFreeCharacter(args[1]);
		return 0;
	case BOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt(botlib_export->ai.Characteristic_Float(args[1],
				args[2]));
	case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt(botlib_export->ai.Characteristic_BFloat(args[1
				], args[2], VMF(3), VMF(4)));
	case BOTLIB_AI_CHARACTERISTIC_INTEGER:
		return botlib_export->ai.Characteristic_Integer(args[1], args[2]);
	case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return botlib_export->ai.Characteristic_BInteger(args[1],
			args[2], args[3],
			args[4]);
	case BOTLIB_AI_CHARACTERISTIC_STRING:
		botlib_export->ai.Characteristic_String(args[1], args[2], VMA(
				3), args[4]);
		return 0;

	case BOTLIB_AI_ALLOC_CHAT_STATE:
		return botlib_export->ai.BotAllocChatState();
	case BOTLIB_AI_FREE_CHAT_STATE:
		botlib_export->ai.BotFreeChatState(args[1]);
		return 0;
	case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		botlib_export->ai.BotQueueConsoleMessage(args[1], args[2], VMA(3));
		return 0;
	case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		botlib_export->ai.BotRemoveConsoleMessage(args[1], args[2]);
		return 0;
	case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNextConsoleMessage(args[1], VMA(2));
	case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNumConsoleMessages(args[1]);
	case BOTLIB_AI_INITIAL_CHAT:
		botlib_export->ai.BotInitialChat(args[1], VMA(2), args[3], VMA(
				4), VMA(5), VMA(6), VMA(7), VMA(8), VMA(9),
			VMA(10), VMA(11));
		return 0;
	case BOTLIB_AI_NUM_INITIAL_CHATS:
		return botlib_export->ai.BotNumInitialChats(args[1], VMA(2));
	case BOTLIB_AI_REPLY_CHAT:
		return botlib_export->ai.BotReplyChat(args[1], VMA(
				2), args[3], args[4], VMA(5), VMA(6), VMA(
				7), VMA(8), VMA(9),
			VMA(10), VMA(11), VMA(12));
	case BOTLIB_AI_CHAT_LENGTH:
		return botlib_export->ai.BotChatLength(args[1]);
	case BOTLIB_AI_ENTER_CHAT:
		botlib_export->ai.BotEnterChat(args[1], args[2], args[3]);
		return 0;
	case BOTLIB_AI_GET_CHAT_MESSAGE:
		botlib_export->ai.BotGetChatMessage(args[1], VMA(2), args[3]);
		return 0;
	case BOTLIB_AI_STRING_CONTAINS:
		return botlib_export->ai.StringContains(VMA(1), VMA(2), args[3]);
	case BOTLIB_AI_FIND_MATCH:
		return botlib_export->ai.BotFindMatch(VMA(1), VMA(2), args[3]);
	case BOTLIB_AI_MATCH_VARIABLE:
		botlib_export->ai.BotMatchVariable(VMA(1), args[2], VMA(
				3), args[4]);
		return 0;
	case BOTLIB_AI_UNIFY_WHITE_SPACES:
		botlib_export->ai.UnifyWhiteSpaces(VMA(1));
		return 0;
	case BOTLIB_AI_REPLACE_SYNONYMS:
		botlib_export->ai.BotReplaceSynonyms(VMA(1), args[2]);
		return 0;
	case BOTLIB_AI_LOAD_CHAT_FILE:
		return botlib_export->ai.BotLoadChatFile(args[1], VMA(2), VMA(3));
	case BOTLIB_AI_SET_CHAT_GENDER:
		botlib_export->ai.BotSetChatGender(args[1], args[2]);
		return 0;
	case BOTLIB_AI_SET_CHAT_NAME:
		botlib_export->ai.BotSetChatName(args[1], VMA(2), args[3]);
		return 0;

	case BOTLIB_AI_RESET_GOAL_STATE:
		botlib_export->ai.BotResetGoalState(args[1]);
		return 0;
	case BOTLIB_AI_RESET_AVOID_GOALS:
		botlib_export->ai.BotResetAvoidGoals(args[1]);
		return 0;
	case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		botlib_export->ai.BotRemoveFromAvoidGoals(args[1], args[2]);
		return 0;
	case BOTLIB_AI_PUSH_GOAL:
		botlib_export->ai.BotPushGoal(args[1], VMA(2));
		return 0;
	case BOTLIB_AI_POP_GOAL:
		botlib_export->ai.BotPopGoal(args[1]);
		return 0;
	case BOTLIB_AI_EMPTY_GOAL_STACK:
		botlib_export->ai.BotEmptyGoalStack(args[1]);
		return 0;
	case BOTLIB_AI_DUMP_AVOID_GOALS:
		botlib_export->ai.BotDumpAvoidGoals(args[1]);
		return 0;
	case BOTLIB_AI_DUMP_GOAL_STACK:
		botlib_export->ai.BotDumpGoalStack(args[1]);
		return 0;
	case BOTLIB_AI_GOAL_NAME:
		botlib_export->ai.BotGoalName(args[1], VMA(2), args[3]);
		return 0;
	case BOTLIB_AI_GET_TOP_GOAL:
		return botlib_export->ai.BotGetTopGoal(args[1], VMA(2));
	case BOTLIB_AI_GET_SECOND_GOAL:
		return botlib_export->ai.BotGetSecondGoal(args[1], VMA(2));
	case BOTLIB_AI_CHOOSE_LTG_ITEM:
		return botlib_export->ai.BotChooseLTGItem(args[1], VMA(2), VMA(
				3), args[4]);
	case BOTLIB_AI_CHOOSE_NBG_ITEM:
		return botlib_export->ai.BotChooseNBGItem(args[1], VMA(2), VMA(
				3), args[4], VMA(5), VMF(6));
	case BOTLIB_AI_TOUCHING_GOAL:
		return botlib_export->ai.BotTouchingGoal(VMA(1), VMA(2));
	case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return botlib_export->ai.BotItemGoalInVisButNotVisible(args[1],
			VMA(
				2), VMA(3), VMA(4));
	case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return botlib_export->ai.BotGetLevelItemGoal(args[1], VMA(
				2), VMA(3));
	case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return botlib_export->ai.BotGetNextCampSpotGoal(args[1], VMA(2));
	case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return botlib_export->ai.BotGetMapLocationGoal(VMA(1), VMA(2));
	case BOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt(botlib_export->ai.BotAvoidGoalTime(args[1],
				args[2]));
	case BOTLIB_AI_SET_AVOID_GOAL_TIME:
		botlib_export->ai.BotSetAvoidGoalTime(args[1], args[2], VMF(3));
		return 0;
	case BOTLIB_AI_INIT_LEVEL_ITEMS:
		botlib_export->ai.BotInitLevelItems();
		return 0;
	case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
		botlib_export->ai.BotUpdateEntityItems();
		return 0;
	case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return botlib_export->ai.BotLoadItemWeights(args[1], VMA(2));
	case BOTLIB_AI_FREE_ITEM_WEIGHTS:
		botlib_export->ai.BotFreeItemWeights(args[1]);
		return 0;
	case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotInterbreedGoalFuzzyLogic(args[1], args[2],
			args[3]);
		return 0;
	case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotSaveGoalFuzzyLogic(args[1], VMA(2));
		return 0;
	case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotMutateGoalFuzzyLogic(args[1], VMF(2));
		return 0;
	case BOTLIB_AI_ALLOC_GOAL_STATE:
		return botlib_export->ai.BotAllocGoalState(args[1]);
	case BOTLIB_AI_FREE_GOAL_STATE:
		botlib_export->ai.BotFreeGoalState(args[1]);
		return 0;

	case BOTLIB_AI_RESET_MOVE_STATE:
		botlib_export->ai.BotResetMoveState(args[1]);
		return 0;
	case BOTLIB_AI_ADD_AVOID_SPOT:
		botlib_export->ai.BotAddAvoidSpot(args[1], VMA(2), VMF(3),
			args[4]);
		return 0;
	case BOTLIB_AI_MOVE_TO_GOAL:
		botlib_export->ai.BotMoveToGoal(VMA(1), args[2], VMA(3), args[4]);
		return 0;
	case BOTLIB_AI_MOVE_IN_DIRECTION:
		return botlib_export->ai.BotMoveInDirection(args[1], VMA(2),
			VMF(3), args[4]);
	case BOTLIB_AI_RESET_AVOID_REACH:
		botlib_export->ai.BotResetAvoidReach(args[1]);
		return 0;
	case BOTLIB_AI_RESET_LAST_AVOID_REACH:
		botlib_export->ai.BotResetLastAvoidReach(args[1]);
		return 0;
	case BOTLIB_AI_REACHABILITY_AREA:
		return botlib_export->ai.BotReachabilityArea(VMA(1), args[2]);
	case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return botlib_export->ai.BotMovementViewTarget(args[1], VMA(
				2), args[3], VMF(4), VMA(5));
	case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return botlib_export->ai.BotPredictVisiblePosition(VMA(1),
			args[2], VMA(3), args[4], VMA(5));
	case BOTLIB_AI_ALLOC_MOVE_STATE:
		return botlib_export->ai.BotAllocMoveState();
	case BOTLIB_AI_FREE_MOVE_STATE:
		botlib_export->ai.BotFreeMoveState(args[1]);
		return 0;
	case BOTLIB_AI_INIT_MOVE_STATE:
		botlib_export->ai.BotInitMoveState(args[1], VMA(2));
		return 0;

	case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return botlib_export->ai.BotChooseBestFightWeapon(args[1], VMA(2));
	case BOTLIB_AI_GET_WEAPON_INFO:
		botlib_export->ai.BotGetWeaponInfo(args[1], args[2], VMA(3));
		return 0;
	case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return botlib_export->ai.BotLoadWeaponWeights(args[1], VMA(2));
	case BOTLIB_AI_ALLOC_WEAPON_STATE:
		return botlib_export->ai.BotAllocWeaponState();
	case BOTLIB_AI_FREE_WEAPON_STATE:
		botlib_export->ai.BotFreeWeaponState(args[1]);
		return 0;
	case BOTLIB_AI_RESET_WEAPON_STATE:
		botlib_export->ai.BotResetWeaponState(args[1]);
		return 0;

	case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return botlib_export->ai.GeneticParentsAndChildSelection(args[1],
			VMA(2), VMA(3), VMA(4), VMA(5));

	case G_MEMSET:
		Q_Memset(VMA(1), args[2], args[3]);
		return 0;
	case G_MEMCPY:
		Q_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;
	case G_STRNCPY:
		strncpy(VMA(1), VMA(2), args[3]);
		return args[1];
	case G_SIN:
		return FloatAsInt(sin(VMF(1)));
	case G_COS:
		return FloatAsInt(cos(VMF(1)));
	case G_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));
	case G_SQRT:
		return FloatAsInt(sqrt(VMF(1)));
	case G_MATRIXMULTIPLY:
		MatrixMultiply(VMA(1), VMA(2), VMA(3));
		return 0;
	case G_ANGLEVECTORS:
		anglev3s(VMA(1), VMA(2), VMA(3), VMA(4));
		return 0;
	case G_PERPENDICULARVECTOR:
		perpv3(VMA(1), VMA(2));
		return 0;
	case G_FLOOR:
		return FloatAsInt(floor(VMF(1)));
	case G_CEIL:
		return FloatAsInt(ceil(VMF(1)));
	case G_ACOS:
		return FloatAsInt(Q_acos(VMF(1)));
	case G_ASIN:
		return FloatAsInt(asin(VMF(1)));
	case G_ATAN:
		return FloatAsInt(atan(VMF(1)));
	default:
		comerrorf(ERR_DROP, "Bad game system trap: %ld",
			(long int)args[0]);
	}
	return 0;
}

/*
 * svshutdownGameProgs
 *
 * Called every time a map changes
 */
void
svshutdownGameProgs(void)
{
	if(!gvm)
		return;
	vmcall(gvm, GAME_SHUTDOWN, qfalse);
	vmfree(gvm);
	gvm = NULL;
}

/*
 * svinitGameVM
 *
 * Called for both a full init and a restart
 */
static void
svinitGameVM(qbool restart)
{
	int i;

	/* start the entity parsing at the beginning */
	sv.entityParsePoint = CM_EntityString();

	/* clear all gentity pointers that might still be set from
	 * a previous level
	 * https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	 *   now done before GAME_INIT call */
	for(i = 0; i < sv_maxclients->integer; i++)
		svs.clients[i].gentity = NULL;

	/* use the current msec count for a random seed
	 * init for this gamestate */
	vmcall (gvm, GAME_INIT, sv.time, commillisecs(), restart);
}



/*
 * SV_RestartGameProgs
 *
 * Called on a map_restart, but not on a normal map change
 */
void
SV_RestartGameProgs(void)
{
	if(!gvm)
		return;
	vmcall(gvm, GAME_SHUTDOWN, qtrue);

	/* do a restart instead of a free */
	gvm = vmrestart(gvm, qtrue);
	if(!gvm)
		comerrorf(ERR_FATAL, "vmrestart on game failed");

	svinitGameVM(qtrue);
}


/*
 * svinitGameProgs
 *
 * Called on a normal map change, not on a map_restart
 */
void
svinitGameProgs(void)
{
	Cvar *var;
	/* FIXME these are temp while I make bots run in vm */
	extern int bot_enable;

	var = cvarget("bot_enable", "1", CVAR_LATCH);
	if(var)
		bot_enable = var->integer;
	else
		bot_enable = 0;

	/* load the dll or bytecode */
	gvm =
		vmcreate("game", SV_GameSystemCalls,
			cvargetf("vm_game"));
	if(!gvm)
		comerrorf(ERR_FATAL, "vmcreate on game failed");

	svinitGameVM(qfalse);
}


/*
 * svgamecmd
 *
 * See if the current console command is claimed by the game
 */
qbool
svgamecmd(void)
{
	if(sv.state != SS_GAME)
		return qfalse;

	return vmcall(gvm, GAME_CONSOLE_COMMAND);
}
