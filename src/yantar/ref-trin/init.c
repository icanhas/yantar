/* functions that are not called every frame */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

Glconfig glConfig;
qbool textureFilterAnisotropic = qfalse;
int maxAnisotropy	= 0;
float displayAspect	= 0.0f;

glstate_t glState;

static void GfxInfo_f(void);

Cvar *r_flareSize;
Cvar *r_flareFade;
Cvar *r_flareCoeff;

Cvar *r_railWidth;
Cvar *r_railCoreWidth;
Cvar *r_railSegmentLength;

Cvar *r_ignoreFastPath;

Cvar *r_verbose;
Cvar *r_ignore;

Cvar *r_detailTextures;

Cvar *r_znear;
Cvar *r_zproj;
Cvar *r_stereoSeparation;

Cvar *r_smp;
Cvar *r_showSmp;
Cvar *r_skipBackEnd;

Cvar *r_stereoEnabled;
Cvar *r_anaglyphMode;

Cvar *r_greyscale;

Cvar *r_ignorehwgamma;
Cvar *r_measureOverdraw;

Cvar *r_inGameVideo;
Cvar *r_fastsky;
Cvar *r_drawSun;
Cvar *r_dynamiclight;
Cvar *r_dlightBacks;

Cvar *r_lodbias;
Cvar *r_lodscale;

Cvar *r_norefresh;
Cvar *r_drawentities;
Cvar *r_drawworld;
Cvar *r_speeds;
Cvar *r_fullbright;
Cvar *r_novis;
Cvar *r_nocull;
Cvar *r_facePlaneCull;
Cvar *r_showcluster;
Cvar *r_nocurves;

Cvar *r_allowExtensions;

Cvar *r_ext_compressed_textures;
Cvar *r_ext_multitexture;
Cvar *r_ext_compiled_vertex_array;
Cvar *r_ext_texture_env_add;
Cvar *r_ext_texture_filter_anisotropic;
Cvar *r_ext_max_anisotropy;

Cvar *r_ignoreGLErrors;
Cvar *r_logFile;

Cvar *r_stencilbits;
Cvar *r_depthbits;
Cvar *r_colorbits;
Cvar *r_primitives;
Cvar *r_texturebits;
Cvar *r_ext_multisample;

Cvar *r_drawBuffer;
Cvar *r_lightmap;
Cvar *r_vertexLight;
Cvar *r_uiFullScreen;
Cvar *r_shadows;
Cvar *r_flares;
Cvar *r_mode;
Cvar *r_nobind;
Cvar *r_singleShader;
Cvar *r_roundImagesDown;
Cvar *r_colorMipLevels;
Cvar *r_picmip;
Cvar *r_showtris;
Cvar *r_showsky;
Cvar *r_shownormals;
Cvar *r_finish;
Cvar *r_clear;
Cvar *r_swapInterval;
Cvar *r_textureMode;
Cvar *r_offsetFactor;
Cvar *r_offsetUnits;
Cvar *r_gamma;
Cvar *r_intensity;
Cvar *r_lockpvs;
Cvar *r_noportals;
Cvar *r_portalOnly;

Cvar *r_subdivisions;
Cvar *r_lodCurveError;

Cvar *r_fullscreen;
Cvar *r_noborder;

Cvar *r_customwidth;
Cvar *r_customheight;
Cvar *r_customPixelAspect;

Cvar *r_overBrightBits;
Cvar *r_mapOverBrightBits;

Cvar *r_debugSurface;
Cvar *r_simpleMipMaps;

Cvar *r_showImages;

Cvar *r_ambientScale;
Cvar *r_directedScale;
Cvar *r_debugLight;
Cvar *r_debugSort;
Cvar *r_printShaders;
Cvar *r_saveFontData;

Cvar *r_marksOnTriangleMeshes;

Cvar *r_aviMotionJpegQuality;
Cvar *r_screenshotJpegQuality;

Cvar	*r_maxpolys;
int	max_polys;
Cvar  *r_maxpolyverts;
int	max_polyverts;

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void
InitOpenGL(void)
{
	char renderer_buffer[1024];

	/*
	 * initialize OS specific portions of the renderer
	 *
	 * GLimp_Init directly or indirectly references the following cvars:
	 *      - r_fullscreen
	 *      - r_mode
	 *      - r_(color|depth|stencil)bits
	 *      - r_ignorehwgamma
	 *      - r_gamma
	 *  */

	if(glConfig.vidWidth == 0){
		GLint temp;

		GLimp_Init();

		strcpy(renderer_buffer, glConfig.renderer_string);
		Q_strlwr(renderer_buffer);

		/* OpenGL driver constants */
		qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		glConfig.maxTextureSize = temp;

		/* stubbed or broken drivers may have reported 0... */
		if(glConfig.maxTextureSize <= 0){
			glConfig.maxTextureSize = 0;
		}
	}

	/* init command buffers and SMP */
	R_InitCommandBuffers();

	/* print info */
	GfxInfo_f();

	/* set default state */
	GL_SetDefaultState();
}

