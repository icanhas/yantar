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
 *
 * PUSHMOVE
 *
 */

typedef struct {
	Gentity	*ent;
	Vec3		origin;
	Vec3		angles;
	float		deltayaw;
} Pushed;
Pushed pushed[MAX_GENTITIES], *pushed_p;


/*
 * G_TestEntityPosition
 *
 */
Gentity       *
G_TestEntityPosition(Gentity *ent)
{
	Trace tr;
	int mask;

	if(ent->clipmask)
		mask = ent->clipmask;
	else
		mask = MASK_SOLID;
	if(ent->client)
		trap_Trace(&tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs,
			ent->client->ps.origin, ent->s.number,
			mask);
	else
		trap_Trace(&tr, ent->s.traj.base, ent->r.mins, ent->r.maxs,
			ent->s.traj.base, ent->s.number,
			mask);

	if(tr.startsolid)
		return &g_entities[ tr.entityNum ];

	return NULL;
}

/*
 * G_CreateRotationMatrix
 */
void
G_CreateRotationMatrix(Vec3 angles, Vec3 matrix[3])
{
	anglev3s(angles, matrix[0], matrix[1], matrix[2]);
	invv3(matrix[1]);
}

/*
 * G_TransposeMatrix
 */
void
G_TransposeMatrix(Vec3 matrix[3], Vec3 transpose[3])
{
	int i, j;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			transpose[i][j] = matrix[j][i];
}

/*
 * G_RotatePoint
 */
void
G_RotatePoint(Vec3 point, Vec3 matrix[3])
{
	Vec3 tvec;

	copyv3(point, tvec);
	point[0] = dotv3(matrix[0], tvec);
	point[1] = dotv3(matrix[1], tvec);
	point[2] = dotv3(matrix[2], tvec);
}

/*
 * G_TryPushingEntity
 *
 * Returns qfalse if the move is blocked
 */
qbool
G_TryPushingEntity(Gentity *check, Gentity *pusher, Vec3 move,
		   Vec3 amove)
{
	Vec3	matrix[3], transpose[3];
	Vec3	org, org2, move2;
	Gentity *block;

	/* EF_MOVER_STOP will just stop when contacting another entity
	 * instead of pushing it, but entities can still ride on top of it */
	if((pusher->s.eFlags & EF_MOVER_STOP) &&
	   check->s.groundEntityNum != pusher->s.number)
		return qfalse;

	/* save off the old position */
	if(pushed_p > &pushed[MAX_GENTITIES])
		G_Error("pushed_p > &pushed[MAX_GENTITIES]");
	pushed_p->ent = check;
	copyv3 (check->s.traj.base, pushed_p->origin);
	copyv3 (check->s.apos.base, pushed_p->angles);
	if(check->client){
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		copyv3 (check->client->ps.origin, pushed_p->origin);
	}
	pushed_p++;

	/* try moving the contacted entity
	 * figure movement due to the pusher's amove */
	G_CreateRotationMatrix(amove, transpose);
	G_TransposeMatrix(transpose, matrix);
	if(check->client)
		subv3 (check->client->ps.origin,
			pusher->r.currentOrigin,
			org);
	else
		subv3 (check->s.traj.base, pusher->r.currentOrigin,
			org);
	copyv3(org, org2);
	G_RotatePoint(org2, matrix);
	subv3 (org2, org, move2);
	/* add movement */
	addv3 (check->s.traj.base, move, check->s.traj.base);
	addv3 (check->s.traj.base, move2, check->s.traj.base);
	if(check->client){
		addv3 (check->client->ps.origin, move,
			check->client->ps.origin);
		addv3 (check->client->ps.origin, move2,
			check->client->ps.origin);
		/* make sure the client's view rotates when on a rotating mover */
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
	}

	/* may have pushed them off an edge */
	if(check->s.groundEntityNum != pusher->s.number)
		check->s.groundEntityNum = ENTITYNUM_NONE;

	block = G_TestEntityPosition(check);
	if(!block){
		/* pushed ok */
		if(check->client)
			copyv3(check->client->ps.origin,
				check->r.currentOrigin);
		else
			copyv3(check->s.traj.base, check->r.currentOrigin);
		trap_LinkEntity (check);
		return qtrue;
	}

	/* if it is ok to leave in the old position, do it
	 * this is only relevent for riding entities, not pushed
	 * Sliding trapdoors can cause this. */
	copyv3((pushed_p-1)->origin, check->s.traj.base);
	if(check->client)
		copyv3((pushed_p-1)->origin, check->client->ps.origin);
	copyv3((pushed_p-1)->angles, check->s.apos.base);
	block = G_TestEntityPosition (check);
	if(!block){
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return qtrue;
	}

	/* blocked */
	return qfalse;
}

/*
 * G_CheckProxMinePosition
 */
qbool
G_CheckProxMinePosition(Gentity *check)
{
	Vec3	start, end;
	Trace tr;

	saddv3(check->s.traj.base, 0.125, check->movedir, start);
	saddv3(check->s.traj.base, 2, check->movedir, end);
	trap_Trace(&tr, start, NULL, NULL, end, check->s.number, MASK_SOLID);

	if(tr.startsolid || tr.fraction < 1)
		return qfalse;

	return qtrue;
}

