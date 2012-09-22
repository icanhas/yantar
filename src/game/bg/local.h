/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 * bg_local.h -- local definitions for the bg (both games) files */

#define MIN_WALK_NORMAL 0.7f	/* can't walk on very steep slopes */

#define STEPSIZE	18

#define JUMP_VELOCITY	270

#define TIMER_LAND	130
#define TIMER_GESTURE	(34*66+50)

#define OVERCLIP	1.001f

/* all of the locals will be zeroed before each
 * pmove, just to make damn sure we don't have
 * any differences when running on client or server */
typedef struct {
	Vec3		forward, right, up;
	float		frametime;

	int		msec;

	qbool		walking;
	qbool		groundPlane;
	trace_t		groundTrace;

	float		impactSpeed;

	Vec3		previous_origin;
	Vec3		previous_velocity;
	int		previous_waterlevel;
} pml_t;

extern pmove_t	*pm;
extern pml_t	pml;

/* movement parameters */
extern float	pm_stopspeed;
extern float	pm_duckScale;
extern float	pm_swimScale;

extern float	pm_accelerate;
extern float	pm_airaccelerate;
extern float	pm_wateraccelerate;
extern float	pm_flyaccelerate;

extern float	pm_friction;
extern float	pm_waterfriction;
extern float	pm_flightfriction;

extern uint	cnt;

void PM_ClipVelocity(Vec3 in, Vec3 normal, Vec3 out, float overbounce);
void PM_AddTouchEnt(int entityNum);
void PM_AddEvent(int newEvent);

qbool        PM_SlideMove(qbool gravity);
void            PM_StepSlideMove(qbool gravity);
