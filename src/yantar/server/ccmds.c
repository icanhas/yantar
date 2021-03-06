/*
 * These operator commands can only be entered from stdin or by a remote operator datagram
 */
 /*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "server.h"

/*
 * Returns the player with player id or name from cmdargv(1)
 */
static Client *
SV_GetPlayerByHandle(void)
{
	Client *cl;
	int i;
	char *s;
	char	cleanName[64];

	/* make sure server is running */
	if(!com_sv_running->integer)
		return NULL;
	if(cmdargc() < 2){
		comprintf("No player specified.\n");
		return NULL;
	}

	s = cmdargv(1);
	/* Check whether this is a numeric player handle */
	for(i = 0; s[i] >= '0' && s[i] <= '9'; i++)
		;
	if(!s[i]){
		int plid = atoi(s);

		/* Check for numeric playerid match */
		if(plid >= 0 && plid < sv_maxclients->integer){
			cl = &svs.clients[plid];

			if(cl->state)
				return cl;
		}
	}

	/* check for a name match */
	for(i=0, cl=svs.clients; i < sv_maxclients->integer; i++,cl++){
		if(!cl->state)
			continue;
		if(!Q_stricmp(cl->name, s))
			return cl;

		Q_strncpyz(cleanName, cl->name, sizeof(cleanName));
		Q_cleanstr(cleanName);
		if(!Q_stricmp(cleanName, s))
			return cl;
	}
	comprintf("Player %s is not on the server\n", s);
	return NULL;
}

/*
 * Returns the player with idnum from cmdargv(1)
 */
static Client *
SV_GetPlayerByNum(void)
{
	Client *cl;
	int i, idnum;
	char *s;

	/* make sure server is running */
	if(!com_sv_running->integer)
		return NULL;
	if(cmdargc() < 2){
		comprintf("No player specified.\n");
		return NULL;
	}

	s = cmdargv(1);
	for(i = 0; s[i]; i++)
		if(s[i] < '0' || s[i] > '9'){
			comprintf("Bad slot number: %s\n", s);
			return NULL;
		}
	idnum = atoi(s);
	if(idnum < 0 || idnum >= sv_maxclients->integer){
		comprintf("Bad client slot: %i\n", idnum);
		return NULL;
	}

	cl = &svs.clients[idnum];
	if(!cl->state){
		comprintf("Client %i is not active\n", idnum);
		return NULL;
	}
	return cl;
}

/*
 * Restart the server on a different map
 */
static void
SV_Map_f(void)
{
	char *cmd;
	char	*map;
	qbool killBots, cheat;
	char	expanded[MAX_QPATH];
	char	mapname[MAX_QPATH];

	map = cmdargv(1);
	if(!map)
		return;

	/* make sure the level exists before trying to change, so that
	 * a typo at the server console won't end the game */
	Q_sprintf (expanded, sizeof(expanded), Pmaps "/%s.bsp", map);
	if(fsreadfile (expanded, NULL) == -1){
		comprintf ("Can't find map %s\n", expanded);
		return;
	}

	/* force latched values to get set */
	cvarget ("g_gametype", "0",
		CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH);

	cmd = cmdargv(0);
	if(Q_stricmpn(cmd, "sp", 2) == 0){
		cvarsetf("g_gametype", GT_SINGLE_PLAYER);
		cvarsetf("g_doWarmup", 0);
		/* may not set sv_maxclients directly, always set latched */
		cvarsetstrlatched("sv_maxclients", "8");
		cmd += 2;
		if(!Q_stricmp(cmd, "devmap"))
			cheat = qtrue;
		else
			cheat = qfalse;
		killBots = qtrue;
	}else{
		if(!Q_stricmp(cmd, "devmap")){
			cheat = qtrue;
			killBots = qtrue;
		}else{
			cheat = qfalse;
			killBots = qfalse;
		}
		if(sv_gametype->integer == GT_SINGLE_PLAYER)
			cvarsetf("g_gametype", GT_FFA);
	}

	/* save the map name here cause on a map restart we reload the user.cfg
	 * and thus nuke the arguments of the map command */
	Q_strncpyz(mapname, map, sizeof(mapname));

	/* start up the map */
	SV_SpawnServer(mapname, killBots);

	/* set the cheat value
	 * if the level was started with "map <levelname>", then
	 * cheats will not be allowed.  If started with "devmap <levelname>"
	 * then cheats will be allowed */
	if(cheat)
		cvarsetstr("sv_cheats", "1");
	else
		cvarsetstr("sv_cheats", "0");
}

