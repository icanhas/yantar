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

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
 * Gives the activator all the items pointed to.
 */
void
Use_Target_Give(Gentity *ent, Gentity *other, Gentity *activator)
{
	Gentity	*t;
	Trace		trace;

	if(!activator->client)
		return;

	if(!ent->target)
		return;

	memset(&trace, 0, sizeof(trace));
	t = NULL;
	while((t = G_Find (t, FOFS(targetname), ent->target)) != NULL){
		if(!t->item)
			continue;
		Touch_Item(t, activator, &trace);

		/* make sure it isn't going to respawn or show any events */
		t->nextthink = 0;
		trap_UnlinkEntity(t);
	}
}

void
SP_target_give(Gentity *ent)
{
	ent->use = Use_Target_Give;
}


/* ========================================================== */

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
 * takes away all the activators powerups.
 * Used to drop flight powerups into death puts.
 */
void
Use_target_remove_powerups(Gentity *ent, Gentity *other,
			   Gentity *activator)
{
	if(!activator->client)
		return;

	if(activator->client->ps.powerups[PW_REDFLAG])
		Team_ReturnFlag(TEAM_RED);
	else if(activator->client->ps.powerups[PW_BLUEFLAG])
		Team_ReturnFlag(TEAM_BLUE);
	else if(activator->client->ps.powerups[PW_NEUTRALFLAG])
		Team_ReturnFlag(TEAM_FREE);

	memset(activator->client->ps.powerups, 0,
		sizeof(activator->client->ps.powerups));
}

void
SP_target_remove_powerups(Gentity *ent)
{
	ent->use = Use_target_remove_powerups;
}


/* ========================================================== */

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
 * "wait" seconds to pause before firing targets.
 * "random" delay variance, total delay = delay +/- random seconds
 */
void
Think_Target_Delay(Gentity *ent)
{
	G_UseTargets(ent, ent->activator);
}

void
Use_Target_Delay(Gentity *ent, Gentity *other, Gentity *activator)
{
	ent->nextthink = level.time +
			 (ent->wait + ent->random * crandom()) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void
SP_target_delay(Gentity *ent)
{
	/* check delay for backwards compatability */
	if(!G_SpawnFloat("delay", "0", &ent->wait))
		G_SpawnFloat("wait", "1", &ent->wait);

	if(!ent->wait)
		ent->wait = 1;
	ent->use = Use_Target_Delay;
}


/* ========================================================== */

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
 * "count" number of points to add, default 1
 *
 * The activator is given this many points.
 */
void
Use_Target_Score(Gentity *ent, Gentity *other, Gentity *activator)
{
	AddScore(activator, ent->r.currentOrigin, ent->count);
}

void
SP_target_score(Gentity *ent)
{
	if(!ent->count)
		ent->count = 1;
	ent->use = Use_Target_Score;
}


/* ========================================================== */

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
 * "message"	text to print
 * If "private", only the activator gets the message.  If no checks, all clients get the message.
 */
void
Use_Target_Print(Gentity *ent, Gentity *other, Gentity *activator)
{
	if(activator->client && (ent->spawnflags & 4)){
		trap_SendServerCommand(activator-g_entities,
			va("cp \"%s\"", ent->message));
		return;
	}

	if(ent->spawnflags & 3){
		if(ent->spawnflags & 1)
			G_TeamCommand(TEAM_RED, va("cp \"%s\"", ent->message));
		if(ent->spawnflags & 2)
			G_TeamCommand(TEAM_BLUE, va("cp \"%s\"", ent->message));
		return;
	}

	trap_SendServerCommand(-1, va("cp \"%s\"", ent->message));
}

void
SP_target_print(Gentity *ent)
{
	ent->use = Use_Target_Print;
}


/* ========================================================== */


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
 * "noise"		wav file to play
 *
 * A global sound will play full volume throughout the level.
 * Activator sounds will play on the player that activated the target.
 * Global and activator sounds can't be combined with looping.
 * Normal sounds play each time the target is used.
 * Looped sounds will be toggled by use functions.
 * Multiple identical looping sounds will just increase volume without any speed cost.
 * "wait" : Seconds between auto triggerings, 0 = don't auto trigger
 * "random"	wait variance, default is 0
 */
void
Use_Target_Speaker(Gentity *ent, Gentity *other, Gentity *activator)
{
	if(ent->spawnflags & 3){	/* looping sound toggles */
		if(ent->s.loopSound)
			ent->s.loopSound = 0;	/* turn it off */
		else
			ent->s.loopSound = ent->noise_index;	/* start it */
	}else{							/* normal sound */
		if(ent->spawnflags & 8)
			G_AddEvent(activator, EV_GENERAL_SOUND, ent->noise_index);
		else if(ent->spawnflags & 4)
			G_AddEvent(ent, EV_GLOBAL_SOUND, ent->noise_index);
		else
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->noise_index);
	}
}

void
SP_target_speaker(Gentity *ent)
{
	char buffer[MAX_QPATH];
	char *s;

	G_SpawnFloat("wait", "0", &ent->wait);
	G_SpawnFloat("random", "0", &ent->random);

	if(!G_SpawnString("noise", "NOSOUND", &s))
		G_Error("target_speaker without a noise key at %s",
			vtos(ent->s.origin));

	/* force all client reletive sounds to be "activator" speakers that
	 * play on the entity that activates it */
	if(s[0] == '*')
		ent->spawnflags |= 8;

	if(!strstr(s, ".wav"))
		Q_sprintf (buffer, sizeof(buffer), "%s.wav", s);
	else
		Q_strncpyz(buffer, s, sizeof(buffer));
	ent->noise_index = G_SoundIndex(buffer);

	/* a repeating speaker can be done completely client side */
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;


	/* check for prestarted looping sound */
	if(ent->spawnflags & 1)
		ent->s.loopSound = ent->noise_index;

	ent->use = Use_Target_Speaker;

	if(ent->spawnflags & 4)
		ent->r.svFlags |= SVF_BROADCAST;

	copyv3(ent->s.origin, ent->s.traj.base);

	/* must link the entity so we get areas and clusters so
	 * the server can determine who to send updates to */
	trap_LinkEntity(ent);
}



