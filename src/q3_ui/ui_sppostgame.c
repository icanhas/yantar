/* single player post-game menu */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ui_local.h"

enum
{
	MAX_SCOREBOARD_CLIENTS = 8,

	AWARD_PRESENTATION_TIME = 2000,

	ID_AGAIN = 10,
	ID_NEXT = 11,
	ID_MENU = 12
};

#define ART_MENU0	Pmenuart "/menu_0"
#define ART_MENU1	Pmenuart "/menu_1"
#define ART_REPLAY0	Pmenuart "/replay_0"
#define ART_REPLAY1	Pmenuart "/replay_1"
#define ART_NEXT0	Pmenuart "/next_0"
#define ART_NEXT1	Pmenuart "/next_1"
#define MUSIC_LOSS	Pmusic "/loss"
#define MUSIC_WIN	Pmusic "/win"
#define ANNOUNCE_YOUWIN	Pannounce "/announce_youwin"
#define FMT_WHOWINS	Pannounce "/%s_wins"

typedef struct
{
	menuframework_s menu;
	menubitmap_s	item_again;
	menubitmap_s	item_next;
	menubitmap_s	item_menu;

	int		phase;
	int		ignoreKeysTime;
	int		starttime;
	int		scoreboardtime;
	int		serverId;

	int		clientNums[MAX_SCOREBOARD_CLIENTS];
	int		ranks[MAX_SCOREBOARD_CLIENTS];
	int		scores[MAX_SCOREBOARD_CLIENTS];

	char		placeNames[3][64];

	int		level;
	int		numClients;
	int		won;
	int		numAwards;
	int		awardsEarned[6];
	int		awardsLevels[6];
	qbool		playedSound[6];
	int		lastTier;
	sfxHandle_t	winnerSound;
} postgame_t;

static postgame_t postgame;
static char	arenainfo[MAX_INFO_VALUE];

char *ui_medalNames[] = {"Accuracy", "Impressive", "Excellent", "Gauntlet", 
	"Frags", "Perfect"};

char *ui_medalPicNames[] = 
{
	Pmedalart "/medal_accuracy",
	Pmedalart "/medal_impressive",
	Pmedalart "/medal_excellent",
	Pmedalart "/medal_gauntlet",
	Pmedalart "/medal_frags",
	Pmedalart "/medal_victory"
};

char *ui_medalSounds[] = 
{
	Pfeedback "/accuracy",
	Pfeedback "/impressive_a",
	Pfeedback "/excellent_a",
	Pfeedback "/gauntlet",
	Pfeedback "/frags",
	Pfeedback "/perfect"
};

static void
AgainEvent(void* ptr, int event)
{
	if(event != QM_ACTIVATED)
		return;
	UI_PopMenu();
	trap_Cmd_ExecuteText(EXEC_APPEND, "map_restart 0\n");
}


static void
NextEvent(void* ptr, int event)
{
	int	currentSet, levelSet, level, currentLevel;
	const char *arenaInfo;

	if(event != QM_ACTIVATED)
		return;
	UI_PopMenu();

	/* handle specially if we just won the training map */
	if(postgame.won == 0)
		level = 0;
	else
		level = postgame.level + 1;
	levelSet = level / ARENAS_PER_TIER;

	currentLevel = UI_GetCurrentGame();
	if(currentLevel == -1)
		currentLevel = postgame.level;
	currentSet = currentLevel / ARENAS_PER_TIER;

	if(levelSet > currentSet || levelSet == UI_GetNumSPTiers())
		level = currentLevel;

	arenaInfo = UI_GetArenaInfoByNumber(level);
	if(!arenaInfo)
		return;

	UI_SPArena_Start(arenaInfo);
}

static void
MenuEvent(void* ptr, int event)
{
	if(event != QM_ACTIVATED)
		return;
	UI_PopMenu();
	trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect; levelselect\n");
}

static sfxHandle_t
MenuKey(int key)
{
	if(uis.realtime < postgame.ignoreKeysTime)
		return 0;

	if(postgame.phase == 1){
		trap_Cmd_ExecuteText(EXEC_APPEND, "abort_podium\n");
		postgame.phase = 2;
		postgame.starttime = uis.realtime;
		postgame.ignoreKeysTime = uis.realtime + 250;
		return 0;
	}

	if(postgame.phase == 2){
		postgame.phase = 3;
		postgame.starttime = uis.realtime;
		postgame.ignoreKeysTime = uis.realtime + 250;
		return 0;
	}

	if(key == K_ESCAPE || key == K_MOUSE2)
		return 0;

	return Menu_DefaultKey(&postgame.menu, key);
}

