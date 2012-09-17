/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * cg_effects.c -- these functions generate localentities, usually as a result
 * of event processing */

#include "local.h"


/*
 * CG_BubbleTrail
 *
 * Bullets shot underwater
 */
void
CG_BubbleTrail(Vec3 start, Vec3 end, float spacing)
{
	Vec3	move;
	Vec3	vec;
	float	len;
	int	i;

	if(cg_noProjectileTrail.integer)
		return;

	vec3copy (start, move);
	vec3sub (end, start, vec);
	len = vec3normalize (vec);

	/* advance a random amount first */
	i = rand() % (int)spacing;
	vec3ma(move, i, vec, move);

	vec3scale (vec, spacing, vec);

	for(; i < len; i += spacing){
		localEntity_t *le;
		refEntity_t *re;

		le = CG_AllocLocalEntity();
		le->leFlags	= LEF_PUFF_DONT_SCALE;
		le->leType	= LE_MOVE_SCALE_FADE;
		le->startTime	= cg.time;
		le->endTime	= cg.time + 1000 + random() * 250;
		le->lifeRate	= 1.0 / (le->endTime - le->startTime);

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType	= RT_SPRITE;
		re->rotation	= 0;
		re->radius	= 3;
		re->customShader	= cgs.media.waterBubbleShader;
		re->shaderRGBA[0]	= 0xff;
		re->shaderRGBA[1]	= 0xff;
		re->shaderRGBA[2]	= 0xff;
		re->shaderRGBA[3]	= 0xff;

		le->color[3] = 1.0;

		le->pos.trType	= TR_LINEAR;
		le->pos.trTime	= cg.time;
		vec3copy(move, le->pos.trBase);
		le->pos.trDelta[0]	= crandom()*5;
		le->pos.trDelta[1]	= crandom()*5;
		le->pos.trDelta[2]	= crandom()*5 + 6;

		vec3add (move, vec, move);
	}
}

/*
 * CG_SmokePuff
 *
 * Adds a smoke puff or blood trail localEntity.
 */
localEntity_t *
CG_SmokePuff(const Vec3 p, const Vec3 vel,
	     float radius,
	     float r, float g, float b, float a,
	     float duration,
	     int startTime,
	     int fadeInTime,
	     int leFlags,
	     qhandle_t hShader)
{
	static int seed = 0x92;
	localEntity_t *le;
	refEntity_t *re;
/*	int fadeInTime = startTime + duration / 2; */

	le = CG_AllocLocalEntity();
	le->leFlags	= leFlags;
	le->radius	= radius;

	re = &le->refEntity;
	re->rotation	= Q_random(&seed) * 360;
	re->radius	= radius;
	re->shaderTime	= startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime	= startTime;
	le->fadeInTime	= fadeInTime;
	le->endTime = startTime + duration;
	if(fadeInTime > startTime)
		le->lifeRate = 1.0 / (le->endTime - le->fadeInTime);
	else
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
	le->color[0]	= r;
	le->color[1]	= g;
	le->color[2]	= b;
	le->color[3]	= a;


	le->pos.trType	= TR_LINEAR;
	le->pos.trTime	= startTime;
	vec3copy(vel, le->pos.trDelta);
	vec3copy(p, le->pos.trBase);

	vec3copy(p, re->origin);
	re->customShader = hShader;

	/* rage pro can't alpha fade, so use a different shader */
	if(cgs.glconfig.hardwareType == GLHW_RAGEPRO){
		re->customShader	= cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0]	= 0xff;
		re->shaderRGBA[1]	= 0xff;
		re->shaderRGBA[2]	= 0xff;
		re->shaderRGBA[3]	= 0xff;
	}else{
		re->shaderRGBA[0]	= le->color[0] * 0xff;
		re->shaderRGBA[1]	= le->color[1] * 0xff;
		re->shaderRGBA[2]	= le->color[2] * 0xff;
		re->shaderRGBA[3]	= 0xff;
	}

	re->reType	= RT_SPRITE;
	re->radius	= le->radius;

	return le;
}

/*
 * CG_SpawnEffect
 *
 * Player teleporting in or out
 */
