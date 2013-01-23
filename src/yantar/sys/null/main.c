/*
 * ===========================================================================
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 * ===========================================================================
 */
/* sys_null.h -- null system driver to aid porting efforts */

#include <errno.h>
#include <stdio.h>
#include "common.h"

int sys_curtime;


/* =================================================================== */

void
Sys_BeginStreamedFile(FILE *f, int readAhead)
{
}

void
Sys_EndStreamedFile(FILE *f)
{
}

int
Sys_StreamedRead(void *buffer, int size, int count, FILE *f)
{
	return fread(buffer, size, count, f);
}

void
Sys_StreamSeek(FILE *f, int offset, int origin)
{
	fseek(f, offset, origin);
}


/* =================================================================== */


void
Sys_mkdir(const char *path)
{
}

void
syserrorf(char *error, ...)
{
	va_list argptr;

	printf ("syserrorf: ");
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void
sysquit(void)
{
	exit (0);
}

void
sysunloadgame(void)
{
}

void    *
sysgetgameapi(void *parms)
{
	return NULL;
}

char *
sysgetclipboarddata(void)
{
	return NULL;
}

int
sysmillisecs(void)
{
	return 0;
}

void
sysmkdir(char *path)
{
}

char    *
Sys_FindFirst(char *path, unsigned musthave, unsigned canthave)
{
	return NULL;
}

char    *
Sys_FindNext(unsigned musthave, unsigned canthave)
{
	return NULL;
}

void
Sys_FindClose(void)
{
}

void
sysinit(void)
{
}


void
Sys_EarlyOutput(char *string)
{
	printf("%s", string);
}


void
main(int argc, char **argv)
{
	cominit (argc, argv);

	while(1)
		comframe( );
}
