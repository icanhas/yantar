/* 
 * local definitions for game module 
 * "bg.h" and "public.h" must be included before me
 */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/* the "gameversion" client command will print this plus compile date */
#define GAMEVERSION			BASEGAME

#define BODY_QUEUE_SIZE			8

#define INFINITE			1000000

#define FRAMETIME			100	/* msec */
#define CARNAGE_REWARD_TIME		3000
#define REWARD_SPRITE_TIME		2000

#define INTERMISSION_DELAY_TIME		1000
#define SP_INTERMISSION_DELAY_TIME	5000

/* gentity->flags */
#define FL_GODMODE		0x00000010
#define FL_NOTARGET		0x00000020
#define FL_TEAMSLAVE		0x00000400	/* not the first on the team */
#define FL_NO_KNOCKBACK		0x00000800
#define FL_DROPPED_ITEM		0x00001000
#define FL_NO_BOTS		0x00002000	/* spawn point not for bot use */
#define FL_NO_HUMANS		0x00004000	/* spawn point just for bots */
#define FL_FORCE_GESTURE	0x00008000	/* force gesture on client */

/* movers are things like doors, plats, buttons, etc */
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

#define SP_PODIUM_MODEL Pobjectmodels "/podium/podium4.md3"

typedef struct gentity_s Gentity;
typedef struct gclient_s gClient;

struct gentity_s {
	Entstate	s;	/* communicated by server to clients */
	Entshared	r;	/* shared by both the server system and game */

	/* DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	 * EXPECTS THE FIELDS IN THAT ORDER!
	 * ================================ */

	struct gclient_s	*client;	/* NULL if not a client */

	qbool			inuse;

	char			*classname;	/* set in QuakeEd */
	int			spawnflags;	/* set in QuakeEd */

	qbool			neverFree;	/* if true, FreeEntity will only unlink */
	/* bodyque uses this */

	int		flags;	/* FL_* variables */

	char		*model;
	char		*model2;
	int		freetime;	/* level.time when the object was freed */

	int		eventTime;	/* events will be cleared EVENT_VALID_MSEC after set */
	qbool		freeAfterEvent;
	qbool		unlinkAfterEvent;

	qbool		physicsObject;	/* if true, it can be pushed by movers and fall off edges */
	/* all game items are physicsObjects, */
	float		physicsBounce;	/* 1.0 = continuous bounce, 0.0 = no bounce */
	int		clipmask;	/* brushes with this content value will be collided against */
	/* when moving.  items and corpses do not collide against
	 * players, for instance */

	/* movers */
	moverState_t	moverState;
	int		soundPos1;
	int		sound1to2;
	int		sound2to1;
	int		soundPos2;
	int		soundLoop;
	Gentity	*parent;
	Gentity	*nextTrain;
	Gentity	*prevTrain;
	Vec3		pos1, pos2;

	char		*message;

	int		timestamp;	/* body queue sinking, etc */

	char		*target;
	char		*targetname;
	char		*team;
	char		*targetShaderName;
	char		*targetShaderNewName;
	Gentity	*target_ent;

	float		speed;
	Vec3		movedir;

	int		nextthink;
	void		(*think)(Gentity *self);
	void		(*reached)(Gentity *self);	/* movers call this when hitting endpoint */
	void		(*blocked)(Gentity *self, Gentity *other);
	void		(*touch)(Gentity *self, Gentity *other,
				 Trace *trace);
	void		(*use)(Gentity *self, Gentity *other,
			       Gentity *activator);
	void		(*pain)(Gentity *self, Gentity *attacker, int damage);
	void		(*die)(Gentity *self, Gentity *inflictor,
			       Gentity *attacker, int damage, int mod);

	int		pain_debounce_time;
	int		fly_sound_debounce_time;	/* wind tunnel */
	int		last_move_time;

	int		health;

	qbool		takedamage;

	int		damage;
	int		splashDamage;	/* quad will increase this without increasing radius */
	int		splashRadius;
	int		methodOfDeath;
	int		splashMethodOfDeath;

	int		count;

	Gentity	*chain;
	Gentity	*enemy;
	Gentity	*activator;
	Gentity	*teamchain;	/* next entity in team */
	Gentity	*teammaster;	/* master of the team */

#ifdef MISSIONPACK
	int	kamikazeTime;
	int	kamikazeShockTime;
#endif

	int	watertype;
	int	waterlevel;

	int	noise_index;

	/* timing variables */
	float	wait;
	float	random;

	Gitem *item;	/* for bonus items */
};

typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,	/* Beginning a team game, spawn at base */
	TEAM_ACTIVE	/* Now actively playing */
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	int	location;

	int	captures;
	int	basedefense;
	int	carrierdefense;
	int	flagrecovery;
	int	fragcarrier;
	int	assists;

	float	lasthurtcarrier; 
	float	lastreturnedflag;
	float	flagsince;
	float	lastfraggedcarrier;
} playerTeamState_t;

/* client data that stays across multiple levels or tournament restarts
 * this is achieved by writing all the data to cvar strings at game shutdown
 * time and reading them back at connection time.  Anything added here
 * MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData() */
typedef struct {
	Team			sessionTeam;
	int			spectatorNum;		/* for determining next-in-line to play */
	spectatorState_t	spectatorState;
	int			spectatorClient;	/* for chasecam and follow mode */
	int			wins, losses;		/* tournament stats */
	qbool			teamLeader;		/* true when this client is a team leader */
} clientSession_t;

#define MAX_NETNAME	36
#define MAX_VOTE_COUNT	3

/* client data that stays across multiple respawns, but is cleared
 * on each level change or team change at ClientBegin() */
typedef struct {
	clientConnected_t	connected;
	Usrcmd		cmd;			/* we would lose angles if not persistent */
	qbool			localClient;		/* true if "ip" info key is "localhost" */
	qbool			initialSpawn;		/* the first spawn should be at a cool location */
	qbool			predictItemPickup;	/* based on cg_predictItems userinfo */
	qbool			pmoveFixed;
	char			netname[MAX_NETNAME];
	int			maxHealth;		/* for handicapping */
	int			enterTime;		/* level.time the client entered the game */
	playerTeamState_t	teamState;		/* status in teamplay games */
	int			voteCount;		/* to prevent people from constantly calling votes */
	int			teamVoteCount;		/* to prevent people from constantly calling votes */
	qbool			teamInfo;		/* send team overlay updates? */
} clientPersistant_t;

/* this structure is cleared on each ClientSpawn(),
 * except for 'client->pers' and 'client->sess' */
struct gclient_s {
	/* ps MUST be the first element, because the server expects it */
	Playerstate ps;	/* communicated by server to clients */

	/* the rest of the structure is private to game */
	clientPersistant_t	pers;
	clientSession_t		sess;

	qbool			readyToExit;	/* wishes to leave the intermission */

	qbool			noclip;

	int			lastCmdTime;	/* level.time of last Usrcmd, for EF_CONNECTION */
	/* we can't just use pers.lastCommand.time, because
	 * of the g_sycronousclients case */
	int	buttons;
	int	oldbuttons;
	int	latched_buttons;

	Vec3	oldOrigin;

	/* sum up damage over an entire frame, so
	 * shotgun blasts give a single big kick */
	int		damage_armor;		/* damage absorbed by armor */
	int		damage_blood;		/* damage taken out of health */
	int		damage_knockback;	/* impact damage */
	Vec3		damage_from;		/* origin for vector calculation */
	qbool		damage_fromWorld;	/* if true, don't use the damage_from vector */

	int		accurateCount;	/* for "impressive" reward sound */

	int		accuracy_shots;	/* total number of shots */
	int		accuracy_hits;	/* total number of hits */

	int	lastkilled_client;	/* last client that this client killed */
	int	lasthurt_client;	/* last client that damaged this client */
	int	lasthurt_mod;		/* type of damage the client did */

	/* timers */
	int		respawnTime;		/* can respawn when time > this, force after g_forcerespwan */
	int		inactivityTime;		/* kick players when time > this */
	qbool		inactivityWarning;	/* qtrue if the five seoond warning has been given */
	int		rewardTime;		/* clear the EF_AWARD_IMPRESSIVE, etc when time > this */

	int		airOutTime;

	int		lastKillTime;	/* for multiple kill rewards */

	qbool		fireHeld;	/* used for hook */
	Gentity	*hook;		/* grapple hook if out */

	int		switchTeamTime;	/* time the player switched teams */

	/* timeResidual is used to handle events that happen every second
	 * like health / armor countdowns and regeneration */
	int timeResidual;

	Gentity	*persistantPowerup;

	char *areabits;
};

/*
 * this structure is cleared as each map is entered
 */
#define MAX_SPAWN_VARS		64
#define MAX_SPAWN_VARS_CHARS	4096

