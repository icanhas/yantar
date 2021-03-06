/* handle the media and animation for player entities */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "local.h"

char *cg_customSoundNames[] = {
	"*death1",
	"*death2",
	"*death3",
	"*jump1",
	"*pain25_1",
	"*pain50_1",
	"*pain75_1",
	"*pain100_1",
	"*falling1",
	"*gasp",
	"*drown",
	"*fall1",
	"*taunt",
	nil
};

Handle
CG_CustomSound(int cnum, const char *soundname)
{
	Clientinfo *ci;
	char **p;
	int i;

	if(soundname[0] != '*')
		return trap_sndregister(soundname, qfalse);

	if(cnum < 0 || cnum >= MAX_CLIENTS)
		cnum = 0;
	ci = &cgs.clientinfo[cnum];

	p = cg_customSoundNames;
	/* N.B.: extensions will need to be stripped if using full filenames */
	for(i = 0; (i < MAX_CUSTOM_SOUNDS) && (*p != nil); i++, p++)
		if(!strcmp(soundname, *p))
			return ci->sounds[i];
	CG_Error("Unknown custom sound: %s", soundname);
	return 0;
}

/*
 * Client info
 */

/*
 * Read a configuration file containing animation counts and rates
 * (models/players/visor/animation.cfg, etc)
 */
static qbool
parseanimfile(const char *filename, Clientinfo *ci)
{
	char txt[20000], *txtp, *prev, *tok;
	int i, len, skip;
	float fps;
	Fhandle f;
	Anim *p, *anims;

	/* load the file */
	len = trap_fsopen(filename, &f, FS_READ);
	if(len <= 0)
		return qfalse;
	if(len >= sizeof(txt) - 1){
		CG_Printf("File %s too long\n", filename);
		trap_fsclose(f);
		return qfalse;
	}
	trap_fsread(txt, len, f);
	txt[len] = 0;
	trap_fsclose(f);

	/* parse the text */
	txtp = txt;
	skip = 0;

	ci->footsteps = FOOTSTEP_NORMAL;
	clearv3(ci->headOffset);
	ci->fixedlegs	= qfalse;
	ci->fixedtorso	= qfalse;

	/* read optional parameters */
	for(;;){
		prev = txtp;	/* so we can unget */
		tok = Q_readtok(&txtp);
		if(!tok)
			break;
		if(!Q_stricmp(tok, "footsteps")){
			tok = Q_readtok(&txtp);
			if(!tok)
				break;
			if(!Q_stricmp(tok, "default") || !Q_stricmp(tok, "normal"))
				ci->footsteps = FOOTSTEP_NORMAL;
			else if(!Q_stricmp(tok, "boot"))
				ci->footsteps = FOOTSTEP_BOOT;
			else if(!Q_stricmp(tok, "flesh"))
				ci->footsteps = FOOTSTEP_FLESH;
			else if(!Q_stricmp(tok, "mech"))
				ci->footsteps = FOOTSTEP_MECH;
			else if(!Q_stricmp(tok, "energy"))
				ci->footsteps = FOOTSTEP_ENERGY;
			else
				CG_Printf("Bad footsteps parm in %s: %s\n",
					filename,
					tok);
			continue;
		}else if(!Q_stricmp(tok, "headoffset")){
			for(i = 0; i < 3; i++){
				tok = Q_readtok(&txtp);
				if(!tok)
					break;
				ci->headOffset[i] = atof(tok);
			}
			continue;
		}else if(!Q_stricmp(tok, "fixedlegs")){
			ci->fixedlegs = qtrue;
			continue;
		}else if(!Q_stricmp(tok, "fixedtorso")){
			ci->fixedtorso = qtrue;
			continue;
		}

		/* if it is a number, start parsing animations */
		if(tok[0] >= '0' && tok[0] <= '9'){
			txtp = prev;	/* unget the tok */
			break;
		}
		comprintf("unknown tok '%s' is %s\n", tok, filename);
	}

	anims = ci->animations;
	/* read information for each frame */
	for(i = 0, p = anims; i < MAX_ANIMATIONS; i++, p++){
		Anim *p2;
		
		tok = Q_readtok(&txtp);
		if(!*tok){
			if(i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE){
				p2 = &anims[TORSO_GESTURE];
				p->firstFrame = p2->firstFrame;
				p->frameLerp = p2->frameLerp;
				p->initialLerp = p2->initialLerp;
				p->loopFrames = p2->loopFrames;
				p->numFrames = p2->numFrames;
				p->reversed = qfalse;
				p->flipflop = qfalse;
				continue;
			}
			break;
		}
		p->firstFrame = atoi(tok);
		/* leg only frames are adjusted to not count the upper body only frames */
		if(i == LEGS_WALKCR)
			skip = anims[LEGS_WALKCR].firstFrame 
				- anims[TORSO_GESTURE].firstFrame;
		if(i >= LEGS_WALKCR && i<TORSO_GETFLAG)
			p->firstFrame -= skip;

		tok = Q_readtok(&txtp);
		if(!*tok)
			break;
		p->numFrames = atoi(tok);

		p->reversed = qfalse;
		p->flipflop = qfalse;
		/* if numFrames is negative the animation is reversed */
		if(p->numFrames < 0){
			p->numFrames = -p->numFrames;
			p->reversed = qtrue;
		}

		tok = Q_readtok(&txtp);
		if(!*tok)
			break;
		p->loopFrames = atoi(tok);

		tok = Q_readtok(&txtp);
		if(!*tok)
			break;
		fps = atof(tok);
		if(fps == 0)
			fps = 1;
		p->frameLerp = 1000 / fps;
		p->initialLerp = 1000 / fps;
	}

	if(i != MAX_ANIMATIONS){
		CG_Printf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}

	/* crouch backward animation */
	memcpy(&anims[LEGS_BACKCR], &anims[LEGS_WALKCR],
		sizeof(Anim));
	anims[LEGS_BACKCR].reversed = qtrue;
	/* walk backward animation */
	memcpy(&anims[LEGS_BACKWALK], &anims[LEGS_WALK], 
		sizeof(Anim));
	anims[LEGS_BACKWALK].reversed = qtrue;
	/* flag moving fast */
	p = &anims[FLAG_RUN];
	p->firstFrame = 0;
	p->numFrames = 16;
	p->loopFrames = 16;
	p->frameLerp = 1000 / 15;
	p->initialLerp = 1000 / 15;
	p->reversed = qfalse;
	/* flag not moving or moving slowly */
	p = &anims[FLAG_STAND];
	p->firstFrame = 16;
	p->numFrames = 5;
	p->loopFrames = 0;
	p->frameLerp = 1000 / 20;
	p->initialLerp = 1000 / 20;
	p->reversed = qfalse;
	/* flag speeding up */
	p = &anims[FLAG_STAND2RUN];
	p->firstFrame = 16;
	p->numFrames = 5;
	p->loopFrames = 1;
	p->frameLerp = 1000 / 15;
	p->initialLerp = 1000 / 15;
	p->reversed = qtrue;
	/*
	 * new anims changes
	 */
	/* anims[TORSO_GETFLAG].flipflop = qtrue;
	 * anims[TORSO_GUARDBASE].flipflop = qtrue;
	 * animns[TORSO_PATROL].flipflop = qtrue;
	 * anims[TORSO_AFFIRMATIVE].flipflop = qtrue;
	 * anims[TORSO_NEGATIVE].flipflop = qtrue; */
	return qtrue;
}

