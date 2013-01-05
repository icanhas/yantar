/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
/* setup the deathmatch AI */
void BotSetupDeathmatchAI(void);
/* shutdown the deathmatch AI */
void BotShutdownDeathmatchAI(void);
/* let the bot live within its deathmatch AI net */
void BotDeathmatchAI(bot_state_t *bs, float thinktime);
/* free waypoints */
void BotFreeWaypoints(bot_waypoint_t *wp);
/* choose a weapon */
void BotChooseWeapon(bot_state_t *bs);
/* setup movement stuff */
void BotSetupForMovement(bot_state_t *bs);
/* update the inventory */
void BotUpdateInventory(bot_state_t *bs);
/* update the inventory during battle */
void BotUpdateBattleInventory(bot_state_t *bs, int enemy);
/* use holdable items during battle */
void BotBattleUseItems(bot_state_t *bs);
/* return true if the bot is dead */
qbool BotIsDead(bot_state_t *bs);
/* returns true if the bot is in observer mode */
qbool BotIsObserver(bot_state_t *bs);
/* returns true if the bot is in the intermission */
qbool BotIntermission(bot_state_t *bs);
/* returns true if the bot is in lava or slime */
qbool BotInLavaOrSlime(bot_state_t *bs);
/* returns true if the entity is dead */
qbool EntityIsDead(aas_entityinfo_t *entinfo);
/* returns true if the entity is invisible */
qbool EntityIsInvisible(aas_entityinfo_t *entinfo);
/* returns true if the entity is shooting */
qbool EntityIsShooting(aas_entityinfo_t *entinfo);
/* set a user info key/value pair */
void BotSetUserInfo(bot_state_t *bs, char *key, char *value);
/* set the team status (offense, defense etc.) */
void BotSetTeamStatus(bot_state_t *bs);
/* returns the name of the client */
char*ClientName(int client, char *name, int size);
/* returns an simplyfied client name */
char*EasyClientName(int client, char *name, int size);
/* returns the skin used by the client */
char*ClientSkin(int client, char *skin, int size);
/* returns the appropriate synonym context for the current game type and situation */
int BotSynonymContext(bot_state_t *bs);
/* set last ordered task */
int BotSetLastOrderedTask(bot_state_t *bs);
/* selection of goals for teamplay */
void BotTeamGoals(bot_state_t *bs, int retreat);
/* returns the aggression of the bot in the range [0, 100] */
float BotAggression(bot_state_t *bs);
/* returns how bad the bot feels */
float BotFeelingBad(bot_state_t *bs);
/* returns true if the bot wants to retreat */
int BotWantsToRetreat(bot_state_t *bs);
/* returns true if the bot wants to chase */
int BotWantsToChase(bot_state_t *bs);
/* returns true if the bot wants to help */
int BotWantsToHelp(bot_state_t *bs);
/* returns true if the bot can and wants to rocketjump */
int BotCanAndWantsToRocketJump(bot_state_t *bs);
/* returns true if the bot has a persistant powerup and a weapon */
int BotHasPersistantPowerupAndWeapon(bot_state_t *bs);
/* returns true if the bot wants to and goes camping */
int BotWantsToCamp(bot_state_t *bs);
/* the bot will perform attack movements */
bot_moveresult_t BotAttackMove(bot_state_t *bs, int tfl);
/* returns true if the bot and the entity are in the same team */
int BotSameTeam(bot_state_t *bs, int entnum);
/* returns true if teamplay is on */
int TeamPlayIsOn(void);
/* returns the client number of the team mate flag carrier (-1 if none) */
int BotTeamFlagCarrier(bot_state_t *bs);
/* returns visible team mate flag carrier if available */
int BotTeamFlagCarrierVisible(bot_state_t *bs);
/* returns visible enemy flag carrier if available */
int BotEnemyFlagCarrierVisible(bot_state_t *bs);
/* get the number of visible teammates and enemies */
void BotVisibleTeamMatesAndEnemies(bot_state_t *bs, int *teammates, int *enemies,
				   float range);
