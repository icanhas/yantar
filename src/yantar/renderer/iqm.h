/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifndef __IQM_H__
#define __IQM_H__

#define IQM_MAGIC "INTERQUAKEMODEL"

enum {
	IQM_VERSION		= 2,
	IQM_MAX_JOINTS	= 128,
	
	IQM_POSITION		= 0,
	IQM_TEXCOORD	= 1,
	IQM_NORMAL		= 2,
	IQM_TANGENT		= 3,
	IQM_BLENDINDEXES	= 4,
	IQM_BLENDWEIGHTS= 5,
	IQM_COLOR		= 6,
	IQM_CUSTOM		= 0x10,
	
	IQM_BYTE	= 0,
	IQM_UBYTE		= 1,
	IQM_SHORT		= 2,
	IQM_USHORT		= 3,
	IQM_INT			= 4,
	IQM_UINT			= 5,
	IQM_HALF			= 6,
	IQM_FLOAT		= 7,
	IQM_DOUBLE		= 8,
	
	IQM_LOOP		= 1<<0
};

typedef struct iqmHeader_t iqmHeader_t;
typedef struct iqmMesh_t iqmMesh_t;
typedef struct iqmTriangle_t iqmTriangle_t;
typedef struct iqmJoint_t iqmJoint_t;
typedef struct iqmPose_t iqmPose_t;
typedef struct iqmAnim_t iqmAnim_t;
typedef struct iqmVertexArray_t iqmVertexArray_t;
typedef struct iqmBounds_t iqmBounds_t;

struct iqmHeader_t {
	char	magic[16];
	uint	version;
	uint	filesize;
	uint	flags;
	uint	num_text, ofs_text;
	uint	num_meshes, ofs_meshes;
	uint	num_vertexarrays, num_vertexes, ofs_vertexarrays;
	uint	num_triangles, ofs_triangles, ofs_adjacency;
	uint	num_joints, ofs_joints;
	uint	num_poses, ofs_poses;
	uint	num_anims, ofs_anims;
	uint	num_frames, num_framechannels, ofs_frames, ofs_bounds;
	uint	num_comment, ofs_comment;
	uint	num_extensions, ofs_extensions;
};

struct iqmMesh_t {
	uint	name;
	uint	material;
	uint	first_vertex, num_vertexes;
	uint	first_triangle, num_triangles;
};

struct iqmTriangle_t {
	uint vertex[3];
};

struct iqmJoint_t {
	uint	name;
	int	parent;
	float	translate[3], rotate[4], scale[3];
};

struct iqmPose_t {
	int	parent;
	uint	mask;
	float	channeloffset[10];
	float	channelscale[10];
};

struct iqmAnim_t {
	uint	name;
	uint	first_frame, num_frames;
	float	framerate;
	uint	flags;
};

struct iqmVertexArray_t {
	uint	type;
	uint	flags;
	uint	format;
	uint	size;
	uint	offset;
};

struct iqmBounds_t {
	float	bbmin[3], bbmax[3];
	float	xyradius, radius;
};

#endif
