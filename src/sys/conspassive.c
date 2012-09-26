/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "local.h"

#include <stdio.h>

/*
 * CON_Shutdown
 */
void
CON_Shutdown(void)
{
}

/*
 * CON_Init
 */
void
CON_Init(void)
{
}

/*
 * CON_Input
 */
char *
CON_Input(void)
{
	return NULL;
}

/*
 * CON_Print
 */
void
CON_Print(const char *msg)
{
	if(com_ansiColor && com_ansiColor->integer)
		Sys_AnsiColorPrint(msg);
	else
		fputs(msg, stderr);
}
