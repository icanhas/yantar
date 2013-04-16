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
 * ScorePlum
 */
void
ScorePlum(Gentity *ent, Vec3 origin, int score)
{
	Gentity *plum;

	plum = G_TempEntity(origin, EV_SCOREPLUM);
	/* only send this temp entity to a single client */
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

/*
 * AddScore
 *
 * Adds score to both the client and his team
 */
void
AddScore(Gentity *ent, Vec3 origin, int score)
{
	if(!ent->client)
		return;
	/* no scoring during pre-match warmup */
	if(level.warmupTime)
		return;
	/* show score plum */
	ScorePlum(ent, origin, score);
	ent->client->ps.persistant[PERS_SCORE] += score;
	if(g_gametype.integer == GT_TEAM)
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] +=
			score;
	CalculateRanks();
}

/*
 * TossClientItems
 *
 * Toss the weapon and powerups for the killed player
 */
void
TossClientItems(Gentity *self)
{
	Gitem *item;
	int weapon;
	float	angle;
	int i;
	Gentity *drop;

	/* drop the weapon if not a gauntlet or machinegun */
	weapon = self->s.weap[WSpri];

	/* 
	 * make a special check to see if they are changing to a new
	 * weapon that isn't the mg or gauntlet.  Without this, a client
	 * can pick up a weapon, be killed, and not drop the weapon because
	 * their weapon change hasn't completed yet and they are still holding the MG. 
	 */
	if(weapon == Wmachinegun || weapon == Whook){
		if(self->client->ps.weapstate[WSpri] == WEAPON_DROPPING)
			weapon = self->client->pers.cmd.weap[WSpri];
		if(!(self->client->ps.stats[STAT_PRIWEAPS] & (1 << weapon)))
			weapon = Wnone;
	}

	if(weapon > Wmachinegun && weapon != Whook 
	   && self->client->ps.ammo[weapon])
	then{
		/* find the item type for this weapon */
		item = BG_FindItemForWeapon(weapon);
		/* spawn the item */
		Drop_Item(self, item, 0);
	}

	/* drop all the powerups if not in teamplay */
	if(g_gametype.integer != GT_TEAM){
		angle = 45;
		for(i = 1; i < PW_NUM_POWERUPS; i++){
			if(self->client->ps.powerups[ i ] > level.time){
				item = BG_FindItemForPowerup(i);
				if(!item)
					continue;
				drop = Drop_Item(self, item, angle);
				/* decide how many seconds it has left */
				drop->count = (self->client->ps.powerups[ i ] - level.time) / 1000;
				if(drop->count < 1)
					drop->count = 1;
				angle += 45;
			}
		}
	}
}

/*
 * TossClientPersistantPowerups
 */
void
TossClientPersistantPowerups(Gentity *ent)
{
	Gentity *powerup;

	if(!ent->client)
		return;

	if(!ent->client->persistantPowerup)
		return;

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity(powerup);

	ent->client->persistantPowerup = NULL;
}

/*
 * LookAtKiller
 */
