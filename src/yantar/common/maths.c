/* stateless support routines that are included in each code module */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/* 
 * Some of the vector functions are static inline in q_shared.h. q3asm
 * doesn't understand static functions though, so we only want them in
 * one file. That's what this is about. 
 */
#ifdef Q3_VM
#define __Q3_VM_MATH
#endif

#include "shared.h"

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
		d = dotv3 (dir, bytedirs[i]);
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
		copyv3(vec3_origin, dir);
		return;
	}
	copyv3 (bytedirs[b], dir);
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
		clearv3(out);
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

	subv3(b, a, d1);
	subv3(c, a, d2);
	crossv3(d2, d1, plane);
	if(normv3(plane) == 0)
		return qfalse;

	plane[3] = dotv3(a, plane);
	return qtrue;
}

/* FIXME:This is not implemented very well... */
void
RotatePointAroundVector(Vec3 dst, const Vec3 dir, const Vec3 point,
	float degrees)
{
	float m[3][3], im[3][3], zrot[3][3], tmpmat[3][3], rot[3][3];
	int i;
	Vec3 vr, vup, vf;
	float rad;

	vf[0]	= dir[0];
	vf[1]	= dir[1];
	vf[2]	= dir[2];

	perpv3(vr, dir);
	crossv3(vr, vf, vup);

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
	perpv3(axis[1], axis[0]);

	/* rotate it around axis[0] by yaw */
	if(yaw){
		Vec3 temp;

		copyv3(axis[1], temp);
		RotatePointAroundVector(axis[1], axis[0], temp, yaw);
	}

	/* cross to get axis[2] */
	crossv3(axis[0], axis[1], axis[2]);
}

