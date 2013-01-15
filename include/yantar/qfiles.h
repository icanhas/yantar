/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#ifndef __QFILES_H__
#define __QFILES_H__

/*
 * qfiles.h: quake file formats
 * This file must be identical in the quake and utils directories
 *  */

/* Ignore __attribute__ on non-gcc platforms */
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

/* surface geometry should not exceed these limits */
#define SHADER_MAX_VERTEXES	40000	/* FIXME */
#define SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)


/* the maximum size of game relative pathnames */
#define MAX_QPATH 64

/*
 *
 * QVM files
 *
 */

#define VM_MAGIC	0x12721444
#define VM_MAGIC_VER2	0x12721445
typedef struct {
	int	vmMagic;

	int	instructionCount;

	int	codeOffset;
	int	codeLength;

	int	dataOffset;
	int	dataLength;
	int	litLength;	/* ( dataLength - litLength ) should be byteswapped on load */
	int	bssLength;	/* zero filled memory appended to datalength */

	/* !!! below here is VM_MAGIC_VER2 !!! */
	int jtrgLength;	/* number of jump table targets */
} Vmheader;

/*
 *
 * .MD3 triangle model file format
 *
 */

#define MD3_IDENT	(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION	15

/* limits */
#define MD3_MAX_LODS		3
#define MD3_MAX_TRIANGLES	8192	/* per surface */
#define MD3_MAX_VERTS		4096	/* per surface */
#define MD3_MAX_SHADERS		256	/* per surface */
#define MD3_MAX_FRAMES		1024	/* per model */
#define MD3_MAX_SURFACES	32	/* per model */
#define MD3_MAX_TAGS		16	/* per frame */

/* vertex scales */
#define MD3_XYZ_SCALE (1.0f/64)

typedef struct MD3frame {
	Vec3	bounds[2];
	Vec3	localOrigin;
	float	radius;
	char	name[16];
} MD3frame;

typedef struct MD3tag {
	char	name[MAX_QPATH];	/* tag name */
	Vec3	origin;
	Vec3	axis[3];
} MD3tag;

/*
** MD3surf
**
** CHUNK			SIZE
** header			sizeof( MD3surf )
** shaders			sizeof( MD3shader ) * numShaders
** triangles[0]		sizeof( MD3tri ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( MD3xyznorm ) * numVerts * numFrames
*/
typedef struct {
	int	ident;

	char	name[MAX_QPATH];	/* polyset name */

	int	flags;
	int	numFrames;	/* all surfaces in a model should have the same */

	int	numShaders;	/* all surfaces in a model should have the same */
	int	numVerts;

	int	numTriangles;
	int	ofsTriangles;

	int	ofsShaders;	/* offset from start of MD3surf */
	int	ofsSt;		/* texture coords are common for all frames */
	int	ofsXyzNormals;	/* numVerts * numFrames */

	int	ofsEnd;	/* next surface follows */
} MD3surf;

typedef struct {
	char	name[MAX_QPATH];
	int	shaderIndex;	/* for in-game use */
} MD3shader;

typedef struct {
	int indexes[3];
} MD3tri;

typedef struct {
	float st[2];
} md3St_t;

typedef struct {
	short	xyz[3];
	short	normal;
} MD3xyznorm;

typedef struct {
	int	ident;
	int	version;

	char	name[MAX_QPATH];	/* model name */

	int	flags;

	int	numFrames;
	int	numTags;
	int	numSurfaces;

	int	numSkins;

	int	ofsFrames;	/* offset for first frame */
	int	ofsTags;	/* numFrames * numTags */
	int	ofsSurfaces;	/* first surface, others follow */

	int	ofsEnd;	/* end of file */
} MD3header;

/*
 *
 * MD4 file format
 *
 */

#define MD4_IDENT	(('4'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD4_VERSION	1
#define MD4_MAX_BONES	128

typedef struct {
	int	boneIndex;	/* these are indexes into the boneReferences, */
	float	boneWeight;	/* not the global per-frame bone list */
	Vec3	offset;
} md4Weight_t;

typedef struct {
	Vec3		normal;
	Vec2		texCoords;
	int		numWeights;
	md4Weight_t	weights[1];	/* variable sized */
} md4Vertex_t;

typedef struct {
	int indexes[3];
} md4Triangle_t;

typedef struct {
	int	ident;

	char	name[MAX_QPATH];	/* polyset name */
	char	shader[MAX_QPATH];
	int	shaderIndex;	/* for in-game use */

	int	ofsHeader;	/* this will be a negative number */

	int	numVerts;
	int	ofsVerts;

	int	numTriangles;
	int	ofsTriangles;

	/* Bone references are a set of ints representing all the bones
	 * present in any vertex weights for this surface.  This is
	 * needed because a model may have surfaces that need to be
	 * drawn at different sort times, and we don't want to have
	 * to re-interpolate all the bones for each surface. */
	int	numBoneReferences;
	int	ofsBoneReferences;

	int	ofsEnd;	/* next surface follows */
} md4Surface_t;

typedef struct {
	float matrix[3][4];
} md4Bone_t;

typedef struct {
	Vec3		bounds[2];	/* bounds of all surfaces of all LOD's for this frame */
	Vec3		localOrigin;	/* midpoint of bounds, used for sphere cull */
	float		radius;		/* dist from localOrigin to corner */
	md4Bone_t	bones[1];	/* [numBones] */
} md4Frame_t;

typedef struct {
	int	numSurfaces;
	int	ofsSurfaces;	/* first surface, others follow */
	int	ofsEnd;		/* next lod follows */
} md4LOD_t;

