/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "bg.h"
#include "game.h"
#include "local.h"

Gentity *podium1;
Gentity *podium2;
Gentity *podium3;


/*
 * UpdateTournamentInfo
 */
void
UpdateTournamentInfo(void)
{
	int	i;
	Gentity *player;
	int	playerClientNum;
	int	n, accuracy, perfect,   msglen;
#ifdef MISSIONPACK
	int	score1, score2;
	qbool won;
#endif
	char	buf[32];
	char	msg[MAX_STRING_CHARS];

	/* find the real player */
	player = NULL;
	for(i = 0; i < level.maxclients; i++){
		player = &g_entities[i];
		if(!player->inuse)
			continue;
		if(!(player->r.svFlags & SVF_BOT))
			break;
	}
	/* this should never happen! */
	if(!player || i == level.maxclients)
		return;
	playerClientNum = i;

	CalculateRanks();

	if(level.clients[playerClientNum].sess.team == TEAM_SPECTATOR){
#ifdef MISSIONPACK
		Q_sprintf(msg, sizeof(msg),
			"postgame %i %i 0 0 0 0 0 0 0 0 0 0 0",
			level.numNonSpectatorClients,
			playerClientNum);
#else
		Q_sprintf(msg, sizeof(msg), "postgame %i %i 0 0 0 0 0 0",
			level.numNonSpectatorClients,
			playerClientNum);
#endif
	}else{
		if(player->client->accuracy_shots)
			accuracy = player->client->accuracy_hits * 100 /
				   player->client->accuracy_shots;
		else
			accuracy = 0;

#ifdef MISSIONPACK
		won = qfalse;
		if(g_gametype.integer >= GT_CTF){
			score1	= level.teamScores[TEAM_RED];
			score2	= level.teamScores[TEAM_BLUE];
			if(level.clients[playerClientNum].sess.team
			   == TEAM_RED)
				won =
					(level.teamScores[TEAM_RED] >
					 level.teamScores[TEAM_BLUE]);
			else
				won =
					(level.teamScores[TEAM_BLUE] >
					 level.teamScores[TEAM_RED]);
		}else{
			if(&level.clients[playerClientNum] ==
			   &level.clients[ level.sortedClients[0] ]){
				won = qtrue;
				score1 =
					level.clients[ level.sortedClients[0] ].
					ps.
					persistant[PERS_SCORE];
				score2 =
					level.clients[ level.sortedClients[1] ].
					ps.
					persistant[PERS_SCORE];
			}else{
				score2 =
					level.clients[ level.sortedClients[0] ].
					ps.
					persistant[PERS_SCORE];
				score1 =
					level.clients[ level.sortedClients[1] ].
					ps.
					persistant[PERS_SCORE];
			}
		}
		if(won && player->client->ps.persistant[PERS_KILLED] == 0)
			perfect = 1;
		else
			perfect = 0;
		Q_sprintf(
			msg, sizeof(msg),
			"postgame %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			level.numNonSpectatorClients, playerClientNum, accuracy,
			player->client->ps.persistant[PERS_IMPRESSIVE_COUNT],
			player->client->ps.persistant[PERS_EXCELLENT_COUNT],
			player->client->ps.persistant[PERS_DEFEND_COUNT],
			player->client->ps.persistant[PERS_ASSIST_COUNT],
			player->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			player->client->ps.persistant[PERS_SCORE],
			perfect, score1, score2, level.time,
			player->client->ps.persistant[PERS_CAPTURES]);

#else
		perfect =
			(level.clients[playerClientNum].ps.persistant[PERS_RANK]
			 == 0 &&
			 player->client->ps.persistant[PERS_KILLED] ==
			 0) ? 1 : 0;
		Q_sprintf(
			msg, sizeof(msg), "postgame %i %i %i %i %i %i %i %i",
			level.numNonSpectatorClients, playerClientNum, accuracy,
			player->client->ps.persistant[PERS_IMPRESSIVE_COUNT],
			player->client->ps.persistant[PERS_EXCELLENT_COUNT],
			player->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			player->client->ps.persistant[PERS_SCORE],
			perfect);
#endif
	}

	msglen = strlen(msg);
	for(i = 0; i < level.numNonSpectatorClients; i++){
		n = level.sortedClients[i];
		Q_sprintf(buf, sizeof(buf), " %i %i %i", n,
			level.clients[n].ps.persistant[PERS_RANK],
			level.clients[n].ps.persistant[PERS_SCORE]);
		msglen += strlen(buf);
		if(msglen >= sizeof(msg))
			break;
		strcat(msg, buf);
	}
	trap_SendConsoleCommand(EXEC_APPEND, msg);
}


