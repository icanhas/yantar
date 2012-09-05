/* included first by ALL program modules. */
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
#ifndef __Q_SHARED_H
#define __Q_SHARED_H

#define PRODUCT_NAME		"yantar"
#define BASEGAME		"base"
#define BASETA			"missionpack"	/* remove this */
#define CLIENT_WINDOW_TITLE	"Konkrete"
#define CLIENT_WINDOW_MIN_TITLE "Konkrete"
#define HOMEPATH_NAME_UNIX	".konkrete"
#define HOMEPATH_NAME_WIN	"Konkrete"
#define HOMEPATH_NAME_MACOSX	HOMEPATH_NAME_WIN
/* must not contain whitespace */
#define GAMENAME_FOR_MASTER	"Konkrete"

/*
 * Heartbeat for dpmaster protocol. You shouldn't change 
 * this unless you know what you're doing
 */
#define HEARTBEAT_FOR_MASTER	"DarkPlaces"

#ifndef PRODUCT_VERSION
#define PRODUCT_VERSION "???"
#endif

#define Q3_VERSION		PRODUCT_NAME " " PRODUCT_VERSION

#define MAX_TEAMNAME		32
#define MAX_MASTER_SERVERS	5	/* number of supported master servers */

#define DEMOEXT			"dem_"	/* standard demo extension */

/* Ignore __attribute__ on non-gcc platforms */
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#if (defined _MSC_VER)
#define Q_EXPORT	__declspec(dllexport)
#elif (defined __SUNPRO_C)
#define Q_EXPORT	__global
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
#define Q_EXPORT	__attribute__((visibility("default")))
#else
#define Q_EXPORT
#endif

/*
 * VM Considerations
 *
 *  The VM can not use the standard system headers because we aren't really
 *  using the compiler they were meant for.  We use bg_lib.h which contains
 *  prototypes for the functions we define for our own use in bg_lib.c.
 *
 *  When writing mods, please add needed headers HERE, do not start including
 *  stuff like <stdio.h> in the various .c files that make up each of the VMs
 *  since you will be including system headers files can will have issues.
 *
 *  Remember, if you use a C library function that is not defined in bg_lib.c,
 *  you will have to add your own version for support in the VM.
 *
 */
#ifdef Q3_VM

#include "../game/bg_lib.h"

typedef int intptr_t;

#else

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#ifdef _MSC_VER
  #include <io.h>

typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8 int8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

/*
 * vsnprintf is ISO/IEC 9899:1999
 * abstracting this to make it portable
 */
int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#else
  #include <stdint.h>

  #define Q_vsnprintf vsnprintf
#endif

#endif

#include "q_paths.h"

#include "q_platform.h"

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef enum {
	qfalse,
	qtrue
} qbool;

typedef union {
	float		f;
	int		i;
	unsigned int	ui;
} floatint_t;

typedef int qhandle_t;
typedef int sfxHandle_t;
typedef int fileHandle_t;
typedef int clipHandle_t;

#define PAD(base, alignment)	(((base)+(alignment)-1) & ~((alignment)-1))
#define PADLEN(base, alignment) (PAD((base), (alignment)) - (base))

#define PADP(base, alignment)	((void*)PAD((intptr_t)(base), (alignment)))

#ifdef __GNUC__
#define QALIGN(x) __attribute__((aligned(x)))
#else
#define QALIGN(x)
#endif

#ifndef UNUSED
#	define UNUSED(x) (void)(x)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define nil NULL

#define STRING(s)	# s
/* expand constants before stringifying them */
#define XSTRING(s)	STRING(s)

#define MAX_QINT	0x7fffffff
#define MIN_QINT	(-MAX_QINT-1)

#define ARRAY_LEN(x)	(sizeof(x) / sizeof(*(x)))
#define STRARRAY_LEN(x) (ARRAY_LEN(x) - 1)

enum {
	/* angle indexes */
	PITCH,	/* up / down */
	YAW,	/* left / right */
	ROLL,	/* fall over */

	/*
	 * the game guarantees that no string from the network will ever
	 * exceed MAX_STRING_CHARS
	 */
	MAX_STRING_CHARS = 1024,	/* max len of a string passed
					 * to Cmd_TokenizeString */
	MAX_STRING_TOKENS = 1024,	/* max tokens resulting
					 * from Cmd_TokenizeString */
	MAX_TOKEN_CHARS = 1024,		/* max length of an individual token */

	MAX_INFO_STRING = 1024,
	MAX_INFO_KEY = 1024,
	MAX_INFO_VALUE = 1024,

	BIG_INFO_STRING = 8192,	/* used for system info key only */
	BIG_INFO_KEY = 8192,
	BIG_INFO_VALUE = 8192,

	MAX_QPATH = 64,	/* max length of a quake game pathname */
#ifdef PATH_MAX
	MAX_OSPATH = PATH_MAX,
#else
	MAX_OSPATH = 256,	/* max length of a filesystem pathname */
#endif

	MAX_NAME_LENGTH = 32,	/* max length of a client name */

	MAX_SAY_TEXT = 150
};

/* paramters for command buffer stuffing */
typedef enum {
	EXEC_NOW,	/* don't return until completed, a VM should NEVER use this, */
	/* because some commands might cause the VM to be unloaded... */
	EXEC_INSERT,	/* insert at current position, but don't run yet */
	EXEC_APPEND	/* add to end of the command buffer (normal case) */
} cbufExec_t;

/* these aren't needed by any of the VMs.  put in another header? */
#define MAX_MAP_AREA_BYTES 32	/* bit vector of area visibility */

