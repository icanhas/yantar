/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"

Cvar *cl_shownet;

void
CL_Shutdown(char *finalmsg, qbool disconnect, qbool quit)
{
}

void
CL_Init(void)
{
	cl_shownet = cvarget ("cl_shownet", "0", CVAR_TEMP);
}

void
CL_MouseEvent(int dx, int dy, int time)
{
}

void
Key_WriteBindings(Fhandle f)
{
}

void
CL_Frame(int msec)
{
}

void
CL_PacketEvent(Netaddr from, Bitmsg *msg)
{
}

void
CL_CharEvent(int key)
{
}

void
CL_Disconnect(qbool showMainMenu)
{
}

void
CL_MapLoading(void)
{
}

qbool
CL_GameCommand(void)
{
	return qfalse;
}

void
CL_KeyEvent(int key, qbool down, unsigned time)
{
}

qbool
UI_GameCommand(void)
{
	return qfalse;
}

void
CL_ForwardCommandToServer(const char *string)
{
}

void
CL_ConsolePrint(char *txt)
{
}

void
CL_JoystickEvent(int axis, int value, int time)
{
}

void
CL_InitKeyCommands(void)
{
}

void
CL_FlushMemory(void)
{
}

void
CL_ShutdownAll(qbool shutdownRef)
{
}

void
CL_StartHunkUsers(qbool rendererOnly)
{
}

void
CL_InitRef(void)
{
}

void
CL_Snd_Shutdown(void)
{
}
