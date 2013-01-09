/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

enum { REF_API_VERSION = 0 };

#define MAX_DLIGHTS	32	/* can't be increased, because bit flags are used on surfaces */
#define ENTITYNUM_BITS	10
#define MAX_ENTITIES	((1 << ENTITYNUM_BITS) - 1)	/* can't be increased without changing drawsurf bit packing */

/* renderfx flags */
#define RF_MINLIGHT	0x0001	/* allways have some light (viewmodel, some items) */
#define RF_THIRD_PERSON 0x0002	/* don't draw through eyes, only mirrors (player bodies, chat sprites) */
#define RF_FIRST_PERSON 0x0004	/* only draw through eyes (view weapon, damage blood blob) */
#define RF_DEPTHHACK	0x0008	/* for view weapon Z crunching */

#define RF_CROSSHAIR	0x0010	/* This item is a cross hair and will draw over everything similar to
				 * DEPTHHACK in stereo rendering mode, with the difference that the
				 * projection matrix won't be hacked to reduce the stereo separation as
				 * is done for the gun. */

#define RF_NOSHADOW		0x0040	/* don't add stencil shadows */

#define RF_LIGHTING_ORIGIN	0x0080	/* use refEntity->lightingOrigin instead of refEntity->origin
					 * for lighting.  This allows entities to sink into the floor
					 * with their origin going solid, and allows all parts of a
					 * player to get the same lighting */

#define RF_SHADOW_PLANE 0x0100	/* use refEntity->shadowPlane */
#define RF_WRAP_FRAMES	0x0200	/* mod the model frames by the maxframes to allow continuous */

/* refdef flags */
#define RDF_NOWORLDMODEL	0x0001	/* used for player configuration screen */
#define RDF_HYPERSPACE		0x0004	/* teleportation effect */

typedef struct {
	Vec3	xyz;
	float	st[2];
	byte	modulate[4];
} Polyvert;

typedef struct Poly {
	Handle		hShader;
	int		numVerts;
	Polyvert *verts;
} Poly;

typedef enum {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,	/* doesn't draw anything, just info for portals */

	RT_MAX_REF_ENTITY_TYPE
} Refenttype;

typedef struct {
	Refenttype reType;
	int		renderfx;

	Handle		hModel;	/* opaque type outside refresh */

	/* most recent data */
	Vec3		lightingOrigin;	/* so multi-part models can be lit identically (RF_LIGHTING_ORIGIN) */
	float		shadowPlane;	/* projection shadows go here, stencils go slightly lower */

	Vec3		axis[3];		/* rotation vectors */
	qbool		nonNormalizedAxes;	/* axis are not normalized, i.e. they have scale */
	float		origin[3];		/* also used as MODEL_BEAM's "from" */
	int		frame;			/* also used as MODEL_BEAM's diameter */

	/* previous data for frame interpolation */
	float	oldorigin[3];	/* also used as MODEL_BEAM's "to" */
	int	oldframe;
	float	backlerp;	/* 0.0 = current, 1.0 = old */

	/* texturing */
	int		skinNum;	/* inline skin index */
	Handle		customSkin;	/* NULL for default skin */
	Handle		customShader;	/* use one image for the entire thing */

	/* misc */
	byte	shaderRGBA[4];		/* colors used by rgbgen entity shaders */
	float	shaderTexCoord[2];	/* texture coordinates used by tcMod entity modifiers */
	float	shaderTime;		/* subtracted from refdef time to control effect start times */

	/* extra sprite information */
	float	radius;
	float	rotation;
} Refent;


#define MAX_RENDER_STRINGS		8
#define MAX_RENDER_STRING_LENGTH	32

typedef struct {
	int	x, y, width, height;
	float	fov_x, fov_y;
	Vec3	vieworg;
	Vec3	viewaxis[3];	/* transformation matrix */

	/* time in milliseconds for shader effects and other time dependent rendering issues */
	int	time;

	int	rdflags;	/* RDF_NOWORLDMODEL, etc */

	/* 1 bits will prevent the associated area from rendering at all */
	byte areamask[MAX_MAP_AREA_BYTES];

	/* text messages for deform text shaders */
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} Refdef;


typedef enum {
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} Stereoframe;


/*
** Glconfig
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
typedef enum {
	TC_NONE,
	TC_S3TC,	/* this is for the GL_S3_s3tc extension. */
	TC_S3TC_ARB	/* this is for the GL_EXT_texture_compression_s3tc extension. */
} textureCompression_t;

