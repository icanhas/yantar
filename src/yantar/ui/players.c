/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "ui.h"
#include "local.h"

#define UI_TIMER_GESTURE	2300
#define UI_TIMER_JUMP		1000
#define UI_TIMER_LAND		130
#define UI_TIMER_WEAPON_SWITCH	300
#define UI_TIMER_ATTACK		500
#define UI_TIMER_MUZZLE_FLASH	20
#define UI_TIMER_WEAPON_DELAY	250

#define JUMP_HEIGHT		56

#define SWINGSPEED		0.3f

#define SPIN_SPEED		0.9f
#define COAST_TIME		1000

static int dp_realtime;
static float jumpHeight;

/*
 * UI_PlayerInfo_SetWeapon
 */
static void
UI_PlayerInfo_SetWeapon(Playerinfo *pi, Weapon weaponNum)
{
	Gitem * item;
	char	path[MAX_QPATH];

	pi->currentWeapon = weaponNum;
tryagain:
	pi->realWeapon	= weaponNum;
	pi->weaponModel = 0;
	pi->barrelModel = 0;
	pi->flashModel	= 0;

	if(weaponNum == Wnone)
		return;

	for(item = bg_itemlist + 1; item->classname; item++){
		if(item->type != IT_PRIWEAP)
			continue;
		if(item->tag == weaponNum)
			break;
	}

	if(item->classname)
		pi->weaponModel = trap_R_RegisterModel(item->worldmodel[0]);

	if(pi->weaponModel == 0){
		if(weaponNum == Wmachinegun){
			weaponNum = Wnone;
			goto tryagain;
		}
		weaponNum = Wmachinegun;
		goto tryagain;
	}

	if(weaponNum == Wmachinegun || weaponNum == Wmelee){
		strcpy(path, item->worldmodel[0]);
		Q_stripext(path, path, sizeof(path));
		strcat(path, "_barrel");
		pi->barrelModel = trap_R_RegisterModel(path);
	}

	strcpy(path, item->worldmodel[0]);
	Q_stripext(path, path, sizeof(path));
	strcat(path, "_flash");
	pi->flashModel = trap_R_RegisterModel(path);

	switch(weaponNum){
	case Wmelee:
		MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
		break;
	case Wmachinegun:
		MAKERGB(pi->flashDlightColor, 1, 1, 0);
		break;
	case Wshotgun:
		MAKERGB(pi->flashDlightColor, 1, 1, 0);
		break;
	case Wgrenadelauncher:
		MAKERGB(pi->flashDlightColor, 1, 0.7f, 0.5f);
		break;
	case Wrocketlauncher:
		MAKERGB(pi->flashDlightColor, 1, 0.75f, 0);
		break;
	case Wlightning:
		MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
		break;
	case Wrailgun:
		MAKERGB(pi->flashDlightColor, 1, 0.5f, 0);
		break;
	case Wplasmagun:
		MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
		break;
	case Whook:
		MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
		break;
	default:
		MAKERGB(pi->flashDlightColor, 1, 1, 1);
		break;
	}
}


/*
 * UI_ForceLegsAnim
 */
