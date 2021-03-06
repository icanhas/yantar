/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* 
 * Parse messages received from the server 
 */

#include "shared.h"
#include "client.h"

int cl_connectedToPureServer;
int cl_connectedToCheatServer;

static char *svcstr[] = {
	[svc_bad]=			"svc_bad",
	[svc_nop]=			"svc_nop",
	[svc_configstring]=		"svc_configstring",
	[svc_baseline]=		"svc_baseline",
	[svc_serverCommand]=	"svc_serverCommand",
	[svc_download]=		"svc_download",
	[svc_snapshot]=		"svc_snapshot",
	[svc_EOF]=			"svc_EOF",
	[svc_voip]=			"svc_voip"
};

static void
SHOWNET(Bitmsg *msg, char *s)
{
	if(cl_shownet->integer >= 2)
		comprintf ("%3i:%s\n", msg->readcount-1, s);
}

/*
 * Message parsing
 */

/*
 * Parses deltas from the given base and adds the resulting entity
 * to the current frame
 */
static void
deltaentity(Bitmsg *msg, Clsnapshot *frame, int newnum, Entstate *old, qbool unchanged)
{
	Entstate *state;

	/* 
	 * save the parsed entity state into the big circular buffer so
	 * it can be used as the source for a later delta 
	 */
	state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];

	if(unchanged)
		*state = *old;
	else
		bmreaddeltaEntstate(msg, old, state, newnum);

	if(state->number == (MAX_GENTITIES-1))
		return;		/* entity was delta removed */
	cl.parseEntitiesNum++;
	frame->numEntities++;
}

static void
parsepacketentities(Bitmsg *msg, Clsnapshot *oldframe, Clsnapshot *newframe)
{
	int i, newnum, oldindex, oldnum;
	Entstate *oldstate;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	/* delta from the entities present in oldframe */
	oldindex = 0;
	oldstate = NULL;
	if(!oldframe)
		oldnum = 99999;
	else{
		if(oldindex >= oldframe->numEntities)
			oldnum = 99999;
		else{
			i = (oldframe->parseEntitiesNum + oldindex) 
				& (MAX_PARSE_ENTITIES-1);
			oldstate = &cl.parseEntities[i];
			oldnum = oldstate->number;
		}
	}

	for(;;){
		/* read the entity index number */
		newnum = bmreadbits(msg, GENTITYNUM_BITS);

		if(newnum == (MAX_GENTITIES-1))
			break;

		if(msg->readcount > msg->cursize)
			comerrorf (ERR_DROP,
				"parsepacketentities: end of message");

		while(oldnum < newnum){
			/* one or more entities from the old packet are unchanged */
			if(cl_shownet->integer == 3)
				comprintf("%3i:  unchanged: %i\n",
					msg->readcount,
					oldnum);
			deltaentity(msg, newframe, oldnum, oldstate, qtrue);

			oldindex++;

			if(oldindex >= oldframe->numEntities)
				oldnum = 99999;
			else{
				i = (oldframe->parseEntitiesNum + oldindex) 
					& (MAX_PARSE_ENTITIES-1);
				oldstate = &cl.parseEntities[i];
				oldnum = oldstate->number;
			}
		}
		if(oldnum == newnum){
			/* delta from previous state */
			if(cl_shownet->integer == 3)
				comprintf("%3i:  delta: %i\n", msg->readcount,
					newnum);
			deltaentity(msg, newframe, newnum, oldstate, qfalse);

			oldindex++;

			if(oldindex >= oldframe->numEntities)
				oldnum = 99999;
			else{
				i = (oldframe->parseEntitiesNum + oldindex) 
					& (MAX_PARSE_ENTITIES-1);
				oldstate = &cl.parseEntities[i];
				oldnum = oldstate->number;
			}
			continue;
		}

		if(oldnum > newnum){
			/* delta from baseline */
			if(cl_shownet->integer == 3)
				comprintf("%3i:  baseline: %i\n",
					msg->readcount,
					newnum);
			deltaentity(msg, newframe, newnum,
				&cl.entityBaselines[newnum],
				qfalse);
			continue;
		}

	}

	/* any remaining entities in the old frame are copied over */
	while(oldnum != 99999){
		/* one or more entities from the old packet are unchanged */
		if(cl_shownet->integer == 3)
			comprintf ("%3i:  unchanged: %i\n", msg->readcount,
				oldnum);
		deltaentity(msg, newframe, oldnum, oldstate, qtrue);

		oldindex++;

		if(oldindex >= oldframe->numEntities)
			oldnum = 99999;
		else{
			i = (oldframe->parseEntitiesNum + oldindex) 
				& (MAX_PARSE_ENTITIES-1);
			oldstate = &cl.parseEntities[i];
			oldnum = oldstate->number;
		}
	}
}

