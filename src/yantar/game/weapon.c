/* perform the server side effects of a weapon firing */
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

static float s_quadFactor;
static Vec3 forward, right, up;
static Vec3 muzzle;

enum {
	Nhomingshots	= 3,
	Nnanoshots	= 16
};

#define CHAINGUN_SPREAD		600
#define MACHINEGUN_SPREAD		200
#define MACHINEGUN_DAMAGE	7
#define MACHINEGUN_TEAM_DAMAGE	5	/* wimpier MG in teamplay */

/*
 * `DEFAULT_SHOTGUN_SPREAD' and `DEFAULT_SHOTGUN_COUNT' are in
 * bg.h, because client predicts same spreads
*/
#define DEFAULT_SHOTGUN_DAMAGE 10

#define MAX_RAIL_HITS 4

/*
 * Round a vector to integers for more efficient network
 * transmission, but make sure that it rounds towards a given point
 * rather than blindly truncating.  This prevents it from truncating
 * into a wall.
 */
void
snapv3Towards(Vec3 v, Vec3 to)
{
	int i;

	for(i = 0; i < 3; i++){
		if(to[i] <= v[i])
			v[i] = floor(v[i]);
		else
			v[i] = ceil(v[i]);
	}
}

void
G_BounceProjectile(Vec3 start, Vec3 impact, Vec3 dir, Vec3 endout)
{
	Vec3	v, newv;
	float	dot;

	subv3(impact, start, v);
	dot = dotv3(v, dir);
	saddv3(v, -2*dot, dir, newv);
	normv3(newv);
	saddv3(impact, 8192, newv, endout);
}

/*
 * Mêlée
 */

void
Weapon_Gauntlet(Gentity *ent)
{
}

qbool
CheckGauntletAttack(Gentity *ent)
{
	Trace tr;
	Vec3	end;
	Gentity	*tent;
	Gentity	*traceEnt;
	int damage;

	/* set aiming directions */
	anglev3s (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint (ent, forward, right, up, muzzle);

	saddv3 (muzzle, 32, forward, end);

	trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	if(tr.surfaceFlags & SURF_NOIMPACT)
		return qfalse;
	if(ent->client->noclip)
		return qfalse;

	traceEnt = &g_entities[tr.entityNum];

	/* send blood impact */
	if(traceEnt->takedamage && traceEnt->client){
		tent = G_TempEntity(tr.endpos, EV_MISSILE_HIT);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weap[WSpri] = ent->s.weap[WSpri];
		tent->s.weap[WSsec] = ent->s.weap[WSsec];
		tent->s.weap[WShook] = ent->s.weap[WShook];
	}

	if(!traceEnt->takedamage)
		return qfalse;

	if(ent->client->ps.powerups[PW_QUAD]){
		G_AddEvent(ent, EV_POWERUP_QUAD, 0);
		s_quadFactor = g_quadfactor.value;
	}else
		s_quadFactor = 1;

#ifdef MISSIONPACK
	if(ent->client->persistantPowerup &&
	   ent->client->persistantPowerup->item &&
	   ent->client->persistantPowerup->item->tag == PW_DOUBLER)
	then
		s_quadFactor *= 2;
#endif

	damage = 50 * s_quadFactor;
	G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_GAUNTLET);
	return qtrue;
}

/*
 * Machinegun & chaingun
 */
 
