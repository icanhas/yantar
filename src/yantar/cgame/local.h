/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "ref.h"
#include "bg.h"
#include "cgame.h"

/* 
 * The entire cgame module is unloaded and reloaded on each level change,
 * so there is NO persistent data between levels on the client side.
 * If you absolutely need something stored, it can either be kept
 * by the server in the server stored userinfos, or stashed in a cvar.
 */

#ifdef MISSIONPACK
#define CG_FONT_THRESHOLD 0.1
#endif

#define POWERUP_BLINKS		5

#define POWERUP_BLINK_TIME	1000
#define FADE_TIME		200
#define PULSE_TIME		200
#define DAMAGE_DEFLECT_TIME	100
#define DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME		500
#define LAND_DEFLECT_TIME	150
#define LAND_RETURN_TIME	300
#define STEP_TIME		200
#define DUCK_TIME		100
#define PAIN_TWITCH_TIME	200
#define WEAPON_SELECT_TIME	1400
#define ITEM_SCALEUP_TIME	1000
#define ITEM_BLOB_TIME		200
#define MUZZLE_FLASH_TIME	20
#define SINK_TIME		1000	/* time for fragments to sink into ground before going away */
#define ATTACKER_HEAD_TIME	10000
#define REWARD_TIME		3000

#define PULSE_SCALE		1.5	/* amount to scale up the icons when activating */

#define MAX_STEP_CHANGE		32

#define MAX_VERTS_ON_POLY	10
#define MAX_MARK_POLYS		256

#define Maxtestlights		16

#define STAT_MINUS		10	/* num frame for '-' stats digit */

#define ICON_SIZE		48
#define CHAR_WIDTH		32
#define CHAR_HEIGHT		48
#define TEXT_ICON_SPACE		4

#define TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

/* very large characters */
#define GIANT_WIDTH			32
#define GIANT_HEIGHT			48

#define NUM_CROSSHAIRS			10

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define DEFAULT_MODEL			"griever"
#define DEFAULT_TEAM_MODEL		"griever"
#define DEFAULT_TEAM_HEAD		"griever"

#define DEFAULT_REDTEAM_NAME	"Stroggs"
#define DEFAULT_BLUETEAM_NAME	"Pagans"

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} Footstep;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} Impactsnd;

/* player entities need to track more information
 * than any other type of entity. */

/* note that not every player entity is a client entity,
 * because corpses after respawn are outside the normal
 * client numbering range */

/* when changing animation, set animationTime to frameTime + lerping time
 * The current lerp will finish out, then it will lerp to the new animation */
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
	Lerpframe	legs, torso, flag;
	int		painTime;
	int		painDirection;	/* flip from 0 to 1 */
} Playerent;

typedef struct Weapent	Weapent;
struct Weapent {
	int		muzzleFlashTime;
	qbool		lightningFiring;
	int		railFireTime;
	float		barrelAngle;	/* machinegun spinning */
	int		barrelTime;
	qbool		barrelSpinning;
};

/* 
 * Centity have a direct corespondence with Gentity in the game, but
 * only the Entstate is directly communicated to the cgame 
 */
typedef struct Centity {
	Entstate	currentState;	/* from cg.frame */
	Entstate	nextState;	/* from cg.nextFrame, if available */
	Playerent	pe;
	Weapent		w[WSnumslots];

	qbool		interpolate;	/* true if next state is valid to interpolate to */
	qbool		currentValid;	/* true if cg.frame holds this entity */

	int		previousEvent;
	int		teleportFlag;

	int		trailTime;	/* so missile trails can handle dropped initial packets */
	int		dustTrailTime;
	int		miscTime;

	int		snapShotTime;	/* last time this entity was found in a snapshot */

	int		errorTime;	/* decay the error from this time */
	Vec3		errorOrigin;
	Vec3		errorAngles;

	qbool		extrapolated;	/* false if origin / angles is an interpolation */
	Vec3		rawOrigin;
	Vec3		rawAngles;

	/* exact interpolated position of entity on this frame */
	Vec3	lerpOrigin;
	Vec3	lerpAngles;
} Centity;

/* 
 * local entities are created as a result of events or predicted actions,
 * and live independantly from all server transmitted entities
 */

typedef struct Markpoly {
	struct Markpoly	*prevMark, *nextMark;
	int			time;
	Handle			markShader;
	qbool			alphaFade;	/* fade alpha instead of rgb */
	float			color[4];
	Poly			poly;
	Polyvert		verts[MAX_VERTS_ON_POLY];
} Markpoly;

typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
	LE_SHOWREFENTITY	/* FIXME: MISSIONPACK; remove? */
} Letype;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001,	/* do not scale size over time */
	LEF_TUMBLE	= 0x0002,	/* tumble over time, used for ejecting shells */
	LEF_SOUND1	= 0x0004,	/* sound 1 for kamikaze */
	LEF_SOUND2	= 0x0008	/* sound 2 for kamikaze */
} Leflag;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} Lemarktype;	/* fragment local entities can leave marks on walls */

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS
} Lebouncesndtype;	/* fragment local entities can make sounds on impacts */

typedef struct Localent {
	struct Localent	*prev, *next;
	Letype		leType;
	int			leFlags;

	int			startTime;
	int			endTime;
	int			fadeInTime;

	float			lifeRate;	/* 1.0 / (endTime - startTime) */

	Trajectory		pos;
	Trajectory		angles;

	float			bounceFactor;	/* 0.0 = no bounce, 1.0 = perfect */

	float			color[4];

	float			radius;

	float			light;
	Vec3			lightColor;

	Lemarktype		leMarkType;	/* mark to leave on fragment impact */
	Lebouncesndtype	leBounceSoundType;

	Refent		refEntity;
} Localent;

typedef struct {
	int		client;
	int		score;
	int		ping;
	int		time;
	int		scoreFlags;
	int		powerUps;
	int		accuracy;
	int		impressiveCount;
	int		excellentCount;
	int		guantletCount;
	int		defendCount;
	int		assistCount;
	int		captures;
	qbool		perfect;
	int		team;
} Score;

/* 
 * each client has an associated Clientinfo
 * that contains media references necessary to present the
 * client model and other color coded effects
 * this is regenerated each time a client's configstring changes,
 * usually as a result of a userinfo (name, model, etc) change 
 */
#define MAX_CUSTOM_SOUNDS 32