static int medalLocations[6] = {144, 448, 88, 504, 32, 560};

static void
DrawAwardsMedals(int max)
{
	int	n, medal, amount, x, y;
	char	buf[16];

	for(n = 0; n < max; n++){
		x = medalLocations[n];
		y = 64;
		medal	= postgame.awardsEarned[n];
		amount	= postgame.awardsLevels[n];

		UI_DrawNamedPic(x, y, 48, 48, ui_medalPicNames[medal]);

		if(medal == AWARD_ACCURACY)
			Q_sprintf(buf, sizeof(buf), "%i%%", amount);
		else{
			if(amount == 1)
				continue;
			Q_sprintf(buf, sizeof(buf), "%i", amount);
		}
		UI_DrawString(x + 24, y + 52, buf, UI_CENTER, color_yellow);
	}
}

static void
DrawAwardsPresentation(int timer)
{
	int	awardNum, atimer, a;
	vec4_t	color;
	const int t = AWARD_PRESENTATION_TIME;
	
	awardNum = timer / t;
	atimer = timer % t;
	a = postgame.awardsEarned[awardNum];
	color[0] = color[1] = color[2] = 1.0f;
	color[3] = (float)(t - atimer) / (float)t;
	UI_DrawProportionalString(320, 64, ui_medalNames[a], UI_CENTER, color);

	DrawAwardsMedals(awardNum + 1);

	if(!postgame.playedSound[awardNum]){
		sfxHandle_t snd;

		postgame.playedSound[awardNum] = qtrue;
		snd = trap_S_RegisterSound(ui_medalSounds[a], qfalse);
		trap_S_StartLocalSound(snd, CHAN_ANNOUNCER);
	}
}

static void
MenuDrawScoreLine(int n, int y)
{
	int	rank;
	char	name[64], info[MAX_INFO_STRING];

	if(n > (postgame.numClients + 1))
		n -= (postgame.numClients + 2);

	if(n >= postgame.numClients)
		return;

	rank = postgame.ranks[n];
	if(rank & RANK_TIED_FLAG){
		UI_DrawString(640 - 31 * SMALLCHAR_WIDTH, y, "(tie)", 
			UI_LEFT | UI_SMALLFONT, color_white);
		rank &= ~RANK_TIED_FLAG;
	}
	trap_GetConfigString(CS_PLAYERS + postgame.clientNums[n], info,
		MAX_INFO_STRING);
	Q_strncpyz(name, Info_ValueForKey(info, "n"), sizeof(name));
	Q_CleanStr(name);

	UI_DrawString(640 - 25 * SMALLCHAR_WIDTH, y,
		va("#%i: %-16s %2i", rank + 1, name, postgame.scores[n]),
		UI_LEFT | UI_SMALLFONT, color_white);
}

