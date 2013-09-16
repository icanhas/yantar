/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "bg.h"
#include "itemlist.h"

Gitem*
BG_FindItemForPowerup(Powerup pw)
{
	int i;

	for(i = 0; i < bg_numItems; i++)
		if((bg_itemlist[i].type == IT_POWERUP ||
		    bg_itemlist[i].type == IT_TEAM ||
		    bg_itemlist[i].type == IT_PERSISTANT_POWERUP) &&
		    bg_itemlist[i].tag == pw)
		then{
			return &bg_itemlist[i];
		}
	return NULL;
}

Gitem *
BG_FindItemForHoldable(Holdable pw)
{
	int i;

	for(i = 0; i < bg_numItems; i++)
		if(bg_itemlist[i].type == IT_HOLDABLE &&
		   bg_itemlist[i].tag == pw)
			return &bg_itemlist[i];

	comerrorf(ERR_DROP, "HoldableItem not found");

	return NULL;
}

Gitem *
BG_FindItemForWeapon(Weapon weapon)
{
	Gitem *it;

	for(it = bg_itemlist + 1; it->classname; it++)
		if((it->type == IT_PRIWEAP || it->type == IT_SECWEAP) 
		  && it->tag == weapon)
		then{
			return it;
		}

	comerrorf(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

Gitem *
BG_FindItem(const char *pickupName)
{
	Gitem *it;

	for(it = bg_itemlist + 1; it->classname; it++)
		if(!Q_stricmp(it->pickupname, pickupName))
			return it;

	return NULL;
}

/*
 * Items can be picked up without actually touching their physical bounds to make
 * grabbing them easier
 */
qbool
BG_PlayerTouchesItem(Playerstate *ps, Entstate *item, int atTime)
{
	Vec3 origin;

	BG_EvaluateTrajectory(&item->traj, atTime, origin);

	/* we are ignoring ducked differences here */
	if(ps->origin[0] - origin[0] > ITEM_PICKUPRADIUS
	   || ps->origin[0] - origin[0] < -ITEM_PICKUPRADIUS
	   || ps->origin[1] - origin[1] > ITEM_PICKUPRADIUS
	   || ps->origin[1] - origin[1] < -ITEM_PICKUPRADIUS
	   || ps->origin[2] - origin[2] > ITEM_PICKUPRADIUS
	   || ps->origin[2] - origin[2] < -ITEM_PICKUPRADIUS)
	then{
		return qfalse;
	}

	return qtrue;
}

/*
 * Returns false if the item should not be picked up.
 * This needs to be the same for client side prediction and server use.
 */
qbool
BG_CanItemBeGrabbed(int gametype, const Entstate *ent,
		    const Playerstate *ps)
{
	Gitem *item;

	if(ent->modelindex < 1 || ent->modelindex >= bg_numItems)
		comerrorf(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");

	item = &bg_itemlist[ent->modelindex];

	switch(item->type){
	case IT_PRIWEAP:
	case IT_SECWEAP:
		return qtrue;	/* weapons are always picked up */
	case IT_AMMO:
		if(ps->ammo[ item->tag ] >= 200)
			return qfalse;	/* can't hold any more */
		return qtrue;
	case IT_SHIELD:
		if(ps->stats[STAT_SHIELD] >= ps->stats[STAT_MAX_HEALTH] * 2)
			return qfalse;
		return qtrue;
	case IT_HEALTH:
		/* 
		 * Small and mega healths will go over the max, otherwise
		 * don't pick up if already at max 
		 */
		if(item->quantity == 5 || item->quantity == 100){
			if(ps->stats[STAT_HEALTH] >=
			   ps->stats[STAT_MAX_HEALTH] * 2)
				return qfalse;
			return qtrue;
		}
		if(ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
			return qfalse;
		return qtrue;
	case IT_POWERUP:
		return qtrue;	/* powerups are always picked up */
	case IT_PERSISTANT_POWERUP:
		/* FIXME */
		/* check team only */
		if((ent->generic1 & 2) && (ps->persistant[PERS_TEAM] != TEAM_RED))
			return qfalse;
		if((ent->generic1 & 4) &&
		   (ps->persistant[PERS_TEAM] != TEAM_BLUE))
			return qfalse;
		return qtrue;
	case IT_TEAM:	/* team items, such as flags */
		if(gametype == GT_1FCTF){
			/* neutral flag can always be picked up */
			if(item->tag == PW_NEUTRALFLAG)
				return qtrue;
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->tag == PW_BLUEFLAG  &&
				   ps->powerups[PW_NEUTRALFLAG])
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
				if(item->tag == PW_REDFLAG  &&
				   ps->powerups[PW_NEUTRALFLAG])
					return qtrue;
		}
		if(gametype == GT_CTF){
			/* 
			 * ent->modelindex2 is non-zero on items if they are dropped
			 * we need to know this because we can pick up our dropped flag (and return it)
			 * but we can't pick up our flag at base 
			 */
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->tag == PW_BLUEFLAG ||
				   (item->tag == PW_REDFLAG &&
				    ent->modelindex2) ||
				   (item->tag == PW_REDFLAG &&
				    ps->powerups[PW_BLUEFLAG]))
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
				if(item->tag == PW_REDFLAG ||
				   (item->tag == PW_BLUEFLAG &&
				    ent->modelindex2) ||
				   (item->tag == PW_BLUEFLAG &&
				    ps->powerups[PW_REDFLAG]))
					return qtrue;
		}
		return qfalse;
	case IT_HOLDABLE:
		/* can only hold one item at a time */
		if(ps->stats[STAT_HOLDABLE_ITEM])
			return qfalse;
		return qtrue;
	case IT_BAD:
		comerrorf(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
	default:
#ifndef Q3_VM
#ifndef NDEBUG
		comprintf("BG_CanItemBeGrabbed: unknown enum %d\n",
			item->type);
#endif
#endif
		break;
	}

	return qfalse;
}
