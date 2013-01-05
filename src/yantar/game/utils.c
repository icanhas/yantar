/* misc utility functions for game module */
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

typedef struct {
	char	oldShader[MAX_QPATH];
	char	newShader[MAX_QPATH];
	float	timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS 128

int remapCount = 0;
shaderRemap_t remappedShaders[MAX_SHADER_REMAPS];

void
AddRemap(const char *oldShader, const char *newShader, float timeOffset)
{
	int i;

	for(i = 0; i < remapCount; i++)
		if(Q_stricmp(oldShader, remappedShaders[i].oldShader) == 0){
			/* found it, just update this one */
			strcpy(remappedShaders[i].newShader,newShader);
			remappedShaders[i].timeOffset = timeOffset;
			return;
		}
	if(remapCount < MAX_SHADER_REMAPS){
		strcpy(remappedShaders[remapCount].newShader,newShader);
		strcpy(remappedShaders[remapCount].oldShader,oldShader);
		remappedShaders[remapCount].timeOffset = timeOffset;
		remapCount++;
	}
}

char *
BuildShaderStateConfig(void)
{
	static char buff[MAX_STRING_CHARS*4];
	char	out[(MAX_QPATH * 2) + 5];
	int	i;

	memset(buff, 0, MAX_STRING_CHARS);
	for(i = 0; i < remapCount; i++){
		Q_sprintf(out, (MAX_QPATH * 2) + 5, "%s=%s:%5.2f@",
			remappedShaders[i].oldShader,
			remappedShaders[i].newShader,
			remappedShaders[i].timeOffset);
		Q_strcat(buff, sizeof(buff), out);
	}
	return buff;
}

/*
 * model / sound configstring indexes
 */

int
G_FindConfigstringIndex(char *name, int start, int max, qbool create)
{
	int	i;
	char	s[MAX_STRING_CHARS];

	if(!name || !name[0])
		return 0;

	for(i=1; i<max; i++){
		trap_GetConfigstring(start + i, s, sizeof(s));
		if(!s[0])
			break;
		if(!strcmp(s, name))
			return i;
	}

	if(!create)
		return 0;

	if(i == max)
		G_Error("G_FindConfigstringIndex: overflow");

	trap_SetConfigstring(start + i, name);

	return i;
}


int
G_ModelIndex(char *name)
{
	return G_FindConfigstringIndex (name, CS_MODELS, MAX_MODELS, qtrue);
}

int
G_SoundIndex(char *name)
{
	return G_FindConfigstringIndex (name, CS_SOUNDS, MAX_SOUNDS, qtrue);
}

/*
 * Broadcasts a command to only a specific team
 */
void
G_TeamCommand(team_t team, char *cmd)
{
	int i;

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].pers.connected == CON_CONNECTED)
			if(level.clients[i].sess.sessionTeam == team)
				trap_SendServerCommand(i, va("%s", cmd));
}

/*
 * Searches all active entities for the next one that holds
 * the matching string at fieldofs (use the FOFS() macro) in the structure.
 *
 * Searches beginning at the entity after from, or the beginning if NULL
 * NULL will be returned if the end of the list is reached.
 *
 */
Gentity *
G_Find(Gentity *from, int fieldofs, const char *match)
{
	char *s;

	if(!from)
		from = g_entities;
	else
		from++;

	for(; from < &g_entities[level.num_entities]; from++){
		if(!from->inuse)
			continue;
		s = *(char**)((byte*)from + fieldofs);
		if(!s)
			continue;
		if(!Q_stricmp (s, match))
			return from;
	}

	return NULL;
}

/*
 * Selects a random entity from among the targets
 */
#define MAXCHOICES 32

