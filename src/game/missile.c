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

enum { Presteptime = 50 };

void
G_BounceMissile(gentity_t *ent, trace_t *trace)
{
	Vec3 velocity;
	float dot;
	int hitTime;

	/* reflect the velocity on the trace plane */
	hitTime = level.previousTime +
		  (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = dotv3(velocity, trace->plane.normal);
	maddv3(velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta);

	if(ent->s.eFlags & EF_BOUNCE_HALF){
		scalev3(ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta);
		/* check for stop */
		if(trace->plane.normal[2] > 0.2 &&
		   lenv3(ent->s.pos.trDelta) < 40){
			G_SetOrigin(ent, trace->endpos);
			ent->s.time = level.time / 4;
			return;
		}
	}
	addv3(ent->r.currentOrigin, trace->plane.normal,
		ent->r.currentOrigin);
	copyv3(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}


/* Explode a missile without an impact */
void
G_ExplodeMissile(gentity_t *ent)
{
	Vec3 dir;
	Vec3 origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	snapv3(origin);
	G_SetOrigin(ent, origin);

	/* we don't have a valid direction, so just point straight up */
	dir[0] = dir[1] = 0;
	dir[2] = 1;
	ent->s.eType = ET_GENERAL;
	G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));
	ent->freeAfterEvent = qtrue;

	/* splash damage */
	if(ent->splashDamage)
		if(G_RadiusDamage(ent->r.currentOrigin, ent->parent,
			   ent->splashDamage, ent->splashRadius, ent
			   , ent->splashMethodOfDeath))
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
	trap_LinkEntity(ent);
}

#ifdef MISSIONPACK
static void
ProximityMine_Explode(gentity_t *mine)
{
	G_ExplodeMissile(mine);
	/* if the prox mine has a trigger free it */
	if(mine->activator){
		G_FreeEntity(mine->activator);
		mine->activator = nil;
	}
}