/*
 * G_TryPushingProxMine
 */
qbool
G_TryPushingProxMine(Gentity *check, Gentity *pusher, Vec3 move,
		     Vec3 amove)
{
	Vec3	forward, right, up;
	Vec3	org, org2, move2;
	int	ret;

	/* we need this for pushing things later */
	subv3 (vec3_origin, amove, org);
	anglev3s (org, forward, right, up);

	/* try moving the contacted entity */
	addv3 (check->s.traj.base, move, check->s.traj.base);

	/* figure movement due to the pusher's amove */
	subv3 (check->s.traj.base, pusher->r.currentOrigin, org);
	org2[0] = dotv3 (org, forward);
	org2[1] = -dotv3 (org, right);
	org2[2] = dotv3 (org, up);
	subv3 (org2, org, move2);
	addv3 (check->s.traj.base, move2, check->s.traj.base);

	ret = G_CheckProxMinePosition(check);
	if(ret){
		copyv3(check->s.traj.base, check->r.currentOrigin);
		trap_LinkEntity (check);
	}
	return ret;
}

void G_ExplodeMissile(Gentity *ent);

/*
 * G_MoverPush
 *
 * Objects need to be moved back on a failed push,
 * otherwise riders would continue to slide.
 * If qfalse is returned, *obstacle will be the blocking entity
 */
