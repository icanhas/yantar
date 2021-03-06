/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "client.h"
#include "keycodes.h"
#include "keys.h"

int g_console_field_width = 78;

#define NUM_CON_TIMES		4

#define         CON_TEXTSIZE	32768
typedef struct {
	qbool		initialized;

	short		text[CON_TEXTSIZE];
	int		current;	/* line where next message will be printed */
	int		x;		/* offset in current line for next print */
	int		display;	/* bottom of console displays this line */

	int		linewidth;	/* characters across screen */
	int		totallines;	/* total lines in console scrollback */

	float		xadjust;	/* for wide aspect screens */

	float		opacity;	/* aproaches finalopac at scr_conspeed */
	float		finalopac;

	int		vislines;	/* in scanlines */

	int		times[NUM_CON_TIMES];	/* cls.realtime time the line was generated */
	Vec4		color;	/* for transparent notify lines */
} Console;

extern Console con;

Console	con;

Cvar		*con_conspeed;
Cvar		*con_notifytime;

#define DEFAULT_CONSOLE_WIDTH 78


/*
 * Con_ToggleConsole_f
 */
void
Con_ToggleConsole_f(void)
{
	/* Can't toggle the console when it's the only thing available */
	if(clc.state == CA_DISCONNECTED && Key_GetCatcher( ) ==
	   KEYCATCH_CONSOLE)
		return;

	fieldclear(&g_consoleField);
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	Key_SetCatcher(Key_GetCatcher( ) ^ KEYCATCH_CONSOLE);
}

/*
 * Con_MessageMode_f
 */
void
Con_MessageMode_f(void)
{
	chat_playerNum = -1;
	chat_team = qfalse;
	fieldclear(&chatField);
	chatField.widthInChars = 30;

	Key_SetCatcher(Key_GetCatcher( ) ^ KEYCATCH_MESSAGE);
}

/*
 * Con_MessageMode2_f
 */
void
Con_MessageMode2_f(void)
{
	chat_playerNum = -1;
	chat_team = qtrue;
	fieldclear(&chatField);
	chatField.widthInChars = 25;
	Key_SetCatcher(Key_GetCatcher( ) ^ KEYCATCH_MESSAGE);
}

/*
 * Con_MessageMode3_f
 */
