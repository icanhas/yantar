/* load bsp, models, etc */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

#ifdef BSPC
#include "../bspc/l_qfiles.h"

void
SetPlaneSignbits(Cplane *out)
{
	int bits, j;

	/* for fast box on planeside test */
	bits = 0;
	for(j=0; j<3; j++)
		if(out->normal[j] < 0)
			bits |= 1<<j;
	out->signbits = bits;
}
#endif	/* BSPC */

enum {
	/*
	 * to allow boxes to be treated as brush models, we allocate
	 * some extra indexes along with those needed by the map 
	 */
	BOX_BRUSHES	= 1,
	BOX_SIDES	= 6,
	BOX_LEAFS	= 2,
	BOX_PLANES	= 12,

	VIS_HEADER	= 8,

	MAX_PATCH_VERTS	= 1024
};

#define LL(x) x=LittleLong(x)

clipMap_t cm;
int c_pointcontents;
int c_traces, c_brush_traces, c_patch_traces;
byte *cmod_base;
#ifndef BSPC
Cvar *cm_noAreas;
Cvar *cm_noCurves;
Cvar *cm_playerCurveClip;
#endif
Cmodel box_model;
Cplane *box_planes;
cbrush_t *box_brush;

void    CM_InitBoxHull(void);
void    CM_FloodAreaConnections(void);

/*
 * Map loading
 */

void
CMod_LoadShaders(Lump *l)
{
	Dmaterial *in, *out;
	int i, count;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "CMod_LoadShaders: funny lump size");
	count = l->filelen / sizeof(*in);

	if(count < 1)
		comerrorf (ERR_DROP, "Map with no shaders");
	cm.shaders = hunkalloc(count * sizeof(*cm.shaders), h_high);
	cm.numShaders = count;

	Q_Memcpy(cm.shaders, in, count * sizeof(*cm.shaders));

	out = cm.shaders;
	for(i=0; i<count; i++, in++, out++){
		out->contentFlags = LittleLong(out->contentFlags);
		out->surfaceFlags = LittleLong(out->surfaceFlags);
	}
}

void
CMod_LoadSubmodels(Lump *l)
{
	Dmodel *in;
	Cmodel	*out;
	int i, j, count;
	int		*indexes;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "CMod_LoadSubmodels: funny lump size");
	count = l->filelen / sizeof(*in);

	if(count < 1)
		comerrorf (ERR_DROP, "Map with no models");
	cm.cmodels = hunkalloc(count * sizeof(*cm.cmodels), h_high);
	cm.numSubModels = count;

	if(count > MAX_SUBMODELS)
		comerrorf(ERR_DROP, "MAX_SUBMODELS exceeded");

	for(i=0; i<count; i++, in++){
		out = &cm.cmodels[i];

		for(j=0; j<3; j++){	/* spread the mins / maxs by a pixel */
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}

		if(i == 0)
			continue;	/* world model doesn't need other info */

		/* make a "leaf" just to hold the model's brushes and surfaces */
		out->leaf.numLeafBrushes = LittleLong(in->numBrushes);
		indexes = hunkalloc(out->leaf.numLeafBrushes * 4, h_high);
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
		for(j = 0; j < out->leaf.numLeafBrushes; j++)
			indexes[j] = LittleLong(in->firstBrush) + j;

		out->leaf.numLeafSurfaces = LittleLong(in->numSurfaces);
		indexes = hunkalloc(out->leaf.numLeafSurfaces * 4, h_high);
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
		for(j = 0; j < out->leaf.numLeafSurfaces; j++)
			indexes[j] = LittleLong(in->firstSurface) + j;
	}
}

void
CMod_LoadNodes(Lump *l)
{
	Dnode *in;
	int child;
	cNode_t *out;
	int i, j, count;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if(count < 1)
		comerrorf (ERR_DROP, "Map has no nodes");
	cm.nodes = hunkalloc(count * sizeof(*cm.nodes), h_high);
	cm.numNodes = count;

	out = cm.nodes;

	for(i=0; i<count; i++, out++, in++){
		out->plane = cm.planes + LittleLong(in->planeNum);
		for(j=0; j<2; j++){
			child = LittleLong (in->children[j]);
			out->children[j] = child;
		}
	}

}