qbool
G_MoverPush(Gentity *pusher, Vec3 move, Vec3 amove, Gentity **obstacle)
{
	int i, e;
	Gentity	*check;
	Vec3		mins, maxs;
	Pushed *p;
	int	entityList[MAX_GENTITIES];
	int	listedEntities;
	Vec3 totalMins, totalMaxs;

	*obstacle = NULL;


	/* mins/maxs are the bounds at the destination
	 * totalMins / totalMaxs are the bounds for the entire move */
	if(pusher->r.currentAngles[0] || pusher->r.currentAngles[1] ||
	   pusher->r.currentAngles[2]
	   || amove[0] || amove[1] || amove[2]){
		float radius;

		radius = RadiusFromBounds(pusher->r.mins, pusher->r.maxs);
		for(i = 0; i < 3; i++){
			mins[i] = pusher->r.currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->r.currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
		}
	}else{
		for(i=0; i<3; i++){
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		copyv3(pusher->r.absmin, totalMins);
		copyv3(pusher->r.absmax, totalMaxs);
		for(i=0; i<3; i++){
			if(move[i] > 0)
				totalMaxs[i] += move[i];
			else
				totalMins[i] += move[i];
		}
	}

	/* unlink the pusher so we don't get it in the entityList */
	trap_UnlinkEntity(pusher);

	listedEntities = trap_EntitiesInBox(totalMins, totalMaxs, entityList,
		MAX_GENTITIES);

	/* move the pusher to its final position */
	addv3(pusher->r.currentOrigin, move, pusher->r.currentOrigin);
	addv3(pusher->r.currentAngles, amove, pusher->r.currentAngles);
	trap_LinkEntity(pusher);

	/* see if any solid entities are inside the final position */
	for(e = 0; e < listedEntities; e++){
		check = &g_entities[ entityList[ e ] ];

#ifdef MISSIONPACK
		if(check->s.eType == ET_MISSILE)
			/* if it is a prox mine */
			if(!strcmp(check->classname, "prox mine")){
				/* if this prox mine is attached to this mover try to move it with the pusher */
				if(check->enemy == pusher){
					if(!G_TryPushingProxMine(check, pusher,
						   move, amove)){
						/* explode */
						check->s.loopSound = 0;
						G_AddEvent(
							check,
							EV_PROXIMITY_MINE_TRIGGER,
							0);
						G_ExplodeMissile(check);
						if(check->activator){
							G_FreeEntity(
								check->activator);
							check->activator = NULL;
						}
						/* G_Printf("prox mine explodes\n"); */
					}
				}else
				/* check if the prox mine is crushed by the mover */
				if(!G_CheckProxMinePosition(check)){
					/* explode */
					check->s.loopSound = 0;
					G_AddEvent(check,
						EV_PROXIMITY_MINE_TRIGGER,
						0);
					G_ExplodeMissile(check);
					if(check->activator){
						G_FreeEntity(check->activator);
						check->activator = NULL;
					}
					/* G_Printf("prox mine explodes\n"); */
				}
				continue;
			}

#endif
		/* only push items and players */
		if(check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER &&
		   !check->physicsObject)
			continue;

		/* if the entity is standing on the pusher, it will definitely be moved */
		if(check->s.groundEntityNum != pusher->s.number){
			/* see if the ent needs to be tested */
			if(check->r.absmin[0] >= maxs[0]
			   || check->r.absmin[1] >= maxs[1]
			   || check->r.absmin[2] >= maxs[2]
			   || check->r.absmax[0] <= mins[0]
			   || check->r.absmax[1] <= mins[1]
			   || check->r.absmax[2] <= mins[2])
				continue;
			/* see if the ent's bbox is inside the pusher's final position
			 * this does allow a fast moving object to pass through a thin entity... */
			if(!G_TestEntityPosition (check))
				continue;
		}

		/* the entity needs to be pushed */
		if(G_TryPushingEntity(check, pusher, move, amove))
			continue;

		/* the move was blocked an entity */

		/* bobbing entities are instant-kill and never get blocked */
		if(pusher->s.traj.type == TR_SINE || pusher->s.apos.type ==
		   TR_SINE){
			G_Damage(check, pusher, pusher, NULL, NULL, 99999, 0,
				MOD_CRUSH);
			continue;
		}


		/* save off the obstacle so we can call the block function (crush, etc) */
		*obstacle = check;

		/* move back any entities we already moved
		 * go backwards, so if the same entity was pushed
		 * twice, it goes back to the original position */
		for(p=pushed_p-1; p>=pushed; p--){
			copyv3 (p->origin, p->ent->s.traj.base);
			copyv3 (p->angles, p->ent->s.apos.base);
			if(p->ent->client){
				p->ent->client->ps.delta_angles[YAW] =
					p->deltayaw;
				copyv3 (p->origin, p->ent->client->ps.origin);
			}
			trap_LinkEntity (p->ent);
		}
		return qfalse;
	}

	return qtrue;
}


/*
 * G_MoverTeam
 */
void
G_MoverTeam(Gentity *ent)
{
	Vec3	move, amove;
	Gentity       *part, *obstacle;
	Vec3	origin, angles;

	obstacle = NULL;

	/* make sure all team slaves can move before commiting
	 * any moves or calling any think functions
	 * if the move is blocked, all moved objects will be backed out */
	pushed_p = pushed;
	for(part = ent; part; part=part->teamchain){
		/* get current position */
		BG_EvaluateTrajectory(&part->s.traj, level.time, origin);
		BG_EvaluateTrajectory(&part->s.apos, level.time, angles);
		subv3(origin, part->r.currentOrigin, move);
		subv3(angles, part->r.currentAngles, amove);
		if(!G_MoverPush(part, move, amove, &obstacle))
			break;	/* move was blocked */
	}

	if(part){
		/* go back to the previous position */
		for(part = ent; part; part = part->teamchain){
			part->s.traj.time += level.time -
					      level.previousTime;
			part->s.apos.time += level.time -
					       level.previousTime;
			BG_EvaluateTrajectory(&part->s.traj, level.time,
				part->r.currentOrigin);
			BG_EvaluateTrajectory(&part->s.apos, level.time,
				part->r.currentAngles);
			trap_LinkEntity(part);
		}

		/* if the pusher has a "blocked" function, call it */
		if(ent->blocked)
			ent->blocked(ent, obstacle);
		return;
	}

	/* the move succeeded */
	for(part = ent; part; part = part->teamchain)
		/* call the reached function if time is at or past end point */
		if(part->s.traj.type == TR_LINEAR_STOP)
			if(level.time >= part->s.traj.time +
			   part->s.traj.duration)
				if(part->reached)
					part->reached(part);
}

/*
 * G_RunMover
 *
 */
void
G_RunMover(Gentity *ent)
{
	/* if not a team captain, don't do anything, because
	 * the captain will handle everything */
	if(ent->flags & FL_TEAMSLAVE)
		return;

	/* if stationary at one of the positions, don't move anything */
	if(ent->s.traj.type != TR_STATIONARY || ent->s.apos.type !=
	   TR_STATIONARY)
		G_MoverTeam(ent);

	/* check think function */
	G_RunThink(ent);
}

/*
 *
 * GENERAL MOVERS
 *
 * Doors, plats, and buttons are all binary (two position) movers
 * Pos1 is "at rest", pos2 is "activated"
 */

/*
 * SetMoverState
 */
void
SetMoverState(Gentity *ent, moverState_t moverState, int time)
{
	Vec3	delta;
	float	f;

	ent->moverState = moverState;

	ent->s.traj.time = time;
	switch(moverState){
	case MOVER_POS1:
		copyv3(ent->pos1, ent->s.traj.base);
		ent->s.traj.type = TR_STATIONARY;
		break;
	case MOVER_POS2:
		copyv3(ent->pos2, ent->s.traj.base);
		ent->s.traj.type = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		copyv3(ent->pos1, ent->s.traj.base);
		subv3(ent->pos2, ent->pos1, delta);
		f = 1000.0 / ent->s.traj.duration;
		scalev3(delta, f, ent->s.traj.delta);
		ent->s.traj.type = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:
		copyv3(ent->pos2, ent->s.traj.base);
		subv3(ent->pos1, ent->pos2, delta);
		f = 1000.0 / ent->s.traj.duration;
		scalev3(delta, f, ent->s.traj.delta);
		ent->s.traj.type = TR_LINEAR_STOP;
		break;
	}
	BG_EvaluateTrajectory(&ent->s.traj, level.time, ent->r.currentOrigin);
	trap_LinkEntity(ent);
}

/*
 * MatchTeam
 *
 * All entities in a mover team will move from pos1 to pos2
 * in the same amount of time
 */
void
MatchTeam(Gentity *teamLeader, int moverState, int time)
{
	Gentity *slave;

	for(slave = teamLeader; slave; slave = slave->teamchain)
		SetMoverState(slave, moverState, time);
}



/*
 * ReturnToPos1
 */
void
ReturnToPos1(Gentity *ent)
{
	MatchTeam(ent, MOVER_2TO1, level.time);

	/* looping sound */
	ent->s.loopSound = ent->soundLoop;

	/* starting sound */
	if(ent->sound2to1)
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);
}


