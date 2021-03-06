/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "server.h"

#ifdef USE_VOIP
Cvar *sv_voip;
#endif

serverStatic_t svs;		/* persistant server info */
Server	sv;		/* local server */
Vm		*gvm = NULL;	/* game virtual machine */

Cvar		*sv_fps = NULL;		/* time rate for running non-clients */
Cvar		*sv_timeout;		/* seconds without any message */
Cvar		*sv_zombietime;		/* seconds to sink messages after disconnect */
Cvar		*sv_rconPassword;	/* password for remote server commands */
Cvar		*sv_privatePassword;	/* password for the privateClient slots */
Cvar		*sv_allowDownload;
Cvar		*sv_maxclients;

Cvar		*sv_privateClients;	/* number of clients reserved for password */
Cvar		*sv_hostname;
Cvar		*sv_master[MAX_MASTER_SERVERS];	/* master server ip address */
Cvar		*sv_reconnectlimit;		/* minimum seconds between connect messages */
Cvar		*sv_showloss;			/* report when usercmds are lost */
Cvar		*sv_padPackets;			/* add nop bytes to messages */
Cvar		*sv_killserver;			/* menu system can set to 1 to shut server down */
Cvar		*sv_mapname;
Cvar		*sv_mapChecksum;
Cvar		*sv_serverid;
Cvar		*sv_minRate;
Cvar		*sv_maxRate;
Cvar		*sv_dlRate;
Cvar		*sv_minPing;
Cvar		*sv_maxPing;
Cvar		*sv_gametype;
Cvar		*sv_pure;
Cvar		*sv_floodProtect;
Cvar		*sv_lanForceRate;	/* dedicated 1 (LAN) server forces local client rates to 99999 (bug #491) */
#ifndef STANDALONE
Cvar		*sv_strictAuth;
#endif
Cvar		*sv_banFile;

serverBan_t serverBans[SERVER_MAXBANS];
uint serverBansCount = 0;

/*
 *
 * EVENT MESSAGES
 *
 */

/*
 * SV_ExpandNewlines
 *
 * Converts newlines to "\n" so a line prints nicer
 */
static char     *
SV_ExpandNewlines(char *in)
{
	static char string[1024];
	int l;

	l = 0;
	while(*in && l < sizeof(string) - 3){
		if(*in == '\n'){
			string[l++]	= '\\';
			string[l++]	= 'n';
		}else
			string[l++] = *in;
		in++;
	}
	string[l] = 0;

	return string;
}

/*
 * SV_ReplacePendingServerCommands
 *
 * FIXME: This is ugly
 */
#if 0	/* unused */
static int
SV_ReplacePendingServerCommands(Client *client, const char *cmd)
{
	int i, index, csnum1, csnum2;

	for(i = client->reliableSent+1; i <= client->reliableSequence; i++){
		index = i & (MAX_RELIABLE_COMMANDS - 1);
		if(!Q_strncmp(cmd, client->reliableCommands[ index ],
			   strlen("cs"))){
			sscanf(cmd, "cs %i", &csnum1);
			sscanf(client->reliableCommands[ index ], "cs %i",
				&csnum2);
			if(csnum1 == csnum2){
				Q_strncpyz(
					client->reliableCommands[ index ], cmd,
					sizeof(client->reliableCommands[ index ]));
				/*
				 * if ( client->netchan.remoteAddress.type != NA_BOT ) {
				 *      comprintf( "WARNING: client %i removed double pending config string %i: %s\n", client-svs.clients, csnum1, cmd );
				 * }
				 */
				return qtrue;
			}
		}
	}
	return qfalse;
}
#endif

/*
 * SV_AddServerCommand
 *
 * The given command will be transmitted to the client, and is guaranteed to
 * not have future Snap executed before it is executed
 */
void
SV_AddServerCommand(Client *client, const char *cmd)
{
	int index, i;

	/* this is very ugly but it's also a waste to for instance send multiple config string updates
	 * for the same config string index in one snapshot */
/* if ( SV_ReplacePendingServerCommands( client, cmd ) ) {
 *      return;
 * } */

	/* do not send commands until the gamestate has been sent */
	if(client->state < CS_PRIMED)
		return;

	client->reliableSequence++;
	/* if we would be losing an old command that hasn't been acknowledged,
	 * we must drop the connection
	 * we check == instead of >= so a broadcast print added by SV_DropClient()
	 * doesn't cause a recursive drop client */
	if(client->reliableSequence - client->reliableAcknowledge ==
	   MAX_RELIABLE_COMMANDS + 1){
		comprintf("===== pending server commands =====\n");
		for(i = client->reliableAcknowledge + 1;
		    i <= client->reliableSequence; i++)
			comprintf("cmd %5d: %s\n", i,
				client->reliableCommands[ i &
							  (MAX_RELIABLE_COMMANDS
							   -1) ]);
		comprintf("cmd %5d: %s\n", i, cmd);
		SV_DropClient(client, "Server command overflow");
		return;
	}
	index = client->reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
	Q_strncpyz(client->reliableCommands[ index ], cmd,
		sizeof(client->reliableCommands[ index ]));
}