void
CM_BoundBrush(cbrush_t *b)
{
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}

void
CMod_LoadBrushes(Lump *l)
{
	Dbrush *in;
	cbrush_t *out;
	int i, count;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.brushes = hunkalloc((BOX_BRUSHES + count) * sizeof(*cm.brushes),
		h_high);
	cm.numBrushes = count;

	out = cm.brushes;

	for(i=0; i<count; i++, out++, in++){
		out->sides = cm.brushsides + LittleLong(in->firstSide);
		out->numsides = LittleLong(in->numSides);

		out->shaderNum = LittleLong(in->shaderNum);
		if(out->shaderNum < 0 || out->shaderNum >= cm.numShaders)
			comerrorf(ERR_DROP,
				"CMod_LoadBrushes: bad shaderNum: %i",
				out->shaderNum);
		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush(out);
	}

}

void
CMod_LoadLeafs(Lump *l)
{
	int i;
	cLeaf_t *out;
	Dleaf *in;
	int count;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if(count < 1)
		comerrorf (ERR_DROP, "Map with no leafs");

	cm.leafs = hunkalloc((BOX_LEAFS + count) * sizeof(*cm.leafs), h_high);
	cm.numLeafs = count;

	out = cm.leafs;
	for(i=0; i<count; i++, in++, out++){
		out->cluster = LittleLong (in->cluster);
		out->area = LittleLong (in->area);
		out->firstLeafBrush = LittleLong (in->firstLeafBrush);
		out->numLeafBrushes = LittleLong (in->numLeafBrushes);
		out->firstLeafSurface	= LittleLong (in->firstLeafSurface);
		out->numLeafSurfaces	= LittleLong (in->numLeafSurfaces);

		if(out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
		if(out->area >= cm.numAreas)
			cm.numAreas = out->area + 1;
	}

	cm.areas = hunkalloc(cm.numAreas * sizeof(*cm.areas), h_high);
	cm.areaPortals =
		hunkalloc(cm.numAreas * cm.numAreas * sizeof(*cm.areaPortals),
			h_high);
}

void
CMod_LoadPlanes(Lump *l)
{
	int i, j;
	Cplane *out;
	Dplane *in;
	int count;
	int bits;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if(count < 1)
		comerrorf (ERR_DROP, "Map with no planes");
	cm.planes = hunkalloc((BOX_PLANES + count) * sizeof(*cm.planes), h_high);
	cm.numPlanes = count;

	out = cm.planes;

	for(i=0; i<count; i++, in++, out++){
		bits = 0;
		for(j=0; j<3; j++){
			out->normal[j] = LittleFloat (in->normal[j]);
			if(out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		out->signbits = bits;
	}
}

void
CMod_LoadLeafBrushes(Lump *l)
{
	int i;
	int *out;
	int *in;
	int count;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafbrushes =
		hunkalloc((count + BOX_BRUSHES) * sizeof(*cm.leafbrushes),
			h_high);
	cm.numLeafBrushes = count;

	out = cm.leafbrushes;

	for(i=0; i<count; i++, in++, out++)
		*out = LittleLong (*in);
}

void
CMod_LoadLeafSurfaces(Lump *l)
{
	int i, *out, *in, count;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafsurfaces = hunkalloc(count * sizeof(*cm.leafsurfaces), h_high);
	cm.numLeafSurfaces = count;

	out = cm.leafsurfaces;

	for(i=0; i<count; i++, in++, out++)
		*out = LittleLong (*in);
}

void
CMod_LoadBrushSides(Lump *l)
{
	int i;
	cbrushside_t *out;
	Dbrushside *in;
	int count, num;

	in = (void*)(cmod_base + l->fileofs);
	if(l->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.brushsides = hunkalloc((BOX_SIDES + count) * sizeof(*cm.brushsides),
		h_high);
	cm.numBrushSides = count;

	out = cm.brushsides;

	for(i=0; i<count; i++, in++, out++){
		num = LittleLong(in->planeNum);
		out->plane = &cm.planes[num];
		out->shaderNum = LittleLong(in->shaderNum);
		if(out->shaderNum < 0 || out->shaderNum >= cm.numShaders)
			comerrorf(ERR_DROP,
				"CMod_LoadBrushSides: bad shaderNum: %i",
				out->shaderNum);
		out->surfaceFlags = cm.shaders[out->shaderNum].surfaceFlags;
	}
}

void
CMod_LoadEntityString(Lump *l)
{
	cm.entityString = hunkalloc(l->filelen, h_high);
	cm.numEntityChars = l->filelen;
	Q_Memcpy (cm.entityString, cmod_base + l->fileofs, l->filelen);
}

void
CMod_LoadVisibility(Lump *l)
{
	int len;
	byte *buf;

	len = l->filelen;
	if(!len){
		cm.clusterBytes = (cm.numClusters + 31) & ~31;
		cm.visibility	= hunkalloc(cm.clusterBytes, h_high);
		Q_Memset(cm.visibility, 255, cm.clusterBytes);
		return;
	}
	buf = cmod_base + l->fileofs;

	cm.vised = qtrue;
	cm.visibility	= hunkalloc(len, h_high);
	cm.numClusters	= LittleLong(((int*)buf)[0]);
	cm.clusterBytes = LittleLong(((int*)buf)[1]);
	Q_Memcpy (cm.visibility, buf + VIS_HEADER, len - VIS_HEADER);
}

void
CMod_LoadPatches(Lump *surfs, Lump *verts)
{
	Drawvert *dv, *dv_p;
	Dsurf *in;
	int count, i, j, c, width, height, shaderNum;
	cPatch_t *patch;
	Vec3 points[MAX_PATCH_VERTS];

	in = (void*)(cmod_base + surfs->fileofs);
	if(surfs->filelen % sizeof(*in))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	cm.numSurfaces = count = surfs->filelen / sizeof(*in);
	cm.surfaces = hunkalloc(cm.numSurfaces * sizeof(cm.surfaces[0]), h_high);

	dv = (void*)(cmod_base + verts->fileofs);
	if(verts->filelen % sizeof(*dv))
		comerrorf (ERR_DROP, "MOD_LoadBmodel: funny lump size");

	/* scan through all the surfaces, but only load patches,
	 * not planar faces */
	for(i = 0; i < count; i++, in++){
		if(LittleLong(in->surfaceType) != MST_PATCH)
			continue;	/* ignore other surfaces */
		/* FIXME: check for non-colliding patches */

		cm.surfaces[ i ] = patch = hunkalloc(sizeof(*patch), h_high);

		/* load the full drawverts onto the stack */
		width	= LittleLong(in->patchWidth);
		height	= LittleLong(in->patchHeight);
		c = width * height;
		if(c > MAX_PATCH_VERTS)
			comerrorf(ERR_DROP, "ParseMesh: MAX_PATCH_VERTS");

		dv_p = dv + LittleLong(in->firstVert);
		for(j = 0; j < c; j++, dv_p++){
			points[j][0] = LittleFloat(dv_p->xyz[0]);
			points[j][1] = LittleFloat(dv_p->xyz[1]);
			points[j][2] = LittleFloat(dv_p->xyz[2]);
		}

		shaderNum = LittleLong(in->shaderNum);
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

		/* create the internal facet structure */
		patch->pc = CM_GeneratePatchCollide(width, height, points);
	}
}

unsigned
CM_LumpChecksum(Lump *lump)
{
	return LittleLong (blockchecksum (cmod_base + lump->fileofs,
			lump->filelen));
}

unsigned
CM_Checksum(Dheader *header)
{
	unsigned checksums[16];
	checksums[0] = CM_LumpChecksum(&header->lumps[LUMP_SHADERS]);
	checksums[1] = CM_LumpChecksum(&header->lumps[LUMP_LEAFS]);
	checksums[2] = CM_LumpChecksum(&header->lumps[LUMP_LEAFBRUSHES]);
	checksums[3] = CM_LumpChecksum(&header->lumps[LUMP_LEAFSURFACES]);
	checksums[4] = CM_LumpChecksum(&header->lumps[LUMP_PLANES]);
	checksums[5] = CM_LumpChecksum(&header->lumps[LUMP_BRUSHSIDES]);
	checksums[6] = CM_LumpChecksum(&header->lumps[LUMP_BRUSHES]);
	checksums[7] = CM_LumpChecksum(&header->lumps[LUMP_MODELS]);
	checksums[8] = CM_LumpChecksum(&header->lumps[LUMP_NODES]);
	checksums[9] = CM_LumpChecksum(&header->lumps[LUMP_SURFACES]);
	checksums[10] = CM_LumpChecksum(&header->lumps[LUMP_DRAWVERTS]);

	return LittleLong(blockchecksum(checksums, 11 * 4));
}

/*
 * Loads in the map and all submodels
 */
void
CM_LoadMap(const char *name, qbool clientload, int *checksum)
{
	union {
		int	*i;
		void	*v;
	} buf;
	int i, length;
	Dheader header;
	static uint last_checksum;

	if(!name || !name[0])
		comerrorf(ERR_DROP, "CM_LoadMap: NULL name");

#ifndef BSPC
	cm_noAreas = cvarget ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = cvarget ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = cvarget ("cm_playerCurveClip", "1",
		CVAR_ARCHIVE|CVAR_CHEAT);
#endif
	comdprintf("CM_LoadMap( %s, %i )\n", name, clientload);

	if(!strcmp(cm.name, name) && clientload){
		*checksum = last_checksum;
		return;
	}

	/* free old stuff */
	Q_Memset(&cm, 0, sizeof(cm));
	CM_ClearLevelPatches();

	if(!name[0]){
		cm.numLeafs = 1;
		cm.numClusters	= 1;
		cm.numAreas	= 1;
		cm.cmodels	= hunkalloc(sizeof(*cm.cmodels), h_high);
		*checksum	= 0;
		return;
	}

	/*
	 * load the file
	 */
#ifndef BSPC
	length = fsreadfile(name, &buf.v);
#else
	length = LoadQuakeFile((quakefile_t*)name, &buf.v);
#endif

	if(!buf.i)
		comerrorf (ERR_DROP, "Couldn't load %s", name);

	last_checksum = LittleLong (blockchecksum (buf.i, length));
	*checksum = last_checksum;

	header = *(Dheader*)buf.i;
	for(i=0; i<sizeof(Dheader)/4; i++)
		((int*)&header)[i] = LittleLong (((int*)&header)[i]);

	if(header.version != BSP_VERSION)
		comerrorf (
			ERR_DROP,
			"CM_LoadMap: %s has wrong version number (%i should be %i)"
			, name, header.version, BSP_VERSION);

	cmod_base = (byte*)buf.i;

	/* load into heap */
	CMod_LoadShaders(&header.lumps[LUMP_SHADERS]);
	CMod_LoadLeafs (&header.lumps[LUMP_LEAFS]);
	CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES]);
	CMod_LoadLeafSurfaces (&header.lumps[LUMP_LEAFSURFACES]);
	CMod_LoadPlanes (&header.lumps[LUMP_PLANES]);
	CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES]);
	CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES]);
	CMod_LoadSubmodels (&header.lumps[LUMP_MODELS]);
	CMod_LoadNodes (&header.lumps[LUMP_NODES]);
	CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES]);
	CMod_LoadVisibility(&header.lumps[LUMP_VISIBILITY]);
	CMod_LoadPatches(&header.lumps[LUMP_SURFACES],
		&header.lumps[LUMP_DRAWVERTS]);

	/* we are NOT freeing the file, because it is cached for the ref */
	fsfreefile (buf.v);

	CM_InitBoxHull ();

	CM_FloodAreaConnections ();

	/* allow this to be cached if it is loaded by the server */
	if(!clientload)
		Q_strncpyz(cm.name, name, sizeof(cm.name));
}

