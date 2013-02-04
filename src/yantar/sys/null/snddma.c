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

#include "shared.h"
#include "common.h"

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

Handle
sndregister(const char *name, qbool compressed)
{
	return 0;
}

void
sndstartlocalsound(Handle sfxHandle, int channelNum)
{
}

void
sndclearbuf(void)
{
}
