/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		be_aas_debug.h
*
* desc:		AAS
*
* $Archive: /source/code/botlib/be_aas_debug.h $
*
*****************************************************************************/

/* clear the shown debug lines */
void AAS_ClearShownDebugLines(void);
/*  */
void AAS_ClearShownPolygons(void);
/* show a debug line */
void AAS_DebugLine(Vec3 start, Vec3 end, int color);
/* show a permenent line */
void AAS_PermanentLine(Vec3 start, Vec3 end, int color);
/* show a permanent cross */
void AAS_DrawPermanentCross(Vec3 origin, float size, int color);
/* draw a cross in the plane */
void AAS_DrawPlaneCross(Vec3 point, Vec3 normal, float dist, int type,
			int color);
/* show a bounding box */
void AAS_ShowBoundingBox(Vec3 origin, Vec3 mins, Vec3 maxs);
/* show a face */
void AAS_ShowFace(int facenum);
/* show an area */
void AAS_ShowArea(int areanum, int groundfacesonly);
/*  */
void AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly);
/* draw a cros */
void AAS_DrawCross(Vec3 origin, float size, int color);
/* print the travel type */
void AAS_PrintTravelType(int traveltype);
/* draw an arrow */
void AAS_DrawArrow(Vec3 start, Vec3 end, int linecolor, int arrowcolor);
/* visualize the given reachability */
void AAS_ShowReachability(struct aas_reachability_s *reach);
/* show the reachable areas from the given area */
void AAS_ShowReachableAreas(int areanum);
