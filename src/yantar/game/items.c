/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "bg.h"
#include "game.h"
#include "local.h"
#include "team.h"

/*
 * Items are any object that a player can touch to gain some effect.
 *
 * Pickup will return the number of seconds until they should respawn.
 *
 * all items should pop when dropped in lava or slime
 *
 * Respawnable items don't actually go away when picked up, they are
 * just made invisible and untouchable.  This allows them to ride
 * movers and respawn apropriately.
 */

#define RESPAWN_SHIELD		25
#define RESPAWN_HEALTH		35
#define RESPAWN_AMMO		40
#define RESPAWN_HOLDABLE	60
#define RESPAWN_MEGAHEALTH	35	/* 120 */
#define RESPAWN_POWERUP		120

static int
Pickup_Powerup(Gentity *ent, Gentity *other)
{
	int	quantity;
	int	i;
	gClient *client;

	if(!other->client->ps.powerups[ent->item->giTag])
		/* round timing to seconds to make multiple powerup timers
		 * count in sync */
		other->client->ps.powerups[ent->item->giTag] =
			level.time - (level.time % 1000);

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

	/* give any nearby players a "denied" anti-reward */
	for(i = 0; i < level.maxclients; i++){
		Vec3	delta;
		float	len;
		Vec3	forward;
		Trace tr;

		client = &level.clients[i];
		if(client == other->client)
			continue;
		if(client->pers.connected == CON_DISCONNECTED)
			continue;
		if(client->ps.stats[STAT_HEALTH] <= 0)
			continue;

		/* if same team in team game, no sound
		 * cannot use OnSameTeam as it expects to g_entities, not clients */
		if(g_gametype.integer >= GT_TEAM &&
		   other->client->sess.sessionTeam == client->sess.sessionTeam)
			continue;

		/* if too far away, no sound */
		subv3(ent->s.traj.base, client->ps.origin, delta);
		len = normv3(delta);
		if(len > 192)
			continue;

		/* if not facing, no sound */
		anglev3s(client->ps.viewangles, forward, NULL, NULL);
		if(dotv3(delta, forward) < 0.4)
			continue;

		/* if not line of sight, no sound */
		trap_Trace(&tr, client->ps.origin, NULL, NULL, ent->s.traj.base,
			ENTITYNUM_NONE,
			CONTENTS_SOLID);
		if(tr.fraction != 1.0)
			continue;

		/* anti-reward */
		client->ps.persistant[PERS_PLAYEREVENTS] ^=
			PLAYEREVENT_DENIEDREWARD;
	}
	return RESPAWN_POWERUP;
}

#ifdef MISSIONPACK
static int
Pickup_PersistantPowerup(Gentity *ent, Gentity *other)
{
	int	clientNum;
	char	userinfo[MAX_INFO_STRING];
	float	handicap;
	int	max;

	other->client->persistantPowerup = ent;

	switch(ent->item->giTag){
	case PW_GUARD:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		max = (int)(2 *  handicap);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_SHIELD] = max;
		other->client->pers.maxHealth = max;

		break;

	case PW_SCOUT:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxHealth = handicap;
		other->client->ps.stats[STAT_SHIELD] = 0;
		break;

	case PW_DOUBLER:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxHealth = handicap;
		break;
	case PW_AMMOREGEN:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxHealth = handicap;
		break;
	default:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
		handicap = atof(Info_ValueForKey(userinfo, "handicap"));
		if(handicap<=0.0f || handicap>100.0f)
			handicap = 100.0f;
		other->client->pers.maxHealth = handicap;
		break;
	}

	return -1;
}

#endif

static int
Pickup_Holdable(Gentity *ent, Gentity *other)
{

	other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	if(ent->item->giTag == HI_KAMIKAZE)
		other->client->ps.eFlags |= EF_KAMIKAZE;

	return RESPAWN_HOLDABLE;
}

void
Add_Ammo(Gentity *ent, int weapon, int count)
{
	ent->client->ps.ammo[weapon] += count;
	if(ent->client->ps.ammo[weapon] > 200)
		ent->client->ps.ammo[weapon] = 200;
}

static int
Pickup_Ammo(Gentity *ent, Gentity *other)
{
	int quantity;

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	Add_Ammo (other, ent->item->giTag, quantity);

	return RESPAWN_AMMO;
}

