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
/* tr_main.c -- main control flow for each frame */

#include "tr_local.h"

#include <string.h>	/* memcpy */

trGlobals_t	tr;

static float	s_flipMatrix[16] = {
	/* convert from our coordinate system (looking down X)
	 * to OpenGL's coordinate system (looking down -Z) */
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


refimport_t ri;

/* entities that will have procedurally generated surfaces will just
 * point at this for their sorting surface */
surfaceType_t entitySurface = SF_ENTITY;

/*
 * R_CompareVert
 */
qbool
R_CompareVert(srfVert_t * v1, srfVert_t * v2, qbool checkST)
{
	int i;

	for(i = 0; i < 3; i++){
		if(floor(v1->xyz[i] + 0.1) != floor(v2->xyz[i] + 0.1)){
			return qfalse;
		}

		if(checkST && ((v1->st[0] != v2->st[0]) || (v1->st[1] != v2->st[1]))){
			return qfalse;
		}
	}

	return qtrue;
}

/*
 * R_CalcNormalForTriangle
 */
void
R_CalcNormalForTriangle(vec3_t normal, const vec3_t v0, const vec3_t v1, const vec3_t v2)
{
	vec3_t udir, vdir;

	/* compute the face normal based on vertex points */
	Vec3Sub(v2, v0, udir);
	Vec3Sub(v1, v0, vdir);
	Vec3Cross(udir, vdir, normal);

	Vec3Normalize(normal);
}

/*
 * R_CalcTangentsForTriangle
 * http://members.rogers.com/deseric/tangentspace.htm
 */
void
R_CalcTangentsForTriangle(vec3_t tangent, vec3_t bitangent,
			  const vec3_t v0, const vec3_t v1, const vec3_t v2,
			  const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	int i;
	vec3_t	planes[3];
	vec3_t	u, v;

	for(i = 0; i < 3; i++){
		VectorSet(u, v1[i] - v0[i], t1[0] - t0[0], t1[1] - t0[1]);
		VectorSet(v, v2[i] - v0[i], t2[0] - t0[0], t2[1] - t0[1]);

		Vec3Normalize(u);
		Vec3Normalize(v);

		Vec3Cross(u, v, planes[i]);
	}

	/* So your tangent space will be defined by this :
	 * Normal = Normal of the triangle or Tangent X Bitangent (careful with the cross product,
	 * you have to make sure the normal points in the right direction)
	 * Tangent = ( dp(Fx(s,t)) / ds,  dp(Fy(s,t)) / ds, dp(Fz(s,t)) / ds )   or     ( -Bx/Ax, -By/Ay, - Bz/Az )
	 * Bitangent =  ( dp(Fx(s,t)) / dt,  dp(Fy(s,t)) / dt, dp(Fz(s,t)) / dt )  or     ( -Cx/Ax, -Cy/Ay, -Cz/Az ) */

	/* tangent... */
	tangent[0]	= -planes[0][1] / planes[0][0];
	tangent[1]	= -planes[1][1] / planes[1][0];
	tangent[2]	= -planes[2][1] / planes[2][0];
	Vec3Normalize(tangent);

	/* bitangent... */
	bitangent[0]	= -planes[0][2] / planes[0][0];
	bitangent[1]	= -planes[1][2] / planes[1][0];
	bitangent[2]	= -planes[2][2] / planes[2][0];
	Vec3Normalize(bitangent);
}




/*
 * R_CalcTangentSpace
 */
void
R_CalcTangentSpace(vec3_t tangent, vec3_t bitangent, vec3_t normal,
		   const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1,
		   const vec2_t t2)
{
	vec3_t	cp, u, v;
	vec3_t	faceNormal;

	VectorSet(u, v1[0] - v0[0], t1[0] - t0[0], t1[1] - t0[1]);
	VectorSet(v, v2[0] - v0[0], t2[0] - t0[0], t2[1] - t0[1]);

	Vec3Cross(u, v, cp);
	if(fabs(cp[0]) > 10e-6){
		tangent[0]	= -cp[1] / cp[0];
		bitangent[0]	= -cp[2] / cp[0];
	}

	u[0]	= v1[1] - v0[1];
	v[0]	= v2[1] - v0[1];

	Vec3Cross(u, v, cp);
	if(fabs(cp[0]) > 10e-6){
		tangent[1]	= -cp[1] / cp[0];
		bitangent[1]	= -cp[2] / cp[0];
	}

	u[0]	= v1[2] - v0[2];
	v[0]	= v2[2] - v0[2];

	Vec3Cross(u, v, cp);
	if(fabs(cp[0]) > 10e-6){
		tangent[2]	= -cp[1] / cp[0];
		bitangent[2]	= -cp[2] / cp[0];
	}

	Vec3Normalize(tangent);
	Vec3Normalize(bitangent);

	/* compute the face normal based on vertex points */
	if(normal[0] == 0.0f && normal[1] == 0.0f && normal[2] == 0.0f){
		Vec3Sub(v2, v0, u);
		Vec3Sub(v1, v0, v);
		Vec3Cross(u, v, faceNormal);
	}else{
		Vec3Copy(normal, faceNormal);
	}

	Vec3Normalize(faceNormal);

#if 1
	/* Gram-Schmidt orthogonalize
	 * tangent[a] = (t - n * Dot(n, t)).Normalize(); */
	Vec3MA(tangent, -Vec3Dot(faceNormal, tangent), faceNormal, tangent);
	Vec3Normalize(tangent);

	/* compute the cross product B=NxT
	 * Vec3Cross(normal, tangent, bitangent); */
#else
	/* normal, compute the cross product N=TxB */
	Vec3Cross(tangent, bitangent, normal);
	Vec3Normalize(normal);

	if(Vec3Dot(normal, faceNormal) < 0){
		/* Vec3Inverse(normal);
		 * Vec3Inverse(tangent);
		 * Vec3Inverse(bitangent); */

		/* compute the cross product T=BxN */
		Vec3Cross(bitangent, faceNormal, tangent);

		/* compute the cross product B=NxT
		 * Vec3Cross(normal, tangent, bitangent); */
	}
#endif

	Vec3Copy(faceNormal, normal);
}

void
R_CalcTangentSpaceFast(vec3_t tangent, vec3_t bitangent, vec3_t normal,
		       const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1,
		       const vec2_t t2)
{
	vec3_t	cp, u, v;
	vec3_t	faceNormal;

	VectorSet(u, v1[0] - v0[0], t1[0] - t0[0], t1[1] - t0[1]);
	VectorSet(v, v2[0] - v0[0], t2[0] - t0[0], t2[1] - t0[1]);

	Vec3Cross(u, v, cp);
	if(fabs(cp[0]) > 10e-6){
		tangent[0]	= -cp[1] / cp[0];
		bitangent[0]	= -cp[2] / cp[0];
	}

	u[0]	= v1[1] - v0[1];
	v[0]	= v2[1] - v0[1];

	Vec3Cross(u, v, cp);
	if(fabs(cp[0]) > 10e-6){
		tangent[1]	= -cp[1] / cp[0];
		bitangent[1]	= -cp[2] / cp[0];
	}

	u[0]	= v1[2] - v0[2];
	v[0]	= v2[2] - v0[2];

	Vec3Cross(u, v, cp);
	if(fabs(cp[0]) > 10e-6){
		tangent[2]	= -cp[1] / cp[0];
		bitangent[2]	= -cp[2] / cp[0];
	}

	Vec3NormalizeFast(tangent);
	Vec3NormalizeFast(bitangent);

	/* compute the face normal based on vertex points */
	Vec3Sub(v2, v0, u);
	Vec3Sub(v1, v0, v);
	Vec3Cross(u, v, faceNormal);

	Vec3NormalizeFast(faceNormal);

#if 0
	/* normal, compute the cross product N=TxB */
	Vec3Cross(tangent, bitangent, normal);
	Vec3NormalizeFast(normal);

	if(Vec3Dot(normal, faceNormal) < 0){
		Vec3Inverse(normal);
		/* Vec3Inverse(tangent);
		 * Vec3Inverse(bitangent); */

		Vec3Cross(normal, tangent, bitangent);
	}

	Vec3Copy(faceNormal, normal);
#else
	/* Gram-Schmidt orthogonalize
	 * tangent[a] = (t - n * Dot(n, t)).Normalize(); */
	Vec3MA(tangent, -Vec3Dot(faceNormal, tangent), faceNormal, tangent);
	Vec3NormalizeFast(tangent);
#endif

	Vec3Copy(faceNormal, normal);
}

/*
 * http://www.terathon.com/code/tangent.html
 */
void
R_CalcTBN(vec3_t tangent, vec3_t bitangent, vec3_t normal,
	  const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec2_t w1, const vec2_t w2,
	  const vec2_t w3)
{
	vec3_t	u, v;
	float	x1, x2, y1, y2, z1, z2;
	float	s1, s2, t1, t2;
	float	r, dot;

	x1	= v2[0] - v1[0];
	x2	= v3[0] - v1[0];
	y1	= v2[1] - v1[1];
	y2	= v3[1] - v1[1];
	z1	= v2[2] - v1[2];
	z2	= v3[2] - v1[2];

	s1	= w2[0] - w1[0];
	s2	= w3[0] - w1[0];
	t1	= w2[1] - w1[1];
	t2	= w3[1] - w1[1];

	r = 1.0f / (s1 * t2 - s2 * t1);

	VectorSet(tangent, (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
	VectorSet(bitangent, (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

	/* compute the face normal based on vertex points */
	Vec3Sub(v3, v1, u);
	Vec3Sub(v2, v1, v);
	Vec3Cross(u, v, normal);

	Vec3Normalize(normal);

	/* Gram-Schmidt orthogonalize
	 * tangent[a] = (t - n * Dot(n, t)).Normalize(); */
	dot = Vec3Dot(normal, tangent);
	Vec3MA(tangent, -dot, normal, tangent);
	Vec3Normalize(tangent);

	/* B=NxT
	 * Vec3Cross(normal, tangent, bitangent); */
}

void
R_CalcTBN2(vec3_t tangent, vec3_t bitangent, vec3_t normal,
	   const vec3_t v1, const vec3_t v2, const vec3_t v3, const vec2_t t1, const vec2_t t2,
	   const vec2_t t3)
{
	vec3_t	v2v1;
	vec3_t	v3v1;

	float	c2c1_T;
	float	c2c1_B;

	float	c3c1_T;
	float	c3c1_B;

	float	denominator;
	float	scale1, scale2;

	vec3_t	T, B, N, C;


	/* Calculate the tangent basis for each vertex of the triangle
	 * UPDATE: In the 3rd edition of the accompanying article, the for-loop located here has
	 * been removed as it was redundant (the entire TBN matrix was calculated three times
	 * instead of just one).
	 *
	 * Please note, that this function relies on the fact that the input geometry are triangles
	 * and the tangent basis for each vertex thus is identical!
	 *  */

	/* Calculate the vectors from the current vertex to the two other vertices in the triangle */
	Vec3Sub(v2, v1, v2v1);
	Vec3Sub(v3, v1, v3v1);

	/* The equation presented in the article states that:
	 * c2c1_T = V2.texcoord.x  V1.texcoord.x
	 * c2c1_B = V2.texcoord.y  V1.texcoord.y
	 * c3c1_T = V3.texcoord.x  V1.texcoord.x
	 * c3c1_B = V3.texcoord.y  V1.texcoord.y */

	/* Calculate c2c1_T and c2c1_B */
	c2c1_T	= t2[0] - t1[0];
	c2c1_B	= t2[1] - t2[1];

	/* Calculate c3c1_T and c3c1_B */
	c3c1_T	= t3[0] - t1[0];
	c3c1_B	= t3[1] - t1[1];

	denominator = c2c1_T * c3c1_B - c3c1_T * c2c1_B;
	/* if(ROUNDOFF(fDenominator) == 0.0f) */
	if(denominator == 0.0f){
		/* We won't risk a divide by zero, so set the tangent matrix to the identity matrix */
		VectorSet(tangent, 1, 0, 0);
		VectorSet(bitangent, 0, 1, 0);
		VectorSet(normal, 0, 0, 1);
	}else{
		/* Calculate the reciprocal value once and for all (to achieve speed) */
		scale1 = 1.0f / denominator;

		/* T and B are calculated just as the equation in the article states */
		VectorSet(T, (c3c1_B * v2v1[0] - c2c1_B * v3v1[0]) * scale1,
			(c3c1_B * v2v1[1] - c2c1_B * v3v1[1]) * scale1,
			(c3c1_B * v2v1[2] - c2c1_B * v3v1[2]) * scale1);

		VectorSet(B, (-c3c1_T * v2v1[0] + c2c1_T * v3v1[0]) * scale1,
			(-c3c1_T * v2v1[1] + c2c1_T * v3v1[1]) * scale1,
			(-c3c1_T * v2v1[2] + c2c1_T * v3v1[2]) * scale1);

		/* The normal N is calculated as the cross product between T and B */
		Vec3Cross(T, B, N);

#if 0
		Vec3Copy(T, tangent);
		Vec3Copy(B, bitangent);
		Vec3Copy(N, normal);
#else
		/* Calculate the reciprocal value once and for all (to achieve speed) */
		scale2 = 1.0f / ((T[0] * B[1] * N[2] - T[2] * B[1] * N[0]) +
				 (B[0] * N[1] * T[2] - B[2] * N[1] * T[0]) +
				 (N[0] * T[1] * B[2] - N[2] * T[1] * B[0]));

		/* Calculate the inverse if the TBN matrix using the formula described in the article.
		 * We store the basis vectors directly in the provided TBN matrix: pvTBNMatrix */
		Vec3Cross(B, N, C); tangent[0]	= C[0] * scale2;
		Vec3Cross(N, T, C); tangent[1]	= -C[0] * scale2;
		Vec3Cross(T, B, C); tangent[2]	= C[0] * scale2;
		Vec3Normalize(tangent);

		Vec3Cross(B, N, C); bitangent[0]	= -C[1] * scale2;
		Vec3Cross(N, T, C); bitangent[1]	= C[1] * scale2;
		Vec3Cross(T, B, C); bitangent[2]	= -C[1] * scale2;
		Vec3Normalize(bitangent);

		Vec3Cross(B, N, C); normal[0]	= C[2] * scale2;
		Vec3Cross(N, T, C); normal[1]	= -C[2] * scale2;
		Vec3Cross(T, B, C); normal[2]	= C[2] * scale2;
		Vec3Normalize(normal);
#endif
	}
}


qbool
R_CalcTangentVectors(srfVert_t * dv[3])
{
	int i;
	float	bb, s, t;
	vec3_t	bary;


	/* calculate barycentric basis for the triangle */
	bb =
		(dv[1]->st[0] -
		 dv[0]->st[0]) *
		(dv[2]->st[1] - dv[0]->st[1]) - (dv[2]->st[0] - dv[0]->st[0]) * (dv[1]->st[1] - dv[0]->st[1]);
	if(fabs(bb) < 0.00000001f)
		return qfalse;

	/* do each vertex */
	for(i = 0; i < 3; i++){
		/* calculate s tangent vector */
		s	= dv[i]->st[0] + 10.0f;
		t	= dv[i]->st[1];
		bary[0] =
			((dv[1]->st[0] -
			  s) * (dv[2]->st[1] - t) - (dv[2]->st[0] - s) * (dv[1]->st[1] - t)) / bb;
		bary[1] =
			((dv[2]->st[0] -
			  s) * (dv[0]->st[1] - t) - (dv[0]->st[0] - s) * (dv[2]->st[1] - t)) / bb;
		bary[2] =
			((dv[0]->st[0] -
			  s) * (dv[1]->st[1] - t) - (dv[1]->st[0] - s) * (dv[0]->st[1] - t)) / bb;

		dv[i]->tangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] *
				    dv[2]->xyz[0];
		dv[i]->tangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] *
				    dv[2]->xyz[1];
		dv[i]->tangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] *
				    dv[2]->xyz[2];

		Vec3Sub(dv[i]->tangent, dv[i]->xyz, dv[i]->tangent);
		Vec3Normalize(dv[i]->tangent);

		/* calculate t tangent vector */
		s	= dv[i]->st[0];
		t	= dv[i]->st[1] + 10.0f;
		bary[0] =
			((dv[1]->st[0] -
			  s) * (dv[2]->st[1] - t) - (dv[2]->st[0] - s) * (dv[1]->st[1] - t)) / bb;
		bary[1] =
			((dv[2]->st[0] -
			  s) * (dv[0]->st[1] - t) - (dv[0]->st[0] - s) * (dv[2]->st[1] - t)) / bb;
		bary[2] =
			((dv[0]->st[0] -
			  s) * (dv[1]->st[1] - t) - (dv[1]->st[0] - s) * (dv[0]->st[1] - t)) / bb;

		dv[i]->bitangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] *
				      dv[2]->xyz[0];
		dv[i]->bitangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] *
				      dv[2]->xyz[1];
		dv[i]->bitangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] *
				      dv[2]->xyz[2];

		Vec3Sub(dv[i]->bitangent, dv[i]->xyz, dv[i]->bitangent);
		Vec3Normalize(dv[i]->bitangent);

		/* debug code
		 * % Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		 * %     stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] ); */
	}

	return qtrue;
}


