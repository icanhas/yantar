/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef AASINTERN
/* initialize calculating the reachabilities */
void AAS_InitReachability(void);
/* continue calculating the reachabilities */
int AAS_ContinueInitReachability(float time);
int AAS_BestReachableLinkArea(aas_link_t *areas);
#endif	/* AASINTERN */

/* returns true if the are has reachabilities to other areas */
int AAS_AreaReachability(int areanum);
/* returns the best reachable area and goal origin for a bounding box at the given origin */
int AAS_BestReachableArea(Vec3 origin, Vec3 mins, Vec3 maxs,
			  Vec3 goalorigin);
/* returns the best jumppad area from which the bbox at origin is reachable */
int AAS_BestReachableFromJumpPadArea(Vec3 origin, Vec3 mins, Vec3 maxs);
/* returns the next reachability using the given model */
int AAS_NextModelReachability(int num, int modelnum);
/* returns the total area of the ground faces of the given area */
float AAS_AreaGroundFaceArea(int areanum);
/* returns true if the area is crouch only */
int AAS_AreaCrouch(int areanum);
/* returns true if a player can swim in this area */
int AAS_AreaSwim(int areanum);
/* returns true if the area is filled with a liquid */
int AAS_AreaLiquid(int areanum);
/* returns true if the area contains lava */
int AAS_AreaLava(int areanum);
/* returns true if the area contains slime */
int AAS_AreaSlime(int areanum);
/* returns true if the area has one or more ground faces */
int AAS_AreaGrounded(int areanum);
/* returns true if the area has one or more ladder faces */
int AAS_AreaLadder(int areanum);
/* returns true if the area is a jump pad */
int AAS_AreaJumpPad(int areanum);
/* returns true if the area is donotenter */
int AAS_AreaDoNotEnter(int areanum);
