/* present snapshot entities, happens every single frame */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

/*
 * Modifies the entities position and axis by the given
 * tag location
 */
void
CG_PositionEntityOnTag(Refent *entity, const Refent *parent,
		       Handle parentModel, char *tagName)
{
	int i;
	Orient lerped;

	/* lerp the tag */
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName);

	/* FIXME: allow origin offsets along tag? */
	copyv3(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		saddv3(entity->origin, lerped.origin[i], parent->axis[i],
			entity->origin);

	/* had to cast away the const to avoid compiler problems... */
	MatrixMultiply(lerped.axis, ((Refent*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
 * Modifies the entities position and axis by the given
 * tag location
 */
void
CG_PositionRotatedEntityOnTag(Refent *entity, const Refent *parent,
			      Handle parentModel, char *tagName)
{
	int i;
	Orient lerped;
	Vec3 tempAxis[3];

/* clearaxis( entity->axis ); */
	/* lerp the tag */
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName);

	/* FIXME: allow origin offsets along tag? */
	copyv3(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		saddv3(entity->origin, lerped.origin[i], parent->axis[i],
			entity->origin);

	/* had to cast away the const to avoid compiler problems... */
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((Refent*)parent)->axis, entity->axis);
}

/*
 * Functions called each frame
 */

/*
 * Also called by event processing code
 */
void
CG_SetEntitySoundPosition(Centity *cent)
{
	if(cent->currentState.solid == SOLID_BMODEL){
		Vec3	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		addv3(cent->lerpOrigin, v, origin);
		trap_sndupdateentpos(cent->currentState.number, origin);
	}else
		trap_sndupdateentpos(cent->currentState.number,
			cent->lerpOrigin);
}

/*
 * Add continuous entity effects, like local entity emission and lighting
 */
static void
CG_EntityEffects(Centity *cent)
{

	/* update sound origins */
	CG_SetEntitySoundPosition(cent);

	/* add loop sound */
	if(cent->currentState.loopSound){
		if(cent->currentState.eType != ET_SPEAKER)
			trap_sndaddloop(
				cent->currentState.number, cent->lerpOrigin,
				vec3_origin,
				cgs.gameSounds[ cent->currentState.loopSound ]);
		else
			trap_sndaddrealloop(
				cent->currentState.number, cent->lerpOrigin,
				vec3_origin,
				cgs.gameSounds[ cent->currentState.loopSound ]);
	}


	/* constant light glow */
	if(cent->currentState.constantLight){
		int cl;
		float i, r, g, b;

		cl = cent->currentState.constantLight;
		r = (float)(cl & 0xFF) / 255.0;
		g = (float)((cl >> 8) & 0xFF) / 255.0;
		b = (float)((cl >> 16) & 0xFF) / 255.0;
		i = (float)((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerpOrigin, i, r, g, b);
	}

}

static void
CG_General(Centity *cent)
{
	Refent ent;
	Entstate *s1;

	s1 = &cent->currentState;

	/* if set to invisible, skip */
	if(!s1->modelindex)
		return;

	memset (&ent, 0, sizeof(ent));

	/* set frame */

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	/* player model */
	if(s1->number == cg.snap->ps.clientNum)
		ent.renderfx |= RF_THIRD_PERSON;	/* only draw from mirrors */

	/* convert angles to axis */
	eulertoaxis(cent->lerpAngles, ent.axis);

	/* add to refresh list */
	trap_R_AddRefEntityToScene (&ent);
}

/*
 * Speaker entities can automatically play sounds
 */
static void
CG_Speaker(Centity *cent)
{
	if(!cent->currentState.clientNum)	/* FIXME: use something other than clientNum... */
		return;				/* not auto triggering */

	if(cg.time < cent->miscTime)
		return;

	trap_sndstartsound (NULL, cent->currentState.number, CHAN_ITEM,
		cgs.gameSounds[cent->currentState.eventParm]);

	/* ent->s.frame = ent->wait * 10;
	 * ent->s.clientNum = ent->random * 10; */
	cent->miscTime = cg.time + cent->currentState.frame * 100 +
			 cent->currentState.clientNum * 100 * crandom();
}

static void
CG_Item(Centity *cent)
{
	Refent	ent;
	Entstate *es;
	Gitem		*item;
	int msec;
	float	frac;
	float	scale;
	Weapinfo *wi;

	es = &cent->currentState;
	if(es->modelindex >= bg_numItems)
		CG_Error("Bad item index %i on entity", es->modelindex);

	/* if set to invisible, skip */
	if(!es->modelindex || (es->eFlags & EF_NODRAW))
		return;

	item = &bg_itemlist[ es->modelindex ];
	if(cg_simpleItems.value > 0.001f && item->type != IT_TEAM){
		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		copyv3(cent->lerpOrigin, ent.origin);
		ent.radius = 16 * cg_simpleItems.value;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	/* items bob up and down continuously */
	scale = 0.005 + cent->currentState.number * 0.00001;
	cent->lerpOrigin[2] += 4 + cos((cg.time + 1000) *  scale) * 4;

	memset (&ent, 0, sizeof(ent));

	/* autorotate at one of two speeds */
	if(item->type == IT_HEALTH){
		copyv3(cg.autoAnglesFast, cent->lerpAngles);
		copyaxis(cg.autoAxisFast, ent.axis);
	}else{
		copyv3(cg.autoAngles, cent->lerpAngles);
		copyaxis(cg.autoAxis, ent.axis);
	}

	wi = NULL;
	/* the weapons have their origin where they attatch to player
	 * models, so we need to offset them or they will rotate
	 * eccentricly */
	if(item->type == IT_PRIWEAP){
		wi = &cg_weapons[item->tag];
		cent->lerpOrigin[0] -=
			wi->weaponMidpoint[0] * ent.axis[0][0] +
			wi->weaponMidpoint[1] * ent.axis[1][0] +
			wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -=
			wi->weaponMidpoint[0] * ent.axis[0][1] +
			wi->weaponMidpoint[1] * ent.axis[1][1] +
			wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -=
			wi->weaponMidpoint[0] * ent.axis[0][2] +
			wi->weaponMidpoint[1] * ent.axis[1][2] +
			wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8;	/* an extra height boost */
	}

	if(item->type == IT_PRIWEAP && item->tag == Wrailgun){
		Clientinfo *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
		byte4copy(ci->c1RGBA, ent.shaderRGBA);
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	/* if just respawned, slowly scale up */
	msec = cg.time - cent->miscTime;
	if(msec >= 0 && msec < ITEM_SCALEUP_TIME){
		frac = (float)msec / ITEM_SCALEUP_TIME;
		scalev3(ent.axis[0], frac, ent.axis[0]);
		scalev3(ent.axis[1], frac, ent.axis[1]);
		scalev3(ent.axis[2], frac, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}else
		frac = 1.0;

	/* items without glow textures need to keep a minimum light value
	 * so they are always visible */
	if((item->type == IT_PRIWEAP) ||
	   (item->type == IT_SHIELD))
		ent.renderfx |= RF_MINLIGHT;

	/* increase the size of the weapons when they are presented as items */
	if(item->type == IT_PRIWEAP){
		scalev3(ent.axis[0], 1.5, ent.axis[0]);
		scalev3(ent.axis[1], 1.5, ent.axis[1]);
		scalev3(ent.axis[2], 1.5, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
		trap_sndaddloop(cent->currentState.number,
			cent->lerpOrigin, vec3_origin,
			cgs.media.weaponHoverSound);
	}

	/* add to refresh list */
	trap_R_AddRefEntityToScene(&ent);

	if((item->type == IT_PRIWEAP)
	  && wi->barrelModel)
	then{
		Refent barrel;

		memset(&barrel, 0, sizeof(barrel));

		barrel.hModel = wi->barrelModel;

		copyv3(ent.lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag(&barrel, &ent, wi->weaponModel,
			"tag_barrel");

		copyaxis(ent.axis, barrel.axis);
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene(&barrel);
	}

	/* accompanying rings / spheres for powerups */
	if(!cg_simpleItems.integer){
		Vec3 spinAngles;

		clearv3(spinAngles);

		if(item->type == IT_HEALTH || item->type == IT_POWERUP)
			if((ent.hModel = cg_items[es->modelindex].models[1]) !=
			   0){
				if(item->type == IT_POWERUP){
					ent.origin[2]	+= 12;
					spinAngles[1]	=
						(cg.time &
						 1023) * 360 / -1024.0f;
				}
				eulertoaxis(spinAngles, ent.axis);

				/* scale up if respawning */
				if(frac != 1.0){
					scalev3(ent.axis[0], frac,
						ent.axis[0]);
					scalev3(ent.axis[1], frac,
						ent.axis[1]);
					scalev3(ent.axis[2], frac,
						ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}
	}
}

static void
CG_Missile(Centity *cent)
{
	Refent ent;
	Entstate *s1;
	const Weapinfo *weapon;
/*	int	col; */

	s1 = &cent->currentState;
	if(s1->parentweap >= Wnumweaps)
		s1->parentweap = Wnone;
	weapon = &cg_weapons[s1->parentweap];

	/* calculate the axis */
	copyv3(s1->angles, cent->lerpAngles);

	/* add trails */
	if(weapon->missileTrailFunc)
		weapon->missileTrailFunc(cent, weapon);
	/*
	 *      if ( cent->currentState.modelindex == TEAM_RED ) {
	 *              col = 1;
	 *      }
	 *      else if ( cent->currentState.modelindex == TEAM_BLUE ) {
	 *              col = 2;
	 *      }
	 *      else {
	 *              col = 0;
	 *      }
	 *
	 *      // add dynamic light
	 *      if ( weapon->missileDlight ) {
	 *              trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
	 *                      weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1], weapon->missileDlightColor[col][2] );
	 *      }
	 */
	/* add dynamic light */
	if(weapon->missileDlight)
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
			weapon->missileDlightColor[0],
			weapon->missileDlightColor[1],
			weapon->missileDlightColor[2]);

	/* add missile sound */
	if(weapon->missileSound){
		Vec3 velocity;

		BG_EvaluateTrajectoryDelta(&cent->currentState.traj, cg.time,
			velocity);

		trap_sndaddloop(cent->currentState.number,
			cent->lerpOrigin, velocity,
			weapon->missileSound);
	}

	/* create the render entity */
	memset (&ent, 0, sizeof(ent));
	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(cent->lerpOrigin, ent.oldorigin);

	if(cent->currentState.parentweap == Wplasmagun){
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	/* flicker between two skins */
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	if(cent->currentState.parentweap == Wproxlauncher)
		if(s1->generic1 == TEAM_BLUE)
			ent.hModel = cgs.media.blueProxMine;

	/* convert direction of travel into axis */
	if(norm2v3(s1->traj.delta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	/* spin as it moves */
	if(s1->traj.type != TR_STATIONARY)
		RotateAroundDirection(ent.axis, cg.time / 4);
	else{
		if(s1->parentweap == Wproxlauncher)
			eulertoaxis(cent->lerpAngles, ent.axis);
		else{
			RotateAroundDirection(ent.axis, s1->time);
		}
	}

	/* add to refresh list, possibly with quad glow */
	CG_AddRefEntityWithPowerups(&ent, s1, TEAM_FREE);
}

/*
 * Ignores light, smoke trail, etc.
 */
static void
CG_Inertmissile(Centity *cent)
{
	Refent ent;
	Entstate *s1;
	const Weapinfo *weapon;

	s1 = &cent->currentState;
	if(s1->parentweap >= Wnumweaps)
		s1->parentweap = Wnone;
	weapon = &cg_weapons[s1->parentweap];

	/* calculate the axis */
	copyv3(s1->angles, cent->lerpAngles);

	/* create the render entity */
	memset (&ent, 0, sizeof(ent));
	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(cent->lerpOrigin, ent.oldorigin);

	/* flicker between two skins */
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	/* convert direction of travel into axis */
	if(norm2v3(s1->traj.delta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	/* spin as it moves */
	if(s1->traj.type != TR_STATIONARY)
		RotateAroundDirection(ent.axis, cg.time / 8);

	/* add to refresh list, possibly with quad glow */
	CG_AddRefEntityWithPowerups(&ent, s1, TEAM_FREE);
}

/*
 * This is called when the grapple is sitting up against the wall
 */
static void
CG_Grapple(Centity *cent)
{
	Refent ent;
	Entstate *s1;
	const Weapinfo *weapon;

	s1 = &cent->currentState;
	if(s1->parentweap >= Wnumweaps)
		s1->parentweap = Wnone;
	weapon = &cg_weapons[s1->parentweap];

	/* calculate the axis */
	copyv3(s1->angles, cent->lerpAngles);

	/* FIXME: add grapple pull sound */
	/* add missile sound */
	if(weapon->missileSound)
		trap_sndaddloop(cent->currentState.number,
			cent->lerpOrigin, vec3_origin,
			weapon->missileSound);

	/* Will draw cable if needed */
	CG_GrappleTrail(cent, weapon);

	/* create the render entity */
	memset (&ent, 0, sizeof(ent));
	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(cent->lerpOrigin, ent.oldorigin);

	/* flicker between two skins */
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	/* convert direction of travel into axis */
	if(norm2v3(s1->traj.delta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	trap_R_AddRefEntityToScene(&ent);
}

static void
CG_Mover(Centity *cent)
{
	Refent ent;
	Entstate *s1;

	s1 = &cent->currentState;

	/* create the render entity */
	memset (&ent, 0, sizeof(ent));
	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(cent->lerpOrigin, ent.oldorigin);
	eulertoaxis(cent->lerpAngles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	/* flicker between two skins (FIXME?) */
	ent.skinNum = (cg.time >> 6) & 1;

	/* get the model, either as a bmodel or a modelindex */
	if(s1->solid == SOLID_BMODEL)
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	else
		ent.hModel = cgs.gameModels[s1->modelindex];

	/* add to refresh list */
	trap_R_AddRefEntityToScene(&ent);

	/* add the secondary model */
	if(s1->modelindex2){
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}

}

/*
 * Also called as an event
 */
void
CG_Beam(Centity *cent)
{
	Refent ent;
	Entstate *s1;

	s1 = &cent->currentState;

	/* create the render entity */
	memset (&ent, 0, sizeof(ent));
	copyv3(s1->traj.base, ent.origin);
	copyv3(s1->origin2, ent.oldorigin);
	clearaxis(ent.axis);
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	/* add to refresh list */
	trap_R_AddRefEntityToScene(&ent);
}

static void
CG_Portal(Centity *cent)
{
	Refent ent;
	Entstate *s1;

	s1 = &cent->currentState;

	/* create the render entity */
	memset (&ent, 0, sizeof(ent));
	copyv3(cent->lerpOrigin, ent.origin);
	copyv3(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	perpv3(ent.axis[1], ent.axis[0]);

	/* negating this tends to get the directions like they want
	 * we really should have a camera roll value */
	subv3(vec3_origin, ent.axis[1], ent.axis[1]);

	crossv3(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;				/* rotation speed */
	ent.skinNum = s1->clientNum/256.0 * 360;	/* roll offset */

	/* add to refresh list */
	trap_R_AddRefEntityToScene(&ent);
}

/*
 * Also called by client movement prediction code
 */
void
CG_AdjustPositionForMover(const Vec3 in, int moverNum, int fromTime,
			  int toTime,
			  Vec3 out)
{
	Centity	*cent;
	Vec3		oldOrigin, origin, deltaOrigin;
	Vec3		oldAngles, angles;
	/* Vec3	deltaAngles; */

	if(moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL){
		copyv3(in, out);
		return;
	}

	cent = &cg_entities[ moverNum ];
	if(cent->currentState.eType != ET_MOVER){
		copyv3(in, out);
		return;
	}

	BG_EvaluateTrajectory(&cent->currentState.traj, fromTime, oldOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);

	BG_EvaluateTrajectory(&cent->currentState.traj, toTime, origin);
	BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);

	subv3(origin, oldOrigin, deltaOrigin);
	/* subv3( angles, oldAngles, deltaAngles ); */

	addv3(in, deltaOrigin, out);

	/* FIXME: origin change when on a rotating object */
}

static void
CG_InterpolateEntityPosition(Centity *cent)
{
	Vec3	current, next;
	float	f;

	/* it would be an internal error to find an entity that interpolates without
	 * a snapshot ahead of the current one */
	if(cg.nextSnap == NULL)
		CG_Error("CG_InterpoateEntityPosition: cg.nextSnap == NULL");

	f = cg.frameInterpolation;

	/* this will linearize a sine or parabolic curve, but it is important
	 * to not extrapolate player positions if more recent data is available */
	BG_EvaluateTrajectory(&cent->currentState.traj, cg.snap->serverTime,
		current);
	BG_EvaluateTrajectory(&cent->nextState.traj, cg.nextSnap->serverTime,
		next);

	cent->lerpOrigin[0] = current[0] + f * (next[0] - current[0]);
	cent->lerpOrigin[1] = current[1] + f * (next[1] - current[1]);
	cent->lerpOrigin[2] = current[2] + f * (next[2] - current[2]);

	BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime,
		current);
	BG_EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime,
		next);

	cent->lerpAngles[0] = lerpeuler(current[0], next[0], f);
	cent->lerpAngles[1] = lerpeuler(current[1], next[1], f);
	cent->lerpAngles[2] = lerpeuler(current[2], next[2], f);

}

static void
CG_CalcEntityLerpPositions(Centity *cent)
{

	/* if this player does not want to see extrapolated players */
	if(!cg_smoothClients.integer)
		/* make sure the clients use TR_INTERPOLATE */
		if(cent->currentState.number < MAX_CLIENTS){
			cent->currentState.traj.type = TR_INTERPOLATE;
			cent->nextState.traj.type = TR_INTERPOLATE;
		}

	if(cent->interpolate && cent->currentState.traj.type ==
	   TR_INTERPOLATE){
		CG_InterpolateEntityPosition(cent);
		return;
	}

	/* first see if we can interpolate between two snaps for
	 * linear extrapolated clients */
	if(cent->interpolate && cent->currentState.traj.type ==
	   TR_LINEAR_STOP &&
	   cent->currentState.number < MAX_CLIENTS){
		CG_InterpolateEntityPosition(cent);
		return;
	}

	/* just use the current frame and evaluate as best we can */
	BG_EvaluateTrajectory(&cent->currentState.traj, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time,
		cent->lerpAngles);

	/* adjust for riding a mover if it wasn't rolled into the predicted
	 * player state */
	if(cent != &cg.predictedPlayerEntity)
		CG_AdjustPositionForMover(cent->lerpOrigin,
			cent->currentState.groundEntityNum,
			cg.snap->serverTime, cg.time,
			cent->lerpOrigin);
}

static void
CG_TeamBase(Centity *cent)
{
	Refent model;

	if(cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF){
		/* show the flag base */
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		copyv3(cent->lerpOrigin, model.lightingOrigin);
		copyv3(cent->lerpOrigin, model.origin);
		eulertoaxis(cent->currentState.angles, model.axis);
		if(cent->currentState.modelindex == TEAM_RED)
			model.hModel = cgs.media.redFlagBaseModel;
		else if(cent->currentState.modelindex == TEAM_BLUE)
			model.hModel = cgs.media.blueFlagBaseModel;
		else
			model.hModel = cgs.media.neutralFlagBaseModel;
		trap_R_AddRefEntityToScene(&model);
	}
}

static void
CG_AddCEntity(Centity *cent)
{
	/* event-only entities will have been dealt with already */
	if(cent->currentState.eType >= ET_EVENTS)
		return;

	/* calculate the current origin */
	CG_CalcEntityLerpPositions(cent);

	/* add automatic effects */
	CG_EntityEffects(cent);

	switch(cent->currentState.eType){
	default:
		CG_Error("Bad entity type: %i\n", cent->currentState.eType);
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General(cent);
		break;
	case ET_PLAYER:
		CG_Player(cent);
		break;
	case ET_ITEM:
		CG_Item(cent);
		break;
	case ET_INERTMISSILE:
		CG_Inertmissile(cent);
		break;
	case ET_MISSILE:
		CG_Missile(cent);
		break;
	case ET_MOVER:
		CG_Mover(cent);
		break;
	case ET_BEAM:
		CG_Beam(cent);
		break;
	case ET_PORTAL:
		CG_Portal(cent);
		break;
	case ET_SPEAKER:
		CG_Speaker(cent);
		break;
	case ET_GRAPPLE:
		CG_Grapple(cent);
		break;
	case ET_TEAM:
		CG_TeamBase(cent);
		break;
	}
}

void
CG_AddPacketEntities(void)
{
	int num;
	Centity *cent;
	Playerstate *ps;

	/* set cg.frameInterpolation */
	if(cg.nextSnap){
		int delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if(delta == 0)
			cg.frameInterpolation = 0;
		else
			cg.frameInterpolation =
				(float)(cg.time - cg.snap->serverTime) / delta;
	}else
		cg.frameInterpolation = 0;	/* actually, it should never be used, because */
	/* no entities should be marked as interpolating */

	/* the auto-rotating items will all have the same axis */
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 2047) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	eulertoaxis(cg.autoAngles, cg.autoAxis);
	eulertoaxis(cg.autoAnglesFast, cg.autoAxisFast);

	/* generate and add the entity from the playerstate */
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState(ps, &cg.predictedPlayerEntity.currentState,
		qfalse);
	CG_AddCEntity(&cg.predictedPlayerEntity);

	/* lerp the non-predicted value for lightning gun origins */
	CG_CalcEntityLerpPositions(&cg_entities[ cg.snap->ps.clientNum ]);

	/* add each entity sent over by the server */
	for(num = 0; num < cg.snap->numEntities; num++){
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity(cent);
	}
}
