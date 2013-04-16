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

/*
 * Session data is the only data that stays persistant across level loads
 * and tournament restarts.
 */

/*
 * Called on game shutdown
 */
void
G_WriteClientSessionData(Gclient *client)
{
	const char *s, *var;

	s = va("%i %i %i %i %i %i %i",
		client->sess.team,
		client->sess.specnum,
		client->sess.specstate,
		client->sess.specclient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamleader);
	var = va("session%i", (int)(client - level.clients));
	trap_cvarsetstr(var, s);
}

/*
 * Called on a reconnect
 */
void
G_ReadSessionData(Gclient *client)
{
	char s[MAX_STRING_CHARS];
	const char *var;
	int teamleader, specstate, team;

	var = va("session%i", (int)(client - level.clients));
	trap_cvargetstrbuf(var, s, sizeof(s));
	sscanf(s, "%i %i %i %i %i %i %i",
		&team,
		&client->sess.specnum,
		&specstate,
		&client->sess.specclient,
		&client->sess.wins,
		&client->sess.losses,
		&teamleader);
	client->sess.team = (Team)team;
	client->sess.specstate = (Spectatorstate)specstate;
	client->sess.teamleader = (qbool)teamleader;
}

/*
 * Called on a first-time connect
 */
void
G_InitSessionData(Gclient *client, char *userinfo)
{
	Clientsess *sess;
	const char *value;

	sess = &client->sess;

	/* initial team determination */
	if(g_gametype.integer >= GT_TEAM){
		if(g_teamAutoJoin.integer){
			sess->team = PickTeam(-1);
			BroadcastTeamChange(client, -1);
		}else
			/* always spawn as spectator in team games */
			sess->team = TEAM_SPECTATOR;
	}else{
		value = Info_ValueForKey(userinfo, "team");
		if(value[0] == 's')
			/* a willing spectator, not a waiting-in-line */
			sess->team = TEAM_SPECTATOR;
		else{
			switch(g_gametype.integer){
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if(g_maxGameClients.integer > 0 
				&& level.numNonSpectatorClients >= g_maxGameClients.integer)
				then
					sess->team = TEAM_SPECTATOR;
				else
					sess->team = TEAM_FREE;
				break;
			case GT_TOURNAMENT:
				/* if the game is full, go into a waiting mode */
				if(level.numNonSpectatorClients >= 2)
					sess->team = TEAM_SPECTATOR;
				else
					sess->team = TEAM_FREE;
				break;
			}
		}
	}
	sess->specstate = SPECTATOR_FREE;
	AddTournamentQueue(client);
	G_WriteClientSessionData(client);
}

void
G_InitWorldSession(void)
{
	char s[MAX_STRING_CHARS];
	int gt;

	trap_cvargetstrbuf("session", s, sizeof(s));
	gt = atoi(s);

	/* 
	 * if the gametype changed since the last session, don't use any
	 * client sessions 
	 */
	if(g_gametype.integer != gt){
		level.newSession = qtrue;
		G_Printf("Gametype changed, clearing session data.\n");
	}
}

void
G_WriteSessionData(void)
{
	int i;

	trap_cvarsetstr("session", va("%i", g_gametype.integer));

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].pers.connected == CON_CONNECTED)
			G_WriteClientSessionData(&level.clients[i]);
}
