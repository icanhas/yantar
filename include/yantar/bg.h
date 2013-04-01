/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
/* Definitions shared by both the server game and client game modules. */

/* 
 * Because games can change separately from the main system version, we need a
 * second version that must match between game and cgame.
 */
#define GAME_VERSION		BASEGAME "-1"

#define DEFAULT_GRAVITY		800
#define GIB_HEALTH		-40
#define SHIELD_PROTECTION	0.66f

#define MAX_ITEMS		256

#define RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define ITEM_RADIUS		15	/* item sizes are needed for client side pickup detection */

#define LIGHTNING_RANGE		768

#define SCORE_NOT_PRESENT	-9999	/* for the CS_SCORES[12] when only one player is present */

#define VOTE_TIME		30000	/* 30 seconds before vote times out */

/* default player bounds when alive and well */
#define MaxsX			30
#define MaxsY			40
#define MaxsZ			30
#define MinsX			-MaxsX
#define MinsY			-MaxsY
#define MinsZ			-MaxsZ
#define DEFAULT_VIEWHEIGHT	15
#define CROUCH_VIEWHEIGHT	4
#define DEAD_VIEWHEIGHT		-16

/*
 * Config strings are a general means of communicating variable length strings
 * from the server to all connected clients.
 */
/* CS_SERVERINFO and CS_SYSTEMINFO are defined in shared.h */
#define CS_MUSIC		2
#define CS_MESSAGE		3	/* from the map worldspawn's message field */
#define CS_MOTD			4	/* g_motd string for server message of the day */
#define CS_WARMUP		5	/* server time when the match will be restarted */
#define CS_SCORES1		6
#define CS_SCORES2		7
#define CS_VOTE_TIME		8
#define CS_VOTE_STRING		9
#define CS_VOTE_YES		10
#define CS_VOTE_NO		11

#define CS_TEAMVOTE_TIME	12
#define CS_TEAMVOTE_STRING	14
#define CS_TEAMVOTE_YES		16
#define CS_TEAMVOTE_NO		18

#define CS_GAME_VERSION		20
#define CS_LEVEL_START_TIME	21	/* so the timer only shows the current level */
#define CS_INTERMISSION		22	/* when 1, fraglimit/timelimit has been hit and intermission will start in a second or two */
#define CS_FLAGSTATUS		23	/* string indicating flag status in CTF */
#define CS_SHADERSTATE		24
#define CS_BOTINFO		25

#define CS_ITEMS		27	/* string of 0's and 1's that tell which items are present */

#define CS_MODELS		32
#define CS_SOUNDS		(CS_MODELS+MAX_MODELS)
#define CS_PLAYERS		(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS		(CS_PLAYERS+MAX_CLIENTS)
#define CS_PARTICLES		(CS_LOCATIONS+MAX_LOCATIONS)

#define CS_MAX			(CS_PARTICLES+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef struct Pmove		Pmove;
typedef struct Gitem		Gitem;
typedef struct Anim	Anim;

typedef enum {
	GT_FFA,			/* free for all */
	GT_TOURNAMENT,		/* one on one tournament */
	GT_SINGLE_PLAYER,	/* single player ffa */

	/* -- team games go after this -- */

	GT_TEAM,		/* team deathmatch */
	GT_CTF,			/* capture the flag */
	GT_1FCTF,		/* one-flag CTF */
	GT_MAX_GAME_TYPE
} Gametype;

/* FIXME: remarkably, machines don't have genders */
typedef enum { 
	GENDER_MALE, 
	GENDER_FEMALE, 
	GENDER_NEUTER 
} Gender;

/*
 * pmove module
 *
 * The pmove code takes a player_state_t and a Usrcmd and generates a new player_state_t
 * and some other output data.  Used for local prediction on the client game and true
 * movement on the server game.
 */