/*
 * Reached_BinaryMover
 */
void
Reached_BinaryMover(Gentity *ent)
{

	/* stop the looping sound */
	ent->s.loopSound = ent->soundLoop;

	if(ent->moverState == MOVER_1TO2){
		/* reached pos2 */
		SetMoverState(ent, MOVER_POS2, level.time);

		/* play sound */
		if(ent->soundPos2)
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos2);

		/* return to pos1 after a delay */
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + ent->wait;

		/* fire targets */
		if(!ent->activator)
			ent->activator = ent;
		G_UseTargets(ent, ent->activator);
	}else if(ent->moverState == MOVER_2TO1){
		/* reached pos1 */
		SetMoverState(ent, MOVER_POS1, level.time);

		/* play sound */
		if(ent->soundPos1)
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos1);

		/* close areaportals */
		if(ent->teammaster == ent || !ent->teammaster)
			trap_AdjustAreaPortalState(ent, qfalse);
	}else
		G_Error("Reached_BinaryMover: bad moverState");
}


/*
 * Use_BinaryMover
 */
void
Use_BinaryMover(Gentity *ent, Gentity *other, Gentity *activator)
{
	int	total;
	int	partial;

	/* only the master should be used */
	if(ent->flags & FL_TEAMSLAVE){
		Use_BinaryMover(ent->teammaster, other, activator);
		return;
	}

	ent->activator = activator;

	if(ent->moverState == MOVER_POS1){
		/* start moving 50 msec later, becase if this was player
		 * triggered, level.time hasn't been advanced yet */
		MatchTeam(ent, MOVER_1TO2, level.time + 50);

		/* starting sound */
		if(ent->sound1to2)
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);

		/* looping sound */
		ent->s.loopSound = ent->soundLoop;

		/* open areaportal */
		if(ent->teammaster == ent || !ent->teammaster)
			trap_AdjustAreaPortalState(ent, qtrue);
		return;
	}

	/* if all the way up, just delay before coming down */
	if(ent->moverState == MOVER_POS2){
		ent->nextthink = level.time + ent->wait;
		return;
	}

	/* only partway down before reversing */
	if(ent->moverState == MOVER_2TO1){
		total	= ent->s.traj.duration;
		partial = level.time - ent->s.traj.time;
		if(partial > total)
			partial = total;

		MatchTeam(ent, MOVER_1TO2, level.time - (total - partial));

		if(ent->sound1to2)
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
		return;
	}

	/* only partway up before reversing */
	if(ent->moverState == MOVER_1TO2){
		total	= ent->s.traj.duration;
		partial = level.time - ent->s.traj.time;
		if(partial > total)
			partial = total;

		MatchTeam(ent, MOVER_2TO1, level.time - (total - partial));

		if(ent->sound2to1)
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);
		return;
	}
}



/*
 * InitMover
 *
 * "pos1", "pos2", and "speed" should be set before calling,
 * so the movement delta can be calculated
 */
void
InitMover(Gentity *ent)
{
	Vec3	move;
	float	distance;
	float	light;
	Vec3	color;
	qbool lightSet, colorSet;
	char *sound;

	/* if the "model2" key is set, use a seperate model
	 * for drawing, but clip against the brushes */
	if(ent->model2)
		ent->s.modelindex2 = G_ModelIndex(ent->model2);

	/* if the "loopsound" key is set, use a constant looping sound when moving */
	if(G_SpawnString("noise", "100", &sound))
		ent->s.loopSound = G_SoundIndex(sound);

	/* if the "color" or "light" keys are set, setup constantLight */
	lightSet = G_SpawnFloat("light", "100", &light);
	colorSet = G_SpawnVector("color", "1 1 1", color);
	if(lightSet || colorSet){
		int r, g, b, i;

		r = color[0] * 255;
		if(r > 255)
			r = 255;
		g = color[1] * 255;
		if(g > 255)
			g = 255;
		b = color[2] * 255;
		if(b > 255)
			b = 255;
		i = light / 4;
		if(i > 255)
			i = 255;
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}


	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags	= SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;
	copyv3 (ent->pos1, ent->r.currentOrigin);
	trap_LinkEntity (ent);

	ent->s.traj.type = TR_STATIONARY;
	copyv3(ent->pos1, ent->s.traj.base);

	/* calculate time to reach second position from speed */
	subv3(ent->pos2, ent->pos1, move);
	distance = lenv3(move);
	if(!ent->speed)
		ent->speed = 100;
	scalev3(move, ent->speed, ent->s.traj.delta);
	ent->s.traj.duration = distance * 1000 / ent->speed;
	if(ent->s.traj.duration <= 0)
		ent->s.traj.duration = 1;
}


/*
 *
 * DOOR
 *
 * A use can be triggered either by a touch function, by being shot, or by being
 * targeted by another entity.
 *
 */

/*
 * Blocked_Door
 */
