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
NET_StringToAdr(char *s, Netaddr *a)
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
 * syssendpacket
 * ==================
 */
void
syssendpacket(int length, void *data, Netaddr to)
{
}