static void
ProximityMine_Die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker,
	int damage, int mod)
{
	UNUSED(inflictor);
	UNUSED(attacker);
	UNUSED(damage);
	UNUSED(mod);
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

void
ProximityMine_Trigger(gentity_t *trigger, gentity_t *other, trace_t *trace)
{
	Vec3 v;
	gentity_t *mine;
	
	UNUSED(trace);
	if(!other->client)
		return;
	/* trigger is a cube, do a distance test now to act as if it's a sphere */
	subv3(trigger->s.pos.trBase, other->s.pos.trBase, v);
	if(lenv3(v) > trigger->parent->splashRadius)
		return;
		
	if(g_gametype.integer >= GT_TEAM)
		/* don't trigger same team mines */
		if(trigger->parent->s.generic1 == other->client->sess.sessionTeam)
			return;
	/*
	 * now check for ability to damage so we don't get triggered through
	 * walls, closed doors, etc.
	 */
	if(!CanDamage(other, trigger->s.pos.trBase))
		return;
	/* trigger the mine! */
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent(mine, EV_PROXIMITY_MINE_TRIGGER, 0);
	mine->nextthink = level.time + 500;
	G_FreeEntity(trigger);
}

static void
ProximityMine_Activate(gentity_t *ent)
{
	gentity_t *trigger;
	float r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;
	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;
	ent->s.loopSound = G_SoundIndex(Pproxsounds "/wstbtick.wav");

	/* build the proximity trigger */
	trigger = G_Spawn ();
	trigger->classname = "proxmine_trigger";
	r = ent->splashRadius;
	setv3(trigger->r.mins, -r, -r, -r);
	setv3(trigger->r.maxs, r, r, r);
	G_SetOrigin(trigger, ent->s.pos.trBase);
	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;
	trap_LinkEntity (trigger);
	/* set pointer to trigger so the entity can be freed when the mine explodes */
	ent->activator = trigger;
}

static void
ProximityMine_ExplodeOnPlayer(gentity_t *mine)
{
	gentity_t *player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if(player->client->invulnerabilityTime > level.time){
		G_Damage(player, mine->parent, mine->parent, vec3_origin,
			mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK,
			MOD_JUICED);
		player->client->invulnerabilityTime = 0;
		G_TempEntity(player->client->ps.origin, EV_JUICED);
	}else{
		G_SetOrigin(mine, player->s.pos.trBase);
		/* make sure the explosion gets to the client */
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile(mine);
	}
}

static void
ProximityMine_Player(gentity_t *mine, gentity_t *player)
{
	if(mine->s.eFlags & EF_NODRAW)
		return;
	G_AddEvent(mine, EV_PROXIMITY_MINE_STICK, 0);
	
	if(player->s.eFlags & EF_TICKING){
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}
	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;
	mine->s.eFlags	|= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	clearv3(mine->s.pos.trDelta);
	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if(player->client->invulnerabilityTime > level.time)
		mine->nextthink = level.time + 2 * 1000;
	else
		mine->nextthink = level.time + 10 * 1000;
}
#endif

void
G_MissileImpact(gentity_t *ent, trace_t *trace)
{
	gentity_t *other;
	qbool hitClient = qfalse;
#ifdef MISSIONPACK
	Vec3 forward, impactpoint, bouncedir;
	int eFlags;
#endif
	other = &g_entities[trace->entityNum];

	/* check for bounce */
	if(!other->takedamage &&
	   (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF))){
		G_BounceMissile(ent, trace);
		G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		return;
	}

#ifdef MISSIONPACK
	if(other->takedamage){
		if(ent->s.weapon != WP_PROX_LAUNCHER)
			if(other->client &&
			   other->client->invulnerabilityTime > level.time){
				copyv3(ent->s.pos.trDelta, forward);
				normv3(forward);
				if(G_InvulnerabilityEffect(other, forward,
				  ent->s.pos.trBase, impactpoint, bouncedir)){
					copyv3(bouncedir, trace->plane.normal);
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile(ent, trace);
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
	}
#endif
	/* impact damage */
	if(other->takedamage)
		/* FIXME: wrong damage direction? */
		if(ent->damage){
			Vec3 velocity;

			if(LogAccuracyHit(other,
				   &g_entities[ent->r.ownerNum])){
				g_entities[ent->r.ownerNum].client->
				accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if(lenv3(velocity) == 0)
				velocity[2] = 1;	/* stepped on a grenade */
			G_Damage (other, ent, &g_entities[ent->r.ownerNum],
				velocity, ent->s.origin, ent->damage, 0,
				ent->methodOfDeath);
		}
#ifdef MISSIONPACK
	if(ent->s.weapon == WP_PROX_LAUNCHER){
		if(ent->s.pos.trType != TR_GRAVITY)
			return;
		/* if it's a player, stick it on to them (flag them and remove this entity) */
		if(other->s.eType == ET_PLAYER && other->health > 0){
			ProximityMine_Player(ent, other);
			return;
		}

		snapv3Towards(trace->endpos, ent->s.pos.trBase);
		G_SetOrigin(ent, trace->endpos);
		ent->s.pos.trType = TR_STATIONARY;
		clearv3(ent->s.pos.trDelta);
		G_AddEvent(ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags);
		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;
		v3toeuler(trace->plane.normal, ent->s.angles);
		ent->s.angles[0] += 90;
		/* link the prox mine to the other entity */
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		copyv3(trace->plane.normal, ent->movedir);
		setv3(ent->r.mins, -4, -4, -4);
		setv3(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);
		return;
	}
#endif

	if(!strcmp(ent->classname, "hook")){
		gentity_t	*nent;
		Vec3		v;

		nent = G_Spawn();
		if(other->takedamage && other->client){

			G_AddEvent(nent, EV_MISSILE_HIT,
				DirToByte(trace->plane.normal));
			nent->s.otherEntityNum = other->s.number;
			ent->enemy = other;
			v[0] = other->r.currentOrigin[0] +
			       (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] +
			       (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] +
			       (other->r.mins[2] + other->r.maxs[2]) * 0.5;
			snapv3Towards(v, ent->s.pos.trBase);	/* save net bandwidth */
		}else{
			copyv3(trace->endpos, v);
			G_AddEvent(nent, EV_MISSILE_MISS,
				DirToByte(trace->plane.normal));
			ent->enemy = nil;
		}
		snapv3Towards(v, ent->s.pos.trBase);	/* save net bandwidth */
		nent->freeAfterEvent = qtrue;
		/* change over to a normal entity right at the point of impact */
		nent->s.eType	= ET_GENERAL;
		ent->s.eType	= ET_GRAPPLE;
		G_SetOrigin(ent, v);
		G_SetOrigin(nent, v);
		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;
		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		copyv3(ent->r.currentOrigin,
			ent->parent->client->ps.grapplePoint);
		trap_LinkEntity(ent);
		trap_LinkEntity(nent);
		return;
	}
	/* 
	 * is it cheaper in bandwidth to just remove this ent and create a new
	 * one, rather than changing the missile into the explosion?
	 */
	if(other->takedamage && other->client){
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	}else if(trace->surfaceFlags & SURF_METALSTEPS)
		G_AddEvent(ent, EV_MISSILE_MISS_METAL,
			DirToByte(trace->plane.normal));
	else
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));

	ent->freeAfterEvent = qtrue;
	/* change over to a normal entity right at the point of impact */
	ent->s.eType = ET_GENERAL;
	snapv3Towards(trace->endpos, ent->s.pos.trBase);	/* save net bandwidth */
	G_SetOrigin(ent, trace->endpos);

	/* splash damage (doesn't apply to person directly hit) */
	if(ent->splashDamage)
		if(G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage,
			   ent->splashRadius,
			   other, ent->splashMethodOfDeath))
			if(!hitClient)
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
	trap_LinkEntity(ent);
}