typedef enum {
	PM_NORMAL,		/* can accelerate and turn */
	PM_NOCLIP,		/* noclip movement */
	PM_SPECTATOR,		/* still run into walls */
	PM_DEAD,		/* no acceleration or turning, but free falling */
	PM_FREEZE,		/* stuck in place with no control */
	PM_INTERMISSION,	/* no movement or status bar */
	PM_SPINTERMISSION	/* no movement or status bar */
} Pmtype;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} Weapstate;

/* pmove->pm_flags */
enum pmflags {
	PMF_JUMP_HELD		= (1<<0),
	PMF_BACKWARDS_JUMP	= (1<<1),	/* go into backwards land */
	PMF_BACKWARDS_RUN	= (1<<2),	/* coast down to backwards run */
	PMF_TIME_LAND		= (1<<3),	/* pm_time is time before rejump */
	PMF_TIME_KNOCKBACK	= (1<<4),	/* pm_time is an air-accelerate only time */
	PMF_TIME_WATERJUMP	= (1<<5),	/* pm_time is waterjump */
	PMF_RESPAWNED		= (1<<6),	/* clear after attack and jump buttons come up */
	PMF_USE_ITEM_HELD	= (1<<7),
	PMF_GRAPPLE_PULL	= (1<<8),	/* pull towards grapple location */
	PMF_FOLLOW		= (1<<9),	/* spectate following another player */
	PMF_SCOREBOARD		= (1<<10),	/* spectate as a scoreboard */
	PMF_ALL_TIMES		= (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)
};

#define MAXTOUCH		32
struct Pmove {
	/* state (in / out) */
	Playerstate *ps;

	/* command (in) */
	Usrcmd	cmd;
	int	tracemask;	/* collide against these types of surfaces */
	int	debugLevel;	/* if set, diagnostic output will be printed */
	qbool	noFootsteps;	/* if the game is setup for no footsteps by the server */
	qbool	gauntletHit;	/* true if a gauntlet attack would actually hit something */
	int	framecount;
	
	/* results (out) */
	int	numtouch;
	int	touchents[MAXTOUCH];
	Vec3	mins, maxs;	/* bounding box size */
	int	watertype;
	int	waterlevel;
	float	xyspeed;

	/* for fixed msec Pmove */
	int	pmove_fixed;
	int	pmove_msec;

	/* 
	 * callbacks to test the world
	 * these will be different functions during game and cgame 
	 */
	void (*trace)(Trace *results, const Vec3 start, const Vec3 mins,
		const Vec3 maxs, const Vec3 end, int passEntityNum,
		int contentMask);
	int (*pointcontents)(const Vec3 point, int passEntityNum);
};

/* player_state->stats[] indexes
 * NOTE: may not have more than 16 */
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_PERSISTENT_POWERUP,
	STAT_PRIWEAPS,	/* 16 bit fields, primary weapons */
	STAT_SECWEAPS,	/* 16 bit fields, secondary weapons (e.g. rockets) */
	STAT_HOOKWEAPS,	/* offhand hook has a stats slot as well, for uniformity */
	STAT_SHIELD,
	STAT_DEAD_YAW,		/* look this direction when dead (FIXME: get rid of?) */
	STAT_CLIENTS_READY,	/* bit mask of clients wishing to exit the intermission (FIXME: configstring?) */
	STAT_MAX_HEALTH		/* health / armor limit, changable by handicap */
} Statindex;

enum {
	Maxhealth	= 1000,	/* default HP/shield limit */
	Maxhealthguard	= 2000,	/* boosted HP limit (guard) */
};

/* 
 * player_state->persistant[] indexes
 * these fields are the only part of player_state that isn't
 * cleared on respawn
 * N.B.: may not have more than 16 
 */
