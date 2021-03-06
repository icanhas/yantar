/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "shared.h"
#include "ref.h"
/* NOTE: include the public.h from the new UI */
#include "paths.h"
/* redefine to old API version */
#undef UI_API_VERSION
#define UI_API_VERSION 0
#include "../client/keycodes.h"
#include "bg.h"

typedef void (*voidfunc_f)(void);

extern Vmcvar ui_ffa_fraglimit;
extern Vmcvar ui_ffa_timelimit;

extern Vmcvar ui_tourney_fraglimit;
extern Vmcvar ui_tourney_timelimit;

extern Vmcvar ui_team_fraglimit;
extern Vmcvar ui_team_timelimit;
extern Vmcvar ui_team_friendly;

extern Vmcvar ui_ctf_capturelimit;
extern Vmcvar ui_ctf_timelimit;
extern Vmcvar ui_ctf_friendly;

extern Vmcvar ui_arenasFile;
extern Vmcvar ui_botsFile;
extern Vmcvar ui_spScores1;
extern Vmcvar ui_spScores2;
extern Vmcvar ui_spScores3;
extern Vmcvar ui_spScores4;
extern Vmcvar ui_spScores5;
extern Vmcvar ui_spAwards;
extern Vmcvar ui_spVideos;
extern Vmcvar ui_spSkill;

extern Vmcvar ui_spSelection;

extern Vmcvar ui_browserMaster;
extern Vmcvar ui_browserGameType;
extern Vmcvar ui_browserSortKey;
extern Vmcvar ui_browserShowFull;
extern Vmcvar ui_browserShowEmpty;

extern Vmcvar ui_brassTime;
extern Vmcvar ui_drawCrosshair;
extern Vmcvar ui_drawCrosshairNames;
extern Vmcvar ui_marks;

extern Vmcvar ui_server1;
extern Vmcvar ui_server2;
extern Vmcvar ui_server3;
extern Vmcvar ui_server4;
extern Vmcvar ui_server5;
extern Vmcvar ui_server6;
extern Vmcvar ui_server7;
extern Vmcvar ui_server8;
extern Vmcvar ui_server9;
extern Vmcvar ui_server10;
extern Vmcvar ui_server11;
extern Vmcvar ui_server12;
extern Vmcvar ui_server13;
extern Vmcvar ui_server14;
extern Vmcvar ui_server15;
extern Vmcvar ui_server16;

extern Vmcvar ui_skipExitCredits;


/*
 * ui_qmenu.c
 *  */

#define RCOLUMN_OFFSET		(BIGCHAR_WIDTH)
#define LCOLUMN_OFFSET		(-BIGCHAR_WIDTH)

#define SLIDER_RANGE		10
#define MAX_EDIT_LINE		256

#define MAX_MENUDEPTH		8
#define MAX_MENUITEMS		64

#define MTYPE_NULL		0
#define MTYPE_SLIDER		1
#define MTYPE_ACTION		2
#define MTYPE_SPINCONTROL	3
#define MTYPE_FIELD		4
#define MTYPE_RADIOBUTTON	5
#define MTYPE_BITMAP		6
#define MTYPE_TEXT		7
#define MTYPE_SCROLLLIST	8
#define MTYPE_PTEXT		9
#define MTYPE_BTEXT		10

#define QMF_BLINK		((unsigned int)0x00000001)
#define QMF_SMALLFONT		((unsigned int)0x00000002)
#define QMF_LEFT_JUSTIFY	((unsigned int)0x00000004)
#define QMF_CENTER_JUSTIFY	((unsigned int)0x00000008)
#define QMF_RIGHT_JUSTIFY	((unsigned int)0x00000010)
#define QMF_NUMBERSONLY		((unsigned int)0x00000020)	/* edit field is only numbers */
#define QMF_HIGHLIGHT		((unsigned int)0x00000040)
#define QMF_HIGHLIGHT_IF_FOCUS	((unsigned int)0x00000080)	/* steady focus */
#define QMF_PULSEIFFOCUS	((unsigned int)0x00000100)	/* pulse if focus */
#define QMF_HASMOUSEFOCUS	((unsigned int)0x00000200)
#define QMF_NOONOFFTEXT		((unsigned int)0x00000400)
#define QMF_MOUSEONLY		((unsigned int)0x00000800)	/* only mouse input allowed */
#define QMF_HIDDEN		((unsigned int)0x00001000)	/* skips drawing */
#define QMF_GRAYED		((unsigned int)0x00002000)	/* grays and disables */
#define QMF_INACTIVE		((unsigned int)0x00004000)	/* disables any input */
#define QMF_NODEFAULTINIT	((unsigned int)0x00008000)	/* skip default initialization */
#define QMF_OWNERDRAW		((unsigned int)0x00010000)
#define QMF_PULSE		((unsigned int)0x00020000)
#define QMF_LOWERCASE		((unsigned int)0x00040000)	/* edit field is all lower case */
#define QMF_UPPERCASE		((unsigned int)0x00080000)	/* edit field is all upper case */
#define QMF_SILENT		((unsigned int)0x00100000)

