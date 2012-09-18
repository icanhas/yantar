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
#include "local.h"

/* 
 * the shader is parsed into these global variables, then copied into
 * dynamic memory if it is valid.
 */
extern shaderStage_t	stages[MAX_SHADER_STAGES];
extern material_t shader;
extern texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

static qbool
ParseVector(char **text, int count, float *v)
{
	char	*token;
	int	i;

	/* FIXME: spaces are currently required after parens, should change parseext... */
	token = Q_readtok2(text, qfalse);
	if(strcmp(token, "(")){
		ri.Printf(PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	for(i = 0; i < count; i++){
		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n",
				shader.name);
			return qfalse;
		}
		v[i] = atof(token);
	}

	token = Q_readtok2(text, qfalse);
	if(strcmp(token, ")")){
		ri.Printf(PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	return qtrue;
}

static unsigned
NameToAFunc(const char *funcname)
{
	if(!Q_stricmp(funcname, "GT0")){
		return GLS_ATEST_GT_0;
	}else if(!Q_stricmp(funcname, "LT128")){
		return GLS_ATEST_LT_80;
	}else if(!Q_stricmp(funcname, "GE128")){
		return GLS_ATEST_GE_80;
	}

	ri.Printf(PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname,
		shader.name);
	return 0;
}

static int
NameToSrcBlendMode(const char *name)
{
	if(!Q_stricmp(name, "GL_ONE"))
		return GLS_SRCBLEND_ONE;
	else if(!Q_stricmp(name, "GL_ZERO"))
		return GLS_SRCBLEND_ZERO;
	else if(!Q_stricmp(name, "GL_DST_COLOR"))
		return GLS_SRCBLEND_DST_COLOR;
	else if(!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR"))
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	else if(!Q_stricmp(name, "GL_SRC_ALPHA"))
		return GLS_SRCBLEND_SRC_ALPHA;
	else if(!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	else if(!Q_stricmp(name, "GL_DST_ALPHA"))
		return GLS_SRCBLEND_DST_ALPHA;
	else if(!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	else if(!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE"))
		return GLS_SRCBLEND_ALPHA_SATURATE;

	ri.Printf(PRINT_WARNING, "WARNING: unknown blend mode '%s'"
		" in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_SRCBLEND_ONE;
}

static int
NameToDstBlendMode(const char *name)
{
	if(!Q_stricmp(name, "GL_ONE"))
		return GLS_DSTBLEND_ONE;
	else if(!Q_stricmp(name, "GL_ZERO"))
		return GLS_DSTBLEND_ZERO;
	else if(!Q_stricmp(name, "GL_SRC_ALPHA"))
		return GLS_DSTBLEND_SRC_ALPHA;
	else if(!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	else if(!Q_stricmp(name, "GL_DST_ALPHA"))
		return GLS_DSTBLEND_DST_ALPHA;
	else if(!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	else if(!Q_stricmp(name, "GL_SRC_COLOR"))
		return GLS_DSTBLEND_SRC_COLOR;
	else if(!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR"))
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;

	ri.Printf(PRINT_WARNING, "WARNING: unknown blend mode '%s'"
		" in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_DSTBLEND_ONE;
}

static genFunc_t
NameToGenFunc(const char *funcname)
{
	if(!Q_stricmp(funcname, "sin"))
		return GF_SIN;
	else if(!Q_stricmp(funcname, "square"))
		return GF_SQUARE;
	else if(!Q_stricmp(funcname, "triangle"))
		return GF_TRIANGLE;
	else if(!Q_stricmp(funcname, "sawtooth"))
		return GF_SAWTOOTH;
	else if(!Q_stricmp(funcname, "inversesawtooth"))
		return GF_INVERSE_SAWTOOTH;
	else if(!Q_stricmp(funcname, "noise"))
		return GF_NOISE;

	ri.Printf(PRINT_WARNING, "WARNING: invalid genfunc name '%s'"
		" in shader '%s'\n", funcname, shader.name);
	return GF_SIN;
}

static void
ParseWaveForm(char **text, waveForm_t *wave)
{
	char *token;

	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->func = NameToGenFunc(token);

	/* BASE, AMP, PHASE, FREQ */
	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->base = atof(token);

	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->amplitude = atof(token);

	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->phase = atof(token);

	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->frequency = atof(token);
}

static void
ParseTexMod(char *_text, shaderStage_t *stage)
{
	const char *token;
	char **text = &_text;
	texModInfo_t *tmi;

	if(stage->bundle[0].numTexMods == TR_MAX_TEXMODS){
		ri.Error(ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name);
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = Q_readtok2(text, qfalse);

	if(!Q_stricmp(token, "turb")){
		/* turb */
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->wave.base = atof(token);
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_TURBULENT;
	}else if(!Q_stricmp(token, "scale")){
		/* scale */
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scale[0] = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scale[1] = atof(token);
		tmi->type = TMOD_SCALE;
	}else if(!Q_stricmp(token, "scroll")){
		/* scroll */
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->scroll[0] = atof(token);
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->scroll[1] = atof(token);
		tmi->type = TMOD_SCROLL;
	}else if(!Q_stricmp(token, "stretch")){
		/* stretch */
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->wave.func = NameToGenFunc(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->wave.base = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->wave.phase = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_STRETCH;
	}else if(!Q_stricmp(token, "transform")){
		/* transform */
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->matrix[0][0] = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->matrix[0][1] = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->matrix[1][0] = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->matrix[1][1] = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->translate[0] = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->translate[1] = atof(token);

		tmi->type = TMOD_TRANSFORM;
	}else if(!Q_stricmp(token, "rotate")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n",
				shader.name);
			return;
		}
		tmi->rotateSpeed = atof(token);
		tmi->type = TMOD_ROTATE;
	}else if(!Q_stricmp(token, "entityTranslate")){
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}else{
		ri.Printf(PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name);
	}
}

static qbool
ParseStage(shaderStage_t *stage, char **text)
{
	char	*token;
	int	depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, 
		blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qbool	depthMaskExplicit = qfalse;

	stage->active = qtrue;

beginparse:
	token = Q_readtok2(text, qtrue);
	if(!token[0]){
		ri.Printf(PRINT_WARNING, "WARNING: no matching '}' found\n");
		return qfalse;
	}

	if(token[0] == '}')
		/* end */
		goto done;

	else if(!Q_stricmp(token, "map")){
		/* map <name> */
		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parameter for 'map'"
				" keyword in shader '%s'\n", shader.name);
			return qfalse;
		}

		if(!Q_stricmp(token, "$whiteimage")){
			stage->bundle[0].image[0] = tr.whiteImage;
			goto beginparse;
		}else if(!Q_stricmp(token, "$lightmap")){
			stage->bundle[0].isLightmap = qtrue;
			if(shader.lightmapIndex < 0){
				stage->bundle[0].image[0] = tr.whiteImage;
			}else{
				stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
			}
			goto beginparse;
		}else if(!Q_stricmp(token, "$deluxemap")){
			if(!tr.worldDeluxeMapping){
				ri.Printf(PRINT_WARNING, "WARNING: shader '%s' wants a deluxe map in a map compiled without them\n",
					shader.name);
				return qfalse;
			}

			stage->bundle[0].isLightmap = qtrue;
			if(shader.lightmapIndex < 0){
				stage->bundle[0].image[0] = tr.whiteImage;
			}else{
				stage->bundle[0].image[0] = tr.deluxemaps[shader.lightmapIndex];
			}
			goto beginparse;
		}else{
			imgFlags_t flags = IMGFLAG_NONE;

			if(!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if(!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			if(stage->type == ST_NORMALMAP || stage->type == ST_NORMALPARALLAXMAP)
				flags |= IMGFLAG_SWIZZLE | IMGFLAG_NORMALIZED | IMGFLAG_NOLIGHTSCALE;

			stage->bundle[0].image[0] = R_FindImageFile2(token, flags);

			if(!stage->bundle[0].image[0]){
				ri.Printf(PRINT_WARNING,"WARNING: R_FindImageFile could not find '%s' in shader '%s'\n",
					token, shader.name);
				return qfalse;
			}

			if(stage->type == ST_DIFFUSEMAP){
				char filename[MAX_QPATH];
				/* if ( r_autoFindNormalMap->integer ) */
				if(r_normalMapping->integer){
					Q_stripext(token, filename, sizeof(filename));
					Q_strcat(filename, sizeof(filename), "_normal");

					stage->bundle[TB_NORMALMAP].image[0] = R_FindImageFile2(filename,
						flags | IMGFLAG_SWIZZLE 
						| IMGFLAG_NORMALIZED |
						IMGFLAG_NOLIGHTSCALE);
				}

				/* if ( r_autoFindSpecularMap->integer ) */
				if(r_specularMapping->integer){
					Q_stripext(token, filename, sizeof(filename));
					Q_strcat(filename, sizeof(filename), "_specular");

					stage->bundle[TB_SPECULARMAP].image[0] = R_FindImageFile2(filename, flags);
					stage->specularReflectance = 0.04f;
				}
			}
		}
	}else if(!Q_stricmp(token, "clampmap")){
		 /* clampmap <name> */
		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parameter for"
				" 'clampmap' keyword in shader '%s'\n",
				shader.name);
			return qfalse;
		}

		stage->bundle[0].image[0] =
			R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip,
				GL_CLAMP_TO_EDGE);
		if(!stage->bundle[0].image[0]){
			ri.Printf(PRINT_WARNING,
				"WARNING: R_FindImageFile could not find '%s' in shader '%s'\n",
				token,
				shader.name);
			return qfalse;
		}
	}
	/*
	 * animMap <frequency> <image1> .... <imageN>
	 *  */
	else if(!Q_stricmp(token, "animMap")){
		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(
				PRINT_WARNING,
				"WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n",
				shader.name);
			return qfalse;
		}
		stage->bundle[0].imageAnimationSpeed = atof(token);

		/* parse up to MAX_IMAGE_ANIMATIONS animations */
		for(;;){
			int num;

			token = Q_readtok2(text, qfalse);
			if(!token[0]){
				break;
			}
			num = stage->bundle[0].numImageAnimations;
			if(num < MAX_IMAGE_ANIMATIONS){
				stage->bundle[0].image[num] =
					R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip,
						GL_REPEAT);
				if(!stage->bundle[0].image[num]){
					ri.Printf(
						PRINT_WARNING,
						"WARNING: R_FindImageFile could not find '%s' in shader '%s'\n",
						token, shader.name);
					return qfalse;
				}
				stage->bundle[0].numImageAnimations++;
			}
		}
	}else if(!Q_stricmp(token, "videoMap")){
		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(
				PRINT_WARNING,
				"WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n",
				shader.name);
			return qfalse;
		}
		stage->bundle[0].videoMapHandle =
			ri.CIN_PlayCinematic(token, 0, 0, 256, 256,
				(CIN_loop | CIN_silent | CIN_shader));
		if(stage->bundle[0].videoMapHandle != -1){
			stage->bundle[0].isVideoMap	= qtrue;
			stage->bundle[0].image[0]	=
				tr.scratchImage[stage->bundle[0].videoMapHandle];
		}
	}
	/*
	 * alphafunc <func>
	 *  */
	else if(!Q_stricmp(token, "alphaFunc")){
		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(
				PRINT_WARNING,
				"WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n",
				shader.name);
			return qfalse;
		}

		atestBits = NameToAFunc(token);
	}
	/*
	 * depthFunc <func>
	 *  */
	else if(!Q_stricmp(token, "depthfunc")){
		token = Q_readtok2(text, qfalse);

		if(!token[0]){
			ri.Printf(
				PRINT_WARNING,
				"WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n",
				shader.name);
			return qfalse;
		}

		if(!Q_stricmp(token, "lequal")){
			depthFuncBits = 0;
		}else if(!Q_stricmp(token, "equal")){
			depthFuncBits = GLS_DEPTHFUNC_EQUAL;
		}else{
			ri.Printf(PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n",
				token,
				shader.name);
			goto beginparse;
		}
	}
	/*
	 * detail
	 *  */
	else if(!Q_stricmp(token, "detail")){
		stage->isDetail = qtrue;
	}
	/*
	 * blendfunc <srcFactor> <dstFactor>
	 * or blendfunc <add|filter|blend>
	 *  */
	else if(!Q_stricmp(token, "blendfunc")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parm for blendFunc in shader '%s'\n",
				shader.name);
			goto beginparse;
		}
		/* check for "simple" blends first */
		if(!Q_stricmp(token, "add")){
			blendSrcBits	= GLS_SRCBLEND_ONE;
			blendDstBits	= GLS_DSTBLEND_ONE;
		}else if(!Q_stricmp(token, "filter")){
			blendSrcBits	= GLS_SRCBLEND_DST_COLOR;
			blendDstBits	= GLS_DSTBLEND_ZERO;
		}else if(!Q_stricmp(token, "blend")){
			blendSrcBits	= GLS_SRCBLEND_SRC_ALPHA;
			blendDstBits	= GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		}else{
			/* complex double blends */
			blendSrcBits = NameToSrcBlendMode(token);

			token = Q_readtok2(text, qfalse);
			if(token[0] == 0){
				ri.Printf(PRINT_WARNING,
					"WARNING: missing parm for blendFunc in shader '%s'\n",
					shader.name);
				goto beginparse;
			}
			blendDstBits = NameToDstBlendMode(token);
		}

		/* clear depth mask for blended surfaces */
		if(!depthMaskExplicit){
			depthMaskBits = 0;
		}
	}
	/*
	 * stage <type>
	 *  */
	else if(!Q_stricmp(token, "stage")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parameters for stage in shader '%s'\n",
				shader.name);
			goto beginparse;
		}

		if(!Q_stricmp(token, "diffuseMap")){
			stage->type = ST_DIFFUSEMAP;
		}else if(!Q_stricmp(token, "normalMap") || !Q_stricmp(token, "bumpMap")){
			stage->type = ST_NORMALMAP;
		}else if(!Q_stricmp(token,
				 "normalParallaxMap") || !Q_stricmp(token, "bumpParallaxMap")){
			stage->type = ST_NORMALPARALLAXMAP;
		}else if(!Q_stricmp(token, "specularMap")){
			stage->type = ST_SPECULARMAP;
			stage->specularReflectance = 0.04f;
		}else{
			ri.Printf(PRINT_WARNING,
				"WARNING: unknown stage parameter '%s' in shader '%s'\n", token,
				shader.name);
			goto beginparse;
		}
	}
	/*
	 * specularReflectance <value>
	 *  */
	else if(!Q_stricmp(token, "specularreflectance")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(
				PRINT_WARNING,
				"WARNING: missing parameter for specular reflectance in shader '%s'\n",
				shader.name);
			goto beginparse;
		}
		stage->specularReflectance = atof(token);
	}
	/*
	 * diffuseRoughness <value>
	 *  */
	else if(!Q_stricmp(token, "diffuseroughness")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parameter for diffuse roughness in shader '%s'\n",
				shader.name);
			goto beginparse;
		}
		stage->diffuseRoughness = atof(token);
	}
	/*
	 * rgbGen
	 *  */
	else if(!Q_stricmp(token, "rgbGen")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parameters for rgbGen in shader '%s'\n",
				shader.name);
			goto beginparse;
		}

		if(!Q_stricmp(token, "wave")){
			ParseWaveForm(text, &stage->rgbWave);
			stage->rgbGen = CGEN_WAVEFORM;
		}else if(!Q_stricmp(token, "const")){
			Vec3 color;

			ParseVector(text, 3, color);
			stage->constantColor[0] = 255 * color[0];
			stage->constantColor[1] = 255 * color[1];
			stage->constantColor[2] = 255 * color[2];

			stage->rgbGen = CGEN_CONST;
		}else if(!Q_stricmp(token, "identity")){
			stage->rgbGen = CGEN_IDENTITY;
		}else if(!Q_stricmp(token, "identityLighting")){
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		}else if(!Q_stricmp(token, "entity")){
			stage->rgbGen = CGEN_ENTITY;
		}else if(!Q_stricmp(token, "oneMinusEntity")){
			stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
		}else if(!Q_stricmp(token, "vertex")){
			stage->rgbGen = CGEN_VERTEX;
			if(stage->alphaGen == 0){
				stage->alphaGen = AGEN_VERTEX;
			}
		}else if(!Q_stricmp(token, "exactVertex")){
			stage->rgbGen = CGEN_EXACT_VERTEX;
		}else if(!Q_stricmp(token, "vertexLit")){
			stage->rgbGen = CGEN_VERTEX_LIT;
			if(stage->alphaGen == 0){
				stage->alphaGen = AGEN_VERTEX;
			}
		}else if(!Q_stricmp(token, "exactVertexLit")){
			stage->rgbGen = CGEN_EXACT_VERTEX_LIT;
		}else if(!Q_stricmp(token, "lightingDiffuse")){
			stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
		}else if(!Q_stricmp(token, "oneMinusVertex")){
			stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
		}else{
			ri.Printf(PRINT_WARNING,
				"WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token,
				shader.name);
			goto beginparse;
		}
	}
	/*
	 * alphaGen
	 *  */
	else if(!Q_stricmp(token, "alphaGen")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parameters for alphaGen in shader '%s'\n",
				shader.name);
			goto beginparse;
		}

		if(!Q_stricmp(token, "wave")){
			ParseWaveForm(text, &stage->alphaWave);
			stage->alphaGen = AGEN_WAVEFORM;
		}else if(!Q_stricmp(token, "const")){
			token = Q_readtok2(text, qfalse);
			stage->constantColor[3] = 255 * atof(token);
			stage->alphaGen = AGEN_CONST;
		}else if(!Q_stricmp(token, "identity")){
			stage->alphaGen = AGEN_IDENTITY;
		}else if(!Q_stricmp(token, "entity")){
			stage->alphaGen = AGEN_ENTITY;
		}else if(!Q_stricmp(token, "oneMinusEntity")){
			stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
		}else if(!Q_stricmp(token, "vertex")){
			stage->alphaGen = AGEN_VERTEX;
		}else if(!Q_stricmp(token, "lightingSpecular")){
			stage->alphaGen = AGEN_LIGHTING_SPECULAR;
		}else if(!Q_stricmp(token, "oneMinusVertex")){
			stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
		}else if(!Q_stricmp(token, "portal")){
			stage->alphaGen = AGEN_PORTAL;
			token = Q_readtok2(text, qfalse);
			if(token[0] == 0){
				shader.portalRange = 256;
				ri.Printf(
					PRINT_WARNING,
					"WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n",
					shader.name);
			}else{
				shader.portalRange = atof(token);
			}
		}else if(!Q_stricmp(token, "fresnel")){
			stage->alphaGen = AGEN_FRESNEL;
		}else{
			ri.Printf(PRINT_WARNING,
				"WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token,
				shader.name);
			goto beginparse;
		}
	}
	/*
	 * tcGen <function>
	 *  */
	else if(!Q_stricmp(token, "texgen") || !Q_stricmp(token, "tcGen")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n",
				shader.name);
			goto beginparse;
		}

		if(!Q_stricmp(token, "environment")){
			stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
		}else if(!Q_stricmp(token, "lightmap")){
			stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
		}else if(!Q_stricmp(token, "texture") || !Q_stricmp(token, "base")){
			stage->bundle[0].tcGen = TCGEN_TEXTURE;
		}else if(!Q_stricmp(token, "vector")){
			ParseVector(text, 3, stage->bundle[0].tcGenVectors[0]);
			ParseVector(text, 3, stage->bundle[0].tcGenVectors[1]);

			stage->bundle[0].tcGen = TCGEN_VECTOR;
		}else{
			ri.Printf(PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n",
				shader.name);
		}
	}
	/*
	 * tcMod <type> <...>
	 *  */
	else if(!Q_stricmp(token, "tcMod")){
		char buffer[1024] = "";

		for(;;){
			token = Q_readtok2(text, qfalse);
			if(token[0] == 0)
				break;
			strcat(buffer, token);
			strcat(buffer, " ");
		}

		ParseTexMod(buffer, stage);

		goto beginparse;
	}
	/* depthmask */
	else if(!Q_stricmp(token, "depthwrite")){
		depthMaskBits = GLS_DEPTHMASK_TRUE;
		depthMaskExplicit = qtrue;

		goto beginparse;
	}else{
		ri.Printf(PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token,
			shader.name);
		return qfalse;
	}
	goto beginparse;

