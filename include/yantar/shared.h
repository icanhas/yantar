/* included first by ALL compilation units, module or not */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#ifndef __Q_SHARED_H
#define __Q_SHARED_H

#define PRODUCT_NAME		"yantar"
#define BASEGAME		"base"
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

/* FIXME: include this in modules themselves, not in shared */
#include "vmlibc.h"

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

#include "paths.h"

#include "platform.h"

typedef unsigned char	byte;
typedef unsigned char	uchar
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;

typedef enum {
	qfalse,
	qtrue
} qbool;

typedef union {
	float		f;
	int		i;
	unsigned int	ui;
} Flint;

typedef int Handle;
typedef int Sfxhandle;
typedef int Fhandle;
typedef int Cliphandle;

#define PAD(base, alignment)	(((base)+(alignment)-1) & ~((alignment)-1))
#define PADLEN(base, alignment)	(PAD((base), (alignment)) - (base))
#define PADP(base, alignment)	((void*)PAD((intptr_t)(base), (alignment)))

#ifdef __GNUC__
#define QALIGN(x) __attribute__((aligned(x)))
#else
#define QALIGN(x)
#endif

#ifndef UNUSED
#	define UNUSED(x) (void)(x)
#endif

/* pseudo-keyword, same as Tcl's 'then' */
#define then
/* this one may be summarised as 'wank' */
#define fallthrough

#ifndef NULL
#define NULL ((void*)0)
#endif
#define nil NULL

#define STRING(s)	# s
/* expand constants before token-pasting them */
#define XSTRING(s)	STRING(s)

#define MAX_QINT	0x7fffffff
#define MIN_QINT	(-MAX_QINT-1)

#define ARRAY_LEN(x)	(sizeof(x) / sizeof(*(x)))
#define STRARRAY_LEN(x)	(ARRAY_LEN(x) - 1)

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
					 * to cmdstrtok */
	MAX_STRING_TOKENS = 1024,	/* max tokens resulting
					 * from cmdstrtok */
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
} errorParm_t;

/* font rendering values used by ui and cgame */
#define PROP_SMALL_SIZE_SCALE 0.75f

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
#define hunkalloc(size, preference) hunkallocdebug(size, preference, # size, \
	__FILE__, \
	__LINE__)
void*	hunkallocdebug(int size, ha_pref preference, char *label, char *file,
		     int line);
#else
void*	hunkalloc(int size, ha_pref preference);
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
typedef float	Scalar;
typedef Scalar	Vec2[2];
typedef Scalar	Vec3[3];
typedef Scalar	Vec4[4];
typedef Scalar	Vec5[5];
/* 
 * N.B.: Matrices in yantar are treated in row-major order.
 * Aij -> A[i*ncols + j]
 */
typedef Scalar	Mat2[2*2];
typedef Scalar	Mat3[3*3];
typedef Scalar	Mat4[4*4];
typedef Vec4	Quat;	/* r, v₀, v₁, v₂ */
typedef int	fixed4_t;
typedef int	fixed8_t;
typedef int	fixed16_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846f	/* matches value in gcc v2 math.h */
#endif

#define NUMVERTEXNORMALS 162
extern Vec3 bytedirs[NUMVERTEXNORMALS];

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

extern Vec4	colorBlack;
extern Vec4	colorRed;
extern Vec4	colorGreen;
extern Vec4	colorBlue;
extern Vec4	colorYellow;
extern Vec4	colorMagenta;
extern Vec4	colorCyan;
extern Vec4	colorWhite;
extern Vec4	colorLtGrey;
extern Vec4	colorMdGrey;
extern Vec4	colorDkGrey;

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
#define ColorIndex(c)	(((c) - '0') & 0x07)

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED	"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"

extern Vec4 g_color_table[8];

#define MAKERGB(v, r, g, b)	(v[0]=r; v[1]=g; v[2]=b)
#define MAKERGBA(v, r, g, b, a)	(v[0]=r; v[1]=g; v[2]=b; v[3]=a)

