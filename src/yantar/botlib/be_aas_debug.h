/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

void	AAS_ClearShownDebugLines(void);
void	AAS_ClearShownPolygons(void);
void	AAS_DebugLine(Vec3 start, Vec3 end, int color);
void	AAS_PermanentLine(Vec3 start, Vec3 end, int color);
void	AAS_DrawPermanentCross(Vec3 origin, float size, int color);
void	AAS_DrawPlaneCross(Vec3 point, Vec3 normal, float dist, int type,
		int color);
void	AAS_ShowBoundingBox(Vec3 origin, Vec3 mins, Vec3 maxs);
void	AAS_ShowFace(int facenum);
void	AAS_ShowArea(int areanum, int groundfacesonly);
void	AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly);
void	AAS_DrawCross(Vec3 origin, float size, int color);
void	AAS_PrintTravelType(int traveltype);
void	AAS_DrawArrow(Vec3 start, Vec3 end, int linecolor, int arrowcolor);
void	AAS_ShowReachability(struct aas_reachability_s *reach);
void	AAS_ShowReachableAreas(int areanum);
