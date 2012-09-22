/* display information while data is being loaded */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "local.h"

enum {
	MAX_LOADING_PLAYER_ICONS = 16,
	MAX_LOADING_ITEM_ICONS = 26
};

static int	loadingPlayerIconCount;
static int	loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];

static void
CG_DrawLoadingIcons(void)
{
	int	n, x, y;

	for(n = 0; n < loadingPlayerIconCount; n++){
		x	= 16 + n * 78;
		y	= 324-40;
		CG_DrawPic(x, y, 64, 64, loadingPlayerIcons[n]);
	}
	for(n = 0; n < loadingItemIconCount; n++){
		y = 400-40;
		if(n >= 13)
			y += 40;
		x = 16 + n % 13 * 48;
		CG_DrawPic(x, y, 32, 32, loadingItemIcons[n]);
	}
}

void
CG_LoadingString(const char *s)
{
	Q_strncpyz(cg.infoScreenText, s, sizeof(cg.infoScreenText));
	trap_UpdateScreen();
}

void
CG_LoadingItem(int itemNum)
{
	gitem_t *item;

	item = &bg_itemlist[itemNum];
	if(item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS)
		loadingItemIcons[loadingItemIconCount++] =
			trap_R_RegisterShaderNoMip(item->icon);
	CG_LoadingString(item->pickup_name);
}

void
CG_LoadingClient(int clientNum)
{
	const char	*info;
	char		*skin;
	char		personality[MAX_QPATH];
	char		model[MAX_QPATH];
	char		iconName[MAX_QPATH];

	info = CG_ConfigString(CS_PLAYERS + clientNum);

	if(loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS){
		Q_strncpyz(model, Info_ValueForKey(info, "model"), sizeof(model));
		skin = strrchr(model, '/');
		if(skin)
			*skin++ = '\0';
		else
			skin = "default";

		Q_sprintf(iconName, MAX_QPATH,
			Pplayermodels "/%s/icon_%s", model, skin);

		loadingPlayerIcons[loadingPlayerIconCount]
			= trap_R_RegisterShaderNoMip(iconName);
		if(!loadingPlayerIcons[loadingPlayerIconCount]){
			Q_sprintf(iconName, MAX_QPATH,
				Pplayermodels "/characters/%s/icon_%s",
				model, skin);
			loadingPlayerIcons[loadingPlayerIconCount]
				= trap_R_RegisterShaderNoMip(iconName);
		}
		if(!loadingPlayerIcons[loadingPlayerIconCount]){
			Q_sprintf(iconName, MAX_QPATH,
				Pplayermodels "/%s/icon_%s",
				DEFAULT_MODEL, "default");
			loadingPlayerIcons[loadingPlayerIconCount]
				= trap_R_RegisterShaderNoMip(iconName);
		}
		if(loadingPlayerIcons[loadingPlayerIconCount])
			loadingPlayerIconCount++;
	}

	Q_strncpyz(personality, Info_ValueForKey(info, "n"),
		sizeof(personality));
	Q_cleanstr(personality);

	if(cgs.gametype == GT_SINGLE_PLAYER)
		trap_S_RegisterSound(va(Pannounce "/%s", personality), qtrue);
	CG_LoadingString(personality);
}

void
CG_DrawInformation(void)
{
	const char *s, *info, *sysInfo;
	int y, value;
	qhandle_t levelshot, detail;
	char buf[1024];

	info = CG_ConfigString(CS_SERVERINFO);
	sysInfo = CG_ConfigString(CS_SYSTEMINFO);

	s = Info_ValueForKey(info, "mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("levelshots/%s", s));
	if(!levelshot)
		levelshot = trap_R_RegisterShaderNoMip(Pmenuart "/unknownmap");
	trap_R_SetColor(NULL);
	CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot);

	/* blend a detail texture over it */
	detail = trap_R_RegisterShader("levelShotDetail");
	trap_R_DrawStretchPic(0, 0, cgs.glconfig.vidWidth,
		cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail);

	/* draw the icons of things as they are loaded */
	CG_DrawLoadingIcons();

	/*
	 * the first 150 rows are reserved for the client connection
	 * screen to write into 
	 */
	if(cg.infoScreenText[0])
		UI_DrawProportionalString(320, 128-32, va("Loading... %s",
				cg.infoScreenText),
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
	else
		UI_DrawProportionalString(320, 128-32, "Awaiting snapshot...",
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);

	/* draw info string information */

	y = 180-32;

	/* don't print server lines if playing a local game */
	trap_Cvar_VariableStringBuffer("sv_running", buf, sizeof(buf));
	if(!atoi(buf)){
		/* server hostname */
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), 1024);
		Q_cleanstr(buf);
		UI_DrawProportionalString(320, y, buf,
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;

		/* pure server */
		s = Info_ValueForKey(sysInfo, "sv_pure");
		if(s[0] == '1'){
			UI_DrawProportionalString(320, y, "Pure Server",
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW,
				colorWhite);
			y += PROP_HEIGHT;
		}

		/* server-specific message of the day */
		s = CG_ConfigString(CS_MOTD);
		if(s[0]){
			UI_DrawProportionalString(320, y, s,
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW,
				colorWhite);
			y += PROP_HEIGHT;
		}

		/* some extra space after hostname and motd */
		y += 10;
	}

	/* map-specific message (long map name) */
	s = CG_ConfigString(CS_MESSAGE);
	if(s[0]){
		UI_DrawProportionalString(320, y, s,
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	/* cheats warning */
	s = Info_ValueForKey(sysInfo, "sv_cheats");
	if(s[0] == '1'){
		UI_DrawProportionalString(320, y, "CHEATS ARE ENABLED",
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	/* game type */
	switch(cgs.gametype){
	case GT_FFA:
		s = "Free For All";
		break;
	case GT_SINGLE_PLAYER:
		s = "Single Player";
		break;
	case GT_TOURNAMENT:
		s = "Tournament";
		break;
	case GT_TEAM:
		s = "Team Deathmatch";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
#ifdef MISSIONPACK
	case GT_1FCTF:
		s = "One Flag CTF";
		break;
	case GT_OBELISK:
		s = "Overload";
		break;
	case GT_HARVESTER:
		s = "Harvester";
		break;
#endif
	default:
		s = "Unknown Gametype";
		break;
	}
	UI_DrawProportionalString(320, y, s,
		UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
	y += PROP_HEIGHT;

	value = atoi(Info_ValueForKey(info, "timelimit"));
	if(value){
		UI_DrawProportionalString(320, y, va("timelimit %i", value),
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	if(cgs.gametype < GT_CTF){
		value = atoi(Info_ValueForKey(info, "fraglimit"));
		if(value){
			UI_DrawProportionalString(320, y,
				va("fraglimit %i", value),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}
	}

	if(cgs.gametype >= GT_CTF){
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if(value){
			UI_DrawProportionalString(320, y,
				va("capturelimit %i", value),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}
	}
}
