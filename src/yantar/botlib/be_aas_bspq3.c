/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 * BSP, Environment Sampling
 */

#include "shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern botlib_import_t botimport;

/* #define TRACE_DEBUG */
#define ON_EPSILON 0.005f
#define MAX_BSPENTITIES 2048

typedef struct rgb_s {
	int	red;
	int	green;
	int	blue;
} rgb_t;

/* bsp entity epair */
typedef struct bsp_epair_s {
	char *key;
	char *value;
	struct bsp_epair_s *next;
} bsp_epair_t;

/* bsp data entity */
typedef struct bsp_entity_s {
	bsp_epair_t *epairs;
} bsp_entity_t;

/* id Sofware BSP data */
typedef struct bsp_s {
	int		loaded;	/* true when bsp file is loaded */
	/* entity data */
	int		entdatasize;
	char		*dentdata;
	/* bsp entities */
	int		numentities;
	bsp_entity_t	entities[MAX_BSPENTITIES];
} bsp_t;

/* global bsp */
bsp_t bspworld;


#ifdef BSP_DEBUG
typedef struct cname_s {
	int	value;
	char	*name;
} cname_t;

cname_t contentnames[] =
{
	{CONTENTS_SOLID,"CONTENTS_SOLID"},
	{CONTENTS_WINDOW,"CONTENTS_WINDOW"},
	{CONTENTS_AUX,"CONTENTS_AUX"},
	{CONTENTS_LAVA,"CONTENTS_LAVA"},
	{CONTENTS_SLIME,"CONTENTS_SLIME"},
	{CONTENTS_WATER,"CONTENTS_WATER"},
	{CONTENTS_MIST,"CONTENTS_MIST"},
	{LAST_VISIBLE_CONTENTS,"LAST_VISIBLE_CONTENTS"},

	{CONTENTS_AREAPORTAL,"CONTENTS_AREAPORTAL"},
	{CONTENTS_PLAYERCLIP,"CONTENTS_PLAYERCLIP"},
	{CONTENTS_MONSTERCLIP,"CONTENTS_MONSTERCLIP"},
	{CONTENTS_CURRENT_0,"CONTENTS_CURRENT_0"},
	{CONTENTS_CURRENT_90,"CONTENTS_CURRENT_90"},
	{CONTENTS_CURRENT_180,"CONTENTS_CURRENT_180"},
	{CONTENTS_CURRENT_270,"CONTENTS_CURRENT_270"},
	{CONTENTS_CURRENT_UP,"CONTENTS_CURRENT_UP"},
	{CONTENTS_CURRENT_DOWN,"CONTENTS_CURRENT_DOWN"},
	{CONTENTS_ORIGIN,"CONTENTS_ORIGIN"},
	{CONTENTS_MONSTER,"CONTENTS_MONSTER"},
	{CONTENTS_DEADMONSTER,"CONTENTS_DEADMONSTER"},
	{CONTENTS_DETAIL,"CONTENTS_DETAIL"},
	{CONTENTS_TRANSLUCENT,"CONTENTS_TRANSLUCENT"},
	{CONTENTS_LADDER,"CONTENTS_LADDER"},
	{0, 0}
};

void
PrintContents(int contents)
{
	int i;

	for(i = 0; contentnames[i].value; i++)
		if(contents & contentnames[i].value)
			botimport.Print(PRT_MESSAGE, "%s\n",
				contentnames[i].name);
}

#endif	/* BSP_DEBUG */

/*
 * traces axial boxes of any size through the world
 */
bsp_Trace
AAS_Trace(Vec3 start, Vec3 mins, Vec3 maxs, Vec3 end, int passent,
	  int contentmask)
{
	bsp_Trace bsptrace;
	botimport.Trace(&bsptrace, start, mins, maxs, end, passent, contentmask);
	return bsptrace;
}

/*
 * returns the contents at the given point
 */
int
AAS_PointContents(Vec3 point)
{
	return botimport.PointContents(point);
}

qbool
AAS_EntityCollision(int entnum,
		    Vec3 start, Vec3 boxmins, Vec3 boxmaxs, Vec3 end,
		    int contentmask, bsp_Trace *trace)
{
	bsp_Trace enttrace;

	botimport.EntityTrace(&enttrace, start, boxmins, boxmaxs, end, entnum,
		contentmask);
	if(enttrace.fraction < trace->fraction){
		Q_Memcpy(trace, &enttrace, sizeof(bsp_Trace));
		return qtrue;
	}
	return qfalse;
}

/*
 * returns true if in Potentially Visible Set
 */
qbool
AAS_inPVS(Vec3 p1, Vec3 p2)
{
	return botimport.inPVS(p1, p2);
}

/*
 * returns true if in Potentially Hearable Set
 */
qbool
AAS_inPHS(Vec3 p1, Vec3 p2)
{
	return qtrue;
}

void
AAS_BSPModelMinsMaxsOrigin(int modelnum, Vec3 angles, Vec3 mins, Vec3 maxs,
			   Vec3 origin)
{
	botimport.BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
}

/*
 * unlinks the entity from all leaves
 */
void
AAS_UnlinkFromBSPLeaves(bsp_link_t *leaves)
{
}

bsp_link_t *
AAS_BSPLinkEntity(Vec3 absmins, Vec3 absmaxs, int entnum, int modelnum)
{
	return NULL;
}

