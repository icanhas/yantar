/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 * AAS entities
 */

#include "shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define MASK_SOLID CONTENTS_PLAYERCLIP

/* FIXME: these might change */
enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER
};

int
AAS_UpdateEntity(int entnum, bot_entitystate_t *state)
{
	int relink;
	aas_entity_t *ent;
	Vec3 absmins, absmaxs;

	if(!aasworld.loaded){
		botimport.Print(PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n");
		return BLERR_NOAASFILE;
	}

	ent = &aasworld.entities[entnum];

	if(!state){
		/* unlink the entity */
		AAS_UnlinkFromAreas(ent->areas);
		/* unlink the entity from the BSP leaves */
		AAS_UnlinkFromBSPLeaves(ent->leaves);
		ent->areas = NULL;
		ent->leaves = NULL;
		return BLERR_NOERROR;
	}

	ent->i.update_time = AAS_Time() - ent->i.ltime;
	ent->i.type = state->type;
	ent->i.flags = state->flags;
	ent->i.ltime = AAS_Time();
	copyv3(ent->i.origin, ent->i.lastvisorigin);
	copyv3(state->old_origin, ent->i.old_origin);
	ent->i.solid = state->solid;
	ent->i.groundent = state->groundent;
	ent->i.modelindex = state->modelindex;
	ent->i.modelindex2 = state->modelindex2;
	ent->i.frame = state->frame;
	ent->i.event = state->event;
	ent->i.eventParm = state->eventParm;
	ent->i.powerups = state->powerups;
	ent->i.weapon = state->weapon;
	ent->i.legsAnim = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;
	ent->i.number = entnum;
	ent->i.valid = qtrue;	/* updated so set valid flag */
	/* link everything the first frame */
	if(aasworld.numframes == 1) relink = qtrue;
	else relink = qfalse;
	if(ent->i.solid == SOLID_BSP){
		/* if the angles of the model changed */
		if(!cmpv3(state->angles, ent->i.angles)){
			copyv3(state->angles, ent->i.angles);
			relink = qtrue;
		}
		/* get the mins and maxs of the model */
		/* FIXME: rotate mins and maxs */
		AAS_BSPModelMinsMaxsOrigin(ent->i.modelindex, ent->i.angles,
			ent->i.mins, ent->i.maxs,
			NULL);
	}else if(ent->i.solid == SOLID_BBOX){
		/* if the bounding box size changed */
		if(!cmpv3(state->mins, ent->i.mins) ||
		   !cmpv3(state->maxs, ent->i.maxs)){
			copyv3(state->mins, ent->i.mins);
			copyv3(state->maxs, ent->i.maxs);
			relink = qtrue;
		}
		copyv3(state->angles, ent->i.angles);
	}
	/* if the origin changed */
	if(!cmpv3(state->origin, ent->i.origin)){
		copyv3(state->origin, ent->i.origin);
		relink = qtrue;
	}
	/* if the entity should be relinked */
	if(relink)
		/* don't link the world model */
		if(entnum != ENTITYNUM_WORLD){
			/* absolute mins and maxs */
			addv3(ent->i.mins, ent->i.origin, absmins);
			addv3(ent->i.maxs, ent->i.origin, absmaxs);
			AAS_UnlinkFromAreas(ent->areas);	/* unlink the entity */
			/* relink the entity to the AAS areas (use the larges bbox) */
			ent->areas =
				AAS_LinkEntityClientBBox(absmins, absmaxs,
					entnum,
					PRESENCE_NORMAL);
			/* unlink the entity from the BSP leaves */
			AAS_UnlinkFromBSPLeaves(ent->leaves);
			/* link the entity to the world BSP tree */
			ent->leaves = AAS_BSPLinkEntity(absmins, absmaxs, entnum,
				0);
		}
	return BLERR_NOERROR;
}

void
AAS_EntityInfo(int entnum, aas_entityinfo_t *info)
{
	if(!aasworld.initialized){
		botimport.Print(PRT_FATAL,
			"AAS_EntityInfo: aasworld not initialized\n");
		Q_Memset(info, 0, sizeof(aas_entityinfo_t));
		return;
	}

	if(entnum < 0 || entnum >= aasworld.maxentities){
		botimport.Print(PRT_FATAL,
			"AAS_EntityInfo: entnum %d out of range\n",
			entnum);
		Q_Memset(info, 0, sizeof(aas_entityinfo_t));
		return;
	}

	Q_Memcpy(info, &aasworld.entities[entnum].i, sizeof(aas_entityinfo_t));
}

void
AAS_EntityOrigin(int entnum, Vec3 origin)
{
	if(entnum < 0 || entnum >= aasworld.maxentities){
		botimport.Print(PRT_FATAL,
			"AAS_EntityOrigin: entnum %d out of range\n",
			entnum);
		clearv3(origin);
		return;
	}

	copyv3(aasworld.entities[entnum].i.origin, origin);
}

int
AAS_EntityModelindex(int entnum)
{
	if(entnum < 0 || entnum >= aasworld.maxentities){
		botimport.Print(PRT_FATAL,
			"AAS_EntityModelindex: entnum %d out of range\n",
			entnum);
		return 0;
	}
	return aasworld.entities[entnum].i.modelindex;
}

int
AAS_EntityType(int entnum)
{
	if(!aasworld.initialized) return 0;

	if(entnum < 0 || entnum >= aasworld.maxentities){
		botimport.Print(PRT_FATAL,
			"AAS_EntityType: entnum %d out of range\n",
			entnum);
		return 0;
	}
	return aasworld.entities[entnum].i.type;
}

int
AAS_EntityModelNum(int entnum)
{
	if(!aasworld.initialized) return 0;

	if(entnum < 0 || entnum >= aasworld.maxentities){
		botimport.Print(PRT_FATAL,
			"AAS_EntityModelNum: entnum %d out of range\n",
			entnum);
		return 0;
	}
	return aasworld.entities[entnum].i.modelindex;
}

int
AAS_OriginOfMoverWithModelNum(int modelnum, Vec3 origin)
{
	int i;
	aas_entity_t *ent;

	for(i = 0; i < aasworld.maxentities; i++){
		ent = &aasworld.entities[i];
		if(ent->i.type == ET_MOVER)
			if(ent->i.modelindex == modelnum){
				copyv3(ent->i.origin, origin);
				return qtrue;
			}
	}
	return qfalse;
}

void
AAS_EntitySize(int entnum, Vec3 mins, Vec3 maxs)
{
	aas_entity_t *ent;

	if(!aasworld.initialized) return;

	if(entnum < 0 || entnum >= aasworld.maxentities){
		botimport.Print(PRT_FATAL,
			"AAS_EntitySize: entnum %d out of range\n",
			entnum);
		return;
	}

	ent = &aasworld.entities[entnum];
	copyv3(ent->i.mins, mins);
	copyv3(ent->i.maxs, maxs);
}

void
AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata)
{
	aas_entity_t *ent;

	ent = &aasworld.entities[entnum];
	copyv3(ent->i.origin, entdata->origin);
	copyv3(ent->i.angles, entdata->angles);
	addv3(ent->i.origin, ent->i.mins, entdata->absmins);
	addv3(ent->i.origin, ent->i.maxs, entdata->absmaxs);
	entdata->solid = ent->i.solid;
	entdata->modelnum = ent->i.modelindex - 1;
}