/* ========================================================== */

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
 * When triggered, fires a laser.  You can either set a target or a direction.
 */
void
target_laser_think(Gentity *self)
{
	Vec3	end;
	Trace tr;
	Vec3	point;

	/* if pointed at another entity, set movedir to point at it */
	if(self->enemy){
		saddv3 (self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		saddv3 (point, 0.5, self->enemy->r.maxs, point);
		subv3 (point, self->s.origin, self->movedir);
		normv3 (self->movedir);
	}

	/* fire forward and see what we hit */
	saddv3 (self->s.origin, 2048, self->movedir, end);

	trap_Trace(&tr, self->s.origin, NULL, NULL, end, self->s.number,
		CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE);

	if(tr.entityNum)
		/* hurt it if we can */
		G_Damage (&g_entities[tr.entityNum], self, self->activator,
			self->movedir,
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK,
			MOD_TARGET_LASER);

	copyv3 (tr.endpos, self->s.origin2);

	trap_LinkEntity(self);
	self->nextthink = level.time + FRAMETIME;
}

void
target_laser_on(Gentity *self)
{
	if(!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void
target_laser_off(Gentity *self)
{
	trap_UnlinkEntity(self);
	self->nextthink = 0;
}

void
target_laser_use(Gentity *self, Gentity *other, Gentity *activator)
{
	self->activator = activator;
	if(self->nextthink > 0)
		target_laser_off (self);
	else
		target_laser_on (self);
}

void
target_laser_start(Gentity *self)
{
	Gentity *ent;

	self->s.eType = ET_BEAM;

	if(self->target){
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if(!ent)
			G_Printf ("%s at %s: %s is a bad target\n",
				self->classname, vtos(
					self->s.origin), self->target);
		self->enemy = ent;
	}else
		G_SetMovedir (self->s.angles, self->movedir);

	self->use	= target_laser_use;
	self->think	= target_laser_think;

	if(!self->damage)
		self->damage = 1;

	if(self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void
SP_target_laser(Gentity *self)
{
	/* let everything else get spawned before we start firing */
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


/* ========================================================== */

void
target_teleporter_use(Gentity *self, Gentity *other, Gentity *activator)
{
	Gentity *dest;

	if(!activator->client)
		return;
	dest =  G_PickTarget(self->target);
	if(!dest){
		G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer(activator, dest->s.origin, dest->s.angles);
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
 * The activator will be teleported away.
 */
void
SP_target_teleporter(Gentity *self)
{
	if(!self->targetname)
		G_Printf("untargeted %s at %s\n", self->classname,
			vtos(self->s.origin));

	self->use = target_teleporter_use;
}

/* ========================================================== */


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM
 * This doesn't perform any actions except fire its targets.
 * The activator can be forced to be from a certain team.
 * if RANDOM is checked, only one of the targets will be fired, not all of them
 */
void
target_relay_use(Gentity *self, Gentity *other, Gentity *activator)
{
	if((self->spawnflags & 1) && activator->client
	   && activator->client->sess.team != TEAM_RED)
		return;
	if((self->spawnflags & 2) && activator->client
	   && activator->client->sess.team != TEAM_BLUE)
		return;
	if(self->spawnflags & 4){
		Gentity *ent;

		ent = G_PickTarget(self->target);
		if(ent && ent->use)
			ent->use(ent, self, activator);
		return;
	}
	G_UseTargets (self, activator);
}

void
SP_target_relay(Gentity *self)
{
	self->use = target_relay_use;
}


/* ========================================================== */

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
 * Kills the activator.
 */
void
target_kill_use(Gentity *self, Gentity *other, Gentity *activator)
{
	G_Damage (activator, NULL, NULL, NULL, NULL, 100000,
		DAMAGE_NO_PROTECTION,
		MOD_TELEFRAG);
}

void
SP_target_kill(Gentity *self)
{
	self->use = target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
 * Used as a positional target for in-game calculation, like jumppad targets.
 */
void
SP_target_position(Gentity *self)
{
	G_SetOrigin(self, self->s.origin);
}

static void
target_location_linkup(Gentity *ent)
{
	int	i;
	int	n;

	if(level.locationLinked)
		return;

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring(CS_LOCATIONS, "unknown");

	for(i = 0, ent = g_entities, n = 1;
	    i < level.num_entities;
	    i++, ent++)
		if(ent->classname &&
		   !Q_stricmp(ent->classname, "target_location")){
			/* lets overload some variables! */
			ent->health = n;	/* use for location marking */
			trap_SetConfigstring(CS_LOCATIONS + n, ent->message);
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}

	/* All linked together now */
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
 * Set "message" to the name of this location.
 * Set "count" to 0-7 for color.
 * 0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white
 *
 * Closest target_location in sight used for the location, if none
 * in site, closest in distance
 */
void
SP_target_location(Gentity *self)
{
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;	/* Let them all spawn first */

	G_SetOrigin(self, self->s.origin);
}