/*
 * Completely restarts a level, but doesn't send a new gamestate to the clients.
 * This allows fair starts with variable load times.
 */
static void
SV_MapRestart_f(void)
{
	int i;
	Client *client;
	char *denied;
	qbool isBot;
	int delay;


	if(com_frameTime == sv.serverId)
		return;	/* don't restart twice in the same frame */
	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(sv.restartTime)
		return;

	if(cmdargc() > 1)
		delay = atoi(cmdargv(1));
	else
		delay = 5;
	if(delay && !cvargetf("g_doWarmup")){
		sv.restartTime = sv.time + delay * 1000;
		SV_SetConfigstring(CS_WARMUP, va("%i", sv.restartTime));
		return;
	}

	/* 
	 * check for changes in variables that can't just be restarted
	 * check for maxclients change 
	 */
	if(sv_maxclients->modified || sv_gametype->modified){
		char mapname[MAX_QPATH];

		comprintf("variable change -- restarting.\n");
		/* restart the map the slow way */
		Q_strncpyz(mapname, cvargetstr("mapname"), sizeof(mapname));
		SV_SpawnServer(mapname, qfalse);
		return;
	}

	/* 
	 * toggle the server bit so clients can detect that a
	 * map_restart has happened 
	 */
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	/* 
	 * generate a new serverid.
	 * don't update restartedserverId there, otherwise we won't deal
	 * correctly with multiple map_restart
	 */
	sv.serverId = com_frameTime;
	cvarsetstr("sv_serverid", va("%i", sv.serverId));

	/* if a map_restart occurs while a client is changing maps, we need
	 * to give them the correct time so that when they finish loading
	 * they don't violate the backwards time check in cl_cgame.c */
	for(i=0; i<sv_maxclients->integer; i++)
		if(svs.clients[i].state == CS_PRIMED)
			svs.clients[i].oldServerTime = sv.restartTime;

	/* 
	 * reset all the vm data in place without changing memory allocation
	 * note that we do NOT set sv.state = SS_LOADING, so configstrings that
	 * had been changed from their default values will generate broadcast updates 
	 */
	sv.state = SS_LOADING;
	sv.restarting = qtrue;
	SV_RestartGameProgs();
	/* run a few frames to allow everything to settle */
	/* FIXME? */
	for(i = 0; i < 3; i++){
		vmcall(gvm, GAME_RUN_FRAME, sv.time);
		sv.time += 100;
		svs.time += 100;
	}
	sv.state = SS_GAME;
	sv.restarting = qfalse;

	/* connect and begin all the clients */
	for(i=0; i<sv_maxclients->integer; i++){
		client = &svs.clients[i];
		/* send the new gamestate to all connected clients */
		if(client->state < CS_CONNECTED)
			continue;

		if(client->netchan.remoteAddress.type == NA_BOT)
			isBot = qtrue;
		else
			isBot = qfalse;

		/* add the map_restart command */
		SV_AddServerCommand(client, "map_restart\n");

		/* connect the client again, without the firstTime flag */
		denied = vmexplicitargptr(gvm,
				vmcall(gvm, GAME_CLIENT_CONNECT, i, qfalse,
					isBot));
		if(denied){
			/* 
			 * this generally shouldn't happen, because the client
			 * was connected before the level change 
			 */
			SV_DropClient(client, denied);
			comprintf("SV_MapRestart_f(%d): dropped client %i - denied!\n",
				delay, i);
			continue;
		}

		if(client->state == CS_ACTIVE)
			SV_ClientEnterWorld(client, &client->lastUsercmd);
		else
			/* 
			 * If we don't reset client->lastUsercmd and are restarting during map load,
			 * the client will hang because we'll use the last Usercmd from the previous map,
			 * which is wrong, obviously.
			 */
			SV_ClientEnterWorld(client, NULL);
	}

	/* run another frame to allow things to look at all the players */
	vmcall (gvm, GAME_RUN_FRAME, sv.time);
	sv.time += 100;
	svs.time += 100;
}

