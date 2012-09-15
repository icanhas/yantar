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
#ifndef __MATHLIB__
#define __MATHLIB__

/* mathlib.h */

#include <math.h>

#ifdef DOUBLEVEC_T
typedef double Scalar;
#else
typedef float Scalar;
#endif
typedef Scalar Vec2[3];
typedef Scalar Vec3[3];
typedef Scalar Vec4[4];

#define SIDE_FRONT	0
#define SIDE_ON		2
#define SIDE_BACK	1
#define SIDE_CROSS	-2

#define Q_PI		3.14159265358979323846
#define DEG2RAD(a)	(((a) * Q_PI) / 180.0F)
#define RAD2DEG(a)	(((a) * 180.0f) / Q_PI)

extern Vec3 vec3_origin;

#define EQUAL_EPSILON 0.001

/* plane types are used to speed some tests
 * 0-2 are axial planes */
#define PLANE_X		0
#define PLANE_Y		1
#define PLANE_Z		2
#define PLANE_NON_AXIAL 3

qbool VectorCompare(const Vec3 v1, const Vec3 v2);

#define Vec3Dot(x,y)		(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define Vec3Sub(a,b,c)	{c[0]=a[0]-b[0]; c[1]=a[1]-b[1]; c[2]=a[2]-b[2]; }
#define Vec3Add(a,b,c)	{c[0]=a[0]+b[0]; c[1]=a[1]+b[1]; c[2]=a[2]+b[2]; }
#define Vec3Copy(a,b)		{b[0]=a[0]; b[1]=a[1]; b[2]=a[2]; }
#define VectorScale(a,b,c)	{c[0]=b*a[0]; c[1]=b*a[1]; c[2]=b*a[2]; }
#define VectorClear(x)		{x[0] = x[1] = x[2] = 0; }
#define VectorNegate(x)		{x[0]=-x[0]; x[1]=-x[1]; x[2]=-x[2]; }
void Vec10Copy(Scalar *in, Scalar *out);

Scalar Q_rint(Scalar in);
Scalar _Vec3Dot(Vec3 v1, Vec3 v2);
void _Vec3Sub(Vec3 va, Vec3 vb, Vec3 out);
void _Vec3Add(Vec3 va, Vec3 vb, Vec3 out);
void _Vec3Copy(Vec3 in, Vec3 out);
void _VectorScale(Vec3 v, Scalar scale, Vec3 out);

double Vec3Len(const Vec3 v);

void Vec3MA(const Vec3 va, double scale, const Vec3 vb, Vec3 vc);

void Vec3Cross(const Vec3 v1, const Vec3 v2, Vec3 cross);
Scalar Vec3Normalize(const Vec3 in, Vec3 out);
Scalar ColorNormalize(const Vec3 in, Vec3 out);
void Vec3Inverse(Vec3 v);

void ClearBounds(Vec3 mins, Vec3 maxs);
void AddPointToBounds(const Vec3 v, Vec3 mins, Vec3 maxs);

qbool PlaneFromPoints(Vec4 plane, const Vec3 a, const Vec3 b,
			 const Vec3 c);

void NormalToLatLong(const Vec3 normal, byte bytes[2]);

int     PlaneTypeForNormal(Vec3 normal);

#endif