typedef enum {
	PERS_SCORE,		/* !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!! */
	PERS_HITS,		/* total points damage inflicted so damage beeps can sound on change */
	PERS_RANK,		/* player rank or team rank */
	PERS_TEAM,		/* player team */
	PERS_SPAWN_COUNT,	/* incremented every respawn */
	PERS_PLAYEREVENTS,	/* 16 bits that can be flipped for events */
	PERS_ATTACKER,		/* clientnum of last damage inflicter */
	PERS_ATTACKEE_SHIELD,	/* health/armor of last person we attacked */
	PERS_KILLED,		/* count of the number of times you died */
	/* 
	 * player awards tracking 
	 */
	PERS_IMPRESSIVE_COUNT,		/* two railgun hits in a row */
	PERS_EXCELLENT_COUNT,		/* two successive kills in a short amount of time */
	PERS_DEFEND_COUNT,		/* defend awards */
	PERS_ASSIST_COUNT,		/* assist awards */
	PERS_GAUNTLET_FRAG_COUNT,	/* kills with the guantlet */
	PERS_CAPTURES			/* captures */
} Persenum;

/* Entstate->eFlags */
enum entflags {
	EF_DEAD			= (1<<0),	/* don't draw a foe marker over players with EF_DEAD */
	EF_TICKING		= (1<<1),	/* used to make players play the prox mine ticking sound */
	EF_TELEPORT_BIT		= (1<<2),	/* toggled every time the origin abruptly changes */
	EF_AWARD_EXCELLENT	= (1<<3),	/* draw an excellent sprite */
	EF_PLAYER_EVENT		= (1<<4),
	EF_BOUNCE		= (1<<5),	/* for missiles */
	EF_BOUNCE_HALF		= (1<<6),	/* for missiles */
	EF_AWARD_GAUNTLET	= (1<<7),	/* draw a gauntlet sprite */
	EF_NODRAW		= (1<<8),	/* may have an event, but no model (unspawned items) */
	EF_PRIFIRING		= (1<<9),	/* for continuous-fire weapons like LG (FIXME: feels hacky) */
	EF_SECFIRING		= (1<<10),
	EF_HOOKFIRING		= (1<<11),
	EF_KAMIKAZE		= (1<<12),
	EF_MOVER_STOP		= (1<<13),	/* will push otherwise */
	EF_AWARD_CAP		= (1<<14),	/* draw the capture sprite */
	EF_TALK			= (1<<15),	/* draw a talk balloon */
	EF_CONNECTION		= (1<<16),	/* draw a connection trouble sprite */
	EF_VOTED		= (1<<17),	/* already cast a vote */
	EF_AWARD_IMPRESSIVE	= (1<<18),	/* draw an impressive sprite */
	EF_AWARD_DEFEND		= (1<<19),	/* draw a defend sprite */
	EF_AWARD_ASSIST		= (1<<20),	/* draw a assist sprite */
	EF_AWARD_DENIED		= (1<<21),	/* denied */
	EF_TEAMVOTED		= (1<<22)	/* already cast a team vote */
};

/* NOTE: may not have more than 16 */
typedef enum {
	PW_NONE,
	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,
	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,
	PW_NUM_POWERUPS
} Powerup;

typedef enum {
	HI_NONE,
	HI_TELEPORTER,
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
	HI_INVULNERABILITY,
	HI_NUM_HOLDABLE
} Holdable;

typedef enum {
	/* weapons for primary mount point */
	Wnone,
	Wmelee,
	Wmachinegun,
	Wchaingun,
	Wshotgun,
	Wnanoidcannon,
	Wlightning,
	Wrailgun,
	Wplasmagun,
	Whook,
	/* weapons for secondary mount point */
	Wrocketlauncher,
	Whominglauncher,
	Wgrenadelauncher,
	Wproxlauncher,
	Wbfg,
	Wnumweaps
} Weapon;

/* reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS]) */
enum rewardsounds {
	PLAYEREVENT_DENIEDREWARD	= (1<<0),
	PLAYEREVENT_GAUNTLETREWARD	= (1<<1),
	PLAYEREVENT_HOLYSHIT		= (1<<2)
};

/* 
 * Entstate->event values
 * Entity events are for effects that take place relative
 * to an existing entities origin.  Very network efficient. 
 */
