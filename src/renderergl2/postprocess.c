/*
 * Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete
 *
 * This file is part of Reaction source code.
 *
 * Reaction source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Reaction source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Reaction source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "local.h"

void
RB_ToneMap(int autoExposure)
{
	Vec4	white;
	Vec2	texScale;

	texScale[0] =
		texScale[1] = 1.0f;

	vec3set4(white, 1, 1, 1, 1);

	if(glRefConfig.framebufferObject && autoExposure){
		/* determine average log luminance */
		Vec4	srcBox, dstBox, color;
		int	size = 256, currentScratch, nextScratch, tmp;

		vec3set4(srcBox, 0, 0, tr.renderFbo->width, tr.renderFbo->height);
		vec3set4(dstBox, 0, 0, 256, 256);

		FBO_Blit(tr.renderFbo, srcBox, texScale, tr.textureScratchFbo[0], dstBox,
			&tr.calclevels4xShader[0], white,
			0);

		currentScratch = 0;
		nextScratch = 1;

		/* downscale to 1x1 texture */
		while(size > 1){
			vec3set4(srcBox, 0, 0, size, size);
			size >>= 2;
			vec3set4(dstBox, 0, 0, size, size);
			FBO_Blit(tr.textureScratchFbo[currentScratch], srcBox, texScale,
				tr.textureScratchFbo[nextScratch], dstBox, &tr.calclevels4xShader[1], white,
				0);

			tmp = currentScratch;
			currentScratch = nextScratch;
			nextScratch = tmp;
		}

		/* blend with old log luminance for gradual change */
		vec3set4(srcBox, 0, 0, 0, 0);
		vec3set4(dstBox, 0, 0, 1, 1);

		color[0] =
			color[1] =
				color[2] = 1.0f;
		if(glRefConfig.textureFloat)
			color[3] = 0.03f;
		else
			color[3] = 0.1f;

		FBO_Blit(tr.textureScratchFbo[currentScratch], srcBox, texScale, tr.calcLevelsFbo, dstBox,
			&tr.textureColorShader, color,
			GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	if(glRefConfig.framebufferObject){
		/* tonemap */
		Vec4 srcBox, dstBox, color;

		vec3set4(srcBox, 0, 0, tr.renderFbo->width, tr.renderFbo->height);
		vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

		color[0] =
			color[1] =
				color[2] = pow(2, r_cameraExposure->value);	/* exp2(r_cameraExposure->value); */
		color[3] = 1.0f;

		if(autoExposure)
			GL_BindToTMU(tr.calcLevelsImage,  TB_LEVELSMAP);
		else
			GL_BindToTMU(tr.fixedLevelsImage, TB_LEVELSMAP);

		if(r_hdr->integer)
			FBO_Blit(tr.renderFbo, srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.tonemapShader, color,
				0);
		else
			FBO_Blit(tr.renderFbo, srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, color,
				0);
	}
}


