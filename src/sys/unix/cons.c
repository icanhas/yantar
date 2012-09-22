/* TTY routines */
 /*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "q_shared.h"
#include "qcommon.h"
#include "../local.h"

#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

/*
 * NOTE: if the user is editing a line when something gets printed to the early
 * console then it won't look good so we provide Hide and Show to be
 * called before and after a stdout or stderr output
 */
 
 enum { HISTLEN = 32 };

extern qbool stdinIsATTY;
static qbool stdin_active;
static qbool ttycon_on = qfalse;	/* general flag to tell about tty console mode */
static int ttycon_hide = 0;
static int	TTY_erase;	/* some key codes that the terminal */
static int	TTY_eof;		/*   may be using; initialised on startup */
static struct termios TTY_tc;
static field_t TTY_con;
/* This is a bit of duplicate of the graphical console history
 * but it's safer/more modular to have our own here */
static field_t ttyEditLines[HISTLEN];
static int hist_current = -1;
static int hist_count = 0;

/*
 * Flush stdin, I suspect some terminals are sending a LOT of shit
 * FIXME relevant?
 */
static void
FlushIn(void)
{
	char key;
	while(read(STDIN_FILENO, &key, 1) != -1)
		;
}

/*
 * Reinitialize console input after receiving SIGCONT, as on Linux the terminal seems to lose all
 * set attributes if user did CTRL+Z and then does fg again.
 */
static void
SigCont(int signum)
{
	CON_Init();
}

/*
 * Output a backspace
 *
 * NOTE: it seems on some terminals just sending '\b' is not enough so instead we
 * send "\b \b"
 * (FIXME there may be a way to find out if '\b' alone would work though)
 */
static void
Back(void)
{
	char key;

	key = '\b';
	(void)write(STDOUT_FILENO, &key, 1);
	key = ' ';
	(void)write(STDOUT_FILENO, &key, 1);
	key = '\b';
	(void)write(STDOUT_FILENO, &key, 1);
}

/*
 * Clear the display of the line currently edited
 * bring cursor back to beginning of line
 */
static void
Hide(void)
{
	if(ttycon_on){
		int i;
		if(ttycon_hide){
			ttycon_hide++;
			return;
		}
		if(TTY_con.cursor>0)
			for(i=0; i<TTY_con.cursor; i++)
				Back();
		Back();	/* Delete "]" */
		ttycon_hide++;
	}
}

/*
 * Show the current line
 * FIXME need to position the cursor if needed?
 */
static void
Show(void)
{
	if(ttycon_on){
		int i;

		assert(ttycon_hide>0);
		ttycon_hide--;
		if(ttycon_hide == 0){
			(void)write(STDOUT_FILENO, "]", 1);
			if(TTY_con.cursor)
				for(i=0; i<TTY_con.cursor; i++)
					(void)write(STDOUT_FILENO,
						TTY_con.buffer+i, 1);
		}
	}
}

static void
HistAdd(field_t *field)
{
	int i;
	assert(hist_count <= HISTLEN);
	assert(hist_count >= 0);
	assert(hist_current >= -1);
	assert(hist_current <= hist_count);
	/* make some room */
	for(i=HISTLEN-1; i>0; i--)
		ttyEditLines[i] = ttyEditLines[i-1];
	ttyEditLines[0] = *field;
	if(hist_count<HISTLEN)
		hist_count++;
	hist_current = -1;	/* re-init */
}

static field_t *
HistPrev(void)
{
	int hist_prev;
	assert(hist_count <= HISTLEN);
	assert(hist_count >= 0);
	assert(hist_current >= -1);
	assert(hist_current <= hist_count);
	hist_prev = hist_current + 1;
	if(hist_prev >= hist_count)
		return NULL;
	hist_current++;
	return &(ttyEditLines[hist_current]);
}

static field_t *
HistNext(void)
{
	assert(hist_count <= HISTLEN);
	assert(hist_count >= 0);
	assert(hist_current >= -1);
	assert(hist_current <= hist_count);
	if(hist_current >= 0)
		hist_current--;
	if(hist_current == -1)
		return NULL;
	return &(ttyEditLines[hist_current]);
}

