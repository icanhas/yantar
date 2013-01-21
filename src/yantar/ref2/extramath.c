/*
 * Copyright (C) 2010 James Canete (use.less01@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* extra math needed by the renderer not in qmath.c */

#include "local.h"

/* Some matrix helper functions
 * FIXME: do these already exist in ioq3 and I don't know about them? */

qbool
SpheresIntersect(Vec3 origin1, float radius1, Vec3 origin2, float radius2)
{
	float radiusSum = radius1 + radius2;
	Vec3 diff;

	subv3(origin1, origin2, diff);

	if(dotv3(diff, diff) <= radiusSum * radiusSum){
		return qtrue;
	}

	return qfalse;
}

void
BoundingSphereOfSpheres(Vec3 origin1, float radius1, Vec3 origin2, float radius2, Vec3 origin3,
			float *radius3)
{
	Vec3 diff;

	scalev3(origin1, 0.5f, origin3);
	saddv3(origin3, 0.5f, origin2, origin3);

	subv3(origin1, origin2, diff);
	*radius3 = lenv3(diff) * 0.5f + MAX(radius1, radius2);
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
