/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "shared.h"
#include "botlib.h"
#include "be_interface.h"	/* for botimport.Print */
#include "l_libvar.h"
#include "l_log.h"

#define MAX_LOGFILENAMESIZE 1024

static struct {
	char	filename[MAX_LOGFILENAMESIZE];
	FILE	*fp;
	int	numwrites;
} logfile;

/* open a log file */
void
Log_Open(char *filename)
{
	if(!LibVarValue("log", "0")) return;
	if(!filename || !strlen(filename)){
		botimport.Print(PRT_MESSAGE, "openlog <filename>\n");
		return;
	}
	if(logfile.fp){
		botimport.Print(PRT_ERROR, "log file %s is already opened\n",
			logfile.filename);
		return;
	}
	logfile.fp = fopen(filename, "wb");
	if(!logfile.fp){
		botimport.Print(PRT_ERROR, "can't open the log file %s\n",
			filename);
		return;
	}
	strncpy(logfile.filename, filename, MAX_LOGFILENAMESIZE);
	botimport.Print(PRT_MESSAGE, "Opened log %s\n", logfile.filename);
}

/* close the current log file */
void
Log_Close(void)
{
	if(!logfile.fp) return;
	if(fclose(logfile.fp)){
		botimport.Print(PRT_ERROR, "can't close log file %s\n",
			logfile.filename);
		return;
	}
	logfile.fp = NULL;
	botimport.Print(PRT_MESSAGE, "Closed log %s\n", logfile.filename);
}

/* close log file if present */
void
Log_Shutdown(void)
{
	if(logfile.fp) Log_Close();
}

/* write to the current opened log file */
void QDECL
Log_Write(char *fmt, ...)
{
	va_list ap;

	if(!logfile.fp) return;
	va_start(ap, fmt);
	vfprintf(logfile.fp, fmt, ap);
	va_end(ap);
	/* fprintf(logfile.fp, "\r\n"); */
	fflush(logfile.fp);
}

/* write to the current opened log file with a time stamp */
void QDECL
Log_WriteTimeStamped(char *fmt, ...)
{
	va_list ap;

	if(!logfile.fp) return;
	fprintf(logfile.fp, "%d   %02d:%02d:%02d:%02d   ",
		logfile.numwrites,
		(int)(botlibglobals.time / 60 / 60),
		(int)(botlibglobals.time / 60),
		(int)(botlibglobals.time),
		(int)((int)(botlibglobals.time * 100)) -
		((int)botlibglobals.time) * 100);
	va_start(ap, fmt);
	vfprintf(logfile.fp, fmt, ap);
	va_end(ap);
	fprintf(logfile.fp, "\r\n");
	logfile.numwrites++;
	fflush(logfile.fp);
}

/* returns a pointer to the log file */
FILE *
Log_FilePointer(void)
{
	return logfile.fp;
}

/* flush log file */
void
Log_Flush(void)
{
	if(logfile.fp) fflush(logfile.fp);
}
