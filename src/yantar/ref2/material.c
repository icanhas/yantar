/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "local.h"

extern qbool ParseShader(char**);

/* 
 * the shader is parsed into these global variables, then copied into
 * dynamic memory if it is valid.
 */
shaderStage_t	stages[MAX_SHADER_STAGES];
material_t shader;
texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
char *s_shaderText;

#define FILE_HASH_SIZE 1024
material_t * hashTable[FILE_HASH_SIZE];
#define MAX_SHADERTEXT_HASH 2048
char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

/*
 * Shader optimization and fogging
 */

/*
 * ComputeStageIteratorFunc
 *
 * See if we can use on of the simple fastpath stage functions,
 * otherwise set to the generic stage function
 */
static void
ComputeStageIteratorFunc(void)
{
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	/* see if this should go into the sky path */
	if(shader.isSky){
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}
}

/*
 * ComputeVertexAttribs: Check which vertex attributes we only need, so we
 * don't need to submit/copy all of them.
 */
static void
ComputeVertexAttribs(void)
{
	int i, stage;

	/* dlights always need ATTR_NORMAL */
	shader.vertexAttribs = ATTR_POSITION | ATTR_NORMAL;

	/* portals always need normals, for SurfIsOffscreen() */
	if(shader.isPortal){
		shader.vertexAttribs |= ATTR_NORMAL;
	}

	if(shader.defaultShader){
		shader.vertexAttribs |= ATTR_TEXCOORD;
		return;
	}

	if(shader.numDeforms){
		for(i = 0; i < shader.numDeforms; i++){
			deformStage_t *ds = &shader.deforms[i];

			switch(ds->deformation){
			case DEFORM_BULGE:
				shader.vertexAttribs |= ATTR_NORMAL | ATTR_TEXCOORD;
				break;

			case DEFORM_AUTOSPRITE:
				shader.vertexAttribs |= ATTR_NORMAL | ATTR_COLOR;
				break;

			case DEFORM_WAVE:
			case DEFORM_NORMALS:
			case DEFORM_TEXT0:
			case DEFORM_TEXT1:
			case DEFORM_TEXT2:
			case DEFORM_TEXT3:
			case DEFORM_TEXT4:
			case DEFORM_TEXT5:
			case DEFORM_TEXT6:
			case DEFORM_TEXT7:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			default:
			case DEFORM_NONE:
			case DEFORM_MOVE:
			case DEFORM_PROJECTION_SHADOW:
			case DEFORM_AUTOSPRITE2:
				break;
			}
		}
	}

	for(stage = 0; stage < MAX_SHADER_STAGES; stage++){
		shaderStage_t *pStage = &stages[stage];

		if(!pStage->active){
			break;
		}

		if(pStage->glslShaderGroup == tr.lightallShader){
			shader.vertexAttribs |= ATTR_NORMAL;

			if(pStage->glslShaderIndex & LIGHTDEF_USE_NORMALMAP){
				shader.vertexAttribs |= ATTR_BITANGENT | ATTR_TANGENT;
			}

			switch(pStage->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK){
			case LIGHTDEF_USE_LIGHTMAP:
			case LIGHTDEF_USE_LIGHT_VERTEX:
				shader.vertexAttribs |= ATTR_LIGHTDIRECTION;
				break;
			default:
				break;
			}
		}

		for(i = 0; i < NUM_TEXTURE_BUNDLES; i++){
			if(pStage->bundle[i].image[0] == 0){
				continue;
			}

			switch(pStage->bundle[i].tcGen){
			case TCGEN_TEXTURE:
				shader.vertexAttribs |= ATTR_TEXCOORD;
				break;
			case TCGEN_LIGHTMAP:
				shader.vertexAttribs |= ATTR_LIGHTCOORD;
				break;
			case TCGEN_ENVIRONMENT_MAPPED:
				shader.vertexAttribs |= ATTR_NORMAL;
				break;

			default:
				break;
			}
		}

		switch(pStage->rgbGen){
		case CGEN_EXACT_VERTEX:
		case CGEN_VERTEX:
		case CGEN_EXACT_VERTEX_LIT:
		case CGEN_VERTEX_LIT:
		case CGEN_ONE_MINUS_VERTEX:
			shader.vertexAttribs |= ATTR_COLOR;
			break;

		case CGEN_LIGHTING_DIFFUSE:
			shader.vertexAttribs |= ATTR_NORMAL;
			break;

		default:
			break;
		}

		switch(pStage->alphaGen){
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_FRESNEL:
			shader.vertexAttribs |= ATTR_NORMAL;
			break;

		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			shader.vertexAttribs |= ATTR_COLOR;
			break;

		default:
			break;
		}
	}
}

typedef struct {
	int	blendA;
	int	blendB;

	int	multitextureEnv;
	int	multitextureBlend;
} collapse_t;

static collapse_t collapse[] = {
	{ 0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, 0 },

	{ 0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },
#if 0
	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
	  GL_DECAL, 0 },
#endif
	{ -1 }
};

/*
 * CollapseMultitexture
 *
 * Attempt to combine two stages into a single multitexture stage
 * FIXME: I think modulated add + modulated add collapses incorrectly
 */