/* callback notifications */
#define QM_GOTFOCUS	1
#define QM_LOSTFOCUS	2
#define QM_ACTIVATED	3

typedef struct _tag_menuframework {
	int	cursor;
	int	cursor_prev;

	int	nitems;
	void	*items[MAX_MENUITEMS];

	void (*draw)(void);
	Handle (*key)(int key);

	qbool		wrapAround;
	qbool		fullscreen;
	qbool		showlogo;
} menuframework_s;

typedef struct {
	int		type;
	const char	*name;
	int		id;
	int		x, y;
	int		left;
	int		top;
	int		right;
	int		bottom;
	menuframework_s *parent;
	int		menuPosition;
	unsigned int	flags;

	void (*callback)(void *self, int event);
	void (*statusbar)(void *self);
	void (*ownerdraw)(void *self);
} menucommon_s;

typedef struct {
	int	cursor;
	int	scroll;
	int	widthInChars;
	char	buffer[MAX_EDIT_LINE];
	int	maxchars;
} mfield_t;

typedef struct {
	menucommon_s	generic;
	mfield_t	field;
} menufield_s;

typedef struct {
	menucommon_s	generic;

	float		minvalue;
	float		maxvalue;
	float		curvalue;

	float		range;
} menuslider_s;

typedef struct {
	menucommon_s	generic;

	int		oldvalue;
	int		curvalue;
	int		numitems;
	int		top;

	const char	**itemnames;

	int		width;
	int		height;
	int		columns;
	int		seperation;
} menulist_s;

typedef struct {
	menucommon_s generic;
} menuaction_s;

typedef struct {
	menucommon_s	generic;
	int		curvalue;
} menuradiobutton_s;

typedef struct {
	menucommon_s	generic;
	char		* focuspic;
	char		* errorpic;
	Handle		shader;
	Handle		focusshader;
	int		width;
	int		height;
	float		* focuscolor;
} menubitmap_s;

typedef struct {
	menucommon_s	generic;
	char		* string;
	int		style;
	float		* color;
} menutext_s;

extern void                     Menu_Cache(void);
extern void                     Menu_Focus(menucommon_s *m);
extern void                     Menu_AddItem(menuframework_s *menu, void *item);
extern void                     Menu_AdjustCursor(menuframework_s *menu, int dir);
extern void                     Menu_Draw(menuframework_s *menu);
extern void*Menu_ItemAtCursor(menuframework_s *m);
extern Handle      Menu_ActivateItem(menuframework_s *s, menucommon_s* item);
extern void                     Menu_SetCursor(menuframework_s *s, int cursor);
extern void                     Menu_SetCursorToItem(menuframework_s *m,
						     void* ptr);
extern Handle      Menu_DefaultKey(menuframework_s *s, int key);
extern void                     Bitmap_Init(menubitmap_s *b);
extern void                     Bitmap_Draw(menubitmap_s *b);
extern void                     ScrollList_Draw(menulist_s *l);
extern Handle      ScrollList_Key(menulist_s *l, int key);
extern Handle	menu_in_sound;
extern Handle	menu_move_sound;
extern Handle	menu_out_sound;
extern Handle	menu_buzz_sound;
extern Handle	menu_null_sound;
extern Handle	weaponChangeSound;
extern Vec4	menu_text_color;
extern Vec4	menu_grayed_color;
extern Vec4	menu_dark_color;
extern Vec4	menu_highlight_color;
extern Vec4	menu_red_color;
extern Vec4	menu_black_color;
extern Vec4	menu_dim_color;
extern Vec4	color_black;
extern Vec4	color_white;
extern Vec4	color_yellow;
extern Vec4	color_blue;
extern Vec4	color_orange;
extern Vec4	color_red;
extern Vec4	color_dim;
extern Vec4	name_color;
extern Vec4	list_color;
extern Vec4	listbar_color;
extern Vec4	text_color_disabled;
extern Vec4	text_color_normal;
extern Vec4	text_color_highlight;

