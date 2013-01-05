/* "keycodes.h" and "client.h" must be included before me */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
typedef struct qkey_t qkey_t;

struct qkey_t {
	qbool		down;
	int		repeats;	/* if > 1, it is autorepeating */
	char		*binding;
};

enum { COMMAND_HISTORY = 32 };
extern field_t	historyEditLines[COMMAND_HISTORY];
extern field_t	g_consoleField;
extern field_t	chatField;
extern int		anykeydown;
extern qbool	chat_team;
extern int		chat_playerNum;
extern qbool	key_overstrikeMode;
extern qkey_t	keys[MAX_KEYS];

/* NOTE TTimo the declaration of field_t and Field_Clear is now in qcommon/qcommon.h */
/* well done ttimo! go on my son! */
void		Field_KeyDownEvent(field_t *edit, int key);
void		Field_CharEvent(field_t *edit, int ch);
void		Field_Draw(field_t *edit, int x, int y, int width, qbool showCursor,
			qbool noColorEscape);
void		Field_BigDraw(field_t *edit, int x, int y, int width, qbool showCursor,
			qbool noColorEscape);
void		Key_WriteBindings(Fhandle f);
void		Key_SetBinding(int keynum, const char *binding);
char*	Key_GetBinding(int keynum);
qbool	Key_IsDown(int keynum);
qbool	Key_GetOverstrikeMode(void);
void		Key_SetOverstrikeMode(qbool state);
void		Key_ClearStates(void);
int		Key_GetKey(const char *binding);
