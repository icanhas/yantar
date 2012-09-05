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

/* this is only used for visualization tools in cm_ debug functions */


#include "cm_local.h"


/* counters are only bumped when running single threaded,
 * because they are an awful coherence problem */
int	c_active_windings;
int	c_peak_windings;
int	c_winding_allocs;
int	c_winding_points;

void
pw(winding_t *w)
{
	int i;
	for(i=0; i<w->numpoints; i++)
		printf ("(%5.1f, %5.1f, %5.1f)\n",w->p[i][0], w->p[i][1],
			w->p[i][2]);
}


/*
 * AllocWinding
 */
winding_t       *
AllocWinding(int points)
{
	winding_t *w;
	int s;

	c_winding_allocs++;
	c_winding_points += points;
	c_active_windings++;
	if(c_active_windings > c_peak_windings)
		c_peak_windings = c_active_windings;

	s	= sizeof(Scalar)*3*points + sizeof(int);
	w	= Z_Malloc (s);
	Q_Memset (w, 0, s);
	return w;
}

void
FreeWinding(winding_t *w)
{
	if(*(unsigned*)w == 0xdeaddead)
		Com_Errorf (ERR_FATAL, "FreeWinding: freed a freed winding");
	*(unsigned*)w = 0xdeaddead;

	c_active_windings--;
	Z_Free (w);
}

/*
 * RemoveColinearPoints
 */
int c_removed;

void
RemoveColinearPoints(winding_t *w)
{
	int	i, j, k;
	Vec3 v1, v2;
	int	nump;
	Vec3 p[MAX_POINTS_ON_WINDING];

	nump = 0;
	for(i=0; i<w->numpoints; i++){
		j	= (i+1)%w->numpoints;
		k	= (i+w->numpoints-1)%w->numpoints;
		vec3sub (w->p[j], w->p[i], v1);
		vec3sub (w->p[i], w->p[k], v2);
		vec3normalize2(v1,v1);
		vec3normalize2(v2,v2);
		if(vec3dot(v1, v2) < 0.999){
			vec3copy (w->p[i], p[nump]);
			nump++;
		}
	}

	if(nump == w->numpoints)
		return;

	c_removed += w->numpoints - nump;
	w->numpoints = nump;
	Q_Memcpy (w->p, p, nump*sizeof(p[0]));
}

/*
 * WindingPlane
 */
void
WindingPlane(winding_t *w, Vec3 normal, Scalar *dist)
{
	Vec3 v1, v2;

	vec3sub (w->p[1], w->p[0], v1);
	vec3sub (w->p[2], w->p[0], v2);
	vec3cross (v2, v1, normal);
	vec3normalize2(normal, normal);
	*dist = vec3dot (w->p[0], normal);

}

/*
 * WindingArea
 */
Scalar
WindingArea(winding_t *w)
{
	int i;
	Vec3	d1, d2, cross;
	Scalar	total;

	total = 0;
	for(i=2; i<w->numpoints; i++){
		vec3sub (w->p[i-1], w->p[0], d1);
		vec3sub (w->p[i], w->p[0], d2);
		vec3cross (d1, d2, cross);
		total += 0.5 * vec3len (cross);
	}
	return total;
}

/*
 * WindingBounds
 */
void
WindingBounds(winding_t *w, Vec3 mins, Vec3 maxs)
{
	Scalar	v;
	int	i,j;

	mins[0] = mins[1] = mins[2] = MAX_MAP_BOUNDS;
	maxs[0] = maxs[1] = maxs[2] = -MAX_MAP_BOUNDS;

	for(i=0; i<w->numpoints; i++)
		for(j=0; j<3; j++){
			v = w->p[i][j];
			if(v < mins[j])
				mins[j] = v;
			if(v > maxs[j])
				maxs[j] = v;
		}
}

/*
 * WindingCenter
 */
void
WindingCenter(winding_t *w, Vec3 center)
{
	int i;
	float scale;

	vec3copy (vec3_origin, center);
	for(i=0; i<w->numpoints; i++)
		vec3add (w->p[i], center, center);

	scale = 1.0/w->numpoints;
	vec3scale (center, scale, center);
}

/*
 * BaseWindingForPlane
 */
