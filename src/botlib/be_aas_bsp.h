/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		be_aas_bsp.h
*
* desc:		AAS
*
* $Archive: /source/code/botlib/be_aas_bsp.h $
*
*****************************************************************************/

#ifdef AASINTERN
/* loads the given BSP file */
int AAS_LoadBSPFile(void);
/* dump the loaded BSP data */
void AAS_DumpBSPData(void);
/* unlink the given entity from the bsp tree leaves */
void AAS_UnlinkFromBSPLeaves(bsp_link_t *leaves);
/* link the given entity to the bsp tree leaves of the given model */
bsp_link_t*AAS_BSPLinkEntity(Vec3 absmins,
			     Vec3 absmaxs,
			     int entnum,
			     int modelnum);

/* calculates collision with given entity */
qbool AAS_EntityCollision(int entnum,
			     Vec3 start,
			     Vec3 boxmins,
			     Vec3 boxmaxs,
			     Vec3 end,
			     int contentmask,
			     bsp_trace_t *trace);
/* for debugging */
void AAS_PrintFreeBSPLinks(char *str);
/*  */
#endif	/* AASINTERN */

#define MAX_EPAIRKEY 128

/* trace through the world */
bsp_trace_t AAS_Trace(Vec3 start,
		      Vec3 mins,
		      Vec3 maxs,
		      Vec3 end,
		      int passent,
		      int contentmask);
/* returns the contents at the given point */
int AAS_PointContents(Vec3 point);
/* returns true when p2 is in the PVS of p1 */
qbool AAS_inPVS(Vec3 p1, Vec3 p2);
/* returns true when p2 is in the PHS of p1 */
qbool AAS_inPHS(Vec3 p1, Vec3 p2);
/* returns true if the given areas are connected */
qbool AAS_AreasConnected(int area1, int area2);
/* creates a list with entities totally or partly within the given box */
int AAS_BoxEntities(Vec3 absmins, Vec3 absmaxs, int *list, int maxcount);
/* gets the mins, maxs and origin of a BSP model */
void AAS_BSPModelMinsMaxsOrigin(int modelnum, Vec3 angles, Vec3 mins,
				Vec3 maxs,
				Vec3 origin);
/* handle to the next bsp entity */
int AAS_NextBSPEntity(int ent);
/* return the value of the BSP epair key */
int AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
/* get a vector for the BSP epair key */
int AAS_VectorForBSPEpairKey(int ent, char *key, Vec3 v);
/* get a float for the BSP epair key */
int AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
/* get an integer for the BSP epair key */
int AAS_IntForBSPEpairKey(int ent, char *key, int *value);
