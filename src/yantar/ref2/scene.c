/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

int r_firstSceneDrawSurf;

int r_numdlights;
int r_firstSceneDlight;

int r_numentities;
int r_firstSceneEntity;

int r_numpolys;
int r_firstScenePoly;

int r_numpolyverts;


/*
 * R_ToggleSmpFrame
 *
 */
void
R_ToggleSmpFrame(void)
{
	if(r_smp->integer){
		/* use the other buffers next frame, because another CPU
		 * may still be rendering into the current ones */
		tr.smpFrame ^= 1;
	}else{
		tr.smpFrame = 0;
	}

	backEndData[tr.smpFrame]->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}


/*
 * RE_ClearScene
 *
 */
void
RE_ClearScene(void)
{
	r_firstSceneDlight = r_numdlights;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
}

/*
 *
 * DISCRETE POLYS
 *
 */

/*
 * R_AddPolygonSurfaces
 *
 * Adds all the scene's polys into this view's drawsurf list
 */
void
R_AddPolygonSurfaces(void)
{
	int i;
	material_t *sh;
	srfPoly_t *poly;
/* JBravo: Fog fixes */
	int fogMask;

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;
	fogMask = -((tr.refdef.rdflags & RDF_NOFOG) == 0);

	for(i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys; i++, poly++){
		sh = R_GetShaderByHandle(poly->hShader);
		R_AddDrawSurf(( void* )poly, sh, poly->fogIndex & fogMask, qfalse, qfalse);
	}
}

/*
 * RE_AddPolyToScene
 *
 */