/*
 * If the snapshot is parsed properly, it will be copied to
 * cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
 * for any reason, no changes to the state will be made at all.
 */
static void
parsesnap(Bitmsg *msg)
{
	Clsnapshot *old, newSnap;
	int len, deltaNum, oldMessageNum, i, packetNum;

	/* 
	 * get the reliable sequence acknowledge number
	 * NOTE: now sent with all server to client messages
	 * clc.reliableAcknowledge = bmreadl(msg); 
	 */

	/* 
	 * read in the new snapshot to a temporary buffer
	 * we will only copy to cl.snap if it is valid 
	 */
	Q_Memset (&newSnap, 0, sizeof(newSnap));

	/* 
	 * we will have read any new server commands in this
	 * message before we got to svc_snapshot 
	 */
	newSnap.serverCommandNum = clc.serverCommandSequence;

	newSnap.serverTime = bmreadl(msg);

	/* 
	 * if we were just unpaused, we can only *now* really let the
	 * change come into effect or the client hangs. 
	 */
	cl_paused->modified = 0;

	newSnap.messageNum = clc.serverMessageSequence;

	deltaNum = bmreadb(msg);
	if(!deltaNum)
		newSnap.deltaNum = -1;
	else
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	newSnap.snapFlags = bmreadb(msg);

	/* If the frame is delta compressed from data that we
	 * no longer have available, we must suck up the rest of
	 * the frame, but not use it, then ask for a non-compressed
	 * message */
	if(newSnap.deltaNum <= 0){
		newSnap.valid = qtrue;	/* uncompressed frame */
		old = NULL;
		clc.demowaiting = qfalse;	/* we can start recording now */
	}else{
		old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if(!old->valid)
			/* should never happen */
			comprintf("Delta from invalid frame (not supposed to happen!).\n");
		else if(old->messageNum != newSnap.deltaNum)
			/* 
			 * The frame that the server did the delta from
			 * is too old, so we can't reconstruct it properly. 
			 */
			comprintf ("Delta frame too old.\n");
		else if(cl.parseEntitiesNum - old->parseEntitiesNum >
			MAX_PARSE_ENTITIES-128)
			comprintf ("Delta parseEntitiesNum too old.\n");
		else
			newSnap.valid = qtrue;	/* valid delta parse */
	}

	/* read areamask */
	len = bmreadb(msg);

	if(len > sizeof(newSnap.areamask)){
		comerrorf (ERR_DROP,
			"parsesnap: Invalid size %d for areamask",
			len);
		return;
	}

	bmread(msg, &newSnap.areamask, len);

	/* read playerinfo */
	SHOWNET(msg, "playerstate");
	if(old)
		bmreaddeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	else
		bmreaddeltaPlayerstate(msg, NULL, &newSnap.ps);

	/* read packet entities */
	SHOWNET(msg, "packet entities");
	parsepacketentities(msg, old, &newSnap);

	/* 
	 * if not valid, dump the entire thing now that it has
	 * been properly read 
	 */
	if(!newSnap.valid)
		return;

	/* clear the valid flags of any snapshots between the last
	 * received and this one, so if there was a dropped packet
	 * it won't look like something valid to delta from next
	 * time we wrap around in the buffer */
	oldMessageNum = cl.snap.messageNum + 1;

	if(newSnap.messageNum - oldMessageNum >= PACKET_BACKUP)
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP - 1);
	for(; oldMessageNum < newSnap.messageNum; oldMessageNum++)
		cl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;

	/* copy to the current good spot */
	cl.snap = newSnap;
	cl.snap.ping = 999;
	/* calculate ping time */
	for(i = 0; i < PACKET_BACKUP; i++){
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK;
		if(cl.snap.ps.commandTime >=
		   cl.outPackets[packetNum].p_serverTime){
			cl.snap.ping = cls.simtime -
				       cl.outPackets[packetNum].p_simtime;
			break;
		}
	}
	/* save the frame off in the backup array for later delta comparisons */
	cl.snapshots[cl.snap.messageNum & PACKET_MASK] = cl.snap;

	if(cl_shownet->integer == 3)
		comprintf("   snapshot:%i  delta:%i  ping:%i\n",
			cl.snap.messageNum,
			cl.snap.deltaNum,
			cl.snap.ping);

	cl.newSnapshots = qtrue;
}