/*
 * SV_SendServerCommand
 *
 * Sends a reliable command string to be interpreted by
 * the client game module: "cp", "print", "chat", etc
 * A NULL client will broadcast to all clients
 */
void QDECL
SV_SendServerCommand(Client *cl, const char *fmt, ...)
{
	va_list argptr;
	byte	message[MAX_MSGLEN];
	Client *client;
	int j;

	va_start (argptr,fmt);
	Q_vsnprintf ((char*)message, sizeof(message), fmt,argptr);
	va_end (argptr);

	/* Fix to http://aluigi.altervista.org/adv/q3msgboom-adv.txt
	 * The actual cause of the bug is probably further downstream
	 * and should maybe be addressed later, but this certainly
	 * fixes the problem for now */
	if(strlen ((char*)message) > 1022)
		return;

	if(cl != NULL){
		SV_AddServerCommand(cl, (char*)message);
		return;
	}

	/* hack to echo broadcast prints to console */
	if(com_dedicated->integer && !strncmp((char*)message, "print", 5))
		comprintf ("broadcast: %s\n",
			SV_ExpandNewlines((char*)message));

	/* send the data to all relevent clients */
	for(j = 0, client = svs.clients; j < sv_maxclients->integer;
	    j++, client++)
		SV_AddServerCommand(client, (char*)message);
}


/*
 *
 * MASTER SERVER FUNCTIONS
 *
 */

/*
 * SV_MasterHeartbeat
 *
 * Send a message to the masters every few minutes to
 * let it know we are alive, and log information.
 * We will also have a heartbeat sent when a server
 * changes from empty to non-empty, and full to non-full,
 * but not on every player enter or exit.
 */
#define HEARTBEAT_MSEC 300*1000
void
SV_MasterHeartbeat(const char *message)
{
	static Netaddr adr[MAX_MASTER_SERVERS][2];	/* [2] for v4 and v6 address for the same address string. */
	int	i;
	int	res;
	int	netenabled;

	netenabled = cvargeti("net_enabled");

	/* "dedicated 1" is for lan play, "dedicated 2" is for inet public play */
	if(!com_dedicated || com_dedicated->integer != 2 ||
	   !(netenabled & (NET_ENABLEV4 | NET_ENABLEV6)))
		return;		/* only dedicated servers send heartbeats */

	/* if not time yet, don't send anything */
	if(svs.time < svs.nextHeartbeatTime)
		return;

	svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

	/* send to group masters */
	for(i = 0; i < MAX_MASTER_SERVERS; i++){
		if(!sv_master[i]->string[0])
			continue;

		/* see if we haven't already resolved the name
		 * resolving usually causes hitches on win95, so only
		 * do it when needed */
		if(sv_master[i]->modified ||
		   (adr[i][0].type == NA_BAD && adr[i][1].type == NA_BAD)){
			sv_master[i]->modified = qfalse;

			if(netenabled & NET_ENABLEV4){
				comprintf("Resolving %s (IPv4)\n",
					sv_master[i]->string);
				res =
					strtoaddr(sv_master[i]->string,
						&adr[i][0],
						NA_IP);

				if(res == 2)
					/* if no port was specified, use the default master port */
					adr[i][0].port = BigShort(PORT_MASTER);

				if(res)
					comprintf(
						"%s resolved to %s\n",
						sv_master[i]->string,
						addrporttostr(adr[i][0]));
				else
					comprintf("%s has no IPv4 address.\n",
						sv_master[i]->string);
			}

			if(netenabled & NET_ENABLEV6){
				comprintf("Resolving %s (IPv6)\n",
					sv_master[i]->string);
				res =
					strtoaddr(sv_master[i]->string,
						&adr[i][1],
						NA_IP6);

				if(res == 2)
					/* if no port was specified, use the default master port */
					adr[i][1].port = BigShort(PORT_MASTER);

				if(res)
					comprintf(
						"%s resolved to %s\n",
						sv_master[i]->string,
						addrporttostr(adr[i][1]));
				else
					comprintf("%s has no IPv6 address.\n",
						sv_master[i]->string);
			}

			if(adr[i][0].type == NA_BAD && adr[i][1].type ==
			   NA_BAD){
				/* if the address failed to resolve, clear it
				 * so we don't take repeated dns hits */
				comprintf("Couldn't resolve address: %s\n",
					sv_master[i]->string);
				cvarsetstr(sv_master[i]->name, "");
				sv_master[i]->modified = qfalse;
				continue;
			}
		}


		comprintf ("Sending heartbeat to %s\n", sv_master[i]->string);

		/* this command should be changed if the server info / status format
		 * ever incompatably changes */

		if(adr[i][0].type != NA_BAD)
			netprintOOB(NS_SERVER, adr[i][0],
				"heartbeat %s\n",
				message);
		if(adr[i][1].type != NA_BAD)
			netprintOOB(NS_SERVER, adr[i][1],
				"heartbeat %s\n",
				message);
	}
}

