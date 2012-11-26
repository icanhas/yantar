/* common function replacements for modular renderer */
/*
 * Copyright (C) 2010 James Canete (use.less01@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char	text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

void QDECL
Com_Errorf(int level, const char *error, ...)
{
	va_list argptr;
	char	text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}