/*
 * R_FindSurfaceTriangleWithEdge
 * Tr3B - recoded from Q2E
 */
static int
R_FindSurfaceTriangleWithEdge(int numTriangles, srfTriangle_t * triangles, int start, int end, int ignore)
{
	srfTriangle_t *tri;
	int	count, match;
	int	i;

	count	= 0;
	match	= -1;

	for(i = 0, tri = triangles; i < numTriangles; i++, tri++){
		if((tri->indexes[0] == start && tri->indexes[1] == end) ||
		   (tri->indexes[1] == start &&
		    tri->indexes[2] == end) || (tri->indexes[2] == start && tri->indexes[0] == end)){
			if(i != ignore){
				match = i;
			}

			count++;
		}else if((tri->indexes[1] == start && tri->indexes[0] == end) ||
			 (tri->indexes[2] == start &&
			  tri->indexes[1] == end) || (tri->indexes[0] == start && tri->indexes[2] == end)){
			count++;
		}
	}

	/* detect edges shared by three triangles and make them seams */
	if(count > 2){
		match = -1;
	}

	return match;
}


/*
 * R_CalcSurfaceTriangleNeighbors
 * Tr3B - recoded from Q2E
 */
void
R_CalcSurfaceTriangleNeighbors(int numTriangles, srfTriangle_t * triangles)
{
	int i;
	srfTriangle_t *tri;

	for(i = 0, tri = triangles; i < numTriangles; i++, tri++){
		tri->neighbors[0] =
			R_FindSurfaceTriangleWithEdge(numTriangles, triangles, tri->indexes[1],
				tri->indexes[0],
				i);
		tri->neighbors[1] =
			R_FindSurfaceTriangleWithEdge(numTriangles, triangles, tri->indexes[2],
				tri->indexes[1],
				i);
		tri->neighbors[2] =
			R_FindSurfaceTriangleWithEdge(numTriangles, triangles, tri->indexes[0],
				tri->indexes[2],
				i);
	}
}