typedef struct {
	qbool		infoValid;

	char		name[MAX_QPATH];
	Team		team;

	int		botSkill;	/* 0 = not bot, 1-5 = bot */

	Vec3		color1;
	Vec3		color2;

	byte		c1RGBA[4];
	byte		c2RGBA[4];

	int		score;		/* updated by score servercmds */
	int		location;	/* location index for team mode */
	int		health;		/* you only get this info about your teammates */
	int		armor;
	int		curWeapon;

	int		handicap;
	int		wins, losses;	/* in tourney mode */

	int		teamTask;	/* task in teamplay (offence/defence) */
	qbool		teamLeader;	/* true when this is a team leader */

	int		powerups;	/* so can display quad/flag status */

	int		medkitUsageTime;
	int		invulnerabilityStartTime;
	int		invulnerabilityStopTime;

	int		breathPuffTime;

	/* when clientinfo is changed, the loading of models/skins/sounds
	 * can be deferred until you are dead, to prevent hitches in
	 * gameplay */
	char		modelName[MAX_QPATH];
	char		skinName[MAX_QPATH];
	char		redTeam[MAX_TEAMNAME];
	char		blueTeam[MAX_TEAMNAME];
	qbool		deferred;

	qbool		newAnims;	/* true if using the new mission pack animations */
	qbool		fixedlegs;	/* true if legs yaw is always the same as torso yaw */
	qbool		fixedtorso;	/* true if torso never changes yaw */

	Vec3		headOffset;	/* move head in icon views */
	Footstep	footsteps;
	Gender	gender;	/* from model */

	Handle hullmodel;	/* craft body */
	Handle hullskin;

	Handle		modelIcon;

	Anim	animations[MAX_TOTALANIMATIONS];

	Handle	sounds[MAX_CUSTOM_SOUNDS];
	
	// remove these
	char		headModelName[MAX_QPATH];
	char		headSkinName[MAX_QPATH];
	Handle		legsModel;
	Handle		legsSkin;
	Handle		torsoModel;
	Handle		torsoSkin;
	Handle		headModel;
	Handle		headSkin;
} Clientinfo;

/* 
 * each W* weapon enum has an associated Weapinfo
 * that contains media references necessary to present the
 * weapon and its effects 
 */
typedef struct Weapinfo {
	qbool		registered;
	Gitem		*item;

	Handle		handsModel;	/* the hands don't actually draw, they just position the weapon */
	Handle		weaponModel;
	Handle		barrelModel;
	Handle		flashModel;

	Vec3		weaponMidpoint;	/* so it will rotate centered instead of by tag */

	float		flashDlight;
	Vec3		flashDlightColor;
	Handle	flashSound[4];	/* fast firing weapons randomly choose */

	Handle		weaponIcon;
	Handle		ammoIcon;

	Handle		ammoModel;

	Handle		missileModel;
	Handle	missileSound;
	void (*missileTrailFunc)(Centity *, const struct Weapinfo *wi);
	float		missileDlight;
	Vec3		missileDlightColor;
	int		missileRenderfx;

	void (*ejectBrassFunc)(Centity *);

	float		trailRadius;
	float		wiTrailTime;

	Handle	readySound;
	Handle	firingSound;
} Weapinfo;

/* 
 * each IT_* item has an associated Iteminfo
 * that constains media references necessary to present the
 * item and its effects
 */
typedef struct {
	qbool		registered;
	Handle		models[MAX_ITEM_MODELS];
	Handle		icon;
} Iteminfo;

typedef struct {
	int itemNum;
} Powerupinfo;

#define MAX_SKULLTRAIL 10

typedef struct {
	Vec3	positions[MAX_SKULLTRAIL];
	int	numpositions;
} Skulltrail;

#define MAX_REWARDSTACK 10
#define MAX_SOUNDBUFFER 20

/* all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
 * occurs, and they will have visible effects for #define STEP_TIME or whatever msec after */

#define MAX_PREDICTED_EVENTS 16

typedef struct {
	int		clientFrame;	/* incremented each frame */

	int		clientNum;

	qbool		demoPlayback;
	qbool		levelShot;	/* taking a level menu screenshot */
	int		deferredPlayerLoading;
	qbool		loading;		/* don't defer players at initial startup */
	qbool		intermissionStarted;	/* don't play voice rewards, because game will end shortly */

	/* there are only one or two Snap that are relevent at a time */
	int		latestSnapshotNum;	/* the number of snapshots the client system has received */
	int		latestSnapshotTime;	/* the time from latestSnapshotNum, so we don't need to read the snapshot yet */

	Snap	*snap;		/* cg.snap->serverTime <= cg.time */
	Snap	*nextSnap;	/* cg.nextSnap->serverTime > cg.time, or NULL */
	Snap	activeSnapshots[2];

	float		frameInterpolation;	/* (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime) */

	qbool		thisFrameTeleport;
	qbool		nextFrameTeleport;

	int		frametime;	/* cg.time - cg.oldTime */

	int		time;	/* this is the time value that the client */
	/* is rendering at. */
	int		oldTime;	/* time at last frame, used for missile trails and prediction checking */

	int		physicsTime;	/* either cg.snap->time or cg.nextSnap->time */

	int		timelimitWarnings;	/* 5 min, 1 min, overtime */
	int		fraglimitWarnings;

	qbool		mapRestart;	/* set on a map restart to set back the weapon */

	qbool		renderingThirdPerson;	/* during deaths, chasecams, etc */

	/* prediction state */
	qbool		hyperspace;	/* true if prediction has hit a trigger_teleport */
	Playerstate	predictedPlayerState;
	Centity	predictedPlayerEntity;
	qbool		validPPS;	/* clear until the first call to CG_PredictPlayerState */
	int		predictedErrorTime;
	Vec3		predictedError;

	int		eventSequence;
	int		predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;	/* for stair up smoothing */
	int		stepTime;

	float		duckChange;	/* for duck viewheight smoothing */
	int		duckTime;

	float		landChange;	/* for landing hard */
	int		landTime;

	/* input state sent to server */
	int weapsel[WSnumslots];

	/* auto rotating items */
	Vec3	autoAngles;
	Vec3	autoAxis[3];
	Vec3	autoAnglesFast;
	Vec3	autoAxisFast[3];

	/* view rendering */
	Refdef	refdef;
	Vec3		refdefViewAngles;	/* will be converted to refdef.viewaxis */

	/* zoom key */
	qbool		zoomed;
	int		zoomTime;
	float		zoomSensitivity;

	/* information screen text during loading */
	char infoScreenText[MAX_STRING_CHARS];

	/* scoreboard */
	int		scoresRequestTime;
	int		numScores;
	int		selectedScore;
	int		teamScores[2];
	Score		scores[MAX_CLIENTS];
	qbool		showScores;
	qbool		scoreBoardShowing;
	int		scoreFadeTime;
	char		killerName[MAX_NAME_LENGTH];
	char		spectatorList[MAX_STRING_CHARS];	/* list of names */
	int		spectatorLen;				/* length of list */
	float		spectatorWidth;				/* width in device units */
	int		spectatorTime;				/* next time to offset */
	int		spectatorPaintX;			/* current paint x */
	int		spectatorPaintX2;			/* current paint x */
	int		spectatorOffset;			/* current offset from start */
	int		spectatorPaintLen;			/* current offset from start */

	/* centerprinting */
	int	centerPrintTime;
	int	centerPrintCharWidth;
	int	centerPrintY;
	char	centerPrint[1024];
	int	centerPrintLines;

	/* low ammo warning state */
	int lowAmmoWarning;	/* 1 = low, 2 = empty */

	/* crosshair client ID */
	int	crosshairClientNum;
	int	crosshairClientTime;

	/* powerup active flashing */
	int	powerupActive;
	int	powerupTime;

	/* attacking player */
	int	attackerTime;
	int	voiceTime;

	/* reward medals */
	int		rewardStack;
	int		rewardTime;
	int		rewardCount[MAX_REWARDSTACK];
	Handle		rewardShader[MAX_REWARDSTACK];
	Handle		rewardSound[MAX_REWARDSTACK];

	/* sound buffer mainly for announcer sounds */
	int		soundBufferIn;
	int		soundBufferOut;
	int		soundTime;
	Handle		soundBuffer[MAX_SOUNDBUFFER];

	/* for voice chat buffer */
	int	voiceChatTime;
	int	voiceChatBufferIn;
	int	voiceChatBufferOut;

	/* warmup countdown */
	int	warmup;
	int	warmupCount;

	/* ========================== */

	int	itemPickup;
	int	itemPickupTime;
	int	itemPickupBlendTime;	/* the pulse around the crosshair is timed seperately */

	int	weapseltime[WSnumslots];
	int	weapanim[WSnumslots];
	int	weapanimtime[WSnumslots];

	/* blend blobs */
	float	damageTime;
	float	damageX, damageY, damageValue;

	/* status bar head */
	float	headYaw;
	float	headEndPitch;
	float	headEndYaw;
	int	headEndTime;
	float	headStartPitch;
	float	headStartYaw;
	int	headStartTime;

	/* view movement */
	float	v_dmg_time;
	float	v_dmg_pitch;
	float	v_dmg_roll;

	/* temp working variables for player view */
	float	bobfracsin;
	int	bobcycle;
	float	xyspeed;
	int	nextOrbitTime;

	/* development tool */
	Refent	testModelEntity;
	char		testModelName[MAX_QPATH];
	qbool		testGun;
	uint		ntestlights;
	Vec3	testlightorigs[Maxtestlights];
} Cg;

