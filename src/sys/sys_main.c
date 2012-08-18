/*
 * Copyright (C) 1999-2005 Id Software, Inc.
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

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifndef DEDICATED
#ifdef USE_LOCAL_HEADERS
#       include "SDL.h"
#       include "SDL_cpuinfo.h"
#else
#       include <SDL.h>
#       include <SDL_cpuinfo.h>
#endif
#endif

#include "sys_local.h"
#include "sys_loadlib.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };

void
Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

char *
Sys_BinaryPath(void)
{
	return binaryPath;
}

void
Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

char *
Sys_DefaultInstallPath(void)
{
	if(*installPath)
		return installPath;
	else
		return Sys_Cwd();
}

char *
Sys_DefaultAppPath(void)
{
	return Sys_BinaryPath();
}

/* Restart the input subsystem */
void
Sys_In_Restart_f(void)
{
	IN_Restart( );
}

/* Handle new console input */
char *
Sys_ConsoleInput(void)
{
	return CON_Input( );
}

/* Single exit point (regular exit or in case of error) */
static __attribute__ ((noreturn)) void
Sys_Exit(int exitCode)
{
	CON_Shutdown( );
#ifndef DEDICATED
	SDL_Quit( );
#endif
	Sys_PlatformExit( );
	exit(exitCode);
}

void
Sys_Quit(void)
{
	Sys_Exit(0);
}

cpuFeatures_t
Sys_GetProcessorFeatures(void)
{
	cpuFeatures_t f;
	
	f = 0;
#ifndef DEDICATED
	if(SDL_HasRDTSC()) f |= CF_RDTSC;
	if(SDL_HasMMX()) f |= CF_MMX;
	if(SDL_HasMMXExt()) f |= CF_MMX_EXT;
	if(SDL_Has3DNow()) f |= CF_3DNOW;
	if(SDL_Has3DNowExt()) f |= CF_3DNOW_EXT;
	if(SDL_HasSSE()) f |= CF_SSE;
	if(SDL_HasSSE2()) f |= CF_SSE2;
	if(SDL_HasAltiVec()) f |= CF_ALTIVEC;
#endif
	return f;

}

void
Sys_Init(void)
{
	char pidstr[16];

	Cmd_AddCommand("in_restart", Sys_In_Restart_f);
	Cvar_Set("arch", OS_STRING " " ARCH_STRING);
	Cvar_Set("username", Sys_GetCurrentUser( ));
	Q_sprintf(pidstr, sizeof(pidstr), "%d", Sys_PID());
	Cvar_Get("pid", pidstr, CVAR_ROM);
	Cvar_SetDesc("pid", "process ID, for debugging purposes");
}

/* Transform Q3 colour codes to ANSI escape sequences */
void
Sys_AnsiColorPrint(const char *msg)
{
	static char buffer[ MAXPRINTMSG ];
	int length = 0;
	static int	q3ToAnsi[ 8 ] =
	{
		30,	/* COLOR_BLACK */
		31,	/* COLOR_RED */
		32,	/* COLOR_GREEN */
		33,	/* COLOR_YELLOW */
		34,	/* COLOR_BLUE */
		36,	/* COLOR_CYAN */
		35,	/* COLOR_MAGENTA */
		0	/* COLOR_WHITE */
	};

	while(*msg){
		if(Q_IsColorString(msg) || *msg == '\n'){
			/* First empty the buffer */
			if(length > 0){
				buffer[ length ] = '\0';
				fputs(buffer, stderr);
				length = 0;
			}

			if(*msg == '\n'){
				/* Issue a reset and then the newline */
				fputs("\033[0m\n", stderr);
				msg++;
			}else{
				/* Print the color code */
				Q_sprintf(buffer, sizeof(buffer), "\033[%dm",
					q3ToAnsi[ ColorIndex(*(msg + 1)) ]);
				fputs(buffer, stderr);
				msg += 2;
			}
		}else{
			if(length >= MAXPRINTMSG - 1)
				break;

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}
	/* Empty anything still left in the buffer */
	if(length > 0){
		buffer[ length ] = '\0';
		fputs(buffer, stderr);
	}
}

void
Sys_Print(const char *msg)
{
	CON_LogWrite(msg);
	CON_Print(msg);
}

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char	string[1024];

	va_start (argptr,error);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	Sys_ErrorDialog(string);
	Sys_Exit(3);
}

#if 0
static __attribute__ ((format (printf, 1, 2))) void
Sys_Warn(char *warning, ...)
{
	va_list argptr;
	char	string[1024];

	va_start (argptr,warning);
	Q_vsnprintf (string, sizeof(string), warning, argptr);
	va_end (argptr);

	CON_Print(va("Warning: %s", string));
}
#endif

/* returns -1 if not present */
int
Sys_FileTime(char *path)
{
	struct stat buf;

	if(stat (path,&buf) == -1)
		return -1;
	return buf.st_mtime;
}

void
Sys_UnloadDll(void *dllHandle)
{
	if(!dllHandle){
		Q_Printf("Sys_UnloadDll(NULL)\n");
		return;
	}

	Sys_UnloadLibrary(dllHandle);
}

/*
 * First try to load library name from system library path,
 * from executable path, then fs_basepath.
 */

