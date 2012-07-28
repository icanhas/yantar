/* builds an intended movement command to send to the server */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "client.h"

static unsigned frame_msec;
static int old_com_frameTime;

/*
 * KEY BUTTONS
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

static kbutton_t	left, right, forward, back;
static kbutton_t	lookup, lookdown, moveleft, moveright;
static kbutton_t	strafe, speed, brake;
static kbutton_t	up, down;
static kbutton_t	rollleft, rollright;
#ifdef USE_VOIP
kbutton_t voiprecord;
#endif
kbutton_t buttons[16];
qbool mlooking;

static void
keydown(kbutton_t *b)
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
		Q_Printf("Three keys down for a button!\n");
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
keyup(kbutton_t *b)
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
keystate(kbutton_t *key)
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
		Q_Printf("%i ", msec);

	val = (float)msec / frame_msec;
	if(val < 0)
		val = 0;
	if(val > 1)
		val = 1;
	return val;
}

static void
IN_UpDown(void)
{
	keydown(&up);
}

static void
IN_UpUp(void)
{
	keyup(&up);
}

static void
IN_DownDown(void)
{
	keydown(&down);
}

static void
IN_DownUp(void)
{
	keyup(&down);
}

static void
IN_LeftDown(void)
{
	keydown(&left);
}

static void
IN_LeftUp(void)
{
	keyup(&left);
}

static void
IN_RightDown(void)
{
	keydown(&right);
}

static void
IN_RightUp(void)
{
	keyup(&right);
}

static void
IN_ForwardDown(void)
{
	keydown(&forward);
}

static void
IN_ForwardUp(void)
{
	keyup(&forward);
}

static void
IN_BackDown(void)
{
	keydown(&back);
}

static void
IN_BackUp(void)
{
	keyup(&back);
}

static void
IN_LookupDown(void)
{
	keydown(&lookup);
}

static void
IN_LookupUp(void)
{
	keyup(&lookup);
}

static void
IN_LookdownDown(void)
{
	keydown(&lookdown);
}

static void
IN_LookdownUp(void)
{
	keyup(&lookdown);
}

static void
IN_MoveleftDown(void)
{
	keydown(&moveleft);
}

static void
IN_MoveleftUp(void)
{
	keyup(&moveleft);
}

static void
IN_MoverightDown(void)
{
	keydown(&moveright);
}

static void
IN_MoverightUp(void)
{
	keyup(&moveright);
}

static void
IN_SpeedDown(void)
{
	keydown(&speed);
}

static void
IN_SpeedUp(void)
{
	keyup(&speed);
}

static void
IN_BrakeDown(void)
{
	keydown(&brake);
}

static void
IN_BrakeUp(void)
{
	keyup(&brake);
}

static void
IN_StrafeDown(void)
{
	keydown(&strafe);
}

static void
IN_StrafeUp(void)
{
	keyup(&strafe);
}

#ifdef USE_VOIP
static void
IN_VoipRecordDown(void)
{
	keydown(&voiprecord);
	Cvar_Set("cl_voipSend", "1");
}

static void
IN_VoipRecordUp(void)
{
	keyup(&voiprecord);
	Cvar_Set("cl_voipSend", "0");
}
#endif

static void
IN_Button0Down(void)
{
	keydown(&buttons[0]);
}

static void
IN_Button0Up(void)
{
	keyup(&buttons[0]);
}

static void
IN_Button1Down(void)
{
	keydown(&buttons[1]);
}

static void
IN_Button1Up(void)
{
	keyup(&buttons[1]);
}

static void
IN_Button2Down(void)
{
	keydown(&buttons[2]);
}

static void
IN_Button2Up(void)
{
	keyup(&buttons[2]);
}

static void
IN_Button3Down(void)
{
	keydown(&buttons[3]);
}

static void
IN_Button3Up(void)
{
	keyup(&buttons[3]);
}

static void
IN_Button4Down(void)
{
	keydown(&buttons[4]);
}

static void
IN_Button4Up(void)
{
	keyup(&buttons[4]);
}

static void
IN_Button5Down(void)
{
	keydown(&buttons[5]);
}

static void
IN_Button5Up(void)
{
	keyup(&buttons[5]);
}

static void
IN_Button6Down(void)
{
	keydown(&buttons[6]);
}

static void
IN_Button6Up(void)
{
	keyup(&buttons[6]);
}

static void
IN_Button7Down(void)
{
	keydown(&buttons[7]);
}

static void
IN_Button7Up(void)
{
	keyup(&buttons[7]);
}

static void
IN_Button8Down(void)
{
	keydown(&buttons[8]);
}

static void
IN_Button8Up(void)
{
	keyup(&buttons[8]);
}

static void
IN_Button9Down(void)
{
	keydown(&buttons[9]);
}

static void
IN_Button9Up(void)
{
	keyup(&buttons[9]);
}

static void
IN_Button10Down(void)
{
	keydown(&buttons[10]);
}

static void
IN_Button10Up(void)
{
	keyup(&buttons[10]);
}

static void
IN_Button11Down(void)
{
	keydown(&buttons[11]);
}

static void
IN_Button11Up(void)
{
	keyup(&buttons[11]);
}

static void
IN_Button12Down(void)
{
	keydown(&buttons[12]);
}

static void
IN_Button12Up(void)
{
	keyup(&buttons[12]);
}

static void
IN_Button13Down(void)
{
	keydown(&buttons[13]);
}

static void
IN_Button13Up(void)
{
	keyup(&buttons[13]);
}

static void
IN_Button14Down(void)
{
	keydown(&buttons[14]);
}

static void
IN_Button14Up(void)
{
	keyup(&buttons[14]);
}

static void
IN_Button15Down(void)
{
	keydown(&buttons[15]);
}

static void
IN_Button15Up(void)
{
	keyup(&buttons[15]);
}

static void
IN_RollLeftDown(void)
{
	keydown(&rollleft);
}

static void
IN_RollLeftUp(void)
{
	keyup(&rollleft);
}

static void
IN_RollRightDown(void)
{
	keydown(&rollright);
}

static void
IN_RollRightUp(void)
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

cvar_t *cl_yawspeed;
cvar_t *cl_pitchspeed;
cvar_t *cl_rollspeed;
cvar_t *cl_run;
cvar_t *cl_anglespeedkey;

/* Moves the local angle positions */
static void
adjustangles(void)
{
	float spd;
	float ys, ps, rs;	/* yaw, pitch, roll speeds */
	
	ys = cl_yawspeed->value;
	ps = cl_pitchspeed->value;
	rs = cl_rollspeed->value;
	if(speed.active)
		spd = 0.001 * cls.frametime * cl_anglespeedkey->value;
	else
		spd = 0.001 * cls.frametime;

	if(!strafe.active){
		cl.viewangles[YAW] -= spd * ys * keystate(&right);
		cl.viewangles[YAW] += spd * ys * keystate(&left);
	}

	cl.viewangles[PITCH] -= spd * ps * keystate(&lookup);
	cl.viewangles[PITCH] += spd * ps * keystate(&lookdown);
	
	/* FIXME: roll mustn't be so immediate */
	cl.viewangles[ROLL] -= spd * rs * keystate(&rollleft);		
	cl.viewangles[ROLL] += spd * rs * keystate(&rollright);	
}

