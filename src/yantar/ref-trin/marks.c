/* polygon projection on the world polygons */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

#define MAX_VERTS_ON_POLY	64
#define MARKER_OFFSET		0	/* 1 */

/*
 * R_ChopPolyBehindPlane
 *
 * Out must have space for two more vertexes than in
 */
#define SIDE_FRONT	0
#define SIDE_BACK	1
#define SIDE_ON		2
static void
R_ChopPolyBehindPlane(int numInPoints, Vec3 inPoints[MAX_VERTS_ON_POLY],
		      int *numOutPoints, Vec3 outPoints[MAX_VERTS_ON_POLY],
		      Vec3 normal, Scalar dist, Scalar epsilon)
{
	float dists[MAX_VERTS_ON_POLY+4];
	int sides[MAX_VERTS_ON_POLY+4];
	int counts[3];
	float dot;
	int i, j;
	float *p1, *p2, *clip;
	float d;

	/* don't clip if it might overflow */
	if(numInPoints >= MAX_VERTS_ON_POLY - 2){
		*numOutPoints = 0;
		return;
	}

	counts[0] = counts[1] = counts[2] = 0;

	/* determine sides for each point */
	for(i = 0; i < numInPoints; i++){
		dot = dotv3(inPoints[i], normal);
		dot -= dist;
		dists[i] = dot;
		if(dot > epsilon){
			sides[i] = SIDE_FRONT;
		}else if(dot < -epsilon){
			sides[i] = SIDE_BACK;
		}else{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*numOutPoints = 0;

	if(!counts[0]){
		return;
	}
	if(!counts[1]){
		*numOutPoints = numInPoints;
		Q_Memcpy(outPoints, inPoints, numInPoints * sizeof(Vec3));
		return;
	}

	for(i = 0; i < numInPoints; i++){
		p1 = inPoints[i];
		clip = outPoints[ *numOutPoints ];

		if(sides[i] == SIDE_ON){
			copyv3(p1, clip);
			(*numOutPoints)++;
			continue;
		}

		if(sides[i] == SIDE_FRONT){
			copyv3(p1, clip);
			(*numOutPoints)++;
			clip = outPoints[ *numOutPoints ];
		}

		if(sides[i+1] == SIDE_ON || sides[i+1] == sides[i]){
			continue;
		}

		/* generate a split point */
		p2 = inPoints[ (i+1) % numInPoints ];

		d = dists[i] - dists[i+1];
		if(d == 0){
			dot = 0;
		}else{
			dot = dists[i] / d;
		}

		/* clip xyz */

		for(j=0; j<3; j++)
			clip[j] = p1[j] + dot * (p2[j] - p1[j]);

		(*numOutPoints)++;
	}
}

/*
 * R_BoxSurfaces_r
 *
 */
void
R_BoxSurfaces_r(mnode_t *node, Vec3 mins, Vec3 maxs, surfaceType_t **list, int listsize, int *listlength,
		Vec3 dir)
{

	int s, c;
	msurface_t *surf, **mark;

	/* do the tail recursion in a loop */
	while(node->contents == -1){
		s = BoxOnPlaneSide(mins, maxs, node->plane);
		if(s == 1){
			node = node->children[0];
		}else if(s == 2){
			node = node->children[1];
		}else{
			R_BoxSurfaces_r(node->children[0], mins, maxs, list, listsize, listlength, dir);
			node = node->children[1];
		}
	}

	/* add the individual surfaces */
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while(c--){
		if(*listlength >= listsize) break;
		surf = *mark;
		/* check if the surface has NOIMPACT or NOMARKS set */
		if((surf->shader->surfaceFlags & (SURF_NOIMPACT | SURF_NOMARKS))
		   || (surf->shader->contentFlags & CONTENTS_FOG)){
			surf->viewCount = tr.viewCount;
		}
		/* extra check for surfaces to avoid list overflows */
		else if(*(surf->data) == SF_FACE){
			/* the face plane should go through the box */
			s = BoxOnPlaneSide(mins, maxs, &(( srfSurfaceFace_t* )surf->data)->plane);
			if(s == 1 || s == 2){
				surf->viewCount = tr.viewCount;
			}else if(dotv3((( srfSurfaceFace_t* )surf->data)->plane.normal, dir) > -0.5){
				/* don't add faces that make sharp angles with the projection direction */
				surf->viewCount = tr.viewCount;
			}
		}else if(*(surfaceType_t*)(surf->data) != SF_GRID &&
			 *(surfaceType_t*)(surf->data) != SF_TRIANGLES)
			surf->viewCount = tr.viewCount;
		/* check the viewCount because the surface may have
		 * already been added if it spans multiple leafs */
		if(surf->viewCount != tr.viewCount){
			surf->viewCount = tr.viewCount;
			list[*listlength] = (surfaceType_t*)surf->data;
			(*listlength)++;
		}
		mark++;
	}
}

/*
 * R_AddMarkFragments
 *
 */
void
R_AddMarkFragments(int numClipPoints, Vec3 clipPoints[2][MAX_VERTS_ON_POLY],
		   int numPlanes, Vec3 *normals, float *dists,
		   int maxPoints, Vec3 pointBuffer,
		   int maxFragments, Markfrag *fragmentBuffer,
		   int *returnedPoints, int *returnedFragments,
		   Vec3 mins, Vec3 maxs)
{
	int pingPong, i;
	Markfrag *mf;

	/* chop the surface by all the bounding planes of the to be projected polygon */
	pingPong = 0;

	for(i = 0; i < numPlanes; i++){

		R_ChopPolyBehindPlane(numClipPoints, clipPoints[pingPong],
			&numClipPoints, clipPoints[!pingPong],
			normals[i], dists[i], 0.5);
		pingPong ^= 1;
		if(numClipPoints == 0){
			break;
		}
	}
	/* completely clipped away? */
	if(numClipPoints == 0){
		return;
	}

	/* add this fragment to the returned list */
	if(numClipPoints + (*returnedPoints) > maxPoints){
		return;	/* not enough space for this polygon */
	}
	/*
	 * // all the clip points should be within the bounding box
	 * for ( i = 0 ; i < numClipPoints ; i++ ) {
	 *      int j;
	 *      for ( j = 0 ; j < 3 ; j++ ) {
	 *              if (clipPoints[pingPong][i][j] < mins[j] - 0.5) break;
	 *              if (clipPoints[pingPong][i][j] > maxs[j] + 0.5) break;
	 *      }
	 *      if (j < 3) break;
	 * }
	 * if (i < numClipPoints) return;
	 */

	mf = fragmentBuffer + (*returnedFragments);
	mf->firstPoint	= (*returnedPoints);
	mf->numPoints	= numClipPoints;
	Q_Memcpy(pointBuffer + (*returnedPoints) * 3, clipPoints[pingPong], numClipPoints * sizeof(Vec3));

	(*returnedPoints) += numClipPoints;
	(*returnedFragments)++;
}

/*
 * R_MarkFragments
 *
 */
int
R_MarkFragments(int numPoints, const Vec3 *points, const Vec3 projection,
		int maxPoints, Vec3 pointBuffer, int maxFragments, Markfrag *fragmentBuffer)
{
	int numsurfaces, numPlanes;
	int i, j, k, m, n;
	surfaceType_t *surfaces[64];
	Vec3	mins, maxs;
	int	returnedFragments;
	int	returnedPoints;
	Vec3	normals[MAX_VERTS_ON_POLY+2];
	float	dists[MAX_VERTS_ON_POLY+2];
	Vec3	clipPoints[2][MAX_VERTS_ON_POLY];
	int	numClipPoints;
	float	*v;
	srfGridMesh_t	*cv;
	Drawvert	*dv;
	Vec3	normal;
	Vec3	projectionDir;
	Vec3	v1, v2;
	int	*indexes;

	if(numPoints <= 0){
		return 0;
	}

	/* increment view count for double check prevention */
	tr.viewCount++;

	norm2v3(projection, projectionDir);
	/* find all the brushes that are to be considered */
	ClearBounds(mins, maxs);
	for(i = 0; i < numPoints; i++){
		Vec3 temp;

		AddPointToBounds(points[i], mins, maxs);
		addv3(points[i], projection, temp);
		AddPointToBounds(temp, mins, maxs);
		/* make sure we get all the leafs (also the one(s) in front of the hit surface) */
		saddv3(points[i], -20, projectionDir, temp);
		AddPointToBounds(temp, mins, maxs);
	}

	if(numPoints > MAX_VERTS_ON_POLY) numPoints = MAX_VERTS_ON_POLY;
	/* create the bounding planes for the to be projected polygon */
	for(i = 0; i < numPoints; i++){
		subv3(points[(i+1)%numPoints], points[i], v1);
		addv3(points[i], projection, v2);
		subv3(points[i], v2, v2);
		crossv3(v1, v2, normals[i]);
		fastnormv3(normals[i]);
		dists[i] = dotv3(normals[i], points[i]);
	}
	/* add near and far clipping planes for projection */
	copyv3(projectionDir, normals[numPoints]);
	dists[numPoints] = dotv3(normals[numPoints], points[0]) - 32;
	copyv3(projectionDir, normals[numPoints+1]);
	invv3(normals[numPoints+1]);
	dists[numPoints+1] = dotv3(normals[numPoints+1], points[0]) - 20;
	numPlanes = numPoints + 2;

	numsurfaces = 0;
	R_BoxSurfaces_r(tr.world->nodes, mins, maxs, surfaces, 64, &numsurfaces, projectionDir);
	/* assert(numsurfaces <= 64);
	 * assert(numsurfaces != 64); */

	returnedPoints = 0;
	returnedFragments = 0;

	for(i = 0; i < numsurfaces; i++){

		if(*surfaces[i] == SF_GRID){

			cv = (srfGridMesh_t*)surfaces[i];
			for(m = 0; m < cv->height - 1; m++)
				for(n = 0; n < cv->width - 1; n++){
					/* We triangulate the grid and chop all triangles within
					 * the bounding planes of the to be projected polygon.
					 * LOD is not taken into account, not such a big deal though.
					 *
					 * It's probably much nicer to chop the grid itself and deal
					 * with this grid as a normal SF_GRID surface so LOD will
					 * be applied. However the LOD of that chopped grid must
					 * be synced with the LOD of the original curve.
					 * One way to do this; the chopped grid shares vertices with
					 * the original curve. When LOD is applied to the original
					 * curve the unused vertices are flagged. Now the chopped curve
					 * should skip the flagged vertices. This still leaves the
					 * problems with the vertices at the chopped grid edges.
					 *
					 * To avoid issues when LOD applied to "hollow curves" (like
					 * the ones around many jump pads) we now just add a 2 unit
					 * offset to the triangle vertices.
					 * The offset is added in the vertex normal vector direction
					 * so all triangles will still fit together.
					 * The 2 unit offset should avoid pretty much all LOD problems. */

					numClipPoints = 3;

					dv = cv->verts + m * cv->width + n;

					copyv3(dv[0].xyz, clipPoints[0][0]);
					saddv3(clipPoints[0][0], MARKER_OFFSET, dv[0].normal,
						clipPoints[0][0]);
					copyv3(dv[cv->width].xyz, clipPoints[0][1]);
					saddv3(clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal,
						clipPoints[0][1]);
					copyv3(dv[1].xyz, clipPoints[0][2]);
					saddv3(clipPoints[0][2], MARKER_OFFSET, dv[1].normal,
						clipPoints[0][2]);
					/* check the normal of this triangle */
					subv3(clipPoints[0][0], clipPoints[0][1], v1);
					subv3(clipPoints[0][2], clipPoints[0][1], v2);
					crossv3(v1, v2, normal);
					fastnormv3(normal);
					if(dotv3(normal, projectionDir) < -0.1){
						/* add the fragments of this triangle */
						R_AddMarkFragments(numClipPoints, clipPoints,
							numPlanes, normals, dists,
							maxPoints, pointBuffer,
							maxFragments, fragmentBuffer,
							&returnedPoints, &returnedFragments, mins, maxs);

						if(returnedFragments == maxFragments){
							return returnedFragments;	/* not enough space for more fragments */
						}
					}

					copyv3(dv[1].xyz, clipPoints[0][0]);
					saddv3(clipPoints[0][0], MARKER_OFFSET, dv[1].normal,
						clipPoints[0][0]);
					copyv3(dv[cv->width].xyz, clipPoints[0][1]);
					saddv3(clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal,
						clipPoints[0][1]);
					copyv3(dv[cv->width+1].xyz, clipPoints[0][2]);
					saddv3(clipPoints[0][2], MARKER_OFFSET, dv[cv->width+1].normal,
						clipPoints[0][2]);
					/* check the normal of this triangle */
					subv3(clipPoints[0][0], clipPoints[0][1], v1);
					subv3(clipPoints[0][2], clipPoints[0][1], v2);
					crossv3(v1, v2, normal);
					fastnormv3(normal);
					if(dotv3(normal, projectionDir) < -0.05){
						/* add the fragments of this triangle */
						R_AddMarkFragments(numClipPoints, clipPoints,
							numPlanes, normals, dists,
							maxPoints, pointBuffer,
							maxFragments, fragmentBuffer,
							&returnedPoints, &returnedFragments, mins, maxs);

						if(returnedFragments == maxFragments){
							return returnedFragments;	/* not enough space for more fragments */
						}
					}
				}
		}else if(*surfaces[i] == SF_FACE){

			srfSurfaceFace_t *surf = ( srfSurfaceFace_t* )surfaces[i];

			/* check the normal of this face */
			if(dotv3(surf->plane.normal, projectionDir) > -0.5){
				continue;
			}

			indexes = (int*)((byte*)surf + surf->ofsIndices);
			for(k = 0; k < surf->numIndices; k += 3){
				for(j = 0; j < 3; j++){
					v = surf->points[0] + VERTEXSIZE * indexes[k+j];;
					saddv3(v, MARKER_OFFSET, surf->plane.normal, clipPoints[0][j]);
				}

				/* add the fragments of this face */
				R_AddMarkFragments(3, clipPoints,
					numPlanes, normals, dists,
					maxPoints, pointBuffer,
					maxFragments, fragmentBuffer,
					&returnedPoints, &returnedFragments, mins, maxs);
				if(returnedFragments == maxFragments){
					return returnedFragments;	/* not enough space for more fragments */
				}
			}
		}else if(*surfaces[i] == SF_TRIANGLES && r_marksOnTriangleMeshes->integer){

			srfTriangles_t *surf = (srfTriangles_t*)surfaces[i];

			for(k = 0; k < surf->numIndexes; k += 3){
				for(j = 0; j < 3; j++){
					v = surf->verts[surf->indexes[k + j]].xyz;
					saddv3(v, MARKER_OFFSET, surf->verts[surf->indexes[k + j]].normal,
						clipPoints[0][j]);
				}

				/* add the fragments of this face */
				R_AddMarkFragments(3, clipPoints,
					numPlanes, normals, dists,
					maxPoints, pointBuffer,
					maxFragments, fragmentBuffer, &returnedPoints, &returnedFragments,
					mins,
					maxs);
				if(returnedFragments == maxFragments){
					return returnedFragments;	/* not enough space for more fragments */
				}
			}
		}
	}
	return returnedFragments;
}
