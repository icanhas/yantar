/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 /* 
  * Misc. functions used in client and server 
  */

#include "shared.h"
#include "common.h"
#include <setjmp.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/stat.h>	/* umask */
#else
#include <winsock.h>
#endif

int demo_protocols[] = { 67, 66, 0 };

enum {
	MAX_NUM_ARGVS = 50
};

static jmp_buf abortframe;	/* an ERR_DROP occured, exit the entire frame */

FILE *debuglogfile;
static Fhandle	pipefile;
Fhandle	logfile;
Fhandle	com_journalFile;	/* events are written here */
Fhandle	com_journalDataFile;	/* config files are written here */

Cvar	*com_speeds;
Cvar	*com_developer;
Cvar	*com_dedicated;
Cvar	*com_timescale;
Cvar	*com_fixedtime;
Cvar	*com_journal;
Cvar	*com_maxfps;
Cvar	*com_altivec;
Cvar	*com_timedemo;
Cvar	*com_sv_running;
Cvar	*com_cl_running;
Cvar	*com_logfile;	/* 1 = buffer log, 2 = flush after each print */
Cvar	*com_pipefile;
Cvar	*com_showtrace;
Cvar	*com_version;
Cvar	*com_blood;
Cvar	*com_buildScript;	/* for automated data building scripts */
Cvar	*com_introPlayed;
Cvar	*cl_paused;
Cvar	*sv_paused;
Cvar	*cl_packetdelay;
Cvar	*sv_packetdelay;
Cvar	*com_cameraMode;
Cvar	*com_ansiColor;
Cvar	*com_unfocused;
Cvar	*com_maxfpsUnfocused;
Cvar	*com_minimized;
Cvar	*com_maxfpsMinimized;
Cvar	*com_abnormalExit;
Cvar	*com_standalone;
Cvar	*com_gamename;
Cvar	*com_protocol;
Cvar	*com_basegame;
Cvar	*com_homepath;
Cvar	*com_busyWait;

#if idx64
int	(*Q_VMftol)(void);
#elif id386
long	(QDECL *Q_ftol)(float f);
int	(QDECL *Q_VMftol)(void);
void	(QDECL *Q_snapv3)(Vec3 vec);
#endif

/* com_speeds times */
int	time_game;
int	time_frontend;	/* renderer frontend time */
int	time_backend;	/* renderer backend time */

int	com_frameTime;
int	com_frameNumber;

qbool		com_errorEntered = qfalse;
qbool		com_fullyInitialized = qfalse;
qbool		com_gameRestarting = qfalse;

char com_errorMessage[MAXPRINTMSG];

extern void	Com_Inithunk(void);
extern void	Com_Initsmallzone(void);
extern void	Com_Initzone(void);
extern void	CIN_CloseAllVideos(void);

static char	*rd_buffer;
static uint	rd_buffersize;
static void	(*rd_flush)(char *buffer);