void
G_RunMissile(gentity_t *ent)
{
	Vec3 origin;
	trace_t tr;
	int passent;

	/* get current position */
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	/* if this missile bounced off an invulnerability sphere */
	if(ent->target_ent)
		passent = ent->target_ent->s.number;
#ifdef MISSIONPACK
	/* prox mines that left the owner bbox will attach to anything, even the owner */
	else if(ent->s.weapon == WP_PROX_LAUNCHER && ent->count)
		passent = ENTITYNUM_NONE;
#endif
	else
		/* ignore interactions with the missile owner */
		passent = ent->r.ownerNum;
	/* trace a line from the previous position to the current position */
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		passent,
		ent->clipmask);

	if(tr.startsolid || tr.allsolid){
		/* make sure the tr.entityNum is set to the entity we're stuck in */
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
			ent->r.currentOrigin, passent,
			ent->clipmask);
		tr.fraction = 0;
	}else
		copyv3(tr.endpos, ent->r.currentOrigin);

	trap_LinkEntity(ent);
	
	if(tr.fraction != 1){
		/* never explode or bounce on sky */
		if(tr.surfaceFlags & SURF_NOIMPACT){
			/* If grapple, reset owner */
			if(ent->parent && ent->parent->client &&
			   ent->parent->client->hook == ent)
				ent->parent->client->hook = nil;
			G_FreeEntity(ent);
			return;
		}
		G_MissileImpact(ent, &tr);
		if(ent->s.eType != ET_MISSILE)
			return;		/* exploded */
	}
#ifdef MISSIONPACK
	/* if the prox mine wasn't yet outside the player body */
	if(ent->s.weapon == WP_PROX_LAUNCHER && !ent->count){
		/* check if the prox mine is outside the owner bbox */
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
			ent->r.currentOrigin, ENTITYNUM_NONE,
			ent->clipmask);
		if(!tr.startsolid || tr.entityNum != ent->r.ownerNum)
			ent->count = 1;
	}
#endif
	/* check think function after bouncing */
	G_RunThink(ent);
}

gentity_t *
fire_plasma(gentity_t *self, Vec3 start, Vec3 dir)
{
	gentity_t *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.pos.trBase);
	scalev3(dir, 2000, bolt->s.pos.trDelta);
	snapv3(bolt->s.pos.trDelta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

gentity_t *
fire_grenade(gentity_t *self, Vec3 start, Vec3 dir)
{
	gentity_t *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.pos.trBase);
	scalev3(dir, 700, bolt->s.pos.trDelta);
	snapv3(bolt->s.pos.trDelta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

gentity_t *
fire_bfg(gentity_t *self, Vec3 start, Vec3 dir)
{
	gentity_t *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.pos.trBase);
	scalev3(dir, 2000, bolt->s.pos.trDelta);
	snapv3(bolt->s.pos.trDelta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

gentity_t *
fire_rocket(gentity_t *self, Vec3 start, Vec3 dir)
{
	gentity_t *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath	= MOD_ROCKET_SPLASH;
	bolt->clipmask			= MASK_SHOT;
	bolt->target_ent		= nil;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.pos.trBase);
	scalev3(dir, 900, bolt->s.pos.trDelta);
	snapv3(bolt->s.pos.trDelta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

gentity_t *
fire_grapple(gentity_t *self, Vec3 start, Vec3 dir)
{
	gentity_t *hook;

	normv3 (dir);
	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = nil;
	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - Presteptime;	/* move a bit on the very first frame */
	hook->s.otherEntityNum = self->s.number;		/* use to match beam in client */
	copyv3(start, hook->s.pos.trBase);
	scalev3(dir, 3000, hook->s.pos.trDelta);
	snapv3(hook->s.pos.trDelta);	/* save net bandwidth */
	copyv3(start, hook->r.currentOrigin);
	self->client->hook = hook;
	return hook;
}

#ifdef MISSIONPACK
enum { Nailgunspread = 500 };
gentity_t *
fire_nail(gentity_t *self, Vec3 start, Vec3 forward, Vec3 right, Vec3 up)
{
	gentity_t *bolt;
	Vec3 dir;
	Vec3 end;
	float r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	copyv3(start, bolt->s.pos.trBase);
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * Nailgunspread * 16;
	r = cos(r) * crandom() * Nailgunspread * 16;
	maddv3(start, 8192 * 16, forward, end);
	maddv3(end, r, right, end);
	maddv3(end, u, up, end);
	subv3(end, start, dir);
	normv3(dir);
	scale = 555 + random() * 1800;
	scalev3(dir, scale, bolt->s.pos.trDelta);
	snapv3(bolt->s.pos.trDelta);
	copyv3(start, bolt->r.currentOrigin);
	return bolt;
}

gentity_t *
fire_prox(gentity_t *self, Vec3 start, Vec3 dir)
{
	gentity_t *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	/*
	 * count is used to check if the prox mine left the player bbox
	 * if count == 1 then the prox mine left the player bbox and 
	 * can attack to it
	 */
	bolt->count = 0;
	/* FIXME: we prolly wanna abuse another field */
	bolt->s.generic1 = self->client->sess.sessionTeam;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.pos.trBase);
	scalev3(dir, 700, bolt->s.pos.trDelta);
	snapv3(bolt->s.pos.trDelta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}
#endif