/* returns true if within the field of vision for the given angles */
qbool InFieldOfVision(Vec3 viewangles, float fov, Vec3 angles);
/* returns true and sets the .enemy field when an enemy is found */
int BotFindEnemy(bot_state_t *bs, int curenemy);
/* returns a roam goal */
void BotRoamGoal(bot_state_t *bs, Vec3 goal);
/* returns entity visibility in the range [0, 1] */
float BotEntityVisible(int viewer, Vec3 eye, Vec3 viewangles, float fov,
		       int ent);
/* the bot will aim at the current enemy */
void BotAimAtEnemy(bot_state_t *bs);
/* check if the bot should attack */
void BotCheckAttack(bot_state_t *bs);
/* AI when the bot is blocked */
void BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate);
/* AI to predict obstacles */
int BotAIPredictObstacles(bot_state_t *bs, bot_goal_t *goal);
/* enable or disable the areas the blocking entity is in */
void BotEnableActivateGoalAreas(bot_activategoal_t *activategoal, int enable);
/* pop an activate goal from the stack */
int BotPopFromActivateGoalStack(bot_state_t *bs);
/* clear the activate goal stack */
void BotClearActivateGoalStack(bot_state_t *bs);
/* returns the team the bot is in */
int BotTeam(bot_state_t *bs);
/* retuns the opposite team of the bot */
int BotOppositeTeam(bot_state_t *bs);
/* returns the flag the bot is carrying (CTFFLAG_?) */
int BotCTFCarryingFlag(bot_state_t *bs);
/* remember the last ordered task */
void BotRememberLastOrderedTask(bot_state_t *bs);
/* set ctf goals (defend base, get enemy flag) during seek */
void BotCTFSeekGoals(bot_state_t *bs);
/* set ctf goals (defend base, get enemy flag) during retreat */
void BotCTFRetreatGoals(bot_state_t *bs);
int Bot1FCTFCarryingFlag(bot_state_t *bs);
void Bot1FCTFSeekGoals(bot_state_t *bs);
void Bot1FCTFRetreatGoals(bot_state_t *bs);
/* get a random alternate route goal towards the given base */
int BotGetAlternateRouteGoal(bot_state_t *bs, int base);
/* returns either the alternate route goal or the given goal */
bot_goal_t* BotAlternateRoute(bot_state_t *bs, bot_goal_t *goal);
/* create a new waypoint */
bot_waypoint_t*BotCreateWayPoint(char *name, Vec3 origin, int areanum);
/* find a waypoint with the given name */
bot_waypoint_t*BotFindWayPoint(bot_waypoint_t *waypoints, char *name);
/* strstr but case insensitive */
char* stristr(char *str, char *charset);
/* returns the number of the client with the given name */
int ClientFromName(char *name);
int ClientOnSameTeamFromName(bot_state_t *bs, char *name);
int BotPointAreaNum(Vec3 origin);
void BotMapScripts(bot_state_t *bs);

/* ctf flags */
#define CTF_FLAG_NONE		0
#define CTF_FLAG_RED		1
#define CTF_FLAG_BLUE		2
/* CTF skins */
#define CTF_SKIN_REDTEAM	"red"
#define CTF_SKIN_BLUETEAM	"blue"

extern int	gametype;	/* game type */
extern int	maxclients;	/* maximum number of clients */
extern Vmcvar bot_grapple;
extern Vmcvar bot_rocketjump;
extern Vmcvar bot_fastchat;
extern Vmcvar bot_nochat;
extern Vmcvar bot_testrchat;
extern Vmcvar bot_challenge;
extern bot_goal_t	ctf_redflag;
extern bot_goal_t	ctf_blueflag;
extern bot_goal_t	ctf_neutralflag;
extern bot_goal_t	neutralobelisk;	/* FIXME: remove? */