typedef struct {
	struct gclient_s	*clients;	/* [maxclients] */

	struct gentity_s	*gentities;
	int			gentitySize;
	int			num_entities;	/* MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL */

	int			warmupTime;	/* restart match at this time */

	Fhandle		logFile;

	/* store latched cvars here that we want to get at often */
	int		maxclients;

	int		framenum;
	int		time;		/* in msec */
	int		previousTime;	/* so movers can back up when blocked */

	int		startTime;	/* level.time the map was started */

	int		teamScores[TEAM_NUM_TEAMS];
	int		lastTeamLocationTime;	/* last time of client team location update */

	qbool		newSession;	/* don't use any old session data, because */
	/* we changed gametype */

	qbool		restarted;	/* waiting for a map_restart to fire */

	int		numConnectedClients;
	int		numNonSpectatorClients;		/* includes connecting clients */
	int		numPlayingClients;		/* connected, non-spectators */
	int		sortedClients[MAX_CLIENTS];	/* sorted by score */
	int		follow1, follow2;		/* clientNums for auto-follow spectators */

	int		snd_fry;	/* sound index for standing in lava */

	int		warmupModificationCount;	/* for detecting if g_warmup is changed */

	/* voting state */
	char	voteString[MAX_STRING_CHARS];
	char	voteDisplayString[MAX_STRING_CHARS];
	int	voteTime;		/* level.time vote was called */
	int	voteExecuteTime;	/* time the vote is executed */
	int	voteYes;
	int	voteNo;
	int	numVotingClients;	/* set by CalculateRanks */

	/* team voting state */
	char	teamVoteString[2][MAX_STRING_CHARS];
	int	teamVoteTime[2];	/* level.time vote was called */
	int	teamVoteYes[2];
	int	teamVoteNo[2];
	int	numteamVotingClients[2];	/* set by CalculateRanks */

	/* spawn variables */
	qbool		spawning;	/* the G_Spawn*() functions are valid */
	int		numSpawnVars;
	char		*spawnVars[MAX_SPAWN_VARS][2];	/* key / value pairs */
	int		numSpawnVarChars;
	char		spawnVarChars[MAX_SPAWN_VARS_CHARS];

	/* intermission state */
	int intermissionQueued;	/* intermission was qualified, but */
	/* wait INTERMISSION_DELAY_TIME before
	 * actually going there so the last
	 * frag can be watched.  Disable future
	 * kills during this delay */
	int		intermissiontime;	/* time the intermission was started */
	char		*changemap;
	qbool		readyToExit;	/* at least one client wants to exit */
	int		exitTime;
	Vec3		intermission_origin;	/* also used for spectator spawns */
	Vec3		intermission_angle;

	qbool		locationLinked;	/* target_locations get linked */
	Gentity	*locationHead;	/* head of the location list */
	int		bodyQueIndex;	/* dead bodies */
	Gentity	*bodyQue[BODY_QUEUE_SIZE];
#ifdef MISSIONPACK
	int		portalSequence;
#endif
} level_locals_t;

/*
 * spawn.c
 */
qbool	G_SpawnString(const char *key, const char *defaultString,
		char **out);
/*	spawn string returns a temporary reference, you must Copystr() if you want to keep it */
qbool	G_SpawnFloat(const char *key, const char *defaultString,
		float *out);
qbool	G_SpawnInt(const char *key, const char *defaultString, int *out);
qbool	G_SpawnVector(const char *key, const char *defaultString,
		float *out);
void	G_SpawnEntitiesFromString(void);
char*	G_NewString(const char *string);

/*
 * cmds.c
 */
void	Cmd_Score_f(Gentity *ent);
void	StopFollowing(Gentity *ent);
void	BroadcastTeamChange(gClient *client, int oldTeam);
void	SetTeam(Gentity *ent, char *s);
void	Cmd_FollowCycle_f(Gentity *ent, int dir);

/*
 * items.c
 */
void	G_CheckTeamItems(void);
void	G_RunItem(Gentity *ent);
void	RespawnItem(Gentity *ent);

void	UseHoldableItem(Gentity *ent);
void	PrecacheItem(Gitem *it);
Gentity*	Drop_Item(Gentity *ent, Gitem *item, float angle);
Gentity*	LaunchItem(Gitem *item, Vec3 origin, Vec3 velocity);
void	SetRespawn(Gentity *ent, float delay);
void	G_SpawnItem(Gentity *ent, Gitem *item);
void	FinishSpawningItem(Gentity *ent);
void	Think_Weapon(Gentity *ent);
int	ArmorIndex(Gentity *ent);
void	Add_Ammo(Gentity *ent, int weapon, int count);
void	Touch_Item(Gentity *ent, Gentity *other, Trace *trace);

void	ClearRegisteredItems(void);
void	RegisterItem(Gitem *item);
void	SaveRegisteredItems(void);

/*
 * utils.c
 */
int	G_ModelIndex(char *name);
int	G_SoundIndex(char *name);
void	G_TeamCommand(Team team, char *cmd);
void	G_KillBox(Gentity *ent);
Gentity*	G_Find(Gentity *from, int fieldofs, const char *match);
Gentity*	G_PickTarget(char *targetname);
void	G_UseTargets(Gentity *ent, Gentity *activator);
void	G_SetMovedir(Vec3 angles, Vec3 movedir);

