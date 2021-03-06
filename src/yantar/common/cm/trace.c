/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "local.h"

/* always use bbox vs. bbox collision and never capsule vs. bbox or vice versa
 * #define ALWAYS_BBOX_VS_BBOX
 * always use capsule vs. capsule collision and never capsule vs. bbox or vice versa
 * #define ALWAYS_CAPSULE_VS_CAPSULE */

void
TransposeMatrix(/*const*/ Vec3 matrix[3], Vec3 transpose[3])	/* FIXME */
{
	int i, j;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			transpose[i][j] = matrix[j][i];
}

void
CreateRotationMatrix(const Vec3 angles, Vec3 matrix[3])
{
	anglev3s(angles, matrix[0], matrix[1], matrix[2]);
	invv3(matrix[1]);
}

void
CM_ProjectPointOntoVector(Vec3 point, Vec3 vStart, Vec3 vDir, Vec3 vProj)
{
	Vec3 pVec;

	subv3(point, vStart, pVec);
	/* project onto the directional vector for this segment */
	saddv3(vStart, dotv3(pVec, vDir), vDir, vProj);
}

float
CM_distv3FromLineSquared(Vec3 p, Vec3 lp1, Vec3 lp2, Vec3 dir)
{
	Vec3	proj, t;
	int	j;

	CM_ProjectPointOntoVector(p, lp1, dir, proj);
	for(j = 0; j < 3; j++)
		if((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
		   (proj[j] < lp1[j] && proj[j] < lp2[j]))
			break;
	if(j < 3){
		if(fabs(proj[j] - lp1[j]) < fabs(proj[j] - lp2[j]))
			subv3(p, lp1, t);
		else
			subv3(p, lp2, t);
		return lensqrv3(t);
	}
	subv3(p, proj, t);
	return lensqrv3(t);
}

/*
 *	Position testing
 */

void
CM_TestBoxInBrush(Tracework *tw, cbrush_t *brush)
{
	int i;
	Cplane	*plane;
	float		dist;
	float		d1;
	cbrushside_t *side;
	float		t;
	Vec3		startp;

	if(!brush->numsides)
		return;

	/* special test for axial */
	if(tw->bounds[0][0] > brush->bounds[1][0]
	   || tw->bounds[0][1] > brush->bounds[1][1]
	   || tw->bounds[0][2] > brush->bounds[1][2]
	   || tw->bounds[1][0] < brush->bounds[0][0]
	   || tw->bounds[1][1] < brush->bounds[0][1]
	   || tw->bounds[1][2] < brush->bounds[0][2]
	   )
		return;

	if(tw->sphere.use)
		/* the first six planes are the axial planes, so we only
		 * need to test the remainder */
		for(i = 6; i < brush->numsides; i++){
			side = brush->sides + i;
			plane = side->plane;

			/* adjust the plane distance apropriately for radius */
			dist = plane->dist + tw->sphere.radius;
			/* find the closest point on the capsule to the plane */
			t = dotv3(plane->normal, tw->sphere.offset);
			if(t > 0)
				subv3(tw->start, tw->sphere.offset,
					startp);
			else
				addv3(tw->start, tw->sphere.offset, startp);
			d1 = dotv3(startp, plane->normal) - dist;
			/* if completely in front of face, no intersection */
			if(d1 > 0)
				return;
		}
	else
		/* the first six planes are the axial planes, so we only
		 * need to test the remainder */
		for(i = 6; i < brush->numsides; i++){
			side = brush->sides + i;
			plane = side->plane;

			/* adjust the plane distance apropriately for mins/maxs */
			dist = plane->dist -
			       dotv3(tw->offsets[ plane->signbits ],
				plane->normal);

			d1 = dotv3(tw->start, plane->normal) - dist;

			/* if completely in front of face, no intersection */
			if(d1 > 0)
				return;
		}

	/* inside this brush */
	tw->trace.startsolid = tw->trace.allsolid = qtrue;
	tw->trace.fraction = 0;
	tw->trace.contents = brush->contents;
}

void
CM_TestInLeaf(Tracework *tw, cLeaf_t *leaf)
{
	int	k;
	int	brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	/* test box position against all brushes in the leaf */
	for(k=0; k<leaf->numLeafBrushes; k++){
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];
		if(b->checkcount == cm.checkcount)
			continue;	/* already checked this brush in another leaf */
		b->checkcount = cm.checkcount;

		if(!(b->contents & tw->contents))
			continue;

		CM_TestBoxInBrush(tw, b);
		if(tw->trace.allsolid)
			return;
	}

	/* test against all patches */
#ifdef BSPC
	if(1){
#else
	if(!cm_noCurves->integer){
#endif	/* BSPC */
		for(k = 0; k < leaf->numLeafSurfaces; k++){
			patch =
				cm.surfaces[ cm.leafsurfaces[ leaf->
							      firstLeafSurface +
							      k ] ];
			if(!patch)
				continue;
			if(patch->checkcount == cm.checkcount)
				continue;	/* already checked this brush in another leaf */
			patch->checkcount = cm.checkcount;

			if(!(patch->contents & tw->contents))
				continue;

			if(CM_PositionTestInPatchCollide(tw, patch->pc)){
				tw->trace.startsolid = tw->trace.allsolid =
							       qtrue;
				tw->trace.fraction = 0;
				tw->trace.contents = patch->contents;
				return;
			}
		}
	}
}

/*
 * capsule inside capsule check
 */
void
CM_TestCapsuleInCapsule(Tracework *tw, Cliphandle model)
{
	int i;
	Vec3	mins, maxs;
	Vec3	top, bottom;
	Vec3	p1, p2, tmp;
	Vec3	offset, symetricSize[2];
	float	radius, halfwidth, halfheight, offs, r;

	CM_ModelBounds(model, mins, maxs);

	addv3(tw->start, tw->sphere.offset, top);
	subv3(tw->start, tw->sphere.offset, bottom);
	for(i = 0; i < 3; i++){
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
	}
	halfwidth = symetricSize[ 1 ][ 0 ];
	halfheight = symetricSize[ 1 ][ 2 ];
	radius	= (halfwidth > halfheight) ? halfheight : halfwidth;
	offs	= halfheight - radius;

	r = Square(tw->sphere.radius + radius);
	/* check if any of the spheres overlap */
	copyv3(offset, p1);
	p1[2] += offs;
	subv3(p1, top, tmp);
	if(lensqrv3(tmp) < r){
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	subv3(p1, bottom, tmp);
	if(lensqrv3(tmp) < r){
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	copyv3(offset, p2);
	p2[2] -= offs;
	subv3(p2, top, tmp);
	if(lensqrv3(tmp) < r){
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	subv3(p2, bottom, tmp);
	if(lensqrv3(tmp) < r){
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0;
	}
	/* if between cylinder up and lower bounds */
	if((top[2] >= p1[2] && top[2] <= p2[2]) ||
	   (bottom[2] >= p1[2] && bottom[2] <= p2[2])){
		/* 2d coordinates */
		top[2] = p1[2] = 0;
		/* if the cylinders overlap */
		subv3(top, p1, tmp);
		if(lensqrv3(tmp) < r){
			tw->trace.startsolid = tw->trace.allsolid = qtrue;
			tw->trace.fraction = 0;
		}
	}
}

/*
 * bounding box inside capsule check
 */
void
CM_TestBoundingBoxInCapsule(Tracework *tw, Cliphandle model)
{
	Vec3 mins, maxs, offset, size[2];
	Cliphandle h;
	Cmodel *cmod;
	int i;

	/* mins maxs of the capsule */
	CM_ModelBounds(model, mins, maxs);

	/* offset for capsule center */
	for(i = 0; i < 3; i++){
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		size[0][i] = mins[i] - offset[i];
		size[1][i] = maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	/* replace the bounding box with the capsule */
	tw->sphere.use = qtrue;
	tw->sphere.radius = (size[1][0] > size[1][2]) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	setv3(tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius);

	/* replace the capsule with the bounding box */
	h = CM_TempBoxModel(tw->size[0], tw->size[1], qfalse);
	/* calculate collision */
	cmod = CM_ClipHandleToModel(h);
	CM_TestInLeaf(tw, &cmod->leaf);
}

#define MAX_POSITION_LEAFS 1024
void
CM_PositionTest(Tracework *tw)
{
	int	leafs[MAX_POSITION_LEAFS];
	int	i;
	leafList_t ll;

	/* identify the leafs we are touching */
	addv3(tw->start, tw->size[0], ll.bounds[0]);
	addv3(tw->start, tw->size[1], ll.bounds[1]);

	for(i=0; i<3; i++){
		ll.bounds[0][i] -= 1;
		ll.bounds[1][i] += 1;
	}

	ll.count = 0;
	ll.maxcount = MAX_POSITION_LEAFS;
	ll.list = leafs;
	ll.storeLeafs	= CM_StoreLeafs;
	ll.lastLeaf	= 0;
	ll.overflowed	= qfalse;

	cm.checkcount++;

	CM_BoxLeafnums_r(&ll, 0);


	cm.checkcount++;

	/* test the contents of the leafs */
	for(i=0; i < ll.count; i++){
		CM_TestInLeaf(tw, &cm.leafs[leafs[i]]);
		if(tw->trace.allsolid)
			break;
	}
}

/*
 *	Tracing
 */

void
CM_TraceThroughPatch(Tracework *tw, cPatch_t *patch)
{
	float oldFrac;

	c_patch_traces++;

	oldFrac = tw->trace.fraction;

	CM_TraceThroughPatchCollide(tw, patch->pc);

	if(tw->trace.fraction < oldFrac){
		tw->trace.surfaceFlags = patch->surfaceFlags;
		tw->trace.contents = patch->contents;
	}
}

void
CM_TraceThroughBrush(Tracework *tw, cbrush_t *brush)
{
	int i;
	Cplane	*plane, *clipplane;
	float		dist;
	float		enterFrac, leaveFrac;
	float		d1, d2;
	qbool		getout, startout;
	float		f;
	cbrushside_t *side, *leadside;
	float		t;
	Vec3		startp;
	Vec3		endp;

	enterFrac = -1.0;
	leaveFrac = 1.0;
	clipplane = NULL;
	
	if((tw == nil) || (brush == nil) || (!brush->numsides))
		return;

	c_brush_traces++;

	getout = qfalse;
	startout = qfalse;

	leadside = NULL;

	if(tw->sphere.use)
		/*
		 * compare the trace against all planes of the brush
		 * find the latest time the trace crosses a plane towards the interior
		 * and the earliest time the trace crosses a plane towards the exterior
		 *  */
		for(i = 0; i < brush->numsides; i++){
			side = brush->sides + i;
			plane = side->plane;

			/* adjust the plane distance apropriately for radius */
			dist = plane->dist + tw->sphere.radius;

			/* find the closest point on the capsule to the plane */
			t = dotv3(plane->normal, tw->sphere.offset);
			if(t > 0){
				subv3(tw->start, tw->sphere.offset,
					startp);
				subv3(tw->end, tw->sphere.offset, endp);
			}else{
				addv3(tw->start, tw->sphere.offset, startp);
				addv3(tw->end, tw->sphere.offset, endp);
			}

			d1 = dotv3(startp, plane->normal) - dist;
			d2 = dotv3(endp, plane->normal) - dist;

			if(d2 > 0)
				getout = qtrue;		/* endpoint is not in solid */
			if(d1 > 0)
				startout = qtrue;

			/* if completely in front of face, no intersection with the entire brush */
			if(d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
				return;

			/* if it doesn't cross the plane, the plane isn't relevent */
			if(d1 <= 0 && d2 <= 0)
				continue;

			/* crosses face */
			if(d1 > d2){	/* enter */
				f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
				if(f < 0)
					f = 0;
				if(f > enterFrac){
					enterFrac = f;
					clipplane = plane;
					leadside = side;
				}
			}else{	/* leave */
				f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
				if(f > 1)
					f = 1;
				if(f < leaveFrac)
					leaveFrac = f;
			}
		}
	else
		/*
		 * compare the trace against all planes of the brush
		 * find the latest time the trace crosses a plane towards the interior
		 * and the earliest time the trace crosses a plane towards the exterior
		 *  */
		for(i = 0; i < brush->numsides; i++){
			side = brush->sides + i;
			plane = side->plane;

			/* adjust the plane distance apropriately for mins/maxs */
			dist = plane->dist -
			       dotv3(tw->offsets[ plane->signbits ],
				plane->normal);

			d1 = dotv3(tw->start, plane->normal) - dist;
			d2 = dotv3(tw->end, plane->normal) - dist;

			if(d2 > 0)
				getout = qtrue;		/* endpoint is not in solid */
			if(d1 > 0)
				startout = qtrue;

			/* if completely in front of face, no intersection with the entire brush */
			if(d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
				return;

			/* if it doesn't cross the plane, the plane isn't relevent */
			if(d1 <= 0 && d2 <= 0)
				continue;

			/* crosses face */
			if(d1 > d2){	/* enter */
				f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
				if(f < 0)
					f = 0;
				if(f > enterFrac){
					enterFrac = f;
					clipplane = plane;
					leadside = side;
				}
			}else{	/* leave */
				f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
				if(f > 1)
					f = 1;
				if(f < leaveFrac)
					leaveFrac = f;
			}
		}

	/*
	 * all planes have been checked, and the trace was not
	 * completely outside the brush
	 *  */
	if(!startout){	/* original point was inside brush */
		tw->trace.startsolid = qtrue;
		if(!getout){
			tw->trace.allsolid = qtrue;
			tw->trace.fraction = 0;
			tw->trace.contents = brush->contents;
		}
		return;
	}

	if(enterFrac < leaveFrac)
		if(enterFrac > -1 && enterFrac < tw->trace.fraction){
			if(enterFrac < 0)
				enterFrac = 0;
			tw->trace.fraction = enterFrac;
			if(clipplane != nil)
				tw->trace.plane = *clipplane;
			if(leadside != nil)
				tw->trace.surfaceFlags = leadside->surfaceFlags;
			tw->trace.contents = brush->contents;
		}
}

void
CM_TraceThroughLeaf(Tracework *tw, cLeaf_t *leaf)
{
	int	k;
	int	brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	/* trace line against all brushes in the leaf */
	for(k = 0; k < leaf->numLeafBrushes; k++){
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];

		b = &cm.brushes[brushnum];
		if(b->checkcount == cm.checkcount)
			continue;	/* already checked this brush in another leaf */
		b->checkcount = cm.checkcount;

		if(!(b->contents & tw->contents))
			continue;

		if(!CM_BoundsIntersect(tw->bounds[0], tw->bounds[1],
			   b->bounds[0], b->bounds[1]))
			continue;

		CM_TraceThroughBrush(tw, b);
		if(!tw->trace.fraction)
			return;
	}

	/* trace line against all patches in the leaf */
#ifdef BSPC
	if(1){
#else
	if(!cm_noCurves->integer){
#endif
		for(k = 0; k < leaf->numLeafSurfaces; k++){
			patch =
				cm.surfaces[ cm.leafsurfaces[ leaf->
							      firstLeafSurface +
							      k ] ];
			if(!patch)
				continue;
			if(patch->checkcount == cm.checkcount)
				continue;	/* already checked this patch in another leaf */
			patch->checkcount = cm.checkcount;

			if(!(patch->contents & tw->contents))
				continue;

			CM_TraceThroughPatch(tw, patch);
			if(!tw->trace.fraction)
				return;
		}
	}
}

#define RADIUS_EPSILON 1.0f

/*
 * get the first intersection of the ray with the sphere
 */
void
CM_TraceThroughSphere(Tracework *tw, Vec3 origin, float radius, Vec3 start,
		      Vec3 end)
{
	float	l1, l2, length, scale, fraction;
	/* float a; */
	float	b, c, d, sqrtd;
	Vec3	v1, dir, intersection;

	/* if inside the sphere */
	subv3(start, origin, dir);
	l1 = lensqrv3(dir);
	if(l1 < Square(radius)){
		tw->trace.fraction = 0;
		tw->trace.startsolid = qtrue;
		/* test for allsolid */
		subv3(end, origin, dir);
		l1 = lensqrv3(dir);
		if(l1 < Square(radius))
			tw->trace.allsolid = qtrue;
		return;
	}
	subv3(end, start, dir);
	length = normv3(dir);
	l1 = CM_distv3FromLineSquared(origin, start, end, dir);
	subv3(end, origin, v1);
	l2 = lensqrv3(v1);
	/* if no intersection with the sphere and the end point is at least an epsilon away */
	if(l1 >= Square(radius) && l2 > Square(radius+SURFACE_CLIP_EPSILON))
		return;
	/*
	 * | origin - (start + t * dir) | = radius
	 * a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	 * b = 2 * (dir[0] * (start[0] - origin[0]) + dir[1] * (start[1] - origin[1]) + dir[2] * (start[2] - origin[2]));
	 * c = (start[0] - origin[0])^2 + (start[1] - origin[1])^2 + (start[2] - origin[2])^2 - radius^2;
	 *  */
	subv3(start, origin, v1);
	/* dir is normalized so a = 1
	 * a = 1.0f;//dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]; */
	b = 2.0f * (dir[0] * v1[0] + dir[1] * v1[1] + dir[2] * v1[2]);
	c = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2] -
	    (radius+RADIUS_EPSILON) * (radius+RADIUS_EPSILON);

	d = b * b - 4.0f * c;	/* * a; */
	if(d > 0){
		sqrtd = sqrt(d);
		/* = (- b + sqrtd) * 0.5f; // / (2.0f * a); */
		fraction = (-b - sqrtd) * 0.5f;	/* / (2.0f * a); */
		if(fraction < 0)
			fraction = 0;
		else
			fraction /= length;
		if(fraction < tw->trace.fraction){
			tw->trace.fraction = fraction;
			subv3(end, start, dir);
			saddv3(start, fraction, dir, intersection);
			subv3(intersection, origin, dir);
			scale = 1 / (radius+RADIUS_EPSILON);
			scalev3(dir, scale, dir);
			copyv3(dir, tw->trace.plane.normal);
			addv3(tw->modelOrigin, intersection, intersection);
			tw->trace.plane.dist = dotv3(
				tw->trace.plane.normal, intersection);
			tw->trace.contents = CONTENTS_BODY;
		}
	}else if(d == 0){
		/* t1 = (- b ) / 2;
		 * slide along the sphere */
	}
	/* no intersection at all */
}

/*
 * get the first intersection of the ray with the cylinder
 * the cylinder extends halfheight above and below the origin
 */
void
CM_TraceThroughVerticalCylinder(Tracework *tw, Vec3 origin, float radius,
				float halfheight, Vec3 start,
				Vec3 end)
{
	float	length, scale, fraction, l1, l2;
	/* float a; */
	float	b, c, d, sqrtd;
	Vec3	v1, dir, start2d, end2d, org2d, intersection;

	/* 2d coordinates */
	setv3(start2d, start[0], start[1], 0);
	setv3(end2d, end[0], end[1], 0);
	setv3(org2d, origin[0], origin[1], 0);
	/* if between lower and upper cylinder bounds */
	if(start[2] <= origin[2] + halfheight &&
	   start[2] >= origin[2] - halfheight){
		/* if inside the cylinder */
		subv3(start2d, org2d, dir);
		l1 = lensqrv3(dir);
		if(l1 < Square(radius)){
			tw->trace.fraction = 0;
			tw->trace.startsolid = qtrue;
			subv3(end2d, org2d, dir);
			l1 = lensqrv3(dir);
			if(l1 < Square(radius))
				tw->trace.allsolid = qtrue;
			return;
		}
	}
	subv3(end2d, start2d, dir);
	length = normv3(dir);
	l1 = CM_distv3FromLineSquared(org2d, start2d, end2d, dir);
	subv3(end2d, org2d, v1);
	l2 = lensqrv3(v1);
	/* if no intersection with the cylinder and the end point is at least an epsilon away */
	if(l1 >= Square(radius) && l2 > Square(radius+SURFACE_CLIP_EPSILON))
		return;
	/*
	 *
	 * (start[0] - origin[0] - t * dir[0]) ^ 2 + (start[1] - origin[1] - t * dir[1]) ^ 2 = radius ^ 2
	 * (v1[0] + t * dir[0]) ^ 2 + (v1[1] + t * dir[1]) ^ 2 = radius ^ 2;
	 * v1[0] ^ 2 + 2 * v1[0] * t * dir[0] + (t * dir[0]) ^ 2 +
	 *                                      v1[1] ^ 2 + 2 * v1[1] * t * dir[1] + (t * dir[1]) ^ 2 = radius ^ 2
	 * t ^ 2 * (dir[0] ^ 2 + dir[1] ^ 2) + t * (2 * v1[0] * dir[0] + 2 * v1[1] * dir[1]) +
	 *                                      v1[0] ^ 2 + v1[1] ^ 2 - radius ^ 2 = 0
	 *  */
	subv3(start, origin, v1);
	/* dir is normalized so we can use a = 1
	 * a = 1.0f;// * (dir[0] * dir[0] + dir[1] * dir[1]); */
	b = 2.0f * (v1[0] * dir[0] + v1[1] * dir[1]);
	c = v1[0] * v1[0] + v1[1] * v1[1] -
	    (radius+RADIUS_EPSILON) * (radius+RADIUS_EPSILON);

	d = b * b - 4.0f * c;	/* * a; */
	if(d > 0){
		sqrtd = sqrt(d);
		/* = (- b + sqrtd) * 0.5f;// / (2.0f * a); */
		fraction = (-b - sqrtd) * 0.5f;	/* / (2.0f * a); */
		if(fraction < 0)
			fraction = 0;
		else
			fraction /= length;
		if(fraction < tw->trace.fraction){
			subv3(end, start, dir);
			saddv3(start, fraction, dir, intersection);
			/* if the intersection is between the cylinder lower and upper bound */
			if(intersection[2] <= origin[2] + halfheight &&
			   intersection[2] >= origin[2] - halfheight){
				tw->trace.fraction = fraction;
				subv3(intersection, origin, dir);
				dir[2] = 0;
				scale = 1 / (radius+RADIUS_EPSILON);
				scalev3(dir, scale, dir);
				copyv3(dir, tw->trace.plane.normal);
				addv3(tw->modelOrigin, intersection,
					intersection);
				tw->trace.plane.dist = dotv3(
					tw->trace.plane.normal, intersection);
				tw->trace.contents = CONTENTS_BODY;
			}
		}
	}else if(d == 0){
		/* t[0] = (- b ) / 2 * a;
		 * slide along the cylinder */
	}
	/* no intersection at all */
}

/*
 * capsule vs. capsule collision (not rotated)
 */
void
CM_TraceCapsuleThroughCapsule(Tracework *tw, Cliphandle model)
{
	int i;
	Vec3	mins, maxs;
	Vec3	top, bottom, starttop, startbottom, endtop, endbottom;
	Vec3	offset, symetricSize[2];
	float	radius, halfwidth, halfheight, offs, h;

	CM_ModelBounds(model, mins, maxs);
	/* test trace bounds vs. capsule bounds */
	if(tw->bounds[0][0] > maxs[0] + RADIUS_EPSILON
	   || tw->bounds[0][1] > maxs[1] + RADIUS_EPSILON
	   || tw->bounds[0][2] > maxs[2] + RADIUS_EPSILON
	   || tw->bounds[1][0] < mins[0] - RADIUS_EPSILON
	   || tw->bounds[1][1] < mins[1] - RADIUS_EPSILON
	   || tw->bounds[1][2] < mins[2] - RADIUS_EPSILON
	   )
		return;
	/* top origin and bottom origin of each sphere at start and end of trace */
	addv3(tw->start, tw->sphere.offset, starttop);
	subv3(tw->start, tw->sphere.offset, startbottom);
	addv3(tw->end, tw->sphere.offset, endtop);
	subv3(tw->end, tw->sphere.offset, endbottom);

	/* calculate top and bottom of the capsule spheres to collide with */
	for(i = 0; i < 3; i++){
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
	}
	halfwidth = symetricSize[ 1 ][ 0 ];
	halfheight = symetricSize[ 1 ][ 2 ];
	radius	= (halfwidth > halfheight) ? halfheight : halfwidth;
	offs	= halfheight - radius;
	copyv3(offset, top);
	top[2] += offs;
	copyv3(offset, bottom);
	bottom[2] -= offs;
	/* expand radius of spheres */
	radius += tw->sphere.radius;
	/* if there is horizontal movement */
	if(tw->start[0] != tw->end[0] || tw->start[1] != tw->end[1]){
		/* height of the expanded cylinder is the height of both cylinders minus the radius of both spheres */
		h = halfheight + tw->sphere.halfheight - radius;
		/* if the cylinder has a height */
		if(h > 0)
			/* test for collisions between the cylinders */
			CM_TraceThroughVerticalCylinder(tw, offset, radius, h,
				tw->start,
				tw->end);
	}
	/* test for collision between the spheres */
	CM_TraceThroughSphere(tw, top, radius, startbottom, endbottom);
	CM_TraceThroughSphere(tw, bottom, radius, starttop, endtop);
}

/*
 * bounding box vs. capsule collision
 */
void
CM_TraceBoundingBoxThroughCapsule(Tracework *tw, Cliphandle model)
{
	Vec3 mins, maxs, offset, size[2];
	Cliphandle h;
	Cmodel *cmod;
	int i;

	/* mins maxs of the capsule */
	CM_ModelBounds(model, mins, maxs);

	/* offset for capsule center */
	for(i = 0; i < 3; i++){
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		size[0][i] = mins[i] - offset[i];
		size[1][i] = maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	/* replace the bounding box with the capsule */
	tw->sphere.use = qtrue;
	tw->sphere.radius = (size[1][0] > size[1][2]) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	setv3(tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius);

	/* replace the capsule with the bounding box */
	h = CM_TempBoxModel(tw->size[0], tw->size[1], qfalse);
	/* calculate collision */
	cmod = CM_ClipHandleToModel(h);
	CM_TraceThroughLeaf(tw, &cmod->leaf);
}

/*
 * Traverse all the contacted leafs from the start to the end position.
 * If the trace is a point, they will be exactly in order, but for larger
 * trace volumes it is possible to hit something in a later leaf with
 * a smaller intercept fraction.
 */
void
CM_TraceThroughTree(Tracework *tw, int num, float p1f, float p2f, Vec3 p1,
		    Vec3 p2)
{
	cNode_t *node;
	Cplane        *plane;
	float	t1, t2, offset;
	float	frac, frac2;
	float	idist;
	Vec3	mid;
	int side;
	float	midf;

	if(tw->trace.fraction <= p1f)
		return;		/* already hit something nearer */

	/* if < 0, we are in a leaf node */
	if(num < 0){
		CM_TraceThroughLeaf(tw, &cm.leafs[-1-num]);
		return;
	}

	/*
	 * find the point distances to the seperating plane
	 * and the offset for the size of the box
	 *  */
	node = cm.nodes + num;
	plane = node->plane;

	/* adjust the plane distance apropriately for mins/maxs */
	if(plane->type < 3){
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = tw->extents[plane->type];
	}else{
		t1 = dotv3 (plane->normal, p1) - plane->dist;
		t2 = dotv3 (plane->normal, p2) - plane->dist;
		if(tw->isPoint)
			offset = 0;
		else
			/* this is silly */
			offset = 2048;
	}

	/* see which sides we need to consider */
	if(t1 >= offset + 1 && t2 >= offset + 1){
		CM_TraceThroughTree(tw, node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if(t1 < -offset - 1 && t2 < -offset - 1){
		CM_TraceThroughTree(tw, node->children[1], p1f, p2f, p1, p2);
		return;
	}

	/* put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side */
	if(t1 < t2){
		idist	= 1.0/(t1-t2);
		side	= 1;
		frac2	= (t1 + offset + SURFACE_CLIP_EPSILON)*idist;
		frac	= (t1 - offset + SURFACE_CLIP_EPSILON)*idist;
	}else if(t1 > t2){
		idist	= 1.0/(t1-t2);
		side	= 0;
		frac2	= (t1 - offset - SURFACE_CLIP_EPSILON)*idist;
		frac	= (t1 + offset + SURFACE_CLIP_EPSILON)*idist;
	}else{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	/* move up to the node */
	if(frac < 0)
		frac = 0;
	if(frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;

	mid[0]	= p1[0] + frac*(p2[0] - p1[0]);
	mid[1]	= p1[1] + frac*(p2[1] - p1[1]);
	mid[2]	= p1[2] + frac*(p2[2] - p1[2]);

	CM_TraceThroughTree(tw, node->children[side], p1f, midf, p1, mid);


	/* go past the node */
	if(frac2 < 0)
		frac2 = 0;
	if(frac2 > 1)
		frac2 = 1;

	midf = p1f + (p2f - p1f)*frac2;

	mid[0]	= p1[0] + frac2*(p2[0] - p1[0]);
	mid[1]	= p1[1] + frac2*(p2[1] - p1[1]);
	mid[2]	= p1[2] + frac2*(p2[2] - p1[2]);

	CM_TraceThroughTree(tw, node->children[side^1], midf, p2f, mid, p2);
}

void
CM_Trace(Trace *results, const Vec3 start, const Vec3 end, Vec3 mins,
	 Vec3 maxs,
	 Cliphandle model, const Vec3 origin, int brushmask, int capsule,
	 Sphere *sphere)
{
	int i;
	Tracework	tw;
	Vec3 offset;
	Cmodel	*cmod;

	cmod = CM_ClipHandleToModel(model);

	cm.checkcount++;	/* for multi-check avoidance */

	c_traces++;	/* for statistics, may be zeroed */

	/* fill in a default trace */
	Q_Memset(&tw, 0, sizeof(tw));
	tw.trace.fraction = 1;	/* assume it goes the entire distance until shown otherwise */
	copyv3(origin, tw.modelOrigin);

	if(!cm.numNodes){
		*results = tw.trace;

		return;	/* map not loaded, shouldn't happen */
	}

	/* allow NULL to be passed in for 0,0,0 */
	if(!mins)
		mins = vec3_origin;
	if(!maxs)
		maxs = vec3_origin;

	/* set basic parms */
	tw.contents = brushmask;

	/* adjust so that mins and maxs are always symmetric, which
	 * avoids some complications with plane expanding of rotated
	 * bmodels */
	for(i = 0; i < 3; i++){
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		tw.size[0][i] = mins[i] - offset[i];
		tw.size[1][i] = maxs[i] - offset[i];
		tw.start[i]	= start[i] + offset[i];
		tw.end[i] = end[i] + offset[i];
	}

	/* if a sphere is already specified */
	if(sphere)
		tw.sphere = *sphere;
	else{
		tw.sphere.use = capsule;
		tw.sphere.radius = min(tw.size[1][0], tw.size[1][2]);
		tw.sphere.halfheight = tw.size[1][2];
		setv3(tw.sphere.offset, 0, 0, tw.size[1][2] - tw.sphere.radius);
	}

	tw.maxOffset = tw.size[1][0] + tw.size[1][1] + tw.size[1][2];

	/* tw.offsets[signbits] = vector to appropriate corner from origin */
	tw.offsets[0][0] = tw.size[0][0];
	tw.offsets[0][1] = tw.size[0][1];
	tw.offsets[0][2] = tw.size[0][2];

	tw.offsets[1][0] = tw.size[1][0];
	tw.offsets[1][1] = tw.size[0][1];
	tw.offsets[1][2] = tw.size[0][2];

	tw.offsets[2][0] = tw.size[0][0];
	tw.offsets[2][1] = tw.size[1][1];
	tw.offsets[2][2] = tw.size[0][2];

	tw.offsets[3][0] = tw.size[1][0];
	tw.offsets[3][1] = tw.size[1][1];
	tw.offsets[3][2] = tw.size[0][2];

	tw.offsets[4][0] = tw.size[0][0];
	tw.offsets[4][1] = tw.size[0][1];
	tw.offsets[4][2] = tw.size[1][2];

	tw.offsets[5][0] = tw.size[1][0];
	tw.offsets[5][1] = tw.size[0][1];
	tw.offsets[5][2] = tw.size[1][2];

	tw.offsets[6][0] = tw.size[0][0];
	tw.offsets[6][1] = tw.size[1][1];
	tw.offsets[6][2] = tw.size[1][2];

	tw.offsets[7][0] = tw.size[1][0];
	tw.offsets[7][1] = tw.size[1][1];
	tw.offsets[7][2] = tw.size[1][2];

	/*
	 * calculate bounds
	 */
	if(tw.sphere.use){
		Scalar off, rad;

		rad = tw.sphere.radius;
		for(i = 0; i < 3; i++){
			off = fabs(tw.sphere.offset[i]);
			if(tw.start[i] < tw.end[i]){
				tw.bounds[0][i] = tw.start[i] - off - rad;
				tw.bounds[1][i] = tw.end[i] + off + rad;
			}else{
				tw.bounds[0][i] = tw.end[i] - off - rad;
				tw.bounds[1][i] = tw.start[i] + off + rad;
			}
		}
	}else{
		for(i = 0; i < 3; i++){
			if(tw.start[i] < tw.end[i]){
				tw.bounds[0][i] = tw.start[i] + tw.size[0][i];
				tw.bounds[1][i] = tw.end[i] + tw.size[1][i];
			}else{
				tw.bounds[0][i] = tw.end[i] + tw.size[0][i];
				tw.bounds[1][i] = tw.start[i] + tw.size[1][i];
			}
		}
	}
	/*
	 * check for position test special case
	 */
	if(start[0] == end[0] && start[1] == end[1] && start[2] == end[2]){
		if(model){
#ifdef ALWAYS_BBOX_VS_BBOX	/* FIXME - compile time flag? */
			if(model == BOX_MODEL_HANDLE || model ==
			   CAPSULE_MODEL_HANDLE){
				tw.sphere.use = qfalse;
				CM_TestInLeaf(&tw, &cmod->leaf);
			}else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
			if(model == BOX_MODEL_HANDLE || model ==
			   CAPSULE_MODEL_HANDLE)
				CM_TestCapsuleInCapsule(&tw, model);
			else
#endif
			if(model == CAPSULE_MODEL_HANDLE){
				if(tw.sphere.use)
					CM_TestCapsuleInCapsule(&tw, model);
				else
					CM_TestBoundingBoxInCapsule(&tw, model);
			}else
				CM_TestInLeaf(&tw, &cmod->leaf);
		}else
			CM_PositionTest(&tw);
	}else{
		/*
		 * check for point special case
		 */
		if(tw.size[0][0] == 0 && tw.size[0][1] == 0 && tw.size[0][2] == 0){
			tw.isPoint = qtrue;
			clearv3(tw.extents);
		}else{
			tw.isPoint = qfalse;
			tw.extents[0]	= tw.size[1][0];
			tw.extents[1]	= tw.size[1][1];
			tw.extents[2]	= tw.size[1][2];
		}

		/*
		 * general sweeping through world
		 */
		if(model){
#ifdef ALWAYS_BBOX_VS_BBOX
			if(model == BOX_MODEL_HANDLE || model ==
			   CAPSULE_MODEL_HANDLE){
				tw.sphere.use = qfalse;
				CM_TraceThroughLeaf(&tw, &cmod->leaf);
			}else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
			if(model == BOX_MODEL_HANDLE || model ==
			   CAPSULE_MODEL_HANDLE)
				CM_TraceCapsuleThroughCapsule(&tw, model);
			else
#endif
			if(model == CAPSULE_MODEL_HANDLE){
				if(tw.sphere.use)
					CM_TraceCapsuleThroughCapsule(&tw, model);
				else
					CM_TraceBoundingBoxThroughCapsule(&tw,
						model);
			}else
				CM_TraceThroughLeaf(&tw, &cmod->leaf);
		}else
			CM_TraceThroughTree(&tw, 0, 0, 1, tw.start, tw.end);
	}

	/* generate endpos from the original, unmodified start/end */
	if(tw.trace.fraction == 1)
		copyv3 (end, tw.trace.endpos);
	else
		for(i=0; i<3; i++)
			tw.trace.endpos[i] = start[i] + tw.trace.fraction *
					     (end[i] - start[i]);

	/* If allsolid is set (was entirely inside something solid), the plane is not valid.
	 * If fraction == 1.0, we never hit anything, and thus the plane is not valid.
	 * Otherwise, the normal on the plane should have unit length */
	assert(tw.trace.allsolid ||
		tw.trace.fraction == 1.0 ||
		lensqrv3(tw.trace.plane.normal) > 0.9999);
	*results = tw.trace;
}

void
CM_BoxTrace(Trace *results, const Vec3 start, const Vec3 end,
	    Vec3 mins, Vec3 maxs,
	    Cliphandle model, int brushmask, int capsule)
{
	CM_Trace(results, start, end, mins, maxs, model, vec3_origin, brushmask,
		capsule,
		NULL);
}

/*
 * Handles offseting and rotation of the end points for moving and
 * rotating entities
 */
void
CM_TransformedBoxTrace(Trace *results, const Vec3 start, const Vec3 end,
		       Vec3 mins, Vec3 maxs,
		       Cliphandle model, int brushmask,
		       const Vec3 origin, const Vec3 angles, int capsule)
{
	Trace		trace;
	Vec3		start_l, end_l;
	qbool		rotated;
	Vec3		offset;
	Vec3		symetricSize[2];
	Vec3		matrix[3], transpose[3];
	int i;
	float		halfwidth;
	float		halfheight;
	float		t;
	Sphere	sphere;

	if(!mins)
		mins = vec3_origin;
	if(!maxs)
		maxs = vec3_origin;

	/* adjust so that mins and maxs are always symetric, which
	 * avoids some complications with plane expanding of rotated
	 * bmodels */
	for(i = 0; i < 3; i++){
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
		start_l[i] = start[i] + offset[i];
		end_l[i] = end[i] + offset[i];
	}

	/* subtract origin offset */
	subv3(start_l, origin, start_l);
	subv3(end_l, origin, end_l);

	/* rotate start and end into the models frame of reference */
	if(model != BOX_MODEL_HANDLE &&
	   (angles[0] || angles[1] || angles[2]))
		rotated = qtrue;
	else
		rotated = qfalse;

	halfwidth = symetricSize[ 1 ][ 0 ];
	halfheight = symetricSize[ 1 ][ 2 ];

	sphere.use = capsule;
	sphere.radius = (halfwidth > halfheight) ? halfheight : halfwidth;
	sphere.halfheight = halfheight;
	t = halfheight - sphere.radius;

	if(rotated){
		/* rotation on trace line (start-end) instead of rotating the bmodel
		 * NOTE: This is still incorrect for bounding boxes because the actual bounding
		 *       box that is swept through the model is not rotated. We cannot rotate
		 *       the bounding box or the bmodel because that would make all the brush
		 *       bevels invalid.
		 *       However this is correct for capsules since a capsule itself is rotated too. */
		CreateRotationMatrix(angles, matrix);
		rotv3(start_l, matrix, start_l);
		rotv3(end_l, matrix, start_l);
		/* rotated sphere offset for capsule */
		sphere.offset[0] = matrix[0][ 2 ] * t;
		sphere.offset[1] = -matrix[1][ 2 ] * t;
		sphere.offset[2] = matrix[2][ 2 ] * t;
	}else
		setv3(sphere.offset, 0, 0, t);

	/* sweep the box through the model */
	CM_Trace(&trace, start_l, end_l, symetricSize[0], symetricSize[1], model,
		origin, brushmask, capsule,
		&sphere);

	/* if the bmodel was rotated and there was a collision */
	if(rotated && trace.fraction != 1.0){
		/* rotation of bmodel collision plane */
		TransposeMatrix(matrix, transpose);
		rotv3(trace.plane.normal, transpose, trace.plane.normal);
	}

	/* re-calculate the end position of the trace because the trace.endpos
	 * calculated by CM_Trace could be rotated and have an offset */
	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	*results = trace;
}