static int
Pickup_Weapon(Gentity *ent, Gentity *other, Weapslot sl)
{
	int quantity;

	if(ent->count < 0)
		quantity = 0;	/* None for you, sir! */
	else{
		if(ent->count)
			quantity = ent->count;
		else
			quantity = ent->item->quantity;

		/* dropped items and teamplay weapons always have full ammo */
		if(!(ent->flags & FL_DROPPED_ITEM) 
		  && g_gametype.integer != GT_TEAM)
		then{
			/* 
			 * respawning rules
			 * drop the quantity if they already have over the minimum 
			 */
			if(other->client->ps.ammo[ent->item->giTag] 
			  < quantity)
			then{
				quantity -= other->client->ps.ammo[ent->item->giTag];
			}else
				quantity = 1;	/* only add a single shot */
		}
	}

	/* add the weapon */
	switch(sl){
	case Wpri:
		other->client->ps.stats[STAT_PRIWEAPS] |= (1<<ent->item->giTag);
		break;
	case Wsec:
		other->client->ps.stats[STAT_SECWEAPS] |= (1<<ent->item->giTag);
		break;
	default:
		return g_weaponRespawn.integer;
	}

	Add_Ammo(other, ent->item->giTag, quantity);

	if(ent->item->giTag == Whook)
		other->client->ps.ammo[ent->item->giTag] = -1;	/* unlimited ammo */

	/* team deathmatch has slow weapon respawns */
	if(g_gametype.integer == GT_TEAM)
		return g_weaponTeamRespawn.integer;

	return g_weaponRespawn.integer;
}

static int
Pickup_Health(Gentity *ent, Gentity *other)
{
	int	max;
	int	quantity;

	/* small and mega healths will go over the max */
	if(ent->item->quantity != 5 && ent->item->quantity != 100)
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	else
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;

	if(ent->count)
		quantity = ent->count;
	else
		quantity = ent->item->quantity;

	other->health += quantity;

	if(other->health > max)
		other->health = max;
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if(ent->item->quantity == 100)	/* mega health respawns slow */
		return RESPAWN_MEGAHEALTH;

	return RESPAWN_HEALTH;
}

static int
Pickup_Armor(Gentity *ent, Gentity *other)
{
	other->client->ps.stats[STAT_SHIELD] += ent->item->quantity;
	if(other->client->ps.stats[STAT_SHIELD] >
	   other->client->ps.stats[STAT_MAX_HEALTH] * 2)
		other->client->ps.stats[STAT_SHIELD] =
			other->client->ps.stats[STAT_MAX_HEALTH] * 2;

	return RESPAWN_SHIELD;
}