static void
parseservinfo(void)
{
	const char *serverInfo;

	serverInfo = cl.gameState.stringData
		     + cl.gameState.stringOffsets[CS_SERVERINFO];
	clc.sv_allowDownload = atoi(Info_ValueForKey(serverInfo, "sv_allowDownload"));
	Q_strncpyz(clc.sv_dlURL,
		Info_ValueForKey(serverInfo, "sv_dlURL"),
		sizeof(clc.sv_dlURL));
}

static void
parsegamestate(Bitmsg *msg)
{
	int i, newnum, cmd;
	Entstate *es, nullstate;
	char *s, oldGame[MAX_QPATH];

	Con_Close();

	clc.connectPacketCount = 0;

	/* wipe local client state */
	CL_ClearState();

	/* a gamestate always marks a server command sequence */
	clc.serverCommandSequence = bmreadl(msg);

	/* parse all the configstrings and baselines */
	cl.gameState.dataCount = 1;	/* leave a 0 at the beginning for uninitialized configstrings */
	for(;;){
		cmd = bmreadb(msg);
		if(cmd == svc_EOF)
			break;
		if(cmd == svc_configstring){
			int len;

			i = bmreads(msg);
			if(i < 0 || i >= MAX_CONFIGSTRINGS)
				comerrorf(ERR_DROP,
					"configstring > MAX_CONFIGSTRINGS");
			s = bmreadbigstr(msg);
			len = strlen(s);

			if(len + 1 + cl.gameState.dataCount >
			   MAX_GAMESTATE_CHARS)
				comerrorf(ERR_DROP,
					"MAX_GAMESTATE_CHARS exceeded");

			/* append it to the gameState string buffer */
			cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
			Q_Memcpy(
				cl.gameState.stringData + cl.gameState.dataCount,
				s,
				len + 1);
			cl.gameState.dataCount += len + 1;
		}else if(cmd == svc_baseline){
			newnum = bmreadbits(msg, GENTITYNUM_BITS);
			if(newnum < 0 || newnum >= MAX_GENTITIES)
				comerrorf(ERR_DROP,
					"Baseline number out of range: %i",
					newnum);
			Q_Memset (&nullstate, 0, sizeof(nullstate));
			es = &cl.entityBaselines[ newnum ];
			bmreaddeltaEntstate(msg, &nullstate, es, newnum);
		}else
			comerrorf(ERR_DROP,
				"parsegamestate: bad command byte");
	}

	clc.clientNum = bmreadl(msg);
	clc.checksumFeed = bmreadl(msg);

	cvargetstrbuf("fs_game", oldGame, sizeof(oldGame));	/* save old gamedir */
	parseservinfo();	/* parse useful values out of CS_SERVERINFO */
	CL_SystemInfoChanged();	/* parse serverId and other cvars */

	/* stop recording now so the demo won't have an unnecessary level load at the end. */
	if(cl_autoRecordDemo->integer && clc.demorecording)
		CL_StopRecord_f();

	/* reinitialize the filesystem if the game directory has changed */
	if(!cls.oldGameSet && (cvarflags("fs_game") & CVAR_MODIFIED)){
		cls.oldGameSet = qtrue;
		Q_strncpyz(cls.oldGame, oldGame, sizeof(cls.oldGame));
	}

	fscondrestart(clc.checksumFeed, qfalse);

	/* 
	 * This used to call clstarthunkusers, but now we enter the download state before loading the
	 * cgame 
	 */
	clinitDownloads();

	/* make sure the game starts */
	cvarsetstr("cl_paused", "0");
}