static Gentity *
SpawnModelOnVictoryPad(Gentity *pad, Vec3 offset, Gentity *ent, int place)
{
	Gentity	*body;
	Vec3		vec;
	Vec3		f, r, u;

	body = G_Spawn();
	if(!body){
		G_Printf(S_COLOR_RED "ERROR: out of gentities\n");
		return NULL;
	}

	body->classname = ent->client->pers.netname;
	body->client = ent->client;
	body->s = ent->s;
	body->s.eType		= ET_PLAYER;	/* could be ET_INVISIBLE */
	body->s.eFlags		= 0;		/* clear EF_TALK, etc */
	body->s.powerups	= 0;		/* clear powerups */
	body->s.loopSound	= 0;		/* clear lava burning */
	body->s.number		= body - g_entities;
	body->timestamp		= level.time;
	body->physicsObject	= qtrue;
	body->physicsBounce	= 0;	/* don't bounce */
	body->s.event		= 0;
	body->s.traj.type	= TR_STATIONARY;
	body->s.groundEntityNum = ENTITYNUM_WORLD;
	body->s.legsAnim	= LEGS_IDLE;
	body->s.torsoAnim	= TORSO_STAND;
	if(body->s.weap[WSpri] == Wnone)
		body->s.weap[WSpri] = Wmachinegun;
	if(body->s.weap[WSpri] == Wmelee)
		body->s.torsoAnim = TORSO_STAND2;
	body->s.event	= 0;
	body->r.svFlags = ent->r.svFlags;
	copyv3 (ent->r.mins, body->r.mins);
	copyv3 (ent->r.maxs, body->r.maxs);
	copyv3 (ent->r.absmin, body->r.absmin);
	copyv3 (ent->r.absmax, body->r.absmax);
	body->clipmask		= CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents	= CONTENTS_BODY;
	body->r.ownerNum	= ent->r.ownerNum;
	body->takedamage	= qfalse;

	subv3(level.intermission_origin, pad->r.currentOrigin, vec);
	v3toeuler(vec, body->s.apos.base);
	body->s.apos.base[PITCH]	= 0;
	body->s.apos.base[ROLL]	= 0;

	anglev3s(body->s.apos.base, f, r, u);
	saddv3(pad->r.currentOrigin, offset[0], f, vec);
	saddv3(vec, offset[1], r, vec);
	saddv3(vec, offset[2], u, vec);

	G_SetOrigin(body, vec);

	trap_LinkEntity (body);

	body->count = place;

	return body;
}