void
Blocked_Door(Gentity *ent, Gentity *other)
{
	/* remove anything other than a client */
	if(!other->client){
		/* except CTF flags!!!! */
		if(other->s.eType == ET_ITEM && other->item->type ==
		   IT_TEAM){
			Team_DroppedFlagThink(other);
			return;
		}
		G_TempEntity(other->s.origin, EV_ITEM_POP);
		G_FreeEntity(other);
		return;
	}

	if(ent->damage)
		G_Damage(other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH);
	if(ent->spawnflags & 4)
		return;		/* crushers don't reverse */

	/* reverse direction */
	Use_BinaryMover(ent, ent, other);
}

/*
 * Touch_DoorTriggerSpectator
 */
static void
Touch_DoorTriggerSpectator(Gentity *ent, Gentity *other, Trace *trace)
{
	int axis;
	float	doorMin, doorMax;
	Vec3	origin;

	axis = ent->count;
	/* the constants below relate to constants in Think_SpawnNewDoorTrigger() */
	doorMin = ent->r.absmin[axis] + 100;
	doorMax = ent->r.absmax[axis] - 100;

	copyv3(other->client->ps.origin, origin);

	if(origin[axis] < doorMin || origin[axis] > doorMax) return;

	if(fabs(origin[axis] - doorMax) < fabs(origin[axis] - doorMin))
		origin[axis] = doorMin - 10;
	else
		origin[axis] = doorMax + 10;

	TeleportPlayer(other, origin, tv(10000000.0, 0, 0));
}

/*
 * Touch_DoorTrigger
 */
void
Touch_DoorTrigger(Gentity *ent, Gentity *other, Trace *trace)
{
	if(other->client && other->client->sess.sessionTeam ==
	   TEAM_SPECTATOR){
		/* if the door is not open and not opening */
		if(ent->parent->moverState != MOVER_1TO2 &&
		   ent->parent->moverState != MOVER_POS2)
			Touch_DoorTriggerSpectator(ent, other, trace);
	}else if(ent->parent->moverState != MOVER_1TO2)
		Use_BinaryMover(ent->parent, ent, other);
}


/*
 * Think_SpawnNewDoorTrigger
 *
 * All of the parts of a door have been spawned, so create
 * a trigger that encloses all of them
 */
void
Think_SpawnNewDoorTrigger(Gentity *ent)
{
	Gentity	*other;
	Vec3		mins, maxs;
	int i, best;

	/* set all of the slaves as shootable */
	for(other = ent; other; other = other->teamchain)
		other->takedamage = qtrue;

	/* find the bounds of everything on the team */
	copyv3 (ent->r.absmin, mins);
	copyv3 (ent->r.absmax, maxs);

	for(other = ent->teamchain; other; other=other->teamchain){
		AddPointToBounds (other->r.absmin, mins, maxs);
		AddPointToBounds (other->r.absmax, mins, maxs);
	}

	/* find the thinnest axis, which will be the one we expand */
	best = 0;
	for(i = 1; i < 3; i++)
		if(maxs[i] - mins[i] < maxs[best] - mins[best])
			best = i;
	maxs[best] += 120;
	mins[best] -= 120;

	/* create a trigger with this size */
	other = G_Spawn ();
	other->classname = "door_trigger";
	copyv3 (mins, other->r.mins);
	copyv3 (maxs, other->r.maxs);
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	/* remember the thinnest axis */
	other->count = best;
	trap_LinkEntity (other);

	MatchTeam(ent, ent->moverState, level.time);
}