void
CG_SpawnEffect(Vec3 org)
{
	localEntity_t *le;
	refEntity_t *re;

	le = CG_AllocLocalEntity();
	le->leFlags	= 0;
	le->leType	= LE_FADE_RGB;
	le->startTime	= cg.time;
	le->endTime	= cg.time + 500;
	le->lifeRate	= 1.0 / (le->endTime - le->startTime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

#ifndef MISSIONPACK
	re->customShader = cgs.media.teleportEffectShader;
#endif
	re->hModel = cgs.media.teleportEffectModel;
	axisclear(re->axis);

	vec3copy(org, re->origin);
#ifdef MISSIONPACK
	re->origin[2] += 16;
#else
	re->origin[2] -= 24;
#endif
}


#ifdef MISSIONPACK
/*
 * CG_LightningBoltBeam
 */
void
CG_LightningBoltBeam(Vec3 start, Vec3 end)
{
	localEntity_t *le;
	refEntity_t *beam;

	le = CG_AllocLocalEntity();
	le->leFlags	= 0;
	le->leType	= LE_SHOWREFENTITY;
	le->startTime	= cg.time;
	le->endTime	= cg.time + 50;

	beam = &le->refEntity;

	vec3copy(start, beam->origin);
	/* this is the end point */
	vec3copy(end, beam->oldorigin);

	beam->reType = RT_LIGHTNING;
	beam->customShader = cgs.media.lightningShader;
}

/*
 * CG_KamikazeEffect
 */
void
CG_KamikazeEffect(Vec3 org)
{
	localEntity_t *le;
	refEntity_t *re;

	le = CG_AllocLocalEntity();
	le->leFlags	= 0;
	le->leType	= LE_KAMIKAZE;
	le->startTime	= cg.time;
	le->endTime	= cg.time + 3000;	/* 2250; */
	le->lifeRate	= 1.0 / (le->endTime - le->startTime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	vec3clear(le->angles.trBase);

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.kamikazeEffectModel;

	vec3copy(org, re->origin);

}

/*
 * CG_ObeliskExplode
 */
void
CG_ObeliskExplode(Vec3 org, int entityNum)
{
	localEntity_t *le;
	Vec3 origin;

	/* create an explosion */
	vec3copy(org, origin);
	origin[2] += 64;
	le = CG_MakeExplosion(origin, vec3_origin,
		cgs.media.dishFlashModel,
		cgs.media.rocketExplosionShader,
		600, qtrue);
	le->light = 300;
	le->lightColor[0]	= 1;
	le->lightColor[1]	= 0.75;
	le->lightColor[2]	= 0.0;
}

/*
 * CG_ObeliskPain
 */
void
CG_ObeliskPain(Vec3 org)
{
	float r;
	sfxHandle_t sfx;

	/* hit sound */
	r = rand() & 3;
	if(r < 2)
		sfx = cgs.media.obeliskHitSound1;
	else if(r == 2)
		sfx = cgs.media.obeliskHitSound2;
	else
		sfx = cgs.media.obeliskHitSound3;
	trap_S_StartSound (org, ENTITYNUM_NONE, CHAN_BODY, sfx);
}


/*
 * CG_InvulnerabilityImpact
 */
void
CG_InvulnerabilityImpact(Vec3 org, Vec3 angles)
{
	localEntity_t	*le;
	refEntity_t	*re;
	int r;
	sfxHandle_t	sfx;

	le = CG_AllocLocalEntity();
	le->leFlags	= 0;
	le->leType	= LE_INVULIMPACT;
	le->startTime	= cg.time;
	le->endTime	= cg.time + 1000;
	le->lifeRate	= 1.0 / (le->endTime - le->startTime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityImpactModel;

	vec3copy(org, re->origin);
	euler2axis(angles, re->axis);

	r = rand() & 3;
	if(r < 2)
		sfx = cgs.media.invulnerabilityImpactSound1;
	else if(r == 2)
		sfx = cgs.media.invulnerabilityImpactSound2;
	else
		sfx = cgs.media.invulnerabilityImpactSound3;
	trap_S_StartSound (org, ENTITYNUM_NONE, CHAN_BODY, sfx);
}

/*
 * CG_InvulnerabilityJuiced
 */
void
CG_InvulnerabilityJuiced(Vec3 org)
{
	localEntity_t *le;
	refEntity_t *re;
	Vec3 angles;

	le = CG_AllocLocalEntity();
	le->leFlags	= 0;
	le->leType	= LE_INVULJUICED;
	le->startTime	= cg.time;
	le->endTime	= cg.time + 10000;
	le->lifeRate	= 1.0 / (le->endTime - le->startTime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->hModel = cgs.media.invulnerabilityJuicedModel;

	vec3copy(org, re->origin);
	vec3clear(angles);
	euler2axis(angles, re->axis);

	trap_S_StartSound (org, ENTITYNUM_NONE, CHAN_BODY,
		cgs.media.invulnerabilityJuicedSound);
}

#endif

/*
 * CG_ScorePlum
 */
void
CG_ScorePlum(int client, Vec3 org, int score)
{
	localEntity_t *le;
	refEntity_t *re;
	Vec3 angles;
	static Vec3 lastPos;

	/* only visualize for the client that scored */
	if(client != cg.predictedPlayerState.clientNum ||
	   cg_scorePlum.integer == 0)
		return;

	le = CG_AllocLocalEntity();
	le->leFlags	= 0;
	le->leType	= LE_SCOREPLUM;
	le->startTime	= cg.time;
	le->endTime	= cg.time + 4000;
	le->lifeRate	= 1.0 / (le->endTime - le->startTime);


	le->color[0]	= le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius	= score;

	vec3copy(org, le->pos.trBase);
	if(org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20)
		le->pos.trBase[2] -= 20;

	/* CG_Printf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)vec3dist(org, lastPos)); */
	vec3copy(org, lastPos);


	re = &le->refEntity;

	re->reType	= RT_SPRITE;
	re->radius	= 16;

	vec3clear(angles);
	euler2axis(angles, re->axis);
}


/*
 * CG_MakeExplosion
 */
localEntity_t *
CG_MakeExplosion(Vec3 origin, Vec3 dir,
		 qhandle_t hModel, qhandle_t shader,
		 int msec, qbool isSprite)
{
	float	ang;
	localEntity_t *ex;
	int	offset;
	Vec3	tmpVec, newOrigin;

	if(msec <= 0)
		CG_Error("CG_MakeExplosion: msec = %i", msec);

	/* skew the time a bit so they aren't all in sync */
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if(isSprite){
		ex->leType = LE_SPRITE_EXPLOSION;

		/* randomly rotate sprite orientation */
		ex->refEntity.rotation = rand() % 360;
		vec3scale(dir, 16, tmpVec);
		vec3add(tmpVec, origin, newOrigin);
	}else{
		ex->leType = LE_EXPLOSION;
		vec3copy(origin, newOrigin);

		/* set axis with random rotate */
		if(!dir)
			axisclear(ex->refEntity.axis);
		else{
			ang = rand() % 360;
			vec3copy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, ang);
		}
	}

	ex->startTime	= cg.time - offset;
	ex->endTime	= ex->startTime + msec;

	/* bias the time so all shader effects start correctly */
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	/* set origin */
	vec3copy(newOrigin, ex->refEntity.origin);
	vec3copy(newOrigin, ex->refEntity.oldorigin);

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
 * CG_Bleed
 *
 * This is the spurt of blood when a character gets hit
 */
void
CG_Bleed(Vec3 origin, int entityNum)
{
	localEntity_t *ex;

	if(!cg_blood.integer)
		return;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	ex->startTime	= cg.time;
	ex->endTime	= ex->startTime + 500;

	vec3copy (origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation	= rand() % 360;
	ex->refEntity.radius	= 24;

	ex->refEntity.customShader = cgs.media.bloodExplosionShader;

	/* don't show player's own blood in view */
	if(entityNum == cg.snap->ps.clientNum)
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
}



/*
 * CG_LaunchGib
 */
void
CG_LaunchGib(Vec3 origin, Vec3 velocity, qhandle_t hModel)
{
	localEntity_t *le;
	refEntity_t *re;

	le	= CG_AllocLocalEntity();
	re	= &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime	= cg.time;
	le->endTime	= le->startTime + 5000 + random() * 3000;

	vec3copy(origin, re->origin);
	axiscopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	vec3copy(origin, le->pos.trBase);
	vec3copy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}

/*
 * CG_GibPlayer
 *
 * Generated a bunch of gibs launching out from the bodies location
 */
#define GIB_VELOCITY	250
#define GIB_JUMP	250
void
CG_GibPlayer(Vec3 playerOrigin)
{
	Vec3 origin, velocity;

	if(!cg_blood.integer)
		return;

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	if(rand() & 1)
		CG_LaunchGib(origin, velocity, cgs.media.gibSkull);
	else
		CG_LaunchGib(origin, velocity, cgs.media.gibBrain);

	/* allow gibs to be turned off for speed */
	if(!cg_gibs.integer)
		return;

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibAbdomen);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibArm);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibChest);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibFist);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibFoot);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibForearm);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibIntestine);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibLeg);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*GIB_VELOCITY;
	velocity[1]	= crandom()*GIB_VELOCITY;
	velocity[2]	= GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibLeg);
}

