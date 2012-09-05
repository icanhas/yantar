/* stateless support routines that are included in each code module */
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

/* Some of the vector functions are static inline in q_shared.h. q3asm
 * doesn't understand static functions though, so we only want them in
 * one file. That's what this is about. */
#ifdef Q3_VM
#define __Q3_VM_MATH
#endif

#include "q_shared.h"

Vec3	vec3_origin = {0,0,0};
Vec3	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

Vec4	colorBlack = {0, 0, 0, 1};
Vec4	colorRed = {1, 0, 0, 1};
Vec4	colorGreen = {0, 1, 0, 1};
Vec4	colorBlue = {0, 0, 1, 1};
Vec4	colorYellow = {1, 1, 0, 1};
Vec4	colorMagenta= {1, 0, 1, 1};
Vec4	colorCyan = {0, 1, 1, 1};
Vec4	colorWhite = {1, 1, 1, 1};
Vec4	colorLtGrey = {0.75, 0.75, 0.75, 1};
Vec4	colorMdGrey = {0.5, 0.5, 0.5, 1};
Vec4	colorDkGrey = {0.25, 0.25, 0.25, 1};

Vec4	g_color_table[8] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
};

Vec3 bytedirs[NUMVERTEXNORMALS] =
{
	{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f},
	{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f},
	{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f},
	{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f},
	{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f},
	{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f},
	{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f},
	{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f},
	{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f},
	{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f},
	{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f},
	{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f},
	{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f},
	{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f},
	{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f},
	{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f},
	{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f},
	{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f},
	{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f},
	{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f},
	{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f},
	{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f},
	{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f},
	{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f},
	{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f},
	{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f},
	{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f},
	{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f},
	{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f},
	{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f},
	{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f},
	{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f},
	{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f},
	{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f},
	{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f},
	{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f},
	{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f},
	{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f},
	{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f},
	{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f},
	{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f},
	{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f},
	{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f},
	{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f},
	{-0.309017f, -0.500000f,
	 -0.809017f}, {-0.162460f, -0.262866f, -0.951056f},
	{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f},
	{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f},
	{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f},
	{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f},
	{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f},
	{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f},
	{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f},
	{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f},
	{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f},
	{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f},
	{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f},
	{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f},
	{-0.500000f, -0.809017f,
	 -0.309017f}, {-0.262866f, -0.951056f, -0.162460f},
	{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f},
	{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f},
	{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f},
	{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f},
	{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f},
	{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f},
	{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f},
	{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f},
	{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f},
	{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f},
	{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f},
	{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f},
	{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f},
	{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f},
	{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f},
	{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f},
	{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f},
	{-0.864188f, -0.442863f,
	 -0.238856f}, {-0.951056f, -0.162460f, -0.262866f},
	{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f},
	{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f},
	{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f},
	{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f},
	{-0.587785f, -0.425325f,
	 -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

int
Q_rand(int *seed)
{
	*seed = (69069 * *seed + 1);
	return *seed;
}

float
Q_random(int *seed)
{
	return (Q_rand(seed) & 0xffff) / (float)0x10000;
}

float
Q_crandom(int *seed)
{
	return 2.0 * (Q_random(seed) - 0.5);
}

signed char
clampchar(int i)
{
	if(i < -128)
		return -128;
	if(i > 127)
		return 127;
	return i;
}

signed short
clampshort(int i)
{
	if(i < -32768)
		return -32768;
	if(i > 0x7fff)
		return 0x7fff;
	return i;
}

/* this isn't a real cheap function to call! */
int
DirToByte(Vec3 dir)
{
	int i, best;
	float d, bestd;

	if(!dir)
		return 0;

	bestd	= 0;
	best	= 0;
	for(i=0; i<NUMVERTEXNORMALS; i++){
		d = vec3dot (dir, bytedirs[i]);
		if(d > bestd){
			bestd	= d;
			best	= i;
		}
	}

	return best;
}

void
ByteToDir(int b, Vec3 dir)
{
	if(b < 0 || b >= NUMVERTEXNORMALS){
		vec3copy(vec3_origin, dir);
		return;
	}
	vec3copy (bytedirs[b], dir);
}

unsigned
colourbytes3(float r, float g, float b)
{
	unsigned i;

	((byte*)&i)[0]	= r * 255;
	((byte*)&i)[1]	= g * 255;
	((byte*)&i)[2]	= b * 255;

	return i;
}

unsigned
colourbytes4(float r, float g, float b, float a)
{
	unsigned i;

	((byte*)&i)[0]	= r * 255;
	((byte*)&i)[1]	= g * 255;
	((byte*)&i)[2]	= b * 255;
	((byte*)&i)[3]	= a * 255;

	return i;
}

float
normalizecolour(const Vec3 in, Vec3 out)
{
	float max;

	max = in[0];
	if(in[1] > max)
		max = in[1];
	if(in[2] > max)
		max = in[2];

	if(!max)
		vec3clear(out);
	else{
		out[0]	= in[0] / max;
		out[1]	= in[1] / max;
		out[2]	= in[2] / max;
	}
	return max;
}

/*
 * PlaneFromPoints: Returns false if the triangle is degenrate.
 * The normal will point out of the clock for clockwise ordered points
 */
qbool
PlaneFromPoints(Vec4 plane, const Vec3 a, const Vec3 b, const Vec3 c)
{
	Vec3 d1, d2;

	vec3sub(b, a, d1);
	vec3sub(c, a, d2);
	vec3cross(d2, d1, plane);
	if(vec3normalize(plane) == 0)
		return qfalse;

	plane[3] = vec3dot(a, plane);
	return qtrue;
}

/* RotatePointAroundVector: This is not implemented very well... */
void
RotatePointAroundVector(Vec3 dst, const Vec3 dir, const Vec3 point,
			float degrees)
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	Vec3	vr, vup, vf;
	float	rad;

	vf[0]	= dir[0];
	vf[1]	= dir[1];
	vf[2]	= dir[2];

	PerpendicularVector(vr, dir);
	vec3cross(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1]	= m[1][0];
	im[0][2]	= m[2][0];
	im[1][0]	= m[0][1];
	im[1][2]	= m[2][1];
	im[2][0]	= m[0][2];
	im[2][1]	= m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD(degrees);
	zrot[0][0]	= cos(rad);
	zrot[0][1]	= sin(rad);
	zrot[1][0]	= -sin(rad);
	zrot[1][1]	= cos(rad);

	MatrixMultiply(m, zrot, tmpmat);
	MatrixMultiply(tmpmat, im, rot);

	for(i = 0; i < 3; i++)
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] +
			 rot[i][2] * point[2];
}

