/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "../local.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <fenv.h>
#include <sys/wait.h>

qbool stdinIsATTY;

/* Used to determine where to store user-specific files */
static char homePath[ MAX_OSPATH ] = { 0 };

/*
 * sysgetdefaulthomepath
 */
char *
sysgetdefaulthomepath(void)
{
	char *p;

	if(!*homePath){
		if((p = getenv("HOME")) != NULL){
			Q_sprintf(homePath, sizeof(homePath), "%s%c", p,
				PATH_SEP);
#ifdef MACOS_X
			Q_strcat(homePath, sizeof(homePath),
				"Library/Application Support/");

			if(com_homepath->string[0])
				Q_strcat(homePath, sizeof(homePath),
					com_homepath->string);
			else
				Q_strcat(homePath, sizeof(homePath),
					HOMEPATH_NAME_MACOSX);
#else
			if(com_homepath->string[0])
				Q_strcat(homePath, sizeof(homePath),
					com_homepath->string);
			else
				Q_strcat(homePath, sizeof(homePath),
					HOMEPATH_NAME_UNIX);
#endif
		}
	}

	return homePath;
}

#ifndef MACOS_X
/*
 * systemppath
 */
const char *
systemppath(void)
{
	const char *TMPDIR = getenv("TMPDIR");

	if(TMPDIR == NULL || TMPDIR[ 0 ] == '\0')
		return "/tmp";
	else
		return TMPDIR;
}
#endif

/*
 * sysmillisecs
 */
/* base time in seconds, that's our origin
 * timeval:tv_sec is an int:
 * assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038 */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
 * NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
 *   0x7fffffff ms - ~24 days
 * although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
 *   (which would affect the wrap period) */
