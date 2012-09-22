/*
 * ===========================================================================
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 * ===========================================================================
 */

/* snddma_null.c
 * all other sound mixing is portable */

#include "q_shared.h"
#include "qcommon.h"

qbool
SNDDMA_Init(void)
{
	return qfalse;
}

int
SNDDMA_GetDMAPos(void)
{
	return 0;
}

void
SNDDMA_Shutdown(void)
{
}

void
SNDDMA_BeginPainting(void)
{
}

void
SNDDMA_Submit(void)
{
}

sfxHandle_t
S_RegisterSound(const char *name, qbool compressed)
{
	return 0;
}

void
S_StartLocalSound(sfxHandle_t sfxHandle, int channelNum)
{
}

void
S_ClearSoundBuffer(void)
{
}