extern char *ui_medalNames[];
extern char *ui_medalPicNames[];
extern char *ui_medalSounds[];

/*
 * ui_mfield.c
 *  */
extern void                     Mfieldclear(mfield_t *edit);
extern void                     MField_KeyDownEvent(mfield_t *edit, int key);
extern void                     MField_CharEvent(mfield_t *edit, int ch);
extern void                     MField_Draw(mfield_t *edit, int x, int y,
					    int style,
					    Vec4 color);
extern void                     MenuField_Init(menufield_s* m);
extern void                     MenuField_Draw(menufield_s *f);
extern Handle      MenuField_Key(menufield_s* m, int* key);

/*
 * ui_menu.c
 *  */
extern void MainMenu_Cache(void);
extern void UI_MainMenu(void);
extern void UI_RegisterCvars(void);
extern void UI_UpdateCvars(void);

/*
 * ui_credits.c
 *  */
extern void UI_CreditMenu(void);

/*
 * ui_ingame.c
 *  */
extern void InGame_Cache(void);
extern void UI_InGameMenu(void);

/*
 * ui_confirm.c
 *  */
extern void ConfirmMenu_Cache(void);
extern void UI_ConfirmMenu(const char *question, void (*draw)(
				   void), void (*action)(qbool result));
extern void UI_ConfirmMenu_Style(const char *question, int style, void (*draw)(
					 void), void (*action)(qbool result));
extern void UI_Message(const char **lines);

/*
 * ui_setup.c
 *  */
extern void UI_SetupMenu_Cache(void);
extern void UI_SetupMenu(void);

/*
 * ui_team.c
 *  */
extern void UI_TeamMainMenu(void);
extern void TeamMain_Cache(void);

/*
 * ui_connect.c
 *  */
extern void UI_DrawConnectScreen(qbool overlay);

/*
 * ui_controls2.c
 *  */
extern void UI_ControlsMenu(void);
extern void Controls_Cache(void);

/*
 * ui_demo2.c
 *  */
extern void UI_DemosMenu(void);
extern void Demos_Cache(void);

/*
 * ui_cinematics.c
 *  */
extern void UI_CinematicsMenu(void);
extern void UI_CinematicsMenu_f(void);
extern void UI_CinematicsMenu_Cache(void);

/*
 * ui_mods.c
 *  */
extern void UI_ModsMenu(void);
extern void UI_ModsMenu_Cache(void);

/*
 * ui_playermodel.c
 *  */
extern void UI_PlayerModelMenu(void);
extern void PlayerModel_Cache(void);

/*
 * ui_playersettings.c
 *  */
extern void UI_PlayerSettingsMenu(void);
extern void PlayerSettings_Cache(void);

/*
 * ui_preferences.c
 *  */
extern void UI_PreferencesMenu(void);
extern void Preferences_Cache(void);

/*
 * ui_specifyleague.c
 *  */
extern void UI_SpecifyLeagueMenu(void);
extern void SpecifyLeague_Cache(void);

/*
 * ui_specifyserver.c
 *  */
extern void UI_SpecifyServerMenu(void);
extern void SpecifyServer_Cache(void);

/*
 * ui_servers2.c
 *  */
#define MAX_FAVORITESERVERS 16

extern void UI_ArenaServersMenu(void);
extern void ArenaServers_Cache(void);

/*
 * ui_startserver.c
 *  */
extern void UI_StartServerMenu(qbool multiplayer);
extern void StartServer_Cache(void);
extern void ServerOptions_Cache(void);
extern void UI_BotSelectMenu(char *bot);
extern void UI_BotSelectMenu_Cache(void);

/*
 * ui_serverinfo.c
 *  */
extern void UI_ServerInfoMenu(void);
extern void ServerInfo_Cache(void);

/*
 * ui_video.c
 *  */
extern void UI_GraphicsOptionsMenu(void);
extern void GraphicsOptions_Cache(void);
extern void DriverInfo_Cache(void);

/*
 * ui_players.c
 *  */

/* FIXME ripped from local.h */
typedef struct {
	int		oldFrame;
	int		oldFrameTime;	/* time when ->oldFrame was exactly on */

	int		frame;
	int		frameTime;	/* time when ->frame will be exactly on */

	float		backlerp;

	float		yawAngle;
	qbool		yawing;
	float		pitchAngle;
	qbool		pitching;

	int		animationNumber;	/* may include ANIM_TOGGLEBIT */
	Anim	*animation;
	int		animationTime;	/* time when the first frame of the animation will be exact */
} Lerpframe;