int curtime;
int
sysmillisecs(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if(!sys_timeBase){
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

	return curtime;
}

/*
 * sysrandbytes
 */
qbool
sysrandbytes(byte *string, int len)
{
	FILE *fp;

	fp = fopen("/dev/urandom", "r");
	if(!fp)
		return qfalse;

	if(!fread(string, sizeof(byte), len, fp)){
		fclose(fp);
		return qfalse;
	}

	fclose(fp);
	return qtrue;
}

/*
 * sysgetcurrentuser
 */
char *
sysgetcurrentuser(void)
{
	struct passwd *p;

	if((p = getpwuid(getuid())) == NULL)
		return "player";
	return p->pw_name;
}

/*
 * sysgetclipboarddata
 */
char *
sysgetclipboarddata(void)
{
	return NULL;
}

#define MEM_THRESHOLD 96*1024*1024

/*
 * syslowmem
 *
 * TODO
 */
qbool
syslowmem(void)
{
	return qfalse;
}

/*
 * sysbasename
 */
const char *
sysbasename(char *path)
{
	return basename(path);
}

/*
 * sysdirname
 */
const char *
sysdirname(char *path)
{
	return dirname(path);
}

/*
 * sysmkdir
 */
qbool
sysmkdir(const char *path)
{
	int result = mkdir(path, 0750);

	if(result != 0)
		return errno == EEXIST;

	return qtrue;
}

/*
 * sysmkfifo
 */
FILE *
sysmkfifo(const char *ospath)
{
	FILE	*fifo;
	int	result;
	int	fn;
	struct  stat buf;

	/* if file already exists AND is a pipefile, remove it */
	if(!stat(ospath, &buf) && S_ISFIFO(buf.st_mode))
		fsremove(ospath);

	result = mkfifo(ospath, 0600);
	if(result != 0)
		return NULL;

	fifo = fopen(ospath, "w+");
	if(fifo){
		fn = fileno(fifo);
		fcntl(fn, F_SETFL, O_NONBLOCK);
	}

	return fifo;
}

/*
 * syspwd
 */
char *
syspwd(void)
{
	static char cwd[MAX_OSPATH];

	char *result = getcwd(cwd, sizeof(cwd) - 1);
	if(result != cwd)
		return NULL;

	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
 *
 * DIRECTORY SCANNING
 *
 */

#define MAX_FOUND_FILES 0x1000

/*
 * Sys_ListFilteredFiles
 */
void
Sys_ListFilteredFiles(const char *basedir, char *subdirs, char *filter,
		      char **list,
		      int *numfiles)
{
	char	search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char	filename[MAX_OSPATH];
	DIR	*fdir;
	struct dirent	*d;
	struct stat	st;

	if(*numfiles >= MAX_FOUND_FILES - 1)
		return;

	if(strlen(subdirs))
		Q_sprintf(search, sizeof(search), "%s/%s", basedir, subdirs);
	else
		Q_sprintf(search, sizeof(search), "%s", basedir);

	if((fdir = opendir(search)) == NULL)
		return;

	while((d = readdir(fdir)) != NULL){
		Q_sprintf(filename, sizeof(filename), "%s/%s", search,
			d->d_name);
		if(stat(filename, &st) == -1)
			continue;

		if(st.st_mode & S_IFDIR)
			if(Q_stricmp(d->d_name,
				   ".") && Q_stricmp(d->d_name, "..")){
				if(strlen(subdirs))
					Q_sprintf(newsubdirs,
						sizeof(newsubdirs), "%s/%s",
						subdirs,
						d->d_name);
				else
					Q_sprintf(newsubdirs,
						sizeof(newsubdirs), "%s",
						d->d_name);
				Sys_ListFilteredFiles(basedir, newsubdirs,
					filter, list,
					numfiles);
			}
		if(*numfiles >= MAX_FOUND_FILES - 1)
			break;
		Q_sprintf(filename, sizeof(filename), "%s/%s", subdirs,
			d->d_name);
		if(!filterpath(filter, filename, qfalse))
			continue;
		list[ *numfiles ] = copystr(filename);
		(*numfiles)++;
	}

	closedir(fdir);
}

/*
 * syslistfiles
 */
char **
syslistfiles(const char *directory, const char *extension, char *filter,
	      int *numfiles,
	      qbool wantsubs)
{
	struct dirent *d;
	DIR	*fdir;
	qbool dironly = wantsubs;
	char	search[MAX_OSPATH];
	int	nfiles;
	char	**listCopy;
	char    *list[MAX_FOUND_FILES];
	int	i;
	struct stat st;

	int extLen;

	if(filter){

		nfiles = 0;
		Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

		list[ nfiles ] = NULL;
		*numfiles = nfiles;

		if(!nfiles)
			return NULL;

		listCopy = zalloc((nfiles + 1) * sizeof(*listCopy));
		for(i = 0; i < nfiles; i++)
			listCopy[i] = list[i];
		listCopy[i] = NULL;

		return listCopy;
	}

	if(!extension)
		extension = "";

	if(extension[0] == '/' && extension[1] == 0){
		extension = "";
		dironly = qtrue;
	}

	extLen = strlen(extension);

	/* search */
	nfiles = 0;

	if((fdir = opendir(directory)) == NULL){
		*numfiles = 0;
		return NULL;
	}

	while((d = readdir(fdir)) != NULL){
		Q_sprintf(search, sizeof(search), "%s/%s", directory,
			d->d_name);
		if(stat(search, &st) == -1)
			continue;
		if((dironly && !(st.st_mode & S_IFDIR)) ||
		   (!dironly && (st.st_mode & S_IFDIR)))
			continue;

		if(*extension)
			if(strlen(d->d_name) < extLen ||
			   Q_stricmp(
				   d->d_name + strlen(d->d_name) - extLen,
				   extension))
				continue;	/* didn't match */

		if(nfiles == MAX_FOUND_FILES - 1)
			break;
		list[ nfiles ] = copystr(d->d_name);
		nfiles++;
	}

	list[ nfiles ] = NULL;

	closedir(fdir);

	/* return a copy of the list */
	*numfiles = nfiles;

	if(!nfiles)
		return NULL;

	listCopy = zalloc((nfiles + 1) * sizeof(*listCopy));
	for(i = 0; i < nfiles; i++)
		listCopy[i] = list[i];
	listCopy[i] = NULL;

	return listCopy;
}

/*
 * sysfreefilelist
 */
void
sysfreefilelist(char **list)
{
	int i;

	if(!list)
		return;

	for(i = 0; list[i]; i++)
		zfree(list[i]);

	zfree(list);
}

/*
 * syssleep
 *
 * Block execution for msec or until input is recieved.
 */
void
syssleep(int msec)
{
	if(msec == 0)
		return;

	if(stdinIsATTY){
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);
		if(msec < 0)
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, NULL);
		else{
			struct timeval timeout;

			timeout.tv_sec	= msec/1000;
			timeout.tv_usec = (msec%1000)*1000;
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout);
		}
	}else{
		/* With nothing to select() on, we can't wait indefinitely */
		if(msec < 0)
			msec = 10;

		usleep(msec * 1000);
	}
}

/*
 * syserrorfDialog
 *
 * Display an error message
 */