/* 
 * all of the model, shader, and sound references that are
 * loaded at gamestate time are stored in Cgmedia
 * Other media that can be tied to clients, weapons, or items are
 * stored in the Clientinfo, Iteminfo, Weapinfo, and Powerupinfo 
 */
typedef struct {
	Handle		charsetShader;
	Handle		charsetProp;
	Handle		charsetPropGlow;
	Handle		charsetPropB;
	Handle		whiteShader;
	
	Handle		redFlagModel;
	Handle		blueFlagModel;
	Handle		neutralFlagModel;
	Handle		redFlagShader[3];
	Handle		blueFlagShader[3];
	Handle		flagShader[4];

	Handle		flagPoleModel;
	Handle		flagFlapModel;

	Handle		redFlagFlapSkin;
	Handle		blueFlagFlapSkin;
	Handle		neutralFlagFlapSkin;

	Handle		redFlagBaseModel;
	Handle		blueFlagBaseModel;
	Handle		neutralFlagBaseModel;

	Handle		armorModel;
	Handle		armorIcon;

	Handle		teamStatusBar;

	Handle		deferShader;

	/* gib explosions */
	Handle		gibAbdomen;
	Handle		gibArm;
	Handle		gibChest;
	Handle		gibFist;
	Handle		gibFoot;
	Handle		gibForearm;
	Handle		gibIntestine;
	Handle		gibLeg;
	Handle		gibSkull;
	Handle		gibBrain;

	Handle		smoke2;

	Handle		machinegunBrassModel;
	Handle		shotgunBrassModel;

	Handle		railRingsShader;
	Handle		railCoreShader;

	Handle		lightningShader;

	Handle		friendShader;

	Handle		balloonShader;
	Handle		connectionShader;

	Handle		selectShader;
	Handle		viewBloodShader;
	Handle		tracerShader;
	Handle		crosshairShader[NUM_CROSSHAIRS];
	Handle		lagometerShader;
	Handle		backTileShader;
	Handle		noammoShader;

	Handle		smokePuffShader;
	Handle		smokePuffRageProShader;
	Handle		shotgunSmokePuffShader;
	Handle		plasmaBallShader;
	Handle		waterBubbleShader;
	Handle		bloodTrailShader;
	Handle		nailPuffShader;
	Handle		blueProxMine;

	Handle		numberShaders[11];

	Handle		shadowMarkShader;

	Handle		botSkillShaders[5];

	/* wall mark shaders */
	Handle		wakeMarkShader;
	Handle		bloodMarkShader;
	Handle		bulletMarkShader;
	Handle		burnMarkShader;
	Handle		holeMarkShader;
	Handle		energyMarkShader;

	/* powerup shaders */
	Handle		quadShader;
	Handle		redQuadShader;
	Handle		quadWeaponShader;
	Handle		invisShader;
	Handle		regenShader;
	Handle		battleSuitShader;
	Handle		battleWeaponShader;
	Handle		hastePuffShader;
	Handle		redKamikazeShader;
	Handle		blueKamikazeShader;

	/* weapon effect models */
	Handle		bulletFlashModel;
	Handle		ringFlashModel;
	Handle		dishFlashModel;
	Handle		lightningExplosionModel;

	/* weapon effect shaders */
	Handle		railExplosionShader;
	Handle		plasmaExplosionShader;
	Handle		bulletExplosionShader;
	Handle		rocketExplosionShader;
	Handle		grenadeExplosionShader;
	Handle		bfgExplosionShader;
	Handle		bloodExplosionShader;

	/* special effects models */
	Handle		teleportEffectModel;
	Handle		teleportEffectShader;
	Handle		medkitUsageModel;
	Handle		dustPuffShader;
	Handle		invulnerabilityPowerupModel;

	/* scoreboard headers */
	Handle		scoreboardName;
	Handle		scoreboardPing;
	Handle		scoreboardScore;
	Handle		scoreboardTime;

	/* medals shown during gameplay */
	Handle		medalImpressive;
	Handle		medalExcellent;
	Handle		medalGauntlet;
	Handle		medalDefend;
	Handle		medalAssist;
	Handle		medalCapture;

	/* sounds */
	Handle	quadSound;
	Handle	tracerSound;
	Handle	selectSound;
	Handle	useNothingSound;
	Handle	wearOffSound;
	Handle	footsteps[FOOTSTEP_TOTAL][4];
	Handle	sfx_lghit1;
	Handle	sfx_lghit2;
	Handle	sfx_lghit3;
	Handle	sfx_ric1;
	Handle	sfx_ric2;
	Handle	sfx_ric3;
	/* Handle	sfx_railg; */
	Handle	sfx_rockexp;
	Handle	sfx_plasmaexp;
	Handle	sfx_proxexp;
	Handle	sfx_nghit;
	Handle	sfx_nghitflesh;
	Handle	sfx_nghitmetal;
	Handle	sfx_chghit;
	Handle	sfx_chghitflesh;
	Handle	sfx_chghitmetal;
	Handle	winnerSound;
	Handle	loserSound;
	Handle	gibSound;
	Handle	gibBounce1Sound;
	Handle	gibBounce2Sound;
	Handle	gibBounce3Sound;
	Handle	teleInSound;
	Handle	teleOutSound;
	Handle	noAmmoSound;
	Handle	respawnSound;
	Handle	talkSound;
	Handle	landSound;
	Handle	fallSound;
	Handle	jumpPadSound;

	Handle	oneMinuteSound;
	Handle	fiveMinuteSound;
	Handle	suddenDeathSound;

	Handle	threeFragSound;
	Handle	twoFragSound;
	Handle	oneFragSound;

	Handle	hitSound;
	Handle	hitSoundHighArmor;
	Handle	hitSoundLowArmor;
	Handle	hitTeamSound;
	Handle	impressiveSound;
	Handle	excellentSound;
	Handle	deniedSound;
	Handle	humiliationSound;
	Handle	assistSound;
	Handle	defendSound;
	Handle	firstImpressiveSound;
	Handle	firstExcellentSound;
	Handle	firstHumiliationSound;

	Handle	takenLeadSound;
	Handle	tiedLeadSound;
	Handle	lostLeadSound;

	Handle	voteNow;
	Handle	votePassed;
	Handle	voteFailed;

	Handle	watrInSound;
	Handle	watrOutSound;
	Handle	watrUnSound;

	Handle	flightSound;
	Handle	medkitSound;

	Handle	weaponHoverSound;

	/* teamplay sounds */
	Handle	captureAwardSound;
	Handle	redScoredSound;
	Handle	blueScoredSound;
	Handle	redLeadsSound;
	Handle	blueLeadsSound;
	Handle	teamsTiedSound;

	Handle	captureYourTeamSound;
	Handle	captureOpponentSound;
	Handle	returnYourTeamSound;
	Handle	returnOpponentSound;
	Handle	takenYourTeamSound;
	Handle	takenOpponentSound;

	Handle	redFlagReturnedSound;
	Handle	blueFlagReturnedSound;
	Handle	neutralFlagReturnedSound;
	Handle	enemyTookYourFlagSound;
	Handle	enemyTookTheFlagSound;
	Handle	yourTeamTookEnemyFlagSound;
	Handle	yourTeamTookTheFlagSound;
	Handle	youHaveFlagSound;
	Handle	yourBaseIsUnderAttackSound;
	Handle	holyShitSound;

	/* tournament sounds */
	Handle	count3Sound;
	Handle	count2Sound;
	Handle	count1Sound;
	Handle	countFightSound;
	Handle	countPrepareSound;

#ifdef MISSIONPACK
	/* new stuff */
	Handle		patrolShader;
	Handle		assaultShader;
	Handle		campShader;
	Handle		followShader;
	Handle		defendShader;
	Handle		teamLeaderShader;
	Handle		retrieveShader;
	Handle		escortShader;
	Handle		flagShaders[3];
	Handle	countPrepareTeamSound;
#endif
	Handle		cursor;
	Handle		selectCursor;
	Handle		sizeCursor;

	Handle	regenSound;
	Handle	protectSound;
	Handle	n_healthSound;
	Handle	hgrenb1aSound;
	Handle	hgrenb2aSound;
	Handle	wstbimplSound;
	Handle	wstbimpmSound;
	Handle	wstbimpdSound;
	Handle	wstbactvSound;
} Cgmedia;

