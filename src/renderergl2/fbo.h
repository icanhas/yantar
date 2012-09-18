/*
 * Copyright (C) 2010 James Canete (use.less01@gmail.com)
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