/* print levels from renderer (FIXME: set up for game / cgame?) */
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER,	/* only print when "developer 1" */
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;

#ifdef ERR_FATAL
#undef ERR_FATAL	/* this is be defined in malloc.h */
#endif

/* parameters to the main Error routine */
typedef enum {
	ERR_FATAL,		/* exit the entire game with a popup window */
	ERR_DROP,		/* print to console and disconnect from game */
	ERR_SERVERDISCONNECT,	/* don't kill server */
	ERR_DISCONNECT,		/* client disconnected from the server */
	ERR_NEED_CD		/* pop up the need-cd dialog */
} errorParm_t;

/* font rendering values used by ui and cgame */
#define PROP_SMALL_SIZE_SCALE 0.75

enum {
	PROP_GAP_WIDTH	= 3,
	PROP_SPACE_WIDTH= 8,
	PROP_HEIGHT	= 27,

	BLINK_DIVISOR	= 200,
	PULSE_DIVISOR	= 75,

	UI_LEFT = 0,	/* default */
	UI_CENTER = 0x1,
	UI_RIGHT = 0x2,
	UI_FORMATMASK	= 0x7,
	UI_SMALLFONT	= 0x10,
	UI_BIGFONT	= 0x20,	/* default */
	UI_GIANTFONT	= 0x40,
	UI_DROPSHADOW	= 0x800,
	UI_BLINK	= 0x1000,
	UI_INVERSE	= 0x2000,
	UI_PULSE	= 0x4000
};

/* memory */
#if !defined(NDEBUG) && !defined(BSPC)
#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

#ifdef HUNK_DEBUG
#define Hunk_Alloc(size, preference) Hunk_AllocDebug(size, preference, # size, \
	__FILE__, \
	__LINE__)
void*	Hunk_AllocDebug(int size, ha_pref preference, char *label, char *file,
		     int line);
#else
void*	Hunk_Alloc(int size, ha_pref preference);
#endif

#define Q_Memset memset
#define Q_Memcpy memcpy

/* cin */
enum {
	CIN_system	= 1,
	CIN_loop	= 2,
	CIN_hold	= 4,
	CIN_silent	= 8,
	CIN_shader	= 16
};

/* mathlib */
typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];
typedef vec_t mat4x4[16];
typedef vec4_t quat_t;

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846f	/* matches value in gcc v2 math.h */
#endif

#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

/*
 * all drawing is done to a 640*480 virtual screen size and will be
 * automatically scaled to the real resolution
 */
enum {
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 480,
	SMALLCHAR_WIDTH = 8,
	SMALLCHAR_HEIGHT = 16,
	TINYCHAR_WIDTH = SMALLCHAR_WIDTH,
	TINYCHAR_HEIGHT = SMALLCHAR_HEIGHT / 2,
	BIGCHAR_WIDTH = 16,
	BIGCHAR_HEIGHT = 16,
	GIANTCHAR_WIDTH = 32,
	GIANTCHAR_HEIGHT = 48
};

extern vec4_t	colorBlack;
extern vec4_t	colorRed;
extern vec4_t	colorGreen;
extern vec4_t	colorBlue;
extern vec4_t	colorYellow;
extern vec4_t	colorMagenta;
extern vec4_t	colorCyan;
extern vec4_t	colorWhite;
extern vec4_t	colorLtGrey;
extern vec4_t	colorMdGrey;
extern vec4_t	colorDkGrey;

#define Q_COLOR_ESCAPE '^'
#define Q_IsColorString(p) ((p) && *(p) == Q_COLOR_ESCAPE && *((p)+1) && \
			    isalnum(*((p)+1)))	/* ^[0-9a-zA-Z] */

#define COLOR_BLACK	'0'
#define COLOR_RED	'1'
#define COLOR_GREEN	'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE	'4'
#define COLOR_CYAN	'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE	'7'
#define ColorIndex(c) (((c) - '0') & 0x07)

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED	"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA "^6"
#define S_COLOR_WHITE	"^7"

extern vec4_t g_color_table[8];

#define MAKERGB(v, r, g, b)	v[0]	=r; v[1]=g; v[2]=b
#define MAKERGBA(v, r, g, b, a) v[0]	=r; v[1]=g; v[2]=b; v[3]=a

#define DEG2RAD(a)		(((a) * M_PI) / 180.0F)
#define RAD2DEG(a)		(((a) * 180.0f) / M_PI)

struct cplane_s;

extern vec3_t	vec3_origin;
extern vec3_t	axisDefault[3];

#define nanmask (255 << 23)

#define IS_NAN(x) (((*(int*)&x)&nanmask)==nanmask)

int Q_isnan(float x);

#if idx64
extern long qftolsse(float f);
extern int qvmftolsse(void);
extern void qsnapvectorsse(vec3_t vec);

#define Q_ftol		qftolsse
#define Q_SnapVector	qsnapvectorsse

extern int (*Q_VMftol)(void);

#elif id386
extern long QDECL qftolx87(float f);
extern long QDECL qftolsse(float f);
extern int QDECL qvmftolx87(void);
extern int QDECL qvmftolsse(void);
extern void QDECL qsnapvectorx87(vec3_t vec);
extern void QDECL qsnapvectorsse(vec3_t vec);

extern long	(QDECL *Q_ftol)(float f);
extern int	(QDECL *Q_VMftol)(void);
extern void	(QDECL *Q_SnapVector)(vec3_t vec);

#else
/* Q_ftol must expand to a function name so the pluggable renderer can take
 * its address */
  #define Q_ftol lrintf
  #define Q_SnapVector(vec) \
	do \
	{ \
		vec3_t *temp = (vec); \
		\
		(*temp)[0]	= round((*temp)[0]); \
		(*temp)[1]	= round((*temp)[1]); \
		(*temp)[2]	= round((*temp)[2]); \
	} while(0)
