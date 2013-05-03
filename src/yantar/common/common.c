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

extern void	cominithunk(void);
extern void	cominitsmallzone(void);
extern void	cominitzone(void);
extern void	CIN_CloseAllVideos(void);

static char	*rd_buffer;
static uint	rd_buffersize;
static void	(*rd_flush)(char *buffer);

void
comstartredirect(char *buffer, int buffersize, void (*flush)(char *))
{
	if(!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void
comendredirect(void)
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
comprintf(const char *fmt, ...)
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
	clconsoleprint(msg);
#endif
	sysprint(msg); /* echo to dedicated console and early console */
	
	/* logfile */
	if(com_logfile && com_logfile->integer){
		/* TTimo: only open the qconsole.log if the filesystem is in an initialized state
		*   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on) */
		if(!logfile && fsisinitialized() && !opening_qconsole){
			struct tm *newtime;
			time_t aclock;

			opening_qconsole = qtrue;
			time(&aclock);
			newtime = localtime(&aclock);
			logfile = fsopenw("qconsole.log");
			if(logfile){
				comprintf("logfile opened on %s\n",
					asctime(newtime));

				if(com_logfile->integer > 1)
					/* force it to not buffer so we get valid
					 * data even if we are crashing */
					fsforceflush(logfile);
			}else{
				comprintf("Opening qconsole.log failed!\n");
				cvarsetf("logfile", 0);
			}
			opening_qconsole = qfalse;
		}
		if(logfile && fsisinitialized())
			fswrite(msg, strlen(msg), logfile);
	}
}

/*
 * A comprintf that only shows up if the "developer" cvar is set
 */
void QDECL
comdprintf(const char *fmt, ...)
{
	va_list argptr;
	char	msg[MAXPRINTMSG];

	if(!com_developer || !com_developer->integer)
		return;		/* don't confuse non-developers with techie stuff... */

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	comprintf ("%s", msg);
}

/*
 * Both client and server can use this, and it will
 * do the appropriate thing.
 */
void QDECL
comerrorf(int code, const char *fmt, ...)
{
	va_list argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int currentTime;

	if(com_errorEntered)
		syserrorf("recursive error after: %s", com_errorMessage);

	com_errorEntered = qtrue;

	cvarsetstr("com_errorCode", va("%i", code));

	/* when we are running automated scripts, make sure we
	 * know if anything failed */
	if(com_buildScript && com_buildScript->integer)
		code = ERR_FATAL;

	/* if we are getting a solid stream of ERR_DROP, do an ERR_FATAL */
	currentTime = sysmillisecs();
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
		cvarsetstr("com_errorMessage", com_errorMessage);

	if(code == ERR_DISCONNECT || code == ERR_SERVERDISCONNECT){
		vmsetforceunload();
		svshutdown("Server disconnected");
		cldisconnect(qtrue);
		clflushmem( );
		vmclearforceunload();
		/* make sure we can get at our local stuff */
		fspureservsetloadedpaks("", "");
		com_errorEntered = qfalse;
		longjmp (abortframe, -1);
	}else if(code == ERR_DROP){
		comprintf (
			"********************\nERROR: %s\n********************\n",
			com_errorMessage);
		vmsetforceunload();
		svshutdown (va("Server crashed: %s",  com_errorMessage));
		cldisconnect(qtrue);
		clflushmem( );
		vmclearforceunload();
		fspureservsetloadedpaks("", "");
		com_errorEntered = qfalse;
		longjmp(abortframe, -1);
	}else{
		vmsetforceunload();
		clshutdown(va("Client fatal crashed: %s",
				com_errorMessage), qtrue, qtrue);
		svshutdown(va("Server fatal crashed: %s", com_errorMessage));
		vmclearforceunload();
	}

	comshutdown ();

	syserrorf ("%s", com_errorMessage);
}

/*
 * Both client and server can use this, and it will
 * do the apropriate things.
 */
void
Com_Quit_f(void)
{
	/* don't try to shutdown if we are in a recursive error */
	char *p = cmdargs( );
	if(!com_errorEntered){
		/* Some VMs might execute "quit" command directly,
		 * which would trigger an unload of active VM error.
		 * sysquit will kill this process anyways, so
		 * a corrupt call stack makes no difference */
		vmsetforceunload();
		svshutdown(p[0] ? p : "Server quit");
		clshutdown(p[0] ? p : "Client quit", qtrue, qtrue);
		vmclearforceunload();
		comshutdown ();
		fsshutdown(qtrue);
	}
	sysquit ();
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
static void
parsecmdline(char *commandLine)
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
cominsafemode(void)
{
	int i;

	for(i = 0; i < com_numConsoleLines; i++){
		cmdstrtok(com_consoleLines[i]);
		if(!Q_stricmp(cmdargv(0), "safe")
		   || !Q_stricmp(cmdargv(0), "cvar_restart")){
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
comstartupvar(const char *match)
{
	int i;
	char *s;

	for(i=0; i < com_numConsoleLines; i++){
		cmdstrtok(com_consoleLines[i]);
		if(strcmp(cmdargv(0), "set"))
			continue;

		s = cmdargv(1);

		if(!match || !strcmp(s, match)){
			if(cvarflags(s) == CVAR_NONEXISTENT)
				cvarget(s, cmdargv(2), CVAR_USER_CREATED);
			else
				cvarsetstr2(s, cmdargv(2), qfalse);
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
static qbool
addstartupcmds(void)
{
	int i;
	qbool added;

	added = qfalse;
	/* quote every token, so args with semicolons can work */
	for(i=0; i < com_numConsoleLines; i++){
		if(!com_consoleLines[i] || !com_consoleLines[i][0])
			continue;
		/* set commands already added with comstartupvar */
		if(!Q_stricmpn(com_consoleLines[i], "set", 3))
			continue;
		added = qtrue;
		cbufaddstr(com_consoleLines[i]);
		cbufaddstr("\n");
	}
	return added;
}

void
infoprint(const char *s)
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
		comprintf ("%s ", key);

		if(!*s){
			comprintf("MISSING VALUE\n");
			return;
		}
		o = value;
		s++;
		while(*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if(*s)
			s++;
		comprintf("%s\n", value);
	}
}

static char *
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
filterstr(char *filter, char *name, int casesensitive)
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
filterpath(char *filter, char *name, int casesensitive)
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
	return filterstr(new_filter, new_name, casesensitive);
}

int
comrealtime(Qtime *qtime)
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
cominitjournaling(void)
{
	comstartupvar("journal");
	com_journal = cvarget ("journal", "0", CVAR_INIT);
	if(!com_journal->integer)
		return;

	if(com_journal->integer == 1){
		comprintf("Journaling events\n");
		com_journalFile = fsopenw("journal.dat");
		com_journalDataFile = fsopenw("journaldata.dat");
	}else if(com_journal->integer == 2){
		comprintf("Replaying journaled events\n");
		fsopenr("journal.dat", &com_journalFile, qtrue);
		fsopenr("journaldata.dat", &com_journalDataFile, qtrue);
	}

	if(!com_journalFile || !com_journalDataFile){
		cvarsetstr("com_journal", "0");
		com_journalFile = 0;
		com_journalDataFile = 0;
		comprintf("Couldn't open journal files\n");
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
comqueueevent(int time, Syseventtype type, int value, int value2,
	       int ptrLength,
	       void *ptr)
{
	Sysevent *ev;

	ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];

	if(eventHead - eventTail >= MAX_QUEUED_EVENTS){
		comprintf("comqueueevent: overflow\n");
		/* we are discarding an event, but don't leak memory */
		if(ev->evPtr)
			zfree(ev->evPtr);
		eventTail++;
	}

	eventHead++;

	if(time == 0)
		time = sysmillisecs();

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

Sysevent
comgetsysevent(void)
{
	Sysevent ev;
	char *s;

	/* return if we have data */
	if(eventHead > eventTail){
		eventTail++;
		return eventQueue[ (eventTail - 1) & MASK_QUEUED_EVENTS ];
	}

	/* check for console commands */
	s = sysconsoleinput();
	if(s){
		char	*b;
		int	len;

		len = strlen(s) + 1;
		b = zalloc(len);
		strcpy(b, s);
		comqueueevent(0, SE_CONSOLE, 0, 0, len, b);
	}

	/* return if we have data */
	if(eventHead > eventTail){
		eventTail++;
		return eventQueue[ (eventTail - 1) & MASK_QUEUED_EVENTS ];
	}

	/* create an empty event to return */
	memset(&ev, 0, sizeof(ev));
	ev.evTime = sysmillisecs();

	return ev;
}

static Sysevent
getrealevent(void)
{
	int r;
	Sysevent ev;

	/* either get an event from the system or the journal file */
	if(com_journal->integer == 2){
		r = fsread(&ev, sizeof(ev), com_journalFile);
		if(r != sizeof(ev))
			comerrorf(ERR_FATAL, "Error reading from journal file");
		if(ev.evPtrLength){
			ev.evPtr = zalloc(ev.evPtrLength);
			r = fsread(ev.evPtr, ev.evPtrLength, com_journalFile);
			if(r != ev.evPtrLength)
				comerrorf(ERR_FATAL,
					"Error reading from journal file");
		}
	}else{
		ev = comgetsysevent();

		/* write the journal value out if needed */
		if(com_journal->integer == 1){
			r = fswrite(&ev, sizeof(ev), com_journalFile);
			if(r != sizeof(ev))
				comerrorf(ERR_FATAL,
					"Error writing to journal file");
			if(ev.evPtrLength){
				r = fswrite(ev.evPtr, ev.evPtrLength,
					com_journalFile);
				if(r != ev.evPtrLength)
					comerrorf(
						ERR_FATAL,
						"Error writing to journal file");
			}
		}
	}

	return ev;
}

void
cominitpushevent(void)
{
	/* clear the static buffer array
	 * this requires SE_NONE to be accepted as a valid but NOP event */
	memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
	/* reset counters while we are at it
	 * beware: GetEvent might still return an SE_NONE from the buffer */
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}

static void
pushevent(Sysevent *event)
{
	Sysevent	*ev;
	static int	printedWarning = 0;

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if(com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS){

		/* don't print the warning constantly, or it can give time for more... */
		if(!printedWarning){
			printedWarning = qtrue;
			comprintf("WARNING: pushevent overflow\n");
		}

		if(ev->evPtr)
			zfree(ev->evPtr);
		com_pushedEventsTail++;
	}else
		printedWarning = qfalse;

	*ev = *event;
	com_pushedEventsHead++;
}

static Sysevent
getevent(void)
{
	if(com_pushedEventsHead > com_pushedEventsTail){
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-
					  1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return getrealevent();
}

void
comrunservpacket(Netaddr *evFrom, Bitmsg *buf)
{
	int t1, t2, msec;

	t1 = 0;
	if(com_speeds->integer)
		t1 = sysmillisecs ();
	svpacketevent(*evFrom, buf);
	if(com_speeds->integer){
		t2 = sysmillisecs ();
		msec = t2 - t1;
		if(com_speeds->integer == 3)
			comprintf("svpacketevent time: %i\n", msec);
	}
}

/* Returns last event time */
int
comflushevents(void)
{
	Sysevent	ev;
	Netaddr	evFrom;
	byte	bufData[MAX_MSGLEN];
	Bitmsg	buf;

	bminit(&buf, bufData, sizeof(bufData));
	while(1){
		ev = getevent();
		/* if no more events are available */
		if(ev.evType == SE_NONE){
			/* manually send packet events for the loopback channel */
			while(netgetlooppacket(NS_CLIENT, &evFrom, &buf))
				clpacketevent(evFrom, &buf);
			while(netgetlooppacket(NS_SERVER, &evFrom, &buf))
				/* if the server just shut down, flush the events */
				if(com_sv_running->integer)
					comrunservpacket(&evFrom, &buf);
			return ev.evTime;
		}

		switch(ev.evType){
		case SE_KEY:
			clkeyevent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CHAR:
			clcharevent(ev.evValue);
			break;
		case SE_MOUSE:
			clmouseevent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_JOYSTICK_AXIS:
			cljoystickevent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CONSOLE:
			cbufaddstr((char*)ev.evPtr);
			cbufaddstr("\n");
			break;
		default:
			comerrorf(ERR_FATAL, "comflushevents: bad event type %i",
				ev.evType);
			break;
		}
		/* free any block data */
		if(ev.evPtr)
			zfree(ev.evPtr);
	}

	return 0;	/* never reached */
}

/* Can be used for profiling, but will be journaled accurately */
int
commillisecs(void)
{
	Sysevent ev;

	/* get events and push them until we get a null event with the current time */
	do{
		ev = getrealevent();
		if(ev.evType != SE_NONE)
			pushevent(&ev);
	}while(ev.evType != SE_NONE);
	return ev.evTime;
}

/* Just throw a fatal error to test error shutdown procedures */
static void
error_f(void)
{
	if(cmdargc() > 1)
		comerrorf(ERR_DROP, "Testing drop error");
	else
		comerrorf(ERR_FATAL, "Testing fatal error");
}

/*
 * Just freeze in place for a given number of seconds to test
 * error recovery
 */
static void
freeze_f(void)
{
	float	s;
	int	start, now;

	if(cmdargc() != 2){
		comprintf("freeze <seconds>\n");
		return;
	}
	s = atof(cmdargv(1));
	start = commillisecs();
	for(;;){
		now = commillisecs();
		if((now - start) * 0.001 > s)
			break;
	}
}

/* A way to force a bus error for development reasons */
static void
crash_f(void)
{
	*(volatile int*)0x0000dead = 0xdeaddead;
}

/* just for testutf8_f */
static void
runeprops(Rune r)
{
	comprintf("rune %#x properties:", r);
	if(Q_isalpharune(r)) comprintf("isalpha ");
	if(Q_isdigitrune(r)) comprintf("isdigit ");
	if(Q_isupperrune(r)) comprintf("isupper ");
	if(Q_islowerrune(r)) comprintf("islower ");
	if(Q_istitlerune(r)) comprintf("istitle ");
	if(Q_isspacerune(r)) comprintf("isspace");
	comprintf("\n");
}

/*
 * crap tests
 */

static void
testutf8_f(void)
{
	char str[50] = "test тест δοκιμή próf 시험 テスト";
	Rune r;
	Rune rstr[29];
	int n;
	uint i;
	
	n = Q_chartorune(&r, &str[5]);
	comprintf("read a utf sequence %i bytes long: %#x\n", n, r);
	for(i = 0; i < 6; i++){
		if(Q_fullrune(&str[5], i)){
			comprintf("letter appears to be a full utf sequence at %u bytes long\n", i);
			break;
		}
		comprintf("letter doesn't appear to be a proper sequence at %u bytes long\n", i);
	}
	comprintf("%s\n", str);
	rstr[0] = r;
	rstr[1] = 'a'; rstr[2] = 's'; rstr[3] = 'd'; rstr[4] = 'f'; rstr[5] = r;
	comprintf("rstr has runestrlen %lu\n", (ulong)Q_runestrlen(rstr));
	comprintf("str has utflen %lu\n", (ulong)Q_utflen(str));
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
			comprintf(" %-7.2f", m[i*w + j]);
		comprintf("\n");
	}
	comprintf("\n");
}

static void
printquat(Quat q)
{
	comprintf("% .4g%+.4gi%+.4gj%+.4gk\n", q[0], q[1], q[2], q[3]);
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
	
	comprintf("matrixtest:\n\n");
	#define test(stmt, var)	(stmt); \
		comprintf("%s =\n", XSTRING(stmt)); \
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
	comprintf("c2 %s c2; ", (cmpm2(c2, c2)) ? "==" : "!=");
	comprintf("c2 %s a2\n\n", (cmpm2(c2, a2)) ? "==" : "!=");
	#undef test

	#define test(stmt, var)	(stmt); \
		comprintf("%s =\n", XSTRING(stmt)); \
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
		comprintf("%s =\n", XSTRING(stmt)); \
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
	
	comprintf("catternyantest:\n\n");
	#define test(stmt, var)	(stmt); \
		comprintf("%-30s = ", XSTRING(stmt)); \
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
		comprintf("%-30s = ", XSTRING(stmt)); \
		comprintf("%f\n", (var))
	test(m = magq(a), m);
	test(m = magq(angle), m);	/* norm of an angle should always be 1.0 */
	#undef test
}

static void
testmaths_f(void)
{
	matrixtest();
	catternyantest();
}

static void
teststrfuncs_f(void)
{
	Vec4 c;
	
	hextriplet2colour("ff00ff", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("12345611", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("0xff332299", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("fff", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("zkxljlzxjcvlkxjcv", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("There's no cure for being a cunt.", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("0X123", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("0X", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("0x", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("0xx", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("0xxx", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
	hextriplet2colour("", c);
	comprintf("(%f, %f, %f, %f)\n", c[0], c[1], c[2], c[3]);
}

/* For controlling environment variables */
static void
setenv_f(void)
{
	int argc = cmdargc();
	char *arg1 = cmdargv(1);

	if(argc > 2){
		char *arg2 = cmdargsfrom(2);

		syssetenv(arg1, arg2);
	}else if(argc == 2){
		char *env = getenv(arg1);

		if(env)
			comprintf("%s=%s\n", arg1, env);
		else
			comprintf("%s undefined\n", arg1);
	}
}

/* For controlling environment variables */
static void
execconfig(void)
{
	cbufexecstr(EXEC_NOW, "exec default.cfg\n");
	cbufflush();	/* Always execute after exec to prevent text buffer overflowing */

	if(!cominsafemode()){
		/* skip the user.cfg if "safe" is on the command line */
		cbufexecstr(EXEC_NOW, "exec " Q3CONFIG_CFG "\n");
		cbufflush();
	}
}

/* Change to a new mod properly with cleaning up cvars before switching. */
void
comgamerestart(int checksumFeed, qbool disconnect)
{
	/* make sure no recursion can be triggered */
	if(!com_gameRestarting && com_fullyInitialized){
		int clWasRunning;

		com_gameRestarting = qtrue;
		clWasRunning = com_cl_running->integer;

		/* Kill server if we have one */
		if(com_sv_running->integer)
			svshutdown("Game directory changed");

		if(clWasRunning){
			if(disconnect)
				cldisconnect(qfalse);

			clshutdown("Game directory changed", disconnect, qfalse);
		}

		fsrestart(checksumFeed);

		/* Clean out any user and VM created cvars */
		cvarrestart(qtrue);
		execconfig();

		if(disconnect){
			/* 
			 * We don't want to change any network settings if gamedir
			 * change was triggered by a connect to server because the
			 * new network settings might make the connection fail. 
			 */
			netrestart();
		}

		if(clWasRunning){
			clinit();
			clstarthunkusers(qfalse);
		}

		com_gameRestarting = qfalse;
	}
}

/* Expose possibility to change current running mod to the user */
void
comgamerestart_f(void)
{
	if(!fscomparefname(cmdargv(1), com_basegame->string))
		/* This is the standard base game. Servers and clients should
		 * use "" and not the standard basegame name because this messes
		 * up pak file negotiation and lots of other stuff */
		cvarsetstr("fs_game", "");
	else
		cvarsetstr("fs_game", cmdargv(1));

	comgamerestart(0, qtrue);
}

static void
writeconfigfile(const char *filename)
{
	Fhandle f;

	f = fsopenw(filename);
	if(!f){
		comprintf ("Couldn't write %s.\n", filename);
		return;
	}
	fsprintf(f, "// ／人◕‿‿ ◕人＼\n");
	keywritebindings(f);
	cvarwritevars(f);
	fsclose(f);
}

/* Writes key bindings and archived cvars to config file if modified */
static void
writeconfig(void)
{
	/* if we are quitting without fully initializing, make sure
	 * we don't write out anything */
	if(!com_fullyInitialized)
		return;

	if(!(cvar_modifiedFlags & CVAR_ARCHIVE))
		return;
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	writeconfigfile(Q3CONFIG_CFG);
}

/* Write the config file to a specific name */
static void
writeconfig_f(void)
{
	char filename[MAX_QPATH];

	if(cmdargc() != 2){
		comprintf("Usage: writeconfig <filename>\n");
		return;
	}
	Q_strncpyz(filename, cmdargv(1), sizeof(filename));
	Q_defaultext(filename, sizeof(filename), ".cfg");
	comprintf("Writing %s.\n", filename);
	writeconfigfile(filename);
}

static int
modifymsec(int msec)
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
			comprintf("Hitch warning: %i msec frame time\n", msec);

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

	timeVal = sysmillisecs() - com_frameTime;

	if(timeVal >= minMsec)
		timeVal = 0;
	else
		timeVal = minMsec - timeVal;
	return timeVal;
}

static void
detectaltivec(void)
{
	/* Only detect if user hasn't forcibly disabled it. */
	if(com_altivec->integer){
		static qbool altivec = qfalse;
		static qbool detected = qfalse;
		if(!detected){
			altivec = (sysgetprocessorfeatures( ) & CF_ALTIVEC);
			detected = qtrue;
		}
		if(!altivec)
			cvarsetstr("com_altivec", "0");	/* we don't have it! Disable support! */
	}
}

#if id386 || idx64
/* Find out whether we have SSE support for Q_ftol function */
static void
detectsse(void)
{
#if !idx64
	CPUfeatures feat;

	feat = sysgetprocessorfeatures();
	if(feat & CF_SSE){
		if(feat & CF_SSE2)
			Q_snapv3 = qsnapv3sse;
		else
			Q_snapv3 = qsnapv3x87;
		Q_ftol = qftolsse;
#endif
		Q_VMftol = qvmftolsse;
		comprintf("Have SSE support\n");
#if !idx64
	}else{
		Q_ftol = qftolx87;
		Q_VMftol = qvmftolx87;
		Q_snapv3 = qsnapv3x87;
		comprintf("No SSE support on this machine\n");
	}
#endif
}

#else

static void
detectsse(void)
{
	;
}
#endif

/* Seed the random number generator, if possible with an OS supplied random seed. */
static void
cominitrand(void)
{
	unsigned int seed;

	if(sysrandbytes((byte*)&seed, sizeof(seed)))
		srand(seed);
	else
		srand(time(nil));
}

/* commandLine should not include the executable name (argv[0]) */
void
cominit(char *commandLine)
{
	char	*s;
	int	qport;

	comprintf("%s %s %s\n", Q3_VERSION, PLATFORM_STRING, __DATE__);

	if(setjmp (abortframe))
		syserrorf ("Error during initialization");

	/* Clear queues */
	Q_Memset(&eventQueue[ 0 ], 0, MAX_QUEUED_EVENTS * sizeof(Sysevent));

	/* initialize the weak pseudo-random number generator for use later. */
	cominitrand();

	/* do this before anything else decides to push events */
	cominitpushevent();

	cominitsmallzone();
	cvarinit();

	/* prepare enough of the subsystems to handle
	 * cvar and command buffer management */
	parsecmdline(commandLine);

/*	Swap_Init (); */
	cbufinit ();

	detectsse();

	/* override anything from the config files with command line args */
	comstartupvar(nil);

	cominitzone();
	cmdinit ();

	/* get the developer cvar set as early as possible */
	com_developer = cvarget("developer", "0", CVAR_TEMP);

	/* done early so bind command exists */
	clinitkeycmds();

	com_standalone	= cvarget("com_standalone", "0", CVAR_ROM);
	com_basegame	= cvarget("com_basegame", BASEGAME, CVAR_INIT);
	com_homepath	= cvarget("com_homepath", "", CVAR_INIT);

	if(!com_basegame->string[0])
		cvarforcereset("com_basegame");

	fsinit ();

	cominitjournaling();

	/* Add some commands here already so users can use them from config files */
	cmdadd ("setenv", setenv_f);
	if(com_developer && com_developer->integer){
		cmdadd("error", error_f);
		cmdadd("crash", crash_f);
		cmdadd("freeze", freeze_f);
	}
	cmdadd("testutf", testutf8_f);
	cmdadd("testmaths", testmaths_f);
	cmdadd("teststrfuncs", teststrfuncs_f);
	cmdadd("quit", Com_Quit_f);
	cmdadd("q", Com_Quit_f);
	cmdadd("writeconfig", writeconfig_f);
	cmdsetcompletion("writeconfig", cmdcompletecfgname);
	cmdadd("game_restart", comgamerestart_f);

	execconfig();

	/* override anything from the config files with command line args */
	comstartupvar(nil);

	/* get dedicated here for proper hunk megs initialization */
#ifdef DEDICATED
	com_dedicated = cvarget("dedicated", "1", CVAR_INIT);
	cvarcheckrange(com_dedicated, 1, 2, qtrue);
#else
	com_dedicated = cvarget("dedicated", "0", CVAR_LATCH);
	cvarcheckrange(com_dedicated, 0, 2, qtrue);
#endif
	/* allocate the stack based hunk allocator */
	cominithunk();

	/* 
	 * if any archived cvars are modified after this, we will trigger a write
	 * of the config file 
	 */
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	/*
	 * init commands and vars
	 *  */
	com_altivec = cvarget("com_altivec", "1", CVAR_ARCHIVE);
	com_maxfps = cvarget("com_maxfps", "85", CVAR_ARCHIVE);
	com_blood = cvarget("com_blood", "1", CVAR_ARCHIVE);

	com_logfile = cvarget("logfile", "0", CVAR_TEMP);

	com_timescale = cvarget ("timescale", "1",
		CVAR_CHEAT | CVAR_SYSTEMINFO);
	com_fixedtime	= cvarget("fixedtime", "0", CVAR_CHEAT);
	com_showtrace	= cvarget("com_showtrace", "0", CVAR_CHEAT);
	com_speeds	= cvarget("com_speeds", "0", 0);
	com_timedemo	= cvarget("timedemo", "0", CVAR_CHEAT);
	com_cameraMode	= cvarget("com_cameraMode", "0", CVAR_CHEAT);

	cl_paused = cvarget ("cl_paused", "0", CVAR_ROM);
	sv_paused = cvarget ("sv_paused", "0", CVAR_ROM);
	cl_packetdelay	= cvarget ("cl_packetdelay", "0", CVAR_CHEAT);
	sv_packetdelay	= cvarget ("sv_packetdelay", "0", CVAR_CHEAT);
	com_sv_running	= cvarget ("sv_running", "0", CVAR_ROM);
	com_cl_running	= cvarget ("cl_running", "0", CVAR_ROM);
	com_buildScript = cvarget("com_buildScript", "0", 0);
	com_ansiColor	= cvarget("com_ansiColor", "0", CVAR_ARCHIVE);

	com_unfocused = cvarget("com_unfocused", "0", CVAR_ROM);
	com_maxfpsUnfocused = cvarget("com_maxfpsUnfocused", "0", CVAR_ARCHIVE);
	com_minimized = cvarget("com_minimized", "0", CVAR_ROM);
	com_maxfpsMinimized = cvarget("com_maxfpsMinimized", "0", CVAR_ARCHIVE);
	com_abnormalExit = cvarget("com_abnormalExit", "0", CVAR_ROM);
	com_busyWait = cvarget("com_busyWait", "0", CVAR_ARCHIVE);
	cvarget("com_errorMessage", "", CVAR_ROM | CVAR_NORESTART);

	s = va("%s %s %s", Q3_VERSION, PLATFORM_STRING, __DATE__);
	com_version = cvarget ("version", s, CVAR_ROM | CVAR_SERVERINFO);
	com_gamename = cvarget("com_gamename", GAMENAME_FOR_MASTER,
		CVAR_SERVERINFO | CVAR_INIT);
	com_protocol = cvarget("com_protocol", va("%i",
			PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_INIT);
	cvarget("protocol", com_protocol->string, CVAR_ROM);

	sysinit();
	
	/* Pick a random port value */
	comrandbytes((byte*)&qport, sizeof(int));
	ncinit(qport & 0xffff);

	vminit();
	svinit();

	com_dedicated->modified = qfalse;
	if(!com_dedicated->integer)
		clinit();

	/* 
	 * set com_frameTime so that if a map is started on the
	 * command line it will still be able to count on com_frameTime
	 * being random enough for a serverid 
	 */
	com_frameTime = commillisecs();

	/* add + commands from command line */
	if(!addstartupcmds())
		/* if the user didn't give any commands, run default action */
		if(!com_dedicated->integer){
			/* (used to play intro cinematic) */
			;
		}

	/* start in full screen ui mode */
	cvarsetstr("r_uiFullScreen", "1");

	clstarthunkusers(qfalse);

	/* make sure single player is off by default */
	cvarsetstr("ui_singlePlayerActive", "0");

	com_fullyInitialized = qtrue;

	/* always set the cvar, but only print the info if it makes sense. */
	detectaltivec();
#if idppc
	comprintf ("Altivec support is %s\n",
		com_altivec->integer ? "enabled" : "disabled");
#endif

	com_pipefile = cvarget("com_pipefile", "", CVAR_ARCHIVE|CVAR_LATCH);
	if(com_pipefile->string[0])
		pipefile = fscreatepipefile(com_pipefile->string);

	comprintf ("--- Common Initialization Complete ---\n");
}

void
comshutdown(void)
{
	if(logfile){
		fsclose (logfile);
		logfile = 0;
	}

	if(com_journalFile){
		fsclose(com_journalFile);
		com_journalFile = 0;
	}

	if(pipefile){
		fsclose(pipefile);
		fshomeremove(com_pipefile->string);
	}

}

/* Read whatever is in com_pipefile, if anything, and execute it */
static void
readpipe(void)
{
	static char	buf[MAX_STRING_CHARS];
	static uint	accu = 0;
	int read;

	if(!pipefile)
		return;
	while((read = fsread(buf+accu, sizeof(buf)-accu-1, pipefile)) > 0){
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
			cbufexecstr(EXEC_APPEND, buf);
			*brk = tmp;

			accu -= brk - buf;
			memmove(buf, brk, accu + 1);
		}else if(accu >= sizeof(buf) - 1){	/* full */
			cbufexecstr(EXEC_APPEND, buf);
			accu = 0;
		}
	}
}

void
comframe(void)
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
	writeconfig();

	/* main event loop */
	if(com_speeds->integer)
		timeBeforeFirstEvents = sysmillisecs ();
	/* Figure out how much time we have */
	if(!com_timedemo->integer){
		if(com_dedicated->integer)
			minMsec = svframemsec();
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
			timeValSV = svsendqueuedpackets();

			timeVal = Q_TimeVal(minMsec);

			if(timeValSV < timeVal)
				timeVal = timeValSV;
		}else
			timeVal = Q_TimeVal(minMsec);

		if(com_busyWait->integer || timeVal < 1)
			netsleep(0);
		else
			netsleep(timeVal - 1);
	}while(Q_TimeVal(minMsec));

	lastTime = com_frameTime;
	com_frameTime = comflushevents();

	msec = com_frameTime - lastTime;

	cbufflush();

	if(com_altivec->modified){
		detectaltivec();
		com_altivec->modified = qfalse;
	}

	/* mess with msec if needed */
	msec = modifymsec(msec);

	/* server side */
	if(com_speeds->integer)
		timeBeforeServer = sysmillisecs ();
	svframe(msec);
	/* if "dedicated" has been modified, start up
	 * or shut down the client system.
	 * Do this after the server may have started,
	 * but before the client tries to auto-connect */
	if(com_dedicated->modified){
		/* get the latched value */
		cvarget("dedicated", "0", 0);
		com_dedicated->modified = qfalse;
		if(!com_dedicated->integer){
			svshutdown("dedicated set to 0");
			clflushmem();
		}
	}

	if(com_dedicated->integer){
		if(com_speeds->integer){
			timeAfter = sysmillisecs ();
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
			timeBeforeEvents = sysmillisecs ();
		comflushevents();
		cbufflush ();


		/* client side */
		if(com_speeds->integer)
			timeBeforeClient = sysmillisecs ();
		clframe(msec);
		if(com_speeds->integer)
			timeAfter = sysmillisecs ();
	}

	netflushpacketqueue();

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
		comprintf (
			"frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
			com_frameNumber, all, sv, ev, cl, time_game,
			time_frontend, time_backend);
	}

	/* trace optimization tracking */
	if(com_showtrace->integer){

		extern int	c_traces, c_brush_traces, c_patch_traces;
		extern int	c_pointcontents;

		comprintf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces	= 0;
		c_patch_traces	= 0;
		c_pointcontents = 0;
	}

	readpipe( );

	com_frameNumber++;
}

/*
 * command line completion
 */

void
fieldclear(Field *edit)
{
	memset(edit->buffer, 0, MAX_EDIT_LINE);
	edit->cursor = 0;
	edit->scroll = 0;
}

static const char *completionString;
static char	shortestMatch[MAX_TOKEN_CHARS];
static int	matchCount;
/* field we are working on, passed to fieldautocomplete(&g_consoleCommand for instance) */
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
		comprintf("    %s\n", s);
}

static void
printcvarmatches(const char *s)
{
	char value[ TRUNCATE_LENGTH ];

	if(!Q_stricmpn(s, shortestMatch, strlen(shortestMatch))){
		Q_truncstr(value, cvargetstr(s));
		comprintf("    %s = \"%s\"\n", s, value);
	}
}

static char *
findfirstsep(char *s)
{
	uint i;

	for(i = 0; i < strlen(s); i++)
		if(s[ i ] == ';')
			return &s[ i ];

	return nil;
}

static qbool
fieldcomplete(void)
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
	comprintf("]%s\n", completionField->buffer);
	return qfalse;
}

#ifndef DEDICATED
void
fieldcompletekeyname(void)
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	keykeynamecompletion(findmatches);

	if(!fieldcomplete( ))
		keykeynamecompletion(printmatches);
}
#endif

void
fieldcompletefilename(const char *dir, const char *ext, qbool stripExt,
	qbool allowNonPureFilesOnDisk)
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	fsfnamecompletion(dir, ext, stripExt, findmatches,
		allowNonPureFilesOnDisk);
	if(!fieldcomplete( ))
		fsfnamecompletion(dir, ext, stripExt, printmatches,
			allowNonPureFilesOnDisk);
}

void
fieldcompletecmd(char *cmd,
		      qbool doCommands, qbool doCvars)
{
	int completionArgument = 0;

	/* Skip leading whitespace and quotes */
	cmd = Q_skipcharset(cmd, " \"");

	cmdstrtokignorequotes(cmd);
	completionArgument = cmdargc( );

	/* If there is trailing whitespace on the cmd */
	if(*(cmd + strlen(cmd) - 1) == ' '){
		completionString = "";
		completionArgument++;
	}else
		completionString = cmdargv(completionArgument - 1);
		
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
		const char *baseCmd = cmdargv(0);
		char *p;

#ifndef DEDICATED
		/* This should always be true */
		if(baseCmd[0] == '\\' || baseCmd[0] == '/')
			baseCmd++;
#endif

		if((p = findfirstsep(cmd)))
			fieldcompletecmd(p + 1, qtrue, qtrue);	/* Compound command */
		else
			cmdcompletearg(baseCmd, cmd, completionArgument);
	}else{
		if(completionString[0] == '\\' || completionString[0] == '/')
			completionString++;

		matchCount = 0;
		shortestMatch[0] = 0;

		if(strlen(completionString) == 0)
			return;

		if(doCommands)
			cmdcompletion(findmatches);

		if(doCvars)
			cvarcmdcompletion(findmatches);

		if(!fieldcomplete()){
			/* run through again, printing matches */
			if(doCommands)
				cmdcompletion(printmatches);

			if(doCvars)
				cvarcmdcompletion(printcvarmatches);
		}
	}
}

/* Perform Tab expansion */
void
fieldautocomplete(Field *field)
{
	completionField = field;

	fieldcompletecmd(completionField->buffer, qtrue, qtrue);
}

/* fills string array with len radom bytes, peferably from the OS randomizer */
void
comrandbytes(byte *string, int len)
{
	int i;

	if(sysrandbytes(string, len))
		return;
	comprintf("comrandbytes: using weak randomization\n");
	for(i = 0; i < len; i++)
		string[i] = (unsigned char)(rand() % 255);
}

/*
 * Returns non-zero if given clientNum is enabled in voipTargets, zero otherwise.
 * If clientNum is negative return if any bit is set.
 */
qbool
comisvoiptarget(uint8_t *voipTargets, int voipTargetsSize, int clientNum)
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
