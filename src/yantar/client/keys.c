/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "client.h"
#include "keycodes.h"
#include "keys.h"
#include "ui.h"

/*
 * key up events are sent even if in console mode
 */
 
typedef struct keyname_t keyname_t;

struct keyname_t {
	char	*name;
	int	keynum;
};
 
/* This must not exceed MAX_CMD_LINE */
#define	MAX_CONSOLE_SAVE_BUFFER 1024
#define	CONSOLE_HISTORY_FILE	"consolehistory"

Field	g_consoleField;
Field	chatField;
qbool	chat_team;
int		chat_playerNum;
qbool	key_overstrikeMode;
int		anykeydown;
Qkey	keys[MAX_KEYS];
Field	historyEditLines[COMMAND_HISTORY];

static int	nextHistoryLine;	/* the last line in the history buffer, not masked */
static int	historyLine;		/* the line being displayed from history buffer
						 * will be <= nextHistoryLine */
static int	keyCatchers = 0;
static char	consoleSaveBuffer[MAX_CONSOLE_SAVE_BUFFER];
static int	consoleSaveBufferSize = 0;

/* names not in this list can either be lowercase ascii, or '0xnn' hex sequences */
keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},

	{"COMMAND", K_COMMAND},

	{"CAPSLOCK", K_CAPSLOCK},


	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"MWHEELUP",    K_MWHEELUP },
	{"MWHEELDOWN",  K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"KP_HOME",                     K_KP_HOME },
	{"KP_UPARROW",          K_KP_UPARROW },
	{"KP_PGUP",                     K_KP_PGUP },
	{"KP_LEFTARROW",        K_KP_LEFTARROW },
	{"KP_5",                        K_KP_5 },
	{"KP_RIGHTARROW",       K_KP_RIGHTARROW },
	{"KP_END",                      K_KP_END },
	{"KP_DOWNARROW",        K_KP_DOWNARROW },
	{"KP_PGDN",                     K_KP_PGDN },
	{"KP_ENTER",            K_KP_ENTER },
	{"KP_INS",                      K_KP_INS },
	{"KP_DEL",                      K_KP_DEL },
	{"KP_SLASH",            K_KP_SLASH },
	{"KP_MINUS",            K_KP_MINUS },
	{"KP_PLUS",                     K_KP_PLUS },
	{"KP_NUMLOCK",          K_KP_NUMLOCK },
	{"KP_STAR",                     K_KP_STAR },
	{"KP_EQUALS",           K_KP_EQUALS },

	{"PAUSE", K_PAUSE},

	{"SEMICOLON", ';'},	/* because a raw semicolon seperates commands */

	{"WORLD_0", K_WORLD_0},
	{"WORLD_1", K_WORLD_1},
	{"WORLD_2", K_WORLD_2},
	{"WORLD_3", K_WORLD_3},
	{"WORLD_4", K_WORLD_4},
	{"WORLD_5", K_WORLD_5},
	{"WORLD_6", K_WORLD_6},
	{"WORLD_7", K_WORLD_7},
	{"WORLD_8", K_WORLD_8},
	{"WORLD_9", K_WORLD_9},
	{"WORLD_10", K_WORLD_10},
	{"WORLD_11", K_WORLD_11},
	{"WORLD_12", K_WORLD_12},
	{"WORLD_13", K_WORLD_13},
	{"WORLD_14", K_WORLD_14},
	{"WORLD_15", K_WORLD_15},
	{"WORLD_16", K_WORLD_16},
	{"WORLD_17", K_WORLD_17},
	{"WORLD_18", K_WORLD_18},
	{"WORLD_19", K_WORLD_19},
	{"WORLD_20", K_WORLD_20},
	{"WORLD_21", K_WORLD_21},
	{"WORLD_22", K_WORLD_22},
	{"WORLD_23", K_WORLD_23},
	{"WORLD_24", K_WORLD_24},
	{"WORLD_25", K_WORLD_25},
	{"WORLD_26", K_WORLD_26},
	{"WORLD_27", K_WORLD_27},
	{"WORLD_28", K_WORLD_28},
	{"WORLD_29", K_WORLD_29},
	{"WORLD_30", K_WORLD_30},
	{"WORLD_31", K_WORLD_31},
	{"WORLD_32", K_WORLD_32},
	{"WORLD_33", K_WORLD_33},
	{"WORLD_34", K_WORLD_34},
	{"WORLD_35", K_WORLD_35},
	{"WORLD_36", K_WORLD_36},
	{"WORLD_37", K_WORLD_37},
	{"WORLD_38", K_WORLD_38},
	{"WORLD_39", K_WORLD_39},
	{"WORLD_40", K_WORLD_40},
	{"WORLD_41", K_WORLD_41},
	{"WORLD_42", K_WORLD_42},
	{"WORLD_43", K_WORLD_43},
	{"WORLD_44", K_WORLD_44},
	{"WORLD_45", K_WORLD_45},
	{"WORLD_46", K_WORLD_46},
	{"WORLD_47", K_WORLD_47},
	{"WORLD_48", K_WORLD_48},
	{"WORLD_49", K_WORLD_49},
	{"WORLD_50", K_WORLD_50},
	{"WORLD_51", K_WORLD_51},
	{"WORLD_52", K_WORLD_52},
	{"WORLD_53", K_WORLD_53},
	{"WORLD_54", K_WORLD_54},
	{"WORLD_55", K_WORLD_55},
	{"WORLD_56", K_WORLD_56},
	{"WORLD_57", K_WORLD_57},
	{"WORLD_58", K_WORLD_58},
	{"WORLD_59", K_WORLD_59},
	{"WORLD_60", K_WORLD_60},
	{"WORLD_61", K_WORLD_61},
	{"WORLD_62", K_WORLD_62},
	{"WORLD_63", K_WORLD_63},
	{"WORLD_64", K_WORLD_64},
	{"WORLD_65", K_WORLD_65},
	{"WORLD_66", K_WORLD_66},
	{"WORLD_67", K_WORLD_67},
	{"WORLD_68", K_WORLD_68},
	{"WORLD_69", K_WORLD_69},
	{"WORLD_70", K_WORLD_70},
	{"WORLD_71", K_WORLD_71},
	{"WORLD_72", K_WORLD_72},
	{"WORLD_73", K_WORLD_73},
	{"WORLD_74", K_WORLD_74},
	{"WORLD_75", K_WORLD_75},
	{"WORLD_76", K_WORLD_76},
	{"WORLD_77", K_WORLD_77},
	{"WORLD_78", K_WORLD_78},
	{"WORLD_79", K_WORLD_79},
	{"WORLD_80", K_WORLD_80},
	{"WORLD_81", K_WORLD_81},
	{"WORLD_82", K_WORLD_82},
	{"WORLD_83", K_WORLD_83},
	{"WORLD_84", K_WORLD_84},
	{"WORLD_85", K_WORLD_85},
	{"WORLD_86", K_WORLD_86},
	{"WORLD_87", K_WORLD_87},
	{"WORLD_88", K_WORLD_88},
	{"WORLD_89", K_WORLD_89},
	{"WORLD_90", K_WORLD_90},
	{"WORLD_91", K_WORLD_91},
	{"WORLD_92", K_WORLD_92},
	{"WORLD_93", K_WORLD_93},
	{"WORLD_94", K_WORLD_94},
	{"WORLD_95", K_WORLD_95},

	{"WINDOWS", K_SUPER},
	{"COMPOSE", K_COMPOSE},
	{"MODE", K_MODE},
	{"HELP", K_HELP},
	{"PRINT", K_PRINT},
	{"SYSREQ", K_SYSREQ},
	{"SCROLLOCK", K_SCROLLOCK },
	{"BREAK", K_BREAK},
	{"MENU", K_MENU},
	{"POWER", K_POWER},
	{"EURO", K_EURO},
	{"UNDO", K_UNDO},

	{NULL,0}
};

