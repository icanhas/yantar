/* events and effects dealing with weapons */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "local.h"

static void
CG_MachineGunEjectBrass(centity_t *cent)
{
	localEntity_t	*le;
	refEntity_t	*re;
	Vec3	velocity, xvelocity;
	Vec3	offset, xoffset;
	float	waterScale = 1.0f;
	Vec3	v[3];

	if(cg_brassTime.integer <= 0)
		return;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime	= cg.time;
	le->endTime	= le->startTime + cg_brassTime.integer +
			  (cg_brassTime.integer / 4) * random();

	le->pos.trType	= TR_GRAVITY;
	le->pos.trTime	= cg.time - (rand()&15);

	eulertoaxis(cent->lerpAngles, v);

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] +
		     offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] +
		     offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] +
		     offset[2] * v[2][2];
	addv3(cent->lerpOrigin, xoffset, re->origin);

	copyv3(re->origin, le->pos.trBase);

	if(CG_PointContents(re->origin, -1) & CONTENTS_WATER)
		waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] +
		       velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] +
		       velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] +
		       velocity[2] * v[2][2];
	scalev3(xvelocity, waterScale, le->pos.trDelta);

	copyaxis(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0]	= 2;
	le->angles.trDelta[1]	= 1;
	le->angles.trDelta[2]	= 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

static void
CG_ShotgunEjectBrass(centity_t *cent)
{
	localEntity_t	*le;
	refEntity_t	*re;
	Vec3	velocity, xvelocity;
	Vec3	offset, xoffset;
	Vec3	v[3];
	int	i;

	if(cg_brassTime.integer <= 0)
		return;

	for(i = 0; i < 2; i++){
		float waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if(i == 0)
			velocity[1] = 40 + 10 * crandom();
		else
			velocity[1] = -40 + 10 * crandom();
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime	= cg.time;
		le->endTime	= le->startTime + cg_brassTime.integer*3 +
				  cg_brassTime.integer*random();

		le->pos.trType	= TR_GRAVITY;
		le->pos.trTime	= cg.time;

		eulertoaxis(cent->lerpAngles, v);

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] +
			     offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] +
			     offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] +
			     offset[2] * v[2][2];
		addv3(cent->lerpOrigin, xoffset, re->origin);
		copyv3(re->origin, le->pos.trBase);
		if(CG_PointContents(re->origin, -1) & CONTENTS_WATER)
			waterScale = 0.10f;

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] *
			       v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] *
			       v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] *
			       v[1][2] + velocity[2] * v[2][2];
		scalev3(xvelocity, waterScale, le->pos.trDelta);

		copyaxis(axisDefault, re->axis);
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0]	= 1;
		le->angles.trDelta[1]	= 0.5;
		le->angles.trDelta[2]	= 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}

static void
CG_NailgunEjectBrass(centity_t *cent)
{
	localEntity_t *smoke;
	Vec3	origin;
	Vec3	v[3];
	Vec3	offset;
	Vec3	xoffset;
	Vec3	up;

	eulertoaxis(cent->lerpAngles, v);

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] +
		     offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] +
		     offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] +
		     offset[2] * v[2][2];
	addv3(cent->lerpOrigin, xoffset, origin);

	setv3(up, 0, 0, 64);

	smoke = CG_SmokePuff(origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0,
		cgs.media.smokePuffShader);
	/* use the optimized local entity add */
	smoke->leType = LE_SCALE_FADE;
}

#define RADIUS		4
#define ROTATION	1
#define SPACING		5
void
CG_RailTrail(clientInfo_t *ci, Vec3 start, Vec3 end)
{
	Vec3	axis[36], move, move2, vec, temp;
	float	len;
	int	i, j, skip;

	localEntity_t *le;
	refEntity_t *re;

	start[2] -= 4;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime	= cg.time;
	le->endTime	= cg.time + cg_railTrailTime.value;
	le->lifeRate	= 1.0 / (le->endTime - le->startTime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	copyv3(start, re->origin);
	copyv3(end, re->oldorigin);

	re->shaderRGBA[0] = ci->color1[0] * 255;
	re->shaderRGBA[1] = ci->color1[1] * 255;
	re->shaderRGBA[2] = ci->color1[2] * 255;
	re->shaderRGBA[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	clearaxis(re->axis);

	if(cg_oldRail.integer){
		/* nudge down a bit so it isn't exactly in center */
		re->origin[2] -= 8;
		re->oldorigin[2] -= 8;
		return;
	}

	copyv3 (start, move);
	subv3 (end, start, vec);
	len = normv3 (vec);
	perpv3(temp, vec);
	for(i = 0; i < 36; i++)
		RotatePointAroundVector(axis[i], vec, temp, i * 10);	/* banshee 2.4 was 10 */

	maddv3(move, 20, vec, move);
	scalev3 (vec, SPACING, vec);

	skip = -1;

	j = 18;
	for(i = 0; i < len; i += SPACING){
		if(i != skip){
			skip = i + SPACING;
			le = CG_AllocLocalEntity();
			re = &le->refEntity;
			le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
			le->startTime	= cg.time;
			le->endTime	= cg.time + (i>>1) + 600;
			le->lifeRate	= 1.0 / (le->endTime - le->startTime);

			re->shaderTime = cg.time / 1000.0f;
			re->reType	= RT_SPRITE;
			re->radius	= 1.1f;
			re->customShader = cgs.media.railRingsShader;

			re->shaderRGBA[0] = ci->color2[0] * 255;
			re->shaderRGBA[1] = ci->color2[1] * 255;
			re->shaderRGBA[2] = ci->color2[2] * 255;
			re->shaderRGBA[3] = 255;

			le->color[0] = ci->color2[0] * 0.75;
			le->color[1] = ci->color2[1] * 0.75;
			le->color[2] = ci->color2[2] * 0.75;
			le->color[3] = 1.0f;

			le->pos.trType	= TR_LINEAR;
			le->pos.trTime	= cg.time;

			copyv3(move, move2);
			maddv3(move2, RADIUS, axis[j], move2);
			copyv3(move2, le->pos.trBase);

			le->pos.trDelta[0] = axis[j][0]*6;
			le->pos.trDelta[1] = axis[j][1]*6;
			le->pos.trDelta[2] = axis[j][2]*6;
		}

		addv3 (move, vec, move);

		j = (j + ROTATION) % 36;
	}
}

static void
CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int	step;
	Vec3 origin, lastPos;
	int	t;
	int	startTime, contents;
	int	lastContents;
	entityState_t	*es;
	Vec3 up;
	localEntity_t	*smoke;

	if(cg_noProjectileTrail.integer)
		return;

	up[0]	= 0;
	up[1]	= 0;
	up[2]	= 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);

	/* if object (e.g. grenade) is stationary, don't toss up smoke */
	if(es->pos.trType == TR_STATIONARY){
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		if(contents & lastContents & CONTENTS_WATER)
			CG_BubbleTrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailTime; t += step){
		BG_EvaluateTrajectory(&es->pos, t, lastPos);

		smoke = CG_SmokePuff(lastPos, up,
			wi->trailRadius,
			1, 1, 1, 0.33f,
			wi->wiTrailTime,
			t,
			0,
			0,
			cgs.media.smokePuffShader);
		/* use the optimized local entity add */
		smoke->leType = LE_SCALE_FADE;
	}

}