#define DEG2RAD(a)	(((a) * M_PI) / 180.0F)
#define RAD2DEG(a)	(((a) * 180.0f) / M_PI)

struct Cplane;

extern Vec3	vec3_origin;
extern Vec3	axisDefault[3];

#define nanmask (255 << 23)

#define IS_NAN(x) (((*(int*)&x)&nanmask)==nanmask)

int Q_isnan(float x);

#if idx64
extern long qftolsse(float f);
extern int qvmftolsse(void);
extern void qsnapv3sse(Vec3 vec);

#define Q_ftol		qftolsse
#define Q_snapv3	qsnapv3sse

extern int (*Q_VMftol)(void);

#elif id386
extern long QDECL qftolx87(float f);
extern long QDECL qftolsse(float f);
extern int QDECL qvmftolx87(void);
extern int QDECL qvmftolsse(void);
extern void QDECL qsnapv3x87(Vec3 vec);
extern void QDECL qsnapv3sse(Vec3 vec);

extern long	(QDECL *Q_ftol)(float f);
extern int	(QDECL *Q_VMftol)(void);
extern void	(QDECL *Q_snapv3)(Vec3 vec);

#else
/* Q_ftol must expand to a function name so the pluggable renderer can take
 * its address */
  #define Q_ftol lrintf
  #define Q_snapv3(vec) \
	do \
	{ \
		Vec3 *temp = (vec); \
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
 * #define Q_snapv3(vec) \
 *      do\
 *      {\
 *              Vec3 *temp = (vec);\
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

#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))

qbool closeenough(float, float, float);

#define SQRTFAST(x) ((x) * Q_rsqrt(x))

signed char clampchar(int i);
signed short clampshort(int i);

/* this isn't a real cheap function to call! */
int DirToByte(Vec3 dir);
void ByteToDir(int b, Vec3 dir);

#if 0

#define dotv3(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define subv3(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1], \
					(c)[2]=(a)[2]-(b)[2])
#define addv3(a,b,c)	((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1], \
				 (c)[2]=(a)[2]+(b)[2])
#define copyv3(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define scalev3(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]= \
						(v)[2]*(s))
#define saddv3(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*	\
							(s),(o)[2]=(v)[2]+(b)[2]*(s))

#else

#define dotv3(x,y)		_dotv3(x,y)
#define subv3(a,b,c)	_subv3(a,b,c)
#define addv3(a,b,c)	_addv3(a,b,c)
#define copyv3(a,b)		_copyv3(a,b)
#define scalev3(v, s, o)	_scalev3(v,s,o)
#define saddv3(v, s, b, o)	_saddv3(v,s,b,o)

#endif

#ifdef Q3_VM
#ifdef copyv3
#undef copyv3

/* this is a little hack to get more efficient copies in our interpreter */
typedef struct {
	float v[3];
} Vec3struct;

#define copyv3(a,b) (*(Vec3struct*)b=*(Vec3struct*)a)
#endif
#endif

#define clearv3(a)		((a)[0]=(a)[1]=(a)[2]=0)
#define negv3(a,b)	((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define setv3(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define copyv4(a,b)	((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2], \
				 (b)[3]=(a)[3])

#define byte4copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2], \
				 (b)[3]=(a)[3])

#define snapv3(v)		{v[0]=((int)(v[0])); v[1]=((int)(v[1])); v[2]= \
					 ((int)(v[2])); }
/* just in case you don't want to use the macros */
Scalar	_dotv3(const Vec3 v1, const Vec3 v2);
void	_subv3(const Vec3 veca, const Vec3 vecb, Vec3 out);
void	_addv3(const Vec3 veca, const Vec3 vecb, Vec3 out);
void	_copyv3(const Vec3 in, Vec3 out);
void	_scalev3(const Vec3 in, float scale, Vec3 out);
void	_saddv3(const Vec3 veca, float scale, const Vec3 vecb, Vec3 vecc);
void	lerpv3(const Vec3 a, const Vec3 b, float lerp, Vec3 c);