void
Bullet_Fire(Gentity *ent, float spread, int damage)
{
	Trace tr;
	Vec3	end;
#ifdef MISSIONPACK
	Vec3	impactpoint, bouncedir;
#endif
	float	r;
	float	u;
	Gentity	*tent;
	Gentity	*traceEnt;
	int i, passent;

	damage *= s_quadFactor;

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
	saddv3 (muzzle, 8192*16, forward, end);
	saddv3 (end, r, right, end);
	saddv3 (end, u, up, end);

	passent = ent->s.number;
	for(i = 0; i < 10; i++){

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if(tr.surfaceFlags & SURF_NOIMPACT)
			return;

		traceEnt = &g_entities[ tr.entityNum ];

		/* snap the endpos to integers, but nudged towards the line */
		snapv3Towards(tr.endpos, muzzle);

		/* send bullet impact */
		if(traceEnt->takedamage && traceEnt->client){
			tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_FLESH);
			tent->s.eventParm = traceEnt->s.number;
			if(LogAccuracyHit(traceEnt, ent))
				ent->client->accuracy_hits++;
		}else{
			tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_WALL);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}
		tent->s.otherEntityNum = ent->s.number;

		if(traceEnt->takedamage){
#if 0 /* we still want bouncing rails, so saving for later */
			if(traceEnt->client &&
			   traceEnt->client->invulnerabilityTime >
			   level.time){
				if(G_InvulnerabilityEffect(traceEnt, forward,
					   tr.endpos, impactpoint, bouncedir)){
					G_BounceProjectile(muzzle, impactpoint,
						bouncedir,
						end);
					copyv3(impactpoint, muzzle);
					/* the player can hit him/herself with the bounced rail */
					passent = ENTITYNUM_NONE;
				}else{
					copyv3(tr.endpos, muzzle);
					passent = traceEnt->s.number;
				}
				continue;
			}else{
#endif
			G_Damage(traceEnt, ent, ent, forward, tr.endpos,
				damage, 0, MOD_MACHINEGUN);
#ifdef MISSIONPACK
		}
#endif
		}
		break;
	}
}

/*
 * BFG
 */