void
AAS_ResetEntityLinks(void)
{
	int i;
	for(i = 0; i < aasworld.maxentities; i++){
		aasworld.entities[i].areas = NULL;
		aasworld.entities[i].leaves = NULL;
	}
}

void
AAS_InvalidateEntities(void)
{
	int i;
	for(i = 0; i < aasworld.maxentities; i++){
		aasworld.entities[i].i.valid = qfalse;
		aasworld.entities[i].i.number = i;
	}
}

void
AAS_UnlinkInvalidEntities(void)
{
	int i;
	aas_entity_t *ent;

	for(i = 0; i < aasworld.maxentities; i++){
		ent = &aasworld.entities[i];
		if(!ent->i.valid){
			AAS_UnlinkFromAreas(ent->areas);
			ent->areas = NULL;
			AAS_UnlinkFromBSPLeaves(ent->leaves);
			ent->leaves = NULL;
		}
	}
}

int
AAS_NearestEntity(Vec3 origin, int modelindex)
{
	int i, bestentnum;
	float	dist, bestdist;
	aas_entity_t *ent;
	Vec3	dir;

	bestentnum = 0;
	bestdist = 99999;
	for(i = 0; i < aasworld.maxentities; i++){
		ent = &aasworld.entities[i];
		if(ent->i.modelindex != modelindex) continue;
		subv3(ent->i.origin, origin, dir);
		if(abs(dir[0]) < 40)
			if(abs(dir[1]) < 40){
				dist = lenv3(dir);
				if(dist < bestdist){
					bestdist = dist;
					bestentnum = i;
				}
			}
	}
	return bestentnum;
}

int
AAS_BestReachableEntityArea(int entnum)
{
	aas_entity_t *ent;

	ent = &aasworld.entities[entnum];
	return AAS_BestReachableLinkArea(ent->areas);
}

int
AAS_NextEntity(int entnum)
{
	if(!aasworld.loaded) return 0;

	if(entnum < 0) entnum = -1;
	while(++entnum < aasworld.maxentities)
		if(aasworld.entities[entnum].i.valid) return entnum;
	return 0;
}
