/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		l_util.h
*
* desc:		utils
*
* $Archive: /source/code/botlib/l_util.h $
*
*****************************************************************************/

#define Vector2Angles(v,a) v3toeuler(v,a)
#ifndef MAX_PATH
#define MAX_PATH MAX_QPATH
#endif
#define Maximum(x,y)	(x > y ? x : y)
#define Minimum(x,y)	(x < y ? x : y)
