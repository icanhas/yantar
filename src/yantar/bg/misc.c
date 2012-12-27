/* both games' misc functions, all completely stateless */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "bg.h"

void
BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, Vec3 result)
{
	float	deltaTime;
	float	phase;

	switch(tr->type){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		copyv3(tr->base, result);
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->time) * 0.001;	/* milliseconds to seconds */
		maddv3(tr->base, deltaTime, tr->delta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->time) / (float)tr->duration;
		phase = sin(deltaTime * M_PI * 2);
		maddv3(tr->base, phase, tr->delta, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->time + tr->duration)
			atTime = tr->time + tr->duration;
		deltaTime = (atTime - tr->time) * 0.001;	/* milliseconds to seconds */
		if(deltaTime < 0)
			deltaTime = 0;
		maddv3(tr->base, deltaTime, tr->delta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->time) * 0.001;	/* milliseconds to seconds */
		maddv3(tr->base, deltaTime, tr->delta, result);
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;	/* FIXME: local gravity... */
		break;
	default:
		Com_Errorf(ERR_DROP, "BG_EvaluateTrajectory: unknown type: %i",
			tr->time);
		break;
	}
}

/*
 * For determining velocity at a given time
 */
void
BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, Vec3 result)
{
	float	deltaTime;
	float	phase;

	switch(tr->type){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		clearv3(result);
		break;
	case TR_LINEAR:
		copyv3(tr->delta, result);
		break;
	case TR_SINE:
		deltaTime	= (atTime - tr->time) / (float)tr->duration;
		phase		= cos(deltaTime * M_PI * 2);	/* derivative of sin = cos */
		phase		*= 0.5;
		scalev3(tr->delta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->time + tr->duration){
			clearv3(result);
			return;
		}
		copyv3(tr->delta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->time) * 0.001;	/* milliseconds to seconds */
		copyv3(tr->delta, result);
		result[2] -= DEFAULT_GRAVITY * deltaTime;	/* FIXME: local gravity... */
		break;
	default:
		Com_Errorf(ERR_DROP,
			"BG_EvaluateTrajectoryDelta: unknown type: %i",
			tr->time);
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",	/* boing sound at origin", jump sound on player */

	"EV_JUMP",
	"EV_WATER_TOUCH",	/* foot touches */
	"EV_WATER_LEAVE",	/* foot leaves */
	"EV_WATER_UNDER",	/* head touches */
	"EV_WATER_CLEAR",	/* head leaves */

	"EV_ITEM_PICKUP",		/* normal item pickups are predictable */
	"EV_GLOBAL_ITEM_PICKUP",	/* powerup / team sounds are broadcast to everyone */

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRESECWEAP",
	"EV_FIREHOOK",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",	/* eventParm will be the soundindex */

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",	/* no attenuation */
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",	/* otherEntity is the shooter */

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",	/* gib a previously living player */
	"EV_SCOREPLUM",		/* score plum */

	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL"
};

/*
 * Handles the sequence numbers
 */

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

void
BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps)
{
#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if(atof(buf) != 0){
#ifdef QAGAME
			Com_Printf(
				" game event svt %5d -> %5d: num = %20s parm %d\n",
				ps->pmove_framecount /*ps->commandTime*/,
				ps->eventSequence, eventnames[newEvent],
				eventParm);
#else
			Com_Printf(
				"Cgame event svt %5d -> %5d: num = %20s parm %d\n",
				ps->pmove_framecount /*ps->commandTime*/,
				ps->eventSequence, eventnames[newEvent],
				eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

void
BG_TouchJumpPad(playerState_t *ps, entityState_t *jumppad)
{
	Vec3	angles;
	float	p;
	int	effectNum;

	/* spectators don't use jump pads */
	if(ps->pm_type != PM_NORMAL)
		return;

	/* flying characters don't hit bounce pads */
	if(ps->powerups[PW_FLIGHT])
		return;

	/* 
	 * if we didn't hit this same jumppad the previous frame
	 * then don't play the event sound again if we are in a fat trigger 
	 */
	if(ps->jumppad_ent != jumppad->number){

		v3toeuler(jumppad->origin2, angles);
		p = fabs(norm180euler(angles[PITCH]));
		if(p < 45)
			effectNum = 0;
		else
			effectNum = 1;
		BG_AddPredictableEventToPlayerstate(EV_JUMP_PAD, effectNum, ps);
	}
	/* remember hitting this jumppad this frame */
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	/* give the player the velocity from the jumppad */
	copyv3(jumppad->origin2, ps->velocity);
}

/*
 * This is done after each set of usercmd_t on the server,
 * and after local prediction on the client
 */
void
BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qbool snap)
{
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR)
		s->eType = ET_INVISIBLE;
	else if(ps->stats[STAT_HEALTH] <= GIB_HEALTH)
		s->eType = ET_INVISIBLE;
	else
		s->eType = ET_PLAYER;

	s->number = ps->clientNum;

	s->traj.type = TR_INTERPOLATE;
	copyv3(ps->origin, s->traj.base);
	if(snap)
		snapv3(s->traj.base);
	/* set the delta for flag direction */
	copyv3(ps->velocity, s->traj.delta);

	s->apos.type = TR_INTERPOLATE;
	copyv3(ps->viewangles, s->apos.base);
	if(snap)
		snapv3(s->apos.base);

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim	= ps->legsAnim;
	s->torsoAnim	= ps->torsoAnim;
	s->clientNum	= ps->clientNum;	/* ET_PLAYER looks here instead of at number */
	/* so corpses can also reference the proper config */
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0)
		s->eFlags |= EF_DEAD;
	else
		s->eFlags &= ~EF_DEAD;

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS)
			ps->entityEventSequence = ps->eventSequence -
						  MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] |
			   ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weap[Wpri] = ps->weap[Wpri];
	s->weap[Wsec] = ps->weap[Wsec];
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ps->powerups[ i ])
			s->powerups |= 1 << i;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
 * This is done after each set of usercmd_t on the server,
 * and after local prediction on the client
 */
void
BG_PlayerStateToEntityStateExtraPolate(playerState_t *ps, entityState_t *s, int time, qbool snap)
{
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR)
		s->eType = ET_INVISIBLE;
	else if(ps->stats[STAT_HEALTH] <= GIB_HEALTH)
		s->eType = ET_INVISIBLE;
	else
		s->eType = ET_PLAYER;

	s->number = ps->clientNum;

	s->traj.type = TR_LINEAR_STOP;
	copyv3(ps->origin, s->traj.base);
	if(snap)
		snapv3(s->traj.base);
	/* set the delta for flag direction and linear prediction */
	copyv3(ps->velocity, s->traj.delta);
	/* set the time for linear prediction */
	s->traj.time = time;
	/* set maximum extra polation time */
	s->traj.duration = 50;	/* 1000 / sv_fps (default = 20) */

	s->apos.type = TR_INTERPOLATE;
	copyv3(ps->viewangles, s->apos.base);
	if(snap)
		snapv3(s->apos.base);

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim	= ps->legsAnim;
	s->torsoAnim	= ps->torsoAnim;
	s->clientNum	= ps->clientNum;	/* ET_PLAYER looks here instead of at number */
	/* so corpses can also reference the proper config */
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0)
		s->eFlags |= EF_DEAD;
	else
		s->eFlags &= ~EF_DEAD;

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS)
			ps->entityEventSequence = ps->eventSequence -
						  MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] |
			   ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weap[Wpri] = ps->weap[Wpri];
	s->weap[Wsec] = ps->weap[Wsec];
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ps->powerups[ i ])
			s->powerups |= 1 << i;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