static void
MenuDraw(void)
{
	int	n, timer, serverId;
	char	info[MAX_INFO_STRING];

	trap_GetConfigString(CS_SYSTEMINFO, info, sizeof(info));
	serverId = atoi(Info_ValueForKey(info, "sv_serverid"));
	if(serverId != postgame.serverId){
		UI_PopMenu();
		return;
	}

	if(postgame.numClients > 2)
		UI_DrawProportionalString(510, 480 - 64 - PROP_HEIGHT,
			postgame.placeNames[2], UI_CENTER,
			color_white);

	UI_DrawProportionalString(130, 480 - 64 - PROP_HEIGHT,
			postgame.placeNames[1], UI_CENTER, color_white);
	UI_DrawProportionalString(320, 480 - 64 - 2*PROP_HEIGHT,
		postgame.placeNames[0], UI_CENTER, color_white);

	if(postgame.phase == 1){
		timer = uis.realtime - postgame.starttime;

		if(timer >= 1000 && postgame.winnerSound){
			trap_S_StartLocalSound(postgame.winnerSound,
				CHAN_ANNOUNCER);
			postgame.winnerSound = 0;
		}

		if(timer < 5000)
			return;
		postgame.phase = 2;
		postgame.starttime = uis.realtime;
	}else if(postgame.phase == 2){
		timer = uis.realtime - postgame.starttime;
		if(timer >= (postgame.numAwards * AWARD_PRESENTATION_TIME)){
			if(timer < 5000)
				return;

			postgame.phase = 3;
			postgame.starttime = uis.realtime;
		}else{
			DrawAwardsPresentation(timer);
		}
	}else if(postgame.phase == 3){
		if(uis.demoversion){
			if(postgame.won == 1 && UI_ShowTierVideo(8)){
				trap_Cvar_Set("nextmap", "");
				trap_Cmd_ExecuteText(EXEC_APPEND,
					"disconnect; cinematic demoEnd.RoQ\n");
				return;
			}
		}else if(postgame.won > -1 && UI_ShowTierVideo(postgame.won + 1)){
			if(postgame.won == postgame.lastTier){
				trap_Cvar_Set("nextmap", "");
				trap_Cmd_ExecuteText(EXEC_APPEND,
					"disconnect; cinematic end.RoQ\n");
				return;
			}

			trap_Cvar_SetValue("ui_spSelection",
				postgame.won * ARENAS_PER_TIER);
			trap_Cvar_Set("nextmap", "levelselect");
			trap_Cmd_ExecuteText(EXEC_APPEND,
				va("disconnect; cinematic tier%i.RoQ\n",
					postgame.won + 1));
			return;
		}

		postgame.item_again.generic.flags &= ~QMF_INACTIVE;
		postgame.item_next.generic.flags &= ~QMF_INACTIVE;
		postgame.item_menu.generic.flags &= ~QMF_INACTIVE;

		DrawAwardsMedals(postgame.numAwards);

		Menu_Draw(&postgame.menu);
	}

	/* draw the scoreboard */
	if(!trap_Cvar_VariableValue("ui_spScoreboard"))
		return;

	timer = uis.realtime - postgame.scoreboardtime;
	if(postgame.numClients <= 3)
		n = 0;
	else
		n = timer / 1500 % (postgame.numClients + 2);
	MenuDrawScoreLine(n, 0);
	MenuDrawScoreLine(n+1, 0 + SMALLCHAR_HEIGHT);
	MenuDrawScoreLine(n+2, 0 + 2*SMALLCHAR_HEIGHT);
}

void
UI_SPPostgameMenu_Cache(void)
{
	int n;
	qbool buildscript;

	buildscript = trap_Cvar_VariableValue("com_buildscript");

	trap_R_RegisterShaderNoMip(ART_MENU0);
	trap_R_RegisterShaderNoMip(ART_MENU1);
	trap_R_RegisterShaderNoMip(ART_REPLAY0);
	trap_R_RegisterShaderNoMip(ART_REPLAY1);
	trap_R_RegisterShaderNoMip(ART_NEXT0);
	trap_R_RegisterShaderNoMip(ART_NEXT1);
	for(n = 0; n < 6; n++){
		trap_R_RegisterShaderNoMip(ui_medalPicNames[n]);
		trap_S_RegisterSound(ui_medalSounds[n], qfalse);
	}

	if(buildscript){
		trap_S_RegisterSound(MUSIC_LOSS, qfalse);
		trap_S_RegisterSound(MUSIC_WIN, qfalse);
		trap_S_RegisterSound(ANNOUNCE_YOUWIN, qfalse);
	}
}