void
syserrorfDialog(const char *error)
{
	char	buffer[ 1024 ];
	unsigned int size;
	int	f = -1;
	const char *homepath = cvargetstr("fs_homepath");
	const char *gamedir = cvargetstr("fs_game");
	const char *fileName = "crashlog.txt";
	char *ospath = fsbuildospath(homepath, gamedir, fileName);

	sysprint(va("%s\n", error));
	
	if(!com_dedicated->integer)
		sysmkdialog(DT_ERROR, va("%s. See \"%s\" for details.", error,
		   ospath), "Error");

	/* Make sure the write path for the crashlog exists */
	if(fscreatepath(ospath)){
		comprintf("ERROR: couldn't create path '%s' for crash log.\n",
			ospath);
		return;
	}

	/* We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	 * which will come through here, so we don't want to recurse forever by
	 * calling fsopenw()...use the Unix system APIs instead. */
	f = open(ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640);
	if(f == -1){
		comprintf("ERROR: couldn't open %s\n", fileName);
		return;
	}

	/* We're crashing, so we don't care much if write() or close() fails. */
	while((size = CON_LogRead(buffer, sizeof(buffer))) > 0)
		if(write(f, buffer, size) != size){
			comprintf("ERROR: couldn't fully write to %s\n",
				fileName);
			break;
		}

	close(f);
}

#ifndef MACOS_X
static char execBuffer[ 1024 ];
static char	*execBufferPointer;
static char     *execArgv[ 16 ];
static int	execArgc;

/*
 * Sys_ClearExecBuffer
 */
static void
Sys_ClearExecBuffer(void)
{
	execBufferPointer = execBuffer;
	Q_Memset(execArgv, 0, sizeof(execArgv));
	execArgc = 0;
}

/*
 * Sys_AppendToExecBuffer
 */
static void
Sys_AppendToExecBuffer(const char *text)
{
	size_t	size = sizeof(execBuffer) - (execBufferPointer - execBuffer);
	int	length = strlen(text) + 1;

	if(length > size || execArgc >= ARRAY_LEN(execArgv))
		return;

	Q_strncpyz(execBufferPointer, text, size);
	execArgv[ execArgc++ ] = execBufferPointer;

	execBufferPointer += length;
}

/*
 * Sys_Exec
 */
static int
Sys_Exec(void)
{
	pid_t pid = fork( );

	if(pid < 0)
		return -1;

	if(pid){
		/* Parent */
		int exitCode;

		wait(&exitCode);

		return WEXITSTATUS(exitCode);
	}else{
		/* Child */
		execvp(execArgv[ 0 ], execArgv);

		/* Failed to execute */
		exit(-1);

		return -1;
	}
}

/*
 * Sys_ZenityCommand
 */
static void
Sys_ZenityCommand(dialogType_t type, const char *message, const char *title)
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer("zenity");

	switch(type){
	default:
	case DT_INFO:      Sys_AppendToExecBuffer("--info"); break;
	case DT_WARNING:   Sys_AppendToExecBuffer("--warning"); break;
	case DT_ERROR:     Sys_AppendToExecBuffer("--error"); break;
	case DT_YES_NO:
		Sys_AppendToExecBuffer("--question");
		Sys_AppendToExecBuffer("--ok-label=Yes");
		Sys_AppendToExecBuffer("--cancel-label=No");
		break;

	case DT_OK_CANCEL:
		Sys_AppendToExecBuffer("--question");
		Sys_AppendToExecBuffer("--ok-label=OK");
		Sys_AppendToExecBuffer("--cancel-label=Cancel");
		break;
	}

	Sys_AppendToExecBuffer(va("--text=%s", message));
	Sys_AppendToExecBuffer(va("--title=%s", title));
}

/*
 * Sys_KdialogCommand
 */
static void
Sys_KdialogCommand(dialogType_t type, const char *message, const char *title)
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer("kdialog");

	switch(type){
	default:
	case DT_INFO:      Sys_AppendToExecBuffer("--msgbox"); break;
	case DT_WARNING:   Sys_AppendToExecBuffer("--sorry"); break;
	case DT_ERROR:     Sys_AppendToExecBuffer("--error"); break;
	case DT_YES_NO:    Sys_AppendToExecBuffer("--warningyesno"); break;
	case DT_OK_CANCEL: Sys_AppendToExecBuffer("--warningcontinuecancel");
		break;
	}

	Sys_AppendToExecBuffer(message);
	Sys_AppendToExecBuffer(va("--title=%s", title));
}