void
v3toeuler(const Vec3 value1, Vec3 angles)
{
	float	forward, yaw, pitch;

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
eulertoaxis(const Vec3 angles, Vec3 axis[3])
{
	Vec3 right;

	/* angle vectors returns "right" instead of "y axis" */
	anglev3s(angles, axis[0], right, axis[2]);
	subv3(vec3_origin, right, axis[1]);
}

void
clearaxis(Vec3 axis[3])
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
copyaxis(Vec3 in[3], Vec3 out[3])
{
	copyv3(in[0], out[0]);
	copyv3(in[1], out[1]);
	copyv3(in[2], out[2]);
}

void
ProjectPointOnPlane(Vec3 dst, const Vec3 p, const Vec3 normal)
{
	float	d;
	Vec3	n;
	float	inv_denom;

	inv_denom =  dotv3(normal, normal);
#ifndef Q3_VM
	assert(Q_fabs(inv_denom) != 0.0f);	/* zero vectors get here */
#endif
	inv_denom = 1.0f / inv_denom;

	d = dotv3(normal, p) * inv_denom;

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
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = dotv3 (right, forward);
	saddv3(right, -d, forward, right);
	normv3(right);
	crossv3(right, forward, up);
}

void
rotv3(Vec3 in, Vec3 matrix[3], Vec3 out)
{
	Vec3 tmp;

	copyv3(in, tmp);
	out[0] = dotv3(tmp, matrix[0]);
	out[1] = dotv3(tmp, matrix[1]);
	out[2] = dotv3(tmp, matrix[2]);
}

#if !idppc
float
Q_rsqrt(float number)
{
	float x2, y;
	const float threehalfs = 1.5f;
	Flint t;

	x2 = number * 0.5f;
	t.f = number;
	t.i = 0x5f375a86 - (t.i >> 1);
	y = t.f;
	/* first iteration */
	y = y * (threehalfs - (x2 * y * y));
	/* second iteration, this can be removed */
	y = y * (threehalfs - (x2 * y * y));
	return y; /* or number*y for just sqrt */
}

float
Q_fabs(float f)
{
	Flint fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}
#endif	/* !idppc */

qbool
closeenough(float x, float y, float thresh)
{
	return (Q_fabs(y - x) < thresh);
}

float
lerpeuler(float from, float to, float frac)
{
	float a;

	if(to - from > 180)
		to -= 360;
	if(to - from < -180)
		to += 360;
	a = from + frac * (to - from);

	return a;
}

/* subeuler: Always returns a value from -180 to 180 */
float
subeuler(float a1, float a2)
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
subeulers(Vec3 v1, Vec3 v2, Vec3 v3)
{
	v3[0] = subeuler(v1[0], v2[0]);
	v3[1] = subeuler(v1[1], v2[1]);
	v3[2] = subeuler(v1[2], v2[2]);
}

float
modeuler(float a)
{
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

/* returns angle normalized to the range [0 <= angle < 360] */
float
norm360euler(float angle)
{
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}

/* returns angle normalized to the range [-180 < angle <= 180] */
float
norm180euler(float angle)
{
	angle = norm360euler(angle);
	if(angle > 180.0)
		angle -= 360.0;
	return angle;
}

/* returns the normalized delta from angle1 to angle2 */
float
deltaeuler(float angle1, float angle2)
{
	return norm180euler(angle1 - angle2);
}

void
SetPlaneSignbits(Cplane *out)
{
	int bits, j;

	/* for fast box on planeside test */
	bits = 0;
	for(j=0; j<3; j++)
		if(out->normal[j] < 0)
			bits |= 1<<j;
	out->signbits = bits;
}

/* returns 1, 2, or 1 + 2 */
int
BoxOnPlaneSide(const Vec3 emins, const Vec3 emaxs, const struct Cplane *p)
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
		a = fabs(mins[i]);
		b = fabs(maxs[i]);
		corner[i] = a > b ? a : b;
	}
	return lenv3(corner);
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

/* Normalize v. Returns vector length. */
Scalar
normv3(Vec3 v)
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
norm2v3(const Vec3 v, Vec3 out)
{
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if(length){
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt(length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	}else
		clearv3(out);
	return length;
}

/*
  * combines a vector multiply and a vector addition.
  * make b s units long, add to v, result in o
  */
void
_saddv3(const Vec3 veca, float scale, const Vec3 vecb, Vec3 vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}

Scalar
_dotv3(const Vec3 v1, const Vec3 v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void
_subv3(const Vec3 veca, const Vec3 vecb, Vec3 out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void
_addv3(const Vec3 veca, const Vec3 vecb, Vec3 out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void
_copyv3(const Vec3 in, Vec3 out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void
_scalev3(const Vec3 in, Scalar scale, Vec3 out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void
scalev4(const Vec4 in, Scalar scale, Vec4 out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}

void
lerpv3(const Vec3 a, const Vec3 b, float amount, Vec3 c)
{
	c[0] = a[0] * (1.0f-amount) + b[0] * amount;
	c[1] = a[1] * (1.0f-amount) + b[1] * amount;
	c[2] = a[2] * (1.0f-amount) + b[2] * amount;
}

#define identmatrix(m, w) \
	do{	int i, j; \
		for(i = 0; i < (w); ++i) \
			for(j = 0; j < (w); ++j) \
				(m)[i*(w) + j] = (i==j ? 1.0f : 0.0f); \
	}while(0);
#define clearmatrix(m, w, h) \
	do{	int i; \
		for(i = 0; i < (w)*(h); ++i) \
			(m)[i] = 0.0f; \
	}while(0);
#define copymatrix(src, dst, w, h) \
	do{	int i; \
		for(i = 0; i < (w)*(h); ++i) \
			(dst)[i] = (src)[i]; \
	}while(0);
#define transposematrix(m, o, w, h) \
	do{	int i, j; \
		for(i = 0; i < (h); ++i) \
			for(j = 0; j < (w); ++j) \
				(o)[i + j*h] = (m)[i*w + j]; \
	}while(0);
#define scalematrix(m, s, o, w, h) \
	do{	int i; \
		for(i = 0; i < (w)*(h); ++i) \
			(o)[i] = (m)[i]*(s); \
	}while(0);
#define mulmatrix(a, b, out, sz) \
	do{	int i, j, k; Scalar ij; \
		for(i = 0; i < (sz); ++i) \
			for(j = 0; j < (sz); ++j){ \
				for(k = 0, ij = 0.0f; k < (sz); ++k) \
					ij += (a)[i*(sz) + k] * (b)[k*(sz) + j]; \
				(out)[i*(sz) + j] = ij; \
			} \
	}while(0);

static qbool
eqmatrix(const Scalar *a, const Scalar *b, int w, int h)
{
	int i;
	const int sz = w*h;

	for(i = 0; i < sz; ++i)
		if(a[i] != b[i])
			return qfalse;
	return qtrue;
}

void
clearm2(Mat2 m)
{
	clearmatrix(m, 2, 2);
}

void
identm2(Mat2 m)
{
	identmatrix(m, 2);
}

void
copym2(const Mat2 src, Mat2 dst)
{
	copymatrix(src, dst, 2, 2);
}

void
transposem2(const Mat2 m, Mat2 out)
{
	transposematrix(m, out, 2, 2);
}

void
scalem2(const Mat2 m, Scalar s, Mat2 out)
{
	scalematrix(m, s, out, 2, 2);
}

void
mulm2(const Mat2 a, const Mat2 b, Mat2 out)
{
	mulmatrix(a, b, out, 2);
}

qbool
cmpm2(const Mat2 a, const Mat2 b)
{
	return eqmatrix(a, b, 2, 2);
}

void
clearm3(Mat3 m)
{
	clearmatrix(m, 3, 3);
}

void
identm3(Mat3 m)
{
	identmatrix(m, 3);
}

void
copym3(const Mat3 src, Mat3 dst)
{
	copymatrix(src, dst, 3, 3);
}

void
transposem3(const Mat3 m, Mat3 out)
{
	transposematrix(m, out, 3, 3);
}

void
scalem3(const Mat3 m, Scalar s, Mat3 out)
{
	scalematrix(m, s, out, 3, 3);
}

void
mulm3(const Mat3 a, const Mat3 b, Mat3 out)
{
	mulmatrix(a, b, out, 3);
}

qbool
cmpm3(const Mat3 a, const Mat3 b)
{
	return eqmatrix(a, b, 3, 3);
}

void
clearm4(Mat4 m)
{
	clearmatrix(m, 4, 4);
}

void
identm4(Mat4 m)
{
	identmatrix(m, 4);
}

void
copym4(const Mat4 src, Mat4 dst)
{
	copymatrix(src, dst, 4, 4);
}

void
transposem4(const Mat4 m, Mat4 out)
{
	transposematrix(m, out, 4, 4);
}

void
scalem4(const Mat4 m, Scalar s, Mat4 out)
{
	scalematrix(m, s, out, 4, 4);
}

void
mulm4(const Mat4 a, const Mat4 b, Mat4 out)
{
	mulmatrix(a, b, out, 4);
}

void
transformm4(const Mat4 in1, const Vec4 in2, Vec4 out)
{
	out[ 0] = in1[ 0] * in2[ 0] + in1[ 4] * in2[ 1] + in1[ 8] * in2[ 2] + in1[12] * in2[ 3];
	out[ 1] = in1[ 1] * in2[ 0] + in1[ 5] * in2[ 1] + in1[ 9] * in2[ 2] + in1[13] * in2[ 3];
	out[ 2] = in1[ 2] * in2[ 0] + in1[ 6] * in2[ 1] + in1[10] * in2[ 2] + in1[14] * in2[ 3];
	out[ 3] = in1[ 3] * in2[ 0] + in1[ 7] * in2[ 1] + in1[11] * in2[ 2] + in1[15] * in2[ 3];
}

qbool
cmpm4(const Mat4 a, const Mat4 b)
{
	return eqmatrix(a, b, 4, 4);
}

void
translationm4(const Vec3 v, Mat4 out)
{	
	identm4(out);
	out[12] = v[0];
	out[13] = v[1];
	out[14] = v[2];
}

void
orthom4(float l, float r, float bottom, float top, float znear, float zfar, Mat4 out)
{
	clearm4(out);
	out[ 0] = 2.0f / (r - l);
	out[ 5] = 2.0f / (top - bottom);
	out[10] = 2.0f / (zfar - znear);
	out[12] = -(r + l) / (r - l);
	out[13] = -(top + bottom) / (top - bottom);
	out[14] = -(zfar + znear) / (zfar - znear);
	out[15] = 1.0f;
}

void
setq(Quat q, Scalar w, Scalar x, Scalar y, Scalar z)
{
	q[0] = w;
	q[1] = x;
	q[2] = y;
	q[3] = z;
}

/* Euler angles to quaternion */
void
eulertoq(const Vec3 angles, Quat out)
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
	out[0] = cr*cp*cy + sr*sp*sy;	/* r */
	out[1] = sr*cp*cy - cr*sp*sy;	/* v₀ */
	out[2] = cr*sp*cy + sr*cp*sy;	/* v₁ */
	out[3] = cr*cp*sy - sr*sp*cy;	/* v₂ */
}

/* Quaternion to Euler angles */
void
qtoeuler(const Quat q, Vec3 a)
{
	Quat q2;
	
	q2[0] = q[0]*q[0];
	q2[1] = q[1]*q[1];
	q2[2] = q[2]*q[2];
	q2[3] = q[3]*q[3];
	a[ROLL] = (180.0/M_PI)*atan2(2*(q[0]*q[1] + q[2]*q[3]), 1 - 2*(q2[1] + q2[2]));	/* φ */
	a[PITCH] = (180.0/M_PI)*asin(2*(q[0]*q[2] - q[3]*q[1]));			/* θ */
	a[YAW] = (180.0/M_PI)*atan2(2*(q[0]*q[3] + q[1]*q[2]), 1 - 2*(q2[2] + q2[3]));	/* ψ */
}

/* Quaternion  to axis representation */
void
qtoaxis(Quat q, Vec3  axis[3])
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

/*
 * out = q1q2
 */
void
mulq(const Quat q1, const Quat q2, Quat out)
{
if(1){
	float a, b, c, d, e, f, g, h;

	a = (q1[0] + q1[1])*(q2[0] + q2[1]);
	b = (q1[3] - q1[2])*(q2[2] - q2[3]);
	c = (q1[0] - q1[1])*(q2[2] + q2[3]);
	d = (q1[2] + q1[3])*(q2[0] - q2[1]);
	e = (q1[1] + q1[3])*(q2[1] + q2[2]);
	f = (q1[1] - q1[3])*(q2[1] - q2[2]);
	g = (q1[0] + q1[2])*(q2[0] - q2[3]);
	h = (q1[0] - q1[2])*(q2[0] + q2[3]);
	out[0] = b + (h - e - f + g)*0.5;	/* r */
	out[1] = a - (e + f + g + h)*0.5;	/* v0 */
	out[2] = c + (e - f + g - h)*0.5;	/* v1 */
	out[3] = d + (e - f - g + h)*0.5;	/* v2 */
}else{
	Scalar a, c;
	Vec3 b, d, cros, acc;

	/* 
	 * Sweetser calls this 'the easy way to multiply quaternions',
	 * and it almost works, but turns sort of flat & degenerate as
	 * you approach ±180° yaw.  I must be doing this wrong.
	 */
	a = q1[0];
	c = q2[0];
	copyv3(&q1[1], b);
	copyv3(&q2[1], d);
	out[0] = a*c - dotv3(b, d);
	scalev3(d, a, d);
	scalev3(b, c, b);
	addv3(d, b, acc);
	crossv3(d, b, cros);
	addv3(acc, cros, acc);
	copyv3(acc, &out[1]);
}
}

/*
 * returns magnitude/norm of q
 */
Scalar
magq(const Quat q)
{
	return sqrt(Square(q[0]) + Square(q[1]) + Square(q[2])
		+ Square(q[3]));
}

/*
 * out = conjugate of q
 * a - bi - cj - dk
 */
void
conjq(const Quat q, Quat out)
{
	copyv4(q, out);
	invv3(&out[1]);
}

/*
 * out = inverse/reciprocal of q
 * q^-1 = q* / ||q||^2 = q* / q x q*
 */
void
invq(const Quat q, Quat out)
{
	Scalar m;

	conjq(q, out);
	m = Square(magq(q));
	out[0] /= m;
	out[1] /= m;
	out[2] /= m;
	out[3] /= m;
}

/*
 * out = orientation between initial and final unit quaternions
 *
 * initial * diff = final
 * diff = final * initial^-1
 * diff = final * conj(initial)
 */
void
diffq(const Quat initial, const Quat final, Quat out)
{
	Quat ci;

	conjq(initial, ci);
	mulq(final, ci, out);
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

/* Rotate each 'axis' vector as specified by angles */
void
anglev3s(const Vec3 angles, Vec3 forward, Vec3 right, Vec3 up)
{
	float angle;
	static float sr, sp, sy, cr, cp, cy;	/* static to help MS compiler fp bugs */
	
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if(forward){
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if(right){
		right[0] = (-1*sr*sp*cy+ -1*cr* -sy);
		right[1] = (-1*sr*sp*sy+ -1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if(up){
		up[0] = (cr*sp*cy+ -sr* -sy);
		up[1] = (cr*sp*sy+ -sr*cy);
		up[2] = cr*cp;
	}
}

/* 
 * Generate perpendicular vector. Assumes src 
 * is normalized.
 */
void
perpv3(Vec3 dst, const Vec3 src)
{
	int pos, i;
	float	minelem = 1.0f;
	Vec3	tmp;

	/*
	 * find the smallest magnitude axially aligned vector
	 */
	for(pos = 0, i = 0; i < 3; i++)
		if(fabs(src[i]) < minelem){
			pos = i;
			minelem = fabs(src[i]);
		}
	tmp[0] = tmp[1] = tmp[2] = 0.0f;
	setv3(tmp, 0.0, 0.0, 0.0);
	tmp[pos] = 1.0f;

	/* project the point onto the plane defined by src */
	ProjectPointOnPlane(dst, tmp, src);

	/* normalize the result */
	normv3(dst);
}

/* Don't pass doubles to this */
int
Q_isnan(float x)
{
	Flint fi;

	fi.f = x;
	fi.ui	&= 0x7FFFFFFF;
	fi.ui	= 0x7F800000 - fi.ui;
	return (int)((unsigned int)fi.ui >> 31);
}

#ifndef Q3_VM
/*
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