void
RE_AddPolyToScene(Handle hShader, int numVerts, const Polyvert *verts, int numPolys)
{
	srfPoly_t	*poly;
	int	i, j;
	int	fogIndex;
	fog_t	*fog;
	Vec3	bounds[2];

	if(!tr.registered){
		return;
	}

	if(!hShader){
		/* This isn't a useful warning, and an hShader of zero isn't a null shader, it's
		 * the default shader.
		 * ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		 * return; */
	}

	for(j = 0; j < numPolys; j++){
		if(r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys){
			/*
			 * NOTE TTimo this was initially a PRINT_WARNING
			 * but it happens a lot with high fighting scenes and particles
			 * since we don't plan on changing the const and making for room for those effects
			 * simply cut this message to developer only
			 */
			ri.Printf(PRINT_DEVELOPER,
				"WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
			return;
		}

		poly = &backEndData[tr.smpFrame]->polys[r_numpolys];
		poly->surfaceType	= SF_POLY;
		poly->hShader	= hShader;
		poly->numVerts	= numVerts;
		poly->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];

		Q_Memcpy(poly->verts, &verts[numVerts*j], numVerts * sizeof(*verts));

		/* done. */
		r_numpolys++;
		r_numpolyverts += numVerts;

		/* if no world is loaded */
		if(tr.world == NULL){
			fogIndex = 0;
		}
		/* see if it is in a fog volume */
		else if(tr.world->numfogs == 1){
			fogIndex = 0;
		}else{
			/* find which fog volume the poly is in */
			copyv3(poly->verts[0].xyz, bounds[0]);
			copyv3(poly->verts[0].xyz, bounds[1]);
			for(i = 1; i < poly->numVerts; i++)
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			for(fogIndex = 1; fogIndex < tr.world->numfogs; fogIndex++){
				fog = &tr.world->fogs[fogIndex];
				if(bounds[1][0] >= fog->bounds[0][0]
				   && bounds[1][1] >= fog->bounds[0][1]
				   && bounds[1][2] >= fog->bounds[0][2]
				   && bounds[0][0] <= fog->bounds[1][0]
				   && bounds[0][1] <= fog->bounds[1][1]
				   && bounds[0][2] <= fog->bounds[1][2]){
					break;
				}
			}
			if(fogIndex == tr.world->numfogs){
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}


/* ================================================================================= */


/*
 * RE_AddRefEntityToScene
 *
 */
void
RE_AddRefEntityToScene(const Refent *ent)
{
#ifdef REACTION
	/* JBravo: Mirrored models */
	Vec3 cross;
#endif

	if(!tr.registered){
		return;
	}
	if(r_numentities >= MAX_ENTITIES){
		ri.Printf(PRINT_DEVELOPER,
			"RE_AddRefEntityToScene: Dropping refEntity, reached MAX_ENTITIES\n");
		return;
	}
	if(Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2])){
		static qbool firstTime = qtrue;
		if(firstTime){
			firstTime = qfalse;
			ri.Printf(
				PRINT_WARNING,
				"RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}
	if((int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE){
		ri.Error(ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType);
	}

	backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

#ifdef REACTION
	/* JBravo: Mirrored models */
	crossv3(ent->axis[0], ent->axis[1], cross);
	backEndData[tr.smpFrame]->entities[r_numentities].mirrored = (dotv3(ent->axis[2], cross) < 0.f);
#endif

	r_numentities++;
}


/*
 * RE_AddDynamicLightToScene
 *
 */
void
RE_AddDynamicLightToScene(const Vec3 org, float intensity, float r, float g, float b, int additive)
{
	dlight_t *dl;

	if(!tr.registered){
		return;
	}
	if(r_numdlights >= MAX_DLIGHTS){
		return;
	}
	if(intensity <= 0){
		return;
	}
	dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
	copyv3 (org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = additive;
}

/*
 * RE_AddLightToScene
 *
 */
void
RE_AddLightToScene(const Vec3 org, float intensity, float r, float g, float b)
{
	RE_AddDynamicLightToScene(org, intensity, r, g, b, qfalse);
}

/*
 * RE_AddAdditiveLightToScene
 *
 */
void
RE_AddAdditiveLightToScene(const Vec3 org, float intensity, float r, float g, float b)
{
	RE_AddDynamicLightToScene(org, intensity, r, g, b, qtrue);
}

/*
 * @@@@@@@@@@@@@@@@@@@@@
 * RE_RenderScene
 *
 * Draw a 3D view into a part of the window, then return
 * to 2D drawing.
 *
 * Rendering a scene may require multiple views to be rendered
 * to handle mirrors,
 * @@@@@@@@@@@@@@@@@@@@@
 */
void
RE_RenderScene(const Refdef *fd)
{
	viewParms_t parms;
	int startTime;

	if(!tr.registered){
		return;
	}
	GLimp_LogComment("====== RE_RenderScene =====\n");

	if(r_norefresh->integer){
		return;
	}

	startTime = ri.Milliseconds();

	if(!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)){
		ri.Error (ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	Q_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	copyv3(fd->vieworg, tr.refdef.vieworg);
	copyv3(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	copyv3(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	copyv3(fd->viewaxis[2], tr.refdef.viewaxis[2]);

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	/* copy the areamask data over and note if it has changed, which
	 * will force a reset of the visible leafs even if the view hasn't moved */
	tr.refdef.areamaskModified = qfalse;
	if(!(tr.refdef.rdflags & RDF_NOWORLDMODEL)){
		int areaDiff;
		int i;

		/* compare the area bits */
		areaDiff = 0;
		for(i = 0; i < MAX_MAP_AREA_BYTES/4; i++){
			areaDiff |= ((int*)tr.refdef.areamask)[i] ^ ((int*)fd->areamask)[i];
			((int*)tr.refdef.areamask)[i] = ((int*)fd->areamask)[i];
		}

		if(areaDiff){
			/* a door just opened or something */
			tr.refdef.areamaskModified = qtrue;
		}
	}

#ifdef REACTION
	/* Makro - copy exta info if present */
	if(fd->rdflags & RDF_EXTRA){
		const refdefex_t * extra = (const refdefex_t*)(fd+1);
		tr.refdef.blurFactor = extra->blurFactor;
	}else{
		tr.refdef.blurFactor = 0.f;
	}
#endif

	/* derived info */

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData[tr.smpFrame]->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData[tr.smpFrame]->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData[tr.smpFrame]->dlights[r_firstSceneDlight];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[tr.smpFrame]->polys[r_firstScenePoly];

	tr.refdef.num_pshadows = 0;
	tr.refdef.pshadows = &backEndData[tr.smpFrame]->pshadows[0];

	/* turn off dynamic lighting globally by clearing all the
	 * dlights if it needs to be disabled or if vertex lighting is enabled */
	if(r_dynamiclight->integer == 0 ||
	   r_vertexLight->integer == 1){
		tr.refdef.num_dlights = 0;
	}

	/* a single frame may have multiple scenes draw inside it --
	 * a 3D game view, 3D status bar renderings, 3D menus, etc.
	 * They need to be distinguished by the light flare code, because
	 * the visibility state for a given surface may be different in
	 * each scene / view. */
	tr.frameSceneNum++;
	tr.sceneCount++;

	/* SmileTheory: playing with shadow mapping */
	if(!(fd->rdflags & RDF_NOWORLDMODEL) && tr.refdef.num_dlights && r_dlightMode->integer >= 2){
		R_RenderDlightCubemaps(fd);
	}

	/* playing with more shadows */
	if(!(fd->rdflags & RDF_NOWORLDMODEL) && r_shadows->integer == 4){
		R_RenderPshadowMaps(fd);
	}

	/* setup view parms for the initial view
	 *
	 * set up viewport
	 * The refdef takes 0-at-the-top y coordinates, so
	 * convert to GL's 0-at-the-bottom space
	 *  */
	Q_Memset(&parms, 0, sizeof(parms));
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	parms.stereoFrame = tr.refdef.stereoFrame;

	if(glRefConfig.framebufferObject){
		parms.targetFbo = tr.renderFbo;
	}

	copyv3(fd->vieworg, parms.or.origin);
	copyv3(fd->viewaxis[0], parms.or.axis[0]);
	copyv3(fd->viewaxis[1], parms.or.axis[1]);
	copyv3(fd->viewaxis[2], parms.or.axis[2]);

	copyv3(fd->vieworg, parms.pvsOrigin);

	R_RenderView(&parms);

	if(!(fd->rdflags & RDF_NOWORLDMODEL))
		R_AddPostProcessCmd();

	/* the next scene rendered in this frame will tack on after this one */
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight	= r_numdlights;
	r_firstScenePoly	= r_numpolys;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}
