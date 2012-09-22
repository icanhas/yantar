/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*  */

/*****************************************************************************
* name:		cmd.h
*
* desc:		Quake3 bot AI
*
* $Archive: /source/code/botai/ai_chat.c $
*
*****************************************************************************/

extern int notleader[MAX_CLIENTS];

int BotMatchMessage(bot_state_t *bs, char *message);
void BotPrintTeamGoal(bot_state_t *bs);
