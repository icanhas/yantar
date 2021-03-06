/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "local.h"

backEndData_t *backEndData[SMP_FRAMES];
backEndState_t backEnd;


static float s_flipMatrix[16] = {
	/* convert from our coordinate system (looking down X)
	 * to OpenGL's coordinate system (looking down -Z) */
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


/*
** GL_Bind2
*/
void
GL_Bind2(Img *image, GLenum type)
{
	int texnum;

	if(!image){
		ri.Printf(PRINT_WARNING, "GL_Bind2: NULL image\n");
		texnum = tr.defaultImage->texnum;
	}else{
		texnum = image->texnum;
	}

	if(r_nobind->integer && tr.dlightImage){	/* performance evaluation option */
		texnum = tr.dlightImage->texnum;
	}

	if(glState.currenttextures[glState.currenttmu] != texnum){
		image->frameUsed = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture (type, texnum);
	}
}

/*
** GL_Bind2
*/
void
GL_Bind(Img *image)
{
	GL_Bind2(image, GL_TEXTURE_2D);
}

/*
** GL_BindCubemap
*/
void
GL_BindCubemap(Img *image)
{
	GL_Bind2(image, GL_TEXTURE_CUBE_MAP);
}

/*
** GL_SelectTexture
*/
void
GL_SelectTexture(int unit)
{
	if(glState.currenttmu == unit){
		return;
	}

	if(!(unit >= 0 && unit <= 31))
		ri.Error(ERR_DROP, "GL_SelectTexture: unit = %i", unit);

	qglActiveTextureARB(GL_TEXTURE0_ARB + unit);

	glState.currenttmu = unit;
}


/*
** GL_BindMultitexture
*/
void
GL_BindMultitexture(Img *image0, GLuint env0, Img *image1, GLuint env1)
{
	int texnum0, texnum1;

	texnum0 = image0->texnum;
	texnum1 = image1->texnum;

	if(r_nobind->integer && tr.dlightImage){	/* performance evaluation option */
		texnum0 = texnum1 = tr.dlightImage->texnum;
	}

	if(glState.currenttextures[1] != texnum1){
		GL_SelectTexture(1);
		image1->frameUsed = tr.frameCount;
		glState.currenttextures[1] = texnum1;
		qglBindTexture(GL_TEXTURE_2D, texnum1);
	}
	if(glState.currenttextures[0] != texnum0){
		GL_SelectTexture(0);
		image0->frameUsed = tr.frameCount;
		glState.currenttextures[0] = texnum0;
		qglBindTexture(GL_TEXTURE_2D, texnum0);
	}
}

/*
** GL_BindToTMU
*/
void
GL_BindToTMU(Img *image, int tmu)
{
	int texnum;
	int oldtmu = glState.currenttmu;

	texnum = image->texnum;

	if(glState.currenttextures[tmu] != texnum){
		GL_SelectTexture(tmu);
		image->frameUsed = tr.frameCount;
		glState.currenttextures[tmu] = texnum;
		qglBindTexture(GL_TEXTURE_2D, texnum);
		GL_SelectTexture(oldtmu);
	}
}


/*
** GL_Cull
*/
void
GL_Cull(int cullType)
{
#ifdef REACTION
	/* Makro - flip culling if needed */
	qbool flip =
		(backEnd.currentEntity != NULL && backEnd.currentEntity->mirrored != qfalse && cullType !=
	CT_TWO_SIDED);
	cullType ^= flip;	/* this assumes CT_BACK_SIDED and CT_FRONT_SIDED are 0 or 1 */
#endif

	if(glState.faceCulling == cullType){
		return;
	}

	glState.faceCulling = cullType;

	if(cullType == CT_TWO_SIDED){
		qglDisable(GL_CULL_FACE);
	}else{
		qbool cullFront;
		qglEnable(GL_CULL_FACE);

		cullFront = (cullType == CT_FRONT_SIDED);
		if(backEnd.viewParms.isMirror && !backEnd.viewParms.isShadowmap){
			cullFront = !cullFront;
		}

		qglCullFace(cullFront ? GL_FRONT : GL_BACK);
	}
}

/*
** GL_TexEnv
*/
void
GL_TexEnv(int env)
{
	if(env == glState.texEnv[glState.currenttmu]){
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch(env){
	case GL_MODULATE:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case GL_REPLACE:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	case GL_DECAL:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		break;
	case GL_ADD:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		break;
	default:
		ri.Error(ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env);
		break;
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void
GL_State(unsigned long stateBits)
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if(!diff){
		return;
	}

	/*
	 * check depthFunc bits
	 *  */
	if(diff & GLS_DEPTHFUNC_EQUAL){
		if(stateBits & GLS_DEPTHFUNC_EQUAL){
			qglDepthFunc(GL_EQUAL);
		}else{
			qglDepthFunc(GL_LEQUAL);
		}
	}

	/*
	 * check blend bits
	 *  */
	if(diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)){
		GLenum srcFactor, dstFactor;

		if(stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)){
			switch(stateBits & GLS_SRCBLEND_BITS){
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				srcFactor = GL_ONE;	/* to get warning to shut up */
				ri.Error(ERR_DROP, "GL_State: invalid src blend state bits");
				break;
			}

			switch(stateBits & GLS_DSTBLEND_BITS){
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				dstFactor = GL_ONE;	/* to get warning to shut up */
				ri.Error(ERR_DROP, "GL_State: invalid dst blend state bits");
				break;
			}

			qglEnable(GL_BLEND);
			qglBlendFunc(srcFactor, dstFactor);
		}else{
			qglDisable(GL_BLEND);
		}
	}

	/*
	 * check depthmask
	 *  */
	if(diff & GLS_DEPTHMASK_TRUE){
		if(stateBits & GLS_DEPTHMASK_TRUE){
			qglDepthMask(GL_TRUE);
		}else{
			qglDepthMask(GL_FALSE);
		}
	}

	/*
	 * fill/line mode
	 *  */
	if(diff & GLS_POLYMODE_LINE){
		if(stateBits & GLS_POLYMODE_LINE){
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}else{
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	/*
	 * depthtest
	 *  */
	if(diff & GLS_DEPTHTEST_DISABLE){
		if(stateBits & GLS_DEPTHTEST_DISABLE){
			qglDisable(GL_DEPTH_TEST);
		}else{
			qglEnable(GL_DEPTH_TEST);
		}
	}

	/*
	 * alpha test
	 *  */
	if(diff & GLS_ATEST_BITS){
		switch(stateBits & GLS_ATEST_BITS){
		case 0:
			qglDisable(GL_ALPHA_TEST);
			break;
		case GLS_ATEST_GT_0:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GREATER, 0.0f);
			break;
		case GLS_ATEST_LT_80:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_LESS, 0.5f);
			break;
		case GLS_ATEST_GE_80:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GEQUAL, 0.5f);
			break;
		default:
			assert(0);
			break;
		}
	}

	glState.glStateBits = stateBits;
}