winding_t *
BaseWindingForPlane(Vec3 normal, Scalar dist)
{
	int i, x;
	Scalar	max, v;
	Vec3	org, vright, vup;
	winding_t *w;

/* find the major axis */

	max	= -MAX_MAP_BOUNDS;
	x	= -1;
	for(i=0; i<3; i++){
		v = fabs(normal[i]);
		if(v > max){
			x	= i;
			max	= v;
		}
	}
	if(x==-1)
		Com_Errorf (ERR_DROP, "BaseWindingForPlane: no axis found");

	vec3copy (vec3_origin, vup);
	switch(x){
	case 0:
	case 1:
		vup[2] = 1;
		break;
	case 2:
		vup[0] = 1;
		break;
	}

	v = vec3dot (vup, normal);
	vec3ma (vup, -v, normal, vup);
	vec3normalize2(vup, vup);

	vec3scale (normal, dist, org);

	vec3cross (vup, normal, vright);

	vec3scale (vup, MAX_MAP_BOUNDS, vup);
	vec3scale (vright, MAX_MAP_BOUNDS, vright);

/* project a really big	axis aligned box onto the plane */
	w = AllocWinding (4);

	vec3sub (org, vright, w->p[0]);
	vec3add (w->p[0], vup, w->p[0]);

	vec3add (org, vright, w->p[1]);
	vec3add (w->p[1], vup, w->p[1]);

	vec3add (org, vright, w->p[2]);
	vec3sub (w->p[2], vup, w->p[2]);

	vec3sub (org, vright, w->p[3]);
	vec3sub (w->p[3], vup, w->p[3]);

	w->numpoints = 4;

	return w;
}

/*
 * CopyWinding
 */
winding_t       *
CopyWinding(winding_t *w)
{
	intptr_t size;
	winding_t *c;

	c = AllocWinding (w->numpoints);
	size = (intptr_t)((winding_t*)0)->p[w->numpoints];
	Q_Memcpy (c, w, size);
	return c;
}

/*
 * ReverseWinding
 */
winding_t       *
ReverseWinding(winding_t *w)
{
	int i;
	winding_t *c;

	c = AllocWinding (w->numpoints);
	for(i=0; i<w->numpoints; i++)
		vec3copy (w->p[w->numpoints-1-i], c->p[i]);
	c->numpoints = w->numpoints;
	return c;
}


/*
 * ClipWindingEpsilon
 */