void
RB_BokehBlur(float blur)
{
	Vec4	white;
	Vec2	texScale;

	texScale[0] =
		texScale[1] = 1.0f;

	vec3set4(white, 1, 1, 1, 1);

	blur *= 10.0f;

	if(blur < 0.004f)
		return;

	if(glRefConfig.framebufferObject){
		/* bokeh blur */
		Vec4 srcBox, dstBox, color;

		if(blur > 0.0f){
			/* create a quarter texture */
			vec3set4(srcBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);
			vec3set4(dstBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);

			FBO_Blit(tr.screenScratchFbo, srcBox, texScale, tr.quarterFbo[0], dstBox,
				&tr.textureColorShader, white,
				0);
		}

#ifndef HQ_BLUR
		if(blur > 1.0f){
			/* create a 1/16th texture */
			vec3set4(srcBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			vec3set4(dstBox, 0, 0, tr.textureScratchFbo[0]->width,
				tr.textureScratchFbo[0]->height);

			FBO_Blit(tr.quarterFbo[0], srcBox, texScale, tr.textureScratchFbo[0], dstBox,
				&tr.textureColorShader, white,
				0);
		}
#endif

		if(blur > 0.0f && blur <= 1.0f){
			/* Crossfade original with quarter texture */
			vec3set4(srcBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			vec3set4(color, 1, 1, 1, blur);

			FBO_Blit(tr.quarterFbo[0], srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, color,
				GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}
#ifndef HQ_BLUR
		/* ok blur, but can see some pixelization */
		else if(blur > 1.0f && blur <= 2.0f){
			/* crossfade quarter texture with 1/16th texture */
			vec3set4(srcBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			FBO_Blit(tr.quarterFbo[0], srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, white,
				0);

			vec3set4(srcBox, 0, 0, tr.textureScratchFbo[0]->width,
				tr.textureScratchFbo[0]->height);
			vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			vec3set4(color, 1, 1, 1, blur - 1.0f);

			FBO_Blit(tr.textureScratchFbo[0], srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, color,
				GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}else if(blur > 2.0f){
			/* blur 1/16th texture then replace */
			int i;

			vec3set4(srcBox, 0, 0, tr.textureScratchFbo[0]->width,
				tr.textureScratchFbo[0]->height);
			vec3set4(dstBox, 0, 0, tr.textureScratchFbo[1]->width,
				tr.textureScratchFbo[1]->height);

			for(i = 0; i < 2; i++){
				Vec2	blurTexScale;
				float	subblur;

				subblur = ((blur - 2.0f) / 2.0f) / 3.0f * (float)(i + 1);

				blurTexScale[0] =
					blurTexScale[1] = subblur;

				color[0] =
					color[1] =
						color[2] = 0.5f;
				color[3] = 1.0f;

				if(i != 0)
					FBO_Blit(tr.textureScratchFbo[0], srcBox, blurTexScale,
						tr.textureScratchFbo[1], dstBox, &tr.bokehShader, color,
						GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				else
					FBO_Blit(tr.textureScratchFbo[0], srcBox, blurTexScale,
						tr.textureScratchFbo[1], dstBox, &tr.bokehShader, color,
						0);
			}

			vec3set4(srcBox, 0, 0, tr.textureScratchFbo[1]->width,
				tr.textureScratchFbo[1]->height);
			vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			FBO_Blit(tr.textureScratchFbo[1], srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, white,
				0);
		}
#else	/* higher quality blur, but slower */
		else if(blur > 1.0f){
			/* blur quarter texture then replace */
			int i;

			vec3set4(srcBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			vec3set4(dstBox, 0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);

			src = tr.quarterFbo[0];
			dst = tr.quarterFbo[1];

			vec3set4(color, 0.5f, 0.5f, 0.5f, 1);

			for(i = 0; i < 2; i++){
				Vec2	blurTexScale;
				float	subblur;

				subblur = (blur - 1.0f) / 2.0f * (float)(i + 1);

				blurTexScale[0] =
					blurTexScale[1] = subblur;

				color[0] =
					color[1] =
						color[2] = 1.0f;
				if(i != 0)
					color[3] = 1.0f;
				else
					color[3] = 0.5f;

				FBO_Blit(tr.quarterFbo[0], srcBox, blurTexScale, tr.quarterFbo[1], dstBox,
					&tr.bokehShader, color,
					GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
			}

			vec3set4(srcBox, 0, 0, 512, 512);
			vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);

			FBO_Blit(tr.quarterFbo[1], srcBox, texScale, tr.screenScratchFbo, dstBox,
				&tr.textureColorShader, white,
				0);
		}
#endif
	}
}


#ifdef REACTION
static void
RB_RadialBlur(FBO_t *srcFbo, FBO_t *dstFbo, int passes, float stretch, float x, float y, float w, float h,
	      float xcenter, float ycenter,
	      float alpha)
{
	const float inc = 1.f / passes;
	const float mul = powf(stretch, inc);
	float scale;

	{
		Vec4	srcBox, dstBox, color;
		Vec2	texScale;

		texScale[0] =
			texScale[1] = 1.0f;

		alpha *= inc;
		vec3set4(color, alpha, alpha, alpha, 1.0f);

		vec3set4(srcBox, 0, 0, srcFbo->width, srcFbo->height);
		vec3set4(dstBox, x, y, w, h);
		FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color, 0);

		--passes;
		scale = mul;
		while(passes > 0){
			float iscale	= 1.f / scale;
			float s0	= xcenter * (1.f - iscale);
			float t0	= (1.0f - ycenter) * (1.f - iscale);
			float s1	= iscale + s0;
			float t1	= iscale + t0;

			srcBox[0] = s0 * srcFbo->width;
			srcBox[1] = t0 * srcFbo->height;
			srcBox[2] = (s1 - s0) * srcFbo->width;
			srcBox[3] = (t1 - t0) * srcFbo->height;

			FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color,
				GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

			scale *= mul;
			--passes;
		}
	}
}


static qbool
RB_UpdateSunFlareVis(void)
{
	GLuint sampleCount = 0;
	if(!glRefConfig.occlusionQuery)
		return qtrue;

	tr.sunFlareQueryIndex ^= 1;
	if(!tr.sunFlareQueryActive[tr.sunFlareQueryIndex])
		return qtrue;

	/* debug code */
	if(0){
		int iter;
		for(iter=0;; ++iter){
			GLint available = 0;
			qglGetQueryObjectivARB(tr.sunFlareQuery[tr.sunFlareQueryIndex],
				GL_QUERY_RESULT_AVAILABLE_ARB,
				&available);
			if(available)
				break;
		}

		ri.Printf(PRINT_DEVELOPER, "Waited %d iterations\n", iter);
	}

	qglGetQueryObjectuivARB(tr.sunFlareQuery[tr.sunFlareQueryIndex], GL_QUERY_RESULT_ARB, &sampleCount);
	return sampleCount > 0;
}

void
RB_GodRays(void)
{
	Vec3	dir;
	float	dot;
	const float cutoff = 0.25f;
	qbool		colorize = qtrue;

	float		w, h, w2, h2;
	Mat44	mvp;
	Vec4		pos, hpos;

	if(!backEnd.hasSunFlare)
		return;

	vec3sub(backEnd.sunFlarePos, backEnd.viewParms.or.origin, dir);
	vec3normalize(dir);

	dot = vec3dot(dir, backEnd.viewParms.or.axis[0]);
	if(dot < cutoff)
		return;

	if(!RB_UpdateSunFlareVis())
		return;

	vec3copy(backEnd.sunFlarePos, pos);
	pos[3] = 1.f;

	/* project sun point */
	mat44mul(backEnd.viewParms.projectionMatrix, backEnd.viewParms.world.modelMatrix, mvp);
	mat44transform(mvp, pos, hpos);

	/* transform to UV coords */
	hpos[3] = 0.5f / hpos[3];

	pos[0]	= 0.5f + hpos[0] * hpos[3];
	pos[1]	= 0.5f + hpos[1] * hpos[3];

	/* viewport dimensions */
	w = glConfig.vidWidth;
	h = glConfig.vidHeight;
	w2 = glConfig.vidWidth / 2;
	h2 = glConfig.vidHeight / 2;

	/* initialize quarter buffers */
	{
		float mul = 1.f;

		Vec4	srcBox, dstBox;
		Vec4	color;
		Vec2	texScale;

		texScale[0] =
			texScale[1] = 1.0f;

		vec3set4(color, mul, mul, mul, 1);

		/* first, downsample the framebuffer */
		vec3set4(srcBox, 0, 0, tr.godRaysFbo->width, tr.godRaysFbo->height);
		vec3set4(dstBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
		FBO_Blit(tr.godRaysFbo, srcBox, texScale, tr.quarterFbo[0], dstBox, &tr.textureColorShader,
			color,
			0);

		if(colorize){
			vec3set4(srcBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);
			FBO_Blit(tr.screenScratchFbo, srcBox, texScale, tr.quarterFbo[0], dstBox,
				&tr.textureColorShader, color,
				GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
		}
	}

	/* radial blur passes, ping-ponging between the two quarter-size buffers */
	{
		const float stretch_add = 2.f/3.f;
		float stretch = 1.f + stretch_add;
		int i;
		for(i=0; i<2; ++i){
			RB_RadialBlur(tr.quarterFbo[i&1], tr.quarterFbo[(~i) & 1], 5, stretch, 0.f, 0.f,
				tr.quarterFbo[0]->width, tr.quarterFbo[0]->height, pos[0], pos[1],
				1.125f);
			stretch += stretch_add;
		}
	}

	/* add result back on top of the main buffer */
	{
		float mul = 1.f;

		Vec4	srcBox, dstBox;
		Vec4	color;
		Vec2	texScale;

		texScale[0] =
			texScale[1] = 1.0f;

		vec3set4(color, mul, mul, mul, 1);

		vec3set4(srcBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
		vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);
		FBO_Blit(tr.quarterFbo[0], srcBox, texScale, tr.screenScratchFbo, dstBox,
			&tr.textureColorShader, color,
			GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
}
#endif

static void
RB_BlurAxis(FBO_t *srcFbo, FBO_t *dstFbo, float strength, qbool horizontal)
{
	float dx, dy;
	float xmul, ymul;
	float weights[3] = {
		0.227027027f,
		0.316216216f,
		0.070270270f,
	};
	float offsets[3] = {
		0.f,
		1.3846153846f,
		3.2307692308f,
	};

	xmul = horizontal;
	ymul = 1.f - xmul;

	xmul *= strength;
	ymul *= strength;

	{
		Vec4	srcBox, dstBox, color;
		Vec2	texScale;

		texScale[0] =
			texScale[1] = 1.0f;

		vec3set4(color, weights[0], weights[0], weights[0], 1.0f);
		vec3set4(srcBox, 0, 0, srcFbo->width, srcFbo->height);
		vec3set4(dstBox, 0, 0, dstFbo->width, dstFbo->height);
		FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color, 0);

		vec3set4(color, weights[1], weights[1], weights[1], 1.0f);
		dx = offsets[1] * xmul;
		dy = offsets[1] * ymul;
		vec3set4(srcBox, dx, dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color,
			GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		vec3set4(srcBox, -dx, -dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color,
			GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		vec3set4(color, weights[2], weights[2], weights[2], 1.0f);
		dx = offsets[2] * xmul;
		dy = offsets[2] * ymul;
		vec3set4(srcBox, dx, dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color,
			GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		vec3set4(srcBox, -dx, -dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, texScale, dstFbo, dstBox, &tr.textureColorShader, color,
			GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
}

static void
RB_HBlur(FBO_t *srcFbo, FBO_t *dstFbo, float strength)
{
	RB_BlurAxis(srcFbo, dstFbo, strength, qtrue);
}

static void
RB_VBlur(FBO_t *srcFbo, FBO_t *dstFbo, float strength)
{
	RB_BlurAxis(srcFbo, dstFbo, strength, qfalse);
}

void
RB_GaussianBlur(float blur)
{
	float factor = Q_clamp(0.f, 1.f, blur);

	if(factor <= 0.f)
		return;

	{
		Vec4	srcBox, dstBox;
		Vec4	color;
		Vec2	texScale;

		texScale[0] =
			texScale[1] = 1.0f;

		vec3set4(color, 1, 1, 1, 1);

		/* first, downsample the framebuffer */
		vec3set4(srcBox, 0, 0, tr.screenScratchFbo->width, tr.screenScratchFbo->height);
		vec3set4(dstBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
		FBO_Blit(tr.screenScratchFbo, srcBox, texScale, tr.quarterFbo[0], dstBox,
			&tr.textureColorShader, color,
			0);

		vec3set4(srcBox, 0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
		vec3set4(dstBox, 0, 0, tr.textureScratchFbo[0]->width, tr.textureScratchFbo[0]->height);
		FBO_Blit(tr.quarterFbo[0], srcBox, texScale, tr.textureScratchFbo[0], dstBox,
			&tr.textureColorShader, color,
			0);

		/* set the alpha channel */
		vec3set4(srcBox, 0, 0, tr.whiteImage->width, tr.whiteImage->height);
		vec3set4(dstBox, 0, 0, tr.textureScratchFbo[0]->width, tr.textureScratchFbo[0]->height);
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		FBO_BlitFromTexture(tr.whiteImage, srcBox, texScale, tr.textureScratchFbo[0], dstBox,
			&tr.textureColorShader, color,
			0);
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		/* blur the tiny buffer horizontally and vertically */
		RB_HBlur(tr.textureScratchFbo[0], tr.textureScratchFbo[1], factor);
		RB_VBlur(tr.textureScratchFbo[1], tr.textureScratchFbo[0], factor);

		/* finally, merge back to framebuffer */
		vec3set4(srcBox, 0, 0, tr.textureScratchFbo[0]->width, tr.textureScratchFbo[0]->height);
		vec3set4(dstBox, 0, 0, tr.screenScratchFbo->width,     tr.screenScratchFbo->height);
		color[3] = factor;
		FBO_Blit(tr.textureScratchFbo[0], srcBox, texScale, tr.screenScratchFbo, dstBox,
			&tr.textureColorShader, color,
			GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
}