/*
 * Kick a user off of the server  FIXME: move to game (really?)
 */
static void
SV_Kick_f(void)
{
	Client *cl;
	int i;

	/* make sure server is running */
	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}

	if(cmdargc() != 2){
		comprintf (
			"Usage: kick <player name>\nkick all = kick everyone\nkick allbots = kick all bots\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if(!cl){
		if(!Q_stricmp(cmdargv(1), "all"))
			for(i=0, cl=svs.clients; i < sv_maxclients->integer;
			    i++,cl++){
				if(!cl->state)
					continue;
				if(cl->netchan.remoteAddress.type ==
				   NA_LOOPBACK)
					continue;
				SV_DropClient(cl, "was kicked");
				cl->lastPacketTime = svs.time;	/* in case there is a funny zombie */
			}
		else if(!Q_stricmp(cmdargv(1), "allbots"))
			for(i=0, cl=svs.clients; i < sv_maxclients->integer;
			    i++,cl++){
				if(!cl->state)
					continue;
				if(cl->netchan.remoteAddress.type != NA_BOT)
					continue;
				SV_DropClient(cl, "was kicked");
				cl->lastPacketTime = svs.time;	/* in case there is a funny zombie */
			}
		return;
	}
	if(cl->netchan.remoteAddress.type == NA_LOOPBACK){
		SV_SendServerCommand(NULL, "print \"%s\"",
			"Cannot kick host player\n");
		return;
	}

	SV_DropClient(cl, "was kicked");
	cl->lastPacketTime = svs.time;	/* in case there is a funny zombie */
}

#ifndef STANDALONE
/*
 * These functions require the auth server which of course is not
 * available anymore for stand-alone games.
 */

/*
 * Ban a user from being able to play on this server through the auth
 * server (by name).
 */
static void
SV_Ban_f(void)
{
	Client *cl;

	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(cmdargc() != 2){
		comprintf ("Usage: banuser <player name>\n");
		return;
	}
	cl = SV_GetPlayerByHandle();
	if(!cl)
		return;
	if(cl->netchan.remoteAddress.type == NA_LOOPBACK){
		SV_SendServerCommand(NULL, "print \"%s\"",
			"Cannot kick host player\n");
		return;
	}

	/* look up the authorize server's IP */
	if(!svs.authorizeAddress.ip[0] && svs.authorizeAddress.type !=
	   NA_BAD){
		comprintf("Resolving %s\n", AUTHORIZE_SERVER_NAME);
		if(!strtoaddr(AUTHORIZE_SERVER_NAME,
			   &svs.authorizeAddress, NA_IP)){
			comprintf("Couldn't resolve address\n");
			return;
		}
		svs.authorizeAddress.port = BigShort(PORT_AUTHORIZE);
		comprintf("%s resolved to %i.%i.%i.%i:%i\n",
			AUTHORIZE_SERVER_NAME,
			svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1],
			svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3],
			BigShort(
				svs.authorizeAddress.port));
	}

	/* otherwise send their ip to the authorize server */
	if(svs.authorizeAddress.type != NA_BAD){
		netprintOOB(NS_SERVER, svs.authorizeAddress,
			"banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0],
			cl->netchan.remoteAddress.ip[1],
			cl->netchan.remoteAddress.ip[2],
			cl->netchan.remoteAddress.ip[3]);
		comprintf("%s was banned from coming back\n", cl->name);
	}
}

/*
 * Ban a user from being able to play on this server through the auth
 * server (by client number).
 */