/* Mat2 */
void	clearm2(Mat2);
void	identm2(Mat2);
void	copym2(const Mat2, Mat2);
void	transposem2(const Mat2, Mat2);
void	scalem2(const Mat2, Scalar, Mat2);
void	mulm2(const Mat2, const Mat2, Mat2);
qbool	cmpm2(const Mat2, const Mat2);
/* Mat3 */
void	clearm3(Mat3);
void	identm3(Mat3);
void	copym3(const Mat3, Mat3);
void	transposem3(const Mat3, Mat3);
void	scalem3(const Mat3, Scalar, Mat3);
void	mulm3(const Mat3, const Mat3, Mat3);
qbool	cmpm3(const Mat3, const Mat3);
/* Mat4 */
void	clearm4(Mat4);
void	identm4(Mat4);
void	copym4(const Mat4, Mat4);
void	transposem4(const Mat4, Mat4);
void	scalem4(const Mat4, Scalar, Mat4);
void	mulm4(const Mat4, const Mat4, Mat4);
void	transformm4(const Mat4, const Vec4, Vec4);
qbool	cmpm4(const Mat4, const Mat4);
void	translationm4(const Vec3, Mat4);
void	orthom4(float left, float right, float bottom, float top, float znear, float zfar, Mat4 out);

/* Quat */
void	mulq(const Quat, const Quat, Quat);
Scalar	magq(const Quat);
void	conjq(const Quat, Quat);
void	invq(const Quat, Quat);
void	diffq(const Quat, const Quat, Quat);

unsigned colourbytes3(float r, float g, float b);
unsigned colourbytes4(float r, float g, float b, float a);

float normalizecolour(const Vec3 in, Vec3 out);

float RadiusFromBounds(const Vec3 mins, const Vec3 maxs);
void ClearBounds(Vec3 mins, Vec3 maxs);
void AddPointToBounds(const Vec3 v, Vec3 mins, Vec3 maxs);

#if !defined(Q3_VM) || (defined(Q3_VM) && defined(__Q3_VM_MATH))
static ID_INLINE int
cmpv3(const Vec3 v1, const Vec3 v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return 0;
	return 1;
}

