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
clshutdown(char *finalmsg, qbool disconnect, qbool quit)
{
}

void
clinit(void)
{
	cl_shownet = cvarget ("cl_shownet", "0", CVAR_TEMP);
}

void
clmouseevent(int dx, int dy, int time)
{
}

void
keywritebindings(Fhandle f)
{
}

void
clframe(int msec)
{
}

void
clpacketevent(Netaddr from, Bitmsg *msg)
{
}

void
clcharevent(int key)
{
}

void
cldisconnect(qbool showMainMenu)
{
}

void
clmaploading(void)
{
}

qbool
clgamecmd(void)
{
	return qfalse;
}

void
clkeyevent(int key, qbool down, unsigned time)
{
}

qbool
UI_GameCommand(void)
{
	return qfalse;
}

void
clforwardcmdtoserver(const char *string)
{
}

void
clconsoleprint(char *txt)
{
}

void
cljoystickevent(int axis, int value, int time)
{
}

void
clinitkeycmds(void)
{
}

void
clflushmem(void)
{
}

void
clshutdownall(qbool shutdownRef)
{
}

void
clstarthunkusers(qbool rendererOnly)
{
}

void
clinitref(void)
{
}

void
clsndshutdown(void)
{
}
