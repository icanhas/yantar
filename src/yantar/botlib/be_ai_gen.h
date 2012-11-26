/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*  */

/*****************************************************************************
* name:		be_ai_gen.h
*
* desc:		genetic selection
*
* $Archive: /source/code/botlib/be_ai_gen.h $
*
*****************************************************************************/

int GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1,
				    int *parent2,
				    int *child);