static qbool
fileexists(const char *name)
{
	int len;

	len = trap_fsopen(name, NULL, FS_READ);
	if(len>0)
		return qtrue;
	return qfalse;
}

static qbool
findclientmodelfile(char *out, int len, Clientinfo *ci,
	const char *team, const char *model, const char *skin, 
	const char *base, const char *ext)
{
	char *teamcolour;
	int i;

	if(cgs.gametype >= GT_TEAM){
		switch(ci->team){
		case TEAM_BLUE:
			teamcolour = "blue";
			break;
		default: 
			teamcolour = "red";
			break;
		}
	}else
		team = "default";
		
	for(i = 0; i < 2; i++){
		if(i == 0 && team && *team)
			/* e.g. "vis/models/players/james/stroggs/lower_lily_red.skin" */
			Q_sprintf(out, len, Pplayermodels "/%s/%s/%s_%s_%s.%s",
				model, team, base, skin, teamcolour, ext);
		else
			/* e.g. "vis/models/players/james/lower_lily_red.skin" */
			Q_sprintf(out, len, Pplayermodels "/%s/%s_%s_%s.%s",
				model, base, skin, teamcolour, ext);
		if(fileexists(out))
			return qtrue;
		if(cgs.gametype >= GT_TEAM){
			if(i == 0 && team && *team)
				/* e.g. "vis/models/players/james/stroggs/lower_red.skin" */
				Q_sprintf(out, len, Pplayermodels "/%s/%s/%s_%s.%s",
					model, team, base, teamcolour, ext);	/* FIXME */
			else
				/* e.g. "vis/models/players/james/lower_red.skin" */
				Q_sprintf(out, len, Pplayermodels "/%s/%s_%s.%s",
					model, base, teamcolour, ext);
		}else{
			if(i == 0 && team && *team)
				/* e.g "vis/models/players/james/stroggs/lower_lily.skin" */
				Q_sprintf(out, len, Pplayermodels "/%s/%s/%s_%s.%s",
					model, team, base, skin, ext);	/* FIXME */
			else
				/* e.g. "vis/models/players/james/lower_lily.skin" */
				Q_sprintf(out, len, Pplayermodels "/%s/%s_%s.%s",
					model, base, skin, ext);
		}
		if(fileexists(out))
			return qtrue;
		if(!team || !*team)
			break;
	}
	return qfalse;
}

static qbool
regclientskin(Clientinfo *ci, const char *team, const char *model, 
	const char *skin)
{
	char fname[MAX_QPATH];
	
	*fname = '\0';
	if(findclientmodelfile(fname, sizeof fname, ci, team, model, skin, "hull", "skin"))
		ci->hullskin = trap_R_RegisterSkin(fname);
	if(!ci->hullskin){
		comprintf("Head skin load failure: %s\n", fname);
		return qfalse;
	}
	return qtrue;
}

/*
 * register a client's model by name, along with its animation.cfg, 
 * icon, and chosen skin
 */
static qbool
regclientmodel(Clientinfo *ci, const char *name,
	const char *skin, const char *team)
{
	char	fname[MAX_QPATH], newteam[MAX_QPATH];

	Q_sprintf(fname, sizeof fname, Pplayermodels "/%s/hull", name);
	ci->hullmodel = trap_R_RegisterModel(fname);
	if(!ci->hullmodel){
		comprintf("Failed to load model file %s\n", fname);
		return qfalse;
	}

	/* register the skin */
	if(!regclientskin(ci, team, name, skin)){
		if((team != nil) && (*team != '\0')){
			comprintf("Failed to load skin file: %s : %s : %s\n",
				team, name, skin);
			switch(ci->team){
			case TEAM_BLUE:
				Q_sprintf(newteam, sizeof newteam,
					"%s/", DEFAULT_BLUETEAM_NAME);
				break;
			default:
				Q_sprintf(newteam, sizeof newteam,
					"%s/", DEFAULT_REDTEAM_NAME);
			}
			if(!regclientskin(ci, newteam, name, skin)){
				comprintf("Failed to load skin file: %s : %s : %s\n",
					newteam, name, skin);
				return qfalse;
			}
		}else{
			comprintf("Failed to load skin file: %s : %s\n",
				name, skin);
			return qfalse;
		}
	}

	/* load the animations */
	Q_sprintf(fname, sizeof fname, Pplayermodels "/%s/animation.cfg", name);
	if(!parseanimfile(fname, ci)){
		comprintf("Failed to load animation file %s\n", fname);
		return qfalse;
	}

	/* load the icon */
	/* FIXME: account for teams/skins etc. */
	Q_sprintf(fname, sizeof fname, Pplayermodels "/%s/icon_default", name);
	ci->modelIcon = trap_R_RegisterShaderNoMip(fname);
	if(ci->modelIcon <= 0){
		/* use the nomen nescio icon */
		ci->modelIcon = trap_R_RegisterShaderNoMip(Picons "/anon");
	}
	if(ci->modelIcon <= 0)
		return qfalse;
	return qtrue;
}

static void
colourfromstr(const char *v, Vec3 color)
{
	int val;

	clearv3(color);
	val = atoi(v);
	if(val < 1 || val > 7){
		setv3(color, 1, 1, 1);
		return;
	}
	if(val & 1)
		color[2] = 1.0f;
	if(val & 2)
		color[1] = 1.0f;
	if(val & 4)
		color[0] = 1.0f;
}

/*
 * Load it now, taking the disk hits.
 * This will usually be deferred to a safe time
 */