/* 
 * two bits at the top of the entityState->event field
 * will be incremented with each change in the event so
 * that an identical event started twice in a row can
 * be distinguished.  And off the value with ~EV_EVENT_BITS
 * to retrieve the actual event number 
 */
#define EV_EVENT_BIT1		0x00000100
#define EV_EVENT_BIT2		0x00000200
#define EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define EVENT_VALID_MSEC	300

/* TODO: remove foot stuff, add wall collisions and so on */
typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,

	EV_JUMP_PAD,		/* boing sound at origin, jump sound on player */

	EV_JUMP,
	EV_WATER_TOUCH,		/* foot touches */
	EV_WATER_LEAVE,		/* foot leaves */
	EV_WATER_UNDER,		/* head touches */
	EV_WATER_CLEAR,		/* head leaves */

	EV_ITEM_PICKUP,		/* normal item pickups are predictable */
	EV_GLOBAL_ITEM_PICKUP,	/* powerup / team sounds are broadcast to everyone */

	EV_NOAMMO,		/* primary ammo ran out */
	EV_NOSECAMMO,		/* seconary ammo ran out */
	EV_CHANGE_WEAPON,	/* primary weapon changes */
	EV_CHANGESECWEAP,	/* secondary weapon changes */
	EV_FIREPRIWEAP,		/* firing primary weapon */
	EV_FIRESECWEAP,		/* firing secondary weapon */
	EV_FIREHOOK,		/* firing grappling hook */

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,	/* eventParm will be the soundindex */

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,	/* no attenuation */
	EV_GLOBAL_TEAM_SOUND,

	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,		/* otherEntity is the shooter */

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,

	EV_GIB_PLAYER,		/* gib a previously living player */
	EV_SCOREPLUM,		/* score plum */

	EV_PROXIMITY_MINE_STICK,
	EV_PROXIMITY_MINE_TRIGGER,

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL
} Entevent;

typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;

/* animations */
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,

	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} Animnum;

struct Anim {
	int	firstFrame;
	int	numFrames;
	int	loopFrames;	/* 0 to numFrames */
	int	frameLerp;	/* msec between frames */
	int	initialLerp;	/* msec to get to first frame */
	int	reversed;	/* true if animation is reversed */
	int	flipflop;	/* true if animation should flipflop back to base */
};

/* 
 * flip the togglebit every time an animation
 * changes so a restart of the same anim can be detected 
 */
#define ANIM_TOGGLEBIT 128

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,
	TEAM_NUM_TEAMS
} Team;

/* Time between location updates */
#define TEAM_LOCATION_UPDATE_TIME 1000

/* How many players on the overlay */
#define TEAM_MAXOVERLAY 32

/* team task */
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE,
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} Teamtask;

/* means of death */
typedef enum {
	MOD_UNKNOWN,
	MOD_SHOTGUN,
	MOD_GAUNTLET,
	MOD_MACHINEGUN,
	MOD_GRENADE,
	MOD_GRENADE_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_HOMINGROCKET,
	MOD_HOMINGROCKET_SPLASH,
	MOD_PLASMA,
	MOD_PLASMA_SPLASH,
	MOD_RAILGUN,
	MOD_LIGHTNING,
	MOD_BFG,
	MOD_BFG_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_NANOID,
	MOD_CHAINGUN,
	MOD_PROXIMITY_MINE,
	MOD_GRAPPLE
} Meansofdeath;

/* Gitem->type */
typedef enum {
	IT_BAD,
	IT_PRIWEAP,	/* EFX: rotate + upscale + minlight */
	IT_SECWEAP,	/* secondary weapon */
	IT_AMMO,	/* EFX: rotate */
	IT_SHIELD,	/* EFX: rotate + minlight */
	IT_HEALTH,	/* EFX: static external sphere + rotating internal */
	IT_POWERUP,	/* instant on, timer based */
	/* EFX: rotate + external ring that rotates */
	IT_HOLDABLE,	/* single use, holdable item */
	/* EFX: rotate + bob */
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} Itemtype;