static void
SV_BanNum_f(void)
{
	Client *cl;

	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(cmdargc() != 2){
		comprintf ("Usage: banclient <client number>\n");
		return;
	}
	cl = SV_GetPlayerByNum();
	if(!cl)
		return;
	if(cl->netchan.remoteAddress.type == NA_LOOPBACK){
		SV_SendServerCommand(NULL, "print \"%s\"",
			"Cannot kick host player\n");
		return;
	}

	/* look up the authorize server's IP */
	if(!svs.authorizeAddress.ip[0] && svs.authorizeAddress.type !=
	   NA_BAD){
		comprintf("Resolving %s\n", AUTHORIZE_SERVER_NAME);
		if(!strtoaddr(AUTHORIZE_SERVER_NAME,
			   &svs.authorizeAddress, NA_IP)){
			comprintf("Couldn't resolve address\n");
			return;
		}
		svs.authorizeAddress.port = BigShort(PORT_AUTHORIZE);
		comprintf("%s resolved to %i.%i.%i.%i:%i\n",
			AUTHORIZE_SERVER_NAME,
			svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1],
			svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3],
			BigShort(
				svs.authorizeAddress.port));
	}

	/* otherwise send their ip to the authorize server */
	if(svs.authorizeAddress.type != NA_BAD){
		netprintOOB(NS_SERVER, svs.authorizeAddress,
			"banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0],
			cl->netchan.remoteAddress.ip[1],
			cl->netchan.remoteAddress.ip[2],
			cl->netchan.remoteAddress.ip[3]);
		comprintf("%s was banned from coming back\n", cl->name);
	}
}
#endif

/*
 * Load saved bans from file.
 */
static void
SV_RehashBans_f(void)
{
	int i, flen;
	Fhandle readfrom;
	char	*textbuf, *curpos, *maskpos, *newlinepos, *endpos;
	char	path[MAX_QPATH];

	serverBansCount = 0;

	if(!sv_banFile->string || !*sv_banFile->string)
		return;

	Q_sprintf(path, sizeof(path), "%s/%s",
		fsgetcurrentgamedir(), sv_banFile->string);
	flen = fssvopenr(path, &readfrom);
	if(flen < 2){
		/* Don't bother if file is too short. */
		fsclose(readfrom);
		return;
	}

	curpos = textbuf = zalloc(flen);
	flen = fsread(textbuf, flen, readfrom);
	fsclose(readfrom);
	endpos = textbuf + flen;

	for(i = 0; i < SERVER_MAXBANS && curpos + 2 < endpos; i++){
		/* find the end of the address string */
		for(maskpos = curpos + 2;
		    maskpos < endpos && *maskpos != ' '; maskpos++) ;

		if(maskpos + 1 >= endpos)
			break;

		*maskpos = '\0';
		maskpos++;

		/* find the end of the subnet specifier */
		for(newlinepos = maskpos; 
		   newlinepos < endpos && *newlinepos != '\n';
		   newlinepos++)
			;

		if(newlinepos >= endpos)
			break;

		*newlinepos = '\0';

		if(strtoaddr(curpos + 2, &serverBans[i].ip, NA_UNSPEC)){
			serverBans[i].isexception = (curpos[0] != '0');
			serverBans[i].subnet = atoi(maskpos);

			if(serverBans[i].ip.type == NA_IP &&
			   (serverBans[i].subnet < 1 ||
			    serverBans[i].subnet > 32))
			then
				serverBans[i].subnet = 32;
			else if(serverBans[i].ip.type == NA_IP6 &&
				(serverBans[i].subnet < 1 ||
				 serverBans[i].subnet > 128))
			then
				serverBans[i].subnet = 128;
		}
		curpos = newlinepos + 1;
	}
	serverBansCount = i;
	zfree(textbuf);
}

/*
 * Save bans to file.
 */