/*
 * R_CalcSurfaceTrianglePlanes
 */
void
R_CalcSurfaceTrianglePlanes(int numTriangles, srfTriangle_t * triangles, srfVert_t * verts)
{
	int i;
	srfTriangle_t *tri;

	for(i = 0, tri = triangles; i < numTriangles; i++, tri++){
		float	*v1, *v2, *v3;
		vec3_t	d1, d2;

		v1	= verts[tri->indexes[0]].xyz;
		v2	= verts[tri->indexes[1]].xyz;
		v3	= verts[tri->indexes[2]].xyz;

		Vec3Sub(v2, v1, d1);
		Vec3Sub(v3, v1, d2);

		Vec3Cross(d2, d1, tri->plane);
		tri->plane[3] = Vec3Dot(tri->plane, v1);
	}
}


/*
 * R_CullLocalBox
 *
 * Returns CULL_IN, CULL_CLIP, or CULL_OUT
 */
int
R_CullLocalBox(vec3_t localBounds[2])
{
#if 0
	int i, j;
	vec3_t	transformed[8];
	float	dists[8];
	vec3_t	v;
	cplane_t        *frust;
	int	anyBack;
	int	front, back;

	if(r_nocull->integer){
		return CULL_CLIP;
	}

	/* transform into world space */
	for(i = 0; i < 8; i++){
		v[0]	= bounds[i&1][0];
		v[1]	= bounds[(i>>1)&1][1];
		v[2]	= bounds[(i>>2)&1][2];

		Vec3Copy(tr.or.origin, transformed[i]);
		Vec3MA(transformed[i], v[0], tr.or.axis[0], transformed[i]);
		Vec3MA(transformed[i], v[1], tr.or.axis[1], transformed[i]);
		Vec3MA(transformed[i], v[2], tr.or.axis[2], transformed[i]);
	}

	/* check against frustum planes */
	anyBack = 0;
	for(i = 0; i < 4; i++){
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for(j = 0; j < 8; j++){
			dists[j] = Vec3Dot(transformed[j], frust->normal);
			if(dists[j] > frust->dist){
				front = 1;
				if(back){
					break;	/* a point is in front */
				}
			}else{
				back = 1;
			}
		}
		if(!front){
			/* all points were behind one of the planes */
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if(!anyBack){
		return CULL_IN;	/* completely inside frustum */
	}

	return CULL_CLIP;	/* partially clipped */
#else
	int j;
	vec3_t	transformed;
	vec3_t	v;
	vec3_t	worldBounds[2];

	if(r_nocull->integer){
		return CULL_CLIP;
	}

	/* transform into world space */
	ClearBounds(worldBounds[0], worldBounds[1]);

	for(j = 0; j < 8; j++){
		v[0]	= localBounds[j & 1][0];
		v[1]	= localBounds[(j >> 1) & 1][1];
		v[2]	= localBounds[(j >> 2) & 1][2];

		R_LocalPointToWorld(v, transformed);

		AddPointToBounds(transformed, worldBounds[0], worldBounds[1]);
	}

	return R_CullBox(worldBounds);
#endif
}

/*
 * R_CullBox
 *
 * Returns CULL_IN, CULL_CLIP, or CULL_OUT
 */
int
R_CullBox(vec3_t worldBounds[2])
{
	int i;
	cplane_t	*frust;
	qbool		anyClip;
	int r;

	/* check against frustum planes */
	anyClip = qfalse;
	for(i = 0; i < 4 /*FRUSTUM_PLANES*/; i++){
		frust = &tr.viewParms.frustum[i];

		r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], frust);

		if(r == 2){
			/* completely outside frustum */
			return CULL_OUT;
		}
		if(r == 3){
			anyClip = qtrue;
		}
	}

	if(!anyClip){
		/* completely inside frustum */
		return CULL_IN;
	}

	/* partially clipped */
	return CULL_CLIP;
}

/*
** R_CullLocalPointAndRadius
*/
int
R_CullLocalPointAndRadius(const vec3_t pt, float radius)
{
	vec3_t transformed;

	R_LocalPointToWorld(pt, transformed);

	return R_CullPointAndRadius(transformed, radius);
}

/*
** R_CullPointAndRadius
*/
int
R_CullPointAndRadiusEx(const vec3_t pt, float radius, const cplane_t* frustum, int numPlanes)
{
	int i;
	float dist;
	const cplane_t *frust;
	qbool mightBeClipped = qfalse;

	if(r_nocull->integer){
		return CULL_CLIP;
	}

	/* check against frustum planes */
	for(i = 0; i < numPlanes; i++){
		frust = &frustum[i];

		dist = Vec3Dot(pt, frust->normal) - frust->dist;
		if(dist < -radius){
			return CULL_OUT;
		}else if(dist <= radius){
			mightBeClipped = qtrue;
		}
	}

	if(mightBeClipped){
		return CULL_CLIP;
	}

	return CULL_IN;	/* completely inside frustum */
}

/*
** R_CullPointAndRadius
*/
int
R_CullPointAndRadius(const vec3_t pt, float radius)
{
	return R_CullPointAndRadiusEx(pt, radius, tr.viewParms.frustum, ARRAY_LEN(tr.viewParms.frustum));
}

/*
 * R_LocalNormalToWorld
 *
 */
void
R_LocalNormalToWorld(const vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] *
		   tr.or.axis[2][0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] *
		   tr.or.axis[2][1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] *
		   tr.or.axis[2][2];
}

/*
 * R_LocalPointToWorld
 *
 */
void
R_LocalPointToWorld(const vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] *
		   tr.or.axis[2][0] + tr.or.origin[0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] *
		   tr.or.axis[2][1] + tr.or.origin[1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] *
		   tr.or.axis[2][2] + tr.or.origin[2];
}

/*
 * R_WorldToLocal
 *
 */
void
R_WorldToLocal(const vec3_t world, vec3_t local)
{
	local[0]	= Vec3Dot(world, tr.or.axis[0]);
	local[1]	= Vec3Dot(world, tr.or.axis[1]);
	local[2]	= Vec3Dot(world, tr.or.axis[2]);
}

/*
 * R_TransformModelToClip
 *
 */
void
R_TransformModelToClip(const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
		       vec4_t eye, vec4_t dst)
{
	int i;

	for(i = 0; i < 4; i++)
		eye[i] =
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];

	for(i = 0; i < 4; i++)
		dst[i] =
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
}

/*
 * R_TransformClipToWindow
 *
 */
void
R_TransformClipToWindow(const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window)
{
	normalized[0]	= clip[0] / clip[3];
	normalized[1]	= clip[1] / clip[3];
	normalized[2]	= (clip[2] + clip[3]) / (2 * clip[3]);

	window[0]	= 0.5f * (1.0f + normalized[0]) * view->viewportWidth;
	window[1]	= 0.5f * (1.0f + normalized[1]) * view->viewportHeight;
	window[2]	= normalized[2];

	window[0]	= (int)(window[0] + 0.5);
	window[1]	= (int)(window[1] + 0.5);
}


/*
 * myGlMultMatrix
 *
 */
void
myGlMultMatrix(const float *a, const float *b, float *out)
{
	int i, j;

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
}

/*
 * R_RotateForEntity
 *
 * Generates an orientation for an entity and viewParms
 * Does NOT produce any GL calls
 * Called by both the front end and the back end
 */
void
R_RotateForEntity(const trRefEntity_t *ent, const viewParms_t *viewParms,
		  orientationr_t *or)
{
	float glMatrix[16];
	vec3_t	delta;
	float	axisLength;

	if(ent->e.reType != RT_MODEL){
		*or = viewParms->world;
		return;
	}

	Vec3Copy(ent->e.origin, or->origin);

	Vec3Copy(ent->e.axis[0], or->axis[0]);
	Vec3Copy(ent->e.axis[1], or->axis[1]);
	Vec3Copy(ent->e.axis[2], or->axis[2]);

	glMatrix[0]	= or->axis[0][0];
	glMatrix[4]	= or->axis[1][0];
	glMatrix[8]	= or->axis[2][0];
	glMatrix[12]	= or->origin[0];

	glMatrix[1]	= or->axis[0][1];
	glMatrix[5]	= or->axis[1][1];
	glMatrix[9]	= or->axis[2][1];
	glMatrix[13]	= or->origin[1];

	glMatrix[2]	= or->axis[0][2];
	glMatrix[6]	= or->axis[1][2];
	glMatrix[10]	= or->axis[2][2];
	glMatrix[14]	= or->origin[2];

	glMatrix[3]	= 0;
	glMatrix[7]	= 0;
	glMatrix[11]	= 0;
	glMatrix[15]	= 1;

	Mat4x4Copy(glMatrix, or->transformMatrix);
	myGlMultMatrix(glMatrix, viewParms->world.modelMatrix, or->modelMatrix);

	/* calculate the viewer origin in the model's space
	 * needed for fog, specular, and environment mapping */
	Vec3Sub(viewParms->or.origin, or->origin, delta);

	/* compensate for scale in the axes if necessary */
	if(ent->e.nonNormalizedAxes){
		axisLength = Vec3Len(ent->e.axis[0]);
		if(!axisLength){
			axisLength = 0;
		}else{
			axisLength = 1.0f / axisLength;
		}
	}else{
		axisLength = 1.0f;
	}

	or->viewOrigin[0]	= Vec3Dot(delta, or->axis[0]) * axisLength;
	or->viewOrigin[1]	= Vec3Dot(delta, or->axis[1]) * axisLength;
	or->viewOrigin[2]	= Vec3Dot(delta, or->axis[2]) * axisLength;
}