/*
 * Edit fields
 */

/*
 * Handles horizontal scrolling and cursor blinking
 * x, y, and width are in pixels
 */
/* FIXME: this is all shit */
void
Field_VariableSizeDraw(Field *edit, int x, int y, int width, int size,
		       qbool showCursor,
		       qbool noColorEscape)
{
	int	len, drawLen, prestep, cursorChar;
	char	str[MAX_STRING_CHARS];
	int	i;

	drawLen = edit->widthInChars - 1;	/* - 1 so there is always a space for the cursor */
	len = strlen(edit->buffer);

	/* guarantee that cursor will be visible */
	if(len <= drawLen)
		prestep = 0;
	else{
		if(edit->scroll + drawLen > len){
			edit->scroll = len - drawLen;
			if(edit->scroll < 0)
				edit->scroll = 0;
		}
		prestep = edit->scroll;
	}

	if(prestep + drawLen > len)
		drawLen = len - prestep;

	/* extract <drawLen> characters from the field at <prestep> */
	if(drawLen >= MAX_STRING_CHARS)
		comerrorf(ERR_DROP, "drawLen >= MAX_STRING_CHARS");

	Q_Memcpy(str, edit->buffer + prestep, drawLen);
	str[ drawLen ] = 0;

	/* draw it */
	if(size == SMALLCHAR_WIDTH){
		float color[4];

		color[0] = color[1] = color[2] = 1.0f;
		color[3] = 1.0;
		SCR_DrawSmallStringExt(x, y, str, color, qfalse, noColorEscape);
	}else
		/* draw big string with drop shadow */
		SCR_DrawBigString(x, y, str, 1.0, noColorEscape);

	/* draw the cursor */
	if(showCursor){
		if((int)(cls.realtime >> 7) & 1)
			return;		/* off blink */

		cursorChar = 11;	/* a block */

		i = drawLen - strlen(str);

		if(size == SMALLCHAR_WIDTH)
			SCR_DrawSmallChar(
				x + (edit->cursor - prestep - i) * size, y,
				cursorChar);
		else{
			str[0]	= cursorChar;
			str[1]	= 0;
			SCR_DrawBigString(
				x + (edit->cursor - prestep - i) * size, y, str,
				1.0,
				qfalse);

		}
	}
}

