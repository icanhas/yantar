/* 
 * Draw all of the graphical elements during active (after loading) 
 * gameplay 
 */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

#ifdef MISSIONPACK
#include "../ui/uishared.h"

/* used for scoreboard */
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int	sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char	systemChat[256];
char	teamChat1[256];
char	teamChat2[256];

#ifdef MISSIONPACK

int
CG_Text_Width(const char *text, float scale, int limit)
{
	int count,len;
	float	out;
	Glyphinfo *glyph;
	float	useScale;
/* FIXME: see ui_main.c, same problem
 * const unsigned char *s = text; */
	const char	*s = text;
	Fontinfo	*font = &cgDC.Assets.textFont;
	if(scale <= cg_smallFont.value)
		font = &cgDC.Assets.smallFont;
	else if(scale > cg_bigFont.value)
		font = &cgDC.Assets.bigFont;
	useScale = scale * font->glyphScale;
	out = 0;
	if(text){
		len = strlen(text);
		if(limit > 0 && len > limit)
			len = limit;
		count = 0;
		while(s && *s && count < len){
			if(Q_IsColorString(s)){
				s += 2;
				continue;
			}else{
				glyph	= &font->glyphs[(int)*s];	/* TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build */
				out	+= glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

int
CG_Text_Height(const char *text, float scale, int limit)
{
	int len, count;
	float	max;
	Glyphinfo *glyph;
	float	useScale;
/* TTimo: FIXME
 * const unsigned char *s = text; */
	const char	*s = text;
	Fontinfo	*font = &cgDC.Assets.textFont;
	if(scale <= cg_smallFont.value)
		font = &cgDC.Assets.smallFont;
	else if(scale > cg_bigFont.value)
		font = &cgDC.Assets.bigFont;
	useScale = scale * font->glyphScale;
	max = 0;
	if(text){
		len = strlen(text);
		if(limit > 0 && len > limit)
			len = limit;
		count = 0;
		while(s && *s && count < len){
			if(Q_IsColorString(s)){
				s += 2;
				continue;
			}else{
				glyph = &font->glyphs[(int)*s];	/* TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build */
				if(max < glyph->height)
					max = glyph->height;
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void
CG_Text_PaintChar(float x, float y, float width, float height, float scale,
		  float s, float t, float s2, float t2,
		  Handle hShader)
{
	float w, h;
	w	= width * scale;
	h	= height * scale;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void
CG_Text_Paint(float x, float y, float scale, Vec4 color, const char *text,
	      float adjust, int limit,
	      int style)
{
	int len, count;
	Vec4	newColor;
	Glyphinfo *glyph;
	float	useScale;
	Fontinfo *font = &cgDC.Assets.textFont;
	if(scale <= cg_smallFont.value)
		font = &cgDC.Assets.smallFont;
	else if(scale > cg_bigFont.value)
		font = &cgDC.Assets.bigFont;
	useScale = scale * font->glyphScale;
	if(text){
/* TTimo: FIXME
 *      const unsigned char *s = text; */
		const char *s = text;
		trap_R_SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof(Vec4));
		len = strlen(text);
		if(limit > 0 && len > limit)
			len = limit;
		count = 0;
		while(s && *s && count < len){
			glyph = &font->glyphs[(int)*s];	/* TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build */
			/* int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			 * float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height); */
			if(Q_IsColorString(s)){
				memcpy(newColor, g_color_table[ColorIndex(
								       *(s+1))],
					sizeof(newColor));
				newColor[3] = color[3];
				trap_R_SetColor(newColor);
				s += 2;
				continue;
			}else{
				float yadj = useScale * glyph->top;
				if(style == ITEM_TEXTSTYLE_SHADOWED || style ==
				   ITEM_TEXTSTYLE_SHADOWEDMORE){
					int ofs = style ==
						  ITEM_TEXTSTYLE_SHADOWED ? 1 :
						  2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor(colorBlack);
					CG_Text_PaintChar(x + ofs, y - yadj +
						ofs,
						glyph->imageWidth,
						glyph->imageHeight,
						useScale,
						glyph->s,
						glyph->t,
						glyph->s2,
						glyph->t2,
						glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor(newColor);
				}
				CG_Text_PaintChar(x, y - yadj,
					glyph->imageWidth,
					glyph->imageHeight,
					useScale,
					glyph->s,
					glyph->t,
					glyph->s2,
					glyph->t2,
					glyph->glyph);
				/* CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph); */
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}
#endif

/*
 * Draws large numbers for status bar and powerups
 */
#ifndef MISSIONPACK
static void
CG_DrawField(int x, int y, int width, int value)
{
	char	num[16], *ptr;
	int	l;
	int	frame;

	if(width < 1)
		return;

	/* draw number string */
	if(width > 5)
		width = 5;

	switch(width){
	case 1:
		value	= value > 9 ? 9 : value;
		value	= value < 0 ? 0 : value;
		break;
	case 2:
		value	= value > 99 ? 99 : value;
		value	= value < -9 ? -9 : value;
		break;
	case 3:
		value	= value > 999 ? 999 : value;
		value	= value < -99 ? -99 : value;
		break;
	case 4:
		value	= value > 9999 ? 9999 : value;
		value	= value < -999 ? -999 : value;
		break;
	}

	Q_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if(l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while(*ptr && l){
		if(*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic(x,y, CHAR_WIDTH, CHAR_HEIGHT,
			cgs.media.numberShaders[frame]);
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif	/* MISSIONPACK */

void
CG_Draw3DModel(float x, float y, float w, float h, Handle model,
	       Handle skin, Vec3 origin,
	       Vec3 angles)
{
	Refdef refdef;
	Refent ent;

	if(!cg_draw3dIcons.integer || !cg_drawIcons.integer)
		return;

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	eulertoaxis(angles, ent.axis);
	copyv3(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin	= skin;
	ent.renderfx	= RF_NOSHADOW;	/* no stencil shadows */

	refdef.rdflags = RDF_NOWORLDMODEL;

	clearaxis(refdef.viewaxis);

	refdef.fov_x	= 30;
	refdef.fov_y	= 30;

	refdef.x	= x;
	refdef.y	= y;
	refdef.width	= w;
	refdef.height	= h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/*
 * Used for both the status bar and the scoreboard
 */
void
CG_DrawHead(float x, float y, float w, float h, int clientNum, Vec3 headAngles)
{
	Cliphandle cm;
	Clientinfo *ci;
	float	len;
	Vec3	origin;
	Vec3	mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if(cg_draw3dIcons.integer){
		cm = ci->headModel;
		if(!cm)
			return;

		/* offset the origin y and z to center the head */
		trap_R_ModelBounds(cm, mins, maxs);

		origin[2]	= -0.5 * (mins[2] + maxs[2]);
		origin[1]	= 0.5 * (mins[1] + maxs[1]);

		/* calculate distance so the head nearly fills the box
		 * assume heads are taller than wide */
		len = 0.7 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;	/* len / tan( fov/2 ) */

		/* allow per-model tweaking */
		addv3(origin, ci->headOffset, origin);

		CG_Draw3DModel(x, y, w, h, ci->headModel, ci->headSkin, origin,
			headAngles);
	}else if(cg_drawIcons.integer)
		CG_DrawPic(x, y, w, h, ci->modelIcon);

	/* if they are deferred, draw a cross out */
	if(ci->deferred)
		CG_DrawPic(x, y, w, h, cgs.media.deferShader);
}

/*
 * Used for both the status bar and the scoreboard
 */
void
CG_DrawFlagModel(float x, float y, float w, float h, int team, qbool force2D)
{
	Handle		cm;
	float len;
	Vec3		origin, angles;
	Vec3		mins, maxs;
	Handle		handle;

	if(!force2D && cg_draw3dIcons.integer){

		clearv3(angles);

		cm = cgs.media.redFlagModel;

		/* offset the origin y and z to center the flag */
		trap_R_ModelBounds(cm, mins, maxs);

		origin[2]	= -0.5 * (mins[2] + maxs[2]);
		origin[1]	= 0.5 * (mins[1] + maxs[1]);

		/* calculate distance so the flag nearly fills the box
		 * assume heads are taller than wide */
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;	/* len / tan( fov/2 ) */

		angles[YAW] = 60 * sin(cg.time / 2000.0);;

		if(team == TEAM_RED)
			handle = cgs.media.redFlagModel;
		else if(team == TEAM_BLUE)
			handle = cgs.media.blueFlagModel;
		else if(team == TEAM_FREE)
			handle = cgs.media.neutralFlagModel;
		else
			return;
		CG_Draw3DModel(x, y, w, h, handle, 0, origin, angles);
	}else if(cg_drawIcons.integer){
		Gitem *item;

		if(team == TEAM_RED)
			item = BG_FindItemForPowerup(PW_REDFLAG);
		else if(team == TEAM_BLUE)
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
		else if(team == TEAM_FREE)
			item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
		else
			return;
		if(item)
			CG_DrawPic(x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon);
	}
}

#ifndef MISSIONPACK

static void
CG_DrawStatusBarHead(float x)
{
	Vec3	angles;
	float	size, stretch;
	float	frac;

	clearv3(angles);

	if(cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME){
		frac	= (float)(cg.time - cg.damageTime) / DAMAGE_TIME;
		size	= ICON_SIZE * 1.25 * (1.5 - frac * 0.5);

		stretch = size - ICON_SIZE * 1.25;
		/* kick in the direction of damage */
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw	= 180 + 20 * cos(crandom()*M_PI);
		cg.headEndPitch = 5 * cos(crandom()*M_PI);

		cg.headStartTime	= cg.time;
		cg.headEndTime		= cg.time + 100 + random() * 2000;
	}else{
		if(cg.time >= cg.headEndTime){
			/* select a new head angle */
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime	= cg.headEndTime;
			cg.headEndTime		= cg.time + 100 + random() *
						  2000;

			cg.headEndYaw	= 180 + 20 * cos(crandom()*M_PI);
			cg.headEndPitch = 5 * cos(crandom()*M_PI);
		}

		size = ICON_SIZE * 1.25;
	}

	/* if the server was frozen for a while we may have a bad head start time */
	if(cg.headStartTime > cg.time)
		cg.headStartTime = cg.time;

	frac =
		(cg.time -
		 cg.headStartTime) / (float)(cg.headEndTime - cg.headStartTime);
	frac = frac * frac * (3 - 2 * frac);
	angles[YAW] = cg.headStartYaw +
		      (cg.headEndYaw - cg.headStartYaw) * frac;
	angles[PITCH] = cg.headStartPitch +
			(cg.headEndPitch - cg.headStartPitch) * frac;

	CG_DrawHead(x, 480 - size, size, size,
		cg.snap->ps.clientNum, angles);
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static void
CG_DrawStatusBarFlag(float x, int team)
{
	CG_DrawFlagModel(x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse);
}
#endif	/* MISSIONPACK */

void
CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team)
{
	Vec4 hcolor;

	hcolor[3] = alpha;
	if(team == TEAM_RED){
		hcolor[0]	= 1;
		hcolor[1]	= 0;
		hcolor[2]	= 0;
	}else if(team == TEAM_BLUE){
		hcolor[0]	= 0;
		hcolor[1]	= 0;
		hcolor[2]	= 1;
	}else
		return;
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);
}

#ifndef MISSIONPACK
static void
CG_DrawStatusBar(void)
{
	int color;
	Centity *cent;
	Playerstate *ps;
	int value;
	Vec4	hcolor;
	Vec3	angles;
	Vec3	origin;

	static float colors[4][4] = {
/*		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} }; */
		{ 1.0f, 1.0f, 1.0f, 0.7f },	/* normal */
		{ 1.0f, 0.2f, 0.2f, 0.7f },	/* low health */
		{ 0.5f, 0.5f, 0.5f, 0.7f },	/* weapon firing */
		{ 1.0f, 1.0f, 1.0f, 0.7f }
	};	/* health > 100 */

	if(cg_drawStatus.integer == 0)
		return;

	/* draw the team background */
	CG_DrawTeamBackground(0, 420, 640, 60, 0.33f,
		cg.snap->ps.persistant[PERS_TEAM]);

	cent	= &cg_entities[cg.snap->ps.clientNum];
	ps	= &cg.snap->ps;

	clearv3(angles);

	/* draw any 3D icons first, so the changes back to 2D are minimized */
	if(cent->currentState.weap[WSpri] &&
	   cg_weapons[cent->currentState.weap[WSpri]].ammoModel){
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
		CG_Draw3DModel(CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE,
			ICON_SIZE,
			cg_weapons[cent->currentState.weap[WSpri]].ammoModel, 0,
			origin,
			angles);
	}

	CG_DrawStatusBarHead(185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE);

	if(cg.predictedPlayerState.powerups[PW_REDFLAG])
		CG_DrawStatusBarFlag(
			185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE,
			TEAM_RED);
	else if(cg.predictedPlayerState.powerups[PW_BLUEFLAG])
		CG_DrawStatusBarFlag(
			185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE,
			TEAM_BLUE);
	else if(cg.predictedPlayerState.powerups[PW_NEUTRALFLAG])
		CG_DrawStatusBarFlag(
			185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE,
			TEAM_FREE);

	if(ps->stats[ STAT_SHIELD ]){
		origin[0]	= 90;
		origin[1]	= 0;
		origin[2]	= -10;
		angles[YAW]	= (cg.time & 2047) * 360 / 2048.0;
		CG_Draw3DModel(370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432,
			ICON_SIZE, ICON_SIZE,
			cgs.media.armorModel, 0, origin,
			angles);
	}
	/*
	 * ammo
	 */
	if(cent->currentState.weap[WSpri]){
		value = ps->ammo[cent->currentState.weap[WSpri]];
		if(value > -1){
			if(cg.predictedPlayerState.weapstate[WSpri] == WEAPON_FIRING
			   && cg.predictedPlayerState.weaptime[WSpri] > 100)
			then{
				/* draw as dark grey when reloading */
				color = 2;	/* dark grey */
			}else{
				if(value >= 0)
					color = 0;	/* green */
				else
					color = 1;	/* red */
			}
			trap_R_SetColor(colors[color]);

			CG_DrawField (0, 384, 4, value);
			trap_R_SetColor(NULL);

			/* if we didn't draw a 3D icon, draw a 2D icon for ammo */
			if(!cg_draw3dIcons.integer && cg_drawIcons.integer){
				Handle icon;

				icon = cg_weapons[cg.predictedPlayerState.weap[WSpri]].ammoIcon;
				if(icon){
					CG_DrawPic(CHAR_WIDTH*3 + TEXT_ICON_SPACE,
						432, ICON_SIZE, ICON_SIZE, icon);
				}
			}
		}
	}
	if(cent->currentState.weap[WSsec]){
		value = ps->ammo[cent->currentState.weap[WSsec]];
		if(value > -1){
			if(cg.predictedPlayerState.weapstate[WSsec] == WEAPON_FIRING
			   && cg.predictedPlayerState.weaptime[WSsec] > 100)
			then{
				/* draw as dark grey when reloading */
				color = 2;	/* dark grey */
			}else{
				if(value >= 0)
					color = 0;	/* green */
				else
					color = 1;	/* red */
			}
			trap_R_SetColor(colors[color]);

			CG_DrawField(0, 432, 4, value);
			trap_R_SetColor(NULL);

			/* if we didn't draw a 3D icon, draw a 2D icon for ammo */
			if(!cg_draw3dIcons.integer && cg_drawIcons.integer){
				Handle icon;

				icon = cg_weapons[cg.predictedPlayerState.weap[WSsec]].ammoIcon;
				if(icon){
					CG_DrawPic(CHAR_WIDTH*3 + TEXT_ICON_SPACE,
						432, ICON_SIZE, ICON_SIZE, icon);
				}
			}
		}
	}

	/*
	 * health
	 */
	value = ps->stats[STAT_HEALTH];
	if(value > 100)
		trap_R_SetColor(colors[3]);	/* white */
	else if(value > 25)
		trap_R_SetColor(colors[0]);	/* green */
	else if(value > 0){
		color = (cg.time >> 8) & 1;	/* flash */
		trap_R_SetColor(colors[color]);
	}else
		trap_R_SetColor(colors[1]);	/* red */

	/* stretch the health up when taking damage */
	CG_DrawField (185, 432, 4, value);
	CG_ColorForHealth(hcolor);
	trap_R_SetColor(hcolor);

	/*
	 * armor
	 */
	value = ps->stats[STAT_SHIELD];
	if(value > 0){
		trap_R_SetColor(colors[0]);
		CG_DrawField (370, 432, 4, value);
		trap_R_SetColor(NULL);
		/* if we didn't draw a 3D icon, draw a 2D icon for armor */
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
			CG_DrawPic(370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432,
				ICON_SIZE, ICON_SIZE,
				cgs.media.armorIcon);

	}
}
#endif

/*
 * Upper right corner
 */

static float
CG_DrawAttacker(float y)
{
	int t;
	float	size;
	Vec3	angles;
	const char *info;
	const char *name;
	int clientNum;

	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
		return y;

	if(!cg.attackerTime)
		return y;

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if(clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum ==
	   cg.snap->ps.clientNum)
		return y;

	t = cg.time - cg.attackerTime;
	if(t > ATTACKER_HEAD_TIME){
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH]	= 0;
	angles[YAW]	= 180;
	angles[ROLL]	= 0;
	CG_DrawHead(640 - size, y, size, size, clientNum, angles);

	info	= CG_ConfigString(CS_PLAYERS + clientNum);
	name	= Info_ValueForKey(info, "n");
	y += size;
	CG_DrawBigString(640 - (Q_printablelen(
					name) * BIGCHAR_WIDTH), y, name, 0.5);

	return y + BIGCHAR_HEIGHT + 2;
}

static float
CG_DrawSnapshot(float y)
{
	char	*s;
	int	w;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

#define FPS_FRAMES 4
static float
CG_DrawFPS(float y)
{
	char	*s;
	int	w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int	i, total;
	int	fps;
	static int previous;
	int	t, frameTime;

	/* don't use serverTime, because that will be drifting to
	 * correct for internet lag changes, timescales, timedemos, etc */
	t = trap_Milliseconds();
	frameTime	= t - previous;
	previous	= t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if(index > FPS_FRAMES){
		/* average multiple frames together to smooth changes out a bit */
		total = 0;
		for(i = 0; i < FPS_FRAMES; i++)
			total += previousTimes[i];
		if(!total)
			total = 1;
		fps = 1000 * FPS_FRAMES / total;

		s	= va("%ifps", fps);
		w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

static float
CG_DrawTimer(float y)
{
	char	*s;
	int	w;
	int	mins, seconds, tens;
	int	msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s	= va("%i:%i%i", mins, tens, seconds);
	w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/* FIXME: this is well wrong, like */
static float
CG_Drawspeed(float y)
{
	Scalar speed;
	char *s;
	int w;
	
	speed = lenv3(cg.snap->ps.velocity);
	s = va("%.2f km/h", ((speed/100.0f)/1000.0f)*60.0f*60.0f);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(635 - w, y + 2, s, 1.0f);
	return y + BIGCHAR_HEIGHT + 4;
}

static float
CG_DrawTeamOverlay(float y, qbool right, qbool upper)
{
	int	x, w, h, xx;
	int	i, j, len;
	const char *p;
	Vec4	hcolor;
	int	pwidth, lwidth;
	int	plyrs;
	char	st[16];
	Clientinfo *ci;
	Gitem *item;
	int ret_y, count;

	if(!cg_drawTeamOverlay.integer)
		return y;

	if(cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED &&
	   cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
		return y;	/* Not on any team */

	plyrs = 0;

	/* max player name width */
	pwidth	= 0;
	count	= (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for(i = 0; i < count; i++){
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if(ci->infoValid && ci->team ==
		   cg.snap->ps.persistant[PERS_TEAM]){
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if(len > pwidth)
				pwidth = len;
		}
	}

	if(!plyrs)
		return y;

	if(pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	/* max location name width */
	lwidth = 0;
	for(i = 1; i < MAX_LOCATIONS; i++){
		p = CG_ConfigString(CS_LOCATIONS + i);
		if(p && *p){
			len = CG_DrawStrlen(p);
			if(len > lwidth)
				lwidth = len;
		}
	}

	if(lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if(right)
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if(upper)
		ret_y = y + h;
	else{
		y -= h;
		ret_y = y;
	}

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
		hcolor[0]	= 1.0f;
		hcolor[1]	= 0.0f;
		hcolor[2]	= 0.0f;
		hcolor[3]	= 0.33f;
	}else{	/* if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) */
		hcolor[0]	= 0.0f;
		hcolor[1]	= 0.0f;
		hcolor[2]	= 1.0f;
		hcolor[3]	= 0.33f;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);

	for(i = 0; i < count; i++){
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if(ci->infoValid && ci->team ==
		   cg.snap->ps.persistant[PERS_TEAM]){

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt(xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
				TEAM_OVERLAY_MAXNAME_WIDTH);

			if(lwidth){
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if(!p || !*p)
					p = "unknown";
/*                      len = CG_DrawStrlen(p);
*                      if (len > lwidth)
*                              len = lwidth; */

/*                      xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
 *                              ((lwidth/2 - len/2) * TINYCHAR_WIDTH); */
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH *
				     pwidth;
				CG_DrawStringExt(xx, y,
					p, hcolor, qfalse, qfalse,
					TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth(ci->health, ci->armor, hcolor);

			Q_sprintf (st, sizeof(st), "%3i %3i", ci->health,
				ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
			     TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt(xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

			/* draw weapon icon */
			xx += TINYCHAR_WIDTH * 3;

			if(cg_weapons[ci->curWeapon].weaponIcon)
				CG_DrawPic(xx, y, TINYCHAR_WIDTH,
					TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon);
			else
				CG_DrawPic(xx, y, TINYCHAR_WIDTH,
					TINYCHAR_HEIGHT,
					cgs.media.deferShader);

			/* Draw powerup icons */
			if(right)
				xx = x;
			else
				xx = x + w - TINYCHAR_WIDTH;
			for(j = 0; j <= PW_NUM_POWERUPS; j++)
				if(ci->powerups & (1 << j)){

					item = BG_FindItemForPowerup(j);

					if(item){
						CG_DrawPic(
							xx, y, TINYCHAR_WIDTH,
							TINYCHAR_HEIGHT,
							trap_R_RegisterShader(
								item->icon));
						if(right)
							xx -= TINYCHAR_WIDTH;
						else
							xx += TINYCHAR_WIDTH;
					}
				}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
/* #endif */
}

static void
CG_DrawUpperRight(Stereoframe stereoFrame)
{
	float y;

	y = 0;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1)
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	if(cg_drawSnapshot.integer)
		y = CG_DrawSnapshot(y);
	if(cg_drawFPS.integer &&
	   (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT))
		y = CG_DrawFPS(y);
	if(cg_drawspeed.integer)
		y = CG_Drawspeed(y);
	if(cg_drawTimer.integer)
		y = CG_DrawTimer(y);
	if(cg_drawAttacker.integer)
		y = CG_DrawAttacker(y);

}

/*
 * Lower right corner
 */

/*
 * Draw the small two score display
 */
#ifndef MISSIONPACK
static float
CG_DrawScores(float y)
{
	const char *s;
	int	s1, s2, score;
	int	x, w;
	int	v;
	Vec4	color;
	float	y1;
	Gitem *item;

	s1	= cgs.scores1;
	s2	= cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	/* draw from the right side to left */
	if(cgs.gametype >= GT_TEAM){
		x = 640;
		color[0]	= 0.0f;
		color[1]	= 0.0f;
		color[2]	= 1.0f;
		color[3]	= 0.33f;
		s		= va("%2i", s2);
		w		= CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
		x		-= w;
		CG_FillRect(x, y-4,  w, BIGCHAR_HEIGHT+8, color);
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
			CG_DrawPic(x, y-4, w, BIGCHAR_HEIGHT+8,
				cgs.media.selectShader);
		CG_DrawBigString(x + 4, y, s, 1.0F);

		if(cgs.gametype == GT_CTF){
			/* Display flag status */
			item = BG_FindItemForPowerup(PW_BLUEFLAG);

			if(item){
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.blueflag >= 0 && cgs.blueflag <= 2)
					CG_DrawPic(
						x, y1-4, w, BIGCHAR_HEIGHT+8,
						cgs.media.blueFlagShader[cgs.
									 blueflag
						]);
			}
		}
		color[0]	= 1.0f;
		color[1]	= 0.0f;
		color[2]	= 0.0f;
		color[3]	= 0.33f;
		s		= va("%2i", s1);
		w		= CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
		x		-= w;
		CG_FillRect(x, y-4,  w, BIGCHAR_HEIGHT+8, color);
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
			CG_DrawPic(x, y-4, w, BIGCHAR_HEIGHT+8,
				cgs.media.selectShader);
		CG_DrawBigString(x + 4, y, s, 1.0F);

		if(cgs.gametype == GT_CTF){
			/* Display flag status */
			item = BG_FindItemForPowerup(PW_REDFLAG);

			if(item){
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.redflag >= 0 && cgs.redflag <= 2)
					CG_DrawPic(
						x, y1-4, w, BIGCHAR_HEIGHT+8,
						cgs.media.redFlagShader[cgs.
									redflag]);
			}
		}

		if(cgs.gametype >= GT_CTF)
			v = cgs.capturelimit;
		else
			v = cgs.fraglimit;
		if(v){
			s	= va("%2i", v);
			w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x	-= w;
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

	}else{
		qbool spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);

		/* always show your score in the second box if not in first place */
		if(s1 != score)
			s2 = score;
		if(s2 != SCORE_NOT_PRESENT){
			s	= va("%2i", s2);
			w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x	-= w;
			if(!spectator && score == s2 && score != s1){
				color[0]	= 1.0f;
				color[1]	= 0.0f;
				color[2]	= 0.0f;
				color[3]	= 0.33f;
				CG_FillRect(x, y-4,  w, BIGCHAR_HEIGHT+8, color);
				CG_DrawPic(x, y-4, w, BIGCHAR_HEIGHT+8,
					cgs.media.selectShader);
			}else{
				color[0]	= 0.5f;
				color[1]	= 0.5f;
				color[2]	= 0.5f;
				color[3]	= 0.33f;
				CG_FillRect(x, y-4,  w, BIGCHAR_HEIGHT+8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

		/* first place */
		if(s1 != SCORE_NOT_PRESENT){
			s	= va("%2i", s1);
			w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x	-= w;
			if(!spectator && score == s1){
				color[0]	= 0.0f;
				color[1]	= 0.0f;
				color[2]	= 1.0f;
				color[3]	= 0.33f;
				CG_FillRect(x, y-4,  w, BIGCHAR_HEIGHT+8, color);
				CG_DrawPic(x, y-4, w, BIGCHAR_HEIGHT+8,
					cgs.media.selectShader);
			}else{
				color[0]	= 0.5f;
				color[1]	= 0.5f;
				color[2]	= 0.5f;
				color[3]	= 0.33f;
				CG_FillRect(x, y-4,  w, BIGCHAR_HEIGHT+8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

		if(cgs.fraglimit){
			s	= va("%2i", cgs.fraglimit);
			w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x	-= w;
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

	}

	return y1 - 8;
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static float
CG_DrawPowerups(float y)
{
	int	sorted[MAX_POWERUPS];
	int	sortedTime[MAX_POWERUPS];
	int	i, j, k;
	int	active;
	Playerstate *ps;
	int	t;
	Gitem *item;
	int	x;
	int	color;
	float	size;
	float	f;
	static float colors[2][4] = {
		{ 0.2f, 1.0f, 0.2f, 1.0f },
		{ 1.0f, 0.2f, 0.2f, 1.0f }
	};

	ps = &cg.snap->ps;

	if(ps->stats[STAT_HEALTH] <= 0)
		return y;

	/* sort the list by time remaining */
	active = 0;
	for(i = 0; i < MAX_POWERUPS; i++){
		if(!ps->powerups[ i ])
			continue;
		t = ps->powerups[ i ] - cg.time;
		/* ZOID--don't draw if the power up has unlimited time (999 seconds)
		 * This is true of the CTF flags */
		if(t < 0 || t > 999000)
			continue;

		/* insert into the list */
		for(j = 0; j < active; j++)
			if(sortedTime[j] >= t){
				for(k = active - 1; k >= j; k--){
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	/* draw the icons and timers */
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for(i = 0; i < active; i++){
		item = BG_FindItemForPowerup(sorted[i]);

		if(item){

			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor(colors[color]);
			CG_DrawField(x, y, 2, sortedTime[ i ] / 1000);

			t = ps->powerups[ sorted[i] ];
			if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
				trap_R_SetColor(NULL);
			else{
				Vec4 modulate;

				f =
					(float)(t -
						cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0]
					= modulate[1] = modulate[2] =
								modulate
								[3] = f;
				trap_R_SetColor(modulate);
			}

			if(cg.powerupActive == sorted[i] &&
			   cg.time - cg.powerupTime < PULSE_TIME){
				f = 1.0 -
				    (((float)cg.time -
				      cg.powerupTime) / PULSE_TIME);
				size = ICON_SIZE *
				       (1.0 + (PULSE_SCALE - 1.0) * f);
			}else
				size = ICON_SIZE;

			CG_DrawPic(640 - size, y + ICON_SIZE / 2 - size / 2,
				size, size, trap_R_RegisterShader(item->icon));
		}
	}
	trap_R_SetColor(NULL);

	return y;
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static void
CG_DrawLowerRight(void)
{
	float y;

	y = 480 - ICON_SIZE;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2)
		y = CG_DrawTeamOverlay(y, qtrue, qfalse);

	y	= CG_DrawScores(y);
	y	= CG_DrawPowerups(y);
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static int
CG_DrawPickupItem(int y)
{
	int value;
	float *fadeColor;

	if(cg.snap->ps.stats[STAT_HEALTH] <= 0)
		return y;

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if(value){
		fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
		if(fadeColor){
			CG_RegisterItemVisuals(value);
			trap_R_SetColor(fadeColor);
			CG_DrawPic(8, y, ICON_SIZE, ICON_SIZE,
				cg_items[ value ].icon);
			CG_DrawBigString(ICON_SIZE + 16, y +
				(ICON_SIZE/2 - BIGCHAR_HEIGHT/2),
				bg_itemlist[ value ].pickupname,
				fadeColor[0]);
			trap_R_SetColor(NULL);
		}
	}

	return y;
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static void
CG_DrawLowerLeft(void)
{
	float y;

	y = 420 - ICON_SIZE;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3)
		y = CG_DrawTeamOverlay(y, qfalse, qfalse);


	y = CG_DrawPickupItem(y);
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static void
CG_DrawTeamInfo(void)
{
	int	h;
	int	i;
	Vec4 hcolor;
	int	chatHeight;

#define CHATLOC_Y	420	/* bottom end */
#define CHATLOC_X	0

	if(cg_ChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_ChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if(chatHeight <= 0)
		return;		/* disabled */

	if(cgs.teamLastChatPos != cgs.teamChatPos){
		if(cg.time -
		   cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] >
		   cg_ChatTime.integer)
			cgs.teamLastChatPos++;

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
			hcolor[0]	= 1.0f;
			hcolor[1]	= 0.0f;
			hcolor[2]	= 0.0f;
			hcolor[3]	= 0.33f;
		}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
			hcolor[0]	= 0.0f;
			hcolor[1]	= 0.0f;
			hcolor[2]	= 1.0f;
			hcolor[3]	= 0.33f;
		}else{
			hcolor[0]	= 0.0f;
			hcolor[1]	= 1.0f;
			hcolor[2]	= 0.0f;
			hcolor[3]	= 0.33f;
		}

		trap_R_SetColor(hcolor);
		CG_DrawPic(CHATLOC_X, CHATLOC_Y - h, 640, h,
			cgs.media.teamStatusBar);
		trap_R_SetColor(NULL);

		hcolor[0]	= hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3]	= 1.0f;

		for(i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--)
			CG_DrawStringExt(
				CHATLOC_X + TINYCHAR_WIDTH,
				CHATLOC_Y -
				(cgs.teamChatPos - i)*TINYCHAR_HEIGHT,
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse,
				qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}
}
#endif	/* MISSIONPACK */

#ifndef MISSIONPACK
static void
CG_DrawHoldableItem(void)
{
	int value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if(value){
		CG_RegisterItemVisuals(value);
		CG_DrawPic(640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE,
			ICON_SIZE,
			cg_items[ value ].icon);
	}

}
#endif	/* !MISSIONPACK */

static void
CG_DrawReward(void)
{
	float	*color;
	int	i, count;
	float	x, y;
	char	buf[32];

	if(!cg_drawRewards.integer)
		return;

	color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
	if(!color){
		if(cg.rewardStack > 0){
			for(i = 0; i < cg.rewardStack; i++){
				cg.rewardSound[i]	= cg.rewardSound[i+1];
				cg.rewardShader[i]	= cg.rewardShader[i+1];
				cg.rewardCount[i]	= cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
			trap_sndstartlocalsound(cg.rewardSound[0], CHAN_ANNOUNCER);
		}else
			return;
	}

	trap_R_SetColor(color);

	/*
	 * count = cg.rewardCount[0]/10;				// number of big rewards to draw
	 *
	 * if (count) {
	 *      y = 4;
	 *      x = 320 - count * ICON_SIZE;
	 *      for ( i = 0 ; i < count ; i++ ) {
	 *              CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
	 *              x += (ICON_SIZE*2);
	 *      }
	 * }
	 *
	 * count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	 */

	if(cg.rewardCount[0] >= 10){
		y	= 56;
		x	= 320 - ICON_SIZE/2;
		CG_DrawPic(x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0]);
		Q_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen(buf)) / 2;
		CG_DrawStringExt(x, y+ICON_SIZE, buf, color, qfalse, qtrue,
			SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
	}else{

		count = cg.rewardCount[0];

		y	= 56;
		x	= 320 - count * ICON_SIZE/2;
		for(i = 0; i < count; i++){
			CG_DrawPic(x, y, ICON_SIZE-4, ICON_SIZE-4,
				cg.rewardShader[0]);
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor(NULL);
}

/*
 * Lagometer
 */

#define LAG_SAMPLES 128

typedef struct lagometer_t lagometer_t;
struct lagometer_t {
	int	frameSamples[LAG_SAMPLES];
	int	frameCount;
	int	snapshotFlags[LAG_SAMPLES];
	int	snapshotSamples[LAG_SAMPLES];
	int	snapshotCount;
};

lagometer_t lagometer;

/*
 * Adds the current interpolate / extrapolate bar for this frame
 */
void
CG_AddLagometerFrameInfo(void)
{
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount &
				(LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
 * Each time a snapshot is received, log its ping time and
 * the number of snapshots that were dropped before it.
 *
 * Pass NULL for a dropped packet.
 */
void
CG_AddLagometerSnapshotInfo(Snap *snap)
{
	/* dropped packet */
	if(!snap){
		lagometer.snapshotSamples[ lagometer.snapshotCount &
					   (LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	/* add this snapshot's info */
	lagometer.snapshotSamples[ lagometer.snapshotCount &
				   (LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount &
				 (LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
 * Should we draw something differnet for long lag vs no packets?
 */
static void
CG_DrawDisconnect(void)
{
	float	x, y;
	int	cmdNum;
	Usrcmd cmd;
	const char *s;
	int w;

	/* draw the phone jack if we are completely past our buffers */
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if(cmd.serverTime <= cg.snap->ps.commandTime
	   || cmd.serverTime > cg.time)	/* special check for map_restart */
		return;

	/* also add text in center of screen */
	s	= "Connection Interrupted";
	w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w/2, 100, s, 1.0F);

	/* blink the icon */
	if((cg.time >> 9) & 1)
		return;

	x	= 640 - 48;
	y	= 480 - 48;

	CG_DrawPic(x, y, 48, 48, trap_R_RegisterShader(P2dart "/net.tga"));
}

#define MAX_LAGOMETER_PING	900
#define MAX_LAGOMETER_RANGE	300

static void
CG_DrawLagometer(void)
{
	int a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int	color;
	float	vscale;

	if(!cg_lagometer.integer || cgs.localServer){
		CG_DrawDisconnect();
		return;
	}

	/*
	 * draw the graph
	 */
#ifdef MISSIONPACK
	x	= 640 - 48;
	y	= 480 - 144;
#else
	x	= 640 - 48;
	y	= 480 - 48;
#endif

	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);

	ax	= x;
	ay	= y;
	aw	= 48;
	ah	= 48;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color	= -1;
	range	= ah / 3;
	mid	= ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	/* draw the frame interpoalte / extrapolate graph */
	for(a = 0; a < aw; a++){
		i	= (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v	= lagometer.frameSamples[i];
		v	*= vscale;
		if(v > 0){
			if(color != 1){
				color = 1;
				trap_R_SetColor(g_color_table[ColorIndex(
								      COLOR_YELLOW)
					]);
			}
			if(v > range)
				v = range;
			trap_R_DrawStretchPic (ax + aw - a, mid - v, 1, v, 0, 0,
				0, 0,
				cgs.media.whiteShader);
		}else if(v < 0){
			if(color != 2){
				color = 2;
				trap_R_SetColor(g_color_table[ColorIndex(
								      COLOR_BLUE)
					]);
			}
			v = -v;
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0,
				cgs.media.whiteShader);
		}
	}

	/* draw the snapshot latency / drop graph */
	range	= ah / 2;
	vscale	= range / MAX_LAGOMETER_PING;

	for(a = 0; a < aw; a++){
		i	= (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v	= lagometer.snapshotSamples[i];
		if(v > 0){
			if(lagometer.snapshotFlags[i] &
			   SNAPFLAG_RATE_DELAYED){
				if(color != 5){
					color = 5;	/* YELLOW for rate delay */
					trap_R_SetColor(g_color_table[ColorIndex(
									      COLOR_YELLOW)
						]);
				}
			}else if(color != 3){
				color = 3;
				trap_R_SetColor(g_color_table[ColorIndex(
								      COLOR_GREEN)
					]);
			}
			v = v * vscale;
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0,
				0, 0, 0,
				cgs.media.whiteShader);
		}else if(v < 0){
			if(color != 4){
				color = 4;	/* RED for dropped snapshots */
				trap_R_SetColor(g_color_table[ColorIndex(
								      COLOR_RED)
					]);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1,
				range, 0, 0, 0, 0,
				cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if(cg_nopredict.integer || cg_synchronousClients.integer)
		CG_DrawBigString(ax, ay, "snc", 1.0);

	CG_DrawDisconnect();
}

/*
 * Centre printing
 */

/*
 * Called for important messages that should stay in the center of the screen
 * for a few moments
 */
void
CG_CenterPrint(const char *str, int y, int charWidth)
{
	char *s;

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	/* count the number of lines for centering */
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while(*s){
		if(*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

static void
CG_DrawCenterString(void)
{
	char	*start;
	int	l;
	int	x, y, w;
#ifdef MISSIONPACK
	int	h;
#endif
	float *color;

	if(!cg.centerPrintTime)
		return;

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if(!color)
		return;

	trap_R_SetColor(color);

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while(1){
		char linebuffer[1024];

		for(l = 0; l < 50; l++){
			if(!start[l] || start[l] == '\n')
				break;
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w	= CG_Text_Width(linebuffer, 0.5, 0);
		h	= CG_Text_Height(linebuffer, 0.5, 0);
		x	= (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0,
			ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		w = cg.centerPrintCharWidth * CG_DrawStrlen(linebuffer);

		x = (SCREEN_WIDTH - w) / 2;

		CG_DrawStringExt(x, y, linebuffer, color, qfalse, qtrue,
			cg.centerPrintCharWidth,
			(int)(cg.centerPrintCharWidth * 1.5), 0);

		y += cg.centerPrintCharWidth * 1.5;
#endif
		while(*start && (*start != '\n'))
			start++;
		if(!*start)
			break;
		start++;
	}

	trap_R_SetColor(NULL);
}

/*
 * Crosshair
 */

static void
CG_DrawCrosshair(void)
{
	Scalar dotw, doth, ringw, ringh, outline;
	Handle hShader;
	float f;
	float x, y;
	int ca;
	Vec4 c1, c2;
	Handle dot, ring;

	if(!cg_drawCrosshair.integer)
		return;
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;
	if(cg.renderingThirdPerson)	/* FIXME */
		return;

	if(cg_crosshairHealth.integer)
		CG_ColorForHealth(c1);	/* set color based on health */
	else
		hextriplet2colour(cg_crosshaircolour1.string, c1);
	hextriplet2colour(cg_crosshaircolour2.string, c2);

	dotw = doth = cg_crosshairdotdiameter.value;
	ringw = ringh = cg_crosshairringdiameter.value;
	outline = cg_crosshairoutline.value;

	/* pulse the size of the crosshair when picking up items */
	f = cg.time - cg.itemPickupBlendTime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f /= ITEM_BLOB_TIME;
		dotw *= (1 + f);
		doth *= (1 + f);
		ringw *= (1 + f);
		ringh *= (1 + f);
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &dotw, &doth);
	CG_AdjustFrom640(&x, &y, &ringw, &ringh);

	/* FIXME */
	dot = trap_R_RegisterShader(Pxhairs "/dot");
	ring = trap_R_RegisterShader(Pxhairs "/ring");

	/*
	 * Draw the outlines
	 */
	trap_R_SetColor(c2);
	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - dotw - outline),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - doth - outline),
		dotw + outline, doth + outline, 0, 0, 1, 1, dot);
	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - ringw - outline),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - ringh - outline),
		ringw + outline, ringh + outline, 0, 0, 1, 1, ring);
	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - dotw + outline),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - doth + outline),
		dotw - outline, doth - outline, 0, 0, 1, 1, dot);
	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - ringw + outline),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - ringh + outline),
		ringw - outline, ringh - outline, 0, 0, 1, 1, ring);
	/*
	 * Draw the main parts
	 */
	trap_R_SetColor(c1);
	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - dotw),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - doth),
		dotw, doth, 0, 0, 1, 1, dot);
	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - ringw),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - ringh),
		ringw, ringh, 0, 0, 1, 1, ring);
	trap_R_SetColor(nil);
}

/* FIXME: implement this with new crosshairs */
static void
CG_DrawCrosshair3D(void)
{
	float	w;
	Handle hShader;
	float	f;
	int	ca;

	Trace trace;
	Vec3	endpos;
	float	stereoSep, zProj, maxdist, xmax;
	char	rendererinfos[128];
	Refent ent;

	if(!cg_drawCrosshair.integer)
		return;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(cg.renderingThirdPerson)
		return;

	//w = cg_crosshairSize.value;

	/* pulse the size of the crosshair when picking up items */
	f = cg.time - cg.itemPickupBlendTime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f	/= ITEM_BLOB_TIME;
		w	*= (1 + f);
	}

	ca = cg_drawCrosshair.integer;
	if(ca < 1)
		return;
	hShader = cgs.media.crosshairShader[ (ca - 1) % NUM_CROSSHAIRS ];

	/* Use a different method rendering the crosshair so players don't see two of them when
	 * focusing their eyes at distant objects with high stereo separation
	 * We are going to trace to the next shootable object and place the crosshair in front of it. */

	/* first get all the important renderer information */
	trap_cvargetstrbuf("r_zProj", rendererinfos,
		sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_cvargetstrbuf("r_stereoSeparation", rendererinfos,
		sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);

	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);

	/* let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel. */
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	saddv3(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);

	memset(&ent, 0, sizeof(ent));
	ent.reType	= RT_SPRITE;
	ent.renderfx	= RF_DEPTHHACK | RF_CROSSHAIR;

	copyv3(trace.endpos, ent.origin);

	/* scale the crosshair so it appears the same size for all distances */
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}

static void
CG_ScanForCrosshairEntity(void)
{
	Trace trace;
	Vec3	start, end;
	int content;

	copyv3(cg.refdef.vieworg, start);
	saddv3(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY);
	if(trace.entityNum >= MAX_CLIENTS)
		return;

	/* if the player is in fog, don't show it */
	content = CG_PointContents(trace.endpos, 0);
	if(content & CONTENTS_FOG)
		return;

	/* if the player is invisible, don't show it */
	if(cg_entities[ trace.entityNum ].currentState.powerups &
	   (1 << PW_INVIS))
		return;

	/* update the fade timer */
	cg.crosshairClientNum	= trace.entityNum;
	cg.crosshairClientTime	= cg.time;
}

static void
CG_DrawCrosshairNames(void)
{
	float	*color;
	char	*name;
	float	w;

	if(!cg_drawCrosshair.integer)
		return;
	if(!cg_drawCrosshairNames.integer)
		return;
	if(cg.renderingThirdPerson)
		return;

	/* scan the known entities to see if the crosshair is sighted on one */
	CG_ScanForCrosshairEntity();

	/* draw the name of the player being looked at */
	color = CG_FadeColor(cg.crosshairClientTime, 1000);
	if(!color){
		trap_R_SetColor(NULL);
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint(320 - w / 2, 190, 0.3f, color, name, 0, 0,
		ITEM_TEXTSTYLE_SHADOWED);
#else
	w = CG_DrawStrlen(name) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 170, name, color[3] * 0.5f);
#endif
	trap_R_SetColor(NULL);
}

static void
CG_DrawSpectator(void)
{
	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if(cgs.gametype == GT_TOURNAMENT || cgs.gametype == GT_DUEL)
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	else if(cgs.gametype >= GT_TEAM)
		CG_DrawBigString(320 - 39 * 8, 460,
			"press ESC and use the JOIN menu to play",
			1.0F);
}

static void
CG_DrawVote(void)
{
	char	*s;
	int	sec;

	if(!cgs.voteTime)
		return;

	/* play a talk beep whenever it is modified */
	if(cgs.voteModified){
		cgs.voteModified = qfalse;
		trap_sndstartlocalsound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
	if(sec < 0)
		sec = 0;

#ifdef MISSIONPACK
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes,
		cgs.voteNo);
	CG_DrawSmallString(0, 58, s, 1.0F);
	s = "or press ESC then click Vote";
	CG_DrawSmallString(0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F);
#else
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes,
		cgs.voteNo);
	CG_DrawSmallString(0, 58, s, 1.0F);
#endif
}

static void
CG_DrawTeamVote(void)
{
	char	*s;
	int	sec, cs_offset;

	if(cgs.clientinfo[cg.clientNum].team == TEAM_RED)
		cs_offset = 0;
	else if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!cgs.teamVoteTime[cs_offset])
		return;

	/* play a talk beep whenever it is modified */
	if(cgs.teamVoteModified[cs_offset]){
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_sndstartlocalsound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
	if(sec < 0)
		sec = 0;
	s =
		va("TEAMVOTE(%i):%s yes:%i no:%i", sec,
			cgs.teamVoteString[cs_offset],
			cgs.teamVoteYes[cs_offset],
			cgs.teamVoteNo[cs_offset]);
	CG_DrawSmallString(0, 90, s, 1.0F);
}

static qbool
CG_DrawScoreboard(void)
{
#ifdef MISSIONPACK
	static qbool firstTime = qtrue;

	if(menuScoreboard)
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	if(cg_paused.integer){
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	/* should never happen in Team Arena */
	if(cgs.gametype == GT_SINGLE_PLAYER &&
	   cg.predictedPlayerState.pm_type == PM_INTERMISSION){
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	/* don't draw scoreboard during death while warmup up */
	if(cg.warmup && !cg.showScores)
		return qfalse;

	if(cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
	   cg.predictedPlayerState.pm_type == PM_INTERMISSION){
	}else if(!CG_FadeColor(cg.scoreFadeTime, FADE_TIME)){
		/* next time scoreboard comes up, don't print killer */
		cg.deferredPlayerLoading = 0;
		cg.killerName[0] = 0;
		firstTime = qtrue;
		return qfalse;
	}

	if(menuScoreboard == NULL){
		if(cgs.gametype >= GT_TEAM)
			menuScoreboard = Menus_FindByName("teamscore_menu");
		else
			menuScoreboard = Menus_FindByName("score_menu");
	}

	if(menuScoreboard){
		if(firstTime){
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	/* load any models that have been deferred */
	if(++cg.deferredPlayerLoading > 10)
		CG_LoadDeferredPlayers();

	return qtrue;
#else
	return CG_DrawOldScoreboard();
#endif
}

static void
CG_DrawIntermission(void)
{
/*	int key; */
#ifdef MISSIONPACK
	/* if (cg_singlePlayer.integer) {
	 * CG_DrawCenterString();
	 * return;
	 * } */
#else
	if(cgs.gametype == GT_SINGLE_PLAYER){
		CG_DrawCenterString();
		return;
	}
#endif
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

static qbool
CG_DrawFollow(void)
{
	float	x;
	Vec4	color;
	const char *name;

	if(!(cg.snap->ps.pm_flags & PMF_FOLLOW))
		return qfalse;
	color[0]	= 1;
	color[1]	= 1;
	color[2]	= 1;
	color[3]	= 1;


	CG_DrawBigString(320 - 9 * 8, 24, "following", 1.0F);

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(name));

	CG_DrawStringExt(x, 40, name, color, qtrue, qtrue, GIANT_WIDTH,
		GIANT_HEIGHT,
		0);

	return qtrue;
}

static void
CG_DrawAmmoWarning(void)
{
	const char *s;
	int w;

	if(cg_drawAmmoWarning.integer == 0)
		return;

	if(!cg.lowAmmoWarning)
		return;

	if(cg.lowAmmoWarning == 2)
		s = "OUT OF AMMO";
	else
		s = "LOW AMMO WARNING";
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}

static void
CG_DrawProxWarning(void)
{
	char	s [32];
	int	w;
	static int	proxTime;
	static int	proxCounter;
	static int	proxTick;

	if(!(cg.snap->ps.eFlags & EF_TICKING)){
		proxTime = 0;
		return;
	}

	if(proxTime == 0){
		proxTime = cg.time + 5000;
		proxCounter = 5;
		proxTick = 0;
	}

	if(cg.time > proxTime){
		proxTick	= proxCounter--;
		proxTime	= cg.time + 1000;
	}

	if(proxTick != 0)
		Q_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
	else
		Q_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");

	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor(320 - w / 2, 64 + BIGCHAR_HEIGHT, s,
		g_color_table[ColorIndex(
				      COLOR_RED)]);
}

static void
CG_DrawWarmup(void)
{
	int	w;
	int	sec;
	int	i;
#ifdef MISSIONPACK
	float	scale;
#else
	int	cw;
#endif
	Clientinfo *ci1, *ci2;
	const char *s;

	sec = cg.warmup;
	if(!sec)
		return;

	if(sec < 0){
		s	= "Waiting for players";
		w	= CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if(cgs.gametype == GT_TOURNAMENT){
		/* find the two active players */
		ci1	= NULL;
		ci2	= NULL;
		for(i = 0; i < cgs.maxclients; i++)
			if(cgs.clientinfo[i].infoValid &&
			   cgs.clientinfo[i].team == TEAM_FREE){
				if(!ci1)
					ci1 = &cgs.clientinfo[i];
				else
					ci2 = &cgs.clientinfo[i];
			}

		if(ci1 && ci2){
			s = va("%s vs %s", ci1->name, ci2->name);
#ifdef MISSIONPACK
			w = CG_Text_Width(s, 0.6f, 0);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0,
				ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
			w = CG_DrawStrlen(s);
			if(w > 640 / GIANT_WIDTH)
				cw = 640 / w;
			else
				cw = GIANT_WIDTH;
			CG_DrawStringExt(320 - w * cw/2, 20,s, colorWhite,
				qfalse, qtrue, cw, (int)(cw * 1.5f), 0);
#endif
		}
	}else{
		if(cgs.gametype == GT_FFA)
			s = "Free For All";
		else if(cgs.gametype == GT_TEAM)
			s = "Team Deathmatch";
		else if(cgs.gametype == GT_CTF)
			s = "Capture the Flag";
		else if(cgs.gametype == GT_1FCTF)
			s = "One Flag CTF";
		else
			s = "";

#ifdef MISSIONPACK
		w = CG_Text_Width(s, 0.6f, 0);
		CG_Text_Paint(320 - w / 2, 90, 0.6f, colorWhite, s, 0, 0,
			ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
		w = CG_DrawStrlen(s);
		if(w > 640 / GIANT_WIDTH)
			cw = 640 / w;
		else
			cw = GIANT_WIDTH;
		CG_DrawStringExt(320 - w * cw/2, 25,s, colorWhite,
			qfalse, qtrue, cw, (int)(cw * 1.1f), 0);
#endif
	}

	sec = (sec - cg.time) / 1000;
	if(sec < 0){
		cg.warmup = 0;
		sec = 0;
	}
	s = va("Starts in: %i", sec + 1);
	if(sec != cg.warmupCount){
		cg.warmupCount = sec;
		switch(sec){
		case 0:
			trap_sndstartlocalsound(cgs.media.count1Sound,
				CHAN_ANNOUNCER);
			break;
		case 1:
			trap_sndstartlocalsound(cgs.media.count2Sound,
				CHAN_ANNOUNCER);
			break;
		case 2:
			trap_sndstartlocalsound(cgs.media.count3Sound,
				CHAN_ANNOUNCER);
			break;
		default:
			break;
		}
	}

#ifdef MISSIONPACK
	switch(cg.warmupCount){
	case 0:
		scale = 0.54f;
		break;
	case 1:
		scale = 0.51f;
		break;
	case 2:
		scale = 0.48f;
		break;
	default:
		scale = 0.45f;
		break;
	}

	w = CG_Text_Width(s, scale, 0);
	CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0,
		ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
	switch(cg.warmupCount){
	case 0:
		cw = 28;
		break;
	case 1:
		cw = 24;
		break;
	case 2:
		cw = 20;
		break;
	default:
		cw = 16;
		break;
	}

	w = CG_DrawStrlen(s);
	CG_DrawStringExt(320 - w * cw/2, 70, s, colorWhite,
		qfalse, qtrue, cw, (int)(cw * 1.5), 0);
#endif
}

#ifdef MISSIONPACK
void
CG_DrawTimedMenus(void)
{
	if(cg.voiceTime){
		int t = cg.time - cg.voiceTime;
		if(t > 2500){
			Menus_CloseByName("voiceMenu");
			trap_cvarsetstr("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif

static void
CG_Draw2D(Stereoframe stereoFrame)
{
#ifdef MISSIONPACK
	if(cgs.orderPending && cg.time > cgs.orderTime)
		CG_CheckOrderPending();

#endif
	/* if we are taking a levelshot for the menu, don't draw anything */
	if(cg.levelShot)
		return;

	if(cg_draw2D.integer == 0)
		return;

	if(cg.snap->ps.pm_type == PM_INTERMISSION){
		CG_DrawIntermission();
		return;
	}

/*
 *      if (cg.cameraMode) {
 *              return;
 *      }
 */
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR){
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();
	}else{
		/* don't draw any status if dead or the scoreboard is being explicitly shown */
		if(!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0){

#ifdef MISSIONPACK
			if(cg_drawStatus.integer){
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#else
			CG_DrawStatusBar();
#endif

			CG_DrawAmmoWarning();

#ifdef MISSIONPACK
			CG_DrawProxWarning();
#endif
			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect(WSpri);
			CG_DrawWeaponSelect(WSsec);
			CG_DrawHoldableItem();
			CG_DrawReward();
		}

		if(cgs.gametype >= GT_TEAM){
#ifndef MISSIONPACK
			CG_DrawTeamInfo();
#endif
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

#ifdef MISSIONPACK
	if(!cg_paused.integer)
		CG_DrawUpperRight(stereoFrame);

#else
	CG_DrawUpperRight(stereoFrame);
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	if(!CG_DrawFollow())
		CG_DrawWarmup();

	/* don't draw center string if scoreboard is up */
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if(!cg.scoreBoardShowing)
		CG_DrawCenterString();
}

static void
CG_DrawTourneyScoreboard(void)
{
#ifdef MISSIONPACK
#else
	CG_DrawOldTourneyScoreboard();
#endif
}

/*
 * Perform all drawing needed to completely fill the screen
 */
void
CG_DrawActive(Stereoframe stereoView)
{
	/* optionally draw the info screen instead */
	if(!cg.snap){
		CG_DrawInformation();
		return;
	}

	/* optionally draw the tournement scoreboard instead */
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
	   (cg.snap->ps.pm_flags & PMF_SCOREBOARD)){
		CG_DrawTourneyScoreboard();
		return;
	}

	/* clear around the rendered view if sized down */
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	/* draw 3D view */
	trap_R_RenderScene(&cg.refdef);

	/* draw status bar and other floating elements */
	CG_Draw2D(stereoView);
}
