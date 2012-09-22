/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*  */

/*****************************************************************************
* name:		vcmd.h
*
* desc:		Quake3 bot AI
*
* $Archive: /source/code/botai/ai_vcmd.c $
*
*****************************************************************************/

int BotVoiceChatCommand(bot_state_t *bs, int mode, char *voicechat);
void BotVoiceChat_Defend(bot_state_t *bs, int client, int mode);
