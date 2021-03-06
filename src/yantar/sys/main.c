/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef DEDICATED
#  include <SDL/SDL.h>
#  include <SDL/SDL_cpuinfo.h>
#endif
#include "local.h"
#include "loadlib.h"
#include "shared.h"
#include "common.h"

static char binaryPath[MAX_OSPATH] = { 0 };
static char installPath[MAX_OSPATH] = { 0 };

static int q3ToAnsi[8] =
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

static char*
signame(int sig)
{
	switch(sig){
	case SIGABRT:	return "SIGABRT";
	case SIGFPE:	return "SIGFPE";
	case SIGILL:	return "SIGILL";
	case SIGINT:	return "SIGINT";
	case SIGSEGV:	return "SIGSEGV";
	case SIGTERM:	return "SIGTERM";
	default:	return "unknown";
	}
}

void
Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

char*
Sys_BinaryPath(void)
{
	return binaryPath;
}

void
syssetdefaultinstallpath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

char*
sysgetdefaultinstallpath(void)
{
	if(*installPath)
		return installPath;
	else
		return syspwd();
}

char*
sysgetdefaultapppath(void)
{
	return Sys_BinaryPath();
}

/* Restart the input subsystem */
void
Sys_In_Restart_f(void)
{
	IN_Restart();
}

/* Handle new console input */
char *
sysconsoleinput(void)
{
	return CON_Input();
}

/* Single exit point (regular exit or in case of error) */
static __attribute__ ((noreturn)) void
Sys_Exit(int exitCode)
{
	CON_Shutdown();
#ifndef DEDICATED
	SDL_Quit();
#endif
	Sys_PlatformExit();
	exit(exitCode);
}

void
sysquit(void)
{
	Sys_Exit(0);
}

