/* main control flow for each frame */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include <string.h>	/* memcpy */
#include "local.h"

Refimport ri;
trGlobals_t tr;
static float s_flipMatrix[16] = {
	/* convert from our coordinate system (looking down X)
	 * to OpenGL's coordinate system (looking down -Z) */
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

/* entities that will have procedurally generated surfaces will just
 * point at this for their sorting surface */
surfaceType_t entitySurface = SF_ENTITY;

/*
 * R_CullLocalBox
 *
 * Returns CULL_IN, CULL_CLIP, or CULL_OUT
 */
int
R_CullLocalBox(Vec3 bounds[2])
{
	int i, j;
	Vec3	transformed[8];
	float	dists[8];
	Vec3	v;
	Cplane        *frust;
	int	anyBack;
	int	front, back;

	if(r_nocull->integer){
		return CULL_CLIP;
	}

	/* transform into world space */
	for(i = 0; i < 8; i++){
		v[0] = bounds[i&1][0];
		v[1] = bounds[(i>>1)&1][1];
		v[2] = bounds[(i>>2)&1][2];

		copyv3(tr.or.origin, transformed[i]);
		saddv3(transformed[i], v[0], tr.or.axis[0], transformed[i]);
		saddv3(transformed[i], v[1], tr.or.axis[1], transformed[i]);
		saddv3(transformed[i], v[2], tr.or.axis[2], transformed[i]);
	}

	/* check against frustum planes */
	anyBack = 0;
	for(i = 0; i < 4; i++){
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for(j = 0; j < 8; j++){
			dists[j] = dotv3(transformed[j], frust->normal);
			if(dists[j] > frust->dist){
				front = 1;
				if(back){
					break;	/* a point is in front */
				}
			}else{
				back = 1;
			}
		}
		if(!front){
			/* all points were behind one of the planes */
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if(!anyBack){
		return CULL_IN;	/* completely inside frustum */
	}

	return CULL_CLIP;	/* partially clipped */
}

/*
** R_CullLocalPointAndRadius
*/
int
R_CullLocalPointAndRadius(Vec3 pt, float radius)
{
	Vec3 transformed;

	R_LocalPointToWorld(pt, transformed);

	return R_CullPointAndRadius(transformed, radius);
}

/*
** R_CullPointAndRadius
*/
int
R_CullPointAndRadius(Vec3 pt, float radius)
{
	int i;
	float dist;
	Cplane *frust;
	qbool mightBeClipped = qfalse;

	if(r_nocull->integer){
		return CULL_CLIP;
	}

	/* check against frustum planes */
	for(i = 0; i < 4; i++){
		frust = &tr.viewParms.frustum[i];

		dist = dotv3(pt, frust->normal) - frust->dist;
		if(dist < -radius){
			return CULL_OUT;
		}else if(dist <= radius){
			mightBeClipped = qtrue;
		}
	}

	if(mightBeClipped){
		return CULL_CLIP;
	}

	return CULL_IN;	/* completely inside frustum */
}


/*
 * R_LocalNormalToWorld
 *
 */
void
R_LocalNormalToWorld(Vec3 local, Vec3 world)
{
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] *
		   tr.or.axis[2][0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] *
		   tr.or.axis[2][1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] *
		   tr.or.axis[2][2];
}

/*
 * R_LocalPointToWorld
 *
 */
void
R_LocalPointToWorld(Vec3 local, Vec3 world)
{
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] *
		   tr.or.axis[2][0] + tr.or.origin[0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] *
		   tr.or.axis[2][1] + tr.or.origin[1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] *
		   tr.or.axis[2][2] + tr.or.origin[2];
}

/*
 * R_WorldToLocal
 *
 */
void
R_WorldToLocal(Vec3 world, Vec3 local)
{
	local[0] = dotv3(world, tr.or.axis[0]);
	local[1] = dotv3(world, tr.or.axis[1]);
	local[2] = dotv3(world, tr.or.axis[2]);
}

/*
 * R_TransformModelToClip
 *
 */
void
R_TransformModelToClip(const Vec3 src, const float *modelMatrix, const float *projectionMatrix,
		       Vec4 eye, Vec4 dst)
{
	int i;

	for(i = 0; i < 4; i++)
		eye[i] =
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];

	for(i = 0; i < 4; i++)
		dst[i] =
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
}

/*
 * R_TransformClipToWindow
 *
 */