static void
UI_ForceLegsAnim(Playerinfo *pi, int anim)
{
	pi->legsAnim = ((pi->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

	if(anim == LEGS_JUMP)
		pi->legsAnimationTimer = UI_TIMER_JUMP;
}


/*
 * UI_SetLegsAnim
 */
static void
UI_SetLegsAnim(Playerinfo *pi, int anim)
{
	if(pi->pendingLegsAnim){
		anim = pi->pendingLegsAnim;
		pi->pendingLegsAnim = 0;
	}
	UI_ForceLegsAnim(pi, anim);
}


/*
 * UI_ForceTorsoAnim
 */
static void
UI_ForceTorsoAnim(Playerinfo *pi, int anim)
{
	pi->torsoAnim =
		((pi->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

	if(anim == TORSO_GESTURE)
		pi->torsoAnimationTimer = UI_TIMER_GESTURE;

	if(anim == TORSO_ATTACK || anim == TORSO_ATTACK2)
		pi->torsoAnimationTimer = UI_TIMER_ATTACK;
}


/*
 * UI_SetTorsoAnim
 */
static void
UI_SetTorsoAnim(Playerinfo *pi, int anim)
{
	if(pi->pendingTorsoAnim){
		anim = pi->pendingTorsoAnim;
		pi->pendingTorsoAnim = 0;
	}

	UI_ForceTorsoAnim(pi, anim);
}


/*
 * UI_TorsoSequencing
 */
static void
UI_TorsoSequencing(Playerinfo *pi)
{
	int currentAnim;

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if(pi->weapon != pi->currentWeapon)
		if(currentAnim != TORSO_DROP){
			pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
			UI_ForceTorsoAnim(pi, TORSO_DROP);
		}

	if(pi->torsoAnimationTimer > 0)
		return;

	if(currentAnim == TORSO_GESTURE){
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}

	if(currentAnim == TORSO_ATTACK || currentAnim == TORSO_ATTACK2){
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}

	if(currentAnim == TORSO_DROP){
		UI_PlayerInfo_SetWeapon(pi, pi->weapon);
		pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
		UI_ForceTorsoAnim(pi, TORSO_RAISE);
		return;
	}

	if(currentAnim == TORSO_RAISE){
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}
}


/*
 * UI_LegsSequencing
 */
static void
UI_LegsSequencing(Playerinfo *pi)
{
	int currentAnim;

	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;

	if(pi->legsAnimationTimer > 0){
		if(currentAnim == LEGS_JUMP)
			jumpHeight = JUMP_HEIGHT *
				     sin(
				M_PI *
				(UI_TIMER_JUMP -
				 pi->legsAnimationTimer) / UI_TIMER_JUMP);
		return;
	}

	if(currentAnim == LEGS_JUMP){
		UI_ForceLegsAnim(pi, LEGS_LAND);
		pi->legsAnimationTimer = UI_TIMER_LAND;
		jumpHeight = 0;
		return;
	}

	if(currentAnim == LEGS_LAND){
		UI_SetLegsAnim(pi, LEGS_IDLE);
		return;
	}
}


/*
 * UI_PositionEntityOnTag
 */
static void
UI_PositionEntityOnTag(Refent *entity, const Refent *parent,
		       Cliphandle parentModel, char *tagName)
{
	int i;
	Orient lerped;

	/* lerp the tag */
	trap_CM_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName);

	/* FIXME: allow origin offsets along tag? */
	copyv3(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		saddv3(entity->origin, lerped.origin[i], parent->axis[i],
			entity->origin);

	/* cast away const because of compiler problems */
	MatrixMultiply(lerped.axis, ((Refent*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}


/*
 * UI_PositionRotatedEntityOnTag
 */
static void
UI_PositionRotatedEntityOnTag(Refent *entity, const Refent *parent,
			      Cliphandle parentModel, char *tagName)
{
	int i;
	Orient lerped;
	Vec3 tempAxis[3];

	/* lerp the tag */
	trap_CM_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName);

	/* FIXME: allow origin offsets along tag? */
	copyv3(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		saddv3(entity->origin, lerped.origin[i], parent->axis[i],
			entity->origin);

	/* cast away const because of compiler problems */
	MatrixMultiply(entity->axis, ((Refent*)parent)->axis, tempAxis);
	MatrixMultiply(lerped.axis, tempAxis, entity->axis);
}


/*
 * UI_SetLerpFrameAnimation
 */
static void
UI_SetLerpFrameAnimation(Playerinfo *ci, Lerpframe *lf, int newAnimation)
{
	Anim *anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if(newAnimation < 0 || newAnimation >= MAX_ANIMATIONS)
		trap_Error(va("Bad animation number: %i", newAnimation));

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}


/*
 * UI_RunLerpFrame
 */
static void
UI_RunLerpFrame(Playerinfo *ci, Lerpframe *lf, int newAnimation)
{
	int f;
	Anim *anim;

	/* see if the animation sequence is switching */
	if(newAnimation != lf->animationNumber || !lf->animation)
		UI_SetLerpFrameAnimation(ci, lf, newAnimation);

	/* if we have passed the current frame, move it to
	 * oldFrame and calculate a new frame */
	if(dp_realtime >= lf->frameTime){
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		/* get the next frame based on the animation */
		anim = lf->animation;
		if(dp_realtime < lf->animationTime)
			lf->frameTime = lf->animationTime;	/* initial lerp */
		else
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		if(f >= anim->numFrames){
			f -= anim->numFrames;
			if(anim->loopFrames){
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}else{
				f = anim->numFrames - 1;
				/* the animation is stuck at the end, so it
				 * can immediately transition to another sequence */
				lf->frameTime = dp_realtime;
			}
		}
		lf->frame = anim->firstFrame + f;
		if(dp_realtime > lf->frameTime)
			lf->frameTime = dp_realtime;
	}

	if(lf->frameTime > dp_realtime + 200)
		lf->frameTime = dp_realtime;

	if(lf->oldFrameTime > dp_realtime)
		lf->oldFrameTime = dp_realtime;
	/* calculate current lerp value */
	if(lf->frameTime == lf->oldFrameTime)
		lf->backlerp = 0;
	else
		lf->backlerp = 1.0 -
			       (float)(dp_realtime -
				       lf->oldFrameTime) /
			       (lf->frameTime - lf->oldFrameTime);
}


/*
 * UI_PlayerAnimation
 */
static void
UI_PlayerAnimation(Playerinfo *pi, int *legsOld, int *legs,
		   float *legsBackLerp,
		   int *torsoOld, int *torso,
		   float *torsoBackLerp)
{

	/* legs animation */
	pi->legsAnimationTimer -= uis.frametime;
	if(pi->legsAnimationTimer < 0)
		pi->legsAnimationTimer = 0;

	UI_LegsSequencing(pi);

	if(pi->legs.yawing && (pi->legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE)
		UI_RunLerpFrame(pi, &pi->legs, LEGS_TURN);
	else
		UI_RunLerpFrame(pi, &pi->legs, pi->legsAnim);
	*legsOld = pi->legs.oldFrame;
	*legs = pi->legs.frame;
	*legsBackLerp = pi->legs.backlerp;

	/* torso animation */
	pi->torsoAnimationTimer -= uis.frametime;
	if(pi->torsoAnimationTimer < 0)
		pi->torsoAnimationTimer = 0;

	UI_TorsoSequencing(pi);

	UI_RunLerpFrame(pi, &pi->torso, pi->torsoAnim);
	*torsoOld = pi->torso.oldFrame;
	*torso = pi->torso.frame;
	*torsoBackLerp = pi->torso.backlerp;
}


/*
 * UI_SwingAngles
 */
static void
UI_SwingAngles(float destination, float swingTolerance, float clampTolerance,
	       float speed, float *angle, qbool *swinging)
{
	float	swing;
	float	move;
	float	scale;

	if(!*swinging){
		/* see if a swing should be started */
		swing = subeuler(*angle, destination);
		if(swing > swingTolerance || swing < -swingTolerance)
			*swinging = qtrue;
	}

	if(!*swinging)
		return;

	/* modify the speed depending on the delta
	 * so it doesn't seem so linear */
	swing	= subeuler(destination, *angle);
	scale	= fabs(swing);
	if(scale < swingTolerance * 0.5)
		scale = 0.5;
	else if(scale < swingTolerance)
		scale = 1.0;
	else
		scale = 2.0;

	/* swing towards the destination angle */
	if(swing >= 0){
		move = uis.frametime * scale * speed;
		if(move >= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = modeuler(*angle + move);
	}else if(swing < 0){
		move = uis.frametime * scale * -speed;
		if(move <= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = modeuler(*angle + move);
	}

	/* clamp to no more than tolerance */
	swing = subeuler(destination, *angle);
	if(swing > clampTolerance)
		*angle = modeuler(destination - (clampTolerance - 1));
	else if(swing < -clampTolerance)
		*angle = modeuler(destination + (clampTolerance - 1));
}


/*
 * UI_MovedirAdjustment
 */
static float
UI_MovedirAdjustment(Playerinfo *pi)
{
	Vec3	relativeAngles;
	Vec3	moveVector;

	subv3(pi->viewAngles, pi->moveAngles, relativeAngles);
	anglev3s(relativeAngles, moveVector, NULL, NULL);
	if(Q_fabs(moveVector[0]) < 0.01)
		moveVector[0] = 0.0;
	if(Q_fabs(moveVector[1]) < 0.01)
		moveVector[1] = 0.0;

	if(moveVector[1] == 0 && moveVector[0] > 0)
		return 0;
	if(moveVector[1] < 0 && moveVector[0] > 0)
		return 22;
	if(moveVector[1] < 0 && moveVector[0] == 0)
		return 45;
	if(moveVector[1] < 0 && moveVector[0] < 0)
		return -22;
	if(moveVector[1] == 0 && moveVector[0] < 0)
		return 0;
	if(moveVector[1] > 0 && moveVector[0] < 0)
		return 22;
	if(moveVector[1] > 0 && moveVector[0] == 0)
		return -45;

	return -22;
}


/*
 * UI_PlayerAngles
 */
static void
UI_PlayerAngles(Playerinfo *pi, Vec3 legs[3], Vec3 torso[3],
		Vec3 head[3])
{
	Vec3	legsAngles, torsoAngles, headAngles;
	float	dest;
	float	adjust;

	copyv3(pi->viewAngles, headAngles);
	headAngles[YAW] = modeuler(headAngles[YAW]);
	clearv3(legsAngles);
	clearv3(torsoAngles);

	/* --------- yaw ------------- */

	/* allow yaw to drift a bit */
	if((pi->legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE
	   || (pi->torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND){
		/* if not standing still, always point all in the same direction */
		pi->torso.yawing = qtrue;	/* always center */
		pi->torso.pitching = qtrue;	/* always center */
		pi->legs.yawing = qtrue;	/* always center */
	}

	/* adjust legs for movement dir */
	adjust = UI_MovedirAdjustment(pi);
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * adjust;


	/* torso */
	UI_SwingAngles(torsoAngles[YAW], 25, 90, SWINGSPEED, &pi->torso.yawAngle,
		&pi->torso.yawing);
	UI_SwingAngles(legsAngles[YAW], 40, 90, SWINGSPEED, &pi->legs.yawAngle,
		&pi->legs.yawing);

	torsoAngles[YAW] = pi->torso.yawAngle;
	legsAngles[YAW] = pi->legs.yawAngle;

	/* --------- pitch ------------- */

	/* only show a fraction of the pitch angle in the torso */
	if(headAngles[PITCH] > 180)
		dest = (-360 + headAngles[PITCH]) * 0.75;
	else
		dest = headAngles[PITCH] * 0.75;
	UI_SwingAngles(dest, 15, 30, 0.1f, &pi->torso.pitchAngle,
		&pi->torso.pitching);
	torsoAngles[PITCH] = pi->torso.pitchAngle;

	/* pull the angles back out of the hierarchial chain */
	subeulers(headAngles, torsoAngles, headAngles);
	subeulers(torsoAngles, legsAngles, torsoAngles);
	eulertoaxis(legsAngles, legs);
	eulertoaxis(torsoAngles, torso);
	eulertoaxis(headAngles, head);
}


/*
 * UI_PlayerFloatSprite
 */
static void
UI_PlayerFloatSprite(Playerinfo *pi, Vec3 origin, Handle shader)
{
	Refent ent;

	UNUSED(pi);
	memset(&ent, 0, sizeof(ent));
	copyv3(origin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader	= shader;
	ent.radius		= 10;
	ent.renderfx		= 0;
	trap_R_AddRefEntityToScene(&ent);
}


/*
 * UI_MachinegunSpinAngle
 */
float
UI_MachinegunSpinAngle(Playerinfo *pi)
{
	int delta;
	float	angle;
	float	speed;
	int	torsoAnim;

	delta = dp_realtime - pi->barrelTime;
	if(pi->barrelSpinning)
		angle = pi->barrelAngle + delta * SPIN_SPEED;
	else{
		if(delta > COAST_TIME)
			delta = COAST_TIME;

		speed = 0.5 *
			(SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = pi->barrelAngle + delta * speed;
	}

	torsoAnim = pi->torsoAnim  & ~ANIM_TOGGLEBIT;
	if(torsoAnim == TORSO_ATTACK2)
		torsoAnim = TORSO_ATTACK;
	if(pi->barrelSpinning == !(torsoAnim == TORSO_ATTACK)){
		pi->barrelTime	= dp_realtime;
		pi->barrelAngle = modeuler(angle);
		pi->barrelSpinning = !!(torsoAnim == TORSO_ATTACK);
	}

	return angle;
}


/*
 * UI_DrawPlayer
 */
void
UI_DrawPlayer(float x, float y, float w, float h, Playerinfo *pi, int time)
{
	Refdef refdef;
	Refent	legs;
	Refent	torso;
	Refent	head;
	Refent	gun;
	Refent	barrel;
	Refent	flash;
	Vec3	origin;
	int	renderfx;
	Vec3	mins = {-16, -16, -24};
	Vec3	maxs = {16, 16, 32};
	float	len;
	float	xx;

	if(!pi->legsModel || !pi->torsoModel || !pi->headModel ||
	   !pi->animations[0].numFrames)
		return;

	dp_realtime = time;

	if(pi->pendingWeapon != -1 && dp_realtime > pi->weaponTimer){
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
		if(pi->currentWeapon != pi->weapon)
			trap_sndstartlocalsound(weaponChangeSound, CHAN_LOCAL);
	}

	UI_AdjustFrom640(&x, &y, &w, &h);

	y -= jumpHeight;

	memset(&refdef, 0, sizeof(refdef));
	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	refdef.rdflags = RDF_NOWORLDMODEL;

	clearaxis(refdef.viewaxis);

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = (int)((float)refdef.width / 640.0f * 90.0f);
	xx = refdef.width / tan(refdef.fov_x / 360 * M_PI);
	refdef.fov_y = atan2(refdef.height, xx);
	refdef.fov_y *= (360 / M_PI);

	/* calculate distance so the player nearly fills the box */
	len = 0.7 * (maxs[2] - mins[2]);
	origin[0] = len / tan(DEG2RAD(refdef.fov_x) * 0.5);
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	/* get the rotation information */
	UI_PlayerAngles(pi, legs.axis, torso.axis, head.axis);

	/* get the animation state (after rotation, to allow feet shuffle) */
	UI_PlayerAnimation(pi, &legs.oldframe, &legs.frame, &legs.backlerp,
		&torso.oldframe, &torso.frame, &torso.backlerp);

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	/*
	 * add the legs
	 *  */
	legs.hModel = pi->legsModel;
	legs.customSkin = pi->legsSkin;

	copyv3(origin, legs.origin);

	copyv3(origin, legs.lightingOrigin);
	legs.renderfx = renderfx;
	copyv3 (legs.origin, legs.oldorigin);

	trap_R_AddRefEntityToScene(&legs);

	if(!legs.hModel)
		return;

	/*
	 * add the torso
	 *  */
	torso.hModel = pi->torsoModel;
	if(!torso.hModel)
		return;

	torso.customSkin = pi->torsoSkin;

	copyv3(origin, torso.lightingOrigin);

	UI_PositionRotatedEntityOnTag(&torso, &legs, pi->legsModel, "tag_torso");

	torso.renderfx = renderfx;

	trap_R_AddRefEntityToScene(&torso);

	/*
	 * add the head
	 *  */
	head.hModel = pi->headModel;
	if(!head.hModel)
		return;
	head.customSkin = pi->headSkin;

	copyv3(origin, head.lightingOrigin);

	UI_PositionRotatedEntityOnTag(&head, &torso, pi->torsoModel, "tag_head");

	head.renderfx = renderfx;

	trap_R_AddRefEntityToScene(&head);

	/*
	 * add the gun
	 *  */
	if(pi->currentWeapon != Wnone){
		memset(&gun, 0, sizeof(gun));
		gun.hModel = pi->weaponModel;
		if(pi->currentWeapon == Wrailgun)
			byte4copy(pi->c1RGBA, gun.shaderRGBA);
		else
			byte4copy(colorWhite, gun.shaderRGBA);
		copyv3(origin, gun.lightingOrigin);
		UI_PositionEntityOnTag(&gun, &torso, pi->torsoModel,
			"tag_weapon");
		gun.renderfx = renderfx;
		trap_R_AddRefEntityToScene(&gun);
	}

	/*
	 * add the spinning barrel
	 *  */
	if(pi->realWeapon == Wmachinegun || pi->realWeapon == Wmelee){
		Vec3 angles;

		memset(&barrel, 0, sizeof(barrel));
		copyv3(origin, barrel.lightingOrigin);
		barrel.renderfx = renderfx;

		barrel.hModel	= pi->barrelModel;
		angles[YAW]	= 0;
		angles[PITCH]	= 0;
		angles[ROLL]	= UI_MachinegunSpinAngle(pi);
		if(pi->realWeapon == Wmelee){
			angles[PITCH] = angles[ROLL];
			angles[ROLL] = 0;
		}
		eulertoaxis(angles, barrel.axis);

		UI_PositionRotatedEntityOnTag(&barrel, &gun, pi->weaponModel,
			"tag_barrel");

		trap_R_AddRefEntityToScene(&barrel);
	}

	/*
	 * add muzzle flash
	 *  */
	if(dp_realtime <= pi->muzzleFlashTime){
		if(pi->flashModel){
			memset(&flash, 0, sizeof(flash));
			flash.hModel = pi->flashModel;
			if(pi->currentWeapon == Wrailgun)
				byte4copy(pi->c1RGBA, flash.shaderRGBA);
			else
				byte4copy(colorWhite, flash.shaderRGBA);
			copyv3(origin, flash.lightingOrigin);
			UI_PositionEntityOnTag(&flash, &gun, pi->weaponModel,
				"tag_flash");
			flash.renderfx = renderfx;
			trap_R_AddRefEntityToScene(&flash);
		}

		/* make a dlight for the flash */
		if(pi->flashDlightColor[0] || pi->flashDlightColor[1] ||
		   pi->flashDlightColor[2])
			trap_R_AddLightToScene(flash.origin,
				200 + (rand()&31), pi->flashDlightColor[0],
				pi->flashDlightColor[1], pi->flashDlightColor[2]);
	}

	/*
	 * add the chat icon
	 *  */
	if(pi->chat)
		UI_PlayerFloatSprite(pi, origin,
			trap_R_RegisterShaderNoMip("sprites/balloon4"));

	/*
	 * add an accent light
	 *  */
	origin[0] -= 100;	/* + = behind, - = in front */
	origin[1] += 100;	/* + = left, - = right */
	origin[2] += 100;	/* + = above, - = below */
	trap_R_AddLightToScene(origin, 500, 1.0, 1.0, 1.0);

	origin[0] -= 100;
	origin[1] -= 100;
	origin[2] -= 100;
	trap_R_AddLightToScene(origin, 500, 1.0, 0.0, 0.0);

	trap_R_RenderScene(&refdef);
}


/*
 * UI_RegisterClientSkin
 */
static qbool
UI_RegisterClientSkin(Playerinfo *pi, const char *modelName,
		      const char *skinName)
{
	char filename[MAX_QPATH];

	Q_sprintf(filename, sizeof(filename),
		Pplayermodels "/%s/lower_%s.skin", modelName,
		skinName);
	pi->legsSkin = trap_R_RegisterSkin(filename);

	Q_sprintf(filename, sizeof(filename),
		Pplayermodels "/%s/upper_%s.skin", modelName,
		skinName);
	pi->torsoSkin = trap_R_RegisterSkin(filename);

	Q_sprintf(filename, sizeof(filename), Pplayermodels "/%s/head_%s.skin",
		modelName,
		skinName);
	pi->headSkin = trap_R_RegisterSkin(filename);

	if(!pi->legsSkin || !pi->torsoSkin || !pi->headSkin)
		return qfalse;

	return qtrue;
}


/*
 * UI_ParseAnimationFile
 */
static qbool
UI_ParseAnimationFile(const char *filename, Anim *animations)
{
	char	*text_p, *prev;
	int	len;
	int	i;
	char    *token;
	float	fps;
	int	skip;
	char	text[20000];
	Fhandle f;

	memset(animations, 0, sizeof(Anim) * MAX_ANIMATIONS);

	/* load the file */
	len = trap_fsopen(filename, &f, FS_READ);
	if(len <= 0)
		return qfalse;
	if(len >= (sizeof(text) - 1)){
		comprintf("File %s too long\n", filename);
		trap_fsclose(f);
		return qfalse;
	}
	trap_fsread(text, len, f);
	text[len] = 0;
	trap_fsclose(f);

	/* parse the text */
	text_p	= text;
	skip	= 0;	/* quite the compiler warning */

	/* read optional parameters */
	while(1){
		prev = text_p;	/* so we can unget */
		token = Q_readtok(&text_p);
		if(!token)
			break;
		if(!Q_stricmp(token, "footsteps")){
			token = Q_readtok(&text_p);
			if(!token)
				break;
			continue;
		}else if(!Q_stricmp(token, "headoffset")){
			for(i = 0; i < 3; i++){
				token = Q_readtok(&text_p);
				if(!token)
					break;
			}
			continue;
		}else if(!Q_stricmp(token, "sex")){
			token = Q_readtok(&text_p);
			if(!token)
				break;
			continue;
		}

		/* if it is a number, start parsing animations */
		if(token[0] >= '0' && token[0] <= '9'){
			text_p = prev;	/* unget the token */
			break;
		}

		comprintf("unknown token '%s' is %s\n", token, filename);
	}

	/* read information for each frame */
	for(i = 0; i < MAX_ANIMATIONS; i++){

		token = Q_readtok(&text_p);
		if(!token)
			break;
		animations[i].firstFrame = atoi(token);
		/* leg only frames are adjusted to not count the upper body only frames */
		if(i == LEGS_WALKCR)
			skip = animations[LEGS_WALKCR].firstFrame -
			       animations[TORSO_GESTURE].firstFrame;
		if(i >= LEGS_WALKCR)
			animations[i].firstFrame -= skip;

		token = Q_readtok(&text_p);
		if(!token)
			break;
		animations[i].numFrames = atoi(token);

		token = Q_readtok(&text_p);
		if(!token)
			break;
		animations[i].loopFrames = atoi(token);

		token = Q_readtok(&text_p);
		if(!token)
			break;
		fps = atof(token);
		if(fps == 0)
			fps = 1;
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if(i != MAX_ANIMATIONS){
		comprintf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}

	return qtrue;
}


/*
 * UI_RegisterClientModelname
 */
qbool
UI_RegisterClientModelname(Playerinfo *pi, const char *modelSkinName)
{
	char	modelName[MAX_QPATH];
	char	skinName[MAX_QPATH];
	char	filename[MAX_QPATH];
	char	*slash;

	pi->torsoModel	= 0;
	pi->headModel	= 0;

	if(!modelSkinName[0])
		return qfalse;

	Q_strncpyz(modelName, modelSkinName, sizeof(modelName));

	slash = strchr(modelName, '/');
	if(!slash)
		/* modelName did not include a skin name */
		Q_strncpyz(skinName, "default", sizeof(skinName));
	else{
		Q_strncpyz(skinName, slash + 1, sizeof(skinName));
		/* truncate modelName */
		*slash = 0;
	}

	/* load cmodels before models so filecache works */

	Q_sprintf(filename, sizeof(filename), Pplayermodels "/%s/lower",
		modelName);
	pi->legsModel = trap_R_RegisterModel(filename);
	if(!pi->legsModel){
		comprintf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	Q_sprintf(filename, sizeof(filename), Pplayermodels "/%s/upper",
		modelName);
	pi->torsoModel = trap_R_RegisterModel(filename);
	if(!pi->torsoModel){
		comprintf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	Q_sprintf(filename, sizeof(filename), Pplayermodels "/%s/head",
		modelName);
	pi->headModel = trap_R_RegisterModel(filename);
	if(!pi->headModel){
		comprintf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	/* if any skins failed to load, fall back to default */
	if(!UI_RegisterClientSkin(pi, modelName, skinName))
		if(!UI_RegisterClientSkin(pi, modelName, "default")){
			comprintf("Failed to load skin file: %s : %s\n",
				modelName,
				skinName);
			return qfalse;
		}

	/* load the animations */
	Q_sprintf(filename, sizeof(filename),
		Pplayermodels "/%s/animation.cfg",
		modelName);
	if(!UI_ParseAnimationFile(filename, pi->animations)){
		comprintf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

	return qtrue;
}


/*
 * UI_PlayerInfo_SetModel
 */
void
UI_PlayerInfo_SetModel(Playerinfo *pi, const char *model)
{
	memset(pi, 0, sizeof(*pi));
	UI_RegisterClientModelname(pi, model);
	pi->weapon = Wmachinegun;
	pi->currentWeapon = pi->weapon;
	pi->lastWeapon = pi->weapon;
	pi->pendingWeapon = -1;
	pi->weaponTimer = 0;
	pi->chat = qfalse;
	pi->newModel = qtrue;
	UI_PlayerInfo_SetWeapon(pi, pi->weapon);
}


/*
 * UI_PlayerInfo_SetInfo
 */
void
UI_PlayerInfo_SetInfo(Playerinfo *pi, int legsAnim, int torsoAnim,
		      Vec3 viewAngles, Vec3 moveAngles,
		      Weapon weaponNumber,
		      qbool chat)
{
	int	currentAnim;
	Weapon weaponNum;
	int	c;

	pi->chat = chat;

	c = (int)trap_cvargetf("color1");

	clearv3(pi->color1);

	if(c < 1 || c > 7)
		setv3(pi->color1, 1, 1, 1);
	else{
		if(c & 1)
			pi->color1[2] = 1.0f;

		if(c & 2)
			pi->color1[1] = 1.0f;

		if(c & 4)
			pi->color1[0] = 1.0f;
	}

	pi->c1RGBA[0]	= 255 * pi->color1[0];
	pi->c1RGBA[1]	= 255 * pi->color1[1];
	pi->c1RGBA[2]	= 255 * pi->color1[2];
	pi->c1RGBA[3]	= 255;

	/* view angles */
	copyv3(viewAngles, pi->viewAngles);

	/* move angles */
	copyv3(moveAngles, pi->moveAngles);

	if(pi->newModel){
		pi->newModel = qfalse;

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);
		pi->legs.yawAngle = viewAngles[YAW];
		pi->legs.yawing = qfalse;

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);
		pi->torso.yawAngle = viewAngles[YAW];
		pi->torso.yawing = qfalse;

		if(weaponNumber != -1){
			pi->weapon = weaponNumber;
			pi->currentWeapon = weaponNumber;
			pi->lastWeapon = weaponNumber;
			pi->pendingWeapon = -1;
			pi->weaponTimer = 0;
			UI_PlayerInfo_SetWeapon(pi, pi->weapon);
		}

		return;
	}

	/* weapon */
	if(weaponNumber == -1){
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
	}else if(weaponNumber != Wnone){
		pi->pendingWeapon = weaponNumber;
		pi->weaponTimer = dp_realtime + UI_TIMER_WEAPON_DELAY;
	}
	weaponNum = pi->lastWeapon;
	pi->weapon = weaponNum;

	if(torsoAnim == BOTH_DEATH1 || legsAnim == BOTH_DEATH1){
		torsoAnim = legsAnim = BOTH_DEATH1;
		pi->weapon = pi->currentWeapon = Wnone;
		UI_PlayerInfo_SetWeapon(pi, pi->weapon);

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);

		return;
	}

	/* leg animation */
	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;
	if(legsAnim != LEGS_JUMP &&
	   (currentAnim == LEGS_JUMP || currentAnim == LEGS_LAND))
		pi->pendingLegsAnim = legsAnim;
	else if(legsAnim != currentAnim){
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);
	}

	/* torso animation */
	if(torsoAnim == TORSO_STAND || torsoAnim == TORSO_STAND2){
		if(weaponNum == Wnone || weaponNum == Wmelee)
			torsoAnim = TORSO_STAND2;
		else
			torsoAnim = TORSO_STAND;
	}

	if(torsoAnim == TORSO_ATTACK || torsoAnim == TORSO_ATTACK2){
		if(weaponNum == Wnone || weaponNum == Wmelee)
			torsoAnim = TORSO_ATTACK2;
		else
			torsoAnim = TORSO_ATTACK;
		pi->muzzleFlashTime = dp_realtime + UI_TIMER_MUZZLE_FLASH;
		/* FIXME play firing sound here */
	}

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if(weaponNum != pi->currentWeapon || currentAnim == TORSO_RAISE ||
	   currentAnim == TORSO_DROP)
		pi->pendingTorsoAnim = torsoAnim;
	else if((currentAnim == TORSO_GESTURE ||
		 currentAnim == TORSO_ATTACK) && (torsoAnim != currentAnim))
		pi->pendingTorsoAnim = torsoAnim;
	else if(torsoAnim != currentAnim){
		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);
	}
}