done:

	/*
	 * if cgen isn't explicitly specified, use either identity 
	 * or identitylighting
	 */
	if(stage->rgbGen == CGEN_BAD){
		if(blendSrcBits == 0 || blendSrcBits == GLS_SRCBLEND_ONE ||
		   blendSrcBits == GLS_SRCBLEND_SRC_ALPHA){
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		}else{
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	/* implicitly assume that a GL_ONE GL_ZERO blend mask disables blending */
	if((blendSrcBits == GLS_SRCBLEND_ONE) &&
	   (blendDstBits == GLS_DSTBLEND_ZERO)){
		blendDstBits	= blendSrcBits = 0;
		depthMaskBits	= GLS_DEPTHMASK_TRUE;
	}

	/* decide which agens we can skip */
	if(stage->alphaGen == AGEN_IDENTITY)
		if(stage->rgbGen == CGEN_IDENTITY || stage->rgbGen == CGEN_LIGHTING_DIFFUSE)
			stage->alphaGen = AGEN_SKIP;

	/* compute state bits */
	stage->stateBits = depthMaskBits |
			   blendSrcBits | blendDstBits |
			   atestBits |
			   depthFuncBits;

	return qtrue;
}

/*
 * ParseDeform
 *
 * deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
 * deformVertexes normal <frequency> <amplitude>
 * deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
 * deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
 * deformVertexes projectionShadow
 * deformVertexes autoSprite
 * deformVertexes autoSprite2
 * deformVertexes text[0-7]
 */
static void
ParseDeform(char **text)
{
	char *token;
	deformStage_t *ds;

	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name);
		return;
	}

	if(shader.numDeforms == MAX_SHADER_DEFORMS){
		ri.Printf(PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name);
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if(!Q_stricmp(token, "projectionShadow")){
		ds->deformation = DEFORM_PROJECTION_SHADOW;
	}else if(!Q_stricmp(token, "autosprite")){
		ds->deformation = DEFORM_AUTOSPRITE;
	}else if(!Q_stricmp(token, "autosprite2")){
		ds->deformation = DEFORM_AUTOSPRITE2;
	}else if(!Q_stricmpn(token, "text", 4)){
		int n;

		n = token[4] - '0';
		if(n < 0 || n > 7){
			n = 0;
		}
		ds->deformation = DEFORM_TEXT0 + n;
	}else if(!Q_stricmp(token, "bulge")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing deformVertexes bulge parm in shader '%s'\n",
				shader.name);
			return;
		}
		ds->bulgeWidth = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing deformVertexes bulge parm in shader '%s'\n",
				shader.name);
			return;
		}
		ds->bulgeHeight = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing deformVertexes bulge parm in shader '%s'\n",
				shader.name);
			return;
		}
		ds->bulgeSpeed = atof(token);
		ds->deformation = DEFORM_BULGE;
	}else if(!Q_stricmp(token, "wave")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n",
				shader.name);
			return;
		}

		if(atof(token) != 0){
			ds->deformationSpread = 1.0f / atof(token);
		}else{
			ds->deformationSpread = 100.0f;
			ri.Printf(
				PRINT_WARNING,
				"WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n",
				shader.name);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_WAVE;
	}else if(!Q_stricmp(token, "normal")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n",
				shader.name);
			return;
		}
		ds->deformationWave.amplitude = atof(token);

		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n",
				shader.name);
			return;
		}
		ds->deformationWave.frequency = atof(token);

		ds->deformation = DEFORM_NORMALS;
	}else if(!Q_stricmp(token, "move")){
		int i;

		for(i = 0; i < 3; i++){
			token = Q_readtok2(text, qfalse);
			if(token[0] == 0){
				ri.Printf(PRINT_WARNING,
					"WARNING: missing deformVertexes parm in shader '%s'\n",
					shader.name);
				return;
			}
			ds->moveVector[i] = atof(token);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_MOVE;
	}else
		ri.Printf(PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n",
			token, shader.name);
}

