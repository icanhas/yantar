/* part of pmove functionality */
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
 *
 * input: origin, velocity, bounds, groundPlane, trace function
 *
 * output: origin, velocity, impacts, stairup boolean
 *
 */

/*
 * Returns qtrue if the velocity was clipped in some way
 */
#define MAX_CLIP_PLANES 5
qbool
PM_SlideMove(pmove_t *pm, pml_t *pml, qbool gravity)
{
	int bumpcount, numbumps;
	Vec3	dir;
	float	d;
	int	numplanes;
	Vec3	planes[MAX_CLIP_PLANES];
	Vec3	primal_velocity;
	Vec3	clipVelocity;
	int	i, j, k;
	trace_t trace;
	Vec3	end;
	float	time_left;
	float	into;
	Vec3	endVelocity;
	Vec3	endClipVelocity;

	numbumps = 4;

	copyv3 (pm->ps->velocity, primal_velocity);

	if(gravity){
		copyv3(pm->ps->velocity, endVelocity);
		endVelocity[2] -= pm->ps->gravity * pml->frametime;
		pm->ps->velocity[2] =
			(pm->ps->velocity[2] + endVelocity[2]) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if(pml->groundPlane)
			/* slide along the ground plane */
			PM_ClipVelocity (pm->ps->velocity,
				pml->groundTrace.plane.normal,
				pm->ps->velocity,
				OVERCLIP);
	}

	time_left = pml->frametime;

	/* never turn against the ground plane */
	if(pml->groundPlane){
		numplanes = 1;
		copyv3(pml->groundTrace.plane.normal, planes[0]);
	}else
		numplanes = 0;

	/* never turn against original velocity */
	norm2v3(pm->ps->velocity, planes[numplanes]);
	numplanes++;

	for(bumpcount=0; bumpcount < numbumps; bumpcount++){

		/* calculate position we are trying to move to */
		maddv3(pm->ps->origin, time_left, pm->ps->velocity, end);

		/* see if we can make it there */
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, end,
			pm->ps->clientNum,
			pm->tracemask);

		if(trace.allsolid){
			/* entity is completely trapped in another solid */
			pm->ps->velocity[2] = 0;	/* don't build up falling damage, but allow sideways acceleration */
			return qtrue;
		}

		if(trace.fraction > 0)
			/* actually covered some distance */
			copyv3 (trace.endpos, pm->ps->origin);

		if(trace.fraction == 1)
			break;	/* moved the entire distance */

		/* save entity for contact */
		PM_AddTouchEnt(pm, trace.entityNum);

		time_left -= time_left * trace.fraction;

		if(numplanes >= MAX_CLIP_PLANES){
			/* this shouldn't really happen */
			clearv3(pm->ps->velocity);
			return qtrue;
		}

		/*
		 * if this is the same plane we hit before, nudge velocity
		 * out along it, which fixes some epsilon issues with
		 * non-axial planes
		 *  */
		for(i = 0; i < numplanes; i++)
			if(dotv3(trace.plane.normal, planes[i]) > 0.99){
				addv3(trace.plane.normal, pm->ps->velocity,
					pm->ps->velocity);
				break;
			}
		if(i < numplanes)
			continue;
		copyv3 (trace.plane.normal, planes[numplanes]);
		numplanes++;

		/*
		 * modify velocity so it parallels all of the clip planes
		 *  */

		/* find a plane that it enters */
		for(i = 0; i < numplanes; i++){
			into = dotv3(pm->ps->velocity, planes[i]);
			if(into >= 0.1)
				continue;	/* move doesn't interact with the plane */

			/* see how hard we are hitting things */
			if(-into > pml->impactSpeed)
				pml->impactSpeed = -into;

			/* slide along the plane */
			PM_ClipVelocity (pm->ps->velocity, planes[i],
				clipVelocity,
				OVERCLIP);

			/* slide along the plane */
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity,
				OVERCLIP);

			/* see if there is a second plane that the new move enters */
			for(j = 0; j < numplanes; j++){
				if(j == i)
					continue;
				if(dotv3(clipVelocity, planes[j]) >= 0.1)
					continue;	/* move doesn't interact with the plane */

				/* try clipping the move to the plane */
				PM_ClipVelocity(clipVelocity, planes[j],
					clipVelocity,
					OVERCLIP);
				PM_ClipVelocity(endClipVelocity, planes[j],
					endClipVelocity,
					OVERCLIP);

				/* see if it goes back into the first clip plane */
				if(dotv3(clipVelocity, planes[i]) >= 0)
					continue;

				/* slide the original velocity along the crease */
				crossv3 (planes[i], planes[j], dir);
				normv3(dir);
				d = dotv3(dir, pm->ps->velocity);
				scalev3(dir, d, clipVelocity);

				crossv3 (planes[i], planes[j], dir);
				normv3(dir);
				d = dotv3(dir, endVelocity);
				scalev3(dir, d, endClipVelocity);

				/* see if there is a third plane the the new move enters */
				for(k = 0; k < numplanes; k++){
					if(k == i || k == j)
						continue;
					if(dotv3(clipVelocity,
						   planes[k]) >= 0.1)
						continue;	/* move doesn't interact with the plane */

					/* stop dead at a triple plane interaction */
					clearv3(pm->ps->velocity);
					return qtrue;
				}
			}

			/* if we have fixed all interactions, try another move */
			copyv3(clipVelocity, pm->ps->velocity);
			copyv3(endClipVelocity, endVelocity);
			break;
		}
	}

	if(gravity)
		copyv3(endVelocity, pm->ps->velocity);

	/* don't change velocity if in a timer (FIXME: is this correct?) */
	if(pm->ps->pm_time)
		copyv3(primal_velocity, pm->ps->velocity);

	return (bumpcount != 0);
}