static ID_INLINE Scalar
lenv3(const Vec3 v)
{
	return (Scalar)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static ID_INLINE Scalar
lensqrv3(const Vec3 v)
{
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static ID_INLINE Scalar
distv3(const Vec3 p1, const Vec3 p2)
{
	Vec3 v;

	subv3 (p2, p1, v);
	return lenv3(v);
}

static ID_INLINE Scalar
distsqrv3(const Vec3 p1, const Vec3 p2)
{
	Vec3 v;

	subv3 (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

/* fast vector normalize routine that does not check to make sure
 * that length != 0, nor does it return length, uses rsqrt approximation */
static ID_INLINE void
fastnormv3(Vec3 v)
{
	float ilength;

	ilength = Q_rsqrt(dotv3(v, v));

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

static ID_INLINE void
invv3(Vec3 v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

static ID_INLINE void
crossv3(const Vec3 v1, const Vec3 v2, Vec3 cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

#else
int cmpv3(const Vec3 v1, const Vec3 v2);
Scalar lenv3(const Vec3 v);
Scalar lensqrv3(const Vec3 v);
Scalar distv3(const Vec3 p1, const Vec3 p2);
Scalar distsqrv3(const Vec3 p1, const Vec3 p2);
void fastnormv3(Vec3 v);
void invv3(Vec3 v);
void crossv3(const Vec3 v1, const Vec3 v2, Vec3 cross);

#endif

Scalar	normv3(Vec3 v);	/* returns vector length */
Scalar	norm2v3(const Vec3 v, Vec3 out);
void	scalev4(const Vec4 in, Scalar scale, Vec4 out);
void	rotv3(Vec3 in, Vec3 matrix[3], Vec3 out);
int	Q_log2(int val);

float	Q_acos(float c);

int	Q_rand(int *seed);
float	Q_random(int *seed);
float	Q_crandom(int *seed);

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0f * (random() - 0.5f))

void	v3toeuler(const Vec3 value1, Vec3 angles);
void	eulertoq(const Vec3, Quat);
void	qtoeuler(const Quat, Vec3);
void	eulertoaxis(const Vec3 angles, Vec3 axis[3]);
void	qtoaxis(Quat, Vec3 axis[3]);

void	clearaxis(Vec3 axis[3]);
void	copyaxis(Vec3 in[3], Vec3 out[3]);

void	setq(Quat, Scalar w, Scalar x, Scalar y, Scalar z);

void	SetPlaneSignbits(struct Cplane *out);
int	BoxOnPlaneSide(const Vec3 emins, const Vec3 emaxs, const struct Cplane *plane);

qbool	BoundsIntersect(const Vec3 mins, const Vec3 maxs,
			 const Vec3 mins2, const Vec3 maxs2);
qbool	BoundsIntersectSphere(const Vec3 mins, const Vec3 maxs,
			       const Vec3 origin, Scalar radius);
qbool	BoundsIntersectPoint(const Vec3 mins, const Vec3 maxs,
			      const Vec3 origin);

float	modeuler(float a);
float	lerpeuler(float from, float to, float frac);
float	subeuler(float a1, float a2);
void	subeulers(Vec3 v1, Vec3 v2, Vec3 v3);

float	norm360euler(float angle);
float	norm180euler(float angle);
float	deltaeuler(float angle1, float angle2);

qbool	PlaneFromPoints(Vec4 plane, const Vec3 a, const Vec3 b,
			 const Vec3 c);
void	ProjectPointOnPlane(Vec3 dst, const Vec3 p, const Vec3 normal);
void	RotatePointAroundVector(Vec3 dst, const Vec3 dir, const Vec3 point,
			     float degrees);
void	RotateAroundDirection(Vec3 axis[3], float yaw);
void	MakeNormalVectors(const Vec3 forward, Vec3 right, Vec3 up);
/* perpendicular vector could be replaced by this */

/* int	PlaneTypeForNormal (Vec3 normal); */

void	MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);	/* FIXME: get rid of this */
void	anglev3s(const Vec3 angles, Vec3 forward, Vec3 right, Vec3 up);
void	perpv3(Vec3 dst, const Vec3 src);

#ifndef MAX
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#endif

/* common */
long		Q_hashstr(const char *s, int size);
float		Q_clamp(float min, float max, float value);
char*	Q_skippath(char *pathname);
char*	Q_getext(const char *name);
void		Q_stripext(const char *in, char *out, int destsize);
qbool	Q_cmpext(const char *in, const char *ext);
void		Q_defaultext(char *path, int maxSize, const char *extension);

char*	Q_readtok(char **data_p);
char*	Q_readtok2(char **data_p, qbool allowLineBreak);
int         	Q_compresstr(char *data_p);

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
void	Q_matchtok(char**buf_p, char *match);

void	Q_skipblock(char **program);
void	Q_skipline(char **data);

void	Parse1DMatrix(char **buf_p, int x, float *m);
void	Parse2DMatrix(char **buf_p, int y, int x, float *m);
void	Parse3DMatrix(char **buf_p, int z, int y, int x, float *m);
int	Q_hexstr2int(const char *str);

int QDECL	Q_sprintf(char *dest, int size, 
	const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

char*	Q_skiptoks(char *s, int numTokens, char *sep);
char*	Q_skipcharset(char *s, char *sep);

/* mode parm for fsopen */
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} Fsmode;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} Fsorigin;

/*
 * unicode (q_utf.c)
 */
typedef uint Rune;	/* a unicode code point */

enum {
	Runemax		= 0x10ffff,	/* max legal code point as defined by Unicode 6.x */
	Runesync	= 0x80,		/* if <, byte can't be part of a UTF sequence */
	Runeself		= 0x80,		/* if <, rune and UTF sequence are the same */
	Runeerror	= 0xfffd,		/* '�' represents a bad UTF sequence */
};

int		Q_runetochar(char*, Rune*);
int		Q_chartorune(Rune*, char*);
int		Q_runelen(ulong);
int		Q_fullrune(char*, int);
size_t	Q_utflen(char*);
char*	Q_utfrune(char*, ulong);
char*	Q_utfutf(char*, char*);

Rune*	Q_runestrcat(Rune*, Rune*);
Rune*	Q_runestrchr(Rune*, Rune);
int		Q_runestrcmp(Rune*, Rune*);
Rune*	Q_runestrcpy(Rune*, Rune*);
Rune*	Q_runestrncpy(Rune*, Rune*, size_t);
Rune*	Q_runestrncat(Rune*, Rune*, size_t);
int		Q_runestrncmp(Rune*, Rune*, size_t);
size_t	Q_runestrlen(Rune*);
Rune*	Q_runestrstr(Rune*, Rune*);

Rune	Q_tolowerrune(Rune);
Rune	Q_totitlerune(Rune);
Rune	Q_toupperrune(Rune);
int		Q_isalpharune(Rune);
int		Q_isdigitrune(Rune);
int		Q_islowerrune(Rune);
int		Q_isspacerune(Rune);
int		Q_istitlerune(Rune);
int		Q_isupperrune(Rune);

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
void QDECL comerrorf(int level, const char *error,
		     ...) __attribute__ ((noreturn, format(printf, 2, 3)));
void QDECL comprintf(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));


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
					 * a cvarget(), so it can't be changed
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
	/* These flags are only returned by the cvarflags() function */
	CVAR_MODIFIED = 1 << 30,	/* Cvar was modified */
	CVAR_NONEXISTENT = 1 << 31	/* Cvar doesn't exist. */
};

/* nothing outside the Cvar_*() functions should modify these fields! */
typedef struct Cvar Cvar;
struct Cvar {
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

	Cvar		*next;
	Cvar		*prev;
	Cvar		*hashNext;
	Cvar		*hashPrev;
	int		hashIndex;
};

typedef int Cvarhandle;

/*
 * the modules that run in the virtual machine can't access the Cvar directly,
 * so they must ask for structured updates
 */
typedef struct {
	Cvarhandle	handle;
	int		modificationCount;
	float		value;
	int		integer;
	char		string[MAX_CVAR_VALUE_STRING];
} Vmcvar;

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
	(x[0] == 1.0f ? PLANE_X : \
	(x[1] == 1.0f ? PLANE_Y : \
	(x[2] == 1.0f ? PLANE_Z : \
	PLANE_NON_AXIAL)))

/* plane_t structure
 * !!! if this is changed, it must be changed in asm code too !!! */
typedef struct Cplane {
	Vec3	normal;
	float	dist;
	byte	type;		/* for fast side tests: 0,1,2 = axial, 3 = nonaxial */
	byte	signbits;	/* signx + (signy<<1) + (signz<<2), used as lookup during collision */
	byte	pad[2];
} Cplane;


/* a trace is returned when a box is swept through the world */
typedef struct {
	qbool		allsolid;	/* if true, plane is not valid */
	qbool		startsolid;	/* if true, the initial point was in a solid area */
	float		fraction;	/* time completed, 1.0 = didn't hit anything */
	Vec3		endpos;		/* final position */
	Cplane	plane;		/* surface normal at impact, transformed to world space */
	int		surfaceFlags;	/* surface hit */
	int		contents;	/* contents on other side of surface hit */
	int		entityNum;	/* entity the contacted sirface is a part of */
} Trace;

/* trace->entityNum can also be 0 to (MAX_GENTITIES-1)
 * or ENTITYNUM_NONE, ENTITYNUM_WORLD */

/* markfragments are returned by CM_MarkFragments() */
typedef struct {
	int	firstPoint;
	int	numPoints;
} Markfrag;

typedef struct {
	Vec3	origin;
	Vec3	axis[3];
} Orient;

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
} Sndchan;

/* elements communicated across the net */
#define ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)	((x)*(360.0f/65536))
#define eulertoshorts(e, s) \
	do{ \
		(s)[0] = ANGLE2SHORT((e)[0]); \
		(s)[1] = ANGLE2SHORT((e)[1]); \
		(s)[2] = ANGLE2SHORT((e)[2]); \
	}while(0);
#define shortstoeuler(s, e) \
	do{ \
		(e)[0] = SHORT2ANGLE((s)[0]); \
		(e)[1] = SHORT2ANGLE((s)[1]); \
		(e)[2] = SHORT2ANGLE((s)[2]); \
	}while(0);

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
} Gamestate;

