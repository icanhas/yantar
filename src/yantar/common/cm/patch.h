/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/* #define	CULL_BBOX */

/*
 *
 * This file does not reference any globals, and has these entry points:
 *
 * void CM_ClearLevelPatches( void );
 * struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, const Vec3 *points );
 * void CM_TraceThroughPatchCollide( Tracework *tw, const struct patchCollide_s *pc );
 * qbool CM_PositionTestInPatchCollide( Tracework *tw, const struct patchCollide_s *pc );
 * void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, flaot *points) );
 *
 *
 * Issues for collision against curved surfaces:
 *
 * Surface edges need to be handled differently than surface planes
 *
 * Plane expansion causes raw surfaces to expand past expanded bounding box
 *
 * Position test of a volume against a surface is tricky.
 *
 * Position test of a point against a surface is not well defined, because the surface has no volume.
 *
 *
 * Tracing leading edge points instead of volumes?
 * Position test by tracing corner to corner? (8*7 traces -- ouch)
 *
 * coplanar edges
 * triangulated patches
 * degenerate patches
 *
 * endcaps
 * degenerate
 *
 * WARNING: this may misbehave with meshes that have rows or columns that only
 * degenerate a few triangles.  Completely degenerate rows and columns are handled
 * properly.
 */


#define MAX_FACETS		1024
#define MAX_PATCH_PLANES	2048

typedef struct {
	float	plane[4];
	int	signbits;	/* signx + (signy<<1) + (signz<<2), used as lookup during collision */
} Patchplane;

typedef struct {
	int		surfacePlane;
	int		numBorders;	/* 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels */
	int		borderPlanes[4+6+16];
	int		borderInward[4+6+16];
	qbool		borderNoAdjust[4+6+16];
} facet_t;

typedef struct patchCollide_s {
	Vec3		bounds[2];
	int		numPlanes;	/* surface planes plus edge planes */
	Patchplane	*planes;
	int		numFacets;
	facet_t		*facets;
} patchCollide_t;


#define MAX_GRID_SIZE 129

typedef struct {
	int		width;
	int		height;
	qbool		wrapWidth;
	qbool		wrapHeight;
	Vec3		points[MAX_GRID_SIZE][MAX_GRID_SIZE];	/* [width][height] */
} cGrid_t;

#define SUBDIVIDE_DISTANCE	16	/* 4	// never more than this units away from curve */
#define PLANE_TRI_EPSILON	0.1
#define WRAP_POINT_EPSILON	0.1


struct patchCollide_s *CM_GeneratePatchCollide(int width, int height,
					       Vec3 *points);