static void
SV_WriteBans(void)
{
	uint i;
	Fhandle writeto;
	char	filepath[MAX_QPATH];

	if(!sv_banFile->string || !*sv_banFile->string)
		return;

	Q_sprintf(filepath, sizeof(filepath), "%s/%s",
		fsgetcurrentgamedir(), sv_banFile->string);

	if((writeto = fssvopenw(filepath))){
		char writebuf[128];
		serverBan_t *curban;

		for(i = 0; i < serverBansCount; i++){
			curban = &serverBans[i];

			Q_sprintf(writebuf, sizeof(writebuf), "%d %s %d\n",
				curban->isexception, addrtostr(
					curban->ip), curban->subnet);
			fswrite(writebuf, strlen(writebuf), writeto);
		}

		fsclose(writeto);
	}
}

/*
 * Remove a ban or an exception from the list.
 */
static qbool
SV_DelBanEntryFromList(uint index)
{
	if(index == serverBansCount - 1)
		serverBansCount--;
	else if(index < ARRAY_LEN(serverBans) - 1){
		memmove(serverBans + index, serverBans + index + 1,
			(serverBansCount - index - 1) * sizeof(*serverBans));
		serverBansCount--;
	}else
		return qtrue;

	return qfalse;
}

/*
 * Parse a CIDR notation type string and return a Netaddr and suffix by reference
 */
static qbool
SV_ParseCIDRNotation(Netaddr *dest, int *mask, char *adrstr)
{
	char *suffix;

	suffix = strchr(adrstr, '/');
	if(suffix){
		*suffix = '\0';
		suffix++;
	}

	if(!strtoaddr(adrstr, dest, NA_UNSPEC))
		return qtrue;

	if(suffix){
		*mask = atoi(suffix);

		if(dest->type == NA_IP){
			if(*mask < 1 || *mask > 32)
				*mask = 32;
		}else if(*mask < 1 || *mask > 128)
			*mask = 128;
	}else if(dest->type == NA_IP)
		*mask = 32;
	else
		*mask = 128;

	return qfalse;
}

/*
 * Ban a user from being able to play on this server based on his ip address.
 *
 * FIXME: this is well worthy of a rewrite
 */
static void
SV_AddBanToList(qbool isexception)
{
	char	*banstring;
	char	addy2[NET_ADDRSTRMAXLEN];
	Netaddr ip;
	uint index;
	int argc;
	int mask;
	serverBan_t *curban;

	argc = cmdargc();

	if(argc < 2 || argc > 3){
		comprintf ("Usage: %s (ip[/subnet] | clientnum [subnet])\n", cmdargv(0));
		return;
	}
	if(serverBansCount > ARRAY_LEN(serverBans)){
		comprintf ("Error: Maximum number of bans/exceptions exceeded.\n");
		return;
	}

	banstring = cmdargv(1);
	if(strchr(banstring, '.') || strchr(banstring, ':')){
		/* This is an ip address, not a client num. */
		if(SV_ParseCIDRNotation(&ip, &mask, banstring)){
			comprintf("Error: Invalid address %s\n", banstring);
			return;
		}
	}else{
		Client *cl;

		/* client num. */
		if(!com_sv_running->integer){
			comprintf("Server is not running.\n");
			return;
		}

		cl = SV_GetPlayerByNum();

		if(!cl){
			comprintf("Error: Playernum %s does not exist.\n",
				cmdargv(
					1));
			return;
		}

		ip = cl->netchan.remoteAddress;

		if(argc == 3){
			mask = atoi(cmdargv(2));

			if(ip.type == NA_IP){
				if(mask < 1 || mask > 32)
					mask = 32;
			}else if(mask < 1 || mask > 128)
				mask = 128;
		}else
			mask = (ip.type == NA_IP6) ? 128 : 32;
	}

	if(ip.type != NA_IP && ip.type != NA_IP6){
		comprintf(
			"Error: Can ban players connected via the internet only.\n");
		return;
	}

	/* first check whether a conflicting ban exists that would supersede the new one. */
	for(index = 0; index < serverBansCount; index++){
		curban = &serverBans[index];
		if(curban->subnet <= mask){
			if((curban->isexception || !isexception) 
			   && equalbaseaddrmask(curban->ip, ip, curban->subnet))
			then{
				Q_strncpyz(addy2, addrtostr(ip),
					sizeof(addy2));

				comprintf("Error: %s %s/%d supersedes %s %s/%d\n",
					curban->isexception ? "Exception" :
					"Ban",
					addrtostr(curban->ip), curban->subnet,
					isexception ? "exception" : "ban", addy2,
					mask);
				return;
			}
		}
		if(curban->subnet >= mask)
			if(!curban->isexception && isexception &&
			   equalbaseaddrmask(curban->ip, ip, mask)){
				Q_strncpyz(addy2, addrtostr(
						curban->ip), sizeof(addy2));

				comprintf(
					"Error: %s %s/%d supersedes already existing %s %s/%d\n",
					isexception ? "Exception" : "Ban",
					addrtostr(
						ip), mask,
					curban->isexception ? "exception" :
					"ban", addy2, curban->subnet);
				return;
			}
	}

	/* now delete bans that are superseded by the new one */
	index = 0;
	while(index < serverBansCount){
		curban = &serverBans[index];

		if(curban->subnet > mask &&
		   (!curban->isexception ||
		    isexception) && equalbaseaddrmask(curban->ip, ip, mask))
			SV_DelBanEntryFromList(index);
		else
			index++;
	}

	serverBans[serverBansCount].ip = ip;
	serverBans[serverBansCount].subnet = mask;
	serverBans[serverBansCount].isexception = isexception;
	serverBansCount++;
	SV_WriteBans();
	comprintf("Added %s: %s/%d\n", isexception ? "ban exception" : "ban",
		addrtostr(ip), mask);
}

