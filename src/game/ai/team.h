/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

void	BotTeamAI(bot_state_t *bs);
int	BotGetTeamMateTaskPreference(bot_state_t *bs, int teammate);
void	BotSetTeamMateTaskPreference(bot_state_t *bs, int teammate, int preference);
void	BotVoiceChat(bot_state_t *bs, int toclient, char *voicechat);
void	BotVoiceChatOnly(bot_state_t *bs, int toclient, char *voicechat);