static void
loadclientinfo(int clientNum, Clientinfo *ci)
{
	const char *dir, *fallback;
	int i, modelloaded;
	const char *s;
	char teamname[MAX_QPATH];

	teamname[0] = 0;
	if(cgs.gametype >= GT_TEAM){
		if(ci->team == TEAM_BLUE)
			Q_strncpyz(teamname, cg_blueTeamName.string,
				sizeof(teamname));
		else
			Q_strncpyz(teamname, cg_redTeamName.string,
				sizeof(teamname));
	}
	if(teamname[0])
		strcat(teamname, "/");
	modelloaded = qtrue;
	if(!regclientmodel(ci, ci->modelName, ci->skinName, teamname)){
		if(cg_buildScript.integer)
			CG_Error(
				"regclientmodel(%s, %s, %s) failed",
				ci->modelName, ci->skinName, teamname);

		/* fall back to default team name */
		if(cgs.gametype >= GT_TEAM){
			/* keep skin name */
			if(ci->team == TEAM_BLUE)
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME,
					sizeof(teamname));
			else
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME,
					sizeof(teamname));
			if(!regclientmodel(ci, DEFAULT_TEAM_MODEL,
				   ci->skinName, teamname))
				CG_Error("DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register",
					DEFAULT_TEAM_MODEL, ci->skinName);
		}else if(!regclientmodel(ci, DEFAULT_MODEL,
				 "default", teamname))
			CG_Error("DEFAULT_MODEL (%s) failed to register",
				DEFAULT_MODEL);

		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
	if(ci->hullmodel){
		Orient tag;
		/* if the body model has the "tag_flag" */
		if(trap_R_LerpTag(&tag, ci->hullmodel, 0, 0, 1, "tag_flag"))
			ci->newAnims = qtrue;
	}

	/* sounds */
	dir = ci->modelName;
	fallback =
		(cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for(i = 0; i < MAX_CUSTOM_SOUNDS; i++){
		s = cg_customSoundNames[i];
		if(!s)
			break;
		ci->sounds[i] = 0;
		/* if the model didn't load use the sounds of the default model */
		if(modelloaded)
			ci->sounds[i] = trap_sndregister(
				va(Pplayersounds "/%s/%s", dir, s+1), qfalse);
		if(!ci->sounds[i])
			ci->sounds[i] = trap_sndregister(
				va(Pplayersounds "/%s/%s", fallback, s+1), qfalse);
	}
	ci->deferred = qfalse;

	/* 
	 * reset any existing players and bodies, because they might be in bad
	 * frames for this new model 
	 */
	for(i = 0; i < MAX_GENTITIES; i++)
		if(cg_entities[i].currentState.clientNum == clientNum
		   && cg_entities[i].currentState.eType == ET_PLAYER)
			CG_ResetPlayerEntity(&cg_entities[i]);
}

/* copy model info */
static void
cpclientinfomodel(Clientinfo *from, Clientinfo *to)
{
	copyv3(from->headOffset, to->headOffset);
	to->footsteps = from->footsteps;
	to->hullmodel = from->hullmodel;
	to->hullskin = from->hullskin;
	to->modelIcon = from->modelIcon;
	to->newAnims = from->newAnims;
	memcpy(to->animations, from->animations, sizeof(to->animations));
	memcpy(to->sounds, from->sounds, sizeof(to->sounds));
}

/*
 * scan for an existing clientinfo that matches ci
 * so we can avoid loading checks if possible 
 */
static qbool
ciexists(Clientinfo *ci)
{
	int i;
	Clientinfo *match;

	for(i = 0; i < cgs.maxclients; i++){
		match = &cgs.clientinfo[ i ];
		if(!match->infoValid)
			continue;
		if(match->deferred)
			continue;
		if(!Q_stricmp(ci->modelName, match->modelName)
		   && !Q_stricmp(ci->skinName, match->skinName)
		   && !Q_stricmp(ci->blueTeam, match->blueTeam)
		   && !Q_stricmp(ci->redTeam, match->redTeam)
		   && (cgs.gametype < GT_TEAM || ci->team == match->team)){
			/* this clientinfo is identical, so use its handles */
			ci->deferred = qfalse;
			cpclientinfomodel(match, ci);
			return qtrue;
		}
	}
	/* nothing matches, so defer the load */
	return qfalse;
}

/*
 * set deferred client info
 * We aren't going to load it now, so grab some other
 * client's info to use until we have some spare time.
 */
static void
setdeferredci(int clientnum, Clientinfo *ci)
{
	Clientinfo *match;
	int i;

	/* 
	 * if someone else is already the same models and skins we
	 * can just load the client info 
	 */
	for(i = 0; i < cgs.maxclients; i++){
		match = &cgs.clientinfo[i];
		if(!match->infoValid || match->deferred)
			continue;
		if(Q_stricmp(ci->skinName, match->skinName) ||
		   Q_stricmp(ci->modelName, match->modelName) ||
		   ((cgs.gametype >= GT_TEAM) && (ci->team != match->team)))
			continue;
		/* just load the real info because it uses the same models and skins */
		loadclientinfo(clientnum, ci);
		return;
	}

	/* if we are in teamplay, only grab a model if the skin is correct */
	if(cgs.gametype >= GT_TEAM){
		for(i = 0; i < cgs.maxclients; i++){
			match = &cgs.clientinfo[ i ];
			if(!match->infoValid || match->deferred)
				continue;
			if(Q_stricmp(ci->skinName, match->skinName) ||
			   (cgs.gametype >= GT_TEAM && ci->team != match->team))
				continue;
			ci->deferred = qtrue;
			cpclientinfomodel(match, ci);
			return;
		}
		/* 
		 * load the full model, because we don't ever want to show
		 * an improper team skin.  This will cause a hitch for the first
		 * player, when the second enters.  Combat shouldn't be going on
		 * yet, so it shouldn't matter
		 */
		loadclientinfo(clientnum, ci);
		return;
	}

	/* find the first valid clientinfo and grab its stuff */
	for(i = 0; i < cgs.maxclients; i++){
		match = &cgs.clientinfo[ i ];
		if(!match->infoValid)
			continue;
		ci->deferred = qtrue;
		cpclientinfomodel(match, ci);
		return;
	}

	/* we should never get here... */
	CG_Printf("setdeferredci: no valid clients!\n");

	loadclientinfo(clientnum, ci);
}