void
BFG_Fire(Gentity *ent)
{
	Gentity       *m;

	m = fire_bfg(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

/*	addv3( m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta );	// "real" physics */
}

/*
 * Shotgun
 */

qbool
ShotgunPellet(Vec3 start, Vec3 end, Gentity *ent)
{
	Trace tr;
	int damage, i, passent;
	Gentity       *traceEnt;
#ifdef MISSIONPACK
	Vec3 impactpoint, bouncedir;
#endif
	Vec3 tr_start, tr_end;

	passent = ent->s.number;
	copyv3(start, tr_start);
	copyv3(end, tr_end);
	for(i = 0; i < 10; i++){
		trap_Trace(&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[tr.entityNum];

		/* send bullet impact */
		if(tr.surfaceFlags & SURF_NOIMPACT)
			return qfalse;

		if(traceEnt->takedamage){
			damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;
			G_Damage(traceEnt, ent, ent, forward, tr.endpos,
				damage, 0, MOD_SHOTGUN);
			if(LogAccuracyHit(traceEnt, ent))
				return qtrue;
		}
		return qfalse;
	}
	return qfalse;
}

/* this should match CG_ShotgunPattern */
void
ShotgunPattern(Vec3 origin, Vec3 origin2, int seed, Gentity *ent)
{
	int i;
	float r, u;
	Vec3 end;
	Vec3 forward, right, up;
	qbool hitClient = qfalse;

	/* derive the right and up vectors from the forward vector, because
	 * the client won't have any other information */
	norm2v3(origin2, forward);
	perpv3(right, forward);
	crossv3(forward, right, up);

	/* generate the "random" spread pattern */
	for(i = 0; i < DEFAULT_SHOTGUN_COUNT; i++){
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		saddv3(origin, 8192 * 16, forward, end);
		saddv3 (end, r, right, end);
		saddv3 (end, u, up, end);
		if(ShotgunPellet(origin, end, ent) && !hitClient){
			hitClient = qtrue;
			ent->client->accuracy_hits++;
		}
	}
}

void
weapon_supershotgun_fire(Gentity *ent)
{
	Gentity *tent;

	/* send shotgun blast */
	tent = G_TempEntity(muzzle, EV_SHOTGUN);
	scalev3(forward, 4096, tent->s.origin2);
	snapv3(tent->s.origin2);
	tent->s.eventParm = rand() & 255;	/* seed for spread pattern */
	tent->s.otherEntityNum = ent->s.number;
	ShotgunPattern(tent->s.traj.base, tent->s.origin2, tent->s.eventParm,
		ent);
}

/*
 * Grenade launcher
 */

void
weapon_grenadelauncher_fire(Gentity *ent)
{
	Gentity *m;
	
	normv3(forward);
	m = fire_grenade(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	addv3(m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta);	// "real" physics
}

/*
 * Rocket
 */
 
void
Weapon_RocketLauncher_Fire(Gentity *ent)
{
	Gentity *m;
	Vec3 fw;
	Scalar s;
	
	m = fire_rocket(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	addv3(m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta);	// "real" physics
}

/*
 * Homing rocket launcher
 */

void
firehominglauncher(Gentity *ent)
{
	Gentity *m;
	int n;

	for(n = 0; n < Nhomingshots; ++n){
		m = firehoming(ent, muzzle, forward, right, up);
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;
		addv3(m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta);	// "real" physics
	}
	
}

/*
 * Plasma gun
 */

void
Weapon_Plasmagun_Fire(Gentity *ent)
{
	Gentity       *m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	addv3(m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta);	// "real" physics
}

/*
 * Railgun
 */

void
weapon_railgun_fire(Gentity *ent)
{
	Vec3 end;
#ifdef MISSIONPACK
	Vec3 impactpoint, bouncedir;
#endif
	Trace trace;
	Gentity *tent;
	Gentity *traceEnt;
	int damage;
	int i;
	int hits;
	int unlinked;
	int passent;
	Gentity *unlinkedEntities[MAX_RAIL_HITS];

	damage = 100 * s_quadFactor;

	saddv3 (muzzle, 8192, forward, end);

	/* trace only against the solids, so the railgun will go through people */
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if(trace.entityNum >= ENTITYNUM_MAX_NORMAL)
			break;
		traceEnt = &g_entities[ trace.entityNum ];
		if(traceEnt->takedamage){
			if(LogAccuracyHit(traceEnt, ent))
				hits++;
			G_Damage (traceEnt, ent, ent, forward, trace.endpos,
				damage, 0,
				MOD_RAILGUN);
		}
		if(trace.contents & CONTENTS_SOLID)
			break;	/* we hit something solid enough to stop the beam */
		/* unlink this entity, so the next trace will go past it */
		trap_UnlinkEntity(traceEnt);
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while(unlinked < MAX_RAIL_HITS);

	/* link back in any entities we unlinked */
	for(i = 0; i < unlinked; i++)
		trap_LinkEntity(unlinkedEntities[i]);

	/* the final trace endpos will be the terminal point of the rail trail */

	/* snap the endpos to integers to save net bandwidth, but nudged towards the line */
	snapv3Towards(trace.endpos, muzzle);

	/* send railgun beam effect */
	tent = G_TempEntity(trace.endpos, EV_RAILTRAIL);

	/* set player number for custom colors on the railtrail */
	tent->s.clientNum = ent->s.clientNum;

	copyv3(muzzle, tent->s.origin2);
	/* move origin a bit to come closer to the drawn gun muzzle */
	saddv3(tent->s.origin2, 4, right, tent->s.origin2);
	saddv3(tent->s.origin2, -1, up, tent->s.origin2);

	/* no explosion at end if SURF_NOIMPACT, but still make the trail */
	if(trace.surfaceFlags & SURF_NOIMPACT)
		tent->s.eventParm = 255;	/* don't make the explosion at the end */
	else
		tent->s.eventParm = DirToByte(trace.plane.normal);
	tent->s.clientNum = ent->s.clientNum;

	/* give the shooter a reward sound if they have made two railgun hits in a row */
	if(hits == 0)
		/* complete miss */
		ent->client->accurateCount = 0;
	else{
		/* check for "impressive" reward sound */
		ent->client->accurateCount += hits;
		if(ent->client->accurateCount >= 2){
			ent->client->accurateCount -= 2;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
			/* add the sprite over the player's head */
			ent->client->ps.eFlags &=
				~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT |
				  EF_AWARD_GAUNTLET | EF_AWARD_ASSIST |
				  EF_AWARD_DEFEND |
				  EF_AWARD_CAP);
			ent->client->ps.eFlags	|= EF_AWARD_IMPRESSIVE;
			ent->client->rewardTime = level.time +
						  REWARD_SPRITE_TIME;
		}
		ent->client->accuracy_hits++;
	}

}

/*
 * Grappling hook
 */

void
Weapon_GrapplingHook_Fire(Gentity *ent)
{
	if(!ent->client->fireHeld && !ent->client->hook)
		fire_grapple(ent, muzzle, forward);
	ent->client->fireHeld = qtrue;
}

void
Weapon_HookFree(Gentity *ent)
{
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity(ent);
}

void
Weapon_HookThink(Gentity *ent)
{
	if(ent->enemy){
		Vec3 v, oldorigin;

		copyv3(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] +
		       (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] +
		       (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] +
		       (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		snapv3Towards(v, oldorigin);	/* save net bandwidth */

		G_SetOrigin(ent, v);
	}

	copyv3(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

/*
 * Lightning gun
 */

void
Weapon_LightningFire(Gentity *ent)
{
	Trace tr;
	Vec3 end;
#ifdef MISSIONPACK
	Vec3 impactpoint, bouncedir;
#endif
	Gentity       *traceEnt, *tent;
	int damage, i, passent;

	damage = 8 * s_quadFactor;

	passent = ent->s.number;
	for(i = 0; i < 10; i++){
		saddv3(muzzle, LIGHTNING_RANGE, forward, end);

		trap_Trace(&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

#ifdef MISSIONPACK
		/* if not the first trace (the lightning bounced of an invulnerability sphere) */
		if(i){
			/* add bounced off lightning bolt temp entity
			 * the first lightning bolt is a cgame only visual
			 *  */
			tent = G_TempEntity(muzzle, EV_LIGHTNINGBOLT);
			copyv3(tr.endpos, end);
			snapv3(end);
			copyv3(end, tent->s.origin2);
		}
#endif
		if(tr.entityNum == ENTITYNUM_NONE)
			return;

		traceEnt = &g_entities[ tr.entityNum ];

		if(traceEnt->takedamage){
			G_Damage(traceEnt, ent, ent, forward, tr.endpos,
				damage, 0, MOD_LIGHTNING);
		}

		if(traceEnt->takedamage && traceEnt->client){
			tent = G_TempEntity(tr.endpos, EV_MISSILE_HIT);
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte(tr.plane.normal);
			tent->s.weap[WSpri] = ent->s.weap[WSpri];
			tent->s.weap[WSsec] = ent->s.weap[WSsec];
			tent->s.weap[WShook] = ent->s.weap[WShook];
			if(LogAccuracyHit(traceEnt, ent))
				ent->client->accuracy_hits++;
		}else if(!(tr.surfaceFlags & SURF_NOIMPACT)){
			tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}

		break;
	}
}

/*
 * Nanoid cannon
 */

void
firenanoidcannon(Gentity *ent)
{
	Gentity *m;
	int count;

	for(count = 0; count < Nnanoshots; count++){
		m = firenanoid(ent, muzzle, forward, right, up);
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;
		addv3( m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta );	// "real" physics
	}
}

/*
 * Proximity mine launcher
 */

void
weapon_proxlauncher_fire(Gentity *ent)
{
	Gentity       *m;

	/* extra vertical velocity */
	forward[2] += 0.2f;
	normv3(forward);
	m = fire_prox(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
	
	addv3( m->s.traj.delta, ent->client->ps.velocity, m->s.traj.delta );	// "real" physics 
}

/*
 * etc.
 */

qbool
LogAccuracyHit(Gentity *target, Gentity *attacker)
{
	if(!target->takedamage)
		return qfalse;
	if(target == attacker)
		return qfalse;
	if(!target->client)
		return qfalse;
	if(!attacker->client)
		return qfalse;
	if(target->client->ps.stats[STAT_HEALTH] <= 0)
		return qfalse;
	if(OnSameTeam(target, attacker))
		return qfalse;
	return qtrue;
}

/*
 * set muzzle location relative to pivoting eye
 */
void
CalcMuzzlePoint(Gentity *ent, Vec3 forward, Vec3 right, Vec3 up,
		Vec3 muzzlePoint)
{
	copyv3(ent->s.traj.base, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	saddv3(muzzlePoint, 14, forward, muzzlePoint);
	/* snap to integer coordinates for more efficient network bandwidth usage */
	snapv3(muzzlePoint);
}

/*
 * set muzzle location relative to pivoting eye
 */
void
CalcMuzzlePointOrigin(Gentity *ent, Vec3 origin, Vec3 forward,
		      Vec3 right, Vec3 up,
		      Vec3 muzzlePoint)
{
	copyv3(ent->s.traj.base, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	saddv3(muzzlePoint, 14, forward, muzzlePoint);
	/* snap to integer coordinates for more efficient network bandwidth usage */
	snapv3(muzzlePoint);
}

void
FireWeapon(Gentity *ent, Weapslot slot)
{
	Weapon w;
	
	if(slot >= WSnumslots){
		G_Printf("bad weapslot %u\n", slot);
		return;
	}
	w = ent->s.weap[slot];

	if(ent->client->ps.powerups[PW_QUAD])
		s_quadFactor = g_quadfactor.value;
	else
		s_quadFactor = 1;

#ifdef MISSIONPACK
	if(ent->client->persistantPowerup &&
	   ent->client->persistantPowerup->item &&
	   ent->client->persistantPowerup->item->tag == PW_DOUBLER)
		s_quadFactor *= 2;

#endif

	/* track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked */
	if(w != Whook && w != Wmelee){
		if(w == Wnanoidcannon)
			ent->client->accuracy_shots += Nnanoshots;
		else
			ent->client->accuracy_shots++;
	}

	/* set aiming directions */
	anglev3s(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin(ent, ent->client->oldOrigin, forward, right, up,
		muzzle);

	/* fire the specific weapon */
	switch(w){
	case Wmelee:
		Weapon_Gauntlet(ent);
		break;
	case Wlightning:
		Weapon_LightningFire(ent);
		break;
	case Wshotgun:
		weapon_supershotgun_fire(ent);
		break;
	case Wnanoidcannon:
		firenanoidcannon(ent);
		break;
	case Wmachinegun:
		if(g_gametype.integer != GT_TEAM)
			Bullet_Fire(ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE);
		else
			Bullet_Fire(ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE);
		break;
	case Wplasmagun:
		Weapon_Plasmagun_Fire(ent);
		break;
	case Wrailgun:
		weapon_railgun_fire(ent);
		break;
	case Wchaingun:
		Bullet_Fire(ent, CHAINGUN_SPREAD, MACHINEGUN_DAMAGE);
		break;
	case Wgrenadelauncher:
		weapon_grenadelauncher_fire(ent);
		break;
	case Wrocketlauncher:
		Weapon_RocketLauncher_Fire(ent);
		break;
	case Whominglauncher:
		firehominglauncher(ent);
		break;
	case Wproxlauncher:
		weapon_proxlauncher_fire(ent);
		break;
	case Wbfg:
		BFG_Fire(ent);
		break;
	case Whook:
		Weapon_GrapplingHook_Fire(ent);
		break;
	default:
		G_Printf("bad weap %u (slot %u)\n", w, slot);
		break;
	}
}

#ifdef MISSIONPACK

/*
 * Kamikaze
 */

static void
KamikazeRadiusDamage(Vec3 origin, Gentity *attacker, float damage,
		     float radius)
{
	float dist;
	Gentity       *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	Vec3 mins, maxs;
	Vec3 v;
	Vec3 dir;
	int i, e;

	if(radius < 1)
		radius = 1;

	for(i = 0; i < 3; i++){
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList,
		MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++){
		ent = &g_entities[entityList[ e ]];

		if(!ent->takedamage)
			continue;

		/* dont hit things we have already hit */
		if(ent->kamikazeTime > level.time)
			continue;

		/* find the distance from the edge of the bounding box */
		for(i = 0; i < 3; i++){
			if(origin[i] < ent->r.absmin[i])
				v[i] = ent->r.absmin[i] - origin[i];
			else if(origin[i] > ent->r.absmax[i])
				v[i] = origin[i] - ent->r.absmax[i];
			else
				v[i] = 0;
		}

		dist = lenv3(v);
		if(dist >= radius)
			continue;

/*		if( CanDamage (ent, origin) ) { */
		subv3 (ent->r.currentOrigin, origin, dir);
		/* push the center of mass higher than the origin so players
		 * get knocked into the air more */
		dir[2] += 24;
		G_Damage(ent, NULL, attacker, dir, origin, damage,
			DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION,
			MOD_KAMIKAZE);
		ent->kamikazeTime = level.time + 3000;
/*		} */
	}
}

static void
KamikazeShockWave(Vec3 origin, Gentity *attacker, float damage, float push,
		  float radius)
{
	float dist;
	Gentity       *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	Vec3 mins, maxs;
	Vec3 v;
	Vec3 dir;
	int i, e;

	if(radius < 1)
		radius = 1;

	for(i = 0; i < 3; i++){
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList,
		MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++){
		ent = &g_entities[entityList[ e ]];

		/* dont hit things we have already hit */
		if(ent->kamikazeShockTime > level.time)
			continue;

		/* find the distance from the edge of the bounding box */
		for(i = 0; i < 3; i++){
			if(origin[i] < ent->r.absmin[i])
				v[i] = ent->r.absmin[i] - origin[i];
			else if(origin[i] > ent->r.absmax[i])
				v[i] = origin[i] - ent->r.absmax[i];
			else
				v[i] = 0;
		}

		dist = lenv3(v);
		if(dist >= radius)
			continue;

/*		if( CanDamage (ent, origin) ) { */
		subv3 (ent->r.currentOrigin, origin, dir);
		dir[2] += 24;
		G_Damage(ent, NULL, attacker, dir, origin, damage,
			DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION,
			MOD_KAMIKAZE);
		dir[2] = 0;
		normv3(dir);
		if(ent->client){
			ent->client->ps.velocity[0] = dir[0] * push;
			ent->client->ps.velocity[1] = dir[1] * push;
			ent->client->ps.velocity[2] = 100;
		}
		ent->kamikazeShockTime = level.time + 3000;
/*		} */
	}
}

static void
KamikazeDamage(Gentity *self)
{
	int i;
	float t;
	Gentity *ent;
	Vec3 newangles;

	self->count += 100;

	if(self->count >= KAMI_SHOCKWAVE_STARTTIME){
		/* shockwave push back */
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.traj.base, self->activator, 25, 400,
			(int)(float)t * KAMI_SHOCKWAVE_MAXRADIUS /
			(KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME));
	}
	if(self->count >= KAMI_EXPLODE_STARTTIME){
		/* do our damage */
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage(self->s.traj.base, self->activator, 400,
			(int)(float)t * KAMI_BOOMSPHERE_MAXRADIUS /
			(KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME));
	}

	/* either cycle or kill self */
	if(self->count >= KAMI_SHOCKWAVE_ENDTIME){
		G_FreeEntity(self);
		return;
	}
	self->nextthink = level.time + 100;

	/* add earth quake effect */
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for(i = 0; i < MAX_CLIENTS; i++){
		ent = &g_entities[i];
		if(!ent->inuse)
			continue;
		if(!ent->client)
			continue;

		if(ent->client->ps.groundEntityNum != ENTITYNUM_NONE){
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(
			newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(
			newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(
			newangles[2] - self->movedir[2]);
	}
	copyv3(newangles, self->movedir);
}

void
G_StartKamikaze(Gentity *ent)
{
	Gentity       *explosion;
	Gentity       *te;
	Vec3 snapped;

	/* start up the explosion logic */
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventTime = level.time;

	if(ent->client)
		copyv3(ent->s.traj.base, snapped);
	else
		copyv3(ent->activator->s.traj.base, snapped);
	snapv3(snapped);	/* save network bandwidth */
	G_SetOrigin(explosion, snapped);

	explosion->classname = "kamikaze";
	explosion->s.traj.type = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	clearv3(explosion->movedir);

	trap_LinkEntity(explosion);

	if(ent->client){
		explosion->activator = ent;
		ent->s.eFlags &= ~EF_KAMIKAZE;
		/* nuke the guy that used it */
		G_Damage(ent, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION,
			MOD_KAMIKAZE);
	}else{
		if(!strcmp(ent->activator->classname, "bodyque"))
			explosion->activator =
				&g_entities[ent->activator->r.ownerNum];
		else
			explosion->activator = ent->activator;
	}

	/* play global sound at all clients */
	te = G_TempEntity(snapped, EV_GLOBAL_TEAM_SOUND);
	te->r.svFlags	|= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}
#endif /* MISSIONPACK */