void
CM_ClearMap(void)
{
	Q_Memset(&cm, 0, sizeof(cm));
	CM_ClearLevelPatches();
}

Cmodel        *
CM_ClipHandleToModel(Cliphandle handle)
{
	if(handle < 0)
		comerrorf(ERR_DROP, "CM_ClipHandleToModel: bad handle %i",
			handle);
	if(handle < cm.numSubModels)
		return &cm.cmodels[handle];
	if(handle == BOX_MODEL_HANDLE)
		return &box_model;
	if(handle < MAX_SUBMODELS)
		comerrorf(ERR_DROP,
			"CM_ClipHandleToModel: bad handle %i < %i < %i",
			cm.numSubModels, handle,
			MAX_SUBMODELS);
	comerrorf(ERR_DROP, "CM_ClipHandleToModel: bad handle %i",
		handle + MAX_SUBMODELS);

	return NULL;

}

Cliphandle
CM_InlineModel(int index)
{
	if(index < 0 || index >= cm.numSubModels)
		comerrorf (ERR_DROP, "CM_InlineModel: bad number");
	return index;
}

int
CM_NumClusters(void)
{
	return cm.numClusters;
}

int
CM_NumInlineModels(void)
{
	return cm.numSubModels;
}

char    *
CM_EntityString(void)
{
	return cm.entityString;
}

