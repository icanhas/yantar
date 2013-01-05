/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "qfiles.h"

void		CM_LoadMap(const char *name, qbool clientload,
			   int *checksum);
void		CM_ClearMap(void);
Cliphandle	CM_InlineModel(int index);	/* 0 = world, 1 + are bmodels */
Cliphandle	CM_TempBoxModel(const Vec3 mins, const Vec3 maxs, int capsule);
void		CM_ModelBounds(Cliphandle model, Vec3 mins, Vec3 maxs);
int		CM_NumClusters(void);
int		CM_NumInlineModels(void);
char*	CM_EntityString(void);
/* returns an ORed contents mask */
int		CM_PointContents(const Vec3 p, Cliphandle model);
int		CM_TransformedPointContents(const Vec3 p, Cliphandle model,
			const Vec3 origin, const Vec3 angles);
void		CM_BoxTrace(Trace *results, const Vec3 start,
			const Vec3 end, Vec3 mins, Vec3 maxs,
			Cliphandle model, int brushmask, int capsule);
void		CM_TransformedBoxTrace(Trace *results, const Vec3 start,
			const Vec3 end, Vec3 mins, Vec3 maxs,
			Cliphandle model, int brushmask,
			const Vec3 origin, const Vec3 angles,
			int capsule);
byte*	CM_ClusterPVS(int cluster);
int		CM_PointLeafnum(const Vec3 p);
/* only returns non-solid leafs
 * overflow if return listsize and if *lastLeaf != list[listsize-1] */
int		CM_BoxLeafnums(const Vec3 mins, const Vec3 maxs, int *list,
				int listsize, int *lastLeaf);
int		CM_LeafCluster(int leafnum);
int		CM_LeafArea(int leafnum);
void		CM_AdjustAreaPortalState(int area1, int area2, qbool open);
qbool	CM_AreasConnected(int area1, int area2);
int		CM_WriteAreaBits(byte *buffer, int area);
/* cm/tag.c */
int		CM_LerpTag(Orient *tag,  Cliphandle model,
			int startFrame, int endFrame, float frac,
			const char *tagName);
/* cm/marks.c */
int		CM_MarkFragments(int numPoints, const Vec3 *points,
			const Vec3 projection, int maxPoints,
			Vec3 pointBuffer, int maxFragments,
			Markfrag *fragmentBuffer);
/* cm/patch.c */
void		CM_DrawDebugSurface(void (*drawPoly)(int color, int numPoints,
			float *points));