void *
Sys_LoadDll(const char *name, qbool useSystemLib)
{
	void *dllhandle;

	if(useSystemLib)
		Q_Printf("Trying to load \"%s\"...\n", name);

	if(!useSystemLib || !(dllhandle = Sys_LoadLibrary(name))){
		const char *topDir;
		char libPath[MAX_OSPATH];

		topDir = Sys_BinaryPath();

		if(!*topDir)
			topDir = ".";

		Q_Printf("Trying to load \"%s\" from \"%s\"...\n", name,
			topDir);
		Q_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP,
			name);

		if(!(dllhandle = Sys_LoadLibrary(libPath))){
			const char *basePath = Cvar_VariableString("fs_basepath");

			if(!basePath || !*basePath)
				basePath = ".";

			if(FS_FilenameCompare(topDir, basePath)){
				Q_Printf(
					"Trying to load \"%s\" from \"%s\"...\n",
					name,
					basePath);
				Q_sprintf(libPath, sizeof(libPath), "%s%c%s",
					basePath, PATH_SEP,
					name);
				dllhandle = Sys_LoadLibrary(libPath);
			}

			if(!dllhandle)
				Q_Printf("Loading \"%s\" failed\n", name);
		}
	}
	return dllhandle;
}

/* Used to load a development dll instead of a virtual machine */
void *
Sys_LoadGameDll(const char *name,
	intptr_t (QDECL **entryPoint) (int, ...),
	intptr_t (*systemcalls)(intptr_t, ...))
{
	void	*libHandle;
	void	(*dllEntry)(intptr_t (*syscallptr)(intptr_t, ...));

	assert(name);
	Q_Printf("Loading DLL file: %s\n", name);
	libHandle = Sys_LoadLibrary(name);

	if(!libHandle){
		Q_Printf("Sys_LoadGameDll(%s) failed:\n\"%s\"\n", name,
			Sys_LibraryError());
		return NULL;
	}

	dllEntry = Sys_LoadFunction(libHandle, "dllEntry");
	*entryPoint = Sys_LoadFunction(libHandle, "vmMain");

	if(!*entryPoint || !dllEntry){
		Q_Printf (
			"Sys_LoadGameDll(%s) failed to find vmMain function:\n\"%s\" !\n",
			name, Sys_LibraryError( ));
		Sys_UnloadLibrary(libHandle);

		return NULL;
	}

	Q_Printf ("Sys_LoadGameDll(%s) found vmMain function at %p\n", name,
		*entryPoint);
	dllEntry(systemcalls);
	return libHandle;
}

void
Sys_ParseArgs(int argc, char **argv)
{
	if(argc == 2){
		if(!strcmp(argv[1], "--version") ||
		   !strcmp(argv[1], "-v")){
			const char * date = __DATE__;
#ifdef DEDICATED
			fprintf(stdout, Q3_VERSION " dedicated server (%s)\n",
				date);
#else
			fprintf(stdout, Q3_VERSION " client (%s)\n", date);
#endif
			Sys_Exit(0);
		}
	}
}

#ifndef DEFAULT_BASEDIR
#       ifdef MACOS_X
#               define DEFAULT_BASEDIR	Sys_StripAppBundle(Sys_BinaryPath())
#       else
#               define DEFAULT_BASEDIR	Sys_BinaryPath()
#       endif
#endif

void
Sys_SigHandler(int signal)
{
	static qbool signalcaught = qfalse;

	if(signalcaught)
		fprintf(stderr,
			"DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n",
			signal);
	else{
		signalcaught = qtrue;
		VM_Forced_Unload_Start();
#ifndef DEDICATED
		CL_Shutdown(va("Received signal %d", signal), qtrue, qtrue);
#endif
		SV_Shutdown(va("Received signal %d", signal));
		VM_Forced_Unload_Done();
	}

	if(signal == SIGTERM || signal == SIGINT)
		Sys_Exit(1);
	else
		Sys_Exit(2);
}

int
main(int argc, char **argv)
{
	int i;
	char commandLine[ MAX_STRING_CHARS ] = { 0 };

#ifndef DEDICATED
	/* SDL version check */

	/* Compile time */
#       if !SDL_VERSION_ATLEAST(MINSDL_MAJOR,MINSDL_MINOR,MINSDL_PATCH)
#               error A more recent version of SDL is required
#       endif

	/* Run time */
	const SDL_version *ver = SDL_Linked_Version( );

#define MINSDL_VERSION \
	XSTRING(MINSDL_MAJOR) "." \
	XSTRING(MINSDL_MINOR) "." \
	XSTRING(MINSDL_PATCH)

	if(SDL_VERSIONNUM(ver->major, ver->minor, ver->patch) <
	   SDL_VERSIONNUM(MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH)){
		Sys_Dialog(DT_ERROR,
			va(
				"SDL version " MINSDL_VERSION
				" or greater is required, "
				"but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
				"from http://www.libsdl.org/.",
				ver->major, ver->minor,
				ver->patch), "SDL Library Too Old");

		Sys_Exit(1);
	}
#endif
	Sys_PlatformInit( );
	/* Set the initial time base */
	Sys_Milliseconds( );
	Sys_ParseArgs(argc, argv);
	Sys_SetBinaryPath(Sys_Dirname(argv[ 0 ]));
	Sys_SetDefaultInstallPath(DEFAULT_BASEDIR);

	/* Concatenate the command line for passing to Q_Init */
	for(i = 1; i < argc; i++){
		const qbool containsSpaces = strchr(argv[i], ' ') != NULL;
		if(containsSpaces)
			Q_strcat(commandLine, sizeof(commandLine), "\"");

		Q_strcat(commandLine, sizeof(commandLine), argv[ i ]);

		if(containsSpaces)
			Q_strcat(commandLine, sizeof(commandLine), "\"");

		Q_strcat(commandLine, sizeof(commandLine), " ");
	}

	Q_Init(commandLine);
	NET_Init( );
	CON_Init( );

	signal(SIGILL, Sys_SigHandler);
	signal(SIGFPE, Sys_SigHandler);
	signal(SIGSEGV, Sys_SigHandler);
	signal(SIGTERM, Sys_SigHandler);
	signal(SIGINT, Sys_SigHandler);

	for(;;){
		IN_Frame( );
		Q_Frame( );
	}
	return 0;
}
