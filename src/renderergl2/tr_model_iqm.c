/*
 * Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
 * Copyright (C) 2011 Matthias Bentrup <matthias.bentrup@googlemail.com>
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

#include "tr_local.h"

#define LL(x) x=LittleLong(x)

/* 
 * return true if the range specified by offset, count and size
* doesn't fit into the file
*/
static qbool
checkrange(iqmHeader_t *header, uint offset,
	uint count, size_t size)
{
	return (count <= 0 ||
		// offset < 0 ||
		offset > header->filesize ||
		// offset + count * size < 0 ||
		offset + count * size > header->filesize);
}

/* 
 * "multiply" 3x4 matrices, these are assumed to be the top 3 rows
 * of a 4x4 matrix with the last row = (0 0 0 1) 
 */
static void
Matrix34Multiply(float *a, float *b, float *out)
{
	out[ 0] = a[0] * b[0] + a[1] * b[4] + a[ 2] * b[ 8];
	out[ 1] = a[0] * b[1] + a[1] * b[5] + a[ 2] * b[ 9];
	out[ 2] = a[0] * b[2] + a[1] * b[6] + a[ 2] * b[10];
	out[ 3] = a[0] * b[3] + a[1] * b[7] + a[ 2] * b[11] + a[ 3];
	out[ 4] = a[4] * b[0] + a[5] * b[4] + a[ 6] * b[ 8];
	out[ 5] = a[4] * b[1] + a[5] * b[5] + a[ 6] * b[ 9];
	out[ 6] = a[4] * b[2] + a[5] * b[6] + a[ 6] * b[10];
	out[ 7] = a[4] * b[3] + a[5] * b[7] + a[ 6] * b[11] + a[ 7];
	out[ 8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[ 8];
	out[ 9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[ 9];
	out[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10];
	out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11];
}

static void
InterpolateMatrix(float *a, float *b, float lerp, float *mat)
{
	float unLerp = 1.0f - lerp;

	mat[ 0] = a[ 0] * unLerp + b[ 0] * lerp;
	mat[ 1] = a[ 1] * unLerp + b[ 1] * lerp;
	mat[ 2] = a[ 2] * unLerp + b[ 2] * lerp;
	mat[ 3] = a[ 3] * unLerp + b[ 3] * lerp;
	mat[ 4] = a[ 4] * unLerp + b[ 4] * lerp;
	mat[ 5] = a[ 5] * unLerp + b[ 5] * lerp;
	mat[ 6] = a[ 6] * unLerp + b[ 6] * lerp;
	mat[ 7] = a[ 7] * unLerp + b[ 7] * lerp;
	mat[ 8] = a[ 8] * unLerp + b[ 8] * lerp;
	mat[ 9] = a[ 9] * unLerp + b[ 9] * lerp;
	mat[10] = a[10] * unLerp + b[10] * lerp;
	mat[11] = a[11] * unLerp + b[11] * lerp;
}

static void
JointToMatrix(Vec4 rot, Vec3 scale, Vec3 trans,
	      float *mat)
{
	float xx = 2.0f * rot[0] * rot[0];
	float yy = 2.0f * rot[1] * rot[1];
	float zz = 2.0f * rot[2] * rot[2];
	float xy = 2.0f * rot[0] * rot[1];
	float xz = 2.0f * rot[0] * rot[2];
	float yz = 2.0f * rot[1] * rot[2];
	float wx = 2.0f * rot[3] * rot[0];
	float wy = 2.0f * rot[3] * rot[1];
	float wz = 2.0f * rot[3] * rot[2];

	mat[ 0] = scale[0] * (1.0f - (yy + zz));
	mat[ 1] = scale[0] * (xy - wz);
	mat[ 2] = scale[0] * (xz + wy);
	mat[ 3] = trans[0];
	mat[ 4] = scale[1] * (xy + wz);
	mat[ 5] = scale[1] * (1.0f - (xx + zz));
	mat[ 6] = scale[1] * (yz - wx);
	mat[ 7] = trans[1];
	mat[ 8] = scale[2] * (xz - wy);
	mat[ 9] = scale[2] * (yz + wx);
	mat[10] = scale[2] * (1.0f - (xx + yy));
	mat[11] = trans[2];
}

static void
Matrix34Invert(float *inMat, float *outMat)
{
	Vec3	trans;
	float	invSqrLen, *v;

	outMat[ 0] = inMat[ 0]; outMat[ 1] = inMat[ 4]; outMat[ 2] = inMat[ 8];
	outMat[ 4] = inMat[ 1]; outMat[ 5] = inMat[ 5]; outMat[ 6] = inMat[ 9];
	outMat[ 8] = inMat[ 2]; outMat[ 9] = inMat[ 6]; outMat[10] = inMat[10];

	v = outMat + 0; invSqrLen = 1.0f / vec3dot(v, v); vec3scale(v, invSqrLen, v);
	v = outMat + 4; invSqrLen = 1.0f / vec3dot(v, v); vec3scale(v, invSqrLen, v);
	v = outMat + 8; invSqrLen = 1.0f / vec3dot(v, v); vec3scale(v, invSqrLen, v);

	trans[0] = inMat[ 3];
	trans[1] = inMat[ 7];
	trans[2] = inMat[11];

	outMat[ 3] = -vec3dot(outMat + 0, trans);
	outMat[ 7] = -vec3dot(outMat + 4, trans);
	outMat[11] = -vec3dot(outMat + 8, trans);
}

/* byte order */
static void
swapheader(iqmHeader_t *p)
{
	LL(p->version);
	LL(p->filesize);
	LL(p->flags);
	LL(p->num_text);
	LL(p->ofs_text);
	LL(p->num_meshes);
	LL(p->ofs_meshes);
	LL(p->num_vertexarrays);
	LL(p->num_vertexes);
	LL(p->ofs_vertexarrays);
	LL(p->num_triangles);
	LL(p->ofs_triangles);
	LL(p->ofs_adjacency);
	LL(p->num_joints);
	LL(p->ofs_joints);
	LL(p->num_poses);
	LL(p->ofs_poses);
	LL(p->num_anims);
	LL(p->ofs_anims);
	LL(p->num_frames);
	LL(p->num_framechannels);
	LL(p->ofs_frames);
	LL(p->ofs_bounds);
	LL(p->num_comment);
	LL(p->ofs_comment);
	LL(p->num_extensions);
	LL(p->ofs_extensions);
}

static void
swaptri(iqmTriangle_t *p)
{
	LL(p->vertex[0]);
	LL(p->vertex[1]);
	LL(p->vertex[2]);
}

static void
swapmesh(iqmMesh_t *p)
{
	LL(p->name);
	LL(p->material);
	LL(p->first_vertex);
	LL(p->num_vertexes);
	LL(p->first_triangle);
	LL(p->num_triangles);
}

static void
swapanim(iqmAnim_t *p)
{
	LL(p->name);
	LL(p->first_frame);
	LL(p->num_frames);
	LL(p->framerate);
	LL(p->flags);
}

static void
swapjoint(iqmJoint_t *p)
{
	LL(p->name);
	LL(p->parent);
	LL(p->translate[0]);
	LL(p->translate[1]);
	LL(p->translate[2]);
	LL(p->rotate[0]);
	LL(p->rotate[1]);
	LL(p->rotate[2]);
	LL(p->rotate[3]);
	LL(p->scale[0]);
	LL(p->scale[1]);
	LL(p->scale[2]);
}

static void
swappose(iqmPose_t *p)
{
	LL(p->parent);
	LL(p->mask);
	LL(p->channeloffset[0]);
	LL(p->channeloffset[1]);
	LL(p->channeloffset[2]);
	LL(p->channeloffset[3]);
	LL(p->channeloffset[4]);
	LL(p->channeloffset[5]);
	LL(p->channeloffset[6]);
	LL(p->channeloffset[7]);
	LL(p->channeloffset[8]);
	LL(p->channeloffset[9]);
	LL(p->channelscale[0]);
	LL(p->channelscale[1]);
	LL(p->channelscale[2]);
	LL(p->channelscale[3]);
	LL(p->channelscale[4]);
	LL(p->channelscale[5]);
	LL(p->channelscale[6]);
	LL(p->channelscale[7]);
	LL(p->channelscale[8]);
	LL(p->channelscale[9]);
}

static void
swapbounds(iqmBounds_t *p)
{
	LL(p->bbmin[0]);
	LL(p->bbmin[1]);
	LL(p->bbmin[2]);
	LL(p->bbmax[0]);
	LL(p->bbmax[1]);
	LL(p->bbmax[2]);
}

/* sanity checks */
static qbool
sanetri(const iqmHeader_t *h, const iqmTriangle_t *t)
{
	if(t->vertex[0] > h->num_vertexes
	  || t->vertex[1] > h->num_vertexes
	  || t->vertex[2] > h->num_vertexes)
		return qfalse;
	return qtrue;
}

static qbool
sanemesh(const iqmHeader_t *h, const iqmMesh_t *m)
{
	if(m->first_vertex >= h->num_vertexes
	  || m->first_vertex + m->num_vertexes > h->num_vertexes
	  || m->first_triangle >= h->num_triangles
	  || m->first_triangle + m->num_triangles > h->num_triangles
	  || m->name >= h->num_text
	  || m->material >= h->num_text)
		return qfalse;
	return qtrue;
}

static qbool
saneanim(const iqmHeader_t *h, const iqmAnim_t *a)
{
	if(a->first_frame > (uint)h->num_frames
	  || a->num_frames > (uint)h->num_frames
	  || a->first_frame > a->num_frames
	  || a->first_frame + a->num_frames > h->num_frames)
		return qfalse;
	return qtrue;
}

static qbool
sanejoint(const iqmHeader_t *h, const iqmJoint_t *j)
{
	if(j->parent < -1 
	  || j->parent >= (int)h->num_joints
	  || j->name >= (uint)h->num_text)
		return qfalse;
	return qtrue;
}

/* Load an IQM model and compute the joint matrices for every frame. */
qbool
R_LoadIQM(model_t *mod, void *buffer, size_t filesize, const char *mod_name)
{
	iqmHeader_t *header;
	iqmVertexArray_t *vertexarray;
	iqmTriangle_t *triangle;
	iqmMesh_t *mesh;
	iqmAnim_t *anim;
	iqmJoint_t *joint;
	iqmPose_t *pose;
	iqmBounds_t *bounds;
	unsigned short *framedata;
	char *str;
	uint i, j;
	float jointMats[IQM_MAX_JOINTS * 2 * 12];
	float *mat;
	size_t size, joint_names;
	iqmData_t *iqmData;
	srfIQModel_t *surface;

	if(filesize < sizeof(iqmHeader_t)){
		return qfalse;
	}
	
	header = (iqmHeader_t*)buffer;
	swapheader(header);
	if(Q_strncmp(header->magic, IQM_MAGIC, sizeof(header->magic)))
		return qfalse;
	if(header->version != IQM_VERSION){
		ri.Printf(PRINT_WARNING,
			"R_LoadIQM: %s is a unsupported IQM version (%d), only version %d is supported.\n",
			mod_name, header->version, IQM_VERSION);
		return qfalse;
	}
	if(header->filesize > filesize || header->filesize > 16<<20)
		return qfalse;
		
	/* check ioq3 joint limit */
	if(header->num_joints > IQM_MAX_JOINTS){
		ri.Printf(PRINT_WARNING, "R_LoadIQM: %s has more than %d joints (%d).\n",
			mod_name, IQM_MAX_JOINTS, header->num_joints);
		return qfalse;
	}

	/* check and swap vertex arrays */
	if(checkrange(header, header->ofs_vertexarrays,
		   header->num_vertexarrays,
		   sizeof(iqmVertexArray_t))){
		return qfalse;
	}
	vertexarray = (iqmVertexArray_t*)((byte*)header + header->ofs_vertexarrays);
	for(i = 0; i < header->num_vertexarrays; i++, vertexarray++){
		int j, n, *intPtr;

		if(vertexarray->size <= 0 || vertexarray->size > 4){
			return qfalse;
		}

		/* total number of values */
		n = header->num_vertexes * vertexarray->size;

		switch(vertexarray->format){
		case IQM_BYTE:
		case IQM_UBYTE:
			/* 1 byte, no swapping necessary */
			if(checkrange(header, vertexarray->offset,
				   n, sizeof(byte))){
				return qfalse;
			}
			break;
		case IQM_INT:
		case IQM_UINT:
		case IQM_FLOAT:
			/* 4-byte swap */
			if(checkrange(header, vertexarray->offset,
				   n, sizeof(float))){
				return qfalse;
			}
			intPtr = (int*)((byte*)header + vertexarray->offset);
			for(j = 0; j < n; j++, intPtr++)
				LL(*intPtr);
			break;
		default:
			/* not supported */
			return qfalse;
			break;
		}

		switch(vertexarray->type){
		case IQM_POSITION:
		case IQM_NORMAL:
			if(vertexarray->format != IQM_FLOAT ||
			   vertexarray->size != 3){
				return qfalse;
			}
			break;
		case IQM_TANGENT:
			if(vertexarray->format != IQM_FLOAT ||
			   vertexarray->size != 4){
				return qfalse;
			}
			break;
		case IQM_TEXCOORD:
			if(vertexarray->format != IQM_FLOAT ||
			   vertexarray->size != 2){
				return qfalse;
			}
			break;
		case IQM_BLENDINDEXES:
		case IQM_BLENDWEIGHTS:
			if(vertexarray->format != IQM_UBYTE ||
			   vertexarray->size != 4){
				return qfalse;
			}
			break;
		case IQM_COLOR:
			if(vertexarray->format != IQM_UBYTE ||
			   vertexarray->size != 4){
				return qfalse;
			}
			break;
		}
	}

	/* check and swap triangles */
	if(checkrange(header, header->ofs_triangles,
		   header->num_triangles, sizeof(iqmTriangle_t))){
		return qfalse;
	}
	triangle = (iqmTriangle_t*)((byte*)header + header->ofs_triangles);
	for(i = 0; i < header->num_triangles; i++, triangle++){
		swaptri(triangle);		
		if(!sanetri(header, triangle))
			return qfalse;
	}

	/* check and swap meshes */
	if(checkrange(header, header->ofs_meshes,
		   header->num_meshes, sizeof(iqmMesh_t))){
		return qfalse;
	}
	mesh = (iqmMesh_t*)((byte*)header + header->ofs_meshes);
	for(i = 0; i < header->num_meshes; i++, mesh++){
		swapmesh(mesh);
		/* check ioq3 limits */ /* FIXME */
		if(mesh->num_vertexes > SHADER_MAX_VERTEXES){
			ri.Printf(PRINT_WARNING, "R_LoadIQM: %s has more than %i verts on a surface (%i).\n",
				mod_name, SHADER_MAX_VERTEXES, mesh->num_vertexes);
			return qfalse;
		}
		if(mesh->num_triangles*3 > SHADER_MAX_INDEXES){
			ri.Printf(PRINT_WARNING,
				"R_LoadIQM: %s has more than %i triangles on a surface (%i).\n",
				mod_name, SHADER_MAX_INDEXES / 3,
				mesh->num_triangles);
			return qfalse;
		}

		if(!sanemesh(header, mesh))
			return qfalse;
	}
	
	/* check and swap anims */
	if(checkrange(header, header->ofs_anims, header->num_anims
	  , sizeof(iqmAnim_t))){
		return qfalse;
	}
	anim = (iqmAnim_t*)((byte*)header + header->ofs_anims);
	for(i = 0; i < header->num_anims; i++, anim++){
		swapanim(anim);
		if(!saneanim(header, anim))
			return qfalse;
	}

	/* check and swap joints */
	if(checkrange(header, header->ofs_joints,
	  header->num_joints, sizeof(iqmJoint_t))){
		return qfalse;
	}
	joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
	joint_names = 0;
	for(i = 0; i < header->num_joints; i++, joint++){
		swapjoint(joint);
		if(!sanejoint(header, joint))
			return qfalse;
		joint_names += strlen((char*)header + header->ofs_text +
			joint->name) + 1;
	}

	/* check and swap poses */
	if(header->num_poses != header->num_joints){
		return qfalse;
	}
	if(checkrange(header, header->ofs_poses,
		   header->num_poses, sizeof(iqmPose_t))){
		return qfalse;
	}
	pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
	for(i = 0; i < header->num_poses; i++, pose++)
		swappose(pose);

	if(header->ofs_bounds){
		/* check and swap model bounds */
		if(checkrange(header, header->ofs_bounds,
		  header->num_frames, sizeof(*bounds))){
			return qfalse;
		}
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for(i = 0; i < header->num_frames; i++){
			swapbounds(bounds);
			bounds++;
		}
	}

	/* allocate the model and copy the data */
	size = sizeof(iqmData_t);
	size += header->num_meshes * sizeof(srfIQModel_t);
	size += header->num_joints * header->num_frames * 12 * sizeof(float);
	if(header->ofs_bounds)
		size += header->num_frames * 6 * sizeof(float);		/* model bounds */
	size += header->num_vertexes * 3 * sizeof(float);		/* positions */
	size += header->num_vertexes * 2 * sizeof(float);		/* texcoords */
	size += header->num_vertexes * 3 * sizeof(float);		/* normals */
	size += header->num_vertexes * 4 * sizeof(float);		/* tangents */
	size += header->num_vertexes * 4 * sizeof(byte);		/* blendIndexes */
	size += header->num_vertexes * 4 * sizeof(byte);		/* blendWeights */
	size += header->num_vertexes * 4 * sizeof(byte);		/* colors */
	size += header->num_joints * sizeof(int);			/* parents */
	size += header->num_triangles * 3 * sizeof(int);		/* triangles */
	size += joint_names;						/* joint names */

	mod->type = MOD_IQM;
	iqmData = (iqmData_t*)ri.Hunk_Alloc(size, h_low);
	mod->modelData = iqmData;

	/* fill header */
	iqmData->num_vertexes	= header->num_vertexes;
	iqmData->num_triangles	= header->num_triangles;
	iqmData->num_frames	= header->num_frames;
	iqmData->num_surfaces	= header->num_meshes;
	iqmData->num_joints	= header->num_joints;
	iqmData->surfaces	= (srfIQModel_t*)(iqmData + 1);
	iqmData->poseMats	= (float*)(iqmData->surfaces + iqmData->num_surfaces);
	if(header->ofs_bounds){
		iqmData->bounds = iqmData->poseMats + 12 * header->num_joints * header->num_frames;
		iqmData->positions = iqmData->bounds + 6 * header->num_frames;
	}else
		iqmData->positions = iqmData->poseMats + 12 * header->num_joints * header->num_frames;
	iqmData->texcoords	= iqmData->positions + 3 * header->num_vertexes;
	iqmData->normals	= iqmData->texcoords + 2 * header->num_vertexes;
	iqmData->tangents	= iqmData->normals + 3 * header->num_vertexes;
	iqmData->blendIndexes	= (byte*)(iqmData->tangents + 4 * header->num_vertexes);
	iqmData->blendWeights	= iqmData->blendIndexes + 4 * header->num_vertexes;
	iqmData->colors		= iqmData->blendWeights + 4 * header->num_vertexes;
	iqmData->jointParents	= (int*)(iqmData->colors + 4 * header->num_vertexes);
	iqmData->triangles	= iqmData->jointParents + header->num_joints;
	iqmData->names		= (char*)(iqmData->triangles + 3 * header->num_triangles);

	/* 
	 * calculate joint matrices and their inverses
	 * they are needed only until the pose matrices are calculated 
	 */
	mat = jointMats;
	joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
	for(i = 0; i < header->num_joints; i++, joint++){
		float baseFrame[12], invBaseFrame[12];

		JointToMatrix(joint->rotate, joint->scale, joint->translate, baseFrame);
		Matrix34Invert(baseFrame, invBaseFrame);

		if(joint->parent >= 0){
			Matrix34Multiply(jointMats + 2 * 12 * joint->parent, baseFrame, mat);
			mat += 12;
			Matrix34Multiply(invBaseFrame, jointMats + 2 * 12 * joint->parent + 12, mat);
			mat += 12;
		}else{
			Q_Memcpy(mat, baseFrame,    sizeof(baseFrame));
			mat += 12;
			Q_Memcpy(mat, invBaseFrame, sizeof(invBaseFrame));
			mat += 12;
		}
	}

	/* calculate pose matrices */
	framedata = (unsigned short*)((byte*)header + header->ofs_frames);
	mat = iqmData->poseMats;
	for(i = 0; i < header->num_frames; i++){
		pose = (iqmPose_t*)((byte*)header + header->ofs_poses);
		for(j = 0; j < header->num_poses; j++, pose++){
			Vec3	translate;
			Vec4	rotate;
			Vec3	scale;
			float	mat1[12], mat2[12];

			translate[0] = pose->channeloffset[0];
			if(pose->mask & 0x001)
				translate[0] += *framedata++ *pose->channelscale[0];
			translate[1] = pose->channeloffset[1];
			if(pose->mask & 0x002)
				translate[1] += *framedata++ *pose->channelscale[1];
			translate[2] = pose->channeloffset[2];
			if(pose->mask & 0x004)
				translate[2] += *framedata++ *pose->channelscale[2];

			rotate[0] = pose->channeloffset[3];
			if(pose->mask & 0x008)
				rotate[0] += *framedata++ *pose->channelscale[3];
			rotate[1] = pose->channeloffset[4];
			if(pose->mask & 0x010)
				rotate[1] += *framedata++ *pose->channelscale[4];
			rotate[2] = pose->channeloffset[5];
			if(pose->mask & 0x020)
				rotate[2] += *framedata++ *pose->channelscale[5];
			rotate[3] = pose->channeloffset[6];
			if(pose->mask & 0x040)
				rotate[3] += *framedata++ *pose->channelscale[6];

			scale[0] = pose->channeloffset[7];
			if(pose->mask & 0x080)
				scale[0] += *framedata++ *pose->channelscale[7];
			scale[1] = pose->channeloffset[8];
			if(pose->mask & 0x100)
				scale[1] += *framedata++ *pose->channelscale[8];
			scale[2] = pose->channeloffset[9];
			if(pose->mask & 0x200)
				scale[2] += *framedata++ *pose->channelscale[9];

			/* construct transformation matrix */
			JointToMatrix(rotate, scale, translate, mat1);

			if(pose->parent >= 0){
				Matrix34Multiply(jointMats + 12 * 2 * pose->parent,
					mat1, mat2);
			}else{
				Q_Memcpy(mat2, mat1, sizeof(mat1));
			}

			Matrix34Multiply(mat2, jointMats + 12 * (2 * j + 1), mat);
			mat += 12;
		}
	}

	/* register shaders */
	/* overwrite the material offset with the shader index */
	mesh = (iqmMesh_t*)((byte*)header + header->ofs_meshes);
	surface = iqmData->surfaces;
	str = (char*)header + header->ofs_text;
	for(i = 0; i < header->num_meshes; i++, mesh++, surface++){
		surface->surfaceType = SF_IQM;
		Q_strncpyz(surface->name, str + mesh->name, sizeof(surface->name));
		Q_strlwr(surface->name);	/* lowercase the surface name so skin compares are faster */
		surface->shader = R_FindShader(str + mesh->material, LIGHTMAP_NONE, qtrue);
		if(surface->shader->defaultShader)
			surface->shader = tr.defaultShader;
		surface->data = iqmData;
		surface->first_vertex	= mesh->first_vertex;
		surface->num_vertexes	= mesh->num_vertexes;
		surface->first_triangle = mesh->first_triangle;
		surface->num_triangles	= mesh->num_triangles;
	}

	/* copy vertexarrays and indexes */
	vertexarray = (iqmVertexArray_t*)((byte*)header + header->ofs_vertexarrays);
	for(i = 0; i < header->num_vertexarrays; i++, vertexarray++){
		int n;

		/* total number of values */
		n = header->num_vertexes * vertexarray->size;

		switch(vertexarray->type){
		case IQM_POSITION:
			Q_Memcpy(iqmData->positions,
				(byte*)header + vertexarray->offset,
				n * sizeof(float));
			break;
		case IQM_NORMAL:
			Q_Memcpy(iqmData->normals,
				(byte*)header + vertexarray->offset,
				n * sizeof(float));
			break;
		case IQM_TANGENT:
			Q_Memcpy(iqmData->tangents,
				(byte*)header + vertexarray->offset,
				n * sizeof(float));
			break;
		case IQM_TEXCOORD:
			Q_Memcpy(iqmData->texcoords,
				(byte*)header + vertexarray->offset,
				n * sizeof(float));
			break;
		case IQM_BLENDINDEXES:
			Q_Memcpy(iqmData->blendIndexes,
				(byte*)header + vertexarray->offset,
				n * sizeof(byte));
			break;
		case IQM_BLENDWEIGHTS:
			Q_Memcpy(iqmData->blendWeights,
				(byte*)header + vertexarray->offset,
				n * sizeof(byte));
			break;
		case IQM_COLOR:
			Q_Memcpy(iqmData->colors,
				(byte*)header + vertexarray->offset,
				n * sizeof(byte));
			break;
		}
	}

	/* copy joint parents */
	joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
	for(i = 0; i < header->num_joints; i++, joint++)
		iqmData->jointParents[i] = joint->parent;

	/* copy triangles */
	triangle = (iqmTriangle_t*)((byte*)header + header->ofs_triangles);
	for(i = 0; i < header->num_triangles; i++, triangle++){
		iqmData->triangles[3*i+0] = triangle->vertex[0];
		iqmData->triangles[3*i+1] = triangle->vertex[1];
		iqmData->triangles[3*i+2] = triangle->vertex[2];
	}

	/* copy joint names */
	str = iqmData->names;
	joint = (iqmJoint_t*)((byte*)header + header->ofs_joints);
	for(i = 0; i < header->num_joints; i++, joint++){
		char *name = (char*)header + header->ofs_text +
			     joint->name;
		int len = strlen(name) + 1;
		Q_Memcpy(str, name, len);
		str += len;
	}

	/* copy model bounds */
	if(header->ofs_bounds){
		mat = iqmData->bounds;
		bounds = (iqmBounds_t*)((byte*)header + header->ofs_bounds);
		for(i = 0; i < header->num_frames; i++){
			mat[0]	= bounds->bbmin[0];
			mat[1]	= bounds->bbmin[1];
			mat[2]	= bounds->bbmin[2];
			mat[3]	= bounds->bbmax[0];
			mat[4]	= bounds->bbmax[1];
			mat[5]	= bounds->bbmax[2];

			mat += 6;
			bounds++;
		}
	}
	return qtrue;
}

static int
culliqm(iqmData_t *data, trRefEntity_t *ent)
{
	Vec3 bounds[2];
	Scalar *oldBounds, *newBounds;
	int	i;

	if(!data->bounds){
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	}

	/* compute bounds pointers */
	oldBounds = data->bounds + 6*ent->e.oldframe;
	newBounds = data->bounds + 6*ent->e.frame;

	/* calculate a bounding box in the current coordinate system */
	for(i = 0; i < 3; i++){
		bounds[0][i] = oldBounds[i] < newBounds[i] ? oldBounds[i] : newBounds[i];
		bounds[1][i] = oldBounds[i+3] > newBounds[i+3] ? oldBounds[i+3] : newBounds[i+3];
	}

	switch(R_CullLocalBox(bounds)){
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}

int
R_ComputeIQMFogNum(iqmData_t *data, trRefEntity_t *ent)
{
	int i, j;
	fog_t *fog;
	const Scalar *bounds;
	const Scalar defaultBounds[6] = { -8, -8, -8, 8, 8, 8 };
	Vec3	diag, center;
	Vec3	localOrigin;
	Scalar	radius;

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL){
		return 0;
	}

	/* FIXME: non-normalized axis issues */
	if(data->bounds){
		bounds = data->bounds + 6*ent->e.frame;
	}else{
		bounds = defaultBounds;
	}
	vec3sub(bounds+3, bounds, diag);
	vec3ma(bounds, 0.5f, diag, center);
	vec3add(ent->e.origin, center, localOrigin);
	radius = 0.5f * vec3len(diag);

	for(i = 1; i < tr.world->numfogs; i++){
		fog = &tr.world->fogs[i];
		for(j = 0; j < 3; j++){
			if(localOrigin[j] - radius >= fog->bounds[1][j]){
				break;
			}
			if(localOrigin[j] + radius <= fog->bounds[0][j]){
				break;
			}
		}
		if(j == 3){
			return i;
		}
	}

	return 0;
}

/* Add all surfaces of this model */
void
R_AddIQMSurfaces(trRefEntity_t *ent)
{
	iqmData_t *data;
	srfIQModel_t *surface;
	int i, j;
	qbool personalModel;
	int cull;
	int fogNum;
	material_t *shader;
	skin_t *skin;

	data = tr.currentModel->modelData;
	surface = data->surfaces;

	/* don't add third_person objects if not in a portal */
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	if(ent->e.renderfx & RF_WRAP_FRAMES){
		ent->e.frame %= data->num_frames;
		ent->e.oldframe %= data->num_frames;
	}

	/*
	 * Validate the frames so there is no chance of a crash.
	 * This will write directly into the entity structure, so
	 * when the surfaces are rendered, they don't need to be
	 * range checked again.
	 *  */
	if((ent->e.frame >= data->num_frames)
	   || (ent->e.frame < 0)
	   || (ent->e.oldframe >= data->num_frames)
	   || (ent->e.oldframe < 0)){
		ri.Printf(PRINT_DEVELOPER, "R_AddIQMSurfaces: no such frame %d to %d for '%s'\n",
			ent->e.oldframe, ent->e.frame,
			tr.currentModel->name);
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	/*
	 * cull the entire model if merged bounding box of both frames
	 * is outside the view frustum.
	 */
	cull = culliqm (data, ent);
	if(cull == CULL_OUT){
		return;
	}

	/* set up lighting now that we know we aren't culled */
	if(!personalModel || r_shadows->integer > 1){
		R_SetupEntityLighting(&tr.refdef, ent);
	}

	/* see if we are in a fog volume */
	fogNum = R_ComputeIQMFogNum(data, ent);

	for(i = 0; i < data->num_surfaces; i++){
		if(ent->e.customShader)
			shader = R_GetShaderByHandle(ent->e.customShader);
		else if(ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins){
			skin = R_GetSkinByHandle(ent->e.customSkin);
			shader = tr.defaultShader;

			for(j = 0; j < skin->numSurfaces; j++)
				if(!strcmp(skin->surfaces[j]->name, surface->name)){
					shader = skin->surfaces[j]->shader;
					break;
				}
		}else{
			shader = surface->shader;
		}

		/* we will add shadows even if the main object isn't visible in the view */

		/* stencil shadows can't do personal models unless I polyhedron clip */
		if(!personalModel
		   && r_shadows->integer == 2
		   && fogNum == 0
		   && !(ent->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
		   && shader->sort == SS_OPAQUE){
			R_AddDrawSurf((void*)surface, tr.shadowShader, 0, 0, 0);
		}

		/* projection shadows work fine with personal models */
		if(r_shadows->integer == 3
		   && fogNum == 0
		   && (ent->e.renderfx & RF_SHADOW_PLANE)
		   && shader->sort == SS_OPAQUE){
			R_AddDrawSurf((void*)surface, tr.projectionShadowShader, 0, 0, 0);
		}

		if(!personalModel){
			R_AddDrawSurf((void*)surface, shader, fogNum, 0, 0);
		}

		surface++;
	}
}

static void
calcjointmats(iqmData_t *data, int frame, int oldframe,
		 float backlerp, float *mat)
{
	float *mat1, *mat2;
	int *joint = data->jointParents;
	int i;

	if(oldframe == frame){
		mat1 = data->poseMats + 12 * data->num_joints * frame;
		for(i = 0; i < data->num_joints; i++, joint++){
			if(*joint >= 0){
				Matrix34Multiply(mat + 12 * *joint,
					mat1 + 12*i, mat + 12*i);
			}else{
				Q_Memcpy(mat + 12*i, mat1 + 12*i, 12 * sizeof(float));
			}
		}
	}else{
		mat1 = data->poseMats + 12 * data->num_joints * frame;
		mat2 = data->poseMats + 12 * data->num_joints * oldframe;

		for(i = 0; i < data->num_joints; i++, joint++){
			if(*joint >= 0){
				float tmpMat[12];
				InterpolateMatrix(mat1 + 12*i, mat2 + 12*i,
					backlerp, tmpMat);
				Matrix34Multiply(mat + 12 * *joint,
					tmpMat, mat + 12*i);

			}else{
				InterpolateMatrix(mat1 + 12*i, mat2 + 12*i,
					backlerp, mat);
			}
		}
	}
}

/* Compute vertices for this model surface */
void
RB_IQMSurfaceAnim(surfaceType_t *surface)
{
	srfIQModel_t *surf = (srfIQModel_t*)surface;
	iqmData_t *data = surf->data;
	float jointMats[IQM_MAX_JOINTS * 12];
	int i;
	Vec4 *outXYZ = &tess.xyz[tess.numVertexes];
	Vec4 *outNormal = &tess.normal[tess.numVertexes];
	Vec2 (*outTexCoord)[2] = &tess.texCoords[tess.numVertexes];
	Vec4 *outColor = &tess.vertexColors[tess.numVertexes];
	int frame = backEnd.currentEntity->e.frame % data->num_frames;
	int oldframe = backEnd.currentEntity->e.oldframe % data->num_frames;
	float backlerp = backEnd.currentEntity->e.backlerp;
	int *tri;
	glIndex_t *ptr;
	glIndex_t base;

	RB_CHECKOVERFLOW(surf->num_vertexes, surf->num_triangles * 3);

	/* compute interpolated joint matrices */
	calcjointmats(data, frame, oldframe, backlerp, jointMats);

	/* transform vertexes and fill other data */
	for(i = 0; i < surf->num_vertexes;
	    i++, outXYZ++, outNormal++, outTexCoord++, outColor++){
		int j, k;
		float vtxMat[12];
		float nrmMat[9];
		int vtx = i + surf->first_vertex;

		/* compute the vertex matrix by blending the up to
		 * four blend weights */
		for(k = 0; k < 12; k++)
			vtxMat[k] = data->blendWeights[4*vtx]
				    * jointMats[12*data->blendIndexes[4*vtx] + k];
		for(j = 1; j < 4; j++){
			if(data->blendWeights[4*vtx + j] <= 0)
				break;
			for(k = 0; k < 12; k++)
				vtxMat[k] += data->blendWeights[4*vtx + j]
					     * jointMats[12*data->blendIndexes[4*vtx + j] + k];
		}
		for(k = 0; k < 12; k++)
			vtxMat[k] *= 1.0f / 255.0f;

		/* 
		 * compute the normal matrix as transpose of the adjoint
		 * of the vertex matrix 
		 */
		nrmMat[ 0] = vtxMat[ 5]*vtxMat[10] - vtxMat[ 6]*vtxMat[ 9];
		nrmMat[ 1] = vtxMat[ 6]*vtxMat[ 8] - vtxMat[ 4]*vtxMat[10];
		nrmMat[ 2] = vtxMat[ 4]*vtxMat[ 9] - vtxMat[ 5]*vtxMat[ 8];
		nrmMat[ 3] = vtxMat[ 2]*vtxMat[ 9] - vtxMat[ 1]*vtxMat[10];
		nrmMat[ 4] = vtxMat[ 0]*vtxMat[10] - vtxMat[ 2]*vtxMat[ 8];
		nrmMat[ 5] = vtxMat[ 1]*vtxMat[ 8] - vtxMat[ 0]*vtxMat[ 9];
		nrmMat[ 6] = vtxMat[ 1]*vtxMat[ 6] - vtxMat[ 2]*vtxMat[ 5];
		nrmMat[ 7] = vtxMat[ 2]*vtxMat[ 4] - vtxMat[ 0]*vtxMat[ 6];
		nrmMat[ 8] = vtxMat[ 0]*vtxMat[ 5] - vtxMat[ 1]*vtxMat[ 4];

		(*outTexCoord)[0][0] = data->texcoords[2*vtx + 0];
		(*outTexCoord)[0][1] = data->texcoords[2*vtx + 1];
		(*outTexCoord)[1][0] = (*outTexCoord)[0][0];
		(*outTexCoord)[1][1] = (*outTexCoord)[0][1];

		(*outXYZ)[0] =
			vtxMat[ 0] * data->positions[3*vtx+0] +
			vtxMat[ 1] * data->positions[3*vtx+1] +
			vtxMat[ 2] * data->positions[3*vtx+2] +
			vtxMat[ 3];
		(*outXYZ)[1] =
			vtxMat[ 4] * data->positions[3*vtx+0] +
			vtxMat[ 5] * data->positions[3*vtx+1] +
			vtxMat[ 6] * data->positions[3*vtx+2] +
			vtxMat[ 7];
		(*outXYZ)[2] =
			vtxMat[ 8] * data->positions[3*vtx+0] +
			vtxMat[ 9] * data->positions[3*vtx+1] +
			vtxMat[10] * data->positions[3*vtx+2] +
			vtxMat[11];
		(*outXYZ)[3] = 1.0f;

		(*outNormal)[0] =
			nrmMat[ 0] * data->normals[3*vtx+0] +
			nrmMat[ 1] * data->normals[3*vtx+1] +
			nrmMat[ 2] * data->normals[3*vtx+2];
		(*outNormal)[1] =
			nrmMat[ 3] * data->normals[3*vtx+0] +
			nrmMat[ 4] * data->normals[3*vtx+1] +
			nrmMat[ 5] * data->normals[3*vtx+2];
		(*outNormal)[2] =
			nrmMat[ 6] * data->normals[3*vtx+0] +
			nrmMat[ 7] * data->normals[3*vtx+1] +
			nrmMat[ 8] * data->normals[3*vtx+2];
		(*outNormal)[3] = 0.0f;

		(*outColor)[0]	= data->colors[4*vtx+0] / 255.0f;
		(*outColor)[1]	= data->colors[4*vtx+1] / 255.0f;
		(*outColor)[2]	= data->colors[4*vtx+2] / 255.0f;
		(*outColor)[3]	= data->colors[4*vtx+3] / 255.0f;
	}

	tri = data->triangles + 3 * surf->first_triangle;
	ptr = &tess.indexes[tess.numIndexes];
	base = tess.numVertexes;

	for(i = 0; i < surf->num_triangles; i++){
		*ptr++	= base + (*tri++ - surf->first_vertex);
		*ptr++	= base + (*tri++ - surf->first_vertex);
		*ptr++	= base + (*tri++ - surf->first_vertex);
	}

	tess.numIndexes += 3 * surf->num_triangles;
	tess.numVertexes += surf->num_vertexes;
}

int
R_IQMLerpTag(orientation_t *tag, iqmData_t *data,
	     int startFrame, int endFrame,
	     float frac, const char *tagName)
{
	float	jointMats[IQM_MAX_JOINTS * 12];
	int	joint;
	char	*names = data->names;

	/* get joint number by reading the joint names */
	for(joint = 0; joint < data->num_joints; joint++){
		if(!strcmp(tagName, names))
			break;
		names += strlen(names) + 1;
	}
	if(joint >= data->num_joints){
		axisclear(tag->axis);
		vec3clear(tag->origin);
		return qfalse;
	}

	calcjointmats(data, startFrame, endFrame, frac, jointMats);

	tag->axis[0][0] = jointMats[12 * joint + 0];
	tag->axis[1][0] = jointMats[12 * joint + 1];
	tag->axis[2][0] = jointMats[12 * joint + 2];
	tag->origin[0]	= jointMats[12 * joint + 3];
	tag->axis[0][1] = jointMats[12 * joint + 4];
	tag->axis[1][1] = jointMats[12 * joint + 5];
	tag->axis[2][1] = jointMats[12 * joint + 6];
	tag->origin[1]	= jointMats[12 * joint + 7];
	tag->axis[0][2] = jointMats[12 * joint + 8];
	tag->axis[1][2] = jointMats[12 * joint + 9];
	tag->axis[2][2] = jointMats[12 * joint + 10];
	tag->origin[2]	= jointMats[12 * joint + 11];

	return qtrue;
}