static void
CG_NailTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int	step;
	Vec3 origin, lastPos;
	int	t;
	int	startTime, contents;
	int	lastContents;
	entityState_t	*es;
	Vec3 up;
	localEntity_t	*smoke;

	if(cg_noProjectileTrail.integer)
		return;

	up[0]	= 0;
	up[1]	= 0;
	up[2]	= 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);

	/* if object (e.g. grenade) is stationary, don't toss up smoke */
	if(es->pos.trType == TR_STATIONARY){
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		if(contents & lastContents & CONTENTS_WATER)
			CG_BubbleTrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailTime; t += step){
		BG_EvaluateTrajectory(&es->pos, t, lastPos);

		smoke = CG_SmokePuff(lastPos, up,
			wi->trailRadius,
			1, 1, 1, 0.33f,
			wi->wiTrailTime,
			t,
			0,
			0,
			cgs.media.nailPuffShader);
		/* use the optimized local entity add */
		smoke->leType = LE_SCALE_FADE;
	}

}

static void
CG_PlasmaTrail(centity_t *cent, const weaponInfo_t *wi)
{
	localEntity_t	*le;
	refEntity_t	*re;
	entityState_t	*es;
	Vec3	velocity, xvelocity, origin;
	Vec3	offset, xoffset;
	Vec3	v[3];

	float	waterScale = 1.0f;

	if(cg_noProjectileTrail.integer || cg_oldPlasma.integer)
		return;

	es = &cent->currentState;

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	le->startTime	= cg.time;
	le->endTime	= le->startTime + 600;

	le->pos.trType	= TR_GRAVITY;
	le->pos.trTime	= cg.time;

	eulertoaxis(cent->lerpAngles, v);

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] +
		     offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] +
		     offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] +
		     offset[2] * v[2][2];

	addv3(origin, xoffset, re->origin);
	copyv3(re->origin, le->pos.trBase);

	if(CG_PointContents(re->origin, -1) & CONTENTS_WATER)
		waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] +
		       velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] +
		       velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] +
		       velocity[2] * v[2][2];
	scalev3(xvelocity, waterScale, le->pos.trDelta);

	copyaxis(axisDefault, re->axis);
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_SPRITE;
	re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bounceFactor = 0.3f;

	re->shaderRGBA[0] = wi->flashDlightColor[0] * 63;
	re->shaderRGBA[1] = wi->flashDlightColor[1] * 63;
	re->shaderRGBA[2] = wi->flashDlightColor[2] * 63;
	re->shaderRGBA[3] = 63;

	le->color[0] = wi->flashDlightColor[0] * 0.2;
	le->color[1] = wi->flashDlightColor[1] * 0.2;
	le->color[2] = wi->flashDlightColor[2] * 0.2;
	le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0]	= 1;
	le->angles.trDelta[1]	= 0.5;
	le->angles.trDelta[2]	= 0;

}

void
CG_GrappleTrail(centity_t *ent, const weaponInfo_t *wi)
{
	Vec3	origin;
	entityState_t   *es;
	Vec3	forward, up;
	refEntity_t beam;

	UNUSED(wi);
	es = &ent->currentState;

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&beam, 0, sizeof(beam));
	/* FIXME adjust for muzzle position */
	copyv3 (cg_entities[ent->currentState.otherEntityNum].lerpOrigin,
		beam.origin);
	beam.origin[2] += 26;
	anglev3s(cg_entities[ent->currentState.otherEntityNum].lerpAngles,
		forward, NULL,
		up);
	maddv3(beam.origin, -6, up, beam.origin);
	copyv3(origin, beam.oldorigin);

	if(distv3(beam.origin, beam.oldorigin) < 64)
		return;		/* Don't draw if close */

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;

	clearaxis(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene(&beam);
}

static void
CG_GrenadeTrail(centity_t *ent, const weaponInfo_t *wi)
{
	CG_RocketTrail(ent, wi);
}