#endif
/*
 * if your system does not have lrintf() and round() you can try this block.
 * Please also open a bug report at bugzilla.icculus.org or write a mail to the 
 * ioq3 mailing list.
 * #else
 * #define Q_ftol(v) ((long) (v))
 * #define Q_round(v) do { if((v) < 0) (v) -= 0.5f; else (v) += 0.5f; (v) = Q_ftol((v)); } while(0)
 * #define Q_SnapVector(vec) \
 *      do\
 *      {\
 *              vec3_t *temp = (vec);\
 \
 \              Q_round((*temp)[0]);\
 \              Q_round((*temp)[1]);\
 \              Q_round((*temp)[2]);\
 \      } while(0)
 \\#endif
 */

#if idppc

static ID_INLINE float
Q_rsqrt(float number)
{
	float	x = 0.5f * number;
	float	y;
#ifdef __GNUC__
	asm ("frsqrte %0,%1" : "=f" (y) : "f" (number));
#else
	y = __frsqrte(number);
#endif
	return y * (1.5f - (x * y * y));
}

#ifdef __GNUC__
static ID_INLINE float
Q_fabs(float x)
{
	float abs_x;

	asm ("fabs %0,%1" : "=f" (abs_x) : "f" (x));
	return abs_x;
}
#else
#define Q_fabs __fabsf
#endif

#else
float Q_fabs(float f);
float Q_rsqrt(float f);	/* reciprocal square root */
#endif

#define SQRTFAST(x) ((x) * Q_rsqrt(x))

signed char ClampChar(int i);
signed short ClampShort(int i);

/* this isn't a real cheap function to call! */
int DirToByte(vec3_t dir);
void ByteToDir(int b, vec3_t dir);

#if     0

#define Vec3Dot(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define Vec3Sub(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1], \
				 (c)[2]=(a)[2]-(b)[2])
#define Vec3Add(a,b,c)	((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1], \
				 (c)[2]=(a)[2]+(b)[2])
#define Vec3Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]= \
					 (v)[2]*(s))
#define Vec3MA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*	\
								  (s),(o)[2]= \
					 (v)[2]+(b)[2]*(s))

#else

#define Vec3Dot(x,y)		_Vec3Dot(x,y)
#define Vec3Sub(a,b,c)	_Vec3Sub(a,b,c)
#define Vec3Add(a,b,c)	_Vec3Add(a,b,c)
#define Vec3Copy(a,b)		_Vec3Copy(a,b)
#define VectorScale(v, s, o)	_VectorScale(v,s,o)
#define Vec3MA(v, s, b, o)	_Vec3MA(v,s,b,o)

#endif

#ifdef Q3_VM
#ifdef Vec3Copy
#undef Vec3Copy

/* this is a little hack to get more efficient copies in our interpreter */
typedef struct {
	float v[3];
} vec3struct_t;

#define Vec3Copy(a,b) (*(vec3struct_t*)b=*(vec3struct_t*)a)
#endif
#endif

#define VectorClear(a)		((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(a,b)	((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define VectorSet(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define Vector4Copy(a,b)	((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2], \
				 (b)[3]=(a)[3])

#define Byte4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2], \
				 (b)[3]=(a)[3])

#define SnapVector(v)		{v[0]=((int)(v[0])); v[1]=((int)(v[1])); v[2]= \
					 ((int)(v[2])); }
/* just in case you don't want to use the macros */
vec_t	_Vec3Dot(const vec3_t v1, const vec3_t v2);
void	_Vec3Sub(const vec3_t veca, const vec3_t vecb, vec3_t out);
void	_Vec3Add(const vec3_t veca, const vec3_t vecb, vec3_t out);
void	_Vec3Copy(const vec3_t in, vec3_t out);
void	_VectorScale(const vec3_t in, float scale, vec3_t out);
void	_Vec3MA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);
void	Vec3Lerp(const vec3_t a, const vec3_t b, float lerp, vec3_t c);

void	Mat4x4ToZero(mat4x4 out);
void	Mat4x4ToIdentity(mat4x4 out);
void	Mat4x4Copy(const mat4x4 in, mat4x4 out);
void	Mat4x4Mul(const mat4x4 in1, const mat4x4 in2, mat4x4 out);
void	Mat4x4Transform(const mat4x4 in1, const vec4_t in2, vec4_t out);
qbool	Mat4x4Compare(const mat4x4 a, const mat4x4 b);
void	Mat4x4Translation(vec3_t vec, mat4x4 out);
void	Mat4x4Ortho(float left, float right, float bottom, float top, float znear, float zfar, mat4x4 out);

void QuatMul(const quat_t, const quat_t, quat_t);

unsigned ColorBytes3(float r, float g, float b);
unsigned ColorBytes4(float r, float g, float b, float a);

float NormalizeColor(const vec3_t in, vec3_t out);

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);

#if !defined(Q3_VM) || (defined(Q3_VM) && defined(__Q3_VM_MATH))
static ID_INLINE int
VectorCompare(const vec3_t v1, const vec3_t v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return 0;
	return 1;
}