void
Com_Beginredirect(char *buffer, int buffersize, void (*flush)(char *))
{
	if(!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void
Com_Endredirect(void)
{
	if(rd_flush)
		rd_flush(rd_buffer);
	rd_buffer = nil;
	rd_buffersize = 0;
	rd_flush = nil;
}

/*
 * Both client and server can use this, and it will output
 * to the apropriate place.
 * 
 * May be passed UTF-8.
 *
 * A string variable should NEVER be passed as fmt, because of "%f" type crashers.
 */
void QDECL
Com_Printf(const char *fmt, ...)
{
	va_list argptr;
	char	msg[MAXPRINTMSG];
	static qbool opening_qconsole = qfalse;

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);
	
	if(rd_buffer){
		if((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1)){
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		Q_strcat(rd_buffer, rd_buffersize, msg);
		/* TTimo nooo .. that would defeat the purpose
		 * rd_flush(rd_buffer);
		 * *rd_buffer = 0; */
		return;
	}

#ifndef DEDICATED
	CL_ConsolePrint(msg);
#endif
	Sys_Print(msg); /* echo to dedicated console and early console */
	
	/* logfile */
	if(com_logfile && com_logfile->integer){
		/* TTimo: only open the qconsole.log if the filesystem is in an initialized state
		*   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on) */
		if(!logfile && FS_Initialized() && !opening_qconsole){
			struct tm *newtime;
			time_t aclock;

			opening_qconsole = qtrue;
			time(&aclock);
			newtime = localtime(&aclock);
			logfile = FS_FOpenFileWrite("qconsole.log");
			if(logfile){
				Com_Printf("logfile opened on %s\n",
					asctime(newtime));

				if(com_logfile->integer > 1)
					/* force it to not buffer so we get valid
					 * data even if we are crashing */
					FS_ForceFlush(logfile);
			}else{
				Com_Printf("Opening qconsole.log failed!\n");
				Cvar_SetValue("logfile", 0);
			}
			opening_qconsole = qfalse;
		}
		if(logfile && FS_Initialized())
			FS_Write(msg, strlen(msg), logfile);
	}
}

/*
 * A Com_Printf that only shows up if the "developer" cvar is set
 */
void QDECL
Com_DPrintf(const char *fmt, ...)
{
	va_list argptr;
	char	msg[MAXPRINTMSG];

	if(!com_developer || !com_developer->integer)
		return;		/* don't confuse non-developers with techie stuff... */

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Printf ("%s", msg);
}

/*
 * Both client and server can use this, and it will
 * do the appropriate thing.
 */
void QDECL
Com_Errorf(int code, const char *fmt, ...)
{
	va_list argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int currentTime;

	if(com_errorEntered)
		Sys_Error("recursive error after: %s", com_errorMessage);

	com_errorEntered = qtrue;

	Cvar_Set("com_errorCode", va("%i", code));

	/* when we are running automated scripts, make sure we
	 * know if anything failed */
	if(com_buildScript && com_buildScript->integer)
		code = ERR_FATAL;

	/* if we are getting a solid stream of ERR_DROP, do an ERR_FATAL */
	currentTime = Sys_Milliseconds();
	if(currentTime - lastErrorTime < 100){
		if(++errorCount > 3)
			code = ERR_FATAL;
	}else
		errorCount = 0;
	lastErrorTime = currentTime;

	va_start (argptr,fmt);
	Q_vsnprintf (com_errorMessage, sizeof(com_errorMessage),fmt,argptr);
	va_end (argptr);

	if(code != ERR_DISCONNECT)
		Cvar_Set("com_errorMessage", com_errorMessage);

	if(code == ERR_DISCONNECT || code == ERR_SERVERDISCONNECT){
		VM_Forced_Unload_Start();
		SV_Shutdown("Server disconnected");
		CL_Disconnect(qtrue);
		CL_FlushMemory( );
		VM_Forced_Unload_Done();
		/* make sure we can get at our local stuff */
		FS_PureServerSetLoadedPaks("", "");
		com_errorEntered = qfalse;
		longjmp (abortframe, -1);
	}else if(code == ERR_DROP){
		Com_Printf (
			"********************\nERROR: %s\n********************\n",
			com_errorMessage);
		VM_Forced_Unload_Start();
		SV_Shutdown (va("Server crashed: %s",  com_errorMessage));
		CL_Disconnect(qtrue);
		CL_FlushMemory( );
		VM_Forced_Unload_Done();
		FS_PureServerSetLoadedPaks("", "");
		com_errorEntered = qfalse;
		longjmp(abortframe, -1);
	}else{
		VM_Forced_Unload_Start();
		CL_Shutdown(va("Client fatal crashed: %s",
				com_errorMessage), qtrue, qtrue);
		SV_Shutdown(va("Server fatal crashed: %s", com_errorMessage));
		VM_Forced_Unload_Done();
	}

	Com_Shutdown ();

	Sys_Error ("%s", com_errorMessage);
}

/*
 * Both client and server can use this, and it will
 * do the apropriate things.
 */
void
Com_Quit_f(void)
{
	/* don't try to shutdown if we are in a recursive error */
	char *p = Cmd_Args( );
	if(!com_errorEntered){
		/* Some VMs might execute "quit" command directly,
		 * which would trigger an unload of active VM error.
		 * Sys_Quit will kill this process anyways, so
		 * a corrupt call stack makes no difference */
		VM_Forced_Unload_Start();
		SV_Shutdown(p[0] ? p : "Server quit");
		CL_Shutdown(p[0] ? p : "Client quit", qtrue, qtrue);
		VM_Forced_Unload_Done();
		Com_Shutdown ();
		FS_Shutdown(qtrue);
	}
	Sys_Quit ();
}

/*
 * Command-line functions
 *
 * '+' characters separate the commandLine string into multiple console
 * command lines.
 *
 * All of these are valid:
 * yantar +set test blah +map test
 * yantar set test blah+map test
 * yantar set test blah + map test
 */

#define MAX_CONSOLE_LINES 32
int com_numConsoleLines;
char *com_consoleLines[MAX_CONSOLE_LINES];

/*
 * Break it up into multiple console lines
 */
void
Com_Parsecmdline(char *commandLine)
{
	int inq = 0;
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while(*commandLine){
		if(*commandLine == '"')
			inq = !inq;
		/* look for a + seperating character
		 * if commandLine came from a file, we might have real line seperators */
		if((*commandLine == '+' &&
		    !inq) || *commandLine == '\n'  || *commandLine == '\r'){
			if(com_numConsoleLines == MAX_CONSOLE_LINES)
				return;
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}

/*
 * Check for "safe" on the command line, which will
 * skip loading of user.cfg
 */
qbool
Com_Insafemode(void)
{
	int i;

	for(i = 0; i < com_numConsoleLines; i++){
		Cmd_TokenizeString(com_consoleLines[i]);
		if(!Q_stricmp(Cmd_Argv(0), "safe")
		   || !Q_stricmp(Cmd_Argv(0), "cvar_restart")){
			com_consoleLines[i][0] = 0;
			return qtrue;
		}
	}
	return qfalse;
}

/*
 * Searches for command line parameters that are set commands.
 * If match is not nil, only that cvar will be looked for.
 * That is necessary because cddir and basedir need to be set
 * before the filesystem is started, but all other sets should
 * be after execing the config and default.
 */
void
Com_Startupvar(const char *match)
{
	int i;
	char *s;

	for(i=0; i < com_numConsoleLines; i++){
		Cmd_TokenizeString(com_consoleLines[i]);
		if(strcmp(Cmd_Argv(0), "set"))
			continue;

		s = Cmd_Argv(1);

		if(!match || !strcmp(s, match)){
			if(Cvar_Flags(s) == CVAR_NONEXISTENT)
				Cvar_Get(s, Cmd_Argv(2), CVAR_USER_CREATED);
			else
				Cvar_Set2(s, Cmd_Argv(2), qfalse);
		}
	}
}

/*
 * Adds command line parameters as script statements
 * Commands are seperated by + signs
 *
 * Returns qtrue if any late commands were added, which
 * will keep the demoloop from immediately starting
 */
qbool
Com_Addstartupcmds(void)
{
	int i;
	qbool added;

	added = qfalse;
	/* quote every token, so args with semicolons can work */
	for(i=0; i < com_numConsoleLines; i++){
		if(!com_consoleLines[i] || !com_consoleLines[i][0])
			continue;
		/* set commands already added with Com_Startupvar */
		if(!Q_stricmpn(com_consoleLines[i], "set", 3))
			continue;
		added = qtrue;
		Cbuf_AddText(com_consoleLines[i]);
		Cbuf_AddText("\n");
	}
	return added;
}

void
Info_Print(const char *s)
{
	char key[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char *o;
	int l;

	if(*s == '\\')
		s++;
	while(*s){
		o = key;
		while(*s && *s != '\\')
			*o++ = *s++;
		l = o - key;
		if(l < 20){
			Q_Memset(o, ' ', 20-l);
			key[20] = 0;
		}else
			*o = 0;
		Com_Printf ("%s ", key);

		if(!*s){
			Com_Printf("MISSING VALUE\n");
			return;
		}
		o = value;
		s++;
		while(*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if(*s)
			s++;
		Com_Printf("%s\n", value);
	}
}

char *
Q_StringContains(char *str1, char *str2, int casesensitive)
{
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for(i = 0; i <= len; i++, str1++){
		for(j = 0; str2[j]; j++){
			if(casesensitive){
				if(str1[j] != str2[j])
					break;
			}else if(toupper(str1[j]) != toupper(str2[j]))
				break;
		}
		if(!str2[j])
			return str1;
	}
	return nil;
}

int
Q_Filter(char *filter, char *name, int casesensitive)
{
	char buf[MAX_TOKEN_CHARS];
	char *ptr;
	int i, found;

	while(*filter){
		if(*filter == '*'){
			filter++;
			for(i = 0; *filter; i++){
				if(*filter == '*' || *filter == '?') break;
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if(strlen(buf)){
				ptr = Q_StringContains(name, buf,
					casesensitive);
				if(!ptr) return qfalse;
				name = ptr + strlen(buf);
			}
		}else if(*filter == '?'){
			filter++;
			name++;
		}else if(*filter == '[' && *(filter+1) == '[')
			filter++;
		else if(*filter == '['){
			filter++;
			found = qfalse;
			while(*filter && !found){
				if(*filter == ']' && *(filter+1) != ']') break;
				if(*(filter+1) == '-' && *(filter+2) &&
				   (*(filter+2) != ']' || *(filter+3) == ']'))
				then{
					if(casesensitive){
						if(*name >= *filter && *name <= *(filter+2)) 
							found = qtrue;
					}else if(toupper(*name) >= toupper(*filter) &&
					   toupper(*name) <= toupper(*(filter+2))) 
					then{
						found = qtrue;
					}
					filter += 3;
				}else{
					if(casesensitive){
						if(*filter == *name) 
							found = qtrue;
					}else if(toupper(*filter) == toupper(*name)) 
						found = qtrue;
					filter++;
				}
			}
			if(!found) 
				return qfalse;
			while(*filter){
				if(*filter == ']' && *(filter+1) != ']') 
					break;
				filter++;
			}
			filter++;
			name++;
		}else{
			if(casesensitive){
				if(*filter != *name) 
					return qfalse;
			}else if(toupper(*filter) != toupper(*name))
				return qfalse;
			filter++;
			name++;
		}
	}
	return qtrue;
}

int
Q_FilterPath(char *filter, char *name, int casesensitive)
{
	int	i;
	char	new_filter[MAX_QPATH];
	char	new_name[MAX_QPATH];

	for(i = 0; i < MAX_QPATH-1 && filter[i]; i++){
		if(filter[i] == '\\' || filter[i] == ':')
			new_filter[i] = '/';
		else
			new_filter[i] = filter[i];
	}
	new_filter[i] = '\0';
	for(i = 0; i < MAX_QPATH-1 && name[i]; i++){
		if(name[i] == '\\' || name[i] == ':')
			new_name[i] = '/';
		else
			new_name[i] = name[i];
	}
	new_name[i] = '\0';
	return Q_Filter(new_filter, new_name, casesensitive);
}

int
Com_RealTime(Qtime *qtime)
{
	time_t t;
	struct tm *tms;

	t = time(nil);
	if(!qtime)
		return t;
	tms = localtime(&t);
	if(tms){
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}

/*
 * Events and journaling
 *
 * In addition to these events, .cfg files are also copied to the
 * journaled file
 */

enum { MAX_PUSHED_EVENTS = 1024 };
static int	com_pushedEventsHead = 0;
static int	com_pushedEventsTail = 0;
static Sysevent com_pushedEvents[MAX_PUSHED_EVENTS];

void
Com_Initjournaling(void)
{
	Com_Startupvar("journal");
	com_journal = Cvar_Get ("journal", "0", CVAR_INIT);
	if(!com_journal->integer)
		return;

	if(com_journal->integer == 1){
		Com_Printf("Journaling events\n");
		com_journalFile = FS_FOpenFileWrite("journal.dat");
		com_journalDataFile = FS_FOpenFileWrite("journaldata.dat");
	}else if(com_journal->integer == 2){
		Com_Printf("Replaying journaled events\n");
		FS_FOpenFileRead("journal.dat", &com_journalFile, qtrue);
		FS_FOpenFileRead("journaldata.dat", &com_journalDataFile, qtrue);
	}

	if(!com_journalFile || !com_journalDataFile){
		Cvar_Set("com_journal", "0");
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf("Couldn't open journal files\n");
	}
}

/*
 * Event loop
 */

#define MAX_QUEUED_EVENTS	256
#define MASK_QUEUED_EVENTS	(MAX_QUEUED_EVENTS - 1)

static Sysevent eventQueue[ MAX_QUEUED_EVENTS ];
static int	eventHead = 0;
static int	eventTail = 0;

/*
 * A time of 0 will get the current time
 * Ptr should either be null, or point to a block of data that can
 * be freed by the game later.
 */
void
Com_Queueevent(int time, Syseventtype type, int value, int value2,
	       int ptrLength,
	       void *ptr)
{
	Sysevent *ev;

	ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];

	if(eventHead - eventTail >= MAX_QUEUED_EVENTS){
		Com_Printf("Com_Queueevent: overflow\n");
		/* we are discarding an event, but don't leak memory */
		if(ev->evPtr)
			Z_Free(ev->evPtr);
		eventTail++;
	}

	eventHead++;

	if(time == 0)
		time = Sys_Milliseconds();

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

Sysevent
Com_Getsysevent(void)
{
	Sysevent ev;
	char *s;

	/* return if we have data */
	if(eventHead > eventTail){
		eventTail++;
		return eventQueue[ (eventTail - 1) & MASK_QUEUED_EVENTS ];
	}

	/* check for console commands */
	s = Sys_ConsoleInput();
	if(s){
		char	*b;
		int	len;

		len = strlen(s) + 1;
		b = Z_Malloc(len);
		strcpy(b, s);
		Com_Queueevent(0, SE_CONSOLE, 0, 0, len, b);
	}

	/* return if we have data */
	if(eventHead > eventTail){
		eventTail++;
		return eventQueue[ (eventTail - 1) & MASK_QUEUED_EVENTS ];
	}

	/* create an empty event to return */
	memset(&ev, 0, sizeof(ev));
	ev.evTime = Sys_Milliseconds();

	return ev;
}

Sysevent
Com_Getrealevent(void)
{
	int r;
	Sysevent ev;

	/* either get an event from the system or the journal file */
	if(com_journal->integer == 2){
		r = FS_Read(&ev, sizeof(ev), com_journalFile);
		if(r != sizeof(ev))
			Com_Errorf(ERR_FATAL, "Error reading from journal file");
		if(ev.evPtrLength){
			ev.evPtr = Z_Malloc(ev.evPtrLength);
			r = FS_Read(ev.evPtr, ev.evPtrLength, com_journalFile);
			if(r != ev.evPtrLength)
				Com_Errorf(ERR_FATAL,
					"Error reading from journal file");
		}
	}else{
		ev = Com_Getsysevent();

		/* write the journal value out if needed */
		if(com_journal->integer == 1){
			r = FS_Write(&ev, sizeof(ev), com_journalFile);
			if(r != sizeof(ev))
				Com_Errorf(ERR_FATAL,
					"Error writing to journal file");
			if(ev.evPtrLength){
				r = FS_Write(ev.evPtr, ev.evPtrLength,
					com_journalFile);
				if(r != ev.evPtrLength)
					Com_Errorf(
						ERR_FATAL,
						"Error writing to journal file");
			}
		}
	}

	return ev;
}

void
Com_Initpushevent(void)
{
	/* clear the static buffer array
	 * this requires SE_NONE to be accepted as a valid but NOP event */
	memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
	/* reset counters while we are at it
	 * beware: GetEvent might still return an SE_NONE from the buffer */
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}

void
Com_Pushevent(Sysevent *event)
{
	Sysevent	*ev;
	static int	printedWarning = 0;

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if(com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS){

		/* don't print the warning constantly, or it can give time for more... */
		if(!printedWarning){
			printedWarning = qtrue;
			Com_Printf("WARNING: Com_Pushevent overflow\n");
		}

		if(ev->evPtr)
			Z_Free(ev->evPtr);
		com_pushedEventsTail++;
	}else
		printedWarning = qfalse;

	*ev = *event;
	com_pushedEventsHead++;
}

Sysevent
Q_GetEvent(void)
{
	if(com_pushedEventsHead > com_pushedEventsTail){
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-
					  1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return Com_Getrealevent();
}

void
Com_Runserverpacket(Netaddr *evFrom, Bitmsg *buf)
{
	int t1, t2, msec;

	t1 = 0;
	if(com_speeds->integer)
		t1 = Sys_Milliseconds ();
	SV_PacketEvent(*evFrom, buf);
	if(com_speeds->integer){
		t2 = Sys_Milliseconds ();
		msec = t2 - t1;
		if(com_speeds->integer == 3)
			Com_Printf("SV_PacketEvent time: %i\n", msec);
	}
}

/* Returns last event time */
int
Com_Eventloop(void)
{
	Sysevent	ev;
	Netaddr	evFrom;
	byte	bufData[MAX_MSGLEN];
	Bitmsg	buf;

	MSG_Init(&buf, bufData, sizeof(bufData));
	while(1){
		ev = Q_GetEvent();
		/* if no more events are available */
		if(ev.evType == SE_NONE){
			/* manually send packet events for the loopback channel */
			while(NET_GetLoopPacket(NS_CLIENT, &evFrom, &buf))
				CL_PacketEvent(evFrom, &buf);
			while(NET_GetLoopPacket(NS_SERVER, &evFrom, &buf))
				/* if the server just shut down, flush the events */
				if(com_sv_running->integer)
					Com_Runserverpacket(&evFrom, &buf);
			return ev.evTime;
		}

		switch(ev.evType){
		case SE_KEY:
			CL_KeyEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CHAR:
			CL_CharEvent(ev.evValue);
			break;
		case SE_MOUSE:
			CL_MouseEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CONSOLE:
			Cbuf_AddText((char*)ev.evPtr);
			Cbuf_AddText("\n");
			break;
		default:
			Com_Errorf(ERR_FATAL, "Com_Eventloop: bad event type %i",
				ev.evType);
			break;
		}
		/* free any block data */
		if(ev.evPtr)
			Z_Free(ev.evPtr);
	}

	return 0;	/* never reached */
}

/* Can be used for profiling, but will be journaled accurately */
int
Com_Millisecs(void)
{
	Sysevent ev;

	/* get events and push them until we get a null event with the current time */
	do{
		ev = Com_Getrealevent();
		if(ev.evType != SE_NONE)
			Com_Pushevent(&ev);
	}while(ev.evType != SE_NONE);
	return ev.evTime;
}

/* ============================================================================ */

/* Just throw a fatal error to test error shutdown procedures */
static void
Com_Errorf_f(void)
{
	if(Cmd_Argc() > 1)
		Com_Errorf(ERR_DROP, "Testing drop error");
	else
		Com_Errorf(ERR_FATAL, "Testing fatal error");
}

/*
 * Just freeze in place for a given number of seconds to test
 * error recovery
 */
static void
Com_Freeze_f(void)
{
	float	s;
	int	start, now;

	if(Cmd_Argc() != 2){
		Com_Printf("freeze <seconds>\n");
		return;
	}
	s = atof(Cmd_Argv(1));
	start = Com_Millisecs();
	for(;;){
		now = Com_Millisecs();
		if((now - start) * 0.001 > s)
			break;
	}
}

/* A way to force a bus error for development reasons */
static void
Com_Crash_f(void)
{
	*(volatile int*)0x0000dead = 0xdeaddead;
}

/* just for Com_Testutf8_f */
static void
runeprops(Rune r)
{
	Com_Printf("rune %#x properties:", r);
	if(Q_isalpharune(r)) Com_Printf("isalpha ");
	if(Q_isdigitrune(r)) Com_Printf("isdigit ");
	if(Q_isupperrune(r)) Com_Printf("isupper ");
	if(Q_islowerrune(r)) Com_Printf("islower ");
	if(Q_istitlerune(r)) Com_Printf("istitle ");
	if(Q_isspacerune(r)) Com_Printf("isspace");
	Com_Printf("\n");
}

/*
 * crap tests
 */

static void
Com_Testutf8_f(void)
{
	char str[50] = "test тест δοκιμή próf 시험 テスト";
	Rune r;
	Rune rstr[29];
	int n;
	uint i;
	
	n = Q_chartorune(&r, &str[5]);
	Com_Printf("read a utf sequence %i bytes long: %#x\n", n, r);
	for(i = 0; i < 6; i++){
		if(Q_fullrune(&str[5], i)){
			Com_Printf("letter appears to be a full utf sequence at %u bytes long\n", i);
			break;
		}
		Com_Printf("letter doesn't appear to be a proper sequence at %u bytes long\n", i);
	}
	Com_Printf("%s\n", str);
	rstr[0] = r;
	rstr[1] = 'a'; rstr[2] = 's'; rstr[3] = 'd'; rstr[4] = 'f'; rstr[5] = r;
	Com_Printf("rstr has runestrlen %lu\n", (ulong)Q_runestrlen(rstr));
	Com_Printf("str has utflen %lu\n", (ulong)Q_utflen(str));
	runeprops(r);
	runeprops(Q_toupperrune(r));
	runeprops(Q_tolowerrune(r));
	runeprops(Q_totitlerune(r));
	r = '4';
	runeprops(r);
	runeprops(Q_toupperrune(r));
	runeprops(Q_tolowerrune(r));
	runeprops(Q_totitlerune(r));
	r = 0x01C5;	/* Titlecase 'Dz' */
	runeprops(r);
	runeprops(Q_toupperrune(r));
	runeprops(Q_tolowerrune(r));
	runeprops(Q_totitlerune(r));
}

static void
printmatrix(Scalar *m, int w, int h)
{
	int i, j;

	for(i = 0; i < h; ++i){
		for(j = 0; j < w; ++j)
			Com_Printf(" %-7.2f", m[i*w + j]);
		Com_Printf("\n");
	}
	Com_Printf("\n");
}

static void
printquat(Quat q)
{
	Com_Printf("% .4g%+.4gi%+.4gj%+.4gk\n", q[0], q[1], q[2], q[3]);
}

static void
matrixtest(void)
{
	Mat2 a2, b2, c2 = {
		1, 2, 
		3, 4
	};
	Mat3 a3, b3, c3 = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9
	};
	Mat4 a4, b4, c4 = {
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16
	};
	
	Com_Printf("matrixtest:\n\n");
	#define test(stmt, var)	(stmt); \
		Com_Printf("%s =\n", XSTRING(stmt)); \
		printmatrix((var), 2, 2)
	test(clearm2(a2), a2);
	test(identm2(a2), a2);
	test(copym2(a2, b2), b2);
	test(transposem2(a2, a2), a2);
	test(scalem2(a2, 1234.0f, a2), a2);
	/* M * id */
	identm2(b2);
	test(mulm2(c2, b2, a2), a2);
	/* M * MT */
	test(transposem2(c2, a2), a2);
	test(mulm2(c2, a2, b2), b2);
	clearm2(a2);
	Com_Printf("c2 %s c2; ", (cmpm2(c2, c2)) ? "==" : "!=");
	Com_Printf("c2 %s a2\n\n", (cmpm2(c2, a2)) ? "==" : "!=");
	#undef test

	#define test(stmt, var)	(stmt); \
		Com_Printf("%s =\n", XSTRING(stmt)); \
		printmatrix((var), 3, 3)
	test(clearm3(a3), a3);
	test(identm3(a3), a3);
	test(copym3(a3, b3), b3);
	test(transposem3(a3, a3), a3);
	test(scalem3(a3, 77.0f, a3), a3);
	/* M * id */
	identm3(b3);
	test(mulm3(c3, b3, a3), a3);
	/* M * MT */
	test(transposem3(c3, a3), a3);
	test(mulm3(c3, a3, b3), b3);
	#undef test

	#define test(stmt, var)	(stmt); \
		Com_Printf("%s =\n", XSTRING(stmt)); \
		printmatrix((var), 4, 4)
	test(clearm4(a4), a4);
	test(identm4(a4), a4);
	test(copym4(a4, b4), b4);
	test(transposem4(a4, a4), a4);
	test(scalem4(a4, 77.0f, a4), a4);
	/* M * id */
	identm4(b4);
	test(mulm4(c4, b4, a4), a4);
	/* M * MT */
	test(transposem4(c4, a4), a4);
	test(mulm4(c4, a4, b4), b4);
	#undef test
}

static void
catternyantest(void)
{
	Quat a = { 1, 2, 3, 4 }, b, c = { 7, 8, 9, 10 }, angle;
	Vec3 eangle = { 23, -90, 353 };
	Scalar m;
	
	Com_Printf("catternyantest:\n\n");
	#define test(stmt, var)	(stmt); \
		Com_Printf("%-30s = ", XSTRING(stmt)); \
		printquat((var))
	test(conjq(a, b), b);
	test(invq(a, b), b);
	test(mulq(a, b, b), b);
	test(mulq(a, c, b), b);
	test(eulertoq(eangle, angle), angle);
	test(invq(angle, b), b);
	test(invq(b, b), b);
	#undef test
	
	#define test(stmt, var)	(stmt); \
		Com_Printf("%-30s = ", XSTRING(stmt)); \
		Com_Printf("%f\n", (var))
	test(m = magq(a), m);
	test(m = magq(angle), m);	/* norm of an angle should always be 1.0 */
	#undef test
}

static void
Com_Testmaths_f(void)
{
	matrixtest();
	catternyantest();
}
	
/* For controlling environment variables */
void
Com_Setenv_f(void)
{
	int argc = Cmd_Argc();
	char *arg1 = Cmd_Argv(1);

	if(argc > 2){
		char *arg2 = Cmd_ArgsFrom(2);

		Sys_SetEnv(arg1, arg2);
	}else if(argc == 2){
		char *env = getenv(arg1);

		if(env)
			Com_Printf("%s=%s\n", arg1, env);
		else
			Com_Printf("%s undefined\n", arg1);
	}
}

/* For controlling environment variables */
void
Com_Execconfig(void)
{
	Cbuf_ExecuteText(EXEC_NOW, "exec default.cfg\n");
	Cbuf_Execute();	/* Always execute after exec to prevent text buffer overflowing */

	if(!Com_Insafemode()){
		/* skip the user.cfg if "safe" is on the command line */
		Cbuf_ExecuteText(EXEC_NOW, "exec " Q3CONFIG_CFG "\n");
		Cbuf_Execute();
	}
}

/* Change to a new mod properly with cleaning up cvars before switching. */
void
Com_Gamerestart(int checksumFeed, qbool disconnect)
{
	/* make sure no recursion can be triggered */
	if(!com_gameRestarting && com_fullyInitialized){
		int clWasRunning;

		com_gameRestarting = qtrue;
		clWasRunning = com_cl_running->integer;

		/* Kill server if we have one */
		if(com_sv_running->integer)
			SV_Shutdown("Game directory changed");

		if(clWasRunning){
			if(disconnect)
				CL_Disconnect(qfalse);

			CL_Shutdown("Game directory changed", disconnect, qfalse);
		}

		FS_Restart(checksumFeed);

		/* Clean out any user and VM created cvars */
		Cvar_Restart(qtrue);
		Com_Execconfig();

		if(disconnect){
			/* 
			 * We don't want to change any network settings if gamedir
			 * change was triggered by a connect to server because the
			 * new network settings might make the connection fail. 
			 */
			NET_Restart_f();
		}

		if(clWasRunning){
			CL_Init();
			CL_StartHunkUsers(qfalse);
		}

		com_gameRestarting = qfalse;
	}
}

/* Expose possibility to change current running mod to the user */
void
Com_Gamerestart_f(void)
{
	if(!FS_FilenameCompare(Cmd_Argv(1), com_basegame->string))
		/* This is the standard base game. Servers and clients should
		 * use "" and not the standard basegame name because this messes
		 * up pak file negotiation and lots of other stuff */
		Cvar_Set("fs_game", "");
	else
		Cvar_Set("fs_game", Cmd_Argv(1));

	Com_Gamerestart(0, qtrue);
}

void
Com_Writeconfigtofile(const char *filename)
{
	Fhandle f;

	f = FS_FOpenFileWrite(filename);
	if(!f){
		Com_Printf ("Couldn't write %s.\n", filename);
		return;
	}
	FS_Printf(f, "// ／人◕‿‿ ◕人＼\n");
	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	FS_FCloseFile(f);
}

/* Writes key bindings and archived cvars to config file if modified */
void
Com_Writeconfig(void)
{
	/* if we are quitting without fully initializing, make sure
	 * we don't write out anything */
	if(!com_fullyInitialized)
		return;

	if(!(cvar_modifiedFlags & CVAR_ARCHIVE))
		return;
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	Com_Writeconfigtofile(Q3CONFIG_CFG);
}

/* Write the config file to a specific name */
void
Com_Writeconfig_f(void)
{
	char filename[MAX_QPATH];

	if(Cmd_Argc() != 2){
		Com_Printf("Usage: writeconfig <filename>\n");
		return;
	}
	Q_strncpyz(filename, Cmd_Argv(1), sizeof(filename));
	Q_defaultext(filename, sizeof(filename), ".cfg");
	Com_Printf("Writing %s.\n", filename);
	Com_Writeconfigtofile(filename);
}

int
Com_Modifymsec(int msec)
{
	int clampTime;

	/* modify time for debugging values */
	if(com_fixedtime->integer)
		msec = com_fixedtime->integer;
	else if(com_timescale->value)
		msec *= com_timescale->value;
	else if(com_cameraMode->integer)
		msec *= com_timescale->value;

	/* don't let it scale below 1 msec */
	if(msec < 1 && com_timescale->value)
		msec = 1;

	if(com_dedicated->integer){
		/* dedicated servers don't want to clamp for a much longer
		 * period, because it would mess up all the client's views
		 * of time. */
		if(com_sv_running->integer && msec > 500)
			Com_Printf("Hitch warning: %i msec frame time\n", msec);

		clampTime = 5000;
	}else if(!com_sv_running->integer)
		/* clients of remote servers do not want to clamp time, because
		 * it would skew their view of the server's time temporarily */
		clampTime = 5000;
	else
		/* for local single player gaming
		 * we may want to clamp the time to prevent players from
		 * flying off edges when something hitches. */
		clampTime = 200;

	if(msec > clampTime)
		msec = clampTime;

	return msec;
}

int
Q_TimeVal(int minMsec)
{
	int timeVal;

	timeVal = Sys_Milliseconds() - com_frameTime;

	if(timeVal >= minMsec)
		timeVal = 0;
	else
		timeVal = minMsec - timeVal;
	return timeVal;
}

static void
Com_Detectaltivec(void)
{
	/* Only detect if user hasn't forcibly disabled it. */
	if(com_altivec->integer){
		static qbool altivec = qfalse;
		static qbool detected = qfalse;
		if(!detected){
			altivec = (Sys_GetProcessorFeatures( ) & CF_ALTIVEC);
			detected = qtrue;
		}
		if(!altivec)
			Cvar_Set("com_altivec", "0");	/* we don't have it! Disable support! */
	}
}

#if id386 || idx64
/* Find out whether we have SSE support for Q_ftol function */
static void
Com_Detectsse(void)
{
#if !idx64
	CPUfeatures feat;

	feat = Sys_GetProcessorFeatures();
	if(feat & CF_SSE){
		if(feat & CF_SSE2)
			Q_snapv3 = qsnapv3sse;
		else
			Q_snapv3 = qsnapv3x87;
		Q_ftol = qftolsse;
#endif
		Q_VMftol = qvmftolsse;
		Com_Printf("Have SSE support\n");
#if !idx64
	}else{
		Q_ftol = qftolx87;
		Q_VMftol = qvmftolx87;
		Q_snapv3 = qsnapv3x87;
		Com_Printf("No SSE support on this machine\n");
	}
#endif
}

#else

static void
Com_Detectsse(void)
{
	;
}
#endif

/* Seed the random number generator, if possible with an OS supplied random seed. */
static void
Com_Initrand(void)
{
	unsigned int seed;

	if(Sys_RandomBytes((byte*)&seed, sizeof(seed)))
		srand(seed);
	else
		srand(time(nil));
}

/* commandLine should not include the executable name (argv[0]) */
void
Com_Init(char *commandLine)
{
	char	*s;
	int	qport;

	Com_Printf("%s %s %s\n", Q3_VERSION, PLATFORM_STRING, __DATE__);

	if(setjmp (abortframe))
		Sys_Error ("Error during initialization");

	/* Clear queues */
	Q_Memset(&eventQueue[ 0 ], 0, MAX_QUEUED_EVENTS * sizeof(Sysevent));

	/* initialize the weak pseudo-random number generator for use later. */
	Com_Initrand();

	/* do this before anything else decides to push events */
	Com_Initpushevent();

	Com_Initsmallzone();
	Cvar_Init();

	/* prepare enough of the subsystems to handle
	 * cvar and command buffer management */
	Com_Parsecmdline(commandLine);

/*	Swap_Init (); */
	Cbuf_Init ();

	Com_Detectsse();

	/* override anything from the config files with command line args */
	Com_Startupvar(nil);

	Com_Initzone();
	Cmd_Init ();

	/* get the developer cvar set as early as possible */
	com_developer = Cvar_Get("developer", "0", CVAR_TEMP);

	/* done early so bind command exists */
	CL_InitKeyCommands();

	com_standalone	= Cvar_Get("com_standalone", "0", CVAR_ROM);
	com_basegame	= Cvar_Get("com_basegame", BASEGAME, CVAR_INIT);
	com_homepath	= Cvar_Get("com_homepath", "", CVAR_INIT);

	if(!com_basegame->string[0])
		Cvar_ForceReset("com_basegame");

	FS_InitFilesystem ();

	Com_Initjournaling();

	/* Add some commands here already so users can use them from config files */
	Cmd_AddCommand ("setenv", Com_Setenv_f);
	if(com_developer && com_developer->integer){
		Cmd_AddCommand("error", Com_Errorf_f);
		Cmd_AddCommand("crash", Com_Crash_f);
		Cmd_AddCommand("freeze", Com_Freeze_f);
	}
	Cmd_AddCommand("testutf", Com_Testutf8_f);
	Cmd_AddCommand("testmaths", Com_Testmaths_f);
	Cmd_AddCommand("quit", Com_Quit_f);
	Cmd_AddCommand("q", Com_Quit_f);
	Cmd_AddCommand("writeconfig", Com_Writeconfig_f);
	Cmd_SetCommandCompletionFunc("writeconfig", Cmd_CompleteCfgName);
	Cmd_AddCommand("game_restart", Com_Gamerestart_f);

	Com_Execconfig();

	/* override anything from the config files with command line args */
	Com_Startupvar(nil);

	/* get dedicated here for proper hunk megs initialization */
#ifdef DEDICATED
	com_dedicated = Cvar_Get("dedicated", "1", CVAR_INIT);
	Cvar_CheckRange(com_dedicated, 1, 2, qtrue);
#else
	com_dedicated = Cvar_Get("dedicated", "0", CVAR_LATCH);
	Cvar_CheckRange(com_dedicated, 0, 2, qtrue);
#endif
	/* allocate the stack based hunk allocator */
	Com_Inithunk();

	/* 
	 * if any archived cvars are modified after this, we will trigger a write
	 * of the config file 
	 */
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	/*
	 * init commands and vars
	 *  */
	com_altivec = Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);
	com_maxfps = Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE);
	com_blood = Cvar_Get("com_blood", "1", CVAR_ARCHIVE);

	com_logfile = Cvar_Get("logfile", "0", CVAR_TEMP);

	com_timescale = Cvar_Get ("timescale", "1",
		CVAR_CHEAT | CVAR_SYSTEMINFO);
	com_fixedtime	= Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace	= Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
	com_speeds	= Cvar_Get("com_speeds", "0", 0);
	com_timedemo	= Cvar_Get("timedemo", "0", CVAR_CHEAT);
	com_cameraMode	= Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);

	cl_paused = Cvar_Get ("cl_paused", "0", CVAR_ROM);
	sv_paused = Cvar_Get ("sv_paused", "0", CVAR_ROM);
	cl_packetdelay	= Cvar_Get ("cl_packetdelay", "0", CVAR_CHEAT);
	sv_packetdelay	= Cvar_Get ("sv_packetdelay", "0", CVAR_CHEAT);
	com_sv_running	= Cvar_Get ("sv_running", "0", CVAR_ROM);
	com_cl_running	= Cvar_Get ("cl_running", "0", CVAR_ROM);
	com_buildScript = Cvar_Get("com_buildScript", "0", 0);
	com_ansiColor	= Cvar_Get("com_ansiColor", "0", CVAR_ARCHIVE);

	com_unfocused = Cvar_Get("com_unfocused", "0", CVAR_ROM);
	com_maxfpsUnfocused = Cvar_Get("com_maxfpsUnfocused", "0", CVAR_ARCHIVE);
	com_minimized = Cvar_Get("com_minimized", "0", CVAR_ROM);
	com_maxfpsMinimized = Cvar_Get("com_maxfpsMinimized", "0", CVAR_ARCHIVE);
	com_abnormalExit = Cvar_Get("com_abnormalExit", "0", CVAR_ROM);
	com_busyWait = Cvar_Get("com_busyWait", "0", CVAR_ARCHIVE);
	Cvar_Get("com_errorMessage", "", CVAR_ROM | CVAR_NORESTART);

	com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);

	s = va("%s %s %s", Q3_VERSION, PLATFORM_STRING, __DATE__);
	com_version = Cvar_Get ("version", s, CVAR_ROM | CVAR_SERVERINFO);
	com_gamename = Cvar_Get("com_gamename", GAMENAME_FOR_MASTER,
		CVAR_SERVERINFO | CVAR_INIT);
	com_protocol = Cvar_Get("com_protocol", va("%i",
			PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_INIT);
	Cvar_Get("protocol", com_protocol->string, CVAR_ROM);

	Sys_Init();
	
	/* Pick a random port value */
	Com_Randombytes((byte*)&qport, sizeof(int));
	Netchan_Init(qport & 0xffff);

	VM_Init();
	SV_Init();

	com_dedicated->modified = qfalse;
	if(!com_dedicated->integer)
		CL_Init();

	/* 
	 * set com_frameTime so that if a map is started on the
	 * command line it will still be able to count on com_frameTime
	 * being random enough for a serverid 
	 */
	com_frameTime = Com_Millisecs();

	/* add + commands from command line */
	if(!Com_Addstartupcmds())
		/* if the user didn't give any commands, run default action */
		if(!com_dedicated->integer){
			Cbuf_AddText ("cinematic idlogo.RoQ\n");
			if(!com_introPlayed->integer){
				Cvar_Set(com_introPlayed->name, "1");
				Cvar_Set("nextmap", "cinematic intro.RoQ");
			}
		}

	/* start in full screen ui mode */
	Cvar_Set("r_uiFullScreen", "1");

	CL_StartHunkUsers(qfalse);

	/* make sure single player is off by default */
	Cvar_Set("ui_singlePlayerActive", "0");

	com_fullyInitialized = qtrue;

	/* always set the cvar, but only print the info if it makes sense. */
	Com_Detectaltivec();
#if idppc
	Com_Printf ("Altivec support is %s\n",
		com_altivec->integer ? "enabled" : "disabled");
#endif

	com_pipefile = Cvar_Get("com_pipefile", "", CVAR_ARCHIVE|CVAR_LATCH);
	if(com_pipefile->string[0])
		pipefile = FS_FCreateOpenPipeFile(com_pipefile->string);

	Com_Printf ("--- Common Initialization Complete ---\n");
}

void
Com_Shutdown(void)
{
	if(logfile){
		FS_FCloseFile (logfile);
		logfile = 0;
	}

	if(com_journalFile){
		FS_FCloseFile(com_journalFile);
		com_journalFile = 0;
	}

	if(pipefile){
		FS_FCloseFile(pipefile);
		FS_HomeRemove(com_pipefile->string);
	}

}

/* Read whatever is in com_pipefile, if anything, and execute it */
void
Com_Readpipe(void)
{
	static char	buf[MAX_STRING_CHARS];
	static uint	accu = 0;
	int read;

	if(!pipefile)
		return;
	while((read = FS_Read(buf+accu, sizeof(buf)-accu-1, pipefile)) > 0){
		char *brk = nil;
		uint i;

		for(i = accu; i < accu + read; ++i){
			if(buf[i] == '\0')
				buf[ i ] = '\n';
			if(buf[i] == '\n' || buf[i] == '\r')
				brk = &buf[i+1];
		}
		buf[accu+read] = '\0';
		accu += read;
		if(brk){
			char tmp = *brk;
			*brk = '\0';
			Cbuf_ExecuteText(EXEC_APPEND, buf);
			*brk = tmp;

			accu -= brk - buf;
			memmove(buf, brk, accu + 1);
		}else if(accu >= sizeof(buf) - 1){	/* full */
			Cbuf_ExecuteText(EXEC_APPEND, buf);
			accu = 0;
		}
	}
}

void
Com_Frame(void)
{
	int	msec, minMsec;
	int	timeVal, timeValSV;
	static int lastTime = 0, bias = 0;
	int	timeBeforeFirstEvents;
	int	timeBeforeServer;
	int	timeBeforeEvents;
	int	timeBeforeClient;
	int	timeAfter;


	if(setjmp (abortframe))
		return;		/* an ERR_DROP was thrown */
	timeBeforeFirstEvents	=0;
	timeBeforeServer	=0;
	timeBeforeEvents	=0;
	timeBeforeClient	= 0;
	timeAfter = 0;

	/* write config file if anything changed */
	/* FIXME: add interval? */
	Com_Writeconfig();

	/* main event loop */
	if(com_speeds->integer)
		timeBeforeFirstEvents = Sys_Milliseconds ();
	/* Figure out how much time we have */
	if(!com_timedemo->integer){
		if(com_dedicated->integer)
			minMsec = SV_FrameMsec();
		else{
			if(com_minimized->integer &&
			   com_maxfpsMinimized->integer > 0)
				minMsec = 1000 / com_maxfpsMinimized->integer;
			else if(com_unfocused->integer &&
				com_maxfpsUnfocused->integer > 0)
				minMsec = 1000 / com_maxfpsUnfocused->integer;
			else if(com_maxfps->integer > 0)
				minMsec = 1000 / com_maxfps->integer;
			else
				minMsec = 1;

			timeVal = com_frameTime - lastTime;
			bias += timeVal - minMsec;

			if(bias > minMsec)
				bias = minMsec;

			/* Adjust minMsec if previous frame took too long to render so
			 * that framerate is stable at the requested value. */
			minMsec -= bias;
		}
	}else
		minMsec = 1;

	do{
		if(com_sv_running->integer){
			timeValSV = SV_SendQueuedPackets();

			timeVal = Q_TimeVal(minMsec);

			if(timeValSV < timeVal)
				timeVal = timeValSV;
		}else
			timeVal = Q_TimeVal(minMsec);

		if(com_busyWait->integer || timeVal < 1)
			NET_Sleep(0);
		else
			NET_Sleep(timeVal - 1);
	}while(Q_TimeVal(minMsec));

	lastTime = com_frameTime;
	com_frameTime = Com_Eventloop();

	msec = com_frameTime - lastTime;

	Cbuf_Execute();

	if(com_altivec->modified){
		Com_Detectaltivec();
		com_altivec->modified = qfalse;
	}

	/* mess with msec if needed */
	msec = Com_Modifymsec(msec);

	/* server side */
	if(com_speeds->integer)
		timeBeforeServer = Sys_Milliseconds ();
	SV_Frame(msec);
	/* if "dedicated" has been modified, start up
	 * or shut down the client system.
	 * Do this after the server may have started,
	 * but before the client tries to auto-connect */
	if(com_dedicated->modified){
		/* get the latched value */
		Cvar_Get("dedicated", "0", 0);
		com_dedicated->modified = qfalse;
		if(!com_dedicated->integer){
			SV_Shutdown("dedicated set to 0");
			CL_FlushMemory();
		}
	}

	if(com_dedicated->integer){
		if(com_speeds->integer){
			timeAfter = Sys_Milliseconds ();
			timeBeforeEvents = timeAfter;
			timeBeforeClient = timeAfter;
		}
	}else{
		/*
		 * client system
		 *
		 * run event loop a second time to get server to client packets
		 * without a frame of latency
		 */
		if(com_speeds->integer)
			timeBeforeEvents = Sys_Milliseconds ();
		Com_Eventloop();
		Cbuf_Execute ();


		/* client side */
		if(com_speeds->integer)
			timeBeforeClient = Sys_Milliseconds ();
		CL_Frame(msec);
		if(com_speeds->integer)
			timeAfter = Sys_Milliseconds ();
	}

	NET_FlushPacketQueue();

	/* report timing information */
	if(com_speeds->integer){
		int all, sv, ev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		ev = timeBeforeServer - timeBeforeFirstEvents +
		     timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;
		Com_Printf (
			"frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
			com_frameNumber, all, sv, ev, cl, time_game,
			time_frontend, time_backend);
	}

	/* trace optimization tracking */
	if(com_showtrace->integer){

		extern int	c_traces, c_brush_traces, c_patch_traces;
		extern int	c_pointcontents;

		Com_Printf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces	= 0;
		c_patch_traces	= 0;
		c_pointcontents = 0;
	}

	Com_Readpipe( );

	com_frameNumber++;
}

/*
 * command line completion
 */

void
Field_Clear(Field *edit)
{
	memset(edit->buffer, 0, MAX_EDIT_LINE);
	edit->cursor = 0;
	edit->scroll = 0;
}

static const char *completionString;
static char	shortestMatch[MAX_TOKEN_CHARS];
static int	matchCount;
/* field we are working on, passed to Field_AutoComplete(&g_consoleCommand for instance) */
static Field *completionField;

static void
findmatches(const char *s)
{
	uint i;

	if(Q_stricmpn(s, completionString, strlen(completionString)))
		return;
	matchCount++;
	if(matchCount == 1){
		Q_strncpyz(shortestMatch, s, sizeof(shortestMatch));
		return;
	}
	/* cut shortestMatch to the amount common with s */
	for(i = 0; shortestMatch[i]; i++){
		if(i >= strlen(s)){
			shortestMatch[i] = 0;
			break;
		}
		if(tolower(shortestMatch[i]) != tolower(s[i]))
			shortestMatch[i] = 0;
	}
}

static void
printmatches(const char *s)
{
	if(!Q_stricmpn(s, shortestMatch, strlen(shortestMatch)))
		Com_Printf("    %s\n", s);
}

static void
printcvarmatches(const char *s)
{
	char value[ TRUNCATE_LENGTH ];

	if(!Q_stricmpn(s, shortestMatch, strlen(shortestMatch))){
		Q_truncstr(value, Cvar_VariableString(s));
		Com_Printf("    %s = \"%s\"\n", s, value);
	}
}

static char *
Field_FindFirstSeparator(char *s)
{
	uint i;

	for(i = 0; i < strlen(s); i++)
		if(s[ i ] == ';')
			return &s[ i ];

	return nil;
}

static qbool
Field_Complete(void)
{
	int completionOffset;

	if(matchCount == 0)
		return qtrue;
	completionOffset = strlen(completionField->buffer) - strlen(
		completionString);
	Q_strncpyz(&completionField->buffer[ completionOffset ], shortestMatch,
		sizeof(completionField->buffer) - completionOffset);
	completionField->cursor = strlen(completionField->buffer);

	if(matchCount == 1){
		Q_strcat(completionField->buffer, sizeof(completionField->buffer),
			" ");
		completionField->cursor++;
		return qtrue;
	}
	Com_Printf("]%s\n", completionField->buffer);
	return qfalse;
}

#ifndef DEDICATED
void
Field_CompleteKeyname(void)
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	Key_KeynameCompletion(findmatches);

	if(!Field_Complete( ))
		Key_KeynameCompletion(printmatches);
}
#endif

void
Field_CompleteFilename(const char *dir, const char *ext, qbool stripExt,
	qbool allowNonPureFilesOnDisk)
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	FS_FilenameCompletion(dir, ext, stripExt, findmatches,
		allowNonPureFilesOnDisk);
	if(!Field_Complete( ))
		FS_FilenameCompletion(dir, ext, stripExt, printmatches,
			allowNonPureFilesOnDisk);
}

void
Field_CompleteCommand(char *cmd,
		      qbool doCommands, qbool doCvars)
{
	int completionArgument = 0;

	/* Skip leading whitespace and quotes */
	cmd = Q_skipcharset(cmd, " \"");

	Cmd_TokenizeStringIgnoreQuotes(cmd);
	completionArgument = Cmd_Argc( );

	/* If there is trailing whitespace on the cmd */
	if(*(cmd + strlen(cmd) - 1) == ' '){
		completionString = "";
		completionArgument++;
	}else
		completionString = Cmd_Argv(completionArgument - 1);
		
#ifndef DEDICATED
	/* Unconditionally add a '/' to the start of the buffer */
	if(completionField->buffer[0] &&
	   completionField->buffer[0] != '/'){
		if(completionField->buffer[0] != '\\'){
			/* Buffer is full, refuse to complete */
			if(strlen(completionField->buffer) + 1 >=
			   sizeof(completionField->buffer))
				return;

			memmove(&completionField->buffer[1],
				&completionField->buffer[0],
				strlen(completionField->buffer) + 1);
			completionField->cursor++;
		}
		completionField->buffer[0] = '/';
	}
#endif
	if(completionArgument > 1){
		const char *baseCmd = Cmd_Argv(0);
		char *p;

#ifndef DEDICATED
		/* This should always be true */
		if(baseCmd[0] == '\\' || baseCmd[0] == '/')
			baseCmd++;
#endif

		if((p = Field_FindFirstSeparator(cmd)))
			Field_CompleteCommand(p + 1, qtrue, qtrue);	/* Compound command */
		else
			Cmd_CompleteArgument(baseCmd, cmd, completionArgument);
	}else{
		if(completionString[0] == '\\' || completionString[0] == '/')
			completionString++;

		matchCount = 0;
		shortestMatch[0] = 0;

		if(strlen(completionString) == 0)
			return;

		if(doCommands)
			Cmd_CommandCompletion(findmatches);

		if(doCvars)
			Cvar_CommandCompletion(findmatches);

		if(!Field_Complete()){
			/* run through again, printing matches */
			if(doCommands)
				Cmd_CommandCompletion(printmatches);

			if(doCvars)
				Cvar_CommandCompletion(printcvarmatches);
		}
	}
}

/* Perform Tab expansion */
void
Field_AutoComplete(Field *field)
{
	completionField = field;

	Field_CompleteCommand(completionField->buffer, qtrue, qtrue);
}

/* fills string array with len radom bytes, peferably from the OS randomizer */
void
Com_Randombytes(byte *string, int len)
{
	int i;

	if(Sys_RandomBytes(string, len))
		return;
	Com_Printf("Com_Randombytes: using weak randomization\n");
	for(i = 0; i < len; i++)
		string[i] = (unsigned char)(rand() % 255);
}

/*
 * Returns non-zero if given clientNum is enabled in voipTargets, zero otherwise.
 * If clientNum is negative return if any bit is set.
 */
qbool
Com_Isvoiptarget(uint8_t *voipTargets, int voipTargetsSize, int clientNum)
{
	int index;
	if(clientNum < 0){
		for(index = 0; index < voipTargetsSize; index++)
			if(voipTargets[index])
				return qtrue;
		return qfalse;
	}
	index = clientNum >> 3;
	if(index < voipTargetsSize)
		return (voipTargets[index] & (1 << (clientNum & 0x07)));
	return qfalse;
}