void	G_InitGentity(Gentity *e);
Gentity*	G_Spawn(void);
Gentity*	G_TempEntity(Vec3 origin, int event);
void	G_Sound(Gentity *ent, int channel, int soundIndex);
void	G_FreeEntity(Gentity *e);
qbool	G_EntitiesFree(void);

void	G_TouchTriggers(Gentity *ent);

float*	tv(float x, float y, float z);
char*	vtos(const Vec3 v);

float	vectoyaw(const Vec3 vec);

void	G_AddPredictableEvent(Gentity *ent, int event, int eventParm);
void	G_AddEvent(Gentity *ent, int event, int eventParm);
void	G_SetOrigin(Gentity *ent, Vec3 origin);
void	AddRemap(const char *oldShader, const char *newShader, float timeOffset);
char*	BuildShaderStateConfig(void); /* FIXME */

/*
 * combat.c
 */
qbool	CanDamage(Gentity *targ, Vec3 origin);
void	G_Damage(Gentity *targ, Gentity *inflictor, Gentity *attacker,
		Vec3 dir, Vec3 point, int damage, int dflags, int mod);
qbool	G_RadiusDamage(Vec3 origin, Gentity *attacker, float damage,
		float radius, Gentity *ignore, int mod);
int	G_InvulnerabilityEffect(Gentity *targ, Vec3 dir, Vec3 point,
		Vec3 impactpoint, Vec3 bouncedir);
void	body_die(Gentity *self, Gentity *inflictor, Gentity *attacker,
		int damage, int meansOfDeath);
void	TossClientItems(Gentity *self);
#ifdef MISSIONPACK
void	TossClientPersistantPowerups(Gentity *self);
#endif
void	TossClientCubes(Gentity *self);

/* damage flags */
#define DAMAGE_RADIUS			0x00000001	/* damage was indirect */
#define DAMAGE_NO_SHIELD			0x00000002	/* armour does not protect from this damage */
#define DAMAGE_NO_KNOCKBACK		0x00000004	/* do not affect velocity, just view angles */
#define DAMAGE_NO_PROTECTION		0x00000008	/* armor, shields, invulnerability, and godmode have no effect */
#ifdef MISSIONPACK
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010	/* armor, shields, invulnerability, and godmode have no effect */
#endif

/*
 * missile.c
 */
void	G_RunMissile(Gentity *ent);
Gentity*	fire_plasma(Gentity *self, Vec3 start, Vec3 aimdir);
Gentity*	fire_grenade(Gentity *self, Vec3 start, Vec3 aimdir);
Gentity*	fire_rocket(Gentity *self, Vec3 start, Vec3 dir);
Gentity*	fire_bfg(Gentity *self, Vec3 start, Vec3 dir);
Gentity*	fire_grapple(Gentity *self, Vec3 start, Vec3 dir);
Gentity*	firenanoid(Gentity *self, Vec3 start, Vec3 forward, Vec3 right,
		Vec3 up);
Gentity*	fire_prox(Gentity *self, Vec3 start, Vec3 aimdir);

/*
 * mover.c
 */
void	G_RunMover(Gentity *ent);
void	Touch_DoorTrigger(Gentity *ent, Gentity *other, Trace *trace);

/*
 * trigger.c
 */
void	trigger_teleporter_touch(Gentity *self, Gentity *other, Trace *trace);

/*
 * misc.c
 */
void	TeleportPlayer(Gentity *player, Vec3 origin, Vec3 angles);
#ifdef MISSIONPACK
void	DropPortalSource(Gentity *ent);
void	DropPortalDestination(Gentity *ent);
#endif

/*
 * weapon.c
 */
qbool	LogAccuracyHit(Gentity *target, Gentity *attacker);
void	CalcMuzzlePoint(Gentity *ent, Vec3 forward, Vec3 right, Vec3 up,
		Vec3 muzzlePoint);
void	snapv3Towards(Vec3 v, Vec3 to);
qbool	CheckGauntletAttack(Gentity *ent);
void	Weapon_HookFree(Gentity *ent);
void	Weapon_HookThink(Gentity *ent);

/*
 * client.c
 */
Team	TeamCount(int ignoreClientNum, int team);
int	TeamLeader(int team);
Team	PickTeam(int ignoreClientNum);
void	SetClientViewAngle(Gentity *ent, Vec3 angle);
Gentity*	SelectSpawnPoint(Vec3 avoidPoint, Vec3 origin, Vec3 angles,
		qbool isbot);