/* 
 * The client game static (cgs) structure hold everything
 * loaded or calculated from the gamestate.  It will NOT
 * be cleared when a tournement restart is done, allowing
 * all clients to begin playing instantly
 */
typedef struct {
	Gamestate	gameState;	/* gamestate from server */
	Glconfig	glconfig;	/* rendering configuration */
	float		screenXScale;	/* derived from glconfig */
	float		screenYScale;
	float		screenXBias;

	int		serverCommandSequence;	/* reliable command stream counter */
	int		processedSnapshotNum;	/* the number of snapshots cgame has requested */

	qbool		localServer;	/* detected on startup by checking sv_running */

	/* parsed from serverinfo */
	Gametype	gametype;
	int		dmflags;
	int		teamflags;
	int		fraglimit;
	int		capturelimit;
	int		timelimit;
	int		maxclients;
	char		mapname[MAX_QPATH];
	char		redTeam[MAX_QPATH];
	char		blueTeam[MAX_QPATH];

	int		voteTime;
	int		voteYes;
	int		voteNo;
	qbool		voteModified;	/* beep whenever changed */
	char		voteString[MAX_STRING_TOKENS];

	int		teamVoteTime[2];
	int		teamVoteYes[2];
	int		teamVoteNo[2];
	qbool		teamVoteModified[2];	/* beep whenever changed */
	char		teamVoteString[2][MAX_STRING_TOKENS];

	int		levelStartTime;

	int		scores1, scores2;	/* from configstrings */
	int		redflag, blueflag;	/* flag status from configstrings */
	int		flagStatus;

	qbool		newHud;

	/*
	 * locally derived information from gamestate
	 *  */
	Handle		gameModels[MAX_MODELS];
	Handle	gameSounds[MAX_SOUNDS];

	int		numInlineModels;
	Handle		inlineDrawModel[MAX_MODELS];
	Vec3		inlineModelMidpoints[MAX_MODELS];

	Clientinfo	clientinfo[MAX_CLIENTS];

	/* teamchat width is *3 because of embedded color codes */
	char		teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int		teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int		teamChatPos;
	int		teamLastChatPos;

	int		cursorX;
	int		cursorY;
	qbool		eventHandling;
	qbool		mouseCaptured;
	qbool		sizingHud;
	void		*capturedItem;
	Handle		activeCursor;

	/* orders */
	int		currentOrder;
	qbool		orderPending;
	int		orderTime;
	int		currentVoiceClient;
	int		acceptOrderTime;
	int		acceptTask;
	int		acceptLeader;
	char		acceptVoice[MAX_NAME_LENGTH];

	/* media */
	Cgmedia media;
} Cgs;

