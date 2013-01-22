/* builds an intended movement command to send to the server */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "client.h"
#include "keycodes.h"
#include "keys.h"
#include "ui.h"

/*
 * keybuttons
 *
 * Continuous button event tracking is complicated by the fact that two different
 * input sources (say, mouse button 1 and the control key) can both press the
 * same button, but the button should only be released when both of the
 * pressing keys have been released.
 *
 * When a key event issues a button command (+forward, +attack, etc), it appends
 * its key number as argv(1) so it can be matched up with the release.
 *
 * argv(2) will be set to the time the event happened, which allows exact
 * control even at low framerates when the down and up events may both get qued
 * at the same time.
 */

Cvar *cl_yawspeed;
Cvar *cl_pitchspeed;
Cvar *cl_rollspeed;
Cvar *cl_run;
Cvar *cl_anglespeedkey;

static uint frame_msec;
static int old_com_frameTime;
static Kbutton left, right, forward, back;
static Kbutton lookup, lookdown, moveleft, moveright;
static Kbutton strafe, speed, brake;
static Kbutton up, down;
static Kbutton rollleft, rollright;
#ifdef USE_VOIP
static Kbutton voiprecord;
#endif
static Kbutton buttons[16];
static qbool mlooking;

static void
keydown(Kbutton *b)
{
	int k;
	char *c;

	c = Cmd_Argv(1);
	if(c[0])
		k = atoi(c);
	else
		k = -1;		/* typed manually at the console for continuous down */

	if(k == b->down[0] || k == b->down[1])
		return;		/* repeating key */

	if(!b->down[0])
		b->down[0] = k;
	else if(!b->down[1])
		b->down[1] = k;
	else{
		Com_Printf("Three keys down for a button!\n");
		return;
	}

	if(b->active)
		return;		/* still down */

	/* save timestamp for partial frame summing */
	c = Cmd_Argv(2);
	b->downtime = atoi(c);
	b->active = qtrue;
	b->wasPressed = qtrue;
}