/*
 * Sys_XmessageCommand
 */
static void
Sys_XmessageCommand(dialogType_t type, const char *message, const char *title)
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer("xmessage");
	Sys_AppendToExecBuffer("-buttons");

	switch(type){
	default:           Sys_AppendToExecBuffer("OK:0"); break;
	case DT_YES_NO:    Sys_AppendToExecBuffer("Yes:0,No:1"); break;
	case DT_OK_CANCEL: Sys_AppendToExecBuffer("OK:0,Cancel:1"); break;
	}

	Sys_AppendToExecBuffer("-center");
	Sys_AppendToExecBuffer(message);
}

/*
 * sysmkdialog
 *
 * Display a *nix dialog box
 */
dialogResult_t
sysmkdialog(dialogType_t type, const char *message, const char *title)
{
	typedef enum {
		NONE = 0,
		ZENITY,
		KDIALOG,
		XMESSAGE,
		NUM_DIALOG_PROGRAMS
	} dialogCommandType_t;
	typedef void (*dialogCommandBuilder_t)(dialogType_t, const char *,
					       const char *);

	const char	*session = getenv("DESKTOP_SESSION");
	qbool		tried[ NUM_DIALOG_PROGRAMS ] = { qfalse };
	dialogCommandBuilder_t	commands[ NUM_DIALOG_PROGRAMS ] = { NULL };
	dialogCommandType_t	preferredCommandType = NONE;

	commands[ ZENITY ] = &Sys_ZenityCommand;
	commands[ KDIALOG ] = &Sys_KdialogCommand;
	commands[ XMESSAGE ] = &Sys_XmessageCommand;

	/* This may not be the best way */
	if(!Q_stricmp(session, "gnome"))
		preferredCommandType = ZENITY;
	else if(!Q_stricmp(session, "kde"))
		preferredCommandType = KDIALOG;

	while(1){
		int i;

		for(i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++){
			if(preferredCommandType != NONE &&
			   preferredCommandType != i)
				continue;

			if(!tried[ i ]){
				int exitCode;

				commands[ i ](type, message, title);
				exitCode = Sys_Exec( );

				if(exitCode >= 0){
					switch(type){
					case DT_YES_NO:    return exitCode ?
						       DR_NO : DR_YES;
					case DT_OK_CANCEL: return exitCode ?
						       DR_CANCEL : DR_OK;
					default:           return DR_OK;
					}
				}

				tried[ i ] = qtrue;

				/* The preference failed, so start again in order */
				if(preferredCommandType != NONE){
					preferredCommandType = NONE;
					break;
				}
			}
		}

		for(i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++)
			if(!tried[ i ])
				continue;

		break;
	}

	comdprintf(S_COLOR_YELLOW "WARNING: failed to show a dialog\n");
	return DR_OK;
}
#endif

/*
 * Sys_GLimpSafeInit
 *
 * Unix specific "safe" GL implementation initialisation
 */
void
Sys_GLimpSafeInit(void)
{
	/* NOP */
}

/*
 * Sys_GLimpInit
 *
 * Unix specific GL implementation initialisation
 */
void
Sys_GLimpInit(void)
{
	/* NOP */
}

void
Sys_SetFloatEnv(void)
{
	/* rounding toward nearest */
	fesetround(FE_TONEAREST);
}

/*
 * Sys_PlatformInit
 *
 * Unix specific initialisation
 */
void
Sys_PlatformInit(void)
{
	const char * term = getenv("TERM");

	signal(SIGHUP, Sys_SigHandler);
	signal(SIGQUIT, Sys_SigHandler);
	signal(SIGTRAP, Sys_SigHandler);
	signal(SIGIOT, Sys_SigHandler);
	signal(SIGBUS, Sys_SigHandler);

	stdinIsATTY = isatty(STDIN_FILENO) &&
		      !(term && (!strcmp(term, "raw") || !strcmp(term, "dumb")));
}

/*
 * Sys_PlatformExit
 *
 * Unix specific deinitialisation
 */
void
Sys_PlatformExit(void)
{
}

/*
 * syssetenv
 *
 * set/unset environment variables (empty value removes it)
 */

void
syssetenv(const char *name, const char *value)
{
	if(value && *value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}

/*
 * Sys_PID
 */
int
Sys_PID(void)
{
	return getpid( );
}

/*
 * Sys_PIDIsRunning
 */
qbool
Sys_PIDIsRunning(int pid)
{
	return kill(pid, 0) == 0;
}