static ID_INLINE vec_t
Vec3Len(const vec3_t v)
{
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static ID_INLINE vec_t
Vec3LenSquared(const vec3_t v)
{
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static ID_INLINE vec_t
Vec3Distance(const vec3_t p1, const vec3_t p2)
{
	vec3_t v;

	Vec3Sub (p2, p1, v);
	return Vec3Len(v);
}

static ID_INLINE vec_t
Vec3DistanceSquared(const vec3_t p1, const vec3_t p2)
{
	vec3_t v;

	Vec3Sub (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

/* fast vector normalize routine that does not check to make sure
 * that length != 0, nor does it return length, uses rsqrt approximation */
static ID_INLINE void
Vec3NormalizeFast(vec3_t v)
{
	float ilength;

	ilength = Q_rsqrt(Vec3Dot(v, v));

	v[0]	*= ilength;
	v[1]	*= ilength;
	v[2]	*= ilength;
}

static ID_INLINE void
Vec3Inverse(vec3_t v)
{
	v[0]	= -v[0];
	v[1]	= -v[1];
	v[2]	= -v[2];
}

static ID_INLINE void
Vec3Cross(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0]	= v1[1]*v2[2] - v1[2]*v2[1];
	cross[1]	= v1[2]*v2[0] - v1[0]*v2[2];
	cross[2]	= v1[0]*v2[1] - v1[1]*v2[0];
}

#else
int VectorCompare(const vec3_t v1, const vec3_t v2);

vec_t Vec3Len(const vec3_t v);

vec_t Vec3LenSquared(const vec3_t v);

vec_t Vec3Distance(const vec3_t p1, const vec3_t p2);

vec_t Vec3DistanceSquared(const vec3_t p1, const vec3_t p2);

void Vec3NormalizeFast(vec3_t v);

void Vec3Inverse(vec3_t v);

void Vec3Cross(const vec3_t v1, const vec3_t v2, vec3_t cross);

#endif

vec_t Vec3Normalize(vec3_t v);	/* returns vector length */
vec_t Vec3Normalize2(const vec3_t v, vec3_t out);
void Vec4Scale(const vec4_t in, vec_t scale, vec4_t out);
void Vec3Rotate(vec3_t in, vec3_t matrix[3], vec3_t out);
int Q_log2(int val);

float Q_acos(float c);

int             Q_rand(int *seed);
float   Q_random(int *seed);
float   Q_crandom(int *seed);

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

void Vec3ToAngles(const vec3_t value1, vec3_t angles);
void AnglesToQuat(const vec3_t, quat_t);
void quat2euler(const quat_t, vec3_t);
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]);
void QuatToAxis(quat_t, vec3_t axis[3]);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

void QuatSet(quat_t, vec_t w, vec_t x, vec_t y, vec_t z);

void SetPlaneSignbits(struct cplane_s *out);
int BoxOnPlaneSide(const vec3_t emins, const vec3_t emaxs, const struct cplane_s *plane);

qbool BoundsIntersect(const vec3_t mins, const vec3_t maxs,
			 const vec3_t mins2, const vec3_t maxs2);
qbool BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs,
			       const vec3_t origin, vec_t radius);
qbool BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs,
			      const vec3_t origin);

float   AngleMod(float a);
float   LerpAngle(float from, float to, float frac);
float   AngleSubtract(float a1, float a2);
void    AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3);

float AngleNormalize360(float angle);
float AngleNormalize180(float angle);
float AngleDelta(float angle1, float angle2);

qbool PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b,
			 const vec3_t c);
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point,
			     float degrees);
void RotateAroundDirection(vec3_t axis[3], float yaw);
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up);
/* perpendicular vector could be replaced by this */

/* int	PlaneTypeForNormal (vec3_t normal); */

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);
void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector(vec3_t dst, const vec3_t src);

#ifndef MAX
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#endif

/* common */
long			Q_hashstr(const char *s, int size);
float			Q_clamp(float min, float max, float value);
char*		Q_skippath(char *pathname);
const char*	Q_getext(const char *name);
void			Q_stripext(const char *in, char *out, int destsize);
qbool		Q_cmpext(const char *in, const char *ext);
void			Q_defaultext(char *path, int maxSize, const char *extension);

char*		Q_readtok(char **data_p);
char*		Q_readtok2(char **data_p, qbool allowLineBreak);
int         		Q_compresstr(char *data_p);

enum {
	/* token types */
	TT_STRING,
	TT_LITERAL,
	TT_NUMBER,
	TT_NAME,
	TT_PUNCTUATION,

	MAX_TOKENLENGTH = 1024
};

typedef struct pc_token_s {
	int	type;
	int	subtype;
	int	intvalue;
	float	floatvalue;
	char	string[MAX_TOKENLENGTH];
} pc_token_t;

/* data is an in/out parm, returns a parsed out token */
void    Q_matchtok(char**buf_p, char *match);

void    Q_skipblock(char **program);
void    Q_skipline(char **data);

void    Parse1DMatrix(char **buf_p, int x, float *m);
void    Parse2DMatrix(char **buf_p, int y, int x, float *m);
void    Parse3DMatrix(char **buf_p, int z, int y, int x, float *m);
int     Q_hexstr2int(const char *str);

