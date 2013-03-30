/* setup all the parameters (position, angle, etc) for a 3D rendering */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "local.h"

enum { Focusdistance = 512 };

/*
 * Model Testing
 *
 * The viewthing and gun positioning tools from Q2 have been integrated and
 * enhanced into a single model testing facility.
 *
 * Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".
 *
 * The names must be the full pathname after the basedir, like
 * "vis/models/weapons/v_launch/tris.iqm" or "players/male/tris.iqm"
 *
 * Testmodel will create a fake entity 100 units in front of the current view
 * position, directly facing the viewer.  It will remain immobile, so you can
 * move around it to view it from different angles.
 *
 * Testgun will cause the model to follow the player around and supress the real
 * view weapon model.  The default frame 0 of most guns is completely off screen,
 * so you will probably have to cycle a couple frames to see it.
 *
 * "testmodelnextframe", "testmodelprevframe", "testmodelnextskin", and 
 * "testmodelprevskin" commands will change the frame or skin of the 
 * testmodel.  These are bound to F5, F6, F7, and F8 in q3default.cfg.
 *
 * If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
 * you adjust the positioning.
 *
 * Note that none of the model testing features update while the game is paused, so
 * it may be convenient to test with deathmatch set to 1 so that bringing down the
 * console doesn't pause the game.
 */

/*
 * Creates an entity in front of the current position, which
 * can then be moved around
 */
void
CG_TestModel_f(void)
{
	Vec3 angles;

	memset(&cg.testModelEntity, 0, sizeof(cg.testModelEntity));
	if(trap_Argc() < 2){
		comprintf("Usage: testmodel [path to model]\n");
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv(1), MAX_QPATH);
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);

	if(trap_Argc() == 3){
		cg.testModelEntity.backlerp = atof(CG_Argv(2));
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if(!cg.testModelEntity.hModel){
		CG_Printf("Can't register model\n");
		return;
	}

	saddv3(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0],
		cg.testModelEntity.origin);

	angles[PITCH]	= 0;
	angles[YAW]	= 180 + cg.refdefViewAngles[1];
	angles[ROLL]	= 0;

	eulertoaxis(angles, cg.testModelEntity.axis);
	cg.testGun = qfalse;
}

/* Replaces the current view weapon with the given model */
void
CG_TestGun_f(void)
{
	if(trap_Argc() < 2){
		comprintf("Usage: testgun [path to model]\n");
		return;
	}
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK |
				      RF_FIRST_PERSON;
}