void
GL_SetProjectionMatrix(Mat4 matrix)
{
	copym4(matrix, glState.projection);
	mulm4(glState.modelview, glState.projection, glState.modelviewProjection);
}


void
GL_SetModelviewMatrix(Mat4 matrix)
{
	copym4(matrix, glState.modelview);
	mulm4(glState.modelview, glState.projection, glState.modelviewProjection);
}


/*
 * RB_Hyperspace
 *
 * A player has predicted a teleport, but hasn't arrived yet
 */
static void
RB_Hyperspace(void)
{
	float c;

	if(!backEnd.isHyperspace){
		/* do initialization shit */
	}

	c = (backEnd.refdef.time & 255) / 255.0f;
	qglClearColor(c, c, c, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	backEnd.isHyperspace = qtrue;
}


static void
SetViewportAndScissor(void)
{
	GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);

	/* set the window clipping */
	qglViewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
	qglScissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}

/*
 * RB_BeginDrawingView
 *
 * Any mirrored or portaled views have already been drawn, so prepare
 * to actually render the visible surfaces for this view
 */
void
RB_BeginDrawingView(void)
{
	int clearBits = 0;

	/* sync with gl if needed */
	if(r_finish->integer == 1 && !glState.finishCalled){
		qglFinish ();
		glState.finishCalled = qtrue;
	}
	if(r_finish->integer == 0){
		glState.finishCalled = qtrue;
	}

	/* we will need to change the projection matrix before drawing
	 * 2D images again */
	backEnd.projection2D = qfalse;

	if(glRefConfig.framebufferObject){
		/* FIXME: HUGE HACK: render to the screen fbo if we've already postprocessed the frame and aren't drawing more world */
		if(backEnd.viewParms.targetFbo == tr.renderFbo && backEnd.framePostProcessed &&
		   (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)){
			FBO_Bind(tr.screenScratchFbo);
		}else{
			FBO_Bind(backEnd.viewParms.targetFbo);
		}
	}

	/*
	 * set the modelview matrix for the viewer
	 *  */
	SetViewportAndScissor();

	/* ensures that depth writes are enabled for the depth clear */
	GL_State(GLS_DEFAULT);
	/* clear relevant buffers */
	clearBits = GL_DEPTH_BUFFER_BIT;

	if(r_measureOverdraw->integer || r_shadows->integer == 2){
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if(r_fastsky->integer && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)){
		clearBits |= GL_COLOR_BUFFER_BIT;	/* FIXME: only if sky shaders have been used */
#ifdef _DEBUG
		qglClearColor(0.8f, 0.7f, 0.4f, 1.0f);	/* FIXME: get color of sky */
#else
		qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);	/* FIXME: get color of sky */
#endif
	}

	/* clear to white for shadow maps */
	if(backEnd.viewParms.isShadowmap){
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	qglClear(clearBits);

	if((backEnd.refdef.rdflags & RDF_HYPERSPACE)){
		RB_Hyperspace();
		return;
	}else{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;	/* force face culling to set next time */

	/* we will only draw a sun if there was sky rendered in this view */
	backEnd.skyRenderedThisView = qfalse;

#ifdef REACTION
	backEnd.hasSunFlare = qfalse;
#endif

	/* clip to the plane of the portal */
	if(backEnd.viewParms.isPortal){
		float plane[4];
		double plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = dotv3 (backEnd.viewParms.or.axis[0], plane);
		plane2[1] = dotv3 (backEnd.viewParms.or.axis[1], plane);
		plane2[2] = dotv3 (backEnd.viewParms.or.axis[2], plane);
		plane2[3] = dotv3 (plane, backEnd.viewParms.or.origin) - plane[3];

		GL_SetModelviewMatrix(s_flipMatrix);
	}
}


#define MAC_EVENT_PUMP_MSEC 5

/*
 * RB_RenderDrawSurfList
 */
void
RB_RenderDrawSurfList(drawSurf_t *drawSurfs, int numDrawSurfs)
{
	material_t	*shader, *oldShader;
	int		fogNum, oldFogNum;
	int		entityNum, oldEntityNum;
	int		dlighted, oldDlighted;
	int		pshadowed, oldPshadowed;
	qbool		depthRange, oldDepthRange, isCrosshair, wasCrosshair;
	int		i;
	drawSurf_t              *drawSurf;
	int		oldSort;
	float		originalTime;
	float		depth[2];
	FBO_t		* fbo	= NULL;
	qbool		inQuery = qfalse;

	/* save original time for entity shader offsets */
	originalTime = backEnd.refdef.floatTime;

	/* clear the z buffer, set the modelview, etc */
	RB_BeginDrawingView ();

	fbo = glState.currentFBO;

	/* draw everything */
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange	= qfalse;
	wasCrosshair	= qfalse;
	oldDlighted	= qfalse;
	oldPshadowed	= qfalse;
	oldSort = -1;
	depthRange = qfalse;
	depth[0] = 0.f;
	depth[1] = 1.f;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for(i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++){
		if(drawSurf->sort == oldSort){
			/* fast path, same as previous sort */
			rb_surfaceTable[ *drawSurf->surface ](drawSurf->surface);
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed);

		/*
		 * change the tess parameters if needed
		 * a "entityMergable" shader is a shader that can have surfaces from seperate
		 * entities merged into a single batch, like smoke and blood puff sprites */
		if(shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted || pshadowed !=
		   oldPshadowed
		   || (entityNum != oldEntityNum && !shader->entityMergable)){
			if(oldShader != NULL){
				RB_EndSurface();
			}
			RB_BeginSurface(shader, fogNum);
			backEnd.pc.c_surfBatches++;
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
			oldPshadowed = pshadowed;
		}

		/*
		 * change the modelview matrix if needed
		 *  */
		if(entityNum != oldEntityNum){
			qbool sunflare = qfalse;
			depthRange = isCrosshair = qfalse;

#ifdef REACTION
			/* if we were rendering to a FBO and the previous entity was a sunflare
			 * and the current one isn't, switch back to the main fbo */
			if(oldEntityNum != -1 && fbo &&
			   RF_SUNFLARE == (backEnd.refdef.entities[oldEntityNum].e.renderfx & RF_SUNFLARE) &&
			   0 == (backEnd.refdef.entities[entityNum].e.renderfx & RF_SUNFLARE)){
				if(inQuery){
					inQuery = qfalse;
					qglEndQueryARB(GL_SAMPLES_PASSED_ARB);
				}
				FBO_Bind(fbo);
				qglDepthRange(depth[0], depth[1]);
			}
#endif

			if(entityNum != ENTITYNUM_WORLD){
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				/* we have to reset the shaderTime as well otherwise image animations start
				 * from the wrong frame */
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				/* set up the transformation matrix */
				R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.or);

				/* set up the dynamic lighting if needed */
				if(backEnd.currentEntity->needDlights){
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights,
						&backEnd.or);
				}

#ifdef REACTION
				/* if the current entity is a sunflare */
				if(backEnd.currentEntity->e.renderfx & RF_SUNFLARE){
					/* if we're rendering to a fbo */
					if(fbo){
						copyv3(backEnd.currentEntity->e.origin,
							backEnd.sunFlarePos);
						/* switch FBO */
						FBO_Bind(tr.godRaysFbo);

						qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
						qglClear(GL_COLOR_BUFFER_BIT);

						qglDepthRange(1.f, 1.f);
						if(glRefConfig.occlusionQuery && !inQuery &&
						   !backEnd.hasSunFlare){
							inQuery = qtrue;
							tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = qtrue;
							qglBeginQueryARB(
								GL_SAMPLES_PASSED_ARB,
								tr.sunFlareQuery[tr.sunFlareQueryIndex]);
						}
						/* backEnd.hasSunFlare = qtrue; */
						sunflare = qtrue;
					}else{
						depthRange = qtrue;
					}
				}
#endif

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK){
					/* hack the depth range to prevent view model from poking into walls */
					depthRange = qtrue;

					if(backEnd.currentEntity->e.renderfx & RF_CROSSHAIR)
						isCrosshair = qtrue;
				}
			}else{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				/* we have to reset the shaderTime as well otherwise image animations on
				 * the world (like water) continue with the wrong frame */
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights,
					&backEnd.or);
			}

			GL_SetModelviewMatrix(backEnd.or.modelMatrix);

			/*
			 * change depthrange. Also change projection matrix so first person weapon does not look like coming
			 * out of the screen.
			 *  */
			if(oldDepthRange != depthRange || wasCrosshair != isCrosshair){
				if(depthRange){
					if(backEnd.viewParms.stereoFrame != STEREO_CENTER){
						if(isCrosshair){
							if(oldDepthRange){
								/* was not a crosshair but now is, change back proj matrix */
								GL_SetProjectionMatrix(
									backEnd.viewParms.projectionMatrix);
							}
						}else{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix(temp.projectionMatrix);
						}
					}

#ifdef REACTION
					if(!oldDepthRange){
						depth[0] = 0;
						depth[1] = 0.3f;
						qglDepthRange (0, 0.3);
					}
#endif
				}else{
					if(!wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER){
						GL_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);
					}

					if(!sunflare)
						qglDepthRange (0, 1);
					depth[0] = 0;
					depth[1] = 1;
				}

				oldDepthRange	= depthRange;
				wasCrosshair	= isCrosshair;
			}

			oldEntityNum = entityNum;
		}

		/* add the triangles for this surface */
		rb_surfaceTable[ *drawSurf->surface ](drawSurf->surface);
	}

	backEnd.refdef.floatTime = originalTime;

	/* draw the contents of the last shader batch */
	if(oldShader != NULL){
		RB_EndSurface();
	}

	if(inQuery){
		inQuery = qfalse;
		qglEndQueryARB(GL_SAMPLES_PASSED_ARB);
	}
	FBO_Bind(fbo);

	/* go back to the world modelview matrix */

	GL_SetModelviewMatrix(backEnd.viewParms.world.modelMatrix);
	/* if ( depthRange ) { */
	qglDepthRange (0, 1);
	/* } */