int QDECL       Q_sprintf(char *dest, int size, 
	const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

char*Q_skiptoks(char *s, int numTokens, char *sep);
char*Q_skipcharset(char *s, char *sep);

void    Com_Randombytes(byte *string, int len);

/* mode parm for FS_FOpenFile */
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

/*
 * unicode (q_utf.c)
 */
typedef uint rune_t;	/* a unicode code point */

enum {
	Runemax		= 0x10ffff,	/* max legal code point as defined by Unicode 6.x */
	Runesync	= 0x80,		/* if <, byte can't be part of a UTF sequence */
	Runeself		= 0x80,		/* if <, rune and UTF sequence are the same */
	Runeerror	= 0xfffd,		/* '�' represents a bad UTF sequence */
};

int		Q_runetochar(char*, rune_t*);
int		Q_chartorune(rune_t*, char*);
int		Q_runelen(ulong);
int		Q_fullrune(char*, int);
size_t	Q_utflen(char*);
char*	Q_utfrune(char*, ulong);
char*	Q_utfutf(char*, char*);

rune_t*	Q_runestrcat(rune_t*, rune_t*);
rune_t*	Q_runestrchr(rune_t*, rune_t);
int		Q_runestrcmp(rune_t*, rune_t*);
rune_t*	Q_runestrcpy(rune_t*, rune_t*);
rune_t*	Q_runestrncpy(rune_t*, rune_t*, size_t);
rune_t*	Q_runestrncat(rune_t*, rune_t*, size_t);
int		Q_runestrncmp(rune_t*, rune_t*, size_t);
size_t	Q_runestrlen(rune_t*);
rune_t*	Q_runestrstr(rune_t*, rune_t*);

rune_t	Q_tolowerrune(rune_t);
rune_t	Q_totitlerune(rune_t);
rune_t	Q_toupperrune(rune_t);
int		Q_isalpharune(rune_t);
int		Q_isdigitrune(rune_t);
int		Q_islowerrune(rune_t);
int		Q_isspacerune(rune_t);
int		Q_istitlerune(rune_t);
int		Q_isupperrune(rune_t);

/*
 * strings
 */
int Q_isprint(int c);
int Q_islower(int c);
int Q_isupper(int c);
int Q_isalpha(int c);
qbool Q_isanumber(const char *s);
qbool Q_isintegral(float f);

/* portable case insensitive compare */
int             Q_stricmp(const char *s1, const char *s2);
int             Q_strncmp(const char *s1, const char *s2, int n);
int             Q_stricmpn(const char *s1, const char *s2, int n);
char*		Q_strlwr(char *s1);
char*		Q_strupr(char *s1);
const char*	Q_stristr(const char *s, const char *find);

/* buffer size safe library replacements */
void		Q_strncpyz(char *dest, const char *src, int destsize);
void		Q_strcat(char *dest, int size, const char *src);

/* strlen that discounts Quake color sequences */
int		Q_printablelen(const char *string);
/* removes color sequences from string */
char*		Q_cleanstr(char *string);
/* Count the number of char tocount encountered in string */
int 		Q_countchar(const char *string, char tocount);

/*
 * 64-bit integers for global rankings interface
 * implemented as a struct for qvm compatibility
 */
typedef struct {
	byte	b0;
	byte	b1;
	byte	b2;
	byte	b3;
	byte	b4;
	byte	b5;
	byte	b6;
	byte	b7;
} qint64;

/*
 * short	BigShort(short l);
 * short	LittleShort(short l);
 * int		BigLong (int l);
 * int		LittleLong (int l);
 * qint64	BigLong64 (qint64 l);
 * qint64	LittleLong64 (qint64 l);
 * float	BigFloat (const float *l);
 * float	LittleFloat (const float *l);
 *
 * void	Swap_Init (void);
 */
char    *QDECL va(char *format, ...) __attribute__ ((format (printf, 1, 2)));

enum {
	TRUNCATE_LENGTH = 64
};

void Q_truncstr(char *buffer, const char *s);

/* key / value info strings */
char*Info_ValueForKey(const char *s, const char *key);
void Info_RemoveKey(char *s, const char *key);
void Info_RemoveKey_big(char *s, const char *key);
void Info_SetValueForKey(char *s, const char *key, const char *value);
void Info_SetValueForKey_Big(char *s, const char *key, const char *value);
qbool Info_Validate(const char *s);
void Info_NextPair(const char **s, char *key, char *value);

/* this is only here so the functions in q_shared.c and bg_*.c can link */
void QDECL Com_Errorf(int level, const char *error,
		     ...) __attribute__ ((noreturn, format(printf, 2, 3)));
void QDECL Com_Printf(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));


/*
 * cvars -- console variables
 *
 * Many variables can be used for cheating purposes, so when
 * cheats is zero, force all unspecified variables to their
 * default values.
 */
enum {
	MAX_CVAR_VALUE_STRING = 256,

	/* flags */
	CVAR_ARCHIVE = 1 << 0,		/* set to cause it to be saved to vars.rc
					 * used for system variables, not for player
					 * specific configurations */
	CVAR_USERINFO = 1 << 1,		/* sent to server on connect or change */
	CVAR_SERVERINFO = 1 << 2,	/* sent in response to front end requests */
	CVAR_SYSTEMINFO = 1 << 3,	/* these cvars will be duplicated on all clients */
	CVAR_INIT = 1 << 4,		/* don't allow change from console at all,
					 * but can be set from the command line */
	CVAR_LATCH = 1 << 5,		/* will only change when C code next does
					 * a Cvar_Get(), so it can't be changed
					 * without proper initialization.  modified
					 * will be set, even though the value hasn't
					 * changed yet */
	CVAR_ROM = 1 << 6,		/* display only, cannot be set by user at all */
	CVAR_USER_CREATED = 1 << 7,	/* created by a set command */
	CVAR_TEMP = 1 << 8,		/* can be set even when cheats are disabled, but is not archived */
	CVAR_CHEAT = 1 << 9,		/* can not be changed if cheats are disabled */
	CVAR_NORESTART = 1 << 10,	/* do not clear when a cvar_restart is issued */