extern Cgs	cgs;
extern Cg	cg;
extern Centity	cg_entities[MAX_GENTITIES];
extern Weapinfo cg_weapons[MAX_WEAPONS];
extern Iteminfo	cg_items[MAX_ITEMS];
extern Markpoly	cg_markPolys[MAX_MARK_POLYS];

extern Vmcvar		cg_centertime;
extern Vmcvar		cg_runpitch;
extern Vmcvar		cg_runroll;
extern Vmcvar		cg_bobup;
extern Vmcvar		cg_bobpitch;
extern Vmcvar		cg_bobroll;
extern Vmcvar		cg_swingSpeed;
extern Vmcvar		cg_shadows;
extern Vmcvar		cg_gibs;
extern Vmcvar		cg_drawTimer;
extern Vmcvar		cg_drawFPS;
extern Vmcvar		cg_drawspeed;
extern Vmcvar		cg_drawSnapshot;
extern Vmcvar		cg_draw3dIcons;
extern Vmcvar		cg_drawIcons;
extern Vmcvar		cg_drawAmmoWarning;
extern Vmcvar		cg_drawCrosshair;
extern Vmcvar		cg_drawCrosshairNames;
extern Vmcvar		cg_drawRewards;
extern Vmcvar		cg_drawTeamOverlay;
extern Vmcvar		cg_teamOverlayUserinfo;
extern Vmcvar		cg_crosshairX;
extern Vmcvar		cg_crosshairY;
extern Vmcvar		cg_crosshairSize;
extern Vmcvar		cg_crosshairHealth;
extern Vmcvar		cg_drawStatus;
extern Vmcvar		cg_draw2D;
extern Vmcvar		cg_animSpeed;
extern Vmcvar		cg_debugAnim;
extern Vmcvar		cg_debugPosition;
extern Vmcvar		cg_debugEvents;
extern Vmcvar		cg_railTrailTime;
extern Vmcvar		cg_errorDecay;
extern Vmcvar		cg_nopredict;
extern Vmcvar		cg_noPlayerAnims;
extern Vmcvar		cg_showmiss;
extern Vmcvar		cg_footsteps;
extern Vmcvar		cg_addMarks;
extern Vmcvar		cg_brassTime;
extern Vmcvar		cg_gun_frame;
extern Vmcvar		cg_gun_x;
extern Vmcvar		cg_gun_y;
extern Vmcvar		cg_gun_z;
extern Vmcvar		cg_drawGun;
extern Vmcvar		cg_viewsize;
extern Vmcvar		cg_tracerChance;
extern Vmcvar		cg_tracerWidth;
extern Vmcvar		cg_tracerLength;
extern Vmcvar		cg_autoswitch;
extern Vmcvar		cg_ignore;
extern Vmcvar		cg_simpleItems;
extern Vmcvar		cg_fov;
extern Vmcvar		cg_zoomFov;
extern Vmcvar		cg_zoomintime;
extern Vmcvar		cg_zoomouttime;
extern Vmcvar		cg_zoomtoggle;
extern Vmcvar		cg_thirdpersonrange;
extern Vmcvar		cg_thirdpersonyaw;
extern Vmcvar		cg_thirdpersonpitch;
extern Vmcvar		cg_thirdperson;
extern Vmcvar		cg_lagometer;
extern Vmcvar		cg_drawAttacker;
extern Vmcvar		cg_synchronousClients;
extern Vmcvar		cg_ChatTime;
extern Vmcvar		cg_ChatHeight;
extern Vmcvar		cg_stats;
extern Vmcvar		cg_forceModel;
extern Vmcvar		cg_buildScript;
extern Vmcvar		cg_paused;
extern Vmcvar		cg_blood;
extern Vmcvar		cg_predictItems;
extern Vmcvar		cg_deferPlayers;
extern Vmcvar		cg_drawFriend;
extern Vmcvar		cg_ChatsOnly;
extern Vmcvar		cg_noVoiceChats;
extern Vmcvar		cg_noVoiceText;
extern Vmcvar		cg_scorePlum;
extern Vmcvar		cg_smoothClients;
extern Vmcvar		pmove_fixed;
extern Vmcvar		pmove_msec;
/* extern	Vmcvar		cg_pmove_fixed; */
extern Vmcvar		cg_cameraOrbit;
extern Vmcvar		cg_cameraOrbitDelay;
extern Vmcvar		cg_timescaleFadeEnd;
extern Vmcvar		cg_timescaleFadeSpeed;
extern Vmcvar		cg_timescale;
extern Vmcvar		cg_cameraMode;
extern Vmcvar		cg_smallFont;
extern Vmcvar		cg_bigFont;
extern Vmcvar		cg_noTaunt;
extern Vmcvar		cg_noProjectileTrail;
extern Vmcvar		cg_oldRail;
extern Vmcvar		cg_oldRocket;
extern Vmcvar		cg_oldPlasma;
extern Vmcvar		cg_trueLightning;

extern Vmcvar		cg_redTeamName;
extern Vmcvar		cg_blueTeamName;
extern Vmcvar		cg_currentSelectedPlayer;
extern Vmcvar		cg_currentSelectedPlayerName;
extern Vmcvar		cg_enableDust;

/*
 * main.c
 */
const char*	CG_ConfigString(int index);
const char*	CG_Argv(int arg);

void QDECL	CG_Printf(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));
void QDECL	CG_Error(const char *msg,
		    ...) __attribute__ ((noreturn, format (printf, 1, 2)));

void	CG_StartMusic(void);

void	CG_UpdateCvars(void);

int	CG_CrosshairPlayer(void);
int	CG_LastAttacker(void);
void	CG_LoadMenus(const char *menuFile);
void	CG_KeyEvent(int key, qbool down);
void	CG_MouseEvent(int x, int y);
void	CG_EventHandling(int type);
void	CG_RankRunFrame(void);
void	CG_SetScoreSelection(void *menu);
Score*	CG_GetSelectedScore(void);
void	CG_BuildSpectatorString(void);


/*
 * view.c
 */
void	CG_TestModel_f(void);
void	CG_TestGun_f(void);
void	CG_TestModelNextFrame_f(void);
void	CG_TestModelPrevFrame_f(void);
void	CG_TestModelNextSkin_f(void);
void	CG_TestModelPrevSkin_f(void);
void	CG_TestLight_f(void);
void	CG_ZoomDown_f(void);
void	CG_ZoomUp_f(void);
void	CG_AddBufferedSound(Handle sfx);
void	CG_DrawActiveFrame(int serverTime, Stereoframe stereoView,
			qbool	demoPlayback);

