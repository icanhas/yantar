/*
 * Copyright (C) 2010 James Canete (use.less01@gmail.com)
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
/* tr_extramath.c - extra math needed by the renderer not in qmath.c */

#include "tr_local.h"

/* Some matrix helper functions
 * FIXME: do these already exist in ioq3 and I don't know about them? */

qbool
SpheresIntersect(vec3_t origin1, float radius1, vec3_t origin2, float radius2)
{
	float radiusSum = radius1 + radius2;
	vec3_t diff;

	VectorSubtract(origin1, origin2, diff);

	if(DotProduct(diff, diff) <= radiusSum * radiusSum){
		return qtrue;
	}

	return qfalse;
}

void
BoundingSphereOfSpheres(vec3_t origin1, float radius1, vec3_t origin2, float radius2, vec3_t origin3,
			float *radius3)
{
	vec3_t diff;

	VectorScale(origin1, 0.5f, origin3);
	VectorMA(origin3, 0.5f, origin2, origin3);

	VectorSubtract(origin1, origin2, diff);
	*radius3 = VectorLength(diff) * 0.5f + MAX(radius1, radius2);
}

int
NextPowerOfTwo(int in)
{
	int out;

	for(out = 1; out < in; out <<= 1)
		;

	return out;
}

unsigned short
FloatToHalf(float in)
{
	unsigned short out;

	union {
		float		f;
		unsigned int	i;
	} f32;

	int sign, inExponent, inFraction;
	int outExponent, outFraction;

	f32.f = in;

	sign = (f32.i & 0x80000000) >> 31;
	inExponent = (f32.i & 0x7F800000) >> 23;
	inFraction =  f32.i & 0x007FFFFF;

	outExponent = CLAMP(inExponent - 127, -15, 16) + 15;

	outFraction = 0;
	if(outExponent == 0x1F){
		if(inExponent == 0xFF && inFraction != 0)
			outFraction = 0x3FF;
	}else if(outExponent == 0x00){
		if(inExponent == 0x00 && inFraction != 0)
			outFraction = 0x3FF;
	}else
		outFraction = inFraction >> 13;

	out = (sign << 15) | (outExponent << 10) | outFraction;

	return out;
}
