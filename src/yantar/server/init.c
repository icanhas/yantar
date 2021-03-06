/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "server.h"

/*
 * Creates and sends the server command necessary to update the CS index for the
 * given client
 */
static void
SV_SendConfigstring(Client *client, int index)
{
	int	maxChunkSize = MAX_STRING_CHARS - 24;
	int	len;

	len = strlen(sv.configstrings[index]);

	if(len >= maxChunkSize){
		int	sent = 0;
		int	remaining = len;
		char    *cmd;
		char	buf[MAX_STRING_CHARS];

		while(remaining > 0){
			if(sent == 0)
				cmd = "bcs0";
			else if(remaining < maxChunkSize)
				cmd = "bcs2";
			else
				cmd = "bcs1";
			Q_strncpyz(buf, &sv.configstrings[index][sent],
				maxChunkSize);

			SV_SendServerCommand(client, "%s %i \"%s\"\n", cmd,
				index, buf);

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	}else
		/* standard cs, just send it */
		SV_SendServerCommand(client, "cs %i \"%s\"\n", index,
			sv.configstrings[index]);
}

/*
 * Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
 * Configstring indexes that have changed while the client was in CS_PRIMED
 */
void
SV_UpdateConfigstrings(Client *client)
{
	int index;

	for(index = 0; index <= MAX_CONFIGSTRINGS; index++){
		/* if the CS hasn't changed since we went to CS_PRIMED, ignore */
		if(!client->csUpdated[index])
			continue;

		/* do not always send server info to all clients */
		if(index == CS_SERVERINFO && client->gentity &&
		   (client->gentity->r.svFlags & SVF_NOSERVERINFO))
			continue;
		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

void
SV_SetConfigstring(int index, const char *val)
{
	int i;
	Client *client;

	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		comerrorf (ERR_DROP, "SV_SetConfigstring: bad index %i", index);

	if(!val)
		val = "";

	/* don't bother broadcasting an update if no change */
	if(!strcmp(val, sv.configstrings[ index ]))
		return;

	/* change the string in sv */
	zfree(sv.configstrings[index]);
	sv.configstrings[index] = copystr(val);

	/* send it to all the clients if we aren't
	 * spawning a new server */
	if(sv.state == SS_GAME || sv.restarting)

		/* send the data to all relevent clients */
		for(i = 0, client = svs.clients; i < sv_maxclients->integer;
		    i++, client++){
			if(client->state < CS_ACTIVE){
				if(client->state == CS_PRIMED)
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			/* do not always send server info to all clients */
			if(index == CS_SERVERINFO && client->gentity &&
			   (client->gentity->r.svFlags & SVF_NOSERVERINFO))
				continue;

			SV_SendConfigstring(client, index);
		}
}

void
SV_GetConfigstring(int index, char *buffer, int bufferSize)
{
	if(bufferSize < 1)
		comerrorf(ERR_DROP, "SV_GetConfigstring: bufferSize == %i",
			bufferSize);
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		comerrorf (ERR_DROP, "SV_GetConfigstring: bad index %i", index);
	if(!sv.configstrings[index]){
		buffer[0] = 0;
		return;
	}

	Q_strncpyz(buffer, sv.configstrings[index], bufferSize);
}

void
SV_SetUserinfo(int index, const char *val)
{
	if(index < 0 || index >= sv_maxclients->integer)
		comerrorf (ERR_DROP, "SV_SetUserinfo: bad index %i", index);

	if(!val)
		val = "";

	Q_strncpyz(svs.clients[index].userinfo, val,
		sizeof(svs.clients[ index ].userinfo));
	Q_strncpyz(svs.clients[index].name, Info_ValueForKey(val,
			"name"), sizeof(svs.clients[index].name));
}

void
SV_GetUserinfo(int index, char *buffer, int bufferSize)
{
	if(bufferSize < 1)
		comerrorf(ERR_DROP, "SV_GetUserinfo: bufferSize == %i",
			bufferSize);
	if(index < 0 || index >= sv_maxclients->integer)
		comerrorf (ERR_DROP, "SV_GetUserinfo: bad index %i", index);
	Q_strncpyz(buffer, svs.clients[ index ].userinfo, bufferSize);
}

/*
 * Entity baselines are used to compress non-delta messages
 * to the clients -- only the fields that differ from the
 * baseline will be transmitted
 */
static void
SV_CreateBaseline(void)
{
	Sharedent *svent;
	int entnum;

	for(entnum = 1; entnum < sv.num_entities; entnum++){
		svent = SV_GentityNum(entnum);
		if(!svent->r.linked)
			continue;
		svent->s.number = entnum;

		/*
		 * take current state as baseline
		 */
		sv.svEntities[entnum].baseline = svent->s;
	}
}

static void
SV_BoundMaxClients(int minimum)
{
	/* get the current maxclients value */
	cvarget("sv_maxclients", "8", 0);

	sv_maxclients->modified = qfalse;

	if(sv_maxclients->integer < minimum)
		cvarsetstr("sv_maxclients", va("%i", minimum));
	else if(sv_maxclients->integer > MAX_CLIENTS)
		cvarsetstr("sv_maxclients", va("%i", MAX_CLIENTS));
}

/*
 * Called when a host starts a map when it wasn't running
 * one before.  Successive map or map_restart commands will
 * NOT cause this to be called, unless the game is exited to
 * the menu system first.
 */
static void
SV_Startup(void)
{
	if(svs.initialized)
		comerrorf(ERR_FATAL, "SV_Startup: svs.initialized");
	SV_BoundMaxClients(1);

	svs.clients = zalloc (sizeof(Client) * sv_maxclients->integer);
	if(com_dedicated->integer)
		svs.numSnapshotEntities = sv_maxclients->integer *
					  PACKET_BACKUP * 64;
	else
		/* we don't need nearly as many when playing locally */
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	svs.initialized = qtrue;

	/* Don't respect sv_killserver unless a server is actually running */
	if(sv_killserver->integer)
		cvarsetstr("sv_killserver", "0");

	cvarsetstr("sv_running", "1");

	/* Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network. */
	netjoinmulticast6();
}

void
SV_ChangeMaxClients(void)
{
	int oldMaxClients, i, count;
	Client *oldClients;

	/* get the highest client number in use */
	count = 0;
	for(i = 0; i < sv_maxclients->integer; i++)
		if(svs.clients[i].state >= CS_CONNECTED)
			if(i > count)
				count = i;
	count++;

	oldMaxClients = sv_maxclients->integer;
	/* never go below the highest client number in use */
	SV_BoundMaxClients(count);
	/* if still the same */
	if(sv_maxclients->integer == oldMaxClients)
		return;

	oldClients = hunkalloctemp(count * sizeof(Client));
	/* copy the clients to hunk memory */
	for(i = 0; i < count; i++){
		if(svs.clients[i].state >= CS_CONNECTED)
			oldClients[i] = svs.clients[i];
		else
			Q_Memset(&oldClients[i], 0, sizeof(Client));
	}

	/* free old clients arrays */
	zfree(svs.clients);

	/* allocate new clients */
	svs.clients = zalloc (sv_maxclients->integer * sizeof(Client));
	Q_Memset(svs.clients, 0, sv_maxclients->integer * sizeof(Client));

	/* copy the clients over */
	for(i = 0; i < count; i++)
		if(oldClients[i].state >= CS_CONNECTED)
			svs.clients[i] = oldClients[i];

	/* free the old clients on the hunk */
	hunkfreetemp(oldClients);

	/* allocate new snapshot entities */
	if(com_dedicated->integer)
		svs.numSnapshotEntities = sv_maxclients->integer *
					  PACKET_BACKUP * 64;
	else
		/* we don't need nearly as many when playing locally */
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
}

static void
SV_ClearServer(void)
{
	int i;

	for(i = 0; i < MAX_CONFIGSTRINGS; i++)
		if(sv.configstrings[i])
			zfree(sv.configstrings[i]);
	Q_Memset (&sv, 0, sizeof(sv));
}

/*
 * touch the cgame.vm so that a pure client can load it if it's in a separate pk3
 */
static void
SV_TouchCGame(void)
{
	Fhandle f;
	char filename[MAX_QPATH];

	Q_sprintf(filename, sizeof(filename), Pvmfiles "/%s.qvm", "cgame");
	fsopenr(filename, &f, qfalse);
	if(f)
		fsclose(f);
}

/*
 * Change the server to a new map, taking all connected
 * clients along with it.
 * This is NOT called for map_restart
 */
void
SV_SpawnServer(char *server, qbool killBots)
{
	int i, checksum;
	qbool isBot;
	char	systemInfo[16384];
	const char *p;

	svshutdownGameProgs();	/* shut down any existing game */

	comprintf("------ Server Initialization ------\n");
	comprintf("Server: %s\n",server);

	/*
	 * FIXME: CL_ stuff should not be here
	 */
	if(!com_dedicated->integer){
		clmaploading();		/* connect the client to the server and print some status stuff */
		clshutdownall(qfalse);	/* make sure all the client stuff is unloaded */
		clstarthunkusers(qtrue);	/* Restart renderer */
	}
	hunkclear();	/* clear it all because we're (re)loading the server */

	CM_ClearMap();

	/* init client structures and svs.numSnapshotEntities */
	if(!cvargetf("sv_running"))
		SV_Startup();
	else if(sv_maxclients->modified)
		SV_ChangeMaxClients();

	fsclearpakrefs(0);
	
	svs.snapshotEntities = hunkalloc(sizeof(Entstate)*svs.numSnapshotEntities, Hhigh);
	svs.nextSnapshotEntities = 0;

	/* 
	 * toggle the server bit so clients can detect that a
	 * server has changed 
	 */
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	/* 
	 * set nextmap to the same map, but it may be overriden
	 * by the game startup or another console command 
	 */
	cvarsetstr("nextmap", "map_restart 0");
/*	cvarsetstr( "nextmap", va("map %s", server) ); */

	for(i=0; i<sv_maxclients->integer; i++){
		/* save when the server started for each client already connected */
		if(svs.clients[i].state >= CS_CONNECTED)
			svs.clients[i].oldServerTime = sv.time;
	}

	/* wipe the entire per-level structure */
	SV_ClearServer();
	for(i = 0; i < MAX_CONFIGSTRINGS; i++)
		sv.configstrings[i] = copystr("");

	cvarsetstr("cl_paused", "0");

	/* get a new checksum feed and restart the file system */
	sv.checksumFeed = (((int)rand() << 16) ^ rand()) ^ commillisecs();
	fsrestart(sv.checksumFeed);

	CM_LoadMap(va(Pmaps "/%s.bsp", server), qfalse, &checksum);


	cvarsetstr("mapname", server);	/* set serverinfo visible name */

	cvarsetstr("sv_mapChecksum", va("%i",checksum));

	sv.serverId = com_frameTime;	/* serverid should be different each time */
	sv.restartedServerId = sv.serverId;	/* I suppose the init here is just to be safe */
	sv.checksumFeedServerId = sv.serverId;
	cvarsetstr("sv_serverid", va("%i", sv.serverId));

	SV_ClearWorld();	/* clear physics interaction links */

	/* 
	 * media configstring setting should be done during
	 * the loading stage, so connected clients don't have
	 * to load during actual gameplay 
	 */
	sv.state = SS_LOADING;

	svinitGameProgs();	/* load and spawn all other entities */
	sv_gametype->modified = qfalse;	/* don't allow a map_restart if game is modified */

	/* run a few frames to allow everything to settle */
	for(i = 0; i < 3; i++){
		vmcall (gvm, GAME_RUN_FRAME, sv.time);
		SV_BotFrame (sv.time);
		sv.time += 100;
		svs.time += 100;
	}

	SV_CreateBaseline();	/* create a baseline for more efficient communications */

	for(i=0; i<sv_maxclients->integer; i++){
		/* send the new gamestate to all connected clients */
		if(svs.clients[i].state >= CS_CONNECTED){
			char *denied;

			if(svs.clients[i].netchan.remoteAddress.type ==
			   NA_BOT){
				if(killBots){
					SV_DropClient(&svs.clients[i], "");
					continue;
				}
				isBot = qtrue;
			}else
				isBot = qfalse;

			/* connect the client again */
			denied =
				vmexplicitargptr(gvm,
					vmcall(gvm, GAME_CLIENT_CONNECT, i,
						qfalse, isBot));	/* firstTime = qfalse */
			if(denied){
				/* 
				 * this generally shouldn't happen, because the client
				 * was connected before the level change 
				 */
				SV_DropClient(&svs.clients[i], denied);
			}else{
				if(!isBot){
					/* 
					 * when we get the next packet from a connected client,
					 * the new gamestate will be sent 
					 */
					svs.clients[i].state = CS_CONNECTED;
				}else{
					Client *client;
					Sharedent *ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum(i);
					ent->s.number	= i;
					client->gentity = ent;
					client->deltaMessage = -1;
					client->lastSnapshotTime = 0;	/* generate a snapshot immediately */
					vmcall(gvm, GAME_CLIENT_BEGIN, i);
				}
			}
		}
	}

	/* run another frame to allow things to look at all the players */
	vmcall (gvm, GAME_RUN_FRAME, sv.time);
	SV_BotFrame (sv.time);
	sv.time += 100;
	svs.time += 100;

	if(sv_pure->integer){
		/* 
		 * the server sends these to the clients so they will only
		 * load pk3s also loaded at the server 
		 */
		p = fsloadedpakchecksums();
		cvarsetstr("sv_paks", p);
		if(strlen(p) == 0)
			comprintf(
				"WARNING: sv_pure set but no PK3 files loaded\n");
		p = fsloadedpaknames();
		cvarsetstr("sv_pakNames", p);

		/* 
		 * if a dedicated pure server we need to touch the cgame because it could be in a
		 * seperate pk3 file and the client will need to load the latest cgame.qvm 
		 */
		if(com_dedicated->integer)
			SV_TouchCGame();
	}else{
		cvarsetstr("sv_paks", "");
		cvarsetstr("sv_pakNames", "");
	}
	/* 
	 * the server sends these to the clients so they can figure
	 * out which pk3s should be auto-downloaded 
	 */
	p = fsreferencedpakchecksums();
	cvarsetstr("sv_referencedPaks", p);
	p = fsreferencedpaknames();
	cvarsetstr("sv_referencedPakNames", p);

	/* save systeminfo and serverinfo strings */
	Q_strncpyz(systemInfo, cvargetbiginfostr(CVAR_SYSTEMINFO),
		sizeof(systemInfo));
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring(CS_SYSTEMINFO, systemInfo);

	SV_SetConfigstring(CS_SERVERINFO, cvargetinfostr(CVAR_SERVERINFO));
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	/* 
	 * any media configstring setting now should issue a warning
	 * and any configstring changes should be reliably transmitted
	 * to all clients 
	 */
	sv.state = SS_GAME;

	SV_Heartbeat_f();	/* One night to be confused */
	hunksetmark();
	comprintf ("-----------------------------------\n");
}

/*
 * Only called at main exe startup, not for each game
 */
void
svinit(void)
{
	int index;

	SV_AddOperatorCommands ();

	/* serverinfo vars */
	cvarget ("dmflags", "0", CVAR_SERVERINFO);
	cvarget ("fraglimit", "20", CVAR_SERVERINFO);
	cvarget ("timelimit", "0", CVAR_SERVERINFO);
	sv_gametype = cvarget ("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH);
	cvarget ("sv_keywords", "", CVAR_SERVERINFO);
	sv_mapname = cvarget ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = cvarget ("sv_privateClients", "0",
		CVAR_SERVERINFO);
	sv_hostname = cvarget ("sv_hostname", "noname",
		CVAR_SERVERINFO | CVAR_ARCHIVE);
	sv_maxclients = cvarget ("sv_maxclients", "8",
		CVAR_SERVERINFO | CVAR_LATCH);

	sv_minRate = cvarget ("sv_minRate", "0",
		CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_maxRate = cvarget ("sv_maxRate", "0",
		CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_dlRate = cvarget("sv_dlRate", "100",
		CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_minPing = cvarget ("sv_minPing", "0",
		CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_maxPing = cvarget ("sv_maxPing", "0",
		CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_floodProtect = cvarget ("sv_floodProtect", "1",
		CVAR_ARCHIVE | CVAR_SERVERINFO);

	/* systeminfo */
	cvarget ("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM);
	sv_serverid = cvarget ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM);
	sv_pure = cvarget ("sv_pure", "0", CVAR_SYSTEMINFO);
#ifdef USE_VOIP
	sv_voip = cvarget("sv_voip", "1", CVAR_SYSTEMINFO | CVAR_LATCH);
	cvarcheckrange(sv_voip, 0, 1, qtrue);
#endif
	cvarget ("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	cvarget ("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);
	cvarget ("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	cvarget ("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);

	/* server vars */
	sv_rconPassword = cvarget ("rconPassword", "", CVAR_TEMP);
	sv_privatePassword = cvarget ("sv_privatePassword", "", CVAR_TEMP);
	sv_fps = cvarget ("sv_fps", "20", CVAR_TEMP);
	sv_timeout = cvarget ("sv_timeout", "200", CVAR_TEMP);
	sv_zombietime = cvarget ("sv_zombietime", "2", CVAR_TEMP);
	cvarget ("nextmap", "", CVAR_TEMP);

	sv_allowDownload = cvarget ("sv_allowDownload", "0", CVAR_SERVERINFO);
	cvarget ("sv_dlURL", "", CVAR_SERVERINFO | CVAR_ARCHIVE);

	sv_master[0] = cvarget("sv_master1", MASTER_SERVER_NAME, 0);
	for(index = 1; index < MAX_MASTER_SERVERS; index++)
		sv_master[index] = cvarget(va("sv_master%d",
				index + 1), "", CVAR_ARCHIVE);

	sv_reconnectlimit = cvarget ("sv_reconnectlimit", "3", 0);
	sv_showloss = cvarget ("sv_showloss", "0", 0);
	sv_padPackets	 = cvarget ("sv_padPackets", "0", 0);
	sv_killserver = cvarget ("sv_killserver", "0", 0);
	sv_mapChecksum = cvarget ("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = cvarget ("sv_lanForceRate", "1", CVAR_ARCHIVE);
#ifndef STANDALONE
	sv_strictAuth = cvarget ("sv_strictAuth", "1", CVAR_ARCHIVE);
#endif
	sv_banFile = cvarget("sv_banFile", "serverbans.dat", CVAR_ARCHIVE);

	/* initialize bot cvars so they are listed and can be set before loading the botlib */
	SV_BotInitCvars();

	/* init the botlib here because we need the pre-compiler in the UI */
	SV_BotInitBotLib();

	/* Load saved bans */
	cbufaddstr("rehashbans\n");
}

/*
 * Used by svshutdown to send a final message to all
 * connected clients before the server goes down.  The messages are sent immediately,
 * not just stuck on the outgoing message list, because the server is going
 * to totally exit after returning from this function.
 */
void
SV_FinalMessage(char *message)
{
	int i, j;
	Client *cl;

	/* send it twice, ignoring rate */
	for(j = 0; j < 2; j++){
		for(i=0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++){
			if(cl->state < CS_CONNECTED)
				continue;
			/* don't send a disconnect to a local client */
			if(cl->netchan.remoteAddress.type != NA_LOOPBACK){
				SV_SendServerCommand(cl, "print \"%s\n\"\n", message);
				SV_SendServerCommand(cl, "disconnect \"%s\"", message);
			}
			/* force a snapshot to be sent */
			cl->lastSnapshotTime = 0;
			SV_SendClientSnapshot(cl);
		}
	}
}

/*
 * Called when each game quits,
 * before sysquit or syserrorf
 */
void
svshutdown(char *finalmsg)
{
	if(!com_sv_running || !com_sv_running->integer)
		return;

	comprintf("----- Server Shutdown (%s) -----\n", finalmsg);

	netleavemulticast6();

	if(svs.clients && !com_errorEntered)
		SV_FinalMessage(finalmsg);

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	svshutdownGameProgs();

	/* free current level */
	SV_ClearServer();

	/* free server static data */
	if(svs.clients){
		int index;

		for(index = 0; index < sv_maxclients->integer; index++)
			SV_FreeClient(&svs.clients[index]);

		zfree(svs.clients);
	}
	Q_Memset(&svs, 0, sizeof(svs));

	cvarsetstr("sv_running", "0");
	cvarsetstr("ui_singlePlayerActive", "0");

	comprintf("---------------------------\n");

	/* disconnect any local clients */
	if(sv_killserver->integer != 2)
		cldisconnect(qfalse);
}