	CVAR_SERVER_CREATED = 1 << 11,	/* cvar was created by a server the client connected to. */
	CVAR_VM_CREATED = 1 << 12,	/* cvar was created exclusively in one of the VMs. */
	CVAR_PROTECTED = 1 << 13,	/* prevent modifying this var from VMs or the server */
	/* These flags are only returned by the Cvar_Flags() function */
	CVAR_MODIFIED = 1 << 30,	/* Cvar was modified */
	CVAR_NONEXISTENT = 1 << 31	/* Cvar doesn't exist. */
};

/* nothing outside the Cvar_*() functions should modify these fields! */
typedef struct cvar_s cvar_t;
struct cvar_s {
	char		*name;
	char		*desc;	/* description string */
	char		*string;
	char		*resetString;		/* cvar_restart will reset to this value */
	char                    *latchedString;	/* for CVAR_LATCH vars */
	int		flags;
	qbool		modified;		/* set each time the cvar is changed */
	int		modificationCount;	/* incremented each time the cvar is changed */
	float		value;			/* atof( string ) */
	int		integer;		/* atoi( string ) */
	qbool		validate;
	qbool		integral;
	float		min;
	float		max;

	cvar_t		*next;
	cvar_t		*prev;
	cvar_t		*hashNext;
	cvar_t		*hashPrev;
	int		hashIndex;
};

typedef int cvarHandle_t;

/*
 * the modules that run in the virtual machine can't access the cvar_t directly,
 * so they must ask for structured updates
 */