/*
 * GL_CheckErrors
 */
void
GL_CheckErrors(void)
{
	int	err;
	char	s[64];

	err = qglGetError();
	if(err == GL_NO_ERROR){
		return;
	}
	if(r_ignoreGLErrors->integer){
		return;
	}
	switch(err){
	case GL_INVALID_ENUM:
		strcpy(s, "GL_INVALID_ENUM");
		break;
	case GL_INVALID_VALUE:
		strcpy(s, "GL_INVALID_VALUE");
		break;
	case GL_INVALID_OPERATION:
		strcpy(s, "GL_INVALID_OPERATION");
		break;
	case GL_STACK_OVERFLOW:
		strcpy(s, "GL_STACK_OVERFLOW");
		break;
	case GL_STACK_UNDERFLOW:
		strcpy(s, "GL_STACK_UNDERFLOW");
		break;
	case GL_OUT_OF_MEMORY:
		strcpy(s, "GL_OUT_OF_MEMORY");
		break;
	default:
		Q_sprintf(s, sizeof(s), "%i", err);
		break;
	}

	ri.Error(ERR_FATAL, "GL_CheckErrors: %s", s);
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s {
	const char	*description;
	int		width, height;
	float		pixelAspect;	/* pixel width / height */
} vidmode_t;

vidmode_t	r_vidModes[] =
{
	{ "Mode  0: 320x240",           320,    240,    1 },
	{ "Mode  1: 400x300",           400,    300,    1 },
	{ "Mode  2: 512x384",           512,    384,    1 },
	{ "Mode  3: 640x480",           640,    480,    1 },
	{ "Mode  4: 800x600",           800,    600,    1 },
	{ "Mode  5: 960x720",           960,    720,    1 },
	{ "Mode  6: 1024x768",          1024,   768,    1 },
	{ "Mode  7: 1152x864",          1152,   864,    1 },
	{ "Mode  8: 1280x1024",         1280,   1024,   1 },
	{ "Mode  9: 1600x1200",         1600,   1200,   1 },
	{ "Mode 10: 2048x1536",         2048,   1536,   1 },
	{ "Mode 11: 856x480 (wide)",856,        480,    1 }
};
static int	s_numVidModes = ARRAY_LEN(r_vidModes);

qbool
R_GetModeInfo(int *width, int *height, float *windowAspect, int mode)
{
	vidmode_t *vm;
	float pixelAspect;

	if(mode < -1){
		return qfalse;
	}
	if(mode >= s_numVidModes){
		return qfalse;
	}

	if(mode == -1){
		*width	= r_customwidth->integer;
		*height = r_customheight->integer;
		pixelAspect = r_customPixelAspect->value;
	}else{
		vm = &r_vidModes[mode];

		*width	= vm->width;
		*height = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = (float)*width / (*height * pixelAspect);

	return qtrue;
}

/*
** R_ModeList_f
*/
static void
R_ModeList_f(void)
{
	int i;

	ri.Printf(PRINT_ALL, "\n");
	for(i = 0; i < s_numVidModes; i++)
		ri.Printf(PRINT_ALL, "%s\n", r_vidModes[i].description);
	ri.Printf(PRINT_ALL, "\n");
}


/*
 *
 *                                              SCREEN SHOTS
 *
 * NOTE TTimo
 * some thoughts about the screenshots system:
 * screenshots get written in fs_homepath + fs_gamedir
 * vanilla q3 .. baseq3/screenshots/ *.tga
 * team arena .. missionpack/screenshots/ *.tga
 *
 * two commands: "screenshot" and "screenshotJPEG"
 * we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
 * (with fsfileexists / fsopenw calls)
 * FIXME: the statics don't get a reinit between fs_game changes
 *
 */

/*
 * RB_ReadPixels
 *
 * Reads an image but takes care of alignment issues for reading RGB images.
 *
 * Reads a minimum offset for where the RGB data starts in the image from
 * integer stored at pointer offset. When the function has returned the actual
 * offset was written back to address offset. This address will always have an
 * alignment of packAlign to ensure efficient copying.
 *
 * Stores the length of padding after a line of pixels to address padlen
 *
 * Return value must be freed with ri.hunkfreetemp()
 */

byte *
RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte	*buffer, *bufstart;
	int	padwidth, linelen;
	GLint	packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen		= width * 3;
	padwidth	= PAD(linelen, packAlign);

	/* Allocate a few more bytes so that we can choose an alignment we like */
	buffer = ri.hunkalloctemp(padwidth * height + *offset + packAlign - 1);

	bufstart = PADP((intptr_t)buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
 * RB_TakeScreenshot
 */
void
RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
	byte	*allbuf, *buffer;
	byte	*srcptr, *destptr;
	byte	*endline, *endmem;
	byte	temp;

	int	linelen, padlen;
	size_t offset = 18, memcount;

	allbuf	= RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer	= allbuf + offset - 18;

	Q_Memset (buffer, 0, 18);
	buffer[2]	= 2;	/* uncompressed type */
	buffer[12]	= width & 255;
	buffer[13]	= width >> 8;
	buffer[14]	= height & 255;
	buffer[15]	= height >> 8;
	buffer[16]	= 24;	/* pixel size */

	/* swap rgb to bgr and remove padding from line endings */
	linelen = width * 3;

	srcptr	= destptr = allbuf + offset;
	endmem	= srcptr + (linelen + padlen) * height;

	while(srcptr < endmem){
		endline = srcptr + linelen;

		while(srcptr < endline){
			temp = srcptr[0];
			*destptr++	= srcptr[2];
			*destptr++	= srcptr[1];
			*destptr++	= temp;

			srcptr += 3;
		}

		/* Skip the pad */
		srcptr += padlen;
	}

	memcount = linelen * height;

	/* gamma correct */
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(allbuf + offset, memcount);

	ri.fswritefile(fileName, buffer, memcount + 18);

	ri.hunkfreetemp(allbuf);
}

/*
 * RB_TakeScreenshotJPEG
 */

void
RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte *buffer;
	size_t	offset = 0, memcount;
	int	padlen;

	buffer		= RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount	= (width * 3 + padlen) * height;

	/* gamma correct */
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri.hunkfreetemp(buffer);
}

/*
 * RB_TakeScreenshotCmd
 */
const void *
RB_TakeScreenshotCmd(const void *data)
{
	const screenshotCommand_t *cmd;

	cmd = (const screenshotCommand_t*)data;

	if(cmd->jpeg)
		RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	else
		RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);

	return (const void*)(cmd + 1);
}