/* Sets the usercmd_t based on key states */
static void
keymove(usercmd_t *cmd)
{
	int	mvspeed;
	int	fwd, side, _up;

	fwd = 0;
	side = 0;
	_up = 0;

	/*
	 * adjust for speed key / running
	 * the walking flag is to keep animations consistant
	 * even during acceleration and develeration
	 */
	if(speed.active ^ cl_run->integer){
		mvspeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	}else{
		cmd->buttons |= BUTTON_WALKING;
		mvspeed = 64;
	}

	if(strafe.active){
		side += mvspeed * keystate (&right);
		side -= mvspeed * keystate (&left);
	}

	side += mvspeed * keystate(&moveright);
	side -= mvspeed * keystate(&moveleft);
	_up += mvspeed * keystate(&up);
	_up -= mvspeed * keystate(&down);
	fwd += mvspeed * keystate(&forward);
	fwd -= mvspeed * keystate(&back);
	cmd->forwardmove = ClampChar(fwd);
	cmd->rightmove = ClampChar(side);
	cmd->upmove = ClampChar(_up);
}

void
CL_MouseEvent(int dx, int dy, int time)
{
	UNUSED(time);
	if(Key_GetCatcher( ) & KEYCATCH_UI)
		VM_Call(uivm, UI_MOUSE_EVENT, dx, dy);
	else if(Key_GetCatcher( ) & KEYCATCH_CGAME)
		VM_Call (cgvm, CG_MOUSE_EVENT, dx, dy);
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
		Q_Error(ERR_DROP, "CL_JoystickEvent: bad axis %i", axis);
	cl.joystickAxis[axis] = value;
}