void
CG_NewClientInfo(int clientNum)
{
	Clientinfo *ci, newinfo;
	const char *configstring, *v;
	char *slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if(!configstring[0]){
		memset(ci, 0, sizeof(*ci));
		return;	/* player just left */
	}

	/* build into a temp buffer so the defer checks can use
	 * the old value */
	memset(&newinfo, 0, sizeof(newinfo));

	/* isolate the player's name */
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newinfo.name, v, sizeof(newinfo.name));

	/* colors */
	v = Info_ValueForKey(configstring, "c1");
	colourfromstr(v, newinfo.color1);

	newinfo.c1RGBA[0] = 255 * newinfo.color1[0];
	newinfo.c1RGBA[1] = 255 * newinfo.color1[1];
	newinfo.c1RGBA[2] = 255 * newinfo.color1[2];
	newinfo.c1RGBA[3] = 255;

	v = Info_ValueForKey(configstring, "c2");
	colourfromstr(v, newinfo.color2);

	newinfo.c2RGBA[0] = 255 * newinfo.color2[0];
	newinfo.c2RGBA[1] = 255 * newinfo.color2[1];
	newinfo.c2RGBA[2] = 255 * newinfo.color2[2];
	newinfo.c2RGBA[3] = 255;

	/* bot skill */
	v = Info_ValueForKey(configstring, "skill");
	newinfo.botSkill = atoi(v);

	/* handicap */
	v = Info_ValueForKey(configstring, "hc");
	newinfo.handicap = atoi(v);

	/* wins */
	v = Info_ValueForKey(configstring, "w");
	newinfo.wins = atoi(v);

	/* losses */
	v = Info_ValueForKey(configstring, "l");
	newinfo.losses = atoi(v);

	/* team */
	v = Info_ValueForKey(configstring, "t");
	newinfo.team = atoi(v);

	/* team task */
	v = Info_ValueForKey(configstring, "tt");
	newinfo.teamTask = atoi(v);

	/* team leader */
	v = Info_ValueForKey(configstring, "tl");
	newinfo.teamleader = atoi(v);

	v = Info_ValueForKey(configstring, "g_redteam");
	Q_strncpyz(newinfo.redTeam, v, MAX_TEAMNAME);

	v = Info_ValueForKey(configstring, "g_blueteam");
	Q_strncpyz(newinfo.blueTeam, v, MAX_TEAMNAME);

	/* model */
	v = Info_ValueForKey(configstring, "model");
	if(cg_forceModel.integer){
		/* forcemodel makes everyone use a single model
		 * to prevent load hitches */
		char modelStr[MAX_QPATH];
		char *skin;

		if(cgs.gametype >= GT_TEAM){
			Q_strncpyz(newinfo.modelName, DEFAULT_TEAM_MODEL,
				sizeof(newinfo.modelName));
			Q_strncpyz(newinfo.skinName, "default",
				sizeof(newinfo.skinName));
		}else{
			trap_cvargetstrbuf("model", modelStr,
				sizeof(modelStr));
			if((skin = strchr(modelStr, '/')) == NULL)
				skin = "default";
			else
				*skin++ = 0;

			Q_strncpyz(newinfo.skinName, skin,
				sizeof(newinfo.skinName));
			Q_strncpyz(newinfo.modelName, modelStr,
				sizeof(newinfo.modelName));
		}

		if(cgs.gametype >= GT_TEAM){
			/* keep skin name */
			slash = strchr(v, '/');
			if(slash)
				Q_strncpyz(newinfo.skinName, slash + 1,
					sizeof(newinfo.skinName));
		}
	}else{
		Q_strncpyz(newinfo.modelName, v, sizeof(newinfo.modelName));

		slash = strchr(newinfo.modelName, '/');
		if(!slash)
			/* modelName didn not include a skin name */
			Q_strncpyz(newinfo.skinName, "default",
				sizeof(newinfo.skinName));
		else{
			Q_strncpyz(newinfo.skinName, slash + 1,
				sizeof(newinfo.skinName));
			/* truncate modelName */
			*slash = 0;
		}
	}

#if 0 /////////
	/* head model */
	v = Info_ValueForKey(configstring, "hmodel");
	if(cg_forceModel.integer){
		/* 
		 * forcemodel makes everyone use a single model
		 * to prevent load hitches 
		 */
		char modelStr[MAX_QPATH];
		char *skin;

		if(cgs.gametype >= GT_TEAM){
			Q_strncpyz(newinfo.hullmodelName, DEFAULT_TEAM_HEAD,
				sizeof(newinfo.hullmodelName));
			Q_strncpyz(newinfo.hullskinName, "default",
				sizeof(newinfo.hullskinName));
		}else{
			trap_cvargetstrbuf("headmodel", modelStr,
				sizeof(modelStr));
			if((skin = strchr(modelStr, '/')) == NULL)
				skin = "default";
			else
				*skin++ = 0;

			Q_strncpyz(newinfo.hullskinName, skin,
				sizeof(newinfo.hullskinName));
			Q_strncpyz(newinfo.hullmodelName, modelStr,
				sizeof(newinfo.hullmodelName));
		}

		if(cgs.gametype >= GT_TEAM){
			/* keep skin name */
			slash = strchr(v, '/');
			if(slash)
				Q_strncpyz(newinfo.hullskinName, slash + 1,
					sizeof(newinfo.hullskinName));
		}
	}else{
		Q_strncpyz(newinfo.hullmodelName, v,
			sizeof(newinfo.hullmodelName));

		slash = strchr(newinfo.hullmodelName, '/');
		if(!slash)
			/* modelName didn not include a skin name */
			Q_strncpyz(newinfo.hullskinName, "default",
				sizeof(newinfo.hullskinName));
		else{
			Q_strncpyz(newinfo.hullskinName, slash + 1,
				sizeof(newinfo.hullskinName));
			/* truncate modelName */
			*slash = 0;
		}
	}
#endif ///////

	/* 
	 * scan for an existing clientinfo that matches this modelname
	 * so we can avoid loading checks if possible
	 */
	if(!ciexists(&newinfo)){
		qbool forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;
		
		/* if we are defering loads, just have it pick the first valid */
		if(forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading)){
			/* keep whatever they had if it won't violate team skins */
			setdeferredci(clientNum, &newinfo);
			/* if we are low on memory, leave them with this model */
			if(forceDefer){
				CG_Printf("Memory is low. Using deferred model.\n");
				newinfo.deferred = qfalse;
			}
		}else
			loadclientinfo(clientNum, &newinfo);
	}
	/* replace whatever was there with the new one */
	newinfo.infoValid = qtrue;
	*ci = newinfo;
}