void	CopyToBodyQue(Gentity *ent);
void	ClientRespawn(Gentity *ent);
void	BeginIntermission(void);
void	InitBodyQue(void);
void	ClientSpawn(Gentity *ent);
void	player_die(Gentity *self, Gentity *inflictor, Gentity *attacker,
		int damage, int mod);
void	AddScore(Gentity *ent, Vec3 origin, int score);
void	CalculateRanks(void);
qbool	SpotWouldTelefrag(Gentity *spot);

/*
 * svcmds.c
 */
qbool	ConsoleCommand(void);
void	G_ProcessIPBans(void);
qbool	G_FilterPacket(char *from);

/*
 * weapon.c
 */
void	FireWeapon(Gentity*, Weapslot);
#ifdef MISSIONPACK
void	G_StartKamikaze(Gentity *ent);
#endif

/*
 * cmds.c
 */
void	DeathmatchScoreboardMessage(Gentity *ent);

/*
 * main.c
 */
void	MoveClientToIntermission(Gentity *ent);
void	FindIntermissionPoint(void);
void	SetLeader(int team, uint client);
void	CheckTeamLeader(int team);
void	G_RunThink(Gentity *ent);
void	AddTournamentQueue(gClient *client);
void	QDECL G_LogPrintf(const char *fmt,
		...) __attribute__ ((format (printf, 1, 2)));
void	SendScoreboardMessageToAllClients(void);
void	QDECL G_Printf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void	QDECL G_Error(const char *fmt,
		...) __attribute__ ((noreturn, format (printf, 1, 2)));

/*
 * client.c
 */
char*	ClientConnect(int clientNum, qbool firstTime, qbool isBot);
void	ClientUserinfoChanged(int clientNum);
void	ClientDisconnect(int clientNum);
void	ClientBegin(int clientNum);
void	ClientCommand(int clientNum);

/*
 * active.c
 */
void	ClientThink(int clientNum);
void	ClientEndFrame(Gentity *ent);
void	G_RunClient(Gentity *ent);

/*
 * team.c
 */
qbool	OnSameTeam(Gentity *ent1, Gentity *ent2);
void	Team_CheckDroppedItem(Gentity *dropped);
qbool	CheckObeliskAttack(Gentity *obelisk, Gentity *attacker);

/*
 * mem.c
 */
void*	G_Alloc(int size);
void	G_InitMemory(void);
void	Svcmd_GameMem_f(void);

/*
 * session.c
 */
void	G_ReadSessionData(gClient *client);
void	G_InitSessionData(gClient *client, char *userinfo);
void	G_InitWorldSession(void);
void	G_WriteSessionData(void);

/*
 * arenas.c
 */
void	UpdateTournamentInfo(void);
void	SpawnModelsOnVictoryPads(void);
void	Svcmd_AbortPodium_f(void);

/*
 * bot.c
 */
void	G_InitBots(qbool restart);
char*	G_GetBotInfoByNumber(int num);
char*	G_GetBotInfoByName(const char *name);
void	G_CheckBotSpawn(void);
void	G_RemoveQueuedBotBegin(int clientNum);
qbool	G_BotConnect(int clientNum, qbool restart);
void	Svcmd_AddBot_f(void);
void	Svcmd_BotList_f(void);
void	BotInterbreedEndMatch(void);

/* 
 * ai_main.c 
 */
#define MAX_FILEPATH 144

/* bot settings */
typedef struct bot_settings_s {
	char	characterfile[MAX_FILEPATH];
	float	skill;
	char	team[MAX_FILEPATH];
} bot_settings_t;

int	BotAISetup(int restart);
int	BotAIShutdown(int restart);
int	BotAILoadMap(int restart);
int	BotAISetupClient(int client, struct bot_settings_s *settings,
		qbool restart);
int	BotAIShutdownClient(int client, qbool restart);
int	BotAIStartFrame(int time);
void	BotTestAAS(Vec3 origin);

extern level_locals_t level;
extern Gentity g_entities[MAX_GENTITIES];

#define FOFS(x) ((size_t)&(((Gentity*)0)->x))

extern Vmcvar g_gametype;
extern Vmcvar g_dedicated;
extern Vmcvar g_cheats;
extern Vmcvar g_maxclients;		/* allow this many total, including spectators */
extern Vmcvar g_maxGameClients;	/* allow this many active */
extern Vmcvar g_restarted;