typedef enum {
	GLDRV_ICD,	/* driver is integrated with window system */
	/* WARNING: there are tests that check for
	 * > GLDRV_ICD for minidriverness, so this
	 * should always be the lowest value in this
	 * enum set */
	GLDRV_STANDALONE,	/* driver is a non-3Dfx standalone driver */
	GLDRV_VOODOO		/* driver is a 3Dfx standalone driver */
} glDriverType_t;

typedef enum {
	GLHW_GENERIC,	/* where everthing works the way it should */
	GLHW_3DFX_2D3D,	/* Voodoo Banshee or Voodoo3, relevant since if this is */
	/* the hardware type then there can NOT exist a secondary
	 * display adapter */
	GLHW_RIVA128,	/* where you can't interpolate alpha */
	GLHW_RAGEPRO,	/* where you can't modulate alpha on alpha textures */
	GLHW_PERMEDIA2	/* where you don't have src*dst */
} glHardwareType_t;

typedef struct {
	char			renderer_string[MAX_STRING_CHARS];
	char			vendor_string[MAX_STRING_CHARS];
	char			version_string[MAX_STRING_CHARS];
	char			extensions_string[BIG_INFO_STRING];

	int			maxTextureSize;		/* queried from GL */
	int			numTextureUnits;	/* multitexture ability */

	int			colorBits, depthBits, stencilBits;

	glDriverType_t		driverType;
	glHardwareType_t	hardwareType;

	qbool			deviceSupportsGamma;
	textureCompression_t	textureCompression;
	qbool			textureEnvAddAvailable;

	int			vidWidth, vidHeight;
	/* aspect is the screen's physical width / height, which may be different
	 * than scrWidth / scrHeight if the pixels are non-square
	 * normal screens should be 4/3, but wide aspect monitors may be 16/9 */
	float	windowAspect;

	int	displayFrequency;

	/* synonymous with "does rendering consume the entire screen?", therefore
	 * a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	 * used CDS. */
	qbool		isFullscreen;
	qbool		stereoEnabled;
	qbool		smpActive;	/* dual processor */
} Glconfig;


typedef struct refimport_t refimport_t;	/* funcs imported by the refresh module */
typedef struct refexport_t refexport_t;	/* funcs exported by the refresh module */
/*
 * this is the only function actually exported at the linker level
 * If the module can't init to a valid rendering state, NULL will be
 * returned.
 */
#ifdef USE_RENDERER_DLOPEN
typedef refexport_t* (QDECL *GetRefAPI_t)(int apiVersion, refimport_t * rimp);
#else
refexport_t* GetRefAPI(int apiVersion, refimport_t *rimp);
#endif

struct refexport_t {
	/* called before the library is unloaded
	 * if the system is just reconfiguring, pass destroyWindow = qfalse,
	 * which will keep the screen from flashing to the desktop. */
	void (*Shutdown)(qbool destroyWindow);

	/* All data that will be used in a level should be
	 * registered before rendering any frames to prevent disk hits,
	 * but they can still be registered at a later time
	 * if necessary.
	 *
	 * BeginRegistration makes any existing media pointers invalid
	 * and returns the current gl configuration, including screen width
	 * and height, which can be used by the client to intelligently
	 * size display elements */
	void (*BeginRegistration)(Glconfig *config);
	Handle (*RegisterModel)(const char *name);
	Handle (*RegisterSkin)(const char *name);
	Handle (*RegisterShader)(const char *name);
	Handle (*RegisterShaderNoMip)(const char *name);
	void (*LoadWorld)(const char *name);

	/* the vis data is a large enough block of data that we go to the trouble
	 * of sharing it with the clipmodel subsystem */
	void (*SetWorldVisData)(const byte *vis);

	/* EndRegistration will draw a tiny polygon with each texture, forcing
	 * them to be loaded into card memory */
	void (*EndRegistration)(void);

	/* a scene is built up by calls to R_ClearScene and the various R_Add functions.
	 * Nothing is drawn until R_RenderScene is called. */
	void (*ClearScene)(void);
	void (*AddRefEntityToScene)(const Refent *re);
	void (*AddPolyToScene)(Handle hShader, int numVerts, const Polyvert *verts, int num);
	int (*LightForPoint)(Vec3 point, Vec3 ambientLight, Vec3 directedLight, Vec3 lightDir);
	void (*AddLightToScene)(const Vec3 org, float intensity, float r, float g, float b);
	void (*AddAdditiveLightToScene)(const Vec3 org, float intensity, float r, float g, float b);
	void (*RenderScene)(const Refdef *fd);

	void (*SetColor)(const float *rgba);	/* NULL = 1,1,1,1 */
	void (*DrawStretchPic)(float x, float y, float w, float h,
			       float s1, float t1, float s2, float t2, Handle hShader);	/* 0 = white */