void
Think_MatchTeam(Gentity *ent)
{
	MatchTeam(ent, ent->moverState, level.time);
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER
 * TOGGLE		wait in both the start and end states for a trigger event.
 * START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
 * NOMONSTER	monsters will not trigger this door
 *
 * "model2"	.md3 model to also draw
 * "angle"		determines the opening direction
 * "targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
 * "speed"		movement speed (100 default)
 * "wait"		wait before returning (3 default, -1 = never return)
 * "lip"		lip remaining at end of move (8 default)
 * "dmg"		damage to inflict when blocked (2 default)
 * "color"		constantLight color
 * "light"		constantLight radius
 * "health"	if set, the door must be shot open
 */
void
SP_func_door(Gentity *ent)
{
	Vec3	abs_movedir;
	float	distance;
	Vec3	size;
	float	lip;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex(
		Pdoorsounds "/dr1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex(
		Pdoorsounds "/dr1_end.wav");

	ent->blocked = Blocked_Door;

	/* default speed of 400 */
	if(!ent->speed)
		ent->speed = 400;

	/* default wait of 2 seconds */
	if(!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	/* default lip of 8 units */
	G_SpawnFloat("lip", "8", &lip);

	/* default damage of 2 points */
	G_SpawnInt("dmg", "2", &ent->damage);

	/* first position at start */
	copyv3(ent->s.origin, ent->pos1);

	/* calculate second position */
	trap_SetBrushModel(ent, ent->model);
	G_SetMovedir (ent->s.angles, ent->movedir);
	abs_movedir[0]	= fabs(ent->movedir[0]);
	abs_movedir[1]	= fabs(ent->movedir[1]);
	abs_movedir[2]	= fabs(ent->movedir[2]);
	subv3(ent->r.maxs, ent->r.mins, size);
	distance = dotv3(abs_movedir, size) - lip;
	saddv3(ent->pos1, distance, ent->movedir, ent->pos2);

	/* if "start_open", reverse position 1 and 2 */
	if(ent->spawnflags & 1){
		Vec3 temp;

		copyv3(ent->pos2, temp);
		copyv3(ent->s.origin, ent->pos2);
		copyv3(temp, ent->pos1);
	}

	InitMover(ent);

	ent->nextthink = level.time + FRAMETIME;

	if(!(ent->flags & FL_TEAMSLAVE)){
		int health;

		G_SpawnInt("health", "0", &health);
		if(health)
			ent->takedamage = qtrue;
		if(ent->targetname || health)
			/* non touch/shoot doors */
			ent->think = Think_MatchTeam;
		else
			ent->think = Think_SpawnNewDoorTrigger;
	}


}

/*
 *
 * PLAT
 *
 */

/*
 * Touch_Plat
 *
 * Don't allow decent if a living player is on it
 */
void
Touch_Plat(Gentity *ent, Gentity *other, Trace *trace)
{
	if(!other->client || other->client->ps.stats[STAT_HEALTH] <= 0)
		return;

	/* delay return-to-pos1 by one second */
	if(ent->moverState == MOVER_POS2)
		ent->nextthink = level.time + 1000;
}

/*
 * Touch_PlatCenterTrigger
 *
 * If the plat is at the bottom position, start it going up
 */
void
Touch_PlatCenterTrigger(Gentity *ent, Gentity *other, Trace *trace)
{
	if(!other->client)
		return;

	if(ent->parent->moverState == MOVER_POS1)
		Use_BinaryMover(ent->parent, ent, other);
}


/*
 * SpawnPlatTrigger
 *
 * Spawn a trigger in the middle of the plat's low position
 * Elevator cars require that the trigger extend through the entire low position,
 * not just sit on top of it.
 */
void
SpawnPlatTrigger(Gentity *ent)
{
	Gentity	*trigger;
	Vec3		tmin, tmax;

	/* the middle trigger will be a thin trigger just
	 * above the starting position */
	trigger = G_Spawn();
	trigger->classname = "plat_trigger";
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[0] = ent->pos1[0] + ent->r.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->r.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->r.mins[2];

	tmax[0] = ent->pos1[0] + ent->r.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->r.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->r.maxs[2] + 8;

	if(tmax[0] <= tmin[0]){
		tmin[0] = ent->pos1[0] + (ent->r.mins[0] + ent->r.maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if(tmax[1] <= tmin[1]){
		tmin[1] = ent->pos1[1] + (ent->r.mins[1] + ent->r.maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}

	copyv3 (tmin, trigger->r.mins);
	copyv3 (tmax, trigger->r.maxs);

	trap_LinkEntity (trigger);
}


/*QUAKED func_plat (0 .5 .8) ?
 * Plats are always drawn in the extended position so they will light correctly.
 *
 * "lip"		default 8, protrusion above rest position
 * "height"	total height of movement, defaults to model height
 * "speed"		overrides default 200.
 * "dmg"		overrides default 2
 * "model2"	.md3 model to also draw
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_plat(Gentity *ent)
{
	float lip, height;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex(
		Pplatformsounds "/pt1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex(
		Pplatformsounds "/pt1_end.wav");

	clearv3 (ent->s.angles);

	G_SpawnFloat("speed", "200", &ent->speed);
	G_SpawnInt("dmg", "2", &ent->damage);
	G_SpawnFloat("wait", "1", &ent->wait);
	G_SpawnFloat("lip", "8", &lip);

	ent->wait = 1000;

	/* create second position */
	trap_SetBrushModel(ent, ent->model);

	if(!G_SpawnFloat("height", "0", &height))
		height = (ent->r.maxs[2] - ent->r.mins[2]) - lip;

	/* pos1 is the rest (bottom) position, pos2 is the top */
	copyv3(ent->s.origin, ent->pos2);
	copyv3(ent->pos2, ent->pos1);
	ent->pos1[2] -= height;

	InitMover(ent);

	/* touch function keeps the plat from returning while
	 * a live player is standing on it */
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;	/* so it can be treated as a door */

	/* spawn the trigger if one hasn't been custom made */
	if(!ent->targetname)
		SpawnPlatTrigger(ent);
}


/*
 *
 * BUTTON
 *
 */

/*
 * Touch_Button
 *
 */
void
Touch_Button(Gentity *ent, Gentity *other, Trace *trace)
{
	if(!other->client)
		return;

	if(ent->moverState == MOVER_POS1)
		Use_BinaryMover(ent, other, other);
}


/*QUAKED func_button (0 .5 .8) ?
 * When a button is touched, it moves some distance in the direction of its angle, triggers all of its targets, waits some time, then returns to its original position where it can be triggered again.
 *
 * "model2"	.md3 model to also draw
 * "angle"		determines the opening direction
 * "target"	all entities with a matching targetname will be used
 * "speed"		override the default 40 speed
 * "wait"		override the default 1 second wait (-1 = never return)
 * "lip"		override the default 4 pixel lip remaining at end of move
 * "health"	if set, the button must be killed instead of touched
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_button(Gentity *ent)
{
	Vec3	abs_movedir;
	float	distance;
	Vec3	size;
	float	lip;

	ent->sound1to2 = G_SoundIndex(Pswitchsounds "/butn2.wav");

	if(!ent->speed)
		ent->speed = 40;

	if(!ent->wait)
		ent->wait = 1;
	ent->wait *= 1000;

	/* first position */
	copyv3(ent->s.origin, ent->pos1);

	/* calculate second position */
	trap_SetBrushModel(ent, ent->model);

	G_SpawnFloat("lip", "4", &lip);

	G_SetMovedir(ent->s.angles, ent->movedir);
	abs_movedir[0]	= fabs(ent->movedir[0]);
	abs_movedir[1]	= fabs(ent->movedir[1]);
	abs_movedir[2]	= fabs(ent->movedir[2]);
	subv3(ent->r.maxs, ent->r.mins, size);
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] +
		   abs_movedir[2] * size[2] - lip;
	saddv3 (ent->pos1, distance, ent->movedir, ent->pos2);

	if(ent->health)
		/* shootable button */
		ent->takedamage = qtrue;
	else
		/* touchable button */
		ent->touch = Touch_Button;

	InitMover(ent);
}



/*
 *
 * TRAIN
 *
 */


#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

/*
 * Think_BeginMoving
 *
 * The wait time at a corner has completed, so start moving again
 */
void
Think_BeginMoving(Gentity *ent)
{
	ent->s.traj.time = level.time;
	ent->s.traj.type = TR_LINEAR_STOP;
}

/*
 * Reached_Train
 */
void
Reached_Train(Gentity *ent)
{
	Gentity	*next;
	float speed;
	Vec3		move;
	float length;

	/* copy the apropriate values */
	next = ent->nextTrain;
	if(!next || !next->nextTrain)
		return;		/* just stop */

	/* fire all other targets */
	G_UseTargets(next, NULL);

	/* set the new trajectory */
	ent->nextTrain = next->nextTrain;
	copyv3(next->s.origin, ent->pos1);
	copyv3(next->nextTrain->s.origin, ent->pos2);

	/* if the path_corner has a speed, use that */
	if(next->speed)
		speed = next->speed;
	else
		/* otherwise use the train's speed */
		speed = ent->speed;
	if(speed < 1)
		speed = 1;

	/* calculate duration */
	subv3(ent->pos2, ent->pos1, move);
	length = lenv3(move);

	ent->s.traj.duration = length * 1000 / speed;

	/* Tequila comment: Be sure to send to clients after any fast move case */
	ent->r.svFlags &= ~SVF_NOCLIENT;

	/* Tequila comment: Fast move case */
	if(ent->s.traj.duration<1){
		/* Tequila comment: As duration is used later in a division, we need to avoid that case now
		 * With null duration,
		 * the calculated rocks bounding box becomes infinite and the engine think for a short time
		 * any entity is riding that mover but not the world entity... In rare case, I found it
		 * can also stuck every map entities after func_door are used.
		 * The desired effect with very very big speed is to have instant move, so any not null duration
		 * lower than a frame duration should be sufficient.
		 * Afaik, the negative case don't have to be supported. */
		ent->s.traj.duration=1;

		/* Tequila comment: Don't send entity to clients so it becomes really invisible */
		ent->r.svFlags |= SVF_NOCLIENT;
	}

	/* looping sound */
	ent->s.loopSound = next->soundLoop;

	/* start it going */
	SetMoverState(ent, MOVER_1TO2, level.time);

	/* if there is a "wait" value on the target, don't start moving yet */
	if(next->wait){
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->s.traj.type = TR_STATIONARY;
	}
}


/*
 * Think_SetupTrainTargets
 *
 * Link all the corners together
 */
void
Think_SetupTrainTargets(Gentity *ent)
{
	Gentity *path, *next, *start;

	ent->nextTrain = G_Find(NULL, FOFS(targetname), ent->target);
	if(!ent->nextTrain){
		G_Printf("func_train at %s with an unfound target\n",
			vtos(ent->r.absmin));
		return;
	}

	start = NULL;
	for(path = ent->nextTrain; path != start; path = next){
		if(!start)
			start = path;

		if(!path->target){
			G_Printf("Train corner at %s without a target\n",
				vtos(path->s.origin));
			return;
		}

		/* find a path_corner among the targets
		 * there may also be other targets that get fired when the corner
		 * is reached */
		next = NULL;
		do {
			next = G_Find(next, FOFS(targetname), path->target);
			if(!next){
				G_Printf(
					"Train corner at %s without a target path_corner\n",
					vtos(path->s.origin));
				return;
			}
		} while(strcmp(next->classname, "path_corner"));

		path->nextTrain = next;
	}

	/* start the train moving from the first corner */
	Reached_Train(ent);
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8)
 * Train path corners.
 * Target: next path corner and other targets to fire
 * "speed" speed to move to the next corner
 * "wait" seconds to wait before behining move to next corner
 */
void
SP_path_corner(Gentity *self)
{
	if(!self->targetname){
		G_Printf ("path_corner with no targetname at %s\n",
			vtos(self->s.origin));
		G_FreeEntity(self);
		return;
	}
	/* path corners don't need to be linked in */
}



/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
 * A train is a mover that moves between path_corner target points.
 * Trains MUST HAVE AN ORIGIN BRUSH.
 * The train spawns at the first target it is pointing at.
 * "model2"	.md3 model to also draw
 * "speed"		default 100
 * "dmg"		default	2
 * "noise"		looping sound to play when the train is in motion
 * "target"	next path corner
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_train(Gentity *self)
{
	clearv3 (self->s.angles);

	if(self->spawnflags & TRAIN_BLOCK_STOPS)
		self->damage = 0;
	else if(!self->damage)
		self->damage = 2;


	if(!self->speed)
		self->speed = 100;

	if(!self->target){
		G_Printf ("func_train without a target at %s\n",
			vtos(self->r.absmin));
		G_FreeEntity(self);
		return;
	}

	trap_SetBrushModel(self, self->model);
	InitMover(self);

	self->reached = Reached_Train;

	/* start trains on the second frame, to make sure their targets have had
	 * a chance to spawn */
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;
}

/*
 *
 * STATIC
 *
 */


/*QUAKED func_static (0 .5 .8) ?
 * A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
 * "model2"	.md3 model to also draw
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_static(Gentity *ent)
{
	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);
	copyv3(ent->s.origin, ent->s.traj.base);
	copyv3(ent->s.origin, ent->r.currentOrigin);
}


/*
 *
 * ROTATING
 *
 */


/*QUAKED func_rotating (0 .5 .8) ? START_ON - X_AXIS Y_AXIS
 * You need to have an origin brush as part of this entity.  The center of that brush will be
 * the point around which it is rotated. It will rotate around the Z axis by default.  You can
 * check either the X_AXIS or Y_AXIS box to change that.
 *
 * "model2"	.md3 model to also draw
 * "speed"		determines how fast it moves; default value is 100.
 * "dmg"		damage to inflict when blocked (2 default)
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_rotating(Gentity *ent)
{
	if(!ent->speed)
		ent->speed = 100;

	/* set the axis of rotation */
	ent->s.apos.type = TR_LINEAR;
	if(ent->spawnflags & 4)
		ent->s.apos.delta[2] = ent->speed;
	else if(ent->spawnflags & 8)
		ent->s.apos.delta[0] = ent->speed;
	else
		ent->s.apos.delta[1] = ent->speed;

	if(!ent->damage)
		ent->damage = 2;

	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);

	copyv3(ent->s.origin, ent->s.traj.base);
	copyv3(ent->s.traj.base, ent->r.currentOrigin);
	copyv3(ent->s.apos.base, ent->r.currentAngles);

	trap_LinkEntity(ent);
}