Gentity *
G_PickTarget(char *targetname)
{
	Gentity	*ent = NULL;
	int num_choices = 0;
	Gentity	*choice[MAXCHOICES];

	if(!targetname){
		G_Printf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1){
		ent = G_Find (ent, FOFS(targetname), targetname);
		if(!ent)
			break;
		choice[num_choices++] = ent;
		if(num_choices == MAXCHOICES)
			break;
	}

	if(!num_choices){
		G_Printf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

/*
 * "activator" should be set to the entity that initiated the firing.
 *
 * Search for (string)targetname in all entities that
 * match (string)self.target and call their .use function
 */
void
G_UseTargets(Gentity *ent, Gentity *activator)
{
	Gentity *t;

	if(!ent)
		return;

	if(ent->targetShaderName && ent->targetShaderNewName){
		float f = level.time * 0.001;
		AddRemap(ent->targetShaderName, ent->targetShaderNewName, f);
		trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
	}

	if(!ent->target)
		return;

	t = NULL;
	while((t = G_Find (t, FOFS(targetname), ent->target)) != NULL){
		if(t == ent)
			G_Printf ("WARNING: Entity used itself.\n");
		else if(t->use)
			t->use (t, ent, activator);

		if(!ent->inuse){
			G_Printf("entity was removed while using targets\n");
			return;
		}
	}
}

/*
 * This is just a convenience function
 * for making temporary vectors for function calls
 */
float   *
tv(float x, float y, float z)
{
	static int index;
	static Vec3 vecs[8];
	float *v;

	/* use an array so that multiple tempvectors won't collide
	 * for a while */
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
 * This is just a convenience function
 * for printing vectors
 */
char    *
vtos(const Vec3 v)
{
	static int	index;
	static char	str[8][32];
	char *s;

	/* use an array so that multiple vtos won't collide */
	s = str[index];
	index = (index + 1)&7;

	Q_sprintf (s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}

/*
 * The editor only specifies a single value for angles (yaw),
 * but we have special constants to generate an up or down direction.
 * Angles will be cleared, because it is being used to represent a direction
 * instead of an orientation.
 */
void
G_SetMovedir(Vec3 angles, Vec3 movedir)
{
	static Vec3	VEC_UP = {0, -1, 0};
	static Vec3	MOVEDIR_UP	= {0, 0, 1};
	static Vec3	VEC_DOWN	= {0, -2, 0};
	static Vec3	MOVEDIR_DOWN	= {0, 0, -1};

	if(cmpv3 (angles, VEC_UP))
		copyv3 (MOVEDIR_UP, movedir);
	else if(cmpv3 (angles, VEC_DOWN))
		copyv3 (MOVEDIR_DOWN, movedir);
	else
		anglev3s (angles, movedir, NULL, NULL);
	clearv3(angles);
}

float
vectoyaw(const Vec3 vec)
{
	float yaw;

	if(vec[YAW] == 0 && vec[PITCH] == 0)
		yaw = 0;
	else{
		if(vec[PITCH])
			yaw = (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		else if(vec[YAW] > 0)
			yaw = 90;
		else
			yaw = 270;
		if(yaw < 0)
			yaw += 360;
	}

	return yaw;
}

void
G_InitGentity(Gentity *e)
{
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
}

/*
 * Either finds a free entity, or allocates a new one.
 *
 * The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
 * never be used by anything else.
 *
 * Try to avoid reusing an entity that was recently freed, because it
 * can cause the client to think the entity morphed into something else
 * instead of being removed and recreated, which can cause interpolated
 * angles and bad trails.
 */
Gentity *
G_Spawn(void)
{
	int i, force;
	Gentity *e;

	e = NULL;
	i = 0;
	for(force = 0; force < 2; force++){
		/* if we go through all entities and can't find one to free,
		 * override the normal minimum times before use */
		e = &g_entities[MAX_CLIENTS];
		for(i = MAX_CLIENTS; i<level.num_entities; i++, e++){
			if(e->inuse)
				continue;

			/* 
			 * the first couple seconds of server time can involve a lot of
			 * freeing and allocating, so relax the replacement policy 
			 */
			if(!force && e->freetime > level.startTime + 2000 &&
			   level.time - e->freetime < 1000)
			then
				continue;

			/* reuse this slot */
			G_InitGentity(e);
			return e;
		}
		if(i != MAX_GENTITIES)
			break;
	}
	if(i == ENTITYNUM_MAX_NORMAL){
		for(i = 0; i < MAX_GENTITIES; i++)
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		G_Error("G_Spawn: no free entities");
	}

	/* open up a new slot */
	level.num_entities++;

	/* let the server system know that there are more entities */
	trap_LocateGameData(level.gentities, level.num_entities,
		sizeof(Gentity),
		&level.clients[0].ps, sizeof(level.clients[0]));

	G_InitGentity(e);
	return e;
}

qbool
G_EntitiesFree(void)
{
	int i;
	Gentity *e;

	e = &g_entities[MAX_CLIENTS];
	for(i = MAX_CLIENTS; i < level.num_entities; i++, e++){
		if(e->inuse)
			continue;
		/* slot available */
		return qtrue;
	}
	return qfalse;
}

/*
 * Marks the entity as free
 */
void
G_FreeEntity(Gentity *ed)
{
	trap_UnlinkEntity (ed);	/* unlink from world */

	if(ed->neverFree)
		return;

	memset (ed, 0, sizeof(*ed));
	ed->classname	= "freed";
	ed->freetime	= level.time;
	ed->inuse = qfalse;
}

/*
 * Spawns an event entity that will be auto-removed
 * The origin will be snapped to save net bandwidth, so care
 * must be taken if the origin is right on a surface (snap towards start vector first)
 */
Gentity *
G_TempEntity(Vec3 origin, int event)
{
	Gentity	*e;
	Vec3		snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	copyv3(origin, snapped);
	snapv3(snapped);	/* save network bandwidth */
	G_SetOrigin(e, snapped);

	/* find cluster for PVS */
	trap_LinkEntity(e);

	return e;
}

/*
 * Kill box
 */

/*
 * Kills all entities that would touch the proposed new positioning
 * of ent.  Ent should be unlinked before calling this!
 */
void
G_KillBox(Gentity *ent)
{
	int	i, num;
	int	touch[MAX_GENTITIES];
	Gentity	*hit;
	Vec3		mins, maxs;

	addv3(ent->client->ps.origin, ent->r.mins, mins);
	addv3(ent->client->ps.origin, ent->r.maxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for(i=0; i<num; i++){
		hit = &g_entities[touch[i]];
		if(!hit->client)
			continue;

		/* nail it */
		G_Damage (hit, ent, ent, NULL, NULL,
			100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}

}

/*
 * Use for non-pmove events that would also be predicted on the
 * client side: jumppads and item pickups
 * Adds an event+parm and twiddles the event counter
 */
void
G_AddPredictableEvent(Gentity *ent, int event, int eventParm)
{
	if(!ent->client)
		return;
	BG_AddPredictableEventToPlayerstate(event, eventParm, &ent->client->ps);
}

/*
 * Adds an event+parm and twiddles the event counter
 */
void
G_AddEvent(Gentity *ent, int event, int eventParm)
{
	int bits;

	if(!event){
		G_Printf("G_AddEvent: zero event added for entity %i\n",
			ent->s.number);
		return;
	}

	/* clients need to add the event in Playerstate instead of Entstate */
	if(ent->client){
		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->client->ps.externalEvent = event | bits;
		ent->client->ps.externalEventParm = eventParm;
		ent->client->ps.externalEventTime = level.time;
	}else{
		bits = ent->s.event & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->s.event = event | bits;
		ent->s.eventParm = eventParm;
	}
	ent->eventTime = level.time;
}

void
G_Sound(Gentity *ent, int channel, int soundIndex)
{
	Gentity *te;

	te = G_TempEntity(ent->r.currentOrigin, EV_GENERAL_SOUND);
	te->s.eventParm = soundIndex;
}

/*
 * Sets the pos trajectory for a fixed position
 */
void
G_SetOrigin(Gentity *ent, Vec3 origin)
{
	copyv3(origin, ent->s.traj.base);
	ent->s.traj.type = TR_STATIONARY;
	ent->s.traj.time = 0;
	ent->s.traj.duration = 0;
	clearv3(ent->s.traj.delta);

	copyv3(origin, ent->r.currentOrigin);
}

/*
 * debug polygons only work when running a local game
 * with r_debugSurface set to 2
 */
int
DebugLine(Vec3 start, Vec3 end, int color)
{
	Vec3	points[4], dir, cross, up = {0, 0, 1};
	float	dot;

	copyv3(start, points[0]);
	copyv3(start, points[1]);
	/* points[1][2] -= 2; */
	copyv3(end, points[2]);
	/* points[2][2] -= 2; */
	copyv3(end, points[3]);


	subv3(end, start, dir);
	normv3(dir);
	dot = dotv3(dir, up);
	if(dot > 0.99 || dot < -0.99) setv3(cross, 1, 0, 0);
	else crossv3(dir, up, cross);

	normv3(cross);

	maddv3(points[0], 2, cross, points[0]);
	maddv3(points[1], -2, cross, points[1]);
	maddv3(points[2], -2, cross, points[2]);
	maddv3(points[3], 2, cross, points[3]);

	return trap_DebugPolygonCreate(color, 4, points);
}