void
CG_TestModelNextFrame_f(void)
{
	cg.testModelEntity.frame++;
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void
CG_TestModelPrevFrame_f(void)
{
	cg.testModelEntity.frame--;
	if(cg.testModelEntity.frame < 0)
		cg.testModelEntity.frame = 0;
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void
CG_TestModelNextSkin_f(void)
{
	cg.testModelEntity.skinNum++;
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

void
CG_TestModelPrevSkin_f(void)
{
	cg.testModelEntity.skinNum--;
	if(cg.testModelEntity.skinNum < 0)
		cg.testModelEntity.skinNum = 0;
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

static void
addtestmodel(void)
{
	uint i;

	/* re-register the model, because the level may have changed */
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);
	if(!cg.testModelEntity.hModel){
		CG_Printf("Can't register model\n");
		return;
	}
	/* if testing a gun, set the origin relative to the view origin */
	if(cg.testGun){
		copyv3(cg.refdef.vieworg, cg.testModelEntity.origin);
		copyv3(cg.refdef.viewaxis[0], cg.testModelEntity.axis[0]);
		copyv3(cg.refdef.viewaxis[1], cg.testModelEntity.axis[1]);
		copyv3(cg.refdef.viewaxis[2], cg.testModelEntity.axis[2]);

		/* allow the position to be adjusted */
		for(i=0; i<3; i++){
			cg.testModelEntity.origin[i] +=
				cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] +=
				cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] +=
				cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}
	trap_R_AddRefEntityToScene(&cg.testModelEntity);
}

void
CG_TestLight_f(void)
{
	Scalar *v;
	
	v = cg.testlightorigs[cg.ntestlights % Maxtestlights];
	saddv3(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], v);
	cg.ntestlights++;
}

static void
addtestlights(void)
{
	uint i;
	Scalar *v;
	
	for(i = 0; i < cg.ntestlights && i < Maxtestlights; i++){
		v = cg.testlightorigs[i];
		trap_R_AddLightToScene(v, 200.0, 1.0, 1.0, 1.0);
	}
}

/* 
 * calculate size of 3D view.
 * Sets the coordinates of the rendered window 
 */
static void
calcvrect(void)
{
	int size;

	/* the intermission should allways be full screen */
	if(cg.snap->ps.pm_type == PM_INTERMISSION)
		size = 100;
	else{
		/* bound normal viewsize */
		if(cg_viewsize.integer < 30){
			trap_cvarsetstr ("cg_viewsize","30");
			size = 30;
		}else if(cg_viewsize.integer > 100){
			trap_cvarsetstr ("cg_viewsize","100");
			size = 100;
		}else
			size = cg_viewsize.integer;

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

/* this causes a compiler bug on mac MrC compiler */
static void
stepoffset(void)
{
	int timeDelta;

	/* smooth out stair climbing */
	timeDelta = cg.time - cg.stepTime;
	if(timeDelta < STEP_TIME)
		cg.refdef.vieworg[2] -= cg.stepChange
					* (STEP_TIME - timeDelta) / STEP_TIME;
}

static void
offset1stpersonview(void)
{
	float	*origin, *angles;
	float bob, ratio, delta, speed, f;
	Vec3 predictedVelocity;
	int timeDelta;

	if(cg.snap->ps.pm_type == PM_INTERMISSION)
		return;

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	/* if dead, fix the angle and don't add any kick */
	if(cg.snap->ps.stats[STAT_HEALTH] <= 0){
		angles[ROLL] = 40;
		angles[PITCH]	= -15;
		angles[YAW]	= cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}
	/* add angles based on damage kick */
	if(cg.damageTime){
		ratio = cg.time - cg.damageTime;
		if(ratio < DAMAGE_DEFLECT_TIME){
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH]	+= ratio * cg.v_dmg_pitch;
			angles[ROLL]	+= ratio * cg.v_dmg_roll;
		}else{
			ratio = 1.0 - (ratio -
				 DAMAGE_DEFLECT_TIME) 
				 / DAMAGE_RETURN_TIME;
			if(ratio > 0){
				angles[PITCH]	+= ratio * cg.v_dmg_pitch;
				angles[ROLL]	+= ratio * cg.v_dmg_roll;
			}
		}
	}

	/* add pitch based on fall kick */
#if 0
	ratio = (cg.time - cg.landTime) / FALL_TIME;
	if(ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	/* add angles based on velocity */
	copyv3(cg.predictedPlayerState.velocity, predictedVelocity);

	delta = dotv3 (predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = dotv3 (predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	/* add angles based on bob */

	/* make sure the bob is visible even at low speeds */
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if(cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

	/* add view height */
	origin[2] += cg.predictedPlayerState.viewheight;

	/* smooth out duck height changes */
	timeDelta = cg.time - cg.duckTime;
	if(timeDelta < DUCK_TIME)
		cg.refdef.vieworg[2] -= cg.duckChange
					* (DUCK_TIME - timeDelta) / DUCK_TIME;

	/* add bob height */
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if(bob > 6)
		bob = 6;

	origin[2] += bob;

	/* add fall height */
	delta = cg.time - cg.landTime;
	if(delta < LAND_DEFLECT_TIME){
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	}else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME){
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	/* add step offset */
	stepoffset();

	/* pivot the eye based on a neck length */
#if 0
	{
#define NECK_LENGTH 8
		Vec3 forward, up;

		cg.refdef.vieworg[2] -= NECK_LENGTH;
		anglev3s(cg.refdefViewAngles, forward, NULL, up);
		saddv3(cg.refdef.vieworg, 3, forward, cg.refdef.vieworg);
		saddv3(cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg);
	}
#endif
}

static void
offset3rdpersonview(void)
{
	const Vec3 mins = { -4, -4, -4 };
	const Vec3 maxs = { 4, 4, 4 };
	Vec3 forward, right, up;
	Vec3 view;
	Vec3 focusAngles;
	Trace trace;
	Vec3 focusPoint;
	float focusDist;
	float forwardScale, sideScale, upscale;

	//cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;
	copyv3(cg.refdefViewAngles, focusAngles);
	/* if dead, look at killer */
	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0){
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] =
			cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	anglev3s(focusAngles, forward, NULL, NULL);
	saddv3(cg.refdef.vieworg, Focusdistance, forward, focusPoint);
	copyv3(cg.refdef.vieworg, view);
	anglev3s(cg.refdefViewAngles, forward, right, up);
	forwardScale = cos(DEG2RAD(cg_thirdpersonyaw.value)) - sin(DEG2RAD(cg_thirdpersonpitch.value));
	sideScale = sin(DEG2RAD(cg_thirdpersonyaw.value));
	upscale = sin(-DEG2RAD(cg_thirdpersonpitch.value));
	saddv3(view, -cg_thirdpersonrange.value * forwardScale, forward, view);
	saddv3(view, -cg_thirdpersonrange.value * sideScale, right, view);
	saddv3(view, -cg_thirdpersonrange.value * upscale, up, view);
	/* 
	 * trace a ray from the origin to the viewpoint to make sure
	 * the view isn't in a solid block.  Use an 8 by 8 block to
	 * prevent the view from near clipping anything
	 */
	if(!cg_cameraMode.integer){
		CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, view,
			cg.predictedPlayerState.clientNum,
			MASK_SOLID);
		if(trace.fraction != 1.0){
			copyv3(trace.endpos, view);
			view[2] += (1.0 - trace.fraction) * 32;
			/* 
			 * try another trace to this position, because
			 * a tunnel may have the ceiling close enogh
			 * that this is poking out
			 */
			CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, view,
				cg.predictedPlayerState.clientNum,
				MASK_SOLID);
			copyv3(trace.endpos, view);
		}
	}
	copyv3(view, cg.refdef.vieworg);

	/* select pitch to look at focus point from vieword */
	subv3(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if(focusDist < 1)
		focusDist = 1;	/* should never happen */
	cg.refdefViewAngles[PITCH] = -180 / M_PI*atan2(focusPoint[2],
						       focusDist);
	cg.refdefViewAngles[YAW] -= cg_thirdpersonyaw.value;
}

void
CG_ZoomDown_f(void)
{
	if(cg_zoomtoggle.integer)
		cg.zoomed ^= qtrue;
	else
		cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}

void
CG_ZoomUp_f(void)
{
	if(cg_zoomtoggle.integer)
		return;
	if(!cg.zoomed)
		return;
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}

/* Fixed fov at intermissions, otherwise account for fov variable and zooms. */
#define Waveamp	1
#define Wavefreq	0.4
static int
calcfov(void)
{
	float x, phase, v, fovx, fovy, zoomfov, f;
	int contents, inwater;

	if(cg.predictedPlayerState.pm_type == PM_INTERMISSION)
		/* if in intermission, use a fixed value */
		fovx = 90;
	else{
		/* user selectable */
		if(cgs.dmflags & DF_FIXED_FOV)
			/* dmflag to prevent wide fov for all clients */
			fovx = 90;
		else{
			fovx = cg_fov.value;
			if(fovx < 1)
				fovx = 1;
			else if(fovx > 160)
				fovx = 160;
		}

		/* account for zooms */
		zoomfov = cg_zoomFov.value;
		if(zoomfov < 1)
			zoomfov = 1;
		else if(zoomfov > 160)
			zoomfov = 160;

		if(cg.zoomed){
			if(cg_zoomintime.value <= 0.001f)
				f = (cg.time - cg.zoomTime) / 0.001f;
			else
				f = (cg.time - cg.zoomTime) / cg_zoomintime.value;
			if(f > 1.0)
				fovx = zoomfov;
			else
				fovx = fovx + f * (zoomfov - fovx);
		}else{
			if(cg_zoomouttime.value <= 0.001f)
				f = (cg.time - cg.zoomTime) / 0.001f;
			else
				f = (cg.time - cg.zoomTime) / cg_zoomouttime.value;
			if(f <= 1.0)
				fovx = zoomfov + f * (fovx - zoomfov);
		}
	}

	x = cg.refdef.width / tan(fovx / 360 * M_PI);
	fovy = atan2(cg.refdef.height, x);
	fovy = fovy * 360 / M_PI;

	/* warp if underwater */
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		phase = cg.time / 1000.0 * Wavefreq * M_PI * 2;
		v = Waveamp * sin(phase);
		fovx	+= v;
		fovy	-= v;
		inwater = qtrue;
	}else
		inwater = qfalse;

	cg.refdef.fov_x = fovx;
	cg.refdef.fov_y = fovy;

	if(!cg.zoomed)
		cg.zoomSensitivity = 1;
	else
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;

	return inwater;
}

/* damage visuals on screen */
static void
dmgblendblob(void)
{
	int	t;
	int	maxTime;
	Refent ent;

	if(!cg.damageValue)
		return;

	/* ragePro systems can't fade blends, so don't obscure the screen */
	if(cgs.glconfig.hardwareType == GLHW_RAGEPRO)
		return;

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if(t <= 0 || t >= maxTime)
		return;

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	saddv3(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
	saddv3(ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin);
	saddv3(ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin);

	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 200 * (1.0 - ((float)t / maxTime));
	trap_R_AddRefEntityToScene(&ent);
}


/* Sets cg.refdef view values */
static int
calcviewvals(void)
{
	Playerstate *ps;

	ps = &cg.predictedPlayerState;
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	calcvrect();
/*
 *      if (cg.cameraMode) {
 *              Vec3 origin, angles;
 *              if (trap_getCameraInfo(cg.time, &origin, &angles)) {
 *                      copyv3(origin, cg.refdef.vieworg);
 *                      angles[ROLL] = 0;
 *                      copyv3(angles, cg.refdefViewAngles);
 *                      eulertoaxis( cg.refdefViewAngles, cg.refdef.viewaxis );
 *                      return calcfov();
 *              } else {
 *                      cg.cameraMode = qfalse;
 *              }
 *      }
 */
	/* intermission view */
	if(ps->pm_type == PM_INTERMISSION){
		copyv3(ps->origin, cg.refdef.vieworg);
		copyv3(ps->viewangles, cg.refdefViewAngles);
		eulertoaxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		return calcfov();
	}

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1]);

	copyv3(ps->origin, cg.refdef.vieworg);
	if(cg_thirdperson.integer != 2)
		copyv3(ps->viewangles, cg.refdefViewAngles);
	if(cg_thirdperson.integer == 3)
		setv3(ps->viewangles, 0, 0, 0);

	if(cg_cameraOrbit.integer)
		if(cg.time > cg.nextOrbitTime){
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdpersonyaw.value += cg_cameraOrbit.value;
		}
	/* add error decay */
	if(cg_errorDecay.value > 0){
		int t;
		float f;

		t = cg.time - cg.predictedErrorTime;
		f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if(f > 0 && f < 1)
			saddv3(cg.refdef.vieworg, f, cg.predictedError,
				cg.refdef.vieworg);
		else
			cg.predictedErrorTime = 0;
	}

	if(cg.renderingThirdPerson)
		/* back away from character */
		offset3rdpersonview();
	else
		/* offset for local bobbing and kicks */
		offset1stpersonview();

	/* position eye relative to origin */
	eulertoaxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	if(cg.hyperspace)
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	return calcfov();
}

/* warning sounds when powerup is wearing off */
static void
poweruptimersounds(void)
{
	int i;
	int t;

	/* powerup timers going away */
	for(i = 0; i < MAX_POWERUPS; i++){
		t = cg.snap->ps.powerups[i];
		if(t <= cg.time)
			continue;
		if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
			continue;
		if((t - cg.time) / POWERUP_BLINK_TIME !=
		   (t - cg.oldTime) / POWERUP_BLINK_TIME)
			trap_sndstartsound(NULL, cg.snap->ps.clientNum, CHAN_ITEM,
				cgs.media.wearOffSound);
	}
}

void
CG_AddBufferedSound(Handle sfx)
{
	if(!sfx)
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if(cg.soundBufferIn == cg.soundBufferOut)
		cg.soundBufferOut++;
}

static void
CG_PlayBufferedSounds(void)
{
	if(cg.soundTime < cg.time)
		if(cg.soundBufferOut != cg.soundBufferIn &&
		   cg.soundBuffer[cg.soundBufferOut]){
			trap_sndstartlocalsound(cg.soundBuffer[cg.soundBufferOut],
				CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut =
				(cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
}

/* 
  * Generates and draws a game scene and status information at the
  * given time.  
  */
void
CG_DrawActiveFrame(int serverTime, Stereoframe stereoView,
		   qbool demoPlayback)
{
	int inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	CG_UpdateCvars();

	/* if we are only updating the screen as a loading
	 * pacifier, don't even try to read snapshots */
	if(cg.infoScreenText[0] != 0){
		CG_DrawInformation();
		return;
	}

	trap_sndclearloops(qfalse);
	trap_R_ClearScene();
	
	/* 
	 * if we haven't received any snapshots yet, all
	 * we can draw is the information screen 
	 */
	CG_ProcessSnapshots();
	if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)){
		CG_DrawInformation();
		return;
	}

	/* let the client system know what our weapon and zoom settings are */
	trap_SetUserCmdValue(cg.weapsel[WSpri], cg.weapsel[WSsec], cg.zoomSensitivity);

	cg.clientFrame++;

	CG_PredictPlayerState();	/* update cg.predictedPlayerState */

	/* decide on third person view */
	cg.renderingThirdPerson = cg_thirdperson.integer ||
				  (cg.snap->ps.stats[STAT_HEALTH] <= 0);

	inwater = calcviewvals();	/* build cg.refdef */

	if(!cg.renderingThirdPerson)
		dmgblendblob();

	/* build the render lists */
	if(!cg.hyperspace){
		CG_AddPacketEntities();	/* after calcViewValues, so 
							 * predicted player state 
							 * is correct */
		CG_AddMarks();
		CG_AddParticles ();
		CG_AddLocalEntities();
	}
	CG_AddViewWeapon(&cg.predictedPlayerState, WSpri);
	CG_AddViewWeapon(&cg.predictedPlayerState, WSsec);
	CG_AddViewWeapon(&cg.predictedPlayerState, WShook);
	CG_PlayBufferedSounds();
	CG_PlayBufferedVoiceChats();

	/* finish up the rest of the refdef */
	if(cg.testModelEntity.hModel)
		addtestmodel();
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));
	
	if(cg.ntestlights > 0)
		addtestlights();

	poweruptimersounds();

	/* update audio positions */
	trap_sndrespatialize(cg.snap->ps.clientNum, cg.refdef.vieworg,
		cg.refdef.viewaxis,
		inwater);

	/* make sure the lagometerSample and frame timing isn't done twice when in stereo */
	if(stereoView != STEREO_RIGHT){
		cg.frametime = cg.time - cg.oldTime;
		if(cg.frametime < 0)
			cg.frametime = 0;
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
	}
	if(cg_timescale.value != cg_timescaleFadeEnd.value){
		if(cg_timescale.value < cg_timescaleFadeEnd.value){
			cg_timescale.value += cg_timescaleFadeSpeed.value *
					      ((float)cg.frametime) / 1000;
			if(cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}else{
			cg_timescale.value -= cg_timescaleFadeSpeed.value *
					      ((float)cg.frametime) / 1000;
			if(cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if(cg_timescaleFadeSpeed.value)
			trap_cvarsetstr("timescale", va("%f", cg_timescale.value));
	}

	/* actually issue the rendering calls */
	CG_DrawActive(stereoView);

	if(cg_stats.integer)
		CG_Printf("cg.clientFrame:%i\n", cg.clientFrame);
}