/* bit field limits */
enum {
	MAX_STATS		= 16,
	MAX_PERSISTANT		= 16,
	MAX_POWERUPS		= 16,
	MAX_WEAPONS		= 16,

	MAX_PS_EVENTS		= 2,

	PS_PMOVEFRAMECOUNTBITS	= 6,
};

typedef enum {
	WSpri,	/* rail guns, etc. */
	WSsec,	/* missile pods, etc. */
	WShook,	/* offhand grappling hook */
	WSnumslots
} Weapslot;

/*
 * Playerstate is the information needed by both the client and server
 * to predict player motion and actions
 * nothing outside of pmove should modify these, or some degree of prediction error
 * will occur
 */

/* you can't add anything to this without modifying the code in msg.c */

/*
 * Playerstate is a full superset of Entstate as it is used by players,
 * so if a Playerstate is transmitted, the Entstate can be fully derived
 * from it.
 */
typedef struct Playerstate {
	int	commandTime;	/* cmd->serverTime of last executed command */
	int	pm_type;
	int	bobCycle;	/* for view bobbing and footstep generation */
	int	pm_flags;	/* ducked, jump_held, etc */
	int	pm_time;

	Vec3	origin;
	Vec3	velocity;
	int	weaptime[WSnumslots];
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

	int	movementDir;	/* a number 0 to 7 that represents the relative angle */
	/* of movement to the view angle (axial and diagonals)
	 * when at rest, the value will remain unchanged
	 * used to twist the legs during strafing */

	Vec3	grapplePoint;	/* location of grapple to pull towards if PMF_GRAPPLE_PULL */

	int	eFlags;	/* copied to Entstate->eFlags */

	int	eventSequence;	/* pmove generated events */
	int	events[MAX_PS_EVENTS];
	int	eventParms[MAX_PS_EVENTS];

	int	externalEvent;	/* events set on player from another source */
	int	externalEventParm;
	int	externalEventTime;

	int	clientNum;	/* ranges from 0 to MAX_CLIENTS-1 */
	int	weap[WSnumslots];		/* copied to Entstate->weapon */
	int	weapstate[WSnumslots];

	Vec3	viewangles;	/* for fixed views */
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
} Playerstate;

