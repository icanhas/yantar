/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef AASINTERN

void AAS_InitAASLinkHeap(void);
void AAS_InitAASLinkedEntities(void);
void AAS_FreeAASLinkHeap(void);
void AAS_FreeAASLinkedEntities(void);
aas_face_t*AAS_AreaGroundFace(int areanum, Vec3 point);
aas_face_t*AAS_TraceEndFace(aas_Trace *trace);
aas_plane_t*AAS_PlaneFromNum(int planenum);
aas_link_t*AAS_AASLinkEntity(Vec3 absmins, Vec3 absmaxs, int entnum);
aas_link_t*AAS_LinkEntityClientBBox(Vec3 absmins, Vec3 absmaxs, int entnum,
				    int presencetype);
qbool AAS_PointInsideFace(int facenum, Vec3 point, float epsilon);
qbool AAS_InsideFace(aas_face_t *face, Vec3 pnormal, Vec3 point,
			float epsilon);
void AAS_UnlinkFromAreas(aas_link_t *areas);

#endif	/* AASINTERN */

/* returns the mins and maxs of the bounding box for the given presence type */
void AAS_PresenceTypeBoundingBox(int presencetype, Vec3 mins, Vec3 maxs);
/* returns the cluster the area is in (negative portal number if the area is a portal) */
int AAS_AreaCluster(int areanum);
/* returns the presence type(s) of the area */
int AAS_AreaPresenceType(int areanum);
/* returns the presence type(s) at the given point */
int AAS_PointPresenceType(Vec3 point);
/* returns the result of the trace of a client bbox */
aas_Trace AAS_TraceClientBBox(Vec3 start, Vec3 end, int presencetype,
				int passent);
/* stores the areas the trace went through and returns the number of passed areas */
int AAS_TraceAreas(Vec3 start, Vec3 end, int *areas, Vec3 *points,
		   int maxareas);
/* returns the areas the bounding box is in */
int AAS_BBoxAreas(Vec3 absmins, Vec3 absmaxs, int *areas, int maxareas);
/* return area information */
int AAS_AreaInfo(int areanum, aas_areainfo_t *info);
/* returns the area the point is in */
int AAS_PointAreaNum(Vec3 point);
int AAS_PointReachabilityAreaIndex(Vec3 point);
/* returns the plane the given face is in */
void AAS_FacePlane(int facenum, Vec3 normal, float *dist);