void
R_TransformClipToWindow(const Vec4 clip, const viewParms_t *view, Vec4 normalized, Vec4 window)
{
	normalized[0]	= clip[0] / clip[3];
	normalized[1]	= clip[1] / clip[3];
	normalized[2]	= (clip[2] + clip[3]) / (2 * clip[3]);

	window[0] = 0.5f * (1.0f + normalized[0]) * view->viewportWidth;
	window[1] = 0.5f * (1.0f + normalized[1]) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int)(window[0] + 0.5);
	window[1] = (int)(window[1] + 0.5);
}


/*
 * myGlMultMatrix
 *
 */
void
myGlMultMatrix(const float *a, const float *b, float *out)
{
	int i, j;

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
}

/*
 * R_RotateForEntity
 *
 * Generates an orientation for an entity and viewParms
 * Does NOT produce any GL calls
 * Called by both the front end and the back end
 */
void
R_RotateForEntity(const trRefEntity_t *ent, const viewParms_t *viewParms,
		  orientationr_t *or)
{
	float glMatrix[16];
	Vec3	delta;
	float	axisLength;

	if(ent->e.reType != RT_MODEL){
		*or = viewParms->world;
		return;
	}

	copyv3(ent->e.origin, or->origin);

	copyv3(ent->e.axis[0], or->axis[0]);
	copyv3(ent->e.axis[1], or->axis[1]);
	copyv3(ent->e.axis[2], or->axis[2]);

	glMatrix[0] = or->axis[0][0];
	glMatrix[4] = or->axis[1][0];
	glMatrix[8] = or->axis[2][0];
	glMatrix[12] = or->origin[0];

	glMatrix[1] = or->axis[0][1];
	glMatrix[5] = or->axis[1][1];
	glMatrix[9] = or->axis[2][1];
	glMatrix[13] = or->origin[1];

	glMatrix[2] = or->axis[0][2];
	glMatrix[6] = or->axis[1][2];
	glMatrix[10] = or->axis[2][2];
	glMatrix[14] = or->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix(glMatrix, viewParms->world.modelMatrix, or->modelMatrix);

	/* calculate the viewer origin in the model's space
	 * needed for fog, specular, and environment mapping */
	subv3(viewParms->or.origin, or->origin, delta);

	/* compensate for scale in the axes if necessary */
	if(ent->e.nonNormalizedAxes){
		axisLength = lenv3(ent->e.axis[0]);
		if(!axisLength){
			axisLength = 0;
		}else{
			axisLength = 1.0f / axisLength;
		}
	}else{
		axisLength = 1.0f;
	}

	or->viewOrigin[0] = dotv3(delta, or->axis[0]) * axisLength;
	or->viewOrigin[1] = dotv3(delta, or->axis[1]) * axisLength;
	or->viewOrigin[2] = dotv3(delta, or->axis[2]) * axisLength;
}

/*
 * R_RotateForViewer
 *
 * Sets up the modelview matrix for a given viewParm
 */
void
R_RotateForViewer(void)
{
	float viewerMatrix[16];
	Vec3 origin;

	Q_Memset (&tr.or, 0, sizeof(tr.or));
	tr.or.axis[0][0] = 1;
	tr.or.axis[1][1] = 1;
	tr.or.axis[2][2] = 1;
	copyv3 (tr.viewParms.or.origin, tr.or.viewOrigin);

	/* transform by the camera placement */
	copyv3(tr.viewParms.or.origin, origin);

	viewerMatrix[0] = tr.viewParms.or.axis[0][0];
	viewerMatrix[4] = tr.viewParms.or.axis[0][1];
	viewerMatrix[8] = tr.viewParms.or.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] *
			   viewerMatrix[8];

	viewerMatrix[1] = tr.viewParms.or.axis[1][0];
	viewerMatrix[5] = tr.viewParms.or.axis[1][1];
	viewerMatrix[9] = tr.viewParms.or.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] *
			   viewerMatrix[9];

	viewerMatrix[2] = tr.viewParms.or.axis[2][0];
	viewerMatrix[6] = tr.viewParms.or.axis[2][1];
	viewerMatrix[10] = tr.viewParms.or.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] *
			   viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	/* convert from our coordinate system (looking down X)
	 * to OpenGL's coordinate system (looking down -Z) */
	myGlMultMatrix(viewerMatrix, s_flipMatrix, tr.or.modelMatrix);

	tr.viewParms.world = tr.or;

}