/*
 * Usrcmd->button bits, many of which are generated by the client system,
 * so they aren't game/cgame only definitions
 */
enum buttonflags {
	BUTTON_PRIATTACK	= (1<<0),	/* primary attack */
	BUTTON_SECATTACK	= (1<<1),	/* secondary attack */
	BUTTON_HOOKFIRE		= (1<<2),	/* grappling hook */
	BUTTON_TALK			= (1<<3),	/* displays talk balloon and disables actions */
	BUTTON_USE_HOLDABLE	= (1<<4),
	BUTTON_GESTURE		= (1<<5),
	BUTTON_WALKING		= (1<<6),	/* walking can't just be inferred from 
								 * MOVE_RUN, because a key pressed late
								 * in the frame will only generate a 
								 * small move value for that frame
								 * walking will use different 
								 * animations and won't generate
								 * footsteps */
	BUTTON_AFFIRMATIVE	= (1<<7),
	BUTTON_NEGATIVE		= (1<<8),

	BUTTON_GETFLAG		= (1<<9),
	BUTTON_GUARDBASE	= (1<<10),
	BUTTON_PATROL		= (1<<11),
	BUTTON_FOLLOWME	= (1<<12),

	BUTTON_ANY			= (1<<13),	/* any key whatsoever */

	MOVE_RUN			= 120,	/* if forwardmove or rightmove >= MOVE_RUN,
								 * then BUTTON_WALKING should be set */
								 /* FIXME */
};

