/* Player movement code */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "bg.h"
#include "local.h"

/*
 * this counter lets us debug movement problems with a journal
 * by setting a conditional breakpoint for the previous frame
 */
uint cnt = 0;

static const Scalar GrapplePullSpeed	= 400.0f;
static const Scalar Hooklinelen		= 2.0f;
static const Scalar Maxhookforce	= 6.0f;
static const Scalar pm_stopspeed	= 100.0f;
static const Scalar pm_duckScale	= 0.25f;
static const Scalar pm_swimScale	= 0.50f;
static const Scalar pm_accelerate	= 10.0f;
static const Scalar pm_airaccelerate	= 1.0f;
static const Scalar pm_brakeforce	= 2.0f;
static const Scalar pm_wateraccelerate	= 4.0f;
static const Scalar pm_flyaccelerate	= 8.0f;
static const Scalar pm_airfriction	= 0.08f;
static const Scalar pm_waterfriction	= 1.0f;
static const Scalar pm_spectatorfriction	= 6.0f;

static void
starttorsoanim(Pmove *pm, int anim)
{
	if(pm->ps->pm_type >= PM_DEAD)
		return;
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

static void
startlegsanim(Pmove *pm, int anim)
{
	if(pm->ps->pm_type >= PM_DEAD)
		return;
	if(pm->ps->legsTimer > 0)
		return;		/* a high priority animation is running */
	pm->ps->legsAnim = ((pm->ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

static void
settorsoanim(Pmove *pm, int anim)
{
	if((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == anim)
		return;
	if(pm->ps->torsoTimer > 0)
		return;		/* a high priority animation is running */
	starttorsoanim(pm, anim);
}

static void
forcelegsanim(Pmove *pm, int anim)
{
	pm->ps->legsTimer = 0;
	startlegsanim(pm, anim);
}

/* Slide off of the impacting surface */
void
PM_ClipVelocity(Vec3 in, Vec3 normal, Vec3 out, float overbounce)
{
	float backoff, change;
	uint i;

	backoff = dotv3(in, normal);
	if(backoff < 0)
		backoff *= overbounce;
	else
		backoff /= overbounce;
	for(i=0; i<3; i++){
		change	= normal[i]*backoff;
		out[i]	= in[i] - change;
	}
}

/* Handles both ground friction and water friction */
static void
dofriction(Pmove *pm, Pml *pml)
{
	Vec3 vec;
	float *vel;
	float speed, newspeed, control, drop;

	vel = pm->ps->velocity;
	copyv3(vel, vec);
	speed = lenv3(vec);
	if(speed < 0.1f){
		vel[0] = 0;
		vel[1] = 0;	/* allow sinking underwater */
		vel[2] = 0;
		/* FIXME: still have z friction underwater? */
		return;
	}
	drop = 0;
	/* apply air  friction */
	drop += speed*pm_airfriction*pml->frametime;
	if(pm->waterlevel)
		/* apply water friction even if just wading */
		drop += speed * pm_waterfriction * pm->waterlevel * pml->frametime;
	if(pm->ps->pm_type == PM_SPECTATOR)
		drop += speed * pm_spectatorfriction * pml->frametime;
	/* scale the velocity */
	newspeed = speed - drop;
	if(newspeed < 0)
		newspeed = 0;
	newspeed /= speed;
	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/* Handles user intended acceleration */
static void
q2accelerate(Pmove *pm, Pml *pml, Vec3 wishdir, float wishspeed, float accel)
{
	/* q2 style */
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = dotv3(pm->ps->velocity, wishdir);
	addspeed = wishspeed;
	if(addspeed < 0)
		return;
	accelspeed = accel*pml->frametime*wishspeed;
	if(accelspeed > addspeed)
		accelspeed = addspeed;
	saddv3(pm->ps->velocity, accelspeed, wishdir, pm->ps->velocity);
}

static void
accelerate(Pmove *pm, Pml *pml, Vec3 wishdir, float wishspeed, float accel)
{
	/* proper way (avoids strafe jump maxspeed bug), but feels bad */
	Vec3	wishVelocity;
	Vec3	pushDir;
	float	pushLen;
	float	canPush;

	scalev3(wishdir, wishspeed, wishVelocity);
	subv3(wishVelocity, pm->ps->velocity, pushDir);
	pushLen = normv3(pushDir);
	canPush = accel*pml->frametime*wishspeed;
	if(canPush > pushLen)
		canPush = pushLen;
	saddv3(pm->ps->velocity, canPush, pushDir, pm->ps->velocity);
}

/*
 * Returns the scale factor to apply to cmd movements
 * This allows the clients to use axial -127 to 127 values for all directions
 * without getting a sqrt(2) distortion in speed.
 */
static float
calccmdscale(Pmove *pm, Pml *pml, const Usrcmd *cmd)
{
	int max;
	float	total, scale;

	UNUSED(pml);
	max = abs(cmd->forwardmove);
	if(abs(cmd->rightmove) > max)
		max = abs(cmd->rightmove);
	if(abs(cmd->upmove) > max)
		max = abs(cmd->upmove);
	if(!max)
		return 0;
	total = sqrt(cmd->forwardmove*cmd->forwardmove 
		+ cmd->rightmove*cmd->rightmove
		+ cmd->upmove*cmd->upmove);
	scale = (float)pm->ps->speed * max / (127.0f * total);
	return scale;
}

/*
 * Determine the rotation of the legs relative to the facing dir
 * so that clients can rotate legs for strafing
 */
static void
setmovedir(Pmove *pm, Pml *pml)
{
	UNUSED(pml);
	if(pm->cmd.forwardmove || pm->cmd.rightmove){
		if(pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 0;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 1;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0)
			pm->ps->movementDir = 2;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 3;
		else if(pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 4;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 5;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0)
			pm->ps->movementDir = 6;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 7;
	}else{
		/* 
		 * if they aren't actively going directly sideways,
		 * change the animation to the diagonal so they
		 * don't stop too crooked
		 */
		if(pm->ps->movementDir == 2)
			pm->ps->movementDir = 1;
		else if(pm->ps->movementDir == 6)
			pm->ps->movementDir = 7;
	}
}

static qbool
checkwaterjump(Pmove *pm, Pml *pml)
{
	Vec3 spot, flatforward;
	int cont;

	if(pm->ps->pm_time)
		return qfalse;
	if(pm->waterlevel != 2)	/* check for water jump */
		return qfalse;

	flatforward[0] = pml->forward[0];
	flatforward[1] = pml->forward[1];
	flatforward[2] = 0;
	normv3(flatforward);

	saddv3(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(!(cont & CONTENTS_SOLID))
		return qfalse;

	spot[2] += 16;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY))
		return qfalse;

	/* jump out of water */
	scalev3(pml->forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags	|= PMF_TIME_WATERJUMP;
	pm->ps->pm_time		= 2000;
	return qtrue;
}

/* Flying out of the water */
static void
waterjumpmove(Pmove *pm, Pml *pml)
{
	/* waterjump has no control, but falls */
	PM_StepSlideMove(pm, pml, qtrue);
	pm->ps->velocity[2] -= pm->ps->gravity * pml->frametime;
	if(pm->ps->velocity[2] < 0){
		/* cancel as soon as we are falling down again */
		pm->ps->pm_flags	&= ~PMF_ALL_TIMES;
		pm->ps->pm_time		= 0;
	}
}

static void
watermove(Pmove *pm, Pml *pml)
{
	int i;
	Vec3 wishvel, wishdir;
	float wishspeed, scale, vel;

	if(checkwaterjump(pm, pml)){
		waterjumpmove(pm, pml);
		return;
	}
#if 0
	/* jump = head for surface */
	if(pm->cmd.upmove >= 10)
		if(pm->ps->velocity[2] > -300){
			if(pm->watertype == CONTENTS_WATER)
				pm->ps->velocity[2] = 100;
			else if(pm->watertype == CONTENTS_SLIME)
				pm->ps->velocity[2] = 80;
			else
				pm->ps->velocity[2] = 50;
		}
#endif
	dofriction(pm, pml);
	
	scale = calccmdscale(pm, pml, &pm->cmd);
	/* user intentions */
	if(!scale){
		wishvel[0]	= 0;
		wishvel[1]	= 0;
		wishvel[2]	= -60;	/* sink towards bottom */
	}else{
		for(i=0; i<3; i++)
			wishvel[i] = scale * pml->forward[i]*
				     pm->cmd.forwardmove + scale * pml->right[i]*
				     pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	copyv3(wishvel, wishdir);
	wishspeed = normv3(wishdir);
	if(wishspeed > pm->ps->speed * pm_swimScale)
		wishspeed = pm->ps->speed * pm_swimScale;
	accelerate(pm, pml, wishdir, wishspeed, pm_wateraccelerate);

	/* make sure we can go up slopes easily under water */
	if(pml->groundPlane &&
	   dotv3(pm->ps->velocity, pml->groundTrace.plane.normal) < 0){
		vel = lenv3(pm->ps->velocity);
		/* slide along the ground plane */
		PM_ClipVelocity(pm->ps->velocity, pml->groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP);
		normv3(pm->ps->velocity);
		scalev3(pm->ps->velocity, vel, pm->ps->velocity);
	}
	PM_SlideMove(pm, pml, qfalse);
}

/* spectator */
static void
specmove(Pmove *pm, Pml *pml)
{
	int i;
	Vec3 wishvel, wishdir;
	float	wishspeed, scale;

	dofriction(pm, pml);	/* normal slowdown */
	scale = calccmdscale(pm, pml, &pm->cmd);
	/* user intentions */
	if(!scale){
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}else{
		for(i=0; i<3; i++)
			wishvel[i] = scale * pml->forward[i]*
				  pm->cmd.forwardmove + scale * pml->right[i]*
				  pm->cmd.rightmove;
		wishvel[2] += scale * pm->cmd.upmove;
	}

	copyv3(wishvel, wishdir);
	wishspeed = normv3(wishdir);
	accelerate(pm, pml, wishdir, wishspeed, pm_flyaccelerate);
	PM_StepSlideMove(pm, pml, qfalse);
}

static void
noclipmove(Pmove *pm, Pml *pml)
{
	Scalar speed, drop, friction, control, newspeed;
	Scalar fmove, smove, umove, wishspeed, scale, frac;
	uint i;
	Vec3 wishvel, wishdir;

	frac = 1.0f + 4.0f * (Scalar)pm->cmd.brakefrac / 127.0f;	/* braking multiplies vel */
	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	/* friction */
	speed = lenv3 (pm->ps->velocity);
	if(speed < 1)
		copyv3 (vec3_origin, pm->ps->velocity);
	else{
		drop = 0;

		friction = pm_spectatorfriction*1.5f;	/* extra friction */
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml->frametime;

		/* scale the velocity */
		newspeed = speed - drop;
		if(newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		scalev3(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	/* accelerate */
	scale = calccmdscale(pm, pml, &pm->cmd);
	fmove = pm->cmd.forwardmove * frac;
	smove = pm->cmd.rightmove * frac;
	umove = pm->cmd.upmove * frac;
	for(i=0; i<3; i++)
		wishvel[i] = pml->forward[i]*fmove + pml->right[i]*smove + pml->up[i]*umove;
	copyv3(wishvel, wishdir);
	wishspeed = normv3(wishdir);
	wishspeed *= scale;
	accelerate(pm, pml, wishdir, wishspeed, pm_accelerate);
	/* move */
	saddv3(pm->ps->origin, pml->frametime, pm->ps->velocity,
		pm->ps->origin);
}

/* helper just to avoid code duplication in air/grapplemove */
static ID_INLINE void
_airmove(Pmove *pm, Pml *pml, const Usrcmd *cmd, Vec3 *wvel, Vec3 *wdir, float *wspeed)
{
	uint i;
	Scalar fm, sm, um, scale, wishspeed;
	Vec3 wishvel, wishdir;
	
	fm = cmd->forwardmove;
	sm = cmd->rightmove;
	um = cmd->upmove;
	wishspeed = 0.0f;
	scale = calccmdscale(pm, pml, cmd);
	setmovedir(pm, pml);
	/* project moves down to flat plane */
	normv3(pml->forward);
	normv3(pml->right);
	normv3(pml->up);
	for(i = 0; i < 3; i++)
		(wishvel)[i] = pml->forward[i]*fm + pml->right[i]*sm + pml->up[i]*um;
	wishspeed = lenv3(wishvel);
	if(wvel != nil)
		copyv3(wishvel, *wvel);
	if(wdir != nil){
		copyv3(wishvel, *wdir);
		normv3(*wdir);
	}
	if(wspeed != nil)
		*wspeed = wishspeed * scale;
}

static void
brakemove(Pmove *pm, Pml *pml)
{
	Vec3 bk, wdir;
	Scalar frac, currspeed;

	copyv3(pm->ps->velocity, bk);
	invv3(bk);
	currspeed = normv3(bk);
	_airmove(pm, pml, &pm->cmd, nil, &wdir, nil);
	addv3(wdir, bk, wdir);
	frac = (Scalar)pm->cmd.brakefrac / 127.0f;
	if(1)	
		/* feels rigid but difficult to control at high speeds, abruptly stops at zero */
		accelerate(pm, pml, wdir, frac*pm->ps->speed, pm_brakeforce);
	else	
		/* slidey, brakes well at high speeds, has 'drift' bug */
		accelerate(pm, pml, wdir, frac*currspeed, pm_brakeforce);
	PM_StepSlideMove(pm, pml, qtrue);
}

static void
airmove(Pmove *pm, Pml *pml)
{
	Vec3 wishvel, wishdir;
	float	wishspeed;
	
	dofriction(pm, pml);
	_airmove(pm, pml, &pm->cmd, &wishvel, &wishdir, &wishspeed);
	/* not on ground, so little effect on velocity */
	q2accelerate(pm, pml, wishdir, wishspeed, pm_airaccelerate);
	PM_StepSlideMove(pm, pml, qtrue);
}

static void
grapplemove(Pmove *pm, Pml *pml)
{
	Vec3 wishvel, wishdir, vel, v;
	float	wishspeed, vlen, oldlen, pullspeedcoef, grspd = GrapplePullSpeed;

	_airmove(pm, pml, &pm->cmd, &wishvel, &wishdir, &wishspeed);
	scalev3(pml->forward, -16, v);
	addv3(pm->ps->grapplePoint, v, v);
	subv3(v, pm->ps->origin, vel);
	vlen = lenv3(vel);
	if(pm->ps->grapplelast == qfalse)
		oldlen = vlen;
	else
		oldlen = pm->ps->oldgrapplelen;
	if(vlen > oldlen){
		pullspeedcoef = vlen - oldlen;
		pullspeedcoef *= pm->ps->swingstrength;
		grspd *= pullspeedcoef;
	}
	if(grspd < GrapplePullSpeed)
		grspd = GrapplePullSpeed;
	
	normv3(vel);
	q2accelerate(pm, pml, wishdir, wishspeed, pm_airaccelerate);
	q2accelerate(pm, pml, vel, grspd, pm_airaccelerate);
	pml->groundPlane = qfalse;
	PM_StepSlideMove(pm, pml, qtrue);
	pm->ps->oldgrapplelen = vlen;
}

static void
deadmove(Pmove *pm, Pml *pml)
{
	float forward;

	if(!pml->walking)
		return;
	forward = lenv3(pm->ps->velocity);	/* extra friction */
	forward -= 20;
	if(forward <= 0)
		clearv3(pm->ps->velocity);
	else{
		normv3(pm->ps->velocity);
		scalev3(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

/* Returns an event number apropriate for the groundsurface */
/* FIXME */
static int
footstepforsurface(Pmove *pm, Pml *pml)
{
	UNUSED(pm);
	UNUSED(pml);
	return 0;
}

/* Check for hard landings that generate sound events */
static void
crashland(Pmove *pm, Pml *pml)
{
	float	delta, dist, vel, acc, t, a, b, c, den;

	/* decide which landing animation to use */
	if(pm->ps->pm_flags & PMF_BACKWARDS_JUMP)
		forcelegsanim(pm, LEGS_LANDB);
	else
		forcelegsanim(pm, LEGS_LAND);
	pm->ps->legsTimer = TIMER_LAND;
	/* calculate the exact velocity on landing */
	dist = pm->ps->origin[2] - pml->previous_origin[2];
	vel = pml->previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if(den < 0)
		return;
	t = (-b - sqrt(den)) / (2 * a);

	delta	= vel + t * acc;
	delta	= delta*delta * 0.0001f;
	/* never take falling damage if completely underwater */
	if(pm->waterlevel == 3)
		return;
	/* reduce falling damage if there is standing water */
	if(pm->waterlevel == 2)
		delta *= 0.25f;
	if(pm->waterlevel == 1)
		delta *= 0.5f;
	if(delta < 1)
		return;

	/* create a local entity event to play the sound */
	if(!(pml->groundTrace.surfaceFlags & SURF_NODAMAGE)){
		/*
		 * SURF_NODAMAGE is used for bounce pads where you don't ever
		 * want to take damage or play a crunch sound
		 */
		if(delta > 60)
			PM_AddEvent(pm, pml, EV_FALL_FAR);
		else if(delta > 40){
			/* this is a pain grunt, so don't play it if dead */
			if(pm->ps->stats[STAT_HEALTH] > 0)
				PM_AddEvent(pm, pml, EV_FALL_MEDIUM);
		}else if(delta > 7)
			PM_AddEvent(pm, pml, EV_FALL_SHORT);
		else
			PM_AddEvent(pm, pml, footstepforsurface(pm, pml));
	}
}

static int
correctallsolid(Pmove *pm, Pml *pml, Trace *trace)
{
	int i, j, k;
	Vec3 point;

	if(pm->debugLevel)
		comprintf("%u:allsolid\n", cnt);
	/* jitter around */
	for(i = -1; i <= 1; i++){
		for(j = -1; j <= 1; j++){
			for(k = -1; k <= 1; k++){
				copyv3(pm->ps->origin, point);
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				pm->trace(trace, point, pm->mins, pm->maxs,
					point, pm->ps->clientNum,
					pm->tracemask);
				if(!trace->allsolid){
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25f;
					pm->trace(trace, pm->ps->origin,
						pm->mins, pm->maxs, point,
						pm->ps->clientNum,
						pm->tracemask);
					/* FIXME: we don't care about the ground at present */
					if(0)
						pml->groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml->groundPlane = qfalse;
	pml->walking = qfalse;
	return qfalse;
}

/* The ground trace didn't hit a surface, so we are in freefall */
static void
setfalling(Pmove *pm, Pml *pml)
{
	Trace trace;
	Vec3 point;

	if(pm->ps->groundEntityNum != ENTITYNUM_NONE){
		/* we just transitioned into freefall */
		if(pm->debugLevel)
			comprintf("%u:lift\n", cnt);
		/*
		 * if they aren't in a jumping animation and the ground is a 
		 * ways away, force into it. if we didn't do the trace, the 
		 * player would be backflipping down staircases
		 */
		copyv3(pm->ps->origin, point);
		point[2] -= 64.0f;
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point,
			pm->ps->clientNum,
			pm->tracemask);
		if(trace.fraction == 1.0f){
			if(pm->cmd.forwardmove >= 0){
				forcelegsanim(pm, LEGS_JUMP);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}else{
				forcelegsanim(pm, LEGS_JUMPB);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml->groundPlane = qfalse;
	pml->walking = qfalse;
}

static void
groundtrace(Pmove *pm, Pml *pml)
{
	Vec3 point, vel;
	Trace trace;

	copyv3(pm->ps->velocity, vel);
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25f;
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point,
		pm->ps->clientNum,
		pm->tracemask);
	pml->groundTrace = trace;
	/* do something corrective if the trace starts in a solid... */
	if(trace.allsolid)
		if(!correctallsolid(pm, pml, &trace))
			return;
	/* if the trace didn't hit anything, we are in free fall */
	if(trace.fraction == 1.0f){
		setfalling(pm, pml);
		pml->groundPlane = qfalse;
		pml->walking = qfalse;
		return;
	}
	
	/* check if getting thrown off the ground */
	if(vel[2] > 0 &&
	   dotv3(vel, trace.plane.normal) > 10)
	then{
		if(pm->debugLevel)
			comprintf("%u:kickoff\n", cnt);
		/* go into jump animation */
		if(pm->cmd.forwardmove >= 0){
			forcelegsanim(pm, LEGS_JUMP);
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}else{
			forcelegsanim(pm, LEGS_JUMPB);
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml->groundPlane = qfalse;
		pml->walking = qfalse;
		return;
	}

	/* slopes that are too steep will not be considered onground */
	if(trace.plane.normal[2] < MIN_WALK_NORMAL){
		if(pm->debugLevel)
			comprintf("%u:steep\n", cnt);
		/* FIXME: if they can't slide down the slope, let them
		 * walk (sharp crevices) */
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml->groundPlane = qtrue;
		pml->walking = qfalse;
		return;
	}
	pml->groundPlane = qtrue;
	pml->walking = qtrue;

	/* hitting solid ground will end a waterjump */
	if(pm->ps->pm_flags & PMF_TIME_WATERJUMP){
		pm->ps->pm_flags	&= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time		= 0;
	}
	
	if(pm->ps->groundEntityNum == ENTITYNUM_NONE){
		/* just hit the ground */
		if(pm->debugLevel)
			comprintf("%u:Land\n", cnt);
		crashland(pm, pml);
		/* don't do landing time if we were just going down a slope */
		if(pml->previous_velocity[2] < -200){
			/* don't allow another jump for a little while */
			pm->ps->pm_flags	|= PMF_TIME_LAND;
			pm->ps->pm_time		= 250;
		}
	}
	pm->ps->groundEntityNum = trace.entityNum;
	PM_AddTouchEnt(pm, trace.entityNum);
}

static void
setwaterlevel(Pmove *pm, Pml *pml)
{
	Vec3 point;
	int cont, samp1, samp2;

	UNUSED(pml);
	/* get waterlevel, accounting for ducking */
	pm->waterlevel = 0;
	pm->watertype = 0;
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MinsZ + 1;
	cont = pm->pointcontents(point, pm->ps->clientNum);
	if(cont & MASK_WATER){
		samp2 = pm->ps->viewheight - MinsZ;
		samp1 = samp2 / 2;
		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MinsZ + samp1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if(cont & MASK_WATER){
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MinsZ + samp2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if(cont & MASK_WATER)
				pm->waterlevel = 3;
		}
	}

}

/* Sets mins, maxs, and pm->ps->viewheight */
static void
setplayerbounds(Pmove *pm, Pml *pml)
{
	UNUSED(pml);
	setv3(pm->mins, MinsX, MinsY, MinsZ);
	setv3(pm->maxs, MaxsX, MaxsY, MaxsZ);
	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	if(pm->ps->pm_type == PM_DEAD){
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}
}

/* Generate sound events for entering and leaving water */
static void
dowaterevents(Pmove *pm, Pml *pml)	/* FIXME? */
{
	/* if just entered a water volume, play a sound */
	if(!pml->previous_waterlevel && pm->waterlevel)
		PM_AddEvent(pm, pml, EV_WATER_TOUCH); 
	/* if just completely exited a water volume, play a sound */
	if(pml->previous_waterlevel && !pm->waterlevel)
		PM_AddEvent(pm, pml, EV_WATER_LEAVE);
	/* check for head just going under water */
	if(pml->previous_waterlevel != 3 && pm->waterlevel == 3)
		PM_AddEvent(pm, pml, EV_WATER_UNDER);
	/* check for head just coming out of water */
	if(pml->previous_waterlevel == 3 && pm->waterlevel != 3)
		PM_AddEvent(pm, pml, EV_WATER_CLEAR);
}

/*
 * Weapon events
 * FIXME: feels wrong having this in pmove
 */

static ID_INLINE int
weaptimetab(Weapon wp)
{
	switch(wp){
	case Wmelee:		return 400;
	case Wmachinegun:	return 50;
	case Wshotgun:		return 1000;
	case Wlightning:	return 50;
	case Wrailgun:		return 1500;
	case Wplasmagun:	return 100;
	case Whook:		return 10;
	case Wnanoidcannon:	return 1000;
	case Wchaingun:		return 30;
	case Wrocketlauncher:
	case Wgrenadelauncher:
	case Wproxlauncher:	return 800;
	case Whominglauncher:	return 1200;
	default:		return 0;
	}
}

static int
slot2attackbutton(Weapslot sl)
{
	switch(sl){
	case WSpri:	return BUTTON_PRIATTACK;
	case WSsec:	return BUTTON_SECATTACK;
	case WShook:	return BUTTON_HOOKFIRE;
	default:	return -1;
	}
}

static int
slot2weapstat(Weapslot sl)
{
	switch(sl){
	case WSpri:	return STAT_PRIWEAPS;
	case WSsec:	return STAT_SECWEAPS;
	default:	return -1;
	}
}

static int
slot2fireevent(Weapslot sl)
{
	switch(sl){
	case WSpri:	return EV_FIREPRIWEAP;
	case WSsec:	return EV_FIRESECWEAP;
	case WShook:	return EV_FIREHOOK;
	default:	return -1;
	}
}

static int
slot2noammoevent(Weapslot sl)
{
	switch(sl){
	default:	return EV_NOAMMO;
	}
}

/*
 * weapon changes
 */
static void
startweapchange(Pmove *pm, Pml *pml, Weapslot sl, int weapon)
{
	if(weapon <= Wnone || weapon >= Wnumweaps 
	  || sl >= WSnumslots)
	then{
		return;
	}
	if(!(pm->ps->stats[slot2weapstat(sl)] & (1 << weapon)))
		return;
	if(pm->ps->weapstate[sl] == WEAPON_DROPPING)
		return;
	PM_AddEvent(pm, pml, EV_CHANGE_WEAPON);
	pm->ps->weapstate[sl] = WEAPON_DROPPING;
	pm->ps->weaptime[sl] += 100;
	starttorsoanim(pm, TORSO_DROP);
}

static void
finishweapchange(Pmove *pm, Pml *pml, Weapslot sl)
{
	Weapon weap;

	UNUSED(pml);
	if(sl >= WSnumslots)
		return;
	weap = pm->cmd.weap[sl];
	if(weap >= Wnumweaps)
		weap = Wnone;
	if(!(pm->ps->stats[slot2weapstat(sl)] & (1 << weap)))
		weap = Wnone;
	pm->ps->weap[sl] = weap;
	pm->ps->weapstate[sl] = WEAPON_RAISING;
	pm->ps->weaptime[sl] += 100;
	starttorsoanim(pm, TORSO_RAISE);
}

/* choose the torso animation */
static void
dotorsoanim(Pmove *pm, Pml *pml)
{
	UNUSED(pml);
	if(pm->ps->weapstate[WSpri] == WEAPON_READY){
		if(pm->ps->weap[WSpri] == Wmelee)
			settorsoanim(pm, TORSO_STAND2);
		else
			settorsoanim(pm, TORSO_STAND);
		return;
	}
}

/* Generates weapon events and modifes the weapon counter */
static void
doweapevents(Pmove *pm, Pml *pml, Weapslot sl)
{
	int addTime;
	Playerstate *p;
	
	if(sl >= WSnumslots)
		return;
	p = pm->ps;
	if(p->pm_flags & PMF_RESPAWNED)
		return;	/* don't allow attack until all buttons are up */
	if(p->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;	/* ignore if spectator */
	if(p->stats[STAT_HEALTH] <= 0){
		/* player is dead */
		p->weap[sl] = Wnone;
		return;
	}
	/* check for item usage */
	if(pm->cmd.buttons & BUTTON_USE_HOLDABLE){
		if(!(p->pm_flags & PMF_USE_ITEM_HELD)){
			if(bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].tag == HI_MEDKIT
			  && p->stats[STAT_HEALTH] >=
			  (p->stats[STAT_MAX_HEALTH] + 25)){
				/* don't use medkit if at max health */
			}else{
				p->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent(pm, pml, EV_USE_ITEM0 + bg_itemlist[p->stats[STAT_HOLDABLE_ITEM]].tag);
				p->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	}else
		p->pm_flags &= ~PMF_USE_ITEM_HELD;

	/* make weapon function */
	if(p->weaptime[sl] > 0)
		p->weaptime[sl] -= pml->msec;

	/* 
	 * check for weapon changes.
	 * can't change if weapon is firing, but can change
	 * again if lowering or raising 
	 */
	if(p->weaptime[sl] <= 0 || p->weapstate[sl] != WEAPON_FIRING){
		if(p->weap[sl] != pm->cmd.weap[sl])
			startweapchange(pm, pml, sl, pm->cmd.weap[sl]);
	}
	/*
	 * do nothing if weapon is still in cooldown state
	 */
	if(p->weaptime[sl] > 0 && p->weapstate[sl] == WEAPON_FIRING)
		return;
		
	/* change weapon if time */
	if(p->weapstate[sl] == WEAPON_DROPPING){
		finishweapchange(pm, pml, sl);
		return;
	}
	if(p->weapstate[sl] == WEAPON_RAISING){
		p->weapstate[sl] = WEAPON_READY;
		if(p->weap[sl] == Wmelee)
			starttorsoanim(pm, TORSO_STAND2);
		else
			starttorsoanim(pm, TORSO_STAND);
		return;
	}
	/* check for fire */
	if(!(pm->cmd.buttons & slot2attackbutton(sl))){
		p->weaptime[sl] = 0;
		p->weapstate[sl] = WEAPON_READY;
		return;
	}
	/* start the animation even if out of ammo */
	if(p->weap[sl] == Wmelee){
		/* melee only "fires" when it actually hits something */
		if(!pm->gauntletHit){
			p->weaptime[sl] = 0;
			p->weapstate[sl] = WEAPON_READY;
			return;
		}
		starttorsoanim(pm, TORSO_ATTACK2);
	}else
		starttorsoanim(pm, TORSO_ATTACK);
	p->weapstate[sl] = WEAPON_FIRING;
	/* check for out of ammo */
	if(p->ammo[p->weap[sl]] == 0){
		PM_AddEvent(pm, pml, slot2noammoevent(sl));
		p->weaptime[sl] += 500;
		return;
	}
	/* take an ammo away if not infinite */
	if(p->ammo[p->weap[sl]] != -1)
		p->ammo[p->weap[sl]]--;
	/* fire weapon */
	PM_AddEvent(pm, pml, slot2fireevent(sl));
	
	addTime = weaptimetab(p->weap[sl]);
	if(p->powerups[PW_HASTE])
		addTime /= 1.3f;
	p->weaptime[sl] += addTime;
}

static void
animate(Pmove *pm, Pml *pml)
{
	if(pm->cmd.buttons & BUTTON_GESTURE){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_GESTURE);
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent(pm, pml, EV_TAUNT);
		}
#ifdef MISSIONPACK
	}else if(pm->cmd.buttons & BUTTON_GETFLAG){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_GETFLAG);
			pm->ps->torsoTimer = 600;	/* TIMER_GESTURE; */
		}
	}else if(pm->cmd.buttons & BUTTON_GUARDBASE){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_GUARDBASE);
			pm->ps->torsoTimer = 600;	/* TIMER_GESTURE; */
		}
	}else if(pm->cmd.buttons & BUTTON_PATROL){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_PATROL);
			pm->ps->torsoTimer = 600;	/* TIMER_GESTURE; */
		}
	}else if(pm->cmd.buttons & BUTTON_FOLLOWME){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_FOLLOWME);
			pm->ps->torsoTimer = 600;	/* TIMER_GESTURE; */
		}
	}else if(pm->cmd.buttons & BUTTON_AFFIRMATIVE){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_AFFIRMATIVE);
			pm->ps->torsoTimer = 600;	/* TIMER_GESTURE; */
		}
	}else if(pm->cmd.buttons & BUTTON_NEGATIVE){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(pm, TORSO_NEGATIVE);
			pm->ps->torsoTimer = 600;	/* TIMER_GESTURE; */
		}
#endif
	}
}

static void
droptimers(Pmove *pm, Pml *pml)
{
	/* drop misc timing counter */
	if(pm->ps->pm_time){
		if(pml->msec >= pm->ps->pm_time){
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}else
			pm->ps->pm_time -= pml->msec;
	}
	/* drop animation counter */
	if(pm->ps->legsTimer > 0){
		pm->ps->legsTimer -= pml->msec;
		if(pm->ps->legsTimer < 0)
			pm->ps->legsTimer = 0;
	}
	if(pm->ps->torsoTimer > 0){
		pm->ps->torsoTimer -= pml->msec;
		if(pm->ps->torsoTimer < 0)
			pm->ps->torsoTimer = 0;
	}
}

/*
 * This can be used as another entry point when only the viewangles
 * are being updated isntead of a full move
 */
void
PM_UpdateViewAngles(Playerstate *ps, const Usrcmd *cmd)
{
	int i;
	Vec3 d;	/* yaw, pitch, roll deltas */
	Quat orient, delta, neworient;

	if((ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION)
	  || (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0))
	then{
		return;	/* no view changes at all */
	}

	shortstoeuler(cmd->angles, d);
	eulertoq(d, orient);
	shortstoeuler(ps->delta_angles, d);
	eulertoq(d, delta);
	mulq(delta, orient, neworient);
	qtoeuler(neworient, ps->viewangles);
}

/* convenience function also used by slidemove */
void
PM_AddEvent(Pmove *pm, Pml *pml, int newEvent)
{
	UNUSED(pml);
	BG_AddPredictableEventToPlayerstate(newEvent, 0, pm->ps);
}

/* as above */
void
PM_AddTouchEnt(Pmove *pm, int entityNum)
{
	int i;

	if(entityNum == ENTITYNUM_WORLD)
		return;
	if(pm->numtouch == MAXTOUCH)
		return;

	/* see if it is already added */
	for(i = 0; i < pm->numtouch; i++)
		if(pm->touchents[ i ] == entityNum)
			return;
	/* add it */
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

void trap_snapv3(float *v);

void
PmoveSingle(Pmove *pm)
{
	Pml pml;
	Weapslot sl;
	
	cnt++;
	/* clear results */
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;
	
	if(pm->ps->stats[STAT_HEALTH] <= 0)
		/* corpses can fly through bodies */
		pm->tracemask &= ~CONTENTS_BODY;
	/*
	 * make sure walking button is clear if they are running, to avoid
	 * proxy no-footsteps cheats
	 */
	if(abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64)
		pm->cmd.buttons &= ~BUTTON_WALKING;

	/* set the talk balloon flag */
	if(pm->cmd.buttons & BUTTON_TALK)
		pm->ps->eFlags |= EF_TALK;
	else
		pm->ps->eFlags &= ~EF_TALK;

	/* set the firing flag for continuous beam weapons */
	if(!(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type !=
	   PM_INTERMISSION)
	then{
		if((pm->cmd.buttons & BUTTON_PRIATTACK)
		&& pm->ps->ammo[pm->ps->weap[WSpri]])
		then{
			pm->ps->eFlags |= EF_PRIFIRING;
		}else 
			pm->ps->eFlags &= ~EF_PRIFIRING;
		
		if((pm->cmd.buttons & BUTTON_SECATTACK)
		&& pm->ps->ammo[pm->ps->weap[WSsec]])
		then{
		   	pm->ps->eFlags |= EF_SECFIRING;
		}else 
			pm->ps->eFlags &= ~EF_SECFIRING;

		if((pm->cmd.buttons & BUTTON_HOOKFIRE)
		&& pm->ps->ammo[pm->ps->weap[WShook]])
		then{
		   	pm->ps->eFlags |= EF_HOOKFIRING;
		}else
			pm->ps->eFlags &= ~EF_HOOKFIRING;
	}

	/* clear the respawned flag if attack and use are cleared */
	if(pm->ps->stats[STAT_HEALTH] > 0 &&
	   !(pm->cmd.buttons & (BUTTON_PRIATTACK | BUTTON_SECATTACK | BUTTON_USE_HOLDABLE)))
		pm->ps->pm_flags &= ~PMF_RESPAWNED;

	/*
	 * if talk button is down, dissallow all other input
	 * this is to prevent any possible intercept proxy from
	 * adding fake talk balloons
	 */
	if(pm->cmd.buttons & BUTTON_TALK){
		/*
		 * keep the talk button set tho for when the cmd.serverTime > 66 msec
		 * and the same cmd is used multiple times in Pmove
		 */
		pm->cmd.buttons = BUTTON_TALK;
		pm->cmd.forwardmove	= 0;
		pm->cmd.rightmove	= 0;
		pm->cmd.upmove = 0;
	}

	memset(&pml, 0, sizeof pml);
	pml.msec = pm->cmd.serverTime - pm->ps->commandTime;
	if(pml.msec < 1)
		pml.msec = 1;
	else if(pml.msec > 200)
		pml.msec = 200;
	pm->ps->commandTime = pm->cmd.serverTime;
	pml.frametime = pml.msec * 0.001f;

	/* save old org in case we get stuck */
	copyv3(pm->ps->origin, pml.previous_origin);
	/* save old velocity for crashlanding */
	copyv3(pm->ps->velocity, pml.previous_velocity);
	
	PM_UpdateViewAngles(pm->ps, &pm->cmd);
	anglev3s(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if(pm->cmd.upmove < 10)	/* not holding jump */
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;

	/* decide if backpedaling animations should be used */
	if(pm->cmd.forwardmove < 0)
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	else if(pm->cmd.forwardmove > 0 ||
		(pm->cmd.forwardmove == 0 && pm->cmd.rightmove))
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;

	if(pm->ps->pm_type >= PM_DEAD){
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}
	if(pm->ps->pm_type == PM_FREEZE
	  || pm->ps->pm_type == PM_INTERMISSION
	  || pm->ps->pm_type == PM_SPINTERMISSION)
		return;		/* no movement at all */

	setwaterlevel(pm, &pml);
	pml.previous_waterlevel = pm->waterlevel;

	setplayerbounds(pm, &pml);
	droptimers(pm, &pml);
	//groundtrace(pm, &pml);	/* set groundentity */
	
	if(pm->ps->pm_type == PM_DEAD)
		deadmove(pm, &pml);
	else if(pm->ps->pm_type == PM_NOCLIP)
		noclipmove(pm, &pml);
	else if(pm->ps->pm_type == PM_SPECTATOR){
		specmove(pm, &pml);
		return;
	}else if(pm->ps->pm_flags & PMF_GRAPPLE_PULL){
		grapplemove(pm, &pml);
		pm->ps->grapplelast = qtrue;
	}else if(pm->ps->pm_flags & PMF_TIME_WATERJUMP)
		waterjumpmove(pm, &pml);
	else if(pm->waterlevel > 1)	/* swimming */
		watermove(pm, &pml);
	else if(pm->cmd.brakefrac > 0)
		brakemove(pm, &pml);
	else{	/* airborne */
		airmove(pm, &pml);
	}
	
	if(!(pm->ps->pm_flags & PMF_GRAPPLE_PULL)){
		pm->ps->grapplelast = qfalse;
		pm->ps->oldgrapplelen = 0.0f;
	}

	animate(pm, &pml);
	for(sl = 0; sl < WSnumslots; ++sl)
		doweapevents(pm, &pml, sl);
	dotorsoanim(pm, &pml);
	dowaterevents(pm, &pml);
	/* snap some parts of playerstate to save network bandwidth */
	trap_snapv3(pm->ps->velocity);
}

/* Can be called by either the server or the client */
void
PM_Pmove(Pmove *p)
{
	int t;	/* final time */
	
	t = p->cmd.serverTime;
	if(t < p->ps->commandTime)
		return;		/* should not happen */
	if(t > p->ps->commandTime + 1000)
		p->ps->commandTime = t - 1000;
	p->ps->pmove_framecount =
		(p->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	/*
	 * chop the move up if it is too long, to prevent framerate
	 * dependent behavior
	 */
	while(p->ps->commandTime != t){
		int msec;

		msec = t - p->ps->commandTime;
		if(p->pmove_fixed){
			if(msec > p->pmove_msec)
				msec = p->pmove_msec;
		}else if(msec > 66)
			msec = 66;
		p->cmd.serverTime = p->ps->commandTime + msec;
		PmoveSingle(p);
		if(p->ps->pm_flags & PMF_JUMP_HELD)
			p->cmd.upmove = 20;
	}
}
