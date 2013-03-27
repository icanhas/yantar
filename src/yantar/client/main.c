/* client main loop */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include <limits.h>
#include "shared.h"
#include "client.h"
#include "keycodes.h"
#include "keys.h"
#include "ui.h"
#include "../sys/local.h"
#include "../sys/loadlib.h"
#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif

typedef struct Servstatus Servstatus;
struct Servstatus
{
	char		string[BIG_INFO_STRING];
	Netaddr	address;
	int		time, startTime;
	qbool		pending;
	qbool		print;
	qbool		retrieved;
};

extern void SV_BotFrame(int time);
void CL_CheckForResend(void);
void CL_ShowIP_f(void);
void CL_ServerStatus_f(void);
void CL_ServerStatusResponse(Netaddr from, Bitmsg *msg);

#ifdef USE_MUMBLE
Cvar *cl_useMumble;
Cvar *cl_mumbleScale;
#endif
#ifdef USE_VOIP
Cvar *cl_voipUseVAD;
Cvar *cl_voipVADThreshold;
Cvar *cl_voipSend;
Cvar *cl_voipSendTarget;
Cvar *cl_voipGainDuringCapture;
Cvar *cl_voipCaptureMult;
Cvar *cl_voipShowMeter;
Cvar *cl_voip;
#endif
#ifdef USE_RENDERER_DLOPEN
Cvar *cl_renderer;
#endif
Cvar *cl_nodelta;
Cvar *cl_debugMove;
Cvar *cl_noprint;
#ifdef UPDATE_SERVER_NAME
Cvar *cl_motd;
#endif
Cvar *rcon_client_password;
Cvar *rconAddress;
Cvar *cl_timeout;
Cvar *cl_maxpackets;
Cvar *cl_packetdup;
Cvar *cl_timeNudge;
Cvar *cl_showTimeDelta;
Cvar *cl_freezeDemo;
Cvar *cl_shownet;
Cvar *cl_showSend;
Cvar *cl_timedemo;
Cvar *cl_timedemoLog;
Cvar *cl_autoRecordDemo;
Cvar *cl_aviFrameRate;
Cvar *cl_aviMotionJpeg;
Cvar *cl_forceavidemo;
Cvar *cl_freelook;
Cvar *cl_sensitivity;
Cvar *cl_mouseAccel;
Cvar *cl_mouseAccelOffset;
Cvar *cl_mouseAccelStyle;
Cvar *cl_showMouseRate;
Cvar *m_pitch;
Cvar *m_yaw;
Cvar *m_forward;
Cvar *m_side;
Cvar *m_filter;
Cvar *j_pitch;
Cvar *j_yaw;
Cvar *j_forward;
Cvar *j_side;
Cvar *j_up;
Cvar *j_pitch_axis;
Cvar *j_yaw_axis;
Cvar *j_forward_axis;
Cvar *j_side_axis;
Cvar *j_up_axis;
Cvar *cl_activeAction;
Cvar *cl_motdString;
Cvar *cl_allowDownload;
Cvar *cl_conXOffset;
Cvar *cl_inGameVideo;
Cvar *cl_serverStatusResendTime;
Cvar *cl_trn;
Cvar *cl_lanForcePackets;
Cvar *cl_guidServerUniq;
Cvar *cl_consoleKeys;
Clientactive	cl;
Clientconn clc;
Clientstatic	cls;
Vm *cgvm;
Refexport re;	/* Structure containing functions exported from refresh DLL */
#ifdef USE_RENDERER_DLOPEN
static void *rendererLib = NULL;
#endif
Ping cl_pinglist[MAX_PINGREQUESTS];
Servstatus cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int serverStatusCount;
static int noGameRestart = qfalse;

#ifdef USE_MUMBLE
static
void
CL_UpdateMumble(void)
{
	Vec3	pos, forward, up;
	float	scale = cl_mumbleScale->value;
	float	tmp;

	if(!cl_useMumble->integer)
		return;

	/* !!! FIXME: not sure if this is even close to correct. */
	anglev3s(cl.snap.ps.viewangles, forward, NULL, up);

	pos[0]	= cl.snap.ps.origin[0] * scale;
	pos[1]	= cl.snap.ps.origin[2] * scale;
	pos[2]	= cl.snap.ps.origin[1] * scale;

	tmp = forward[1];
	forward[1]	= forward[2];
	forward[2]	= tmp;

	tmp = up[1];
	up[1]	= up[2];
	up[2]	= tmp;

	if(cl_useMumble->integer > 1)
		fprintf(stderr, "%f %f %f, %f %f %f, %f %f %f\n",
			pos[0], pos[1], pos[2],
			forward[0], forward[1], forward[2],
			up[0], up[1], up[2]);

	mumble_update_coordinates(pos, forward, up);
}
#endif

#ifdef USE_VOIP
static
void
CL_UpdateVoipIgnore(const char *idstr, qbool ignore)
{
	if((*idstr >= '0') && (*idstr <= '9')){
		const int id = atoi(idstr);
		if((id >= 0) && (id < MAX_CLIENTS)){
			clc.voipIgnore[id] = ignore;
			CL_AddReliableCommand(va("voip %s %d",
					ignore ? "ignore" : "unignore",
					id), qfalse);
			comprintf("VoIP: %s ignoring player #%d\n",
				ignore ? "Now" : "No longer", id);
			return;
		}
	}
	comprintf("VoIP: invalid player ID#\n");
}

static
void
CL_UpdateVoipGain(const char *idstr, float gain)
{
	if((*idstr >= '0') && (*idstr <= '9')){
		const int id = atoi(idstr);
		if(gain < 0.0f)
			gain = 0.0f;
		if((id >= 0) && (id < MAX_CLIENTS)){
			clc.voipGain[id] = gain;
			comprintf("VoIP: player #%d gain now set to %f\n", id,
				gain);
		}
	}
}

void
CL_Voip_f(void)
{
	const char	*cmd = cmdargv(1);
	const char	*reason = NULL;

	if(clc.state != CA_ACTIVE)
		reason = "Not connected to a server";
	else if(!clc.speexInitialized)
		reason = "Speex not initialized";
	else if(!clc.voipEnabled)
		reason = "Server doesn't support VoIP";
	else if(cvargetf("g_gametype") == GT_SINGLE_PLAYER ||
		cvargetf("ui_singlePlayerActive"))
		reason = "running in single-player mode";

	if(reason != NULL){
		comprintf("VoIP: command ignored: %s\n", reason);
		return;
	}

	if(strcmp(cmd, "ignore") == 0)
		CL_UpdateVoipIgnore(cmdargv(2), qtrue);
	else if(strcmp(cmd, "unignore") == 0)
		CL_UpdateVoipIgnore(cmdargv(2), qfalse);
	else if(strcmp(cmd, "gain") == 0){
		if(cmdargc() > 3)
			CL_UpdateVoipGain(cmdargv(2), atof(cmdargv(3)));
		else if(Q_isanumber(cmdargv(2))){
			int id = atoi(cmdargv(2));
			if(id >= 0 && id < MAX_CLIENTS)
				comprintf("VoIP: current gain for player #%d "
					   "is %f\n", id, clc.voipGain[id]);
			else
				comprintf("VoIP: invalid player ID#\n");
		}else
			comprintf("usage: voip gain <playerID#> [value]\n");
	}else if(strcmp(cmd, "muteall") == 0){
		comprintf("VoIP: muting incoming voice\n");
		CL_AddReliableCommand("voip muteall", qfalse);
		clc.voipMuteAll = qtrue;
	}else if(strcmp(cmd, "unmuteall") == 0){
		comprintf("VoIP: unmuting incoming voice\n");
		CL_AddReliableCommand("voip unmuteall", qfalse);
		clc.voipMuteAll = qfalse;
	}else
		comprintf("usage: voip [un]ignore <playerID#>\n"
			   "       voip [un]muteall\n"
			   "       voip gain <playerID#> [value]\n");
}

static
void
CL_VoipNewGeneration(void)
{
	/* don't have a zero generation so new clients won't match, and don't
	 *  wrap to negative so bmreadl() doesn't "fail." */
	clc.voipOutgoingGeneration++;
	if(clc.voipOutgoingGeneration <= 0)
		clc.voipOutgoingGeneration = 1;
	clc.voipPower = 0.0f;
	clc.voipOutgoingSequence = 0;
}

/*
 * sets clc.voipTargets according to cl_voipSendTarget
 * Generally we don't want who's listening to change during a transmission,
 * so this is only called when the key is first pressed
 */
void
CL_VoipParseTargets(void)
{
	const char	*target = cl_voipSendTarget->string;
	char		*end;
	int val;

	Q_Memset(clc.voipTargets, 0, sizeof(clc.voipTargets));
	clc.voipFlags &= ~VOIP_SPATIAL;

	while(target){
		while(*target == ',' || *target == ' ')
			target++;

		if(!*target)
			break;

		if(isdigit(*target)){
			val = strtol(target, &end, 10);
			target = end;
		}else{
			if(!Q_stricmpn(target, "all", 3)){
				Q_Memset(clc.voipTargets, ~0,
					sizeof(clc.voipTargets));
				return;
			}
			if(!Q_stricmpn(target, "spatial", 7)){
				clc.voipFlags |= VOIP_SPATIAL;
				target += 7;
				continue;
			}else{
				if(!Q_stricmpn(target, "attacker", 8)){
					val = vmcall(cgvm, CG_LAST_ATTACKER);
					target += 8;
				}else if(!Q_stricmpn(target, "crosshair", 9)){
					val = vmcall(cgvm, CG_CROSSHAIR_PLAYER);
					target += 9;
				}else{
					while(*target && *target != ',' &&
					      *target != ' ')
						target++;

					continue;
				}

				if(val < 0)
					continue;
			}
		}

		if(val < 0 || val >= MAX_CLIENTS){
			comprintf(S_COLOR_YELLOW "WARNING: VoIP "
					       "target %d is not a valid client "
					       "number\n", val);
			continue;
		}

		clc.voipTargets[val / 8] |= 1 << (val % 8);
	}
}

/*
 * Record more audio from the hardware if required and encode it into Speex
 * data for later transmission.
 */
static
void
CL_CaptureVoip(void)
{
	const float	audioMult = cl_voipCaptureMult->value;
	const qbool useVad = (cl_voipUseVAD->integer != 0);
	qbool		initialFrame	= qfalse;
	qbool		finalFrame	= qfalse;

#if USE_MUMBLE
	/* if we're using Mumble, don't try to handle VoIP transmission ourselves. */
	if(cl_useMumble->integer)
		return;
#endif

	if(!clc.speexInitialized)
		return;		/* just in case this gets called at a bad time. */

	if(clc.voipOutgoingDataSize > 0)
		return;		/* packet is pending transmission, don't record more yet. */

	if(cl_voipUseVAD->modified){
		cvarsetstr("cl_voipSend", (useVad) ? "1" : "0");
		cl_voipUseVAD->modified = qfalse;
	}

	if((useVad) && (!cl_voipSend->integer))
		cvarsetstr("cl_voipSend", "1");	/* lots of things reset this. */

	if(cl_voipSend->modified){
		qbool dontCapture = qfalse;
		if(clc.state != CA_ACTIVE)
			dontCapture = qtrue;	/* not connected to a server. */
		else if(!clc.voipEnabled)
			dontCapture = qtrue;	/* server doesn't support VoIP. */
		else if(clc.demoplaying)
			dontCapture = qtrue;	/* playing back a demo. */
		else if(cl_voip->integer == 0)
			dontCapture = qtrue;	/* client has VoIP support disabled. */
		else if(audioMult == 0.0f)
			dontCapture = qtrue;	/* basically silenced incoming audio. */

		cl_voipSend->modified = qfalse;

		if(dontCapture){
			cvarsetstr("cl_voipSend", "0");
			return;
		}

		if(cl_voipSend->integer)
			initialFrame = qtrue;
		else
			finalFrame = qtrue;
	}

	/* try to get more audio data from the sound card... */

	if(initialFrame){
		sndmastergain(Q_clamp(0.0f, 1.0f,
				cl_voipGainDuringCapture->value));
		sndstartcapture();
		CL_VoipNewGeneration();
		CL_VoipParseTargets();
	}

	if((cl_voipSend->integer) || (finalFrame)){	/* user wants to capture audio? */
		int samples = sndavailcapturesamps();
		const int mult = (finalFrame) ? 1 : 4;	/* 4 == 80ms of audio. */

		/* enough data buffered in audio hardware to process yet? */
		if(samples >= (clc.speexFrameSize * mult)){
			/* audio capture is always MONO16 (and that's what speex wants!).
			 *  2048 will cover 12 uncompressed frames in narrowband mode. */
			static int16_t sampbuffer[2048];
			float	voipPower	= 0.0f;
			int	speexFrames	= 0;
			int	wpos	= 0;
			int	pos	= 0;

			if(samples > (clc.speexFrameSize * 4))
				samples = (clc.speexFrameSize * 4);

			/* !!! FIXME: maybe separate recording from encoding, so voipPower
			 * !!! FIXME:  updates faster than 4Hz? */

			samples -= samples % clc.speexFrameSize;
			sndcapture(samples, (byte*)sampbuffer);	/* grab from audio card. */

			/* this will probably generate multiple speex packets each time. */
			while(samples > 0){
				int16_t *sampptr = &sampbuffer[pos];
				int i, bytes;

				/* preprocess samples to remove noise... */
				speex_preprocess_run(clc.speexPreprocessor,
					sampptr);

				/* check the "power" of this packet... */
				for(i = 0; i < clc.speexFrameSize; i++){
					const float	flsamp =
						(float)sampptr[i];
					const float	s = fabs(flsamp);
					voipPower	+= s * s;
					sampptr[i]	=
						(int16_t)((flsamp) * audioMult);
				}

				/* encode raw audio samples into Speex data... */
				speex_bits_reset(&clc.speexEncoderBits);
				speex_encode_int(clc.speexEncoder, sampptr,
					&clc.speexEncoderBits);
				bytes = speex_bits_write(&clc.speexEncoderBits,
					(char*)&clc.voipOutgoingData[wpos+1],
					sizeof(clc.voipOutgoingData) - (wpos+1));
				assert((bytes > 0) && (bytes < 256));
				clc.voipOutgoingData[wpos] = (byte)bytes;
				wpos += bytes + 1;

				/* look at the data for the next packet... */
				pos += clc.speexFrameSize;
				samples -= clc.speexFrameSize;
				speexFrames++;
			}

			clc.voipPower = (voipPower / (32768.0f * 32768.0f *
						      ((float)(clc.
							       speexFrameSize *
							       speexFrames))))
					*
					100.0f;

			if((useVad) &&
			   (clc.voipPower < cl_voipVADThreshold->value))
				CL_VoipNewGeneration();		/* no "talk" for at least 1/4 second. */
			else{
				clc.voipOutgoingDataSize	= wpos;
				clc.voipOutgoingDataFrames	= speexFrames;

				comdprintf(
					"VoIP: Send %d frames, %d bytes, %f power\n",
					speexFrames, wpos, clc.voipPower);

				#if 0
				static FILE *encio = NULL;
				if(encio == NULL) encio = fopen(
						"voip-outgoing-encoded.bin",
						"wb");
				if(encio != NULL){
					fwrite(clc.voipOutgoingData, wpos, 1,
						encio); fflush(encio);
				}
				static FILE *decio = NULL;
				if(decio == NULL) decio = fopen(
						"voip-outgoing-decoded.bin",
						"wb");
				if(decio != NULL){
					fwrite(
						sampbuffer, speexFrames *
						clc.speexFrameSize * 2, 1, decio);
					fflush(
						decio);
				}
				#endif
			}
		}
	}

	/* User requested we stop recording, and we've now processed the last of
	 *  any previously-buffered data. Pause the capture device, etc. */
	if(finalFrame){
		sndstopcapture();
		sndmastergain(1.0f);
		clc.voipPower = 0.0f;	/* force this value so it doesn't linger. */
	}
}
#endif