/*
 * R_RotateForViewer
 *
 * Sets up the modelview matrix for a given viewParm
 */
void
R_RotateForViewer(void)
{
	float	viewerMatrix[16];
	vec3_t	origin;

	Q_Memset (&tr.or, 0, sizeof(tr.or));
	tr.or.axis[0][0]	= 1;
	tr.or.axis[1][1]	= 1;
	tr.or.axis[2][2]	= 1;
	Vec3Copy (tr.viewParms.or.origin, tr.or.viewOrigin);

	/* transform by the camera placement */
	Vec3Copy(tr.viewParms.or.origin, origin);

	viewerMatrix[0]		= tr.viewParms.or.axis[0][0];
	viewerMatrix[4]		= tr.viewParms.or.axis[0][1];
	viewerMatrix[8]		= tr.viewParms.or.axis[0][2];
	viewerMatrix[12]	= -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] *
				  viewerMatrix[8];

	viewerMatrix[1]		= tr.viewParms.or.axis[1][0];
	viewerMatrix[5]		= tr.viewParms.or.axis[1][1];
	viewerMatrix[9]		= tr.viewParms.or.axis[1][2];
	viewerMatrix[13]	= -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] *
				  viewerMatrix[9];

	viewerMatrix[2]		= tr.viewParms.or.axis[2][0];
	viewerMatrix[6]		= tr.viewParms.or.axis[2][1];
	viewerMatrix[10]	= tr.viewParms.or.axis[2][2];
	viewerMatrix[14]	= -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] *
				  viewerMatrix[10];

	viewerMatrix[3]		= 0;
	viewerMatrix[7]		= 0;
	viewerMatrix[11]	= 0;
	viewerMatrix[15]	= 1;

	/* convert from our coordinate system (looking down X)
	 * to OpenGL's coordinate system (looking down -Z) */
	myGlMultMatrix(viewerMatrix, s_flipMatrix, tr.or.modelMatrix);

	tr.viewParms.world = tr.or;

}