/* Initialize the console input (tty mode if possible) */
void
CON_Init(void)
{
	struct termios tc;

	/* If the process is backgrounded (running non interactively)
	 * then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP */
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	/* If SIGCONT is received, reinitialize console */
	signal(SIGCONT, SigCont);
	/* Make stdin reads non-blocking */
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL,
			0) | O_NONBLOCK);

	if(!stdinIsATTY){
		Com_Printf("tty console mode disabled\n");
		ttycon_on = qfalse;
		stdin_active = qtrue;
		return;
	}

	Field_Clear(&TTY_con);
	tcgetattr (STDIN_FILENO, &TTY_tc);
	TTY_erase = TTY_tc.c_cc[VERASE];
	TTY_eof = TTY_tc.c_cc[VEOF];
	tc = TTY_tc;

	/*
	 * ECHO: don't echo input characters
	 * ICANON: enable canonical mode.  This  enables  the  special
	 * characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	 * STATUS, and WERASE, and buffers by lines.
	 * ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	 * DSUSP are received, generate the corresponding sig
	 * nal
	 */
	tc.c_lflag &= ~(ECHO | ICANON);

	/*
	 * ISTRIP strip off bit 8
	 * INPCK enable input parity checking
	 */
	tc.c_iflag &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN]	= 1;
	tc.c_cc[VTIME]	= 0;
	tcsetattr (STDIN_FILENO, TCSADRAIN, &tc);
	ttycon_on = qtrue;
}

/* Never exit without calling this, or your terminal will be left in a pretty bad state */
void
CON_Shutdown(void)
{
	if(ttycon_on){
		Back();	/* Delete "]" */
		tcsetattr (STDIN_FILENO, TCSADRAIN, &TTY_tc);
	}

	/* Restore blocking to stdin reads */
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL,
			0) & ~O_NONBLOCK);
}

char *
CON_Input(void)
{
	/* we use this when sending back commands */
	static char text[MAX_EDIT_LINE];
	int	avail;
	char	key;
	field_t *history;

	if(ttycon_on){
		avail = read(STDIN_FILENO, &key, 1);
		if(avail != -1){
			/* we have something
			 * backspace?
			 * NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere */
			if((key == TTY_erase) || (key == 127) || (key == 8)){
				if(TTY_con.cursor > 0){
					TTY_con.cursor--;
					TTY_con.buffer[TTY_con.cursor] = '\0';
					Back();
				}
				return NULL;
			}
			/* check if this is a control char */
			if((key) && (key) < ' '){
				if(key == '\n'){
					/* push it in history */
					HistAdd(&TTY_con);
					Q_strncpyz(text, TTY_con.buffer, sizeof(text));
					Field_Clear(&TTY_con);
					key = '\n';
					(void)write(STDOUT_FILENO, &key, 1);
					(void)write(STDOUT_FILENO, "]", 1);
					return text;
				}
				if(key == '\t'){
					Hide();
					Field_AutoComplete(&TTY_con);
					Show();
					return NULL;
				}
				avail = read(STDIN_FILENO, &key, 1);
				if(avail != -1){
					/* VT 100 keys */
					if(key == '[' || key == 'O'){
						avail = read(STDIN_FILENO, &key, 1);
						if(avail != -1){
							switch(key){
							case 'A':
								history = HistPrev();
								if(history){
									Hide();
									TTY_con = *history;
									Show();
								}
								FlushIn();
								return NULL;
								break;
							case 'B':
								history = HistNext();
								Hide();
								if(history)
									TTY_con = *history;
								else
									Field_Clear(&TTY_con);
								Show();
								FlushIn();
								return NULL;
								break;
							case 'C':
								return NULL;
							case 'D':
								return NULL;
							}
						}
					}
				}
				Com_DPrintf("dropping ISCTL sequence: %d, TTY_erase: %d\n",
					key, TTY_erase);
				FlushIn();
				return NULL;
			}
			if(TTY_con.cursor >= sizeof(text) - 1)
				return NULL;
			/* push regular character */
			TTY_con.buffer[TTY_con.cursor] = key;
			TTY_con.cursor++;
			/* print the current line (this is differential) */
			(void)write(STDOUT_FILENO, &key, 1);
		}
		return NULL;
	}else if(stdin_active){
		int len;
		fd_set fdset;
		struct timeval timeout;

		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);	/* stdin */
		timeout.tv_sec	= 0;
		timeout.tv_usec = 0;
		if(select (STDIN_FILENO + 1, &fdset, NULL, NULL,
			   &timeout) == -1 || !FD_ISSET(STDIN_FILENO, &fdset))
			return NULL;
		len = read(STDIN_FILENO, text, sizeof(text));
		if(len == 0){	/* eof! */
			stdin_active = qfalse;
			return NULL;
		}

		if(len < 1)
			return NULL;
		text[len-1] = 0;	/* rip off the /n and terminate */
		return text;
	}
	return NULL;
}

void
CON_Print(const char *msg)
{
	Hide();

	if(com_ansiColor && com_ansiColor->integer)
		Sys_AnsiColorPrint(msg);
	else
		fputs(msg, stderr);

	Show();
}