CPUfeatures
sysgetprocessorfeatures(void)
{
	CPUfeatures f;
	
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
sysinit(void)
{
	char pidbuf[16];

	cmdadd("in_restart", Sys_In_Restart_f);
	cvarsetstr("arch", OS_STRING " " ARCH_STRING);
	cvarsetstr("username", sysgetcurrentuser());
	Q_sprintf(pidbuf, sizeof(pidbuf), "%d", Sys_PID());
	cvarget("pid", pidbuf, CVAR_ROM);
	cvarsetdesc("pid", "process ID, for debugging purposes");
}

/* Transform Q3 colour codes to ANSI escape sequences */
void
Sys_AnsiColorPrint(const char *msg)
{
	static char buffer[MAXPRINTMSG];
	int length = 0;

	while(*msg){
		if(Q_IsColorString(msg) || *msg == '\n'){
			/* First empty the buffer */
			if(length > 0){
				buffer[length] = '\0';
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

			buffer[length] = *msg;
			length++;
			msg++;
		}
	}
	/* Empty anything still left in the buffer */
	if(length > 0){
		buffer[length] = '\0';
		fputs(buffer, stderr);
	}
}

void
sysprint(const char *msg)
{
	CON_LogWrite(msg);
	CON_Print(msg);
}

void
syserrorf(const char *error, ...)
{
	va_list argptr;
	char	string[1024];

	va_start(argptr,error);
	Q_vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);

	syserrorfDialog(string);
	Sys_Exit(3);
}

/* FIXME */
__attribute__ ((format (printf, 1, 2))) void
Sys_Warn(const char *warning, ...)
{
	va_list argptr;
	char	string[1024];

	va_start(argptr, warning);
	Q_vsnprintf(string, sizeof(string), warning, argptr);
	va_end(argptr);

	CON_Print(va("Warning: %s", string));
}

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
sysunloaddll(void *dllHandle)
{
	if(!dllHandle){
		comprintf("sysunloaddll(NULL)\n");
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
		comprintf("Trying to load \"%s\"...\n", name);

	if(!useSystemLib || !(dllhandle = Sys_LoadLibrary(name))){
		const char *topDir;
		char libPath[MAX_OSPATH];

		topDir = Sys_BinaryPath();

		if(!*topDir)
			topDir = ".";

		comprintf("Trying to load \"%s\" from \"%s\"...\n", name,
			topDir);
		Q_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP,
			name);

		if(!(dllhandle = Sys_LoadLibrary(libPath))){
			const char *basePath = cvargetstr("fs_basepath");

			if(!basePath || !*basePath)
				basePath = ".";

			if(fscomparefname(topDir, basePath)){
				comprintf("Trying to load \"%s\" from \"%s\"...\n",
					name, basePath);
				Q_sprintf(libPath, sizeof(libPath), "%s%c%s",
					basePath, PATH_SEP, name);
				dllhandle = Sys_LoadLibrary(libPath);
			}

			if(!dllhandle)
				comprintf("Loading \"%s\" failed\n", name);
		}
	}
	return dllhandle;
}

/* Used to load a development dll instead of a virtual machine */
void *
sysloadgamedll(const char *name,
	intptr_t (QDECL **entryPoint) (int, ...),
	intptr_t (*systemcalls)(intptr_t, ...))
{
	void	*libHandle;
	void	(*dllEntry)(intptr_t (*syscallptr)(intptr_t, ...));

	assert(name);
	comprintf("Loading DLL file: %s\n", name);
	libHandle = Sys_LoadLibrary(name);

	if(!libHandle){
		comprintf("sysloadgamedll(%s) failed:\n\"%s\"\n", name,
			Sys_LibraryError());
		return NULL;
	}

	dllEntry = Sys_LoadFunction(libHandle, "dllEntry");
	*entryPoint = Sys_LoadFunction(libHandle, "vmMain");

	if(!*entryPoint || !dllEntry){
		comprintf (
			"sysloadgamedll(%s) failed to find vmMain function:\n\"%s\" !\n",
			name, Sys_LibraryError( ));
		Sys_UnloadLibrary(libHandle);

		return NULL;
	}

	comprintf ("sysloadgamedll(%s) found vmMain function at %p\n", name,
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
			const char *date = __DATE__;
			
			printf(Q3_VERSION " (%s)\n", date);
			fflush(stdout);
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
	const char *name;
	
	name = signame(signal);
	fprintf(stderr, "GOOFED: received signal %d (%s)", signal, name);
	if(signalcaught)
		fprintf(stderr,
			"DOUBLE SIGNAL FAULT: received signal %d (%s), exiting...\n",
			signal, name);
	else{
		signalcaught = qtrue;
		vmsetforceunload();
		if(!com_dedicated->integer)
			clshutdown(va("received signal %d (%s)", signal, name), qtrue, qtrue);
		svshutdown(va("received signal %d (%s)", signal, name));
		vmclearforceunload();
	}
	fflush(stdout);
	fflush(stderr);
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
	const SDL_version *ver = SDL_Linked_Version();

#define MINSDL_VERSION \
	XSTRING(MINSDL_MAJOR) "." \
	XSTRING(MINSDL_MINOR) "." \
	XSTRING(MINSDL_PATCH)

	if(SDL_VERSIONNUM(ver->major, ver->minor, ver->patch) <
	   SDL_VERSIONNUM(MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH)){
		sysmkdialog(DT_ERROR,
			va(
				"SDL version " MINSDL_VERSION
				" or greater is required, "
				"but only version %d.%d.%d was found.",
			ver->major, ver->minor,
			ver->patch), "SDL library too old");
		Sys_Exit(1);
	}
	syssetenv("SDL_DISABLE_LOCK_KEYS", "1");
#endif
	Sys_PlatformInit( );
	/* Set the initial time base */
	sysmillisecs( );
	Sys_ParseArgs(argc, argv);
	Sys_SetBinaryPath(sysdirname(argv[ 0 ]));
	syssetdefaultinstallpath(DEFAULT_BASEDIR);

	/* Concatenate the command line for passing to cominit */
	for(i = 1; i < argc; i++){
		const qbool containsSpaces = strchr(argv[i], ' ') != NULL;
		if(containsSpaces)
			Q_strcat(commandLine, sizeof(commandLine), "\"");

		Q_strcat(commandLine, sizeof(commandLine), argv[ i ]);

		if(containsSpaces)
			Q_strcat(commandLine, sizeof(commandLine), "\"");

		Q_strcat(commandLine, sizeof(commandLine), " ");
	}

	cominit(commandLine);
	netinit();
	CON_Init();

	signal(SIGILL, Sys_SigHandler);
	signal(SIGFPE, Sys_SigHandler);
	signal(SIGSEGV, Sys_SigHandler);
	signal(SIGTERM, Sys_SigHandler);
	signal(SIGINT, Sys_SigHandler);

	for(;;){
		IN_Frame();
		comframe();
	}
	return 0;
}
