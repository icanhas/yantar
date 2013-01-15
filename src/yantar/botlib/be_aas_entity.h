/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef AASINTERN

void AAS_InvalidateEntities(void);
void AAS_UnlinkInvalidEntities(void);
void AAS_ResetEntityLinks(void);	/* (sets areas and leaves pointers to NULL) */
int AAS_UpdateEntity(int ent, bot_entitystate_t *state);
/* gives the entity data used for collision detection */
void AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata);

#endif	/* AASINTERN */

void AAS_EntitySize(int entnum, Vec3 mins, Vec3 maxs);
int AAS_EntityModelNum(int entnum);
int AAS_OriginOfMoverWithModelNum(int modelnum, Vec3 origin);
int AAS_BestReachableEntityArea(int entnum);
void AAS_EntityInfo(int entnum, aas_entityinfo_t *info);
int AAS_NextEntity(int entnum);
void AAS_EntityOrigin(int entnum, Vec3 origin);
int AAS_EntityType(int entnum);
int AAS_EntityModelindex(int entnum);