extern Vmcvar g_dmflags;
extern Vmcvar g_fraglimit;
extern Vmcvar g_timelimit;
extern Vmcvar g_capturelimit;
extern Vmcvar g_friendlyFire;
extern Vmcvar g_password;
extern Vmcvar g_needpass;
extern Vmcvar g_gravity;
extern Vmcvar g_speed;
extern Vmcvar g_swingstrength;
extern Vmcvar g_knockback;
extern Vmcvar g_quadfactor;
extern Vmcvar g_forcerespawn;
extern Vmcvar g_inactivity;
extern Vmcvar g_debugMove;
extern Vmcvar g_debugAlloc;
extern Vmcvar g_debugDamage;
extern Vmcvar g_weaponRespawn;
extern Vmcvar g_weaponTeamRespawn;
extern Vmcvar g_synchronousClients;
extern Vmcvar g_motd;
extern Vmcvar g_warmup;
extern Vmcvar g_doWarmup;
extern Vmcvar g_blood;
extern Vmcvar g_allowVote;
extern Vmcvar g_teamAutoJoin;
extern Vmcvar g_teamForceBalance;
extern Vmcvar g_banIPs;
extern Vmcvar g_filterBan;
extern Vmcvar g_obeliskHealth;
extern Vmcvar g_obeliskRegenPeriod;
extern Vmcvar g_obeliskRegenAmount;
extern Vmcvar g_obeliskRespawnDelay;
extern Vmcvar g_cubeTimeout;
extern Vmcvar g_redteam;
extern Vmcvar g_blueteam;
extern Vmcvar g_smoothClients;
extern Vmcvar pmove_fixed;
extern Vmcvar pmove_msec;
extern Vmcvar g_rankings;
extern Vmcvar g_enableDust;
extern Vmcvar g_enableBreath;
extern Vmcvar g_singlePlayer;
extern Vmcvar g_proxMineTimeout;

/*
 * vm syscalls
 */
void	trap_Print(const char *fmt);
void	trap_Error(const char *fmt) __attribute__((noreturn));
int	trap_Milliseconds(void);
int	trap_RealTime(Qtime *qtime);
int	trap_Argc(void);
void	trap_Argv(int n, char *buffer, int bufferLength);
void	trap_Args(char *buffer, int bufferLength);
int	trap_FS_FOpenFile(const char *qpath, Fhandle *f,
		Fsmode mode);
void	trap_FS_Read(void *buffer, int len, Fhandle f);
void	trap_FS_Write(const void *buffer, int len, Fhandle f);
void	trap_FS_FCloseFile(Fhandle f);
int	trap_FS_GetFileList(const char *path, const char *extension,
		char *listbuf, int bufsize);
int	trap_FS_Seek(Fhandle f, long offset, int origin);	/* Fsorigin */
void	trap_SendConsoleCommand(int exec_when, const char *text);
void	trap_Cvar_Register(Vmcvar *cvar, const char *var_name,
		const char *value, int flags);
void	trap_Cvar_Update(Vmcvar *cvar);
void	trap_Cvar_Set(const char *var_name, const char *value);
int	trap_Cvar_VariableIntegerValue(const char *var_name);
float	trap_Cvar_VariableValue(const char *var_name);
void	trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer,
		int bufsize);
void	trap_LocateGameData(Gentity *gEnts, int numGEntities,
		int sizeofGEntity_t, Playerstate *gameClients,
		int sizeofGameClient);
void	trap_DropClient(int clientNum, const char *reason);
void	trap_SendServerCommand(int clientNum, const char *text);
void	trap_SetConfigstring(int num, const char *string);
void	trap_GetConfigstring(int num, char *buffer, int bufferSize);
void	trap_GetUserinfo(int num, char *buffer, int bufferSize);
void	trap_SetUserinfo(int num, const char *buffer);
void	trap_GetServerinfo(char *buffer, int bufferSize);
void	trap_SetBrushModel(Gentity *ent, const char *name);
void	trap_Trace(Trace *results, const Vec3 start, const Vec3 mins,
		const Vec3 maxs, const Vec3 end, int passEntityNum,
		int contentmask);
int	trap_PointContents(const Vec3 point, int passEntityNum);
qbool	trap_InPVS(const Vec3 p1, const Vec3 p2);
qbool	trap_InPVSIgnorePortals(const Vec3 p1, const Vec3 p2);
void	trap_AdjustAreaPortalState(Gentity *ent, qbool open);
qbool	trap_AreasConnected(int area1, int area2);
void	trap_LinkEntity(Gentity *ent);
void	trap_UnlinkEntity(Gentity *ent);
int	trap_EntitiesInBox(const Vec3 mins, const Vec3 maxs,
		int *entityList, int maxcount);
qbool	trap_EntityContact(const Vec3 mins, const Vec3 maxs,
		const Gentity *ent);
int	trap_BotAllocateClient(void);
void	trap_BotFreeClient(int clientNum);
void	trap_GetUsercmd(int clientNum, Usrcmd *cmd);
qbool	trap_GetEntityToken(char *buffer, int bufferSize);

int	trap_DebugPolygonCreate(int color, int numPoints, Vec3 *points);
void	trap_DebugPolygonDelete(int id);

