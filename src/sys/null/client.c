/*
 * ===========================================================================
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 * ===========================================================================
 */

#include "q_shared.h"
#include "qcommon.h"

cvar_t *cl_shownet;

void
CL_Shutdown(char *finalmsg, qbool disconnect, qbool quit)
{
}

void
CL_Init(void)
{
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP);
}

void
CL_MouseEvent(int dx, int dy, int time)
{
}

void
Key_WriteBindings(fileHandle_t f)
{
}

void
CL_Frame(int msec)
{
}

void
CL_PacketEvent(netadr_t from, msg_t *msg)
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
CL_CDDialog(void)
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

qbool
CL_CDKeyValidate(const char *key, const char *checksum)
{
	return qtrue;
}