static void
Init(void)
{
	postgame.menu.wrapAround		= qtrue;
	postgame.menu.key			= MenuKey;
	postgame.menu.draw			= MenuDraw;
	postgame.ignoreKeysTime			= uis.realtime + 1500;

	UI_SPPostgameMenu_Cache();

	postgame.item_menu.generic.type		= MTYPE_BITMAP;
	postgame.item_menu.generic.name		= ART_MENU0;
	postgame.item_menu.generic.flags	= QMF_LEFT_JUSTIFY 
		| QMF_PULSEIFFOCUS | QMF_INACTIVE;
	postgame.item_menu.generic.x		= 0;
	postgame.item_menu.generic.y		= 480-64;
	postgame.item_menu.generic.callback = MenuEvent;
	postgame.item_menu.generic.id		= ID_MENU;
	postgame.item_menu.width		= 128;
	postgame.item_menu.height		= 64;
	postgame.item_menu.focuspic		= ART_MENU1;

	postgame.item_again.generic.type	= MTYPE_BITMAP;
	postgame.item_again.generic.name	= ART_REPLAY0;
	postgame.item_again.generic.flags	= QMF_CENTER_JUSTIFY
		| QMF_PULSEIFFOCUS | QMF_INACTIVE;
	postgame.item_again.generic.x		= 320;
	postgame.item_again.generic.y		= 480-64;
	postgame.item_again.generic.callback = AgainEvent;
	postgame.item_again.generic.id		= ID_AGAIN;
	postgame.item_again.width		= 128;
	postgame.item_again.height		= 64;
	postgame.item_again.focuspic		= ART_REPLAY1;

	postgame.item_next.generic.type		= MTYPE_BITMAP;
	postgame.item_next.generic.name		= ART_NEXT0;
	postgame.item_next.generic.flags	= QMF_RIGHT_JUSTIFY
		| QMF_PULSEIFFOCUS | QMF_INACTIVE;
	postgame.item_next.generic.x		= 640;
	postgame.item_next.generic.y		= 480-64;
	postgame.item_next.generic.callback = NextEvent;
	postgame.item_next.generic.id		= ID_NEXT;
	postgame.item_next.width		= 128;
	postgame.item_next.height		= 64;
	postgame.item_next.focuspic		= ART_NEXT1;

	Menu_AddItem(&postgame.menu, (void*)&postgame.item_menu);
	Menu_AddItem(&postgame.menu, (void*)&postgame.item_again);
	Menu_AddItem(&postgame.menu, (void*)&postgame.item_next);
}

static void
Prepname(int i)
{
	int	len;
	char	name[64], info[MAX_INFO_STRING];

	trap_GetConfigString(CS_PLAYERS + postgame.clientNums[i], info,
		MAX_INFO_STRING);
	Q_strncpyz(name, Info_ValueForKey(info, "n"), sizeof(name));
	Q_CleanStr(name);
	len = strlen(name);

	while(len && UI_ProportionalStringWidth(name) > 256){
		len--;
		name[len] = 0;
	}

	Q_strncpyz(postgame.placeNames[i], name, sizeof(postgame.placeNames[i]));
}

