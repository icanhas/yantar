/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		be_interface.h
*
* desc:		botlib interface
*
* $Archive: /source/code/botlib/be_interface.h $
*
*****************************************************************************/

/* #define DEBUG			//debug code */
#define RANDOMIZE	/* randomize bot behaviour */

/* FIXME: get rid of this global structure */
typedef struct botlib_globals_s {
	int		botlibsetup;	/* true when the bot library has been setup */
	int		maxentities;	/* maximum number of entities */
	int		maxclients;	/* maximum number of clients */
	float		time;		/* the global time */
#ifdef DEBUG
	qbool		debug;	/* true if debug is on */
	int		goalareanum;
	Vec3		goalorigin;
	int		runai;
#endif
} botlib_globals_t;


extern botlib_globals_t botlibglobals;
extern botlib_import_t	botimport;
extern int botDeveloper;	/* true if developer is on */

int Sys_MilliSeconds(void);