typedef struct {
	int	ident;
	int	version;

	char	name[MAX_QPATH];	/* model name */

	/* frames and bones are shared by all levels of detail */
	int	numFrames;
	int	numBones;
	int	ofsBoneNames;	/* char	name[ MAX_QPATH ] */
	int	ofsFrames;	/* md4Frame_t[numFrames] */

	/* each level of detail has completely separate sets of surfaces */
	int	numLODs;
	int	ofsLODs;

	int	ofsEnd;	/* end of file */
} md4Header_t;

/*
 *
 * .BSP file format
 *
 */


#define BSP_IDENT (('P'<<24)+('S'<<16)+('B'<<8)+'I')
/* little-endian "IBSP" */

#define BSP_VERSION 46


/* there shouldn't be any problem with increasing these values at the
 * expense of more memory allocation in the utilities */
#define MAX_MAP_MODELS		0x400
#define MAX_MAP_BRUSHES		0x8000
#define MAX_MAP_ENTITIES	0x800
#define MAX_MAP_ENTSTRING	0x40000
#define MAX_MAP_SHADERS		0x400

#define MAX_MAP_AREAS		0x100	/* MAX_MAP_AREA_BYTES in q_shared must match! */
#define MAX_MAP_FOGS		0x100
#define MAX_MAP_PLANES		0x20000
#define MAX_MAP_NODES		0x20000
#define MAX_MAP_BRUSHSIDES	0x20000
#define MAX_MAP_LEAFS		0x20000
#define MAX_MAP_LEAFFACES	0x20000
#define MAX_MAP_LEAFBRUSHES	0x40000
#define MAX_MAP_PORTALS		0x20000
#define MAX_MAP_LIGHTING	0x800000
#define MAX_MAP_LIGHTGRID	0x800000
#define MAX_MAP_VISIBILITY	0x200000

#define MAX_MAP_DRAW_SURFS	0x20000
#define MAX_MAP_DRAW_VERTS	0x80000
#define MAX_MAP_DRAW_INDEXES	0x80000


/* key / value pair sizes in the entities lump */
#define MAX_KEY		32
#define MAX_VALUE	1024

/* the editor uses these predefined yaw angles to orient entities up or down */
#define ANGLE_UP	-1
#define ANGLE_DOWN	-2

#define LIGHTMAP_WIDTH	128
#define LIGHTMAP_HEIGHT 128

#define MAX_WORLD_COORD (128*1024)
#define MIN_WORLD_COORD (-128*1024)
#define WORLD_SIZE	(MAX_WORLD_COORD - MIN_WORLD_COORD)

/* ============================================================================= */


typedef struct {
	int fileofs, filelen;
} Lump;

#define LUMP_ENTITIES		0
#define LUMP_SHADERS		1
#define LUMP_PLANES		2
#define LUMP_NODES		3
#define LUMP_LEAFS		4
#define LUMP_LEAFSURFACES	5
#define LUMP_LEAFBRUSHES	6
#define LUMP_MODELS		7
#define LUMP_BRUSHES		8
#define LUMP_BRUSHSIDES		9
#define LUMP_DRAWVERTS		10
#define LUMP_DRAWINDEXES	11
#define LUMP_FOGS		12
#define LUMP_SURFACES		13
#define LUMP_LIGHTMAPS		14
#define LUMP_LIGHTGRID		15
#define LUMP_VISIBILITY		16
#define HEADER_LUMPS		17

typedef struct {
	int	ident;
	int	version;

	Lump	lumps[HEADER_LUMPS];
} Dheader;

typedef struct {
	float	mins[3], maxs[3];
	int	firstSurface, numSurfaces;
	int	firstBrush, numBrushes;
} Dmodel;

typedef struct {
	char	shader[MAX_QPATH];
	int	surfaceFlags;
	int	contentFlags;
} Dmaterial;

/* planes x^1 is allways the opposite of plane x */

typedef struct {
	float	normal[3];
	float	dist;
} Dplane;

typedef struct {
	int	planeNum;
	int	children[2];	/* negative numbers are -(leafs+1), not nodes */
	int	mins[3];	/* for frustom culling */
	int	maxs[3];
} Dnode;

typedef struct {
	int	cluster;	/* -1 = opaque cluster (do I still store these?) */
	int	area;

	int	mins[3];	/* for frustum culling */
	int	maxs[3];

	int	firstLeafSurface;
	int	numLeafSurfaces;

	int	firstLeafBrush;
	int	numLeafBrushes;
} Dleaf;

typedef struct {
	int	planeNum;	/* positive plane side faces out of the leaf */
	int	shaderNum;
} Dbrushside;

typedef struct {
	int	firstSide;
	int	numSides;
	int	shaderNum;	/* the shader that determines the contents flags */
} Dbrush;

typedef struct {
	char	shader[MAX_QPATH];
	int	brushNum;
	int	visibleSide;	/* the brush side that ray tests need to clip against (-1 == none) */
} Dfog;

typedef struct {
	Vec3	xyz;
	float	st[2];
	float	lightmap[2];
	Vec3	normal;
	byte	color[4];
} Drawvert;

#define drawVert_t_cleared(x) Drawvert (x) = \
{{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0, 0}}

typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE
} Mapsurftype;

typedef struct {
	int	shaderNum;
	int	fogNum;
	int	surfaceType;

	int	firstVert;
	int	numVerts;

	int	firstIndex;
	int	numIndexes;

	int	lightmapNum;
	int	lightmapX, lightmapY;
	int	lightmapWidth, lightmapHeight;

	Vec3	lightmapOrigin;
	Vec3	lightmapVecs[3];	/* for patches, [0] and [1] are lodbounds */

	int	patchWidth;
	int	patchHeight;
} Dsurf;


#endif
