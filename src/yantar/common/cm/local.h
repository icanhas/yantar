/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "polylib.h"

#define MAX_SUBMODELS		256
#define BOX_MODEL_HANDLE	255
#define CAPSULE_MODEL_HANDLE	254


typedef struct {
	Cplane	*plane;
	int		children[2];	/* negative numbers are leafs */
} cNode_t;

typedef struct {
	int	cluster;
	int	area;

	int	firstLeafBrush;
	int	numLeafBrushes;

	int	firstLeafSurface;
	int	numLeafSurfaces;
} cLeaf_t;

typedef struct Cmodel {
	Vec3	mins, maxs;
	cLeaf_t leaf;	/* submodels don't reference the main tree */
} Cmodel;

typedef struct {
	Cplane	*plane;
	int		surfaceFlags;
	int		shaderNum;
} cbrushside_t;

typedef struct {
	int		shaderNum;	/* the shader that determined the contents */
	int		contents;
	Vec3		bounds[2];
	int		numsides;
	cbrushside_t	*sides;
	int		checkcount;	/* to avoid repeated testings */
} cbrush_t;


typedef struct {
	int	checkcount;	/* to avoid repeated testings */
	int	surfaceFlags;
	int	contents;
	struct patchCollide_s *pc;
} cPatch_t;


typedef struct {
	int	floodnum;
	int	floodvalid;
} cArea_t;

typedef struct {
	char		name[MAX_QPATH];

	int		numShaders;
	Dmaterial	*shaders;

	int		numBrushSides;
	cbrushside_t	*brushsides;

	int		numPlanes;
	Cplane	*planes;

	int		numNodes;
	cNode_t		*nodes;

	int		numLeafs;
	cLeaf_t		*leafs;

	int		numLeafBrushes;
	int		*leafbrushes;

	int		numLeafSurfaces;
	int		*leafsurfaces;

	int		numSubModels;
	Cmodel	*cmodels;

	int		numBrushes;
	cbrush_t	*brushes;

	int		numClusters;
	int		clusterBytes;
	byte		*visibility;
	qbool		vised;	/* if false, visibility is just a single cluster of ffs */

	int		numEntityChars;
	char		*entityString;

	int		numAreas;
	cArea_t		*areas;
	int		*areaPortals;	/* [ numAreas*numAreas ] reference counts */

	int		numSurfaces;
	cPatch_t	**surfaces;	/* non-patches will be NULL */

	int		floodvalid;
	int		checkcount;	/* incremented on each trace */
} clipMap_t;


/* keep 1/8 unit away to keep the position valid before network snapping
 * and to avoid various numeric issues */
#define SURFACE_CLIP_EPSILON (0.125)

extern clipMap_t cm;
extern int	c_pointcontents;
extern int	c_traces, c_brush_traces, c_patch_traces;
extern Cvar *cm_noAreas;
extern Cvar *cm_noCurves;
extern Cvar *cm_playerCurveClip;

/* cm_test.c */

/* Used for oriented capsule collision detection */
typedef struct {
	qbool		use;
	float		radius;
	float		halfheight;
	Vec3		offset;
} Sphere;

typedef struct {
	Vec3		start;
	Vec3		end;
	Vec3		size[2];	/* size of the box being swept through the model */
	Vec3		offsets[8];	/* [signbits][x] = either size[0][x] or size[1][x] */
	float		maxOffset;	/* longest corner length from origin */
	Vec3		extents;	/* greatest of abs(size[0]) and abs(size[1]) */
	Vec3		bounds[2];	/* enclosing box of start and end surrounding by size */
	Vec3		modelOrigin;	/* origin of the model tracing through */
	int		contents;	/* ored contents of the model tracing through */
	qbool		isPoint;	/* optimized case */
	Trace		trace;		/* returned from trace call */
	Sphere	sphere;		/* sphere for oriendted capsule collision */
} Tracework;

typedef struct leafList_s {
	int		count;
	int		maxcount;
	qbool		overflowed;
	int		*list;
	Vec3		bounds[2];
	int		lastLeaf;	/* for overflows where each leaf can't be stored individually */
	void (*storeLeafs)(struct leafList_s *ll, int nodenum);
} leafList_t;


int CM_BoxBrushes(const Vec3 mins, const Vec3 maxs, cbrush_t **list,
		  int listsize);

void CM_StoreLeafs(leafList_t *ll, int nodenum);
void CM_StoreBrushes(leafList_t *ll, int nodenum);

void CM_BoxLeafnums_r(leafList_t *ll, int nodenum);

Cmodel*CM_ClipHandleToModel(Cliphandle handle);
qbool CM_BoundsIntersect(const Vec3 mins, const Vec3 maxs,
			    const Vec3 mins2,
			    const Vec3 maxs2);
qbool CM_BoundsIntersectPoint(const Vec3 mins, const Vec3 maxs,
				 const Vec3 point);

/* cm_patch.c */

struct patchCollide_s *CM_GeneratePatchCollide(int width, int height,
					       Vec3 *points);
void CM_TraceThroughPatchCollide(Tracework *tw,
				 const struct patchCollide_s *pc);
qbool CM_PositionTestInPatchCollide(Tracework *tw,
				       const struct patchCollide_s *pc);
void CM_ClearLevelPatches(void);