/*
 * ParseSkyParms
 *
 * skyParms <outerbox> <cloudheight> <innerbox>
 */
static void
ParseSkyParms(char **text)
{
	char	*token;
	static char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	char	pathname[MAX_QPATH];
	int	i;

	/* outerbox */
	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n",
			shader.name);
		return;
	}
	if(strcmp(token, "-")){
		for(i=0; i<6; i++){
			Q_sprintf(pathname, sizeof(pathname), "%s_%s.tga"
				, token, suf[i]);
			shader.sky.outerbox[i] = R_FindImageFile(( char* )pathname, qtrue, qtrue,
				GL_CLAMP_TO_EDGE);

			if(!shader.sky.outerbox[i]){
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	/* cloudheight */
	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n",
			shader.name);
		return;
	}
	shader.sky.cloudHeight = atof(token);
	if(!shader.sky.cloudHeight){
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords(shader.sky.cloudHeight);


	/* innerbox */
	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n",
			shader.name);
		return;
	}
	if(strcmp(token, "-")){
		for(i=0; i<6; i++){
			Q_sprintf(pathname, sizeof(pathname), "%s_%s.tga"
				, token, suf[i]);
			shader.sky.innerbox[i] = R_FindImageFile(( char* )pathname, qtrue, qtrue, GL_REPEAT);
			if(!shader.sky.innerbox[i]){
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}

void
ParseSort(char **text)
{
	char *token;

	token = Q_readtok2(text, qfalse);
	if(token[0] == 0){
		ri.Printf(PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name);
		return;
	}

	if(!Q_stricmp(token, "portal"))
		shader.sort = SS_PORTAL;
	else if(!Q_stricmp(token, "sky"))
		shader.sort = SS_ENVIRONMENT;
	else if(!Q_stricmp(token, "opaque"))
		shader.sort = SS_OPAQUE;
	else if(!Q_stricmp(token, "decal"))
		shader.sort = SS_DECAL;
	else if(!Q_stricmp(token, "seeThrough"))
		shader.sort = SS_SEE_THROUGH;
	else if(!Q_stricmp(token, "banner"))
		shader.sort = SS_BANNER;
	else if(!Q_stricmp(token, "additive"))
		shader.sort = SS_BLEND1;
	else if(!Q_stricmp(token, "nearest"))
		shader.sort = SS_NEAREST;
	else if(!Q_stricmp(token, "underwater"))
		shader.sort = SS_UNDERWATER;
	else
		shader.sort = atof(token);
}

/* this table is also present in q3map */

typedef struct {
	char	*name;
	int	clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t infoParms[] = {
	/* server relevant contents */
	{"water",	1,	0,		CONTENTS_WATER },
	{"slime",	1,	0,		CONTENTS_SLIME },	/* mildly damaging */
	{"lava",	1,	0,		CONTENTS_LAVA },	/* very damaging */
	{"playerclip",	1,      0,		CONTENTS_PLAYERCLIP },
	{"monsterclip",	1,	0,		CONTENTS_MONSTERCLIP },
	{"nodrop",	1,      0,		CONTENTS_NODROP },	/* don't drop items or leave bodies (death fog, lava, etc) */
	{"nonsolid",	1,	SURF_NONSOLID,		0},			/* clears the solid flag */

	/* utility relevant attributes */
	{"origin",	1,	0,		CONTENTS_ORIGIN },		/* center of rotating brushes */
	{"trans",	0,	0,		CONTENTS_TRANSLUCENT },		/* don't eat contained surfaces */
	{"detail",	0,	0,		CONTENTS_DETAIL },		/* don't include in structural bsp */
	{"structural",  0,	0,		CONTENTS_STRUCTURAL },		/* force into structural bsp even if trnas */
	{"areaportal",	1,	0,		CONTENTS_AREAPORTAL },		/* divides areas */
	{"clusterportal",1,	0,  		CONTENTS_CLUSTERPORTAL },	/* for bots */
	{"donotenter",	1,	0,		CONTENTS_DONOTENTER },		/* for bots */

	{"fog",		1,	0,		CONTENTS_FOG},	/* carves surfaces entering */
	{"sky",		0,	SURF_SKY,		0 },	/* emit light from an environment map */
	{"lightfilter", 0,	SURF_LIGHTFILTER,	0 },	/* filter light going through it */
	{"alphashadow", 0,	SURF_ALPHASHADOW,	0 },	/* test light on a per-pixel basis */
	{"hint",	0,	SURF_HINT,		0 },	/* use as a primary splitter */

	/* server attributes */
	{"slick",	0,	SURF_SLICK,		0 },
	{"noimpact",	0,	SURF_NOIMPACT,		0 },	/* don't make impact explosions or marks */
	{"nomarks",	0,	SURF_NOMARKS,	  	0 },	/* don't make impact marks, but still explode */
	{"ladder",	0,	SURF_LADDER,		0 },
	{"nodamage",	0,	SURF_NODAMAGE,		0 },
	{"metalsteps",	0,	SURF_METALSTEPS,	0 },
	{"flesh",	0,	SURF_FLESH,		0 },
	{"nosteps",	0,	SURF_NOSTEPS,		0 },

	/* drawsurf attributes */
	{"nodraw",	0,	SURF_NODRAW,		0 },	/* don't generate a drawsurface (or a lightmap) */
	{"pointlight",  0,	SURF_POINTLIGHT,	0 },	/* sample lighting at vertexes */
	{"nolightmap",  0,	SURF_NOLIGHTMAP,	0 },	/* don't generate a lightmap */
	{"nodlight",	0,	SURF_NODLIGHT,		0 },	/* don't ever add dynamic lights */
	{"dust",	0,	SURF_DUST,		0 }	/* leave a dust trail when walking on this surface */
};

/*
 * ParseSurfaceParm
 *
 * surfaceparm <name>
 */
static void
ParseSurfaceParm(char **text)
{
	char	*token;
	int	numInfoParms = ARRAY_LEN(infoParms);
	int	i;

	token = Q_readtok2(text, qfalse);
	for(i = 0; i < numInfoParms; i++){
		if(!Q_stricmp(token, infoParms[i].name)){
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
			if(0){
				if(infoParms[i].clearSolid){
					/*si->contents &= ~CONTENTS_SOLID;*/
				}
			}
			break;
		}
	}
}

/*
 * ParseShader: The current text pointer is at the explicit text definition of the
 * shader.  Parse it into the global shader variable.  Later functions
 * will optimize it.
 */
qbool
ParseShader(char **text)
{
	char	*token;
	int	s;

	s = 0;

	token = Q_readtok2(text, qtrue);
	if(token[0] != '{'){
		ri.Printf(PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token,
			shader.name);
		return qfalse;
	}

beginparse:
	token = Q_readtok2(text, qtrue);
	if(!token[0]){
		ri.Printf(PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name);
		return qfalse;
	}

	/* end of shader definition */
	if(token[0] == '}'){
		goto done;
	}
	/* stage definition */
	else if(token[0] == '{'){
		if(s >= MAX_SHADER_STAGES){
			ri.Printf(PRINT_WARNING, "WARNING: too many stages in shader %s\n",
				shader.name);
			return qfalse;
		}

		if(!ParseStage(&stages[s], text)){
			return qfalse;
		}
		stages[s].active = qtrue;
		s++;

		goto beginparse;
	}
	/* skip stuff that only the QuakeEdRadient needs */
	else if(!Q_stricmpn(token, "qer", 3)){
		Q_skipline(text);
		goto beginparse;
	}
	/* sun parms */
	else if(!Q_stricmp(token, "q3map_sun")){
		float a, b;

		token = Q_readtok2(text, qfalse);
		tr.sunLight[0] = atof(token);
		token = Q_readtok2(text, qfalse);
		tr.sunLight[1] = atof(token);
		token = Q_readtok2(text, qfalse);
		tr.sunLight[2] = atof(token);

		vec3normalize(tr.sunLight);

		token = Q_readtok2(text, qfalse);
		a = atof(token);
		vec3scale(tr.sunLight, a, tr.sunLight);

		token	= Q_readtok2(text, qfalse);
		a	= atof(token);
		a	= a / 180 * M_PI;

		token	= Q_readtok2(text, qfalse);
		b	= atof(token);
		b	= b / 180 * M_PI;

		tr.sunDirection[0]	= cos(a) * cos(b);
		tr.sunDirection[1]	= sin(a) * cos(b);
		tr.sunDirection[2]	= sin(b);
	}else if(!Q_stricmp(token, "deformVertexes")){
		ParseDeform(text);
		goto beginparse;
	}else if(!Q_stricmp(token, "tesssize")){
		Q_skipline(text);
		goto beginparse;
	}else if(!Q_stricmp(token, "clampTime")){
		token = Q_readtok2(text, qfalse);
		if(token[0]){
			shader.clampTime = atof(token);
		}
	}
	/* skip stuff that only the q3map needs */
	else if(!Q_stricmpn(token, "q3map", 5)){
		Q_skipline(text);
		goto beginparse;
	}
	/* skip stuff that only q3map or the server needs */
	else if(!Q_stricmp(token, "surfaceParm")){
		ParseSurfaceParm(text);
		goto beginparse;
	}
	/* no mip maps */
	else if(!Q_stricmp(token, "nomipmaps")){
		shader.noMipMaps	= qtrue;
		shader.noPicMip		= qtrue;
		goto beginparse;
	}
	/* no picmip adjustment */
	else if(!Q_stricmp(token, "nopicmip")){
		shader.noPicMip = qtrue;
		goto beginparse;
	}
	/* polygonOffset */
	else if(!Q_stricmp(token, "polygonOffset")){
		shader.polygonOffset = qtrue;
		goto beginparse;
	}
	/* entityMergable, allowing sprite surfaces from multiple entities
	 * to be merged into one batch.  This is a savings for smoke
	 * puffs and blood, but can't be used for anything where the
	 * shader calcs (not the surface function) reference the entity color or scroll */
	else if(!Q_stricmp(token, "entityMergable")){
		shader.entityMergable = qtrue;
		goto beginparse;
	}
	/* fogParms */
	else if(!Q_stricmp(token, "fogParms")){
		if(!ParseVector(text, 3, shader.fogParms.color)){
			return qfalse;
		}

		token = Q_readtok2(text, qfalse);
		if(!token[0]){
			ri.Printf(PRINT_WARNING,
				"WARNING: missing parm for 'fogParms' keyword in shader '%s'\n",
				shader.name);
			goto beginparse;
		}
		shader.fogParms.depthForOpaque = atof(token);

		/* skip any old gradient directions */
		Q_skipline(text);
		goto beginparse;
	}
	/* portal */
	else if(!Q_stricmp(token, "portal")){
		shader.sort = SS_PORTAL;
		shader.isPortal = qtrue;
		goto beginparse;
	}
	/* skyparms <cloudheight> <outerbox> <innerbox> */
	else if(!Q_stricmp(token, "skyparms")){
		ParseSkyParms(text);
		goto beginparse;
	}
	/* light <value> determines flaring in q3map, not needed here */
	else if(!Q_stricmp(token, "light")){
		token = Q_readtok2(text, qfalse);
		goto beginparse;
	}
	/* cull <face> */
	else if(!Q_stricmp(token, "cull")){
		token = Q_readtok2(text, qfalse);
		if(token[0] == 0){
			ri.Printf(PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n",
				shader.name);
			goto beginparse;
		}

		if(!Q_stricmp(token,
			   "none") || !Q_stricmp(token, "twosided") || !Q_stricmp(token, "disable")){
			shader.cullType = CT_TWO_SIDED;
		}else if(!Q_stricmp(token,
				 "back") ||
			 !Q_stricmp(token, "backside") || !Q_stricmp(token, "backsided")){
			shader.cullType = CT_BACK_SIDED;
		}else{
			ri.Printf(PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n",
				token,
				shader.name);
		}
		goto beginparse;
	}
	/* sort */
	else if(!Q_stricmp(token, "sort")){
		ParseSort(text);
		goto beginparse;
	}else{
		ri.Printf(PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n",
			token,
			shader.name);
		return qfalse;
	}
	goto beginparse;

done:
	/*
	 * ignore shaders that don't have any stages, unless it is a sky or fog
	 *  */
	if(s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG)){
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;
	return qtrue;
}