	/* Draw images for cinematic rendering, pass as 32 bit rgba */
	void (*DrawStretchRaw)(int x, int y, int w, int h, int cols, int rows, const byte *data, int client,
			       qbool dirty);
	void (*UploadCinematic)(int w, int h, int cols, int rows, const byte *data, int client,
				qbool dirty);

	void (*BeginFrame)(Stereoframe stereoFrame);

	/* if the pointers are not NULL, timing info will be returned */
	void (*EndFrame)(int *frontEndMsec, int *backEndMsec);

	int (*MarkFragments)(int numPoints, const Vec3 *points, const Vec3 projection,
			     int maxPoints, Vec3 pointBuffer, int maxFragments,
			     Markfrag *fragmentBuffer);

	int (*LerpTag)(Orient *tag,  Handle model, int startFrame, int endFrame,
		       float frac, const char *tagName);
	void (*ModelBounds)(Handle model, Vec3 mins, Vec3 maxs);
	
	void (*RegisterFont)(const char *fontName, int pointSize, Fontinfo *font);
	void (*RemapShader)(const char *oldShader, const char *newShader, const char *offsetTime);
	qbool (*GetEntityToken)(char *buffer, int size);
	qbool (*inPVS)(const Vec3 p1, const Vec3 p2);

	void (*TakeVideoFrame)(int h, int w, byte* captureBuffer, byte *encodeBuffer, qbool motionJpeg);
};

struct refimport_t {
	/* print message on the local console */
	void (QDECL *Printf)(int printLevel, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

	/* abort the game */
	void (QDECL *Error)(int errorLevel, const char *fmt,
			    ...) __attribute__ ((noreturn, format (printf, 2, 3)));

	/* milliseconds should only be used for profiling, never
	 * for anything game related.  Get time from the refdef */
	int (*Milliseconds)(void);

	/* stack based memory allocation for per-level things that
	 * won't be freed */
	#ifdef HUNK_DEBUG
	void    *(*Hunk_AllocDebug)(int size, ha_pref pref, char *label, char *file, int line);
	#else
	void    *(*Hunk_Alloc)(int size, ha_pref pref);
	#endif
	void    *(*Hunk_AllocateTempMemory)(int size);
	void (*Hunk_FreeTempMemory)(void *block);

	/* dynamic memory allocator for things that need to be freed */
	void    *(*Malloc)(int bytes);
	void (*Free)(void *buf);

	Cvar  *(*Cvar_Get)(const char *name, const char *value, int flags);
	void (*Cvar_Set)(const char *name, const char *value);
	void (*Cvar_SetValue)(const char *name, float value);
	void (*Cvar_CheckRange)(Cvar *cv, float minVal, float maxVal, qbool shouldBeIntegral);

	int (*Cvar_VariableIntegerValue)(const char *var_name);

	void (*Cmd_AddCommand)(const char *name, void (*cmd)(void));
	void (*Cmd_RemoveCommand)(const char *name);

	int (*Cmd_Argc)(void);
	char    *(*Cmd_Argv)(int i);

	void (*Cmd_ExecuteText)(int exec_when, const char *text);

	byte    *(*CM_ClusterPVS)(int cluster);

	/* visualization for debugging collision detection */
	void (*CM_DrawDebugSurface)(void (*drawPoly)(int color, int numPoints, float *points));

	/* a -1 return means the file does not exist
	 * NULL can be passed for buf to just determine existance */
	int (*FS_FileIsInPAK)(const char *name, int *pCheckSum);
	long (*FS_ReadFile)(const char *name, void **buf);
	void (*FS_FreeFile)(void *buf);
	char ** (*FS_ListFiles)(const char *name, const char *extension, int *numfilesfound);
	void (*FS_FreeFileList)(char **filelist);
	void (*FS_WriteFile)(const char *qpath, const void *buffer, int size);
	qbool (*FS_FileExists)(const char *file);

	/* cinematic stuff */
	void (*CIN_UploadCinematic)(int handle);
	int (*CIN_PlayCinematic)(const char *arg0, int xpos, int ypos, int width, int height, int bits);
	Cinstatus (*CIN_RunCinematic)(int handle);

	void (*CL_WriteAVIVideoFrame)(const byte *buffer, int size);

	/* input event handling */
	void (*IN_Init)(void);
	void (*IN_Shutdown)(void);
	void (*IN_Restart)(void);

	/* math */
	long (*ftol)(float f);

	/* system stuff */
	void (*Sys_SetEnv)(const char *name, const char *value);
	void (*Sys_GLimpSafeInit)(void);
	void (*Sys_GLimpInit)(void);
	qbool (*Sys_LowPhysicalMemory)(void);
};

#endif	/* __TR_PUBLIC_H */