/*
 * SV_MasterShutdown
 *
 * Informs all masters that this server is going down
 */
void
SV_MasterShutdown(void)
{
	/* send a hearbeat right now */
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat(HEARTBEAT_FOR_MASTER);

	/* send it again to minimize chance of drops */
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat(HEARTBEAT_FOR_MASTER);

	/* when the master tries to poll the server, it won't respond, so
	 * it will be removed from the list */
}


/*
 *
 * CONNECTIONLESS COMMANDS
 *
 */

typedef struct Leakybucket Leakybucket;
struct Leakybucket {
	Netaddrtype type;

	union {
		byte	_4[4];
		byte	_6[16];
	} ipv;

	int		lastTime;
	signed char	burst;

	long		hash;

	Leakybucket	*prev, *next;
};

/* This is deliberately quite large to make it more of an effort to DoS */
#define MAX_BUCKETS	16384
#define MAX_HASHES	1024

static Leakybucket	buckets[ MAX_BUCKETS ];
static Leakybucket	*bucketHashes[ MAX_HASHES ];

/*
 * SVC_HashForAddress
 */
static long
SVC_HashForAddress(Netaddr address)
{
	byte	*ip	= NULL;
	size_t size	= 0;
	size_t i;
	long	hash = 0;

	switch(address.type){
	case NA_IP:
		ip = address.ip;
		size = 4;
		break;
	case NA_IP6:
		ip = address.ip6;
		size = 16;
		break;
	default:
		break;
	}

	for(i = 0; i < size; i++)
		hash += (long)(ip[ i ]) * (i + 119);

	hash	= (hash ^ (hash >> 10) ^ (hash >> 20));
	hash	&= (MAX_HASHES - 1);

	return hash;
}

/*
 * SVC_BucketForAddress
 *
 * Find or allocate a bucket for an address
 */
static Leakybucket *
SVC_BucketForAddress(Netaddr address, int burst, int period)
{
	Leakybucket *bucket = NULL;
	int	i;
	long	hash = SVC_HashForAddress(address);
	int	now = sysmillisecs();

	for(bucket = bucketHashes[ hash ]; bucket; bucket = bucket->next){
		switch(bucket->type){
		case NA_IP:
			if(memcmp(bucket->ipv._4, address.ip, 4) == 0)
				return bucket;
			break;

		case NA_IP6:
			if(memcmp(bucket->ipv._6, address.ip6, 16) == 0)
				return bucket;
			break;

		default:
			break;
		}
	}

	for(i = 0; i < MAX_BUCKETS; i++){
		int interval;

		bucket		= &buckets[ i ];
		interval	= now - bucket->lastTime;

		/* Reclaim expired buckets */
		if(bucket->lastTime > 0 && (interval > (burst * period) ||
					    interval < 0)){
			if(bucket->prev != NULL)
				bucket->prev->next = bucket->next;
			else
				bucketHashes[ bucket->hash ] = bucket->next;

			if(bucket->next != NULL)
				bucket->next->prev = bucket->prev;

			Q_Memset(bucket, 0, sizeof(Leakybucket));
		}

		if(bucket->type == NA_BAD){
			bucket->type = address.type;
			switch(address.type){
			case NA_IP:  Q_Memcpy(bucket->ipv._4, address.ip, 4);
				break;
			case NA_IP6: Q_Memcpy(bucket->ipv._6, address.ip6, 16);
				break;
			default: break;
			}

			bucket->lastTime	= now;
			bucket->burst		= 0;
			bucket->hash		= hash;

			/* Add to the head of the relevant hash chain */
			bucket->next = bucketHashes[ hash ];
			if(bucketHashes[ hash ] != NULL)
				bucketHashes[ hash ]->prev = bucket;

			bucket->prev = NULL;
			bucketHashes[ hash ] = bucket;

			return bucket;
		}
	}

	/* Couldn't allocate a bucket for this address */
	return NULL;
}