/*
 * Client reliable command communication
 */

/*
 * The given command will be transmitted to the server, and is gauranteed to
 * not have future Usrcmd executed before it is executed
 */
void
CL_AddReliableCommand(const char *cmd, qbool isDisconnectCmd)
{
	int unacknowledged = clc.reliableSequence - clc.reliableAcknowledge;

	/* if we would be losing an old command that hasn't been acknowledged,
	* we must drop the connection
	* also leave one slot open for the disconnect command in this case. */

	if((isDisconnectCmd && unacknowledged > MAX_RELIABLE_COMMANDS) ||
	   (!isDisconnectCmd && unacknowledged >= MAX_RELIABLE_COMMANDS)){
		if(com_errorEntered)
			return;
		else
			comerrorf(ERR_DROP, "Client command overflow");
	}

	Q_strncpyz(clc.reliableCommands[++clc.reliableSequence &
					(MAX_RELIABLE_COMMANDS - 1)],
		cmd, sizeof(*clc.reliableCommands));
}

void
CL_ChangeReliableCommand(void)
{
	int index, l;

	index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
	l = strlen(clc.reliableCommands[ index ]);
	if(l >= MAX_STRING_CHARS - 1)
		l = MAX_STRING_CHARS - 2;
	clc.reliableCommands[ index ][ l ]	= '\n';
	clc.reliableCommands[ index ][ l+1 ]	= '\0';
}

/*
 * Client-side demo recording
 */

/*
 * Dumps the current net message, prefixed by the length
 */
void
CL_WriteDemoMessage(Bitmsg *msg, int headerBytes)
{
	int len, swlen;

	/* write the packet sequence */
	len	= clc.serverMessageSequence;
	swlen	= LittleLong(len);
	fswrite (&swlen, 4, clc.demofile);
	/* skip the packet sequencing information */
	len	= msg->cursize - headerBytes;
	swlen	= LittleLong(len);
	fswrite (&swlen, 4, clc.demofile);
	fswrite (msg->data + headerBytes, len, clc.demofile);
}

/*
 * stop recording a demo
 */
void
CL_StopRecord_f(void)
{
	int len;

	if(!clc.demorecording){
		comprintf ("Not recording a demo.\n");
		return;
	}

	/* finish up */
	len = -1;
	fswrite (&len, 4, clc.demofile);
	fswrite (&len, 4, clc.demofile);
	fsclose (clc.demofile);
	clc.demofile = 0;
	clc.demorecording	= qfalse;
	clc.spDemoRecording	= qfalse;
	comprintf ("Stopped demo.\n");
}

void
CL_DemoFilename(int number, char *fileName)
{
	int a,b,c,d;

	if(number < 0 || number > 9999)
		number = 9999;

	a = number / 1000;
	number -= a*1000;
	b = number / 100;
	number -= b*100;
	c = number / 10;
	number -= c*10;
	d = number;

	Q_sprintf(fileName, MAX_OSPATH, "demo%i%i%i%i"
		, a, b, c, d);
}

/*
 * record <demoname>
 * Begins recording a demo from the current position
 */