int
AAS_BoxEntities(Vec3 absmins, Vec3 absmaxs, int *list, int maxcount)
{
	return 0;
}

int
AAS_NextBSPEntity(int ent)
{
	ent++;
	if(ent >= 1 && ent < bspworld.numentities) return ent;
	return 0;
}

int
AAS_BSPEntityInRange(int ent)
{
	if(ent <= 0 || ent >= bspworld.numentities){
		botimport.Print(PRT_MESSAGE, "bsp entity out of range\n");
		return qfalse;
	}
	return qtrue;
}

int
AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size)
{
	bsp_epair_t *epair;

	value[0] = '\0';
	if(!AAS_BSPEntityInRange(ent)) return qfalse;
	for(epair = bspworld.entities[ent].epairs; epair; epair = epair->next)
		if(!strcmp(epair->key, key)){
			strncpy(value, epair->value, size-1);
			value[size-1] = '\0';
			return qtrue;
		}
	return qfalse;
}

int
AAS_VectorForBSPEpairKey(int ent, char *key, Vec3 v)
{
	char buf[MAX_EPAIRKEY];
	double v1, v2, v3;

	clearv3(v);
	if(!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY)) return qfalse;
	/* scanf into doubles, then assign, so it is Scalar size independent */
	v1 = v2 = v3 = 0;
	sscanf(buf, "%lf %lf %lf", &v1, &v2, &v3);
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	return qtrue;
}

int
AAS_FloatForBSPEpairKey(int ent, char *key, float *value)
{
	char buf[MAX_EPAIRKEY];

	*value = 0;
	if(!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY)) return qfalse;
	*value = atof(buf);
	return qtrue;
}

int
AAS_IntForBSPEpairKey(int ent, char *key, int *value)
{
	char buf[MAX_EPAIRKEY];

	*value = 0;
	if(!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY)) return qfalse;
	*value = atoi(buf);
	return qtrue;
}

void
AAS_FreeBSPEntities(void)
{
	int i;
	bsp_entity_t *ent;
	bsp_epair_t *epair, *nextepair;

	for(i = 1; i < bspworld.numentities; i++){
		ent = &bspworld.entities[i];
		for(epair = ent->epairs; epair; epair = nextepair){
			nextepair = epair->next;
			if(epair->key) FreeMemory(epair->key);
			if(epair->value) FreeMemory(epair->value);
			FreeMemory(epair);
		}
	}
	bspworld.numentities = 0;
}

void
AAS_ParseBSPEntities(void)
{
	script_t	*script;
	token_t		token;
	bsp_entity_t *ent;
	bsp_epair_t *epair;

	script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize,
		"entdata");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES|SCFL_NOSTRINGESCAPECHARS);	/* SCFL_PRIMITIVE); */

	bspworld.numentities = 1;

	while(PS_ReadToken(script, &token)){
		if(strcmp(token.string, "{")){
			ScriptError(script, "invalid %s\n", token.string);
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}
		if(bspworld.numentities >= MAX_BSPENTITIES){
			botimport.Print(PRT_MESSAGE,
				"too many entities in BSP file\n");
			break;
		}
		ent = &bspworld.entities[bspworld.numentities];
		bspworld.numentities++;
		ent->epairs = NULL;
		while(PS_ReadToken(script, &token)){
			if(!strcmp(token.string, "}")) break;
			epair =
				(bsp_epair_t*)GetClearedHunkMemory(sizeof(
						bsp_epair_t));
			epair->next = ent->epairs;
			ent->epairs = epair;
			if(token.type != TT_STRING){
				ScriptError(script, "invalid %s\n", token.string);
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}
			StripDoubleQuotes(token.string);
			epair->key = (char*)GetHunkMemory(strlen(
					token.string) + 1);
			strcpy(epair->key, token.string);
			if(!PS_ExpectTokenType(script, TT_STRING, 0, &token)){
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}
			StripDoubleQuotes(token.string);
			epair->value =
				(char*)GetHunkMemory(strlen(token.string) + 1);
			strcpy(epair->value, token.string);
		}
		if(strcmp(token.string, "}")){
			ScriptError(script, "missing }\n");
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}
	}
	FreeScript(script);
}

int
AAS_BSPTraceLight(Vec3 start, Vec3 end, Vec3 endpos, int *red, int *green,
		  int *blue)
{
	return 0;
}

void
AAS_DumpBSPData(void)
{
	AAS_FreeBSPEntities();

	if(bspworld.dentdata) FreeMemory(bspworld.dentdata);
	bspworld.dentdata = NULL;
	bspworld.entdatasize = 0;
	bspworld.loaded = qfalse;
	Q_Memset(&bspworld, 0, sizeof(bspworld));
}

/*
 * load a bsp file
 */
int
AAS_LoadBSPFile(void)
{
	AAS_DumpBSPData();
	bspworld.entdatasize = strlen(botimport.BSPEntityData()) + 1;
	bspworld.dentdata = (char*)GetClearedHunkMemory(bspworld.entdatasize);
	Q_Memcpy(bspworld.dentdata,
		botimport.BSPEntityData(), bspworld.entdatasize);
	AAS_ParseBSPEntities();
	bspworld.loaded = qtrue;
	return BLERR_NOERROR;
}