static void
keyup(Kbutton *b)
{
	int k;
	char *c;
	unsigned uptime;

	c = Cmd_Argv(1);
	if(c[0])
		k = atoi(c);
	else{
		/* typed manually at the console, assume for unsticking, so clear all */
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if(b->down[0] == k)
		b->down[0] = 0;
	else if(b->down[1] == k)
		b->down[1] = 0;
	else
		return;		/* key up without coresponding down (menu pass through) */
	if(b->down[0] || b->down[1])
		return;		/* some other key is still holding it down */

	b->active = qfalse;

	/* save timestamp for partial frame summing */
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if(uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += frame_msec / 2;

	b->active = qfalse;
}

/* Returns the fraction of the frame that the key was down */
static float
keystate(Kbutton *key)
{
	float val;
	int msec;

	msec = key->msec;
	key->msec = 0;
	if(key->active){
		/* still down */
		if(!key->downtime)
			msec = com_frameTime;
		else
			msec += com_frameTime - key->downtime;
		key->downtime = com_frameTime;
	}
	
	if(0)
	if(msec)
		Com_Printf("%i ", msec);

	val = (float)msec / frame_msec;
	if(val < 0)
		val = 0;
	if(val > 1)
		val = 1;
	return val;
}

/* Moves the local angle positions */
static void
adjustangles(void)
{
	Scalar spd;
	Scalar ys, ps, rs;	/* yaw, pitch, roll speeds */
	
	ys = cl_yawspeed->value;
	ps = cl_pitchspeed->value;
	rs = cl_rollspeed->value;
	if(speed.active)
		spd = 0.001 * cls.realframetime * cl_anglespeedkey->value;
	else
		spd = 0.001 * cls.realframetime;

	if(!strafe.active){
		cl.viewangles[YAW] -= spd * ys * (keystate(&right) - keystate(&left));
	}
	cl.viewangles[PITCH] -= spd * ps * (keystate(&lookup) - keystate(&lookdown));
	cl.viewangles[ROLL] -= spd * rs * (keystate(&rollright) - keystate(&rollleft));
}

/* Sets the Usrcmd based on key states */
static void
keymove(Usrcmd *cmd)
{
	int mvspeed, fwd, side, _up, brk;

	fwd = 0;
	side = 0;
	_up = 0;
	brk = 0;
	/*
	 * adjust for speed key / running
	 * the walking flag is to keep animations consistant
	 * even during acceleration and deceleration
	 */
	if(speed.active ^ cl_run->integer){
		mvspeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	}else{
		cmd->buttons |= BUTTON_WALKING;
		mvspeed = 64;
	}

	if(strafe.active){
		side += mvspeed * keystate(&right);
		side -= mvspeed * keystate(&left);
	}

	side += mvspeed * keystate(&moveright);
	side -= mvspeed * keystate(&moveleft);
	_up += mvspeed * keystate(&up);
	_up -= mvspeed * keystate(&down);
	fwd += mvspeed * keystate(&forward);
	fwd -= mvspeed * keystate(&back);
	brk = mvspeed * keystate(&brake);
	
	cmd->forwardmove = clampchar(fwd);
	cmd->rightmove = clampchar(side);
	cmd->upmove = clampchar(_up);
	cmd->brakefrac = clampchar(brk);
}

void
CL_MouseEvent(int dx, int dy, int time)
{
	UNUSED(time);
	if(Key_GetCatcher() & KEYCATCH_UI)
		VM_Call(uivm, UI_MOUSE_EVENT, dx, dy);
	else if(Key_GetCatcher() & KEYCATCH_CGAME)
		VM_Call(cgvm, CG_MOUSE_EVENT, dx, dy);
	else{
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/* Joystick values stay set until changed */
void
CL_JoystickEvent(int axis, int value, int time)
{
	UNUSED(time);
	if(axis < 0 || axis >= MAX_JOYSTICK_AXIS)
		Com_Errorf(ERR_DROP, "CL_JoystickEvent: bad axis %i", axis);
	cl.joystickAxis[axis] = value;
}

static void
joystickmove(Usrcmd *cmd)
{
	float anglespeed;

	if(!(speed.active ^ cl_run->integer))
		cmd->buttons |= BUTTON_WALKING;

	if(speed.active)
		anglespeed = 0.001 * cls.realframetime * cl_anglespeedkey->value;
	else
		anglespeed = 0.001 * cls.realframetime;

	if(!strafe.active){
		cl.viewangles[YAW] += anglespeed * j_yaw->value *
				      cl.joystickAxis[j_yaw_axis->integer];
		cmd->rightmove =
			clampchar(cmd->rightmove +
				(int)(j_side->value *
				      cl.joystickAxis[j_side_axis->integer]));
	}else{
		cl.viewangles[YAW] += anglespeed * j_side->value *
				      cl.joystickAxis[j_side_axis->integer];
		cmd->rightmove =
			clampchar(cmd->rightmove +
				(int)(j_yaw->value *
				      cl.joystickAxis[j_yaw_axis->integer]));
	}

	if(mlooking){
		cl.viewangles[PITCH] += anglespeed * j_forward->value *
					cl.joystickAxis[j_forward_axis->integer];
		cmd->forwardmove =
			clampchar(cmd->forwardmove +
				(int)(j_pitch->value *
				      cl.joystickAxis[j_pitch_axis->integer]));
	}else{
		cl.viewangles[PITCH] += anglespeed * j_pitch->value *
					cl.joystickAxis[j_pitch_axis->integer];
		cmd->forwardmove =
			clampchar(cmd->forwardmove +
				(int)(j_forward->value *
				      cl.joystickAxis[j_forward_axis->integer]));
	}
	cmd->upmove = clampchar(cmd->upmove + (int)(j_up->value * 
		cl.joystickAxis[j_up_axis->integer]));
}

/*
 * sensitivity remains pretty much unchanged at low speeds
 * cl_mouseAccel is a power value to how the
 * acceleration is shaped
 *
 * cl_mouseAccelOffset is the
 * rate for which the acceleration will have doubled the
 * non accelerated amplification
 *
 * NOTE: decouple the
 * config cvars for independent acceleration setup along
 * X and Y?
 */
static void
mousemove(Usrcmd *cmd)
{
	float mx, my;

	/* allow mouse smoothing */
	if(m_filter->integer){
		mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5f;
		my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5f;
	}else{
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}

	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	if(cl_mouseAccel->value != 0.0f){
		if(cl_mouseAccelStyle->integer == 0){
			float	accelSensitivity;
			float	rate;

			rate = sqrt(mx * mx + my * my) / (float)frame_msec;

			accelSensitivity = cl_sensitivity->value + rate *
					   cl_mouseAccel->value;
			mx *= accelSensitivity;
			my *= accelSensitivity;

			if(cl_showMouseRate->integer)
				Com_Printf("rate: %f, accelSensitivity: %f\n",
					rate, accelSensitivity);
		}else{
			float	rate[2];
			float	p[2];	/* power */

			rate[0] = fabs(mx) / (float)frame_msec;
			rate[1] = fabs(my) / (float)frame_msec;
			p[0] = powf(rate[0] / cl_mouseAccelOffset->value,
				cl_mouseAccel->value);
			p[1] = powf(rate[1] / cl_mouseAccelOffset->value,
				cl_mouseAccel->value);

			mx = cl_sensitivity->value
				* (mx + ((mx < 0) ? -p[0] : p[0])
				* cl_mouseAccelOffset->value);
			my = cl_sensitivity->value
				* (my + ((my < 0) ? -p[1] : p[1])
				* cl_mouseAccelOffset->value);

			if(cl_showMouseRate->integer)
				Com_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n",
					rate[0], rate[1], p[0], p[1]);
		}
	}else{
		mx *= cl_sensitivity->value;
		my *= cl_sensitivity->value;
	}

	/* ingame FOV */
	mx *= cl.cgameSensitivity;
	my *= cl.cgameSensitivity;

	/* add mouse X/Y movement to cmd */
	if(strafe.active)
		cmd->rightmove = clampchar(cmd->rightmove + m_side->value * mx);
	else
		cl.viewangles[YAW] += m_yaw->value * mx;

	if((mlooking || cl_freelook->integer) && !strafe.active)
		cl.viewangles[PITCH] += m_pitch->value * my;
	else
		cmd->forwardmove = clampchar(
			cmd->forwardmove - m_forward->value * my);
}

static void
cmdbuttons(Usrcmd *cmd)
{
	int i;

	/*
	 * figure button bits
	 * send a button bit even if the key was pressed and released in
	 * less than a frame
	 */
	for(i = 0; i < 15; i++){
		if(buttons[i].active || buttons[i].wasPressed)
			cmd->buttons |= 1 << i;
		buttons[i].wasPressed = qfalse;
	}

	if(Key_GetCatcher())
		cmd->buttons |= BUTTON_TALK;
	/*
	 * allow the game to know if any key at all is
	 * currently pressed, even if it isn't bound to anything
	 */
	if(anykeydown && Key_GetCatcher() == 0)
		cmd->buttons |= BUTTON_ANY;
}

static void
finishmove(Usrcmd *cmd)
{
	int i;

	/* copy the state that the cgame is currently sending */
	cmd->weap[WSpri] = cl.cgameweapsel[WSpri];
	cmd->weap[WSsec] = cl.cgameweapsel[WSsec];
	cmd->weap[WShook] = cl.cgameweapsel[WShook];
	/*
	 * send the current server time so the amount of movement
	 * can be determined without allowing cheating
	 */
	cmd->serverTime = cl.serverTime;
	for(i=0; i<3; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
}

static void
quatify(Vec3 oldangles)
{
	Quat orient, delta, neworient;
	
	eulertoq(cl.viewangles, delta);
	eulertoq(oldangles, orient);
	mulq(orient, delta, neworient);
	qtoeuler(neworient, cl.viewangles);
}

static Usrcmd
createcmd(void)
{
	Usrcmd cmd;
	Vec3 oldangles;

	copyv3(cl.viewangles, oldangles);
	setv3(cl.viewangles, 0.0f, 0.0f, 0.0f);
	adjustangles();	/* keyboard angle adjustment */
	Q_Memset(&cmd, 0, sizeof(cmd));
	cmdbuttons(&cmd);
	keymove(&cmd);
	mousemove(&cmd);
	joystickmove(&cmd);
	
	quatify(oldangles);

	/* store out the final values */
	finishmove(&cmd);

	/* draw debug graphs of turning for mouse testing */
	if(cl_debugMove->integer){
		if(cl_debugMove->integer == 1)
			SCR_DebugGraph(abs(cl.viewangles[YAW] - oldangles[YAW]));
		if(cl_debugMove->integer == 2)
			SCR_DebugGraph(abs(cl.viewangles[PITCH] -
					oldangles[PITCH]));
	}
	return cmd;
}

/* Create a new Usrcmd structure for this frame */
static void
createnewcmds(void)
{
	int cmdnum;

	if(clc.state < CA_PRIMED)
		/* no need to create usercmds until we have a gamestate */
		return;
	frame_msec = com_frameTime - old_com_frameTime;
	/*
	 * if running less than 5fps, truncate the extra time to prevent
	 * unexpected moves after a hitch
	 */
	if(frame_msec > 200)
		frame_msec = 200;
	old_com_frameTime = com_frameTime;

	/* generate a command for this frame */
	cl.cmdNumber++;
	cmdnum = cl.cmdNumber & CMD_MASK;
	cl.cmds[cmdnum] = createcmd();
}

/*
 * Returns qfalse if we are over the maxpackets limit
 * and should choke back the bandwidth a bit by not sending
 * a packet this frame.  All the commands will still get
 * delivered in the next packet, but saving a header and
 * getting more delta compression will reduce total bandwidth.
 */
static qbool
readytosend(void)
{
	int	oldPacketNum;
	int	delta;

	/* don't send anything if playing back a demo */
	if(clc.demoplaying || clc.state == CA_CINEMATIC)
		return qfalse;
	/* If we are downloading, we send no less than 50ms between packets */
	if(*clc.downloadTempName &&
	   cls.simtime - clc.lastPacketSentTime < 50)
		return qfalse;
	/*
	 * if we don't have a valid gamestate yet, only send
	 * one packet a second
	 */
	if(clc.state != CA_ACTIVE && clc.state != CA_PRIMED &&
	   !(*clc.downloadTempName) &&
	   cls.simtime - clc.lastPacketSentTime < 1000)
		return qfalse;
	/* send every frame for loopbacks */
	if(clc.netchan.remoteAddress.type == NA_LOOPBACK)
		return qtrue;
	/* send every frame for LAN */
	if(cl_lanForcePackets->integer &&
	   Sys_IsLANAddress(clc.netchan.remoteAddress))
		return qtrue;
	/* check for exceeding cl_maxpackets */
	if(cl_maxpackets->integer < 15)
		Cvar_Set("cl_maxpackets", "15");
	else if(cl_maxpackets->integer > 125)
		Cvar_Set("cl_maxpackets", "125");
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.simtime -  cl.outPackets[ oldPacketNum ].p_simtime;
	if(delta < 1000 / cl_maxpackets->integer)
		/* the accumulated commands will go out in the next packet */
		return qfalse;
	return qtrue;
}

/*
 * Create and send the command packet to the server
 * Including both the reliable commands and the usercmds
 *
 * During normal gameplay, a client packet will contain something like:
 *
 * 4	sequence number
 * 2	qport
 * 4	serverid
 * 4	acknowledged sequence number
 * 4	clc.serverCommandSequence
 * <optional reliable commands>
 * 1	clc_move or clc_moveNoDelta
 * 1	command count
 * <count * usercmds>
 *
 */
void
CL_WritePacket(void)
{
	Bitmsg buf;
	byte data[MAX_MSGLEN];
	int i, j;
	Usrcmd *cmd, *oldcmd, nullcmd;
	int packetNum, oldPacketNum;
	int count, key;

	if(clc.demoplaying || clc.state == CA_CINEMATIC)
		return;
	Q_Memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);
	/*
	 * write the current serverId so the server
	 * can tell if this is from the current gameState
	 */
	MSG_WriteLong(&buf, cl.serverId);
	/*
	 * write the last message we received, which can
	 * be used for delta compression, and is also used
	 * to tell if we dropped a gamestate
	 */
	MSG_WriteLong(&buf, clc.serverMessageSequence);
	/* write the last reliable message we received */
	MSG_WriteLong(&buf, clc.serverCommandSequence);

	/* write any unacknowledged clientCommands */
	for(i = clc.reliableAcknowledge + 1; i <= clc.reliableSequence; i++){
		MSG_WriteByte(&buf, clc_clientCommand);
		MSG_WriteLong(&buf, i);
		MSG_WriteString(&buf,
			clc.reliableCommands[i & (MAX_RELIABLE_COMMANDS-1)]);
	}

	/*
	 * we want to send all the usercmds that were generated in the last
	 * few packet, so even if a couple packets are dropped in a row,
	 * all the cmds will make it to the server
	 */
	if(cl_packetdup->integer < 0)
		Cvar_Set("cl_packetdup", "0");
	else if(cl_packetdup->integer > 5)
		Cvar_Set("cl_packetdup", "5");
	oldPacketNum = (clc.netchan.outgoingSequence - 1
		- cl_packetdup->integer) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if(count > MAX_PACKET_USERCMDS){
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}

#ifdef USE_VOIP
	if(clc.voipOutgoingDataSize > 0){
		if((clc.voipFlags & VOIP_SPATIAL) ||
		   Com_Isvoiptarget(clc.voipTargets, sizeof(clc.voipTargets),
			   -1)){
			MSG_WriteByte (&buf, clc_voip);
			MSG_WriteByte (&buf, clc.voipOutgoingGeneration);
			MSG_WriteLong (&buf, clc.voipOutgoingSequence);
			MSG_WriteByte (&buf, clc.voipOutgoingDataFrames);
			MSG_WriteData (&buf, clc.voipTargets, sizeof(clc.voipTargets));
			MSG_WriteByte(&buf, clc.voipFlags);
			MSG_WriteShort (&buf, clc.voipOutgoingDataSize);
			MSG_WriteData (&buf, clc.voipOutgoingData, 
				clc.voipOutgoingDataSize);

			/* If we're recording a demo, we have to fake a server packet with
			 *  this VoIP data so it gets to disk; the server doesn't send it
			 *  back to us, and we might as well eliminate concerns about dropped
			 *  and misordered packets here. */
			if(clc.demorecording && !clc.demowaiting){
				const int voipSize = clc.voipOutgoingDataSize;
				Bitmsg fakemsg;
				byte	fakedata[MAX_MSGLEN];
				
				MSG_Init (&fakemsg, fakedata, sizeof(fakedata));
				MSG_Bitstream (&fakemsg);
				MSG_WriteLong (&fakemsg, clc.reliableAcknowledge);
				MSG_WriteByte (&fakemsg, svc_voip);
				MSG_WriteShort (&fakemsg, clc.clientNum);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingGeneration);
				MSG_WriteLong (&fakemsg, clc.voipOutgoingSequence);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingDataFrames);
				MSG_WriteShort (&fakemsg, clc.voipOutgoingDataSize);
				MSG_WriteData (&fakemsg, clc.voipOutgoingData, voipSize);
				MSG_WriteByte (&fakemsg, svc_EOF);
				CL_WriteDemoMessage (&fakemsg, 0);
			}

			clc.voipOutgoingSequence += clc.voipOutgoingDataFrames;
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}else{
			/* We have data, but no targets. Silently discard all data */
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
	}
#endif
	if(count >= 1){
		if(cl_showSend->integer)
			Com_Printf("(%i)", count);
		/* begin a client move command */
		if(cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
		   || clc.serverMessageSequence != cl.snap.messageNum)
			MSG_WriteByte(&buf, clc_moveNoDelta);
		else
			MSG_WriteByte(&buf, clc_move);

		MSG_WriteByte(&buf, count);

		key = clc.checksumFeed;
		key ^= clc.serverMessageSequence;	/* also use the message acknowledge */
		/* also use the last acknowledged server command in the key */
		key ^= MSG_HashKey(clc.serverCommands[clc.serverCommandSequence &
			(MAX_RELIABLE_COMMANDS-1) ], 32);

		/* write all the commands, including the predicted command */
		for(i = 0; i < count; i++){
			j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	/* deliver the message */
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_simtime = cls.simtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.simtime;
	if(cl_showSend->integer)
		Com_Printf("%i ", buf.cursize);
	CL_Netchan_Transmit(&clc.netchan, &buf);
}

/* Called every frame to builds and sends a command packet to the server. */
void
CL_SendCmd(void)
{
	if(clc.state < CA_CONNECTED)
		return;
	if(com_sv_running->integer && sv_paused->integer && cl_paused->integer)
		return;

	createnewcmds();	/* even if a demo is playing */

	/* don't send a packet if the last packet was sent too recently */
	if(!readytosend()){
		if(cl_showSend->integer)
			Com_Printf(". ");
		return;
	}
	CL_WritePacket();
}

static void
UpDown(void)
{
	keydown(&up);
}

static void
UpUp(void)
{
	keyup(&up);
}

static void
DownDown(void)
{
	keydown(&down);
}

static void
DownUp(void)
{
	keyup(&down);
}

static void
LeftDown(void)
{
	keydown(&left);
}

static void
LeftUp(void)
{
	keyup(&left);
}

static void
RightDown(void)
{
	keydown(&right);
}

static void
RightUp(void)
{
	keyup(&right);
}

static void
ForwardDown(void)
{
	keydown(&forward);
}

static void
ForwardUp(void)
{
	keyup(&forward);
}

static void
BackDown(void)
{
	keydown(&back);
}

static void
BackUp(void)
{
	keyup(&back);
}

static void
LookupDown(void)
{
	keydown(&lookup);
}

static void
LookupUp(void)
{
	keyup(&lookup);
}

static void
LookdownDown(void)
{
	keydown(&lookdown);
}

static void
LookdownUp(void)
{
	keyup(&lookdown);
}

static void
MoveleftDown(void)
{
	keydown(&moveleft);
}

static void
MoveleftUp(void)
{
	keyup(&moveleft);
}

static void
MoverightDown(void)
{
	keydown(&moveright);
}

static void
MoverightUp(void)
{
	keyup(&moveright);
}

static void
SpeedDown(void)
{
	keydown(&speed);
}

static void
SpeedUp(void)
{
	keyup(&speed);
}

static void
BrakeDown(void)
{
	keydown(&brake);
}

static void
BrakeUp(void)
{
	keyup(&brake);
}

static void
StrafeDown(void)
{
	keydown(&strafe);
}

static void
StrafeUp(void)
{
	keyup(&strafe);
}

#ifdef USE_VOIP
static void
VoipRecordDown(void)
{
	keydown(&voiprecord);
	Cvar_Set("cl_voipSend", "1");
}

static void
VoipRecordUp(void)
{
	keyup(&voiprecord);
	Cvar_Set("cl_voipSend", "0");
}
#endif

static void
Button0Down(void)
{
	keydown(&buttons[0]);
}

static void
Button0Up(void)
{
	keyup(&buttons[0]);
}

static void
Button1Down(void)
{
	keydown(&buttons[1]);
}

static void
Button1Up(void)
{
	keyup(&buttons[1]);
}

static void
Button2Down(void)
{
	keydown(&buttons[2]);
}

static void
Button2Up(void)
{
	keyup(&buttons[2]);
}

static void
Button3Down(void)
{
	keydown(&buttons[3]);
}

static void
Button3Up(void)
{
	keyup(&buttons[3]);
}

static void
Button4Down(void)
{
	keydown(&buttons[4]);
}

static void
Button4Up(void)
{
	keyup(&buttons[4]);
}

static void
Button5Down(void)
{
	keydown(&buttons[5]);
}

static void
Button5Up(void)
{
	keyup(&buttons[5]);
}

static void
Button6Down(void)
{
	keydown(&buttons[6]);
}

static void
Button6Up(void)
{
	keyup(&buttons[6]);
}

static void
Button7Down(void)
{
	keydown(&buttons[7]);
}

static void
Button7Up(void)
{
	keyup(&buttons[7]);
}

static void
Button8Down(void)
{
	keydown(&buttons[8]);
}

static void
Button8Up(void)
{
	keyup(&buttons[8]);
}

static void
Button9Down(void)
{
	keydown(&buttons[9]);
}

static void
Button9Up(void)
{
	keyup(&buttons[9]);
}

static void
Button10Down(void)
{
	keydown(&buttons[10]);
}

static void
Button10Up(void)
{
	keyup(&buttons[10]);
}

static void
Button11Down(void)
{
	keydown(&buttons[11]);
}

static void
Button11Up(void)
{
	keyup(&buttons[11]);
}

static void
Button12Down(void)
{
	keydown(&buttons[12]);
}

static void
Button12Up(void)
{
	keyup(&buttons[12]);
}

static void
Button13Down(void)
{
	keydown(&buttons[13]);
}

static void
Button13Up(void)
{
	keyup(&buttons[13]);
}

static void
Button14Down(void)
{
	keydown(&buttons[14]);
}

static void
Button14Up(void)
{
	keyup(&buttons[14]);
}

static void
Button15Down(void)
{
	keydown(&buttons[15]);
}

static void
Button15Up(void)
{
	keyup(&buttons[15]);
}

static void
RollLeftDown(void)
{
	keydown(&rollleft);
}

static void
RollLeftUp(void)
{
	keyup(&rollleft);
}

static void
RollRightDown(void)
{
	keydown(&rollright);
}

static void
RollRightUp(void)
{
	keyup(&rollright);
}

static void
centerview(void)
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snap.ps.delta_angles[PITCH]);
}

static void
mlookdown(void)
{
	mlooking = qtrue;
}

static void
mlookup(void)
{
	mlooking = qfalse;
	if(!cl_freelook->integer)
		centerview();
}

void
CL_InitInput(void)
{
	Cmd_AddCommand("centerview", centerview);
	Cmd_AddCommand("+moveup", UpDown);
	Cmd_AddCommand("-moveup", UpUp);
	Cmd_AddCommand("+movedown", DownDown);
	Cmd_AddCommand("-movedown", DownUp);
	Cmd_AddCommand("+left", LeftDown);
	Cmd_AddCommand("-left", LeftUp);
	Cmd_AddCommand("+right", RightDown);
	Cmd_AddCommand("-right", RightUp);
	Cmd_AddCommand("+forward", ForwardDown);
	Cmd_AddCommand("-forward", ForwardUp);
	Cmd_AddCommand("+back", BackDown);
	Cmd_AddCommand("-back", BackUp);
	Cmd_AddCommand("+lookup", LookupDown);
	Cmd_AddCommand("-lookup", LookupUp);
	Cmd_AddCommand("+lookdown", LookdownDown);
	Cmd_AddCommand("-lookdown", LookdownUp);
	Cmd_AddCommand("+strafe", StrafeDown);
	Cmd_AddCommand("-strafe", StrafeUp);
	Cmd_AddCommand("+moveleft", MoveleftDown);
	Cmd_AddCommand("-moveleft", MoveleftUp);
	Cmd_AddCommand("+moveright", MoverightDown);
	Cmd_AddCommand("-moveright", MoverightUp);
	Cmd_AddCommand("+speed", SpeedDown);
	Cmd_AddCommand("-speed", SpeedUp);
	Cmd_AddCommand("+brake", BrakeDown);
	Cmd_AddCommand("-brake", BrakeUp);
	Cmd_AddCommand("+attack", Button0Down);
	Cmd_AddCommand("-attack", Button0Up);
	Cmd_AddCommand("+attack2", Button1Down);
	Cmd_AddCommand("-attack2", Button1Up);
	Cmd_AddCommand("+hook", Button2Down);
	Cmd_AddCommand("-hook",	Button2Up);
	Cmd_AddCommand("+button0", Button0Down);
	Cmd_AddCommand("-button0", Button0Up);
	Cmd_AddCommand("+button1", Button1Down);
	Cmd_AddCommand("-button1", Button1Up);
	Cmd_AddCommand("+button2", Button2Down);
	Cmd_AddCommand("-button2", Button2Up);
	Cmd_AddCommand("+button3", Button3Down);
	Cmd_AddCommand("-button3", Button3Up);
	Cmd_AddCommand("+button4", Button4Down);
	Cmd_AddCommand("-button4", Button4Up);
	Cmd_AddCommand("+button5", Button5Down);
	Cmd_AddCommand("-button5", Button5Up);
	Cmd_AddCommand("+button6", Button6Down);
	Cmd_AddCommand("-button6", Button6Up);
	Cmd_AddCommand("+button7", Button7Down);
	Cmd_AddCommand("-button7", Button7Up);
	Cmd_AddCommand("+button8", Button8Down);
	Cmd_AddCommand("-button8", Button8Up);
	Cmd_AddCommand("+button9", Button9Down);
	Cmd_AddCommand("-button9", Button9Up);
	Cmd_AddCommand("+button10", Button10Down);
	Cmd_AddCommand("-button10", Button10Up);
	Cmd_AddCommand("+button11", Button11Down);
	Cmd_AddCommand("-button11", Button11Up);
	Cmd_AddCommand("+button12", Button12Down);
	Cmd_AddCommand("-button12", Button12Up);
	Cmd_AddCommand("+button13", Button13Down);
	Cmd_AddCommand("-button13", Button13Up);
	Cmd_AddCommand("+button14", Button14Down);
	Cmd_AddCommand("-button14", Button14Up);
	Cmd_AddCommand("+button15",Button15Down);
	Cmd_AddCommand("-button15",Button15Up);
	Cmd_AddCommand("+mlook", mlookdown);
	Cmd_AddCommand("-mlook", mlookup);
	Cmd_AddCommand("+rollleft", RollLeftDown);
	Cmd_AddCommand("-rollleft", RollLeftUp);
	Cmd_AddCommand("+rollright", RollRightDown);
	Cmd_AddCommand("-rollright", RollRightUp);
#ifdef USE_VOIP
	Cmd_AddCommand("+voiprecord", VoipRecordDown);
	Cmd_AddCommand("-voiprecord", VoipRecordUp);
#endif
	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
}

void
CL_ShutdownInput(void)
{
	Cmd_RemoveCommand("centerview");
	Cmd_RemoveCommand("+moveup");
	Cmd_RemoveCommand("-moveup");
	Cmd_RemoveCommand("+movedown");
	Cmd_RemoveCommand("-movedown");
	Cmd_RemoveCommand("+left");
	Cmd_RemoveCommand("-left");
	Cmd_RemoveCommand("+right");
	Cmd_RemoveCommand("-right");
	Cmd_RemoveCommand("+forward");
	Cmd_RemoveCommand("-forward");
	Cmd_RemoveCommand("+back");
	Cmd_RemoveCommand("-back");
	Cmd_RemoveCommand("+lookup");
	Cmd_RemoveCommand("-lookup");
	Cmd_RemoveCommand("+lookdown");
	Cmd_RemoveCommand("-lookdown");
	Cmd_RemoveCommand("+strafe");
	Cmd_RemoveCommand("-strafe");
	Cmd_RemoveCommand("+moveleft");
	Cmd_RemoveCommand("-moveleft");
	Cmd_RemoveCommand("+moveright");
	Cmd_RemoveCommand("-moveright");
	Cmd_RemoveCommand("+speed");
	Cmd_RemoveCommand("-speed");
	Cmd_RemoveCommand("+brake");
	Cmd_RemoveCommand("-brake");
	Cmd_RemoveCommand("+attack");
	Cmd_RemoveCommand("-attack");
	Cmd_RemoveCommand("+attack2");
	Cmd_RemoveCommand("-attack2");
	Cmd_RemoveCommand("+hook");
	Cmd_RemoveCommand("-hook");
	Cmd_RemoveCommand("+button0");
	Cmd_RemoveCommand("-button0");
	Cmd_RemoveCommand("+button1");
	Cmd_RemoveCommand("-button1");
	Cmd_RemoveCommand("+button2");
	Cmd_RemoveCommand("-button2");
	Cmd_RemoveCommand("+button3");
	Cmd_RemoveCommand("-button3");
	Cmd_RemoveCommand("+button4");
	Cmd_RemoveCommand("-button4");
	Cmd_RemoveCommand("+button5");
	Cmd_RemoveCommand("-button5");
	Cmd_RemoveCommand("+button6");
	Cmd_RemoveCommand("-button6");
	Cmd_RemoveCommand("+button7");
	Cmd_RemoveCommand("-button7");
	Cmd_RemoveCommand("+button8");
	Cmd_RemoveCommand("-button8");
	Cmd_RemoveCommand("+button9");
	Cmd_RemoveCommand("-button9");
	Cmd_RemoveCommand("+button10");
	Cmd_RemoveCommand("-button10");
	Cmd_RemoveCommand("+button11");
	Cmd_RemoveCommand("-button11");
	Cmd_RemoveCommand("+button12");
	Cmd_RemoveCommand("-button12");
	Cmd_RemoveCommand("+button13");
	Cmd_RemoveCommand("-button13");
	Cmd_RemoveCommand("+button14");
	Cmd_RemoveCommand("-button14");
	Cmd_RemoveCommand("+button15");
	Cmd_RemoveCommand("-button15");
	Cmd_RemoveCommand("+mlook");
	Cmd_RemoveCommand("-mlook");
	Cmd_RemoveCommand("+rollleft");
	Cmd_RemoveCommand("-rollleft");
	Cmd_RemoveCommand("+rollright");
	Cmd_RemoveCommand("-rollright");
#ifdef USE_VOIP
	Cmd_RemoveCommand("+voiprecord");
	Cmd_RemoveCommand("-voiprecord");
#endif
}
