/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/* this is only used for visualization tools in cm_ debug functions */

typedef struct {
	int	numpoints;
	Vec3	p[4];	/* variable sized */
} winding_t;

#define MAX_POINTS_ON_WINDING	64

#define SIDE_FRONT		0
#define SIDE_BACK		1
#define SIDE_ON			2
#define SIDE_CROSS		3

#define CLIP_EPSILON		0.1f

#define MAX_MAP_BOUNDS		65535

/* you can define on_epsilon in the makefile as tighter */
#ifndef ON_EPSILON
#define ON_EPSILON 0.1f
#endif

winding_t*AllocWinding(int points);
Scalar   WindingArea(winding_t *w);
void    WindingCenter(winding_t *w, Vec3 center);
void    ClipWindingEpsilon(winding_t *in, Vec3 normal, Scalar dist,
			   Scalar epsilon, winding_t **front, winding_t **back);
winding_t*ChopWinding(winding_t *in, Vec3 normal, Scalar dist);
winding_t*CopyWinding(winding_t *w);
winding_t*ReverseWinding(winding_t *w);
winding_t*BaseWindingForPlane(Vec3 normal, Scalar dist);
void    CheckWinding(winding_t *w);
void    WindingPlane(winding_t *w, Vec3 normal, Scalar *dist);
void    RemoveColinearPoints(winding_t *w);
int             WindingOnPlaneSide(winding_t *w, Vec3 normal, Scalar dist);
void    FreeWinding(winding_t *w);
void    WindingBounds(winding_t *w, Vec3 mins, Vec3 maxs);

void    AddWindingToConvexHull(winding_t *w, winding_t **hull, Vec3 normal);

void    ChopWindingInPlace(winding_t **w, Vec3 normal, Scalar dist,
			   Scalar epsilon);
/* frees the original if clipped */

void pw(winding_t *w);
