/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

#define DLIGHT_AT_RADIUS 16
/* at the edge of a dlight's influence, this amount of light will be added */

#define DLIGHT_MINIMUM_RADIUS 16
/* never calculate a range less than this to prevent huge light numbers */


/*
 * R_TransformDlights
 *
 * Transforms the origins of an array of dlights.
 * Used by both the front end (for DlightBmodel) and
 * the back end (before doing the lighting calculation)
 */
void
R_TransformDlights(int count, dlight_t *dl, orientationr_t *or)
{
	int i;
	Vec3 temp;

	for(i = 0; i < count; i++, dl++){
		subv3(dl->origin, or->origin, temp);
		dl->transformed[0]	= dotv3(temp, or->axis[0]);
		dl->transformed[1]	= dotv3(temp, or->axis[1]);
		dl->transformed[2]	= dotv3(temp, or->axis[2]);
	}
}

/*
 * R_DlightBmodel
 *
 * Determine which dynamic lights may effect this bmodel
 */
void
R_DlightBmodel(bmodel_t *bmodel)
{
	int	i, j;
	dlight_t        *dl;
	int	mask;
	msurface_t *surf;

	/* transform all the lights */
	R_TransformDlights(tr.refdef.num_dlights, tr.refdef.dlights, &tr.or);

	mask = 0;
	for(i=0; i<tr.refdef.num_dlights; i++){
		dl = &tr.refdef.dlights[i];

		/* see if the point is close enough to the bounds to matter */
		for(j = 0; j < 3; j++){
			if(dl->transformed[j] - bmodel->bounds[1][j] > dl->radius){
				break;
			}
			if(bmodel->bounds[0][j] - dl->transformed[j] > dl->radius){
				break;
			}
		}
		if(j < 3){
			continue;
		}

		/* we need to check this light */
		mask |= 1 << i;
	}

	tr.currentEntity->needDlights = (mask != 0);

	/* set the dlight bits in all the surfaces */
	for(i = 0; i < bmodel->numSurfaces; i++){
		surf = tr.world->surfaces + bmodel->firstSurface + i;

		if(*surf->data == SF_FACE){
			((srfSurfaceFace_t*)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		}else if(*surf->data == SF_GRID){
			((srfGridMesh_t*)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		}else if(*surf->data == SF_TRIANGLES){
			((srfTriangles_t*)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		}
	}
}


/*
 *
 * LIGHT SAMPLING
 *
 */

extern cvar_t *r_ambientScale;
extern cvar_t *r_directedScale;
extern cvar_t *r_debugLight;

/*
 * R_SetupEntityLightingGrid
 *
 */
static void
R_SetupEntityLightingGrid(trRefEntity_t *ent, world_t *world)
{
	Vec3	lightOrigin;
	int	pos[3];
	int	i, j;
	byte    *gridData;
	float	frac[3];
	int	gridStep[3];
	Vec3	direction;
	float	totalFactor;

	if(ent->e.renderfx & RF_LIGHTING_ORIGIN){
		/* seperate lightOrigins are needed so an object that is
		 * sinking into the ground can still be lit, and so
		 * multi-part models can be lit identically */
		copyv3(ent->e.lightingOrigin, lightOrigin);
	}else{
		copyv3(ent->e.origin, lightOrigin);
	}

	subv3(lightOrigin, world->lightGridOrigin, lightOrigin);
	for(i = 0; i < 3; i++){
		float v;

		v = lightOrigin[i]*world->lightGridInverseSize[i];
		pos[i]	= floor(v);
		frac[i] = v - pos[i];
		if(pos[i] < 0){
			pos[i] = 0;
		}else if(pos[i] >= world->lightGridBounds[i] - 1){
			pos[i] = world->lightGridBounds[i] - 1;
		}
	}

	clearv3(ent->ambientLight);
	clearv3(ent->directedLight);
	clearv3(direction);

	assert(world->lightGridData);	/* NULL with -nolight maps */

	/* trilerp the light value */
	gridStep[0]	= 8;
	gridStep[1]	= 8 * world->lightGridBounds[0];
	gridStep[2]	= 8 * world->lightGridBounds[0] * world->lightGridBounds[1];
	gridData = world->lightGridData + pos[0] * gridStep[0]
		   + pos[1] * gridStep[1] + pos[2] * gridStep[2];

	totalFactor = 0;
	for(i = 0; i < 8; i++){
		float	factor;
		byte    *data;
		int	lat, lng;
		Vec3	normal;
		qbool		ignore;
		#if idppc
		float		d0, d1, d2, d3, d4, d5;
		#endif
		factor	= 1.0;
		data	= gridData;
		ignore	= qfalse;
		for(j = 0; j < 3; j++){
			if(i & (1<<j)){
				if((pos[j] + 1) >= world->lightGridBounds[j] - 1){
					ignore = qtrue;	/* ignore values outside lightgrid */
				}
				factor	*= frac[j];
				data	+= gridStep[j];
			}else{
				factor *= (1.0f - frac[j]);
			}
		}

		if(ignore)
			continue;

		if(world->hdrLightGrid){
			float *hdrData = world->hdrLightGrid + (int)(data - world->lightGridData) / 8 * 6;
			if(!(hdrData[0]+hdrData[1]+hdrData[2]+hdrData[3]+hdrData[4]+hdrData[5])){
				continue;	/* ignore samples in walls */
			}
		}else{
			if(!(data[0]+data[1]+data[2]+data[3]+data[4]+data[5])){
				continue;	/* ignore samples in walls */
			}
		}
		totalFactor += factor;
		#if idppc
		d0	= data[0]; d1 = data[1]; d2 = data[2];
		d3	= data[3]; d4 = data[4]; d5 = data[5];

		ent->ambientLight[0]	+= factor * d0;
		ent->ambientLight[1]	+= factor * d1;
		ent->ambientLight[2]	+= factor * d2;

		ent->directedLight[0]	+= factor * d3;
		ent->directedLight[1]	+= factor * d4;
		ent->directedLight[2]	+= factor * d5;
		#else
		if(world->hdrLightGrid){
			/* FIXME: this is hideous */
			float *hdrData = world->hdrLightGrid + (int)(data - world->lightGridData) / 8 * 6;

			ent->ambientLight[0]	+= factor * hdrData[0];
			ent->ambientLight[1]	+= factor * hdrData[1];
			ent->ambientLight[2]	+= factor * hdrData[2];

			ent->directedLight[0]	+= factor * hdrData[3];
			ent->directedLight[1]	+= factor * hdrData[4];
			ent->directedLight[2]	+= factor * hdrData[5];
		}else{
			ent->ambientLight[0]	+= factor * data[0];
			ent->ambientLight[1]	+= factor * data[1];
			ent->ambientLight[2]	+= factor * data[2];

			ent->directedLight[0]	+= factor * data[3];
			ent->directedLight[1]	+= factor * data[4];
			ent->directedLight[2]	+= factor * data[5];
		}
		#endif
		lat	= data[7];
		lng	= data[6];
		lat	*= (FUNCTABLE_SIZE/256);
		lng	*= (FUNCTABLE_SIZE/256);

		/* decode X as cos( lat ) * sin( long )
		 * decode Y as sin( lat ) * sin( long )
		 * decode Z as cos( long ) */

		normal[0]	= tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
		normal[1]	= tr.sinTable[lat] * tr.sinTable[lng];
		normal[2]	= tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

		maddv3(direction, factor, normal, direction);
	}

	if(totalFactor > 0 && totalFactor < 0.99){
		totalFactor = 1.0f / totalFactor;
		scalev3(ent->ambientLight, totalFactor, ent->ambientLight);
		scalev3(ent->directedLight, totalFactor, ent->directedLight);
	}

	scalev3(ent->ambientLight, r_ambientScale->value, ent->ambientLight);
	scalev3(ent->directedLight, r_directedScale->value, ent->directedLight);

	norm2v3(direction, ent->lightDir);
}


/*
 * LogLight
 */
static void
LogLight(trRefEntity_t *ent)
{
	int max1, max2;

	if(!(ent->e.renderfx & RF_FIRST_PERSON)){
		return;
	}

	max1 = ent->ambientLight[0];
	if(ent->ambientLight[1] > max1){
		max1 = ent->ambientLight[1];
	}else if(ent->ambientLight[2] > max1){
		max1 = ent->ambientLight[2];
	}

	max2 = ent->directedLight[0];
	if(ent->directedLight[1] > max2){
		max2 = ent->directedLight[1];
	}else if(ent->directedLight[2] > max2){
		max2 = ent->directedLight[2];
	}

	ri.Printf(PRINT_ALL, "amb:%i  dir:%i\n", max1, max2);
}

/*
 * R_SetupEntityLighting
 *
 * Calculates all the lighting values that will be used
 * by the Calc_* functions
 */
void
R_SetupEntityLighting(const trRefdef_t *refdef, trRefEntity_t *ent)
{
	int i;
	dlight_t	*dl;
	float		power;
	Vec3		dir;
	float		d;
	Vec3		lightDir;
	Vec3		lightOrigin;

	/* lighting calculations */
	if(ent->lightingCalculated){
		return;
	}
	ent->lightingCalculated = qtrue;

	/*
	 * trace a sample point down to find ambient light
	 *  */
	if(ent->e.renderfx & RF_LIGHTING_ORIGIN){
		/* seperate lightOrigins are needed so an object that is
		 * sinking into the ground can still be lit, and so
		 * multi-part models can be lit identically */
		copyv3(ent->e.lightingOrigin, lightOrigin);
	}else{
		copyv3(ent->e.origin, lightOrigin);
	}

	/* if NOWORLDMODEL, only use dynamic lights (menu system, etc) */
	if(!(refdef->rdflags & RDF_NOWORLDMODEL)
	   && tr.world->lightGridData){
		R_SetupEntityLightingGrid(ent, tr.world);
	}else{
		ent->ambientLight[0] = ent->ambientLight[1] =
					       ent->ambientLight[2] = tr.identityLight * 150;
		ent->directedLight[0] = ent->directedLight[1] =
						ent->directedLight[2] = tr.identityLight * 150;
		copyv3(tr.sunDirection, ent->lightDir);
	}

	/* bonus items and view weapons have a fixed minimum add */
	if(!r_hdr->integer /* ent->e.renderfx & RF_MINLIGHT */){
		/* give everything a minimum light add */
		ent->ambientLight[0]	+= tr.identityLight * 32;
		ent->ambientLight[1]	+= tr.identityLight * 32;
		ent->ambientLight[2]	+= tr.identityLight * 32;
	}

	/*
	 * modify the light by dynamic lights
	 *  */
	d = lenv3(ent->directedLight);
	scalev3(ent->lightDir, d, lightDir);

	for(i = 0; i < refdef->num_dlights; i++){
		dl = &refdef->dlights[i];
		subv3(dl->origin, lightOrigin, dir);
		d = normv3(dir);

		power = DLIGHT_AT_RADIUS * (dl->radius * dl->radius);
		if(d < DLIGHT_MINIMUM_RADIUS){
			d = DLIGHT_MINIMUM_RADIUS;
		}
		d = power / (d * d);

		maddv3(ent->directedLight, d, dl->color, ent->directedLight);
		maddv3(lightDir, d, dir, lightDir);
	}

	/* clamp ambient */
	if(!r_hdr->integer){
		for(i = 0; i < 3; i++)
			if(ent->ambientLight[i] > tr.identityLightByte){
				ent->ambientLight[i] = tr.identityLightByte;
			}
	}

	if(r_debugLight->integer){
		LogLight(ent);
	}

	/* save out the byte packet version */
	((byte*)&ent->ambientLightInt)[0]	= ri.ftol(ent->ambientLight[0]);
	((byte*)&ent->ambientLightInt)[1]	= ri.ftol(ent->ambientLight[1]);
	((byte*)&ent->ambientLightInt)[2]	= ri.ftol(ent->ambientLight[2]);
	((byte*)&ent->ambientLightInt)[3]	= 0xff;

	/* transform the direction to local space
	 * no need to do this if using lightentity glsl shader */
	normv3(lightDir);
	copyv3(lightDir, ent->lightDir);
}

/*
 * R_LightForPoint
 */
int
R_LightForPoint(Vec3 point, Vec3 ambientLight, Vec3 directedLight, Vec3 lightDir)
{
	trRefEntity_t ent;

	if(tr.world->lightGridData == NULL)
		return qfalse;

	Q_Memset(&ent, 0, sizeof(ent));
	copyv3(point, ent.e.origin);
	R_SetupEntityLightingGrid(&ent, tr.world);
	copyv3(ent.ambientLight, ambientLight);
	copyv3(ent.directedLight, directedLight);
	copyv3(ent.lightDir, lightDir);

	return qtrue;
}


int
R_LightDirForPoint(Vec3 point, Vec3 lightDir, Vec3 normal, world_t *world)
{
	trRefEntity_t ent;

	if(world->lightGridData == NULL)
		return qfalse;

	Q_Memset(&ent, 0, sizeof(ent));
	copyv3(point, ent.e.origin);
	R_SetupEntityLightingGrid(&ent, world);

	if((dotv3(ent.lightDir, ent.lightDir) < 0.9f) || (dotv3(ent.lightDir, normal) < 0.3f)){
		copyv3(normal, lightDir);
	}else{
		copyv3(ent.lightDir, lightDir);
	}

	return qtrue;
}