int
CM_LeafCluster(int leafnum)
{
	if(leafnum < 0 || leafnum >= cm.numLeafs)
		comerrorf (ERR_DROP, "CM_LeafCluster: bad number");
	return cm.leafs[leafnum].cluster;
}

int
CM_LeafArea(int leafnum)
{
	if(leafnum < 0 || leafnum >= cm.numLeafs)
		comerrorf (ERR_DROP, "CM_LeafArea: bad number");
	return cm.leafs[leafnum].area;
}

/*
 * Set up the planes and nodes so that the six floats of a bounding box
 * can just be stored out and get a proper clipping hull structure.
 */
void
CM_InitBoxHull(void)
{
	int i, side;
	Cplane *p;
	cbrushside_t *s;

	box_planes = &cm.planes[cm.numPlanes];

	box_brush = &cm.brushes[cm.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cm.brushsides + cm.numBrushSides;
	box_brush->contents = CONTENTS_BODY;

	box_model.leaf.numLeafBrushes = 1;
/*	box_model.leaf.firstLeafBrush = cm.numBrushes; */
	box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
	cm.leafbrushes[cm.numLeafBrushes] = cm.numBrushes;

	for(i=0; i<6; i++){
		side = i&1;

		/* brush sides */
		s = &cm.brushsides[cm.numBrushSides+i];
		s->plane =      cm.planes + (cm.numPlanes+i*2+side);
		s->surfaceFlags = 0;

		/* planes */
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		clearv3 (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		clearv3 (p->normal);
		p->normal[i>>1] = -1;

		SetPlaneSignbits(p);
	}
}

/*
 * To keep everything totally uniform, bounding boxes are turned into small
 * BSP trees instead of being compared directly.
 * Capsules are handled differently though.
 */
Cliphandle
CM_TempBoxModel(const Vec3 mins, const Vec3 maxs, int capsule)
{
	copyv3(mins, box_model.mins);
	copyv3(maxs, box_model.maxs);

	if(capsule)
		return CAPSULE_MODEL_HANDLE;

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	copyv3(mins, box_brush->bounds[0]);
	copyv3(maxs, box_brush->bounds[1]);

	return BOX_MODEL_HANDLE;
}

void
CM_ModelBounds(Cliphandle model, Vec3 mins, Vec3 maxs)
{
	Cmodel *cmod;

	cmod = CM_ClipHandleToModel(model);
	copyv3(cmod->mins, mins);
	copyv3(cmod->maxs, maxs);
}

