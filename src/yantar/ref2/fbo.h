/*
 * Copyright (C) 2010 James Canete (use.less01@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* fbo.h */

#ifndef __TR_FBO_H__
#define __TR_FBO_H__

struct image_s;
struct shaderProgram_s;

typedef struct FBO_s {
	char		name[MAX_QPATH];

	int		index;

	uint32_t	frameBuffer;

	uint32_t	colorBuffers[16];
	int		colorFormat;
	struct image_s	*colorImage[16];

	uint32_t	depthBuffer;
	int		depthFormat;

	uint32_t	stencilBuffer;
	int		stencilFormat;

	uint32_t	packedDepthStencilBuffer;
	int		packedDepthStencilFormat;

	int		width;
	int		height;
} FBO_t;

void FBO_Bind(FBO_t *fbo);
void FBO_Init(void);
void FBO_Shutdown(void);

void FBO_BlitFromTexture(struct image_s *src, Vec4 srcBox, Vec2 srcTexScale, FBO_t *dst, Vec4 dstBox,
			 struct shaderProgram_s *shaderProgram, Vec4 color,
			 int blend);
void FBO_Blit(FBO_t *src, Vec4 srcBox, Vec2 srcTexScale, FBO_t *dst, Vec4 dstBox,
	      struct shaderProgram_s *shaderProgram, Vec4 color,
	      int blend);


#endif