#if 0
	RB_DrawSun();
#endif
	/* darken down any stencil shadows */
	RB_ShadowFinish();

	/* add light flares on lights that aren't obscured */
	RB_RenderFlares();

	if(glRefConfig.framebufferObject)
		FBO_Bind(NULL);
}


/*
 *
 * RENDER BACK END THREAD FUNCTIONS
 *
 */

/*
 * RB_SetGL2D
 *
 */
void
RB_SetGL2D(void)
{
	Mat4 matrix;
	int width, height;

	if(backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO)
		return;

	backEnd.projection2D = qtrue;
	backEnd.last2DFBO = glState.currentFBO;

	if(glState.currentFBO){
		width	= glState.currentFBO->width;
		height	= glState.currentFBO->height;
	}else{
		width	= glConfig.vidWidth;
		height	= glConfig.vidHeight;
	}

	/* set 2D virtual screen size */
	qglViewport(0, 0, width, height);
	qglScissor(0, 0, width, height);

	orthom4(0, width, height, 0, 0, 1, matrix);
	GL_SetProjectionMatrix(matrix);
	identm4(matrix);
	GL_SetModelviewMatrix(matrix);

	GL_State(GLS_DEPTHTEST_DISABLE |
		GLS_SRCBLEND_SRC_ALPHA |
		GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	qglDisable(GL_CULL_FACE);
	qglDisable(GL_CLIP_PLANE0);

	/* set time for 2D shaders */
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}


/*
 * RE_StretchRaw
 *
 * FIXME: not exactly backend
 * Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
 * Used for cinematics.
 */
void
RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qbool dirty)
{
	int i, j;
	int start, end;
	shaderProgram_t *sp = &tr.textureColorShader;
	Vec4 color;

	if(!tr.registered){
		return;
	}
	R_SyncRenderThread();

	/* we definately want to sync every frame for the cinematics */
	qglFinish();

	start = 0;
	if(r_speeds->integer){
		start = ri.Milliseconds();
	}

	/* make sure rows and cols are powers of 2 */
	for(i = 0; (1 << i) < cols; i++){
	}
	for(j = 0; (1 << j) < rows; j++){
	}
	if((1 << i) != cols || (1 << j) != rows){
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	GL_Bind(tr.scratchImage[client]);

	/* if the scratchImage isn't in the format we want, specify it as a new texture */
	if(cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height){
		tr.scratchImage[client]->width	= tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}else{
		if(dirty){
			/* otherwise, just subimage upload it so that drivers can tell we are going to be changing
			 * it and don't try and do a texture compression */
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}

	if(r_speeds->integer){
		end = ri.Milliseconds();
		ri.Printf(PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	/* FIXME: HUGE hack */
	if(glRefConfig.framebufferObject && !glState.currentFBO){
		if(backEnd.framePostProcessed){
			FBO_Bind(tr.screenScratchFbo);
		}else{
			FBO_Bind(tr.renderFbo);
		}
	}

	RB_SetGL2D();

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	tess.xyz[tess.numVertexes][0]	= x;
	tess.xyz[tess.numVertexes][1]	= y;
	tess.xyz[tess.numVertexes][2]	= 0;
	tess.xyz[tess.numVertexes][3]	= 1;
	tess.texCoords[tess.numVertexes][0][0]	= 0.5f / cols;
	tess.texCoords[tess.numVertexes][0][1]	= 0.5f / rows;
	tess.texCoords[tess.numVertexes][0][2]	= 0;
	tess.texCoords[tess.numVertexes][0][3]	= 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]	= x + w;
	tess.xyz[tess.numVertexes][1]	= y;
	tess.xyz[tess.numVertexes][2]	= 0;
	tess.xyz[tess.numVertexes][3]	= 1;
	tess.texCoords[tess.numVertexes][0][0]	= (cols - 0.5f) / cols;
	tess.texCoords[tess.numVertexes][0][1]	= 0.5f / rows;
	tess.texCoords[tess.numVertexes][0][2]	= 0;
	tess.texCoords[tess.numVertexes][0][3]	= 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]	= x + w;
	tess.xyz[tess.numVertexes][1]	= y + h;
	tess.xyz[tess.numVertexes][2]	= 0;
	tess.xyz[tess.numVertexes][3]	= 1;
	tess.texCoords[tess.numVertexes][0][0]	= (cols - 0.5f) / cols;
	tess.texCoords[tess.numVertexes][0][1]	= (rows - 0.5f) / rows;
	tess.texCoords[tess.numVertexes][0][2]	= 0;
	tess.texCoords[tess.numVertexes][0][3]	= 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]	= x;
	tess.xyz[tess.numVertexes][1]	= y + h;
	tess.xyz[tess.numVertexes][2]	= 0;
	tess.xyz[tess.numVertexes][3]	= 1;
	tess.texCoords[tess.numVertexes][0][0]	= 0.5f / cols;
	tess.texCoords[tess.numVertexes][0][1]	= (rows - 0.5f) / rows;
	tess.texCoords[tess.numVertexes][0][2]	= 0;
	tess.texCoords[tess.numVertexes][0][3]	= 1;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	/* FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function */
	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD);

	sp = &tr.textureColorShader;

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD);

	GLSL_BindProgram(sp);

	GLSL_SetUniformMatrix16(sp, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX,
		glState.modelviewProjection);
	setv34(color, 1, 1, 1, 1);
	GLSL_SetUniformVec4(sp, TEXTURECOLOR_UNIFORM_COLOR, color);

	qglDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(0));

	/* R_BindNullVBO();
	 * R_BindNullIBO(); */

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}