void
RotateAroundDirection(Vec3 axis[3], float yaw)
{

	/* create an arbitrary axis[1] */
	PerpendicularVector(axis[1], axis[0]);

	/* rotate it around axis[0] by yaw */
	if(yaw){
		Vec3 temp;

		vec3copy(axis[1], temp);
		RotatePointAroundVector(axis[1], axis[0], temp, yaw);
	}

	/* cross to get axis[2] */
	vec3cross(axis[0], axis[1], axis[2]);
}

void
vec3toeuler(const Vec3 value1, Vec3 angles)
{
	float	forward;
	float	yaw, pitch;

	if(value1[1] == 0 && value1[0] == 0){
		yaw = 0;
		if(value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}else{
		if(value1[0])
			yaw = (atan2 (value1[1], value1[0]) * 180 / M_PI);
		else if(value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;
		if(yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch	= (atan2(value1[2], forward) * 180 / M_PI);
		if(pitch < 0)
			pitch += 360;
	}

	angles[PITCH]	= -pitch;
	angles[YAW]	= yaw;
	angles[ROLL]	= 0;
}

void
euler2axis(const Vec3 angles, Vec3 axis[3])
{
	Vec3 right;

	/* angle vectors returns "right" instead of "y axis" */
	anglevec3s(angles, axis[0], right, axis[2]);
	vec3sub(vec3_origin, right, axis[1]);
}

void
axisclear(Vec3 axis[3])
{
	axis[0][0]	= 1;
	axis[0][1]	= 0;
	axis[0][2]	= 0;
	axis[1][0]	= 0;
	axis[1][1]	= 1;
	axis[1][2]	= 0;
	axis[2][0]	= 0;
	axis[2][1]	= 0;
	axis[2][2]	= 1;
}

void
axiscopy(Vec3 in[3], Vec3 out[3])
{
	vec3copy(in[0], out[0]);
	vec3copy(in[1], out[1]);
	vec3copy(in[2], out[2]);
}

void
ProjectPointOnPlane(Vec3 dst, const Vec3 p, const Vec3 normal)
{
	float	d;
	Vec3	n;
	float	inv_denom;

	inv_denom =  vec3dot(normal, normal);
#ifndef Q3_VM
	assert(Q_fabs(inv_denom) != 0.0f);	/* zero vectors get here */
#endif
	inv_denom = 1.0f / inv_denom;

	d = vec3dot(normal, p) * inv_denom;

	n[0]	= normal[0] * inv_denom;
	n[1]	= normal[1] * inv_denom;
	n[2]	= normal[2] * inv_denom;

	dst[0]	= p[0] - d * n[0];
	dst[1]	= p[1] - d * n[1];
	dst[2]	= p[2] - d * n[2];
}

/*
 * MakeNormalVectors: Given a normalized forward vector, create two
 * other perpendicular vectors
 */
void
MakeNormalVectors(const Vec3 forward, Vec3 right, Vec3 up)
{
	float d;

	/* this rotate and negate guarantees a vector
	 * not colinear with the original */
	right[1]	= -forward[0];
	right[2]	= forward[1];
	right[0]	= forward[2];

	d = vec3dot (right, forward);
	vec3ma(right, -d, forward, right);
	vec3normalize(right);
	vec3cross(right, forward, up);
}

void
vec3rotate(Vec3 in, Vec3 matrix[3], Vec3 out)
{
	out[0]	= vec3dot(in, matrix[0]);
	out[1]	= vec3dot(in, matrix[1]);
	out[2]	= vec3dot(in, matrix[2]);
}

#if !idppc
float
Q_rsqrt(float number)
{
	float x2, y;
	const float	threehalfs = 1.5f;
	floatint_t	t;

	x2	= number * 0.5f;
	t.f	= number;
	t.i	= 0x5f375a86 - (t.i >> 1);
	y	= t.f;
	y	= y * (threehalfs - (x2 * y * y));	/* 1st iteration */
	y  	= y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed 

	return y;
}

float
Q_fabs(float f)
{
	floatint_t fi;
	fi.f	= f;
	fi.i	&= 0x7FFFFFFF;
	return fi.f;
}
#endif

float
lerpangle(float from, float to, float frac)
{
	float a;

	if(to - from > 180)
		to -= 360;
	if(to - from < -180)
		to += 360;
	a = from + frac * (to - from);

	return a;
}

/* anglesub: Always returns a value from -180 to 180 */
float
anglesub(float a1, float a2)
{
	float a;

	a = a1 - a2;
	while(a > 180)
		a -= 360;
	while(a < -180)
		a += 360;
	return a;
}

void
anglessub(Vec3 v1, Vec3 v2, Vec3 v3)
{
	v3[0]	= anglesub(v1[0], v2[0]);
	v3[1]	= anglesub(v1[1], v2[1]);
	v3[2]	= anglesub(v1[2], v2[2]);
}

float
anglemod(float a)
{
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

/* AngleNormalize360: returns angle normalized to the range [0 <= angle < 360] */
float
AngleNormalize360(float angle)
{
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}

/* AngleNormalize180: returns angle normalized to the range [-180 < angle <= 180] */
float
AngleNormalize180(float angle)
{
	angle = AngleNormalize360(angle);
	if(angle > 180.0)
		angle -= 360.0;
	return angle;
}

/* AngleDelta: returns the normalized delta from angle1 to angle2 */
float
AngleDelta(float angle1, float angle2)
{
	return AngleNormalize180(angle1 - angle2);
}

void
SetPlaneSignbits(cplane_t *out)
{
	int bits, j;

	/* for fast box on planeside test */
	bits = 0;
	for(j=0; j<3; j++)
		if(out->normal[j] < 0)
			bits |= 1<<j;
	out->signbits = bits;
}

/* BoxOnPlaneSide: returns 1, 2, or 1 + 2 */
int
BoxOnPlaneSide(const Vec3 emins, const Vec3 emaxs, const struct cplane_s *p)
{
	float	dist[2];
	int	sides;

	/* fast axial cases */
	if(p->type < 3){
		if(p->dist <= emins[p->type])
			return 1;
		if(p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	/* general case */
	switch(p->signbits){
	case 0:
		dist[0] = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emaxs[2];
		dist[1] = p->normal[0] * emins[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emins[2];
		break;
	case 1:
		dist[0] = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emaxs[2];
		dist[1] = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emins[2];
		break;
	case 2:
		dist[0] = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emaxs[2];
		dist[1] = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emins[2];
		break;
	case 3:
		dist[0] = p->normal[0] * emins[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emaxs[2];
		dist[1] = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emins[2];
		break;
	case 4:
		dist[0] = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emins[2];
		dist[1] = p->normal[0] * emins[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emaxs[2];
		break;
	case 5:
		dist[0] = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emins[2];
		dist[1] = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emaxs[2];
		break;
	case 6:
		dist[0] = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emins[2];
		dist[1] = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emaxs[2];
		break;
	case 7:
		dist[0] = p->normal[0] * emins[0] + p->normal[1] * emins[1] +
			  p->normal[2] * emins[2];
		dist[1] = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] +
			  p->normal[2] * emaxs[2];
		break;
	default:
		dist[0] = dist[1] = 0;	/* shut up compiler */
		break;
	}
	sides = 0;
	if(dist[0] >= p->dist)
		sides = 1;
	if(dist[1] < p->dist)
		sides |= 2;

	return sides;
}

float
RadiusFromBounds(const Vec3 mins, const Vec3 maxs)
{
	int i;
	Vec3	corner;
	float	a, b;

	for(i=0; i<3; i++){
		a	= fabs(mins[i]);
		b	= fabs(maxs[i]);
		corner[i] = a > b ? a : b;
	}

	return vec3len (corner);
}


void
ClearBounds(Vec3 mins, Vec3 maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void
AddPointToBounds(const Vec3 v, Vec3 mins, Vec3 maxs)
{
	if(v[0] < mins[0])
		mins[0] = v[0];
	if(v[0] > maxs[0])
		maxs[0] = v[0];

	if(v[1] < mins[1])
		mins[1] = v[1];
	if(v[1] > maxs[1])
		maxs[1] = v[1];

	if(v[2] < mins[2])
		mins[2] = v[2];
	if(v[2] > maxs[2])
		maxs[2] = v[2];
}

qbool
BoundsIntersect(const Vec3 mins, const Vec3 maxs,
		const Vec3 mins2, const Vec3 maxs2)
{
	if(maxs[0] < mins2[0] ||
	   maxs[1] < mins2[1] ||
	   maxs[2] < mins2[2] ||
	   mins[0] > maxs2[0] ||
	   mins[1] > maxs2[1] ||
	   mins[2] > maxs2[2])
		return qfalse;

	return qtrue;
}

qbool
BoundsIntersectSphere(const Vec3 mins, const Vec3 maxs,
		      const Vec3 origin, Scalar radius)
{
	if(origin[0] - radius > maxs[0] ||
	   origin[0] + radius < mins[0] ||
				origin[1] - radius > maxs[1] ||
	   origin[1] + radius < mins[1] ||
				origin[2] - radius > maxs[2] ||
	   origin[2] + radius < mins[2])
		return qfalse;

	return qtrue;
}

qbool
BoundsIntersectPoint(const Vec3 mins, const Vec3 maxs,
		     const Vec3 origin)
{
	if(origin[0] > maxs[0] ||
	   origin[0] < mins[0] ||
	   origin[1] > maxs[1] ||
	   origin[1] < mins[1] ||
	   origin[2] > maxs[2] ||
	   origin[2] < mins[2])
		return qfalse;

	return qtrue;
}

Scalar
vec3normalize(Vec3 v)
{
	/* NOTE: TTimo - Apple G4 altivec source uses double? */
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if(length){
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length	*= ilength;
		v[0]	*= ilength;
		v[1]	*= ilength;
		v[2]	*= ilength;
	}

	return length;
}

Scalar
vec3normalize2(const Vec3 v, Vec3 out)
{
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if(length){
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt(length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length	*= ilength;
		out[0]	= v[0]*ilength;
		out[1]	= v[1]*ilength;
		out[2]	= v[2]*ilength;
	}else
		vec3clear(out);

	return length;

}

/*
  * combines a vector multiply and a vector addition.
  * make b s units long, add to v, result in o
  */
void
_vec3ma(const Vec3 veca, float scale, const Vec3 vecb, Vec3 vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


Scalar
_vec3dot(const Vec3 v1, const Vec3 v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void
_vec3sub(const Vec3 veca, const Vec3 vecb, Vec3 out)
{
	out[0]	= veca[0]-vecb[0];
	out[1]	= veca[1]-vecb[1];
	out[2]	= veca[2]-vecb[2];
}

void
_vec3add(const Vec3 veca, const Vec3 vecb, Vec3 out)
{
	out[0]	= veca[0]+vecb[0];
	out[1]	= veca[1]+vecb[1];
	out[2]	= veca[2]+vecb[2];
}

void
_vec3copy(const Vec3 in, Vec3 out)
{
	out[0]	= in[0];
	out[1]	= in[1];
	out[2]	= in[2];
}

void
_vec3scale(const Vec3 in, Scalar scale, Vec3 out)
{
	out[0]	= in[0]*scale;
	out[1]	= in[1]*scale;
	out[2]	= in[2]*scale;
}

void
vec4scale(const Vec4 in, Scalar scale, Vec4 out)
{
	out[0]	= in[0]*scale;
	out[1]	= in[1]*scale;
	out[2]	= in[2]*scale;
	out[3]	= in[3]*scale;
}

void
vec3lerp(const Vec3 a, const Vec3 b, float amount, Vec3 c)
{
	c[0] = a[0] * (1.0f-amount) + b[0] * amount;
	c[1] = a[1] * (1.0f-amount) + b[1] * amount;
	c[2] = a[2] * (1.0f-amount) + b[2] * amount;
}

void
mat44clear(Mat44 m)
{
	unsigned int i;

	for(i = 0; i < 16; ++i)
		m[i] = 0.0f;
}

void
mat44ident(Mat44 m)
{
	m[ 0] = 1.0f; m[ 4] = 0.0f; m[ 8] = 0.0f; m[12] = 0.0f;
	m[ 1] = 0.0f; m[ 5] = 1.0f; m[ 9] = 0.0f; m[13] = 0.0f;
	m[ 2] = 0.0f; m[ 6] = 0.0f; m[10] = 1.0f; m[14] = 0.0f;
	m[ 3] = 0.0f; m[ 7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
}

void
mat44copy(const Mat44 in, Mat44 out)
{
	unsigned int i;

	for(i = 0; i < 16; ++i)
		out[i] = in[i];
}

void
mat44mul(const Mat44 a, const Mat44 b, Mat44 out)
{
	out[ 0] = a[ 0] * b[ 0] + a[ 4] * b[ 1] + a[ 8] * b[ 2] + a[12] * b[ 3];
	out[ 1] = a[ 1] * b[ 0] + a[ 5] * b[ 1] + a[ 9] * b[ 2] + a[13] * b[ 3];
	out[ 2] = a[ 2] * b[ 0] + a[ 6] * b[ 1] + a[10] * b[ 2] + a[14] * b[ 3];
	out[ 3] = a[ 3] * b[ 0] + a[ 7] * b[ 1] + a[11] * b[ 2] + a[15] * b[ 3];

	out[ 4] = a[ 0] * b[ 4] + a[ 4] * b[ 5] + a[ 8] * b[ 6] + a[12] * b[ 7];
	out[ 5] = a[ 1] * b[ 4] + a[ 5] * b[ 5] + a[ 9] * b[ 6] + a[13] * b[ 7];
	out[ 6] = a[ 2] * b[ 4] + a[ 6] * b[ 5] + a[10] * b[ 6] + a[14] * b[ 7];
	out[ 7] = a[ 3] * b[ 4] + a[ 7] * b[ 5] + a[11] * b[ 6] + a[15] * b[ 7];

	out[ 8] = a[ 0] * b[ 8] + a[ 4] * b[ 9] + a[ 8] * b[10] + a[12] * b[11];
	out[ 9] = a[ 1] * b[ 8] + a[ 5] * b[ 9] + a[ 9] * b[10] + a[13] * b[11];
	out[10] = a[ 2] * b[ 8] + a[ 6] * b[ 9] + a[10] * b[10] + a[14] * b[11];
	out[11] = a[ 3] * b[ 8] + a[ 7] * b[ 9] + a[11] * b[10] + a[15] * b[11];

	out[12] = a[ 0] * b[12] + a[ 4] * b[13] + a[ 8] * b[14] + a[12] * b[15];
	out[13] = a[ 1] * b[12] + a[ 5] * b[13] + a[ 9] * b[14] + a[13] * b[15];
	out[14] = a[ 2] * b[12] + a[ 6] * b[13] + a[10] * b[14] + a[14] * b[15];
	out[15] = a[ 3] * b[12] + a[ 7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

void
mat44transform(const Mat44 in1, const Vec4 in2, Vec4 out)
{
	out[ 0] = in1[ 0] * in2[ 0] + in1[ 4] * in2[ 1] + in1[ 8] * in2[ 2] + in1[12] * in2[ 3];
	out[ 1] = in1[ 1] * in2[ 0] + in1[ 5] * in2[ 1] + in1[ 9] * in2[ 2] + in1[13] * in2[ 3];
	out[ 2] = in1[ 2] * in2[ 0] + in1[ 6] * in2[ 1] + in1[10] * in2[ 2] + in1[14] * in2[ 3];
	out[ 3] = in1[ 3] * in2[ 0] + in1[ 7] * in2[ 1] + in1[11] * in2[ 2] + in1[15] * in2[ 3];
}

qbool
mat44cmp(const Mat44 a, const Mat44 b)
{
	unsigned int i;

	for(i = 0; i < 16; ++i)
		if(a[i] != b[i])
			return qfalse;
	return qtrue;
}

void
mat44translation(Vec3 v, Mat44 out)
{
	out[ 0] = 1.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = v[0];
	out[ 1] = 0.0f; out[ 5] = 1.0f; out[ 9] = 0.0f; out[13] = v[1];
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 1.0f; out[14] = v[2];
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}

void
mat44ortho(float l, float r, float bottom, float top, float znear, float zfar, Mat44 out)
{
	mat44clear(out);
	out[ 0] = 2.0f / (r - l);
	out[ 5] = 2.0f / (top - bottom);
	out[10] = 2.0f / (zfar - znear);
	out[12] = -(r + l) / (r - l);
	out[13] = -(top + bottom) / (top - bottom);
	out[14] = -(zfar + znear) / (zfar - znear);
	out[15] = 1.0f;
}

void
quatset(Quat q, Scalar w, Scalar x, Scalar y, Scalar z)
{
	q[0] = w;
	q[1] = x;
	q[2] = y;
	q[3] = z;
}

void
euler2quat(const Vec3 angles, Quat out)
{
	float cp, cy, cr, sp, sy, sr;
	Vec3 a;
	
	a[PITCH] = (M_PI/360.0f)*angles[PITCH];
	a[YAW] = (M_PI/360.0f)*angles[YAW];
	a[ROLL] = (M_PI/360.0f)*angles[ROLL];
	cp = cos(a[PITCH]);
	cy = cos(a[YAW]);
	cr = cos(a[ROLL]);
	sp = sin(a[PITCH]);
	sy = sin(a[YAW]);
	sr = sin(a[ROLL]);
	/* w, x, y, z */
	out[0] = cr * (cp*cy) + sr * (sp*sy);
	out[1] = sr * (cp*cy) - cr * (sp*sy);
	out[2] = cr * sp * cy + sr * cp * sy;
	out[3] = cr * cp * sy - sr * sp * cy;
}

void
quat2euler(const Quat q, Vec3 a)
{
	Quat q2;
	
	q2[0] = q[0]*q[0];
	q2[1] = q[1]*q[1];
	q2[2] = q[2]*q[2];
	q2[3] = q[3]*q[3];
	a[ROLL] = (180.0/M_PI)*atan2(2*(q[0]*q[1] + q[2]*q[3]), 1 - 2*(q2[1] + q2[2]));
	a[PITCH] = (180.0/M_PI)*asin(2*(q[0]*q[2] - q[3]*q[1]));
	a[YAW] = (180.0/M_PI)*atan2(2*(q[0]*q[3] + q[1]*q[2]), 1 - 2*(q2[2] + q2[3]));
}

void
quat2axis(Quat q, Vec3  axis[3])
{
	float wx, wy, wz, xx, xy, yy, yz, xz, zz, x2, y2, z2;
	
	x2 = q[1] + q[1];
	y2 = q[2] + q[2];
	z2 = q[3] + q[3];
	xx = q[1] * x2;
	xy = q[1] * y2;
	xz = q[1] * z2;
	yy = q[2] * y2;
	yz = q[2] * z2;
	zz = q[3] * z2;
	wx = q[0] * x2;
	wy = q[0] * y2;
	wz = q[0] * z2;
	axis[0][0] = 1.0 - (yy + zz);
	axis[1][0] = xy - wz;
	axis[2][0] = xz + wy;
	axis[0][1] = xy + wz;
	axis[1][1] = 1.0 - (xx + zz);
	axis[2][1] = yz - wx;
	axis[0][2] = xz - wy;
	axis[1][2] = yz + wx;
	axis[2][2] = 1.0 - (xx + yy);
}

void
quatmul(const Quat q1, const Quat q2, Quat out)
{
	float a, b, c, d, e, f, g, h;
	
	a = (q1[0] + q1[1]) * (q2[0] + q2[1]);
	b = (q1[3] - q1[2]) * (q2[2] - q2[3]);
	c = (q1[0] - q1[1]) * (q2[2] + q2[3]);
	d = (q1[2] + q1[3]) * (q2[0] - q2[1]);
	e = (q1[1] + q1[3]) * (q2[1] + q2[2]);
	f = (q1[1] - q1[3]) * (q2[1] - q2[2]);
	g = (q1[0] + q1[2]) * (q2[0] - q2[3]);
	h = (q1[0] - q1[2]) * (q2[0] + q2[3]);
	out[0] = b + (h - e - f + g) * 0.5;
	out[1] = a - (e + f + g + h) * 0.5;
	out[2] = c + (e - f + g - h) * 0.5;
	out[3] = d + (e - f - g + h) * 0.5;
}

int
Q_log2(int val)
{
	int answer;

	answer = 0;
	while((val>>=1) != 0)
		answer++;
	return answer;
}

/*
 * int	PlaneTypeForNormal (Vec3 normal) {
 *      if ( normal[0] == 1.0 )
 *              return PLANE_X;
 *      if ( normal[1] == 1.0 )
 *              return PLANE_Y;
 *      if ( normal[2] == 1.0 )
 *              return PLANE_Z;
 *
 *      return PLANE_NON_AXIAL;
 * }
 */

/*
 * MatrixMultiply
 * remove this!
 */
void
MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		    in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		    in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		    in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		    in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		    in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		    in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		    in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		    in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		    in1[2][2] * in2[2][2];
}


void
anglevec3s(const Vec3 angles, Vec3 forward, Vec3 right, Vec3 up)
{
	float angle;
	static float sr, sp, sy, cr, cp, cy;	/* static to help MS compiler fp bugs */
	
	angle	= angles[YAW] * (M_PI*2 / 360);
	sy	= sin(angle);
	cy	= cos(angle);
	angle	= angles[PITCH] * (M_PI*2 / 360);
	sp	= sin(angle);
	cp	= cos(angle);
	angle	= angles[ROLL] * (M_PI*2 / 360);
	sr	= sin(angle);
	cr	= cos(angle);

	if(forward){
		forward[0]	= cp*cy;
		forward[1]	= cp*sy;
		forward[2]	= -sp;
	}
	if(right){
		right[0]	= (-1*sr*sp*cy+ -1*cr* -sy);
		right[1]	= (-1*sr*sp*sy+ -1*cr*cy);
		right[2]	= -1*sr*cp;
	}
	if(up){
		up[0]	= (cr*sp*cy+ -sr* -sy);
		up[1]	= (cr*sp*sy+ -sr*cy);
		up[2]	= cr*cp;
	}
}

/* assumes "src" is normalized */
void
PerpendicularVector(Vec3 dst, const Vec3 src)
{
	int	pos;
	int	i;
	float	minelem = 1.0F;
	Vec3	tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for(pos = 0, i = 0; i < 3; i++)
		if(fabs(src[i]) < minelem){
			pos = i;
			minelem = fabs(src[i]);
		}
	tempvec[0]	= tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos]	= 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	vec3normalize(dst);
}

/* Don't pass doubles to this */
int
Q_isnan(float x)
{
	floatint_t fi;

	fi.f = x;
	fi.ui	&= 0x7FFFFFFF;
	fi.ui	= 0x7F800000 - fi.ui;
	return (int)((unsigned int)fi.ui >> 31);
}
/* ------------------------------------------------------------------------ */

#ifndef Q3_VM
/*
 * Q_acos
 *
 * the msvc acos doesn't always return a value between -PI and PI:
 *
 * int i;
 * i = 1065353246;
 * acos(*(float*) &i) == -1.#IND0
 *
 */
float
Q_acos(float c)
{
	float angle;

	angle = acos(c);

	if(angle > M_PI)
		return (float)M_PI;
	if(angle < -M_PI)
		return (float)M_PI;
	return angle;
}
#endif