#define MAX_ITEM_MODELS 4

struct Gitem {
	char	*classname;	/* spawning name */
	char	*pickupsound;
	char	*worldmodel[MAX_ITEM_MODELS];
	char	*icon;
	char	*pickupname;	/* for printing on pickup */
	int	quantity;	/* for ammo how much, or duration of powerup */
	Itemtype	type;		/* IT_* flags */
	int	tag;
	char	*precaches;	/* string of all models and images this item will use */
	char	*sounds;	/* string of all sounds this item will use */
};
#define ITEM_INDEX(x) ((x)-bg_itemlist)

/* g_dmflags->integer flags */
#define DF_NO_FALLING	8
#define DF_FIXED_FOV	16
#define DF_NO_FOOTSTEPS 32

/* content masks */
#define MASK_ALL		(-1)
#define MASK_SOLID		(CONTENTS_SOLID)
#define MASK_PLAYERSOLID	(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define MASK_DEADSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define MASK_WATER		(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE		(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT		(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)

/*
 * Entstate->eType
 */
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_INERTMISSILE,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,	/* grapple hooked on wall */
	ET_TEAM,
	ET_EVENTS	/* any of the EV_* events can be added freestanding
			 * by setting eType to ET_EVENTS + eventNum
			 * this avoids having to set eFlags and eventNum */
} Enttype;

#define ARENAS_PER_TIER	4
#define MAX_ARENAS	1024
#define MAX_ARENAS_TEXT	8192
#define MAX_BOTS	1024
#define MAX_BOTS_TEXT	8192

/* Kamikaze */
/* 1st shockwave times */
#define KAMI_SHOCKWAVE_STARTTIME	0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME		2000
/* explosion/implosion times */
#define KAMI_EXPLODE_STARTTIME		250
#define KAMI_IMPLODE_STARTTIME		2000
#define KAMI_IMPLODE_ENDTIME		2250
/* 2nd shockwave times */
#define KAMI_SHOCKWAVE2_STARTTIME	2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME		3000
/* radius of the models without scaling */
#define KAMI_SHOCKWAVEMODEL_RADIUS	88
#define KAMI_BOOMSPHEREMODEL_RADIUS	72
/* maximum radius of the models during the effect */
#define KAMI_SHOCKWAVE_MAXRADIUS	1320
#define KAMI_BOOMSPHERE_MAXRADIUS	720
#define KAMI_SHOCKWAVE2_MAXRADIUS	704

/* included in both the game dll and the client */
extern Gitem bg_itemlist[];
extern int bg_numItems;

Gitem*	BG_FindItem(const char *pickupName);
Gitem*	BG_FindItemForWeapon(Weapon weapon);
Gitem*	BG_FindItemForPowerup(Powerup pw);
Gitem*	BG_FindItemForHoldable(Holdable pw);
qbool	BG_CanItemBeGrabbed(int gametype, const Entstate *ent,
		const Playerstate *ps);
				    
/* if a full pmove isn't done on the client, you can just update the angles */
void	PM_UpdateViewAngles(Playerstate *ps, const Usrcmd *cmd);
void	PM_Pmove(Pmove *pmove);

void	BG_EvaluateTrajectory(const Trajectory *tr, int atTime, Vec3 result);
void	BG_EvaluateTrajectoryDelta(const Trajectory *tr, int atTime,
		Vec3 result);
void	BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm,
		Playerstate *ps);
void	BG_TouchJumpPad(Playerstate *ps, Entstate *jumppad);
void	BG_PlayerStateToEntityState(Playerstate *ps, Entstate *s,
		qbool snap);
void	BG_PlayerStateToEntityStateExtraPolate(Playerstate *ps,
		Entstate *s, int time, qbool snap);
qbool	BG_PlayerTouchesItem(Playerstate *ps, Entstate *item,
		int atTime);