void
LookAtKiller(Gentity *self, Gentity *inflictor, Gentity *attacker)
{
	Vec3 dir;

	if(attacker && attacker != self)
		subv3 (attacker->s.traj.base, self->s.traj.base, dir);
	else if(inflictor && inflictor != self)
		subv3 (inflictor->s.traj.base, self->s.traj.base, dir);
	else{
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw (dir);
}

/*
 * GibEntity
 */
void
GibEntity(Gentity *self, int killer)
{
	Gentity *ent;
	int i;

	/* if this entity still has kamikaze */
	if(self->s.eFlags & EF_KAMIKAZE)
		/* check if there is a kamikaze timer around for this owner */
		for(i = 0; i < MAX_GENTITIES; i++){
			ent = &g_entities[i];
			if(!ent->inuse)
				continue;
			if(ent->activator != self)
				continue;
			if(strcmp(ent->classname, "kamikaze timer"))
				continue;
			G_FreeEntity(ent);
			break;
		}
	G_AddEvent(self, EV_GIB_PLAYER, killer);
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
 * body_die
 */
void
body_die(Gentity *self, Gentity *inflictor, Gentity *attacker, int damage,
	 int meansOfDeath)
{
	if(self->health > GIB_HEALTH)
		return;
	if(!g_blood.integer){
		self->health = GIB_HEALTH+1;
		return;
	}

	GibEntity(self, 0);
}

/* these are just for logging, the client prints its own messages */
char*
mod2str(Meansofdeath mod)
{
	switch(mod){
	case MOD_UNKNOWN:	return "MOD_UNKNOWN";
	case MOD_SHOTGUN: 	return "MOD_SHOTGUN";
	case MOD_NANOID:	return "MOD_NANOID";
	case MOD_GAUNTLET:	return "MOD_GAUNTLET";
	case MOD_MACHINEGUN:	return "MOD_MACHINEGUN";
	case MOD_CHAINGUN:	return "MOD_CHAINGUN";
	case MOD_PLASMA:	return "MOD_PLASMA";
	case MOD_PLASMA_SPLASH:	return "MOD_PLASMA_SPLASH";
	case MOD_RAILGUN:	return "MOD_RAILGUN";
	case MOD_LIGHTNING:	return "MOD_LIGHTNING";
	case MOD_GRENADE:	return "MOD_GRENADE";
	case MOD_GRENADE_SPLASH:	return "MOD_GRENADE_SPLASH";
	case MOD_ROCKET:	return "MOD_ROCKET";
	case MOD_ROCKET_SPLASH:	return "MOD_ROCKET_SPLASH";
	case MOD_PROXIMITY_MINE:	return "MOD_PROXIMITY_MINE";
	case MOD_BFG:		return "MOD_BFG";
	case MOD_BFG_SPLASH:	return "MOD_BFG_SPLASH";
	case MOD_WATER:		return "MOD_WATER";
	case MOD_SLIME:		return "MOD_SLIME";
	case MOD_LAVA:		return "MOD_LAVA";
	case MOD_CRUSH:		return "MOD_CRUSH";
	case MOD_TELEFRAG:	return "MOD_TELEFRAG";
	case MOD_FALLING:	return "MOD_FALLING";
	case MOD_SUICIDE:	return "MOD_SUICIDE";
	case MOD_TARGET_LASER:	return "MOD_TARGET_LASER";
	case MOD_TRIGGER_HURT:	return "MOD_TRIGGER_HURT";
	case MOD_GRAPPLE:	return "MOD_GRAPPLE";
	default:		return "<bad obituary>";
	}
}

/*
 * CheckAlmostCapture
 */
void
CheckAlmostCapture(Gentity *self, Gentity *attacker)
{
	Gentity	*ent;
	Vec3		dir;
	char		*classname;

	/* if this player was carrying a flag */
	if(self->client->ps.powerups[PW_REDFLAG] ||
	   self->client->ps.powerups[PW_BLUEFLAG] ||
	   self->client->ps.powerups[PW_NEUTRALFLAG]){
		/* get the goal flag this player should have been going for */
		if(g_gametype.integer == GT_CTF){
			if(self->client->sess.team == TEAM_BLUE)
				classname = "team_CTF_blueflag";
			else
				classname = "team_CTF_redflag";
		}else{
			if(self->client->sess.team == TEAM_BLUE)
				classname = "team_CTF_redflag";
			else
				classname = "team_CTF_blueflag";
		}
		ent = NULL;
		do {
			ent = G_Find(ent, FOFS(classname), classname);
		} while(ent && (ent->flags & FL_DROPPED_ITEM));
		/* if we found the destination flag and it's not picked up */
		if(ent && !(ent->r.svFlags & SVF_NOCLIENT)){
			/* if the player was *very* close */
			subv3(self->client->ps.origin, ent->s.origin,
				dir);
			if(lenv3(dir) < 200){
				self->client->ps.persistant[PERS_PLAYEREVENTS]
					^= PLAYEREVENT_HOLYSHIT;
				if(attacker->client)
					attacker->client->ps.persistant[
						PERS_PLAYEREVENTS] ^=
						PLAYEREVENT_HOLYSHIT;
			}
		}
	}
}

/*
 * CheckAlmostScored
 */
void
CheckAlmostScored(Gentity *self, Gentity *attacker)
{
	Gentity	*ent;
	Vec3		dir;
	char		*classname;

	/* if the player was carrying cubes */
	if(self->client->ps.generic1){
		if(self->client->sess.team == TEAM_BLUE)
			classname = "team_redobelisk";
		else
			classname = "team_blueobelisk";
		ent = G_Find(NULL, FOFS(classname), classname);
		/* if we found the destination obelisk */
		if(ent){
			/* if the player was *very* close */
			subv3(self->client->ps.origin, ent->s.origin,
				dir);
			if(lenv3(dir) < 200){
				self->client->ps.persistant[PERS_PLAYEREVENTS]
					^= PLAYEREVENT_HOLYSHIT;
				if(attacker->client)
					attacker->client->ps.persistant[
						PERS_PLAYEREVENTS] ^=
						PLAYEREVENT_HOLYSHIT;
			}
		}
	}
}

/*
 * player_die
 */
void
player_die(Gentity *self, Gentity *inflictor, Gentity *attacker,
	   int damage,
	   int meansOfDeath)
{
	Gentity *ent;
	int	anim;
	int	contents;
	int	killer;
	int	i;
	char *killerName, *obit;

	if(self->client->ps.pm_type == PM_DEAD)
		return;

	if(level.intermissiontime)
		return;

	/* check for an almost capture */
	CheckAlmostCapture(self, attacker);
	/* check for a player that almost brought in cubes */
	CheckAlmostScored(self, attacker);

	if(self->client && self->client->hook)
		Weapon_HookFree(self->client->hook);

	if((self->client->ps.eFlags & EF_TICKING) && self->activator){
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think	= G_FreeEntity;
		self->activator->nextthink = level.time;
	}
	self->client->ps.pm_type = PM_DEAD;

	if(attacker){
		killer = attacker->s.number;
		if(attacker->client)
			killerName = attacker->client->pers.netname;
		else
			killerName = "<non-client>";
	}else{
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if(killer < 0 || killer >= MAX_CLIENTS){
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	obit = mod2str(meansOfDeath);
	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n",
		killer, self->s.number, meansOfDeath, killerName,
		self->client->pers.netname, obit);

	/* broadcast the death event to everyone */
	ent = G_TempEntity(self->r.currentOrigin, EV_OBITUARY);
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum	= self->s.number;
	ent->s.otherEntityNum2	= killer;
	ent->r.svFlags = SVF_BROADCAST;	/* send to everyone */

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if(attacker && attacker->client){
		attacker->client->lastkilled_client = self->s.number;

		if(attacker == self || OnSameTeam (self, attacker))
			AddScore(attacker, self->r.currentOrigin, -1);
		else{
			AddScore(attacker, self->r.currentOrigin, 1);

			if(meansOfDeath == MOD_GAUNTLET){

				/* play humiliation on player */
				attacker->client->ps.persistant[
					PERS_GAUNTLET_FRAG_COUNT]++;

				/* add the sprite over the player's head */
				attacker->client->ps.eFlags &=
					~(EF_AWARD_IMPRESSIVE |
					  EF_AWARD_EXCELLENT |
					  EF_AWARD_GAUNTLET | EF_AWARD_ASSIST |
					  EF_AWARD_DEFEND | EF_AWARD_CAP);
				attacker->client->ps.eFlags |=
					EF_AWARD_GAUNTLET;
				attacker->client->rewardTime = level.time +
							       REWARD_SPRITE_TIME;

				/* also play humiliation on target */
				self->client->ps.persistant[PERS_PLAYEREVENTS]
					^= PLAYEREVENT_GAUNTLETREWARD;
			}

			/* check for two kills in a short amount of time
			 * if this is close enough to the last kill, give a reward sound */
			if(level.time - attacker->client->lastKillTime <
			   CARNAGE_REWARD_TIME){
				/* play excellent on player */
				attacker->client->ps.persistant[
					PERS_EXCELLENT_COUNT]++;

				/* add the sprite over the player's head */
				attacker->client->ps.eFlags &=
					~(EF_AWARD_IMPRESSIVE |
					  EF_AWARD_EXCELLENT |
					  EF_AWARD_GAUNTLET | EF_AWARD_ASSIST |
					  EF_AWARD_DEFEND | EF_AWARD_CAP);
				attacker->client->ps.eFlags |=
					EF_AWARD_EXCELLENT;
				attacker->client->rewardTime = level.time +
							       REWARD_SPRITE_TIME;
			}
			attacker->client->lastKillTime = level.time;

		}
	}else
		AddScore(self, self->r.currentOrigin, -1);

	/* Add team bonuses */
	Team_FragBonuses(self, inflictor, attacker);

	/* if I committed suicide, the flag does not fall, it returns. */
	if(meansOfDeath == MOD_SUICIDE){
		if(self->client->ps.powerups[PW_NEUTRALFLAG]){	/* only happens in One Flag CTF */
			Team_ReturnFlag(TEAM_FREE);
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}else if(self->client->ps.powerups[PW_REDFLAG]){	/* only happens in standard CTF */
			Team_ReturnFlag(TEAM_RED);
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}else if(self->client->ps.powerups[PW_BLUEFLAG]){	/* only happens in standard CTF */
			Team_ReturnFlag(TEAM_BLUE);
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}

	TossClientItems(self);
	TossClientPersistantPowerups(self);

	Cmd_Score_f(self);	/* show scores */
	/* send updated scores to any clients that are following this one,
	 * or they would get stale scoreboards */
	for(i = 0; i < level.maxclients; i++){
		Gclient *client;

		client = &level.clients[i];
		if(client->pers.connected != CON_CONNECTED)
			continue;
		if(client->sess.team != TEAM_SPECTATOR)
			continue;
		if(client->sess.specclient == self->s.number)
			Cmd_Score_f(g_entities + i);
	}

	self->takedamage = qtrue;	/* can still be gibbed */

	self->s.weap[WSpri] = Wnone;
	self->s.weap[WSsec] = Wnone;
	self->s.weap[WShook] = Wnone;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller (self, inflictor, attacker);

	copyv3(self->s.angles, self->client->ps.viewangles);

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	/* don't allow respawn until the death anim is done
	 * g_forcerespawn may force spawning at some later time */
	self->client->respawnTime = level.time + 1700;

	/* remove powerups */
	memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

	/* never gib in a nodrop */
	contents = trap_PointContents(self->r.currentOrigin, -1);

	if((self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) &&
	    g_blood.integer) || meansOfDeath == MOD_SUICIDE)
		/* gib death */
		GibEntity(self, killer);
	else{
		/* normal death */
		static int i;

		switch(i){
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		/* for the no-blood option, we need to prevent the health
		 * from going to gib level */
		if(self->health <= GIB_HEALTH)
			self->health = GIB_HEALTH+1;

		self->client->ps.legsAnim =
			((self->client->ps.legsAnim &
			  ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
		self->client->ps.torsoAnim =
			((self->client->ps.torsoAnim &
			  ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

		G_AddEvent(self, EV_DEATH1 + i, killer);

		/* the body can still be gibbed */
		self->die = body_die;

		/* globally cycle through the different death animations */
		i = (i + 1) % 3;
	}

	trap_LinkEntity (self);

}


/*
 * CheckArmor
 */
int
CheckArmor(Gentity *ent, int damage, int dflags)
{
	Gclient *client;
	int	save;
	int	count;

	if(!damage)
		return 0;

	client = ent->client;

	if(!client)
		return 0;

	if(dflags & DAMAGE_NO_SHIELD)
		return 0;

	/* armor */
	count	= client->ps.stats[STAT_SHIELD];
	save	= ceil(damage * SHIELD_PROTECTION);
	if(save >= count)
		save = count;

	if(!save)
		return 0;

	client->ps.stats[STAT_SHIELD] -= save;

	return save;
}

/*
 * RaySphereIntersections
 */
int
RaySphereIntersections(Vec3 origin, float radius, Vec3 point, Vec3 dir,
		       Vec3 intersections[2])
{
	float b, c, d, t;

	/* | origin - (point + t * dir) | = radius
	 * a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	 * b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	 * c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2; */

	/* normalize dir so a = 1 */
	normv3(dir);
	b = 2 *
	    (dir[0] *
	     (point[0] -
	      origin[0]) + dir[1] *
	     (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
	    (point[1] - origin[1]) * (point[1] - origin[1]) +
	    (point[2] - origin[2]) * (point[2] - origin[2]) -
	    radius * radius;

	d = b * b - 4 * c;
	if(d > 0){
		t = (-b + sqrt(d)) / 2;
		saddv3(point, t, dir, intersections[0]);
		t = (-b - sqrt(d)) / 2;
		saddv3(point, t, dir, intersections[1]);
		return 2;
	}else if(d == 0){
		t = (-b) / 2;
		saddv3(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

/*
 * T_Damage
 *
 * targ		entity that is being damaged
 * inflictor	entity that is causing the damage
 * attacker	entity that caused the inflictor to damage targ
 *      example: targ=monster, inflictor=rocket, attacker=player
 *
 * dir			direction of the attack for knockback
 * point		point at which the damage is being inflicted, used for headshots
 * damage		amount of damage being inflicted
 * knockback	force to be applied against targ as a result of the damage
 *
 * inflictor, attacker, dir, and point can be NULL for environmental effects
 *
 * dflags		these flags are used to control how T_Damage works
 *      DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
 *      DAMAGE_NO_SHIELD			armor does not protect from this damage
 *      DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
 *      DAMAGE_NO_PROTECTION	kills godmode, armor, everything
 */

void
G_Damage(Gentity *targ, Gentity *inflictor, Gentity *attacker,
	 Vec3 dir, Vec3 point, int damage, int dflags, int mod)
{
	Gclient *client;
	int	take;
	int	asave;
	int	knockback;
	int	max;
#ifdef MISSIONPACK
	Vec3 bouncedir, impactpoint;
#endif

	if(!targ->takedamage)
		return;

	/* the intermission has allready been qualified for, so don't
	 * allow any extra scoring */
	if(level.intermissionQueued)
		return;
	if(!inflictor)
		inflictor = &g_entities[ENTITYNUM_WORLD];
	if(!attacker)
		attacker = &g_entities[ENTITYNUM_WORLD];

	/* shootable doors / buttons don't actually have any health */
	if(targ->s.eType == ET_MOVER){
		if(targ->use && targ->moverState == MOVER_POS1)
			targ->use(targ, inflictor, attacker);
		return;
	}
	/* reduce damage by the attacker's handicap value
	 * unless they are rocket jumping */
	if(attacker->client && attacker != targ){
		max = attacker->client->ps.stats[STAT_MAX_HEALTH];
		damage = damage * max / Maxhealth;
	}

	client = targ->client;

	if(client)
		if(client->noclip)
			return;

	if(!dir)
		dflags |= DAMAGE_NO_KNOCKBACK;
	else
		normv3(dir);

	knockback = damage/10;
	if(targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;
	if(dflags & DAMAGE_NO_KNOCKBACK)
		knockback = 0;

	/* figure momentum add, even if the damage won't be taken */
	if(knockback && targ->client){
		Vec3	kvel;
		float	mass;

		mass = 200;

		scalev3 (dir, g_knockback.value * (float)knockback / mass,
			kvel);
		addv3 (targ->client->ps.velocity, kvel,
			targ->client->ps.velocity);

		/* set the timer so that the other client can't cancel
		 * out the movement immediately */
		if(!targ->client->ps.pm_time){
			int t;

			t = knockback * 2;
			if(t < 50)
				t = 50;
			if(t > 200)
				t = 200;
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	/* check for completely getting out of the damage */
	if(!(dflags & DAMAGE_NO_PROTECTION)){

		/* if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		 * if the attacker was on the same team */
		if(targ != attacker && OnSameTeam (targ, attacker)){
			if(!g_friendlyFire.integer)
				return;
		}
		if(mod == MOD_PROXIMITY_MINE){
			if(inflictor && inflictor->parent &&
			   OnSameTeam(targ, inflictor->parent))
				return;
			if(targ == attacker)
				return;
		}
		/* check for godmode */
		if(targ->flags & FL_GODMODE)
			return;
	}

	/* battlesuit protects from all radius damage (but takes knockback)
	 * and protects 50% against all damage */
	if(client && client->ps.powerups[PW_BATTLESUIT]){
		G_AddEvent(targ, EV_POWERUP_BATTLESUIT, 0);
		if((dflags & DAMAGE_RADIUS) || (mod == MOD_FALLING))
			return;
		damage *= 0.5;
	}

	/* add to the attacker's hit counter (if the target isn't a general entity like a prox mine) */
	if(attacker->client && client
	   && targ != attacker && targ->health > 0
	   && targ->s.eType != ET_MISSILE
	   && targ->s.eType != ET_GENERAL){
		if(OnSameTeam(targ, attacker))
			attacker->client->ps.persistant[PERS_HITS]--;
		else
			attacker->client->ps.persistant[PERS_HITS]++;
		attacker->client->ps.persistant[PERS_ATTACKEE_SHIELD] =
			(targ->health<<8)|(client->ps.stats[STAT_SHIELD]);
	}

	/* always give half damage if hurting self
	 * calculated after knockback, so rocket jumping works */
	if(targ == attacker)
		damage *= 0.5;

	if(damage < 1)
		damage = 1;
	take = damage;

	/* save some from armor */
	asave	= CheckArmor (targ, take, dflags);
	take	-= asave;

	if(g_debugDamage.integer)
		G_Printf("%i: client:%i health:%i damage:%i armor:%i\n",
			level.time, targ->s.number,
			targ->health, take,
			asave);

	/* add to the damage inflicted on a player this frame
	 * the total will be turned into screen blends and view angle kicks
	 * at the end of the frame */
	if(client){
		if(attacker)
			client->ps.persistant[PERS_ATTACKER] =
				attacker->s.number;
		else
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if(dir){
			copyv3 (dir, client->damage_from);
			client->damage_fromWorld = qfalse;
		}else{
			copyv3 (targ->r.currentOrigin, client->damage_from);
			client->damage_fromWorld = qtrue;
		}
	}

	/* See if it's the player hurting the enemy flag carrier */
	if(g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF){
		Team_CheckHurtCarrier(targ, attacker);
	}

	if(targ->client){
		/* set the last client who damaged the target */
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	/* do the damage */
	if(take){
		targ->health = targ->health - take;
		if(targ->client)
			targ->client->ps.stats[STAT_HEALTH] = targ->health;

		if(targ->health <= 0){
			if(client)
				targ->flags |= FL_NO_KNOCKBACK;

			if(targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
			return;
		}else if(targ->pain)
			targ->pain (targ, attacker, take);
	}

}


/*
 * CanDamage
 *
 * Returns qtrue if the inflictor can directly damage the target.  Used for
 * explosions and melee attacks.
 */
qbool
CanDamage(Gentity *targ, Vec3 origin)
{
	Vec3	dest;
	Trace tr;
	Vec3	midpoint;

	/* use the midpoint of the bounds instead of the origin, because
	 * bmodels may have their origin is 0,0,0 */
	addv3 (targ->r.absmin, targ->r.absmax, midpoint);
	scalev3 (midpoint, 0.5, midpoint);

	copyv3 (midpoint, dest);
	trap_Trace (&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE,
		MASK_SOLID);
	if(tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	/* this should probably check in the plane of projection,
	 * rather than in world coordinate, and also include Z */
	copyv3 (midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace (&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE,
		MASK_SOLID);
	if(tr.fraction == 1.0)
		return qtrue;

	copyv3 (midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace (&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE,
		MASK_SOLID);
	if(tr.fraction == 1.0)
		return qtrue;

	copyv3 (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace (&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE,
		MASK_SOLID);
	if(tr.fraction == 1.0)
		return qtrue;

	copyv3 (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace (&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE,
		MASK_SOLID);
	if(tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}

/*
 * G_RadiusDamage
 */
qbool
G_RadiusDamage(Vec3 origin, Gentity *attacker, float damage, float radius,
	       Gentity *ignore, int mod)
{
	float	points, dist;
	Gentity *ent;
	int	entityList[MAX_GENTITIES];
	int	numListedEntities;
	Vec3	mins, maxs;
	Vec3	v;
	Vec3	dir;
	int	i, e;
	qbool hitClient = qfalse;

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

		if(ent == ignore)
			continue;
		if(!ent->takedamage)
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

		points = damage * (1.0 - dist / radius);

		if(CanDamage (ent, origin)){
			if(LogAccuracyHit(ent, attacker))
				hitClient = qtrue;
			subv3 (ent->r.currentOrigin, origin, dir);
			/* push the center of mass higher than the origin so players
			 * get knocked into the air more */
			dir[2] += 24;
			G_Damage (ent, NULL, attacker, dir, origin, (int)points,
				DAMAGE_RADIUS,
				mod);
		}
	}

	return hitClient;
}
