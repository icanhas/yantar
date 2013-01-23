/* Main menu */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "ui.h"
#include "local.h"

enum {
	ID_SINGLEPLAYER	= 10,
	ID_MULTIPLAYER	= 11,
	ID_SETUP	= 12,
	ID_DEMOS	= 13,
	ID_CINEMATICS	= 14,
	ID_MODS	= 15,
	ID_EXIT	= 16,

	Xbegin	= 16,	/* starting position of menu items */
	Ybegin	= 134,

	Yspacing	= 24	/* vertical space between items */
};

typedef struct {
	menuframework_s menu;

	menutext_s	singleplayer;
	menutext_s	multiplayer;
	menutext_s	setup;
	menutext_s	demos;
	menutext_s	cinematics;
	menutext_s	teamArena;
	menutext_s	mods;
	menutext_s	exit;

	Handle		bannerModel;
} mainmenu_t;


static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;
	char		errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

static void
MainMenu_ExitAction(qbool result)
{
	if(!result)
		return;
	UI_PopMenu();
	UI_CreditMenu();
}

void
Main_MenuEvent(void* ptr, int event)
{
	if(event != QM_ACTIVATED)
		return;
	switch(((menucommon_s*)ptr)->id){
	case ID_SINGLEPLAYER:
		UI_SPLevelMenu();
		break;
	case ID_MULTIPLAYER:
		UI_ArenaServersMenu();
		break;
	case ID_SETUP:
		UI_SetupMenu();
		break;
	case ID_DEMOS:
		UI_DemosMenu();
		break;
	case ID_CINEMATICS:
		UI_CinematicsMenu();
		break;
	case ID_MODS:
		UI_ModsMenu();
		break;
	case ID_EXIT:
		UI_ConfirmMenu("EXIT GAME?", 0, MainMenu_ExitAction);
		break;
	}
}

void
MainMenu_Cache(void)
{
}

Sfxhandle
ErrorMessage_Key(int key)
{
	trap_cvarsetstr("com_errorMessage", "");
	UI_MainMenu();
	return menu_null_sound;
}

/*
 * Main_MenuDraw: TTimo: this function is common to the main menu and
 * errorMessage menu
 */
static void
Main_MenuDraw(void)
{
	Refdef	refdef;
	float		x, y, w, h;
	Vec4		color = {0.5, 0, 0.5, 0.8};

	/* setup the refdef */
	memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = RDF_NOWORLDMODEL;
	clearaxis(refdef.viewaxis);

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640(&x, &y, &w, &h);
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = 60;
	refdef.fov_y = 19.6875;

	refdef.time = uis.realtime;

	trap_R_ClearScene();

	/*
	 * // add the model
	 * memset( &ent, 0, sizeof(ent) );
	 * adjust = 5.0 * sin( (float)uis.realtime / 5000 );
	 * setv3( angles, 0, 180 + adjust, 0 );
	 * eulertoaxis( angles, ent.axis );
	 * ent.hModel = s_main.bannerModel;
	 * copyv3( origin, ent.origin );
	 * copyv3( origin, ent.lightingOrigin );
	 * ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	 * copyv3( ent.origin, ent.oldorigin );
	 *
	 * trap_R_AddRefEntityToScene( &ent );
	 * trap_R_RenderScene( &refdef );
	 */

	if(strlen(s_errorMessage.errorMessage))
		/* draw error message instead of menu if we have been dropped */
		UI_DrawProportionalString_AutoWrapped(320, 192, 600, 20,
			s_errorMessage.errorMessage,
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color);
	else
		Menu_Draw(&s_main.menu);


	UI_DrawString(320, 450, "Y A N T A R", UI_CENTER|UI_SMALLFONT, color);
}

/*
 * UI_MainMenu: The main menu only comes up when not in a game, so make sure
 * that the attract loop server is down and that local cinematics are killed
 */