static void
CelebrateStop(Gentity *player)
{
	int anim;

	if(player->s.weap[WSpri] == Wmelee)
		anim = TORSO_STAND2;
	else
		anim = TORSO_STAND;
	player->s.torsoAnim =
		((player->s.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}


#define TIMER_GESTURE (34*66+50)
static void
CelebrateStart(Gentity *player)
{
	player->s.torsoAnim =
		((player->s.torsoAnim &
		  ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | TORSO_GESTURE;
	player->nextthink = level.time + TIMER_GESTURE;
	player->think = CelebrateStop;

	/*
	 * player->client->ps.events[player->client->ps.eventSequence & (MAX_PS_EVENTS-1)] = EV_TAUNT;
	 * player->client->ps.eventParms[player->client->ps.eventSequence & (MAX_PS_EVENTS-1)] = 0;
	 * player->client->ps.eventSequence++;
	 */
	G_AddEvent(player, EV_TAUNT, 0);
}


static Vec3	offsetFirst = {0, 0, 74};
static Vec3	offsetSecond = {-10, 60, 54};
static Vec3	offsetThird = {-19, -60, 45};

static void
PodiumPlacementThink(Gentity *podium)
{
	Vec3	vec;
	Vec3	origin;
	Vec3	f, r, u;

	podium->nextthink = level.time + 100;

	anglev3s(level.intermission_angle, vec, NULL, NULL);
	saddv3(level.intermission_origin,
		trap_cvargeti("g_podiumDist"), vec, origin);
	origin[2] -= trap_cvargeti("g_podiumDrop");
	G_SetOrigin(podium, origin);

	if(podium1){
		subv3(level.intermission_origin,
			podium->r.currentOrigin,
			vec);
		v3toeuler(vec, podium1->s.apos.base);
		podium1->s.apos.base[PITCH]	= 0;
		podium1->s.apos.base[ROLL]	= 0;

		anglev3s(podium1->s.apos.base, f, r, u);
		saddv3(podium->r.currentOrigin, offsetFirst[0], f, vec);
		saddv3(vec, offsetFirst[1], r, vec);
		saddv3(vec, offsetFirst[2], u, vec);

		G_SetOrigin(podium1, vec);
	}

	if(podium2){
		subv3(level.intermission_origin,
			podium->r.currentOrigin,
			vec);
		v3toeuler(vec, podium2->s.apos.base);
		podium2->s.apos.base[PITCH]	= 0;
		podium2->s.apos.base[ROLL]	= 0;

		anglev3s(podium2->s.apos.base, f, r, u);
		saddv3(podium->r.currentOrigin, offsetSecond[0], f, vec);
		saddv3(vec, offsetSecond[1], r, vec);
		saddv3(vec, offsetSecond[2], u, vec);

		G_SetOrigin(podium2, vec);
	}

	if(podium3){
		subv3(level.intermission_origin,
			podium->r.currentOrigin,
			vec);
		v3toeuler(vec, podium3->s.apos.base);
		podium3->s.apos.base[PITCH]	= 0;
		podium3->s.apos.base[ROLL]	= 0;

		anglev3s(podium3->s.apos.base, f, r, u);
		saddv3(podium->r.currentOrigin, offsetThird[0], f, vec);
		saddv3(vec, offsetThird[1], r, vec);
		saddv3(vec, offsetThird[2], u, vec);

		G_SetOrigin(podium3, vec);
	}
}


static Gentity *
SpawnPodium(void)
{
	Gentity	*podium;
	Vec3		vec;
	Vec3		origin;

	podium = G_Spawn();
	if(!podium)
		return NULL;

	podium->classname	= "podium";
	podium->s.eType		= ET_GENERAL;
	podium->s.number	= podium - g_entities;
	podium->clipmask	= CONTENTS_SOLID;
	podium->r.contents	= CONTENTS_SOLID;
	podium->s.modelindex	= G_ModelIndex(SP_PODIUM_MODEL);

	anglev3s(level.intermission_angle, vec, NULL, NULL);
	saddv3(level.intermission_origin,
		trap_cvargeti("g_podiumDist"), vec, origin);
	origin[2] -= trap_cvargeti("g_podiumDrop");
	G_SetOrigin(podium, origin);

	subv3(level.intermission_origin, podium->r.currentOrigin, vec);
	podium->s.apos.base[YAW] = vectoyaw(vec);
	trap_LinkEntity (podium);

	podium->think = PodiumPlacementThink;
	podium->nextthink = level.time + 100;
	return podium;
}


/*
 * SpawnModelsOnVictoryPads
 */
void
SpawnModelsOnVictoryPads(void)
{
	Gentity *player;
	Gentity *podium;

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	podium = SpawnPodium();

	player =
		SpawnModelOnVictoryPad(
			podium, offsetFirst,
			&g_entities[level.sortedClients[0]],
			level.clients[ level.sortedClients[0] ].ps.persistant[
				PERS_RANK]
			&~RANK_TIED_FLAG);
	if(player){
		player->nextthink = level.time + 2000;
		player->think = CelebrateStart;
		podium1 = player;
	}

	player =
		SpawnModelOnVictoryPad(
			podium, offsetSecond,
			&g_entities[level.sortedClients[1]],
			level.clients[ level.sortedClients[1] ].ps.persistant[
				PERS_RANK]
			&~RANK_TIED_FLAG);
	if(player)
		podium2 = player;

	if(level.numNonSpectatorClients > 2){
		player = SpawnModelOnVictoryPad(
			podium, offsetThird, &g_entities[level.sortedClients[2]],
			level.clients[ level.sortedClients[2] ].ps.persistant[
				PERS_RANK] &~RANK_TIED_FLAG);
		if(player)
			podium3 = player;
	}
}


/*
 * Svcmd_AbortPodium_f
 */
void
Svcmd_AbortPodium_f(void)
{
	if(g_gametype.integer != GT_SINGLE_PLAYER)
		return;

	if(podium1){
		podium1->nextthink = level.time;
		podium1->think = CelebrateStop;
	}
}
