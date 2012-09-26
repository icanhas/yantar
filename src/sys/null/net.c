/*
 * ===========================================================================
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 * ===========================================================================
 */

#include "common.h"

/*
 * =============
 * NET_StringToAdr
 *
 * localhost
 * idnewt
 * idnewt:28000
 * 192.246.40.70
 * 192.246.40.70:28000
 * =============
 */
qbool
NET_StringToAdr(char *s, netadr_t *a)
{
	if(!strcmp (s, "localhost")){
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	return false;
}

/*
 * ==================
 * Sys_SendPacket
 * ==================
 */
void
Sys_SendPacket(int length, void *data, netadr_t to)
{
}