void
ClipWindingEpsilon(winding_t *in, Vec3 normal, Scalar dist,
		   Scalar epsilon, winding_t **front, winding_t **back)
{
	Scalar	dists[MAX_POINTS_ON_WINDING+4];
	int	sides[MAX_POINTS_ON_WINDING+4];
	int	counts[3];
	static Scalar dot;	/* VC 4.2 optimizer bug if not static */
	int	i, j;
	Scalar	*p1, *p2;
	Vec3	mid;
	winding_t       *f, *b;
	int	maxpts;

	counts[0] = counts[1] = counts[2] = 0;

/* determine sides for each point */
	for(i=0; i<in->numpoints; i++){
		dot	= vec3dot (in->p[i], normal);
		dot	-= dist;
		dists[i] = dot;
		if(dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if(dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;
		counts[sides[i]]++;
	}
	sides[i]	= sides[0];
	dists[i]	= dists[0];

	*front = *back = NULL;

	if(!counts[0]){
		*back = CopyWinding (in);
		return;
	}
	if(!counts[1]){
		*front = CopyWinding (in);
		return;
	}

	maxpts = in->numpoints+4;	/* cant use counts[0]+2 because */
	/* of fp grouping errors */

	*front	= f = AllocWinding (maxpts);
	*back	= b = AllocWinding (maxpts);

	for(i=0; i<in->numpoints; i++){
		p1 = in->p[i];

		if(sides[i] == SIDE_ON){
			vec3copy (p1, f->p[f->numpoints]);
			f->numpoints++;
			vec3copy (p1, b->p[b->numpoints]);
			b->numpoints++;
			continue;
		}

		if(sides[i] == SIDE_FRONT){
			vec3copy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}
		if(sides[i] == SIDE_BACK){
			vec3copy (p1, b->p[b->numpoints]);
			b->numpoints++;
		}

		if(sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		/* generate a split point */
		p2 = in->p[(i+1)%in->numpoints];

		dot = dists[i] / (dists[i]-dists[i+1]);
		for(j=0; j<3; j++){	/* avoid round off error when possible */
			if(normal[j] == 1)
				mid[j] = dist;
			else if(normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}

		vec3copy (mid, f->p[f->numpoints]);
		f->numpoints++;
		vec3copy (mid, b->p[b->numpoints]);
		b->numpoints++;
	}

	if(f->numpoints > maxpts || b->numpoints > maxpts)
		Com_Errorf (ERR_DROP, "ClipWinding: points exceeded estimate");
	if(f->numpoints > MAX_POINTS_ON_WINDING || b->numpoints >
	   MAX_POINTS_ON_WINDING)
		Com_Errorf (ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING");
}


/*
 * ChopWindingInPlace
 */
void
ChopWindingInPlace(winding_t **inout, Vec3 normal, Scalar dist, Scalar epsilon)
{
	winding_t *in;
	Scalar	dists[MAX_POINTS_ON_WINDING+4];
	int	sides[MAX_POINTS_ON_WINDING+4];
	int	counts[3];
	static Scalar dot;	/* VC 4.2 optimizer bug if not static */
	int	i, j;
	Scalar   *p1, *p2;
	Vec3	mid;
	winding_t       *f;
	int	maxpts;

	in = *inout;
	counts[0] = counts[1] = counts[2] = 0;

/* determine sides for each point */
	for(i=0; i<in->numpoints; i++){
		dot	= vec3dot (in->p[i], normal);
		dot	-= dist;
		dists[i] = dot;
		if(dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if(dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;
		counts[sides[i]]++;
	}
	sides[i]	= sides[0];
	dists[i]	= dists[0];

	if(!counts[0]){
		FreeWinding (in);
		*inout = NULL;
		return;
	}
	if(!counts[1])
		return;		/* inout stays the same */

	maxpts = in->numpoints+4;	/* cant use counts[0]+2 because */
	/* of fp grouping errors */

	f = AllocWinding (maxpts);

	for(i=0; i<in->numpoints; i++){
		p1 = in->p[i];

		if(sides[i] == SIDE_ON){
			vec3copy (p1, f->p[f->numpoints]);
			f->numpoints++;
			continue;
		}

		if(sides[i] == SIDE_FRONT){
			vec3copy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}

		if(sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		/* generate a split point */
		p2 = in->p[(i+1)%in->numpoints];

		dot = dists[i] / (dists[i]-dists[i+1]);
		for(j=0; j<3; j++){	/* avoid round off error when possible */
			if(normal[j] == 1)
				mid[j] = dist;
			else if(normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}

		vec3copy (mid, f->p[f->numpoints]);
		f->numpoints++;
	}

	if(f->numpoints > maxpts)
		Com_Errorf (ERR_DROP, "ClipWinding: points exceeded estimate");
	if(f->numpoints > MAX_POINTS_ON_WINDING)
		Com_Errorf (ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING");

	FreeWinding (in);
	*inout = f;
}


/*
 * ChopWinding
 *
 * Returns the fragment of in that is on the front side
 * of the cliping plane.  The original is freed.
 */
winding_t       *
ChopWinding(winding_t *in, Vec3 normal, Scalar dist)
{
	winding_t *f, *b;

	ClipWindingEpsilon (in, normal, dist, ON_EPSILON, &f, &b);
	FreeWinding (in);
	if(b)
		FreeWinding (b);
	return f;
}


/*
 * CheckWinding
 *
 */
void
CheckWinding(winding_t *w)
{
	int i, j;
	Scalar	*p1, *p2;
	Scalar	d, edgedist;
	Vec3	dir, edgenormal, facenormal;
	Scalar	area;
	Scalar	facedist;

	if(w->numpoints < 3)
		Com_Errorf (ERR_DROP, "CheckWinding: %i points",w->numpoints);

	area = WindingArea(w);
	if(area < 1)
		Com_Errorf (ERR_DROP, "CheckWinding: %f area", area);

	WindingPlane (w, facenormal, &facedist);

	for(i=0; i<w->numpoints; i++){
		p1 = w->p[i];

		for(j=0; j<3; j++)
			if(p1[j] > MAX_MAP_BOUNDS || p1[j] < -MAX_MAP_BOUNDS)
				Com_Errorf (ERR_DROP,
					"CheckFace: BUGUS_RANGE: %f",
					p1[j]);

		j = i+1 == w->numpoints ? 0 : i+1;

		/* check the point is on the face plane */
		d = vec3dot (p1, facenormal) - facedist;
		if(d < -ON_EPSILON || d > ON_EPSILON)
			Com_Errorf (ERR_DROP, "CheckWinding: point off plane");

		/* check the edge isnt degenerate */
		p2 = w->p[j];
		vec3sub (p2, p1, dir);

		if(vec3len (dir) < ON_EPSILON)
			Com_Errorf (ERR_DROP, "CheckWinding: degenerate edge");

		vec3cross (facenormal, dir, edgenormal);
		vec3normalize2 (edgenormal, edgenormal);
		edgedist	= vec3dot (p1, edgenormal);
		edgedist	+= ON_EPSILON;

		/* all other points must be on front side */
		for(j=0; j<w->numpoints; j++){
			if(j == i)
				continue;
			d = vec3dot (w->p[j], edgenormal);
			if(d > edgedist)
				Com_Errorf (ERR_DROP, "CheckWinding: non-convex");
		}
	}
}


/*
 * WindingOnPlaneSide
 */
int
WindingOnPlaneSide(winding_t *w, Vec3 normal, Scalar dist)
{
	qbool		front, back;
	int i;
	Scalar		d;

	front	= qfalse;
	back	= qfalse;
	for(i=0; i<w->numpoints; i++){
		d = vec3dot (w->p[i], normal) - dist;
		if(d < -ON_EPSILON){
			if(front)
				return SIDE_CROSS;
			back = qtrue;
			continue;
		}
		if(d > ON_EPSILON){
			if(back)
				return SIDE_CROSS;
			front = qtrue;
			continue;
		}
	}

	if(back)
		return SIDE_BACK;
	if(front)
		return SIDE_FRONT;
	return SIDE_ON;
}


/*
 * AddWindingToConvexHull
 *
 * Both w and *hull are on the same plane
 */
#define MAX_HULL_POINTS 128
void
AddWindingToConvexHull(winding_t *w, winding_t **hull, Vec3 normal)
{
	int i, j, k;
	float	*p, *copy;
	Vec3	dir;
	float	d;
	int	numHullPoints, numNew;
	Vec3	hullPoints[MAX_HULL_POINTS];
	Vec3	newHullPoints[MAX_HULL_POINTS];
	Vec3	hullDirs[MAX_HULL_POINTS];
	qbool		hullSide[MAX_HULL_POINTS];
	qbool		outside;

	if(!*hull){
		*hull = CopyWinding(w);
		return;
	}

	numHullPoints = (*hull)->numpoints;
	Q_Memcpy(hullPoints, (*hull)->p, numHullPoints * sizeof(Vec3));

	for(i = 0; i < w->numpoints; i++){
		p = w->p[i];

		/* calculate hull side vectors */
		for(j = 0; j < numHullPoints; j++){
			k = (j + 1) % numHullPoints;

			vec3sub(hullPoints[k], hullPoints[j], dir);
			vec3normalize2(dir, dir);
			vec3cross(normal, dir, hullDirs[j]);
		}

		outside = qfalse;
		for(j = 0; j < numHullPoints; j++){
			vec3sub(p, hullPoints[j], dir);
			d = vec3dot(dir, hullDirs[j]);
			if(d >= ON_EPSILON)
				outside = qtrue;
			if(d >= -ON_EPSILON)
				hullSide[j] = qtrue;
			else
				hullSide[j] = qfalse;
		}

		/* if the point is effectively inside, do nothing */
		if(!outside)
			continue;

		/* find the back side to front side transition */
		for(j = 0; j < numHullPoints; j++)
			if(!hullSide[ j % numHullPoints ] &&
			   hullSide[ (j + 1) % numHullPoints ])
				break;
		if(j == numHullPoints)
			continue;

		/* insert the point here */
		vec3copy(p, newHullPoints[0]);
		numNew = 1;

		/* copy over all points that aren't double fronts */
		j = (j+1)%numHullPoints;
		for(k = 0; k < numHullPoints; k++){
			if(hullSide[ (j+k) % numHullPoints ] &&
			   hullSide[ (j+k+1) % numHullPoints ])
				continue;
			copy = hullPoints[ (j+k+1) % numHullPoints ];
			vec3copy(copy, newHullPoints[numNew]);
			numNew++;
		}

		numHullPoints = numNew;
		Q_Memcpy(hullPoints, newHullPoints, numHullPoints *
			sizeof(Vec3));
	}

	FreeWinding(*hull);
	w = AllocWinding(numHullPoints);
	w->numpoints = numHullPoints;
	*hull = w;
	Q_Memcpy(w->p, hullPoints, numHullPoints * sizeof(Vec3));
}