void
RespawnItem(Gentity *ent)
{
	/* randomly select from teamed entities */
	if(ent->team){
		Gentity *master;
		int	count;
		int	choice;

		if(!ent->teammaster)
			G_Error("RespawnItem: bad teammaster");
		master = ent->teammaster;

		for(count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = rand() % count;

		for(count = 0, ent = master; count < choice;
		    ent = ent->teamchain, count++)
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags	&= ~EF_NODRAW;
	ent->r.svFlags	&= ~SVF_NOCLIENT;
	trap_LinkEntity (ent);

	if(ent->item->giType == IT_POWERUP){
		/* play powerup spawn sound to all clients */
		Gentity *te;

		/* if the powerup respawn sound should Not be global */
		if(ent->speed)
			te = G_TempEntity(ent->s.traj.base, EV_GENERAL_SOUND);
		else
			te = G_TempEntity(ent->s.traj.base, EV_GLOBAL_SOUND);
		te->s.eventParm = G_SoundIndex(Pitemsounds "/poweruprespawn");
		te->r.svFlags	|= SVF_BROADCAST;
	}

	if(ent->item->giType == IT_HOLDABLE && ent->item->giTag ==
	   HI_KAMIKAZE){
		/* play powerup spawn sound to all clients */
		Gentity *te;

		/* if the powerup respawn sound should Not be global */
		if(ent->speed)
			te = G_TempEntity(ent->s.traj.base, EV_GENERAL_SOUND);
		else
			te = G_TempEntity(ent->s.traj.base, EV_GLOBAL_SOUND);
		te->s.eventParm = G_SoundIndex(Pitemsounds "/kamikazerespawn");
		te->r.svFlags	|= SVF_BROADCAST;
	}

	/* play the normal respawn sound only to nearby clients */
	G_AddEvent(ent, EV_ITEM_RESPAWN, 0);

	ent->nextthink = 0;
}

void
Touch_Item(Gentity *ent, Gentity *other, Trace *trace)
{
	int respawn;
	qbool predict;

	if(!other->client)
		return;
	if(other->health < 1)
		return;		/* dead people can't pickup */

	/* the same pickup rules are used for client side and server side */
	if(!BG_CanItemBeGrabbed(g_gametype.integer, &ent->s, &other->client->ps))
		return;

	G_LogPrintf("Item: %i %s\n", other->s.number, ent->item->classname);

	predict = other->client->pers.predictItemPickup;

	/* call the item-specific pickup function */
	switch(ent->item->giType){
	case IT_PRIWEAP:
		respawn = Pickup_Weapon(ent, other, Wpri);
/*		predict = qfalse; */
		break;
	case IT_SECWEAP:
		respawn = Pickup_Weapon(ent, other, Wsec);
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
/*		predict = qfalse; */
		break;
	case IT_SHIELD:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
		break;
#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		respawn = Pickup_PersistantPowerup(ent, other);
		break;
#endif
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	default:
		return;
	}

	if(!respawn)
		return;

	/* play the normal pickup sound */
	if(predict)
		G_AddPredictableEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);
	else
		G_AddEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);

	/* powerup pickups are global broadcasts */
	if(ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM){
		/* if we want the global sound to play */
		if(!ent->speed){
			Gentity *te;

			te = G_TempEntity(ent->s.traj.base,
				EV_GLOBAL_ITEM_PICKUP);
			te->s.eventParm = ent->s.modelindex;
			te->r.svFlags	|= SVF_BROADCAST;
		}else{
			Gentity *te;

			te = G_TempEntity(ent->s.traj.base,
				EV_GLOBAL_ITEM_PICKUP);
			te->s.eventParm = ent->s.modelindex;
			/* only send this temp entity to a single client */
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}

	/* fire item targets */
	G_UseTargets (ent, other);

	/* wait of -1 will not respawn */
	if(ent->wait == -1){
		ent->r.svFlags	|= SVF_NOCLIENT;
		ent->s.eFlags	|= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	/* non zero wait overrides respawn time */
	if(ent->wait)
		respawn = ent->wait;

	/* random can be used to vary the respawn time */
	if(ent->random){
		respawn += crandom() * ent->random;
		if(respawn < 1)
			respawn = 1;
	}

	/* dropped items will not respawn */
	if(ent->flags & FL_DROPPED_ITEM)
		ent->freeAfterEvent = qtrue;

	/* picked up items still stay around, they just don't
	 * draw anything.  This allows respawnable items
	 * to be placed on movers. */
	ent->r.svFlags	|= SVF_NOCLIENT;
	ent->s.eFlags	|= EF_NODRAW;
	ent->r.contents = 0;

	/*
	 * A negative respawn time means to never respawn this item (but don't
	 * delete it).  This is used by items that are respawned by third party
	 * events such as ctf flags 
	 */
	if(respawn <= 0){
		ent->nextthink = 0;
		ent->think = 0;
	}else{
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity(ent);
}

/*
 * Spawns an item and tosses it forward
 */
Gentity *
LaunchItem(Gitem *item, Vec3 origin, Vec3 velocity)
{
	Gentity *dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex	= item - bg_itemlist;	/* store item number in modelindex */
	dropped->s.modelindex2	= 1;			/* This is non-zero is it's a dropped item */

	dropped->classname = item->classname;
	dropped->item = item;
	setv3 (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	setv3 (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin(dropped, origin);
	dropped->s.traj.type	= TR_GRAVITY;
	dropped->s.traj.time	= level.time;
	copyv3(velocity, dropped->s.traj.delta);

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	if((g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF) 
	  && item->giType == IT_TEAM){	/* Special case for CTF flags */
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
		Team_CheckDroppedItem(dropped);
	}else{	/* auto-remove after 30 seconds */
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity (dropped);

	return dropped;
}

/*
 * Spawns an item and tosses it forward
 */
Gentity *
Drop_Item(Gentity *ent, Gitem *item, float angle)
{
	Vec3	velocity;
	Vec3	angles;

	copyv3(ent->s.apos.base, angles);
	angles[YAW] += angle;
	angles[PITCH] = 0;	/* always forward */

	anglev3s(angles, velocity, NULL, NULL);
	scalev3(velocity, 150, velocity);
	velocity[2] += 200 + crandom() * 50;

	return LaunchItem(item, ent->s.traj.base, velocity);
}

/*
 * Respawn the item
 */
static void
Use_Item(Gentity *ent, Gentity *other, Gentity *activator)
{
	RespawnItem(ent);
}

/*
 * Traces down to find where an item should rest, instead of letting them
 * free fall from their spawn points
 */
void
FinishSpawningItem(Gentity *ent)
{
	Trace tr;
	Vec3	dest;

	setv3(ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	setv3(ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;	/* store item number in modelindex */
	ent->s.modelindex2 = 0;				/* zero indicates this isn't a dropped item */

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	/* useing an item causes it to respawn */
	ent->use = Use_Item;

	if(ent->spawnflags & 1)
		/* suspended */
		G_SetOrigin(ent, ent->s.origin);
	else{
		/* drop to floor */
		setv3(dest, ent->s.origin[0], ent->s.origin[1],
			ent->s.origin[2] - 4096);
		trap_Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest,
			ent->s.number,
			MASK_SOLID);
		if(tr.startsolid){
			G_Printf ("FinishSpawningItem: %s startsolid at %s\n",
			  ent->classname, vtos(ent->s.origin));
			G_FreeEntity(ent);
			return;
		}

		/* allow to ride movers */
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin(ent, tr.endpos);
	}

	/* team slaves and targeted items aren't present at start */
	if((ent->flags & FL_TEAMSLAVE) || ent->targetname){
		ent->s.eFlags	|= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	/* powerups don't spawn in for a while */
	if(ent->item->giType == IT_POWERUP){
		float respawn;

		respawn = 45 + crandom() * 15;
		ent->s.eFlags	|= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink	= level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}

	trap_LinkEntity (ent);
}

static qbool itemRegistered[MAX_ITEMS];

void
G_CheckTeamItems(void)
{

	/* Set up team stuff */
	Team_InitGame();

	if(g_gametype.integer == GT_CTF){
		Gitem *item;

		/* check for the two flags */
		item = BG_FindItem("Red Flag");
		if(!item || !itemRegistered[ item - bg_itemlist ])
			G_Printf(S_COLOR_YELLOW
				"WARNING: No team_CTF_redflag in map\n");
		item = BG_FindItem("Blue Flag");
		if(!item || !itemRegistered[ item - bg_itemlist ])
			G_Printf(S_COLOR_YELLOW
				"WARNING: No team_CTF_blueflag in map\n");
	}
	if(g_gametype.integer == GT_1FCTF){
		Gitem *item;

		/* check for all three flags */
		item = BG_FindItem("Red Flag");
		if(!item || !itemRegistered[ item - bg_itemlist ])
			G_Printf(
				S_COLOR_YELLOW
				"WARNING: No team_CTF_redflag in map\n");
		item = BG_FindItem("Blue Flag");
		if(!item || !itemRegistered[ item - bg_itemlist ])
			G_Printf(
				S_COLOR_YELLOW
				"WARNING: No team_CTF_blueflag in map\n");
		item = BG_FindItem("Neutral Flag");
		if(!item || !itemRegistered[ item - bg_itemlist ])
			G_Printf(
				S_COLOR_YELLOW
				"WARNING: No team_CTF_neutralflag in map\n");
	}
}

void
ClearRegisteredItems(void)
{
	memset(itemRegistered, 0, sizeof(itemRegistered));

	/* players always start with the base weapon */
	RegisterItem(BG_FindItemForWeapon(Wmachinegun));
	RegisterItem(BG_FindItemForWeapon(Wmelee));
#ifdef MISSIONPACK
	if(g_gametype.integer == GT_HARVESTER){
		RegisterItem(BG_FindItem("Red Cube"));
		RegisterItem(BG_FindItem("Blue Cube"));
	}
#endif
}

/*
 * The item will be added to the precache list
 */
void
RegisterItem(Gitem *item)
{
	if(!item)
		G_Error("RegisterItem: NULL");
	itemRegistered[ item - bg_itemlist ] = qtrue;
}

/*
 * Write the needed items to a config string
 * so the client will know which ones to precache
 */
void
SaveRegisteredItems(void)
{
	char string[MAX_ITEMS+1];
	int i;
	int count;

	count = 0;
	for(i = 0; i < bg_numItems; i++){
		if(itemRegistered[i]){
			count++;
			string[i] = '1';
		}else
			string[i] = '0';
	}
	string[ bg_numItems ] = 0;

	G_Printf("%i items registered\n", count);
	trap_SetConfigstring(CS_ITEMS, string);
}

int
G_ItemDisabled(Gitem *item)
{

	char name[128];

	Q_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue(name);
}

/*
 * Sets the clipping size and plants the object on the floor.
 *
 * Items can't be immediately dropped to floor, because they might
 * be on an entity that hasn't spawned yet.
 */
void
G_SpawnItem(Gentity *ent, Gitem *item)
{
	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnFloat("wait", "0", &ent->wait);

	RegisterItem(item);
	if(G_ItemDisabled(item))
		return;

	ent->item = item;
	/* some movers spawn on the second frame, so delay item
	 * spawns until the third frame so they can ride trains */
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;	/* items are bouncy */

	if(item->giType == IT_POWERUP){
		G_SoundIndex(Pitemsounds "/poweruprespawn");
		G_SpawnFloat("noglobalsound", "0", &ent->speed);
	}

#ifdef MISSIONPACK
	if(item->giType == IT_PERSISTANT_POWERUP)
		ent->s.generic1 = ent->spawnflags;

#endif
}

void
G_BounceItem(Gentity *ent, Trace *trace)
{
	Vec3	velocity;
	float	dot;
	int	hitTime;

	/* reflect the velocity on the trace plane */
	hitTime = level.previousTime +
		  (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.traj, hitTime, velocity);
	dot = dotv3(velocity, trace->plane.normal);
	maddv3(velocity, -2*dot, trace->plane.normal, ent->s.traj.delta);

	/* cut the velocity to keep from bouncing forever */
	scalev3(ent->s.traj.delta, ent->physicsBounce, ent->s.traj.delta);

	/* check for stop */
	if(trace->plane.normal[2] > 0 && ent->s.traj.delta[2] < 40){
		trace->endpos[2] += 1.0;	/* make sure it is off ground */
		snapv3(trace->endpos);
		G_SetOrigin(ent, trace->endpos);
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	addv3(ent->r.currentOrigin, trace->plane.normal,
		ent->r.currentOrigin);
	copyv3(ent->r.currentOrigin, ent->s.traj.base);
	ent->s.traj.time = level.time;
}

void
G_RunItem(Gentity *ent)
{
	Vec3	origin;
	Trace tr;
	int contents, mask;

	/* if its groundentity has been set to ENTITYNUM_NONE, it may have been pushed off an edge */
	if(ent->s.groundEntityNum == ENTITYNUM_NONE)
		if(ent->s.traj.type != TR_GRAVITY){
			ent->s.traj.type = TR_GRAVITY;
			ent->s.traj.time = level.time;
		}

	if(ent->s.traj.type == TR_STATIONARY){
		/* check think function */
		G_RunThink(ent);
		return;
	}

	/* get current position */
	BG_EvaluateTrajectory(&ent->s.traj, level.time, origin);

	/* trace a line from the previous position to the current position */
	if(ent->clipmask)
		mask = ent->clipmask;
	else
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	/* MASK_SOLID; */
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		ent->r.ownerNum, mask);

	copyv3(tr.endpos, ent->r.currentOrigin);

	if(tr.startsolid)
		tr.fraction = 0;

	trap_LinkEntity(ent);	/* FIXME: avoid this for stationary? */

	/* check think function */
	G_RunThink(ent);

	if(tr.fraction == 1)
		return;

	/* if it is in a nodrop volume, remove it */
	contents = trap_PointContents(ent->r.currentOrigin, -1);
	if(contents & CONTENTS_NODROP){
		if(ent->item && ent->item->giType == IT_TEAM)
			Team_FreeEntity(ent);
		else
			G_FreeEntity(ent);
		return;
	}
	G_BounceItem(ent, &tr);
}