void
UI_SPPostgameMenu_f(void)
{
	int	n, playerGameRank, playerClientNum, oldFrags, newFrags;
	int	awardValues[6];
	char	map[MAX_QPATH], info[MAX_INFO_STRING];
	const char *arena;

	memset(&postgame, 0, sizeof(postgame));

	trap_GetConfigString(CS_SYSTEMINFO, info, sizeof(info));
	postgame.serverId = atoi(Info_ValueForKey(info, "sv_serverid"));

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
	Q_strncpyz(map, Info_ValueForKey(info, "mapname"), sizeof(map));
	arena = UI_GetArenaInfoByMap(map);
	if(!arena)
		return;
	Q_strncpyz(arenainfo, arena, sizeof(arenainfo));

	postgame.level = atoi(Info_ValueForKey(arenainfo, "num"));

	postgame.numClients = atoi(UI_Argv(1));
	playerClientNum = atoi(UI_Argv(2));
	playerGameRank	= 8;	/* in case they ended game as a spectator */

	if(postgame.numClients > MAX_SCOREBOARD_CLIENTS)
		postgame.numClients = MAX_SCOREBOARD_CLIENTS;

	for(n = 0; n < postgame.numClients; n++){
		postgame.clientNums[n]	= atoi(UI_Argv(8 + n*3 + 1));
		postgame.ranks[n]	= atoi(UI_Argv(8 + n*3 + 2));
		postgame.scores[n]	= atoi(UI_Argv(8 + n*3 + 3));

		if(postgame.clientNums[n] == playerClientNum)
			playerGameRank = (postgame.ranks[n] & ~RANK_TIED_FLAG) + 1;
	}

	UI_SetBestScore(postgame.level, playerGameRank);

	/* process award stats and prepare presentation data */
	awardValues[AWARD_ACCURACY] = atoi(UI_Argv(3));
	awardValues[AWARD_IMPRESSIVE]	= atoi(UI_Argv(4));
	awardValues[AWARD_EXCELLENT]	= atoi(UI_Argv(5));
	awardValues[AWARD_GAUNTLET]	= atoi(UI_Argv(6));
	awardValues[AWARD_FRAGS]	= atoi(UI_Argv(7));
	awardValues[AWARD_PERFECT]	= atoi(UI_Argv(8));

	postgame.numAwards = 0;

	if(awardValues[AWARD_ACCURACY] >= 50){
		UI_LogAwardData(AWARD_ACCURACY, 1);
		postgame.awardsEarned[postgame.numAwards]
			= AWARD_ACCURACY;
		postgame.awardsLevels[postgame.numAwards]
			= awardValues[AWARD_ACCURACY];
		postgame.numAwards++;
	}

	if(awardValues[AWARD_IMPRESSIVE]){
		UI_LogAwardData(AWARD_IMPRESSIVE, awardValues[AWARD_IMPRESSIVE]);
		postgame.awardsEarned[postgame.numAwards]
			= AWARD_IMPRESSIVE;
		postgame.awardsLevels[postgame.numAwards]
			= awardValues[AWARD_IMPRESSIVE];
		postgame.numAwards++;
	}

	if(awardValues[AWARD_EXCELLENT]){
		UI_LogAwardData(AWARD_EXCELLENT, awardValues[AWARD_EXCELLENT]);
		postgame.awardsEarned[postgame.numAwards]
			= AWARD_EXCELLENT;
		postgame.awardsLevels[postgame.numAwards]
			= awardValues[AWARD_EXCELLENT];
		postgame.numAwards++;
	}

	if(awardValues[AWARD_GAUNTLET]){
		UI_LogAwardData(AWARD_GAUNTLET, awardValues[AWARD_GAUNTLET]);
		postgame.awardsEarned[postgame.numAwards]
			= AWARD_GAUNTLET;
		postgame.awardsLevels[postgame.numAwards]
			= awardValues[AWARD_GAUNTLET];
		postgame.numAwards++;
	}

	oldFrags = UI_GetAwardLevel(AWARD_FRAGS) / 100;
	UI_LogAwardData(AWARD_FRAGS, awardValues[AWARD_FRAGS]);
	newFrags = UI_GetAwardLevel(AWARD_FRAGS) / 100;
	if(newFrags > oldFrags){
		postgame.awardsEarned[postgame.numAwards]
			= AWARD_FRAGS;
		postgame.awardsLevels[postgame.numAwards]
			= newFrags * 100;
		postgame.numAwards++;
	}

	if(awardValues[AWARD_PERFECT]){
		UI_LogAwardData(AWARD_PERFECT, 1);
		postgame.awardsEarned[postgame.numAwards]
			= AWARD_PERFECT;
		postgame.awardsLevels[postgame.numAwards]
			= 1;
		postgame.numAwards++;
	}

	if(playerGameRank == 1)
		postgame.won = UI_TierCompleted(postgame.level);
	else
		postgame.won = -1;

	postgame.starttime = uis.realtime;
	postgame.scoreboardtime = uis.realtime;

	trap_Key_SetCatcher(KEYCATCH_UI);
	uis.menusp = 0;

	Init();
	UI_PushMenu(&postgame.menu);

	if(playerGameRank == 1)
		Menu_SetCursorToItem(&postgame.menu, &postgame.item_next);
	else
		Menu_SetCursorToItem(&postgame.menu, &postgame.item_again);

	Prepname(0);
	Prepname(1);
	Prepname(2);

	if(playerGameRank != 1){
		postgame.winnerSound = trap_S_RegisterSound(
			va(FMT_WHOWINS, postgame.placeNames[0]), qfalse);
		trap_Cmd_ExecuteText(EXEC_APPEND, "music " MUSIC_LOSS "\n");
	}else{
		postgame.winnerSound = trap_S_RegisterSound(
			ANNOUNCE_YOUWIN, qfalse);
		trap_Cmd_ExecuteText(EXEC_APPEND, "music " MUSIC_WIN "\n");
	}

	postgame.phase = 1;

	postgame.lastTier = UI_GetNumSPTiers();
	if(UI_GetSpecialArenaInfo("final"))
		postgame.lastTier++;
}