void
Con_MessageMode3_f(void)
{
	chat_playerNum = vmcall(cgvm, CG_CROSSHAIR_PLAYER);
	if(chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS){
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	fieldclear(&chatField);
	chatField.widthInChars = 30;
	Key_SetCatcher(Key_GetCatcher( ) ^ KEYCATCH_MESSAGE);
}

/*
 * Con_MessageMode4_f
 */
void
Con_MessageMode4_f(void)
{
	chat_playerNum = vmcall(cgvm, CG_LAST_ATTACKER);
	if(chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS){
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	fieldclear(&chatField);
	chatField.widthInChars = 30;
	Key_SetCatcher(Key_GetCatcher( ) ^ KEYCATCH_MESSAGE);
}

/*
 * Con_Clear_f
 */
void
Con_Clear_f(void)
{
	int i;

	for(i = 0; i < CON_TEXTSIZE; i++)
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';

	Con_Bottom();	/* go to end */
}


/*
 * Con_Dump_f
 *
 * Save the console contents out to a file
 */
void
Con_Dump_f(void)
{
	int l, x, i;
	short	*line;
	Fhandle f;
	char	buffer[1024];

	if(cmdargc() != 2){
		comprintf ("usage: condump <filename>\n");
		return;
	}

	comprintf ("Dumped console text to %s.\n", cmdargv(1));

	f = fsopenw(cmdargv(1));
	if(!f){
		comprintf ("ERROR: couldn't open.\n");
		return;
	}

	/* skip empty lines */
	for(l = con.current - con.totallines + 1; l <= con.current; l++){
		line = con.text + (l%con.totallines)*con.linewidth;
		for(x=0; x<con.linewidth; x++)
			if((line[x] & 0xff) != ' ')
				break;
		if(x != con.linewidth)
			break;
	}

	/* write the remaining lines */
	buffer[con.linewidth] = 0;
	for(; l <= con.current; l++){
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for(x=con.linewidth-1; x>=0; x--){
			if(buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		strcat(buffer, "\n");
		fswrite(buffer, strlen(buffer), f);
	}

	fsclose(f);
}


/*
 * Con_ClearNotify
 */
void
Con_ClearNotify(void)
{
	int i;

	for(i = 0; i < NUM_CON_TIMES; i++)
		con.times[i] = 0;
}



/*
 * Con_CheckResize
 *
 * If the line width has changed, reformat the buffer.
 */
void
Con_CheckResize(void)
{
	int i,j, width, oldwidth, oldtotallines, numlines, numchars;
	short tbuf[CON_TEXTSIZE];

	width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;

	if(width == con.linewidth)
		return;

	if(width < 1){	/* video hasn't been initialized yet */
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth	= width;
		con.totallines	= CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}else{
		oldwidth = con.linewidth;
		con.linewidth	= width;
		oldtotallines	= con.totallines;
		con.totallines	= CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if(con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;

		if(con.linewidth < numchars)
			numchars = con.linewidth;

		Q_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for(i=0; i<numlines; i++)
			for(j=0; j<numchars; j++)
				con.text[(con.totallines - 1 -
					  i) * con.linewidth + j] =
					tbuf[((con.current - i +
					       oldtotallines) %
					      oldtotallines) * oldwidth + j];

		Con_ClearNotify ();
	}

	con.current	= con.totallines - 1;
	con.display	= con.current;
}

/*
 * Cmd_CompleteTxtName
 */
void
Cmd_CompleteTxtName(char *args, int argNum)
{
	if(argNum == 2)
		fieldcompletefilename("", "txt", qfalse, qtrue);
}


/*
 * Con_Init
 */
void
Con_Init(void)
{
	int i;

	con_notifytime	= cvarget("con_notifytime", "3", 0);
	con_conspeed	= cvarget("scr_conspeed", "3", 0);

	fieldclear(&g_consoleField);
	g_consoleField.widthInChars = g_console_field_width;
	for(i = 0; i < COMMAND_HISTORY; i++){
		fieldclear(&historyEditLines[i]);
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory( );

	cmdadd ("toggleconsole", Con_ToggleConsole_f);
	cmdadd ("messagemode", Con_MessageMode_f);
	cmdadd ("messagemode2", Con_MessageMode2_f);
	cmdadd ("messagemode3", Con_MessageMode3_f);
	cmdadd ("messagemode4", Con_MessageMode4_f);
	cmdadd ("clear", Con_Clear_f);
	cmdadd ("condump", Con_Dump_f);
	cmdsetcompletion("condump", Cmd_CompleteTxtName);
}

/*
 * Con_Shutdown
 */
void
Con_Shutdown(void)
{
	cmdremove("toggleconsole");
	cmdremove("messagemode");
	cmdremove("messagemode2");
	cmdremove("messagemode3");
	cmdremove("messagemode4");
	cmdremove("clear");
	cmdremove("condump");
}

/*
 * Con_Linefeed
 */
void
Con_Linefeed(qbool skipnotify)
{
	int i;

	/* mark time for transparent overlay */
	if(con.current >= 0){
		if(skipnotify)
			con.times[con.current % NUM_CON_TIMES] = 0;
		else
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if(con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%
			  con.totallines)*con.linewidth+
			 i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

/*
 * clconsoleprint
 *
 * Handles cursor positioning, line wrapping, etc
 * All console printing must go through this in order to be logged to disk
 * If no console is visible, the text will appear at the top of the game window
 */
void
clconsoleprint(char *txt)
{
	int y, l;
	unsigned char	c;
	unsigned short	color;
	qbool skipnotify = qfalse;	/* NERVE - SMF */
	int prev;			/* NERVE - SMF */

	/* TTimo - prefix for text that shows up in console but not in notify
	 * backported from RTCW */
	if(!Q_strncmp(txt, "[skipnotify]", 12)){
		skipnotify = qtrue;
		txt += 12;
	}

	/* for some demos we don't want to ever show anything on the console */
	if(cl_noprint && cl_noprint->integer)
		return;

	if(!con.initialized){
		con.color[0] = con.color[1] = con.color[2] = con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);

	while((c = *((unsigned char*)txt)) != 0){
		if(Q_IsColorString(txt)){
			color	= ColorIndex(*(txt+1));
			txt	+= 2;
			continue;
		}

		/* count word length */
		for(l=0; l< con.linewidth; l++)
			if(txt[l] <= ' ')
				break;


		/* word wrap */
		if(l != con.linewidth && (con.x + l >= con.linewidth))
			Con_Linefeed(skipnotify);


		txt++;

		switch(c){
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	/* display character and advance */
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (color << 8) | c;
			con.x++;
			if(con.x >= con.linewidth)
				Con_Linefeed(skipnotify);
			break;
		}
	}


	/* mark time for transparent overlay */
	if(con.current >= 0){
		/* NERVE - SMF */
		if(skipnotify){
			prev = con.current % NUM_CON_TIMES - 1;
			if(prev < 0)
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}else
			/* -NERVE - SMF */
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}


/*
 *
 * DRAWING
 *
 */


/*
 * Con_DrawInput
 *
 * Draw the editline after a ))) prompt
 */
void
Con_DrawInput(void)
{
	int y;

	if(clc.state != CA_DISCONNECTED &&
	   !(Key_GetCatcher( ) & KEYCATCH_CONSOLE))
		return;

	y = con.vislines - (SMALLCHAR_HEIGHT * 2);

	re.SetColor(con.color);

	SCR_DrawSmallStringExt(con.xadjust + 1 * SMALLCHAR_WIDTH, y, ")))",
		colorWhite, qtrue,
		qfalse);

	Field_Draw(&g_consoleField, con.xadjust + 5*SMALLCHAR_WIDTH, y,
		SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue, qtrue);
}


/*
 * Con_DrawNotify
 *
 * Draws the last few lines of output transparently over the game top
 */
void
Con_DrawNotify(void)
{
	int	x, v;
	short *text;
	int	i;
	int	time;
	int	skip;
	int	currentColor;

	currentColor = 7;
	re.SetColor(g_color_table[currentColor]);

	v = 0;
	for(i= con.current-NUM_CON_TIMES+1; i<=con.current; i++){
		if(i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if(time == 0)
			continue;
		time = cls.realtime - time;
		if(time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		if(cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) &
		   (KEYCATCH_UI | KEYCATCH_CGAME))
			continue;

		for(x = 0; x < con.linewidth; x++){
			if((text[x] & 0xff) == ' ')
				continue;
			if(((text[x]>>8)&7) != currentColor){
				currentColor = (text[x]>>8)&7;
				re.SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar(cl_conXOffset->integer + con.xadjust +
				(x+1)*SMALLCHAR_WIDTH, v, text[x] & 0xff);
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor(NULL);

	if(Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME))
		return;

	/* draw the chat line */
	if(Key_GetCatcher( ) & KEYCATCH_MESSAGE){
		if(chat_team){
			SCR_DrawBigString (8, v, "say_team:", 1.0f, qfalse);
			skip = 10;
		}else{
			SCR_DrawBigString (8, v, "say:", 1.0f, qfalse);
			skip = 5;
		}

		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v,
			SCREEN_WIDTH - (skip + 1) * BIGCHAR_WIDTH, qtrue, qtrue);

		v += BIGCHAR_HEIGHT;
	}

}

/*
 * Con_DrawSolidConsole: Draws the console to the desired opacity and screen
 * coverage
 */
void
Con_DrawSolidConsole(float frac, float opac)
{
	int	i, x, y;
	int	rows;
	short *text;
	int	row;
	int	lines;
	int	currentColor;
	Vec4 color;

	lines = cls.glconfig.vidHeight * frac;
	if(lines <= 0)
		return;

	if(lines > cls.glconfig.vidHeight)
		lines = cls.glconfig.vidHeight;

	/* on wide screens, we will center the text */
	con.xadjust = 0;
	SCR_AdjustFrom640(&con.xadjust, NULL, NULL, NULL);

	/* draw the background */
	y = frac * SCREEN_HEIGHT;
	if(y < 1)
		y = 0;

	color[0] = color[1] = color[2] = 0.0f;
	color[3] = opac;
	SCR_FillRect(0, 0, SCREEN_WIDTH, y, color);	/* bg */
	color[0] = color[1] = color[2] = 1.0f;
	SCR_FillRect(0, y, SCREEN_WIDTH, 1, color);	/* border */

	/* draw the version number */

	re.SetColor(g_color_table[ColorIndex(COLOR_WHITE)]);

	i = strlen(Q3_VERSION);

	for(x=0; x<i; x++)
		SCR_DrawSmallChar(
			cls.glconfig.vidWidth - (i - x + 1) * SMALLCHAR_WIDTH,
			lines - SMALLCHAR_HEIGHT, Q3_VERSION[x]);


	/* draw the console text */
	con.vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;	/* rows of text to draw */

	y = lines - (SMALLCHAR_HEIGHT*3);

	/* draw from the bottom up */
	if(con.display != con.current){
		/* draw arrows to show the buffer is backscrolled */
		re.SetColor(g_color_table[ColorIndex(COLOR_BLACK)]);
		for(x=0; x<con.linewidth; x+=4)
			SCR_DrawSmallChar(con.xadjust + (x+1)*SMALLCHAR_WIDTH, y,
				'^');
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = con.display;

	if(con.x == 0)
		row--;

	currentColor = 7;
	re.SetColor(g_color_table[currentColor]);

	for(i=0; i<rows; i++, y -= SMALLCHAR_HEIGHT, row--){
		if(row < 0)
			break;
		if(con.current - row >= con.totallines)
			/* past scrollback wrap point */
			continue;

		text = con.text + (row % con.totallines)*con.linewidth;

		for(x=0; x<con.linewidth; x++){
			if((text[x] & 0xff) == ' ')
				continue;

			if(((text[x]>>8)&7) != currentColor){
				currentColor = (text[x]>>8)&7;
				re.SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar(con.xadjust + (x+1)*SMALLCHAR_WIDTH, y,
				text[x] & 0xff);
		}
	}

	/* draw the input prompt, user text, and cursor if desired */
	Con_DrawInput ();

	re.SetColor(NULL);
}



/*
 * Con_DrawConsole
 */
void
Con_DrawConsole(void)
{
	/* check for console width changes from a vid mode change */
	Con_CheckResize ();

	/* if disconnected, render console full screen */
	if(clc.state == CA_DISCONNECTED)
		if(!(Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME))){
			Con_DrawSolidConsole(1.0f, 1.0f);
			return;
		}

	if(con.opacity > 0.0f)
		Con_DrawSolidConsole(0.5f, con.opacity);
	else
	/* draw notify lines */
	if(clc.state == CA_ACTIVE)
		Con_DrawNotify ();

}

/*
 * Con_RunConsole: Fade it in or out
 */
void
Con_RunConsole(void)
{
	/* decide on the destined opacity of the console */
	if(Key_GetCatcher() & KEYCATCH_CONSOLE)
		con.finalopac = 0.75f;
	else
		con.finalopac = 0.0f;	/* not visible */

	/* fade it towards the destined opacity */
	if(con.finalopac < con.opacity){
		con.opacity -= con_conspeed->value*cls.realframetime*0.001;
		if(con.finalopac > con.opacity)
			con.opacity = con.finalopac;

	}else if(con.finalopac > con.opacity){
		con.opacity += con_conspeed->value*cls.realframetime*0.001;
		if(con.finalopac < con.opacity)
			con.opacity = con.finalopac;
	}

}

void
Con_PageUp(void)
{
	con.display -= 2;
	if(con.current - con.display >= con.totallines)
		con.display = con.current - con.totallines + 1;
}

void
Con_PageDown(void)
{
	con.display += 2;
	if(con.display > con.current)
		con.display = con.current;
}

void
Con_Top(void)
{
	con.display = con.totallines;
	if(con.current - con.display >= con.totallines)
		con.display = con.current - con.totallines + 1;
}

void
Con_Bottom(void)
{
	con.display = con.current;
}


void
Con_Close(void)
{
	if(!com_cl_running->integer)
		return;
	fieldclear(&g_consoleField);
	Con_ClearNotify ();
	Key_SetCatcher(Key_GetCatcher( ) & ~KEYCATCH_CONSOLE);
	con.finalopac	= 0;	/* none visible */
	con.opacity	= 0;
}