/*
 * Remove a ban or an exception from the list.
 */
static void
SV_DelBanFromList(qbool isexception)
{
	uint index, count = 0, todel;
	int mask;
	Netaddr ip;
	char *banstring;

	if(cmdargc() != 2){
		comprintf ("Usage: %s (ip[/subnet] | num)\n", cmdargv(0));
		return;
	}

	banstring = cmdargv(1);

	if(strchr(banstring, '.') || strchr(banstring, ':')){
		serverBan_t *curban;

		if(SV_ParseCIDRNotation(&ip, &mask, banstring)){
			comprintf("Error: Invalid address %s\n", banstring);
			return;
		}

		index = 0;

		while(index < serverBansCount){
			curban = &serverBans[index];

			if(curban->isexception == isexception
			   && curban->subnet >= mask
			   && equalbaseaddrmask(curban->ip, ip, mask))
			then{
				comprintf("Deleting %s %s/%d\n",
					isexception ? "exception" : "ban",
					addrtostr(
						curban->ip), curban->subnet);
				SV_DelBanEntryFromList(index);
			}else
				index++;
		}
	}else{
		todel = atoi(cmdargv(1));

		if(todel < 1 || todel > serverBansCount){
			comprintf("Error: Invalid ban number given\n");
			return;
		}

		for(index = 0; index < serverBansCount; index++)
			if(serverBans[index].isexception == isexception){
				count++;

				if(count == todel){
					comprintf("Deleting %s %s/%d\n",
						isexception ? "exception" :
						"ban",
						addrtostr(serverBans[index].ip),
						serverBans[index].subnet);
					SV_DelBanEntryFromList(index);

					break;
				}
			}
	}

	SV_WriteBans();
}


/*
 * List all bans and exceptions on console
 */
static void
SV_ListBans_f(void)
{
	uint index, count;
	serverBan_t *ban;

	/* List all bans */
	for(index = count = 0; index < serverBansCount; index++){
		ban = &serverBans[index];
		if(!ban->isexception){
			count++;

			comprintf("Ban #%d: %s/%d\n", count,
				addrtostr(ban->ip), ban->subnet);
		}
	}
	/* List all exceptions */
	for(index = count = 0; index < serverBansCount; index++){
		ban = &serverBans[index];
		if(ban->isexception){
			count++;

			comprintf("Except #%d: %s/%d\n", count,
				addrtostr(ban->ip), ban->subnet);
		}
	}
}

