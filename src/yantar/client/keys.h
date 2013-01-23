/* "keycodes.h" and "client.h" must be included before me */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
typedef struct Qkey Qkey;

struct Qkey {
	qbool		down;
	int		repeats;	/* if > 1, it is autorepeating */
	char		*binding;
};

enum { COMMAND_HISTORY = 32 };
extern Field	historyEditLines[COMMAND_HISTORY];
extern Field	g_consoleField;
extern Field	chatField;
extern int		anykeydown;
extern qbool	chat_team;
extern int		chat_playerNum;
extern qbool	key_overstrikeMode;
extern Qkey	keys[MAX_KEYS];

/* NOTE TTimo the declaration of Field and fieldclear is now in qcommon/qcommon.h */
/* well done ttimo! go on my son! */
void		Field_KeyDownEvent(Field *edit, int key);
void		Field_CharEvent(Field *edit, int ch);
void		Field_Draw(Field *edit, int x, int y, int width, qbool showCursor,
			qbool noColorEscape);
void		Field_BigDraw(Field *edit, int x, int y, int width, qbool showCursor,
			qbool noColorEscape);
void		keywritebindings(Fhandle f);
void		Key_SetBinding(int keynum, const char *binding);
char*	Key_GetBinding(int keynum);
qbool	Key_IsDown(int keynum);
qbool	Key_GetOverstrikeMode(void);
void		Key_SetOverstrikeMode(qbool state);
void		Key_ClearStates(void);
int		Key_GetKey(const char *binding);
