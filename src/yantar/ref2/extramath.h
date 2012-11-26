/*
 * Copyright (C) 2010 James Canete (use.less01@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* extramath.h */

#ifndef __TR_EXTRAMATH_H__
#define __TR_EXTRAMATH_H__

typedef int vec2i_t[2];
typedef int vec3i_t[2];

#define copyv32(a,b)	((b)[0]=(a)[0],(b)[1]=(a)[1])

#define copyv34(a,b)	((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define setv34(v,x,y,z,w)	((v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3]=(w))
#define dotv34(a,b)	((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + (a)[3]*(b)[3])
#define scalev34(a,b,c)	((c)[0]=(a)[0]*(b),(c)[1]=(a)[1]*(b),(c)[2]=(a)[2]*(b),(c)[3]=(a)[3]*(b))

#define copyv35(a,b)	((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3],(b)[4]=(a)[4])

#define OffsetByteToFloat(a)	((float)(a) * 1.0f/127.5f - 1.0f)
#define ByteToFloat(a)		((float)(a) * 1.0f/255.0f)
#define FloatToOffsetByte(a)	(byte)(((a) + 1.0f) * 127.5f)

static ID_INLINE int
cmpv34(const Vec4 v1, const Vec4 v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3]){
		return 0;
	}
	return 1;
}

static ID_INLINE int
cmpv35(const Vec5 v1, const Vec5 v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3] || v1[4] != v2[4]){
		return 0;
	}
	return 1;
}

qbool SpheresIntersect(Vec3 origin1, float radius1, Vec3 origin2, float radius2);
void BoundingSphereOfSpheres(Vec3 origin1, float radius1, Vec3 origin2, float radius2, Vec3 origin3,
			     float *radius3);

#ifndef SGN
#define SGN(x) (((x) >= 0) ? !!(x) : -1)
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(a,b,c) MIN(MAX((a),(b)),(c))
#endif

int NextPowerOfTwo(int in);
unsigned short FloatToHalf(float in);

#endif