/*
 * SVC_RateLimit
 */
static qbool
SVC_RateLimit(Leakybucket *bucket, int burst, int period)
{
	if(bucket != NULL){
		int	now = sysmillisecs();
		int	interval	= now - bucket->lastTime;
		int	expired		= interval / period;
		int	expiredRemainder = interval % period;

		if(expired > bucket->burst){
			bucket->burst = 0;
			bucket->lastTime = now;
		}else{
			bucket->burst -= expired;
			bucket->lastTime = now - expiredRemainder;
		}

		if(bucket->burst < burst){
			bucket->burst++;

			return qfalse;
		}
	}

	return qtrue;
}

/*
 * SVC_RateLimitAddress
 *
 * Rate limit for a particular address
 */
static qbool
SVC_RateLimitAddress(Netaddr from, int burst, int period)
{
	Leakybucket *bucket = SVC_BucketForAddress(from, burst, period);

	return SVC_RateLimit(bucket, burst, period);
}

/*
 * SVC_Status
 *
 * Responds with all the info that qplug or qspy can see about the server
 * and all connected players.  Used for getting detailed information after
 * the simple info query.
 */
static void
SVC_Status(Netaddr from)
{
	char	player[1024];
	char	status[MAX_MSGLEN];
	int	i;
	Client *cl;
	Playerstate *ps;
	int	statusLength;
	int	playerLength;
	char	infostring[MAX_INFO_STRING];
	static Leakybucket bucket;

	/* ignore if we are in single player */
	if(cvargetf("g_gametype") == GT_SINGLE_PLAYER)
		return;

	/* Prevent using getstatus as an amplifier */
	if(SVC_RateLimitAddress(from, 10, 1000)){
		comdprintf(
			"SVC_Status: rate limit from %s exceeded, dropping request\n",
			addrtostr(from));
		return;
	}

	/* Allow getstatus to be DoSed relatively easily, but prevent
	 * excess outbound bandwidth usage when being flooded inbound */
	if(SVC_RateLimit(&bucket, 10, 100)){
		comdprintf(
			"SVC_Status: rate limit exceeded, dropping request\n");
		return;
	}

	strcpy(infostring, cvargetinfostr(CVAR_SERVERINFO));

	/* echo back the parameter to status. so master servers can use it as a challenge
	 * to prevent timed spoofed reply packets that add ghost servers */
	Info_SetValueForKey(infostring, "challenge", cmdargv(1));

	status[0] = 0;
	statusLength = 0;

	for(i=0; i < sv_maxclients->integer; i++){
		cl = &svs.clients[i];
		if(cl->state >= CS_CONNECTED){
			ps = SV_GameClientNum(i);
			Q_sprintf (player, sizeof(player), "%i %i \"%s\"\n",
				ps->persistant[PERS_SCORE], cl->ping, cl->name);
			playerLength = strlen(player);
			if(statusLength + playerLength >= sizeof(status))
				break;	/* can't hold any more */
			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	netprintOOB(NS_SERVER, from, "statusResponse\n%s\n%s", infostring,
		status);
}

/*
 * SVC_Info
 *
 * Responds with a short info message that should be enough to determine
 * if a user is interested in a server to do a full status
 */
void
SVC_Info(Netaddr from)
{
	int	i, count, humans;
	char	*gamedir;
	char	infostring[MAX_INFO_STRING];

	/* ignore if we are in single player */
	if(cvargetf("g_gametype") == GT_SINGLE_PLAYER ||
	   cvargetf("ui_singlePlayerActive"))
		return;

	/*
	 * Check whether cmdargv(1) has a sane length. This was not done in the original Quake3 version which led
	 * to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
	 */

	/* A maximum challenge length of 128 should be more than plenty. */
	if(strlen(cmdargv(1)) > 128)
		return;

	/* don't count privateclients */
	count = humans = 0;
	for(i = sv_privateClients->integer; i < sv_maxclients->integer; i++)
		if(svs.clients[i].state >= CS_CONNECTED){
			count++;
			if(svs.clients[i].netchan.remoteAddress.type != NA_BOT)
				humans++;
		}

	infostring[0] = 0;

	/* echo back the parameter to status. so servers can use it as a challenge
	 * to prevent timed spoofed reply packets that add ghost servers */
	Info_SetValueForKey(infostring, "challenge", cmdargv(1));

	Info_SetValueForKey(infostring, "gamename", com_gamename->string);

	Info_SetValueForKey(infostring, "protocol",
		va("%i", com_protocol->integer));

	Info_SetValueForKey(infostring, "hostname", sv_hostname->string);
	Info_SetValueForKey(infostring, "mapname", sv_mapname->string);
	Info_SetValueForKey(infostring, "clients", va("%i", count));
	Info_SetValueForKey(infostring, "g_humanplayers", va("%i", humans));
	Info_SetValueForKey(infostring, "sv_maxclients",
		va("%i", sv_maxclients->integer - sv_privateClients->integer));
	Info_SetValueForKey(infostring, "gametype",
		va("%i", sv_gametype->integer));
	Info_SetValueForKey(infostring, "pure", va("%i", sv_pure->integer));
	Info_SetValueForKey(infostring, "g_needpass",
		va("%d", cvargeti("g_needpass")));

#ifdef USE_VOIP
	if(sv_voip->integer)
		Info_SetValueForKey(infostring, "voip",
			va("%i", sv_voip->integer));

#endif

	if(sv_minPing->integer)
		Info_SetValueForKey(infostring, "minPing",
			va("%i", sv_minPing->integer));
	if(sv_maxPing->integer)
		Info_SetValueForKey(infostring, "maxPing",
			va("%i", sv_maxPing->integer));
	gamedir = cvargetstr("fs_game");
	if(*gamedir)
		Info_SetValueForKey(infostring, "game", gamedir);

	netprintOOB(NS_SERVER, from, "infoResponse\n%s", infostring);
}

/*
 * SVC_FlushRedirect
 *
 */
static void
SV_FlushRedirect(char *outputbuf)
{
	netprintOOB(NS_SERVER, svs.redirectAddress, "print\n%s",
		outputbuf);
}

/*
 * SVC_RemoteCommand
 *
 * An rcon packet arrived from the network.
 * Shift down the remaining args
 * Redirect all printfs
 */
static void
SVC_RemoteCommand(Netaddr from, Bitmsg *msg)
{
	qbool valid;
	char remaining[1024];
	/* TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
	 * (OOB messages are the bottleneck here) */
#define SV_OUTPUTBUF_LENGTH (1024 - 16)
	char	sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	char	*cmd_aux;

	/* Prevent using rcon as an amplifier and make dictionary attacks impractical */
	if(SVC_RateLimitAddress(from, 10, 1000)){
		comdprintf(
			"SVC_RemoteCommand: rate limit from %s exceeded, dropping request\n",
			addrtostr(from));
		return;
	}

	if(!strlen(sv_rconPassword->string) ||
	   strcmp (cmdargv(1), sv_rconPassword->string)){
		static Leakybucket bucket;

		/* Make DoS via rcon impractical */
		if(SVC_RateLimit(&bucket, 10, 1000)){
			comdprintf(
				"SVC_RemoteCommand: rate limit exceeded, dropping request\n");
			return;
		}

		valid = qfalse;
		comprintf ("Bad rcon from %s: %s\n", addrtostr (
				from), cmdargsfrom(2));
	}else{
		valid = qtrue;
		comprintf ("Rcon from %s: %s\n", addrtostr (
				from), cmdargsfrom(2));
	}

	/* start redirecting all print outputs to the packet */
	svs.redirectAddress = from;
	comstartredirect (sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if(!strlen(sv_rconPassword->string))
		comprintf ("No rconpassword set on the server.\n");
	else if(!valid)
		comprintf ("Bad rconpassword.\n");
	else{
		remaining[0] = 0;

		/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
		 * get the command directly, "rcon <pass> <command>" to avoid quoting issues
		 * extract the command by walking
		 * since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing */
		cmd_aux = cmdcmd();
		cmd_aux +=4;
		while(cmd_aux[0]==' ')
			cmd_aux++;
		while(cmd_aux[0] && cmd_aux[0]!=' ')	/* password */
			cmd_aux++;
		while(cmd_aux[0]==' ')
			cmd_aux++;

		Q_strcat(remaining, sizeof(remaining), cmd_aux);

		cmdexecstr (remaining);

	}

	comendredirect ();
}

/*
 * SV_ConnectionlessPacket
 *
 * A connectionless packet has four leading 0xff
 * characters to distinguish it from a game channel.
 * Clients that are in the game can still send
 * connectionless packets.
 */
static void
SV_ConnectionlessPacket(Netaddr from, Bitmsg *msg)
{
	char *s;
	char *c;

	bmstartreadingOOB(msg);
	bmreadl(msg);	/* skip the -1 marker */

	if(!Q_strncmp("connect", (char*)&msg->data[4], 7))
		huffdecompress(msg, 12);

	s = bmreadstrline(msg);
	cmdstrtok(s);

	c = cmdargv(0);
	comdprintf ("SV packet %s : %s\n", addrtostr(from), c);

	if(!Q_stricmp(c, "getstatus"))
		SVC_Status(from);
	else if(!Q_stricmp(c, "getinfo"))
		SVC_Info(from);
	else if(!Q_stricmp(c, "getchallenge"))
		SV_GetChallenge(from);
	else if(!Q_stricmp(c, "connect")){
		SV_DirectConnect(from);
#ifndef STANDALONE
	}else if(!Q_stricmp(c, "ipAuthorize")){
		SV_AuthorizeIpPacket(from);
#endif
	}else if(!Q_stricmp(c, "rcon"))
		SVC_RemoteCommand(from, msg);
	else if(!Q_stricmp(c, "disconnect")){
		/* if a client starts up a local server, we may see some spurious
		 * server disconnect messages when their new server sees our final
		 * sequenced messages to the old client */
	}else
		comdprintf ("bad connectionless packet from %s:\n%s\n",
			addrtostr (from), s);
}

/* ============================================================================ */

/*
 * svpacketevent
 */
void
svpacketevent(Netaddr from, Bitmsg *msg)
{
	int	i;
	Client *cl;
	int	qport;

	/* check for connectionless packet (0xffffffff) first */
	if(msg->cursize >= 4 && *(int*)msg->data == -1){
		SV_ConnectionlessPacket(from, msg);
		return;
	}

	/* read the qport out of the message so we can fix up
	 * stupid address translating routers */
	bmstartreadingOOB(msg);
	bmreadl(msg);	/* sequence number */
	qport = bmreads(msg) & 0xffff;

	/* find which client the message is from */
	for(i=0, cl=svs.clients; i < sv_maxclients->integer; i++,cl++){
		if(cl->state == CS_FREE)
			continue;
		if(!equalbaseaddr(from, cl->netchan.remoteAddress))
			continue;
		/* it is possible to have multiple clients from a single IP
		 * address, so they are differentiated by the qport variable */
		if(cl->netchan.qport != qport)
			continue;

		/* the IP port can't be used to differentiate them, because
		 * some address translating routers periodically change UDP
		 * port assignments */
		if(cl->netchan.remoteAddress.port != from.port){
			comprintf(
				"svpacketevent: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		/* make sure it is a valid, in sequence packet */
		if(SV_Netchan_Process(cl, msg))
			/* zombie clients still need to do the ncprocess
			 * to make sure they don't need to retransmit the final
			 * reliable message, but they don't do any other processing */
			if(cl->state != CS_ZOMBIE){
				cl->lastPacketTime = svs.time;	/* don't timeout */
				SV_ExecuteClientMessage(cl, msg);
			}
		return;
	}
}


/*
 * SV_CalcPings
 *
 * Updates the cl->ping variables
 */
static void
SV_CalcPings(void)
{
	int	i, j;
	Client *cl;
	int	total, count;
	int	delta;
	Playerstate *ps;

	for(i=0; i < sv_maxclients->integer; i++){
		cl = &svs.clients[i];
		if(cl->state != CS_ACTIVE){
			cl->ping = 999;
			continue;
		}
		if(!cl->gentity){
			cl->ping = 999;
			continue;
		}
		if(cl->gentity->r.svFlags & SVF_BOT){
			cl->ping = 0;
			continue;
		}

		total	= 0;
		count	= 0;
		for(j = 0; j < PACKET_BACKUP; j++){
			if(cl->frames[j].messageAcked <= 0)
				continue;
			delta = cl->frames[j].messageAcked -
				cl->frames[j].messageSent;
			count++;
			total += delta;
		}
		if(!count)
			cl->ping = 999;
		else{
			cl->ping = total/count;
			if(cl->ping > 999)
				cl->ping = 999;
		}

		/* let the game dll know about the ping */
		ps = SV_GameClientNum(i);
		ps->ping = cl->ping;
	}
}

/*
 * SV_CheckTimeouts
 *
 * If a packet has not been received from a client for timeout->integer
 * seconds, drop the conneciton.  Server time is used instead of
 * realtime to avoid dropping the local client while debugging.
 *
 * When a client is normally dropped, the Client goes into a zombie state
 * for a few seconds to make sure any final reliable message gets resent
 * if necessary
 */
static void
SV_CheckTimeouts(void)
{
	int	i;
	Client *cl;
	int	droppoint;
	int	zombiepoint;

	droppoint	= svs.time - 1000 * sv_timeout->integer;
	zombiepoint	= svs.time - 1000 * sv_zombietime->integer;

	for(i=0,cl=svs.clients; i < sv_maxclients->integer; i++,cl++){
		/* message times may be wrong across a changelevel */
		if(cl->lastPacketTime > svs.time)
			cl->lastPacketTime = svs.time;

		if(cl->state == CS_ZOMBIE
		   && cl->lastPacketTime < zombiepoint){
			/* using the client id cause the cl->name is empty at this point */
			comdprintf(
				"Going from CS_ZOMBIE to CS_FREE for client %d\n",
				i);
			cl->state = CS_FREE;	/* can now be reused */
			continue;
		}
		if(cl->state >= CS_CONNECTED && cl->lastPacketTime <
		   droppoint){
			/* wait several frames so a debugger session doesn't
			 * cause a timeout */
			if(++cl->timeoutCount > 5){
				SV_DropClient (cl, "timed out");
				cl->state = CS_FREE;	/* don't bother with zombie state */
			}
		}else
			cl->timeoutCount = 0;
	}
}


/*
 * SV_CheckPaused
 */
static qbool
SV_CheckPaused(void)
{
	int	count;
	Client *cl;
	int	i;

	if(!cl_paused->integer)
		return qfalse;

	/* only pause if there is just a single client connected */
	count = 0;
	for(i=0,cl=svs.clients; i < sv_maxclients->integer; i++,cl++)
		if(cl->state >= CS_CONNECTED &&
		   cl->netchan.remoteAddress.type != NA_BOT)
			count++;

	if(count > 1){
		/* don't pause */
		if(sv_paused->integer)
			cvarsetstr("sv_paused", "0");
		return qfalse;
	}

	if(!sv_paused->integer)
		cvarsetstr("sv_paused", "1");
	return qtrue;
}

/*
 * svframemsec
 * Return time in millseconds until processing of the next server frame.
 */
int
svframemsec()
{
	if(sv_fps){
		int frameMsec;

		frameMsec = 1000.0f / sv_fps->value;

		if(frameMsec < sv.timeResidual)
			return 0;
		else
			return frameMsec - sv.timeResidual;
	}else
		return 1;
}

/*
 * Player movement occurs as a result of packet events, which
 * happen before svframe is called
 */
void
svframe(int msec)
{
	int frameMsec, startTime;

	/* the menu kills the server with this cvar */
	if(sv_killserver->integer){
		svshutdown ("Server was killed");
		cvarsetstr("sv_killserver", "0");
		return;
	}

	if(!com_sv_running->integer){
		/* Running as a server, but no map loaded */
		if(com_dedicated->integer)
			syssleep(-1);	/* Block till something interesting happens */
		return;
	}

	if(SV_CheckPaused())
		return;	/* allow pause if only the local client is connected */

	/* if it isn't time for the next frame, do nothing */
	if(sv_fps->integer < 1)
		cvarsetstr("sv_fps", "10");

	frameMsec = 1000 / sv_fps->integer * com_timescale->value;
	/* don't let it scale below 1ms */
	if(frameMsec < 1){
		cvarsetstr("timescale", va("%f", sv_fps->integer / 1000.0f));
		frameMsec = 1;
	}

	sv.timeResidual += msec;

	if(!com_dedicated->integer) SV_BotFrame (sv.time + sv.timeResidual);

	/* 
	 * if time is about to hit the 32nd bit, kick all clients
	 * and clear sv.time, rather
	 * than checking for negative time wraparound everywhere.
	 * 2giga-milliseconds = 23 days, so it won't be too often 
	 */
	if(svs.time > 0x70000000){
		svshutdown("Restarting server due to time wrapping");
		cbufaddstr(va("map %s\n", cvargetstr("mapname")));
		return;
	}
	/* this can happen considerably earlier when lots of clients play and the map doesn't change */
	if(svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities){
		svshutdown(
			"Restarting server due to numSnapshotEntities wrapping");
		cbufaddstr(va("map %s\n", cvargetstr("mapname")));
		return;
	}

	if(sv.restartTime && sv.time >= sv.restartTime){
		sv.restartTime = 0;
		cbufaddstr("map_restart 0\n");
		return;
	}

	/* update infostrings if anything has been changed */
	if(cvar_modifiedFlags & CVAR_SERVERINFO){
		SV_SetConfigstring(CS_SERVERINFO,
			cvargetinfostr(CVAR_SERVERINFO));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if(cvar_modifiedFlags & CVAR_SYSTEMINFO){
		SV_SetConfigstring(CS_SYSTEMINFO,
			cvargetbiginfostr(CVAR_SYSTEMINFO));
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}

	if(com_speeds->integer)
		startTime = sysmillisecs ();
	else
		startTime = 0;	/* quite a compiler warning */

	/* update ping based on the all received frames */
	SV_CalcPings();

	if(com_dedicated->integer) SV_BotFrame (sv.time);

	/* run the game simulation in chunks */
	while(sv.timeResidual >= frameMsec){
		sv.timeResidual -= frameMsec;
		svs.time	+= frameMsec;
		sv.time		+= frameMsec;

		/* let everything in the world think and move */
		vmcall (gvm, GAME_RUN_FRAME, sv.time);
	}

	if(com_speeds->integer)
		time_game = sysmillisecs () - startTime;

	/* check timeouts */
	SV_CheckTimeouts();

	/* send messages back to the clients */
	SV_SendClientMessages();

	/* send a heartbeat to the master if needed */
	SV_MasterHeartbeat(HEARTBEAT_FOR_MASTER);
}

/*
 * SV_RateMsec
 *
 * Return the number of msec until another message can be sent to
 * a client based on its rate settings
 */

#define UDPIP_HEADER_SIZE	28
#define UDPIP6_HEADER_SIZE	48

int
SV_RateMsec(Client *client)
{
	int	rate, rateMsec;
	int	messageSize;

	messageSize = client->netchan.lastSentSize;
	rate = client->rate;

	if(sv_maxRate->integer){
		if(sv_maxRate->integer < 1000)
			cvarsetstr("sv_MaxRate", "1000");
		if(sv_maxRate->integer < rate)
			rate = sv_maxRate->integer;
	}

	if(sv_minRate->integer){
		if(sv_minRate->integer < 1000)
			cvarsetstr("sv_minRate", "1000");
		if(sv_minRate->integer > rate)
			rate = sv_minRate->integer;
	}

	if(client->netchan.remoteAddress.type == NA_IP6)
		messageSize += UDPIP6_HEADER_SIZE;
	else
		messageSize += UDPIP_HEADER_SIZE;

	rateMsec = messageSize * 1000 / ((int)(rate * com_timescale->value));
	rate = sysmillisecs() - client->netchan.lastSentTime;

	if(rate > rateMsec)
		return 0;
	else
		return rateMsec - rate;
}

/*
 * svsendqueuedpackets
 *
 * Send download messages and queued packets in the time that we're idle, i.e.
 * not computing a server frame or sending client snapshots.
 * Return the time in msec until we expect to be called next
 */

int
svsendqueuedpackets()
{
	int	numBlocks;
	int	dlStart, deltaT, delayT;
	static int dlNextRound = 0;
	int	timeVal = INT_MAX;

	/* Send out fragmented packets now that we're idle */
	delayT = SV_SendQueuedMessages();
	if(delayT >= 0)
		timeVal = delayT;

	if(sv_dlRate->integer){
		/* Rate limiting. This is very imprecise for high
		 * download rates due to millisecond timedelta resolution */
		dlStart = sysmillisecs();
		deltaT	= dlNextRound - dlStart;

		if(deltaT > 0){
			if(deltaT < timeVal)
				timeVal = deltaT + 1;
		}else{
			numBlocks = SV_SendDownloadMessages();

			if(numBlocks){
				/* There are active downloads */
				deltaT = sysmillisecs() - dlStart;

				delayT = 1000 * numBlocks *
					 MAX_DOWNLOAD_BLKSIZE;
				delayT /= sv_dlRate->integer * 1024;

				if(delayT <= deltaT + 1){
					/* Sending the last round of download messages
					 * took too long for given rate, don't wait for
					 * next round, but always enforce a 1ms delay
					 * between DL message rounds so we don't hog
					 * all of the bandwidth. This will result in an
					 * effective maximum rate of 1MB/s per user, but the
					 * low download window size limits this anyways. */
					if(timeVal > 2)
						timeVal = 2;

					dlNextRound = dlStart + deltaT + 1;
				}else{
					dlNextRound = dlStart + delayT;
					delayT -= deltaT;

					if(delayT < timeVal)
						timeVal = delayT;
				}
			}
		}
	}else if(SV_SendDownloadMessages())
		timeVal = 0;

	return timeVal;
}