/*
** SetFarClip
*/
static void
R_SetFarClip(void)
{
	float	farthestCornerVec3Distance = 0;
	int	i;

	/* if not rendering the world (icons, menus, etc)
	 * set a 2k far clip plane */
	if(tr.refdef.rdflags & RDF_NOWORLDMODEL){
		tr.viewParms.zFar = 2048;
		return;
	}

	/*
	 * set far clipping planes dynamically
	 *  */
	farthestCornerVec3Distance = 0;
	for(i = 0; i < 8; i++){
		vec3_t	v;
		vec3_t	vecTo;
		float	distance;

		if(i & 1){
			v[0] = tr.viewParms.visBounds[0][0];
		}else{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if(i & 2){
			v[1] = tr.viewParms.visBounds[0][1];
		}else{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if(i & 4){
			v[2] = tr.viewParms.visBounds[0][2];
		}else{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		Vec3Sub(v, tr.viewParms.or.origin, vecTo);

		distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if(distance > farthestCornerVec3Distance){
			farthestCornerVec3Distance = distance;
		}
	}
	tr.viewParms.zFar = sqrt(farthestCornerVec3Distance);
}

/*
 * R_SetupFrustum
 *
 * Set up the culling frustum planes for the current view using the results we got from computing the first two rows of
 * the projection matrix.
 */
void
R_SetupFrustum(viewParms_t *dest, float xmin, float xmax, float ymax, float zProj, float zFar,
	       float stereoSep)
{
	vec3_t	ofsorigin;
	float	oppleg, adjleg, length;
	int	i;

	if(stereoSep == 0 && xmin == -xmax){
		/* symmetric case can be simplified */
		Vec3Copy(dest->or.origin, ofsorigin);

		length	= sqrt(xmax * xmax + zProj * zProj);
		oppleg	= xmax / length;
		adjleg	= zProj / length;

		VectorScale(dest->or.axis[0], oppleg, dest->frustum[0].normal);
		Vec3MA(dest->frustum[0].normal, adjleg, dest->or.axis[1], dest->frustum[0].normal);

		VectorScale(dest->or.axis[0], oppleg, dest->frustum[1].normal);
		Vec3MA(dest->frustum[1].normal, -adjleg, dest->or.axis[1], dest->frustum[1].normal);
	}else{
		/* In stereo rendering, due to the modification of the projection matrix, dest->or.origin is not the
		 * actual origin that we're rendering so offset the tip of the view pyramid. */
		Vec3MA(dest->or.origin, stereoSep, dest->or.axis[1], ofsorigin);

		oppleg	= xmax + stereoSep;
		length	= sqrt(oppleg * oppleg + zProj * zProj);
		VectorScale(dest->or.axis[0], oppleg / length, dest->frustum[0].normal);
		Vec3MA(dest->frustum[0].normal, zProj / length, dest->or.axis[1], dest->frustum[0].normal);

		oppleg	= xmin + stereoSep;
		length	= sqrt(oppleg * oppleg + zProj * zProj);
		VectorScale(dest->or.axis[0], -oppleg / length, dest->frustum[1].normal);
		Vec3MA(dest->frustum[1].normal, -zProj / length, dest->or.axis[1], dest->frustum[1].normal);
	}

	length	= sqrt(ymax * ymax + zProj * zProj);
	oppleg	= ymax / length;
	adjleg	= zProj / length;

	VectorScale(dest->or.axis[0], oppleg, dest->frustum[2].normal);
	Vec3MA(dest->frustum[2].normal, adjleg, dest->or.axis[2], dest->frustum[2].normal);

	VectorScale(dest->or.axis[0], oppleg, dest->frustum[3].normal);
	Vec3MA(dest->frustum[3].normal, -adjleg, dest->or.axis[2], dest->frustum[3].normal);

	for(i=0; i<4; i++){
		dest->frustum[i].type	= PLANE_NON_AXIAL;
		dest->frustum[i].dist	= Vec3Dot (ofsorigin, dest->frustum[i].normal);
		SetPlaneSignbits(&dest->frustum[i]);
	}

	if(zFar != 0.0f){
		vec3_t farpoint;

		Vec3MA(ofsorigin, zFar, dest->or.axis[0], farpoint);
		VectorScale(dest->or.axis[0], -1.0f, dest->frustum[4].normal);

		dest->frustum[4].type	= PLANE_NON_AXIAL;
		dest->frustum[4].dist	= Vec3Dot (farpoint, dest->frustum[4].normal);
		SetPlaneSignbits(&dest->frustum[4]);
	}
}

/*
 * R_SetupProjection
 */
void
R_SetupProjection(viewParms_t *dest, float zProj, float zFar, qbool computeFrustum)
{
	float	xmin, xmax, ymin, ymax;
	float	width, height, stereoSep = r_stereoSeparation->value;

	/*
	 * offset the view origin of the viewer for stereo rendering
	 * by setting the projection matrix appropriately.
	 */

	if(stereoSep != 0){
		if(dest->stereoFrame == STEREO_LEFT)
			stereoSep = zProj / stereoSep;
		else if(dest->stereoFrame == STEREO_RIGHT)
			stereoSep = zProj / -stereoSep;
		else
			stereoSep = 0;
	}

	ymax	= zProj * tan(dest->fovY * M_PI / 360.0f);
	ymin	= -ymax;

	xmax	= zProj * tan(dest->fovX * M_PI / 360.0f);
	xmin	= -xmax;

	width	= xmax - xmin;
	height	= ymax - ymin;

	dest->projectionMatrix[0]	= 2 * zProj / width;
	dest->projectionMatrix[4]	= 0;
	dest->projectionMatrix[8]	= (xmax + xmin + 2 * stereoSep) / width;
	dest->projectionMatrix[12]	= 2 * zProj * stereoSep / width;

	dest->projectionMatrix[1]	= 0;
	dest->projectionMatrix[5]	= 2 * zProj / height;
	dest->projectionMatrix[9]	= (ymax + ymin) / height;	/* normally 0 */
	dest->projectionMatrix[13]	= 0;

	dest->projectionMatrix[3]	= 0;
	dest->projectionMatrix[7]	= 0;
	dest->projectionMatrix[11]	= -1;
	dest->projectionMatrix[15]	= 0;

	/* Now that we have all the data for the projection matrix we can also setup the view frustum. */
	if(computeFrustum)
		R_SetupFrustum(dest, xmin, xmax, ymax, zProj, zFar, stereoSep);
}

/*
 * R_SetupProjectionZ
 *
 * Sets the z-component transformation part in the projection matrix
 */
void
R_SetupProjectionZ(viewParms_t *dest)
{
	float zNear, zFar, depth;

	zNear	= r_znear->value;
	zFar	= dest->zFar;

	depth = zFar - zNear;

	dest->projectionMatrix[2]	= 0;
	dest->projectionMatrix[6]	= 0;
	dest->projectionMatrix[10]	= -(zFar + zNear) / depth;
	dest->projectionMatrix[14]	= -2 * zFar * zNear / depth;

	if(dest->isPortal){
		float	plane[4];
		float	plane2[4];
		vec4_t	q, c;

		/* transform portal plane into camera space */
		plane[0]	= dest->portalPlane.normal[0];
		plane[1]	= dest->portalPlane.normal[1];
		plane[2]	= dest->portalPlane.normal[2];
		plane[3]	= dest->portalPlane.dist;

		plane2[0]	= -Vec3Dot (dest->or.axis[1], plane);
		plane2[1]	= Vec3Dot (dest->or.axis[2], plane);
		plane2[2]	= -Vec3Dot (dest->or.axis[0], plane);
		plane2[3]	= Vec3Dot (plane, dest->or.origin) - plane[3];

		/* Lengyel, Eric. Modifying the Projection Matrix to Perform Oblique Near-plane Clipping.
		 * Terathon Software 3D Graphics Library, 2004. http://www.terathon.com/code/oblique.html */
		q[0]	= (SGN(plane2[0]) + dest->projectionMatrix[8]) / dest->projectionMatrix[0];
		q[1]	= (SGN(plane2[1]) + dest->projectionMatrix[9]) / dest->projectionMatrix[5];
		q[2]	= -1.0f;
		q[3]	= (1.0f + dest->projectionMatrix[10]) / dest->projectionMatrix[14];

		VectorScale4(plane2, 2.0f / Vec3Dot4(plane2, q), c);

		dest->projectionMatrix[2]	= c[0];
		dest->projectionMatrix[6]	= c[1];
		dest->projectionMatrix[10]	= c[2] + 1.0f;
		dest->projectionMatrix[14]	= c[3];

	}

}

/*
 * R_MirrorPoint
 */
void
R_MirrorPoint(vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	Vec3Sub(in, surface->origin, local);

	VectorClear(transformed);
	for(i = 0; i < 3; i++){
		d = Vec3Dot(local, surface->axis[i]);
		Vec3MA(transformed, d, camera->axis[i], transformed);
	}

	Vec3Add(transformed, camera->origin, out);
}

void
R_MirrorVector(vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int i;
	float d;

	VectorClear(out);
	for(i = 0; i < 3; i++){
		d = Vec3Dot(in, surface->axis[i]);
		Vec3MA(out, d, camera->axis[i], out);
	}
}


/*
 * R_PlaneForSurface
 */
void
R_PlaneForSurface(surfaceType_t *surfType, cplane_t *plane)
{
	srfTriangles_t *tri;
	srfPoly_t	*poly;
	srfVert_t	*v1, *v2, *v3;
	vec4_t		plane4;

	if(!surfType){
		Q_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch(*surfType){
	case SF_FACE:
		*plane = ((srfSurfaceFace_t*)surfType)->plane;
		return;
	case SF_TRIANGLES:
		tri	= (srfTriangles_t*)surfType;
		v1	= tri->verts + tri->triangles[0].indexes[0];
		v2	= tri->verts + tri->triangles[0].indexes[1];
		v3	= tri->verts + tri->triangles[0].indexes[2];
		PlaneFromPoints(plane4, v1->xyz, v2->xyz, v3->xyz);
		Vec3Copy(plane4, plane->normal);
		plane->dist = plane4[3];
		return;
	case SF_POLY:
		poly = (srfPoly_t*)surfType;
		PlaneFromPoints(plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz);
		Vec3Copy(plane4, plane->normal);
		plane->dist = plane4[3];
		return;
	default:
		Q_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
}

/*
 * R_GetPortalOrientation
 *
 * entityNum is the entity that the portal surface is a part of, which may
 * be moving and rotating.
 *
 * Returns qtrue if it should be mirrored
 */
qbool
R_GetPortalOrientations(drawSurf_t *drawSurf, int entityNum,
			orientation_t *surface, orientation_t *camera,
			vec3_t pvsOrigin, qbool *mirror)
{
	int i;
	cplane_t	originalPlane, plane;
	trRefEntity_t *e;
	float	d;
	vec3_t	transformed;

	/* create plane axis for the portal we are seeing */
	R_PlaneForSurface(drawSurf->surface, &originalPlane);

	/* rotate the plane if necessary */
	if(entityNum != ENTITYNUM_WORLD){
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		/* get the orientation of the entity */
		R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.or);

		/* rotate the plane, but keep the non-rotated version for matching
		 * against the portalSurface entities */
		R_LocalNormalToWorld(originalPlane.normal, plane.normal);
		plane.dist = originalPlane.dist + Vec3Dot(plane.normal, tr.or.origin);

		/* translate the original plane */
		originalPlane.dist = originalPlane.dist + Vec3Dot(originalPlane.normal, tr.or.origin);
	}else{
		plane = originalPlane;
	}

	Vec3Copy(plane.normal, surface->axis[0]);
	PerpendicularVector(surface->axis[1], surface->axis[0]);
	Vec3Cross(surface->axis[0], surface->axis[1], surface->axis[2]);

	/* locate the portal entity closest to this plane.
	 * origin will be the origin of the portal, origin2 will be
	 * the origin of the camera */
	for(i = 0; i < tr.refdef.num_entities; i++){
		e = &tr.refdef.entities[i];
		if(e->e.reType != RT_PORTALSURFACE){
			continue;
		}

		d = Vec3Dot(e->e.origin, originalPlane.normal) - originalPlane.dist;
		if(d > 64 || d < -64){
			continue;
		}

		/* get the pvsOrigin from the entity */
		Vec3Copy(e->e.oldorigin, pvsOrigin);

		/* if the entity is just a mirror, don't use as a camera point */
		if(e->e.oldorigin[0] == e->e.origin[0] &&
		   e->e.oldorigin[1] == e->e.origin[1] &&
		   e->e.oldorigin[2] == e->e.origin[2]){
			VectorScale(plane.normal, plane.dist, surface->origin);
			Vec3Copy(surface->origin, camera->origin);
			Vec3Sub(vec3_origin, surface->axis[0], camera->axis[0]);
			Vec3Copy(surface->axis[1], camera->axis[1]);
			Vec3Copy(surface->axis[2], camera->axis[2]);

			*mirror = qtrue;
			return qtrue;
		}

		/* project the origin onto the surface plane to get
		 * an origin point we can rotate around */
		d = Vec3Dot(e->e.origin, plane.normal) - plane.dist;
		Vec3MA(e->e.origin, -d, surface->axis[0], surface->origin);

		/* now get the camera origin and orientation */
		Vec3Copy(e->e.oldorigin, camera->origin);
		AxisCopy(e->e.axis, camera->axis);
		Vec3Sub(vec3_origin, camera->axis[0], camera->axis[0]);
		Vec3Sub(vec3_origin, camera->axis[1], camera->axis[1]);

		/* optionally rotate */
		if(e->e.oldframe){
			/* if a speed is specified */
			if(e->e.frame){
				/* continuous rotate */
				d = (tr.refdef.time/1000.0f) * e->e.frame;
				Vec3Copy(camera->axis[1], transformed);
				RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
				Vec3Cross(camera->axis[0], camera->axis[1], camera->axis[2]);
			}else{
				/* bobbing rotate, with skinNum being the rotation offset */
				d	= sin(tr.refdef.time * 0.003f);
				d	= e->e.skinNum + d * 4;
				Vec3Copy(camera->axis[1], transformed);
				RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
				Vec3Cross(camera->axis[0], camera->axis[1], camera->axis[2]);
			}
		}else if(e->e.skinNum){
			d = e->e.skinNum;
			Vec3Copy(camera->axis[1], transformed);
			RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
			Vec3Cross(camera->axis[0], camera->axis[1], camera->axis[2]);
		}
		*mirror = qfalse;
		return qtrue;
	}

	/* if we didn't locate a portal entity, don't render anything.
	 * We don't want to just treat it as a mirror, because without a
	 * portal entity the server won't have communicated a proper entity set
	 * in the snapshot */

	/* unfortunately, with local movement prediction it is easily possible
	 * to see a surface before the server has communicated the matching
	 * portal surface entity, so we don't want to print anything here... */

	/* ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" ); */

	return qfalse;
}

static qbool
IsMirror(const drawSurf_t *drawSurf, int entityNum)
{
	int i;
	cplane_t	originalPlane, plane;
	trRefEntity_t *e;
	float		d;

	/* create plane axis for the portal we are seeing */
	R_PlaneForSurface(drawSurf->surface, &originalPlane);

	/* rotate the plane if necessary */
	if(entityNum != ENTITYNUM_WORLD){
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		/* get the orientation of the entity */
		R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.or);

		/* rotate the plane, but keep the non-rotated version for matching
		 * against the portalSurface entities */
		R_LocalNormalToWorld(originalPlane.normal, plane.normal);
		plane.dist = originalPlane.dist + Vec3Dot(plane.normal, tr.or.origin);

		/* translate the original plane */
		originalPlane.dist = originalPlane.dist + Vec3Dot(originalPlane.normal, tr.or.origin);
	}else{
		plane = originalPlane;
	}

	/* locate the portal entity closest to this plane.
	 * origin will be the origin of the portal, origin2 will be
	 * the origin of the camera */
	for(i = 0; i < tr.refdef.num_entities; i++){
		e = &tr.refdef.entities[i];
		if(e->e.reType != RT_PORTALSURFACE){
			continue;
		}

		d = Vec3Dot(e->e.origin, originalPlane.normal) - originalPlane.dist;
		if(d > 64 || d < -64){
			continue;
		}

		/* if the entity is just a mirror, don't use as a camera point */
		if(e->e.oldorigin[0] == e->e.origin[0] &&
		   e->e.oldorigin[1] == e->e.origin[1] &&
		   e->e.oldorigin[2] == e->e.origin[2]){
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qbool
SurfIsOffscreen(const drawSurf_t *drawSurf, vec4_t clipDest[128])
{
	float	shortest = 100000000;
	int	entityNum;
	int	numTriangles;
	material_t *shader;
	int fogNum;
	int dlighted;
	int pshadowed;
	vec4_t clip, eye;
	int i;
	unsigned int	pointOr		= 0;
	unsigned int	pointAnd	= (unsigned int)~0;

	UNUSED(clipDest);
	if(glConfig.smpActive){	/* FIXME!  we can't do RB_BeginSurface/RB_EndSurface stuff with smp! */
		return qfalse;
	}

	R_RotateForViewer();

	R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed);
	RB_BeginSurface(shader, fogNum);
	rb_surfaceTable[ *drawSurf->surface ](drawSurf->surface);

	assert(tess.numVertexes < 128);

	for(i = 0; i < tess.numVertexes; i++){
		int j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip(tess.xyz[i], tr.or.modelMatrix, tr.viewParms.projectionMatrix, eye,
			clip);

		for(j = 0; j < 3; j++){
			if(clip[j] >= clip[3]){
				pointFlags |= (1 << (j*2));
			}else if(clip[j] <= -clip[3]){
				pointFlags |= (1 << (j*2+1));
			}
		}
		pointAnd	&= pointFlags;
		pointOr		|= pointFlags;
	}

	/* trivially reject */
	if(pointAnd){
		return qtrue;
	}

	/* determine if this surface is backfaced and also determine the distance
	 * to the nearest vertex so we can cull based on portal range.  Culling
	 * based on vertex distance isn't 100% correct (we should be checking for
	 * range to the surface), but it's good enough for the types of portals
	 * we have in the game right now. */
	numTriangles = tess.numIndexes / 3;

	for(i = 0; i < tess.numIndexes; i += 3){
		vec3_t	normal;
		float	len;

		Vec3Sub(tess.xyz[tess.indexes[i]], tr.viewParms.or.origin, normal);

		len = Vec3LenSquared(normal);	/* lose the sqrt */
		if(len < shortest){
			shortest = len;
		}

		if(Vec3Dot(normal, tess.normal[tess.indexes[i]]) >= 0){
			numTriangles--;
		}
	}
	if(!numTriangles){
		return qtrue;
	}

	/* mirrors can early out at this point, since we don't do a fade over distance
	 * with them (although we could) */
	if(IsMirror(drawSurf, entityNum)){
		return qfalse;
	}

	if(shortest > (tess.shader->portalRange*tess.shader->portalRange)){
		return qtrue;
	}

	return qfalse;
}

/*
 * R_MirrorViewBySurface
 *
 * Returns qtrue if another view has been rendered
 */
qbool
R_MirrorViewBySurface(drawSurf_t *drawSurf, int entityNum)
{
	vec4_t clipDest[128];
	viewParms_t	newParms;
	viewParms_t	oldParms;
	orientation_t surface, camera;

	/* don't recursively mirror */
	if(tr.viewParms.isPortal){
		ri.Printf(PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n");
		return qfalse;
	}

	if(r_noportals->integer || (r_fastsky->integer == 1)){
		return qfalse;
	}

	/* trivially reject portal/mirror */
	if(SurfIsOffscreen(drawSurf, clipDest)){
		return qfalse;
	}

	/* save old viewParms so we can return to it after the mirror view */
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;
	if(!R_GetPortalOrientations(drawSurf, entityNum, &surface, &camera,
		   newParms.pvsOrigin, &newParms.isMirror)){
		return qfalse;	/* bad portal, no portalentity */
	}

	R_MirrorPoint (oldParms.or.origin, &surface, &camera, newParms.or.origin);

	Vec3Sub(vec3_origin, camera.axis[0], newParms.portalPlane.normal);
	newParms.portalPlane.dist = Vec3Dot(camera.origin, newParms.portalPlane.normal);

	R_MirrorVector (oldParms.or.axis[0], &surface, &camera, newParms.or.axis[0]);
	R_MirrorVector (oldParms.or.axis[1], &surface, &camera, newParms.or.axis[1]);
	R_MirrorVector (oldParms.or.axis[2], &surface, &camera, newParms.or.axis[2]);

	/* OPTIMIZE: restrict the viewport on the mirrored view */

	/* render the mirror view */
	R_RenderView (&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

/*
 * R_SpriteFogNum
 *
 * See if a sprite is inside a fog volume
 */
int
R_SpriteFogNum(trRefEntity_t *ent)
{
	int i, j;
	fog_t *fog;

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL){
		return 0;
	}

	for(i = 1; i < tr.world->numfogs; i++){
		fog = &tr.world->fogs[i];
		for(j = 0; j < 3; j++){
			if(ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j]){
				break;
			}
			if(ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j]){
				break;
			}
		}
		if(j == 3){
			return i;
		}
	}

	return 0;
}

/*
 *
 * DRAWSURF SORTING
 *
 */

/*
 * R_Radix
 */
static ID_INLINE void
R_Radix(int byte, int size, drawSurf_t *source, drawSurf_t *dest)
{
	int	count[ 256 ] = { 0 };
	int	index[ 256 ];
	int	i;
	unsigned char	*sortKey = NULL;
	unsigned char	*end = NULL;

	sortKey = ((unsigned char*)&source[ 0 ].sort) + byte;
	end = sortKey + (size * sizeof(drawSurf_t));
	for(; sortKey < end; sortKey += sizeof(drawSurf_t))
		++count[ *sortKey ];

	index[ 0 ] = 0;

	for(i = 1; i < 256; ++i)
		index[ i ] = index[ i - 1 ] + count[ i - 1 ];

	sortKey = ((unsigned char*)&source[ 0 ].sort) + byte;
	for(i = 0; i < size; ++i, sortKey += sizeof(drawSurf_t))
		dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
 * R_RadixSort
 *
 * Radix sort with 4 byte size buckets
 */
static void
R_RadixSort(drawSurf_t *source, int size)
{
	static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
	R_Radix(0, size, source, scratch);
	R_Radix(1, size, scratch, source);
	R_Radix(2, size, source, scratch);
	R_Radix(3, size, scratch, source);
#else
	R_Radix(3, size, source, scratch);
	R_Radix(2, size, scratch, source);
	R_Radix(1, size, source, scratch);
	R_Radix(0, size, scratch, source);
#endif	/* Q3_LITTLE_ENDIAN */
}

/* ========================================================================================== */

/*
 * R_AddDrawSurf
 */
void
R_AddDrawSurf(surfaceType_t *surface, material_t *shader,
	      int fogIndex, int dlightMap, int pshadowMap)
{
	int index;

	/* instead of checking for overflow, we just mask the index
	 * so it wraps around */
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	/* the sort data is packed into a single 32 bit value so it can be
	 * compared quickly during the qsorting process */
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT)
					  | tr.shiftedEntityNum | (fogIndex << QSORT_FOGNUM_SHIFT)
					  | ((int)pshadowMap << QSORT_PSHADOW_SHIFT) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

/*
 * R_DecomposeSort
 */
void
R_DecomposeSort(unsigned sort, int *entityNum, material_t **shader,
		int *fogNum, int *dlightMap, int *pshadowMap)
{
	*fogNum = (sort >> QSORT_FOGNUM_SHIFT) & 31;
	*shader = tr.sortedShaders[ (sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS-1) ];
	*entityNum	= (sort >> QSORT_ENTITYNUM_SHIFT) & MAX_ENTITIES;
	*pshadowMap	= (sort & 2) >> 1;
	*dlightMap	= sort & 1;
}

/*
 * R_SortDrawSurfs
 */
void
R_SortDrawSurfs(drawSurf_t *drawSurfs, int numDrawSurfs)
{
	material_t	*shader;
	int	fogNum;
	int	entityNum;
	int	dlighted;
	int	pshadowed;
	int	i;

	/* it is possible for some views to not have any surfaces */
	if(numDrawSurfs < 1){
		/* we still need to add it for hyperspace cases */
		R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
		return;
	}

	/* if we overflowed MAX_DRAWSURFS, the drawsurfs
	 * wrapped around in the buffer and we will be missing
	 * the first surfaces, not the last ones */
	if(numDrawSurfs > MAX_DRAWSURFS){
		numDrawSurfs = MAX_DRAWSURFS;
	}

	/* sort the drawsurfs by sort type, then orientation, then shader */
	R_RadixSort(drawSurfs, numDrawSurfs);

	if(tr.viewParms.isShadowmap){
		R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
		return;
	}

	/* check for any pass through drawing, which
	 * may cause another view to be rendered first */
	for(i = 0; i < numDrawSurfs; i++){
		R_DecomposeSort((drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed);

		if(shader->sort > SS_PORTAL){
			break;
		}

		/* no shader should ever have this sort type */
		if(shader->sort == SS_BAD){
			ri.Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name);
		}

		/* if the mirror was completely clipped away, we may need to check another surface */
		if(R_MirrorViewBySurface((drawSurfs+i), entityNum)){
			/* this is a debug option to see exactly what is being mirrored */
			if(r_portalOnly->integer){
				return;
			}
			break;	/* only one mirror view at a time */
		}
	}

	R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
}

static void
R_AddEntitySurface(int entityNum)
{
	trRefEntity_t *ent;
	material_t *shader;

	tr.currentEntityNum = entityNum;

	ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

	ent->needDlights = qfalse;

	/* preshift the value we are going to OR into the drawsurf sort */
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	/*
	 * the weapon model must be handled special --
	 * we don't want the hacked weapon position showing in
	 * mirrors, because the true body position will already be drawn
	 *  */
	if((ent->e.renderfx & RF_FIRST_PERSON) && (tr.viewParms.isPortal || tr.viewParms.isShadowmap)){
		return;
	}

	/* simple generated models, like sprites and beams, are not culled */
	switch(ent->e.reType){
	case RT_PORTALSURFACE:
		break;	/* don't draw anything */
	case RT_SPRITE:
	case RT_BEAM:
	case RT_LIGHTNING:
	case RT_RAIL_CORE:
	case RT_RAIL_RINGS:
		/* self blood sprites, talk balloons, etc should not be drawn in the primary
		 * view.  We can't just do this check for all entities, because md3
		 * entities may still want to cast shadows from them */
		if((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal){
			return;
		}
		shader = R_GetShaderByHandle(ent->e.customShader);
		R_AddDrawSurf(&entitySurface, shader, R_SpriteFogNum(ent), 0, 0);
		break;

	case RT_MODEL:
		/* we must set up parts of tr.or for model culling */
		R_RotateForEntity(ent, &tr.viewParms, &tr.or);

		tr.currentModel = R_GetModelByHandle(ent->e.hModel);
		if(!tr.currentModel){
			R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0, 0);
		}else{
			switch(tr.currentModel->type){
			case MOD_MESH:
				R_AddMD3Surfaces(ent);
				break;
			case MOD_MD4:
				R_AddAnimSurfaces(ent);
				break;
			case MOD_IQM:
				R_AddIQMSurfaces(ent);
				break;
			case MOD_BRUSH:
				R_AddBrushModelSurfaces(ent);
				break;
			case MOD_BAD:	/* null model axis */
				if((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal){
					break;
				}
				R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0, 0);
				break;
			default:
				ri.Error(ERR_DROP, "R_AddEntitySurfaces: Bad modeltype");
				break;
			}
		}
		break;
	default:
		ri.Error(ERR_DROP, "R_AddEntitySurfaces: Bad reType");
	}
}

/*
 * R_AddEntitySurfaces
 */
void
R_AddEntitySurfaces(void)
{
	int i;

	if(!r_drawentities->integer){
		return;
	}

	for(i = 0; i < tr.refdef.num_entities; i++)
		R_AddEntitySurface(i);
}


/*
 * R_GenerateDrawSurfs
 */
void
R_GenerateDrawSurfs(void)
{
	R_AddWorldSurfaces ();

	R_AddPolygonSurfaces();

	/* set the projection matrix with the minimum zfar
	 * now that we have the world bounded
	 * this needs to be done before entities are
	 * added, because they use the projection
	 * matrix for lod calculation */

	/* dynamically compute far clip plane distance */
	if(!tr.viewParms.isShadowmap){
		R_SetFarClip();
	}

	/* we know the size of the clipping volume. Now set the rest of the projection matrix. */
	R_SetupProjectionZ (&tr.viewParms);

	R_AddEntitySurfaces ();
}

/*
 * R_DebugPolygon
 */
void
R_DebugPolygon(int color, int numPoints, float *points)
{
	/* FIXME: implement this */
	int i;

if(0){
	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	/* draw solid shade */

	qglColor3f(color&1, (color>>1)&1, (color>>2)&1);
	qglBegin(GL_POLYGON);
	for(i = 0; i < numPoints; i++)
		qglVertex3fv(points + i * 3);
	qglEnd();

	/* draw wireframe outline */
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	qglDepthRange(0, 0);
	qglColor3f(1, 1, 1);
	qglBegin(GL_POLYGON);
	for(i = 0; i < numPoints; i++)
		qglVertex3fv(points + i * 3);
	qglEnd();
	qglDepthRange(0, 1);
}
}

/*
 * R_DebugGraphics
 *
 * Visualization aid for movement clipping debugging
 */
void
R_DebugGraphics(void)
{
	if(!r_debugSurface->integer){
		return;
	}

	/* the render thread can't make callbacks to the main thread */
	R_SyncRenderThread();

	GL_Bind(tr.whiteImage);
	GL_Cull(CT_FRONT_SIDED);
	ri.CM_DrawDebugSurface(R_DebugPolygon);
}


/*
 * R_RenderView
 *
 * A view may be either the actual camera view,
 * or a mirror / remote location
 */
void
R_RenderView(viewParms_t *parms)
{
	int firstDrawSurf;

	if(parms->viewportWidth <= 0 || parms->viewportHeight <= 0){
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	/* set viewParms.world */
	R_RotateForViewer ();

	R_SetupProjection(&tr.viewParms, r_zproj->value, tr.viewParms.zFar, qtrue);

	R_GenerateDrawSurfs();

	R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf);

	/* draw main system development information (surface outlines, etc) */
	R_DebugGraphics();
}


void
R_RenderDlightCubemaps(const refdef_t *fd)
{
	int i;

	UNUSED(fd);
	for(i = 0; i < tr.refdef.num_dlights; i++){
		viewParms_t shadowParms;
		int j;

		/* use previous frame to determine visible dlights */
		if((1 << i) & tr.refdef.dlightMask)
			continue;

		Q_Memset(&shadowParms, 0, sizeof(shadowParms));

		shadowParms.viewportX	= tr.refdef.x;
		shadowParms.viewportY	= glConfig.vidHeight - (tr.refdef.y + 256);
		shadowParms.viewportWidth	= 256;
		shadowParms.viewportHeight	= 256;
		shadowParms.isPortal	= qfalse;
		shadowParms.isMirror	= qtrue;	/* because it is */

		shadowParms.fovX	= 90;
		shadowParms.fovY	= 90;

		shadowParms.isShadowmap = qtrue;
		shadowParms.zFar = tr.refdef.dlights[i].radius;

		Vec3Copy(tr.refdef.dlights[i].origin, shadowParms.or.origin);

		for(j = 0; j < 6; j++){
			switch(j){
			case 0:
				/* -X */
				VectorSet(shadowParms.or.axis[0], -1,  0,  0);
				VectorSet(shadowParms.or.axis[1],  0,  0, -1);
				VectorSet(shadowParms.or.axis[2],  0,  1,  0);
				break;
			case 1:
				/* +X */
				VectorSet(shadowParms.or.axis[0],  1,  0,  0);
				VectorSet(shadowParms.or.axis[1],  0,  0,  1);
				VectorSet(shadowParms.or.axis[2],  0,  1,  0);
				break;
			case 2:
				/* -Y */
				VectorSet(shadowParms.or.axis[0],  0, -1,  0);
				VectorSet(shadowParms.or.axis[1],  1,  0,  0);
				VectorSet(shadowParms.or.axis[2],  0,  0, -1);
				break;
			case 3:
				/* +Y */
				VectorSet(shadowParms.or.axis[0],  0,  1,  0);
				VectorSet(shadowParms.or.axis[1],  1,  0,  0);
				VectorSet(shadowParms.or.axis[2],  0,  0,  1);
				break;
			case 4:
				/* -Z */
				VectorSet(shadowParms.or.axis[0],  0,  0, -1);
				VectorSet(shadowParms.or.axis[1],  1,  0,  0);
				VectorSet(shadowParms.or.axis[2],  0,  1,  0);
				break;
			case 5:
				/* +Z */
				VectorSet(shadowParms.or.axis[0],  0,  0,  1);
				VectorSet(shadowParms.or.axis[1], -1,  0,  0);
				VectorSet(shadowParms.or.axis[2],  0,  1,  0);
				break;
			}

			R_RenderView(&shadowParms);
			R_AddCapShadowmapCmd(i, j);
		}
	}
}


void
R_RenderPshadowMaps(const refdef_t *fd)
{
	viewParms_t shadowParms;
	int i;

	/* first, make a list of shadows */
	for(i = 0; i < tr.refdef.num_entities; i++){
		trRefEntity_t *ent = &tr.refdef.entities[i];

		if((ent->e.renderfx & (RF_FIRST_PERSON | RF_NOSHADOW)))
			continue;

		/* if((ent->e.renderfx & RF_THIRD_PERSON))
		 * continue; */

		if(ent->e.reType == RT_MODEL){
			model_t *model = R_GetModelByHandle(ent->e.hModel);
			pshadow_t shadow;
			float	radius	= 0.0f;
			float	scale	= 1.0f;
			vec3_t	diff;
			int j;

			if(!model)
				continue;

			if(ent->e.nonNormalizedAxes){
				scale = Vec3Len(ent->e.axis[0]);
			}

			switch(model->type){
			case MOD_MESH:
			{
				mdvFrame_t *frame = &model->mdv[0]->frames[ent->e.frame];

				radius = frame->radius * scale;
			}
			break;

			case MOD_MD4:
			{
				/* FIXME: actually calculate the radius and bounds, this is a horrible hack */
				radius = r_pshadowDist->value / 2.0f;
			}
			break;
			case MOD_IQM:
			{
				/* FIXME: never actually tested this */
				iqmData_t	*data = model->modelData;
				vec3_t	diag;
				float	*framebounds;

				framebounds = data->bounds + 6*ent->e.frame;
				Vec3Sub(framebounds+3, framebounds, diag);
				radius = 0.5f * Vec3Len(diag);
			}
			break;

			default:
				break;
			}

			if(!radius)
				continue;

			/* Cull entities that are behind the viewer by more than lightRadius */
			Vec3Sub(ent->e.origin, fd->vieworg, diff);
			if(Vec3Dot(diff, fd->viewaxis[0]) < -r_pshadowDist->value)
				continue;

			memset(&shadow, 0, sizeof(shadow));

			shadow.numEntities	= 1;
			shadow.entityNums[0]	= i;
			shadow.viewRadius	= radius;
			shadow.lightRadius	= r_pshadowDist->value;
			Vec3Copy(ent->e.origin, shadow.viewOrigin);
			shadow.sort = Vec3Dot(diff, diff) / (radius * radius);
			Vec3Copy(ent->e.origin, shadow.entityOrigins[0]);
			shadow.entityRadiuses[0] = radius;

			for(j = 0; j < MAX_CALC_PSHADOWS; j++){
				pshadow_t swap;

				if(j + 1 > tr.refdef.num_pshadows){
					tr.refdef.num_pshadows	= j + 1;
					tr.refdef.pshadows[j]	= shadow;
					break;
				}

				/* sort shadows by distance from camera divided by radius
				 * FIXME: sort better */
				if(tr.refdef.pshadows[j].sort <= shadow.sort)
					continue;

				swap = tr.refdef.pshadows[j];
				tr.refdef.pshadows[j] = shadow;
				shadow = swap;
			}
		}
	}

	/* next, merge touching pshadows */
	for(i = 0; i < tr.refdef.num_pshadows; i++){
		pshadow_t *ps1 = &tr.refdef.pshadows[i];
		int j;

		for(j = i + 1; j < tr.refdef.num_pshadows; j++){
			pshadow_t	*ps2 = &tr.refdef.pshadows[j];
			int k;
			qbool		touch;

			if(ps1->numEntities == 8)
				break;

			touch = qfalse;
			if(SpheresIntersect(ps1->viewOrigin, ps1->viewRadius, ps2->viewOrigin,
				   ps2->viewRadius)){
				for(k = 0; k < ps1->numEntities; k++)
					if(SpheresIntersect(ps1->entityOrigins[k], ps1->entityRadiuses[k],
						   ps2->viewOrigin, ps2->viewRadius)){
						touch = qtrue;
						break;
					}
			}

			if(touch){
				vec3_t	newOrigin;
				float	newRadius;

				BoundingSphereOfSpheres(ps1->viewOrigin, ps1->viewRadius, ps2->viewOrigin,
					ps2->viewRadius, newOrigin,
					&newRadius);
				Vec3Copy(newOrigin, ps1->viewOrigin);
				ps1->viewRadius = newRadius;

				ps1->entityNums[ps1->numEntities] = ps2->entityNums[0];
				Vec3Copy(ps2->viewOrigin, ps1->entityOrigins[ps1->numEntities]);
				ps1->entityRadiuses[ps1->numEntities] = ps2->viewRadius;

				ps1->numEntities++;

				for(k = j; k < tr.refdef.num_pshadows - 1; k++)
					tr.refdef.pshadows[k] = tr.refdef.pshadows[k + 1];

				j--;
				tr.refdef.num_pshadows--;
			}
		}
	}

	/* cap number of drawn pshadows */
	if(tr.refdef.num_pshadows > MAX_DRAWN_PSHADOWS){
		tr.refdef.num_pshadows = MAX_DRAWN_PSHADOWS;
	}

	/* next, fill up the rest of the shadow info */
	for(i = 0; i < tr.refdef.num_pshadows; i++){
		pshadow_t	*shadow = &tr.refdef.pshadows[i];
		vec3_t	up;
		vec3_t	ambientLight, directedLight, lightDir;

		VectorSet(lightDir, 0.57735f, 0.57735f, 0.57735f);
#if 1
		R_LightForPoint(shadow->viewOrigin, ambientLight, directedLight, lightDir);

		/* sometimes there's no light */
		if(Vec3Dot(lightDir, lightDir) < 0.9f)
			VectorSet(lightDir, 0.0f, 0.0f, 1.0f);
#endif

		if(shadow->viewRadius * 3.0f > shadow->lightRadius){
			shadow->lightRadius = shadow->viewRadius * 3.0f;
		}

		Vec3MA(shadow->viewOrigin, shadow->viewRadius, lightDir, shadow->lightOrigin);

		/* make up a projection, up doesn't matter */
		VectorScale(lightDir, -1.0f, shadow->lightViewAxis[0]);
		VectorSet(up, 0, 0, -1);

		if(abs(Vec3Dot(up, shadow->lightViewAxis[0])) > 0.9f){
			VectorSet(up, -1, 0, 0);
		}

		Vec3Cross(shadow->lightViewAxis[0], up, shadow->lightViewAxis[1]);
		Vec3Normalize(shadow->lightViewAxis[1]);
		Vec3Cross(shadow->lightViewAxis[0], shadow->lightViewAxis[1], shadow->lightViewAxis[2]);

		Vec3Copy(shadow->lightViewAxis[0], shadow->cullPlane.normal);
		shadow->cullPlane.dist	= Vec3Dot(shadow->cullPlane.normal, shadow->lightOrigin);
		shadow->cullPlane.type	= PLANE_NON_AXIAL;
		SetPlaneSignbits(&shadow->cullPlane);
	}

	/* next, render shadowmaps */
	for(i = 0; i < tr.refdef.num_pshadows; i++){
		int firstDrawSurf;
		pshadow_t *shadow = &tr.refdef.pshadows[i];
		int j;

		Q_Memset(&shadowParms, 0, sizeof(shadowParms));

		if(glRefConfig.framebufferObject){
			shadowParms.viewportX	= 0;
			shadowParms.viewportY	= 0;
		}else{
			shadowParms.viewportX	= tr.refdef.x;
			shadowParms.viewportY	= glConfig.vidHeight - (tr.refdef.y + 256);
		}
		shadowParms.viewportWidth	= 256;
		shadowParms.viewportHeight	= 256;
		shadowParms.isPortal	= qfalse;
		shadowParms.isMirror	= qfalse;

		shadowParms.fovX	= 90;
		shadowParms.fovY	= 90;

		if(glRefConfig.framebufferObject)
			shadowParms.targetFbo = tr.pshadowFbos[i];

		shadowParms.isShadowmap = qtrue;

		shadowParms.zFar = shadow->lightRadius;

		Vec3Copy(shadow->lightOrigin, shadowParms.or.origin);

		Vec3Copy(shadow->lightViewAxis[0], shadowParms.or.axis[0]);
		Vec3Copy(shadow->lightViewAxis[1], shadowParms.or.axis[1]);
		Vec3Copy(shadow->lightViewAxis[2], shadowParms.or.axis[2]);

		{
			tr.viewCount++;

			tr.viewParms = shadowParms;
			tr.viewParms.frameSceneNum = tr.frameSceneNum;
			tr.viewParms.frameCount = tr.frameCount;

			firstDrawSurf = tr.refdef.numDrawSurfs;

			tr.viewCount++;

			/* set viewParms.world */
			R_RotateForViewer ();

			{
				float	xmin, xmax, ymin, ymax, znear, zfar;
				viewParms_t *dest = &tr.viewParms;
				vec3_t	pop;

				xmin	= ymin = -shadow->viewRadius;
				xmax	= ymax = shadow->viewRadius;
				znear	= 0;
				zfar	= shadow->lightRadius;

				dest->projectionMatrix[0]	= 2 / (xmax - xmin);
				dest->projectionMatrix[4]	= 0;
				dest->projectionMatrix[8]	= (xmax + xmin) / (xmax - xmin);
				dest->projectionMatrix[12]	=0;

				dest->projectionMatrix[1]	= 0;
				dest->projectionMatrix[5]	= 2 / (ymax - ymin);
				dest->projectionMatrix[9]	= (ymax + ymin) / (ymax - ymin);	/* normally 0 */
				dest->projectionMatrix[13]	= 0;

				dest->projectionMatrix[2]	= 0;
				dest->projectionMatrix[6]	= 0;
				dest->projectionMatrix[10]	= 2 / (zfar - znear);
				dest->projectionMatrix[14]	= 0;

				dest->projectionMatrix[3]	= 0;
				dest->projectionMatrix[7]	= 0;
				dest->projectionMatrix[11]	= 0;
				dest->projectionMatrix[15]	= 1;

				VectorScale(dest->or.axis[1],  1.0f, dest->frustum[0].normal);
				Vec3MA(dest->or.origin, -shadow->viewRadius, dest->frustum[0].normal, pop);
				dest->frustum[0].dist = Vec3Dot(pop, dest->frustum[0].normal);

				VectorScale(dest->or.axis[1], -1.0f, dest->frustum[1].normal);
				Vec3MA(dest->or.origin, -shadow->viewRadius, dest->frustum[1].normal, pop);
				dest->frustum[1].dist = Vec3Dot(pop, dest->frustum[1].normal);

				VectorScale(dest->or.axis[2],  1.0f, dest->frustum[2].normal);
				Vec3MA(dest->or.origin, -shadow->viewRadius, dest->frustum[2].normal, pop);
				dest->frustum[2].dist = Vec3Dot(pop, dest->frustum[2].normal);

				VectorScale(dest->or.axis[2], -1.0f, dest->frustum[3].normal);
				Vec3MA(dest->or.origin, -shadow->viewRadius, dest->frustum[3].normal, pop);
				dest->frustum[3].dist = Vec3Dot(pop, dest->frustum[3].normal);

				VectorScale(dest->or.axis[0], -1.0f, dest->frustum[4].normal);
				Vec3MA(dest->or.origin, -shadow->lightRadius, dest->frustum[4].normal, pop);
				dest->frustum[4].dist = Vec3Dot(pop, dest->frustum[4].normal);

				for(j = 0; j < 5; j++){
					dest->frustum[j].type = PLANE_NON_AXIAL;
					SetPlaneSignbits (&dest->frustum[j]);
				}
			}

			for(j = 0; j < shadow->numEntities; j++)
				R_AddEntitySurface(shadow->entityNums[j]);

			R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf,
				tr.refdef.numDrawSurfs - firstDrawSurf);

			if(!glRefConfig.framebufferObject)
				R_AddCapShadowmapCmd(i, -1);
		}
	}
}
