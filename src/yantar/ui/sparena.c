/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "ui.h"
#include "local.h"

void
UI_SPArena_Start(const char *arenaInfo)
{
	char	*map;
	int	level;
	int	n;
	char	*txt;

	n = (int)trap_cvargetf("sv_maxclients");
	if(n < 8)
		trap_cvarsetf("sv_maxclients", 8);

	level	= atoi(Info_ValueForKey(arenaInfo, "num"));
	txt	= Info_ValueForKey(arenaInfo, "special");
	if(txt[0]){
		if(Q_stricmp(txt, "training") == 0)
			level = -4;
		else if(Q_stricmp(txt, "final") == 0)
			level = UI_GetNumSPTiers() * ARENAS_PER_TIER;
	}
	trap_cvarsetf("ui_spSelection", level);

	map = Info_ValueForKey(arenaInfo, "map");
	trap_Cmd_ExecuteText(EXEC_APPEND, va("spmap %s\n", map));
}