/*
 * Called each frame when a player is dead
 * and the scoreboard is up
 * so deferred players can be loaded
 */
void
CG_LoadDeferredPlayers(void)
{
	int i;
	Clientinfo *ci;

	/* scan for a deferred player to load */
	for(i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++)
		if(ci->infoValid && ci->deferred){
			/* if we are low on memory, leave it deferred */
			if(trap_MemoryRemaining() < 4000000){
				CG_Printf("Memory is low. Using deferred model.\n");
				ci->deferred = qfalse;
				continue;
			}
			loadclientinfo(i, ci);
			/* break; */
		}
}

/*
 * Player animation
 */

/* may include ANIM_TOGGLEBIT */
static void
setlerpframeanim(Clientinfo *ci, Lerpframe *lf, int newAnimation)
{
	Anim *p;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;
	if(newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS)
		CG_Error("Bad animation number: %i", newAnimation);

	p = &ci->animations[newAnimation];
	lf->animation = p;
	lf->animationTime = lf->frameTime + p->initialLerp;

	if(cg_debugAnim.integer)
		CG_Printf("Anim: %i\n", newAnimation);
}

/*
 * Sets cg.snap, cg.oldFrame, and cg.backlerp
 * cg.time should be between oldFrameTime and frameTime after exit
 */
static void
runlerpframe(Clientinfo *ci, Lerpframe *lf, int newAnimation,
		float speedScale)
{
	int f, numFrames;
	Anim *anim;

	/* debugging tool to get no animations */
	if(cg_animSpeed.integer == 0){
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	/* see if the animation sequence is switching */
	if(newAnimation != lf->animationNumber || !lf->animation)
		setlerpframeanim(ci, lf, newAnimation);
	/* 
	 * if we have passed the current frame, move it to
	 * oldFrame and calculate a new frame 
	 */
	if(cg.time >= lf->frameTime){
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		/* get the next frame based on the animation */
		anim = lf->animation;
		if(!anim->frameLerp)
			return;		/* shouldn't happen */
		if(cg.time < lf->animationTime)
			lf->frameTime = lf->animationTime;	/* initial lerp */
		else
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;	/* adjust for haste, etc */

		numFrames = anim->numFrames;
		if(anim->flipflop)
			numFrames *= 2;
		if(f >= numFrames){
			f -= numFrames;
			if(anim->loopFrames){
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}else{
				f = numFrames - 1;
				/* the animation is stuck at the end, so it
				 * can immediately transition to another sequence */
				lf->frameTime = cg.time;
			}
		}
		if(anim->reversed)
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		else if(anim->flipflop && f>=anim->numFrames)
			lf->frame = anim->firstFrame + anim->numFrames - 1 -
				    (f%anim->numFrames);
		else
			lf->frame = anim->firstFrame + f;
		if(cg.time > lf->frameTime){
			lf->frameTime = cg.time;
			if(cg_debugAnim.integer)
				CG_Printf("Clamp lf->frameTime\n");
		}
	}

	if(lf->frameTime > cg.time + 200)
		lf->frameTime = cg.time;

	if(lf->oldFrameTime > cg.time)
		lf->oldFrameTime = cg.time;
	/* calculate current lerp value */
	if(lf->frameTime == lf->oldFrameTime)
		lf->backlerp = 0;
	else
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) 
			/ (lf->frameTime - lf->oldFrameTime);
}