/* Usrcmd is sent to the server each client frame */
typedef struct Usrcmd {
	int		serverTime;
	int		angles[3];
	int		buttons;
	byte		weap[WSnumslots];
	signed char	forwardmove, rightmove, upmove, brakefrac;
} Usrcmd;

/* entities */
/* if entityState->solid == SOLID_BMODEL, modelindex is an inline model number */
#define SOLID_BMODEL 0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,	/* non-parametric, but interpolate between snapshots */
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE,	/* value = base + sin( time / duration ) * delta */
	TR_GRAVITY,
	TR_STOCHASTIC
} type_t;

typedef struct {
	type_t	type;
	int		time;
	int		duration;	/* if non 0, time + duration = stop time */
	Vec3		base;
	Vec3		delta;	/* velocity, etc */
} Trajectory;

/* 
 * Entstate is the information conveyed from the server
 * in an update message about entities that the client will
 * need to render in some way
 * Different eTypes may use the information in different ways
 * The messages are delta compressed, so it doesn't really matter if
 * the structure size is fairly large 
 */
typedef struct Entstate {
	int		number;	/* entity index */
	int		eType;	/* Enttype */
	int		eFlags;

	Trajectory	traj;	/* for calculating position */
	Trajectory	apos;	/* for calculating angles */

	int		time;
	int		time2;

	Vec3		origin;
	Vec3		origin2;

	Vec3		angles;
	Vec3		angles2;

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
	int	weap[WSnumslots];		/* determines weapon and flash model, etc */
	int	parentweap;	/* only for projectiles; determines appearance and behaviour */
	int	legsAnim;	/* mask off ANIM_TOGGLEBIT */
	int	torsoAnim;	/* mask off ANIM_TOGGLEBIT */

	int	generic1;
} Entstate;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED,	/* not talking to a server */
	CA_AUTHORIZING,		/* not used any more, was checking cd key */
	CA_CONNECTING,		/* sending request packets to the server */
	CA_CHALLENGING,		/* sending challenge packets to the server */
	CA_CONNECTED,		/* Netchan established, getting gamestate */
	CA_LOADING,		/* only during cgame initialization, never during main loop */
	CA_PRIMED,		/* got gamestate, waiting for first frame */
	CA_ACTIVE,		/* game views should be displayed */
	CA_CINEMATIC		/* playing a cinematic or a static pic, not connected to a server */
} Connstate;

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
	Handle		glyph;	/* handle to the shader with the glyph */
	char		shaderName[32];
} Glyphinfo;

typedef struct {
	Glyphinfo	glyphs [GLYPHS_PER_FONT];
	float		glyphScale;
	char		name[MAX_QPATH];
} Fontinfo;

#define Square(x) ((x)*(x))

/* real time */
typedef struct Qtime {
	int	tm_sec;		/* seconds after the minute - [0,59] */
	int	tm_min;		/* minutes after the hour - [0,59] */
	int	tm_hour;	/* hours since midnight - [0,23] */
	int	tm_mday;	/* day of the month - [1,31] */
	int	tm_mon;		/* months since January - [0,11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday - [0,6] */
	int	tm_yday;	/* days since January 1 - [0,365] */
	int	tm_isdst;	/* daylight savings time flag */
} Qtime;

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
} Cinstatus;

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,		/* CTF */
	FLAG_TAKEN_RED,		/* One Flag CTF */
	FLAG_TAKEN_BLUE,	/* One Flag CTF */
	FLAG_DROPPED
} Flagstatus;

enum {
	MAX_GLOBAL_SERVERS		= 4096,
	MAX_OTHER_SERVERS		= 128,
	MAX_PINGREQUESTS		= 32,
	MAX_SERVERSTATUSREQUESTS	= 16,

	SAY_ALL				= 0,
	SAY_TEAM			= 1,
	SAY_TELL			= 2
};

#define LERP(a, b, w)		((a) * (1.0f - (w)) + (b) * (w))
#define LUMA(red, green, blue)	(0.2126f * (red) + 0.7152f * (green) + 0.0722f * \
				 (blue))

#endif	/* __Q_SHARED_H */

