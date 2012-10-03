/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#define MAX_NODESWITCHES 50

void	AIEnter_Intermission(bot_state_t *bs, char *s);
void	AIEnter_Observer(bot_state_t *bs, char *s);
void	AIEnter_Respawn(bot_state_t *bs, char *s);
void	AIEnter_Stand(bot_state_t *bs, char *s);
void	AIEnter_Seek_ActivateEntity(bot_state_t *bs, char *s);
void	AIEnter_Seek_NBG(bot_state_t *bs, char *s);
void	AIEnter_Seek_LTG(bot_state_t *bs, char *s);
void	AIEnter_Seek_Camp(bot_state_t *bs, char *s);
void	AIEnter_Battle_Fight(bot_state_t *bs, char *s);
void	AIEnter_Battle_Chase(bot_state_t *bs, char *s);
void	AIEnter_Battle_Retreat(bot_state_t *bs, char *s);
void	AIEnter_Battle_NBG(bot_state_t *bs, char *s);
int	AINode_Intermission(bot_state_t *bs);
int	AINode_Observer(bot_state_t *bs);
int	AINode_Respawn(bot_state_t *bs);
int	AINode_Stand(bot_state_t *bs);
int	AINode_Seek_ActivateEntity(bot_state_t *bs);
int	AINode_Seek_NBG(bot_state_t *bs);
int	AINode_Seek_LTG(bot_state_t *bs);
int	AINode_Battle_Fight(bot_state_t *bs);
int	AINode_Battle_Chase(bot_state_t *bs);
int	AINode_Battle_Retreat(bot_state_t *bs);
int	AINode_Battle_NBG(bot_state_t *bs);
void	BotResetNodeSwitches(void);
void	BotDumpNodeSwitches(bot_state_t *bs);