static void
clearlerpframe(Clientinfo *ci, Lerpframe *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	setlerpframeanim(ci, lf, animationNumber);
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

static void
playeranim(Centity *cent, int *hullold, int *hull, float *hullbacklerp)
{
	Clientinfo *ci;
	int cnum;
	float speedscale;

	cnum = cent->currentState.clientNum;

	if(cg_noPlayerAnims.integer){
		*hullold = *hull = 0;
		return;
	}

	if(cent->currentState.powerups & (1 << PW_HASTE))
		speedscale = 1.5;
	else
		speedscale = 1;

	ci = &cgs.clientinfo[cnum];

	runlerpframe(ci, &cent->pe.torso, cent->currentState.torsoAnim,
		speedscale);

	*hullold = cent->pe.torso.oldFrame;
	*hull = cent->pe.torso.frame;
	*hullbacklerp = cent->pe.torso.backlerp;
}

/*
 * Player angles
 */

static void
swingangles(float destination, float swingTolerance, float clampTolerance,
	       float speed, float *angle, qbool *swinging)
{
	float	swing;
	float	move;
	float	scale;

	if(!*swinging){
		/* see if a swing should be started */
		swing = subeuler(*angle, destination);
		if(swing > swingTolerance || swing < -swingTolerance)
			*swinging = qtrue;
	}
	if(!*swinging)
		return;

	/* 
	 * modify the speed depending on the delta
	 * so it doesn't seem so linear 
	 */
	swing = subeuler(destination, *angle);
	scale	= fabs(swing);
	if(scale < swingTolerance * 0.5)
		scale = 0.5;
	else if(scale < swingTolerance)
		scale = 1.0;
	else
		scale = 2.0;

	/* swing towards the destination angle */
	if(swing >= 0){
		move = cg.frametime * scale * speed;
		if(move >= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = modeuler(*angle + move);
	}else if(swing < 0){
		move = cg.frametime * scale * -speed;
		if(move <= swing){
			move = swing;
			*swinging = qfalse;
		}
		*angle = modeuler(*angle + move);
	}

	/* clamp to no more than tolerance */
	swing = subeuler(destination, *angle);
	if(swing > clampTolerance)
		*angle = modeuler(destination - (clampTolerance - 1));
	else if(swing < -clampTolerance)
		*angle = modeuler(destination + (clampTolerance - 1));
}

/* twitch if hit */
static void
addpaintwitch(Centity *cent, Vec3 torsoAngles)
{
	int t;
	float f;

	t = cg.time - cent->pe.painTime;
	if(t >= PAIN_TWITCH_TIME)
		return;
	f = 1.0 - (float)t / PAIN_TWITCH_TIME;
	if(cent->pe.painDirection)
		torsoAngles[ROLL] += 20 * f;
	else
		torsoAngles[ROLL] -= 20 * f;
}

/* craft always looks exactly at cent->lerpAngles */
static void
playerangles(Centity *cent, Vec3 hull[3])
{
	Vec3 angles, vel;
	Scalar speed;
	int dir;

	copyv3(cent->lerpAngles, angles);
	angles[YAW] = modeuler(angles[YAW]);
	/*
	 * yaw
	 * FIXME
	 */
	/* adjust legs for movement dir -- FIXME */
	if(cent->currentState.eFlags & EF_DEAD)
		/* don't let dead bodies twitch */
		dir = 0;
	else{
		dir = cent->currentState.angles2[YAW];
		if(dir < 0 || dir > 7)
			CG_Error("Bad player movement angle");
	}
	
	/* lean a bit towards the direction of travel */
	copyv3(cent->currentState.traj.delta, vel);
	speed = normv3(vel);
	if(speed > 0.0){
		Vec3 axis[3];
		float	side;

		if(speed >= 100.0f)
			speed = 100.0f;
		speed *= 0.05f;
		eulertoaxis(angles, axis);
		side = speed * dotv3(vel, axis[1]);
		angles[ROLL] -= side;
		side = speed * dotv3(vel, axis[0]);
		angles[PITCH] += side;
	}
	addpaintwitch(cent, angles);
	/* pull the angles back out of the hierarchial chain */
	eulertoaxis(angles, hull);
}

static void
hastetrail(Centity *cent)
{
	Localent *smoke;
	Vec3 origin;
	int anim;

	if(cent->trailTime > cg.time)
		return;
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if(anim != LEGS_RUN && anim != LEGS_BACK)
		return;

	cent->trailTime += 100;
	if(cent->trailTime < cg.time)
		cent->trailTime = cg.time;

	copyv3(cent->lerpOrigin, origin);
	origin[2] -= 16;

	smoke = CG_SmokePuff(origin, vec3_origin, 8, 1, 1, 1, 1,
		500, cg.time, 0, 0, cgs.media.hastePuffShader);
	/* use the optimized local entity add */
	smoke->leType = LE_SCALE_FADE;
}

static void
dusttrail(Centity *cent)
{
	int anim;
	Vec3 end, vel;
	Trace tr;

	if(!cg_enableDust.integer)
		return;
	if(cent->dustTrailTime > cg.time)
		return;

	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if(anim != LEGS_LANDB && anim != LEGS_LAND)
		return;

	cent->dustTrailTime += 40;
	if(cent->dustTrailTime < cg.time)
		cent->dustTrailTime = cg.time;

	copyv3(cent->currentState.traj.base, end);
	end[2] -= 64;
	CG_Trace(&tr, cent->currentState.traj.base, NULL, NULL, end,
		cent->currentState.number,
		MASK_PLAYERSOLID);

	if(!(tr.surfaceFlags & SURF_DUST))
		return;
		
	copyv3(cent->currentState.traj.base, end);
	end[2] -= 16;

	setv3(vel, 0, 0, -30);
	CG_SmokePuff(end, vel, 24, .8f, .8f, .7f, .33f, 500,
		cg.time, 0, 0, cgs.media.dustPuffShader);
}

static void
trailitem(Centity *cent, Handle hModel)
{
	Refent ent;
	Vec3	angles;
	Vec3	axis[3];

	copyv3(cent->lerpAngles, angles);
	angles[PITCH]	= 0;
	angles[ROLL]	= 0;
	eulertoaxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	saddv3(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2]	+= 16;
	angles[YAW]	+= 90;
	eulertoaxis(angles, ent.axis);

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene(&ent);
}

static void
playerflag(Centity *cent, Handle skinh, Refent *hull)
{
	Clientinfo *ci;
	Refent pole, flag;
	Vec3 angles, dir;
	int legsAnim, flagAnim, updateangles;
	float angle, d;

	/* show the flag pole model */
	memset(&pole, 0, sizeof(pole));
	pole.hModel = cgs.media.flagPoleModel;
	copyv3(hull->lightingOrigin, pole.lightingOrigin);
	pole.shadowPlane = hull->shadowPlane;
	pole.renderfx = hull->renderfx;
	CG_PositionEntityOnTag(&pole, hull, hull->hModel, "tag_flag");
	trap_R_AddRefEntityToScene(&pole);

	/* show the flag model */
	memset(&flag, 0, sizeof(flag));
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = skinh;
	copyv3(hull->lightingOrigin, flag.lightingOrigin);
	flag.shadowPlane = hull->shadowPlane;
	flag.renderfx = hull->renderfx;

	clearv3(angles);

	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if(legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR)
		flagAnim = FLAG_STAND;
	else if(legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR){
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	}else{
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if(updateangles){

		copyv3(cent->currentState.traj.delta, dir);
		/* add gravity */
		dir[2] += 100;
		normv3(dir);
		d = dotv3(pole.axis[2], dir);
		/* if there is anough movement orthogonal to the flag pole */
		if(fabs(d) < 0.9){
			d = dotv3(pole.axis[0], dir);
			if(d > 1.0f)
				d = 1.0f;
			else if(d < -1.0f)
				d = -1.0f;
			angle = acos(d);

			d = dotv3(pole.axis[1], dir);
			if(d < 0)
				angles[YAW] = 360 - angle * 180 / M_PI;
			else
				angles[YAW] = angle * 180 / M_PI;
			if(angles[YAW] < 0)
				angles[YAW] += 360;
			if(angles[YAW] > 360)
				angles[YAW] -= 360;

			/* v3toeuler( cent->currentState.pos.delta, tmpangles );
			 * angles[YAW] = tmpangles[YAW] + 45 - cent->pe.hull.yawAngle;
			 * change the yaw angle */
			swingangles(angles[YAW], 25, 90, 0.15f,
				&cent->pe.flag.yawAngle,
				&cent->pe.flag.yawing);
		}

		/*
		 * d = dotv3(pole.axis[2], dir);
		 * angle = Q_acos(d);
		 *
		 * d = dotv3(pole.axis[1], dir);
		 * if (d < 0) {
		 *      angle = 360 - angle * 180 / M_PI;
		 * }
		 * else {
		 *      angle = angle * 180 / M_PI;
		 * }
		 * if (angle > 340 && angle < 20) {
		 *      flagAnim = FLAG_RUNUP;
		 * }
		 * if (angle > 160 && angle < 200) {
		 *      flagAnim = FLAG_RUNDOWN;
		 * }
		 */

	}

	/* set the yaw angle */
	angles[YAW] = cent->pe.flag.yawAngle;
	/* lerp the flag animation frames */
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	runlerpframe(ci, &cent->pe.flag, flagAnim, 1);
	flag.oldframe = cent->pe.flag.oldFrame;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	eulertoaxis(angles, flag.axis);
	CG_PositionRotatedEntityOnTag(&flag, &pole, pole.hModel, "tag_flag");

	trap_R_AddRefEntityToScene(&flag);

}

static void
playerpowerups(Centity *cent, Refent *torso)
{
	int powerups;
	Clientinfo *ci;

	powerups = cent->currentState.powerups;
	if(!powerups)
		return;

	/* quad gives a dlight */
	if(powerups & (1 << PW_QUAD))
		trap_R_AddLightToScene(cent->lerpOrigin,
			200 + (rand()&31), 0.2f, 0.2f, 1);

	/* flight plays a looped sound */
	if(powerups & (1 << PW_FLIGHT))
		trap_sndaddloop(cent->currentState.number,
			cent->lerpOrigin, vec3_origin,
			cgs.media.flightSound);

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	/* redflag */
	if(powerups & (1 << PW_REDFLAG)){
		if(ci->newAnims)
			playerflag(cent, cgs.media.redFlagFlapSkin, torso);
		else
			trailitem(cent, cgs.media.redFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin,
			200 + (rand()&31), 1.0, 0.2f, 0.2f);
	}

	/* blueflag */
	if(powerups & (1 << PW_BLUEFLAG)){
		if(ci->newAnims)
			playerflag(cent, cgs.media.blueFlagFlapSkin, torso);
		else
			trailitem(cent, cgs.media.blueFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin,
			200 + (rand()&31), 0.2f, 0.2f, 1.0);
	}

	/* neutralflag */
	if(powerups & (1 << PW_NEUTRALFLAG)){
		if(ci->newAnims)
			playerflag(cent, cgs.media.neutralFlagFlapSkin, torso);
		else
			trailitem(cent, cgs.media.neutralFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin,
			200 + (rand()&31), 1.0, 1.0, 1.0);
	}

	/* haste leaves smoke trails */
	if(powerups & (1 << PW_HASTE))
		hastetrail(cent);
}

/* Float a sprite over the player */
static void
floatsprite(Centity *cent, Handle shader)
{
	int rf;
	Refent ent;

	if(cent->currentState.number == cg.snap->ps.clientNum &&
	   !cg.renderingThirdPerson)
		rf = RF_THIRD_PERSON;	/* only show in mirrors */
	else
		rf = 0;

	memset(&ent, 0, sizeof(ent));
	copyv3(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene(&ent);
}

/* Float sprites over the player */
static void
playersprites(Centity *cent)
{
	int team;

	if(cent->currentState.eFlags & EF_CONNECTION){
		floatsprite(cent, cgs.media.connectionShader);
		return;
	}

	if(cent->currentState.eFlags & EF_TALK){
		floatsprite(cent, cgs.media.balloonShader);
		return;
	}

	if(cent->currentState.eFlags & EF_AWARD_IMPRESSIVE){
		floatsprite(cent, cgs.media.medalImpressive);
		return;
	}

	if(cent->currentState.eFlags & EF_AWARD_EXCELLENT){
		floatsprite(cent, cgs.media.medalExcellent);
		return;
	}

	if(cent->currentState.eFlags & EF_AWARD_GAUNTLET){
		floatsprite(cent, cgs.media.medalGauntlet);
		return;
	}

	if(cent->currentState.eFlags & EF_AWARD_DEFEND){
		floatsprite(cent, cgs.media.medalDefend);
		return;
	}

	if(cent->currentState.eFlags & EF_AWARD_ASSIST){
		floatsprite(cent, cgs.media.medalAssist);
		return;
	}

	if(cent->currentState.eFlags & EF_AWARD_CAP){
		floatsprite(cent, cgs.media.medalCapture);
		return;
	}

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if(!(cent->currentState.eFlags & EF_DEAD) &&
	   cg.snap->ps.persistant[PERS_TEAM] == team &&
	   cgs.gametype >= GT_TEAM){
		if(cg_drawFriend.integer)
			floatsprite(cent, cgs.media.friendShader);
		return;
	}
}

/* Returns the Z component of the surface being shadowed
 *
 * should it return a full plane instead of a Z?
 */
#define SHADOW_DISTANCE 128
static qbool
playershadow(Centity *cent, float *shadowPlane)
{
	Vec3 end;
	Vec3 mins = {-15, -15, 0};
	Vec3 maxs = { 15,  15, 2};
	Trace trace;
	float	alpha;
	
	*shadowPlane = 0;
	if(cg_shadows.integer == 0)
		return qfalse;
	/* no shadows when invisible */
	if(cent->currentState.powerups & (1 << PW_INVIS))
		return qfalse;
		
	/* trace down from the player to the ground */
	copyv3(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;
	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0,
		MASK_PLAYERSOLID);

	/* no shadow if too high */
	if(trace.fraction == 1.0 || trace.startsolid || trace.allsolid)
		return qfalse;
	*shadowPlane = trace.endpos[2] + 1;
	if(cg_shadows.integer != 1)
		return qtrue;	/* no mark for stencil or projection shadows */

	/* fade the shadow out with height */
	alpha = 1.0 - trace.fraction;

	/* hack / FPE - bogus planes?
	 * assert( dotv3( trace.plane.normal, trace.plane.normal ) != 0.0f ) */

	/* 
	 * add the mark as a temporary, so it goes directly to the renderer
	 * without taking a spot in the cg_marks array 
	 */
	CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos,
		trace.plane.normal, cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24,
		qtrue);
	return qtrue;
}

/* Draw a mark at the water surface */
static void
playersplash(Centity *cent)
{
	Vec3 start, end;
	Trace trace;
	int contents;
	Polyvert verts[4];

	if(!cg_shadows.integer)
		return;
	copyv3(cent->lerpOrigin, end);
	end[2] -= 24;
	/* 
	 * if the feet aren't in liquid, don't make a mark
	 * this won't handle moving water brushes, but they 
	 * wouldn't draw right anyway... 
	 */
	contents = CG_PointContents(end, 0);
	if(!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)))
		return;

	copyv3(cent->lerpOrigin, start);
	start[2] += 32;

	/* if the head isn't out of liquid, don't make a mark */
	contents = CG_PointContents(start, 0);
	if(contents &
	   (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		return;

	/* trace down to find the surface */
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0,
		(CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if(trace.fraction == 1.0)
		return;

	/* create a mark polygon */
	copyv3(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;
	copyv3(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;
	copyv3(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;
	copyv3(trace.endpos, verts[3].xyz);
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;
	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

/*
 * Adds a piece with modifications or duplications for powerups
 * Also called by CG_Missile for quad rockets, but nobody can tell...
 */
void
CG_AddRefEntityWithPowerups(Refent *ent, Entstate *state, int team)
{
	if(state->powerups & (1 << PW_INVIS)){
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene(ent);
	}else{
		trap_R_AddRefEntityToScene(ent);

		if(state->powerups & (1 << PW_QUAD)){
			if(team == TEAM_RED)
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			trap_R_AddRefEntityToScene(ent);
		}
		if(state->powerups & (1 << PW_REGEN))
			if(((cg.time / 100) % 10) == 1){
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene(ent);
			}
		if(state->powerups & (1 << PW_BATTLESUIT)){
			ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene(ent);
		}
	}
}

int
CG_LightVerts(Vec3 normal, int numVerts, Polyvert *verts)
{
	int i, j;
	float	incoming;
	Vec3 ambient;
	Vec3 lightdir;
	Vec3 directed;

	trap_R_LightForPoint(verts[0].xyz, ambient, directed, lightdir);

	for(i = 0; i < numVerts; i++){
		incoming = dotv3 (normal, lightdir);
		if(incoming <= 0){
			verts[i].modulate[0] = ambient[0];
			verts[i].modulate[1] = ambient[1];
			verts[i].modulate[2] = ambient[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = (ambient[0] + incoming * directed[0]);
		if(j > 255)
			j = 255;
		verts[i].modulate[0] = j;

		j = (ambient[1] + incoming * directed[1]);
		if(j > 255)
			j = 255;
		verts[i].modulate[1] = j;

		j = (ambient[2] + incoming * directed[2]);
		if(j > 255)
			j = 255;
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

void
CG_Player(Centity *cent)
{
	Clientinfo *ci;
	Refent hull;
	int cnum;
	int renderfx;
	qbool shadow;
	float shadowplane;
	Refent powerup;
	int t;
	float c;
	Vec3 angles;

	/* 
	 * the client number is stored in cnum.  It can't be derived
	 * from the entity number, because a single client may have
	 * multiple corpses on the level using the same clientinfo 
	 */
	cnum = cent->currentState.clientNum;
	if(cnum < 0 || cnum >= MAX_CLIENTS)
		CG_Error("Bad cnum on player entity");
	ci = &cgs.clientinfo[cnum];

	/* it is possible to see corpses from disconnected players that may
	 * not have valid clientinfo */
	if(!ci->infoValid)
		return;

	/* get the player model information */
	renderfx = 0;
	if(cent->currentState.number == cg.snap->ps.clientNum){
		if(!cg.renderingThirdPerson)
			renderfx = RF_THIRD_PERSON;	/* only draw in mirrors */
		else if(cg_cameraMode.integer)
			return;

	}
	memset(&hull, 0, sizeof(hull));
	/* get the rotation and anim lerp info */
	playerangles(cent, hull.axis);
	playeranim(cent, &hull.oldframe, &hull.frame, &hull.backlerp);
	/* add the talk balloon or disconnect icon */
	playersprites(cent);
	/* add the shadow */
	shadow = playershadow(cent, &shadowplane);
	/* add a water splash if partially in and out of water */
	playersplash(cent);

	if(cg_shadows.integer == 3 && shadow)
		renderfx |= RF_SHADOW_PLANE;
	renderfx |= RF_LIGHTING_ORIGIN;	/* use the same origin for all */

	/* add the craft body */
	hull.hModel = ci->hullmodel;
	if(!hull.hModel)
		return;
	hull.customSkin = ci->hullskin;
	copyv3(cent->lerpOrigin, hull.origin);
	copyv3(cent->lerpOrigin, hull.lightingOrigin);
	hull.shadowPlane = shadowplane;
	hull.renderfx = renderfx;
	copyv3(hull.origin, hull.oldorigin);	/* don't positionally lerp at all */
	CG_AddRefEntityWithPowerups(&hull, &cent->currentState, ci->team);

	t = cg.time - ci->medkitUsageTime;
	if(ci->medkitUsageTime && t < 500){
		memcpy(&powerup, &hull, sizeof(hull));
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.customSkin = 0;
		/* always draw */
		powerup.renderfx &= ~RF_THIRD_PERSON;
		clearv3(angles);
		eulertoaxis(angles, powerup.axis);
		copyv3(cent->lerpOrigin, powerup.origin);
		powerup.origin[2] += -24 + (float)t * 80 / 500;
		if(t > 400){
			c = (float)(t - 1000) * 0xff / 100;
			powerup.shaderRGBA[0]	= 0xff - c;
			powerup.shaderRGBA[1]	= 0xff - c;
			powerup.shaderRGBA[2]	= 0xff - c;
			powerup.shaderRGBA[3]	= 0xff - c;
		}else{
			powerup.shaderRGBA[0]	= 0xff;
			powerup.shaderRGBA[1]	= 0xff;
			powerup.shaderRGBA[2]	= 0xff;
			powerup.shaderRGBA[3]	= 0xff;
		}
		trap_R_AddRefEntityToScene(&powerup);
	}

	dusttrail(cent);

	/*
	 * add the gun / barrel / flash
	 */
	CG_AddPlayerWeapon(&hull, NULL, cent, WSpri, ci->team);
	CG_AddPlayerWeapon(&hull, NULL, cent, WSsec, ci->team);

	/* add powerups floating behind the player */
	playerpowerups(cent, &hull);
}


/*  A player just came into view or teleported, so reset all animation info */
void
CG_ResetPlayerEntity(Centity *cent)
{
	cent->errorTime = -99999;	/* guarantee no error decay added */
	cent->extrapolated = qfalse;

	clearlerpframe(&cgs.clientinfo[ cent->currentState.clientNum ],
		&cent->pe.legs,
		cent->currentState.legsAnim);
	clearlerpframe(&cgs.clientinfo[ cent->currentState.clientNum ],
		&cent->pe.torso,
		cent->currentState.torsoAnim);

	BG_EvaluateTrajectory(&cent->currentState.traj, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time,
		cent->lerpAngles);

	copyv3(cent->lerpOrigin, cent->rawOrigin);
	copyv3(cent->lerpAngles, cent->rawAngles);

	memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset(&cent->pe.torso, 0, sizeof(cent->pe.torso));
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if(cg_debugPosition.integer)
		CG_Printf("%i ResetPlayerEntity yaw=%f\n",
			cent->currentState.number,
			cent->pe.torso.yawAngle);
}