int	trap_BotLibSetup(void);
int	trap_BotLibShutdown(void);
int	trap_BotLibVarSet(char *var_name, char *value);
int	trap_BotLibVarGet(char *var_name, char *value, int size);
int	trap_BotLibDefine(char *string);
int	trap_BotLibStartFrame(float time);
int	trap_BotLibLoadMap(const char *mapname);
int	trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int	trap_BotLibTest(int parm0, char *parm1, Vec3 parm2,
		Vec3 parm3);

int	trap_BotGetSnapshotEntity(int clientNum, int sequence);
int	trap_BotGetServerCommand(int clientNum, char *message, int size);
void	trap_BotUserCommand(int client, Usrcmd *ucmd);

int	trap_AAS_BBoxAreas(Vec3 absmins, Vec3 absmaxs, int *areas,
		int maxareas);
int	trap_AAS_AreaInfo(int areanum,
		void /* struct aas_areainfo_s */ *info);
void	trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int	trap_AAS_Initialized(void);
void	trap_AAS_PresenceTypeBoundingBox(int presencetype, Vec3 mins,
		Vec3 maxs);
float	trap_AAS_Time(void);

int	trap_AAS_PointAreaNum(Vec3 point);
int	trap_AAS_PointReachabilityAreaIndex(Vec3 point);
int	trap_AAS_TraceAreas(Vec3 start, Vec3 end, int *areas,
		Vec3 *points, int maxareas);

int	trap_AAS_PointContents(Vec3 point);
int	trap_AAS_NextBSPEntity(int ent);
int	trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value,
		int size);
int	trap_AAS_VectorForBSPEpairKey(int ent, char *key, Vec3 v);
int	trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int	trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int	trap_AAS_AreaReachability(int areanum);

int	trap_AAS_AreaTravelTimeToGoalArea(int areanum, Vec3 origin,
		int goalareanum, int travelflags);
int	trap_AAS_EnableRoutingArea(int areanum, int enable);
int	trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route,
		int areanum, Vec3 origin, int goalareanum,
		int travelflags, int maxareas, int maxtime,
		int stopevent, int stopcontents, int stoptfl,
		int stopareanum);

int	trap_AAS_AlternativeRouteGoals(Vec3 start, int startareanum, 
		Vec3 goal, int goalareanum, int travelflags,
		void /*struct aas_altroutegoal_s*/ *altroutegoals,
		int maxaltroutegoals, int type);
int	trap_AAS_Swimming(Vec3 origin);
int	trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move,
		int entnum, Vec3 origin, int presencetype, 
		int onground, Vec3 velocity, Vec3 cmdmove,
		int cmdframes, int maxframes, float frametime,
		int stopevent, int stopareanum, int visualize);

void	trap_EA_Say(int client, char *str);
void	trap_EA_SayTeam(int client, char *str);
void	trap_EA_Command(int client, char *command);

void	trap_EA_Action(int client, int action);
void	trap_EA_Gesture(int client);
void	trap_EA_Talk(int client);
void	trap_EA_Attack(int client);
void	trap_EA_Use(int client);
void	trap_EA_Respawn(int client);
void	trap_EA_Crouch(int client);
void	trap_EA_MoveUp(int client);
void	trap_EA_MoveDown(int client);
void	trap_EA_MoveForward(int client);
void	trap_EA_MoveBack(int client);
void	trap_EA_MoveLeft(int client);
void	trap_EA_MoveRight(int client);
void	trap_EA_SelectWeapon(int client, int weapon);
void	trap_EA_Jump(int client);
void	trap_EA_DelayedJump(int client);
void	trap_EA_Move(int client, Vec3 dir, float speed);
void	trap_EA_View(int client, Vec3 viewangles);

void	trap_EA_EndRegular(int client, float thinktime);
void	trap_EA_GetInput(int client, float thinktime,
		void /* struct bot_input_s */ *input);
void	trap_EA_ResetInput(int client);

int	trap_BotLoadCharacter(char *charfile, float skill);
void	trap_BotFreeCharacter(int character);
float	trap_Characteristic_Float(int character, int index);
float	trap_Characteristic_BFloat(int character, int index, float min,
		float max);
int	trap_Characteristic_Integer(int character, int index);
int	trap_Characteristic_BInteger(int character, int index, int min,
		int max);
void	trap_Characteristic_String(int character, int index, char *buf, int size);

int	trap_BotAllocChatState(void);
void	trap_BotFreeChatState(int handle);
void	trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void	trap_BotRemoveConsoleMessage(int chatstate, int handle);
int	trap_BotNextConsoleMessage(int chatstate, 
		void /* struct bot_consolemessage_s */ *cm);