void
RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qbool dirty)
{

	GL_Bind(tr.scratchImage[client]);

	/* if the scratchImage isn't in the format we want, specify it as a new texture */
	if(cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height){
		tr.scratchImage[client]->width	= tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}else{
		if(dirty){
			/* otherwise, just subimage upload it so that drivers can tell we are going to be changing
			 * it and don't try and do a texture compression */
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
}


/*
 * RB_SetColor
 *
 */
const void      *
RB_SetColor(const void *data)
{
	const setColorCommand_t *cmd;

	cmd = (const setColorCommand_t*)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void*)(cmd + 1);
}

/*
 * RB_StretchPic
 */
const void *
RB_StretchPic(const void *data)
{
	const stretchPicCommand_t *cmd;
	material_t *shader;
	int numVerts, numIndexes;

	cmd = (const stretchPicCommand_t*)data;

	/* FIXME: HUGE hack */
	if(glRefConfig.framebufferObject && !glState.currentFBO){
		if(backEnd.framePostProcessed){
			FBO_Bind(tr.screenScratchFbo);
		}else{
			FBO_Bind(tr.renderFbo);
		}
	}

	RB_SetGL2D();

	shader = cmd->shader;
	if(shader != tess.shader){
		if(tess.numIndexes){
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(4, 6);
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ]	= numVerts + 0;
	tess.indexes[ numIndexes + 2 ]	= numVerts + 2;
	tess.indexes[ numIndexes + 3 ]	= numVerts + 2;
	tess.indexes[ numIndexes + 4 ]	= numVerts + 0;
	tess.indexes[ numIndexes + 5 ]	= numVerts + 1;

	{
		Vec4 color;

		scalev34(backEnd.color2D, 1.0f / 255.0f, color);

		copyv34(color, tess.vertexColors[ numVerts ]);
		copyv34(color, tess.vertexColors[ numVerts + 1]);
		copyv34(color, tess.vertexColors[ numVerts + 2]);
		copyv34(color, tess.vertexColors[ numVerts + 3 ]);
	}

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void*)(cmd + 1);
}


/*
 * RB_DrawSurfs
 *
 */
const void      *
RB_DrawSurfs(const void *data)
{
	const drawSurfsCommand_t *cmd;

	/* finish any 2D drawing if needed */
	if(tess.numIndexes){
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t*)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList(cmd->drawSurfs, cmd->numDrawSurfs);

	return (const void*)(cmd + 1);
}


/*
 * RB_DrawBuffer
 *
 */
const void      *
RB_DrawBuffer(const void *data)
{
	const drawBufferCommand_t *cmd;

	cmd = (const drawBufferCommand_t*)data;

	qglDrawBuffer(cmd->buffer);

	/* clear screen for debugging */
	if(r_clear->integer){
		qglClearColor(1, 0, 0.5, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return (const void*)(cmd + 1);
}

/*
 * RB_ShowImages
 *
 * Draw all the images to the screen, on top of whatever
 * was there.  This is used to test for texture thrashing.
 *
 * Also called by RE_EndRegistration
 */
void
RB_ShowImages(void)
{
	int i;
	Img *image;
	float	x, y, w, h;
	int start, end;

	RB_SetGL2D();

	qglClear(GL_COLOR_BUFFER_BIT);

	qglFinish();

	start = ri.Milliseconds();

	for(i=0; i<tr.numImages; i++){
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		/* show in proportional size in mode 2 */
		if(r_showImages->integer == 2){
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			Vec4 quadVerts[4];

			GL_Bind(image);

			setv34(quadVerts[0], x, y, 0, 1);
			setv34(quadVerts[1], x + w, y, 0, 1);
			setv34(quadVerts[2], x + w, y + h, 0, 1);
			setv34(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);
		}
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf(PRINT_ALL, "%i msec to draw all images\n", end - start);

}

/*
 * RB_ColorMask
 *
 */
const void *
RB_ColorMask(const void *data)
{
	const colorMaskCommand_t *cmd = data;

	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);

	return (const void*)(cmd + 1);
}

/*
 * RB_ClearDepth
 *
 */
const void *
RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = data;

	if(tess.numIndexes)
		RB_EndSurface();

	/* texture swapping test */
	if(r_showImages->integer)
		RB_ShowImages();

	qglClear(GL_DEPTH_BUFFER_BIT);

	return (const void*)(cmd + 1);
}

/*
 * RB_SwapBuffers
 *
 */
const void      *
RB_SwapBuffers(const void *data)
{
	const swapBuffersCommand_t *cmd;

	/* finish any 2D drawing if needed */
	if(tess.numIndexes){
		RB_EndSurface();
	}

	/* texture swapping test */
	if(r_showImages->integer){
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t*)data;

	/* we measure overdraw by reading back the stencil buffer and
	 * counting up the number of increments that have happened */
	if(r_measureOverdraw->integer){
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.hunkalloctemp(glConfig.vidWidth * glConfig.vidHeight);
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE,
			stencilReadback);

		for(i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++)
			sum += stencilReadback[i];

		backEnd.pc.c_overDraw += sum;
		ri.hunkfreetemp(stencilReadback);
	}

	if(glRefConfig.framebufferObject){
		/* copy final image to screen */
		Vec2	texScale;
		Vec4	srcBox, dstBox, white;

		texScale[0] =
			texScale[1] = 1.0f;

		white[0] =
			white[1] =
				white[2] = pow(2, tr.overbrightBits);	/* exp2(tr.overbrightBits); */
		white[3] = 1.0f;

		setv34(dstBox, 0, 0, glConfig.vidWidth, glConfig.vidHeight);

		if(backEnd.framePostProcessed){
			/* frame was postprocessed into screen fbo, copy from there */
			setv34(srcBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			FBO_Blit(tr.screenScratchFbo, srcBox, texScale, NULL, dstBox, &tr.textureColorShader,
				white,
				0);
		}else{
			/* frame still in render fbo, copy from there */
			setv34(srcBox, 0, 0, tr.renderFbo->width, tr.renderFbo->height);

			FBO_Blit(tr.renderFbo, srcBox, texScale, NULL, dstBox, &tr.textureColorShader, white,
				0);
		}
	}

	if(!glState.finishCalled){
		qglFinish();
	}

	GLimp_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	GLimp_EndFrame();

	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;

	return (const void*)(cmd + 1);
}

/*
 * RB_CapShadowMap
 *
 */
const void *
RB_CapShadowMap(const void *data)
{
	const capShadowmapCommand_t *cmd = data;

	if(cmd->map != -1){
		GL_SelectTexture(0);
		if(cmd->cubeSide != -1){
			GL_BindCubemap(tr.shadowCubemaps[cmd->map]);
			qglCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, 0, GL_RGBA8,
				backEnd.refdef.x, glConfig.vidHeight - (backEnd.refdef.y + 256), 256, 256,
				0);
		}else{
			GL_Bind(tr.pshadowMaps[cmd->map]);
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, backEnd.refdef.x, glConfig.vidHeight -
				(backEnd.refdef.y + 256), 256, 256, 0);
		}
	}

	return (const void*)(cmd + 1);
}



