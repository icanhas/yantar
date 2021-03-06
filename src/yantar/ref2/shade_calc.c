/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif
#include "local.h"

#define WAVEVALUE(table, base, amplitude, phase, freq) ((base) + \
							table[ ri.ftol((((phase) + \
									 tess.shaderTime * (freq)) * \
									FUNCTABLE_SIZE)) & FUNCTABLE_MASK ] * (	\
								amplitude))

static float *
TableForFunc(genFunc_t func)
{
	switch(func){
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	ri.Error(ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'", func,
		tess.shader->name);
	return NULL;
}

/*
** EvalWaveForm
**
** Evaluates a given Waveform, referencing backEnd.refdef.time directly
*/
static float
EvalWaveForm(const Waveform *wf)
{
	float *table;

	table = TableForFunc(wf->func);

	return WAVEVALUE(table, wf->base, wf->amplitude, wf->phase, wf->frequency);
}

static float
EvalWaveFormClamped(const Waveform *wf)
{
	float glow = EvalWaveForm(wf);

	if(glow < 0){
		return 0;
	}

	if(glow > 1){
		return 1;
	}

	return glow;
}

/*
** RB_CalcStretchTexCoords
*/
void
RB_CalcStretchTexCoords(const Waveform *wf, float *st)
{
	float p;
	texModInfo_t tmi;

	p = 1.0f / EvalWaveForm(wf);

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords(&tmi, st);
}

void
RB_CalcStretchTexMatrix(const Waveform *wf, float *matrix)
{
	float p;
	texModInfo_t tmi;

	p = 1.0f / EvalWaveForm(wf);

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexMatrix(&tmi, matrix);
}

/*
 *
 * DEFORMATIONS
 *
 */

/*
 * RB_CalcDeformVertexes
 *
 */
void
RB_CalcDeformVertexes(deformStage_t *ds)
{
	int i;
	Vec3	offset;
	float	scale;
	float	*xyz = ( float* )tess.xyz;
	float	*normal = ( float* )tess.normal;
	float	*table;

	if(ds->deformationWave.frequency == 0){
		scale = EvalWaveForm(&ds->deformationWave);

		for(i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4){
			scalev3(normal, scale, offset);

			xyz[0]	+= offset[0];
			xyz[1]	+= offset[1];
			xyz[2]	+= offset[2];
		}
	}else{
		table = TableForFunc(ds->deformationWave.func);

		for(i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4){
			float off = (xyz[0] + xyz[1] + xyz[2]) * ds->deformationSpread;

			scale = WAVEVALUE(table, ds->deformationWave.base,
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency);

			scalev3(normal, scale, offset);

			xyz[0]	+= offset[0];
			xyz[1]	+= offset[1];
			xyz[2]	+= offset[2];
		}
	}
}

/*
 * RB_CalcDeformNormals
 *
 * Wiggle the normals for wavy environment mapping
 */
void
RB_CalcDeformNormals(deformStage_t *ds)
{
	int i;
	float	scale;
	float	*xyz = ( float* )tess.xyz;
	float	*normal = ( float* )tess.normal;

	for(i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4){
		scale	= 0.98f;
		scale	= R_NoiseGet4f(xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale	= 0.98f;
		scale	= R_NoiseGet4f(100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale	= 0.98f;
		scale	= R_NoiseGet4f(200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		fastnormv3(normal);
	}
}

/*
 * RB_CalcBulgeVertexes
 *
 */
void
RB_CalcBulgeVertexes(deformStage_t *ds)
{
	int i;
	const float	*st	= ( const float* )tess.texCoords[0];
	float	*xyz	= ( float* )tess.xyz;
	float           *normal = ( float* )tess.normal;
	float	now;

	now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

	for(i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4){
		int off;
		float scale;

		off = (float)(FUNCTABLE_SIZE / (M_PI*2)) * (st[0] * ds->bulgeWidth + now);

		scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;

		xyz[0]	+= normal[0] * scale;
		xyz[1]	+= normal[1] * scale;
		xyz[2]	+= normal[2] * scale;
	}
}


/*
 * RB_CalcMoveVertexes
 *
 * A deformation that can move an entire surface along a wave path
 */
void
RB_CalcMoveVertexes(deformStage_t *ds)
{
	int i;
	float *xyz;
	float *table;
	float scale;
	Vec3 offset;

	table = TableForFunc(ds->deformationWave.func);

	scale = WAVEVALUE(table, ds->deformationWave.base,
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency);

	scalev3(ds->moveVector, scale, offset);

	xyz = ( float* )tess.xyz;
	for(i = 0; i < tess.numVertexes; i++, xyz += 4)
		addv3(xyz, offset, xyz);
}


/*
 * DeformText
 *
 * Change a polygon into a bunch of text polygons
 */
void
DeformText(const char *text)
{
	int i;
	Vec3	origin, width, height;
	int	len;
	int	ch;
	float	color[4];
	float	bottom, top;
	Vec3	mid;

	height[0] = 0;
	height[1] = 0;
	height[2] = -1;
	crossv3(tess.normal[0], height, width);

	/* find the midpoint of the box */
	clearv3(mid);
	bottom = 999999;
	top = -999999;
	for(i = 0; i < 4; i++){
		addv3(tess.xyz[i], mid, mid);
		if(tess.xyz[i][2] < bottom){
			bottom = tess.xyz[i][2];
		}
		if(tess.xyz[i][2] > top){
			top = tess.xyz[i][2];
		}
	}
	scalev3(mid, 0.25f, origin);

	/* determine the individual character size */
	height[0] = 0;
	height[1] = 0;
	height[2] = (top - bottom) * 0.5f;

	scalev3(width, height[2] * -0.75f, width);

	/* determine the starting position */
	len = strlen(text);
	saddv3(origin, (len-1), width, origin);

	/* clear the shader indexes */
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	color[0] = color[1] = color[2] = color[3] = 1.0f;

	/* draw each character */
	for(i = 0; i < len; i++){
		ch = text[i];
		ch &= 255;

		if(ch != ' '){
			int row, col;
			float frow, fcol, size;

			row = ch>>4;
			col = ch&15;

			frow = row*0.0625f;
			fcol = col*0.0625f;
			size = 0.0625f;

			RB_AddQuadStampExt(origin, width, height, color, fcol, frow, fcol + size, frow + size);
		}
		saddv3(origin, -2, width, origin);
	}
}

/*
 * GlobalVectorToLocal
 */
static void
GlobalVectorToLocal(const Vec3 in, Vec3 out)
{
	out[0]	= dotv3(in, backEnd.or.axis[0]);
	out[1]	= dotv3(in, backEnd.or.axis[1]);
	out[2]	= dotv3(in, backEnd.or.axis[2]);
}

/*
 * AutospriteDeform
 *
 * Assuming all the triangles for this shader are independant
 * quads, rebuild them as forward facing sprites
 */
static void
AutospriteDeform(void)
{
	int i;
	int oldVerts;
	float *xyz;
	Vec3	mid, delta;
	float	radius;
	Vec3	left, up;
	Vec3	leftDir, upDir;

	if(tess.numVertexes & 3){
		ri.Printf(PRINT_WARNING, "Autosprite shader %s had odd vertex count\n", tess.shader->name);
	}
	if(tess.numIndexes != (tess.numVertexes >> 2) * 6){
		ri.Printf(PRINT_WARNING, "Autosprite shader %s had odd index count\n", tess.shader->name);
	}

	oldVerts = tess.numVertexes;
	tess.numVertexes	= 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	if(backEnd.currentEntity != &tr.worldEntity){
		GlobalVectorToLocal(backEnd.viewParms.or.axis[1], leftDir);
		GlobalVectorToLocal(backEnd.viewParms.or.axis[2], upDir);
	}else{
		copyv3(backEnd.viewParms.or.axis[1], leftDir);
		copyv3(backEnd.viewParms.or.axis[2], upDir);
	}

	for(i = 0; i < oldVerts; i+=4){
		/* find the midpoint */
		xyz = tess.xyz[i];

		mid[0]	= 0.25f * (xyz[0] + xyz[4] + xyz[8] + xyz[12]);
		mid[1]	= 0.25f * (xyz[1] + xyz[5] + xyz[9] + xyz[13]);
		mid[2]	= 0.25f * (xyz[2] + xyz[6] + xyz[10] + xyz[14]);

		subv3(xyz, mid, delta);
		radius = lenv3(delta) * 0.707f;	/* / sqrt(2) */

		scalev3(leftDir, radius, left);
		scalev3(upDir, radius, up);

		if(backEnd.viewParms.isMirror){
			subv3(vec3_origin, left, left);
		}

		/* compensate for scale in the axes if necessary */
		if(backEnd.currentEntity->e.nonNormalizedAxes){
			float axisLength;
			axisLength = lenv3(backEnd.currentEntity->e.axis[0]);
			if(!axisLength){
				axisLength = 0;
			}else{
				axisLength = 1.0f / axisLength;
			}
			scalev3(left, axisLength, left);
			scalev3(up, axisLength, up);
		}

		RB_AddQuadStamp(mid, left, up, tess.vertexColors[i]);
	}
}


/*
 * Autosprite2Deform
 *
 * Autosprite2 will pivot a rectangular quad along the center of its long axis
 */
int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void
Autosprite2Deform(void)
{
	int i, j, k;
	int indexes;
	float *xyz;
	Vec3 forward;

	if(tess.numVertexes & 3){
		ri.Printf(PRINT_WARNING, "Autosprite2 shader %s had odd vertex count", tess.shader->name);
	}
	if(tess.numIndexes != (tess.numVertexes >> 2) * 6){
		ri.Printf(PRINT_WARNING, "Autosprite2 shader %s had odd index count", tess.shader->name);
	}

	if(backEnd.currentEntity != &tr.worldEntity){
		GlobalVectorToLocal(backEnd.viewParms.or.axis[0], forward);
	}else{
		copyv3(backEnd.viewParms.or.axis[0], forward);
	}

	/* this is a lot of work for two triangles...
	 * we could precalculate a lot of it is an issue, but it would mess up
	 * the shader abstraction */
	for(i = 0, indexes = 0; i < tess.numVertexes; i+=4, indexes+=6){
		float lengths[2];
		int nums[2];
		Vec3	mid[2];
		Vec3	major, minor;
		float	*v1, *v2;

		/* find the midpoint */
		xyz = tess.xyz[i];

		/* identify the two shortest edges */
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for(j = 0; j < 6; j++){
			float l;
			Vec3 temp;

			v1 = xyz + 4 * edgeVerts[j][0];
			v2 = xyz + 4 * edgeVerts[j][1];

			subv3(v1, v2, temp);

			l = dotv3(temp, temp);
			if(l < lengths[0]){
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			}else if(l < lengths[1]){
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for(j = 0; j < 2; j++){
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		/* find the vector of the major axis */
		subv3(mid[1], mid[0], major);

		/* cross this with the view direction to get minor axis */
		crossv3(major, forward, minor);
		normv3(minor);

		/* re-project the points */
		for(j = 0; j < 2; j++){
			float l;

			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			l = 0.5 * sqrt(lengths[j]);

			/* we need to see which direction this edge
			 * is used to determine direction of projection */
			for(k = 0; k < 5; k++)
				if(tess.indexes[ indexes + k ] == i + edgeVerts[nums[j]][0]
				   && tess.indexes[ indexes + k + 1 ] == i + edgeVerts[nums[j]][1]){
					break;
				}

			if(k == 5){
				saddv3(mid[j], l, minor, v1);
				saddv3(mid[j], -l, minor, v2);
			}else{
				saddv3(mid[j], -l, minor, v1);
				saddv3(mid[j], l, minor, v2);
			}
		}
	}
}


/*
 * RB_DeformTessGeometry
 *
 */
void
RB_DeformTessGeometry(void)
{
	int i;
	deformStage_t *ds;

	if(!ShaderRequiresCPUDeforms(tess.shader)){
		/* we don't need the following CPU deforms */
		return;
	}

	for(i = 0; i < tess.shader->numDeforms; i++){
		ds = &tess.shader->deforms[ i ];

		switch(ds->deformation){
		case DEFORM_NONE:
			break;
		case DEFORM_NORMALS:
			RB_CalcDeformNormals(ds);
			break;
		case DEFORM_WAVE:
			RB_CalcDeformVertexes(ds);
			break;
		case DEFORM_BULGE:
			RB_CalcBulgeVertexes(ds);
			break;
		case DEFORM_MOVE:
			RB_CalcMoveVertexes(ds);
			break;
		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform();
			break;
		case DEFORM_AUTOSPRITE:
			AutospriteDeform();
			break;
		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform();
			break;
		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
			DeformText(backEnd.refdef.text[ds->deformation - DEFORM_TEXT0]);
			break;
		}
	}
}

/*
 *
 * COLORS
 *
 */


/*
** RB_CalcColorFromEntity
*/
void
RB_CalcColorFromEntity(unsigned char *dstColors)
{
	int i;
	int *pColors = ( int* )dstColors;
	int c;

	if(!backEnd.currentEntity)
		return;

	c = *( int* )backEnd.currentEntity->e.shaderRGBA;

	for(i = 0; i < tess.numVertexes; i++, pColors++)
		*pColors = c;
}

/*
** RB_CalcColorFromOneMinusEntity
*/
void
RB_CalcColorFromOneMinusEntity(unsigned char *dstColors)
{
	int i;
	int *pColors = ( int* )dstColors;
	unsigned char invModulate[4];
	int c;

	if(!backEnd.currentEntity)
		return;

	invModulate[0]	= 255 - backEnd.currentEntity->e.shaderRGBA[0];
	invModulate[1]	= 255 - backEnd.currentEntity->e.shaderRGBA[1];
	invModulate[2]	= 255 - backEnd.currentEntity->e.shaderRGBA[2];
	invModulate[3]	= 255 - backEnd.currentEntity->e.shaderRGBA[3];	/* this trashes alpha, but the AGEN block fixes it */

	c = *( int* )invModulate;

	for(i = 0; i < tess.numVertexes; i++, pColors++)
		*pColors = c;
}

/*
** RB_CalcAlphaFromEntity
*/
void
RB_CalcAlphaFromEntity(unsigned char *dstColors)
{
	int i;

	if(!backEnd.currentEntity)
		return;

	dstColors += 3;

	for(i = 0; i < tess.numVertexes; i++, dstColors += 4)
		*dstColors = backEnd.currentEntity->e.shaderRGBA[3];
}

/*
** RB_CalcAlphaFromOneMinusEntity
*/
void
RB_CalcAlphaFromOneMinusEntity(unsigned char *dstColors)
{
	int i;

	if(!backEnd.currentEntity)
		return;

	dstColors += 3;

	for(i = 0; i < tess.numVertexes; i++, dstColors += 4)
		*dstColors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
}

/*
** RB_CalcWaveColorSingle
*/
float
RB_CalcWaveColorSingle(const Waveform *wf)
{
	float glow;

	if(wf->func == GF_NOISE){
		glow = wf->base +
		       R_NoiseGet4f(0, 0, 0, (tess.shaderTime + wf->phase) * wf->frequency) * wf->amplitude;
	}else{
		glow = EvalWaveForm(wf) * tr.identityLight;
	}

	if(glow < 0){
		glow = 0;
	}else if(glow > 1){
		glow = 1;
	}

	return glow;
}

/*
** RB_CalcWaveColor
*/
void
RB_CalcWaveColor(const Waveform *wf, unsigned char *dstColors)
{
	int i;
	int v;
	float glow;
	int *colors = ( int* )dstColors;
	byte color[4];

	glow = RB_CalcWaveColorSingle(wf);

	v = ri.ftol(255 * glow);
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *(int*)color;

	for(i = 0; i < tess.numVertexes; i++, colors++)
		*colors = v;
}

/*
** RB_CalcWaveAlphaSingle
*/
float
RB_CalcWaveAlphaSingle(const Waveform *wf)
{
	return EvalWaveFormClamped(wf);
}

/*
** RB_CalcWaveAlpha
*/
void
RB_CalcWaveAlpha(const Waveform *wf, unsigned char *dstColors)
{
	int i;
	int v;
	float glow;

	glow = EvalWaveFormClamped(wf);

	v = 255 * glow;

	for(i = 0; i < tess.numVertexes; i++, dstColors += 4)
		dstColors[3] = v;
}

/*
** RB_CalcModulateColorsByFog
*/
void
RB_CalcModulateColorsByFog(unsigned char *colors)
{
	int i;
	float texCoords[SHADER_MAX_VERTEXES][2];

	/* calculate texcoords so we can derive density
	 * this is not wasted, because it would only have
	 * been previously called if the surface was opaque */
	RB_CalcFogTexCoords(texCoords[0]);

	for(i = 0; i < tess.numVertexes; i++, colors += 4){
		float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
	}
}

/*
** RB_CalcModulateAlphasByFog
*/
void
RB_CalcModulateAlphasByFog(unsigned char *colors)
{
	int i;
	float texCoords[SHADER_MAX_VERTEXES][2];

	/* calculate texcoords so we can derive density
	 * this is not wasted, because it would only have
	 * been previously called if the surface was opaque */
	RB_CalcFogTexCoords(texCoords[0]);

	for(i = 0; i < tess.numVertexes; i++, colors += 4){
		float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		colors[3] *= f;
	}
}

/*
** RB_CalcModulateRGBAsByFog
*/
void
RB_CalcModulateRGBAsByFog(unsigned char *colors)
{
	int i;
	float texCoords[SHADER_MAX_VERTEXES][2];

	/* calculate texcoords so we can derive density
	 * this is not wasted, because it would only have
	 * been previously called if the surface was opaque */
	RB_CalcFogTexCoords(texCoords[0]);

	for(i = 0; i < tess.numVertexes; i++, colors += 4){
		float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
		colors[3] *= f;
	}
}


/*
 *
 * TEX COORDS
 *
 */

/*
 * RB_CalcFogTexCoords
 *
 * To do the clipped fog plane really correctly, we should use
 * projected textures, but I don't trust the drivers and it
 * doesn't fit our shader data.
 */
void
RB_CalcFogTexCoords(float *st)
{
	int i;
	float *v;
	float s, t;
	float eyeT;
	qbool		eyeOutside;
	fog_t	*fog;
	Vec3	local;
	Vec4	fogdistv3Vector, fogDepthVector = {0, 0, 0, 0};

	fog = tr.world->fogs + tess.fogNum;

	/* all fogging distance is based on world Z units */
	subv3(backEnd.or.origin, backEnd.viewParms.or.origin, local);
	fogdistv3Vector[0] = -backEnd.or.modelMatrix[2];
	fogdistv3Vector[1] = -backEnd.or.modelMatrix[6];
	fogdistv3Vector[2] = -backEnd.or.modelMatrix[10];
	fogdistv3Vector[3] = dotv3(local, backEnd.viewParms.or.axis[0]);

	/* scale the fog vectors based on the fog's thickness */
	fogdistv3Vector[0] *= fog->tcScale;
	fogdistv3Vector[1] *= fog->tcScale;
	fogdistv3Vector[2] *= fog->tcScale;
	fogdistv3Vector[3] *= fog->tcScale;

	/* rotate the gradient vector for this orientation */
	if(fog->hasSurface){
		fogDepthVector[0] = fog->surface[0] * backEnd.or.axis[0][0] +
				    fog->surface[1] * backEnd.or.axis[0][1] + fog->surface[2] *
				    backEnd.or.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.or.axis[1][0] +
				    fog->surface[1] * backEnd.or.axis[1][1] + fog->surface[2] *
				    backEnd.or.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.or.axis[2][0] +
				    fog->surface[1] * backEnd.or.axis[2][1] + fog->surface[2] *
				    backEnd.or.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + dotv3(backEnd.or.origin, fog->surface);

		eyeT = dotv3(backEnd.or.viewOrigin, fogDepthVector) + fogDepthVector[3];
	}else{
		eyeT = 1;	/* non-surface fog always has eye inside */
	}

	/* see if the viewpoint is outside
	 * this is needed for clipping distance even for constant fog */

	if(eyeT < 0){
		eyeOutside = qtrue;
	}else{
		eyeOutside = qfalse;
	}

	fogdistv3Vector[3] += 1.0/512;

	/* calculate density for each point */
	for(i = 0, v = tess.xyz[0]; i < tess.numVertexes; i++, v += 4){
		/* calculate the length in fog */
		s = dotv3(v, fogdistv3Vector) + fogdistv3Vector[3];
		t = dotv3(v, fogDepthVector) + fogDepthVector[3];

		/* partially clipped fogs use the T axis */
		if(eyeOutside){
			if(t < 1.0){
				t = 1.0/32;	/* point is outside, so no fogging */
			}else{
				t = 1.0/32 + 30.0/32 * t / (t - eyeT);	/* cut the distance at the fog plane */
			}
		}else{
			if(t < 0){
				t = 1.0/32;	/* point is outside, so no fogging */
			}else{
				t = 31.0/32;
			}
		}

		st[0]	= s;
		st[1]	= t;
		st += 2;
	}
}



/*
** RB_CalcEnvironmentTexCoords
*/
void
RB_CalcEnvironmentTexCoords(float *st)
{
	int i;
	float *v, *normal;
	Vec3	viewer, reflected;
	float	d;

	v = tess.xyz[0];
	normal = tess.normal[0];

	for(i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, st += 2){
		subv3 (backEnd.or.viewOrigin, v, viewer);
		fastnormv3 (viewer);

		d = dotv3 (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0]	= 0.5 + reflected[1] * 0.5;
		st[1]	= 0.5 - reflected[2] * 0.5;
	}
}

/*
** RB_CalcTurbulentTexCoords
*/
void
RB_CalcTurbulentTexCoords(const Waveform *wf, float *st)
{
	int i;
	float now;

	now = (wf->phase + tess.shaderTime * wf->frequency);

	for(i = 0; i < tess.numVertexes; i++, st += 2){
		float s = st[0];
		float t = st[1];

		st[0] = s +
			tr.sinTable[ (( int )(((tess.xyz[i][0] +
						tess.xyz[i][2])* 1.0/128 * 0.125 +
					       now) * FUNCTABLE_SIZE)) & (FUNCTABLE_MASK) ] * wf->amplitude;
		st[1] = t +
			tr.sinTable[ (( int )((tess.xyz[i][1] * 1.0/128 * 0.125 +
					       now) * FUNCTABLE_SIZE)) & (FUNCTABLE_MASK) ] * wf->amplitude;
	}
}

void
RB_CalcTurbulentTexMatrix(const Waveform *wf, Mat4 matrix)
{
	float now;

	now = (wf->phase + tess.shaderTime * wf->frequency);

	/* bit of a hack here, hide amplitude and now in the matrix
	 * the vertex program will extract them and perform a turbulent pass last if it's nonzero */

	matrix[ 0] = 1.0f; matrix[ 4] = 0.0f; matrix[ 8] = 0.0f; matrix[12] = wf->amplitude;
	matrix[ 1] = 0.0f; matrix[ 5] = 1.0f; matrix[ 9] = 0.0f; matrix[13] = now;
	matrix[ 2] = 0.0f; matrix[ 6] = 0.0f; matrix[10] = 1.0f; matrix[14] = 0.0f;
	matrix[ 3] = 0.0f; matrix[ 7] = 0.0f; matrix[11] = 0.0f; matrix[15] = 1.0f;
}

/*
** RB_CalcScaleTexCoords
*/
void
RB_CalcScaleTexCoords(const float scale[2], float *st)
{
	int i;

	for(i = 0; i < tess.numVertexes; i++, st += 2){
		st[0]	*= scale[0];
		st[1]	*= scale[1];
	}
}

void
RB_CalcScaleTexMatrix(const float scale[2], float *matrix)
{
	matrix[ 0] = scale[0]; matrix[ 4] = 0.0f;     matrix[ 8] = 0.0f; matrix[12] = 0.0f;
	matrix[ 1] = 0.0f;     matrix[ 5] = scale[1]; matrix[ 9] = 0.0f; matrix[13] = 0.0f;
	matrix[ 2] = 0.0f;     matrix[ 6] = 0.0f;     matrix[10] = 1.0f; matrix[14] = 0.0f;
	matrix[ 3] = 0.0f;     matrix[ 7] = 0.0f;     matrix[11] = 0.0f; matrix[15] = 1.0f;
}

/*
** RB_CalcScrollTexCoords
*/
void
RB_CalcScrollTexCoords(const float scrollSpeed[2], float *st)
{
	int i;
	float timeScale = tess.shaderTime;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	/* clamp so coordinates don't continuously get larger, causing problems
	 * with hardware limits */
	adjustedScrollS = adjustedScrollS - floor(adjustedScrollS);
	adjustedScrollT = adjustedScrollT - floor(adjustedScrollT);

	for(i = 0; i < tess.numVertexes; i++, st += 2){
		st[0]	+= adjustedScrollS;
		st[1]	+= adjustedScrollT;
	}
}

void
RB_CalcScrollTexMatrix(const float scrollSpeed[2], float *matrix)
{
	float timeScale = tess.shaderTime;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	/* clamp so coordinates don't continuously get larger, causing problems
	 * with hardware limits */
	adjustedScrollS = adjustedScrollS - floor(adjustedScrollS);
	adjustedScrollT = adjustedScrollT - floor(adjustedScrollT);


	matrix[ 0] = 1.0f; matrix[ 4] = 0.0f; matrix[ 8] = adjustedScrollS; matrix[12] = 0.0f;
	matrix[ 1] = 0.0f; matrix[ 5] = 1.0f; matrix[ 9] = adjustedScrollT; matrix[13] = 0.0f;
	matrix[ 2] = 0.0f; matrix[ 6] = 0.0f; matrix[10] = 1.0f;            matrix[14] = 0.0f;
	matrix[ 3] = 0.0f; matrix[ 7] = 0.0f; matrix[11] = 0.0f;            matrix[15] = 1.0f;
}

/*
** RB_CalcTransformTexCoords
*/
void
RB_CalcTransformTexCoords(const texModInfo_t *tmi, float *st)
{
	int i;

	for(i = 0; i < tess.numVertexes; i++, st += 2){
		float s = st[0];
		float t = st[1];

		st[0]	= s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		st[1]	= s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

void
RB_CalcTransformTexMatrix(const texModInfo_t *tmi, float *matrix)
{
	matrix[ 0]	= tmi->matrix[0][0]; matrix[ 4] = tmi->matrix[1][0]; matrix[ 8] = tmi->translate[0];
	matrix[12]	= 0.0f;
	matrix[ 1]	= tmi->matrix[0][1]; matrix[ 5] = tmi->matrix[1][1]; matrix[ 9] = tmi->translate[1];
	matrix[13]	= 0.0f;
	matrix[ 2]	= 0.0f;              matrix[ 6] = 0.0f;              matrix[10] = 1.0f;
	matrix[14]	= 0.0f;
	matrix[ 3]	= 0.0f;              matrix[ 7] = 0.0f;              matrix[11] = 0.0f;
	matrix[15]	= 1.0f;
}

/*
** RB_CalcRotateTexCoords
*/
void
RB_CalcRotateTexCoords(float degsPerSecond, float *st)
{
	float timeScale = tess.shaderTime;
	float degs;
	int index;
	float sinValue, cosValue;
	texModInfo_t tmi;

	degs = -degsPerSecond * timeScale;
	index = degs * (FUNCTABLE_SIZE / 360.0f);

	sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	cosValue = tr.sinTable[ (index + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords(&tmi, st);
}

void
RB_CalcRotateTexMatrix(float degsPerSecond, float *matrix)
{
	float timeScale = tess.shaderTime;
	float degs;
	int index;
	float sinValue, cosValue;
	texModInfo_t tmi;

	degs = -degsPerSecond * timeScale;
	index = degs * (FUNCTABLE_SIZE / 360.0f);

	sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	cosValue = tr.sinTable[ (index + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexMatrix(&tmi, matrix);
}
/*
** RB_CalcSpecularAlpha
**
** Calculates specular coefficient and places it in the alpha channel
*/
Vec3 lightOrigin = { -960, 1980, 96 };	/* FIXME: track dynamically */

void
RB_CalcSpecularAlpha(unsigned char *alphas)
{
	int i;
	float *v, *normal;
	Vec3	viewer,  reflected;
	float	l, d;
	int	b;
	Vec3	lightDir;
	int	numVertexes;

	v = tess.xyz[0];
	normal = tess.normal[0];

	alphas += 3;

	numVertexes = tess.numVertexes;
	for(i = 0; i < numVertexes; i++, v += 4, normal += 4, alphas += 4){
		float ilength;

		subv3(lightOrigin, v, lightDir);
/*		ilength = Q_rsqrt( dotv3( lightDir, lightDir ) ); */
		fastnormv3(lightDir);

		/* calculate the specular color */
		d = dotv3 (normal, lightDir);
/*		d *= ilength; */

		/* we don't optimize for the d < 0 case since this tends to
		 * cause visual artifacts such as faceted "snapping" */
		reflected[0] = normal[0]*2*d - lightDir[0];
		reflected[1] = normal[1]*2*d - lightDir[1];
		reflected[2] = normal[2]*2*d - lightDir[2];

		subv3 (backEnd.or.viewOrigin, v, viewer);
		ilength = Q_rsqrt(dotv3(viewer, viewer));
		l	= dotv3 (reflected, viewer);
		l	*= ilength;

		if(l < 0){
			b = 0;
		}else{
			l = l*l;
			l = l*l;
			b = l * 255;
			if(b > 255){
				b = 255;
			}
		}

		*alphas = b;
	}
}

/*
** RB_CalcDiffuseColor
**
** The basic vertex lighting calc
*/
#if idppc_altivec
static void
RB_CalcDiffuseColor_altivec(unsigned char *colors)
{
	int i;
	float *v, *normal;
	trRefEntity_t *ent;
	int ambientLightInt;
	Vec3	lightDir;
	int	numVertexes;
	vector unsigned char vSel = VECCONST_UINT8(0x00, 0x00, 0x00, 0xff,
		0x00, 0x00, 0x00, 0xff,
		0x00, 0x00, 0x00, 0xff,
		0x00, 0x00, 0x00, 0xff);
	vector float ambientLightVec;
	vector float directedLightVec;
	vector float lightDirVec;
	vector float normalVec0, normalVec1;
	vector float incomingVec0, incomingVec1, incomingVec2;
	vector float zero, jVec;
	vector signed int jVecInt;
	vector signed short jVecShort;
	vector unsigned char jVecChar, normalPerm;
	ent = backEnd.currentEntity;
	ambientLightInt = ent->ambientLightInt;
	/* A lot of this could be simplified if we made sure
	 * entities light info was 16-byte aligned. */
	jVecChar = vec_lvsl(0, ent->ambientLight);
	ambientLightVec = vec_ld(0, (vector float*)ent->ambientLight);
	jVec = vec_ld(11, (vector float*)ent->ambientLight);
	ambientLightVec = vec_perm(ambientLightVec,jVec,jVecChar);

	jVecChar = vec_lvsl(0, ent->directedLight);
	directedLightVec = vec_ld(0,(vector float*)ent->directedLight);
	jVec = vec_ld(11,(vector float*)ent->directedLight);
	directedLightVec = vec_perm(directedLightVec,jVec,jVecChar);

	jVecChar = vec_lvsl(0, ent->lightDir);
	lightDirVec = vec_ld(0,(vector float*)ent->lightDir);
	jVec = vec_ld(11,(vector float*)ent->lightDir);
	lightDirVec = vec_perm(lightDirVec,jVec,jVecChar);

	zero = (vector float)vec_splat_s8(0);
	copyv3(ent->lightDir, lightDir);

	v = tess.xyz[0];
	normal = tess.normal[0];

	normalPerm = vec_lvsl(0,normal);
	numVertexes = tess.numVertexes;
	for(i = 0; i < numVertexes; i++, v += 4, normal += 4){
		normalVec0 = vec_ld(0,(vector float*)normal);
		normalVec1 = vec_ld(11,(vector float*)normal);
		normalVec0 = vec_perm(normalVec0,normalVec1,normalPerm);
		incomingVec0 = vec_madd(normalVec0, lightDirVec, zero);
		incomingVec1 = vec_sld(incomingVec0,incomingVec0,4);
		incomingVec2 = vec_add(incomingVec0,incomingVec1);
		incomingVec1 = vec_sld(incomingVec1,incomingVec1,4);
		incomingVec2 = vec_add(incomingVec2,incomingVec1);
		incomingVec0 = vec_splat(incomingVec2,0);
		incomingVec0 = vec_max(incomingVec0,zero);
		normalPerm = vec_lvsl(12,normal);
		jVec = vec_madd(incomingVec0, directedLightVec, ambientLightVec);
		jVecInt = vec_cts(jVec,0);						/* RGBx */
		jVecShort = vec_pack(jVecInt,jVecInt);					/* RGBxRGBx */
		jVecChar = vec_packsu(jVecShort,jVecShort);				/* RGBxRGBxRGBxRGBx */
		jVecChar = vec_sel(jVecChar,vSel,vSel);					/* RGBARGBARGBARGBA replace alpha with 255 */
		vec_ste((vector unsigned int)jVecChar,0,(unsigned int*)&colors[i*4]);	/* store color */
	}
}
#endif

static void
RB_CalcDiffuseColor_scalar(unsigned char *colors)
{
	int i, j;
	float *v, *normal;
	float incoming;
	trRefEntity_t *ent;
	int ambientLightInt;
	Vec3	ambientLight;
	Vec3	lightDir;
	Vec3	directedLight;
	int	numVertexes;
	ent = backEnd.currentEntity;
	ambientLightInt = ent->ambientLightInt;
	copyv3(ent->ambientLight, ambientLight);
	copyv3(ent->directedLight, directedLight);
	copyv3(ent->lightDir, lightDir);

	v = tess.xyz[0];
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;
	for(i = 0; i < numVertexes; i++, v += 4, normal += 4){
		incoming = dotv3 (normal, lightDir);
		if(incoming <= 0){
			*(int*)&colors[i*4] = ambientLightInt;
			continue;
		}
		j = ri.ftol(ambientLight[0] + incoming * directedLight[0]);
		if(j > 255){
			j = 255;
		}
		colors[i*4+0] = j;

		j = ri.ftol(ambientLight[1] + incoming * directedLight[1]);
		if(j > 255){
			j = 255;
		}
		colors[i*4+1] = j;

		j = ri.ftol(ambientLight[2] + incoming * directedLight[2]);
		if(j > 255){
			j = 255;
		}
		colors[i*4+2] = j;

		colors[i*4+3] = 255;
	}
}

void
RB_CalcDiffuseColor(unsigned char *colors)
{
#if idppc_altivec
	if(com_altivec->integer){
		/* must be in a seperate function or G3 systems will crash. */
		RB_CalcDiffuseColor_altivec(colors);
		return;
	}
#endif
	RB_CalcDiffuseColor_scalar(colors);
}
