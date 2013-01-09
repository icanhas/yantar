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
} Winding;

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

Winding*AllocWinding(int points);
Scalar   WindingArea(Winding *w);
void    WindingCenter(Winding *w, Vec3 center);
void    ClipWindingEpsilon(Winding *in, Vec3 normal, Scalar dist,
			   Scalar epsilon, Winding **front, Winding **back);
Winding*ChopWinding(Winding *in, Vec3 normal, Scalar dist);
Winding*CopyWinding(Winding *w);
Winding*ReverseWinding(Winding *w);
Winding*BaseWindingForPlane(Vec3 normal, Scalar dist);
void    CheckWinding(Winding *w);
void    WindingPlane(Winding *w, Vec3 normal, Scalar *dist);
void    RemoveColinearPoints(Winding *w);
int             WindingOnPlaneSide(Winding *w, Vec3 normal, Scalar dist);
void    FreeWinding(Winding *w);
void    WindingBounds(Winding *w, Vec3 mins, Vec3 maxs);

void    AddWindingToConvexHull(Winding *w, Winding **hull, Vec3 normal);

void    ChopWindingInPlace(Winding **w, Vec3 normal, Scalar dist,
			   Scalar epsilon);
/* frees the original if clipped */

void pw(Winding *w);