static qbool
CollapseMultitexture(void)
{
	int	abits, bbits;
	int	i;
	textureBundle_t tmpBundle;

	if(!qglActiveTextureARB){
		return qfalse;
	}

	/* make sure both stages are active */
	if(!stages[0].active || !stages[1].active){
		return qfalse;
	}

	/* on voodoo2, don't combine different tmus */
	if(glConfig.driverType == GLDRV_VOODOO){
		if(stages[0].bundle[0].image[0]->TMU ==
		   stages[1].bundle[0].image[0]->TMU){
			return qfalse;
		}
	}

	abits	= stages[0].stateBits;
	bbits	= stages[1].stateBits;

	/* make sure that both stages have identical state other than blend modes */
	if((abits & ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE)) !=
	   (bbits & ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE))){
		return qfalse;
	}

	abits	&= (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
	bbits	&= (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

	/* search for a valid multitexture blend function */
	for(i = 0; collapse[i].blendA != -1; i++)
		if(abits == collapse[i].blendA
		   && bbits == collapse[i].blendB){
			break;
		}

	/* nothing found */
	if(collapse[i].blendA == -1){
		return qfalse;
	}

	/* GL_ADD is a separate extension */
	if(collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable){
		return qfalse;
	}

	/* make sure waveforms have identical parameters */
	if((stages[0].rgbGen != stages[1].rgbGen) ||
	   (stages[0].alphaGen != stages[1].alphaGen)){
		return qfalse;
	}

	/* an add collapse can only have identity colors */
	if(collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY){
		return qfalse;
	}

	if(stages[0].rgbGen == CGEN_WAVEFORM){
		if(memcmp(&stages[0].rgbWave,
			   &stages[1].rgbWave,
			   sizeof(stages[0].rgbWave))){
			return qfalse;
		}
	}
	if(stages[0].alphaGen == AGEN_WAVEFORM){
		if(memcmp(&stages[0].alphaWave,
			   &stages[1].alphaWave,
			   sizeof(stages[0].alphaWave))){
			return qfalse;
		}
	}


	/* make sure that lightmaps are in bundle 1 for 3dfx */
	if(stages[0].bundle[0].isLightmap){
		tmpBundle = stages[0].bundle[0];
		stages[0].bundle[0]	= stages[1].bundle[0];
		stages[0].bundle[1]	= tmpBundle;
	}else{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	/* set the new blend state bits */
	shader.multitextureEnv	= collapse[i].multitextureEnv;
	stages[0].stateBits	&= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
	stages[0].stateBits	|= collapse[i].multitextureBlend;

	/*
	 * move down subsequent shaders
	 *  */
	memmove(&stages[1], &stages[2], sizeof(stages[0]) * (MAX_SHADER_STAGES - 2));
	Q_Memset(&stages[MAX_SHADER_STAGES-1], 0, sizeof(stages[0]));

	return qtrue;
}

static void
CollapseStagesToLightall(shaderStage_t *diffuse,
			 shaderStage_t *normal, shaderStage_t *specular, shaderStage_t *lightmap,
			 qbool useLightVector, qbool useLightVertex, qbool parallax,
			 qbool environment)
{
	int defs = 0;

	/* ri.Printf(PRINT_ALL, "shader %s has diffuse %s", shader.name, diffuse->bundle[0].image[0]->imgName); */

	/* reuse diffuse, mark others inactive */
	diffuse->type = ST_GLSL;

	if(lightmap){
		/* ri.Printf(PRINT_ALL, ", lightmap"); */
		diffuse->bundle[TB_LIGHTMAP] = lightmap->bundle[0];
		defs |= LIGHTDEF_USE_LIGHTMAP;
	}else if(useLightVector){
		defs |= LIGHTDEF_USE_LIGHT_VECTOR;
	}else if(useLightVertex){
		defs |= LIGHTDEF_USE_LIGHT_VERTEX;
	}

	if(r_deluxeMapping->integer && tr.worldDeluxeMapping && lightmap){
		/* ri.Printf(PRINT_ALL, ", deluxemap"); */
		diffuse->bundle[TB_DELUXEMAP] = lightmap->bundle[0];
		diffuse->bundle[TB_DELUXEMAP].image[0] = tr.deluxemaps[shader.lightmapIndex];
		defs |= LIGHTDEF_USE_DELUXEMAP;
	}

	if(r_normalMapping->integer){
		if(normal){
			/* ri.Printf(PRINT_ALL, ", normalmap %s", normal->bundle[0].image[0]->imgName); */
			diffuse->bundle[TB_NORMALMAP] = normal->bundle[0];
			defs |= LIGHTDEF_USE_NORMALMAP;
			if(parallax && r_parallaxMapping->integer)
				defs |= LIGHTDEF_USE_PARALLAXMAP;
		}else if(diffuse->bundle[TB_NORMALMAP].image[0]){
			Img *tmpImg = diffuse->bundle[TB_NORMALMAP].image[0];
			diffuse->bundle[TB_NORMALMAP] = diffuse->bundle[TB_DIFFUSEMAP];
			diffuse->bundle[TB_NORMALMAP].image[0] = tmpImg;
			defs |= LIGHTDEF_USE_NORMALMAP;
		}
	}

	if(r_specularMapping->integer){
		if(specular){
			/* ri.Printf(PRINT_ALL, ", specularmap %s", specular->bundle[0].image[0]->imgName); */
			diffuse->bundle[TB_SPECULARMAP] = specular->bundle[0];
			diffuse->specularReflectance = specular->specularReflectance;
			defs |= LIGHTDEF_USE_SPECULARMAP;
		}else if(diffuse->bundle[TB_SPECULARMAP].image[0]){
			Img *tmpImg = diffuse->bundle[TB_SPECULARMAP].image[0];
			diffuse->bundle[TB_SPECULARMAP] = diffuse->bundle[TB_DIFFUSEMAP];
			diffuse->bundle[TB_SPECULARMAP].image[0] = tmpImg;
			defs |= LIGHTDEF_USE_SPECULARMAP;
		}
	}

	if(environment){
		defs |= LIGHTDEF_TCGEN_ENVIRONMENT;
	}

	/* ri.Printf(PRINT_ALL, ".\n"); */

	diffuse->glslShaderGroup	= tr.lightallShader;
	diffuse->glslShaderIndex	= defs;
}

static qbool
CollapseStagesToGLSL(void)
{
	int i, j, numStages;
	qbool skip = qfalse;

	/* skip shaders with deforms */
	if(shader.numDeforms != 0)
		skip = qtrue;

	if(!skip){
		/* if 2+ stages and first stage is lightmap, switch them
		 * this makes it easier for the later bits to process */
		if(stages[0].active && stages[0].bundle[0].isLightmap && stages[1].active){
			int blendBits = stages[1].stateBits & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

			if(blendBits == (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
				|| blendBits == (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR)){
				int bits0, bits1;
				shaderStage_t swapStage;

				bits0 = stages[0].stateBits;
				bits1 = stages[1].stateBits;

				swapStage = stages[0];
				stages[0] = stages[1];
				stages[1] = swapStage;

				stages[0].stateBits = bits0;
				stages[1].stateBits = bits1;
			}
		}
	}

	if(!skip){
		/* scan for shaders that aren't supported */
		for(i = 0; i < MAX_SHADER_STAGES; i++){
			shaderStage_t *pStage = &stages[i];

			if(!pStage->active)
				continue;

			if(pStage->adjustColorsForFog){
				skip = qtrue;
				break;
			}

			if(pStage->bundle[0].isLightmap){
				int blendBits = pStage->stateBits & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

				if(blendBits != (GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO)
				   && blendBits != (GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR)){
					skip = qtrue;
					break;
				}
			}

			switch(pStage->bundle[0].tcGen){
			case TCGEN_TEXTURE:
			case TCGEN_LIGHTMAP:
			case TCGEN_ENVIRONMENT_MAPPED:
				break;
			default:
				skip = qtrue;
				break;
			}

			switch(pStage->alphaGen){
			case AGEN_LIGHTING_SPECULAR:
			case AGEN_PORTAL:
			case AGEN_FRESNEL:
				skip = qtrue;
				break;
			default:
				break;
			}
		}
	}

	if(!skip){
		for(i = 0; i < MAX_SHADER_STAGES; i++){
			shaderStage_t	*pStage = &stages[i];
			shaderStage_t	*diffuse, *normal, *specular, *lightmap;
			qbool parallax, environment, diffuselit, vertexlit;

			if(!pStage->active)
				continue;

			/* skip normal and specular maps */
			if(pStage->type != ST_COLORMAP)
				continue;

			/* skip lightmaps */
			if(pStage->bundle[0].isLightmap)
				continue;

			diffuse		= pStage;
			normal		= nil;
			parallax	= qfalse;
			specular	= nil;
			lightmap	= nil;

			/* we have a diffuse map, find matching normal, specular, and lightmap */
			for(j = i + 1; j < MAX_SHADER_STAGES; j++){
				shaderStage_t *pStage2 = &stages[j];

				if(!pStage2->active)
					continue;

				switch(pStage2->type){
				case ST_NORMALMAP:
					if(!normal){
						normal = pStage2;
					}
					break;

				case ST_NORMALPARALLAXMAP:
					if(!normal){
						normal		= pStage2;
						parallax	= qtrue;
					}
					break;

				case ST_SPECULARMAP:
					if(!specular){
						specular = pStage2;
					}
					break;

				case ST_COLORMAP:
					if(pStage2->bundle[0].isLightmap){
						lightmap = pStage2;
					}
					break;

				default:
					break;
				}
			}

			environment = qfalse;
			if(diffuse->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED){
				environment = qtrue;
			}

			diffuselit = qfalse;
			if(diffuse->rgbGen == CGEN_LIGHTING_DIFFUSE){
				diffuselit = qtrue;
			}

			vertexlit = qfalse;
			if(diffuse->rgbGen == CGEN_VERTEX_LIT || diffuse->rgbGen == CGEN_EXACT_VERTEX_LIT){
				vertexlit = qtrue;
			}

			CollapseStagesToLightall(diffuse, normal, specular, lightmap, diffuselit, vertexlit,
				parallax,
				environment);
		}

		/* deactivate lightmap stages */
		for(i = 0; i < MAX_SHADER_STAGES; i++){
			shaderStage_t *pStage = &stages[i];

			if(!pStage->active)
				continue;

			if(pStage->bundle[0].isLightmap){
				pStage->active = qfalse;
			}
		}
	}

	/* deactivate normal and specular stages */
	for(i = 0; i < MAX_SHADER_STAGES; i++){
		shaderStage_t *pStage = &stages[i];

		if(!pStage->active)
			continue;

		if(pStage->type == ST_NORMALMAP){
			pStage->active = qfalse;
		}

		if(pStage->type == ST_NORMALPARALLAXMAP){
			pStage->active = qfalse;
		}

		if(pStage->type == ST_SPECULARMAP){
			pStage->active = qfalse;
		}
	}

	/* remove inactive stages */
	numStages = 0;
	for(i = 0; i < MAX_SHADER_STAGES; i++){
		if(!stages[i].active)
			continue;

		if(i == numStages){
			numStages++;
			continue;
		}

		stages[numStages]	= stages[i];
		stages[i].active	= qfalse;
		numStages++;
	}

	if(numStages == i && i >= 2 && CollapseMultitexture())
		numStages--;

	return numStages;
}

/*
 *
 * FixRenderCommandList
 * https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
 * Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
 * but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
 * to be rendered with bad shaders. To fix this, need to go through all render commands and fix
 * sortedIndex.
 */
static void
FixRenderCommandList(int newShader)
{
	renderCommandList_t *cmdList = &backEndData[tr.smpFrame]->commands;

	if(cmdList){
		const void *curCmd = cmdList->cmds;

		for(;;){
			curCmd = PADP(curCmd, sizeof(void *));

			switch(*(const int*)curCmd){
			case RC_SET_COLOR:
			{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t*)curCmd;
				curCmd = (const void*)(sc_cmd + 1);
				break;
			}
			case RC_STRETCH_PIC:
			{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t*)curCmd;
				curCmd = (const void*)(sp_cmd + 1);
				break;
			}
			case RC_DRAW_SURFS:
			{
				int i;
				drawSurf_t	*drawSurf;
				material_t	*shader;
				int	fogNum;
				int	entityNum;
				int	dlightMap;
				int	pshadowMap;
				int	sortedIndex;
				const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t*)curCmd;

				for(i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs;
				    i++, drawSurf++){
					R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum,
						&dlightMap,
						&pshadowMap);
					sortedIndex =
						((drawSurf->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS-1));
					if(sortedIndex >= newShader){
						sortedIndex++;
						drawSurf->sort =
							(sortedIndex <<
							 QSORT_SHADERNUM_SHIFT) | entityNum |
							(fogNum <<
							 QSORT_FOGNUM_SHIFT) |
							((int)pshadowMap <<
							 QSORT_PSHADOW_SHIFT) | (int)dlightMap;
					}
				}
				curCmd = (const void*)(ds_cmd + 1);
				break;
			}
			case RC_DRAW_BUFFER:
			{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t*)curCmd;
				curCmd = (const void*)(db_cmd + 1);
				break;
			}
			case RC_SWAP_BUFFERS:
			{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t*)curCmd;
				curCmd = (const void*)(sb_cmd + 1);
				break;
			}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}