typedef struct {
	/* model info */
	Handle		legsModel;
	Handle		legsSkin;
	Lerpframe	legs;

	Handle		torsoModel;
	Handle		torsoSkin;
	Lerpframe	torso;

	Handle		headModel;
	Handle		headSkin;

	Anim	animations[MAX_ANIMATIONS];

	Handle		weaponModel;
	Handle		barrelModel;
	Handle		flashModel;
	Vec3		flashDlightColor;
	int		muzzleFlashTime;

	Vec3		color1;
	byte		c1RGBA[4];

	/* currently in use drawing parms */
	Vec3		viewAngles;
	Vec3		moveAngles;
	Weapon	currentWeapon;
	int		legsAnim;
	int		torsoAnim;

	/* animation vars */
	Weapon	weapon;
	Weapon	lastWeapon;
	Weapon	pendingWeapon;
	int		weaponTimer;
	int		pendingLegsAnim;
	int		torsoAnimationTimer;

	int		pendingTorsoAnim;
	int		legsAnimationTimer;

	qbool		chat;
	qbool		newModel;

	qbool		barrelSpinning;
	float		barrelAngle;
	int		barrelTime;

	int		realWeapon;
} Playerinfo;

void UI_DrawPlayer(float x, float y, float w, float h, Playerinfo *pi,
		   int time);
void UI_PlayerInfo_SetModel(Playerinfo *pi, const char *model);
void UI_PlayerInfo_SetInfo(Playerinfo *pi, int legsAnim, int torsoAnim,
			   Vec3 viewAngles, Vec3 moveAngles,
			   Weapon weaponNum,
			   qbool chat);
qbool UI_RegisterClientModelname(Playerinfo *pi, const char *modelSkinName);

/*
 * ui_atoms.c
 *  */
typedef struct {
	int		frametime;
	int		realtime;
	int		cursorx;
	int		cursory;
	int		menusp;
	menuframework_s * activemenu;
	menuframework_s * stack[MAX_MENUDEPTH];
	Glconfig	glconfig;
	qbool		debug;
	Handle		whiteShader;
	Handle		menuBackShader;
	Handle		menuBackNoLogoShader;
	Handle		charset;
	Handle		charsetProp;
	Handle		charsetPropGlow;
	Handle		charsetPropB;
	Handle		cursor;
	Handle		rb_on;
	Handle		rb_off;
	float		xscale;
	float		yscale;
	float		bias;
	qbool		demoversion;
	qbool		firstdraw;
} UIstatic;

extern void                     UI_Init(void);
extern void                     UI_Shutdown(void);
extern void                     UI_KeyEvent(int key, int down);
extern void                     UI_MouseEvent(int dx, int dy);
extern void                     UI_Refresh(int realtime);
extern qbool         UI_ConsoleCommand(int realTime);
extern float            UI_ClampCvar(float min, float max, float value);
extern void                     UI_DrawNamedPic(float x, float y, float width,
						float height,
						const char *picname);
extern void                     UI_DrawHandlePic(float x, float y, float w,
						 float h,
						 Handle hShader);
extern void                     UI_FillRect(float x, float y, float width,
					    float height,
					    const float *color);
extern void                     UI_DrawRect(float x, float y, float width,
					    float height,
					    const float *color);
extern void                     UI_UpdateScreen(void);
extern void                     UI_SetColor(const float *rgba);
extern void                     UI_LerpColor(Vec4 a, Vec4 b, Vec4 c,
					     float t);
extern void                     UI_DrawBannerString(int x, int y,
						    const char* str, int style,
						    Vec4 color);
extern float            UI_ProportionalSizeScale(int style);
extern void                     UI_DrawProportionalString(int x, int y,
							  const char* str,
							  int style,
							  Vec4 color);
extern void                     UI_DrawProportionalString_AutoWrapped(
	int x, int ystart, int xmax, int ystep, const char* str, int style,
	Vec4 color);
extern int                      UI_ProportionalStringWidth(const char* str);
extern void                     UI_DrawString(int x, int y, const char* str,
					      int style,
					      Vec4 color);
extern void                     UI_DrawChar(int x, int y, int ch, int style,
					    Vec4 color);