int	trap_BotNumConsoleMessages(int chatstate);
void	trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0,
		char *var1, char *var2, char *var3, char *var4,
		char *var5, char *var6, char *var7);
int	trap_BotNumInitialChats(int chatstate, char *type);
int	trap_BotReplyChat(int chatstate, char *message, int mcontext,
		int vcontext, char *var0, char *var1,
		char *var2, char *var3, char *var4,
		char *var5, char *var6, char *var7);
int	trap_BotChatLength(int chatstate);
void	trap_BotEnterChat(int chatstate, int client, int sendto);
void	trap_BotGetChatMessage(int chatstate, char *buf, int size);
int	trap_StringContains(char *str1, char *str2, int casesensitive);
int	trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match,
		unsigned long int context);
void	trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable,
		char *buf, int size);
void	trap_UnifyWhiteSpaces(char *string);
void	trap_BotReplaceSynonyms(char *string, unsigned long int context);
int	trap_BotLoadChatFile(int chatstate, char *chatfile,
					char *chatname);
void	trap_BotSetChatGender(int chatstate, int gender);
void	trap_BotSetChatName(int chatstate, char *name, int client);
void	trap_BotResetGoalState(int goalstate);
void	trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void	trap_BotResetAvoidGoals(int goalstate);
void	trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void	trap_BotPopGoal(int goalstate);
void	trap_BotEmptyGoalStack(int goalstate);
void	trap_BotDumpAvoidGoals(int goalstate);
void	trap_BotDumpGoalStack(int goalstate);
void	trap_BotGoalName(int number, char *name, int size);
int	trap_BotGetTopGoal(int goalstate,
		void /* struct bot_goal_s */ *goal);
int	trap_BotGetSecondGoal(int goalstate,
		void /* struct bot_goal_s */ *goal);
int	trap_BotChooseLTGItem(int goalstate, Vec3 origin,
		int *inventory, int travelflags);
int	trap_BotChooseNBGItem(int goalstate, Vec3 origin,
		int *inventory, int travelflags,
		void /* struct bot_goal_s */ *ltg,
		float maxtime);
int	trap_BotTouchingGoal(Vec3 origin,
		void /* struct bot_goal_s */ *goal);
int	trap_BotItemGoalInVisButNotVisible(int viewer, Vec3 eye, 
		Vec3 viewangles, void /* struct bot_goal_s */ *goal);
int	trap_BotGetNextCampSpotGoal(int num,
		void /* struct bot_goal_s */ *goal);
int	trap_BotGetMapLocationGoal(char *name,
		void /* struct bot_goal_s */ *goal);
int	trap_BotGetLevelItemGoal(int index, char *classname,
		void /* struct bot_goal_s */ *goal);
float	trap_BotAvoidGoalTime(int goalstate, int number);
void	trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void	trap_BotInitLevelItems(void);
void	trap_BotUpdateEntityItems(void);
int	trap_BotLoadItemWeights(int goalstate, char *filename);
void	trap_BotFreeItemWeights(int goalstate);
void	trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void	trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void	trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int	trap_BotAllocGoalState(int state);
void	trap_BotFreeGoalState(int handle);

void	trap_BotResetMoveState(int movestate);
void	trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result,
		int movestate, void /* struct bot_goal_s */ *goal,
		int travelflags);
int	trap_BotMoveInDirection(int movestate, Vec3 dir, float speed,
					int	type);
void	trap_BotResetAvoidReach(int movestate);
void	trap_BotResetLastAvoidReach(int movestate);
int	trap_BotReachabilityArea(Vec3 origin, int testground);
int	trap_BotMovementViewTarget(int movestate,
		void /* struct bot_goal_s */ *goal,
		int travelflags, float lookahead,
		Vec3 target);
int	trap_BotPredictVisiblePosition(Vec3 origin, int areanum, 
		void /* struct bot_goal_s */ *goal,
		int travelflags, Vec3 target);
int	trap_BotAllocMoveState(void);
void	trap_BotFreeMoveState(int handle);
void	trap_BotInitMoveState(int handle,
		void /* struct bot_initmove_s */ *initmove);
void	trap_BotAddAvoidSpot(int movestate, Vec3 origin, float radius,
		int type);

int	trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void	trap_BotGetWeaponInfo(int weaponstate, int weapon,
		void /* struct weaponinfo_s */ *weaponinfo);
int	trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int	trap_BotAllocWeaponState(void);
void	trap_BotFreeWeaponState(int weaponstate);
void	trap_BotResetWeaponState(int weaponstate);

int	trap_GeneticParentsAndChildSelection(int numranks, float *ranks,
		int *parent1, int *parent2, int *child);

void	trap_snapv3(float *v);