/*
 * Delete all bans and exceptions.
 */
static void
SV_FlushBans_f(void)
{
	serverBansCount = 0;
	SV_WriteBans();	/* empty the ban file. */
	comprintf("All bans and exceptions have been deleted.\n");
}

static void
SV_BanAddr_f(void)
{
	SV_AddBanToList(qfalse);
}

static void
SV_ExceptAddr_f(void)
{
	SV_AddBanToList(qtrue);
}

static void
SV_BanDel_f(void)
{
	SV_DelBanFromList(qfalse);
}

static void
SV_ExceptDel_f(void)
{
	SV_DelBanFromList(qtrue);
}

/*
 * Kick a user off of the server  FIXME: move to game
 */
static void
SV_KickNum_f(void)
{
	Client *cl;

	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(cmdargc() != 2){
		comprintf ("Usage: kicknum <client number>\n");
		return;
	}

	cl = SV_GetPlayerByNum();
	if(!cl)
		return;
	if(cl->netchan.remoteAddress.type == NA_LOOPBACK){
		SV_SendServerCommand(NULL, "print \"%s\"",
			"Cannot kick host player\n");
		return;
	}

	SV_DropClient(cl, "was kicked");
	cl->lastPacketTime = svs.time;	/* in case there is a funny zombie */
}

static void
SV_Status_f(void)
{
	int i, j, l;
	Client *cl;
	Playerstate *ps;
	const char *s;
	int ping;

	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	comprintf("map: %s\n", sv_mapname->string);
	comprintf(
		"num score ping name            lastmsg address               qport rate\n");
	comprintf(
		"--- ----- ---- --------------- ------- --------------------- ----- -----\n");
	for(i=0,cl=svs.clients; i < sv_maxclients->integer; i++,cl++){
		if(!cl->state)
			continue;
		comprintf ("%3i ", i);
		ps = SV_GameClientNum(i);
		comprintf ("%5i ", ps->persistant[PERS_SCORE]);

		if(cl->state == CS_CONNECTED)
			comprintf ("CNCT ");
		else if(cl->state == CS_ZOMBIE)
			comprintf ("ZMBI ");
		else{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			comprintf ("%4i ", ping);
		}

		comprintf ("%s", cl->name);

		/* TTimo adding a ^7 to reset the color
		 * NOTE: colored names in status breaks the padding (WONTFIX) */
		comprintf("^7");
		l = 14 - strlen(cl->name);
		j = 0;

		do {
			comprintf(" ");
			j++;
		} while(j < l);

		comprintf("%7i ", svs.time - cl->lastPacketTime);

		s = addrtostr(cl->netchan.remoteAddress);
		comprintf ("%s", s);
		l = 22 - strlen(s);
		j = 0;

		do {
			comprintf(" ");
			j++;
		} while(j < l);
		comprintf("%5i", cl->netchan.qport);
		comprintf(" %5i", cl->rate);
		comprintf("\n");
	}
	comprintf ("\n");
}

static void
SV_ConSay_f(void)
{
	char	*p;
	char	text[1024];

	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(cmdargc () < 2)
		return;
	strcpy (text, "console: ");
	p = cmdargs();
	if(*p == '"'){
		p++;
		p[strlen(p)-1] = 0;
	}
	strcat(text, p);
	SV_SendServerCommand(NULL, "chat \"%s\"", text);
}

static void
SV_ConTell_f(void)
{
	char *p;
	char text[1024];
	Client *cl;
	
	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(cmdargc() < 3){
		comprintf("Usage: tell <client num> <text>\n");
		return;
	}
	cl = SV_GetPlayerByNum();
	if(cl == nil)
		return;
	strcpy(text, "console tell: ");
	p = cmdargsfrom(2);
	if(*p == '"'){
		p++;
		p[strlen(p)-1] = 0;
	}
	strcat(text, p);
	SV_SendServerCommand(cl, "chat \"%s\"", text);
}