void
Field_Draw(Field *edit, int x, int y, int width, qbool showCursor,
	   qbool noColorEscape)
{
	Field_VariableSizeDraw(edit, x, y, width, SMALLCHAR_WIDTH, showCursor,
		noColorEscape);
}

void
Field_BigDraw(Field *edit, int x, int y, int width, qbool showCursor,
	      qbool noColorEscape)
{
	Field_VariableSizeDraw(edit, x, y, width, BIGCHAR_WIDTH, showCursor,
		noColorEscape);
}

void
Field_Paste(Field *edit)
{
	char	*cbd;
	int	pasteLen, i;

	cbd = sysgetclipboarddata();

	if(!cbd)
		return;

	/* send as if typed, so insert / overstrike works properly */
	pasteLen = strlen(cbd);
	for(i = 0; i < pasteLen; i++)
		Field_CharEvent(edit, cbd[i]);

	zfree(cbd);
}

/*
 * Performs the basic line editing functions for the console,
 * in-game talk, and menu fields
 *
 * Key events are used for non-printable characters, others are gotten from char events.
 */
void
Field_KeyDownEvent(Field *edit, int key)
{
	int len;

	/* shift-insert is paste */
	if(((key == K_INS) || (key == K_KP_INS)) && keys[K_SHIFT].down){
		Field_Paste(edit);
		return;
	}

	key = tolower(key);
	len = strlen(edit->buffer);

	switch(key){
	case K_DEL:
		if(edit->cursor < len)
			memmove(edit->buffer + edit->cursor,
				edit->buffer + edit->cursor + 1,
				len - edit->cursor);
		break;

	case K_RIGHTARROW:
		if(edit->cursor < len)
			edit->cursor++;
		break;

	case K_LEFTARROW:
		if(edit->cursor > 0)
			edit->cursor--;
		break;

	case K_HOME:
		edit->cursor = 0;
		break;

	case K_END:
		edit->cursor = len;
		break;

	case K_INS:
		key_overstrikeMode = !key_overstrikeMode;
		break;

	default:
		break;
	}

	/* Change scroll if cursor is no longer visible */
	if(edit->cursor < edit->scroll)
		edit->scroll = edit->cursor;
	else if(edit->cursor >= edit->scroll + edit->widthInChars &&
		edit->cursor <= len)
		edit->scroll = edit->cursor - edit->widthInChars + 1;
}