/*
 * A download message has been received from the server
 */
static void
parsedownload(Bitmsg *msg)
{
	int size;
	unsigned char data[MAX_MSGLEN];
	uint16_t block;

	if(!*clc.downloadTempName){
		comprintf(
			"Server sending download, but no download was requested\n");
		CL_AddReliableCommand("stopdl", qfalse);
		return;
	}

	/* read the data */
	block = bmreads (msg);

	if(!block && !clc.downloadBlock){
		/* block zero is special, contains file size */
		clc.downloadSize = bmreadl(msg);

		cvarsetf("cl_downloadSize", clc.downloadSize);

		if(clc.downloadSize < 0){
			comerrorf(ERR_DROP, "%s", bmreadstr(msg));
			return;
		}
	}

	size = bmreads(msg);
	if(size < 0 || size > sizeof(data)){
		comerrorf(ERR_DROP,
			"parsedownload: Invalid size %d for download chunk",
			size);
		return;
	}

	bmread(msg, data, size);

	if((clc.downloadBlock & 0xFFFF) != block){
		comdprintf("parsedownload: Expected block %d, got %d\n",
			(clc.downloadBlock & 0xFFFF), block);
		return;
	}

	/* open the file if not opened yet */
	if(!clc.download){
		clc.download = fssvopenw(clc.downloadTempName);

		if(!clc.download){
			comprintf("Could not create %s\n", clc.downloadTempName);
			CL_AddReliableCommand("stopdl", qfalse);
			CL_NextDownload();
			return;
		}
	}

	if(size)
		fswrite(data, size, clc.download);

	CL_AddReliableCommand(va("nextdl %d", clc.downloadBlock), qfalse);
	clc.downloadBlock++;

	clc.downloadCount += size;

	/* So UI gets access to it */
	cvarsetf("cl_downloadCount", clc.downloadCount);

	if(!size){	/* A zero length block means EOF */
		if(clc.download){
			fsclose(clc.download);
			clc.download = 0;

			/* rename the file */
			fssvrename (clc.downloadTempName, clc.downloadName);
		}

		/* send intentions now
		 * We need this because without it, we would hold the last nextdl and then start
		 * loading right away.  If we take a while to load, the server is happily trying
		 * to send us that last block over and over.
		 * Write it twice to help make sure we acknowledge the download */
		CL_WritePacket();
		CL_WritePacket();

		/* get another file if needed */
		CL_NextDownload();
	}
}

#ifdef USE_VOIP
static qbool
CL_ShouldIgnoreVoipSender(int sender)
{
	if(!cl_voip->integer)
		return qtrue;	/* VoIP is disabled. */
	else if((sender == clc.clientNum) && (!clc.demoplaying))
		return qtrue;	/* ignore own voice (unless playing back a demo). */
	else if(clc.voipMuteAll)
		return qtrue;	/* all channels are muted with extreme prejudice. */
	else if(clc.voipIgnore[sender])
		return qtrue;	/* just ignoring this guy. */
	else if(clc.voipGain[sender] == 0.0f)
		return qtrue;	/* too quiet to play. */

	return qfalse;
}