/*
 * Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
 */
void
SV_Heartbeat_f(void)
{
	svs.nextHeartbeatTime = -9999999;
}

/*
 * Examine the serverinfo string
 */
static void
SV_Serverinfo_f(void)
{
	comprintf ("Server info settings:\n");
	infoprint (cvargetinfostr(CVAR_SERVERINFO));
}

/*
 * Examine or change the serverinfo string
 */
static void
SV_Systeminfo_f(void)
{
	comprintf ("System info settings:\n");
	infoprint (cvargetbiginfostr(CVAR_SYSTEMINFO));
}

/*
 * Examine all a users info strings FIXME: move to game
 */
static void
SV_DumpUser_f(void)
{
	Client *cl;

	if(!com_sv_running->integer){
		comprintf("Server is not running.\n");
		return;
	}
	if(cmdargc() != 2){
		comprintf ("Usage: dumpuser <userid>\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if(!cl)
		return;

	comprintf("userinfo\n");
	comprintf("--------\n");
	infoprint(cl->userinfo);
}

static void
SV_KillServer_f(void)
{
	svshutdown("killserver");
}

static void
SV_CompleteMapName(char *args, int argNum)
{
	UNUSED(args);
	if(argNum == 2)
		fieldcompletefilename("maps", "bsp", qtrue, qfalse);
}

void
SV_AddOperatorCommands(void)
{
	static qbool initialized;

	if(initialized)
		return;
	initialized = qtrue;

	cmdadd ("heartbeat", SV_Heartbeat_f);
	cmdadd ("kick", SV_Kick_f);
#ifndef STANDALONE
	if(!com_standalone->integer){
		cmdadd ("banUser", SV_Ban_f);
		cmdadd ("banClient", SV_BanNum_f);
	}
#endif
	cmdadd ("clientkick", SV_KickNum_f);
	cmdadd ("status", SV_Status_f);
	cmdadd ("serverinfo", SV_Serverinfo_f);
	cmdadd ("systeminfo", SV_Systeminfo_f);
	cmdadd ("dumpuser", SV_DumpUser_f);
	cmdadd ("map_restart", SV_MapRestart_f);
	cmdadd ("sectorlist", SV_SectorList_f);
	cmdadd ("map", SV_Map_f);
	cmdsetcompletion("map", SV_CompleteMapName);
#ifndef PRE_RELEASE_DEMO
	cmdadd ("devmap", SV_Map_f);
	cmdsetcompletion("devmap", SV_CompleteMapName);
	cmdadd ("spmap", SV_Map_f);
	cmdsetcompletion("spmap", SV_CompleteMapName);
	cmdadd ("spdevmap", SV_Map_f);
	cmdsetcompletion("spdevmap", SV_CompleteMapName);
#endif
	cmdadd ("killserver", SV_KillServer_f);
	if(com_dedicated->integer){
		cmdadd("say", SV_ConSay_f);
		cmdadd("tell", SV_ConTell_f);
	}
	cmdadd("rehashbans", SV_RehashBans_f);
	cmdadd("listbans", SV_ListBans_f);
	cmdadd("banaddr", SV_BanAddr_f);
	cmdadd("exceptaddr", SV_ExceptAddr_f);
	cmdadd("bandel", SV_BanDel_f);
	cmdadd("exceptdel", SV_ExceptDel_f);
	cmdadd("flushbans", SV_FlushBans_f);
}

void
SV_RemoveOperatorCommands(void)
{
#if 0
	/* removing these won't let the server start again */
	cmdremove ("heartbeat");
	cmdremove ("kick");
	cmdremove ("banUser");
	cmdremove ("banClient");
	cmdremove ("status");
	cmdremove ("serverinfo");
	cmdremove ("systeminfo");
	cmdremove ("dumpuser");
	cmdremove ("map_restart");
	cmdremove ("sectorlist");
	cmdremove ("say");
#endif
}
