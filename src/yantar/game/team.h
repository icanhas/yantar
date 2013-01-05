/* "local.h" must be included before me */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*  */

#ifdef MISSIONPACK

#define CTF_CAPTURE_BONUS			100	/* what you get for capture */
#define CTF_TEAM_BONUS				25	/* what your team gets for capture */
#define CTF_RECOVERY_BONUS			10	/* what you get for recovery */
#define CTF_FLAG_BONUS				10	/* what you get for picking up enemy flag */
#define CTF_FRAG_CARRIER_BONUS			20	/* what you get for fragging enemy flag carrier */
#define CTF_FLAG_RETURN_TIME			40000	/* seconds until auto return */

#define CTF_CARRIER_DANGER_PROTECT_BONUS	5	/* bonus for fraggin someone who has recently hurt your flag carrier */
#define CTF_CARRIER_PROTECT_BONUS		2	/* bonus for fraggin someone while either you or your target are near your flag carrier */
#define CTF_FLAG_DEFENSE_BONUS			10	/* bonus for fraggin someone while either you or your target are near your flag */
#define CTF_RETURN_FLAG_ASSIST_BONUS		10	/* awarded for returning a flag that causes a capture to happen almost immediately */
#define CTF_FRAG_CARRIER_ASSIST_BONUS		10	/* award for fragging a flag carrier if a capture happens almost immediately */

#else

#define CTF_CAPTURE_BONUS			5	/* what you get for capture */
#define CTF_TEAM_BONUS				0	/* what your team gets for capture */
#define CTF_RECOVERY_BONUS			1	/* what you get for recovery */
#define CTF_FLAG_BONUS				0	/* what you get for picking up enemy flag */
#define CTF_FRAG_CARRIER_BONUS			2	/* what you get for fragging enemy flag carrier */
#define CTF_FLAG_RETURN_TIME			40000	/* seconds until auto return */

#define CTF_CARRIER_DANGER_PROTECT_BONUS	2	/* bonus for fraggin someone who has recently hurt your flag carrier */
#define CTF_CARRIER_PROTECT_BONUS		1	/* bonus for fraggin someone while either you or your target are near your flag carrier */
#define CTF_FLAG_DEFENSE_BONUS			1	/* bonus for fraggin someone while either you or your target are near your flag */
#define CTF_RETURN_FLAG_ASSIST_BONUS		1	/* awarded for returning a flag that causes a capture to happen almost immediately */
#define CTF_FRAG_CARRIER_ASSIST_BONUS		2	/* award for fragging a flag carrier if a capture happens almost immediately */

#endif

#define CTF_TARGET_PROTECT_RADIUS		1000	/* the radius around an object being defended where a target will be worth extra frags */
#define CTF_ATTACKER_PROTECT_RADIUS		1000	/* the radius around an object being defended where an attacker will get extra frags when making kills */

#define CTF_CARRIER_DANGER_PROTECT_TIMEOUT	8000
#define CTF_FRAG_CARRIER_ASSIST_TIMEOUT		10000
#define CTF_RETURN_FLAG_ASSIST_TIMEOUT		10000

#define CTF_GRAPPLE_SPEED			750	/* speed of grapple in flight */
#define CTF_GRAPPLE_PULL_SPEED			750	/* speed player is pulled at */

#define OVERLOAD_ATTACK_BASE_SOUND_TIME		20000

/* Prototypes */

int OtherTeam(int team);
const char*TeamName(int team);
const char*TeamColorString(int team);
void AddTeamScore(Vec3 origin, int team, int score);

void Team_DroppedFlagThink(Gentity *ent);
void Team_FragBonuses(Gentity *targ, Gentity *inflictor, Gentity *attacker);
void Team_CheckHurtCarrier(Gentity *targ, Gentity *attacker);
void Team_InitGame(void);
void Team_ReturnFlag(int team);
void Team_FreeEntity(Gentity *ent);
Gentity*SelectCTFSpawnPoint(team_t team, int teamstate, Vec3 origin,
			      Vec3 angles,
			      qbool isbot);
Gentity*Team_GetLocation(Gentity *ent);
qbool Team_GetLocationMsg(Gentity *ent, char *loc, int loclen);
void TeamplayInfoMessage(Gentity *ent);
void CheckTeamStatus(void);

int Pickup_Team(Gentity *ent, Gentity *other);