typedef struct {
	cvarHandle_t	handle;
	int		modificationCount;
	float		value;
	int		integer;
	char		string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/* VoIP */
enum {
	/* if you change the count of flags be sure to also change VOIP_FLAGNUM */
	VOIP_SPATIAL = 0x01,	/* spatialized voip message */
	VOIP_DIRECT = 0x02,	/* non-spatialized voip message */

	/*
	 * number of flags voip knows. You will have to bump protocol version
	 * number if you change this.
	 */
	VOIP_FLAGCNT = 2
};

/* collision detection */
#include "surfaceflags.h"	/* shared with the q3map utility */

/*
 * plane types are used to speed some tests
 * 0-2 are axial planes
 */
enum {
	PLANE_X,
	PLANE_Y,
	PLANE_Z,
	PLANE_NON_AXIAL
};

#define PlaneTypeForNormal(x) \
	(x[0] == 1.0 ? PLANE_X : \
	(x[1] == 1.0 ? PLANE_Y : \
	(x[2] == 1.0 ? PLANE_Z : \
	PLANE_NON_AXIAL)))

/* plane_t structure
 * !!! if this is changed, it must be changed in asm code too !!! */
typedef struct cplane_s {
	vec3_t	normal;
	float	dist;
	byte	type;		/* for fast side tests: 0,1,2 = axial, 3 = nonaxial */
	byte	signbits;	/* signx + (signy<<1) + (signz<<2), used as lookup during collision */
	byte	pad[2];
} cplane_t;


/* a trace is returned when a box is swept through the world */
typedef struct {
	qbool		allsolid;	/* if true, plane is not valid */
	qbool		startsolid;	/* if true, the initial point was in a solid area */
	float		fraction;	/* time completed, 1.0 = didn't hit anything */
	vec3_t		endpos;		/* final position */
	cplane_t	plane;		/* surface normal at impact, transformed to world space */
	int		surfaceFlags;	/* surface hit */
	int		contents;	/* contents on other side of surface hit */
	int		entityNum;	/* entity the contacted sirface is a part of */
} trace_t;

/* trace->entityNum can also be 0 to (MAX_GENTITIES-1)
 * or ENTITYNUM_NONE, ENTITYNUM_WORLD */

/* markfragments are returned by CM_MarkFragments() */
typedef struct {
	int	firstPoint;
	int	numPoints;
} markFragment_t;

typedef struct {
	vec3_t	origin;
	vec3_t	axis[3];
} orientation_t;

/* keys/console */
/*
 * in order from highest priority to lowest
 * if none of the catchers are active, bound key strings will be executed
 */
enum {
	KEYCATCH_CONSOLE = 1 << 0,
	KEYCATCH_UI = 1 << 1,
	KEYCATCH_MESSAGE = 1 << 2,
	KEYCATCH_CGAME = 1 << 3
};

/* sound channels */
/*
 * channel 0 never willingly overrides
 * other channels will always override a playing sound on that channel
 */
typedef enum {
	CHAN_AUTO,
	CHAN_LOCAL,	/* menu sounds, etc */
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_LOCAL_SOUND,	/* chat messages, etc */
	CHAN_ANNOUNCER		/* announcer voices, etc */
} soundChannel_t;

/* elements communicated across the net */
#define ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)	((x)*(360.0/65536))

enum {
	SNAPFLAG_RATE_DELAYED	= 1,
	SNAPFLAG_NOT_ACTIVE	= 2,	/* snapshot used during connection and for zombies */
	SNAPFLAG_SERVERCOUNT	= 4,	/* toggled every map_restart so transitions can be detected */

	/* per-level limits */
	MAX_CLIENTS	= 64,	/* absolute limit */
	MAX_LOCATIONS	= 64,

	GENTITYNUM_BITS = 10,	/* don't need to send any more */
	MAX_GENTITIES	= (1 << GENTITYNUM_BITS),

	/*
	 * entitynums are communicated with GENTITY_BITS, so any reserved
	 * values that are going to be communcated over the net need to
	 * also be in this range
	 */
	ENTITYNUM_NONE		= (MAX_GENTITIES-1),
	ENTITYNUM_WORLD		= (MAX_GENTITIES-2),
	ENTITYNUM_MAX_NORMAL	= (MAX_GENTITIES-2),

	MAX_MODELS		= 256,	/* these are sent over the net as 8 bits */
	MAX_SOUNDS		= 256,	/* so they cannot be blindly increased */

	/*
	 * these are the only configstrings that the system reserves, all the
	 * other ones are strictly for servergame to clientgame communication
	 */
	CS_SERVERINFO		= 0,	/* an info string with all the serverinfo cvars */
	CS_SYSTEMINFO		= 1,	/* an info string for server system to client system configuration (timescale, etc) */

	RESERVED_CONFIGSTRINGS	= 2,	/* game can't modify below this, only the system can */

	MAX_GAMESTATE_CHARS	= 16000,
};

#define MAX_CONFIGSTRINGS 1024

typedef struct {
	int	stringOffsets[MAX_CONFIGSTRINGS];
	char	stringData[MAX_GAMESTATE_CHARS];
	int	dataCount;
} gameState_t;

/* bit field limits */
enum {
	MAX_STATS		= 16,
	MAX_PERSISTANT		= 16,
	MAX_POWERUPS		= 16,
	MAX_WEAPONS		= 16,

	MAX_PS_EVENTS		= 2,

	PS_PMOVEFRAMECOUNTBITS	= 6,
};

/*
 * playerState_t is the information needed by both the client and server
 * to predict player motion and actions
 * nothing outside of pmove should modify these, or some degree of prediction error
 * will occur
 */

/* you can't add anything to this without modifying the code in msg.c */

/*
 * playerState_t is a full superset of entityState_t as it is used by players,
 * so if a playerState_t is transmitted, the entityState_t can be fully derived
 * from it.
 */
typedef struct playerState_s {
	int	commandTime;	/* cmd->serverTime of last executed command */
	int	pm_type;
	int	bobCycle;	/* for view bobbing and footstep generation */
	int	pm_flags;	/* ducked, jump_held, etc */
	int	pm_time;

	vec3_t	origin;
	vec3_t	velocity;
	int	weaponTime;
	int	gravity;
	int	speed;
	float	swingstrength;		/* for grapple */
	int	delta_angles[3];	/* add to command angles to get view direction */
	/* changed by spawns, rotating objects, and teleporters */

	int	groundEntityNum;	/* ENTITYNUM_NONE = in air */

	int	legsTimer;	/* don't change low priority animations until this runs out */
	int	legsAnim;	/* mask off ANIM_TOGGLEBIT */

	int	torsoTimer;	/* don't change low priority animations until this runs out */
	int	torsoAnim;	/* mask off ANIM_TOGGLEBIT */

	int	movementDir;	/* a number 0 to 7 that represents the reletive angle */
	/* of movement to the view angle (axial and diagonals)
	 * when at rest, the value will remain unchanged
	 * used to twist the legs during strafing */

	vec3_t	grapplePoint;	/* location of grapple to pull towards if PMF_GRAPPLE_PULL */
	qbool	grapplelast;
	float		oldgrapplelen;

	int	eFlags;	/* copied to entityState_t->eFlags */

	int	eventSequence;	/* pmove generated events */
	int	events[MAX_PS_EVENTS];
	int	eventParms[MAX_PS_EVENTS];

	int	externalEvent;	/* events set on player from another source */
	int	externalEventParm;
	int	externalEventTime;

	int	clientNum;	/* ranges from 0 to MAX_CLIENTS-1 */
	int	weapon;		/* copied to entityState_t->weapon */
	int	weaponstate;

	vec3_t	viewangles;	/* for fixed views */
	int	viewheight;

	/* damage feedback */
	int	damageEvent;	/* when it changes, latch the other parms */
	int	damageYaw;
	int	damagePitch;
	int	damageCount;

	int	stats[MAX_STATS];
	int	persistant[MAX_PERSISTANT];	/* stats that aren't cleared on death */
	int	powerups[MAX_POWERUPS];		/* level.time that the powerup runs out */
	int	ammo[MAX_WEAPONS];

	int	generic1;
	int	loopSound;
	int	jumppad_ent;	/* jumppad entity hit this frame */

	/* not communicated over the net at all */
	int	ping;			/* server to game info for scoreboard */
	int	pmove_framecount;	/* FIXME: don't transmit over the network */
	int	jumppad_frame;
	int	entityEventSequence;
} playerState_t;

/*
 * usercmd_t->button bits, many of which are generated by the client system,
 * so they aren't game/cgame only definitions
 */
enum {
	BUTTON_ATTACK		= 1,
	BUTTON_TALK		= 2,	/* displays talk balloon and disables actions */
	BUTTON_USE_HOLDABLE	= 4,
	BUTTON_GESTURE		= 8,
	BUTTON_WALKING		= 16,	/* walking can't just be inferred from 
					 * MOVE_RUN, because a key pressed late
					 * in the frame will only generate a 
					 * small move value for that frame
					 * walking will use different 
					 * animations and won't generate
					 * footsteps */
	BUTTON_AFFIRMATIVE	= 32,
	BUTTON_NEGATIVE		= 64,

	BUTTON_GETFLAG		= 128,
	BUTTON_GUARDBASE	= 256,
	BUTTON_PATROL		= 512,
	BUTTON_FOLLOWME		= 1024,

	BUTTON_ANY		= 2048,	/* any key whatsoever */

	MOVE_RUN		= 120,	/* if forwardmove or rightmove >= MOVE_RUN,
					 * then BUTTON_WALKING should be set */
};

/* usercmd_t is sent to the server each client frame */
typedef struct usercmd_s {
	int		serverTime;
	int		angles[3];
	int		buttons;
	byte		weapon;	/* weapon */
	signed char	forwardmove, rightmove, upmove, brakefrac;
} usercmd_t;

/* entities */
/* if entityState->solid == SOLID_BMODEL, modelindex is an inline model number */
#define SOLID_BMODEL 0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,	/* non-parametric, but interpolate between snapshots */
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE,	/* value = base + sin( time / duration ) * delta */
	TR_GRAVITY
} trType_t;