/*
 * R_TakeScreenshot
 */
void
R_TakeScreenshot(int x, int y, int width, int height, char *name, qbool jpeg)
{
	static char fileName[MAX_OSPATH];	/* bad things if two screenshots per frame? */
	screenshotCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if(!cmd){
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x	= x;
	cmd->y	= y;
	cmd->width	= width;
	cmd->height	= height;
	Q_strncpyz(fileName, name, sizeof(fileName));
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}

/*
 * R_ScreenshotFilename
 */
void
R_ScreenshotFilename(int lastNumber, char *fileName)
{
	int a,b,c,d;

	if(lastNumber < 0 || lastNumber > 9999){
		Q_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.tga");
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Q_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
		, a, b, c, d);
}

/*
 * R_ScreenshotFilename
 */
void
R_ScreenshotFilenameJPEG(int lastNumber, char *fileName)
{
	int a,b,c,d;

	if(lastNumber < 0 || lastNumber > 9999){
		Q_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.jpg");
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Q_sprintf(fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
		, a, b, c, d);
}

/*
 * R_LevelShot
 *
 * levelshots are specialized 128*128 thumbnails for
 * the menu system, sampled down from full screen distorted images
 */
void
R_LevelShot(void)
{
	char checkname[MAX_OSPATH];
	byte	*buffer;
	byte	*source, *allsource;
	byte	*src, *dst;
	size_t	offset = 0;
	int	padlen;
	int	x, y;
	int	r, g, b;
	float	xScale, yScale;
	int	xx, yy;

	Q_sprintf(checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName);

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	source = allsource + offset;

	buffer = ri.hunkalloctemp(128 * 128*3 + 18);
	Q_Memset (buffer, 0, 18);
	buffer[2]	= 2;	/* uncompressed type */
	buffer[12]	= 128;
	buffer[14]	= 128;
	buffer[16]	= 24;	/* pixel size */

	/* resample from source */
	xScale	= glConfig.vidWidth / 512.0f;
	yScale	= glConfig.vidHeight / 384.0f;
	for(y = 0; y < 128; y++)
		for(x = 0; x < 128; x++){
			r = g = b = 0;
			for(yy = 0; yy < 3; yy++)
				for(xx = 0; xx < 4; xx++){
					src = source +
					      (3 * glConfig.vidWidth + padlen) * (int)((y*3 + yy) * yScale) +
					      3 * (int)((x*4 + xx) * xScale);
					r	+= src[0];
					g	+= src[1];
					b	+= src[2];
				}
			dst = buffer + 18 + 3 * (y * 128 + x);
			dst[0]	= b / 12;
			dst[1]	= g / 12;
			dst[2]	= r / 12;
		}

	/* gamma correct */
	if(glConfig.deviceSupportsGamma){
		R_GammaCorrect(buffer + 18, 128 * 128 * 3);
	}

	ri.fswritefile(checkname, buffer, 128 * 128*3 + 18);

	ri.hunkfreetemp(buffer);
	ri.hunkfreetemp(allsource);

	ri.Printf(PRINT_ALL, "Wrote %s\n", checkname);
}

/*
 * R_ScreenShot_f
 *
 * screenshot
 * screenshot [silent]
 * screenshot [levelshot]
 * screenshot [filename]
 *
 * Doesn't print the pacifier message if there is a second arg
 */
void
R_ScreenShot_f(void)
{
	char checkname[MAX_OSPATH];
	static int	lastNumber = -1;
	qbool		silent;

	if(!strcmp(ri.cmdargv(1), "levelshot")){
		R_LevelShot();
		return;
	}

	if(!strcmp(ri.cmdargv(1), "silent")){
		silent = qtrue;
	}else{
		silent = qfalse;
	}

	if(ri.cmdargc() == 2 && !silent){
		/* explicit filename */
		Q_sprintf(checkname, MAX_OSPATH, "screenshots/%s.tga", ri.cmdargv(1));
	}else{
		/* scan for a free filename */

		/* if we have saved a previous screenshot, don't scan
		 * again, because recording demo avis can involve
		 * thousands of shots */
		if(lastNumber == -1){
			lastNumber = 0;
		}
		/* scan for a free number */
		for(; lastNumber <= 9999; lastNumber++){
			R_ScreenshotFilename(lastNumber, checkname);

			if(!ri.fsfileexists(checkname)){
				break;	/* file doesn't exist */
			}
		}

		if(lastNumber >= 9999){
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse);

	if(!silent){
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

void
R_ScreenShotJPEG_f(void)
{
	char checkname[MAX_OSPATH];
	static int	lastNumber = -1;
	qbool		silent;

	if(!strcmp(ri.cmdargv(1), "levelshot")){
		R_LevelShot();
		return;
	}

	if(!strcmp(ri.cmdargv(1), "silent")){
		silent = qtrue;
	}else{
		silent = qfalse;
	}

	if(ri.cmdargc() == 2 && !silent){
		/* explicit filename */
		Q_sprintf(checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.cmdargv(1));
	}else{
		/* scan for a free filename */

		/* if we have saved a previous screenshot, don't scan
		 * again, because recording demo avis can involve
		 * thousands of shots */
		if(lastNumber == -1){
			lastNumber = 0;
		}
		/* scan for a free number */
		for(; lastNumber <= 9999; lastNumber++){
			R_ScreenshotFilenameJPEG(lastNumber, checkname);

			if(!ri.fsfileexists(checkname)){
				break;	/* file doesn't exist */
			}
		}

		if(lastNumber == 10000){
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qtrue);

	if(!silent){
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

/* ============================================================================ */

/*
 * RB_TakeVideoFrameCmd
 */
const void *
RB_TakeVideoFrameCmd(const void *data)
{
	const videoFrameCommand_t *cmd;
	byte *cBuf;
	size_t	memcount, linelen;
	int	padwidth, avipadwidth, padlen, avipadlen;
	GLint	packAlign;

	cmd = (const videoFrameCommand_t*)data;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	/* Alignment stuff for glReadPixels */
	padwidth	= PAD(linelen, packAlign);
	padlen		= padwidth - linelen;
	/* AVI line padding */
	avipadwidth	= PAD(linelen, AVI_LINE_PADDING);
	avipadlen	= avipadwidth - linelen;

	cBuf = PADP(cmd->captureBuffer, packAlign);

	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
		GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	/* gamma correct */
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg){
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
			r_aviMotionJpegQuality->integer,
			cmd->width, cmd->height, cBuf, padlen);
		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}else{
		byte	*lineend, *memend;
		byte	*srcptr, *destptr;

		srcptr	= cBuf;
		destptr = cmd->encodeBuffer;
		memend	= srcptr + memcount;

		/* swap R and B and remove line paddings */
		while(srcptr < memend){
			lineend = srcptr + linelen;
			while(srcptr < lineend){
				*destptr++	= srcptr[2];
				*destptr++	= srcptr[1];
				*destptr++	= srcptr[0];
				srcptr += 3;
			}

			Q_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void*)(cmd + 1);
}

/* ============================================================================ */

/*
** GL_SetDefaultState
*/
void
GL_SetDefaultState(void)
{
	qglClearDepth(1.0f);

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	/* initialize downstream texture unit if we're running
	 * in a multitexture environment */
	if(qglActiveTextureARB){
		GL_SelectTexture(1);
		GL_TextureMode(r_textureMode->string);
		GL_TexEnv(GL_MODULATE);
		qglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode(r_textureMode->string);
	GL_TexEnv(GL_MODULATE);

	qglShadeModel(GL_SMOOTH);
	qglDepthFunc(GL_LEQUAL);

	/* the vertex array is always enabled, but the color and texture
	 * arrays are enabled and disabled around the compiled vertex array call */
	qglEnableClientState (GL_VERTEX_ARRAY);

	/*
	 * make sure our GL state vector is set correctly
	 *  */
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);
}


/*
 * GfxInfo_f
 */
void
GfxInfo_f(void)
{
	const char	*enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char	*fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf(PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string);
	ri.Printf(PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string);
	ri.Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string);
	ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits);
	ri.Printf(PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits,
		glConfig.depthBits,
		glConfig.stencilBits);
	ri.Printf(PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth,
		glConfig.vidHeight,
		fsstrings[r_fullscreen->integer == 1]);
	if(glConfig.displayFrequency){
		ri.Printf(PRINT_ALL, "%d\n", glConfig.displayFrequency);
	}else{
		ri.Printf(PRINT_ALL, "N/A\n");
	}
	if(glConfig.deviceSupportsGamma){
		ri.Printf(PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}else{
		ri.Printf(PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	/* rendering primitives */
	{
		int primitives;

		/* default is to use triangles if compiled vertex arrays are present */
		ri.Printf(PRINT_ALL, "rendering primitives: ");
		primitives = r_primitives->integer;
		if(primitives == 0){
			if(qglLockArraysEXT){
				primitives = 2;
			}else{
				primitives = 1;
			}
		}
		if(primitives == -1){
			ri.Printf(PRINT_ALL, "none\n");
		}else if(primitives == 2){
			ri.Printf(PRINT_ALL, "single glDrawElements\n");
		}else if(primitives == 1){
			ri.Printf(PRINT_ALL, "multiple glArrayElement\n");
		}else if(primitives == 3){
			ri.Printf(PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n");
		}
	}

	ri.Printf(PRINT_ALL, "texturemode: %s\n", r_textureMode->string);
	ri.Printf(PRINT_ALL, "picmip: %d\n", r_picmip->integer);
	ri.Printf(PRINT_ALL, "texture bits: %d\n", r_texturebits->integer);
	ri.Printf(PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0]);
	ri.Printf(PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ]);
	ri.Printf(PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0]);
	ri.Printf(PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE]);
	if(r_vertexLight->integer){
		ri.Printf(PRINT_ALL, "HACK: using vertex lightmap approximation\n");
	}
	if(glConfig.smpActive){
		ri.Printf(PRINT_ALL, "Using dual processor acceleration\n");
	}
	if(r_finish->integer){
		ri.Printf(PRINT_ALL, "Forcing glFinish\n");
	}
}

/*
 * R_Register
 */
void
R_Register(void)
{
	/*
	 * latched and archived variables
	 *  */
	r_allowExtensions = ri.cvarget("r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_compressed_textures = ri.cvarget("r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_multitexture = ri.cvarget("r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_compiled_vertex_array = ri.cvarget("r_ext_compiled_vertex_array", "1",
		CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_env_add = ri.cvarget("r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_ext_texture_filter_anisotropic = ri.cvarget("r_ext_texture_filter_anisotropic",
		"0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_max_anisotropy = ri.cvarget("r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH);

	r_picmip = ri.cvarget ("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_roundImagesDown	= ri.cvarget ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_colorMipLevels	= ri.cvarget ("r_colorMipLevels", "0", CVAR_LATCH);
	ri.cvarcheckrange(r_picmip, 0, 16, qtrue);
	r_detailTextures = ri.cvarget("r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_texturebits = ri.cvarget("r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_colorbits = ri.cvarget("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stencilbits = ri.cvarget("r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH);
	r_depthbits = ri.cvarget("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_multisample = ri.cvarget("r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.cvarcheckrange(r_ext_multisample, 0, 4, qtrue);
	r_overBrightBits	= ri.cvarget ("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignorehwgamma		= ri.cvarget("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = ri.cvarget("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH);
	r_fullscreen	= ri.cvarget("r_fullscreen", "1", CVAR_ARCHIVE);
	r_noborder	= ri.cvarget("r_noborder", "0", CVAR_ARCHIVE);
	r_customwidth	= ri.cvarget("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight	= ri.cvarget("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_customPixelAspect = ri.cvarget("r_customPixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_simpleMipMaps = ri.cvarget("r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_vertexLight	= ri.cvarget("r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_uiFullScreen	= ri.cvarget("r_uifullscreen", "0", 0);
	r_subdivisions	= ri.cvarget ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_smp = ri.cvarget("r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stereoEnabled		= ri.cvarget("r_stereoEnabled", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreFastPath	= ri.cvarget("r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_greyscale = ri.cvarget("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.cvarcheckrange(r_greyscale, 0, 1, qfalse);

	/*
	 * temporary latched variables that can only change over a restart
	 *  */
	r_fullbright = ri.cvarget ("r_fullbright", "0", CVAR_LATCH|CVAR_CHEAT);
	r_mapOverBrightBits = ri.cvarget ("r_mapOverBrightBits", "2", CVAR_LATCH);
	r_intensity = ri.cvarget ("r_intensity", "1", CVAR_LATCH);
	r_singleShader = ri.cvarget ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);

	/*
	 * archived variables that can change at any time
	 *  */
	r_lodCurveError = ri.cvarget("r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT);
	r_lodbias	= ri.cvarget("r_lodbias", "0", CVAR_ARCHIVE);
	r_flares	= ri.cvarget ("r_flares", "0", CVAR_ARCHIVE);
	r_znear		= ri.cvarget("r_znear", "4", CVAR_CHEAT);
	ri.cvarcheckrange(r_znear, 0.001f, 200, qfalse);
	r_zproj = ri.cvarget("r_zproj", "64", CVAR_ARCHIVE);
	r_stereoSeparation	= ri.cvarget("r_stereoSeparation", "64", CVAR_ARCHIVE);
	r_ignoreGLErrors	= ri.cvarget("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_fastsky = ri.cvarget("r_fastsky", "0", CVAR_ARCHIVE);
	r_inGameVideo = ri.cvarget("r_inGameVideo", "1", CVAR_ARCHIVE);
	r_drawSun = ri.cvarget("r_drawSun", "0", CVAR_ARCHIVE);
	r_dynamiclight	= ri.cvarget("r_dynamiclight", "1", CVAR_ARCHIVE);
	r_dlightBacks	= ri.cvarget("r_dlightBacks", "1", CVAR_ARCHIVE);
	r_finish = ri.cvarget ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode	= ri.cvarget("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_swapInterval	= ri.cvarget("r_swapInterval", "0",
		CVAR_ARCHIVE | CVAR_LATCH);
	r_gamma = ri.cvarget("r_gamma", "1", CVAR_ARCHIVE);
	r_facePlaneCull = ri.cvarget ("r_facePlaneCull", "1", CVAR_ARCHIVE);

	r_railWidth = ri.cvarget("r_railWidth", "16", CVAR_ARCHIVE);
	r_railCoreWidth = ri.cvarget("r_railCoreWidth", "6", CVAR_ARCHIVE);
	r_railSegmentLength = ri.cvarget("r_railSegmentLength", "32", CVAR_ARCHIVE);

	r_primitives = ri.cvarget("r_primitives", "0", CVAR_ARCHIVE);

	r_ambientScale	= ri.cvarget("r_ambientScale", "0.6", CVAR_CHEAT);
	r_directedScale = ri.cvarget("r_directedScale", "1", CVAR_CHEAT);

	r_anaglyphMode = ri.cvarget("r_anaglyphMode", "0", CVAR_ARCHIVE);

	/*
	 * temporary variables that can change at any time
	 *  */
	r_showImages = ri.cvarget("r_showImages", "0", CVAR_TEMP);

	r_debugLight	= ri.cvarget("r_debuglight", "0", CVAR_TEMP);
	r_debugSort	= ri.cvarget("r_debugSort", "0", CVAR_CHEAT);
	r_printShaders	= ri.cvarget("r_printShaders", "0", 0);
	r_saveFontData	= ri.cvarget("r_saveFontData", "0", 0);

	r_nocurves	= ri.cvarget ("r_nocurves", "0", CVAR_CHEAT);
	r_drawworld	= ri.cvarget ("r_drawworld", "1", CVAR_CHEAT);
	r_lightmap	= ri.cvarget ("r_lightmap", "0", 0);
	r_portalOnly	= ri.cvarget ("r_portalOnly", "0", CVAR_CHEAT);

	r_flareSize	= ri.cvarget ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade	= ri.cvarget ("r_flareFade", "7", CVAR_CHEAT);
	r_flareCoeff	= ri.cvarget ("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);

	r_showSmp = ri.cvarget ("r_showSmp", "0", CVAR_CHEAT);
	r_skipBackEnd = ri.cvarget ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw	= ri.cvarget("r_measureOverdraw", "0", CVAR_CHEAT);
	r_lodscale	= ri.cvarget("r_lodscale", "5", CVAR_CHEAT);
	r_norefresh	= ri.cvarget ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities	= ri.cvarget ("r_drawentities", "1", CVAR_CHEAT);
	r_ignore	= ri.cvarget("r_ignore", "1", CVAR_CHEAT);
	r_nocull	= ri.cvarget ("r_nocull", "0", CVAR_CHEAT);
	r_novis		= ri.cvarget ("r_novis", "0", CVAR_CHEAT);
	r_showcluster	= ri.cvarget ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds	= ri.cvarget ("r_speeds", "0", CVAR_CHEAT);
	r_verbose	= ri.cvarget("r_verbose", "0", CVAR_CHEAT);
	r_logFile	= ri.cvarget("r_logFile", "0", CVAR_CHEAT);
	r_debugSurface	= ri.cvarget ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind	= ri.cvarget ("r_nobind", "0", CVAR_CHEAT);
	r_showtris	= ri.cvarget ("r_showtris", "0", CVAR_CHEAT);
	r_showsky	= ri.cvarget ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals	= ri.cvarget ("r_shownormals", "0", CVAR_CHEAT);
	r_clear		= ri.cvarget ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor	= ri.cvarget("r_offsetfactor", "-1", CVAR_CHEAT);
	r_offsetUnits	= ri.cvarget("r_offsetunits", "-2", CVAR_CHEAT);
	r_drawBuffer	= ri.cvarget("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_lockpvs	= ri.cvarget ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals	= ri.cvarget ("r_noportals", "0", CVAR_CHEAT);
	r_shadows	= ri.cvarget("cg_shadows", "1", 0);

	r_marksOnTriangleMeshes = ri.cvarget("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);

	r_aviMotionJpegQuality	= ri.cvarget("r_aviMotionJpegQuality", "90", CVAR_ARCHIVE);
	r_screenshotJpegQuality = ri.cvarget("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	r_maxpolys = ri.cvarget("r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = ri.cvarget("r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);

	/* make sure all the commands added here are also
	 * removed in R_Shutdown */
	ri.cmdadd("imagelist", R_ImageList_f);
	ri.cmdadd("shaderlist", R_ShaderList_f);
	ri.cmdadd("skinlist", R_SkinList_f);
	ri.cmdadd("modellist", R_Modellist_f);
	ri.cmdadd("modelist", R_ModeList_f);
	ri.cmdadd("screenshot", R_ScreenShot_f);
	ri.cmdadd("screenshotJPEG", R_ScreenShotJPEG_f);
	ri.cmdadd("gfxinfo", GfxInfo_f);
	ri.cmdadd("minimize", GLimp_Minimize);
}

/*
 * R_Init
 */
void
R_Init(void)
{
	int	err;
	int	i;
	byte *ptr;

	ri.Printf(PRINT_ALL, "----- R_Init -----\n");

	/* clear all our internal state */
	Q_Memset(&tr, 0, sizeof(tr));
	Q_Memset(&backEnd, 0, sizeof(backEnd));
	Q_Memset(&tess, 0, sizeof(tess));

	if(sizeof(Glconfig) != 11332)
		ri.Error(ERR_FATAL, "Mod ABI incompatible: sizeof(Glconfig) == %u != 11332",
			(unsigned int)sizeof(Glconfig));

/*	Swap_Init(); */

	if((intptr_t)tess.xyz & 15){
		ri.Printf(PRINT_WARNING, "tess.xyz not 16 byte aligned\n");
	}
	Q_Memset(tess.constantColor255, 255, sizeof(tess.constantColor255));

	/*
	 * init function tables
	 *  */
	for(i = 0; i < FUNCTABLE_SIZE; i++){
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / (( float )(FUNCTABLE_SIZE - 1))));
		tr.squareTable[i]	= (i < FUNCTABLE_SIZE/2) ? 1.0f : -1.0f;
		tr.sawToothTable[i]	= (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if(i < FUNCTABLE_SIZE / 2){
			if(i < FUNCTABLE_SIZE / 4){
				tr.triangleTable[i] = ( float )i / (FUNCTABLE_SIZE / 4);
			}else{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}else{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = r_maxpolys->integer;
	if(max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if(max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = ri.hunkalloc(
		sizeof(*backEndData[0]) + sizeof(srfPoly_t) * max_polys + sizeof(Polyvert) * max_polyverts,
		Hlow);
	backEndData[0] = (backEndData_t*)ptr;
	backEndData[0]->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData[0]));
	backEndData[0]->polyVerts =
		(Polyvert*)((char*)ptr + sizeof(*backEndData[0]) + sizeof(srfPoly_t) * max_polys);
	if(r_smp->integer){
		ptr =
			ri.hunkalloc(
				sizeof(*backEndData[1]) + sizeof(srfPoly_t) * max_polys +
				sizeof(Polyvert) * max_polyverts,
				Hlow);
		backEndData[1] = (backEndData_t*)ptr;
		backEndData[1]->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData[1]));
		backEndData[1]->polyVerts =
			(Polyvert*)((char*)ptr + sizeof(*backEndData[1]) + sizeof(srfPoly_t) * max_polys);
	}else{
		backEndData[1] = NULL;
	}
	R_ToggleSmpFrame();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();


	err = qglGetError();
	if(err != GL_NO_ERROR)
		ri.Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	ri.Printf(PRINT_ALL, "----- finished R_Init -----\n");
}

/*
 * RE_Shutdown
 */
void
RE_Shutdown(qbool destroyWindow)
{

	ri.Printf(PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow);

	ri.cmdremove ("modellist");
	ri.cmdremove ("screenshotJPEG");
	ri.cmdremove ("screenshot");
	ri.cmdremove ("imagelist");
	ri.cmdremove ("shaderlist");
	ri.cmdremove ("skinlist");
	ri.cmdremove ("gfxinfo");
	ri.cmdremove("minimize");
	ri.cmdremove("modelist");
	ri.cmdremove("shaderstate");


	if(tr.registered){
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	/* shut down platform specific OpenGL stuff */
	if(destroyWindow){
		GLimp_Shutdown();
	}

	tr.registered = qfalse;
}


/*
 * RE_EndRegistration
 *
 * Touch all images to make sure they are resident
 */
void
RE_EndRegistration(void)
{
	R_SyncRenderThread();
	if(!ri.syslowmem()){
		RB_ShowImages();
	}
}


/*
 * @@@@@@@@@@@@@@@@@@@@@
 * GetRefAPI
 *
 * @@@@@@@@@@@@@@@@@@@@@
 */
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT Refexport QDECL *
GetRefAPI(int apiVersion, Refimport *rimp)
{
#else
Refexport *
GetRefAPI(int apiVersion, Refimport *rimp)
{
#endif

	static Refexport re;

	ri = *rimp;

	Q_Memset(&re, 0, sizeof(re));

	if(apiVersion != REF_API_VERSION){
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
			REF_API_VERSION, apiVersion);
		return NULL;
	}

	/* the RE_ functions are Renderer Entry points */

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel	= RE_RegisterModel;
	re.RegisterSkin		= RE_RegisterSkin;
	re.RegisterShader	= RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData	= RE_SetWorldVisData;
	re.EndRegistration	= RE_EndRegistration;

	re.BeginFrame	= RE_BeginFrame;
	re.EndFrame	= RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene	= RE_AddRefEntityToScene;
	re.AddPolyToScene	= RE_AddPolyToScene;
	re.LightForPoint	= R_LightForPoint;
	re.AddLightToScene	= RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic	= RE_StretchPic;
	re.DrawStretchRaw	= RE_StretchRaw;
	re.UploadCinematic	= RE_UploadCinematic;

	re.RegisterFont		= RE_RegisterFont;
	re.RemapShader		= R_RemapShader;
	re.GetEntityToken	= R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	return &re;
}