void
PM_StepSlideMove(pmove_t *pm, pml_t *pml, qbool gravity)
{
	Vec3	start_o, start_v;
/*	Vec3		down_o, down_v; */
	trace_t trace;
/* float		down_dist, up_dist;
 * Vec3		delta, delta2; */
	Vec3	up, down;
	float	stepSize;

	copyv3 (pm->ps->origin, start_o);
	copyv3 (pm->ps->velocity, start_v);

	if(PM_SlideMove(pm, pml, gravity) == 0)
		return;		/* we got exactly where we wanted to go first try */

	copyv3(start_o, down);
	down[2] -= STEPSIZE;
	pm->trace (&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum,
		pm->tracemask);
	setv3(up, 0, 0, 1);
	/* never step up when you still have up velocity */
	if(pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
				       dotv3(trace.plane.normal, up) < 0.7))
		return;

	/* copyv3 (pm->ps->origin, down_o);
	 * copyv3 (pm->ps->velocity, down_v); */

	copyv3 (start_o, up);
	up[2] += STEPSIZE;

	/* test the player position if they were a stepheight higher */
	pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum,
		pm->tracemask);
	if(trace.allsolid){
		if(pm->debugLevel)
			Com_Printf("%u:bend can't step\n", cnt);
		return;	/* can't step up */
	}

	stepSize = trace.endpos[2] - start_o[2];
	/* try slidemove from this position */
	copyv3 (trace.endpos, pm->ps->origin);
	copyv3 (start_v, pm->ps->velocity);

	PM_SlideMove(pm, pml, gravity);

	/* push down the final amount */
	copyv3 (pm->ps->origin, down);
	down[2] -= stepSize;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down,
		pm->ps->clientNum,
		pm->tracemask);
	if(!trace.allsolid)
		copyv3 (trace.endpos, pm->ps->origin);
	if(trace.fraction < 1.0)
		PM_ClipVelocity(pm->ps->velocity, trace.plane.normal,
			pm->ps->velocity,
			OVERCLIP);

#if 0
	/* if the down trace can trace back to the original position directly, don't step */
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, start_o,
		pm->ps->clientNum,
		pm->tracemask);
	if(trace.fraction == 1.0){
		/* use the original move */
		copyv3 (down_o, pm->ps->origin);
		copyv3 (down_v, pm->ps->velocity);
		if(pm->debugLevel)
			Com_Printf("%u:bend\n", cnt);
	}else
#endif
	{
		/* use the step move */
		float delta;

		delta = pm->ps->origin[2] - start_o[2];
		if(delta > 2){
			if(delta < 7)
				PM_AddEvent(pm, pml, EV_STEP_4);
			else if(delta < 11)
				PM_AddEvent(pm, pml, EV_STEP_8);
			else if(delta < 15)
				PM_AddEvent(pm, pml, EV_STEP_12);
			else
				PM_AddEvent(pm, pml, EV_STEP_16);
		}
		if(pm->debugLevel)
			Com_Printf("%u:stepped\n", cnt);
	}
}