static char demoName[MAX_QPATH];	/* compiler bug workaround */
void
CL_Record_f(void)
{
	char	name[MAX_OSPATH];
	byte	bufData[MAX_MSGLEN];
	Bitmsg	buf;
	int	i;
	int	len;
	Entstate	*ent;
	Entstate	nullstate;
	char *s;

	if(cmdargc() > 2){
		comprintf ("record <demoname>\n");
		return;
	}

	if(clc.demorecording){
		if(!clc.spDemoRecording)
			comprintf ("Already recording.\n");
		return;
	}

	if(clc.state != CA_ACTIVE){
		comprintf ("You must be in a level to record.\n");
		return;
	}

	/* sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 .. */
	if(islocaladdr(clc.serverAddress) &&
	   !cvargetf("g_synchronousClients"))
		comprintf (
			S_COLOR_YELLOW
			"WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");

	if(cmdargc() == 2){
		s = cmdargv(1);
		Q_strncpyz(demoName, s, sizeof(demoName));
		Q_sprintf(name, sizeof(name), "demos/%s.%s%d", demoName,
			DEMOEXT,
			com_protocol->integer);
	}else{
		int number;

		/* scan for a free demo name */
		for(number = 0; number <= 9999; number++){
			CL_DemoFilename(number, demoName);
			Q_sprintf(name, sizeof(name), "demos/%s.%s%d",
				demoName, DEMOEXT,
				com_protocol->integer);

			if(!fsfileexists(name))
				break;	/* file doesn't exist */
		}
	}

	/* open the demo file */

	comprintf ("recording to %s.\n", name);
	clc.demofile = fsopenw(name);
	if(!clc.demofile){
		comprintf ("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = qtrue;
	if(cvargetf("ui_recordSPDemo"))
		clc.spDemoRecording = qtrue;
	else
		clc.spDemoRecording = qfalse;

	Q_strncpyz(clc.demoName, demoName, sizeof(clc.demoName));

	/* don't start saving messages until a non-delta compressed message is received */
	clc.demowaiting = qtrue;

	/* write out the gamestate message */
	bminit (&buf, bufData, sizeof(bufData));
	bmbitstream(&buf);

	/* NOTE, MRE: all server->client messages now acknowledge */
	bmwritel(&buf, clc.reliableSequence);

	bmwriteb (&buf, svc_gamestate);
	bmwritel (&buf, clc.serverCommandSequence);

	/* configstrings */
	for(i = 0; i < MAX_CONFIGSTRINGS; i++){
		if(!cl.gameState.stringOffsets[i])
			continue;
		s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		bmwriteb (&buf, svc_configstring);
		bmwrites (&buf, i);
		bmwritebigstr (&buf, s);
	}

	/* baselines */
	Q_Memset (&nullstate, 0, sizeof(nullstate));
	for(i = 0; i < MAX_GENTITIES; i++){
		ent = &cl.entityBaselines[i];
		if(!ent->number)
			continue;
		bmwriteb (&buf, svc_baseline);
		bmwritedeltaEntstate (&buf, &nullstate, ent, qtrue);
	}

	bmwriteb(&buf, svc_EOF);

	/* finished writing the gamestate stuff */

	/* write the client num */
	bmwritel(&buf, clc.clientNum);
	/* write the checksum feed */
	bmwritel(&buf, clc.checksumFeed);

	/* finished writing the client packet */
	bmwriteb(&buf, svc_EOF);

	/* write it to the demo file */
	len = LittleLong(clc.serverMessageSequence - 1);
	fswrite (&len, 4, clc.demofile);

	len = LittleLong (buf.cursize);
	fswrite (&len, 4, clc.demofile);
	fswrite (buf.data, buf.cursize, clc.demofile);

	/* the rest of the demo file will be copied from net messages */
}

/*
 * Client-side demo playback
 */

static float
CL_DemoFrameDurationSDev(void)
{
	int	i;
	int	numFrames;
	float	mean = 0.0f;
	float	variance = 0.0f;

	if((clc.timeDemoFrames - 1) > MAX_TIMEDEMO_DURATIONS)
		numFrames = MAX_TIMEDEMO_DURATIONS;
	else
		numFrames = clc.timeDemoFrames - 1;

	for(i = 0; i < numFrames; i++)
		mean += clc.timeDemoDurations[ i ];
	mean /= numFrames;

	for(i = 0; i < numFrames; i++){
		float x = clc.timeDemoDurations[ i ];

		variance += ((x - mean) * (x - mean));
	}
	variance /= numFrames;

	return sqrt(variance);
}

void
CL_DemoCompleted(void)
{
	char buffer[ MAX_STRING_CHARS ];

	if(cl_timedemo && cl_timedemo->integer){
		int time;

		time = sysmillisecs() - clc.timeDemoStart;
		if(time > 0){
			/* Millisecond times are frame durations:
			 * minimum/average/maximum/std deviation */
			Q_sprintf(
				buffer, sizeof(buffer),
				"%i frames %3.1f seconds %3.1f fps %d.0/%.1f/%d.0/%.1f ms\n",
				clc.timeDemoFrames,
				time/1000.0,
				clc.timeDemoFrames*1000.0 / time,
				clc.timeDemoMinDuration,
				time / (float)clc.timeDemoFrames,
				clc.timeDemoMaxDuration,
				CL_DemoFrameDurationSDev( ));
			comprintf("%s", buffer);

			/* Write a log of all the frame durations */
			if(cl_timedemoLog && strlen(cl_timedemoLog->string) >
			   0){
				int	i;
				int	numFrames;
				Fhandle f;

				if((clc.timeDemoFrames - 1) >
				   MAX_TIMEDEMO_DURATIONS)
					numFrames = MAX_TIMEDEMO_DURATIONS;
				else
					numFrames = clc.timeDemoFrames - 1;

				f = fsopenw(cl_timedemoLog->string);
				if(f){
					fsprintf(f, "# %s", buffer);

					for(i = 0; i < numFrames; i++)
						fsprintf(
							f, "%d\n",
							clc.timeDemoDurations[ i
							]);

					fsclose(f);
					comprintf("%s written\n",
						cl_timedemoLog->string);
				}else
					comprintf(
						"Couldn't open %s for writing\n",
						cl_timedemoLog->string);
			}
		}
	}

	cldisconnect(qtrue);
	CL_NextDemo();
}

void
CL_ReadDemoMessage(void)
{
	int	r;
	Bitmsg	buf;
	byte	bufData[ MAX_MSGLEN ];
	int	s;

	if(!clc.demofile){
		CL_DemoCompleted ();
		return;
	}

	/* get the sequence number */
	r = fsread(&s, 4, clc.demofile);
	if(r != 4){
		CL_DemoCompleted ();
		return;
	}
	clc.serverMessageSequence = LittleLong(s);

	/* init the message */
	bminit(&buf, bufData, sizeof(bufData));

	/* get the length */
	r = fsread(&buf.cursize, 4, clc.demofile);
	if(r != 4){
		CL_DemoCompleted ();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if(buf.cursize == -1){
		CL_DemoCompleted ();
		return;
	}
	if(buf.cursize > buf.maxsize)
		comerrorf (ERR_DROP,
			"CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
	r = fsread(buf.data, buf.cursize, clc.demofile);
	if(r != buf.cursize){
		comprintf("Demo file was truncated.\n");
		CL_DemoCompleted ();
		return;
	}

	clc.lastPacketTime = cls.simtime;
	buf.readcount = 0;
	CL_ParseServerMessage(&buf);
}

static int
CL_WalkDemoExt(char *arg, char *name, int *demofile)
{
	int i = 0;
	*demofile = 0;

	{
		Q_sprintf(name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT,
			com_protocol->integer);
		fsopenr(name, demofile, qtrue);

		if(*demofile){
			comprintf("Demo file: %s\n", name);
			return com_protocol->integer;
		}
	}

	comprintf("Not found: %s\n", name);

	while(demo_protocols[i]){
		if(demo_protocols[i] == com_protocol->integer)
			continue;

		Q_sprintf (name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT,
			demo_protocols[i]);
		fsopenr(name, demofile, qtrue);
		if(*demofile){
			comprintf("Demo file: %s\n", name);

			return demo_protocols[i];
		}else
			comprintf("Not found: %s\n", name);
		i++;
	}

	return -1;
}

static void
CL_CompleteDemoName(char *args, int argNum)
{
	if(argNum == 2){
		char demoExt[ 16 ];

		Q_sprintf(demoExt, sizeof(demoExt), ".%s%d", DEMOEXT,
			com_protocol->integer);
		fieldcompletefilename("demos", demoExt, qtrue, qtrue);
	}
}

/*
 * demo <demoname>
 */
void
CL_PlayDemo_f(void)
{
	char	name[MAX_OSPATH];
	char	*arg, *ext_test;
	int	protocol, i;
	char	retry[MAX_OSPATH];

	if(cmdargc() != 2){
		comprintf ("demo <demoname>\n");
		return;
	}

	/* make sure a local server is killed
	 * 2 means don't force disconnect of local client */
	cvarsetstr("sv_killserver", "2");

	/* open the demo file */
	arg = cmdargv(1);

	cldisconnect(qtrue);

	/* check for an extension .DEMOEXT_?? (?? is protocol) */
	ext_test = strrchr(arg, '.');

	if(ext_test &&
	   !Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1)){
		protocol = atoi(ext_test + ARRAY_LEN(DEMOEXT));

		for(i = 0; demo_protocols[i]; i++)
			if(demo_protocols[i] == protocol)
				break;

		if(demo_protocols[i] || protocol == com_protocol->integer
		   ){
			Q_sprintf(name, sizeof(name), "demos/%s", arg);
			fsopenr(name, &clc.demofile, qtrue);
		}else{
			int len;

			comprintf("Protocol %d not supported for demos\n",
				protocol);
			len = ext_test - arg;

			if(len >= ARRAY_LEN(retry))
				len = ARRAY_LEN(retry) - 1;

			Q_strncpyz(retry, arg, len + 1);
			retry[len]	= '\0';
			protocol	= CL_WalkDemoExt(retry, name,
				&clc.demofile);
		}
	}else
		protocol = CL_WalkDemoExt(arg, name, &clc.demofile);

	if(!clc.demofile){
		comerrorf(ERR_DROP, "couldn't open %s", name);
		return;
	}
	Q_strncpyz(clc.demoName, cmdargv(1), sizeof(clc.demoName));

	Con_Close();

	clc.state = CA_CONNECTED;
	clc.demoplaying = qtrue;
	Q_strncpyz(clc.servername, cmdargv(1), sizeof(clc.servername));


	/* read demo messages until connected */
	while(clc.state >= CA_CONNECTED && clc.state < CA_PRIMED)
		CL_ReadDemoMessage();
	/* don't get the first snapshot this frame, to prevent the long
	 * time from the gamestate load from messing causing a time skip */
	clc.firstDemoFrameSkipped = qfalse;
}

/*
 * Closing the main menu will restart the demo loop
 */
void
CL_StartDemoLoop(void)
{
	/* start the demo loop again */
	cbufaddstr ("d1\n");
	Key_SetCatcher(0);
}

/*
 * Called when a demo or cinematic finishes
 * If the "nextdemo" cvar is set, that command will be issued
 */
void
CL_NextDemo(void)
{
	char v[MAX_STRING_CHARS];

	Q_strncpyz(v, cvargetstr ("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS-1] = 0;
	comdprintf("CL_NextDemo: %s\n", v);
	if(!v[0])
		return;

	cvarsetstr ("nextdemo","");
	cbufaddstr (v);
	cbufaddstr ("\n");
	cbufflush();
}

void
clshutdownall(qbool shutdownRef)
{
	if(CL_VideoRecording())
		CL_CloseAVI();

	if(clc.demorecording)
		CL_StopRecord_f();

#ifdef USE_CURL
	CL_cURL_Shutdown();
#endif
	/* clear sounds */
	snddisablesounds();
	/* shutdown CGame */
	clshutdownCGame();
	/* shutdown UI */
	clshutdownUI();

	/* shutdown the renderer */
	if(shutdownRef)
		clshutdownRef();
	else if(re.Shutdown)
		re.Shutdown(qfalse);	/* don't destroy window or context */

	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.rendererStarted	= qfalse;
	cls.soundRegistered	= qfalse;
}

/*
 * Called by comgamerestart
 */
void
CL_ClearMemory(qbool shutdownRef)
{
	/* shutdown all the client stuff */
	clshutdownall(shutdownRef);

	/* if not running a server clear the whole hunk */
	if(!com_sv_running->integer){
		clshutdownCGame();
		clshutdownUI();
		CIN_CloseAllVideos();
		/* clear the whole hunk */
		hunkclear();
		/* clear collision map data */
		CM_ClearMap();
	}else
		/* clear all the client data on the hunk */
		hunkcleartomark();
}

/*
 * Called by clmaploading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
 * ways a client gets into a game
 * Also called by comerrorf
 */
void
clflushmem(void)
{
	CL_ClearMemory(qfalse);
	clstarthunkusers(qfalse);
}

/*
 * A local server is starting to load a map, so update the
 * screen to let the user know about it, then dump all client
 * memory on the hunk from cgame, ui, and renderer
 */
void
clmaploading(void)
{
	if(com_dedicated->integer){
		clc.state = CA_DISCONNECTED;
		Key_SetCatcher(KEYCATCH_CONSOLE);
		return;
	}

	if(!com_cl_running->integer)
		return;

	Con_Close();
	Key_SetCatcher(0);

	/* if we are already connected to the local host, stay connected */
	if(clc.state >= CA_CONNECTED &&
	   !Q_stricmp(clc.servername, "localhost")){
		clc.state = CA_CONNECTED;	/* so the connect screen is drawn */
		Q_Memset(cls.updateInfoString, 0, sizeof(cls.updateInfoString));
		Q_Memset(clc.serverMessage, 0, sizeof(clc.serverMessage));
		Q_Memset(&cl.gameState, 0, sizeof(cl.gameState));
		clc.lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	}else{
		/* clear nextmap so the cinematic shutdown doesn't execute it */
		cvarsetstr("nextmap", "");
		cldisconnect(qtrue);
		Q_strncpyz(clc.servername, "localhost", sizeof(clc.servername));
		clc.state = CA_CHALLENGING;	/* so the connect screen is drawn */
		Key_SetCatcher(0);
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		strtoaddr(clc.servername, &clc.serverAddress, NA_UNSPEC);
		/* we don't need a challenge on the localhost */

		CL_CheckForResend();
	}
}

/*
 * Called before parsing a gamestate
 */
void
CL_ClearState(void)
{

/*	sndstopall(); */

	Q_Memset(&cl, 0, sizeof(cl));
}

/*
 * update cl_guid using QKEY_FILE and optional prefix
 */
static void
CL_UpdateGUID(const char *prefix, int prefix_len)
{
	Fhandle f;
	int len;

	len = fssvopenr(QKEY_FILE, &f);
	fsclose(f);

	if(len != QKEY_SIZE)
		cvarsetstr("cl_guid", "");
	else
		cvarsetstr("cl_guid", md5file(QKEY_FILE, QKEY_SIZE,
				prefix, prefix_len));
}

static void
CL_OldGame(void)
{
	if(cls.oldGameSet){
		/* change back to previous fs_game */
		cls.oldGameSet = qfalse;
		cvarsetstr2("fs_game", cls.oldGame, qtrue);
		fscondrestart(clc.checksumFeed, qfalse);
	}
}

/*
 * Called when a connection, demo, or cinematic is being terminated.
 * Goes from a connected state to either a menu state or a console state
 * Sends a disconnect message to the server
 * This is also called on comerrorf and Q_Quit, so it shouldn't cause any errors
 */
void
cldisconnect(qbool showMainMenu)
{
	if(!com_cl_running || !com_cl_running->integer)
		return;

	/* shutting down the client so enter full screen ui mode */
	cvarsetstr("r_uiFullScreen", "1");

	if(clc.demorecording)
		CL_StopRecord_f ();

	if(clc.download){
		fsclose(clc.download);
		clc.download = 0;
	}
	*clc.downloadTempName = *clc.downloadName = 0;
	cvarsetstr("cl_downloadName", "");

#ifdef USE_MUMBLE
	if(cl_useMumble->integer && mumble_islinked()){
		comprintf("Mumble: Unlinking from Mumble application\n");
		mumble_unlink();
	}
#endif

#ifdef USE_VOIP
	if(cl_voipSend->integer){
		int tmp = cl_voipUseVAD->integer;
		cl_voipUseVAD->integer		= 0;	/* disable this for a moment. */
		clc.voipOutgoingDataSize	= 0;	/* dump any pending VoIP transmission. */
		cvarsetstr("cl_voipSend", "0");
		CL_CaptureVoip();	/* clean up any state... */
		cl_voipUseVAD->integer = tmp;
	}

	if(clc.speexInitialized){
		int i;
		speex_bits_destroy(&clc.speexEncoderBits);
		speex_encoder_destroy(clc.speexEncoder);
		speex_preprocess_state_destroy(clc.speexPreprocessor);
		for(i = 0; i < MAX_CLIENTS; i++){
			speex_bits_destroy(&clc.speexDecoderBits[i]);
			speex_decoder_destroy(clc.speexDecoder[i]);
		}
	}
	cmdremove ("voip");
#endif

	if(clc.demofile){
		fsclose(clc.demofile);
		clc.demofile = 0;
	}

	if(uivm && showMainMenu)
		vmcall(uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE);

	SCR_StopCinematic ();
	sndclearbuf();

	/* send a disconnect message to the server
	 * send it a few times in case one is dropped */
	if(clc.state >= CA_CONNECTED){
		CL_AddReliableCommand("disconnect", qtrue);
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}

	/* Remove pure paks */
	fspureservsetloadedpaks("", "");

	CL_ClearState ();

	/* wipe the client connection */
	Q_Memset(&clc, 0, sizeof(clc));

	clc.state = CA_DISCONNECTED;

	/* allow cheats locally */
	cvarsetstr("sv_cheats", "1");

	/* not connected to a pure server anymore */
	cl_connectedToPureServer = qfalse;

#ifdef USE_VOIP
	/* not connected to voip server anymore. */
	clc.voipEnabled = qfalse;
#endif

	/* Stop recording any video */
	if(CL_VideoRecording( )){
		/* Finish rendering current frame */
		SCR_UpdateScreen( );
		CL_CloseAVI( );
	}

	CL_UpdateGUID(NULL, 0);

	if(!noGameRestart)
		CL_OldGame();
	else
		noGameRestart = qfalse;
}

/*
 * adds the current command line as a clientCommand
 * things like godmode, noclip, etc, are commands directed to the server,
 * so when they are typed in at the console, they will need to be forwarded.
 */
void
clforwardcmdtoserver(const char *string)
{
	char *cmd;

	cmd = cmdargv(0);

	/* ignore key up commands */
	if(cmd[0] == '-')
		return;

	if(clc.demoplaying || clc.state < CA_CONNECTED || cmd[0] == '+'){
		comprintf ("Unknown command \"%s" S_COLOR_WHITE "\"\n", cmd);
		return;
	}

	if(cmdargc() > 1)
		CL_AddReliableCommand(string, qfalse);
	else
		CL_AddReliableCommand(cmd, qfalse);
}

void
CL_RequestMotd(void)
{
#ifdef UPDATE_SERVER_NAME
	char info[MAX_INFO_STRING];

	if(!cl_motd->integer)
		return;
	comprintf("Resolving %s\n", UPDATE_SERVER_NAME);
	if(!strtoaddr(UPDATE_SERVER_NAME, &cls.updateServer, NA_IP)){
		comprintf("Couldn't resolve address\n");
		return;
	}
	cls.updateServer.port = BigShort(PORT_UPDATE);
	comprintf("%s resolved to %i.%i.%i.%i:%i\n", UPDATE_SERVER_NAME,
		cls.updateServer.ip[0], cls.updateServer.ip[1],
		cls.updateServer.ip[2], cls.updateServer.ip[3],
		BigShort(cls.updateServer.port));

	info[0] = 0;

	Q_sprintf(cls.updateChallenge, sizeof(cls.updateChallenge), "%i",
		((rand() << 16) ^ rand()) ^ commillisecs());

	Info_SetValueForKey(info, "challenge", cls.updateChallenge);
	Info_SetValueForKey(info, "renderer", cls.glconfig.renderer_string);
	Info_SetValueForKey(info, "version", com_version->string);

	netprintOOB(NS_CLIENT, cls.updateServer, "getmotd \"%s\"\n", info);
#endif
}

/*
 * N.B.: CD keys are long gone, but this function is kept as a curiosity
 *
 * Authorization server protocol
 * -----------------------------
 *
 * All commands are text in Q3 out of band packets (leading 0xff 0xff 0xff 0xff).
 *
 * Whenever the client tries to get a challenge from the server it wants to
 * connect to, it also blindly fires off a packet to the authorize server:
 *
 * getKeyAuthorize <challenge> <cdkey>
 *
 * cdkey may be "demo"
 *
 *
 * #OLD The authorize server returns a:
 * #OLD
 * #OLD keyAthorize <challenge> <accept | deny>
 * #OLD
 * #OLD A client will be accepted if the cdkey is valid and it has not been used by any other IP
 * #OLD address in the last 15 minutes.
 *
 *
 * The server sends a:
 *
 * getIpAuthorize <challenge> <ip>
 *
 * The authorize server returns a:
 *
 * ipAuthorize <challenge> <accept | deny | demo | unknown >
 *
 * A client will be accepted if a valid cdkey was sent by that ip (only) in the last 15 minutes.
 * If no response is received from the authorize server after two tries, the client will be let
 * in anyway.
 */
#ifndef STANDALONE
void
CL_RequestAuthorization(void)
{
	char	nums[64];
	int	i, j, l;
	Cvar *fs;

	if(!cls.authorizeServer.port){
		comprintf("Resolving %s\n", AUTHORIZE_SERVER_NAME);
		if(!strtoaddr(AUTHORIZE_SERVER_NAME, &cls.authorizeServer,
			   NA_IP)){
			comprintf("Couldn't resolve address\n");
			return;
		}

		cls.authorizeServer.port = BigShort(PORT_AUTHORIZE);
		comprintf("%s resolved to %i.%i.%i.%i:%i\n",
			AUTHORIZE_SERVER_NAME,
			cls.authorizeServer.ip[0], cls.authorizeServer.ip[1],
			cls.authorizeServer.ip[2], cls.authorizeServer.ip[3],
			BigShort(
				cls.authorizeServer.port));
	}
	if(cls.authorizeServer.type == NA_BAD)
		return;
		
#if 0
	/* only grab the alphanumeric values from the cdkey, to avoid any dashes or spaces */
	j	= 0;
	l	= strlen(cl_cdkey);
	if(l > 32)
		l = 32;
	for(i = 0; i < l; i++)
		if((cl_cdkey[i] >= '0' && cl_cdkey[i] <= '9')
		   || (cl_cdkey[i] >= 'a' && cl_cdkey[i] <= 'z')
		   || (cl_cdkey[i] >= 'A' && cl_cdkey[i] <= 'Z')
		   ){
			nums[j] = cl_cdkey[i];
			j++;
		}
	nums[j] = 0;
#endif

	fs = cvarget ("cl_anonymous", "0", CVAR_INIT|CVAR_SYSTEMINFO);

	netprintOOB(NS_CLIENT, cls.authorizeServer,
		"getKeyAuthorize %i %s", fs->integer,
		nums);
}
#endif

/*
 * Console commands
 */

void
CL_ForwardToServer_f(void)
{
	if(clc.state != CA_ACTIVE || clc.demoplaying){
		comprintf ("Not connected to a server.\n");
		return;
	}

	/* don't forward the first argument */
	if(cmdargc() > 1)
		CL_AddReliableCommand(cmdargs(), qfalse);
}

void
cldisconnect_f(void)
{
	SCR_StopCinematic();
	cvarsetstr("ui_singlePlayerActive", "0");
	if(clc.state != CA_DISCONNECTED && clc.state != CA_CINEMATIC)
		comerrorf (ERR_DISCONNECT, "Disconnected from server");
}

void
CL_Reconnect_f(void)
{
	if(!strlen(clc.servername) || !strcmp(clc.servername, "localhost")){
		comprintf("Can't reconnect to localhost.\n");
		return;
	}
	cvarsetstr("ui_singlePlayerActive", "0");
	cbufaddstr(va("connect %s\n", clc.servername));
}

void
CL_Connect_f(void)
{
	char	*server;
	const char *serverString;
	int	argc = cmdargc();
	Netaddrtype family = NA_UNSPEC;

	if(argc != 2 && argc != 3){
		comprintf("usage: connect [-4|-6] server\n");
		return;
	}

	if(argc == 2)
		server = cmdargv(1);
	else{
		if(!strcmp(cmdargv(1), "-4"))
			family = NA_IP;
		else if(!strcmp(cmdargv(1), "-6"))
			family = NA_IP6;
		else
			comprintf(
				"warning: only -4 or -6 as address type understood.\n");

		server = cmdargv(2);
	}

	cvarsetstr("ui_singlePlayerActive", "0");

	/* fire a message off to the motd server */
	CL_RequestMotd();

	/* clear any previous "server full" type messages */
	clc.serverMessage[0] = 0;

	if(com_sv_running->integer && !strcmp(server, "localhost"))
		/* if running a local server, kill it */
		svshutdown("Server quit");

	/* make sure a local server is killed */
	cvarsetstr("sv_killserver", "1");
	svframe(0);

	noGameRestart = qtrue;
	cldisconnect(qtrue);
	Con_Close();

	Q_strncpyz(clc.servername, server, sizeof(clc.servername));

	if(!strtoaddr(clc.servername, &clc.serverAddress, family)){
		comprintf ("Bad server address\n");
		clc.state = CA_DISCONNECTED;
		return;
	}
	if(clc.serverAddress.port == 0)
		clc.serverAddress.port = BigShort(PORT_SERVER);

	serverString = addrporttostr(clc.serverAddress);

	comprintf("%s resolved to %s\n", clc.servername, serverString);

	if(cl_guidServerUniq->integer)
		CL_UpdateGUID(serverString, strlen(serverString));
	else
		CL_UpdateGUID(NULL, 0);

	/* if we aren't playing on a lan, we need to authenticate
	 * with the cd key */
	if(islocaladdr(clc.serverAddress))
		clc.state = CA_CHALLENGING;
	else{
		clc.state = CA_CONNECTING;

		/* Set a client challenge number that ideally is mirrored back by the server. */
		clc.challenge = ((rand() << 16) ^ rand()) ^ commillisecs();
	}

	Key_SetCatcher(0);
	clc.connectTime = -99999;	/* CL_CheckForResend() will fire immediately */
	clc.connectPacketCount = 0;

	/* server connection string */
	cvarsetstr("cl_currentServerAddress", server);
}

#define MAX_RCON_MESSAGE 1024

static void
CL_CompleteRcon(char *args, int argNum)
{
	if(argNum == 2){
		/* Skip "rcon " */
		char *p = Q_skiptoks(args, 1, " ");

		if(p > args)
			fieldcompletecmd(p, qtrue, qtrue);
	}
}

/*
 * Send the rest of the command line over as
 * an unconnected command.
 */
void
CL_Rcon_f(void)
{
	char message[MAX_RCON_MESSAGE];
	Netaddr to;

	if(!rcon_client_password->string){
		comprintf ("You must set 'rconpassword' before\n"
			    "issuing an rcon command.\n");
		return;
	}

	message[0]	= -1;
	message[1]	= -1;
	message[2]	= -1;
	message[3]	= -1;
	message[4]	= 0;

	Q_strcat (message, MAX_RCON_MESSAGE, "rcon ");

	Q_strcat (message, MAX_RCON_MESSAGE, rcon_client_password->string);
	Q_strcat (message, MAX_RCON_MESSAGE, " ");

	/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543 */
	Q_strcat (message, MAX_RCON_MESSAGE, cmdcmd()+5);

	if(clc.state >= CA_CONNECTED)
		to = clc.netchan.remoteAddress;
	else{
		if(!strlen(rconAddress->string)){
			comprintf ("You must either be connected,\n"
				    "or set the 'rconAddress' cvar\n"
				    "to issue rcon commands\n");

			return;
		}
		strtoaddr (rconAddress->string, &to, NA_UNSPEC);
		if(to.port == 0)
			to.port = BigShort (PORT_SERVER);
	}

	netsendpacket (NS_CLIENT, strlen(message)+1, message, to);
}

void
CL_SendPureChecksums(void)
{
	char cMsg[MAX_INFO_VALUE];

	/* if we are pure we need to send back a command with our referenced pk3 checksums */
	Q_sprintf(cMsg, sizeof(cMsg), "cp %d %s", cl.serverId,
		fsreferencedpakpurechecksums());

	CL_AddReliableCommand(cMsg, qfalse);
}

void
CL_ResetPureClientAtServer(void)
{
	CL_AddReliableCommand("vdr", qfalse);
}

/*
 * Restart the video subsystem
 *
 * we also have to reload the UI and CGame because the renderer
 * doesn't know what graphics to reload
 */
void
CL_Vid_Restart_f(void)
{

	/* Settings may have changed so stop recording now */
	if(CL_VideoRecording( ))
		CL_CloseAVI( );

	if(clc.demorecording)
		CL_StopRecord_f();

	/* don't let them loop during the restart */
	sndstopall();

	if(!fscondrestart(clc.checksumFeed, qtrue)){
		/* if not running a server clear the whole hunk */
		if(com_sv_running->integer)
			/* clear all the client data on the hunk */
			hunkcleartomark();
		else{
			clshutdownCGame();
			clshutdownUI();
			CIN_CloseAllVideos();
			/* clear the whole hunk */
			hunkclear();
		}

		clshutdownUI();
		clshutdownCGame();
		clshutdownRef();
		CL_ResetPureClientAtServer();	/* client is no longer pure untill new checksums are sent */
		fsclearpakrefs(FS_UI_REF | FS_CGAME_REF);
		/* reinitialize the filesystem if the game directory or checksum has changed */

		cls.rendererStarted = qfalse;
		cls.uiStarted = qfalse;
		cls.cgameStarted = qfalse;
		cls.soundRegistered = qfalse;

		/* unpause so the cgame definately gets a snapshot and renders a frame */
		cvarsetstr("cl_paused", "0");

		/* initialize the renderer interface */
		clinitref();

		/* startup all the client stuff */
		clstarthunkusers(qfalse);

		/* start the cgame if connected */
		if(clc.state > CA_CONNECTED && clc.state != CA_CINEMATIC){
			cls.cgameStarted = qtrue;
			clinitCGame();
			/* send pure checksums */
			CL_SendPureChecksums();
		}
	}
}

/*
 * Restart the sound subsystem
 */
void
clsndshutdown(void)
{
	sndshutdown();
	cls.soundStarted = qfalse;
}

/*
 * Restart the sound subsystem
 * The cgame and game must also be forced to restart because
 * handles will be invalid
 */
void
CL_Snd_Restart_f(void)
{
	clsndshutdown();
	/* sound will be reinitialized by vid_restart */
	CL_Vid_Restart_f();
}

void
CL_OpenedPK3List_f(void)
{
	comprintf("Opened PK3 Names: %s\n", fsloadedpaknames());
}

void
CL_ReferencedPK3List_f(void)
{
	comprintf("Referenced PK3 Names: %s\n", fsreferencedpaknames());
}

void
CL_Configstrings_f(void)
{
	int	i;
	int	ofs;

	if(clc.state != CA_ACTIVE){
		comprintf("Not connected to a server.\n");
		return;
	}

	for(i = 0; i < MAX_CONFIGSTRINGS; i++){
		ofs = cl.gameState.stringOffsets[ i ];
		if(!ofs)
			continue;
		comprintf("%4i: %s\n", i, cl.gameState.stringData + ofs);
	}
}

void
CL_Clientinfo_f(void)
{
	comprintf("--------- Client Information ---------\n");
	comprintf("state: %i\n", clc.state);
	comprintf("Server: %s\n", clc.servername);
	comprintf ("User info settings:\n");
	infoprint(cvargetinfostr(CVAR_USERINFO));
	comprintf("--------------------------------------\n");
}

/*
 * Called when all downloading has been completed
 */
void
CL_DownloadsComplete(void)
{

#ifdef USE_CURL
	/* if we downloaded with cURL */
	if(clc.cURLUsed){
		clc.cURLUsed = qfalse;
		CL_cURL_Shutdown();
		if(clc.cURLDisconnected){
			if(clc.downloadRestart){
				fsrestart(clc.checksumFeed);
				clc.downloadRestart = qfalse;
			}
			clc.cURLDisconnected = qfalse;
			CL_Reconnect_f();
			return;
		}
	}
#endif

	/* if we downloaded files we need to restart the file system */
	if(clc.downloadRestart){
		clc.downloadRestart = qfalse;

		fsrestart(clc.checksumFeed);	/* We possibly downloaded a pak, restart the file system to load it */

		/* inform the server so we get new gamestate info */
		CL_AddReliableCommand("donedl", qfalse);

		/* by sending the donedl command we request a new gamestate
		 * so we don't want to load stuff yet */
		return;
	}

	/* let the client game init and load data */
	clc.state = CA_LOADING;

	/* Pump the loop, this may change gamestate! */
	comflushevents();

	/* if the gamestate was changed by calling comflushevents
	 * then we loaded everything already and we don't want to do it again. */
	if(clc.state != CA_LOADING)
		return;

	/* starting to load a map so we get out of full screen ui mode */
	cvarsetstr("r_uiFullScreen", "0");

	/* flush client memory and start loading stuff
	 * this will also (re)load the UI
	 * if this is a local client then only the client part of the hunk
	 * will be cleared, note that this is done after the hunk mark has been set */
	clflushmem();

	/* initialize the CGame */
	cls.cgameStarted = qtrue;
	clinitCGame();

	/* set pure checksums */
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
 * Requests a file to download from the server.  Stores it in the current
 * game directory.
 */
void
CL_BeginDownload(const char *localName, const char *remoteName)
{

	comdprintf("***** CL_BeginDownload *****\n"
		    "Localname: %s\n"
		    "Remotename: %s\n"
		    "****************************\n", localName, remoteName);

	Q_strncpyz (clc.downloadName, localName, sizeof(clc.downloadName));
	Q_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp",
		localName);

	/* Set so UI gets access to it */
	cvarsetstr("cl_downloadName", remoteName);
	cvarsetstr("cl_downloadSize", "0");
	cvarsetstr("cl_downloadCount", "0");
	cvarsetf("cl_downloadTime", cls.simtime);

	clc.downloadBlock	= 0;	/* Starting new file */
	clc.downloadCount	= 0;

	CL_AddReliableCommand(va("download %s", remoteName), qfalse);
}

/*
 * A download completed or failed
 */
void
CL_NextDownload(void)
{
	char	*s;
	char	*remoteName, *localName;
	qbool useCURL = qfalse;

	/* A download has finished, check whether this matches a referenced checksum */
	if(*clc.downloadName){
		char *zippath = fsbuildospath(cvargetstr(
				"fs_homepath"), clc.downloadName, "");
		zippath[strlen(zippath)-1] = '\0';

		if(!fscompzipchecksum(zippath))
			comerrorf(ERR_DROP, "Incorrect checksum for file: %s",
				clc.downloadName);
	}

	*clc.downloadTempName = *clc.downloadName = 0;
	cvarsetstr("cl_downloadName", "");

	/* We are looking to start a download here */
	if(*clc.downloadList){
		s = clc.downloadList;

		/* format is:
		 *  @remotename@localname@remotename@localname, etc. */

		if(*s == '@')
			s++;
		remoteName = s;

		if((s = strchr(s, '@')) == NULL){
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;
		if((s = strchr(s, '@')) != NULL)
			*s++ = 0;
		else
			s = localName + strlen(localName);	/* point at the nul byte */
#ifdef USE_CURL
		if(!(cl_allowDownload->integer & DLF_NO_REDIRECT)){
			if(clc.sv_allowDownload & DLF_NO_REDIRECT)
				comprintf("WARNING: server does not "
					   "allow download redirection "
					   "(sv_allowDownload is %d)\n",
					clc.sv_allowDownload);
			else if(!*clc.sv_dlURL)
				comprintf("WARNING: server allows "
					   "download redirection, but does not "
					   "have sv_dlURL set\n");
			else if(!CL_cURL_Init())
				comprintf("WARNING: could not load "
					   "cURL library\n");
			else{
				CL_cURL_BeginDownload(localName, va("%s/%s",
						clc.sv_dlURL, remoteName));
				useCURL = qtrue;
			}
		}else if(!(clc.sv_allowDownload & DLF_NO_REDIRECT))
			comprintf("WARNING: server allows download "
				   "redirection, but it disabled by client "
				   "configuration (cl_allowDownload is %d)\n",
				cl_allowDownload->integer);

#endif	/* USE_CURL */
		if(!useCURL){
			if((cl_allowDownload->integer & DLF_NO_UDP)){
				comerrorf(ERR_DROP, "UDP Downloads are "
						    "disabled on your client. "
						    "(cl_allowDownload is %d)",
					cl_allowDownload->integer);
				return;
			}else
				CL_BeginDownload(localName, remoteName);
		}
		clc.downloadRestart = qtrue;

		/* move over the rest */
		memmove(clc.downloadList, s, strlen(s) + 1);

		return;
	}

	CL_DownloadsComplete();
}

/*
 * After receiving a valid game state, we valid the cgame and local zip files here
 * and determine if we need to download them
 */
void
clinitDownloads(void)
{
	char missingfiles[1024];

	if(!(cl_allowDownload->integer & DLF_ENABLE)){
		/* autodownload is disabled on the client
		 * but it's possible that some referenced files on the server are missing */
		if(fscomparepaks(missingfiles, sizeof(missingfiles), qfalse))
			/* NOTE TTimo I would rather have that printed as a modal message box
			 *   but at this point while joining the game we don't know wether we will successfully join or not */
			comprintf(
				"\nWARNING: You are missing some files referenced by the server:\n%s"
				"You might not be able to join the game\n"
				"Go to the setting menu to turn on autodownload, or get the file elsewhere\n\n",
				missingfiles);
	}else if(fscomparepaks(clc.downloadList, sizeof(clc.downloadList),
			 qtrue)){

		comprintf("Need paks: %s\n", clc.downloadList);

		if(*clc.downloadList){
			/* if autodownloading is not enabled on the server */
			clc.state = CA_CONNECTED;

			*clc.downloadTempName = *clc.downloadName = 0;
			cvarsetstr("cl_downloadName", "");

			CL_NextDownload();
			return;
		}

	}

	CL_DownloadsComplete();
}

/*
 * Resend a connect message if the last one has timed out
 */
void
CL_CheckForResend(void)
{
	int	port, i;
	char	info[MAX_INFO_STRING];
	char	data[MAX_INFO_STRING];

	/* don't send anything if playing back a demo */
	if(clc.demoplaying)
		return;

	/* resend if we haven't gotten a reply yet */
	if(clc.state != CA_CONNECTING && clc.state != CA_CHALLENGING)
		return;

	if(cls.simtime - clc.connectTime < RETRANSMIT_TIMEOUT)
		return;

	clc.connectTime = cls.simtime;	/* for retransmit requests */
	clc.connectPacketCount++;


	switch(clc.state){
	case CA_CONNECTING:
		/* requesting a challenge .. IPv6 users always get in as authorize server supports no ipv6. */
#ifndef STANDALONE
		if(!com_standalone->integer && clc.serverAddress.type ==
		   NA_IP && !sysisLANaddr(clc.serverAddress))
			CL_RequestAuthorization();
#endif

		/* The challenge request shall be followed by a client challenge so no malicious server can hijack this connection.
		 * Add the gamename so the server knows we're running the correct game or can reject the client
		 * with a meaningful message */
		Q_sprintf(data, sizeof(data), "getchallenge %d %s",
			clc.challenge,
			com_gamename->string);

		netprintOOB(NS_CLIENT, clc.serverAddress, "%s", data);
		break;

	case CA_CHALLENGING:
		/* sending back the challenge */
		port = cvargetf ("net_qport");

		Q_strncpyz(info, cvargetinfostr(CVAR_USERINFO), sizeof(info));

		Info_SetValueForKey(info, "protocol",
			va("%i", com_protocol->integer));
		Info_SetValueForKey(info, "qport", va("%i", port));
		Info_SetValueForKey(info, "challenge", va("%i", clc.challenge));

		strcpy(data, "connect ");
		/* TTimo adding " " around the userinfo string to avoid truncated userinfo on the server
		 *   (Q_TokenizeString tokenizes around spaces) */
		data[8] = '"';

		for(i=0; i<strlen(info); i++)
			data[9+i] = info[i];	/* + (clc.challenge)&0x3; */
		data[9+i]	= '"';
		data[10+i]	= 0;

		/* NOTE TTimo don't forget to set the right data length! */
		netdataOOB(NS_CLIENT, clc.serverAddress,
			(byte*)&data[0],
			i+10);
		/* the most current userinfo has been sent, so watch for any
		 * newer changes to userinfo variables */
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		comerrorf(ERR_FATAL, "CL_CheckForResend: bad clc.state");
	}
}

/*
 * Sometimes the server can drop the client and the netchan based
 * disconnect can be lost.  If the client continues to send packets
 * to the server, the server will send out of band disconnect packets
 * to the client so it doesn't have to wait for the full timeout period.
 */
void
cldisconnectPacket(Netaddr from)
{
	if(clc.state < CA_AUTHORIZING)
		return;

	/* if not from our server, ignore it */
	if(!equaladdr(from, clc.netchan.remoteAddress))
		return;

	/* if we have received packets within three seconds, ignore it
	 * (it might be a malicious spoof) */
	if(cls.simtime - clc.lastPacketTime < 3000)
		return;

	/* drop the connection */
	comprintf("Server disconnected for unknown reason\n");
	cvarsetstr("com_errorMessage", "Server disconnected for unknown reason\n");
	cldisconnect(qtrue);
}

void
CL_MotdPacket(Netaddr from)
{
#ifdef UPDATE_SERVER_NAME
	char *challenge;
	char *info;

	/* if not from our server, ignore it */
	if(!equaladdr(from, cls.updateServer))
		return;

	info = cmdargv(1);

	/* check challenge */
	challenge = Info_ValueForKey(info, "challenge");
	if(strcmp(challenge, cls.updateChallenge))
		return;

	challenge = Info_ValueForKey(info, "motd");

	Q_strncpyz(cls.updateInfoString, info, sizeof(cls.updateInfoString));
	cvarsetstr("cl_motdString", challenge);
#endif
}

void
clinitServerInfo(Servinfo *server, Netaddr *address)
{
	server->adr = *address;
	server->clients = 0;
	server->hostName[0]	= '\0';
	server->mapName[0]	= '\0';
	server->maxClients	= 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0]		= '\0';
	server->gameType	= 0;
	server->netType		= 0;
}

#define MAX_SERVERSPERPACKET 256

void
CL_ServersResponsePacket(const Netaddr* from, Bitmsg *msg, qbool extended)
{
	int	i, j, count, total;
	Netaddr addresses[MAX_SERVERSPERPACKET];
	int	numservers;
	byte * buffptr;
	byte * buffend;

	comprintf("CL_ServersResponsePacket\n");

	if(cls.numglobalservers == -1){
		/* state to detect lack of servers or lack of response */
		cls.numglobalservers = 0;
		cls.numGlobalServerAddresses = 0;
	}

	/* parse through server response string */
	numservers = 0;
	buffptr = msg->data;
	buffend = buffptr + msg->cursize;

	/* advance to initial token */
	do {
		if(*buffptr == '\\' || (extended && *buffptr == '/'))
			break;

		buffptr++;
	} while(buffptr < buffend);

	while(buffptr + 1 < buffend){
		/* IPv4 address */
		if(*buffptr == '\\'){
			buffptr++;

			if(buffend - buffptr <
			   sizeof(addresses[numservers].ip) +
			   sizeof(addresses[numservers].port) + 1)
				break;

			for(i = 0; i < sizeof(addresses[numservers].ip); i++)
				addresses[numservers].ip[i] = *buffptr++;

			addresses[numservers].type = NA_IP;
		}
		/* IPv6 address, if it's an extended response */
		else if(extended && *buffptr == '/'){
			buffptr++;

			if(buffend - buffptr <
			   sizeof(addresses[numservers].ip6) +
			   sizeof(addresses[numservers].port) + 1)
				break;

			for(i = 0; i < sizeof(addresses[numservers].ip6); i++)
				addresses[numservers].ip6[i] = *buffptr++;

			addresses[numservers].type = NA_IP6;
			addresses[numservers].scope_id = from->scope_id;
		}else
			/* syntax error! */
			break;

		/* parse out port */
		addresses[numservers].port	= (*buffptr++) << 8;
		addresses[numservers].port	+= *buffptr++;
		addresses[numservers].port	= BigShort(
			addresses[numservers].port);

		/* syntax check */
		if(*buffptr != '\\' && *buffptr != '/')
			break;

		numservers++;
		if(numservers >= MAX_SERVERSPERPACKET)
			break;
	}

	count = cls.numglobalservers;

	for(i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++){
		/* build net address */
		Servinfo *server = &cls.globalServers[count];

		/* Tequila: It's possible to have sent many master server requests. Then
		 * we may receive many times the same addresses from the master server.
		 * We just avoid to add a server if it is still in the global servers list. */
		for(j = 0; j < count; j++)
			if(equaladdr(cls.globalServers[j].adr, addresses[i]))
				break;

		if(j < count)
			continue;

		clinitServerInfo(server, &addresses[i]);
		/* advance to next slot */
		count++;
	}

	/* if getting the global list */
	if(count >= MAX_GLOBAL_SERVERS && cls.numGlobalServerAddresses <
	   MAX_GLOBAL_SERVERS)
		/* if we couldn't store the servers in the main list anymore */
		for(;
		    i < numservers && cls.numGlobalServerAddresses <
		    MAX_GLOBAL_SERVERS; i++)
			/* just store the addresses in an additional list */
			cls.globalServerAddresses[cls.numGlobalServerAddresses++
			] = addresses[i];

	cls.numglobalservers = count;
	total = count + cls.numGlobalServerAddresses;

	comprintf("%d servers parsed (total %d)\n", numservers, total);
}

/*
 * Responses to broadcasts, etc
 */
void
CL_ConnectionlessPacket(Netaddr from, Bitmsg *msg)
{
	char	*s;
	char	*c;
	int	challenge = 0;

	bmstartreadingOOB(msg);
	bmreadl(msg);	/* skip the -1 */

	s = bmreadstrline(msg);

	cmdstrtok(s);

	c = cmdargv(0);

	comdprintf ("CL packet %s: %s\n", addrporttostr(from), c);

	/* challenge from the server we are connecting to */
	if(!Q_stricmp(c, "challengeResponse")){
		char	*strver;
		int	ver;

		if(clc.state != CA_CONNECTING){
			comdprintf(
				"Unwanted challenge response received. Ignored.\n");
			return;
		}

		c = cmdargv(2);
		if(*c)
			challenge = atoi(c);

		strver = cmdargv(3);
		if(*strver){
			ver = atoi(strver);

			if(ver != com_protocol->integer){
				{
					comprintf(
						S_COLOR_YELLOW
						"Warning: Server reports protocol version %d, we have %d. "
						"Trying anyways.\n",
						ver, com_protocol->integer);
				}
			}
		}
		{
			if(!*c || challenge != clc.challenge){
				comprintf(
					"Bad challenge for challengeResponse. Ignored.\n");
				return;
			}
		}

		/* start sending challenge response instead of challenge request packets */
		clc.challenge = atoi(cmdargv(1));
		clc.state = CA_CHALLENGING;
		clc.connectPacketCount = 0;
		clc.connectTime = -99999;

		/* take this address as the new server address.  This allows
		 * a server proxy to hand off connections to multiple servers */
		clc.serverAddress = from;
		comdprintf ("challengeResponse: %d\n", clc.challenge);
		return;
	}

	/* server connection */
	if(!Q_stricmp(c, "connectResponse")){
		if(clc.state >= CA_CONNECTED){
			comprintf ("Dup connect received. Ignored.\n");
			return;
		}
		if(clc.state != CA_CHALLENGING){
			comprintf (
				"connectResponse packet while not connecting. Ignored.\n");
			return;
		}
		if(!equaladdr(from, clc.serverAddress)){
			comprintf(
				"connectResponse from wrong address. Ignored.\n");
			return;
		}

		{
			c = cmdargv(1);

			if(*c)
				challenge = atoi(c);
			else{
				comprintf(
					"Bad connectResponse received. Ignored.\n");
				return;
			}

			if(challenge != clc.challenge){
				comprintf(
					"ConnectResponse with bad challenge received. Ignored.\n");
				return;
			}
		}

		ncsetup(NS_CLIENT, &clc.netchan, from,
			cvargetf("net_qport"),
			clc.challenge, qfalse);

		clc.state = CA_CONNECTED;
		clc.lastPacketSentTime = -9999;	/* send first packet immediately */
		return;
	}

	/* server responding to an info broadcast */
	if(!Q_stricmp(c, "infoResponse")){
		CL_ServerInfoPacket(from, msg);
		return;
	}

	/* server responding to a get playerlist */
	if(!Q_stricmp(c, "statusResponse")){
		CL_ServerStatusResponse(from, msg);
		return;
	}

	/* echo request from server */
	if(!Q_stricmp(c, "echo")){
		netprintOOB(NS_CLIENT, from, "%s", cmdargv(1));
		return;
	}

	/* cd check */
	if(!Q_stricmp(c, "keyAuthorize"))
		/* we don't use these now, so dump them on the floor */
		return;

	/* global MOTD from id */
	if(!Q_stricmp(c, "motd")){
		CL_MotdPacket(from);
		return;
	}

	/* echo request from server */
	if(!Q_stricmp(c, "print")){
		s = bmreadstr(msg);

		Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
		comprintf("%s", s);

		return;
	}

	/* list of servers sent back by a master server (classic) */
	if(!Q_strncmp(c, "getserversResponse", 18)){
		CL_ServersResponsePacket(&from, msg, qfalse);
		return;
	}

	/* list of servers sent back by a master server (extended) */
	if(!Q_strncmp(c, "getserversExtResponse", 21)){
		CL_ServersResponsePacket(&from, msg, qtrue);
		return;
	}

	comdprintf ("Unknown connectionless packet command.\n");
}

/*
 * A packet has arrived from the main event loop
 */
void
clpacketevent(Netaddr from, Bitmsg *msg)
{
	int headerBytes;

	clc.lastPacketTime = cls.simtime;

	if(msg->cursize >= 4 && *(int*)msg->data == -1){
		CL_ConnectionlessPacket(from, msg);
		return;
	}

	if(clc.state < CA_CONNECTED)
		return;		/* can't be a valid sequenced packet */

	if(msg->cursize < 4){
		comprintf ("%s: Runt packet\n", addrporttostr(from));
		return;
	}

	/*
	 * packet from server
	 *  */
	if(!equaladdr(from, clc.netchan.remoteAddress)){
		comdprintf ("%s:sequenced packet without connection\n"
			, addrporttostr(from));
		/* FIXME: send a client disconnect? */
		return;
	}

	if(!CL_Netchan_Process(&clc.netchan, msg))
		return;		/* out of order, duplicated, etc */

	/* the header is different lengths for reliable and unreliable messages */
	headerBytes = msg->readcount;

	/* track the last message received so it can be returned in
	 * client messages, allowing the server to detect a dropped
	 * gamestate */
	clc.serverMessageSequence = LittleLong(*(int*)msg->data);

	clc.lastPacketTime = cls.simtime;
	CL_ParseServerMessage(msg);

	/*
	 * we don't know if it is ok to save a demo message until
	 * after we have parsed the frame
	 *  */
	if(clc.demorecording && !clc.demowaiting)
		CL_WriteDemoMessage(msg, headerBytes);
}

void
CL_CheckTimeout(void)
{
	/*
	 * check timeout
	 *  */
	if((!CL_CheckPaused() || !sv_paused->integer)
	   && clc.state >= CA_CONNECTED && clc.state != CA_CINEMATIC
	   && cls.simtime - clc.lastPacketTime > cl_timeout->value*1000){
		if(++cl.timeoutcount > 5){	/* timeoutcount saves debugger */
			comprintf ("\nServer connection timed out.\n");
			cldisconnect(qtrue);
			return;
		}
	}else
		cl.timeoutcount = 0;
}

/*
 * Check whether client has been paused.
 */
qbool
CL_CheckPaused(void)
{
	/* if cl_paused->modified is set, the cvar has only been changed in
	 * this frame. Keep paused in this frame to ensure the server doesn't
	 * lag behind. */
	if(cl_paused->integer || cl_paused->modified)
		return qtrue;

	return qfalse;
}

void
CL_CheckUserinfo(void)
{
	/* don't add reliable commands when not yet connected */
	if(clc.state < CA_CONNECTED)
		return;

	/* don't overflow the reliable command buffer when paused */
	if(CL_CheckPaused())
		return;

	/* send a reliable userinfo update if needed */
	if(cvar_modifiedFlags & CVAR_USERINFO){
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand(va("userinfo \"%s\"",
				cvargetinfostr(CVAR_USERINFO)), qfalse);
	}
}

void
clframe(int msec)
{
	int realframetime, lasttime;

	if(!com_cl_running->integer)
		return;
	lasttime = cls.realtime;

#ifdef USE_CURL
	if(clc.downloadCURLM){
		CL_cURL_PerformDownload();
		/* we can't process frames normally when in disconnected
		 * download mode since the ui vm expects clc.state to be
		 * CA_CONNECTED */
		if(clc.cURLDisconnected){
			cls.simframetime = msec;	/* save the msec before checking pause */
			cls.simtime += cls.simframetime;	/* decide the simulation time */
			cls.realframetime = sysmillisecs() - lasttime;
			cls.realtime += cls.realframetime;
			SCR_UpdateScreen();
			sndupdate();
			Con_RunConsole();
			cls.framecount++;
			return;
		}
	}
#endif

	if(clc.state == CA_DISCONNECTED &&
		 !(Key_GetCatcher( ) & KEYCATCH_UI)
		 && !com_sv_running->integer && uivm){
		/* if disconnected, bring up the menu */
		sndstopall();
		vmcall(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
	}

	/* if recording an avi, lock to a fixed fps */
	if(CL_VideoRecording( ) && cl_aviFrameRate->integer && msec)
		/* save the current screen */
		if(clc.state == CA_ACTIVE || cl_forceavidemo->integer){
			CL_TakeVideoFrame( );

			/* fixed time for next frame' */
			msec = (int)ceil(
				(1000.0f /
				 cl_aviFrameRate->value) * com_timescale->value);
			if(msec == 0)
				msec = 1;
		}

	if(cl_autoRecordDemo->integer){
		if(clc.state == CA_ACTIVE && !clc.demorecording &&
		   !clc.demoplaying){
			/* If not recording a demo, and we should be, start one */
			Qtime now;
			char	*nowString;
			char            *p;
			char	mapName[ MAX_QPATH ];
			char	serverName[ MAX_OSPATH ];

			comrealtime(&now);
			nowString = va("%04d%02d%02d%02d%02d%02d",
				1900 + now.tm_year,
				1 + now.tm_mon,
				now.tm_mday,
				now.tm_hour,
				now.tm_min,
				now.tm_sec);

			Q_strncpyz(serverName, clc.servername, MAX_OSPATH);
			/* Replace the ":" in the address as it is not a valid
			 * file name character */
			p = strstr(serverName, ":");
			if(p)
				*p = '.';

			Q_strncpyz(mapName, Q_skippath(cl.mapname),
				sizeof(cl.mapname));
			Q_stripext(mapName, mapName, sizeof(mapName));

			cbufexecstr(EXEC_NOW,
				va("record %s-%s-%s", nowString, serverName,
					mapName));
		}else if(clc.state != CA_ACTIVE && clc.demorecording)
			/* Recording, but not CA_ACTIVE, so stop recording */
			CL_StopRecord_f( );
	}

	cls.simframetime = msec;	/* save the msec before checking pause */
	cls.simtime += cls.simframetime;	/* decide the simulation time */
	cls.realframetime = sysmillisecs() - lasttime;
	cls.realtime += cls.realframetime;

	if(cl_timegraph->integer)
		scrdebuggraph (cls.simframetime * 0.25);

	/* see if we need to update any userinfo */
	CL_CheckUserinfo();

	/* if we haven't gotten a packet in a long time,
	 * drop the connection */
	CL_CheckTimeout();

	/* send intentions now */
	CL_SendCmd();

	/* resend a connection request if necessary */
	CL_CheckForResend();

	/* decide on the serverTime to render */
	CL_SetCGameTime();

	/* update the screen */
	SCR_UpdateScreen();

	/* update audio */
	sndupdate();

#ifdef USE_VOIP
	CL_CaptureVoip();
#endif

#ifdef USE_MUMBLE
	CL_UpdateMumble();
#endif

	/* advance local effects for next frame */
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}

/*
 * DLL glue
 */
static __attribute__ ((format (printf, 2, 3))) void QDECL
CL_RefPrintf(int print_level, const char *fmt, ...)
{
	va_list argptr;
	char	msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if(print_level == PRINT_ALL)
		comprintf ("%s", msg);
	else if(print_level == PRINT_WARNING)
		comprintf (S_COLOR_YELLOW "%s", msg);	/* yellow */
	else if(print_level == PRINT_DEVELOPER)
		comdprintf (S_COLOR_RED "%s", msg);	/* red */
}

void
clshutdownRef(void)
{
	if(!re.Shutdown)
		return;
	re.Shutdown(qtrue);
	Q_Memset(&re, 0, sizeof(re));
}

void
clinitRenderer(void)
{
	/* this sets up the renderer and calls R_Init */
	re.BeginRegistration(&cls.glconfig);

	/* load character sets */
	cls.charSetShader	= re.RegisterShader(P2dart "/bigchars");
	cls.whiteShader		= re.RegisterShader("white");
	re.RegisterFont("fonts/DejaVuSans.ttf", 24, &cls.consolefont);
	cls.consoleShader = re.RegisterShader("console");
	g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
	g_consoleField.widthInChars = g_console_field_width;
}

/*
 * After the server has cleared the hunk, these will need to be restarted
 * This is the only place that any of these functions are called from
 */
void
clstarthunkusers(qbool rendererOnly)
{
	if(!com_cl_running)
		return;
	if(!com_cl_running->integer)
		return;
	if(!cls.rendererStarted){
		cls.rendererStarted = qtrue;
		clinitRenderer();
	}
	if(rendererOnly)
		return;
	if(!cls.soundStarted){
		cls.soundStarted = qtrue;
		sndinit();
	}
	if(!cls.soundRegistered){
		cls.soundRegistered = qtrue;
		sndbeginreg();
	}
	if(com_dedicated->integer)
		return;
	if(!cls.uiStarted){
		cls.uiStarted = qtrue;
		clinitUI();
	}
}

void *
CL_RefMalloc(int size)
{
	return ztagalloc(size, MTrenderer);
}

int
CL_ScaledMilliseconds(void)
{
	return sysmillisecs()*com_timescale->value;
}

void
clinitref(void)
{
	Refimport	ri;
	Refexport	*ret;
#ifdef USE_RENDERER_DLOPEN
	GetRefAPI_t	GetRefAPI;
	char dllName[MAX_OSPATH];
#endif

	comprintf("----- Initializing refresh module -----\n");

#ifdef USE_RENDERER_DLOPEN
	cl_renderer = cvarget("cl_renderer", "2", CVAR_ARCHIVE | CVAR_LATCH);

	Q_sprintf(dllName, sizeof(dllName), "ref%s-" ARCH_STRING DLL_EXT,
		cl_renderer->string);

	if(!(rendererLib = Sys_LoadDll(dllName, qfalse)) 
	   && strcmp(cl_renderer->string, cl_renderer->resetString) != 0){
		cvarforcereset("cl_renderer");

		Q_sprintf(dllName, sizeof(dllName),
			"ref1-" ARCH_STRING DLL_EXT);
		rendererLib = Sys_LoadLibrary(dllName);
	}

	if(!rendererLib){
		comprintf("failed:\n\"%s\"\n", Sys_LibraryError());
		comerrorf(ERR_FATAL, "Failed to load renderer");
	}

	GetRefAPI = Sys_LoadFunction(rendererLib, "GetRefAPI");
	if(!GetRefAPI)
		comerrorf(ERR_FATAL, "Can't load symbol GetRefAPI: '%s'",
			Sys_LibraryError());
#endif

	ri.cmdadd = cmdadd;
	ri.cmdremove = cmdremove;
	ri.cmdargc	= cmdargc;
	ri.cmdargv	= cmdargv;
	ri.Cmd_ExecuteText = cbufexecstr;
	ri.Printf	= CL_RefPrintf;
	ri.Error	= comerrorf;
	ri.Milliseconds	= CL_ScaledMilliseconds;
	ri.Malloc	= CL_RefMalloc;
	ri.Free		= zfree;
#ifdef HUNK_DEBUG
	ri.hunkallocdebug = hunkallocdebug;
#else
	ri.hunkalloc = hunkalloc;
#endif
	ri.hunkalloctemp	= hunkalloctemp;
	ri.hunkfreetemp	= hunkfreetemp;

	ri.CM_ClusterPVS	= CM_ClusterPVS;
	ri.CM_DrawDebugSurface	= CM_DrawDebugSurface;

	ri.fsreadfile	= fsreadfile;
	ri.fsfreefile	= fsfreefile;
	ri.fswritefile = fswritefile;
	ri.fsfreefilelist = fsfreefilelist;
	ri.fslistfiles		= fslistfiles;
	ri.fsfileisinpak	= fsfileisinpak;
	ri.fsfileexists	= fsfileexists;
	ri.cvarget	= cvarget;
	ri.cvarsetstr	= cvarsetstr;
	ri.cvarsetf	= cvarsetf;
	ri.cvarcheckrange	= cvarcheckrange;
	ri.cvargeti	= cvargeti;

	/* cinematic stuff */
	ri.CIN_UploadCinematic	= CIN_UploadCinematic;
	ri.CIN_PlayCinematic	= CIN_PlayCinematic;
	ri.CIN_RunCinematic	= CIN_RunCinematic;

	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

	/* FIXME: nice separation of concerns there... */
	ri.IN_Init	= IN_Init;
	ri.IN_Shutdown	= IN_Shutdown;
	ri.IN_Restart	= IN_Restart;

	ri.ftol = Q_ftol;

	ri.syssetenv = syssetenv;
	ri.Sys_GLimpSafeInit = Sys_GLimpSafeInit;
	ri.Sys_GLimpInit = Sys_GLimpInit;
	ri.syslowmem = syslowmem;

	ret = GetRefAPI(REF_API_VERSION, &ri);

	comprintf("-------------------------------\n");

	if(!ret)
		comerrorf (ERR_FATAL, "Couldn't initialize refresh module");

	re = *ret;

	/* unpause so the cgame definitely gets a snapshot and renders a frame */
	cvarsetstr("cl_paused", "0");
}

void
CL_SetModel_f(void)
{
	char	*arg;
	char	name[256];

	arg = cmdargv(1);
	if(arg[0]){
		cvarsetstr("model", arg);
		cvarsetstr("headmodel", arg);
	}else{
		cvargetstrbuf("model", name, sizeof(name));
		comprintf("model is set to %s\n", name);
	}
}

/*
 * video
 * video [filename]
 */
void
CL_Video_f(void)
{
	char	filename[ MAX_OSPATH ];
	int	i, last;

	if(!clc.demoplaying){
		comprintf(
			"The video command can only be used when playing back demos\n");
		return;
	}

	if(cmdargc( ) == 2)
		/* explicit filename */
		Q_sprintf(filename, MAX_OSPATH, "videos/%s.avi", cmdargv(1));
	else{
		/* scan for a free filename */
		for(i = 0; i <= 9999; i++){
			int a, b, c, d;

			last = i;

			a = last / 1000;
			last -= a * 1000;
			b = last / 100;
			last -= b * 100;
			c = last / 10;
			last -= c * 10;
			d = last;

			Q_sprintf(filename, MAX_OSPATH,
				"videos/video%d%d%d%d.avi",
				a, b, c,
				d);

			if(!fsfileexists(filename))
				break;	/* file doesn't exist */
		}

		if(i > 9999){
			comprintf(
				S_COLOR_RED
				"ERROR: no free file names to create video\n");
			return;
		}
	}

	CL_OpenAVIForWriting(filename);
}

void
CL_StopVideo_f(void)
{
	CL_CloseAVI( );
}

/*
 * test to see if a valid QKEY_FILE exists.  If one does not, try to generate
 * it by filling it with 2048 bytes of random data.
 */
static void
CL_GenerateQKey(void)
{
	int len = 0;
	unsigned char	buff[ QKEY_SIZE ];
	Fhandle	f;

	len = fssvopenr(QKEY_FILE, &f);
	fsclose(f);
	if(len == QKEY_SIZE){
		comprintf("QKEY found.\n");
		return;
	}else{
		if(len > 0)
			comprintf("QKEY file size != %d, regenerating\n",
				QKEY_SIZE);

		comprintf("QKEY building random string\n");
		comrandbytes(buff, sizeof(buff));

		f = fssvopenw(QKEY_FILE);
		if(!f){
			comprintf("QKEY could not open %s for write\n",
				QKEY_FILE);
			return;
		}
		fswrite(buff, sizeof(buff), f);
		fsclose(f);
		comprintf("QKEY generated\n");
	}
}

void
clinit(void)
{
	comprintf("----- Client Initialization -----\n");

	Con_Init();

	if(!com_fullyInitialized){
		CL_ClearState();
		clc.state = CA_DISCONNECTED;	/* no longer CA_UNINITIALIZED */
		cls.oldGameSet = qfalse;
	}

	cls.simtime = 0;
	cls.realtime = 0;

	clinitInput ();

	/*
	 * register our variables
	 *  */
	cl_noprint = cvarget("cl_noprint", "0", 0);
#ifdef UPDATE_SERVER_NAME
	cl_motd = cvarget ("cl_motd", "1", 0);
#endif

	cl_timeout = cvarget ("cl_timeout", "200", 0);

	cl_timeNudge	= cvarget ("cl_timeNudge", "0", CVAR_TEMP);
	cl_shownet	= cvarget ("cl_shownet", "0", CVAR_TEMP);
	cl_showSend	= cvarget ("cl_showSend", "0", CVAR_TEMP);
	cl_showTimeDelta = cvarget ("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo = cvarget ("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password = cvarget ("rconPassword", "", CVAR_TEMP);
	cl_activeAction = cvarget("activeAction", "", CVAR_TEMP);

	cl_timedemo = cvarget ("timedemo", "0", 0);
	cl_timedemoLog = cvarget ("cl_timedemoLog", "", CVAR_ARCHIVE);
	cl_autoRecordDemo = cvarget ("cl_autoRecordDemo", "0",
		CVAR_ARCHIVE);
	cl_aviFrameRate = cvarget ("cl_aviFrameRate", "25",
		CVAR_ARCHIVE);
	cl_aviMotionJpeg = cvarget ("cl_aviMotionJpeg", "1",
		CVAR_ARCHIVE);
	cl_forceavidemo = cvarget ("cl_forceavidemo", "0", 0);

	rconAddress = cvarget ("rconAddress", "", 0);

	cl_yawspeed	= cvarget ("cl_yawspeed", "140", CVAR_ARCHIVE);
	cl_pitchspeed	= cvarget ("cl_pitchspeed", "140", CVAR_ARCHIVE);
	cl_rollspeed	= cvarget ("cl_rollspeed", "140", CVAR_ARCHIVE);
	cl_anglespeedkey = cvarget ("cl_anglespeedkey", "1.5", 0);

	cl_maxpackets	= cvarget ("cl_maxpackets", "30", CVAR_ARCHIVE);
	cl_packetdup	= cvarget ("cl_packetdup", "1", CVAR_ARCHIVE);

	cl_run = cvarget ("cl_run", "1", CVAR_ARCHIVE);
	cl_sensitivity	= cvarget ("sensitivity", "5", CVAR_ARCHIVE);
	cl_mouseAccel	= cvarget ("cl_mouseAccel", "0", CVAR_ARCHIVE);
	cl_freelook = cvarget("cl_freelook", "1", CVAR_ARCHIVE);

	/* 0: legacy mouse acceleration
	 * 1: new implementation */
	cl_mouseAccelStyle = cvarget("cl_mouseAccelStyle", "0", CVAR_ARCHIVE);
	/* offset for the power function (for style 1, ignored otherwise)
	 * this should be set to the max rate value */
	cl_mouseAccelOffset = cvarget("cl_mouseAccelOffset", "5", CVAR_ARCHIVE);
	cvarcheckrange(cl_mouseAccelOffset, 0.001f, 50000.0f, qfalse);

	cl_showMouseRate = cvarget ("cl_showmouserate", "0", 0);

	cl_allowDownload = cvarget ("cl_allowDownload", "0", CVAR_ARCHIVE);
#ifdef USE_CURL_DLOPEN
	cl_cURLLib = cvarget("cl_cURLLib", DEFAULT_CURL_LIB, CVAR_ARCHIVE);
#endif

	cl_conXOffset = cvarget ("cl_conXOffset", "0", 0);
#ifdef MACOS_X
	/* In game video is REALLY slow in Mac OS X right now due to driver slowness */
	cl_inGameVideo = cvarget ("r_inGameVideo", "0", CVAR_ARCHIVE);
#else
	cl_inGameVideo = cvarget ("r_inGameVideo", "1", CVAR_ARCHIVE);
#endif

	cl_serverStatusResendTime = cvarget ("cl_serverStatusResendTime", "750",
		0);

	/* init autoswitch so the ui will have it correctly even
	 * if the cgame hasn't been started */
	cvarget ("cg_autoswitch", "1", CVAR_ARCHIVE);

	m_pitch		= cvarget ("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw		= cvarget ("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward	= cvarget ("m_forward", "0.25", CVAR_ARCHIVE);
	m_side		= cvarget ("m_side", "0.25", CVAR_ARCHIVE);
#ifdef MACOS_X
	/* Input is jittery on OS X w/o this */
	m_filter = cvarget ("m_filter", "1", CVAR_ARCHIVE);
#else
	m_filter = cvarget ("m_filter", "0", CVAR_ARCHIVE);
#endif

	j_pitch = cvarget("j_pitch", "0.022", CVAR_ARCHIVE);
	j_yaw = cvarget("j_yaw", "-0.022", CVAR_ARCHIVE);
	j_forward = cvarget("j_forward", "-0.25", CVAR_ARCHIVE);
	j_side = cvarget("j_side", "0.25", CVAR_ARCHIVE);
	j_up = cvarget("j_up", "1", CVAR_ARCHIVE);
	j_pitch_axis = cvarget("j_pitch_axis", "3", CVAR_ARCHIVE);
	j_yaw_axis = cvarget("j_yaw_axis", "4", CVAR_ARCHIVE);
	j_forward_axis	= cvarget("j_forward_axis", "1", CVAR_ARCHIVE);
	j_side_axis = cvarget("j_side_axis", "0", CVAR_ARCHIVE);
	j_up_axis = cvarget("j_up_axis", "2", CVAR_ARCHIVE);
	cvarcheckrange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	cvarcheckrange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	cvarcheckrange(j_forward_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	cvarcheckrange(j_side_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);
	cvarcheckrange(j_up_axis, 0, MAX_JOYSTICK_AXIS-1, qtrue);

	cl_motdString = cvarget("cl_motdString", "", CVAR_ROM);

	cvarget("cl_maxPing", "800", CVAR_ARCHIVE);

	cl_lanForcePackets = cvarget ("cl_lanForcePackets", "1", CVAR_ARCHIVE);

	cl_guidServerUniq = cvarget ("cl_guidServerUniq", "1", CVAR_ARCHIVE);

	/* ~ and `, as keys and characters */
	cl_consoleKeys = cvarget("cl_consoleKeys", "~ ` 0x7e 0x60",
		CVAR_ARCHIVE);

	/* userinfo */
	cvarget ("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("model", "griever", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("headmodel", "griever", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("g_redTeam", "Stroggs", CVAR_SERVERINFO | CVAR_ARCHIVE);
	cvarget ("g_blueTeam", "Pagans", CVAR_SERVERINFO | CVAR_ARCHIVE);
	cvarget ("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("handicap", "1000", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("teamtask", "0", CVAR_USERINFO);
	cvarget ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	cvarget ("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	cvarget ("password", "", CVAR_USERINFO);
	cvarget ("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE);

#ifdef USE_MUMBLE
	cl_useMumble = cvarget ("cl_useMumble", "0",
		CVAR_ARCHIVE | CVAR_LATCH);
	cl_mumbleScale = cvarget ("cl_mumbleScale", "0.0254", CVAR_ARCHIVE);
#endif

#ifdef USE_VOIP
	cl_voipSend = cvarget ("cl_voipSend", "0", 0);
	cl_voipSendTarget = cvarget ("cl_voipSendTarget", "spatial", 0);
	cl_voipGainDuringCapture = cvarget ("cl_voipGainDuringCapture", "0.2",
		CVAR_ARCHIVE);
	cl_voipCaptureMult = cvarget ("cl_voipCaptureMult", "2.0", CVAR_ARCHIVE);
	cl_voipUseVAD = cvarget ("cl_voipUseVAD", "0", CVAR_ARCHIVE);
	cl_voipVADThreshold = cvarget ("cl_voipVADThreshold", "0.25",
		CVAR_ARCHIVE);
	cl_voipShowMeter = cvarget ("cl_voipShowMeter", "1", CVAR_ARCHIVE);

	/* This is a protocol version number. */
	cl_voip = cvarget ("cl_voip", "1",
		CVAR_USERINFO | CVAR_ARCHIVE | CVAR_LATCH);
	cvarcheckrange(cl_voip, 0, 1, qtrue);

	/* If your data rate is too low, you'll get Connection Interrupted warnings
	 *  when VoIP packets arrive, even if you have a broadband connection.
	 *  This might work on rates lower than 25000, but for safety's sake, we'll
	 *  just demand it. Who doesn't have at least a DSL line now, anyhow? If
	 *  you don't, you don't need VoIP.  :) */
	if((cl_voip->integer) && (cvargeti("rate") < 25000)){
		comprintf(
			S_COLOR_YELLOW
			"Your network rate is too slow for VoIP.\n");
		comprintf(
			"Set 'Data Rate' to 'LAN/Cable/xDSL' in 'Setup/System/Network' and restart.\n");
		comprintf("Until then, VoIP is disabled.\n");
		cvarsetstr("cl_voip", "0");
	}
#endif


	/* cgame might not be initialized before menu is used */
	cvarget ("cg_viewsize", "100", CVAR_ARCHIVE);
	/* Make sure cg_stereoSeparation is zero as that variable is deprecated and should not be used anymore. */
	cvarget ("cg_stereoSeparation", "0", CVAR_ROM);

	/*
	 * register our commands
	 *  */
	cmdadd ("cmd", CL_ForwardToServer_f);
	cmdadd ("configstrings", CL_Configstrings_f);
	cmdadd ("clientinfo", CL_Clientinfo_f);
	cmdadd ("snd_restart", CL_Snd_Restart_f);
	cmdadd ("vid_restart", CL_Vid_Restart_f);
	cmdadd ("disconnect", cldisconnect_f);
	cmdadd ("record", CL_Record_f);
	cmdadd ("demo", CL_PlayDemo_f);
	cmdsetcompletion("demo", CL_CompleteDemoName);
	cmdadd ("cinematic", CL_PlayCinematic_f);
	cmdadd ("stoprecord", CL_StopRecord_f);
	cmdadd ("connect", CL_Connect_f);
	cmdadd ("reconnect", CL_Reconnect_f);
	cmdadd ("localservers", CL_LocalServers_f);
	cmdadd ("globalservers", CL_GlobalServers_f);
	cmdadd ("rcon", CL_Rcon_f);
	cmdsetcompletion("rcon", CL_CompleteRcon);
	cmdadd ("ping", CL_Ping_f);
	cmdadd ("serverstatus", CL_ServerStatus_f);
	cmdadd ("showip", CL_ShowIP_f);
	cmdadd ("fs_openedList", CL_OpenedPK3List_f);
	cmdadd ("fs_referencedList", CL_ReferencedPK3List_f);
	cmdadd ("model", CL_SetModel_f);
	cmdadd ("video", CL_Video_f);
	cmdadd ("stopvideo", CL_StopVideo_f);
	clinitref();

	SCR_Init ();

/*	cbufflush (); */

	cvarsetstr("cl_running", "1");

	CL_GenerateQKey();
	cvarget("cl_guid", "", CVAR_USERINFO | CVAR_ROM);
	CL_UpdateGUID(NULL, 0);

	comprintf("----- Client Initialization Complete -----\n");
}

void
clshutdown(char *finalmsg, qbool disconnect, qbool quit)
{
	static qbool recursive = qfalse;

	/* check whether the client is running at all. */
	if(!(com_cl_running && com_cl_running->integer))
		return;

	comprintf("----- Client Shutdown (%s) -----\n", finalmsg);

	if(recursive){
		comprintf("WARNING: Recursive shutdown\n");
		return;
	}
	recursive = qtrue;

	noGameRestart = quit;

	if(disconnect)
		cldisconnect(qtrue);

	CL_ClearMemory(qtrue);
	clsndshutdown();

	cmdremove ("cmd");
	cmdremove ("configstrings");
	cmdremove ("clientinfo");
	cmdremove ("snd_restart");
	cmdremove ("vid_restart");
	cmdremove ("disconnect");
	cmdremove ("record");
	cmdremove ("demo");
	cmdremove ("cinematic");
	cmdremove ("stoprecord");
	cmdremove ("connect");
	cmdremove ("reconnect");
	cmdremove ("localservers");
	cmdremove ("globalservers");
	cmdremove ("rcon");
	cmdremove ("ping");
	cmdremove ("serverstatus");
	cmdremove ("showip");
	cmdremove ("fs_openedList");
	cmdremove ("fs_referencedList");
	cmdremove ("model");
	cmdremove ("video");
	cmdremove ("stopvideo");

	clshutdownInput();
	Con_Shutdown();

	cvarsetstr("cl_running", "0");

	recursive = qfalse;

	Q_Memset(&cls, 0, sizeof(cls));
	Key_SetCatcher(0);

	comprintf("-----------------------\n");

}

static void
CL_SetServerInfo(Servinfo *server, const char *info, int ping)
{
	if(server){
		if(info){
			server->clients = atoi(Info_ValueForKey(info, "clients"));
			Q_strncpyz(server->hostName,
				Info_ValueForKey(info,
					"hostname"), MAX_NAME_LENGTH);
			Q_strncpyz(server->mapName,
				Info_ValueForKey(info,
					"mapname"), MAX_NAME_LENGTH);
			server->maxClients =
				atoi(Info_ValueForKey(info, "sv_maxclients"));
			Q_strncpyz(server->game,Info_ValueForKey(info,
					"game"), MAX_NAME_LENGTH);
			server->gameType =
				atoi(Info_ValueForKey(info, "gametype"));
			server->netType =
				atoi(Info_ValueForKey(info, "nettype"));
			server->minPing =
				atoi(Info_ValueForKey(info, "minping"));
			server->maxPing =
				atoi(Info_ValueForKey(info, "maxping"));
			server->g_humanplayers =
				atoi(Info_ValueForKey(info, "g_humanplayers"));
			server->g_needpass =
				atoi(Info_ValueForKey(info, "g_needpass"));
		}
		server->ping = ping;
	}
}

static void
CL_SetServerInfoByAddress(Netaddr from, const char *info, int ping)
{
	int i;

	for(i = 0; i < MAX_OTHER_SERVERS; i++)
		if(equaladdr(from, cls.localServers[i].adr))
			CL_SetServerInfo(&cls.localServers[i], info, ping);

	for(i = 0; i < MAX_GLOBAL_SERVERS; i++)
		if(equaladdr(from, cls.globalServers[i].adr))
			CL_SetServerInfo(&cls.globalServers[i], info, ping);

	for(i = 0; i < MAX_OTHER_SERVERS; i++)
		if(equaladdr(from, cls.favoriteServers[i].adr))
			CL_SetServerInfo(&cls.favoriteServers[i], info, ping);

}

void
CL_ServerInfoPacket(Netaddr from, Bitmsg *msg)
{
	int	i, type;
	char	info[MAX_INFO_STRING];
	char    *infoString;
	int	prot;
	char *gamename;
	qbool gameMismatch;

	infoString = bmreadstr(msg);

	/* if this isn't the correct gamename, ignore it */
	gamename = Info_ValueForKey(infoString, "gamename");

	gameMismatch = !*gamename || strcmp(gamename, com_gamename->string) != 0;

	if(gameMismatch){
		comdprintf("Game mismatch in info packet: %s\n", infoString);
		return;
	}

	/* if this isn't the correct protocol version, ignore it */
	prot = atoi(Info_ValueForKey(infoString, "protocol"));

	if(prot != com_protocol->integer
	   ){
		comdprintf("Different protocol info packet: %s\n", infoString);
		return;
	}

	/* iterate servers waiting for ping response */
	for(i=0; i<MAX_PINGREQUESTS; i++)
		if(cl_pinglist[i].adr.port && !cl_pinglist[i].time &&
		   equaladdr(from, cl_pinglist[i].adr)){
			/* calc ping time */
			cl_pinglist[i].time = sysmillisecs() -
					      cl_pinglist[i].start;
			comdprintf("ping time %dms from %s\n",
				cl_pinglist[i].time, addrtostr(
					from));

			/* save of info */
			Q_strncpyz(cl_pinglist[i].info, infoString,
				sizeof(cl_pinglist[i].info));

			/* tack on the net type
			 * NOTE: make sure these types are in sync with the netnames strings in the UI */
			switch(from.type){
			case NA_BROADCAST:
			case NA_IP:
				type = 1;
				break;
			case NA_IP6:
				type = 2;
				break;
			default:
				type = 0;
				break;
			}
			Info_SetValueForKey(cl_pinglist[i].info, "nettype",
				va("%d", type));
			CL_SetServerInfoByAddress(from, infoString,
				cl_pinglist[i].time);

			return;
		}

	/* if not just sent a local broadcast or pinging local servers */
	if(cls.pingUpdateSource != AS_LOCAL)
		return;

	for(i = 0; i < MAX_OTHER_SERVERS; i++){
		/* empty slot */
		if(cls.localServers[i].adr.port == 0)
			break;

		/* avoid duplicate */
		if(equaladdr(from, cls.localServers[i].adr))
			return;
	}

	if(i == MAX_OTHER_SERVERS){
		comdprintf("MAX_OTHER_SERVERS hit, dropping infoResponse\n");
		return;
	}

	/* add this to the list */
	cls.numlocalservers = i+1;
	cls.localServers[i].adr = from;
	cls.localServers[i].clients = 0;
	cls.localServers[i].hostName[0] = '\0';
	cls.localServers[i].mapName[0]	= '\0';
	cls.localServers[i].maxClients	= 0;
	cls.localServers[i].maxPing	= 0;
	cls.localServers[i].minPing	= 0;
	cls.localServers[i].ping = -1;
	cls.localServers[i].game[0]	= '\0';
	cls.localServers[i].gameType	= 0;
	cls.localServers[i].netType	= from.type;
	cls.localServers[i].g_humanplayers = 0;
	cls.localServers[i].g_needpass = 0;

	Q_strncpyz(info, bmreadstr(msg), MAX_INFO_STRING);
	if(strlen(info)){
		if(info[strlen(info)-1] != '\n')
			strncat(info, "\n", sizeof(info) - 1);
		comprintf("%s: %s", addrporttostr(from), info);
	}
}

Servstatus *
CL_GetServerStatus(Netaddr from)
{
	int i, oldest, oldestTime;

	for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		if(equaladdr(from, cl_serverStatusList[i].address))
			return &cl_serverStatusList[i];
	for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		if(cl_serverStatusList[i].retrieved)
			return &cl_serverStatusList[i];
	oldest = -1;
	oldestTime = 0;
	for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		if(oldest == -1 || cl_serverStatusList[i].startTime <
		   oldestTime){
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	if(oldest != -1)
		return &cl_serverStatusList[oldest];
	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount &
				    (MAX_SERVERSTATUSREQUESTS-1)];
}

int
CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen)
{
	int i;
	Netaddr to;
	Servstatus *serverStatus;

	/* if no server address then reset all server status requests */
	if(!serverAddress){
		for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++){
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = qtrue;
		}
		return qfalse;
	}
	/* get the address */
	if(!strtoaddr(serverAddress, &to, NA_UNSPEC))
		return qfalse;
	serverStatus = CL_GetServerStatus(to);
	/* if no server status string then reset the server status request for this address */
	if(!serverStatusString){
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	/* if this server status request has the same address */
	if(equaladdr(to, serverStatus->address)){
		/* if we recieved an response for this server status request */
		if(!serverStatus->pending){
			Q_strncpyz(serverStatusString, serverStatus->string,
				maxLen);
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		/* resend the request regularly */
		else if(serverStatus->startTime < commillisecs() -
			cl_serverStatusResendTime->integer){
			serverStatus->print = qfalse;
			serverStatus->pending	= qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time = 0;
			serverStatus->startTime = commillisecs();
			netprintOOB(NS_CLIENT, to, "getstatus");
			return qfalse;
		}
	}
	/* if retrieved */
	else if(serverStatus->retrieved){
		serverStatus->address	= to;
		serverStatus->print	= qfalse;
		serverStatus->pending	= qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = commillisecs();
		serverStatus->time = 0;
		netprintOOB(NS_CLIENT, to, "getstatus");
		return qfalse;
	}
	return qfalse;
}

void
CL_ServerStatusResponse(Netaddr from, Bitmsg *msg)
{
	char	*s;
	char	info[MAX_INFO_STRING];
	int	i, l, score, ping;
	int	len;
	Servstatus *serverStatus;

	serverStatus = NULL;
	for(i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		if(equaladdr(from, cl_serverStatusList[i].address)){
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	/* if we didn't request this server status */
	if(!serverStatus)
		return;

	s = bmreadstrline(msg);

	len = 0;
	Q_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len,
		"%s",
		s);

	if(serverStatus->print){
		comprintf("Server settings:\n");
		/* print cvars */
		while(*s)
			for(i = 0; i < 2 && *s; i++){
				if(*s == '\\')
					s++;
				l = 0;
				while(*s){
					info[l++] = *s;
					if(l >= MAX_INFO_STRING-1)
						break;
					s++;
					if(*s == '\\')
						break;
				}
				info[l] = '\0';
				if(i)
					comprintf("%s\n", info);
				else
					comprintf("%-24s", info);
			}
	}

	len = strlen(serverStatus->string);
	Q_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len,
		"\\");

	if(serverStatus->print){
		comprintf("\nPlayers:\n");
		comprintf("num: score: ping: name:\n");
	}
	for(i = 0, s = bmreadstrline(msg); *s;
	    s = bmreadstrline(msg), i++){

		len = strlen(serverStatus->string);
		Q_sprintf(&serverStatus->string[len],
			sizeof(serverStatus->string)-len, "\\%s", s);

		if(serverStatus->print){
			score = ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if(s)
				s = strchr(s+1, ' ');
			if(s)
				s++;
			else
				s = "unknown";
			comprintf("%-2d   %-3d    %-3d   %s\n", i, score, ping,
				s);
		}
	}
	len = strlen(serverStatus->string);
	Q_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len,
		"\\");

	serverStatus->time = commillisecs();
	serverStatus->address	= from;
	serverStatus->pending	= qfalse;
	if(serverStatus->print)
		serverStatus->retrieved = qtrue;
}

void
CL_LocalServers_f(void)
{
	char	*message;
	int	i, j;
	Netaddr to;

	comprintf("Scanning for servers on the local network...\n");

	/* reset the list, waiting for response */
	cls.numlocalservers	= 0;
	cls.pingUpdateSource	= AS_LOCAL;

	for(i = 0; i < MAX_OTHER_SERVERS; i++){
		qbool b = cls.localServers[i].visible;
		Q_Memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
		cls.localServers[i].visible = b;
	}
	Q_Memset(&to, 0, sizeof(to));

	/* The 'xxx' in the message is a challenge that will be echoed back
	 * by the server.  We don't care about that here, but master servers
	 * can use that to prevent spoofed server responses from invalid ip */
	message = "\377\377\377\377getinfo xxx";

	/* send each message twice in case one is dropped */
	for(i = 0; i < 2; i++)
		/* send a broadcast packet on each server port
		 * we support multiple server ports so a single machine
		 * can nicely run multiple servers */
		for(j = 0; j < NUM_SERVER_PORTS; j++){
			to.port = BigShort((short)(PORT_SERVER + j));

			to.type = NA_BROADCAST;
			netsendpacket(NS_CLIENT, strlen(message), message, to);
			to.type = NA_MULTICAST6;
			netsendpacket(NS_CLIENT, strlen(message), message, to);
		}
}

void
CL_GlobalServers_f(void)
{
	Netaddr to;
	int	count, i, masterNum;
	char	command[1024], *masteraddress;

	if((count = cmdargc()) < 3 || (masterNum = atoi(cmdargv(1))) < 0 ||
	   masterNum > MAX_MASTER_SERVERS - 1){
		comprintf(
			"usage: globalservers <master# 0-%d> <protocol> [keywords]\n",
			MAX_MASTER_SERVERS - 1);
		return;
	}

	sprintf(command, "sv_master%d", masterNum + 1);
	masteraddress = cvargetstr(command);

	if(!*masteraddress){
		comprintf(
			"CL_GlobalServers_f: Error: No master server address given.\n");
		return;
	}

	/* reset the list, waiting for response
	 * -1 is used to distinguish a "no response" */

	i = strtoaddr(masteraddress, &to, NA_UNSPEC);

	if(!i){
		comprintf(
			"CL_GlobalServers_f: Error: could not resolve address of master %s\n",
			masteraddress);
		return;
	}else if(i == 2)
		to.port = BigShort(PORT_MASTER);

	comprintf("Requesting servers from master %s...\n", masteraddress);

	cls.numglobalservers	= -1;
	cls.pingUpdateSource	= AS_GLOBAL;

	/* Use the extended query for IPv6 masters */
	if(to.type == NA_IP6 || to.type == NA_MULTICAST6){
		int v4enabled = cvargeti("net_enabled") &
				NET_ENABLEV4;

		if(v4enabled)
			Q_sprintf(command, sizeof(command),
				"getserversExt %s %s",
				com_gamename->string, cmdargv(
					2));
		else
			Q_sprintf(command, sizeof(command),
				"getserversExt %s %s ipv6",
				com_gamename->string, cmdargv(
					2));
	}else
		Q_sprintf(command, sizeof(command), "getservers %s %s",
			com_gamename->string, cmdargv(2));

	for(i=3; i < count; i++){
		Q_strcat(command, sizeof(command), " ");
		Q_strcat(command, sizeof(command), cmdargv(i));
	}

	netprintOOB(NS_SERVER, to, "%s", command);
}

void
CL_GetPing(int n, char *buf, int buflen, int *pingtime)
{
	const char *str;
	int	time;
	int	maxPing;

	if(n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port){
		/* empty or invalid slot */
		buf[0] = '\0';
		*pingtime = 0;
		return;
	}

	str = addrporttostr(cl_pinglist[n].adr);
	Q_strncpyz(buf, str, buflen);

	time = cl_pinglist[n].time;
	if(!time){
		/* check for timeout */
		time = sysmillisecs() - cl_pinglist[n].start;
		maxPing = cvargeti("cl_maxPing");
		if(maxPing < 100)
			maxPing = 100;
		if(time < maxPing)
			/* not timed out yet */
			time = 0;
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info,
		cl_pinglist[n].time);

	*pingtime = time;
}

void
CL_GetPingInfo(int n, char *buf, int buflen)
{
	if(n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port){
		/* empty or invalid slot */
		if(buflen)
			buf[0] = '\0';
		return;
	}

	Q_strncpyz(buf, cl_pinglist[n].info, buflen);
}

void
CL_ClearPing(int n)
{
	if(n < 0 || n >= MAX_PINGREQUESTS)
		return;

	cl_pinglist[n].adr.port = 0;
}

int
CL_GetPingQueueCount(void)
{
	int	i;
	int	count;
	Ping * pingptr;

	count	= 0;
	pingptr = cl_pinglist;

	for(i=0; i<MAX_PINGREQUESTS; i++, pingptr++)
		if(pingptr->adr.port)
			count++;

	return (count);
}

Ping*
CL_GetFreePing(void)
{
	Ping	* pingptr;
	Ping  * best;
	int	oldest;
	int	i;
	int	time;

	pingptr = cl_pinglist;
	for(i=0; i<MAX_PINGREQUESTS; i++, pingptr++){
		/* find free ping slot */
		if(pingptr->adr.port){
			if(!pingptr->time){
				if(sysmillisecs() - pingptr->start < 500)
					/* still waiting for response */
					continue;
			}else if(pingptr->time < 500)
				/* results have not been queried */
				continue;
		}

		/* clear it */
		pingptr->adr.port = 0;
		return (pingptr);
	}

	/* use oldest entry */
	pingptr = cl_pinglist;
	best	= cl_pinglist;
	oldest	= INT_MIN;
	for(i=0; i<MAX_PINGREQUESTS; i++, pingptr++){
		/* scan for oldest */
		time = sysmillisecs() - pingptr->start;
		if(time > oldest){
			oldest	= time;
			best	= pingptr;
		}
	}

	return (best);
}

void
CL_Ping_f(void)
{
	Netaddr	to;
	Ping		* pingptr;
	char	* server;
	int	argc;
	Netaddrtype family = NA_UNSPEC;

	argc = cmdargc();

	if(argc != 2 && argc != 3){
		comprintf("usage: ping [-4|-6] server\n");
		return;
	}

	if(argc == 2)
		server = cmdargv(1);
	else{
		if(!strcmp(cmdargv(1), "-4"))
			family = NA_IP;
		else if(!strcmp(cmdargv(1), "-6"))
			family = NA_IP6;
		else
			comprintf(
				"warning: only -4 or -6 as address type understood.\n");

		server = cmdargv(2);
	}

	Q_Memset(&to, 0, sizeof(Netaddr));

	if(!strtoaddr(server, &to, family))
		return;

	pingptr = CL_GetFreePing();

	memcpy(&pingptr->adr, &to, sizeof(Netaddr));
	pingptr->start	= sysmillisecs();
	pingptr->time	= 0;

	CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	netprintOOB(NS_CLIENT, to, "getinfo xxx");
}

qbool
CL_UpdateVisiblePings_f(int source)
{
	int	slots, i;
	char	buff[MAX_STRING_CHARS];
	int	pingTime;
	int	max;
	qbool status = qfalse;

	if(source < 0 || source > AS_FAVORITES)
		return qfalse;

	cls.pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if(slots < MAX_PINGREQUESTS){
		Servinfo *server = NULL;

		switch(source){
		case AS_LOCAL:
			server = &cls.localServers[0];
			max = cls.numlocalservers;
			break;
		case AS_GLOBAL:
			server = &cls.globalServers[0];
			max = cls.numglobalservers;
			break;
		case AS_FAVORITES:
			server = &cls.favoriteServers[0];
			max = cls.numfavoriteservers;
			break;
		default:
			return qfalse;
		}
		for(i = 0; i < max; i++)
			if(server[i].visible){
				if(server[i].ping == -1){
					int j;

					if(slots >= MAX_PINGREQUESTS)
						break;
					for(j = 0; j < MAX_PINGREQUESTS; j++){
						if(!cl_pinglist[j].adr.port)
							continue;
						if(equaladdr(cl_pinglist[j]
							   .adr, server[i].adr))
							/* already on the list */
							break;
					}
					if(j >= MAX_PINGREQUESTS){
						status = qtrue;
						for(j = 0; j < MAX_PINGREQUESTS; j++)
							if(!cl_pinglist[j].adr.port)
								break;
						memcpy(&cl_pinglist[j].adr,
							&server[i].adr,
							sizeof(Netaddr));
						cl_pinglist[j].start = sysmillisecs();
						cl_pinglist[j].time = 0;
						netprintOOB(NS_CLIENT,
							cl_pinglist[j].adr,
							"getinfo xxx");
						slots++;
					}
				}
				/* if the server has a ping higher than cl_maxPing or
				 * the ping packet got lost */
				else if(server[i].ping == 0){
					/* if we are updating global servers */
					if(source == AS_GLOBAL)
						if(cls.numGlobalServerAddresses > 0){
							/* overwrite this server with one from the additional global servers */
							cls.numGlobalServerAddresses--;
							clinitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
							/* NOTE: the server[i].visible flag stays untouched */
						}
				}
			}
	}

	if(slots)
		status = qtrue;
	for(i = 0; i < MAX_PINGREQUESTS; i++){
		if(!cl_pinglist[i].adr.port)
			continue;
		CL_GetPing(i, buff, MAX_STRING_CHARS, &pingTime);
		if(pingTime != 0){
			CL_ClearPing(i);
			status = qtrue;
		}
	}

	return status;
}

void
CL_ServerStatus_f(void)
{
	Netaddr	to, *toptr = NULL;
	char		*server;
	Servstatus	*serverStatus;
	int argc;
	Netaddrtype	family = NA_UNSPEC;

	argc = cmdargc();

	if(argc != 2 && argc != 3){
		if(clc.state != CA_ACTIVE || clc.demoplaying){
			comprintf ("Not connected to a server.\n");
			comprintf("usage: serverstatus [-4|-6] server\n");
			return;
		}

		toptr = &clc.serverAddress;
	}

	if(!toptr){
		Q_Memset(&to, 0, sizeof(Netaddr));

		if(argc == 2)
			server = cmdargv(1);
		else{
			if(!strcmp(cmdargv(1), "-4"))
				family = NA_IP;
			else if(!strcmp(cmdargv(1), "-6"))
				family = NA_IP6;
			else
				comprintf(
					"warning: only -4 or -6 as address type understood.\n");

			server = cmdargv(2);
		}

		toptr = &to;
		if(!strtoaddr(server, toptr, family))
			return;
	}

	netprintOOB(NS_CLIENT, *toptr, "getstatus");

	serverStatus = CL_GetServerStatus(*toptr);
	serverStatus->address	= *toptr;
	serverStatus->print	= qtrue;
	serverStatus->pending	= qtrue;
}

void
CL_ShowIP_f(void)
{
	sysshowIP();
}
