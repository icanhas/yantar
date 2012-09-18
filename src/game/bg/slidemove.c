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
/*
 * bg_slidemove.c -- part of bg_pmove functionality */

#include "q_shared.h"
#include "public.h"
#include "local.h"

/*
 *
 * input: origin, velocity, bounds, groundPlane, trace function
 *
 * output: origin, velocity, impacts, stairup boolean
 *
 */

/*
 * PM_SlideMove
 *
 * Returns qtrue if the velocity was clipped in some way
 */
#define MAX_CLIP_PLANES 5
qbool
PM_SlideMove(qbool gravity)
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

	vec3copy (pm->ps->velocity, primal_velocity);

	if(gravity){
		vec3copy(pm->ps->velocity, endVelocity);
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] =
			(pm->ps->velocity[2] + endVelocity[2]) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if(pml.groundPlane)
			/* slide along the ground plane */
			PM_ClipVelocity (pm->ps->velocity,
				pml.groundTrace.plane.normal,
				pm->ps->velocity,
				OVERCLIP);
	}

	time_left = pml.frametime;

	/* never turn against the ground plane */
	if(pml.groundPlane){
		numplanes = 1;
		vec3copy(pml.groundTrace.plane.normal, planes[0]);
	}else
		numplanes = 0;

	/* never turn against original velocity */
	vec3normalize2(pm->ps->velocity, planes[numplanes]);
	numplanes++;

	for(bumpcount=0; bumpcount < numbumps; bumpcount++){

		/* calculate position we are trying to move to */
		vec3ma(pm->ps->origin, time_left, pm->ps->velocity, end);

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
			vec3copy (trace.endpos, pm->ps->origin);

		if(trace.fraction == 1)
			break;	/* moved the entire distance */

		/* save entity for contact */
		PM_AddTouchEnt(trace.entityNum);

		time_left -= time_left * trace.fraction;

		if(numplanes >= MAX_CLIP_PLANES){
			/* this shouldn't really happen */
			vec3clear(pm->ps->velocity);
			return qtrue;
		}

		/*
		 * if this is the same plane we hit before, nudge velocity
		 * out along it, which fixes some epsilon issues with
		 * non-axial planes
		 *  */
		for(i = 0; i < numplanes; i++)
			if(vec3dot(trace.plane.normal, planes[i]) > 0.99){
				vec3add(trace.plane.normal, pm->ps->velocity,
					pm->ps->velocity);
				break;
			}
		if(i < numplanes)
			continue;
		vec3copy (trace.plane.normal, planes[numplanes]);
		numplanes++;

		/*
		 * modify velocity so it parallels all of the clip planes
		 *  */

		/* find a plane that it enters */
		for(i = 0; i < numplanes; i++){
			into = vec3dot(pm->ps->velocity, planes[i]);
			if(into >= 0.1)
				continue;	/* move doesn't interact with the plane */

			/* see how hard we are hitting things */
			if(-into > pml.impactSpeed)
				pml.impactSpeed = -into;

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
				if(vec3dot(clipVelocity, planes[j]) >= 0.1)
					continue;	/* move doesn't interact with the plane */

				/* try clipping the move to the plane */
				PM_ClipVelocity(clipVelocity, planes[j],
					clipVelocity,
					OVERCLIP);
				PM_ClipVelocity(endClipVelocity, planes[j],
					endClipVelocity,
					OVERCLIP);

				/* see if it goes back into the first clip plane */
				if(vec3dot(clipVelocity, planes[i]) >= 0)
					continue;

				/* slide the original velocity along the crease */
				vec3cross (planes[i], planes[j], dir);
				vec3normalize(dir);
				d = vec3dot(dir, pm->ps->velocity);
				vec3scale(dir, d, clipVelocity);

				vec3cross (planes[i], planes[j], dir);
				vec3normalize(dir);
				d = vec3dot(dir, endVelocity);
				vec3scale(dir, d, endClipVelocity);

				/* see if there is a third plane the the new move enters */
				for(k = 0; k < numplanes; k++){
					if(k == i || k == j)
						continue;
					if(vec3dot(clipVelocity,
						   planes[k]) >= 0.1)
						continue;	/* move doesn't interact with the plane */

					/* stop dead at a tripple plane interaction */
					vec3clear(pm->ps->velocity);
					return qtrue;
				}
			}

			/* if we have fixed all interactions, try another move */
			vec3copy(clipVelocity, pm->ps->velocity);
			vec3copy(endClipVelocity, endVelocity);
			break;
		}
	}

	if(gravity)
		vec3copy(endVelocity, pm->ps->velocity);

	/* don't change velocity if in a timer (FIXME: is this correct?) */
	if(pm->ps->pm_time)
		vec3copy(primal_velocity, pm->ps->velocity);

	return (bumpcount != 0);
}

/*
 * PM_StepSlideMove
 *
 */
void
PM_StepSlideMove(qbool gravity)
{
	Vec3	start_o, start_v;
/*	Vec3		down_o, down_v; */
	trace_t trace;
/* float		down_dist, up_dist;
 * Vec3		delta, delta2; */
	Vec3	up, down;
	float	stepSize;

	vec3copy (pm->ps->origin, start_o);
	vec3copy (pm->ps->velocity, start_v);

	if(PM_SlideMove(gravity) == 0)
		return;		/* we got exactly where we wanted to go first try */

	vec3copy(start_o, down);
	down[2] -= STEPSIZE;
	pm->trace (&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum,
		pm->tracemask);
	vec3set(up, 0, 0, 1);
	/* never step up when you still have up velocity */
	if(pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
				       vec3dot(trace.plane.normal, up) < 0.7))
		return;

	/* vec3copy (pm->ps->origin, down_o);
	 * vec3copy (pm->ps->velocity, down_v); */

	vec3copy (start_o, up);
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
	vec3copy (trace.endpos, pm->ps->origin);
	vec3copy (start_v, pm->ps->velocity);

	PM_SlideMove(gravity);

	/* push down the final amount */
	vec3copy (pm->ps->origin, down);
	down[2] -= stepSize;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down,
		pm->ps->clientNum,
		pm->tracemask);
	if(!trace.allsolid)
		vec3copy (trace.endpos, pm->ps->origin);
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
		vec3copy (down_o, pm->ps->origin);
		vec3copy (down_v, pm->ps->velocity);
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
				PM_AddEvent(EV_STEP_4);
			else if(delta < 11)
				PM_AddEvent(EV_STEP_8);
			else if(delta < 15)
				PM_AddEvent(EV_STEP_12);
			else
				PM_AddEvent(EV_STEP_16);
		}
		if(pm->debugLevel)
			Com_Printf("%u:stepped\n", cnt);
	}
}