/*
 * Play raw data
 */
static void
CL_PlayVoip(int sender, int samplecnt, const byte *data, int flags)
{
	if(flags & VOIP_DIRECT)
		sndrawsamps(sender + 1, samplecnt, clc.speexSampleRate, 2, 1,
			data, clc.voipGain[sender], -1);

	if(flags & VOIP_SPATIAL)
		sndrawsamps(sender + MAX_CLIENTS + 1, samplecnt,
			clc.speexSampleRate, 2, 1,
			data, 1.0f,
			sender);
}

/*
 * A VoIP message has been received from the server
 */
static void
CL_ParseVoip(Bitmsg *msg)
{
	static short	decoded[4096];	/* !!! FIXME: don't hardcode. */

	const int sender = bmreads(msg);
	const int generation = bmreadb(msg);
	const int sequence	= bmreadl(msg);
	const int frames = bmreadb(msg);
	const int packetsize = bmreads(msg);
	const int flags = bmreadbits(msg, VOIP_FLAGCNT);
	char	encoded[1024];
	int seqdiff = sequence - clc.voipIncomingSequence[sender];
	int written = 0;
	int i;

	comdprintf("VoIP: %d-byte packet from client %d\n", packetsize, sender);

	if(sender < 0)
		return;		/* short/invalid packet, bail. */
	else if(generation < 0)
		return;		/* short/invalid packet, bail. */
	else if(sequence < 0)
		return;		/* short/invalid packet, bail. */
	else if(frames < 0)
		return;		/* short/invalid packet, bail. */
	else if(packetsize < 0)
		return;		/* short/invalid packet, bail. */

	if(packetsize > sizeof(encoded)){	/* overlarge packet? */
		int bytesleft = packetsize;
		while(bytesleft){
			int br = bytesleft;
			if(br > sizeof(encoded))
				br = sizeof(encoded);
			bmread(msg, encoded, br);
			bytesleft -= br;
		}
		return;	/* overlarge packet, bail. */
	}

	if(!clc.speexInitialized){
		bmread(msg, encoded, packetsize);	/* skip payload. */
		return;					/* can't handle VoIP without libspeex! */
	}else if(sender >= MAX_CLIENTS){
		bmread(msg, encoded, packetsize);	/* skip payload. */
		return;					/* bogus sender. */
	}else if(CL_ShouldIgnoreVoipSender(sender)){
		bmread(msg, encoded, packetsize);	/* skip payload. */
		return;					/* Channel is muted, bail. */
	}

	/* !!! FIXME: make sure data is narrowband? Does decoder handle this? */

	comdprintf("VoIP: packet accepted!\n");

	/* This is a new "generation" ... a new recording started, reset the bits. */
	if(generation != clc.voipIncomingGeneration[sender]){
		comdprintf("VoIP: new generation %d!\n", generation);
		speex_bits_reset(&clc.speexDecoderBits[sender]);
		clc.voipIncomingGeneration[sender] = generation;
		seqdiff = 0;
	}else if(seqdiff < 0){	/* we're ahead of the sequence?! */
		/* This shouldn't happen unless the packet is corrupted or something. */
		comdprintf("VoIP: misordered sequence! %d < %d!\n",
			sequence, clc.voipIncomingSequence[sender]);
		/* reset the bits just in case. */
		speex_bits_reset(&clc.speexDecoderBits[sender]);
		seqdiff = 0;
	}else if(seqdiff > 100){	/* more than 2 seconds of audio dropped? */
		/* just start over. */
		comdprintf(
			"VoIP: Dropped way too many (%d) frames from client #%d\n",
			seqdiff, sender);
		speex_bits_reset(&clc.speexDecoderBits[sender]);
		seqdiff = 0;
	}

	if(seqdiff != 0){
		comdprintf("VoIP: Dropped %d frames from client #%d\n",
			seqdiff, sender);
		/* tell speex that we're missing frames... */
		for(i = 0; i < seqdiff; i++){
			assert((written + clc.speexFrameSize) * 2 <
				sizeof(decoded));
			speex_decode_int(clc.speexDecoder[sender], NULL,
				decoded + written);
			written += clc.speexFrameSize;
		}
	}

	for(i = 0; i < frames; i++){
		char encoded[256];
		const int len = bmreadb(msg);
		if(len < 0){
			comdprintf("VoIP: Short packet!\n");
			break;
		}
		bmread(msg, encoded, len);

		/* shouldn't happen, but just in case... */
		if((written + clc.speexFrameSize) * 2 > sizeof(decoded)){
			comdprintf(
				"VoIP: playback %d bytes, %d samples, %d frames\n",
				written * 2, written, i);

			CL_PlayVoip(sender, written, (const byte*)decoded,
				flags);
			written = 0;
		}

		speex_bits_read_from(&clc.speexDecoderBits[sender], encoded, len);
		speex_decode_int(clc.speexDecoder[sender],
			&clc.speexDecoderBits[sender], decoded + written);
		/* 
		 * FIXME: what is this shit? 
		 */
		#if 0
		static FILE *encio = NULL;
		if(encio == NULL) encio = fopen("voip-incoming-encoded.bin",
				"wb");
		if(encio != NULL){
			fwrite(encoded, len, 1, encio); fflush(encio);
		}
		static FILE *decio = NULL;
		if(decio == NULL) decio = fopen("voip-incoming-decoded.bin",
				"wb");
		if(decio != NULL){
			fwrite(decoded+written, clc.speexFrameSize*2, 1, decio);
			fflush(decio);
		}
		#endif

		written += clc.speexFrameSize;
	}

	comdprintf("VoIP: playback %d bytes, %d samples, %d frames\n",
		written * 2, written, i);

	if(written > 0)
		CL_PlayVoip(sender, written, (const byte*)decoded, flags);

	clc.voipIncomingSequence[sender] = sequence + frames;
}
#endif