typedef struct {
	trType_t	trType;
	int		trTime;
	int		trDuration;	/* if non 0, trTime + trDuration = stop time */
	vec3_t		trBase;
	vec3_t		trDelta;	/* velocity, etc */
} trajectory_t;

/* entityState_t is the information conveyed from the server
 * in an update message about entities that the client will
 * need to render in some way
 * Different eTypes may use the information in different ways
 * The messages are delta compressed, so it doesn't really matter if
 * the structure size is fairly large */
typedef struct entityState_s {
	int		number;	/* entity index */
	int		eType;	/* entityType_t */
	int		eFlags;

	trajectory_t	pos;	/* for calculating position */
	trajectory_t	apos;	/* for calculating angles */

	int		time;
	int		time2;

	vec3_t		origin;
	vec3_t		origin2;

	vec3_t		angles;
	vec3_t		angles2;

	int		otherEntityNum;	/* shotgun sources, etc */
	int		otherEntityNum2;

	int		groundEntityNum;	/* ENTITYNUM_NONE = in air */

	int		constantLight;	/* r + (g<<8) + (b<<16) + (intensity<<24) */
	int		loopSound;	/* constantly loop this sound */

	int		modelindex;
	int		modelindex2;
	int		clientNum;	/* 0 to (MAX_CLIENTS - 1), for players and corpses */
	int		frame;

	int		solid;	/* for client side prediction, trap_linkentity sets this properly */

	int		event;	/* impulse events -- muzzle flashes, footsteps, etc */
	int		eventParm;

	/* for players */
	int	powerups;	/* bit flags */
	int	weapon;		/* determines weapon and flash model, etc */
	int	legsAnim;	/* mask off ANIM_TOGGLEBIT */
	int	torsoAnim;	/* mask off ANIM_TOGGLEBIT */

	int	generic1;
} entityState_t;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED,	/* not talking to a server */
	CA_AUTHORIZING,		/* not used any more, was checking cd key */
	CA_CONNECTING,		/* sending request packets to the server */
	CA_CHALLENGING,		/* sending challenge packets to the server */
	CA_CONNECTED,		/* netchan_t established, getting gamestate */
	CA_LOADING,		/* only during cgame initialization, never during main loop */
	CA_PRIMED,		/* got gamestate, waiting for first frame */
	CA_ACTIVE,		/* game views should be displayed */
	CA_CINEMATIC		/* playing a cinematic or a static pic, not connected to a server */
} connstate_t;

/* font support */
enum {

	GLYPH_START	= 0,
	GLYPH_END	= 255,
	GLYPH_CHARSTART = 32,
	GLYPH_CHAREND	= 127,
	GLYPHS_PER_FONT = GLYPH_END - GLYPH_START + 1
};

typedef struct {
	int		height;		/* number of scan lines */
	int		top;		/* top of glyph in buffer */
	int		bottom;		/* bottom of glyph in buffer */
	int		pitch;		/* width for copying */
	int		xSkip;		/* x adjustment */
	int		imageWidth;	/* width of actual image */
	int		imageHeight;	/* height of actual image */
	float		s;		/* x offset in image where glyph starts */
	float		t;		/* y offset in image where glyph starts */
	float		s2;
	float		t2;
	qhandle_t	glyph;	/* handle to the shader with the glyph */
	char		shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t	glyphs [GLYPHS_PER_FONT];
	float		glyphScale;
	char		name[MAX_QPATH];
} fontInfo_t;

#define Square(x) ((x)*(x))

/* real time */
typedef struct qtime_s {
	int	tm_sec;		/* seconds after the minute - [0,59] */
	int	tm_min;		/* minutes after the hour - [0,59] */
	int	tm_hour;	/* hours since midnight - [0,23] */
	int	tm_mday;	/* day of the month - [1,31] */
	int	tm_mon;		/* months since January - [0,11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday - [0,6] */
	int	tm_yday;	/* days since January 1 - [0,365] */
	int	tm_isdst;	/* daylight savings time flag */
} qtime_t;

/* server browser sources
 * TTimo: AS_MPLAYER is no longer used */
enum {
	AS_LOCAL,
	AS_MPLAYER,
	AS_GLOBAL,
	AS_FAVORITES
};

/* cinematic states */
typedef enum {
	FMV_IDLE,
	FMV_PLAY,	/* play */
	FMV_EOF,	/* all other conditions, i.e. stop/EOF/abort */
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,		/* CTF */
	FLAG_TAKEN_RED,		/* One Flag CTF */
	FLAG_TAKEN_BLUE,	/* One Flag CTF */
	FLAG_DROPPED
} flagStatus_t;

enum {
	MAX_GLOBAL_SERVERS		= 4096,
	MAX_OTHER_SERVERS		= 128,
	MAX_PINGREQUESTS		= 32,
	MAX_SERVERSTATUSREQUESTS	= 16,

	SAY_ALL				= 0,
	SAY_TEAM			= 1,
	SAY_TELL			= 2,

	CDKEY_LEN			= 16,
	CDCHKSUM_LEN			= 2
};

#define LERP(a, b, w)		((a) * (1.0f - (w)) + (b) * (w))
#define LUMA(red, green, blue)	(0.2126f * (red) + 0.7152f * (green) + 0.0722f * \
				 (blue))

#endif	/* __Q_SHARED_H */