extern qbool         UI_CursorInRect(int x, int y, int width, int height);
extern void                     UI_AdjustFrom640(float *x, float *y, float *w,
						 float *h);
extern void                     UI_DrawTextBox(int x, int y, int width,
					       int lines);
extern qbool         UI_IsFullscreen(void);
extern void                     UI_SetActiveMenu(uiMenuCommand_t menu);
extern void                     UI_PushMenu(menuframework_s *menu);
extern void                     UI_PopMenu(void);
extern void                     UI_ForceMenuOff(void);
extern char*UI_Argv(int arg);
extern char*UI_cvargetstr(const char *var_name);
extern void                     UI_Refresh(int time);
extern void                     UI_StartDemoLoop(void);
extern qbool m_entersound;
extern UIstatic uis;

/*
 * ui_spLevel.c
 *  */
void UI_SPLevelMenu_Cache(void);
void UI_SPLevelMenu(void);
void UI_SPLevelMenu_f(void);
void UI_SPLevelMenu_ReInit(void);

/*
 * ui_spArena.c
 *  */
void UI_SPArena_Start(const char *arenaInfo);

/*
 * ui_spPostgame.c
 *  */
void UI_SPPostgameMenu_Cache(void);
void UI_SPPostgameMenu_f(void);

/*
 * ui_spSkill.c
 *  */
void UI_SPSkillMenu(const char *arenaInfo);
void UI_SPSkillMenu_Cache(void);

/*
 * ui_syscalls.c
 *  */
void                    trap_Print(const char *string);
void                    trap_Error(const char *string) __attribute__((noreturn));
int                             trap_Milliseconds(void);
void                    trap_cvarregister(Vmcvar *vmCvar, const char *varName,
					   const char *defaultValue,
					   int flags);
void                    trap_cvarupdate(Vmcvar *vmCvar);
void                    trap_cvarsetstr(const char *var_name, const char *value);
float                   trap_cvargetf(const char *var_name);
void                    trap_cvargetstrbuf(const char *var_name,
						       char *buffer,
						       int bufsize);
void                    trap_cvarsetf(const char *var_name, float value);
void                    trap_cvarreset(const char *name);
void                    trap_Cvar_Create(const char *var_name,
					 const char *var_value,
					 int flags);
void                    trap_cvargetinfostrbuf(int bit, char *buffer,
						   int bufsize);
int                             trap_Argc(void);
void                    trap_Argv(int n, char *buffer, int bufferLength);
void                    trap_Cmd_ExecuteText(int exec_when, const char *text);	/* don't use EXEC_NOW! */
int                             trap_fsopen(const char *qpath,
						  Fhandle *f,
						  Fsmode mode);
void                    trap_fsread(void *buffer, int len, Fhandle f);
void                    trap_fswrite(const void *buffer, int len,
				      Fhandle f);
void                    trap_fsclose(Fhandle f);
int                             trap_fsgetfilelist(const char *path,
						    const char *extension,
						    char *listbuf,
						    int bufsize);
int                             trap_fsseek(Fhandle f, long offset,
					     int origin);	/* Fsorigin */
Handle               trap_R_RegisterModel(const char *name);
Handle               trap_R_RegisterSkin(const char *name);
Handle               trap_R_RegisterShaderNoMip(const char *name);
void                    trap_R_ClearScene(void);
void                    trap_R_AddRefEntityToScene(const Refent *re);
void                    trap_R_AddPolyToScene(Handle hShader, int numVerts,
					      const Polyvert *verts);
void                    trap_R_AddLightToScene(const Vec3 org, float intensity,
					       float r, float g,
					       float b);
void                    trap_R_RenderScene(const Refdef *fd);
void                    trap_R_SetColor(const float *rgba);
void                    trap_R_DrawStretchPic(float x, float y, float w, float h,
					      float s1, float t1, float s2,
					      float t2,
					      Handle hShader);
void                    trap_UpdateScreen(void);
int                             trap_CM_LerpTag(Orient *tag,
						Cliphandle mod, int startFrame,
						int endFrame, float frac,
						const char *tagName);
void                    trap_sndstartlocalsound(Handle sfx, int channelNum);
Handle     trap_sndregister(const char *sample, qbool compressed);
void                    trap_Key_KeynumToStringBuf(int keynum, char *buf,
						   int buflen);
