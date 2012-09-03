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
/*
 * cg_main.c -- initialization and primary entry point for cgame */
#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
/* display context for new ui stuff */
displayContextDef_t cgDC;
#endif

int forceModelModificationCount = -1;

void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
void CG_Shutdown(void);


/*
 * vmMain
 *
 * This is the only way control passes into the module.
 * This must be the very first function compiled into the .q3vm file
 */
Q_EXPORT intptr_t
vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
       int arg6, int arg7, int arg8, int arg9, int arg10,
       int arg11)
{

	switch(command){
	case CG_INIT:
		CG_Init(arg0, arg1, arg2);
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame(arg0, arg1, arg2);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
#ifdef MISSIONPACK
		cgDC.cursorx	= cgs.cursorX;
		cgDC.cursory	= cgs.cursorY;
#endif
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error("vmMain: unknown command %i", command);
		break;
	}
	return -1;
}

cg_t	cg;
cgs_t	cgs;
centity_t	cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t	cg_items[MAX_ITEMS];

vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_synchronousClients;
vmCvar_t	cg_teamChatTime;
vmCvar_t	cg_teamChatHeight;
vmCvar_t	cg_stats;
vmCvar_t	cg_buildScript;
vmCvar_t	cg_forceModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
vmCvar_t	cg_hudFiles;
vmCvar_t	cg_scorePlum;
vmCvar_t	cg_smoothClients;
vmCvar_t	pmove_fixed;
/* vmCvar_t	cg_pmove_fixed; */
vmCvar_t	pmove_msec;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_oldRail;
vmCvar_t	cg_oldRocket;
vmCvar_t	cg_oldPlasma;
vmCvar_t	cg_trueLightning;

#ifdef MISSIONPACK
vmCvar_t	cg_redTeamName;
vmCvar_t	cg_blueTeamName;
vmCvar_t	cg_currentSelectedPlayer;
vmCvar_t	cg_currentSelectedPlayerName;
vmCvar_t	cg_singlePlayer;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;
vmCvar_t	cg_singlePlayerActive;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;
vmCvar_t	cg_obeliskRespawnDelay;
#endif

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int		cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0 },	/* used for debugging */
	{ &cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE  },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "60000", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup, "cg_bobup", "0.005", CVAR_CHEAT },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", 0 },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
#endif
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
	/* the following variables are created in other parts of the system,
	 * but we also reference them here */
	{ &cg_buildScript, "com_buildScript", "0", 0 },	/* force loading of all possible data amd error on failures */
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO },
#ifdef MISSIONPACK
	{ &cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE |
	  CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE |
	  CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0",
	  CVAR_ARCHIVE},
	{ &cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "",
	  CVAR_ARCHIVE},
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{ &cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO},
	{ &cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10",
	  CVAR_SERVERINFO},
	{ &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE},
#endif
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO |
	  CVAR_ARCHIVE},
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO},
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{ &cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE},
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE},
	{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE},
	{ &cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE}
/*	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE } */
};

static int cvarTableSize = ARRAY_LEN(cvarTable);

/*
 * CG_RegisterCvars
 */
void
CG_RegisterCvars(void)
{
	int	i;
	cvarTable_t *cv;
	char	var[MAX_TOKEN_CHARS];

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Register(cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags);

	/* see if we are also running the server on this machine */
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = atoi(var);

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL,
		CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL,
		CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL,
		CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD,
		CVAR_USERINFO | CVAR_ARCHIVE);
}

/*
 * CG_ForceModelChange
 */
static void
CG_ForceModelChange(void)
{
	int i;

	for(i=0; i<MAX_CLIENTS; i++){
		const char *clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS+i);
		if(!clientInfo[0])
			continue;
		CG_NewClientInfo(i);
	}
}

/*
 * CG_UpdateCvars
 */
