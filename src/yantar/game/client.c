/* client functions that don't happen every frame */
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
#include "team.h"

static Vec3	playerMins = {-15, -15, -24};
static Vec3	playerMaxs = {15, 15, 32};

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
 * potential spawning position for deathmatch games.
 * The first time a player enters the game, they will be at an 'initial' spot.
 * Targets will be fired when someone spawns in on them.
 * "nobots" will prevent bots from using this spot.
 * "nohumans" will prevent non-bots from using this spot.
 */
void
SP_info_player_deathmatch(Gentity *ent)
{
	int i;

	G_SpawnInt("nobots", "0", &i);
	if(i)
		ent->flags |= FL_NO_BOTS;
	G_SpawnInt("nohumans", "0", &i);
	if(i)
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
 * equivelant to info_player_deathmatch
 */
void
SP_info_player_start(Gentity *ent)
{
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
 * The intermission will be viewed from this point.  Target an info_notnull for the view direction.
 */
void
SP_info_player_intermission(Gentity *ent)
{
}

qbool
SpotWouldTelefrag(Gentity *spot)
{
	int	i, num;
	int	touch[MAX_GENTITIES];
	Gentity	*hit;
	Vec3		mins, maxs;

	addv3(spot->s.origin, playerMins, mins);
	addv3(spot->s.origin, playerMaxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for(i=0; i<num; i++){
		hit = &g_entities[touch[i]];
		/* if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) { */
		if(hit->client)
			return qtrue;

	}

	return qfalse;
}

/*
 * Find the spot that we DON'T want to use
 */
#define MAX_SPAWN_POINTS 128
Gentity *
SelectNearestDeathmatchSpawnPoint(Vec3 from)
{
	Gentity	*spot;
	Vec3		delta;
	float dist, nearestDist;
	Gentity *nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while((spot =
		       G_Find (spot, FOFS(classname),
			       "info_player_deathmatch")) != NULL){

		subv3(spot->s.origin, from, delta);
		dist = lenv3(delta);
		if(dist < nearestDist){
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

/*
 * go to a random point that doesn't telefrag
 */
#define MAX_SPAWN_POINTS 128
Gentity *
SelectRandomDeathmatchSpawnPoint(qbool isbot)
{
	Gentity *spot;
	int	count;
	int	selection;
	Gentity *spots[MAX_SPAWN_POINTS];

	count	= 0;
	spot	= NULL;

	while((spot =
		       G_Find (spot, FOFS(classname),
			       "info_player_deathmatch")) != NULL && count <
	      MAX_SPAWN_POINTS){
		if(SpotWouldTelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			/* spot is not for this human/bot player */
			continue;

		spots[count] = spot;
		count++;
	}

	if(!count)	/* no spots that won't telefrag */
		return G_Find(NULL, FOFS(classname), "info_player_deathmatch");

	selection = rand() % count;
	return spots[ selection ];
}

/*
 * Chooses a player start, deathmatch start, etc
 */
Gentity *
SelectRandomFurthestSpawnPoint(Vec3 avoidPoint, Vec3 origin, Vec3 angles,
			       qbool isbot)
{
	Gentity	*spot;
	Vec3		delta;
	float	dist;
	float	list_dist[MAX_SPAWN_POINTS];
	Gentity       *list_spot[MAX_SPAWN_POINTS];
	int	numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	while((spot =
		       G_Find (spot, FOFS(classname),
			       "info_player_deathmatch")) != NULL){
		if(SpotWouldTelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			/* spot is not for this human/bot player */
			continue;

		subv3(spot->s.origin, avoidPoint, delta);
		dist = lenv3(delta);

		for(i = 0; i < numSpots; i++)
			if(dist > list_dist[i]){
				if(numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;

				for(j = numSpots; j > i; j--){
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}

				list_dist[i] = dist;
				list_spot[i] = spot;

				numSpots++;
				break;
			}

		if(i >= numSpots && numSpots < MAX_SPAWN_POINTS){
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}

	if(!numSpots){
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

		if(!spot)
			G_Error("Couldn't find a spawn point");

		copyv3 (spot->s.origin, origin);
		origin[2] += 9;
		copyv3 (spot->s.angles, angles);
		return spot;
	}

	/* select a random spot from the spawn points furthest away */
	rnd = random() * (numSpots / 2);

	copyv3 (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	copyv3 (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
 * Chooses a player start, deathmatch start, etc
 */
Gentity *
SelectSpawnPoint(Vec3 avoidPoint, Vec3 origin, Vec3 angles, qbool isbot)
{
	return SelectRandomFurthestSpawnPoint(avoidPoint, origin, angles, isbot);

	/*
	 * Gentity	*spot;
	 * Gentity	*nearestSpot;
	 *
	 * nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );
	 *
	 * spot = SelectRandomDeathmatchSpawnPoint ( );
	 * if ( spot == nearestSpot ) {
	 *      // roll again if it would be real close to point of death
	 *      spot = SelectRandomDeathmatchSpawnPoint ( );
	 *      if ( spot == nearestSpot ) {
	 *              // last try
	 *              spot = SelectRandomDeathmatchSpawnPoint ( );
	 *      }
	 * }
	 *
	 * // find a single player start spot
	 * if (!spot) {
	 *      G_Error( "Couldn't find a spawn point" );
	 * }
	 *
	 * copyv3 (spot->s.origin, origin);
	 * origin[2] += 9;
	 * copyv3 (spot->s.angles, angles);
	 *
	 * return spot;
	 */
}

/*
 * Try to find a spawn point marked 'initial', otherwise
 * use normal spawn selection.
 */
Gentity *
SelectInitialSpawnPoint(Vec3 origin, Vec3 angles, qbool isbot)
{
	Gentity *spot;

	spot = NULL;

	while((spot =
		       G_Find (spot, FOFS(classname),
			       "info_player_deathmatch")) != NULL){
		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			continue;

		if((spot->spawnflags & 0x01))
			break;
	}

	if(!spot || SpotWouldTelefrag(spot))
		return SelectSpawnPoint(vec3_origin, origin, angles, isbot);

	copyv3 (spot->s.origin, origin);
	origin[2] += 9;
	copyv3 (spot->s.angles, angles);

	return spot;
}

Gentity *
SelectSpectatorSpawnPoint(Vec3 origin, Vec3 angles)
{
	FindIntermissionPoint();

	copyv3(level.intermission_origin, origin);
	copyv3(level.intermission_angle, angles);

	return NULL;
}

/*
 * BODYQUE
 */

void
InitBodyQue(void)
{
	int i;
	Gentity *ent;

	level.bodyQueIndex = 0;
	for(i=0; i<BODY_QUEUE_SIZE; i++){
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
 * After sitting around for some seconds, fall into the ground and dissapear
 */
void
BodySink(Gentity *ent)
{
	if(level.time - ent->timestamp > 6500){
		/* the body ques are never actually freed, they are just unlinked */
		trap_UnlinkEntity(ent);
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.traj.base[2] -= 1;
}

/*
 * A player is respawning, so make an entity that looks
 * just like the existing corpse to leave behind.
 */
void
CopyToBodyQue(Gentity *ent)
{
#ifdef MISSIONPACK
	Gentity *e;
	int i;
#endif
	Gentity *body;
	int contents;

	trap_UnlinkEntity (ent);

	/* if client is in a nodrop area, don't leave the body */
	contents = trap_PointContents(ent->s.origin, -1);
	if(contents & CONTENTS_NODROP)
		return;

	/* grab a body que and cycle to the next one */
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;	/* clear EF_TALK, etc */
#ifdef MISSIONPACK
	if(ent->s.eFlags & EF_KAMIKAZE){
		body->s.eFlags |= EF_KAMIKAZE;

		/* check if there is a kamikaze timer around for this owner */
		for(i = 0; i < MAX_GENTITIES; i++){
			e = &g_entities[i];
			if(!e->inuse)
				continue;
			if(e->activator != ent)
				continue;
			if(strcmp(e->classname, "kamikaze timer"))
				continue;
			e->activator = body;
			break;
		}
	}
#endif
	body->s.powerups = 0;	/* clear powerups */
	body->s.loopSound = 0;	/* clear lava burning */
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;	/* don't bounce */
	if(body->s.groundEntityNum == ENTITYNUM_NONE){
		body->s.traj.type = TR_GRAVITY;
		body->s.traj.time = level.time;
		copyv3(ent->client->ps.velocity, body->s.traj.delta);
	}else
		body->s.traj.type = TR_STATIONARY;
	body->s.event = 0;

	/* change the animation to the last-frame only, so the sequence
	 * doesn't repeat anew for the body */
	switch(body->s.legsAnim & ~ANIM_TOGGLEBIT){
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	copyv3 (ent->r.mins, body->r.mins);
	copyv3 (ent->r.maxs, body->r.maxs);
	copyv3 (ent->r.absmin, body->r.absmin);
	copyv3 (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	/* don't take more damage if already gibbed */
	if(ent->health <= GIB_HEALTH)
		body->takedamage = qfalse;
	else
		body->takedamage = qtrue;


	copyv3 (body->s.traj.base, body->r.currentOrigin);
	trap_LinkEntity (body);
}

void
SetClientViewAngle(Gentity *ent, Vec3 angle)
{
	int i;

	/* set the delta angle */
	for(i=0; i<3; i++){
		int cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle -
						  ent->client->pers.cmd.angles[i
						  ];
	}
	copyv3(angle, ent->s.angles);
	copyv3 (ent->s.angles, ent->client->ps.viewangles);
}

void
ClientRespawn(Gentity *ent)
{

	CopyToBodyQue (ent);
	ClientSpawn(ent);
}

/*
 * Returns number of players on a team
 */
Team
TeamCount(int ignoreClientNum, int team)
{
	int	i;
	int	count = 0;

	for(i = 0; i < level.maxclients; i++){
		if(i == ignoreClientNum)
			continue;
		if(level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if(level.clients[i].sess.sessionTeam == team)
			count++;
	}

	return count;
}

/*
 * Returns the client number of the team leader
 */
int
TeamLeader(int team)
{
	int i;

	for(i = 0; i < level.maxclients; i++){
		if(level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if(level.clients[i].sess.sessionTeam == team)
			if(level.clients[i].sess.teamLeader)
				return i;
	}

	return -1;
}

Team
PickTeam(int ignoreClientNum)
{
	int counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount(ignoreClientNum, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignoreClientNum, TEAM_RED);

	if(counts[TEAM_BLUE] > counts[TEAM_RED])
		return TEAM_RED;
	if(counts[TEAM_RED] > counts[TEAM_BLUE])
		return TEAM_BLUE;
	/* equal team count, so join the team with the lowest score */
	if(level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED])
		return TEAM_RED;
	return TEAM_BLUE;
}

/*
 * Forces a client's skin (for teamplay)
 */
/*
 * static void ForceClientSkin( gClient *client, char *model, const char *skin ) {
 *      char *p;
 *
 *      if ((p = strrchr(model, '/')) != 0) {
 * *p = 0;
 *      }
 *
 *      Q_strcat(model, MAX_QPATH, "/");
 *      Q_strcat(model, MAX_QPATH, skin);
 * }
 */

static void
ClientCleanName(const char *in, char *out, int outSize)
{
	int outpos = 0, colorlessLen = 0, spaces = 0;

	/* discard leading spaces */
	for(; *in == ' '; in++) ;

	for(; *in && outpos < outSize - 1; in++){
		out[outpos] = *in;

		if(*in == ' '){
			/* don't allow too many consecutive spaces */
			if(spaces > 2)
				continue;

			spaces++;
		}else if(outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE){
			if(Q_IsColorString(&out[outpos - 1])){
				colorlessLen--;

				if(ColorIndex(*in) == 0){
					/* Disallow color black in names to prevent players
					 * from getting advantage playing in front of black backgrounds */
					outpos--;
					continue;
				}
			}else{
				spaces = 0;
				colorlessLen++;
			}
		}else{
			spaces = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	/* don't allow empty names */
	if(*out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "UnnamedPlayer", outSize);
}

/*
 * Called from ClientConnect when the player first connects and
 * directly by the server system when the player updates a userinfo variable.
 *
 * The game can override any of the settings and call trap_SetUserinfo
 * if desired.
 */
void
ClientUserinfoChanged(int clientNum)
{
	Gentity	*ent;
	int teamTask, teamLeader, team, health;
	char		*s;
	char		model[MAX_QPATH];
	char		headModel[MAX_QPATH];
	char		oldname[MAX_STRING_CHARS];
	gClient	*client;
	char		c1[MAX_INFO_STRING];
	char		c2[MAX_INFO_STRING];
	char		redTeam[MAX_INFO_STRING];
	char		blueTeam[MAX_INFO_STRING];
	char		userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	/* check for malformed or illegal info strings */
	if(!Info_Validate(userinfo))
		strcpy (userinfo, "\\name\\badinfo");

	/* check for local client */
	s = Info_ValueForKey(userinfo, "ip");
	if(!strcmp(s, "localhost"))
		client->pers.localClient = qtrue;

	/* check the item prediction */
	s = Info_ValueForKey(userinfo, "cg_predictItems");
	if(!atoi(s))
		client->pers.predictItemPickup = qfalse;
	else
		client->pers.predictItemPickup = qtrue;

	/* set name */
	Q_strncpyz (oldname, client->pers.netname, sizeof(oldname));
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName(s, client->pers.netname, sizeof(client->pers.netname));

	if(client->sess.sessionTeam == TEAM_SPECTATOR)
		if(client->sess.spectatorState == SPECTATOR_SCOREBOARD)
			Q_strncpyz(client->pers.netname, "scoreboard",
				sizeof(client->pers.netname));

	if(client->pers.connected == CON_CONNECTED)
		if(strcmp(oldname, client->pers.netname))
			trap_SendServerCommand(-1,
				va("print \"%s" S_COLOR_WHITE
					" renamed to %s\n\"",
					oldname,
					client->pers.netname));

	/* set max health */
#ifdef MISSIONPACK
	if(client->ps.powerups[PW_GUARD])
		client->pers.maxHealth = 200;
	else{
		health = atoi(Info_ValueForKey(userinfo, "handicap"));
		client->pers.maxHealth = health;
		if(client->pers.maxHealth < 1 || client->pers.maxHealth > 100)
			client->pers.maxHealth = 100;
	}
#else
	health = atoi(Info_ValueForKey(userinfo, "handicap"));
	client->pers.maxHealth = health;
	if(client->pers.maxHealth < 1 || client->pers.maxHealth > 100)
		client->pers.maxHealth = 100;

#endif
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	/* set model */
	if(g_gametype.integer >= GT_TEAM){
		Q_strncpyz(model, Info_ValueForKey (userinfo,
				"team_model"), sizeof(model));
		Q_strncpyz(headModel,
			Info_ValueForKey (userinfo,
				"team_headmodel"), sizeof(headModel));
	}else{
		Q_strncpyz(model,
			Info_ValueForKey (userinfo, "model"), sizeof(model));
		Q_strncpyz(headModel, Info_ValueForKey (userinfo,
				"headmodel"), sizeof(headModel));
	}

	/* bots set their team a few frames later */
	if(g_gametype.integer >= GT_TEAM && g_entities[clientNum].r.svFlags &
	   SVF_BOT){
		s = Info_ValueForKey(userinfo, "team");
		if(!Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
			team = TEAM_RED;
		else if(!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
			team = TEAM_BLUE;
		else
			/* pick the team with the least number of players */
			team = PickTeam(clientNum);
	}else
		team = client->sess.sessionTeam;

/*	NOTE: all client side now
 *
 *      // team
 *      switch( team ) {
 *      case TEAM_RED:
 *              ForceClientSkin(client, model, "red");
 * //		ForceClientSkin(client, headModel, "red");
 *              break;
 *      case TEAM_BLUE:
 *              ForceClientSkin(client, model, "blue");
 * //		ForceClientSkin(client, headModel, "blue");
 *              break;
 *      }
 *      // don't ever use a default skin in teamplay, it would just waste memory
 *      // however bots will always join a team but they spawn in as spectator
 *      if ( g_gametype.integer >= GT_TEAM && team == TEAM_SPECTATOR) {
 *              ForceClientSkin(client, model, "red");
 * //		ForceClientSkin(client, headModel, "red");
 *      }
 */

#ifdef MISSIONPACK
	if(g_gametype.integer >= GT_TEAM)
		client->pers.teamInfo = qtrue;
	else{
		s = Info_ValueForKey(userinfo, "teamoverlay");
		if(!*s || atoi(s) != 0)
			client->pers.teamInfo = qtrue;
		else
			client->pers.teamInfo = qfalse;
	}
#else
	/* teamInfo */
	s = Info_ValueForKey(userinfo, "teamoverlay");
	if(!*s || atoi(s) != 0)
		client->pers.teamInfo = qtrue;
	else
		client->pers.teamInfo = qfalse;

#endif
	/*
	 * s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	 * if ( !*s || atoi( s ) == 0 ) {
	 *      client->pers.pmoveFixed = qfalse;
	 * }
	 * else {
	 *      client->pers.pmoveFixed = qtrue;
	 * }
	 */

	/* team task (0 = none, 1 = offence, 2 = defence) */
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	/* team Leader (1 = leader, 0 is normal player) */
	teamLeader = client->sess.teamLeader;

	/* colors */
	strcpy(c1, Info_ValueForKey(userinfo, "color1"));
	strcpy(c2, Info_ValueForKey(userinfo, "color2"));

	strcpy(redTeam, Info_ValueForKey(userinfo, "g_redteam"));
	strcpy(blueTeam, Info_ValueForKey(userinfo, "g_blueteam"));

	/* send over a subset of the userinfo keys so other clients can
	 * print scoreboards, display models, and play custom sounds */
	if(ent->r.svFlags & SVF_BOT)
		s = va(
			"n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
			client->pers.netname, team, model, headModel, c1, c2,
			client->pers.maxHealth, client->sess.wins,
			client->sess.losses,
			Info_ValueForKey(userinfo,
				"skill"), teamTask, teamLeader);
	else
		s = va(
			"n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\g_redteam\\%s\\g_blueteam\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d",
			client->pers.netname, client->sess.sessionTeam, model,
			headModel, redTeam, blueTeam, c1, c2,
			client->pers.maxHealth, client->sess.wins,
			client->sess.losses, teamTask, teamLeader);

	trap_SetConfigstring(CS_PLAYERS+clientNum, s);

	/* this is not the userinfo, more like the configstring actually */
	G_LogPrintf("ClientUserinfoChanged: %i %s\n", clientNum, s);
}

/*
 * Called when a player begins connecting to the server.
 * Called again for every map change or tournement restart.
 *
 * The session information will be valid after exit.
 *
 * Return NULL if the client should be allowed, otherwise return
 * a string with the reason for denial.
 *
 * Otherwise, the client will be sent the current gamestate
 * and will eventually get to ClientBegin.
 *
 * firstTime will be qtrue the very first time a client connects
 * to the server machine, but qfalse on map changes and tournement
 * restarts.
 */
char *
ClientConnect(int clientNum, qbool firstTime, qbool isBot)
{
	char *value;
/*	char		*areabits; */
	gClient	*client;
	char userinfo[MAX_INFO_STRING];
	Gentity	*ent;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	/* IP filtering
	 * https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
	 * recommanding PB based IP / GUID banning, the builtin system is pretty limited
	 * check to see if they are on the banned IP list */
	value = Info_ValueForKey (userinfo, "ip");
	if(G_FilterPacket(value))
		return "You are banned from this server.";

	/* we don't check password for bots and local client
	 * NOTE: local client <-> "ip" "localhost"
	 *   this means this client is not running in our current process */
	if(!isBot && (strcmp(value, "localhost") != 0)){
		/* check for a password */
		value = Info_ValueForKey (userinfo, "password");
		if(g_password.string[0] &&
		   Q_stricmp(g_password.string, "none") &&
		   strcmp(g_password.string, value) != 0)
			return "Invalid password";
	}

	/* they can connect */
	ent->client = level.clients + clientNum;
	client = ent->client;

/*	areabits = client->areabits; */

	memset(client, 0, sizeof(*client));

	client->pers.connected = CON_CONNECTING;

	/* read or initialize the session data */
	if(firstTime || level.newSession)
		G_InitSessionData(client, userinfo);
	G_ReadSessionData(client);

	if(isBot){
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if(!G_BotConnect(clientNum, !firstTime))
			return "BotConnectfailed";
	}

	/* get and distribute relevent paramters */
	G_LogPrintf("ClientConnect: %i\n", clientNum);
	ClientUserinfoChanged(clientNum);

	/* don't do the "xxx connected" messages if they were caried over from previous level */
	if(firstTime)
		trap_SendServerCommand(-1,
			va("print \"%s" S_COLOR_WHITE " connected\n\"",
				client->pers.netname));

	if(g_gametype.integer >= GT_TEAM &&
	   client->sess.sessionTeam != TEAM_SPECTATOR)
		BroadcastTeamChange(client, -1);

	/* count current clients and rank for scoreboard */
	CalculateRanks();

	/* for statistics */
/* client->areabits = areabits;
 * if ( !client->areabits )
 *      client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 ); */

	return NULL;
}

/*
 * called when a client has finished connecting, and is ready
 * to be placed into the level.  This will happen every level load,
 * and on transition between teams, but doesn't happen on respawns
 */
void
ClientBegin(int clientNum)
{
	Gentity	*ent;
	gClient	*client;
	int flags;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if(ent->r.linked)
		trap_UnlinkEntity(ent);
	G_InitGentity(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected	= CON_CONNECTED;
	client->pers.enterTime	= level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	/* save eflags around this, because changing teams will
	 * cause this to happen with a valid entity, and we
	 * want to make sure the teleport bit is set right
	 * so the viewpoint doesn't interpolate through the
	 * world to the new position */
	flags = client->ps.eFlags;
	memset(&client->ps, 0, sizeof(client->ps));
	client->ps.eFlags = flags;

	/* locate ent at a spawn point */
	ClientSpawn(ent);

	if(client->sess.sessionTeam != TEAM_SPECTATOR)
		if(g_gametype.integer != GT_TOURNAMENT)
			trap_SendServerCommand(-1,
				va("print \"%s" S_COLOR_WHITE
					" entered the game\n\"",
					client->pers.netname));
	G_LogPrintf("ClientBegin: %i\n", clientNum);

	/* count current clients and rank for scoreboard */
	CalculateRanks();
}

/*
 * Called every time a client is placed fresh in the world:
 * after the first ClientBegin, and after each respawn
 * Initializes all non-persistant parts of playerState
 */
void
ClientSpawn(Gentity *ent)
{
	int index;
	Vec3	spawn_origin, spawn_angles;
	gClient       *client;
	int	i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int persistant[MAX_PERSISTANT];
	Gentity		*spawnPoint;
	Gentity		*tent;
	int	flags;
	int	savedPing;
/*	char	*savedAreaBits; */
	int	accuracy_hits, accuracy_shots;
	int	eventSequence;
	char	userinfo[MAX_INFO_STRING];

	index	= ent - g_entities;
	client	= ent->client;

	clearv3(spawn_origin);

	/* find a spawn point
	 * do it before setting health back up, so farthest
	 * ranging doesn't count this client */
	if(client->sess.sessionTeam == TEAM_SPECTATOR)
		spawnPoint = SelectSpectatorSpawnPoint (
			spawn_origin, spawn_angles);
	else if(g_gametype.integer >= GT_CTF)
		/* all base oriented team games use the CTF spawn points */
		spawnPoint = SelectCTFSpawnPoint (
			client->sess.sessionTeam,
			client->pers.teamState.state,
			spawn_origin, spawn_angles,
			!!(ent->r.svFlags & SVF_BOT));
	else{
		/* the first spawn should be at a good looking spot */
		if(!client->pers.initialSpawn && client->pers.localClient){
			client->pers.initialSpawn = qtrue;
			spawnPoint = SelectInitialSpawnPoint(
				spawn_origin, spawn_angles,
				!!(ent->r.svFlags & SVF_BOT));
		}else
			/* don't spawn near existing origin if possible */
			spawnPoint = SelectSpawnPoint (
				client->ps.origin,
				spawn_origin, spawn_angles,
				!!(ent->r.svFlags & SVF_BOT));
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	/* always clear the kamikaze flag */
	ent->s.eFlags &= ~EF_KAMIKAZE;

	/* toggle the teleport bit so the client knows to not lerp
	 * and never clear the voted flag */
	flags = ent->client->ps.eFlags &
		(EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	/* clear everything but the persistant data */

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
/*	savedAreaBits = client->areabits; */
	accuracy_hits	= client->accuracy_hits;
	accuracy_shots	= client->accuracy_shots;
	for(i = 0; i < MAX_PERSISTANT; i++)
		persistant[i] = client->ps.persistant[i];
	eventSequence = client->ps.eventSequence;

	Q_Memset (client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
/*	client->areabits = savedAreaBits; */
	client->accuracy_hits	= accuracy_hits;
	client->accuracy_shots	= accuracy_shots;
	client->lastkilled_client = -1;

	for(i = 0; i < MAX_PERSISTANT; i++)
		client->ps.persistant[i] = persistant[i];
	client->ps.eventSequence = eventSequence;
	/* increment the spawncount so the client will detect the respawn */
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo(index, userinfo, sizeof(userinfo));
	/* set max health */
	client->pers.maxHealth = atoi(Info_ValueForKey(userinfo, "handicap"));
	if(client->pers.maxHealth < 1 || client->pers.maxHealth > 100)
		client->pers.maxHealth = 100;
	/* clear entity values */
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname	= "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask	= MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype	= 0;
	ent->flags = 0;

	copyv3 (playerMins, ent->r.mins);
	copyv3 (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	/* give the client the spawn weapons */
	client->ps.stats[STAT_PRIWEAPS] = (1<<W1machinegun);
	client->ps.stats[STAT_PRIWEAPS] |= (1<<W1melee);
	client->ps.stats[STAT_SECWEAPS] = (1<<W2rocketlauncher);
	client->ps.stats[STAT_HOOKWEAPS] = (1<<W1_GRAPPLING_HOOK);
	
	if(g_gametype.integer == GT_TEAM)
		client->ps.ammo[W1machinegun] = 50;
	else
		client->ps.ammo[W1machinegun] = 100;
	client->ps.ammo[W2rocketlauncher] = 10;
	client->ps.ammo[W1melee] = -1;
	client->ps.ammo[W1_GRAPPLING_HOOK] = -1;

	/* health will count down towards max_health */
	ent->health = client->ps.stats[STAT_HEALTH] =
			      client->ps.stats[STAT_MAX_HEALTH] + 25;

	G_SetOrigin(ent, spawn_origin);
	copyv3(spawn_origin, client->ps.origin);

	/* the respawned flag will be cleared after the attack and jump keys come up */
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd(client - level.clients, &ent->client->pers.cmd);
	SetClientViewAngle(ent, spawn_angles);
	/* don't allow full run speed for a bit */
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	/* set default animations */
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if(!level.intermissiontime){
		if(ent->client->sess.sessionTeam != TEAM_SPECTATOR){
			G_KillBox(ent);
			/* force the base weapon up */
			client->ps.weap[Wpri] = W1machinegun;
			client->ps.weap[Wsec] = W2rocketlauncher;
			client->ps.weap[Whookslot] = W1_GRAPPLING_HOOK;
			client->ps.weapstate[Wpri] = WEAPON_READY;
			client->ps.weapstate[Wsec] = WEAPON_READY;
			client->ps.weapstate[Whookslot] = WEAPON_READY;
			/* fire the targets of the spawn point */
			G_UseTargets(spawnPoint, ent);
			if(0){
			/* select the highest weapon number available, after any spawn given items have fired */
			client->ps.weap[Wpri] = 1;
			client->ps.weap[Wsec] = 1;

			for(i = Wnumweaps - 1; i > 0; i--){
				if(client->ps.stats[STAT_PRIWEAPS] & (1 << i)){
					client->ps.weap[Wpri] = i;
					break;
				}
				if(client->ps.stats[STAT_SECWEAPS] & (1 << i)){
					client->ps.weap[Wsec] = i;
					break;
				}
			}
			}
			/* positively link the client, even if the command times are weird */
			copyv3(ent->client->ps.origin, ent->r.currentOrigin);

			tent = G_TempEntity(ent->client->ps.origin,
				EV_PLAYER_TELEPORT_IN);
			tent->s.clientNum = ent->s.clientNum;

			trap_LinkEntity (ent);
		}
	}else
		/* move players to intermission */
		MoveClientToIntermission(ent);
	/* run a client frame to drop exactly to the floor,
	 * initialize animations and other things */
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink(ent-g_entities);
	/* run the presend to set anything else */
	ClientEndFrame(ent);

	/* clear entity state values */
	BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);
}

/*
 * Called when a player drops from the server.
 * Will not be called between levels.
 *
 * This should NOT be called directly by any game logic,
 * call trap_DropClient(), which will call this and do
 * server system housekeeping.
 */
void
ClientDisconnect(int clientNum)
{
	Gentity	*ent;
	Gentity	*tent;
	int i;

	/* cleanup if we are kicking a bot that
	 * hasn't spawned yet */
	G_RemoveQueuedBotBegin(clientNum);

	ent = g_entities + clientNum;
	if(!ent->client)
		return;

	/* stop any following clients */
	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
		   && level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
		   && level.clients[i].sess.spectatorClient == clientNum)
			StopFollowing(&g_entities[i]);

	/* send effect if they were completely connected */
	if(ent->client->pers.connected == CON_CONNECTED
	   && ent->client->sess.sessionTeam != TEAM_SPECTATOR){
		tent = G_TempEntity(ent->client->ps.origin,
			EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = ent->s.clientNum;

		/* They don't get to take powerups with them!
		 * Especially important for stuff like CTF flags */
		TossClientItems(ent);
#ifdef MISSIONPACK
		TossClientPersistantPowerups(ent);
		if(g_gametype.integer == GT_HARVESTER)
			TossClientCubes(ent);

#endif

	}

	G_LogPrintf("ClientDisconnect: %i\n", clientNum);

	/* if we are playing in tourney mode and losing, give a win to the other player */
	if((g_gametype.integer == GT_TOURNAMENT)
	   && !level.intermissiontime
	   && !level.warmupTime && level.sortedClients[1] == clientNum){
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged(level.sortedClients[0]);
	}

	if(g_gametype.integer == GT_TOURNAMENT &&
	   ent->client->sess.sessionTeam == TEAM_FREE &&
	   level.intermissiontime){

		trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if(ent->r.svFlags & SVF_BOT)
		BotAIShutdownClient(clientNum, qfalse);
}