/*
 * CG_LaunchGib
 */
void
CG_LaunchExplode(Vec3 origin, Vec3 velocity, qhandle_t hModel)
{
	localEntity_t *le;
	refEntity_t *re;

	le	= CG_AllocLocalEntity();
	re	= &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime	= cg.time;
	le->endTime	= le->startTime + 10000 + random() * 6000;

	vec3copy(origin, re->origin);
	axiscopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	vec3copy(origin, le->pos.trBase);
	vec3copy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.1f;

	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

#define EXP_VELOCITY	100
#define EXP_JUMP	150
/*
 * CG_GibPlayer
 *
 * Generated a bunch of gibs launching out from the bodies location
 */
void
CG_BigExplode(Vec3 playerOrigin)
{
	Vec3 origin, velocity;

	if(!cg_blood.integer)
		return;

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*EXP_VELOCITY;
	velocity[1]	= crandom()*EXP_VELOCITY;
	velocity[2]	= EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*EXP_VELOCITY;
	velocity[1]	= crandom()*EXP_VELOCITY;
	velocity[2]	= EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*EXP_VELOCITY*1.5;
	velocity[1]	= crandom()*EXP_VELOCITY*1.5;
	velocity[2]	= EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*EXP_VELOCITY*2.0;
	velocity[1]	= crandom()*EXP_VELOCITY*2.0;
	velocity[2]	= EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	vec3copy(playerOrigin, origin);
	velocity[0]	= crandom()*EXP_VELOCITY*2.5;
	velocity[1]	= crandom()*EXP_VELOCITY*2.5;
	velocity[2]	= EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);
}