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

enum { 
	Presteptime = 50,
	Nanospread = 500
};

void
G_BounceMissile(Gentity *ent, Trace *trace)
{
	Vec3 velocity;
	float dot;
	int hitTime;

	/* reflect the velocity on the trace plane */
	hitTime = level.previousTime +
		  (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.traj, hitTime, velocity);
	dot = dotv3(velocity, trace->plane.normal);
	saddv3(velocity, -2*dot, trace->plane.normal, ent->s.traj.delta);

	if(ent->s.eFlags & EF_BOUNCE_HALF){
		scalev3(ent->s.traj.delta, 0.65, ent->s.traj.delta);
		/* check for stop */
		if(trace->plane.normal[2] > 0.2 &&
		   lenv3(ent->s.traj.delta) < 40){
			G_SetOrigin(ent, trace->endpos);
			ent->s.time = level.time / 4;
			return;
		}
	}
	addv3(ent->r.currentOrigin, trace->plane.normal,
		ent->r.currentOrigin);
	copyv3(ent->r.currentOrigin, ent->s.traj.base);
	ent->s.traj.time = level.time;
}

/* Explode a missile without an impact */
void
G_ExplodeMissile(Gentity *ent)
{
	Vec3 dir;
	Vec3 origin;
	
	ent->takedamage = qfalse;

	BG_EvaluateTrajectory(&ent->s.traj, level.time, origin);
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

static void
missiledie(Gentity *self, Gentity *inflictor, Gentity *attacker, int dmg, int mod)
{
	UNUSED(dmg | mod);
	UNUSED(attacker);
	if(inflictor == self)
		return;
	self->takedamage = qfalse;
	self->think = G_ExplodeMissile;
	self->nextthink = level.time + 50;
}

static void
ProximityMine_Explode(Gentity *mine)
{
	G_ExplodeMissile(mine);
	/* if the prox mine has a trigger free it */
	if(mine->activator){
		G_FreeEntity(mine->activator);
		mine->activator = nil;
	}
}

static void
ProximityMine_Die(Gentity *ent, Gentity *inflictor, Gentity *attacker,
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
ProximityMine_Trigger(Gentity *trigger, Gentity *other, Trace *trace)
{
	Vec3 v;
	Gentity *mine;
	
	UNUSED(trace);
	if(!other->client)
		return;
	/* trigger is a cube, do a distance test now to act as if it's a sphere */
	subv3(trigger->s.traj.base, other->s.traj.base, v);
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
	if(!CanDamage(other, trigger->s.traj.base))
		return;
	/* trigger the mine! */
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent(mine, EV_PROXIMITY_MINE_TRIGGER, 0);
	mine->nextthink = level.time + 500;
	G_FreeEntity(trigger);
}

static void
ProximityMine_Activate(Gentity *ent)
{
	Gentity *trigger;
	float r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;
	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;
	ent->s.loopSound = G_SoundIndex(Pproxsounds "/tick");

	/* build the proximity trigger */
	trigger = G_Spawn();
	trigger->classname = "proxmine_trigger";
	r = ent->splashRadius;
	setv3(trigger->r.mins, -r, -r, -r);
	setv3(trigger->r.maxs, r, r, r);
	G_SetOrigin(trigger, ent->s.traj.base);
	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;
	trap_LinkEntity(trigger);
	/* set pointer to trigger so the entity can be freed when the mine explodes */
	ent->activator = trigger;
}

static void
ProximityMine_ExplodeOnPlayer(Gentity *mine)
{
	Gentity *player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	G_SetOrigin(mine, player->s.traj.base);
	/* make sure the explosion gets to the client */
	mine->r.svFlags &= ~SVF_NOCLIENT;
	mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	G_ExplodeMissile(mine);
}

static void
ProximityMine_Player(Gentity *mine, Gentity *player)
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
	mine->s.traj.type = TR_LINEAR;
	clearv3(mine->s.traj.delta);
	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	mine->nextthink = level.time + 10 * 1000;
}

void
G_MissileImpact(Gentity *ent, Trace *trace)
{
	Gentity *other;
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
	ent->takedamage = qfalse;
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
			BG_EvaluateTrajectoryDelta(&ent->s.traj, level.time, velocity);
			if(lenv3(velocity) == 0)
				velocity[2] = 1;	/* stepped on a grenade */
			G_Damage (other, ent, &g_entities[ent->r.ownerNum],
				velocity, ent->s.origin, ent->damage, 0,
				ent->methodOfDeath);
		}
	if(ent->s.parentweap == Wproxlauncher){
		if(ent->s.traj.type != TR_GRAVITY)
			return;
		/* if it's a player, stick it on to them (flag them and remove this entity) */
		if(other->s.eType == ET_PLAYER && other->health > 0){
			ProximityMine_Player(ent, other);
			return;
		}

		snapv3Towards(trace->endpos, ent->s.traj.base);
		G_SetOrigin(ent, trace->endpos);
		ent->s.traj.type = TR_STATIONARY;
		clearv3(ent->s.traj.delta);
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

	if(ent->s.parentweap == Whook){
		Gentity	*nent;
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
			snapv3Towards(v, ent->s.traj.base);	/* save net bandwidth */
		}else{
			copyv3(trace->endpos, v);
			G_AddEvent(nent, EV_MISSILE_MISS,
				DirToByte(trace->plane.normal));
			ent->enemy = nil;
		}
		snapv3Towards(v, ent->s.traj.base);	/* save net bandwidth */
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
	snapv3Towards(trace->endpos, ent->s.traj.base);	/* save net bandwidth */
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
G_RunMissile(Gentity *ent)
{
	Vec3 origin;
	Trace tr;
	int passent;

	/* get current position */
	BG_EvaluateTrajectory(&ent->s.traj, level.time, origin);
	/* if this missile bounced off an invulnerability sphere */
	if(ent->target_ent)
		passent = ent->target_ent->s.number;
	/* prox mines that left the owner bbox will attach to anything, even the owner */
	else if(ent->s.parentweap == Wproxlauncher && ent->count)
		passent = ENTITYNUM_NONE;
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
	/* if the prox mine wasn't yet outside the player body */
	if(ent->s.parentweap == Wproxlauncher && !ent->count){
		/* check if the prox mine is outside the owner bbox */
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
			ent->r.currentOrigin, ENTITYNUM_NONE,
			ent->clipmask);
		if(!tr.startsolid || tr.entityNum != ent->r.ownerNum)
			ent->count = 1;
	}
	/* check think function after bouncing */
	G_RunThink(ent);
}

Gentity *
fire_plasma(Gentity *self, Vec3 start, Vec3 dir)
{
	Gentity *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Wplasmagun;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 200;
	bolt->splashDamage = 150;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.traj.type = TR_LINEAR;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.traj.base);
	scalev3(dir, 2000, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

Gentity *
fire_grenade(Gentity *self, Vec3 start, Vec3 dir)
{
	Gentity *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Wgrenadelauncher;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 1000;
	bolt->splashDamage = 1000;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.traj.type = TR_GRAVITY;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.traj.base);
	scalev3(dir, 700, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

Gentity *
fire_bfg(Gentity *self, Vec3 start, Vec3 dir)
{
	Gentity *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Wbfg;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 1000;
	bolt->splashDamage = 1000;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.traj.type = TR_LINEAR;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.traj.base);
	scalev3(dir, 2000, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

Gentity *
fire_rocket(Gentity *self, Vec3 start, Vec3 dir)
{
	Gentity *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Wrocketlauncher;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 1000;
	bolt->splashDamage = 1000;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath	= MOD_ROCKET_SPLASH;
	bolt->clipmask			= MASK_SHOT;
	bolt->target_ent		= nil;
	bolt->s.traj.type = TR_LINEAR;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.traj.base);
	scalev3(dir, 900, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}

static void
homingrocketthink(Gentity *ent)
{
	Gentity *targ, *tent;
	Trace tr;
	Scalar targlen, tentlen;
	Vec3 targdir, tentdir, fwd, mid;
	int i;
	
	targ = nil;
	targlen = 1000;
	copyv3(ent->s.traj.delta, fwd);
	normv3(fwd);
	for(i = 0; i < level.maxclients; i++){
		tent = &g_entities[i];
		if(OnSameTeam(tent, ent->parent)
		|| tent == ent->parent || !tent->inuse)
		then
			continue;
		/* 
		 * Obtain midpoint of player
		 */
		addv3(tent->r.mins, tent->r.maxs, mid);
		scalev3(mid, 0.5f, mid);
		addv3(mid, tent->r.currentOrigin, mid);
		
		subv3(mid, ent->r.currentOrigin, tentdir);
		tentlen = normv3(tentdir);
		if(tentlen > targlen		/* Out of scan range */
		|| dotv3(fwd, tentdir) < 0.8f	/* Angle too wide */
		|| tent != &g_entities[tr.entityNum])
		then
			continue;
		targ = tent;
		targlen = tentlen;
		copyv3(tentdir, targdir);
	}
	
	ent->nextthink += 50;
	if(targ == nil)
		return;
	saddv3(fwd, 0.05f, targdir, targdir);
	normv3(targdir);
	scalev3(targdir, 600, ent->s.traj.delta);
}

Gentity*
firehoming(Gentity *self, Vec3 start, Vec3 forward, Vec3 right, Vec3 up)
{
	Gentity *bolt;
	Vec3 dir, end;
	float r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "homingrocket";
	bolt->nextthink = level.time + 1;
	bolt->health = 30;	/* homers can be shot out of the air */
	bolt->takedamage = qtrue;
	bolt->think = homingrocketthink;
	bolt->die = missiledie;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Whominglauncher;
	bolt->r.ownerNum = self->s.number;
	bolt->r.contents = CONTENTS_BODY;
	bolt->parent = self;
	bolt->damage = 240;
	bolt->splashDamage = 100;
	bolt->splashRadius = 100;
	bolt->methodOfDeath = MOD_HOMINGROCKET;
	bolt->splashMethodOfDeath = MOD_HOMINGROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.traj.type = TR_LINEAR;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	/* set bounds for taking damage */
	setv3(bolt->r.mins, -10.0f, -3.0f, 0.0f);
	copyv3(bolt->r.mins, bolt->r.absmin);
	setv3(bolt->r.maxs, 10.0f, 3.0f, 6.0f);
	copyv3(bolt->r.maxs, bolt->r.absmax);
	/* set trajectory */
	copyv3(start, bolt->s.traj.base);
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * Nanospread * 16;
	r = cos(r) * crandom() * Nanospread * 16;
	saddv3(start, 8192 * 16, forward, end);
	saddv3(end, r, right, end);
	saddv3(end, u, up, end);
	subv3(end, start, dir);
	normv3(dir);
	//scale = 555 + random() * 1800;
	scale = 600;
	scalev3(dir, scale, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);
	copyv3(start, bolt->r.currentOrigin);
	return bolt;
}

Gentity *
fire_grapple(Gentity *self, Vec3 start, Vec3 dir)
{
	Gentity *hook;

	normv3 (dir);
	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.parentweap = Whook;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = nil;
	hook->s.traj.type = TR_LINEAR;
	hook->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	hook->s.otherEntityNum = self->s.number;		/* use to match beam in client */
	hook->damage = 10;
	copyv3(start, hook->s.traj.base);
	scalev3(dir, g_hookspeed.value * 100.0f, hook->s.traj.delta);
	snapv3(hook->s.traj.delta);	/* save net bandwidth */
	copyv3(start, hook->r.currentOrigin);
	addv3(hook->s.traj.delta, self->client->ps.velocity, hook->s.traj.delta);	// "real" physics
	self->client->hook = hook;
	return hook;
}

Gentity *
firenanoid(Gentity *self, Vec3 start, Vec3 forward, Vec3 right, Vec3 up)
{
	Gentity *bolt;
	Vec3 dir, end;
	float r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nanoid";
	bolt->nextthink = level.time + 10000 + random()*2000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Wnanoidcannon;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->methodOfDeath = MOD_NANOID;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;
	bolt->s.traj.type = TR_STOCHASTIC;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.traj.base);
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * Nanospread * 16;
	r = cos(r) * crandom() * Nanospread * 16;
	saddv3(start, 8192 * 16, forward, end);
	saddv3(end, r, right, end);
	saddv3(end, u, up, end);
	subv3(end, start, dir);
	normv3(dir);
	scale = 555 + random() * 1800;
	scalev3(dir, scale, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);
	copyv3(start, bolt->r.currentOrigin);
	return bolt;
}

Gentity *
fire_prox(Gentity *self, Vec3 start, Vec3 dir)
{
	Gentity *bolt;

	normv3 (dir);
	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.parentweap = Wproxlauncher;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 1000;
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

	bolt->s.traj.type = TR_GRAVITY;
	bolt->s.traj.time = level.time - Presteptime;	/* move a bit on the very first frame */
	copyv3(start, bolt->s.traj.base);
	scalev3(dir, 700, bolt->s.traj.delta);
	snapv3(bolt->s.traj.delta);	/* save net bandwidth */
	copyv3 (start, bolt->r.currentOrigin);
	return bolt;
}