void
CG_UpdateCvars(void)
{
	int i;
	cvarTable_t *cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Update(cv->vmCvar);

	/* check for modications here */

	/* If team overlay is on, ask for updates from the server.  If it's off,
	 * let the server know so we don't receive it */
	if(drawTeamOverlayModificationCount !=
	   cg_drawTeamOverlay.modificationCount){
		drawTeamOverlayModificationCount =
			cg_drawTeamOverlay.modificationCount;

		if(cg_drawTeamOverlay.integer > 0)
			trap_Cvar_Set("teamoverlay", "1");
		else
			trap_Cvar_Set("teamoverlay", "0");
	}

	/* if force model changed */
	if(forceModelModificationCount != cg_forceModel.modificationCount){
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
}

int
CG_CrosshairPlayer(void)
{
	if(cg.time > (cg.crosshairClientTime + 1000))
		return -1;
	return cg.crosshairClientNum;
}

int
CG_LastAttacker(void)
{
	if(!cg.attackerTime)
		return -1;
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL
CG_Printf(const char *msg, ...)
{
	va_list argptr;
	char	text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print(text);
}

void QDECL
CG_Error(const char *msg, ...)
{
	va_list argptr;
	char	text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error(text);
}

void QDECL
Com_Errorf(int level, const char *error, ...)
{
	va_list argptr;
	char	text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error("%s", text);
}

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char	text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

/*
 * CG_Argv
 */
const char *
CG_Argv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}


/* ======================================================================== */

/*
 * CG_RegisterItemSounds
 *
 * The server says this item is used on this level
 */
static void
CG_RegisterItemSounds(int itemNum)
{
	gitem_t *item;
	char	data[MAX_QPATH];
	char	*s, *start;
	int len;

	item = &bg_itemlist[ itemNum ];

	if(item->pickup_sound)
		trap_S_RegisterSound(item->pickup_sound, qfalse);

	/* parse the space seperated precache string for other media */
	s = item->sounds;
	if(!s || !s[0])
		return;

	while(*s){
		start = s;
		while(*s && *s != ' ')
			s++;

		len = s-start;
		if(len >= MAX_QPATH || len < 5){
			CG_Error("PrecacheItem: %s has bad precache string",
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if(*s)
			s++;

		if(!strcmp(data+len-3, "wav"))
			trap_S_RegisterSound(data, qfalse);
	}
}


/*
 * CG_RegisterSounds
 *
 * called during a precache command
 */
static void
CG_RegisterSounds(void)
{
	int	i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char *soundName;

	/* voice commands */
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif

	cgs.media.oneMinuteSound = trap_S_RegisterSound(
		Pfeedback "/1_minute", qtrue);
	cgs.media.fiveMinuteSound = trap_S_RegisterSound(
		Pfeedback "/5_minute", qtrue);
	cgs.media.suddenDeathSound = trap_S_RegisterSound(
		Pfeedback "/sudden_death", qtrue);
	cgs.media.oneFragSound = trap_S_RegisterSound(
		Pfeedback "/1_frag", qtrue);
	cgs.media.twoFragSound = trap_S_RegisterSound(
		Pfeedback "/2_frags", qtrue);
	cgs.media.threeFragSound = trap_S_RegisterSound(
		Pfeedback "/3_frags", qtrue);
	cgs.media.count3Sound = trap_S_RegisterSound(
		Pfeedback "/three", qtrue);
	cgs.media.count2Sound = trap_S_RegisterSound(
		Pfeedback "/two",
		qtrue);
	cgs.media.count1Sound = trap_S_RegisterSound(
		Pfeedback "/one",
		qtrue);
	cgs.media.countFightSound = trap_S_RegisterSound(
		Pfeedback "/fight", qtrue);
	cgs.media.countPrepareSound = trap_S_RegisterSound(
		Pfeedback "/prepare", qtrue);
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound(
		Pfeedback "/prepare_team", qtrue);
#endif

	if(cgs.gametype >= GT_TEAM || cg_buildScript.integer){

		cgs.media.captureAwardSound = trap_S_RegisterSound(
			Pteamsounds "/flagcapture_yourteam", qtrue);
		cgs.media.redLeadsSound = trap_S_RegisterSound(
			Pfeedback "/redleads", qtrue);
		cgs.media.blueLeadsSound = trap_S_RegisterSound(
			Pfeedback "/blueleads", qtrue);
		cgs.media.teamsTiedSound = trap_S_RegisterSound(
			Pfeedback "/teamstied", qtrue);
		cgs.media.hitTeamSound = trap_S_RegisterSound(
			Pfeedback "/hit_teammate", qtrue);

		cgs.media.redScoredSound = trap_S_RegisterSound(
			Pteamsounds "/voc_red_scores", qtrue);
		cgs.media.blueScoredSound = trap_S_RegisterSound(
			Pteamsounds "/voc_blue_scores", qtrue);

		cgs.media.captureYourTeamSound = trap_S_RegisterSound(
			Pteamsounds "/flagcapture_yourteam", qtrue);
		cgs.media.captureOpponentSound = trap_S_RegisterSound(
			Pteamsounds "/flagcapture_opponent", qtrue);

		cgs.media.returnYourTeamSound = trap_S_RegisterSound(
			Pteamsounds "/flagreturn_yourteam", qtrue);
		cgs.media.returnOpponentSound = trap_S_RegisterSound(
			Pteamsounds "/flagreturn_opponent", qtrue);

		cgs.media.takenYourTeamSound = trap_S_RegisterSound(
			Pteamsounds "/flagtaken_yourteam", qtrue);
		cgs.media.takenOpponentSound = trap_S_RegisterSound(
			Pteamsounds "/flagtaken_opponent", qtrue);

		if(cgs.gametype == GT_CTF || cg_buildScript.integer){
			cgs.media.redFlagReturnedSound =
				trap_S_RegisterSound(
					Pteamsounds "/voc_red_returned",
					qtrue);
			cgs.media.blueFlagReturnedSound =
				trap_S_RegisterSound(
					Pteamsounds "/voc_blue_returned",
					qtrue);
			cgs.media.enemyTookYourFlagSound =
				trap_S_RegisterSound(
					Pteamsounds "/voc_enemy_flag",
					qtrue);
			cgs.media.yourTeamTookEnemyFlagSound =
				trap_S_RegisterSound(
					Pteamsounds "/voc_team_flag",
					qtrue);
		}

#ifdef MISSIONPACK
		if(cgs.gametype == GT_1FCTF || cg_buildScript.integer){
			/* FIXME: get a replacement for this sound ? */
			cgs.media.neutralFlagReturnedSound =
				trap_S_RegisterSound(
					Pteamsounds "/flagreturn_opponent",
					qtrue);
			cgs.media.yourTeamTookTheFlagSound =
				trap_S_RegisterSound(
					Pteamsounds "/voc_team_1flag",
					qtrue);
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound(
				Pteamsounds "/voc_enemy_1flag", qtrue);
		}

		if(cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF ||
		   cg_buildScript.integer){
			cgs.media.youHaveFlagSound = trap_S_RegisterSound(
				Pteamsounds "/voc_you_flag", qtrue);
			cgs.media.holyShitSound = trap_S_RegisterSound(
				Pfeedback "/voc_holyshit", qtrue);
		}

		if(cgs.gametype == GT_OBELISK || cg_buildScript.integer)
			cgs.media.yourBaseIsUnderAttackSound =
				trap_S_RegisterSound(
					Pteamsounds "/voc_base_attack",
					qtrue);

#else
		cgs.media.youHaveFlagSound = trap_S_RegisterSound(
			Pteamsounds "/voc_you_flag", qtrue);
		cgs.media.holyShitSound = trap_S_RegisterSound(
			Pfeedback "/voc_holyshit", qtrue);
		cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound(
			Pteamsounds "/flagreturn_opponent", qtrue);
		cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound(
			Pteamsounds "/voc_team_1flag", qtrue);
		cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound(
			Pteamsounds "/voc_enemy_1flag", qtrue);
#endif
	}

	cgs.media.tracerSound = trap_S_RegisterSound(
		Pmgsounds "/buletby1", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound(
		Pweapsounds "/change", qfalse);
	cgs.media.wearOffSound = trap_S_RegisterSound(Pitemsounds "/wearoff",
		qfalse);
	cgs.media.useNothingSound = trap_S_RegisterSound(
		Pitemsounds "/use_nothing", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound(Pplayersounds "/gibsplt1",
		qfalse);
	cgs.media.gibBounce1Sound = trap_S_RegisterSound(
		Pplayersounds "/gibimp1", qfalse);
	cgs.media.gibBounce2Sound = trap_S_RegisterSound(
		Pplayersounds "/gibimp2", qfalse);
	cgs.media.gibBounce3Sound = trap_S_RegisterSound(
		Pplayersounds "/gibimp3", qfalse);

#ifdef MISSIONPACK
	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound(
		Pitemsounds "/invul_activate", qfalse);
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound(
		Pitemsounds "/invul_impact_01", qfalse);
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound(
		Pitemsounds "/invul_impact_02", qfalse);
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound(
		Pitemsounds "/invul_impact_03", qfalse);
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound(
		Pitemsounds "/invul_juiced", qfalse);
	cgs.media.obeliskHitSound1 = trap_S_RegisterSound(
		Pitemsounds "/obelisk_hit_01", qfalse);
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound(
		Pitemsounds "/obelisk_hit_02", qfalse);
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound(
		Pitemsounds "/obelisk_hit_03", qfalse);
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound(
		Pitemsounds "/obelisk_respawn", qfalse);

	cgs.media.ammoregenSound = trap_S_RegisterSound(
		Pitemsounds "/cl_ammoregen", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound(
		Pitemsounds "/cl_doubler", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound(
		Pitemsounds "/cl_guard", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound(
		Pitemsounds "/cl_scout", qfalse);
#endif

	cgs.media.teleInSound = trap_S_RegisterSound(Pworldsounds "/telein",
		qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound(Pworldsounds "/teleout",
		qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound(
		Pitemsounds "/respawn1", qfalse);

	cgs.media.noAmmoSound = trap_S_RegisterSound(Pweapsounds "s/noammo",
		qfalse);

	cgs.media.talkSound = trap_S_RegisterSound(Pplayersounds "/talk",
		qfalse);
	cgs.media.landSound = trap_S_RegisterSound(Pplayersounds "/land1",
		qfalse);

	cgs.media.hitSound = trap_S_RegisterSound(Pfeedback "/hit",
		qfalse);
#ifdef MISSIONPACK
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound(
		Pfeedback "/hithi", qfalse);
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound(
		Pfeedback "/hitlo", qfalse);
#endif

	cgs.media.impressiveSound = trap_S_RegisterSound(
		Pfeedback "/impressive", qtrue);
	cgs.media.excellentSound = trap_S_RegisterSound(
		Pfeedback "/excellent", qtrue);
	cgs.media.deniedSound = trap_S_RegisterSound(Pfeedback "/denied",
		qtrue);
	cgs.media.humiliationSound = trap_S_RegisterSound(
		Pfeedback "/humiliation", qtrue);
	cgs.media.assistSound = trap_S_RegisterSound(
		Pfeedback "/assist", qtrue);
	cgs.media.defendSound = trap_S_RegisterSound(
		Pfeedback "/defense", qtrue);
#ifdef MISSIONPACK
	cgs.media.firstImpressiveSound = trap_S_RegisterSound(
		Pfeedback "/first_impressive", qtrue);
	cgs.media.firstExcellentSound = trap_S_RegisterSound(
		Pfeedback "/first_excellent", qtrue);
	cgs.media.firstHumiliationSound = trap_S_RegisterSound(
		Pfeedback "/first_gauntlet", qtrue);
#endif

	cgs.media.takenLeadSound = trap_S_RegisterSound(
		Pfeedback "/takenlead", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound(
		Pfeedback "/tiedlead", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound(
		Pfeedback "/lostlead", qtrue);

#ifdef MISSIONPACK
	cgs.media.voteNow = trap_S_RegisterSound(Pfeedback "/vote_now",
		qtrue);
	cgs.media.votePassed = trap_S_RegisterSound(
		Pfeedback "/vote_passed", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound(
		Pfeedback "/vote_failed", qtrue);
#endif

	cgs.media.watrInSound = trap_S_RegisterSound(
		Pplayersounds "/watr_in", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound(
		Pplayersounds "/watr_out", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound(
		Pplayersounds "/watr_un", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound (Pworldsounds "/jumppad",
		qfalse);

	for(i=0; i<4; i++){
		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/step%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (
			name, qfalse);

		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/boot%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (
			name, qfalse);

		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/flesh%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (
			name, qfalse);

		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/mech%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (
			name, qfalse);

		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/energy%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (
			name, qfalse);

		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/splash%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (
			name, qfalse);

		Q_sprintf (name, sizeof(name),
			Pplayersounds "/footsteps/clank%i",
			i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (
			name, qfalse);
	}

	/* only register the items that the server says we need */
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for(i = 1; i < bg_numItems; i++)
/*		if ( items[ i ] == '1' || cg_buildScript.integer ) { */
		CG_RegisterItemSounds(i);
/*		} */

	for(i = 1; i < MAX_SOUNDS; i++){
		soundName = CG_ConfigString(CS_SOUNDS+i);
		if(!soundName[0])
			break;
		if(soundName[0] == '*')
			continue;	/* custom sound */
		cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}

	/* FIXME: only needed with item */
	cgs.media.flightSound = trap_S_RegisterSound(Pitemsounds "/flight",
		qfalse);
	cgs.media.medkitSound = trap_S_RegisterSound (
		Pitemsounds "/use_medkit", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound(Pitemsounds "/damage3",
		qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound (
		Pmgsounds "/ric1", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound (
		Pmgsounds "/ric2", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound (
		Pmgsounds "/ric3", qfalse);
	/* cgs.media.sfx_railg = trap_S_RegisterSound (Prailsounds "/railgf1a", qfalse); */
	cgs.media.sfx_rockexp = trap_S_RegisterSound (
		Prlsounds "/rocklx1a", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound (
		Pplasmasounds "/plasmx1a", qfalse);
#ifdef MISSIONPACK
	cgs.media.sfx_proxexp = trap_S_RegisterSound(
		Pproxsounds "/wstbexpl", qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound(
		"sound/weapons/nailgun/wnalimpd", qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound(
		"sound/weapons/nailgun/wnalimpl", qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound(
		"sound/weapons/nailgun/wnalimpm", qfalse);
	cgs.media.sfx_chghit = trap_S_RegisterSound(
		Pgattlingsounds "/wvulimpd", qfalse);
	cgs.media.sfx_chghitflesh = trap_S_RegisterSound(
		Pgattlingsounds "/wvulimpl", qfalse);
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound(
		Pgattlingsounds "/wvulimpm", qfalse);
	cgs.media.weaponHoverSound = trap_S_RegisterSound(
		Pweapsounds "/weapon_hover", qfalse);
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound(
		Pitemsounds "/kam_explode", qfalse);
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound(
		Pitemsounds "/kam_implode", qfalse);
	cgs.media.kamikazeFarSound = trap_S_RegisterSound(
		Pitemsounds "/kam_explode_far", qfalse);
	cgs.media.winnerSound = trap_S_RegisterSound(
		Pfeedback "/voc_youwin", qfalse);
	cgs.media.loserSound = trap_S_RegisterSound(
		Pfeedback "/voc_youlose", qfalse);

	cgs.media.wstbimplSound = trap_S_RegisterSound(
		Pproxsounds "/wstbimpl", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound(
		Pproxsounds "/wstbimpm", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound(
		Pproxsounds "/wstbimpd", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound(
		Pproxsounds "/wstbactv", qfalse);
#endif

	cgs.media.regenSound = trap_S_RegisterSound(Pitemsounds "/regen",
		qfalse);
	cgs.media.protectSound = trap_S_RegisterSound(
		Pitemsounds "/protect3", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound(
		Pitemsounds "/n_health", qfalse);
	cgs.media.hgrenb1aSound = trap_S_RegisterSound(
		Pgrenadesounds "/hgrenb1a", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound(
		Pgrenadesounds "/hgrenb2a", qfalse);

#ifdef MISSIONPACK
	trap_S_RegisterSound(Pplayersounds "/james/death1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/death2", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/death3", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/jump1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/pain25_1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/pain75_1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/pain100_1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/falling1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/gasp", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/drown", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/fall1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/james/taunt", qfalse);

	trap_S_RegisterSound(Pplayersounds "/janet/death1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/death2", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/death3", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/jump1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/pain25_1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/pain75_1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/pain100_1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/falling1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/gasp", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/drown", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/fall1", qfalse);
	trap_S_RegisterSound(Pplayersounds "/janet/taunt", qfalse);
#endif

}


/* =================================================================================== */


/*
 * CG_RegisterGraphics
 *
 * This function may execute for a couple of minutes with a slow disk.
 */
static void
CG_RegisterGraphics(void)
{
	int	i;
	char	items[MAX_ITEMS+1];
	static char *sb_nums[11] = {
		P2dart "/numbers/zero_32b",
		P2dart "/numbers/one_32b",
		P2dart "/numbers/two_32b",
		P2dart "/numbers/three_32b",
		P2dart "/numbers/four_32b",
		P2dart "/numbers/five_32b",
		P2dart "/numbers/six_32b",
		P2dart "/numbers/seven_32b",
		P2dart "/numbers/eight_32b",
		P2dart "/numbers/nine_32b",
		P2dart "/numbers/minus_32b",
	};

	/* clear any references to old media */
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	CG_LoadingString(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	/* precache status bar pics */
	CG_LoadingString("game media");

	for(i=0; i<11; i++)
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader(
		Pmenuart "/skill1");
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader(
		Pmenuart "/skill2");
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader(
		Pmenuart "/skill3");
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader(
		Pmenuart "/skill4");
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader(
		Pmenuart "/skill5");

	cgs.media.viewBloodShader = trap_R_RegisterShader("viewBloodBlend");

	cgs.media.deferShader = trap_R_RegisterShaderNoMip(P2dart "/defer");

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip(
		Pmenuart "/name");
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip(
		Pmenuart "/ping");
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip(
		Pmenuart "/score");
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip(
		Pmenuart "/time");

	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader(
		"smokePuffRagePro");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader(
		"shotgunSmokePuff");
#ifdef MISSIONPACK
	cgs.media.nailPuffShader	= trap_R_RegisterShader("nailtrail");
	cgs.media.blueProxMine		= trap_R_RegisterModel(
		Pweaphitmodels "/proxmineb");
#endif
	cgs.media.plasmaBallShader = trap_R_RegisterShader(
		"sprites/plasma1");
	cgs.media.bloodTrailShader	= trap_R_RegisterShader("bloodTrail");
	cgs.media.lagometerShader	= trap_R_RegisterShader("lagometer");
	cgs.media.connectionShader	= trap_R_RegisterShader("disconnected");

	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");

	cgs.media.tracerShader	= trap_R_RegisterShader(Pmiscart "/tracer");
	cgs.media.selectShader	= trap_R_RegisterShader(P2dart "/select");

	for(i = 0; i < NUM_CROSSHAIRS; i++)
		cgs.media.crosshairShader[i] =
			trap_R_RegisterShader(va(P2dart "/crosshair%c", 'a'+i));

	cgs.media.backTileShader = trap_R_RegisterShader(
		P2dart "/backtile");
	cgs.media.noammoShader = trap_R_RegisterShader(Picons "/noammo");

	/* powerup shaders */
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad");
	cgs.media.quadWeaponShader = trap_R_RegisterShader(
		"powerups/quadWeapon");
	cgs.media.battleSuitShader = trap_R_RegisterShader(
		"powerups/battleSuit");
	cgs.media.battleWeaponShader = trap_R_RegisterShader(
		"powerups/battleWeapon");
	cgs.media.invisShader	= trap_R_RegisterShader("powerups/invisibility");
	cgs.media.regenShader	= trap_R_RegisterShader("powerups/regen");
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff");

#ifdef MISSIONPACK
	if(cgs.gametype == GT_HARVESTER || cg_buildScript.integer){
		cgs.media.redCubeModel = trap_R_RegisterModel(
			Ppowerupmodels "/orb/r_orb");
		cgs.media.blueCubeModel = trap_R_RegisterModel(
			Ppowerupmodels "/orb/b_orb");
		cgs.media.redCubeIcon = trap_R_RegisterShader(
			Picons "/skull_red");
		cgs.media.blueCubeIcon = trap_R_RegisterShader(
			Picons "/skull_blue");
	}

	if(cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ||
	   cgs.gametype == GT_HARVESTER || cg_buildScript.integer){
#else
	if(cgs.gametype == GT_CTF || cg_buildScript.integer){
#endif
		cgs.media.redFlagModel = trap_R_RegisterModel(
			Pflagmodels "/r_flag");
		cgs.media.blueFlagModel = trap_R_RegisterModel(
			Pflagmodels "/b_flag");
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_red1");
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_red2");
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_red3");
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_blu1");
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_blu2");
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_blu3");
#ifdef MISSIONPACK
		cgs.media.flagPoleModel = trap_R_RegisterModel(
			Pflagmodels "/flagpole");
		cgs.media.flagFlapModel = trap_R_RegisterModel(
			Pflagmodels "/flagflap3");

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin(
			Pflagmodels "/red.skin");
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin(
			Pflagmodels "/blue.skin");
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin(
			Pflagmodels "/white.skin");

		cgs.media.redFlagBaseModel = trap_R_RegisterModel(
			Pflagbasemodels "/red_base");
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel(
			Pflagbasemodels "/blue_base");
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel(
			Pflagbasemodels "/ntrl_base");
#endif
	}

#ifdef MISSIONPACK
	if(cgs.gametype == GT_1FCTF || cg_buildScript.integer){
		cgs.media.neutralFlagModel = trap_R_RegisterModel(
			Pflagmodels "/n_flag");
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_neutral1");
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_red2");
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_blu2");
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip(
			Picons "/iconf_neutral3");
	}

	if(cgs.gametype == GT_OBELISK || cg_buildScript.integer){
		cgs.media.rocketExplosionShader = trap_R_RegisterShader(
			"rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel(
			Ppowerupmodels "/overload_base");
		cgs.media.overloadTargetModel = trap_R_RegisterModel(
			Ppowerupmodels "/overload_target");
		cgs.media.overloadLightsModel = trap_R_RegisterModel(
			Ppowerupmodels "/overload_lights");
		cgs.media.overloadEnergyModel = trap_R_RegisterModel(
			Ppowerupmodels "/overload_energy");
	}

	if(cgs.gametype == GT_HARVESTER || cg_buildScript.integer){
		cgs.media.harvesterModel = trap_R_RegisterModel(
			Pharvestermodels "/harvester");
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin(
			Pharvestermodels "/red.skin");
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin(
			Pharvestermodels "/blue.skin");
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel(
			Pobeliskmodels "/obelisk");
	}

	cgs.media.redKamikazeShader = trap_R_RegisterShader(
		Pweaphitmodels "/kamikred");
	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff");
#endif

	if(cgs.gametype >= GT_TEAM || cg_buildScript.integer){
		cgs.media.friendShader	= trap_R_RegisterShader(Psprites "/foe");
		cgs.media.redQuadShader = trap_R_RegisterShader(
			P2dart "/blueflag");
		cgs.media.teamStatusBar = trap_R_RegisterShader(
			P2dart "/colorbar");
#ifdef MISSIONPACK
		cgs.media.blueKamikazeShader = trap_R_RegisterShader(
			Pweaphitmodels "/kamikblu");
#endif
	}

	cgs.media.armorModel = trap_R_RegisterModel(
		Parmormodels "/armor_yel");
	cgs.media.armorIcon = trap_R_RegisterShaderNoMip(
		Picons "/iconr_yellow");

	cgs.media.machinegunBrassModel = trap_R_RegisterModel(
		Pshellmodels "/m_shell");
	cgs.media.shotgunBrassModel = trap_R_RegisterModel(
		Pshellmodels "/s_shell");

	cgs.media.gibAbdomen = trap_R_RegisterModel(Pgibmodels "/abdomen");
	cgs.media.gibArm	= trap_R_RegisterModel(Pgibmodels "/arm");
	cgs.media.gibChest	= trap_R_RegisterModel(Pgibmodels "/chest");
	cgs.media.gibFist	= trap_R_RegisterModel(Pgibmodels "/fist");
	cgs.media.gibFoot	= trap_R_RegisterModel(Pgibmodels "/foot");
	cgs.media.gibForearm	= trap_R_RegisterModel(Pgibmodels "/forearm");
	cgs.media.gibIntestine = trap_R_RegisterModel(Pgibmodels "/intestine");
	cgs.media.gibLeg = trap_R_RegisterModel(Pgibmodels "/leg");
	cgs.media.gibSkull = trap_R_RegisterModel(Pgibmodels "/skull");
	cgs.media.gibBrain = trap_R_RegisterModel(Pgibmodels "/brain");

	cgs.media.smoke2 = trap_R_RegisterModel(Pshellmodels "/s_shell");

	cgs.media.balloonShader = trap_R_RegisterShader(Psprites "/balloon3");

	cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion");

	cgs.media.bulletFlashModel = trap_R_RegisterModel(Pweaphitmodels "/bullet");
	cgs.media.ringFlashModel = trap_R_RegisterModel(Pweaphitmodels "/ring02");
	cgs.media.dishFlashModel = trap_R_RegisterModel(Pweaphitmodels "/boom01");
#ifdef MISSIONPACK
	cgs.media.teleportEffectModel = trap_R_RegisterModel(Ppowerupmodels "/pop");
#else
	cgs.media.teleportEffectModel = trap_R_RegisterModel(Ptelemodels "/telep");
	cgs.media.teleportEffectShader = trap_R_RegisterShader("teleportEffect");
#endif
#ifdef MISSIONPACK
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel(
		Pweaphitmodels "/kamboom2");
	cgs.media.kamikazeShockWave = trap_R_RegisterModel(
		Pweaphitmodels "/kamwave");
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel(
		Pweaphitmodels "/kamikazi");
	cgs.media.kamikazeHeadTrail = trap_R_RegisterModel(
		Ppowerupmodels "/trailtest");
	cgs.media.guardPowerupModel = trap_R_RegisterModel(
		Ppowerupmodels "/guard_player");
	cgs.media.scoutPowerupModel = trap_R_RegisterModel(
		Ppowerupmodels "/scout_player");
	cgs.media.doublerPowerupModel = trap_R_RegisterModel(
		Ppowerupmodels "/doubler_player");
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel(
		Ppowerupmodels "/ammo_player");
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel(
		Ppowerupmodels "/shield/impact");
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel(
		Ppowerupmodels "/shield/juicer");
	cgs.media.medkitUsageModel = trap_R_RegisterModel(
		Ppowerupmodels "/regen");
	cgs.media.heartShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/selectedhealth");

#endif

	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel(
		Pshieldmodels "/shield");
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip(
		"medal_impressive");
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip(
		"medal_excellent");
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip(
		"medal_gauntlet");
	cgs.media.medalDefend	= trap_R_RegisterShaderNoMip("medal_defend");
	cgs.media.medalAssist	= trap_R_RegisterShaderNoMip("medal_assist");
	cgs.media.medalCapture	= trap_R_RegisterShaderNoMip("medal_capture");


	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	/* only register the items that the server says we need */
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for(i = 1; i < bg_numItems; i++)
		if(items[ i ] == '1' || cg_buildScript.integer){
			CG_LoadingItem(i);
			CG_RegisterItemVisuals(i);
		}

	/* wall marks */
	cgs.media.bulletMarkShader = trap_R_RegisterShader(
		Pdmgart "/bullet_mrk");
	cgs.media.burnMarkShader = trap_R_RegisterShader(
		Pdmgart "/burn_med_mrk");
	cgs.media.holeMarkShader = trap_R_RegisterShader(
		Pdmgart "/hole_lg_mrk");
	cgs.media.energyMarkShader = trap_R_RegisterShader(
		Pdmgart "/plasma_mrk");
	cgs.media.shadowMarkShader	= trap_R_RegisterShader("markShadow");
	cgs.media.wakeMarkShader	= trap_R_RegisterShader("wake");
	cgs.media.bloodMarkShader	= trap_R_RegisterShader("bloodMark");

	/* register the inline models */
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for(i = 1; i < cgs.numInlineModels; i++){
		char	name[10];
		vec3_t mins, maxs;
		int	j;

		Q_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for(j = 0; j < 3; j++)
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 *
							 (maxs[j] - mins[j]);
	}

	/* register all the server specified models */
	for(i=1; i<MAX_MODELS; i++){
		const char *modelName;

		modelName = CG_ConfigString(CS_MODELS+i);
		if(!modelName[0])
			break;
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

#ifdef MISSIONPACK
	/* new stuff */
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/patrol");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/assault");
	cgs.media.campShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/camp");
	cgs.media.followShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/follow");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/defend");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/team_leader");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/retrieve");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/escort");
	cgs.media.cursor = trap_R_RegisterShaderNoMip(Pmenuart "/3_cursor2");
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip(
		"ui/assets/sizecursor");
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip(
		"ui/assets/selectcursor");
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/flag_in_base");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/flag_capture");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip(
		"ui/assets/statusbar/flag_missing");

	trap_R_RegisterModel(Pplayermodels "/james/lower");
	trap_R_RegisterModel(Pplayermodels "/james/upper");
	trap_R_RegisterModel(Pplayermodels "/heads/james/james");

	trap_R_RegisterModel(Pplayermodels "/janet/lower");
	trap_R_RegisterModel(Pplayermodels "/janet/upper");
	trap_R_RegisterModel(Pplayermodels "/heads/janet/janet");

#endif
	CG_ClearParticles();
/*
 *      for (i=1; i<MAX_PARTICLES_AREAS; i++)
 *      {
 *              {
 *                      int rval;
 *
 *                      rval = CG_NewParticleArea ( CS_PARTICLES + i);
 *                      if (!rval)
 *                              break;
 *              }
 *      }
 */
}

/*
 * CG_BuildSpectatorString
 *
 */
void
CG_BuildSpectatorString(void)
{
	int i;
	cg.spectatorList[0] = 0;
	for(i = 0; i < MAX_CLIENTS; i++)
		if(cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team ==
		   TEAM_SPECTATOR)
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList),
				va("%s     ", cgs.clientinfo[i].name));
	i = strlen(cg.spectatorList);
	if(i != cg.spectatorLen){
		cg.spectatorLen		= i;
		cg.spectatorWidth	= -1;
	}
}


/*
 * CG_RegisterClients
 */
static void
CG_RegisterClients(void)
{
	int i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for(i=0; i<MAX_CLIENTS; i++){
		const char *clientInfo;

		if(cg.clientNum == i)
			continue;

		clientInfo = CG_ConfigString(CS_PLAYERS+i);
		if(!clientInfo[0])
			continue;
		CG_LoadingClient(i);
		CG_NewClientInfo(i);
	}
	CG_BuildSpectatorString();
}

/* =========================================================================== */

/*
 * CG_ConfigString
 */
const char *
CG_ConfigString(int index)
{
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		CG_Error("CG_ConfigString: bad index: %i", index);
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

/* ================================================================== */

/*
 * CG_StartMusic
 *
 */
void
CG_StartMusic(void)
{
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	/* start the background music */
	s = (char*)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, Q_ReadToken(&s), sizeof(parm1));
	Q_strncpyz(parm2, Q_ReadToken(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}
#ifdef MISSIONPACK
char *
CG_GetMenuBuffer(const char *filename)
{
	int len;
	fileHandle_t	f;
	static char	buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f){
		trap_Print(va(S_COLOR_RED
				"menu file not found: %s, using default\n",
				filename));
		return NULL;
	}
	if(len >= MAX_MENUFILE){
		trap_Print(va(S_COLOR_RED
				"menu file too large: %s is %i, max allowed is %i\n",
				filename,
				len, MAX_MENUFILE));
		trap_FS_FCloseFile(f);
		return NULL;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	return buf;
}

/*
 * new hud stuff ( mission pack )
 *  */
qbool
CG_Asset_Parse(int handle)
{
	pc_token_t	token;
	const char	*tempStr;

	if(!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if(Q_stricmp(token.string, "{") != 0)
		return qfalse;

	while(1){
		if(!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if(Q_stricmp(token.string, "}") == 0)
			return qtrue;

		/* font */
		if(Q_stricmp(token.string, "font") == 0){
			int pointSize;
			if(!PC_String_Parse(handle,
				   &tempStr) || !PC_Int_Parse(handle, &pointSize))
				return qfalse;
			cgDC.registerFont(tempStr, pointSize,
				&cgDC.Assets.textFont);
			continue;
		}

		/* smallFont */
		if(Q_stricmp(token.string, "smallFont") == 0){
			int pointSize;
			if(!PC_String_Parse(handle,
				   &tempStr) || !PC_Int_Parse(handle, &pointSize))
				return qfalse;
			cgDC.registerFont(tempStr, pointSize,
				&cgDC.Assets.smallFont);
			continue;
		}

		/* font */
		if(Q_stricmp(token.string, "bigfont") == 0){
			int pointSize;
			if(!PC_String_Parse(handle,
				   &tempStr) || !PC_Int_Parse(handle, &pointSize))
				return qfalse;
			cgDC.registerFont(tempStr, pointSize,
				&cgDC.Assets.bigFont);
			continue;
		}

		/* gradientbar */
		if(Q_stricmp(token.string, "gradientbar") == 0){
			if(!PC_String_Parse(handle, &tempStr))
				return qfalse;
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(
				tempStr);
			continue;
		}

		/* enterMenuSound */
		if(Q_stricmp(token.string, "menuEnterSound") == 0){
			if(!PC_String_Parse(handle, &tempStr))
				return qfalse;
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound(
				tempStr, qfalse);
			continue;
		}

		/* exitMenuSound */
		if(Q_stricmp(token.string, "menuExitSound") == 0){
			if(!PC_String_Parse(handle, &tempStr))
				return qfalse;
			cgDC.Assets.menuExitSound = trap_S_RegisterSound(tempStr,
				qfalse);
			continue;
		}

		/* itemFocusSound */
		if(Q_stricmp(token.string, "itemFocusSound") == 0){
			if(!PC_String_Parse(handle, &tempStr))
				return qfalse;
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound(
				tempStr, qfalse);
			continue;
		}

		/* menuBuzzSound */
		if(Q_stricmp(token.string, "menuBuzzSound") == 0){
			if(!PC_String_Parse(handle, &tempStr))
				return qfalse;
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound(tempStr,
				qfalse);
			continue;
		}

		if(Q_stricmp(token.string, "cursor") == 0){
			if(!PC_String_Parse(handle, &cgDC.Assets.cursorStr))
				return qfalse;
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip(
				cgDC.Assets.cursorStr);
			continue;
		}

		if(Q_stricmp(token.string, "fadeClamp") == 0){
			if(!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp))
				return qfalse;
			continue;
		}

		if(Q_stricmp(token.string, "fadeCycle") == 0){
			if(!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle))
				return qfalse;
			continue;
		}

		if(Q_stricmp(token.string, "fadeAmount") == 0){
			if(!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount))
				return qfalse;
			continue;
		}

		if(Q_stricmp(token.string, "shadowX") == 0){
			if(!PC_Float_Parse(handle, &cgDC.Assets.shadowX))
				return qfalse;
			continue;
		}

		if(Q_stricmp(token.string, "shadowY") == 0){
			if(!PC_Float_Parse(handle, &cgDC.Assets.shadowY))
				return qfalse;
			continue;
		}

		if(Q_stricmp(token.string, "shadowColor") == 0){
			if(!PC_Color_Parse(handle, &cgDC.Assets.shadowColor))
				return qfalse;
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse;
}

void
CG_ParseMenu(const char *menuFile)
{
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource(menuFile);
	if(!handle)
		handle = trap_PC_LoadSource("ui/testhud.menu");
	if(!handle)
		return;

	while(1){
		if(!trap_PC_ReadToken(handle, &token))
			break;

		/* if ( Q_stricmp( token, "{" ) ) {
		 * Com_Printf( "Missing { in menu file\n" );
		 * break;
		 * } */

		/* if ( menuCount == MAX_MENUS ) {
		 * Com_Printf( "Too many menus!\n" );
		 * break;
		 * } */

		if(token.string[0] == '}')
			break;

		if(Q_stricmp(token.string, "assetGlobalDef") == 0){
			if(CG_Asset_Parse(handle))
				continue;
			else
				break;
		}


		if(Q_stricmp(token.string, "menudef") == 0)
			/* start a new menu */
			Menu_New(handle);
	}
	trap_PC_FreeSource(handle);
}

qbool
CG_Load_Menu(char **p)
{
	char *token;

	token = Q_ReadTokenExt(p, qtrue);

	if(token[0] != '{')
		return qfalse;

	while(1){

		token = Q_ReadTokenExt(p, qtrue);

		if(Q_stricmp(token, "}") == 0)
			return qtrue;

		if(!token || token[0] == 0)
			return qfalse;

		CG_ParseMenu(token);
	}
	return qfalse;
}



void
CG_LoadMenus(const char *menuFile)
{
	char	*token;
	char	*p;
	int	len, start;
	fileHandle_t	f;
	static char	buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile(menuFile, &f, FS_READ);
	if(!f){
		Com_Printf(
			S_COLOR_YELLOW
			"menu file not found: %s, using default\n",
			menuFile);
		len = trap_FS_FOpenFile("ui/hud.txt", &f, FS_READ);
		if(!f)
			trap_Error(va(S_COLOR_RED
					"default menu file not found: ui/hud.txt, unable to continue!\n"));
	}

	if(len >= MAX_MENUDEFFILE){
		trap_Error(va(S_COLOR_RED
				"menu file too large: %s is %i, max allowed is %i\n",
				menuFile,
				len, MAX_MENUDEFFILE));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	Q_Compress(buf);

	Menu_Reset();

	p = buf;

	while(1){
		token = Q_ReadTokenExt(&p, qtrue);
		if(!token || token[0] == 0 || token[0] == '}')
			break;

		/* if ( Q_stricmp( token, "{" ) ) {
		 * Com_Printf( "Missing { in menu file\n" );
		 * break;
		 * } */

		/* if ( menuCount == MAX_MENUS ) {
		 * Com_Printf( "Too many menus!\n" );
		 * break;
		 * } */

		if(Q_stricmp(token, "}") == 0)
			break;

		if(Q_stricmp(token, "loadmenu") == 0){
			if(CG_Load_Menu(&p))
				continue;
			else
				break;
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n",
		trap_Milliseconds() - start);

}



static qbool
CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key)
{
	return qfalse;
}


static int
CG_FeederCount(float feederID)
{
	int i, count;
	count = 0;
	if(feederID == FEEDER_REDTEAM_LIST){
		for(i = 0; i < cg.numScores; i++)
			if(cg.scores[i].team == TEAM_RED)
				count++;
	}else if(feederID == FEEDER_BLUETEAM_LIST){
		for(i = 0; i < cg.numScores; i++)
			if(cg.scores[i].team == TEAM_BLUE)
				count++;
	}else if(feederID == FEEDER_SCOREBOARD)
		return cg.numScores;
	return count;
}


void
CG_SetScoreSelection(void *p)
{
	menuDef_t *menu		= (menuDef_t*)p;
	playerState_t *ps	= &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for(i = 0; i < cg.numScores; i++){
		if(cg.scores[i].team == TEAM_RED)
			red++;
		else if(cg.scores[i].team == TEAM_BLUE)
			blue++;
		if(ps->clientNum == cg.scores[i].client)
			cg.selectedScore = i;
	}

	if(menu == NULL)
		/* just interested in setting the selected score */
		return;

	if(cgs.gametype >= GT_TEAM){
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if(cg.scores[cg.selectedScore].team == TEAM_BLUE){
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	}else
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD,
			cg.selectedScore,
			NULL);
}

/* FIXME: might need to cache this info */
static clientInfo_t *
CG_InfoFromScoreIndex(int index, int team, int *scoreIndex)
{
	int i, count;
	if(cgs.gametype >= GT_TEAM){
		count = 0;
		for(i = 0; i < cg.numScores; i++)
			if(cg.scores[i].team == team){
				if(count == index){
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].
							       client];
				}
				count++;
			}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *
CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle)
{
	gitem_t *item;
	int	scoreIndex	= 0;
	clientInfo_t *info	= NULL;
	int	team	= -1;
	score_t *sp	= NULL;

	*handle = -1;

	if(feederID == FEEDER_REDTEAM_LIST)
		team = TEAM_RED;
	else if(feederID == FEEDER_BLUETEAM_LIST)
		team = TEAM_BLUE;

	info	= CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp	= &cg.scores[scoreIndex];

	if(info && info->infoValid){
		switch(column){
		case 0:
			if(info->powerups & (1 << PW_NEUTRALFLAG)){
				item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
				*handle = cg_items[ ITEM_INDEX(item) ].icon;
			}else if(info->powerups & (1 << PW_REDFLAG)){
				item = BG_FindItemForPowerup(PW_REDFLAG);
				*handle = cg_items[ ITEM_INDEX(item) ].icon;
			}else if(info->powerups & (1 << PW_BLUEFLAG)){
				item = BG_FindItemForPowerup(PW_BLUEFLAG);
				*handle = cg_items[ ITEM_INDEX(item) ].icon;
			}else{
				if(info->botSkill > 0 && info->botSkill <= 5)
					*handle =
						cgs.media.botSkillShaders[ info
									   ->
									   botSkill
									   - 1 ];
				else if(info->handicap < 100)
					return va("%i", info->handicap);
			}
			break;
		case 1:
			if(team == -1)
				return "";
			else
				*handle = CG_StatusHandle(info->teamTask);
			break;
		case 2:
			if(cg.snap->ps.stats[ STAT_CLIENTS_READY ] &
			   (1 << sp->client))
				return "Ready";
			if(team == -1){
				if(cgs.gametype == GT_TOURNAMENT)
					return va("%i/%i", info->wins,
						info->losses);
				else if(info->infoValid && info->team ==
					TEAM_SPECTATOR)
					return "Spectator";
				else
					return "";
			}else if(info->teamLeader)
				return "Leader";

			break;
		case 3:
			return info->name;
			break;
		case 4:
			return va("%i", info->score);
			break;
		case 5:
			return va("%4i", sp->time);
			break;
		case 6:
			if(sp->ping == -1)
				return "connecting";
			return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t
CG_FeederItemImage(float feederID, int index)
{
	return 0;
}

static void
CG_FeederSelection(float feederID, int index)
{
	if(cgs.gametype >= GT_TEAM){
		int	i, count;
		int	team =
			(feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for(i = 0; i < cg.numScores; i++)
			if(cg.scores[i].team == team){
				if(index == count)
					cg.selectedScore = i;
				count++;
			}
	}else
		cg.selectedScore = index;
}
#endif

#ifdef MISSIONPACK
static float
CG_Cvar_Get(const char *cvar)
{
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}
#endif

#ifdef MISSIONPACK
void
CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color,
			const char *text, int cursorPos, char cursor, int limit,
			int style)
{
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int
CG_OwnerDrawWidth(int ownerDraw, float scale)
{
	switch(ownerDraw){
	case CG_GAME_TYPE:
		return CG_Text_Width(CG_GameTypeString(), scale, 0);
	case CG_GAME_STATUS:
		return CG_Text_Width(CG_GetGameStatusText(), scale, 0);
		break;
	case CG_KILLER:
		return CG_Text_Width(CG_GetKillerText(), scale, 0);
		break;
	case CG_RED_NAME:
		return CG_Text_Width(cg_redTeamName.string, scale, 0);
		break;
	case CG_BLUE_NAME:
		return CG_Text_Width(cg_blueTeamName.string, scale, 0);
		break;


	}
	return 0;
}

static int
CG_PlayCinematic(const char *name, float x, float y, float w, float h)
{
	return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void
CG_StopCinematic(int handle)
{
	trap_CIN_StopCinematic(handle);
}

static void
CG_DrawCinematic(int handle, float x, float y, float w, float h)
{
	trap_CIN_SetExtents(handle, x, y, w, h);
	trap_CIN_DrawCinematic(handle);
}

static void
CG_RunCinematicFrame(int handle)
{
	trap_CIN_RunCinematic(handle);
}

/*
 * CG_LoadHudMenu();
 *
 */
void
CG_LoadHudMenu(void)
{
	char buff[1024];
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic	= &CG_DrawPic;
	cgDC.drawStretchPic	= &trap_R_DrawStretchPic;
	cgDC.drawText	= &CG_Text_Paint;
	cgDC.textWidth	= &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds	= &trap_R_ModelBounds;
	cgDC.fillRect		= &CG_FillRect;
	cgDC.drawRect		= &CG_DrawRect;
	cgDC.drawSides		= &CG_DrawSides;
	cgDC.drawTopBottom	= &CG_DrawTopBottom;
	cgDC.clearScene		= &trap_R_ClearScene;
	cgDC.addRefEntityToScene	= &trap_R_AddRefEntityToScene;
	cgDC.renderScene		= &trap_R_RenderScene;
	cgDC.registerFont		= &trap_R_RegisterFont;
	cgDC.ownerDrawItem		= &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString	= trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue	= CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	/* cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	 * cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode; */
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage	= &CG_FeederItemImage;
	cgDC.feederItemText	= &CG_FeederItemText;
	cgDC.feederSelection	= &CG_FeederSelection;
	/* cgDC.setBinding = &trap_Key_SetBinding;
	 * cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	 * cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	 * cgDC.executeText = &trap_Cmd_ExecuteText; */
	cgDC.Error	= &Com_Errorf;
	cgDC.Print	= &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	/* cgDC.Pause = &CG_Pause; */
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack	= &trap_S_StopBackgroundTrack;
	cgDC.playCinematic		= &CG_PlayCinematic;
	cgDC.stopCinematic		= &CG_StopCinematic;
	cgDC.drawCinematic		= &CG_DrawCinematic;
	cgDC.runCinematicFrame		= &CG_RunCinematicFrame;

	Init_Display(&cgDC);

	Menu_Reset();

	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if(hudSet[0] == '\0')
		hudSet = "ui/hud.txt";

	CG_LoadMenus(hudSet);
}

void
CG_AssetCache(void)
{
	/* if (Assets.textFont == NULL) {
	 *  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	 * }
	 * Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	 * Com_Printf("Menu Size: %i bytes\n", sizeof(Menus)); */
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(ASSET_GRADIENTBAR);
	cgDC.Assets.fxBasePic	= trap_R_RegisterShaderNoMip(ART_FX_BASE);
	cgDC.Assets.fxPic[0]	= trap_R_RegisterShaderNoMip(ART_FX_RED);
	cgDC.Assets.fxPic[1]	= trap_R_RegisterShaderNoMip(ART_FX_YELLOW);
	cgDC.Assets.fxPic[2]	= trap_R_RegisterShaderNoMip(ART_FX_GREEN);
	cgDC.Assets.fxPic[3]	= trap_R_RegisterShaderNoMip(ART_FX_TEAL);
	cgDC.Assets.fxPic[4]	= trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	cgDC.Assets.fxPic[5]	= trap_R_RegisterShaderNoMip(ART_FX_CYAN);
	cgDC.Assets.fxPic[6]	= trap_R_RegisterShaderNoMip(ART_FX_WHITE);
	cgDC.Assets.scrollBar	= trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR);
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip(
		ASSET_SCROLLBAR_ARROWDOWN);
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip(
		ASSET_SCROLLBAR_ARROWUP);
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip(
		ASSET_SCROLLBAR_ARROWLEFT);
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip(
		ASSET_SCROLLBAR_ARROWRIGHT);
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip(
		ASSET_SCROLL_THUMB);
	cgDC.Assets.sliderBar	= trap_R_RegisterShaderNoMip(ASSET_SLIDER_BAR);
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip(ASSET_SLIDER_THUMB);
}
#endif
/*
 * CG_Init
 *
 * Called after every level change or subsystem restart
 * Will perform callbacks to make the loading info screen update.
 */
void
CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum)
{
	const char *s;

	/* clear everything */
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum	= serverMessageNum;
	cgs.serverCommandSequence	= serverCommandSequence;

	/* load a few needed things before we do any screen updates */
	cgs.media.charsetShader = trap_R_RegisterShader(
		P2dart "/bigchars");
	cgs.media.whiteShader = trap_R_RegisterShader("white");
	cgs.media.charsetProp = trap_R_RegisterShaderNoMip(
		Pmenuart "/font1_prop");
	cgs.media.charsetPropGlow = trap_R_RegisterShaderNoMip(
		Pmenuart "/font1_prop_glo");
	cgs.media.charsetPropB = trap_R_RegisterShaderNoMip(
		Pmenuart "/font2_prop");

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1;	/* For compatibily, default to unset for */
	cgs.flagStatus = -1;
	/* old servers */

	/* get the rendering configuration from the client system */
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale	= cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale	= cgs.glconfig.vidHeight / 480.0;

	/* get the gamestate from the client system */
	trap_GetGameState(&cgs.gameState);

	/* check version */
	s = CG_ConfigString(CS_GAME_VERSION);
	if(strcmp(s, GAME_VERSION))
		CG_Error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);

	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	CG_ParseServerinfo();

	/* load the new map */
	CG_LoadingString("collision map");

	trap_CM_LoadMap(cgs.mapname);

#ifdef MISSIONPACK
	String_Init();
#endif

	cg.loading = qtrue;	/* force players to load instead of defer */

	CG_LoadingString("sounds");

	CG_RegisterSounds();

	CG_LoadingString("graphics");

	CG_RegisterGraphics();

	CG_LoadingString("clients");

	CG_RegisterClients();	/* if low on memory, some clients will be deferred */

#ifdef MISSIONPACK
	CG_AssetCache();
	CG_LoadHudMenu();	/* load new hud stuff */
#endif

	cg.loading = qfalse;	/* future players will be deferred */

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	/* remove the last loading update */
	cg.infoScreenText[0] = 0;

	/* Make sure we have update values (scores) */
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString("");

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds(qtrue);
}

/*
 * CG_Shutdown
 *
 * Called before every level change or subsystem restart
 */
void
CG_Shutdown(void)
{
	/* some mods may need to do cleanup work here,
	 * like closing files or archiving session data */
}


/*
 * CG_EventHandling
 * type 0 - no event handling
 *    1 - team menu
 *    2 - hud editor
 *
 */
#ifndef MISSIONPACK
void
CG_EventHandling(int type)
{
}



void
CG_KeyEvent(int key, qbool down)
{
}

void
CG_MouseEvent(int x, int y)
{
}
#endif