/*
 * drawtools.c
 */
void	CG_AdjustFrom640(float *x, float *y, float *w, float *h);
void	CG_FillRect(float x, float y, float width, float height, const float *color);
void	CG_DrawPic(float x, float y, float width, float height, Handle hShader);
void	CG_DrawString(float x, float y, const char *string,
		float charWidth, float charHeight, const float *modulate);
void	CG_DrawStringExt(int x, int y, const char *string, const float *setColor,
		qbool forceColor, qbool shadow, int charWidth,
		int charHeight,
		int maxChars);
void	CG_DrawBigString(int x, int y, const char *s, float alpha);
void	CG_DrawBigStringColor(int x, int y, const char *s, Vec4 color);
void	CG_DrawSmallString(int x, int y, const char *s, float alpha);
void	CG_DrawSmallStringColor(int x, int y, const char *s, Vec4 color);
int	CG_DrawStrlen(const char *str);
float*	CG_FadeColor(int startMsec, int totalMsec);
float*	CG_TeamColor(int team);
void	CG_TileClear(void);
void	CG_ColorForHealth(Vec4 hcolor);
void	CG_GetColorForHealth(int health, int armor, Vec4 hcolor);
void	UI_DrawProportionalString(int x, int y, const char* str, int style,
		Vec4 color);
void	CG_DrawRect(float x, float y, float width, float height, float size,
		const float *color);
void	CG_DrawSides(float x, float y, float w, float h, float size);
void	CG_DrawTopBottom(float x, float y, float w, float h, float size);

/*
 * draw.c, newdraw.c
 */
extern int	sortedTeamPlayers[TEAM_MAXOVERLAY];
extern int	numSortedTeamPlayers;
extern int	drawTeamOverlayModificationCount;
extern char	systemChat[256];
extern char	teamChat1[256];
extern char	teamChat2[256];

void	CG_AddLagometerFrameInfo(void);
void	CG_AddLagometerSnapshotInfo(Snap *snap);
void	CG_CenterPrint(const char *str, int y, int charWidth);
void	CG_DrawHead(float x, float y, float w, float h, int clientNum,
		 Vec3 headAngles);
void	CG_DrawActive(Stereoframe stereoView);
void	CG_DrawFlagModel(float x, float y, float w, float h, int team,
		      qbool force2D);
void	CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team);
void	CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y,
		  int ownerDraw, int ownerDrawFlags, int align, float special,
		  float scale,
		  Vec4 color, Handle shader,
		  int textStyle);
void	CG_Text_Paint(float x, float y, float scale, Vec4 color, const char *text,
		   float adjust, int limit,
		   int style);
int	CG_Text_Width(const char *text, float scale, int limit);
int	CG_Text_Height(const char *text, float scale, int limit);
void	CG_SelectPrevPlayer(void);
void	CG_SelectNextPlayer(void);
float	CG_GetValue(int ownerDraw);
qbool	CG_OwnerDrawVisible(int flags);
void	CG_RunMenuScript(char **args);
void	CG_ShowResponseHead(void);
void	CG_SetPrintString(int type, const char *p);
void	CG_InitTeamChat(void);
void	CG_GetTeamColor(Vec4 *color);
const char*	CG_GetGameStatusText(void);
const char*	CG_GetKillerText(void);
void	CG_Draw3DModel(float x, float y, float w, float h, Handle model,
		    Handle skin, Vec3 origin,
		    Vec3 angles);
void	CG_Text_PaintChar(float x, float y, float width, float height, float scale,
		       float s, float t, float s2, float t2,
		       Handle hShader);
void	CG_CheckOrderPending(void);
const char*	CG_GameTypeString(void);
qbool	CG_YourTeamHasFlag(void);
qbool	CG_OtherTeamHasFlag(void);
Handle	CG_StatusHandle(int task);

/*
 * player.c
 */
void	CG_Player(Centity *cent);
void	CG_ResetPlayerEntity(Centity *cent);
void	CG_AddRefEntityWithPowerups(Refent *ent, Entstate *state,
		int team);
void	CG_NewClientInfo(int clientNum);
Handle	CG_CustomSound(int clientNum, const char *soundName);

/*
 * predict.c
 *  */
void	CG_BuildSolidList(void);
int	CG_PointContents(const Vec3 point, int passEntityNum);
void	CG_Trace(Trace *result, const Vec3 start, const Vec3 mins,
		const Vec3 maxs, const Vec3 end,
		int skipNumber, int mask);
void	CG_TraceCapsule(Trace *result, const Vec3 start, const Vec3 mins,
		const Vec3 maxs, const Vec3 end,
		int skipNumber, int mask);
void	CG_PredictPlayerState(void);
void	CG_LoadDeferredPlayers(void);

/*
 * events.c
 */
void	CG_CheckEvents(Centity *cent);
const char*	CG_PlaceString(int rank);
void	CG_EntityEvent(Centity *cent, Vec3 position);
void	CG_PainEvent(Centity *cent, int health);

/*
 * ents.c
 */
void	CG_SetEntitySoundPosition(Centity *cent);
void	CG_AddPacketEntities(void);
void	CG_Beam(Centity *cent);
void	CG_AdjustPositionForMover(const Vec3 in, int moverNum, int fromTime,
			       int toTime,
			       Vec3 out);

void	CG_PositionEntityOnTag(Refent *entity, const Refent *parent,
		Handle parentModel, char *tagName);
void	CG_PositionRotatedEntityOnTag(Refent *entity,
		const Refent *parent,
		Handle parentModel,
		char *tagName);

/*
 * weapons.c
 */
void	CG_NextPriWeap_f(void);
void	CG_PrevPriWeap_f(void);
void	CG_PriWeap_f(void);
void	CG_NextSecWeap_f(void);
void	CG_PrevSecWeap_f(void);
void	CG_SecWeap_f(void);
void	CG_RegisterWeapon(int weaponNum);
void	CG_RegisterItemVisuals(int itemNum);
void	CG_FireWeapon(Centity *cent, Weapslot);
void	CG_MissileHitWall(int weapon, int clientNum, Vec3 origin, Vec3 dir,
		Impactsnd soundType);
void	CG_MissileHitPlayer(int weapon, Vec3 origin, Vec3 dir, int entityNum);
void	CG_ShotgunFire(Entstate *es);
void	CG_Bullet(Vec3 origin, int sourceEntityNum, Vec3 normal, qbool flesh,
		int fleshEntityNum);
