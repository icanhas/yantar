/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 * elementary actions
 */

#include "shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_ea.h"

#define MAX_USERMOVE		400
#define MAX_COMMANDARGUMENTS	10

bot_input_t *botinputs;

void
EA_Say(int client, char *str)
{
	botimport.BotClientCommand(client, va("say %s", str));
}

void
EA_SayTeam(int client, char *str)
{
	botimport.BotClientCommand(client, va("say_team %s", str));
}

void
EA_Tell(int client, int clientto, char *str)
{
	botimport.BotClientCommand(client, va("tell %d, %s", clientto, str));
}

void
EA_UseItem(int client, char *it)
{
	botimport.BotClientCommand(client, va("use %s", it));
}

void
EA_DropItem(int client, char *it)
{
	botimport.BotClientCommand(client, va("drop %s", it));
}

void
EA_UseInv(int client, char *inv)
{
	botimport.BotClientCommand(client, va("invuse %s", inv));
}

void
EA_DropInv(int client, char *inv)
{
	botimport.BotClientCommand(client, va("invdrop %s", inv));
}

void
EA_Gesture(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_GESTURE;
}

void
EA_Command(int client, char *command)
{
	botimport.BotClientCommand(client, command);
}

void
EA_SelectWeapon(int client, int weapon)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->weapon = weapon;
}

void
EA_Attack(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_ATTACK;
}

void
EA_Talk(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_TALK;
}

void
EA_Use(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_USE;
}

void
EA_Respawn(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_RESPAWN;
}

void
EA_Jump(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	if(bi->actionflags & ACTION_JUMPEDLASTFRAME)
		bi->actionflags &= ~ACTION_JUMP;
	else
		bi->actionflags |= ACTION_JUMP;
}

void
EA_DelayedJump(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	if(bi->actionflags & ACTION_JUMPEDLASTFRAME)
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	else
		bi->actionflags |= ACTION_DELAYEDJUMP;
}

void
EA_Crouch(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_CROUCH;
}

void
EA_Walk(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_WALK;
}

void
EA_Action(int client, int action)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= action;
}

void
EA_MoveUp(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEUP;
}

void
EA_MoveDown(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEDOWN;
}

void
EA_MoveForward(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEFORWARD;
}

void
EA_MoveBack(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEBACK;
}

void
EA_MoveLeft(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVELEFT;
}

void
EA_MoveRight(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVERIGHT;
}

void
EA_Move(int client, Vec3 dir, float speed)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	copyv3(dir, bi->dir);
	/* cap speed */
	if(speed > MAX_USERMOVE) speed = MAX_USERMOVE;
	else if(speed < -MAX_USERMOVE) speed = -MAX_USERMOVE;
	bi->speed = speed;
}

void
EA_View(int client, Vec3 viewangles)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	copyv3(viewangles, bi->viewangles);
}

void
EA_EndRegular(int client, float thinktime)
{
}

void
EA_GetInput(int client, float thinktime, bot_input_t *input)
{
	bot_input_t *bi;

	bi = &botinputs[client];
	bi->thinktime = thinktime;
	Q_Memcpy(input, bi, sizeof(bot_input_t));
}

void
EA_ResetInput(int client)
{
	bot_input_t *bi;
	int jumped = qfalse;

	bi = &botinputs[client];

	bi->thinktime = 0;
	clearv3(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if(jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
}

int
EA_Setup(void)
{
	/* initialize the bot inputs */
	botinputs = (bot_input_t*)GetClearedHunkMemory(
		botlibglobals.maxclients * sizeof(bot_input_t));
	return BLERR_NOERROR;
}

void
EA_Shutdown(void)
{
	FreeMemory(botinputs);
	botinputs = NULL;
}
