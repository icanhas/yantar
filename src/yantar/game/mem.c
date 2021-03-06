/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "bg.h"
#include "game.h"
#include "local.h"

#define POOLSIZE (256 * 1024)

static char	memoryPool[POOLSIZE];
static int	allocPoint;

void *
G_Alloc(int size)
{
	char *p;

	if(g_debugAlloc.integer)
		G_Printf("G_Alloc of %i bytes (%i left)\n", size,
			POOLSIZE - allocPoint - ((size + 31) & ~31));

	if(allocPoint + size > POOLSIZE){
		G_Error("G_Alloc: failed on allocation of %i bytes\n", size);
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += (size + 31) & ~31;

	return p;
}

void
G_InitMemory(void)
{
	allocPoint = 0;
}

void
Svcmd_GameMem_f(void)
{
	G_Printf("Game memory status: %i out of %i bytes allocated\n",
		allocPoint,
		POOLSIZE);
}