void	CG_RailTrail(Clientinfo *ci, Vec3 start, Vec3 end);
void	CG_GrappleTrail(Centity *ent, const Weapinfo *wi);
void	CG_AddViewWeapon(Playerstate *ps, Weapslot);
void	CG_AddPlayerWeapon(Refent *parent, Playerstate *ps, Centity *cent,
		Weapslot, int team);
void	CG_DrawWeaponSelect(Weapslot);
void	CG_OutOfAmmoChange(Weapslot);	/* should this be in pmove? */

/*
 * marks.c
 */
void	CG_InitMarkPolys(void);
void	CG_AddMarks(void);
void	CG_ImpactMark(Handle markShader,
		const Vec3 origin, const Vec3 dir,
		float orientation,
		float r, float g, float b, float a,
		qbool alphaFade,
		float radius, qbool temporary);

/*
 * localents.c
 */
void		CG_InitLocalEntities(void);
Localent*	CG_AllocLocalEntity(void);
void		CG_AddLocalEntities(void);

/*
 * effects.c
 */
Localent*	CG_SmokePuff(const Vec3 p,
		const Vec3 vel,
		float radius,
		float r, float g, float b, float a,
		float duration,
		int startTime,
		int fadeInTime,
		int leFlags,
		Handle hShader);
void	CG_BubbleTrail(Vec3 start, Vec3 end, float spacing);
void	CG_SpawnEffect(Vec3 org);
void	CG_ScorePlum(int client, Vec3 org, int score);
void	CG_GibPlayer(Vec3 playerOrigin);
void	CG_BigExplode(Vec3 playerOrigin);
void	CG_Bleed(Vec3 origin, int entityNum);
Localent*	CG_MakeExplosion(Vec3 origin, Vec3 dir,
		Handle hModel, Handle shader, int msec,
		qbool isSprite);

/*
 * snapshot.c
 */
void	CG_ProcessSnapshots(void);

/*
 * info.c
 */
void	CG_LoadingString(const char *s);
void	CG_LoadingItem(int itemNum);
void	CG_LoadingClient(int clientNum);
void	CG_DrawInformation(void);

/*
 * scoreboard.c
 */
qbool	CG_DrawOldScoreboard(void);
void	CG_DrawOldTourneyScoreboard(void);

/*
 * consolecmds.c
 */
qbool	CG_ConsoleCommand(void);
void	CG_InitConsoleCommands(void);

/*
 * servercmds.c
 */
void	CG_ExecuteNewServerCommands(int latestSequence);
void	CG_ParseServerinfo(void);
void	CG_SetConfigValues(void);
void	CG_LoadVoiceChats(void);
void	CG_ShaderStateChanged(void);
void	CG_VoiceChatLocal(int mode, qbool voiceOnly, int clientNum, int color,
		const char *cmd);
void	CG_PlayBufferedVoiceChats(void);

/*
 * playerstate.c
 */
void	CG_Respawn(void);
void	CG_TransitionPlayerState(Playerstate *ps, Playerstate *ops);
void	CG_CheckChangedPredictableEvents(Playerstate *ps);

/*
 * System traps
 * These functions are how the cgame communicates with the main game system
 */

/* print message on the local console */
void	trap_Print(const char *fmt);

/* abort the game */
void	trap_Error(const char *fmt) __attribute__((noreturn));

/* this should only be used for performance tuning, never
 * for anything game related.  Get time from the CG_DrawActiveFrame parameter */
int	trap_Milliseconds(void);

/* console variable interaction */
void	trap_cvarregister(Vmcvar *vmCvar, const char *varName,
		const char *defaultValue, int flags);
void	trap_cvarupdate(Vmcvar *vmCvar);
void	trap_cvarsetstr(const char *var_name, const char *value);
void	trap_cvargetstrbuf(const char *var_name,
		char *buffer, int bufsize);

/* ServerCommand and ConsoleCommand parameter access */
int	trap_Argc(void);
void	trap_Argv(int n, char *buffer, int bufferLength);
void	trap_Args(char *buffer, int bufferLength);

/* returns length of file */
int	trap_fsopen(const char *qpath, Fhandle *f, Fsmode mode);
void	trap_fsread(void *buffer, int len, Fhandle f);
void	trap_fswrite(const void *buffer, int len, Fhandle f);
void	trap_fsclose(Fhandle f);
int	trap_fsseek(Fhandle f, long offset, int origin);	/* Fsorigin */

/* add commands to the local console as if they were typed in
 * for map changing, etc.  The command is not executed immediately,
 * but will be executed in order the next time console commands
 * are processed */
void	trap_SendConsoleCommand(const char *text);

/* register a command name so the console can perform command completion.
 * FIXME: replace this with a normal console command "defineCommand"? */
void	trap_AddCommand(const char *cmdName);
void	trap_RemoveCommand( const char *cmdName );

/* send a string to the server over the network */
void	trap_SendClientCommand(const char *s);

/* force a screen update, only used during gamestate load */
void	trap_UpdateScreen(void);

/* model collision */
void	trap_CM_LoadMap(const char *mapname);
int	trap_CM_NumInlineModels(void);
Cliphandle	trap_CM_InlineModel(int index);	/* 0 = world, 1+ = bmodels */
Cliphandle	trap_CM_TempBoxModel(const Vec3 mins, const Vec3 maxs);
int	trap_CM_PointContents(const Vec3 p, Cliphandle model);
int	trap_CM_TransformedPointContents(const Vec3 p,
		Cliphandle model, const Vec3 origin,
		const Vec3 angles);
void	trap_CM_BoxTrace(Trace *results, const Vec3 start,
		const Vec3 end,
		const Vec3 mins, const Vec3 maxs,
		Cliphandle model,
		int brushmask);
void	trap_CM_CapsuleTrace(Trace *results, const Vec3 start,
		const Vec3 end,
		const Vec3 mins, const Vec3 maxs,
		Cliphandle model,
		int brushmask);
void	trap_CM_TransformedBoxTrace(Trace *results, const Vec3 start,
		const Vec3 end, const Vec3 mins, const Vec3 maxs,
		Cliphandle model, int brushmask,
		const Vec3 origin, const Vec3 angles);

/* Returns the projection of a polygon onto the solid brushes in the world */
int	trap_CM_MarkFragments(int numPoints,
		const Vec3 *points, const Vec3 projection,
		int maxPoints, Vec3 pointBuffer,
		int maxFragments, Markfrag *fragmentBuffer);

/* normal sounds will have their volume dynamically changed as their entity
 * moves and the listener moves */
void	trap_sndstartsound(Vec3 origin, int entityNum, int entchannel,
		Handle sfx);
void	trap_sndstoploop(int entnum);