static void
joystickmove(usercmd_t *cmd)
{
	float anglespeed;

	if(!(speed.active ^ cl_run->integer))
		cmd->buttons |= BUTTON_WALKING;

	if(speed.active)
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	else
		anglespeed = 0.001 * cls.frametime;

	if(!strafe.active){
		cl.viewangles[YAW] += anglespeed * j_yaw->value *
				      cl.joystickAxis[j_yaw_axis->integer];
		cmd->rightmove =
			ClampChar(cmd->rightmove +
				(int)(j_side->value *
				      cl.joystickAxis[j_side_axis->integer]));
	}else{
		cl.viewangles[YAW] += anglespeed * j_side->value *
				      cl.joystickAxis[j_side_axis->integer];
		cmd->rightmove =
			ClampChar(cmd->rightmove +
				(int)(j_yaw->value *
				      cl.joystickAxis[j_yaw_axis->integer]));
	}

	if(mlooking){
		cl.viewangles[PITCH] += anglespeed * j_forward->value *
					cl.joystickAxis[j_forward_axis->integer];
		cmd->forwardmove =
			ClampChar(cmd->forwardmove +
				(int)(j_pitch->value *
				      cl.joystickAxis[j_pitch_axis->integer]));
	}else{
		cl.viewangles[PITCH] += anglespeed * j_pitch->value *
					cl.joystickAxis[j_pitch_axis->integer];
		cmd->forwardmove =
			ClampChar(cmd->forwardmove +
				(int)(j_forward->value *
				      cl.joystickAxis[j_forward_axis->integer]));
	}
	cmd->upmove = ClampChar(cmd->upmove + (int)(j_up->value * 
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
mousemove(usercmd_t *cmd)
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

	if(mx == 0.0f && my == 0.0f)
		return;

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
				Q_Printf("rate: %f, accelSensitivity: %f\n",
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
				Q_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n",
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
		cmd->rightmove = ClampChar(cmd->rightmove + m_side->value * mx);
	else
		cl.viewangles[YAW] -= m_yaw->value * mx;

	if((mlooking || cl_freelook->integer) && !strafe.active)
		cl.viewangles[PITCH] += m_pitch->value * my;
	else
		cmd->forwardmove = ClampChar(
			cmd->forwardmove - m_forward->value * my);
}

static void
cmdbuttons(usercmd_t *cmd)
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
finishmove(usercmd_t *cmd)
{
	uint i;

	/* copy the state that the cgame is currently sending */
	cmd->weapon = cl.cgameUserCmdValue;
	/*
	 * send the current server time so the amount of movement
	 * can be determined without allowing cheating
	 */
	cmd->serverTime = cl.serverTime;
	for(i=0; i<3; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
}

static usercmd_t
createcmd(void)
{
	usercmd_t cmd;
	vec3_t oldangles;

	Vec3Copy(cl.viewangles, oldangles);
	adjustangles();	/* keyboard angle adjustment */
	Q_Memset(&cmd, 0, sizeof(cmd));
	cmdbuttons(&cmd);
	keymove(&cmd);
	mousemove(&cmd);
	joystickmove(&cmd);

	/* check to make sure the angles haven't wrapped */
	if(cl.viewangles[PITCH] - oldangles[PITCH] > 90)
		cl.viewangles[PITCH] = oldangles[PITCH] + 90;
	else if(oldangles[PITCH] - cl.viewangles[PITCH] > 90)
		cl.viewangles[PITCH] = oldangles[PITCH] - 90;

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

/* Create a new usercmd_t structure for this frame */
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
	   cls.realtime - clc.lastPacketSentTime < 50)
		return qfalse;
	/*
	 * if we don't have a valid gamestate yet, only send
	 * one packet a second
	 */
	if(clc.state != CA_ACTIVE && clc.state != CA_PRIMED &&
	   !(*clc.downloadTempName) &&
	   cls.realtime - clc.lastPacketSentTime < 1000)
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
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
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
	msg_t buf;
	byte data[MAX_MSGLEN];
	int i, j;
	usercmd_t *cmd, *oldcmd, nullcmd;
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
		Q_Printf("MAX_PACKET_USERCMDS\n");
	}

#ifdef USE_VOIP
	if(clc.voipOutgoingDataSize > 0){
		if((clc.voipFlags & VOIP_SPATIAL) ||
		   Q_IsVoipTarget(clc.voipTargets, sizeof(clc.voipTargets),
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
				msg_t fakemsg;
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
			Q_Printf("(%i)", count);
		/* begin a client move command */
		if(cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
		   || clc.serverMessageSequence != cl.snap.messageNum)
			MSG_WriteByte (&buf, clc_moveNoDelta);
		else
			MSG_WriteByte (&buf, clc_move);

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
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;
	if(cl_showSend->integer)
		Q_Printf("%i ", buf.cursize);
	CL_Netchan_Transmit (&clc.netchan, &buf);
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
			Q_Printf(". ");
		return;
	}
	CL_WritePacket();
}

void
CL_InitInput(void)
{
	Cmd_AddCommand ("centerview",centerview);
	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand("+brake", IN_BrakeDown);
	Cmd_AddCommand("-brake", IN_BrakeUp);
	Cmd_AddCommand ("+attack", IN_Button0Down);
	Cmd_AddCommand ("-attack", IN_Button0Up);
	Cmd_AddCommand ("+button0", IN_Button0Down);
	Cmd_AddCommand ("-button0", IN_Button0Up);
	Cmd_AddCommand ("+button1", IN_Button1Down);
	Cmd_AddCommand ("-button1", IN_Button1Up);
	Cmd_AddCommand ("+button2", IN_Button2Down);
	Cmd_AddCommand ("-button2", IN_Button2Up);
	Cmd_AddCommand ("+button3", IN_Button3Down);
	Cmd_AddCommand ("-button3", IN_Button3Up);
	Cmd_AddCommand ("+button4", IN_Button4Down);
	Cmd_AddCommand ("-button4", IN_Button4Up);
	Cmd_AddCommand ("+button5", IN_Button5Down);
	Cmd_AddCommand ("-button5", IN_Button5Up);
	Cmd_AddCommand ("+button6", IN_Button6Down);
	Cmd_AddCommand ("-button6", IN_Button6Up);
	Cmd_AddCommand ("+button7", IN_Button7Down);
	Cmd_AddCommand ("-button7", IN_Button7Up);
	Cmd_AddCommand ("+button8", IN_Button8Down);
	Cmd_AddCommand ("-button8", IN_Button8Up);
	Cmd_AddCommand ("+button9", IN_Button9Down);
	Cmd_AddCommand ("-button9", IN_Button9Up);
	Cmd_AddCommand ("+button10", IN_Button10Down);
	Cmd_AddCommand ("-button10", IN_Button10Up);
	Cmd_AddCommand ("+button11", IN_Button11Down);
	Cmd_AddCommand ("-button11", IN_Button11Up);
	Cmd_AddCommand ("+button12", IN_Button12Down);
	Cmd_AddCommand ("-button12", IN_Button12Up);
	Cmd_AddCommand ("+button13", IN_Button13Down);
	Cmd_AddCommand ("-button13", IN_Button13Up);
	Cmd_AddCommand ("+button14", IN_Button14Down);
	Cmd_AddCommand ("-button14", IN_Button14Up);
	Cmd_AddCommand("+button15", IN_Button15Down);
	Cmd_AddCommand("-button15", IN_Button15Up);
	Cmd_AddCommand ("+mlook", mlookdown);
	Cmd_AddCommand ("-mlook", mlookup);
	Cmd_AddCommand ("+rollleft", IN_RollLeftDown);
	Cmd_AddCommand ("-rollleft", IN_RollLeftUp);
	Cmd_AddCommand ("+rollright", IN_RollRightDown);
	Cmd_AddCommand ("-rollright", IN_RollRightUp);
#ifdef USE_VOIP
	Cmd_AddCommand ("+voiprecord", IN_VoipRecordDown);
	Cmd_AddCommand ("-voiprecord", IN_VoipRecordUp);
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