/*
 *
 * BOBBING
 *
 */


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
 * Normally bobs on the Z axis
 * "model2"	.md3 model to also draw
 * "height"	amplitude of bob (32 default)
 * "speed"		seconds to complete a bob cycle (4 default)
 * "phase"		the 0.0 to 1.0 offset in the cycle to start at
 * "dmg"		damage to inflict when blocked (2 default)
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_bobbing(Gentity *ent)
{
	float	height;
	float	phase;

	G_SpawnFloat("speed", "4", &ent->speed);
	G_SpawnFloat("height", "32", &height);
	G_SpawnInt("dmg", "2", &ent->damage);
	G_SpawnFloat("phase", "0", &phase);

	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);

	copyv3(ent->s.origin, ent->s.traj.base);
	copyv3(ent->s.origin, ent->r.currentOrigin);

	ent->s.traj.duration	= ent->speed * 1000;
	ent->s.traj.time	= ent->s.traj.duration * phase;
	ent->s.traj.type	= TR_SINE;

	/* set the axis of bobbing */
	if(ent->spawnflags & 1)
		ent->s.traj.delta[0] = height;
	else if(ent->spawnflags & 2)
		ent->s.traj.delta[1] = height;
	else
		ent->s.traj.delta[2] = height;
}

/*
 *
 * PENDULUM
 *
 */