/*
 * RB_PostProcess
 *
 */
const void *
RB_PostProcess(const void *data)
{
	const postProcessCommand_t *cmd = data;
	Vec4	white;
	Vec2	texScale;
	qbool autoExposure;

	texScale[0] =
		texScale[1] = 1.0f;

	setv34(white, 1, 1, 1, 1);

	if(!r_postProcess->integer){
		/* if we have an FBO, just copy it out, otherwise, do nothing. */
		if(glRefConfig.framebufferObject){
			Vec4 srcBox, dstBox, color;

			setv34(srcBox, 0, 0, tr.renderFbo->width, tr.renderFbo->height);
			/* setv34(dstBox, 0, 0, glConfig.vidWidth, glConfig.vidHeight); */
			setv34(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			color[0] =
				color[1] =
					color[2] = pow(2, r_cameraExposure->value);	/* exp2(r_cameraExposure->value); */
			color[3] = 1.0f;

			/* FBO_Blit(tr.renderFbo, srcBox, texScale, NULL, dstBox, &tr.textureColorShader, color, 0); */
			FBO_Blit(tr.renderFbo, srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, color,
				0);
		}

		backEnd.framePostProcessed = qtrue;

		return (const void*)(cmd + 1);
	}

#if 0
	if(!glRefConfig.framebufferObject){
		/* we couldn't render straight to it, so just cap the screen instead */
		GL_Bind(tr.renderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
	}
#endif

	if(r_hdr->integer && (r_toneMap->integer == 2 || (r_toneMap->integer == 1 && tr.autoExposure))){
		autoExposure =
			(r_autoExposure->integer == 1 && tr.autoExposure) || (r_autoExposure->integer == 2);
		RB_ToneMap(autoExposure);
	}else{
		Vec4 srcBox, dstBox, color;

		setv34(srcBox, 0, 0, tr.renderFbo->width, tr.renderFbo->height);
		setv34(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

		color[0] =
			color[1] =
				color[2] = pow(2, r_cameraExposure->value);	/* exp2(r_cameraExposure->value); */
		color[3] = 1.0f;

		FBO_Blit(tr.renderFbo, srcBox, texScale, tr.screenScratchFbo, dstBox, &tr.textureColorShader,
			color,
			0);
	}

#ifdef REACTION
	RB_GodRays();

	if(1)
		RB_BokehBlur(backEnd.refdef.blurFactor);
	else
		RB_GaussianBlur(backEnd.refdef.blurFactor);
#endif
/*
 *      if (glRefConfig.framebufferObject)
 *      {
 *              // copy final image to screen
 *              Vec4 srcBox, dstBox;
 *
 *              setv34(srcBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);
 *              setv34(dstBox, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
 *
 *              FBO_Blit(tr.screenScratchFbo, srcBox, texScale, NULL, dstBox, &tr.textureColorShader, white, 0);
 *      }
 */
	backEnd.framePostProcessed = qtrue;

	return (const void*)(cmd + 1);
}

/*
 * RB_ExecuteRenderCommands
 *
 * This function will be called synchronously if running without
 * smp extensions, or asynchronously by another thread.
 */
void
RB_ExecuteRenderCommands(const void *data)
{
	int t1, t2;

	t1 = ri.Milliseconds ();

	if(!r_smp->integer || data == backEndData[0]->commands.cmds){
		backEnd.smpFrame = 0;
	}else{
		backEnd.smpFrame = 1;
	}

	while(1){
		data = PADP(data, sizeof(void *));

		switch(*(const int*)data){
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs(data);
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd(data);
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd(data);
			break;
		case RC_COLORMASK:
			data = RB_ColorMask(data);
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth(data);
			break;
		case RC_CAPSHADOWMAP:
			data = RB_CapShadowMap(data);
			break;
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			break;
		case RC_END_OF_LIST:
		default:
			/* stop rendering on this thread */
			t2 = ri.Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

}


/*
 * RB_RenderThread
 */
void
RB_RenderThread(void)
{
	const void *data;

	/* wait for either a rendering command or a quit command */
	while(1){
		/* sleep until we have work to do */
		data = GLimp_RendererSleep();

		if(!data){
			return;	/* all done, renderer is shutting down */
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands(data);

		renderThreadActive = qfalse;
	}
}