/*
 * Command strings are just saved off until cgame asks for them
 * when it transitions a snapshot
 */
static void
parsecmdstr(Bitmsg *msg)
{
	char *s;
	int seq, i;

	seq = bmreadl(msg);
	s = bmreadstr(msg);

	/* see if we have already executed stored it off */
	if(clc.serverCommandSequence >= seq)
		return;
	clc.serverCommandSequence = seq;

	i = seq & (MAX_RELIABLE_COMMANDS-1);
	Q_strncpyz(clc.serverCommands[i], s,
		sizeof(clc.serverCommands[i]));
}

/*
 * The systeminfo configstring has been changed, so parse
 * new information out of it.  This will happen at every
 * gamestate, and possibly during gameplay.
 */
void
CL_SystemInfoChanged(void)
{
	char	*systemInfo;
	const char *s, *t;
	char	key[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	qbool gameSet;

	systemInfo = cl.gameState.stringData +
		     cl.gameState.stringOffsets[ CS_SYSTEMINFO ];
	/* NOTE TTimo:
	 * when the serverId changes, any further messages we send to the server will use this new serverId
	 * in some cases, outdated cp commands might get sent with this news serverId */
	cl.serverId = atoi(Info_ValueForKey(systemInfo, "sv_serverid"));

	/* don't set any vars when playing a demo */
	if(clc.demoplaying)
		return;

#ifdef USE_VOIP
	{
		s = Info_ValueForKey(systemInfo, "sv_voip");
		if(cvargetf("g_gametype") == GT_SINGLE_PLAYER ||
		   cvargetf("ui_singlePlayerActive"))
			clc.voipEnabled = qfalse;
		else
			clc.voipEnabled = atoi(s);
	}
#endif

	s = Info_ValueForKey(systemInfo, "sv_cheats");
	cl_connectedToCheatServer = atoi(s);
	if(!cl_connectedToCheatServer)
		cvarsetcheatstate();

	/* check pure server string */
	s = Info_ValueForKey(systemInfo, "sv_paks");
	t = Info_ValueForKey(systemInfo, "sv_pakNames");
	fspureservsetloadedpaks(s, t);

	s = Info_ValueForKey(systemInfo, "sv_referencedPaks");
	t = Info_ValueForKey(systemInfo, "sv_referencedPakNames");
	fspureservsetreferencedpaks(s, t);

	gameSet = qfalse;
	/* scan through all the variables in the systeminfo and locally set cvars to match */
	s = systemInfo;
	while(s){
		int cvar_flags;

		Info_NextPair(&s, key, value);
		if(!key[0])
			break;

		/* ehw! */
		if(!Q_stricmp(key, "fs_game")){
			if(fscheckdirtraversal(value)){
				comprintf(S_COLOR_YELLOW
					"WARNING: Server sent invalid fs_game value %s\n",
					value);
				continue;
			}

			gameSet = qtrue;
		}

		if((cvar_flags = cvarflags(key)) == CVAR_NONEXISTENT)
			cvarget(key, value, CVAR_SERVER_CREATED | CVAR_ROM);
		else{
			/* If this cvar may not be modified by a server discard the value. */
			if(!(cvar_flags &
			     (CVAR_SYSTEMINFO | CVAR_SERVER_CREATED |
			      CVAR_USER_CREATED))){
#ifndef STANDALONE
				if(Q_stricmp(key, "g_synchronousClients") 
				   && Q_stricmp(key, "pmove_fixed") 
				   && Q_stricmp(key, "pmove_msec"))
#endif
				then{
					comprintf(S_COLOR_YELLOW
						"WARNING: server is not allowed to set %s=%s\n",
						key, value);
					continue;
				}
			}

			cvarsetstrsafe(key, value);
		}
	}
	/* if game folder should not be set and it is set at the client side */
	if(!gameSet && *cvargetstr("fs_game"))
		cvarsetstr("fs_game", "");
	cl_connectedToPureServer = cvargetf("sv_pure");
}

void
CL_ParseServerMessage(Bitmsg *msg)
{
	int cmd;

	if(cl_shownet->integer == 1)
		comprintf ("%i ",msg->cursize);
	else if(cl_shownet->integer >= 2)
		comprintf ("------------------\n");

	bmbitstream(msg);

	/* get the reliable sequence acknowledge number */
	clc.reliableAcknowledge = bmreadl(msg);
	if(clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS)
		clc.reliableAcknowledge = clc.reliableSequence;

	/*
	 * parse the message
	 */
	for(;;){
		if(msg->readcount > msg->cursize){
			comerrorf(ERR_DROP,
				"CL_ParseServerMessage: read past end of server message");
			break;
		}

		cmd = bmreadb(msg);
		if(cmd == svc_EOF){
			SHOWNET(msg, "END OF MESSAGE");
			break;
		}
		if(cl_shownet->integer >= 2){
			if(cmd < 0 || cmd >= ARRAY_LEN(svcstr))
				comprintf("%3i: BAD CMD %i\n", msg->readcount-1, cmd);
			else
				SHOWNET(msg, svcstr[cmd]);
		}

		/* other commands */
		switch(cmd){
		default:
			comerrorf(ERR_DROP,
				"CL_ParseServerMessage: Illegible server message");
			break;
		case svc_nop:
			break;
		case svc_serverCommand:
			parsecmdstr(msg);
			break;
		case svc_gamestate:
			parsegamestate(msg);
			break;
		case svc_snapshot:
			parsesnap(msg);
			break;
		case svc_download:
			parsedownload(msg);
			break;
		case svc_voip:
#ifdef USE_VOIP
			CL_ParseVoip(msg);
#endif
			break;
		}
	}
}