/*QUAKED func_pendulum (0 .5 .8) ?
 * You need to have an origin brush as part of this entity.
 * Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
 * Pendulum frequency is a physical constant based on the length of the beam and gravity.
 * "model2"	.md3 model to also draw
 * "speed"		the number of degrees each way the pendulum swings, (30 default)
 * "phase"		the 0.0 to 1.0 offset in the cycle to start at
 * "dmg"		damage to inflict when blocked (2 default)
 * "color"		constantLight color
 * "light"		constantLight radius
 */
void
SP_func_pendulum(Gentity *ent)
{
	float	freq;
	float	length;
	float	phase;
	float	speed;

	G_SpawnFloat("speed", "30", &speed);
	G_SpawnInt("dmg", "2", &ent->damage);
	G_SpawnFloat("phase", "0", &phase);

	trap_SetBrushModel(ent, ent->model);

	/* find pendulum length */
	length = fabs(ent->r.mins[2]);
	if(length < 8)
		length = 8;

	freq = 1 / (M_PI * 2) * sqrt(g_gravity.value / (3 * length));

	ent->s.traj.duration = (1000 / freq);

	InitMover(ent);

	copyv3(ent->s.origin, ent->s.traj.base);
	copyv3(ent->s.origin, ent->r.currentOrigin);

	copyv3(ent->s.angles, ent->s.apos.base);

	ent->s.apos.duration	= 1000 / freq;
	ent->s.apos.time	= ent->s.apos.duration * phase;
	ent->s.apos.type	= TR_SINE;
	ent->s.apos.delta[2]	= speed;
}
