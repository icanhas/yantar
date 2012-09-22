/*
 * ===========================================================================
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 * ===========================================================================
 */
#include "../../renderer/tr_local.h"


qbool (* qwglSwapIntervalEXT)(int interval);
void	(* qglMultiTexCoord2fARB)(GLenum texture, float s, float t);
void	(* qglActiveTextureARB)(GLenum texture);
void	(* qglClientActiveTextureARB)(GLenum texture);


void	(* qglLockArraysEXT)(int, int);
void	(* qglUnlockArraysEXT)(void);


void
GLimp_EndFrame(void)
{
}

int
GLimp_Init(void)
{
}

void
GLimp_Shutdown(void)
{
}

void
GLimp_EnableLogging(qbool enable)
{
}

void
GLimp_LogComment(char *comment)
{
}

qbool
QGL_Init(const char *dllname)
{
	return qtrue;
}

void
QGL_Shutdown(void)
{
}
