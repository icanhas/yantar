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

#include "qfiles.h"

void            CM_LoadMap(const char *name, qbool clientload,
			   int *checksum);
void            CM_ClearMap(void);
clipHandle_t    CM_InlineModel(int index);	/* 0 = world, 1 + are bmodels */
clipHandle_t    CM_TempBoxModel(const Vec3 mins, const Vec3 maxs,
				int capsule);
void            CM_ModelBounds(clipHandle_t model, Vec3 mins, Vec3 maxs);
int             CM_NumClusters(void);
int             CM_NumInlineModels(void);
char*CM_EntityString(void);
/* returns an ORed contents mask */
int             CM_PointContents(const Vec3 p, clipHandle_t model);
int             CM_TransformedPointContents(const Vec3 p, clipHandle_t model,
					    const Vec3 origin, const Vec3 angles);
void            CM_BoxTrace(trace_t *results, const Vec3 start,
			    const Vec3 end, Vec3 mins, Vec3 maxs,
			    clipHandle_t model, int brushmask, int capsule);
void            CM_TransformedBoxTrace(trace_t *results, const Vec3 start,
				       const Vec3 end, Vec3 mins, Vec3 maxs,
				       clipHandle_t model, int brushmask,
				       const Vec3 origin, const Vec3 angles,
				       int capsule);
byte*CM_ClusterPVS(int cluster);
int             CM_PointLeafnum(const Vec3 p);
/* only returns non-solid leafs
 * overflow if return listsize and if *lastLeaf != list[listsize-1] */
int             CM_BoxLeafnums(const Vec3 mins, const Vec3 maxs, int *list,
			       int listsize,
			       int *lastLeaf);
int             CM_LeafCluster(int leafnum);
int             CM_LeafArea(int leafnum);
void            CM_AdjustAreaPortalState(int area1, int area2, qbool open);
qbool        CM_AreasConnected(int area1, int area2);
int             CM_WriteAreaBits(byte *buffer, int area);
/* cm_tag.c */
int             CM_LerpTag(orientation_t *tag,  clipHandle_t model,
			   int startFrame, int endFrame, float frac,
			   const char *tagName);
/* cm_marks.c */
int             CM_MarkFragments(int numPoints, const Vec3 *points,
				 const Vec3 projection, int maxPoints,
				 Vec3 pointBuffer, int maxFragments,
				 markFragment_t *fragmentBuffer);
/* cm_patch.c */
void            CM_DrawDebugSurface(void (*drawPoly)(int color, int numPoints,
						     float *points));
