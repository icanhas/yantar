/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 *
 * USER INTERFACE MAIN
 *
 */


#include "shared.h"
#include "ui.h"
#include "local.h"


/*
 * vmMain
 *
 * This is the only way control passes into the module.
 * This must be the very first function compiled into the .qvm file
 */
Q_EXPORT intptr_t
vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
       int arg6, int arg7, int arg8, int arg9, int arg10,
       int arg11)
{
	switch(command){
	case UI_GETAPIVERSION:
		return UI_API_VERSION;

	case UI_INIT:
		UI_Init();
		return 0;

	case UI_SHUTDOWN:
		UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		UI_KeyEvent(arg0, arg1);
		return 0;

	case UI_MOUSE_EVENT:
		UI_MouseEvent(arg0, arg1);
		return 0;

	case UI_REFRESH:
		UI_Refresh(arg0);
		return 0;

	case UI_IS_FULLSCREEN:
		return UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		UI_SetActiveMenu(arg0);
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand(arg0);

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen(arg0);
		return 0;
	}

	return -1;
}


/*
 * cvars
 */

typedef struct {
	Vmcvar	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int		cvarFlags;
} cvarTable_t;

Vmcvar	ui_ffa_fraglimit;
Vmcvar	ui_ffa_timelimit;

Vmcvar	ui_tourney_fraglimit;
Vmcvar	ui_tourney_timelimit;

Vmcvar	ui_team_fraglimit;
Vmcvar	ui_team_timelimit;
Vmcvar	ui_team_friendly;

Vmcvar	ui_ctf_capturelimit;
Vmcvar	ui_ctf_timelimit;
Vmcvar	ui_ctf_friendly;

Vmcvar	ui_arenasFile;
Vmcvar	ui_botsFile;
Vmcvar	ui_spScores1;
Vmcvar	ui_spScores2;
Vmcvar	ui_spScores3;
Vmcvar	ui_spScores4;
Vmcvar	ui_spScores5;
Vmcvar	ui_spAwards;
Vmcvar	ui_spVideos;
Vmcvar	ui_spSkill;

Vmcvar	ui_spSelection;

Vmcvar	ui_browserMaster;
Vmcvar	ui_browserGameType;
Vmcvar	ui_browserSortKey;
Vmcvar	ui_browserShowFull;
Vmcvar	ui_browserShowEmpty;

Vmcvar	ui_brassTime;
Vmcvar	ui_drawCrosshair;
Vmcvar	ui_drawCrosshairNames;
Vmcvar	ui_marks;

Vmcvar	ui_server1;
Vmcvar	ui_server2;
Vmcvar	ui_server3;
Vmcvar	ui_server4;
Vmcvar	ui_server5;
Vmcvar	ui_server6;
Vmcvar	ui_server7;
Vmcvar	ui_server8;
Vmcvar	ui_server9;
Vmcvar	ui_server10;
Vmcvar	ui_server11;
Vmcvar	ui_server12;
Vmcvar	ui_server13;
Vmcvar	ui_server14;
Vmcvar	ui_server15;
Vmcvar	ui_server16;

Vmcvar	ui_skipExitCredits;

static cvarTable_t cvarTable[] = {
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },

	{ &ui_brassTime, "cg_brassTime", "60000", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE },

	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },
	
	{ &ui_skipExitCredits, "ui_skipExitCredits", "1", CVAR_ARCHIVE }
};

static int cvarTableSize = ARRAY_LEN(cvarTable);


/*
 * UI_RegisterCvars
 */
void
UI_RegisterCvars(void)
{
	int i;
	cvarTable_t *cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString,
			cv->cvarFlags);
}

/*
 * UI_UpdateCvars
 */
void
UI_UpdateCvars(void)
{
	int i;
	cvarTable_t *cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Update(cv->vmCvar);
}