void
Field_CharEvent(Field *edit, int ch)
{
	int len;

	if(ch == 'v' - 'a' + 1){	/* ctrl-v is paste */
		Field_Paste(edit);
		return;
	}

	if(ch == 'c' - 'a' + 1){	/* ctrl-c clears the field */
		fieldclear(edit);
		return;
	}

	len = strlen(edit->buffer);

	if(ch == 'h' - 'a' + 1){	/* ctrl-h is backspace */
		if(edit->cursor > 0){
			memmove(edit->buffer + edit->cursor - 1,
				edit->buffer + edit->cursor,
				len + 1 - edit->cursor);
			edit->cursor--;
			if(edit->cursor < edit->scroll)
				edit->scroll--;
		}
		return;
	}

	if(ch == 'a' - 'a' + 1){	/* ctrl-a is home */
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if(ch == 'e' - 'a' + 1){	/* ctrl-e is end */
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars;
		return;
	}

	/*
	 * ignore any other non printable chars
	 *  */
	if(ch < 32)
		return;

	if(key_overstrikeMode){
		/* - 2 to leave room for the leading slash and trailing \0 */
		if(edit->cursor == MAX_EDIT_LINE - 2)
			return;
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}else{	/* insert mode
		 * - 2 to leave room for the leading slash and trailing \0 */
		if(len == MAX_EDIT_LINE - 2)
			return;		/* all full */
		memmove(edit->buffer + edit->cursor + 1,
			edit->buffer + edit->cursor, len + 1 - edit->cursor);
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}

	if(edit->cursor >= edit->widthInChars)
		edit->scroll++;

	if(edit->cursor == len + 1)
		edit->buffer[edit->cursor] = 0;
}

/*
 * Console line editing
 */

/* Handles history and console scrollback */
void
Console_Key(int key)
{
	char *prevline;
	
	/* ctrl-L clears screen */
	if(key == 'l' && keys[K_CTRL].down){
		cbufaddstr("clear\n");
		return;
	}

	/* enter finishes the line */
	if(key == K_ENTER || key == K_KP_ENTER){
		/* if not in the game explicitly prepend a slash if needed */
		if(clc.state != CA_ACTIVE &&
		   g_consoleField.buffer[0] &&
		   g_consoleField.buffer[0] != '\\' &&
		   g_consoleField.buffer[0] != '/'){
			char temp[MAX_EDIT_LINE-1];

			Q_strncpyz(temp, g_consoleField.buffer, sizeof(temp));
			Q_sprintf(g_consoleField.buffer,
				sizeof(g_consoleField.buffer), "/%s", temp);
			g_consoleField.cursor++;
		}

		comprintf("))) %s\n", g_consoleField.buffer);

		/* leading slash is an explicit command */
		if(g_consoleField.buffer[0] == '\\' ||
		   g_consoleField.buffer[0] == '/'){
			cbufaddstr(g_consoleField.buffer+1);	/* valid command */
			cbufaddstr ("\n");
		}else{
			/* other text will be chat messages */
			if(!g_consoleField.buffer[0])
				return;		/* empty lines just scroll the console without adding to history */
			else{
				cbufaddstr ("cmd say ");
				cbufaddstr(g_consoleField.buffer);
				cbufaddstr ("\n");
			}
		}

		/* 
		 * copy line to history buffer if it is different 
		 * to the previous line 
		 */
		prevline = historyEditLines[(nextHistoryLine-1) % COMMAND_HISTORY].buffer;
		if(strcmp(prevline, g_consoleField.buffer) != 0){
			historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
			nextHistoryLine++;
		}
		historyLine = nextHistoryLine;
		fieldclear(&g_consoleField);
		g_consoleField.widthInChars = g_console_field_width;
		CL_SaveConsoleHistory();

		if(clc.state == CA_DISCONNECTED)
			/* force an update, because the command may take some time */
			SCR_UpdateScreen();
		return;
	}

	/*
	 * command completion
	 */
	if(key == K_TAB){
		fieldautocomplete(&g_consoleField);
		return;
	}

	/*
	 * command history (ctrl-p ctrl-n for unix style)
	 */
	if((key == K_MWHEELUP &&
	    keys[K_SHIFT].down) || (key == K_UPARROW) ||
	   (key == K_KP_UPARROW) ||
	   ((tolower(key) == 'p') && keys[K_CTRL].down)){
		if(nextHistoryLine - historyLine < COMMAND_HISTORY
		   && historyLine > 0)
			historyLine--;
		g_consoleField =
			historyEditLines[ historyLine % COMMAND_HISTORY ];
		return;
	}

	if((key == K_MWHEELDOWN &&
	    keys[K_SHIFT].down) || (key == K_DOWNARROW) ||
	   (key == K_KP_DOWNARROW) ||
	   ((tolower(key) == 'n') && keys[K_CTRL].down)){
		historyLine++;
		if(historyLine >= nextHistoryLine){
			historyLine = nextHistoryLine;
			fieldclear(&g_consoleField);
			g_consoleField.widthInChars = g_console_field_width;
			return;
		}
		g_consoleField =
			historyEditLines[ historyLine % COMMAND_HISTORY ];
		return;
	}

	/*
	 * console scrolling
	 */
	if(key == K_PGUP){
		/* hold shift to accelerate scrolling */
		if(keys[K_SHIFT].down){
			Con_PageUp();
			Con_PageUp();
		}
		Con_PageUp();
		return;
	}

	if(key == K_PGDN){
		if(keys[K_SHIFT].down){
			Con_PageDown();
			Con_PageDown();
		}
		Con_PageDown();
		return;
	}

	if(key == K_MWHEELUP){
		Con_PageUp();
		/* hold ctrl to accelerate scrolling */
		if(keys[K_CTRL].down){
			Con_PageUp();
			Con_PageUp();
		}
		return;
	}

	if(key == K_MWHEELDOWN){
		Con_PageDown();
		if(keys[K_CTRL].down){
			Con_PageDown();
			Con_PageDown();
		}
		return;
	}

	/* shift-home = top of console */
	if(key == K_HOME && keys[K_SHIFT].down){
		Con_Top();
		return;
	}

	/* shift-end = bottom of console */
	if(key == K_END && keys[K_SHIFT].down){
		Con_Bottom();
		return;
	}

	/* pass to the normal editline routine */
	Field_KeyDownEvent(&g_consoleField, key);
}

/*
 * In game talk message
 */
void
Message_Key(int key)
{

	char buffer[MAX_STRING_CHARS];


	if(key == K_ESCAPE){
		Key_SetCatcher(Key_GetCatcher( ) & ~KEYCATCH_MESSAGE);
		fieldclear(&chatField);
		return;
	}

	if(key == K_ENTER || key == K_KP_ENTER){
		if(chatField.buffer[0] && clc.state == CA_ACTIVE){
			if(chat_playerNum != -1)

				Q_sprintf(buffer, sizeof(buffer),
					"tell %i \"%s\"\n", chat_playerNum,
					chatField.buffer);

			else if(chat_team)

				Q_sprintf(buffer, sizeof(buffer),
					"say_team \"%s\"\n",
					chatField.buffer);
			else
				Q_sprintf(buffer, sizeof(buffer),
					"say \"%s\"\n",
					chatField.buffer);



			CL_AddReliableCommand(buffer, qfalse);
		}
		Key_SetCatcher(Key_GetCatcher( ) & ~KEYCATCH_MESSAGE);
		fieldclear(&chatField);
		return;
	}

	Field_KeyDownEvent(&chatField, key);
}

qbool
Key_GetOverstrikeMode(void)
{
	return key_overstrikeMode;
}


void
Key_SetOverstrikeMode(qbool state)
{
	key_overstrikeMode = state;
}

qbool
Key_IsDown(int keynum)
{
	if(keynum < 0 || keynum >= MAX_KEYS)
		return qfalse;

	return keys[keynum].down;
}

/*
 * Returns a key number to be used to index keys[] by looking at
 * the given string.  Single ascii characters return themselves, while
 * the K_* names are matched up.
 *
 * 0x11 will be interpreted as raw hex, which will allow new controllers
 *
 * to be configured even if they don't have defined names.
 */
int
Key_StringToKeynum(char *str)
{
	keyname_t *kn;

	if(!str || !str[0])
		return -1;
	if(!str[1])
		return tolower(str[0]);

	/* check for hex code */
	if(strlen(str) == 4){
		int n = Q_hexstr2int(str);

		if(n >= 0)
			return n;
	}

	/* scan for a text match */
	for(kn=keynames; kn->name; kn++)
		if(!Q_stricmp(str,kn->name))
			return kn->keynum;
	return -1;
}

/*
 * Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
 * given keynum.
 */
char *
Key_KeynumToString(int keynum)
{
	keyname_t *kn;
	static char tinystr[5];
	int i, j;

	if(keynum == -1)
		return "<KEY NOT FOUND>";

	if(keynum < 0 || keynum >= MAX_KEYS)
		return "<OUT OF RANGE>";

	/* check for printable ascii (don't use quote) */
	if(keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';'){
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	/* check for a key string */
	for(kn=keynames; kn->name; kn++)
		if(keynum == kn->keynum)
			return kn->name;

	/* make a hex string */
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;
	return tinystr;
}

void
Key_SetBinding(int keynum, const char *binding)
{
	if(keynum < 0 || keynum >= MAX_KEYS)
		return;

	/* free old bindings */
	if(keys[ keynum ].binding)
		zfree(keys[ keynum ].binding);

	/* allocate memory for new binding */
	keys[keynum].binding = copystr(binding);

	/* consider this like modifying an archived cvar, so the
	* file write will be triggered at the next oportunity */
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

char *
Key_GetBinding(int keynum)
{
	if(keynum < 0 || keynum >= MAX_KEYS)
		return "";

	return keys[ keynum ].binding;
}

int
Key_GetKey(const char *binding)
{
	int i;

	if(binding)
		for(i=0; i < MAX_KEYS; i++)
			if(keys[i].binding &&
			   Q_stricmp(binding, keys[i].binding) == 0)
				return i;
	return -1;
}

void
Key_Unbind_f(void)
{
	int b;

	if(cmdargc() != 2){
		comprintf ("unbind <key> : remove commands from a key\n");
		return;
	}
	b = Key_StringToKeynum (cmdargv(1));
	if(b==-1){
		comprintf ("\"%s\" isn't a valid key\n", cmdargv(1));
		return;
	}
	Key_SetBinding (b, "");
}

void
Key_Unbindall_f(void)
{
	int i;

	for(i=0; i < MAX_KEYS; i++)
		if(keys[i].binding)
			Key_SetBinding (i, "");
}

void
Key_Bind_f(void)
{
	int	i, c, b;
	char	cmd[1024];

	c = cmdargc();

	if(c < 2){
		comprintf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (cmdargv(1));
	if(b==-1){
		comprintf ("\"%s\" isn't a valid key\n", cmdargv(1));
		return;
	}

	if(c == 2){
		if(keys[b].binding)
			comprintf ("\"%s\" = \"%s\"\n", cmdargv(
					1), keys[b].binding);
		else
			comprintf ("\"%s\" is not bound\n", cmdargv(1));
		return;
	}

/* copy the rest of the command line */
	cmd[0] = 0;	/* start out with a null string */
	for(i=2; i< c; i++){
		strcat (cmd, cmdargv(i));
		if(i != (c-1))
			strcat (cmd, " ");
	}
	Key_SetBinding (b, cmd);
}

/*
 * Writes lines containing "bind key value"
 */
void
keywritebindings(Fhandle f)
{
	int i;

	fsprintf (f, "unbindall\n");

	for(i=0; i<MAX_KEYS; i++)
		if(keys[i].binding && keys[i].binding[0])
			fsprintf (f, "bind %s \"%s\"\n", Key_KeynumToString(
					i), keys[i].binding);
}

void
Key_Bindlist_f(void)
{
	int i;

	for(i = 0; i < MAX_KEYS; i++)
		if(keys[i].binding && keys[i].binding[0])
			comprintf("%s \"%s\"\n", Key_KeynumToString(
					i), keys[i].binding);
}

void
keykeynamecompletion(void (*callback)(const char *s))
{
	int i;

	for(i = 0; keynames[ i ].name != NULL; i++)
		callback(keynames[ i ].name);
}

static void
Key_CompleteUnbind(char *args, int argNum)
{
	if(argNum == 2){
		/* Skip "unbind " */
		char *p = Q_skiptoks(args, 1, " ");

		if(p > args)
			fieldcompletekeyname( );
	}
}

static void
Key_CompleteBind(char *args, int argNum)
{
	char *p;

	if(argNum == 2){
		/* Skip "bind " */
		p = Q_skiptoks(args, 1, " ");

		if(p > args)
			fieldcompletekeyname( );
	}else if(argNum >= 3){
		/* Skip "bind <key> " */
		p = Q_skiptoks(args, 2, " ");

		if(p > args)
			fieldcompletecmd(p, qtrue, qtrue);
	}
}

void
clinitkeycmds(void)
{
	/* register our functions */
	cmdadd ("bind",Key_Bind_f);
	cmdsetcompletion("bind", Key_CompleteBind);
	cmdadd ("unbind",Key_Unbind_f);
	cmdsetcompletion("unbind", Key_CompleteUnbind);
	cmdadd ("unbindall",Key_Unbindall_f);
	cmdadd ("bindlist",Key_Bindlist_f);
}

/*
 * Execute the commands in the bind string
 */
void
CL_ParseBinding(int key, qbool down, unsigned time)
{
	char buf[ MAX_STRING_CHARS ], *p = buf, *end;

	if(!keys[key].binding || !keys[key].binding[0])
		return;
	Q_strncpyz(buf, keys[key].binding, sizeof(buf));

	while(1){
		while(isspace(*p))
			p++;
		end = strchr(p, ';');
		if(end)
			*end = '\0';
		if(*p == '+'){
			/* button commands add keynum and time as parameters
			 * so that multiple sources can be discriminated and
			 * subframe corrected */
			char cmd[1024];
			Q_sprintf(cmd, sizeof(cmd), "%c%s %d %d\n",
				(down) ? '+' : '-', p + 1, key, time);
			cbufaddstr(cmd);
		}else if(down){
			/* normal commands only execute on key press */
			cbufaddstr(p);
			cbufaddstr("\n");
		}
		if(!end)
			break;
		p = end + 1;
	}
}

/*
 * Called by clkeyevent to handle a keypress
 */
void
CL_KeyDownEvent(int key, unsigned time)
{
	keys[key].down = qtrue;
	keys[key].repeats++;
	if(keys[key].repeats == 1 && key != K_SCROLLOCK && key !=
	   K_KP_NUMLOCK)
		anykeydown++;

	if(keys[K_ALT].down && key == K_ENTER){
		cvarsetf("r_fullscreen",
			!cvargeti("r_fullscreen"));
		return;
	}

	/* console key is hardcoded, so the user can never unbind it */
	if(key == K_CONSOLE || (keys[K_SHIFT].down && key == K_ESCAPE)){
		Con_ToggleConsole_f ();
		Key_ClearStates ();
		return;
	}


	/* keys can still be used for bound actions */
	if((key < 128 || key == K_MOUSE1) &&
	   (clc.demoplaying ||
	    clc.state == CA_CINEMATIC) && Key_GetCatcher( ) == 0)

		if(cvargetf ("com_cameraMode") == 0){
			cvarsetstr ("nextdemo","");
			key = K_ESCAPE;
		}

	/* escape is always handled special */
	if(key == K_ESCAPE){
		if(Key_GetCatcher( ) & KEYCATCH_MESSAGE){
			/* clear message mode */
			Message_Key(key);
			return;
		}

		/* escape always gets out of CGAME stuff */
		if(Key_GetCatcher( ) & KEYCATCH_CGAME){
			Key_SetCatcher(Key_GetCatcher( ) & ~KEYCATCH_CGAME);
			vmcall (cgvm, CG_EVENT_HANDLING, CGAME_EVENT_NONE);
			return;
		}

		if(!(Key_GetCatcher( ) & KEYCATCH_UI)){
			if(clc.state == CA_ACTIVE && !clc.demoplaying)
				vmcall(uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME);
			else if(clc.state != CA_DISCONNECTED){
				cldisconnect_f();
				sndstopall();
				vmcall(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			}
			return;
		}

		vmcall(uivm, UI_KEY_EVENT, key, qtrue);
		return;
	}

	/* distribute the key down event to the apropriate handler */
	if(Key_GetCatcher( ) & KEYCATCH_CONSOLE)
		Console_Key(key);
	else if(Key_GetCatcher( ) & KEYCATCH_UI){
		if(uivm)
			vmcall(uivm, UI_KEY_EVENT, key, qtrue);
	}else if(Key_GetCatcher( ) & KEYCATCH_CGAME){
		if(cgvm)
			vmcall(cgvm, CG_KEY_EVENT, key, qtrue);
	}else if(Key_GetCatcher( ) & KEYCATCH_MESSAGE)
		Message_Key(key);
	else if(clc.state == CA_DISCONNECTED)
		Console_Key(key);
	else
		/* send the bound action */
		CL_ParseBinding(key, qtrue, time);
	return;
}

/*
 * Called by clkeyevent to handle a keyrelease
 */
void
CL_KeyUpEvent(int key, unsigned time)
{
	keys[key].repeats = 0;
	keys[key].down = qfalse;
	if(key != K_SCROLLOCK && key != K_KP_NUMLOCK)
		anykeydown--;

	if(anykeydown < 0)
		anykeydown = 0;

	/* don't process key-up events for the console key */
	if(key == K_CONSOLE || (key == K_ESCAPE && keys[K_SHIFT].down))
		return;

	/*
	 * key up events only perform actions if the game key binding is
	 * a button command (leading + sign).  These will be processed even in
	 * console mode and menu mode, to keep the character from continuing
	 * an action started before a mode switch.
	 *  */
	if(clc.state != CA_DISCONNECTED)
		CL_ParseBinding(key, qfalse, time);

	if(Key_GetCatcher( ) & KEYCATCH_UI && uivm)
		vmcall(uivm, UI_KEY_EVENT, key, qfalse);
	else if(Key_GetCatcher( ) & KEYCATCH_CGAME && cgvm)
		vmcall(cgvm, CG_KEY_EVENT, key, qfalse);
}

/*
 * Called by the system for both key up and key down events
 */
void
clkeyevent(int key, qbool down, unsigned time)
{
	if(down)
		CL_KeyDownEvent(key, time);
	else
		CL_KeyUpEvent(key, time);
}

/*
 * Normal keyboard characters, already shifted / capslocked / etc
 */
void
clcharevent(int key)
{
	/* delete is not a printable character and is
	 * otherwise handled by Field_KeyDownEvent */
	if(key == 127)
		return;

	/* distribute the key down event to the apropriate handler */
	if(Key_GetCatcher( ) & KEYCATCH_CONSOLE)
		Field_CharEvent(&g_consoleField, key);
	else if(Key_GetCatcher( ) & KEYCATCH_UI)
		vmcall(uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, qtrue);
	else if(Key_GetCatcher( ) & KEYCATCH_MESSAGE)
		Field_CharEvent(&chatField, key);
	else if(clc.state == CA_DISCONNECTED)
		Field_CharEvent(&g_consoleField, key);
}

void
Key_ClearStates(void)
{
	int i;

	anykeydown = 0;

	for(i=0; i < MAX_KEYS; i++){
		if(i == K_SCROLLOCK || i == K_KP_NUMLOCK)
			continue;

		if(keys[i].down)
			clkeyevent(i, qfalse, 0);

		keys[i].down = 0;
		keys[i].repeats = 0;
	}
}

int
Key_GetCatcher(void)
{
	return keyCatchers;
}

void
Key_SetCatcher(int catcher)
{
	/* If the catcher state is changing, clear all key states */
	if(catcher != keyCatchers)
		Key_ClearStates( );

	keyCatchers = catcher;
}

/*
 * Load the console history from cl_consoleHistory
 */
void
CL_LoadConsoleHistory(void)
{
	char	*token, *text_p;
	int	i, numChars, numLines = 0;
	Fhandle f;

	consoleSaveBufferSize = fsopenr(CONSOLE_HISTORY_FILE, &f,
		qfalse);
	if(!f){
		comprintf("Couldn't read %s.\n", CONSOLE_HISTORY_FILE);
		return;
	}

	if(consoleSaveBufferSize <= MAX_CONSOLE_SAVE_BUFFER &&
	   fsread(consoleSaveBuffer, consoleSaveBufferSize,
		   f) == consoleSaveBufferSize){
		text_p = consoleSaveBuffer;

		for(i = COMMAND_HISTORY - 1; i >= 0; i--){
			if(!*(token = Q_readtok(&text_p)))
				break;

			historyEditLines[ i ].cursor = atoi(token);

			if(!*(token = Q_readtok(&text_p)))
				break;

			historyEditLines[ i ].scroll = atoi(token);

			if(!*(token = Q_readtok(&text_p)))
				break;

			numChars = atoi(token);
			text_p++;
			if(numChars >
			   (strlen(consoleSaveBuffer) -
			    (text_p - consoleSaveBuffer))){
				comdprintf(
					S_COLOR_YELLOW
					"WARNING: probable corrupt history\n");
				break;
			}
			Q_Memcpy(historyEditLines[ i ].buffer,
				text_p, numChars);
			historyEditLines[ i ].buffer[ numChars ] = '\0';
			text_p += numChars;

			numLines++;
		}

		memmove(&historyEditLines[ 0 ], &historyEditLines[ i + 1 ],
			numLines * sizeof(Field));
		for(i = numLines; i < COMMAND_HISTORY; i++)
			fieldclear(&historyEditLines[ i ]);

		historyLine = nextHistoryLine = numLines;
	}else
		comprintf("Couldn't read %s.\n", CONSOLE_HISTORY_FILE);

	fsclose(f);
}

/*
 * Save the console history into the cvar cl_consoleHistory
 * so that it persists across invocations of q3
 */
void
CL_SaveConsoleHistory(void)
{
	int	i;
	int	lineLength, saveBufferLength, additionalLength;
	Fhandle f;

	consoleSaveBuffer[ 0 ] = '\0';

	i = (nextHistoryLine - 1) % COMMAND_HISTORY;
	do {
		if(historyEditLines[ i ].buffer[ 0 ]){
			lineLength = strlen(historyEditLines[ i ].buffer);
			saveBufferLength = strlen(consoleSaveBuffer);

			/* ICK */
			additionalLength = lineLength + strlen("999 999 999  ");

			if(saveBufferLength + additionalLength <
			   MAX_CONSOLE_SAVE_BUFFER)
				Q_strcat(consoleSaveBuffer,
					MAX_CONSOLE_SAVE_BUFFER,
					va("%d %d %d %s\n",
						historyEditLines[ i ].cursor,
						historyEditLines[ i ].scroll,
						lineLength,
						historyEditLines[ i ].buffer));
			else
				break;
		}
		i = (i - 1 + COMMAND_HISTORY) % COMMAND_HISTORY;
	} while(i != (nextHistoryLine - 1) % COMMAND_HISTORY);

	consoleSaveBufferSize = strlen(consoleSaveBuffer);

	f = fsopenw(CONSOLE_HISTORY_FILE);
	if(!f){
		comprintf("Couldn't write %s.\n", CONSOLE_HISTORY_FILE);
		return;
	}

	if(fswrite(consoleSaveBuffer, consoleSaveBufferSize,
		   f) < consoleSaveBufferSize)
		comprintf("Couldn't write %s.\n", CONSOLE_HISTORY_FILE);
	fsclose(f);
}