void
UI_MainMenu(void)
{
	int	y;
	qbool teamArena = qfalse;
	int	style = 0;	/* UI_CENTER | UI_DROPSHADOW; */

	trap_cvarsetstr("sv_killserver", "1");

	memset(&s_main, 0,sizeof(mainmenu_t));
	memset(&s_errorMessage, 0,sizeof(errorMessage_t));

	/* com_errorMessage would need that too */
	MainMenu_Cache();	/* cleanme... */

	trap_cvargetstrbuf("com_errorMessage",
		s_errorMessage.errorMessage,
		sizeof(s_errorMessage.errorMessage));
	if(strlen(s_errorMessage.errorMessage)){
		/* we have an error */
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen	= qtrue;
		s_errorMessage.menu.wrapAround	= qtrue;
		s_errorMessage.menu.showlogo	= qtrue;

		trap_Key_SetCatcher(KEYCATCH_UI);
		uis.menusp = 0;
		UI_PushMenu(&s_errorMessage.menu);

		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen	= qtrue;
	s_main.menu.wrapAround	= qtrue;
	s_main.menu.showlogo	= qtrue;

	y = Ybegin;
	s_main.singleplayer.generic.type	= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags	= QMF_LEFT_JUSTIFY|
						  QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x	= Xbegin;
	s_main.singleplayer.generic.y	= y;
	s_main.singleplayer.generic.id	= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback = Main_MenuEvent;
	s_main.singleplayer.string	= "SINGLE PLAYER";
	s_main.singleplayer.color	= color_black;
	s_main.singleplayer.style	= style;

	y += Yspacing;
	s_main.multiplayer.generic.type		= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags	= QMF_LEFT_JUSTIFY|
						  QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x	= Xbegin;
	s_main.multiplayer.generic.y	= y;
	s_main.multiplayer.generic.id	= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback = Main_MenuEvent;
	s_main.multiplayer.string	= "MULTIPLAYER";
	s_main.multiplayer.color	= color_black;
	s_main.multiplayer.style	= style;

	y += Yspacing;
	s_main.setup.generic.type	= MTYPE_PTEXT;
	s_main.setup.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x	= Xbegin;
	s_main.setup.generic.y	= y;
	s_main.setup.generic.id = ID_SETUP;
	s_main.setup.generic.callback = Main_MenuEvent;
	s_main.setup.string	= "Options";
	s_main.setup.color	= color_black;
	s_main.setup.style	= style;

	y += Yspacing;
	s_main.demos.generic.type	= MTYPE_PTEXT;
	s_main.demos.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x	= Xbegin;
	s_main.demos.generic.y	= y;
	s_main.demos.generic.id = ID_DEMOS;
	s_main.demos.generic.callback = Main_MenuEvent;
	s_main.demos.string	= "DEMOS";
	s_main.demos.color	= color_black;
	s_main.demos.style	= style;

	y += Yspacing;
	s_main.cinematics.generic.type	= MTYPE_PTEXT;
	s_main.cinematics.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x	= Xbegin;
	s_main.cinematics.generic.y	= y;
	s_main.cinematics.generic.id	= ID_CINEMATICS;
	s_main.cinematics.generic.callback = Main_MenuEvent;
	s_main.cinematics.string	= "CINEMATICS";
	s_main.cinematics.color		= color_black;
	s_main.cinematics.style		= style;

	y += Yspacing;
	s_main.mods.generic.type = MTYPE_PTEXT;
	s_main.mods.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x		= Xbegin;
	s_main.mods.generic.y		= y;
	s_main.mods.generic.id		= ID_MODS;
	s_main.mods.generic.callback = Main_MenuEvent;
	s_main.mods.string	= "MODS";
	s_main.mods.color	= color_black;
	s_main.mods.style	= style;

	y += Yspacing;
	s_main.exit.generic.type = MTYPE_PTEXT;
	s_main.exit.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x		= Xbegin;
	s_main.exit.generic.y		= y;
	s_main.exit.generic.id		= ID_EXIT;
	s_main.exit.generic.callback = Main_MenuEvent;
	s_main.exit.string	= "EXIT";
	s_main.exit.color	= color_black;
	s_main.exit.style	= style;

	Menu_AddItem(&s_main.menu, &s_main.singleplayer);
	Menu_AddItem(&s_main.menu, &s_main.multiplayer);
	Menu_AddItem(&s_main.menu, &s_main.setup);
	Menu_AddItem(&s_main.menu, &s_main.demos);
	Menu_AddItem(&s_main.menu, &s_main.cinematics);
	if(teamArena)
		Menu_AddItem(&s_main.menu, &s_main.teamArena);
	Menu_AddItem(&s_main.menu,&s_main.mods);
	Menu_AddItem(&s_main.menu,&s_main.exit);

	trap_Key_SetCatcher(KEYCATCH_UI);
	uis.menusp = 0;
	UI_PushMenu(&s_main.menu);
}