void                    trap_Key_GetBindingBuf(int keynum, char *buf, int buflen);
void                    trap_Key_SetBinding(int keynum, const char *binding);
qbool                trap_Key_IsDown(int keynum);
qbool                trap_Key_GetOverstrikeMode(void);
void                    trap_Key_SetOverstrikeMode(qbool state);
void                    trap_Key_ClearStates(void);
int                             trap_Key_GetCatcher(void);
void                    trap_Key_SetCatcher(int catcher);
void                    trap_GetClipboardData(char *buf, int bufsize);
void                    trap_GetClientState(UIclientstate *state);
void                    trap_GetGlconfig(Glconfig *glconfig);
int                             trap_GetConfigString(int index, char* buff,
						     int buffsize);
int                             trap_LAN_GetServerCount(int source);
void                    trap_LAN_GetServerAddressString(int source, int n,
							char *buf,
							int buflen);
void                    trap_LAN_GetServerInfo(int source, int n, char *buf,
					       int buflen);
int                             trap_LAN_GetPingQueueCount(void);
int                             trap_LAN_ServerStatus(const char *serverAddress,
						      char *serverStatus,
						      int maxLen);
void                    trap_LAN_ClearPing(int n);
void                    trap_LAN_GetPing(int n, char *buf, int buflen,
					 int *pingtime);
void                    trap_LAN_GetPingInfo(int n, char *buf, int buflen);
int                             trap_MemoryRemaining(void);

/*
 * ui_addbots.c
 *  */
void UI_AddBots_Cache(void);
void UI_AddBotsMenu(void);

/*
 * ui_removebots.c
 *  */
void UI_RemoveBots_Cache(void);
void UI_RemoveBotsMenu(void);

/*
 * ui_teamorders.c
 *  */
extern void UI_TeamOrdersMenu(void);
extern void UI_TeamOrdersMenu_f(void);
extern void UI_TeamOrdersMenu_Cache(void);

/*
 * ui_loadconfig.c
 *  */
void UI_LoadConfig_Cache(void);
void UI_LoadConfigMenu(void);

/*
 * ui_saveconfig.c
 *  */
void UI_SaveConfigMenu_Cache(void);
void UI_SaveConfigMenu(void);

/*
 * ui_display.c
 *  */
void UI_DisplayOptionsMenu_Cache(void);
void UI_DisplayOptionsMenu(void);

/*
 * ui_sound.c
 *  */
void UI_SoundOptionsMenu_Cache(void);
void UI_SoundOptionsMenu(void);

/*
 * ui_network.c
 *  */
void UI_NetworkOptionsMenu_Cache(void);
void UI_NetworkOptionsMenu(void);

/*
 * ui_gameinfo.c
 *  */
typedef enum {
	AWARD_ACCURACY,
	AWARD_IMPRESSIVE,
	AWARD_EXCELLENT,
	AWARD_GAUNTLET,
	AWARD_FRAGS,
	AWARD_PERFECT
} awardType_t;

const char*UI_GetArenaInfoByNumber(int num);
const char*UI_GetArenaInfoByMap(const char *map);
const char*UI_GetSpecialArenaInfo(const char *tag);
int UI_GetNumArenas(void);
int UI_GetNumSPArenas(void);
int UI_GetNumSPTiers(void);

char*UI_GetBotInfoByNumber(int num);
char*UI_GetBotInfoByName(const char *name);
int UI_GetNumBots(void);

void UI_GetBestScore(int level, int *score, int *skill);
void UI_SetBestScore(int level, int score);
int UI_TierCompleted(int levelWon);
qbool UI_ShowTierVideo(int tier);
qbool UI_CanShowTierVideo(int tier);
int  UI_GetCurrentGame(void);
void UI_NewGame(void);
void UI_LogAwardData(int award, int data);
int UI_GetAwardLevel(int award);

void UI_SPUnlock_f(void);
void UI_SPUnlockMedals_f(void);

void UI_InitGameinfo(void);

/* GRank */

/*
 * ui_rankings.c
 *  */
void Rankings_DrawText(void* self);
void Rankings_DrawName(void* self);
void Rankings_DrawPassword(void* self);
void Rankings_Cache(void);
void UI_RankingsMenu(void);

/*
 * ui_login.c
 *  */
void Login_Cache(void);
void UI_LoginMenu(void);

/*
 * ui_signup.c
 *  */
void Signup_Cache(void);
void UI_SignupMenu(void);

/*
 * ui_rankstatus.c
 *  */
void RankStatus_Cache(void);
void UI_RankStatusMenu(void);

#endif