/*
 * SortNewShader
 *
 * Positions the most recently created shader in the tr.sortedShaders[]
 * array so that the shader->sort key is sorted reletive to the other
 * shaders.
 *
 * Sets shader->sortedIndex
 */
static void
SortNewShader(void)
{
	int i;
	float sort;
	material_t *newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for(i = tr.numShaders - 2; i >= 0; i--){
		if(tr.sortedShaders[ i ]->sort <= sort){
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
		tr.sortedShaders[i+1]->sortedIndex++;
	}

	/* Arnout: fix rendercommandlist
	 * https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493 */
	FixRenderCommandList(i+1);

	newShader->sortedIndex	= i+1;
	tr.sortedShaders[i+1]	= newShader;
}


/*
 * GeneratePermanentShader
 */
static material_t *
GeneratePermanentShader(void)
{
	material_t	*newShader;
	int	i, b;
	int	size, hash;

	if(tr.numShaders == MAX_SHADERS){
		ri.Printf(PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = ri.hunkalloc(sizeof(material_t), h_low);

	*newShader = shader;

	if(shader.sort <= SS_OPAQUE){
		newShader->fogPass = FP_EQUAL;
	}else if(shader.contentFlags & CONTENTS_FOG){
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for(i = 0; i < newShader->numUnfoggedPasses; i++){
		if(!stages[i].active){
			break;
		}
		newShader->stages[i]	= ri.hunkalloc(sizeof(stages[i]), h_low);
		*newShader->stages[i]	= stages[i];

		for(b = 0; b < NUM_TEXTURE_BUNDLES; b++){
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof(texModInfo_t);
			newShader->stages[i]->bundle[b].texMods = ri.hunkalloc(size, h_low);
			Q_Memcpy(newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size);
		}
	}

	SortNewShader();

	hash = Q_hashstr(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
 * VertexLightingCollapse
 *
 * If vertex lighting is enabled, only render a single
 * pass, trying to guess which is the correct one to best aproximate
 * what it is supposed to look like.
 */
static void
VertexLightingCollapse(void)
{
	int	stage;
	shaderStage_t *bestStage;
	int	bestImageRank;
	int	rank;

	/* if we aren't opaque, just use the first pass */
	if(shader.sort == SS_OPAQUE){

		/* pick the best texture for the single pass */
		bestStage = &stages[0];
		bestImageRank = -999999;

		for(stage = 0; stage < MAX_SHADER_STAGES; stage++){
			shaderStage_t *pStage = &stages[stage];

			if(!pStage->active){
				break;
			}
			rank = 0;

			if(pStage->bundle[0].isLightmap){
				rank -= 100;
			}
			if(pStage->bundle[0].tcGen != TCGEN_TEXTURE){
				rank -= 5;
			}
			if(pStage->bundle[0].numTexMods){
				rank -= 5;
			}
			if(pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING){
				rank -= 3;
			}

			if(rank > bestImageRank){
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0]	= bestStage->bundle[0];
		stages[0].stateBits	&= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
		stages[0].stateBits	|= GLS_DEPTHMASK_TRUE;
		if(shader.lightmapIndex == LIGHTMAP_NONE){
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		}else{
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	}else{
		/* don't use a lightmap (tesla coils) */
		if(stages[0].bundle[0].isLightmap){
			stages[0] = stages[1];
		}

		/* if we were in a cross-fade cgen, hack it to normal */
		if(stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY){
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH)
		   && (stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH)){
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH)
		   && (stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH)){
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for(stage = 1; stage < MAX_SHADER_STAGES; stage++){
		shaderStage_t *pStage = &stages[stage];

		if(!pStage->active){
			break;
		}

		Q_Memset(pStage, 0, sizeof(*pStage));
	}
}

/*
 * FinishShader
 *
 * Returns a freshly allocated shader with all the needed info
 * from the current global working shader
 */
static material_t *
FinishShader(void)
{
	int stage;
	qbool		hasLightmapStage;
	qbool		vertexLightmap;

	hasLightmapStage	= qfalse;
	vertexLightmap		= qfalse;

	/*
	 * set sky stuff appropriate
	 *  */
	if(shader.isSky){
		shader.sort = SS_ENVIRONMENT;
	}

	/*
	 * set polygon offset
	 *  */
	if(shader.polygonOffset && !shader.sort){
		shader.sort = SS_DECAL;
	}

	/*
	 * set appropriate stage information
	 *  */
	for(stage = 0; stage < MAX_SHADER_STAGES; ){
		shaderStage_t *pStage = &stages[stage];

		if(!pStage->active){
			break;
		}

		/* check for a missing texture */
		if(!pStage->bundle[0].image[0]){
			ri.Printf(PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name);
			pStage->active = qfalse;
			stage++;
			continue;
		}

		/*
		 * ditch this stage if it's detail and detail textures are disabled
		 *  */
		if(pStage->isDetail && !r_detailTextures->integer){
			int index;

			for(index = stage + 1; index < MAX_SHADER_STAGES; index++)
				if(!stages[index].active)
					break;

			if(index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage));
			else{
				if(stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage - 1));

				Q_Memset(&stages[index - 1], 0, sizeof(*stages));
			}

			continue;
		}

		/*
		 * default texture coordinate generation
		 *  */
		if(pStage->bundle[0].isLightmap){
			if(pStage->bundle[0].tcGen == TCGEN_BAD){
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		}else{
			if(pStage->bundle[0].tcGen == TCGEN_BAD){
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}


		/* not a true lightmap but we want to leave existing
		 * behaviour in place and not print out a warning
		 * if (pStage->rgbGen == CGEN_VERTEX) {
		 *  vertexLightmap = qtrue;
		 * } */



		/*
		 * determine sort order and fog color adjustment
		 *  */
		if((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) &&
		   (stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))){
			int	blendSrcBits	= pStage->stateBits & GLS_SRCBLEND_BITS;
			int	blendDstBits	= pStage->stateBits & GLS_DSTBLEND_BITS;

			/* fog color adjustment only works for blend modes that have a contribution
			 * that aproaches 0 as the modulate values aproach 0 --
			 * GL_ONE, GL_ONE
			 * GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			 * GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA */

			/* modulate, additive */
			if(((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) ||
			   ((blendSrcBits == GLS_SRCBLEND_ZERO) &&
			    (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))){
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			/* strict blend */
			else if((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) &&
				(blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)){
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			/* premultiplied alpha */
			else if((blendSrcBits == GLS_SRCBLEND_ONE) &&
				(blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)){
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			}else{
				/* we can't adjust this one correctly, so it won't be exactly correct in fog */
			}

			/* don't screw with sort order if this is a portal or environment */
			if(!shader.sort){
				/* see through item, like a grill or grate */
				if(pStage->stateBits & GLS_DEPTHMASK_TRUE){
					shader.sort = SS_SEE_THROUGH;
				}else{
					shader.sort = SS_BLEND0;
				}
			}
		}

		stage++;
	}

	/* there are times when you will need to manually apply a sort to
	 * opaque alpha tested shaders that have later blend passes */
	if(!shader.sort){
		shader.sort = SS_OPAQUE;
	}

	/*
	 * if we are in r_vertexLight mode, never use a lightmap texture
	 *  */
	if(stage > 1 &&
	   ((r_vertexLight->integer &&
	     !r_uiFullScreen->integer) || glConfig.hardwareType == GLHW_PERMEDIA2)){
		VertexLightingCollapse();
		stage = 1;
		hasLightmapStage = qfalse;
	}

	/*
	 * look for multitexture potential
	 *  */
	stage = CollapseStagesToGLSL();

	if(shader.lightmapIndex >= 0 && !hasLightmapStage){
		if(vertexLightmap){
			ri.Printf(PRINT_DEVELOPER, "WARNING: shader '%s' has VERTEX forced lightmap!\n",
				shader.name);
		}else{
			ri.Printf(PRINT_DEVELOPER,
				"WARNING: shader '%s' has lightmap but no lightmap stage!\n",
				shader.name);
			/* Don't set this, it will just add duplicate shaders to the hash
			 * shader.lightmapIndex = LIGHTMAP_NONE; */
		}
	}


	/*
	 * compute number of passes
	 *  */
	shader.numUnfoggedPasses = stage;

	/* fogonly shaders don't have any normal passes */
	if(stage == 0 && !shader.isSky)
		shader.sort = SS_FOG;

	/* determine which stage iterator function is appropriate */
	ComputeStageIteratorFunc();

	/* determine which vertex attributes this shader needs */
	ComputeVertexAttribs();

	return GeneratePermanentShader();
}

/* ======================================================================================== */

void
R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset)
{
	char		strippedName[MAX_QPATH];
	int		hash;
	material_t	*sh, *sh2;
	Handle		h;

	sh = R_FindShaderByName(shaderName);
	if(sh == nil || sh == tr.defaultShader){
		h = RE_RegisterShaderLightMap(shaderName, 0);
		sh = R_GetShaderByHandle(h);
	}
	if(sh == nil || sh == tr.defaultShader){
		ri.Printf(PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName);
		return;
	}

	sh2 = R_FindShaderByName(newShaderName);
	if(sh2 == nil || sh2 == tr.defaultShader){
		h = RE_RegisterShaderLightMap(newShaderName, 0);
		sh2 = R_GetShaderByHandle(h);
	}

	if(sh2 == nil || sh2 == tr.defaultShader){
		ri.Printf(PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName);
		return;
	}

	/*
	 * remap all the materials with the given name
	 * even though they might have different lightmaps 
	 */
	Q_stripext(shaderName, strippedName, sizeof(strippedName));
	hash = Q_hashstr(strippedName, FILE_HASH_SIZE);
	for(sh = hashTable[hash]; sh; sh = sh->next)
		if(Q_stricmp(sh->name, strippedName) == 0){
			if(sh != sh2){
				sh->remappedShader = sh2;
			}else{
				sh->remappedShader = nil;
			}
		}

	if(timeOffset){
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
 * FindShaderInShaderText
 *
 * Scans the combined text description of all the shader files for
 * the given shader name.
 *
 * return nil if not found
 *
 * If found, it will return a valid shader
 */
static char *
FindShaderInShaderText(const char *shadername)
{

	char	*token, *p;

	int	i, hash;

	hash = Q_hashstr(shadername, MAX_SHADERTEXT_HASH);

	if(shaderTextHashTable[hash]){
		for(i = 0; shaderTextHashTable[hash][i]; i++){
			p = shaderTextHashTable[hash][i];
			token = Q_readtok2(&p, qtrue);

			if(!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;
	if(!p)
		return nil;

	/* look for label */
	for(;;){
		token = Q_readtok2(&p, qtrue);
		if(token[0] == 0){
			break;
		}

		if(!Q_stricmp(token, shadername)){
			return p;
		}else{
			/* skip the definition */
			Q_skipblock(&p);
		}
	}

	return nil;
}

/*
 * R_FindShaderByName
 *
 * Will always return a valid shader, but it might be the
 * default shader if the real one can't be found.
 */
material_t *
R_FindShaderByName(const char *name)
{
	char	strippedName[MAX_QPATH];
	int	hash;
	material_t *sh;

	if((name==nil) || (name[0] == 0)){
		return tr.defaultShader;
	}

	Q_stripext(name, strippedName, sizeof(strippedName));

	hash = Q_hashstr(strippedName, FILE_HASH_SIZE);

	/*
	 * see if the shader is already loaded
	 *  */
	for(sh=hashTable[hash]; sh; sh=sh->next)
		/* NOTE: if there was no shader or image available with the name strippedName
		 * then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		 * have to check all default shaders otherwise for every call to R_FindShader
		 * with that same strippedName a new default shader is created. */
		if(Q_stricmp(sh->name, strippedName) == 0){
			/* match found */
			return sh;
		}

	return tr.defaultShader;
}

/*
 * R_FindShader
 *
 * Will always return a valid shader, but it might be the
 * default shader if the real one can't be found.
 *
 * In the interest of not requiring an explicit shader text entry to
 * be defined for every single image used in the game, three default
 * shader behaviors can be auto-created for any image:
 *
 * If lightmapIndex == LIGHTMAP_NONE, then the image will have
 * dynamic diffuse lighting applied to it, as apropriate for most
 * entity skin surfaces.
 *
 * If lightmapIndex == LIGHTMAP_2D, then the image will be used
 * for 2D rendering unless an explicit shader is found
 *
 * If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
 * the vertex rgba modulate values, as apropriate for misc_model
 * pre-lit surfaces.
 *
 * Other lightmapIndex values will have a lightmap stage created
 * and src*dest blending applied with the texture, as apropriate for
 * most world construction surfaces.
 *
 */
material_t *
R_FindShader(const char *name, int lightmapIndex, qbool mipRawImage)
{
	char strippedName[MAX_QPATH];
	int i, hash;
	char *shaderText;
	Img *image;
	material_t *sh;

	if(name[0] == 0)
		return tr.defaultShader;

	/* use (fullbright) vertex lighting if the bsp file doesn't have
	 * lightmaps */
	if(lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps){
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}else if(lightmapIndex < LIGHTMAP_2D){
		/* negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex]) */
		ri.Printf(PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name,
			lightmapIndex);
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}

	Q_stripext(name, strippedName, sizeof(strippedName));

	hash = Q_hashstr(strippedName, FILE_HASH_SIZE);

	/* see if the shader is already loaded */
	for(sh = hashTable[hash]; sh; sh = sh->next)
		/* NOTE: if there was no shader or image available with the name strippedName
		 * then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		 * have to check all default shaders otherwise for every call to R_FindShader
		 * with that same strippedName a new default shader is created. */
		if((sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
		   !Q_stricmp(sh->name, strippedName)){
			/* match found */
			return sh;
		}

	/* make sure the render thread is stopped, because we are probably
	 * going to have to upload an image */
	if(r_smp->integer)
		R_SyncRenderThread();

	/* clear the global shader */
	Q_Memset(&shader, 0, sizeof(shader));
	Q_Memset(&stages, 0, sizeof(stages));
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;
	for(i = 0; i < MAX_SHADER_STAGES; i++)
		stages[i].bundle[0].texMods = texMods[i];

	/* attempt to define shader from an explicit parameter file */
	shaderText = FindShaderInShaderText(strippedName);
	if(shaderText){
		/* enable this when building a pak file to get a global list
		 * of all explicit shaders */
		if(r_printShaders->integer){
			ri.Printf(PRINT_ALL, "*SHADER* %s\n", name);
		}

		if(!ParseShader(&shaderText)){
			/* had errors, so use default shader */
			shader.defaultShader = qtrue;
		}
		sh = FinishShader();
		return sh;
	}


	/*
	 * if not defined in the in-memory shader descriptions,
	 * look for a single supported image file
	 */
	image = R_FindImageFile(name, mipRawImage, mipRawImage, mipRawImage ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	if(!image){
		ri.Printf(PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name);
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	/*
	 * create the default shading commands
	 */
	if(shader.lightmapIndex == LIGHTMAP_NONE){
		/* dynamic colors at vertexes */
		stages[0].bundle[0].image[0] = image;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}else if(shader.lightmapIndex == LIGHTMAP_BY_VERTEX){
		/* explicit colors at vertexes */
		stages[0].bundle[0].image[0] = image;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_EXACT_VERTEX;
		stages[0].alphaGen	= AGEN_SKIP;
		stages[0].stateBits	= GLS_DEFAULT;
	}else if(shader.lightmapIndex == LIGHTMAP_2D){
		/* GUI elements */
		stages[0].bundle[0].image[0] = image;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_VERTEX;
		stages[0].alphaGen	= AGEN_VERTEX;
		stages[0].stateBits	= GLS_DEPTHTEST_DISABLE |
					  GLS_SRCBLEND_SRC_ALPHA |
					  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}else if(shader.lightmapIndex == LIGHTMAP_WHITEIMAGE){
		/* fullbright level */
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active	= qtrue;
		stages[1].rgbGen	= CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}else{
		/* two pass lightmap */
		stages[0].bundle[0].image[0]	= tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap	= qtrue;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_IDENTITY;	/* lightmaps are scaled on creation */
		/* for identitylight */
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active	= qtrue;
		stages[1].rgbGen	= CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}

Handle
RE_RegisterShaderFromImage(const char *name, int lightmapIndex, Img *image, qbool mipRawImage)
{
	int i, hash;
	material_t *sh;

	(void)mipRawImage; /* shut up 'unused' warning -- remove param? */

	hash = Q_hashstr(name, FILE_HASH_SIZE);

	/* probably not necessary since this function
	 * only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	 * but better safe than sorry. */
	if(lightmapIndex >= tr.numLightmaps){
		lightmapIndex = LIGHTMAP_WHITEIMAGE;
	}

	/*
	 * see if the shader is already loaded
	 *  */
	for(sh=hashTable[hash]; sh; sh=sh->next)
		/* NOTE: if there was no shader or image available with the name strippedName
		 * then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		 * have to check all default shaders otherwise for every call to R_FindShader
		 * with that same strippedName a new default shader is created. */
		if((sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
			/* index by name */
		   !Q_stricmp(sh->name, name)){
			/* match found */
			return sh->index;
		}

	/* make sure the render thread is stopped, because we are probably
	 * going to have to upload an image */
	if(r_smp->integer){
		R_SyncRenderThread();
	}

	/* clear the global shader */
	Q_Memset(&shader, 0, sizeof(shader));
	Q_Memset(&stages, 0, sizeof(stages));
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;
	for(i = 0; i < MAX_SHADER_STAGES; i++)
		stages[i].bundle[0].texMods = texMods[i];

	/*
	 * create the default shading commands
	 */
	if(shader.lightmapIndex == LIGHTMAP_NONE){
		/* dynamic colors at vertexes */
		stages[0].bundle[0].image[0] = image;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}else if(shader.lightmapIndex == LIGHTMAP_BY_VERTEX){
		/* explicit colors at vertexes */
		stages[0].bundle[0].image[0] = image;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_EXACT_VERTEX;
		stages[0].alphaGen	= AGEN_SKIP;
		stages[0].stateBits	= GLS_DEFAULT;
	}else if(shader.lightmapIndex == LIGHTMAP_2D){
		/* GUI elements */
		stages[0].bundle[0].image[0] = image;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_VERTEX;
		stages[0].alphaGen	= AGEN_VERTEX;
		stages[0].stateBits	= GLS_DEPTHTEST_DISABLE |
					  GLS_SRCBLEND_SRC_ALPHA |
					  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}else if(shader.lightmapIndex == LIGHTMAP_WHITEIMAGE){
		/* fullbright level */
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active	= qtrue;
		stages[1].rgbGen	= CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}else{
		/* two pass lightmap */
		stages[0].bundle[0].image[0]	= tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap	= qtrue;
		stages[0].active	= qtrue;
		stages[0].rgbGen	= CGEN_IDENTITY;	/* lightmaps are scaled on creation */
		/* for identitylight */
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active	= qtrue;
		stages[1].rgbGen	= CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}

/*
 * RE_RegisterShader
 *
 * This is the exported shader entry point for the rest of the system
 * It will always return an index that will be valid.
 *
 * This should really only be used for explicit shaders, because there is no
 * way to ask for different implicit lighting modes (vertex, lightmap, etc)
 */
Handle
RE_RegisterShaderLightMap(const char *name, int lightmapIndex)
{
	material_t *sh;

	if(strlen(name) >= MAX_QPATH){
		ri.Printf(PRINT_ALL, "Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, lightmapIndex, qtrue);

	/* we want to return 0 if the shader failed to
	 * load for some reason, but R_FindShader should
	 * still keep a name allocated for it, so if
	 * something calls RE_RegisterShader again with
	 * the same name, we don't try looking for it again */
	if(sh->defaultShader)
		return 0;

	return sh->index;
}

/*
 * RE_RegisterShader
 *
 * This is the exported shader entry point for the rest of the system
 * It will always return an index that will be valid.
 *
 * This should really only be used for explicit shaders, because there is no
 * way to ask for different implicit lighting modes (vertex, lightmap, etc)
 */
Handle
RE_RegisterShader(const char *name)
{
	material_t *sh;

	if(strlen(name) >= MAX_QPATH){
		ri.Printf(PRINT_ALL, "Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, LIGHTMAP_2D, qtrue);

	/* we want to return 0 if the shader failed to
	 * load for some reason, but R_FindShader should
	 * still keep a name allocated for it, so if
	 * something calls RE_RegisterShader again with
	 * the same name, we don't try looking for it again */
	if(sh->defaultShader)
		return 0;

	return sh->index;
}

/*
 * RE_RegisterShaderNoMip
 *
 * For menu graphics that should never be picmiped
 */
Handle
RE_RegisterShaderNoMip(const char *name)
{
	material_t *sh;

	if(strlen(name) >= MAX_QPATH){
		ri.Printf(PRINT_ALL, "Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	sh = R_FindShader(name, LIGHTMAP_2D, qfalse);

	/* we want to return 0 if the shader failed to
	 * load for some reason, but R_FindShader should
	 * still keep a name allocated for it, so if
	 * something calls RE_RegisterShader again with
	 * the same name, we don't try looking for it again */
	if(sh->defaultShader){
		return 0;
	}

	return sh->index;
}

/*
 * R_GetShaderByHandle
 *
 * When a handle is passed in by another module, this range checks
 * it and returns a valid (possibly default) material_t to be used internally.
 */
material_t *
R_GetShaderByHandle(Handle hShader)
{
	if(hShader < 0){
		ri.Printf(PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	if(hShader >= tr.numShaders){
		ri.Printf(PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
 * R_ShaderList_f
 *
 * Dump information on all valid shaders to the console
 * A second parameter will cause it to print in sorted order
 */
void
R_ShaderList_f(void)
{
	int	i;
	int	count;
	material_t *shader;

	ri.Printf (PRINT_ALL, "-----------------------\n");

	count = 0;
	for(i = 0; i < tr.numShaders; i++){
		if(ri.cmdargc() > 1){
			shader = tr.sortedShaders[i];
		}else{
			shader = tr.shaders[i];
		}

		ri.Printf(PRINT_ALL, "%i ", shader->numUnfoggedPasses);

		if(shader->lightmapIndex >= 0){
			ri.Printf (PRINT_ALL, "L ");
		}else{
			ri.Printf (PRINT_ALL, "  ");
		}
		if(shader->multitextureEnv == GL_ADD){
			ri.Printf(PRINT_ALL, "MT(a) ");
		}else if(shader->multitextureEnv == GL_MODULATE){
			ri.Printf(PRINT_ALL, "MT(m) ");
		}else if(shader->multitextureEnv == GL_DECAL){
			ri.Printf(PRINT_ALL, "MT(d) ");
		}else{
			ri.Printf(PRINT_ALL, "      ");
		}
		if(shader->explicitlyDefined){
			ri.Printf(PRINT_ALL, "E ");
		}else{
			ri.Printf(PRINT_ALL, "  ");
		}

		if(shader->optimalStageIteratorFunc == RB_StageIteratorGeneric){
			ri.Printf(PRINT_ALL, "gen ");
		}else if(shader->optimalStageIteratorFunc == RB_StageIteratorSky){
			ri.Printf(PRINT_ALL, "sky ");
		}else{
			ri.Printf(PRINT_ALL, "    ");
		}

		if(shader->defaultShader){
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name);
		}else{
			ri.Printf (PRINT_ALL,  ": %s\n", shader->name);
		}
		count++;
	}
	ri.Printf (PRINT_ALL, "%i total shaders\n", count);
	ri.Printf (PRINT_ALL, "------------------\n");
}

enum { MAX_SHADER_FILES = 4096 };
/*
 * ScanAndLoadShaderFiles
 *
 * Finds and loads all .shader files, combining them into
 * a single large text block that can be scanned for shader names
 */
static void
ScanAndLoadShaderFiles(void)
{
	char	**shaderFiles;
	char	*buffers[MAX_SHADER_FILES];
	char	*p;
	int	numShaderFiles;
	int	i;
	char	*oldp, *token, *hashMem, *textEnd;
	int	shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;

	long	sum = 0, summand;
	/* scan for shader files */
	shaderFiles = ri.fslistfiles("scripts", ".shader", &numShaderFiles);

	if(!shaderFiles || !numShaderFiles){
		ri.Printf(PRINT_WARNING, "WARNING: no shader files found\n");
		return;
	}

	if(numShaderFiles > MAX_SHADER_FILES){
		numShaderFiles = MAX_SHADER_FILES;
	}

	/* load and parse shader files */
	for(i = 0; i < numShaderFiles; i++){
		char filename[MAX_QPATH];

		Q_sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);
		ri.Printf(PRINT_DEVELOPER, "...loading '%s'\n", filename);
		summand = ri.fsreadfile(filename, (void**)&buffers[i]);

		if(!buffers[i])
			ri.Error(ERR_DROP, "Couldn't load %s", filename);

		/* Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders. */
		p = buffers[i];
		for(;;){
			token = Q_readtok2(&p, qtrue);

			if(!*token)
				break;

			oldp = p;

			token = Q_readtok2(&p, qtrue);
			if(token[0] != '{' && token[1] != '\0'){
				ri.Printf(PRINT_WARNING,
					"WARNING: Bad shader file %s has incorrect syntax.\n",
					filename);
				ri.fsfreefile(buffers[i]);
				buffers[i] = nil;
				break;
			}

			Q_skipblock(&oldp);
			p = oldp;
		}

		if(buffers[i])
			sum += summand;
	}

	/* build single large buffer */
	s_shaderText = ri.hunkalloc(sum + numShaderFiles*2, h_low);
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;

	/* free in reverse order, so the temp files are all dumped */
	for(i = numShaderFiles - 1; i >= 0; i--){
		if(!buffers[i])
			continue;

		strcat(textEnd, buffers[i]);
		strcat(textEnd, "\n");
		textEnd += strlen(textEnd);
		ri.fsfreefile(buffers[i]);
	}

	Q_compresstr(s_shaderText);

	/* free up memory */
	ri.fsfreefilelist(shaderFiles);

	Q_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	/* look for shader names */
	for(;;){
		token = Q_readtok2(&p, qtrue);
		if(token[0] == 0)
			break;

		hash = Q_hashstr(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		Q_skipblock(&p);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = ri.hunkalloc(size * sizeof(char *), h_low);

	for(i = 0; i < MAX_SHADERTEXT_HASH; i++){
		shaderTextHashTable[i] = (char**)hashMem;
		hashMem = ((char*)hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Q_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	/* look for shader names */
	for(;;){
		oldp	= p;
		token	= Q_readtok2(&p, qtrue);
		if(token[0] == 0)
			break;

		hash = Q_hashstr(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		Q_skipblock(&p);
	}
	return;
}

static void
CreateInternalShaders(void)
{
	tr.numShaders = 0;

	/* init the default shader */
	Q_Memset(&shader, 0, sizeof(shader));
	Q_Memset(&stages, 0, sizeof(stages));

	Q_strncpyz(shader.name, "<default>", sizeof(shader.name));

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	/* shadow shader is just a marker */
	Q_strncpyz(shader.name, "<stencil shadow>", sizeof(shader.name));
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void
CreateExternalShaders(void)
{
	tr.projectionShadowShader = R_FindShader("projectionShadow", LIGHTMAP_NONE, qtrue);
	tr.flareShader = R_FindShader("flareShader", LIGHTMAP_NONE, qtrue);

	/* Hack to make fogging work correctly on flares. Fog colors are calculated
	 * in tr_flare.c already. */
	if(!tr.flareShader->defaultShader){
		int index;

		for(index = 0; index < tr.flareShader->numUnfoggedPasses; index++){
			tr.flareShader->stages[index]->adjustColorsForFog = ACFF_NONE;
			tr.flareShader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
		}
	}

	tr.sunShader = R_FindShader("sun", LIGHTMAP_NONE, qtrue);
}

void
R_InitShaders(void)
{
	ri.Printf(PRINT_ALL, "Initializing Shaders\n");
	Q_Memset(hashTable, 0, sizeof(hashTable));
	CreateInternalShaders();
	ScanAndLoadShaderFiles();
	CreateExternalShaders();
}