/*
** SetFarClip
*/
static void
R_SetFarClip(void)
{
	float farthestCornerdistv3 = 0;
	int i;

	/* if not rendering the world (icons, menus, etc)
	 * set a 2k far clip plane */
	if(tr.refdef.rdflags & RDF_NOWORLDMODEL){
		tr.viewParms.zFar = 2048;
		return;
	}

	/*
	 * set far clipping planes dynamically
	 *  */
	farthestCornerdistv3 = 0;
	for(i = 0; i < 8; i++){
		Vec3	v;
		Vec3	vecTo;
		float	distance;

		if(i & 1){
			v[0] = tr.viewParms.visBounds[0][0];
		}else{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if(i & 2){
			v[1] = tr.viewParms.visBounds[0][1];
		}else{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if(i & 4){
			v[2] = tr.viewParms.visBounds[0][2];
		}else{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		subv3(v, tr.viewParms.or.origin, vecTo);

		distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if(distance > farthestCornerdistv3){
			farthestCornerdistv3 = distance;
		}
	}
	tr.viewParms.zFar = sqrt(farthestCornerdistv3);
}

/*
 * R_SetupFrustum
 *
 * Set up the culling frustum planes for the current view using the results we got from computing the first two rows of
 * the projection matrix.
 */
void
R_SetupFrustum(viewParms_t *dest, float xmin, float xmax, float ymax, float zProj, float stereoSep)
{
	Vec3	ofsorigin;
	float	oppleg, adjleg, length;
	int	i;

	if(stereoSep == 0 && xmin == -xmax){
		/* symmetric case can be simplified */
		copyv3(dest->or.origin, ofsorigin);

		length	= sqrt(xmax * xmax + zProj * zProj);
		oppleg	= xmax / length;
		adjleg	= zProj / length;

		scalev3(dest->or.axis[0], oppleg, dest->frustum[0].normal);
		saddv3(dest->frustum[0].normal, adjleg, dest->or.axis[1], dest->frustum[0].normal);

		scalev3(dest->or.axis[0], oppleg, dest->frustum[1].normal);
		saddv3(dest->frustum[1].normal, -adjleg, dest->or.axis[1], dest->frustum[1].normal);
	}else{
		/* In stereo rendering, due to the modification of the projection matrix, dest->or.origin is not the
		 * actual origin that we're rendering so offset the tip of the view pyramid. */
		saddv3(dest->or.origin, stereoSep, dest->or.axis[1], ofsorigin);

		oppleg	= xmax + stereoSep;
		length	= sqrt(oppleg * oppleg + zProj * zProj);
		scalev3(dest->or.axis[0], oppleg / length, dest->frustum[0].normal);
		saddv3(dest->frustum[0].normal, zProj / length, dest->or.axis[1], dest->frustum[0].normal);

		oppleg	= xmin + stereoSep;
		length	= sqrt(oppleg * oppleg + zProj * zProj);
		scalev3(dest->or.axis[0], -oppleg / length, dest->frustum[1].normal);
		saddv3(dest->frustum[1].normal, -zProj / length, dest->or.axis[1], dest->frustum[1].normal);
	}

	length	= sqrt(ymax * ymax + zProj * zProj);
	oppleg	= ymax / length;
	adjleg	= zProj / length;

	scalev3(dest->or.axis[0], oppleg, dest->frustum[2].normal);
	saddv3(dest->frustum[2].normal, adjleg, dest->or.axis[2], dest->frustum[2].normal);

	scalev3(dest->or.axis[0], oppleg, dest->frustum[3].normal);
	saddv3(dest->frustum[3].normal, -adjleg, dest->or.axis[2], dest->frustum[3].normal);

	for(i=0; i<4; i++){
		dest->frustum[i].type	= PLANE_NON_AXIAL;
		dest->frustum[i].dist	= dotv3 (ofsorigin, dest->frustum[i].normal);
		SetPlaneSignbits(&dest->frustum[i]);
	}
}

/*
 * R_SetupProjection
 */
void
R_SetupProjection(viewParms_t *dest, float zProj, qbool computeFrustum)
{
	float xmin, xmax, ymin, ymax;
	float width, height, stereoSep = r_stereoSeparation->value;

	/*
	 * offset the view origin of the viewer for stereo rendering
	 * by setting the projection matrix appropriately.
	 */

	if(stereoSep != 0){
		if(dest->stereoFrame == STEREO_LEFT)
			stereoSep = zProj / stereoSep;
		else if(dest->stereoFrame == STEREO_RIGHT)
			stereoSep = zProj / -stereoSep;
		else
			stereoSep = 0;
	}

	ymax = zProj * tan(dest->fovY * M_PI / 360.0f);
	ymin = -ymax;

	xmax = zProj * tan(dest->fovX * M_PI / 360.0f);
	xmin = -xmax;

	width	= xmax - xmin;
	height	= ymax - ymin;

	dest->projectionMatrix[0] = 2 * zProj / width;
	dest->projectionMatrix[4] = 0;
	dest->projectionMatrix[8] = (xmax + xmin + 2 * stereoSep) / width;
	dest->projectionMatrix[12] = 2 * zProj * stereoSep / width;

	dest->projectionMatrix[1] = 0;
	dest->projectionMatrix[5] = 2 * zProj / height;
	dest->projectionMatrix[9] = (ymax + ymin) / height;	/* normally 0 */
	dest->projectionMatrix[13] = 0;

	dest->projectionMatrix[3] = 0;
	dest->projectionMatrix[7] = 0;
	dest->projectionMatrix[11] = -1;
	dest->projectionMatrix[15] = 0;

	/* Now that we have all the data for the projection matrix we can also setup the view frustum. */
	if(computeFrustum)
		R_SetupFrustum(dest, xmin, xmax, ymax, zProj, stereoSep);
}

/*
 * R_SetupProjectionZ
 *
 * Sets the z-component transformation part in the projection matrix
 */
void
R_SetupProjectionZ(viewParms_t *dest)
{
	float zNear, zFar, depth;

	zNear	= r_znear->value;
	zFar	= dest->zFar;
	depth	= zFar - zNear;

	dest->projectionMatrix[2] = 0;
	dest->projectionMatrix[6] = 0;
	dest->projectionMatrix[10] = -(zFar + zNear) / depth;
	dest->projectionMatrix[14] = -2 * zFar * zNear / depth;
}

/*
 * R_MirrorPoint
 */
void
R_MirrorPoint(Vec3 in, Orient *surface, Orient *camera, Vec3 out)
{
	int i;
	Vec3	local;
	Vec3	transformed;
	float	d;

	subv3(in, surface->origin, local);

	clearv3(transformed);
	for(i = 0; i < 3; i++){
		d = dotv3(local, surface->axis[i]);
		saddv3(transformed, d, camera->axis[i], transformed);
	}

	addv3(transformed, camera->origin, out);
}

void
R_MirrorVector(Vec3 in, Orient *surface, Orient *camera, Vec3 out)
{
	int i;
	float d;

	clearv3(out);
	for(i = 0; i < 3; i++){
		d = dotv3(in, surface->axis[i]);
		saddv3(out, d, camera->axis[i], out);
	}
}


/*
 * R_PlaneForSurface
 */
void
R_PlaneForSurface(surfaceType_t *surfType, Cplane *plane)
{
	srfTriangles_t *tri;
	srfPoly_t *poly;
	Drawvert *v1, *v2, *v3;
	Vec4 plane4;

	if(!surfType){
		Q_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch(*surfType){
	case SF_FACE:
		*plane = ((srfSurfaceFace_t*)surfType)->plane;
		return;
	case SF_TRIANGLES:
		tri = (srfTriangles_t*)surfType;
		v1 = tri->verts + tri->indexes[0];
		v2 = tri->verts + tri->indexes[1];
		v3 = tri->verts + tri->indexes[2];
		PlaneFromPoints(plane4, v1->xyz, v2->xyz, v3->xyz);
		copyv3(plane4, plane->normal);
		plane->dist = plane4[3];
		return;
	case SF_POLY:
		poly = (srfPoly_t*)surfType;
		PlaneFromPoints(plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz);
		copyv3(plane4, plane->normal);
		plane->dist = plane4[3];
		return;
	default:
		Q_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
}

/*
 * R_GetPortalOrientation
 *
 * entityNum is the entity that the portal surface is a part of, which may
 * be moving and rotating.
 *
 * Returns qtrue if it should be mirrored
 */
qbool
R_GetPortalOrientations(drawSurf_t *drawSurf, int entityNum,
			Orient *surface, Orient *camera,
			Vec3 pvsOrigin, qbool *mirror)
{
	int i;
	Cplane originalPlane, plane;
	trRefEntity_t *e;
	float d;
	Vec3 transformed;

	/* create plane axis for the portal we are seeing */
	R_PlaneForSurface(drawSurf->surface, &originalPlane);

	/* rotate the plane if necessary */
	if(entityNum != ENTITYNUM_WORLD){
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		/* get the orientation of the entity */
		R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.or);

		/* rotate the plane, but keep the non-rotated version for matching
		 * against the portalSurface entities */
		R_LocalNormalToWorld(originalPlane.normal, plane.normal);
		plane.dist = originalPlane.dist + dotv3(plane.normal, tr.or.origin);

		/* translate the original plane */
		originalPlane.dist = originalPlane.dist + dotv3(originalPlane.normal, tr.or.origin);
	}else{
		plane = originalPlane;
	}

	copyv3(plane.normal, surface->axis[0]);
	perpv3(surface->axis[1], surface->axis[0]);
	crossv3(surface->axis[0], surface->axis[1], surface->axis[2]);

	/* locate the portal entity closest to this plane.
	 * origin will be the origin of the portal, origin2 will be
	 * the origin of the camera */
	for(i = 0; i < tr.refdef.num_entities; i++){
		e = &tr.refdef.entities[i];
		if(e->e.reType != RT_PORTALSURFACE){
			continue;
		}

		d = dotv3(e->e.origin, originalPlane.normal) - originalPlane.dist;
		if(d > 64 || d < -64){
			continue;
		}

		/* get the pvsOrigin from the entity */
		copyv3(e->e.oldorigin, pvsOrigin);

		/* if the entity is just a mirror, don't use as a camera point */
		if(e->e.oldorigin[0] == e->e.origin[0] &&
		   e->e.oldorigin[1] == e->e.origin[1] &&
		   e->e.oldorigin[2] == e->e.origin[2]){
			scalev3(plane.normal, plane.dist, surface->origin);
			copyv3(surface->origin, camera->origin);
			subv3(vec3_origin, surface->axis[0], camera->axis[0]);
			copyv3(surface->axis[1], camera->axis[1]);
			copyv3(surface->axis[2], camera->axis[2]);

			*mirror = qtrue;
			return qtrue;
		}

		/* project the origin onto the surface plane to get
		 * an origin point we can rotate around */
		d = dotv3(e->e.origin, plane.normal) - plane.dist;
		saddv3(e->e.origin, -d, surface->axis[0], surface->origin);

		/* now get the camera origin and orientation */
		copyv3(e->e.oldorigin, camera->origin);
		copyaxis(e->e.axis, camera->axis);
		subv3(vec3_origin, camera->axis[0], camera->axis[0]);
		subv3(vec3_origin, camera->axis[1], camera->axis[1]);

		/* optionally rotate */
		if(e->e.oldframe){
			/* if a speed is specified */
			if(e->e.frame){
				/* continuous rotate */
				d = (tr.refdef.time/1000.0f) * e->e.frame;
				copyv3(camera->axis[1], transformed);
				RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
				crossv3(camera->axis[0], camera->axis[1], camera->axis[2]);
			}else{
				/* bobbing rotate, with skinNum being the rotation offset */
				d = sin(tr.refdef.time * 0.003f);
				d = e->e.skinNum + d * 4;
				copyv3(camera->axis[1], transformed);
				RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
				crossv3(camera->axis[0], camera->axis[1], camera->axis[2]);
			}
		}else if(e->e.skinNum){
			d = e->e.skinNum;
			copyv3(camera->axis[1], transformed);
			RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
			crossv3(camera->axis[0], camera->axis[1], camera->axis[2]);
		}
		*mirror = qfalse;
		return qtrue;
	}

	/* if we didn't locate a portal entity, don't render anything.
	 * We don't want to just treat it as a mirror, because without a
	 * portal entity the server won't have communicated a proper entity set
	 * in the snapshot */

	/* unfortunately, with local movement prediction it is easily possible
	 * to see a surface before the server has communicated the matching
	 * portal surface entity, so we don't want to print anything here... */

	/* ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" ); */

	return qfalse;
}

static qbool
IsMirror(const drawSurf_t *drawSurf, int entityNum)
{
	int i;
	Cplane originalPlane, plane;
	trRefEntity_t *e;
	float d;

	/* create plane axis for the portal we are seeing */
	R_PlaneForSurface(drawSurf->surface, &originalPlane);

	/* rotate the plane if necessary */
	if(entityNum != ENTITYNUM_WORLD){
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		/* get the orientation of the entity */
		R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.or);

		/* rotate the plane, but keep the non-rotated version for matching
		 * against the portalSurface entities */
		R_LocalNormalToWorld(originalPlane.normal, plane.normal);
		plane.dist = originalPlane.dist + dotv3(plane.normal, tr.or.origin);

		/* translate the original plane */
		originalPlane.dist = originalPlane.dist + dotv3(originalPlane.normal, tr.or.origin);
	}else{
		plane = originalPlane;
	}

	/* locate the portal entity closest to this plane.
	 * origin will be the origin of the portal, origin2 will be
	 * the origin of the camera */
	for(i = 0; i < tr.refdef.num_entities; i++){
		e = &tr.refdef.entities[i];
		if(e->e.reType != RT_PORTALSURFACE){
			continue;
		}

		d = dotv3(e->e.origin, originalPlane.normal) - originalPlane.dist;
		if(d > 64 || d < -64){
			continue;
		}

		/* if the entity is just a mirror, don't use as a camera point */
		if(e->e.oldorigin[0] == e->e.origin[0] &&
		   e->e.oldorigin[1] == e->e.origin[1] &&
		   e->e.oldorigin[2] == e->e.origin[2]){
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qbool
SurfIsOffscreen(const drawSurf_t *drawSurf, Vec4 clipDest[128])
{
	float shortest = 100000000;
	int entityNum;
	int numTriangles;
	material_t	*shader;
	int		fogNum;
	int		dlighted;
	Vec4		clip, eye;
	int		i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	if(glConfig.smpActive){	/* FIXME!  we can't do RB_BeginSurface/RB_EndSurface stuff with smp! */
		return qfalse;
	}

	R_RotateForViewer();

	R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted);
	RB_BeginSurface(shader, fogNum);
	rb_surfaceTable[ *drawSurf->surface ](drawSurf->surface);

	assert(tess.numVertexes < 128);

	for(i = 0; i < tess.numVertexes; i++){
		int j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip(tess.xyz[i], tr.or.modelMatrix, tr.viewParms.projectionMatrix, eye,
			clip);

		for(j = 0; j < 3; j++){
			if(clip[j] >= clip[3]){
				pointFlags |= (1 << (j*2));
			}else if(clip[j] <= -clip[3]){
				pointFlags |= (1 << (j*2+1));
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	/* trivially reject */
	if(pointAnd){
		return qtrue;
	}

	/* determine if this surface is backfaced and also determine the distance
	 * to the nearest vertex so we can cull based on portal range.  Culling
	 * based on vertex distance isn't 100% correct (we should be checking for
	 * range to the surface), but it's good enough for the types of portals
	 * we have in the game right now. */
	numTriangles = tess.numIndexes / 3;

	for(i = 0; i < tess.numIndexes; i += 3){
		Vec3	normal;
		float	len;

		subv3(tess.xyz[tess.indexes[i]], tr.viewParms.or.origin, normal);

		len = lensqrv3(normal);	/* lose the sqrt */
		if(len < shortest){
			shortest = len;
		}

		if(dotv3(normal, tess.normal[tess.indexes[i]]) >= 0){
			numTriangles--;
		}
	}
	if(!numTriangles){
		return qtrue;
	}

	/* mirrors can early out at this point, since we don't do a fade over distance
	 * with them (although we could) */
	if(IsMirror(drawSurf, entityNum)){
		return qfalse;
	}

	if(shortest > (tess.shader->portalRange*tess.shader->portalRange)){
		return qtrue;
	}

	return qfalse;
}

/*
 * R_MirrorViewBySurface
 *
 * Returns qtrue if another view has been rendered
 */
qbool
R_MirrorViewBySurface(drawSurf_t *drawSurf, int entityNum)
{
	Vec4 clipDest[128];
	viewParms_t newParms;
	viewParms_t oldParms;
	Orient surface, camera;

	/* don't recursively mirror */
	if(tr.viewParms.isPortal){
		ri.Printf(PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n");
		return qfalse;
	}

	if(r_noportals->integer || (r_fastsky->integer == 1)){
		return qfalse;
	}

	/* trivially reject portal/mirror */
	if(SurfIsOffscreen(drawSurf, clipDest)){
		return qfalse;
	}

	/* save old viewParms so we can return to it after the mirror view */
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;
	if(!R_GetPortalOrientations(drawSurf, entityNum, &surface, &camera,
		   newParms.pvsOrigin, &newParms.isMirror)){
		return qfalse;	/* bad portal, no portalentity */
	}

	R_MirrorPoint (oldParms.or.origin, &surface, &camera, newParms.or.origin);

	subv3(vec3_origin, camera.axis[0], newParms.portalPlane.normal);
	newParms.portalPlane.dist = dotv3(camera.origin, newParms.portalPlane.normal);

	R_MirrorVector (oldParms.or.axis[0], &surface, &camera, newParms.or.axis[0]);
	R_MirrorVector (oldParms.or.axis[1], &surface, &camera, newParms.or.axis[1]);
	R_MirrorVector (oldParms.or.axis[2], &surface, &camera, newParms.or.axis[2]);

	/* OPTIMIZE: restrict the viewport on the mirrored view */

	/* render the mirror view */
	R_RenderView (&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

/*
 * R_SpriteFogNum
 *
 * See if a sprite is inside a fog volume
 */
int
R_SpriteFogNum(trRefEntity_t *ent)
{
	int i, j;
	fog_t *fog;

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL){
		return 0;
	}

	for(i = 1; i < tr.world->numfogs; i++){
		fog = &tr.world->fogs[i];
		for(j = 0; j < 3; j++){
			if(ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j]){
				break;
			}
			if(ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j]){
				break;
			}
		}
		if(j == 3){
			return i;
		}
	}

	return 0;
}

/*
 *
 * DRAWSURF SORTING
 *
 */

/*
 * R_Radix
 */
static ID_INLINE void
R_Radix(int byte, int size, drawSurf_t *source, drawSurf_t *dest)
{
	int count[ 256 ] = { 0 };
	int index[ 256 ];
	int i;
	unsigned char *sortKey = NULL;
	unsigned char *end = NULL;

	sortKey = ((unsigned char*)&source[ 0 ].sort) + byte;
	end = sortKey + (size * sizeof(drawSurf_t));
	for(; sortKey < end; sortKey += sizeof(drawSurf_t))
		++count[ *sortKey ];

	index[ 0 ] = 0;

	for(i = 1; i < 256; ++i)
		index[ i ] = index[ i - 1 ] + count[ i - 1 ];

	sortKey = ((unsigned char*)&source[ 0 ].sort) + byte;
	for(i = 0; i < size; ++i, sortKey += sizeof(drawSurf_t))
		dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
 * R_RadixSort
 *
 * Radix sort with 4 byte size buckets
 */
static void
R_RadixSort(drawSurf_t *source, int size)
{
	static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
	R_Radix(0, size, source, scratch);
	R_Radix(1, size, scratch, source);
	R_Radix(2, size, source, scratch);
	R_Radix(3, size, scratch, source);
#else
	R_Radix(3, size, source, scratch);
	R_Radix(2, size, scratch, source);
	R_Radix(1, size, source, scratch);
	R_Radix(0, size, scratch, source);
#endif	/* Q3_LITTLE_ENDIAN */
}

/* ========================================================================================== */

/*
 * R_AddDrawSurf
 */
void
R_AddDrawSurf(surfaceType_t *surface, material_t *shader,
	      int fogIndex, int dlightMap)
{
	int index;

	/* instead of checking for overflow, we just mask the index
	 * so it wraps around */
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	/* the sort data is packed into a single 32 bit value so it can be
	 * compared quickly during the qsorting process */
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT)
					  | tr.shiftedEntityNum |
					  (fogIndex << QSORT_FOGNUM_SHIFT) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

/*
 * R_DecomposeSort
 */
void
R_DecomposeSort(unsigned sort, int *entityNum, material_t **shader,
		int *fogNum, int *dlightMap)
{
	*fogNum = (sort >> QSORT_FOGNUM_SHIFT) & 31;
	*shader = tr.sortedShaders[ (sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS-1) ];
	*entityNum = (sort >> QSORT_ENTITYNUM_SHIFT) & (MAX_GENTITIES-1);
	*dlightMap = sort & 3;
}

/*
 * R_SortDrawSurfs
 */
void
R_SortDrawSurfs(drawSurf_t *drawSurfs, int numDrawSurfs)
{
	material_t	*shader;
	int	fogNum;
	int	entityNum;
	int	dlighted;
	int	i;

	/* it is possible for some views to not have any surfaces */
	if(numDrawSurfs < 1){
		/* we still need to add it for hyperspace cases */
		R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
		return;
	}

	/* if we overflowed MAX_DRAWSURFS, the drawsurfs
	 * wrapped around in the buffer and we will be missing
	 * the first surfaces, not the last ones */
	if(numDrawSurfs > MAX_DRAWSURFS){
		numDrawSurfs = MAX_DRAWSURFS;
	}

	/* sort the drawsurfs by sort type, then orientation, then shader */
	R_RadixSort(drawSurfs, numDrawSurfs);

	/* check for any pass through drawing, which
	 * may cause another view to be rendered first */
	for(i = 0; i < numDrawSurfs; i++){
		R_DecomposeSort((drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted);

		if(shader->sort > SS_PORTAL){
			break;
		}

		/* no shader should ever have this sort type */
		if(shader->sort == SS_BAD){
			ri.Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name);
		}

		/* if the mirror was completely clipped away, we may need to check another surface */
		if(R_MirrorViewBySurface((drawSurfs+i), entityNum)){
			/* this is a debug option to see exactly what is being mirrored */
			if(r_portalOnly->integer){
				return;
			}
			break;	/* only one mirror view at a time */
		}
	}

	R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
}

/*
 * R_AddEntitySurfaces
 */
void
R_AddEntitySurfaces(void)
{
	trRefEntity_t *ent;
	material_t *shader;

	if(!r_drawentities->integer){
		return;
	}

	for(tr.currentEntityNum = 0;
	    tr.currentEntityNum < tr.refdef.num_entities;
	    tr.currentEntityNum++){
		ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = qfalse;

		/* preshift the value we are going to OR into the drawsurf sort */
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		/*
		 * the weapon model must be handled special --
		 * we don't want the hacked weapon position showing in
		 * mirrors, because the true body position will already be drawn
		 *  */
		if((ent->e.renderfx & RF_FIRST_PERSON) && tr.viewParms.isPortal){
			continue;
		}

		/* simple generated models, like sprites and beams, are not culled */
		switch(ent->e.reType){
		case RT_PORTALSURFACE:
			break;	/* don't draw anything */
		case RT_SPRITE:
		case RT_BEAM:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_RINGS:
			/* self blood sprites, talk balloons, etc should not be drawn in the primary
			 * view.  We can't just do this check for all entities, because md3
			 * entities may still want to cast shadows from them */
			if((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal){
				continue;
			}
			shader = R_GetShaderByHandle(ent->e.customShader);
			R_AddDrawSurf(&entitySurface, shader, R_SpriteFogNum(ent), 0);
			break;

		case RT_MODEL:
			/* we must set up parts of tr.or for model culling */
			R_RotateForEntity(ent, &tr.viewParms, &tr.or);

			tr.currentModel = R_GetModelByHandle(ent->e.hModel);
			if(!tr.currentModel){
				R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0);
			}else{
				switch(tr.currentModel->type){
				case MOD_MESH:
					R_AddMD3Surfaces(ent);
					break;
				case MOD_MD4:
					R_AddAnimSurfaces(ent);
					break;
				case MOD_IQM:
					R_AddIQMSurfaces(ent);
					break;
				case MOD_BRUSH:
					R_AddBrushModelSurfaces(ent);
					break;
				case MOD_BAD:	/* null model axis */
					if((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal){
						break;
					}
					R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0);
					break;
				default:
					ri.Error(ERR_DROP, "R_AddEntitySurfaces: Bad modeltype");
					break;
				}
			}
			break;
		default:
			ri.Error(ERR_DROP, "R_AddEntitySurfaces: Bad reType");
		}
	}

}


/*
 * R_GenerateDrawSurfs
 */
void
R_GenerateDrawSurfs(void)
{
	R_AddWorldSurfaces ();

	R_AddPolygonSurfaces();

	/* set the projection matrix with the minimum zfar
	 * now that we have the world bounded
	 * this needs to be done before entities are
	 * added, because they use the projection
	 * matrix for lod calculation */

	/* dynamically compute far clip plane distance */
	R_SetFarClip();

	/* we know the size of the clipping volume. Now set the rest of the projection matrix. */
	R_SetupProjectionZ (&tr.viewParms);

	R_AddEntitySurfaces ();
}

/*
 * R_DebugPolygon
 */
void
R_DebugPolygon(int color, int numPoints, float *points)
{
	int i;

	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	/* draw solid shade */

	qglColor3f(color&1, (color>>1)&1, (color>>2)&1);
	qglBegin(GL_POLYGON);
	for(i = 0; i < numPoints; i++)
		qglVertex3fv(points + i * 3);
	qglEnd();

	/* draw wireframe outline */
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	qglDepthRange(0, 0);
	qglColor3f(1, 1, 1);
	qglBegin(GL_POLYGON);
	for(i = 0; i < numPoints; i++)
		qglVertex3fv(points + i * 3);
	qglEnd();
	qglDepthRange(0, 1);
}

/*
 * R_DebugGraphics
 *
 * Visualization aid for movement clipping debugging
 */
void
R_DebugGraphics(void)
{
	if(!r_debugSurface->integer){
		return;
	}

	/* the render thread can't make callbacks to the main thread */
	R_SyncRenderThread();

	GL_Bind(tr.whiteImage);
	GL_Cull(CT_FRONT_SIDED);
	ri.CM_DrawDebugSurface(R_DebugPolygon);
}


/*
 * R_RenderView
 *
 * A view may be either the actual camera view,
 * or a mirror / remote location
 */
void
R_RenderView(viewParms_t *parms)
{
	int firstDrawSurf;

	if(parms->viewportWidth <= 0 || parms->viewportHeight <= 0){
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	/* set viewParms.world */
	R_RotateForViewer ();

	R_SetupProjection(&tr.viewParms, r_zproj->value, qtrue);

	R_GenerateDrawSurfs();

	R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf);

	/* draw main system development information (surface outlines, etc) */
	R_DebugGraphics();
}