/* a local sound is always played full volume */
void	trap_sndstartlocalsound(Handle sfx, int channelNum);
void	trap_sndclearloops(qbool killall);
void	trap_sndaddloop(int entityNum, const Vec3 origin,
		const Vec3 velocity, Handle sfx);
void	trap_sndaddrealloop(int entityNum, const Vec3 origin,
		const Vec3 velocity, Handle sfx);
void	trap_sndupdateentpos(int entityNum, const Vec3 origin);

/* respatialize recalculates the volumes of sound as they should be heard by the
 * given entityNum and position */
void	trap_sndrespatialize(int entityNum, const Vec3 origin, Vec3 axis[3],
		int inwater);
Handle	trap_sndregister(const char *sample, qbool compressed);		/* returns a beepif not found */
void	trap_sndstartbackgroundtrack(const char *intro, const char *loop);	/* empty name stops music */
void	trap_sndstopbackgroundtrack(void);

void	trap_R_LoadWorldMap(const char *mapname);

/* all media should be registered during level startup to prevent
 * hitches during gameplay */
Handle	trap_R_RegisterModel(const char *name);		/* returns rgb axis if not found */
Handle	trap_R_RegisterSkin(const char *name);		/* returns all white if not found */
Handle	trap_R_RegisterShader(const char *name);	/* returns all white if not found */
Handle	trap_R_RegisterShaderNoMip(const char *name);	/* returns all white if not found */

/* a scene is built up by calls to R_ClearScene and the various R_Add functions.
 * Nothing is drawn until R_RenderScene is called. */
void	trap_R_ClearScene(void);
void	trap_R_AddRefEntityToScene(const Refent *re);

/* polys are intended for simple wall marks, not really for doing
 * significant construction */
void	trap_R_AddPolyToScene(Handle hShader, int numVerts,
		const Polyvert *verts);
void	trap_R_AddPolysToScene(Handle hShader, int numVerts,
		const Polyvert *verts,
		int numPolys);
void	trap_R_AddLightToScene(const Vec3 org, float intensity,
		float r, float g, float b);
void	trap_R_AddAdditiveLightToScene(const Vec3 org, 
		float intensity, float r, float g, float b);
int	trap_R_LightForPoint(Vec3 point, Vec3 ambientLight,
		Vec3 directedLight, Vec3 lightDir);
void	trap_R_RenderScene(const Refdef *fd);
void	trap_R_SetColor(const float *rgba);	/* NULL = 1,1,1,1 */
void	trap_R_DrawStretchPic(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2,
		Handle hShader);
void	trap_R_ModelBounds(Cliphandle model, Vec3 mins, Vec3 maxs);
int	trap_R_LerpTag(Orient *tag, Cliphandle mod,
		int startFrame, int endFrame,
		float frac, const char *tagName);
void	trap_R_RemapShader(const char *oldShader, const char *newShader,
		const char *timeOffset);
qbool	trap_R_inPVS(const Vec3 p1, const Vec3 p2);

/* The Glconfig will not change during the life of a cgame.
 * If it needs to change, the entire cgame will be restarted, because
 * all the Handle are then invalid. */
void	trap_GetGlconfig(Glconfig *glconfig);

/* the gamestate should be grabbed at startup, and whenever a
 * configstring changes */
void	trap_GetGameState(Gamestate *gamestate);

/* cgame will poll each frame to see if a newer snapshot has arrived
 * that it is interested in.  The time is returned seperately so that
 * snapshot latency can be calculated. */
void	trap_GetCurrentSnapshotNumber(int *snapshotNumber,
		int *serverTime);

/* a snapshot get can fail if the snapshot (or the entties it holds) is so
 * old that it has fallen out of the client system queue */
qbool	trap_GetSnapshot(int snapshotNumber, Snap *snapshot);

/* retrieve a text command from the server stream
 * the current snapshot will hold the number of the most recent command
 * qfalse can be returned if the client system handled the command
 * argc() / argv() can be used to examine the parameters of the command */
qbool	trap_GetServerCommand(int serverCommandNumber);

/* returns the most recent command number that can be passed to GetUserCmd
 * this will always be at least one higher than the number in the current
 * snapshot, and it may be quite a few higher if it is a fast computer on
 * a lagged connection */
int	trap_GetCurrentCmdNumber(void);
qbool	trap_GetUserCmd(int cmdNumber, Usrcmd *ucmd);
/* used for the weapon select and zoom */
void	trap_SetUserCmdValue(int, int, float);

/* aids for VM testing */
void	testPrintInt(char *string, int i);
void	testPrintFloat(char *string, float f);

int	trap_MemoryRemaining(void);
void	trap_R_RegisterFont(const char *fontName, int pointSize,
		Fontinfo *font);
qbool	trap_Key_IsDown(int keynum);
int	trap_Key_GetCatcher(void);
void	trap_Key_SetCatcher(int catcher);
int	trap_Key_GetKey(const char *binding);

typedef enum {
	SYSTEM_PRINT,
	CHAT_PRINT,
	TEAMCHAT_PRINT
} q3print_t;

int	trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width,
		int height, int bits);
Cinstatus	trap_CIN_StopCinematic(int handle);
Cinstatus	trap_CIN_RunCinematic(int handle);
void	trap_CIN_DrawCinematic(int handle);
void	trap_CIN_SetExtents(int handle, int x, int y, int w, int h);

int	trap_RealTime(Qtime *qtime);
void	trap_snapv3(float *v);

qbool	trap_loadCamera(const char *name);
void	trap_startCamera(int time);
qbool	trap_getCameraInfo(int time, Vec3 *origin, Vec3 *angles);

qbool	trap_GetEntityToken(char *buffer, int bufferSize);

void	CG_ClearParticles(void);
void	CG_AddParticles(void);
void	CG_ParticleSnow(Handle pshader, Vec3 origin, Vec3 origin2,
		int turb, float range, int snum);
void	CG_ParticleSmoke(Handle pshader, Centity *cent);
void	CG_AddParticleShrapnel(Localent *le);
void	CG_ParticleSnowFlurry(Handle pshader, Centity *cent);
void	CG_ParticleBulletDebris(Vec3 org, Vec3 vel, int duration);
void	CG_ParticleSparks(Vec3 org, Vec3 vel, int duration, float x, float y,
		float speed);
void	CG_ParticleDust(Centity *cent, Vec3 origin, Vec3 dir);
void	CG_ParticleMisc(Handle pshader, Vec3 origin, int size, int duration,
		float alpha);
void	CG_ParticleExplosion(char *animStr, Vec3 origin, Vec3 vel,
		int duration, int sizeStart, int sizeEnd);
extern qbool initparticles;
int	CG_NewParticleArea(int num);