/* The server says this item is used on this level */
void
CG_RegisterWeapon(int weaponNum)
{
	weaponInfo_t *weaponInfo;
	gitem_t *item, *ammo;
	char	path[MAX_QPATH];
	Vec3	mins, maxs;
	int i;

	weaponInfo = &cg_weapons[weaponNum];

	if(weaponNum == 0)
		return;
	if(weaponInfo->registered)
		return;

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;
	for(item = bg_itemlist + 1; item->classname; item++)
		if(item->giType == IT_PRIWEAP && item->giTag == weaponNum){
			weaponInfo->item = item;
			break;
		}
	if(!item->classname)
		CG_Error("Couldn't find weapon %i", weaponNum);
	CG_RegisterItemVisuals(item - bg_itemlist);

	/* load cmodel before model so filecache works */
	weaponInfo->weaponModel = trap_R_RegisterModel(item->world_model[0]);

	/* calc midpoint for rotation */
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for(i = 0; i < 3; i++)
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 *
						(maxs[i] - mins[i]);

	weaponInfo->weaponIcon	= trap_R_RegisterShader(item->icon);
	weaponInfo->ammoIcon	= trap_R_RegisterShader(item->icon);

	for(ammo = bg_itemlist + 1; ammo->classname; ammo++)
		if(ammo->giType == IT_AMMO && ammo->giTag == weaponNum)
			break;
	if(ammo->classname && ammo->world_model[0])
		weaponInfo->ammoModel = trap_R_RegisterModel(
			ammo->world_model[0]);

	strcpy(path, item->world_model[0]);
	Q_stripext(path, path, sizeof(path));
	strcat(path, "_flash");
	weaponInfo->flashModel = trap_R_RegisterModel(path);

	strcpy(path, item->world_model[0]);
	Q_stripext(path, path, sizeof(path));
	strcat(path, "_barrel");
	weaponInfo->barrelModel = trap_R_RegisterModel(path);

	strcpy(path, item->world_model[0]);
	Q_stripext(path, path, sizeof(path));
	strcat(path, "_hand");
	weaponInfo->handsModel = trap_R_RegisterModel(path);

	if(!weaponInfo->handsModel)
		weaponInfo->handsModel = trap_R_RegisterModel(
			Pshotgunmodels "/shotgun_hand");

	switch(weaponNum){
	case W1melee:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->firingSound = trap_S_RegisterSound(
			Pmeleesounds "/fstrun", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pmeleesounds "/fstatck", qfalse);
		break;
	case W1lightning:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->readySound = trap_S_RegisterSound(
			Pmeleesounds "/fsthum", qfalse);
		weaponInfo->firingSound = trap_S_RegisterSound(
			Plgsounds "/lg_hum", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Plgsounds "/lg_fire", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader(
			"lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel(
			Pweaphitmodels "/crackle");
		cgs.media.sfx_lghit1 = trap_S_RegisterSound(
			Plgsounds "/lg_hit", qfalse);
		cgs.media.sfx_lghit2 = trap_S_RegisterSound(
			Plgsounds "/lg_hit2", qfalse);
		cgs.media.sfx_lghit3 = trap_S_RegisterSound(
			Plgsounds "/lg_hit3", qfalse);
		break;
	case W1_GRAPPLING_HOOK:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->missileModel = trap_R_RegisterModel(
			Prlmodels "/rocket");
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 200;
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
		weaponInfo->readySound = trap_S_RegisterSound(
			Pmeleesounds "/fsthum", qfalse);
		weaponInfo->firingSound = trap_S_RegisterSound(
			Pmeleesounds "/fstrun", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader(
			"lightningBoltNew");
		break;
	case W1chaingun:
		weaponInfo->firingSound = trap_S_RegisterSound(
			Pgattlingsounds "/wvulfire", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pgattlingsounds "/vulcanf1b", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound(
			Pgattlingsounds "/vulcanf2b", qfalse);
		weaponInfo->flashSound[2] = trap_S_RegisterSound(
			Pgattlingsounds "/vulcanf3b", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound(
			Pgattlingsounds "/vulcanf4b", qfalse);
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader(
			"bulletExplosion");
		break;
	case W1machinegun:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pmgsounds "/machgf1b", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound(
			Pmgsounds "/machgf2b", qfalse);
		weaponInfo->flashSound[2] = trap_S_RegisterSound(
			Pmgsounds "/machgf3b", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound(
			Pmgsounds "/machgf4b", qfalse);
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader(
			"bulletExplosion");
		break;
	case W1shotgun:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pshotgunsounds "/sshotf1b", qfalse);
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;
	case W2rocketlauncher:
		weaponInfo->missileModel = trap_R_RegisterModel(
			Prlmodels "/rocket");
		weaponInfo->missileSound = trap_S_RegisterSound(
			Prlsounds "/rockfly", qfalse);
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight	= 200;
		weaponInfo->wiTrailTime		= 4000;
		weaponInfo->trailRadius		= 64;
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Prlsounds "/rocklf1a", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		break;
	case W2proxlauncher:
		weaponInfo->missileModel = trap_R_RegisterModel(
			Pweaphitmodels "/proxmine");
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pproxsounds "/wstbfire", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader(
			"grenadeExplosion");
		break;
	case W2grenadelauncher:
		weaponInfo->missileModel = trap_R_RegisterModel(
			Pammomodels "/grenade1");
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pgrenadesounds "/grenlf1a", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader(
			"grenadeExplosion");
		break;
	case W1nailgun:
		weaponInfo->ejectBrassFunc = CG_NailgunEjectBrass;
		weaponInfo->missileTrailFunc = CG_NailTrail;
/*		weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/nailgun/wnalflit", qfalse ); */
		weaponInfo->trailRadius = 16;
		weaponInfo->wiTrailTime = 250;
		weaponInfo->missileModel = trap_R_RegisterModel(
			Pweaphitmodels "/nail");
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			"sound/weapons/nailgun/wnalfire", qfalse);
		break;
	case W1plasmagun:
/*		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel; */
		weaponInfo->missileTrailFunc = CG_PlasmaTrail;
		weaponInfo->missileSound = trap_S_RegisterSound(
			Pplasmasounds "/lasfly", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pplasmasounds "/hyprbf1a", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader(
			"plasmaExplosion");
		cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
		break;
	case W1railgun:
		weaponInfo->readySound = trap_S_RegisterSound(
			Prailsounds "/rg_hum", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Prailsounds "/railgf1a", qfalse);
		cgs.media.railExplosionShader = trap_R_RegisterShader(
			"railExplosion");
		cgs.media.railRingsShader = trap_R_RegisterShader(
			"railDisc");
		cgs.media.railCoreShader = trap_R_RegisterShader(
			"railCore");
		break;
	case W2bfg:
		weaponInfo->readySound = trap_S_RegisterSound(
			Pbfgsounds "/bfg_hum", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Pbfgsounds "/bfg_fire", qfalse);
		cgs.media.bfgExplosionShader = trap_R_RegisterShader(
			"bfgExplosion");
		weaponInfo->missileModel = trap_R_RegisterModel(
			Pweaphitmodels "/bfg");
		weaponInfo->missileSound = trap_S_RegisterSound(
			Prlsounds "/rockfly", qfalse);
		break;
	default:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound(
			Prlsounds "/rocklf1a", qfalse);
		break;
	}
}

/* The server says this item is used on this level */
void
CG_RegisterItemVisuals(int itemNum)
{
	itemInfo_t	*itemInfo;
	gitem_t		*item;

	if(itemNum < 0 || itemNum >= bg_numItems)
		CG_Error(
			"CG_RegisterItemVisuals: itemNum %d out of range [0-%d]",
			itemNum,
			bg_numItems-1);

	itemInfo = &cg_items[itemNum];
	if(itemInfo->registered)
		return;

	item = &bg_itemlist[itemNum];
	memset(itemInfo, 0, sizeof(*itemInfo));
	itemInfo->registered = qtrue;
	itemInfo->models[0] = trap_R_RegisterModel(item->world_model[0]);
	itemInfo->icon = trap_R_RegisterShader(item->icon);
	if(item->giType == IT_PRIWEAP)
		CG_RegisterWeapon(item->giTag);

	/* powerups have an accompanying ring or sphere */
	if(item->giType == IT_POWERUP || item->giType == IT_HEALTH ||
	   item->giType == IT_SHIELD || item->giType == IT_HOLDABLE)
		if(item->world_model[1])
			itemInfo->models[1] = trap_R_RegisterModel(item->world_model[1]);
}

/*
 * View weapon
 */

static int
CG_MapTorsoToWeaponFrame(clientInfo_t *ci, int frame)
{
	/* change weapon */
	if(frame >= ci->animations[TORSO_DROP].firstFrame
	   && frame < ci->animations[TORSO_DROP].firstFrame + 9)
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;

	/* stand attack */
	if(frame >= ci->animations[TORSO_ATTACK].firstFrame
	   && frame < ci->animations[TORSO_ATTACK].firstFrame + 6)
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;

	/* stand attack 2 */
	if(frame >= ci->animations[TORSO_ATTACK2].firstFrame
	   && frame < ci->animations[TORSO_ATTACK2].firstFrame + 6)
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	return 0;
}

static void
CG_CalculateWeaponPosition(Vec3 origin, Vec3 angles)
{
	float	scale;
	int	delta;
	float	fracsin;

	copyv3(cg.refdef.vieworg, origin);
	copyv3(cg.refdefViewAngles, angles);

	/* on odd legs, invert some angles */
	if(cg.bobcycle & 1)
		scale = -cg.xyspeed;
	else
		scale = cg.xyspeed;

	/* gun angles from bobbing */
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	/* drop the weapon when landing */
	delta = cg.time - cg.landTime;
	if(delta < LAND_DEFLECT_TIME)
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
		origin[2] += cg.landChange*0.25 *
			     (LAND_DEFLECT_TIME + LAND_RETURN_TIME -
			      delta) / LAND_RETURN_TIME;
#if 0
	/* drop the weapon when stair climbing */
	delta = cg.time - cg.stepTime;
	if(delta < STEP_TIME/2)
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	else if(delta < STEP_TIME)
		origin[2] -= cg.stepChange*0.25 *
			     (STEP_TIME - delta) / (STEP_TIME/2);

#endif
	/* idle drift */
	scale	= cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/*
 * Origin will be the exact tag point, which is slightly
 * different than the muzzle point used for determining hits.
 * The cent should be the non-predicted cent if it is from the player,
 * so the endpoint will reflect the simulated strike (lagging the predicted
 * angle)
 */
static void
CG_LightningBolt(centity_t *cent, Vec3 origin)
{
	trace_t trace;
	refEntity_t beam;
	Vec3	forward;
	Vec3	muzzlePoint, endPoint;
	int anim;

	if(cent->currentState.weapon != W1lightning)
		return;

	memset(&beam, 0, sizeof(beam));

	/* CPMA  "true" lightning */
	if((cent->currentState.number == cg.predictedPlayerState.clientNum) &&
	   (cg_trueLightning.value != 0)){
		Vec3	angle;
		int	i;

		for(i = 0; i < 3; i++){
			float a = cent->lerpAngles[i] - cg.refdefViewAngles[i];
			if(a > 180)
				a -= 360;
			if(a < -180)
				a += 360;
			angle[i] = cg.refdefViewAngles[i] + a *
				   (1.0 - cg_trueLightning.value);
			if(angle[i] < 0)
				angle[i] += 360;
			if(angle[i] > 360)
				angle[i] -= 360;
		}

		anglev3s(angle, forward, NULL, NULL);
		copyv3(cent->lerpOrigin, muzzlePoint);
/*		copyv3(cg.refdef.vieworg, muzzlePoint ); */
	}else{
		/* !CPMA */
		anglev3s(cent->lerpAngles, forward, NULL, NULL);
		copyv3(cent->lerpOrigin, muzzlePoint);
	}

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		muzzlePoint[2] += CROUCH_VIEWHEIGHT;
	else
		muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	maddv3(muzzlePoint, 14, forward, muzzlePoint);

	/* project forward by the lightning range */
	maddv3(muzzlePoint, LIGHTNING_RANGE, forward, endPoint);

	/* see if it hit a wall */
	CG_Trace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		cent->currentState.number, MASK_SHOT);

	/* this is the endpoint */
	copyv3(trace.endpos, beam.oldorigin);

	/* use the provided origin, even though it may be slightly
	 * different than the muzzle origin */
	copyv3(origin, beam.origin);

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene(&beam);

	/* add the impact flare if it hit something */
	if(trace.fraction < 1.0){
		Vec3	angles;
		Vec3	dir;

		subv3(beam.oldorigin, beam.origin, dir);
		normv3(dir);

		memset(&beam, 0, sizeof(beam));
		beam.hModel = cgs.media.lightningExplosionModel;

		maddv3(trace.endpos, -16, dir, beam.origin);

		/* make a random orientation */
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		eulertoaxis(angles, beam.axis);
		trap_R_AddRefEntityToScene(&beam);
	}
}
/*
 *
 * static void CG_LightningBolt( centity_t *cent, Vec3 origin ) {
 *      trace_t		trace;
 *      refEntity_t		beam;
 *      Vec3			forward;
 *      Vec3			muzzlePoint, endPoint;
 *
 *      if ( cent->currentState.weapon != W1lightning ) {
 *              return;
 *      }
 *
 *      memset( &beam, 0, sizeof( beam ) );
 *
 *      // find muzzle point for this frame
 *      copyv3( cent->lerpOrigin, muzzlePoint );
 *      anglev3s( cent->lerpAngles, forward, NULL, NULL );
 *
 *      // FIXME: crouch
 *      muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
 *
 *      maddv3( muzzlePoint, 14, forward, muzzlePoint );
 *
 *      // project forward by the lightning range
 *      maddv3( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );
 *
 *      // see if it hit a wall
 *      CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
 *              cent->currentState.number, MASK_SHOT );
 *
 *      // this is the endpoint
 *      copyv3( trace.endpos, beam.oldorigin );
 *
 *      // use the provided origin, even though it may be slightly
 *      // different than the muzzle origin
 *      copyv3( origin, beam.origin );
 *
 *      beam.reType = RT_LIGHTNING;
 *      beam.customShader = cgs.media.lightningShader;
 *      trap_R_AddRefEntityToScene( &beam );
 *
 *      // add the impact flare if it hit something
 *      if ( trace.fraction < 1.0 ) {
 *              Vec3	angles;
 *              Vec3	dir;
 *
 *              subv3( beam.oldorigin, beam.origin, dir );
 *              normv3( dir );
 *
 *              memset( &beam, 0, sizeof( beam ) );
 *              beam.hModel = cgs.media.lightningExplosionModel;
 *
 *              maddv3( trace.endpos, -16, dir, beam.origin );
 *
 *              // make a random orientation
 *              angles[0] = rand() % 360;
 *              angles[1] = rand() % 360;
 *              angles[2] = rand() % 360;
 *              eulertoaxis( angles, beam.axis );
 *              trap_R_AddRefEntityToScene( &beam );
 *      }
 * }
 */

#define         SPIN_SPEED	0.9
#define         COAST_TIME	1000
static float
CG_MachinegunSpinAngle(centity_t *cent)
{
	int delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if(cent->pe.barrelSpinning)
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	else{
		if(delta > COAST_TIME)
			delta = COAST_TIME;

		speed = 0.5 *
			(SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if(cent->pe.barrelSpinning ==
	   !(cent->currentState.eFlags & EF_FIRING)){
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = modeuler(angle);
		cent->pe.barrelSpinning =
			!!(cent->currentState.eFlags & EF_FIRING);
		if(cent->currentState.weapon == W1chaingun &&
		   !cent->pe.barrelSpinning)
			trap_S_StartSound(
				NULL, cent->currentState.number, CHAN_WEAPON,
				trap_S_RegisterSound(
					Pgattlingsounds "/wvulwind",
					qfalse));
	}
	return angle;
}

static void
CG_AddWeaponWithPowerups(refEntity_t *gun, int powerups)
{
	/* add powerup effects */
	if(powerups & (1 << PW_INVIS)){
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene(gun);
	}else{
		trap_R_AddRefEntityToScene(gun);

		if(powerups & (1 << PW_BATTLESUIT)){
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene(gun);
		}
		if(powerups & (1 << PW_QUAD)){
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene(gun);
		}
	}
}

/*
 * Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
 * The main player will have this called for BOTH cases, so effects like light and
 * sound should only be done on the world model case.
 */
void
CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent,
		   int team)
{
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	Vec3 angles;
	Weapon	weaponNum;
	weaponInfo_t	*weapon;
	centity_t       *nonPredictedCent;
	orientation_t	lerped;

	UNUSED(team);
	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon(weaponNum);
	weapon = &cg_weapons[weaponNum];

	/* add the weapon */
	memset(&gun, 0, sizeof(gun));
	copyv3(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	/* set custom shading for railgun refire rate */
	if(weaponNum == W1railgun){
		clientInfo_t *ci = &cgs.clientinfo[cent->currentState.clientNum];
		if(cent->pe.railFireTime + 1500 > cg.time){
			int scale = 255 *
				    (cg.time - cent->pe.railFireTime) / 1500;
			gun.shaderRGBA[0] = (ci->c1RGBA[0] * scale) >> 8;
			gun.shaderRGBA[1] = (ci->c1RGBA[1] * scale) >> 8;
			gun.shaderRGBA[2] = (ci->c1RGBA[2] * scale) >> 8;
			gun.shaderRGBA[3] = 255;
		}else
			byte4copy(ci->c1RGBA, gun.shaderRGBA);
	}

	gun.hModel = weapon->weaponModel;
	if(!gun.hModel)
		return;

	if(!ps){
		/* add weapon ready sound */
		cent->pe.lightningFiring = qfalse;
		if((cent->currentState.eFlags & EF_FIRING) &&
		   weapon->firingSound){
			/* lightning gun and guantlet make a different sound when fire is held down */
			trap_S_AddLoopingSound(cent->currentState.number,
				cent->lerpOrigin, vec3_origin,
				weapon->firingSound);
			cent->pe.lightningFiring = qtrue;
		}else if(weapon->readySound)
			trap_S_AddLoopingSound(cent->currentState.number,
				cent->lerpOrigin, vec3_origin,
				weapon->readySound);
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, "tag_weapon");
	copyv3(parent->origin, gun.origin);

	maddv3(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	/* Make weapon appear left-handed for 2 and centered for 3 */
	if(ps && cg_drawGun.integer == 2)
		maddv3(gun.origin, -lerped.origin[1], parent->axis[1],
			gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
		maddv3(gun.origin, lerped.origin[1], parent->axis[1],
			gun.origin);

	maddv3(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);
	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups);

	/* add the spinning barrel */
	if(weapon->barrelModel){
		memset(&barrel, 0, sizeof(barrel));
		copyv3(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel	= weapon->barrelModel;
		angles[YAW]	= 0;
		angles[PITCH]	= 0;
		angles[ROLL]	= CG_MachinegunSpinAngle(cent);
		eulertoaxis(angles, barrel.axis);

		CG_PositionRotatedEntityOnTag(&barrel, &gun, weapon->weaponModel,
			"tag_barrel");

		CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
	}

	/* make sure we aren't looking at cg.predictedPlayerEntity for LG */
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];
	/* 
	 * if the index of the nonPredictedCent is not the same as the clientNum
	 * then this is a fake player (like on the single player podiums), so
	 * go ahead and use the cent 
	 */
	if((nonPredictedCent - cg_entities) != cent->currentState.clientNum)
		nonPredictedCent = cent;

	/* add the flash */
	if((weaponNum == W1lightning || weaponNum == W1melee ||
	    weaponNum == W1_GRAPPLING_HOOK)
	   && (nonPredictedCent->currentState.eFlags & EF_FIRING)){
		/* continuous flash */
	}else
	/* impulse flash */
	if(cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME)
		return;


	memset(&flash, 0, sizeof(flash));
	copyv3(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if(!flash.hModel)
		return;
	angles[YAW] = 0;
	angles[PITCH]	= 0;
	angles[ROLL]	= crandom() * 10;
	eulertoaxis(angles, flash.axis);

	/* colorize the railgun blast */
	if(weaponNum == W1railgun){
		clientInfo_t *ci;

		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel,
		"tag_flash");
	trap_R_AddRefEntityToScene(&flash);

	if(ps || cg.renderingThirdPerson ||
	   cent->currentState.number != cg.predictedPlayerState.clientNum){
		/* add lightning bolt */
		CG_LightningBolt(nonPredictedCent, flash.origin);

		if(weapon->flashDlightColor[0] ||
		   weapon->flashDlightColor[1] || weapon->flashDlightColor[2])
			trap_R_AddLightToScene(flash.origin,
				300 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1],
				weapon->flashDlightColor[2]);
	}
}

/* Add the weapon, and flash for the player's view */
void
CG_AddViewWeapon(playerState_t *ps)
{
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t *ci;
	float	fovOffset;
	Vec3	angles;
	weaponInfo_t *weapon;

	if(ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(ps->pm_type == PM_INTERMISSION)
		return;

	/* no gun if in third person view or a camera is active
	 * if ( cg.renderingThirdPerson || cg.cameraMode) { */
	if(cg.renderingThirdPerson)
		return;

	/* allow the gun to be completely removed */
	if(!cg_drawGun.integer){
		Vec3 origin;

		if(cg.predictedPlayerState.eFlags & EF_FIRING){
			/* special hack for lightning gun... */
			copyv3(cg.refdef.vieworg, origin);
			maddv3(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	/* don't draw if testing a gun model */
	if(cg.testGun)
		return;

	/* drop gun lower at higher fov */
	if(cg_fov.integer > 90)
		fovOffset = -0.2 * (cg_fov.integer - 90);
	else
		fovOffset = 0;

	cent = &cg.predictedPlayerEntity;	/* &cg_entities[cg.snap->ps.clientNum]; */
	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	/* set up gun position */
	CG_CalculateWeaponPosition(hand.origin, angles);

	maddv3(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	maddv3(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	maddv3(hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2],
		hand.origin);

	eulertoaxis(angles, hand.axis);

	/* map torso animations to weapon animations */
	if(cg_gun_frame.integer){
		/* development tool */
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}else{
		/* get clientinfo for animation map */
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
		hand.oldframe = CG_MapTorsoToWeaponFrame(
			ci, cent->pe.torso.oldFrame);
		hand.backlerp = cent->pe.torso.backlerp;
	}
	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;
	/* add everything onto the hand */
	CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity,
		ps->persistant[PERS_TEAM]);
}

/*
 * Weapon selection
 */

void
CG_DrawWeaponSelect(void)
{
	int	i;
	int	bits;
	int	count;
	int	x, y, w;
	char *name;
	float *color;

	/* don't display if dead */
	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
		return;

	color = CG_FadeColor(cg.weaponSelectTime, WEAPON_SELECT_TIME);
	if(!color)
		return;
	trap_R_SetColor(color);

	/* showing weapon select clears pickup item display, but not the blend blob */
	cg.itemPickupTime = 0;

	/* count the number of weapons owned */
	bits = cg.snap->ps.stats[STAT_PRIWEAPS];
	count = 0;
	for(i = 1; i < MAX_WEAPONS; i++)
		if(bits & (1 << i))
			count++;

	x = 320 - count * 20;
	y = 380;

	for(i = 1; i < MAX_WEAPONS; i++){
		if(!(bits & (1 << i)))
			continue;

		CG_RegisterWeapon(i);

		/* draw weapon icon */
		CG_DrawPic(x, y, 32, 32, cg_weapons[i].weaponIcon);

		/* draw selection marker */
		if(i == cg.weaponSelect)
			CG_DrawPic(x-4, y-4, 40, 40, cgs.media.selectShader);

		/* no ammo cross on top */
		if(!cg.snap->ps.ammo[ i ])
			CG_DrawPic(x, y, 32, 32, cgs.media.noammoShader);

		x += 40;
	}

	/* draw the selected name */
	if(cg_weapons[ cg.weaponSelect ].item){
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if(name){
			w = CG_DrawStrlen(name) * BIGCHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			CG_DrawBigStringColor(x, y - 22, name, color);
		}
	}

	trap_R_SetColor(NULL);
}

static qbool
CG_WeaponSelectable(int i)
{
	if(!cg.snap->ps.ammo[i])
		return qfalse;
	if(!(cg.snap->ps.stats[ STAT_PRIWEAPS ] & (1 << i)))
		return qfalse;

	return qtrue;
}

void
CG_NextWeapon_f(void)
{
	int	i;
	int	original;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weaponSelect++;
		if(cg.weaponSelect == MAX_WEAPONS)
			cg.weaponSelect = 0;
		if(cg.weaponSelect == W1melee)
			continue;	/* never cycle to gauntlet */
		if(CG_WeaponSelectable(cg.weaponSelect))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weaponSelect = original;
}

void
CG_PrevWeapon_f(void)
{
	int	i;
	int	original;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weaponSelect--;
		if(cg.weaponSelect == -1)
			cg.weaponSelect = MAX_WEAPONS - 1;
		if(cg.weaponSelect == W1melee)
			continue;	/* never cycle to gauntlet */
		if(CG_WeaponSelectable(cg.weaponSelect))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weaponSelect = original;
}

void
CG_Weapon_f(void)
{
	int num;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;
	num = atoi(CG_Argv(1));
	if(num < 1 || num > MAX_WEAPONS-1)
		return;
	cg.weaponSelectTime = cg.time;
	if(!(cg.snap->ps.stats[STAT_PRIWEAPS] & (1 << num)))
		return;		/* don't have the weapon */
	cg.weaponSelect = num;
}

/* The current weapon has just run out of ammo */
void
CG_OutOfAmmoChange(void)
{
	int i;

	cg.weaponSelectTime = cg.time;
	for(i = MAX_WEAPONS-1; i > 0; i--)
		if(CG_WeaponSelectable(i)){
			cg.weaponSelect = i;
			break;
		}
}

/*
 * Weapon events
 */

/* Caused by an EV_FIRE_WEAPON event */
void
CG_FireWeapon(centity_t *cent)
{
	entityState_t	*ent;
	int c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if(ent->weapon == Wnone)
		return;
	if(ent->weapon >= Wnumweaps){
		CG_Error("CG_FireWeapon: ent->weapon >= Wnumweaps");
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	/* mark the entity as muzzle flashing, so when it is added it will
	 * append the flash to the weapon model */
	cent->muzzleFlashTime = cg.time;

	/* lightning gun only does this this on initial press */
	if(ent->weapon == W1lightning)
		if(cent->pe.lightningFiring)
			return;
	if(ent->weapon == W1railgun)
		cent->pe.railFireTime = cg.time;

	/* play quad sound if needed */
	if(cent->currentState.powerups & (1 << PW_QUAD))
		trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM,
			cgs.media.quadSound);

	/* play a sound */
	for(c = 0; c < 4; c++)
		if(!weap->flashSound[c])
			break;
	if(c > 0){
		c = rand() % c;
		if(weap->flashSound[c])
			trap_S_StartSound(NULL, ent->number, CHAN_WEAPON,
				weap->flashSound[c]);
	}

	/* do brass ejection */
	if(weap->ejectBrassFunc && cg_brassTime.integer > 0)
		weap->ejectBrassFunc(cent);
}

/* Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing */
void
CG_MissileHitWall(int weapon, int clientNum, Vec3 origin, Vec3 dir,
		  impactSound_t soundType)
{
	qhandle_t	mod, mark, shader;
	sfxHandle_t sfx;
	float	radius, light;
	Vec3	lightColor;
	localEntity_t   *le;
	int	r, duration;
	qbool		alphaFade, isSprite;
	Vec3		sprOrg, sprVel;

	UNUSED(soundType);
	mark = 0;
	radius	= 32;
	sfx	= 0;
	mod	= 0;
	shader	= 0;
	light	= 0;
	lightColor[0]	= 1;
	lightColor[1]	= 1;
	lightColor[2]	= 0;

	/* set defaults */
	isSprite = qfalse;
	duration = 600;

	switch(weapon){
	default:
	case W1nailgun:
		if(soundType == IMPACTSOUND_FLESH)
			sfx = cgs.media.sfx_nghitflesh;
		else if(soundType == IMPACTSOUND_METAL)
			sfx = cgs.media.sfx_nghitmetal;
		else
			sfx = cgs.media.sfx_nghit;
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case W1lightning:
		/* no explosion at LG impact, it is added with the beam */
		r = rand() & 3;
		if(r < 2)
			sfx = cgs.media.sfx_lghit2;
		else if(r == 2)
			sfx = cgs.media.sfx_lghit1;
		else
			sfx = cgs.media.sfx_lghit3;
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case W2proxlauncher:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case W2grenadelauncher:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case W2rocketlauncher:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0]	= 1;
		lightColor[1]	= 0.75;
		lightColor[2]	= 0.0;
		if(cg_oldRocket.integer == 0){
			/* explosion sprite animation */
			maddv3(origin, 24, dir, sprOrg);
			scalev3(dir, 64, sprVel);

			CG_ParticleExplosion("explode1", sprOrg, sprVel, 1400,
				20,
				30);
		}
		break;
	case W1railgun:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		/* sfx = cgs.media.sfx_railg; */
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case W1plasmagun:
		mod = cgs.media.ringFlashModel;
		shader	= cgs.media.plasmaExplosionShader;
		sfx	= cgs.media.sfx_plasmaexp;
		mark	= cgs.media.energyMarkShader;
		radius	= 16;
		break;
	case W2bfg:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case W1shotgun:
		mod = cgs.media.bulletFlashModel;
		shader	= cgs.media.bulletExplosionShader;
		mark	= cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;
	case W1chaingun:
		mod = cgs.media.bulletFlashModel;
		if(soundType == IMPACTSOUND_FLESH)
			sfx = cgs.media.sfx_chghitflesh;
		else if(soundType == IMPACTSOUND_METAL)
			sfx = cgs.media.sfx_chghitmetal;
		else
			sfx = cgs.media.sfx_chghit;
		mark = cgs.media.bulletMarkShader;

		radius = 8;
		break;
	case W1machinegun:
		mod = cgs.media.bulletFlashModel;
		shader	= cgs.media.bulletExplosionShader;
		mark	= cgs.media.bulletMarkShader;

		r = rand() & 3;
		if(r == 0)
			sfx = cgs.media.sfx_ric1;
		else if(r == 1)
			sfx = cgs.media.sfx_ric2;
		else
			sfx = cgs.media.sfx_ric3;

		radius = 8;
		break;
	}

	if(sfx)
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);

	/* create the explosion */
	if(mod){
		le = CG_MakeExplosion(origin, dir,
			mod, shader,
			duration, isSprite);
		le->light = light;
		copyv3(lightColor, le->lightColor);
		if(weapon == W1railgun){
			/* colorize with client color */
			copyv3(cgs.clientinfo[clientNum].color1, le->color);
			le->refEntity.shaderRGBA[0] = le->color[0] * 0xff;
			le->refEntity.shaderRGBA[1] = le->color[1] * 0xff;
			le->refEntity.shaderRGBA[2] = le->color[2] * 0xff;
			le->refEntity.shaderRGBA[3] = 0xff;
		}
	}

	/* impact mark */
	alphaFade = (mark == cgs.media.energyMarkShader);	/* plasma fades alpha, all others fade color */
	if(weapon == W1railgun){
		float *color;

		/* colorize with client color */
		color = cgs.clientinfo[clientNum].color1;
		CG_ImpactMark(mark, origin, dir,
			random()*360, color[0],color[1], color[2],1, alphaFade,
			radius,
			qfalse);
	}else
		CG_ImpactMark(mark, origin, dir,
			random()*360, 1,1,1,1, alphaFade, radius, qfalse);
}

void
CG_MissileHitPlayer(int weapon, Vec3 origin, Vec3 dir, int entityNum)
{
	CG_Bleed(origin, entityNum);

	/* some weapons will make an explosion with the blood, while
	 * others will just make the blood */
	switch(weapon){
	case W2grenadelauncher:
	case W2rocketlauncher:
	case W1plasmagun:
	case W2bfg:
	case W1nailgun:
	case W1chaingun:
	case W2proxlauncher:
		CG_MissileHitWall(weapon, 0, origin, dir, IMPACTSOUND_FLESH);
		break;
	default:
		break;
	}
}

/*
 * Shotgun tracing
 */

static void
CG_ShotgunPellet(Vec3 start, Vec3 end, int skipNum)
{
	trace_t tr;
	int sourceContentType, destContentType;

	CG_Trace(&tr, start, NULL, NULL, end, skipNum, MASK_SHOT);

	sourceContentType = CG_PointContents(start, 0);
	destContentType = CG_PointContents(tr.endpos, 0);

	/* FIXME: should probably move this cruft into CG_BubbleTrail */
	if(sourceContentType == destContentType){
		if(sourceContentType & CONTENTS_WATER)
			CG_BubbleTrail(start, tr.endpos, 32);
	}else if(sourceContentType & CONTENTS_WATER){
		trace_t trace;

		trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0,
			CONTENTS_WATER);
		CG_BubbleTrail(start, trace.endpos, 32);
	}else if(destContentType & CONTENTS_WATER){
		trace_t trace;

		trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0,
			CONTENTS_WATER);
		CG_BubbleTrail(tr.endpos, trace.endpos, 32);
	}

	if(tr.surfaceFlags & SURF_NOIMPACT)
		return;

	if(cg_entities[tr.entityNum].currentState.eType == ET_PLAYER)
		CG_MissileHitPlayer(W1shotgun, tr.endpos, tr.plane.normal,
			tr.entityNum);
	else{
		if(tr.surfaceFlags & SURF_NOIMPACT)
			/* SURF_NOIMPACT will not make a flame puff or a mark */
			return;
		if(tr.surfaceFlags & SURF_METALSTEPS)
			CG_MissileHitWall(W1shotgun, 0, tr.endpos,
				tr.plane.normal,
				IMPACTSOUND_METAL);
		else
			CG_MissileHitWall(W1shotgun, 0, tr.endpos,
				tr.plane.normal,
				IMPACTSOUND_DEFAULT);
	}
}

/*
 * Perform the same traces the server did to locate the
 * hit splashes
 */
static void
CG_ShotgunPattern(Vec3 origin, Vec3 origin2, int seed, int otherEntNum)
{
	int i;
	float	r, u;
	Vec3	end;
	Vec3	forward, right, up;

	/* derive the right and up vectors from the forward vector, because
	 * the client won't have any other information */
	norm2v3(origin2, forward);
	perpv3(right, forward);
	crossv3(forward, right, up);

	/* generate the "random" spread pattern */
	for(i = 0; i < DEFAULT_SHOTGUN_COUNT; i++){
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		maddv3(origin, 8192 * 16, forward, end);
		maddv3 (end, r, right, end);
		maddv3 (end, u, up, end);

		CG_ShotgunPellet(origin, end, otherEntNum);
	}
}

void
CG_ShotgunFire(entityState_t *es)
{
	Vec3	v;
	int	contents;

	subv3(es->origin2, es->pos.trBase, v);
	normv3(v);
	scalev3(v, 32, v);
	addv3(es->pos.trBase, v, v);
	if(cgs.glconfig.hardwareType != GLHW_RAGEPRO){
		/* ragepro can't alpha fade, so don't even bother with smoke */
		Vec3 up;

		contents = CG_PointContents(es->pos.trBase, 0);
		if(!(contents & CONTENTS_WATER)){
			setv3(up, 0, 0, 8);
			CG_SmokePuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0,
				LEF_PUFF_DONT_SCALE,
				cgs.media.shotgunSmokePuffShader);
		}
	}
	CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm,
		es->otherEntityNum);
}

/*
 * Bullets
 */

void
CG_Tracer(Vec3 source, Vec3 dest)
{
	Vec3	forward, right;
	polyVert_t verts[4];
	Vec3	line;
	float	len, begin, end;
	Vec3	start, finish;
	Vec3	midpoint;

	/* tracer */
	subv3(dest, source, forward);
	len = normv3(forward);

	/* start at least a little ways from the muzzle */
	if(len < 100)
		return;
	begin	= 50 + random() * (len - 60);
	end	= begin + cg_tracerLength.value;
	if(end > len)
		end = len;
	maddv3(source, begin, forward, start);
	maddv3(source, end, forward, finish);

	line[0] = dotv3(forward, cg.refdef.viewaxis[1]);
	line[1] = dotv3(forward, cg.refdef.viewaxis[2]);

	scalev3(cg.refdef.viewaxis[1], line[1], right);
	maddv3(right, -line[0], cg.refdef.viewaxis[2], right);
	normv3(right);

	maddv3(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0]	= 0;
	verts[0].st[1]	= 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	maddv3(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0]	= 1;
	verts[1].st[1]	= 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	maddv3(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0]	= 1;
	verts[2].st[1]	= 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	maddv3(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0]	= 0;
	verts[3].st[1]	= 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);

	midpoint[0] = (start[0] + finish[0]) * 0.5;
	midpoint[1] = (start[1] + finish[1]) * 0.5;
	midpoint[2] = (start[2] + finish[2]) * 0.5;

	/* add the tracer sound */
	trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO,
		cgs.media.tracerSound);

}

static qbool
CG_CalcMuzzlePoint(int entityNum, Vec3 muzzle)
{
	Vec3	forward;
	centity_t       *cent;
	int	anim;

	if(entityNum == cg.snap->ps.clientNum){
		copyv3(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		anglev3s(cg.snap->ps.viewangles, forward, NULL, NULL);
		maddv3(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if(!cent->currentValid)
		return qfalse;

	copyv3(cent->currentState.pos.trBase, muzzle);

	anglev3s(cent->currentState.apos.trBase, forward, NULL, NULL);
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		muzzle[2] += CROUCH_VIEWHEIGHT;
	else
		muzzle[2] += DEFAULT_VIEWHEIGHT;

	maddv3(muzzle, 14, forward, muzzle);

	return qtrue;

}

/* Renders bullet effects. */
void
CG_Bullet(Vec3 end, int sourceEntityNum, Vec3 normal, qbool flesh,
	  int fleshEntityNum)
{
	trace_t trace;
	int sourceContentType, destContentType;
	Vec3	start;

	/* if the shooter is currently valid, calc a source point and possibly
	 * do trail effects */
	if(sourceEntityNum >= 0 && cg_tracerChance.value > 0)
		if(CG_CalcMuzzlePoint(sourceEntityNum, start)){
			sourceContentType = CG_PointContents(start, 0);
			destContentType = CG_PointContents(end, 0);

			/* do a complete bubble trail if necessary */
			if((sourceContentType == destContentType) &&
			   (sourceContentType & CONTENTS_WATER))
				CG_BubbleTrail(start, end, 32);
			/* bubble trail from water into air */
			else if((sourceContentType & CONTENTS_WATER)){
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL,
					0,
					CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, 32);
			}
			/* bubble trail from air into water */
			else if((destContentType & CONTENTS_WATER)){
				trap_CM_BoxTrace(&trace, start, end, NULL, NULL,
					0,
					CONTENTS_WATER);
				CG_BubbleTrail(trace.endpos, end, 32);
			}

			/* draw a tracer */
			if(random() < cg_tracerChance.value)
				CG_Tracer(start, end);
		}

	/* impact splash and mark */
	if(flesh)
		CG_Bleed(end, fleshEntityNum);
	else
		CG_MissileHitWall(W1machinegun, 0, end, normal,
			IMPACTSOUND_DEFAULT);

}
