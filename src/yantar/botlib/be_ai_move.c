/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		be_ai_move.c
*
* desc:		bot movement AI
*
* $Archive: /MissionPack/code/botlib/be_ai_move.c $
*
*****************************************************************************/

#include "shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"

#include "be_ea.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"


/* #define DEBUG_AI_MOVE
 * #define DEBUG_ELEVATOR
 * #define DEBUG_GRAPPLE */

/* movement state
 * NOTE: the moveflags MFL_ONGROUND, MFL_TELEPORTED, MFL_WATERJUMP and
 *      MFL_GRAPPLEPULL must be set outside the movement code */
typedef struct bot_movestate_s {
	/* input vars (all set outside the movement code) */
	Vec3		origin;		/* origin of the bot */
	Vec3		velocity;	/* velocity of the bot */
	Vec3		viewoffset;	/* view offset */
	int		entitynum;	/* entity number of the bot */
	int		client;		/* client number of the bot */
	float		thinktime;	/* time the bot thinks */
	int		presencetype;	/* presencetype of the bot */
	Vec3		viewangles;	/* view angles of the bot */
	/* state vars */
	int		areanum;				/* area the bot is in */
	int		lastareanum;				/* last area the bot was in */
	int		lastgoalareanum;			/* last goal area number */
	int		lastreachnum;				/* last reachability number */
	Vec3		lastorigin;				/* origin previous cycle */
	int		reachareanum;				/* area number of the reachabilty */
	int		moveflags;				/* movement flags */
	int		jumpreach;				/* set when jumped */
	float		grapplevisible_time;			/* last time the grapple was visible */
	float		lastgrappledist;			/* last distance to the grapple end */
	float		reachability_time;			/* time to use current reachability */
	int		avoidreach[MAX_AVOIDREACH];		/* reachabilities to avoid */
	float		avoidreachtimes[MAX_AVOIDREACH];	/* times to avoid the reachabilities */
	int		avoidreachtries[MAX_AVOIDREACH];	/* number of tries before avoiding */
	bot_avoidspot_t avoidspots[MAX_AVOIDSPOTS];	/* spots to avoid */
	int		numavoidspots;
} bot_movestate_t;

/* used to avoid reachability links for some time after being used */
#define AVOIDREACH
#define AVOIDREACH_TIME			6	/* avoid links for 6 seconds after use */
#define AVOIDREACH_TRIES		4
/* prediction times */
#define PREDICTIONTIME_JUMP		3	/* in seconds */
#define PREDICTIONTIME_MOVE		2	/* in seconds */
/* weapon indexes for weapon jumping */
#define WEAPONINDEX_ROCKET_LAUNCHER	5
#define WEAPONINDEX_BFG			9

#define MODELTYPE_FUNC_PLAT		1
#define MODELTYPE_FUNC_BOB		2
#define MODELTYPE_FUNC_DOOR		3
#define MODELTYPE_FUNC_STATIC		4

libvar_t *sv_maxstep;
libvar_t *sv_maxbarrier;
libvar_t *sv_gravity;
libvar_t *weapindex_rocketlauncher;
libvar_t *weapindex_bfg10k;
libvar_t *weapindex_grapple;
libvar_t *entitytypemissile;
libvar_t *offhandgrapple;
libvar_t *cmd_grappleoff;
libvar_t *cmd_grappleon;
/* type of model, func_plat or func_bobbing */
int modeltypes[MAX_MODELS];

bot_movestate_t *botmovestates[MAX_CLIENTS+1];

/* ========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * ======================================================================== */
int
BotAllocMoveState(void)
{
	int i;

	for(i = 1; i <= MAX_CLIENTS; i++)
		if(!botmovestates[i]){
			botmovestates[i] =
				GetClearedMemory(sizeof(bot_movestate_t));
			return i;
		}
	return 0;
}
/* ========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * ======================================================================== */
void
BotFreeMoveState(int handle)
{
	if(handle <= 0 || handle > MAX_CLIENTS){
		botimport.Print(PRT_FATAL, "move state handle %d out of range\n",
			handle);
		return;
	}
	if(!botmovestates[handle]){
		botimport.Print(PRT_FATAL, "invalid move state %d\n", handle);
		return;
	}
	FreeMemory(botmovestates[handle]);
	botmovestates[handle] = NULL;
}
/* ========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * ======================================================================== */
bot_movestate_t *
BotMoveStateFromHandle(int handle)
{
	if(handle <= 0 || handle > MAX_CLIENTS){
		botimport.Print(PRT_FATAL, "move state handle %d out of range\n",
			handle);
		return NULL;
	}
	if(!botmovestates[handle]){
		botimport.Print(PRT_FATAL, "invalid move state %d\n", handle);
		return NULL;
	}
	return botmovestates[handle];
}
/* ========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * ======================================================================== */
void
BotInitMoveState(int handle, bot_initmove_t *initmove)
{
	bot_movestate_t *ms;

	ms = BotMoveStateFromHandle(handle);
	if(!ms) return;
	copyv3(initmove->origin, ms->origin);
	copyv3(initmove->velocity, ms->velocity);
	copyv3(initmove->viewoffset, ms->viewoffset);
	ms->entitynum = initmove->entitynum;
	ms->client = initmove->client;
	ms->thinktime = initmove->thinktime;
	ms->presencetype = initmove->presencetype;
	copyv3(initmove->viewangles, ms->viewangles);
	ms->moveflags &= ~MFL_ONGROUND;
	if(initmove->or_moveflags & MFL_ONGROUND) ms->moveflags |= MFL_ONGROUND;
	ms->moveflags &= ~MFL_TELEPORTED;
	if(initmove->or_moveflags & MFL_TELEPORTED) ms->moveflags |=
			MFL_TELEPORTED;
	ms->moveflags &= ~MFL_WATERJUMP;
	if(initmove->or_moveflags & MFL_WATERJUMP) ms->moveflags |=
			MFL_WATERJUMP;
	ms->moveflags &= ~MFL_WALK;
	if(initmove->or_moveflags & MFL_WALK) ms->moveflags |= MFL_WALK;
	ms->moveflags &= ~MFL_GRAPPLEPULL;
	if(initmove->or_moveflags & MFL_GRAPPLEPULL) ms->moveflags |=
			MFL_GRAPPLEPULL;
}
/* ========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * ======================================================================== */
float
AngleDiff(float ang1, float ang2)
{
	float diff;

	diff = ang1 - ang2;
	if(ang1 > ang2){
		if(diff > 180.0) diff -= 360.0;
	}else if(diff < -180.0) diff += 360.0;
	return diff;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotFuzzyPointReachabilityArea(Vec3 origin)
{
	int	firstareanum, j, x, y, z;
	int	areas[10], numareas, areanum, bestareanum;
	float	dist, bestdist;
	Vec3	points[10], v, end;

	firstareanum = 0;
	areanum = AAS_PointAreaNum(origin);
	if(areanum){
		firstareanum = areanum;
		if(AAS_AreaReachability(areanum)) return areanum;
	}
	copyv3(origin, end);
	end[2] += 4;
	numareas = AAS_TraceAreas(origin, end, areas, points, 10);
	for(j = 0; j < numareas; j++)
		if(AAS_AreaReachability(areas[j])) return areas[j];
	bestdist = 999999;
	bestareanum = 0;
	for(z = 1; z >= -1; z -= 1){
		for(x = 1; x >= -1; x -= 1)
			for(y = 1; y >= -1; y -= 1){
				copyv3(origin, end);
				end[0] += x * 8;
				end[1] += y * 8;
				end[2] += z * 12;
				numareas =
					AAS_TraceAreas(origin, end, areas,
						points,
						10);
				for(j = 0; j < numareas; j++){
					if(AAS_AreaReachability(areas[j])){
						subv3(points[j], origin,
							v);
						dist = lenv3(v);
						if(dist < bestdist){
							bestareanum = areas[j];
							bestdist = dist;
						}
					}
					if(!firstareanum) firstareanum =
							areas[j];
				}
			}
		if(bestareanum) return bestareanum;
	}
	return firstareanum;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotReachabilityArea(Vec3 origin, int client)
{
	int modelnum, modeltype, reachnum, areanum;
	aas_reachability_t reach;
	Vec3 org, end, mins, maxs, up = {0, 0, 1};
	bsp_Trace	bsptrace;
	aas_Trace	trace;

	/* check if the bot is standing on something */
	AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, mins, maxs);
	maddv3(origin, -3, up, end);
	bsptrace = AAS_Trace(origin, mins, maxs, end, client,
		CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
	if(!bsptrace.startsolid && bsptrace.fraction < 1 && bsptrace.ent !=
	   ENTITYNUM_NONE){
		/* if standing on the world the bot should be in a valid area */
		if(bsptrace.ent == ENTITYNUM_WORLD)
			return BotFuzzyPointReachabilityArea(origin);

		modelnum = AAS_EntityModelindex(bsptrace.ent);
		modeltype = modeltypes[modelnum];

		/* if standing on a func_plat or func_bobbing then the bot is assumed to be
		 * in the area the reachability points to */
		if(modeltype == MODELTYPE_FUNC_PLAT || modeltype ==
		   MODELTYPE_FUNC_BOB){
			reachnum = AAS_NextModelReachability(0, modelnum);
			if(reachnum){
				AAS_ReachabilityFromNum(reachnum, &reach);
				return reach.areanum;
			}
		}

		/* if the bot is swimming the bot should be in a valid area */
		if(AAS_Swimming(origin))
			return BotFuzzyPointReachabilityArea(origin);
		areanum = BotFuzzyPointReachabilityArea(origin);
		/* if the bot is in an area with reachabilities */
		if(areanum && AAS_AreaReachability(areanum)) return areanum;
		/* trace down till the ground is hit because the bot is standing on some other entity */
		copyv3(origin, org);
		copyv3(org, end);
		end[2]	-= 800;
		trace	= AAS_TraceClientBBox(org, end, PRESENCE_CROUCH, -1);
		if(!trace.startsolid)
			copyv3(trace.endpos, org);
		return BotFuzzyPointReachabilityArea(org);
	}
	return BotFuzzyPointReachabilityArea(origin);
}
/* ===========================================================================
 * returns the reachability area the bot is in
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
/*
 * int BotReachabilityArea(Vec3 origin, int testground)
 * {
 *      int firstareanum, i, j, x, y, z;
 *      int areas[10], numareas, areanum, bestareanum;
 *      float dist, bestdist;
 *      Vec3 org, end, points[10], v;
 *      aas_Trace trace;
 *
 *      firstareanum = 0;
 *      for (i = 0; i < 2; i++)
 *      {
 *              copyv3(origin, org);
 *              //if test at the ground (used when bot is standing on an entity)
 *              if (i > 0)
 *              {
 *                      copyv3(origin, end);
 *                      end[2] -= 800;
 *                      trace = AAS_TraceClientBBox(origin, end, PRESENCE_CROUCH, -1);
 *                      if (!trace.startsolid)
 *                      {
 *                              copyv3(trace.endpos, org);
 *                      }
 *              }
 *
 *              firstareanum = 0;
 *              areanum = AAS_PointAreaNum(org);
 *              if (areanum)
 *              {
 *                      firstareanum = areanum;
 *                      if (AAS_AreaReachability(areanum)) return areanum;
 *              }
 *              bestdist = 999999;
 *              bestareanum = 0;
 *              for (z = 1; z >= -1; z -= 1)
 *              {
 *                      for (x = 1; x >= -1; x -= 1)
 *                      {
 *                              for (y = 1; y >= -1; y -= 1)
 *                              {
 *                                      copyv3(org, end);
 *                                      end[0] += x * 8;
 *                                      end[1] += y * 8;
 *                                      end[2] += z * 12;
 *                                      numareas = AAS_TraceAreas(org, end, areas, points, 10);
 *                                      for (j = 0; j < numareas; j++)
 *                                      {
 *                                              if (AAS_AreaReachability(areas[j]))
 *                                              {
 *                                                      subv3(points[j], org, v);
 *                                                      dist = lenv3(v);
 *                                                      if (dist < bestdist)
 *                                                      {
 *                                                              bestareanum = areas[j];
 *                                                              bestdist = dist;
 *                                                      }
 *                                              }
 *                                      }
 *                              }
 *                      }
 *                      if (bestareanum) return bestareanum;
 *              }
 *              if (!testground) break;
 *      }
 * //#ifdef DEBUG
 *      //botimport.Print(PRT_MESSAGE, "no reachability area\n");
 * //#endif //DEBUG
 *      return firstareanum;
 * } //end of the function BotReachabilityArea*/
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotOnMover(Vec3 origin, int entnum, aas_reachability_t *reach)
{
	int i, modelnum;
	Vec3	mins, maxs, modelorigin, org, end;
	Vec3	angles	= {0, 0, 0};
	Vec3	boxmins = {-16, -16, -8}, boxmaxs = {16, 16, 8};
	bsp_Trace trace;

	modelnum = reach->facenum & 0x0000FFFF;
	/* get some bsp model info */
	AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, NULL);
	if(!AAS_OriginOfMoverWithModelNum(modelnum, modelorigin)){
		botimport.Print(PRT_MESSAGE, "no entity with model %d\n",
			modelnum);
		return qfalse;
	}
	for(i = 0; i < 2; i++){
		if(origin[i] > modelorigin[i] + maxs[i] + 16) return qfalse;
		if(origin[i] < modelorigin[i] + mins[i] - 16) return qfalse;
	}
	copyv3(origin, org);
	org[2] += 24;
	copyv3(origin, end);
	end[2] -= 48;
	trace = AAS_Trace(org, boxmins, boxmaxs, end, entnum,
		CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
	if(!trace.startsolid && !trace.allsolid)
		/* NOTE: the reachability face number is the model number of the elevator */
		if(trace.ent != ENTITYNUM_NONE &&
		   AAS_EntityModelNum(trace.ent) == modelnum)
			return qtrue;
	return qfalse;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
MoverDown(aas_reachability_t *reach)
{
	int modelnum;
	Vec3	mins, maxs, origin;
	Vec3	angles = {0, 0, 0};

	modelnum = reach->facenum & 0x0000FFFF;
	/* get some bsp model info */
	AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
	if(!AAS_OriginOfMoverWithModelNum(modelnum, origin)){
		botimport.Print(PRT_MESSAGE, "no entity with model %d\n",
			modelnum);
		return qfalse;
	}
	/* if the top of the plat is below the reachability start point */
	if(origin[2] + maxs[2] < reach->start[2]) return qtrue;
	return qfalse;
}
/* ========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * ======================================================================== */
void
BotSetBrushModelTypes(void)
{
	int	ent, modelnum;
	char	classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];

	Q_Memset(modeltypes, 0, MAX_MODELS * sizeof(int));
	for(ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent)){
		if(!AAS_ValueForBSPEpairKey(ent, "classname", classname,
			   MAX_EPAIRKEY)) continue;
		if(!AAS_ValueForBSPEpairKey(ent, "model", model,
			   MAX_EPAIRKEY)) continue;
		if(model[0]) modelnum = atoi(model+1);
		else modelnum = 0;

		if(modelnum < 0 || modelnum > MAX_MODELS){
			botimport.Print(PRT_MESSAGE,
				"entity %s model number out of range\n",
				classname);
			continue;
		}

		if(!Q_stricmp(classname, "func_bobbing"))
			modeltypes[modelnum] = MODELTYPE_FUNC_BOB;
		else if(!Q_stricmp(classname, "func_plat"))
			modeltypes[modelnum] = MODELTYPE_FUNC_PLAT;
		else if(!Q_stricmp(classname, "func_door"))
			modeltypes[modelnum] = MODELTYPE_FUNC_DOOR;
		else if(!Q_stricmp(classname, "func_static"))
			modeltypes[modelnum] = MODELTYPE_FUNC_STATIC;
	}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotOnTopOfEntity(bot_movestate_t *ms)
{
	Vec3 mins, maxs, end, up = {0, 0, 1};
	bsp_Trace trace;

	AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
	maddv3(ms->origin, -3, up, end);
	trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum,
		CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
	if(!trace.startsolid &&
	   (trace.ent != ENTITYNUM_WORLD && trace.ent != ENTITYNUM_NONE))
		return trace.ent;
	return -1;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotValidTravel(Vec3 origin, aas_reachability_t *reach, int travelflags)
{
	/* if the reachability uses an unwanted travel type */
	if(AAS_TravelFlagForType(reach->traveltype) &
	   ~travelflags) return qfalse;
	/* don't go into areas with bad travel types */
	if(AAS_AreaContentsTravelFlags(reach->areanum) &
	   ~travelflags) return qfalse;
	return qtrue;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
void
BotAddToAvoidReach(bot_movestate_t *ms, int number, float avoidtime)
{
	int i;

	for(i = 0; i < MAX_AVOIDREACH; i++)
		if(ms->avoidreach[i] == number){
			if(ms->avoidreachtimes[i] >
			   AAS_Time()) ms->avoidreachtries[i]++;
			else ms->avoidreachtries[i] = 1;
			ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
			return;
		}
	/* end for
	 * add the reachability to the reachabilities to avoid for a while */
	for(i = 0; i < MAX_AVOIDREACH; i++)
		if(ms->avoidreachtimes[i] < AAS_Time()){
			ms->avoidreach[i] = number;
			ms->avoidreachtimes[i]	= AAS_Time() + avoidtime;
			ms->avoidreachtries[i]	= 1;
			return;
		}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
float
Vec3distv3FromLineSquared(Vec3 p, Vec3 lp1, Vec3 lp2)
{
	Vec3	proj, dir;
	int	j;

	AAS_ProjectPointOntoVector(p, lp1, lp2, proj);
	for(j = 0; j < 3; j++)
		if((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
		   (proj[j] < lp1[j] && proj[j] < lp2[j]))
			break;
	if(j < 3){
		if(fabs(proj[j] - lp1[j]) < fabs(proj[j] - lp2[j]))
			subv3(p, lp1, dir);
		else
			subv3(p, lp2, dir);
		return lensqrv3(dir);
	}
	subv3(p, proj, dir);
	return lensqrv3(dir);
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
float
VectorVec3distsqrv3(Vec3 p1, Vec3 p2)
{
	Vec3 dir;
	subv3(p2, p1, dir);
	return lensqrv3(dir);
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotAvoidSpots(Vec3 origin, aas_reachability_t *reach,
	      bot_avoidspot_t *avoidspots,
	      int numavoidspots)
{
	int checkbetween, i, type;
	float squareddist, squaredradius;

	switch(reach->traveltype & TRAVELTYPE_MASK){
	case TRAVEL_WALK: checkbetween = qtrue; break;
	case TRAVEL_CROUCH: checkbetween = qtrue; break;
	case TRAVEL_BARRIERJUMP: checkbetween = qtrue; break;
	case TRAVEL_LADDER: checkbetween = qtrue; break;
	case TRAVEL_WALKOFFLEDGE: checkbetween = qfalse; break;
	case TRAVEL_JUMP: checkbetween	= qfalse; break;
	case TRAVEL_SWIM: checkbetween	= qtrue; break;
	case TRAVEL_WATERJUMP: checkbetween = qtrue; break;
	case TRAVEL_TELEPORT: checkbetween = qfalse; break;
	case TRAVEL_ELEVATOR: checkbetween = qfalse; break;
	case TRAVEL_GRAPPLEHOOK: checkbetween	= qfalse; break;
	case TRAVEL_ROCKETJUMP: checkbetween	= qfalse; break;
	case TRAVEL_BFGJUMP: checkbetween	= qfalse; break;
	case TRAVEL_JUMPPAD: checkbetween	= qfalse; break;
	case TRAVEL_FUNCBOB: checkbetween	= qfalse; break;
	default: checkbetween = qtrue; break;
	}	/* end switch */

	type = AVOID_CLEAR;
	for(i = 0; i < numavoidspots; i++){
		squaredradius	= Square(avoidspots[i].radius);
		squareddist	=
			Vec3distv3FromLineSquared(avoidspots[i].origin, origin,
				reach->start);
		/* if moving towards the avoid spot */
		if(squareddist < squaredradius &&
		   VectorVec3distsqrv3(avoidspots[i].origin,
			   origin) > squareddist)
			type = avoidspots[i].type;
		else if(checkbetween){
			squareddist = Vec3distv3FromLineSquared(
				avoidspots[i].origin, reach->start, reach->end);
			/* if moving towards the avoid spot */
			if(squareddist < squaredradius &&
			   VectorVec3distsqrv3(avoidspots[i].origin,
				   reach->start) > squareddist)
				type = avoidspots[i].type;
		}else{
			VectorVec3distsqrv3(avoidspots[i].origin, reach->end);
			/* if the reachability leads closer to the avoid spot */
			if(squareddist < squaredradius &&
			   VectorVec3distsqrv3(avoidspots[i].origin,
				   reach->start) > squareddist)
				type = avoidspots[i].type;
		}
		if(type == AVOID_ALWAYS)
			return type;
	}
	return type;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
void
BotAddAvoidSpot(int movestate, Vec3 origin, float radius, int type)
{
	bot_movestate_t *ms;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return;
	if(type == AVOID_CLEAR){
		ms->numavoidspots = 0;
		return;
	}

	if(ms->numavoidspots >= MAX_AVOIDSPOTS)
		return;
	copyv3(origin, ms->avoidspots[ms->numavoidspots].origin);
	ms->avoidspots[ms->numavoidspots].radius = radius;
	ms->avoidspots[ms->numavoidspots].type = type;
	ms->numavoidspots++;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotGetReachabilityToGoal(Vec3 origin, int areanum,
			 int lastgoalareanum, int lastareanum,
			 int *avoidreach, float *avoidreachtimes,
			 int *avoidreachtries,
			 bot_goal_t *goal, int travelflags, int movetravelflags,
			 struct bot_avoidspot_s *avoidspots, int numavoidspots,
			 int *flags)
{
	int i, t, besttime, bestreachnum, reachnum;
	aas_reachability_t reach;

	/* if not in a valid area */
	if(!areanum) return 0;
	if(AAS_AreaDoNotEnter(areanum) || AAS_AreaDoNotEnter(goal->areanum)){
		travelflags |= TFL_DONOTENTER;
		movetravelflags |= TFL_DONOTENTER;
	}
	/* use the routing to find the next area to go to */
	besttime = 0;
	bestreachnum = 0;
	for(reachnum = AAS_NextAreaReachability(areanum, 0); reachnum;
	    reachnum = AAS_NextAreaReachability(areanum, reachnum)){
#ifdef AVOIDREACH
		/* check if it isn't an reachability to avoid */
		for(i = 0; i < MAX_AVOIDREACH; i++)
			if(avoidreach[i] == reachnum && avoidreachtimes[i] >=
			   AAS_Time()) break;
		if(i != MAX_AVOIDREACH && avoidreachtries[i] >
		   AVOIDREACH_TRIES){
#ifdef DEBUG
			if(botDeveloper)
				botimport.Print(PRT_MESSAGE,
					"avoiding reachability %d\n",
					avoidreach[i]);
#endif	/* DEBUG */
			continue;
		}
#endif	/* AVOIDREACH
	 * get the reachability from the number */
		AAS_ReachabilityFromNum(reachnum, &reach);
		/* NOTE: do not go back to the previous area if the goal didn't change
		 * NOTE: is this actually avoidance of local routing minima between two areas??? */
		if(lastgoalareanum == goal->areanum && reach.areanum ==
		   lastareanum) continue;
		/* if (AAS_AreaContentsTravelFlags(reach.areanum) & ~travelflags) continue;
		 * if the travel isn't valid */
		if(!BotValidTravel(origin, &reach, movetravelflags)) continue;
		/* get the travel time */
		t =
			AAS_AreaTravelTimeToGoalArea(reach.areanum, reach.end,
				goal->areanum,
				travelflags);
		/* if the goal area isn't reachable from the reachable area */
		if(!t) continue;
		/* if the bot should not use this reachability to avoid bad spots */
		if(BotAvoidSpots(origin, &reach, avoidspots, numavoidspots)){
			if(flags)
				*flags |= MOVERESULT_BLOCKEDBYAVOIDSPOT;
			continue;
		}
		/* add the travel time towards the area */
		t += reach.traveltime;	/* + AAS_AreaTravelTime(areanum, origin, reach.start); */
		/* if the travel time is better than the ones already found */
		if(!besttime || t < besttime){
			besttime = t;
			bestreachnum = reachnum;
		}
	}
	return bestreachnum;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
int
BotAddToTarget(Vec3 start, Vec3 end, float maxdist, float *dist,
	       Vec3 target)
{
	Vec3	dir;
	float	curdist;

	subv3(end, start, dir);
	curdist = normv3(dir);
	if(*dist + curdist < maxdist){
		copyv3(end, target);
		*dist += curdist;
		return qfalse;
	}else{
		maddv3(start, maxdist - *dist, dir, target);
		*dist = maxdist;
		return qtrue;
	}
}

int
BotMovementViewTarget(int movestate, bot_goal_t *goal, int travelflags,
		      float lookahead,
		      Vec3 target)
{
	aas_reachability_t reach;
	int reachnum, lastareanum;
	bot_movestate_t *ms;
	Vec3	end;
	float	dist;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return qfalse;
	/* if the bot has no goal or no last reachability */
	if(!ms->lastreachnum || !goal) return qfalse;

	reachnum = ms->lastreachnum;
	copyv3(ms->origin, end);
	lastareanum = ms->lastareanum;
	dist = 0;
	while(reachnum && dist < lookahead){
		AAS_ReachabilityFromNum(reachnum, &reach);
		if(BotAddToTarget(end, reach.start, lookahead, &dist,
			   target)) return qtrue;
		/* never look beyond teleporters */
		if((reach.traveltype & TRAVELTYPE_MASK) ==
		   TRAVEL_TELEPORT) return qtrue;
		/* never look beyond the weapon jump point */
		if((reach.traveltype & TRAVELTYPE_MASK) ==
		   TRAVEL_ROCKETJUMP) return qtrue;
		if((reach.traveltype & TRAVELTYPE_MASK) ==
		   TRAVEL_BFGJUMP) return qtrue;
		/* don't add jump pad distances */
		if((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_JUMPPAD &&
		   (reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_ELEVATOR &&
		   (reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_FUNCBOB)
			if(BotAddToTarget(reach.start, reach.end, lookahead,
				   &dist, target)) return qtrue;
		reachnum = BotGetReachabilityToGoal(reach.end, reach.areanum,
			ms->lastgoalareanum, lastareanum,
			ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries,
			goal, travelflags, travelflags, NULL, 0, NULL);
		copyv3(reach.end, end);
		lastareanum = reach.areanum;
		if(lastareanum == goal->areanum){
			BotAddToTarget(reach.end, goal->origin, lookahead, &dist,
				target);
			return qtrue;
		}
	}
	return qfalse;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotVisible(int ent, Vec3 eye, Vec3 target)
{
	bsp_Trace trace;

	trace = AAS_Trace(eye, NULL, NULL, target, ent,
		CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
	if(trace.fraction >= 1) return qtrue;
	return qfalse;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotPredictVisiblePosition(Vec3 origin, int areanum, bot_goal_t *goal,
			  int travelflags,
			  Vec3 target)
{
	aas_reachability_t reach;
	int	reachnum, lastgoalareanum, lastareanum, i;
	int	avoidreach[MAX_AVOIDREACH];
	float avoidreachtimes[MAX_AVOIDREACH];
	int	avoidreachtries[MAX_AVOIDREACH];
	Vec3 end;

	/* if the bot has no goal or no last reachability */
	if(!goal) return qfalse;
	/* if the areanum is not valid */
	if(!areanum) return qfalse;
	/* if the goal areanum is not valid */
	if(!goal->areanum) return qfalse;

	Q_Memset(avoidreach, 0, MAX_AVOIDREACH * sizeof(int));
	lastgoalareanum = goal->areanum;
	lastareanum = areanum;
	copyv3(origin, end);
	/* only do 20 hops */
	for(i = 0; i < 20 && (areanum != goal->areanum); i++){
		reachnum = BotGetReachabilityToGoal(end, areanum,
			lastgoalareanum, lastareanum,
			avoidreach, avoidreachtimes, avoidreachtries,
			goal, travelflags, travelflags, NULL, 0, NULL);
		if(!reachnum) return qfalse;
		AAS_ReachabilityFromNum(reachnum, &reach);
		if(BotVisible(goal->entitynum, goal->origin, reach.start)){
			copyv3(reach.start, target);
			return qtrue;
		}
		if(BotVisible(goal->entitynum, goal->origin, reach.end)){
			copyv3(reach.end, target);
			return qtrue;
		}
		if(reach.areanum == goal->areanum){
			copyv3(reach.end, target);
			return qtrue;
		}
		lastareanum = areanum;
		areanum = reach.areanum;
		copyv3(reach.end, end);
	}
	return qfalse;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
void
MoverBottomCenter(aas_reachability_t *reach, Vec3 bottomcenter)
{
	int modelnum;
	Vec3	mins, maxs, origin, mids;
	Vec3	angles = {0, 0, 0};

	modelnum = reach->facenum & 0x0000FFFF;
	/* get some bsp model info */
	AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
	if(!AAS_OriginOfMoverWithModelNum(modelnum, origin))
		botimport.Print(PRT_MESSAGE, "no entity with model %d\n",
			modelnum);
	/* get a point just above the plat in the bottom position */
	addv3(mins, maxs, mids);
	maddv3(origin, 0.5, mids, bottomcenter);
	bottomcenter[2] = reach->start[2];
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
float
BotGapVec3distv3(Vec3 origin, Vec3 hordir, int entnum)
{
	int dist;
	float	startz;
	Vec3	start, end;
	aas_Trace trace;

	/* do gap checking
	 * startz = origin[2];
	 * this enables walking down stairs more fluidly */
	{
		copyv3(origin, start);
		copyv3(origin, end);
		end[2]	-= 60;
		trace	= AAS_TraceClientBBox(start, end, PRESENCE_CROUCH,
			entnum);
		if(trace.fraction >= 1) return 1;
		startz = trace.endpos[2] + 1;
	}
	for(dist = 8; dist <= 100; dist += 8){
		maddv3(origin, dist, hordir, start);
		start[2] = startz + 24;
		copyv3(start, end);
		end[2]	-= 48 + sv_maxbarrier->value;
		trace	= AAS_TraceClientBBox(start, end, PRESENCE_CROUCH,
			entnum);
		/* if solid is found the bot can't walk any further and fall into a gap */
		if(!trace.startsolid){
			/* if it is a gap */
			if(trace.endpos[2] < startz - sv_maxstep->value - 8){
				copyv3(trace.endpos, end);
				end[2] -= 20;
				if(AAS_PointContents(end) &
				   CONTENTS_WATER) break;
				/* if a gap is found slow down
				 * botimport.Print(PRT_MESSAGE, "gap at %i\n", dist); */
				return dist;
			}
			startz = trace.endpos[2];
		}
	}
	return 0;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotCheckBarrierJump(bot_movestate_t *ms, Vec3 dir, float speed)
{
	Vec3 start, hordir, end;
	aas_Trace trace;

	copyv3(ms->origin, end);
	end[2] += sv_maxbarrier->value;
	/* trace right up */
	trace = AAS_TraceClientBBox(ms->origin, end, PRESENCE_NORMAL,
		ms->entitynum);
	/* this shouldn't happen... but we check anyway */
	if(trace.startsolid) return qfalse;
	/* if very low ceiling it isn't possible to jump up to a barrier */
	if(trace.endpos[2] - ms->origin[2] < sv_maxstep->value) return qfalse;
	hordir[0] = dir[0];
	hordir[1] = dir[1];
	hordir[2] = 0;
	normv3(hordir);
	maddv3(ms->origin, ms->thinktime * speed * 0.5, hordir, end);
	copyv3(trace.endpos, start);
	end[2] = trace.endpos[2];
	/* trace from previous trace end pos horizontally in the move direction */
	trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, ms->entitynum);
	/* again this shouldn't happen */
	if(trace.startsolid) return qfalse;
	copyv3(trace.endpos, start);
	copyv3(trace.endpos, end);
	end[2] = ms->origin[2];
	/* trace down from the previous trace end pos */
	trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, ms->entitynum);
	/* if solid */
	if(trace.startsolid) return qfalse;
	/* if no obstacle at all */
	if(trace.fraction >= 1.0) return qfalse;
	/* if less than the maximum step height */
	if(trace.endpos[2] - ms->origin[2] < sv_maxstep->value) return qfalse;
	EA_Jump(ms->client);
	EA_Move(ms->client, hordir, speed);
	ms->moveflags |= MFL_BARRIERJUMP;
	/* there is a barrier */
	return qtrue;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotSwimInDirection(bot_movestate_t *ms, Vec3 dir, float speed, int type)
{
	Vec3 normdir;

	copyv3(dir, normdir);
	normv3(normdir);
	EA_Move(ms->client, normdir, speed);
	return qtrue;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotWalkInDirection(bot_movestate_t *ms, Vec3 dir, float speed, int type)
{
	Vec3	hordir, cmdmove, velocity, tmpdir, origin;
	int	presencetype, maxframes, cmdframes, stopevent;
	aas_clientmove_t move;
	float	dist;

	if(AAS_OnGround(ms->origin, ms->presencetype,
		   ms->entitynum)) ms->moveflags |= MFL_ONGROUND;
	/* if the bot is on the ground */
	if(ms->moveflags & MFL_ONGROUND){
		/* if there is a barrier the bot can jump on */
		if(BotCheckBarrierJump(ms, dir, speed)) return qtrue;
		/* remove barrier jump flag */
		ms->moveflags &= ~MFL_BARRIERJUMP;
		/* get the presence type for the movement */
		if((type & MOVE_CROUCH) &&
		   !(type & MOVE_JUMP)) presencetype = PRESENCE_CROUCH;
		else presencetype = PRESENCE_NORMAL;
		/* horizontal direction */
		hordir[0] = dir[0];
		hordir[1] = dir[1];
		hordir[2] = 0;
		normv3(hordir);
		/* if the bot is not supposed to jump */
		if(!(type & MOVE_JUMP))
			/* if there is a gap, try to jump over it */
			if(BotGapVec3distv3(ms->origin, hordir,
				   ms->entitynum) > 0) type |= MOVE_JUMP;
		/* get command movement */
		scalev3(hordir, speed, cmdmove);
		copyv3(ms->velocity, velocity);
		if(type & MOVE_JUMP){
			/* botimport.Print(PRT_MESSAGE, "trying jump\n"); */
			cmdmove[2] = 400;
			maxframes = PREDICTIONTIME_JUMP / 0.1;
			cmdframes = 1;
			stopevent = SE_HITGROUND|SE_HITGROUNDDAMAGE|
				    SE_ENTERWATER|SE_ENTERSLIME|
				    SE_ENTERLAVA;
		}else{
			maxframes = 2;
			cmdframes = 2;
			stopevent = SE_HITGROUNDDAMAGE|
				    SE_ENTERWATER|SE_ENTERSLIME|
				    SE_ENTERLAVA;
		}
		/* AAS_ClearShownDebugLines(); */
		copyv3(ms->origin, origin);
		origin[2] += 0.5;
		AAS_PredictClientMovement(&move, ms->entitynum, origin,
			presencetype, qtrue,
			velocity, cmdmove, cmdframes, maxframes, 0.1f,
			stopevent, 0,
			qfalse);	/* qtrue); */
		/* if prediction time wasn't enough to fully predict the movement */
		if(move.frames >= maxframes && (type & MOVE_JUMP))
			/* botimport.Print(PRT_MESSAGE, "client %d: max prediction frames\n", ms->client); */
			return qfalse;
		/* don't enter slime or lava and don't fall from too high */
		if(move.stopevent &
		   (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
			/* botimport.Print(PRT_MESSAGE, "client %d: would be hurt ", ms->client);
			 * if (move.stopevent & SE_ENTERSLIME) botimport.Print(PRT_MESSAGE, "slime\n");
			 * if (move.stopevent & SE_ENTERLAVA) botimport.Print(PRT_MESSAGE, "lava\n");
			 * if (move.stopevent & SE_HITGROUNDDAMAGE) botimport.Print(PRT_MESSAGE, "hitground\n"); */
			return qfalse;
		/* if ground was hit */
		if(move.stopevent & SE_HITGROUND){
			/* check for nearby gap */
			norm2v3(move.velocity, tmpdir);
			dist = BotGapVec3distv3(move.endpos, tmpdir, ms->entitynum);
			if(dist > 0) return qfalse;
			dist = BotGapVec3distv3(move.endpos, hordir, ms->entitynum);
			if(dist > 0) return qfalse;
		}
		/* get horizontal movement */
		tmpdir[0] = move.endpos[0] - ms->origin[0];
		tmpdir[1] = move.endpos[1] - ms->origin[1];
		tmpdir[2] = 0;
		/*
		 * AAS_DrawCross(move.endpos, 4, LINECOLOR_BLUE);
		 * the bot is blocked by something */
		if(lenv3(tmpdir) < speed * ms->thinktime *
		   0.5) return qfalse;
		/* perform the movement */
		if(type & MOVE_JUMP) EA_Jump(ms->client);
		if(type & MOVE_CROUCH) EA_Crouch(ms->client);
		EA_Move(ms->client, hordir, speed);
		/* movement was succesfull */
		return qtrue;
	}else{
		if(ms->moveflags & MFL_BARRIERJUMP)
			/* if near the top or going down */
			if(ms->velocity[2] < 50)
				EA_Move(ms->client, dir, speed);
		/* FIXME: do air control to avoid hazards */
		return qtrue;
	}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotMoveInDirection(int movestate, Vec3 dir, float speed, int type)
{
	bot_movestate_t *ms;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return qfalse;
	/* if swimming */
	if(AAS_Swimming(ms->origin))
		return BotSwimInDirection(ms, dir, speed, type);
	else
		return BotWalkInDirection(ms, dir, speed, type);
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
Intersection(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2 out)
{
	float x1, dx1, dy1, x2, dx2, dy2, d;

	dx1 = p2[0] - p1[0];
	dy1 = p2[1] - p1[1];
	dx2 = p4[0] - p3[0];
	dy2 = p4[1] - p3[1];

	d = dy1 * dx2 - dx1 * dy2;
	if(d != 0){
		x1 = p1[1] * dx1 - p1[0] * dy1;
		x2 = p3[1] * dx2 - p3[0] * dy2;
		out[0]	= (int)((dx1 * x2 - dx2 * x1) / d);
		out[1]	= (int)((dy1 * x2 - dy2 * x1) / d);
		return qtrue;
	}else
		return qfalse;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
void
BotCheckBlocked(bot_movestate_t *ms, Vec3 dir, int checkbottom,
		bot_moveresult_t *result)
{
	Vec3 mins, maxs, end, up = {0, 0, 1};
	bsp_Trace trace;

	/* test for entities obstructing the bot's path */
	AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
	if(fabs(dotv3(dir, up)) < 0.7){
		mins[2] += sv_maxstep->value;	/* if the bot can step on */
		maxs[2] -= 10;			/* a little lower to avoid low ceiling */
	}
	maddv3(ms->origin, 3, dir, end);
	trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum,
		CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY);
	/* if not started in solid and not hitting the world entity */
	if(!trace.startsolid &&
	   (trace.ent != ENTITYNUM_WORLD && trace.ent != ENTITYNUM_NONE)){
		result->blocked = qtrue;
		result->blockentity = trace.ent;
#ifdef DEBUG
		/* botimport.Print(PRT_MESSAGE, "%d: BotCheckBlocked: I'm blocked\n", ms->client); */
#endif	/* DEBUG */
	}
	/* if not in an area with reachability */
	else if(checkbottom && !AAS_AreaReachability(ms->areanum)){
		/* check if the bot is standing on something */
		AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
		maddv3(ms->origin, -3, up, end);
		trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum,
			CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
		if(!trace.startsolid &&
		   (trace.ent != ENTITYNUM_WORLD && trace.ent !=
		    ENTITYNUM_NONE)){
			result->blocked = qtrue;
			result->blockentity = trace.ent;
			result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
#ifdef DEBUG
			/* botimport.Print(PRT_MESSAGE, "%d: BotCheckBlocked: I'm blocked\n", ms->client); */
#endif	/* DEBUG */
		}
	}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach)
{
	float	dist, speed;
	Vec3	hordir;
	bot_moveresult_t_cleared(result);

	/* first walk straight to the reachability start */
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	BotCheckBlocked(ms, hordir, qtrue, &result);
	if(dist < 10){
		/* walk straight to the reachability end */
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		dist = normv3(hordir);
	}
	/* if going towards a crouch area */
	if(!(AAS_AreaPresenceType(reach->areanum) & PRESENCE_NORMAL))
		/* if pretty close to the reachable area */
		if(dist < 20) EA_Crouch(ms->client);
	dist = BotGapVec3distv3(ms->origin, hordir, ms->entitynum);
	if(ms->moveflags & MFL_WALK){
		if(dist > 0) speed = 200 - (180 - 1 * dist);
		else speed = 200;
		EA_Walk(ms->client);
	}else{
		if(dist > 0) speed = 400 - (360 - 2 * dist);
		else speed = 400;
	}
	/* elemantary action move in direction */
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir;
	float	dist, speed;
	bot_moveresult_t_cleared(result);
	/* if not on the ground and changed areas... don't walk back!!
	 * (doesn't seem to help) */
	/*
	 * ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
	 * if (ms->areanum == reach->areanum)
	 * {
	 * #ifdef DEBUG
	 *      botimport.Print(PRT_MESSAGE, "BotFinishTravel_Walk: already in reach area\n");
	 * #endif //DEBUG
	 *      return result;
	 * } */
	/* go straight to the reachability end */
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	if(dist > 100) dist = 100;
	speed = 400 - (400 - 3 * dist);
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Crouch(bot_movestate_t *ms, aas_reachability_t *reach)
{
	float	speed;
	Vec3	hordir;
	bot_moveresult_t_cleared(result);

	speed = 400;
	/* walk straight to reachability end */
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	normv3(hordir);
	BotCheckBlocked(ms, hordir, qtrue, &result);
	/* elemantary actions */
	EA_Crouch(ms->client);
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	float	dist, speed;
	Vec3	hordir;
	bot_moveresult_t_cleared(result);

	/* walk straight to reachability start */
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	BotCheckBlocked(ms, hordir, qtrue, &result);
	/* if pretty close to the barrier */
	if(dist < 9)
		EA_Jump(ms->client);
	else{
		if(dist > 60) dist = 60;
		speed = 360 - (360 - 6 * dist);
		EA_Move(ms->client, hordir, speed);
	}
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3 hordir;
	bot_moveresult_t_cleared(result);

	/* if near the top or going down */
	if(ms->velocity[2] < 250){
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		BotCheckBlocked(ms, hordir, qtrue, &result);
		EA_Move(ms->client, hordir, 400);
		copyv3(hordir, result.movedir);
	}
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Swim(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3 dir;
	bot_moveresult_t_cleared(result);

	/* swim straight to reachability end */
	subv3(reach->start, ms->origin, dir);
	normv3(dir);
	BotCheckBlocked(ms, dir, qtrue, &result);
	/* elemantary actions */
	EA_Move(ms->client, dir, 400);
	copyv3(dir, result.movedir);
	Vector2Angles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_SWIMVIEW;
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	dir, hordir;
	float	dist;
	bot_moveresult_t_cleared(result);

	/* swim straight to reachability end */
	subv3(reach->end, ms->origin, dir);
	copyv3(dir, hordir);
	hordir[2] = 0;
	dir[2] += 15 + crandom() * 40;
	/* botimport.Print(PRT_MESSAGE, "BotTravel_WaterJump: dir[2] = %f\n", dir[2]); */
	normv3(dir);
	dist = normv3(hordir);
	/* elemantary actions
	 * EA_Move(ms->client, dir, 400); */
	EA_MoveForward(ms->client);
	/* move up if close to the actual out of water jump spot */
	if(dist < 40) EA_MoveUp(ms->client);
	/* set the ideal view angles */
	Vector2Angles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	copyv3(dir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3 dir, pnt;
	bot_moveresult_t_cleared(result);

	/* botimport.Print(PRT_MESSAGE, "BotFinishTravel_WaterJump\n");
	 * if waterjumping there's nothing to do */
	if(ms->moveflags & MFL_WATERJUMP) return result;
	/* if not touching any water anymore don't do anything
	 * otherwise the bot sometimes keeps jumping? */
	copyv3(ms->origin, pnt);
	pnt[2] -= 32;	/* extra for q2dm4 near red armor/mega health */
	if(!(AAS_PointContents(pnt) &
	     (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER))) return result;
	/* swim straight to reachability end */
	subv3(reach->end, ms->origin, dir);
	dir[0]	+= crandom() * 10;
	dir[1]	+= crandom() * 10;
	dir[2]	+= 70 + crandom() * 10;
	/* elemantary actions */
	EA_Move(ms->client, dir, 400);
	/* set the ideal view angles */
	Vector2Angles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	copyv3(dir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir, dir;
	float	dist, speed, reachhordist;
	bot_moveresult_t_cleared(result);

	/* check if the bot is blocked by anything */
	subv3(reach->start, ms->origin, dir);
	normv3(dir);
	BotCheckBlocked(ms, dir, qtrue, &result);
	/* if the reachability start and end are practially above each other */
	subv3(reach->end, reach->start, dir);
	dir[2] = 0;
	reachhordist = lenv3(dir);
	/* walk straight to the reachability start */
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	/* if pretty close to the start focus on the reachability end */
	if(dist < 48){
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		normv3(hordir);
		if(reachhordist < 20)
			speed = 100;
		else if(!AAS_HorizontalVelocityForJump(0, reach->start,
				reach->end, &speed))
			speed = 400;
	}else{
		if(reachhordist < 20){
			if(dist > 64) dist = 64;
			speed = 400 - (256 - 4 * dist);
		}else
			speed = 400;
	}
	BotCheckBlocked(ms, hordir, qtrue, &result);
	/* elemantary action */
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotAirControl(Vec3 origin, Vec3 velocity, Vec3 goal, Vec3 dir,
	      float *speed)
{
	Vec3	org, vel;
	float	dist;
	int	i;

	copyv3(origin, org);
	scalev3(velocity, 0.1, vel);
	for(i = 0; i < 50; i++){
		vel[2] -= sv_gravity->value * 0.01;
		/* if going down and next position would be below the goal */
		if(vel[2] < 0 && org[2] + vel[2] < goal[2]){
			scalev3(vel, (goal[2] - org[2]) / vel[2], vel);
			addv3(org, vel, org);
			subv3(goal, org, dir);
			dist = normv3(dir);
			if(dist > 32) dist = 32;
			*speed = 400 - (400 - 13 * dist);
			return qtrue;
		}else
			addv3(org, vel, org);
	}
	setv3(dir, 0, 0, 0);
	*speed = 400;
	return qfalse;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	dir, hordir, end, v;
	float	dist, speed;
	bot_moveresult_t_cleared(result);

	subv3(reach->end, ms->origin, dir);
	BotCheckBlocked(ms, dir, qtrue, &result);
	subv3(reach->end, ms->origin, v);
	v[2] = 0;
	dist = normv3(v);
	if(dist > 16) maddv3(reach->end, 16, v, end);
	else copyv3(reach->end, end);
	if(!BotAirControl(ms->origin, ms->velocity, end, hordir, &speed)){
		/* go straight to the reachability end */
		copyv3(dir, hordir);
		hordir[2] = 0;
		speed = 400;
	}
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
/*
 * bot_moveresult_t BotTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach)
 * {
 *      Vec3 hordir;
 *      float dist, gapdist, speed, horspeed, sv_jumpvel;
 *      bot_moveresult_t_cleared( result );
 *
 *      //
 *      sv_jumpvel = botlibglobals.sv_jumpvel->value;
 *      //walk straight to the reachability start
 *      hordir[0] = reach->start[0] - ms->origin[0];
 *      hordir[1] = reach->start[1] - ms->origin[1];
 *      hordir[2] = 0;
 *      dist = normv3(hordir);
 *      //
 *      speed = 350;
 *      //
 *      gapdist = BotGapVec3distv3(ms, hordir, ms->entitynum);
 *      //if pretty close to the start focus on the reachability end
 *      if (dist < 50 || (gapdist && gapdist < 50))
 *      {
 *              //NOTE: using max speed (400) works best
 *              //if (AAS_HorizontalVelocityForJump(sv_jumpvel, ms->origin, reach->end, &horspeed))
 *              //{
 *              //	speed = horspeed * 400 / botlibglobals.sv_maxwalkvelocity->value;
 *              //}
 *              hordir[0] = reach->end[0] - ms->origin[0];
 *              hordir[1] = reach->end[1] - ms->origin[1];
 *              normv3(hordir);
 *              //elemantary action jump
 *              EA_Jump(ms->client);
 *              //
 *              ms->jumpreach = ms->lastreachnum;
 *              speed = 600;
 *      }
 *      else
 *      {
 *              if (AAS_HorizontalVelocityForJump(sv_jumpvel, reach->start, reach->end, &horspeed))
 *              {
 *                      speed = horspeed * 400 / botlibglobals.sv_maxwalkvelocity->value;
 *              }
 *      }
 *      //elemantary action
 *      EA_Move(ms->client, hordir, speed);
 *      copyv3(hordir, result.movedir);
 *      //
 *      return result;
 * } //end of the function BotTravel_Jump*/
/*
 * bot_moveresult_t BotTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach)
 * {
 *      Vec3 hordir, dir1, dir2, mins, maxs, start, end;
 *      int gapdist;
 *      float dist1, dist2, speed;
 *      bot_moveresult_t_cleared( result );
 *      bsp_Trace trace;
 *
 *      //
 *      hordir[0] = reach->start[0] - reach->end[0];
 *      hordir[1] = reach->start[1] - reach->end[1];
 *      hordir[2] = 0;
 *      normv3(hordir);
 *      //
 *      copyv3(reach->start, start);
 *      start[2] += 1;
 *      //minus back the bouding box size plus 16
 *      maddv3(reach->start, 80, hordir, end);
 *      //
 *      AAS_PresenceTypeBoundingBox(PRESENCE_NORMAL, mins, maxs);
 *      //check for solids
 *      trace = AAS_Trace(start, mins, maxs, end, ms->entitynum, MASK_PLAYERSOLID);
 *      if (trace.startsolid) copyv3(start, trace.endpos);
 *      //check for a gap
 *      for (gapdist = 0; gapdist < 80; gapdist += 10)
 *      {
 *              maddv3(start, gapdist+10, hordir, end);
 *              end[2] += 1;
 *              if (AAS_PointAreaNum(end) != ms->reachareanum) break;
 *      }
 *      if (gapdist < 80) maddv3(reach->start, gapdist, hordir, trace.endpos);
 * //	dist1 = BotGapVec3distv3(start, hordir, ms->entitynum);
 * //	if (dist1 && dist1 <= trace.fraction * 80) maddv3(reach->start, dist1-20, hordir, trace.endpos);
 *      //
 *      subv3(ms->origin, reach->start, dir1);
 *      dir1[2] = 0;
 *      dist1 = normv3(dir1);
 *      subv3(ms->origin, trace.endpos, dir2);
 *      dir2[2] = 0;
 *      dist2 = normv3(dir2);
 *      //if just before the reachability start
 *      if (dotv3(dir1, dir2) < -0.8 || dist2 < 5)
 *      {
 *              //botimport.Print(PRT_MESSAGE, "between jump start and run to point\n");
 *              hordir[0] = reach->end[0] - ms->origin[0];
 *              hordir[1] = reach->end[1] - ms->origin[1];
 *              hordir[2] = 0;
 *              normv3(hordir);
 *              //elemantary action jump
 *              if (dist1 < 24) EA_Jump(ms->client);
 *              else if (dist1 < 32) EA_DelayedJump(ms->client);
 *              EA_Move(ms->client, hordir, 600);
 *              //
 *              ms->jumpreach = ms->lastreachnum;
 *      }
 *      else
 *      {
 *              //botimport.Print(PRT_MESSAGE, "going towards run to point\n");
 *              hordir[0] = trace.endpos[0] - ms->origin[0];
 *              hordir[1] = trace.endpos[1] - ms->origin[1];
 *              hordir[2] = 0;
 *              normv3(hordir);
 *              //
 *              if (dist2 > 80) dist2 = 80;
 *              speed = 400 - (400 - 5 * dist2);
 *              EA_Move(ms->client, hordir, speed);
 *      }
 *      copyv3(hordir, result.movedir);
 *      //
 *      return result;
 * } //end of the function BotTravel_Jump*/
/* * */
bot_moveresult_t
BotTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir, dir1, dir2, start, end, runstart;
/*	Vec3 runstart, dir1, dir2, hordir; */
	int	gapdist;
	float	dist1, dist2, speed;
	bot_moveresult_t_cleared(result);

	AAS_JumpReachRunStart(reach, runstart);
	/* * */
	hordir[0] = runstart[0] - reach->start[0];
	hordir[1] = runstart[1] - reach->start[1];
	hordir[2] = 0;
	normv3(hordir);
	copyv3(reach->start, start);
	start[2] += 1;
	maddv3(reach->start, 80, hordir, runstart);
	/* check for a gap */
	for(gapdist = 0; gapdist < 80; gapdist += 10){
		maddv3(start, gapdist+10, hordir, end);
		end[2] += 1;
		if(AAS_PointAreaNum(end) != ms->reachareanum) break;
	}
	if(gapdist < 80) maddv3(reach->start, gapdist, hordir, runstart);
	subv3(ms->origin, reach->start, dir1);
	dir1[2] = 0;
	dist1	= normv3(dir1);
	subv3(ms->origin, runstart, dir2);
	dir2[2] = 0;
	dist2	= normv3(dir2);
	/* if just before the reachability start */
	if(dotv3(dir1, dir2) < -0.8 || dist2 < 5){
/*		botimport.Print(PRT_MESSAGE, "between jump start and run start point\n"); */
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		normv3(hordir);
		/* elemantary action jump */
		if(dist1 < 24) EA_Jump(ms->client);
		else if(dist1 < 32) EA_DelayedJump(ms->client);
		EA_Move(ms->client, hordir, 600);
		ms->jumpreach = ms->lastreachnum;
	}else{
/*		botimport.Print(PRT_MESSAGE, "going towards run start point\n"); */
		hordir[0] = runstart[0] - ms->origin[0];
		hordir[1] = runstart[1] - ms->origin[1];
		hordir[2] = 0;
		normv3(hordir);
		if(dist2 > 80) dist2 = 80;
		speed = 400 - (400 - 5 * dist2);
		EA_Move(ms->client, hordir, speed);
	}
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir, hordir2;
	float	speed, dist;
	bot_moveresult_t_cleared(result);

	/* if not jumped yet */
	if(!ms->jumpreach) return result;
	/* go straight to the reachability end */
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	hordir2[0] = reach->end[0] - reach->start[0];
	hordir2[1] = reach->end[1] - reach->start[1];
	hordir2[2] = 0;
	normv3(hordir2);
	if(dotv3(hordir, hordir2) < -0.5 && dist < 24) return result;
	/* always use max speed when traveling through the air */
	speed = 800;
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Ladder(bot_movestate_t *ms, aas_reachability_t *reach)
{
	/* float dist, speed; */
	Vec3	dir, viewdir;	/* , hordir; */
	Vec3	origin = {0, 0, 0};
/*	Vec3 up = {0, 0, 1}; */
	bot_moveresult_t_cleared(result);

/*	if ((ms->moveflags & MFL_AGAINSTLADDER)) */
	/* NOTE: not a good idea for ladders starting in water
	 * || !(ms->moveflags & MFL_ONGROUND)) */
	{
		/* botimport.Print(PRT_MESSAGE, "against ladder or not on ground\n"); */
		subv3(reach->end, ms->origin, dir);
		normv3(dir);
		/* set the ideal view angles, facing the ladder up or down */
		viewdir[0] = dir[0];
		viewdir[1] = dir[1];
		viewdir[2] = 3 * dir[2];
		Vector2Angles(viewdir, result.ideal_viewangles);
		/* elemantary action */
		EA_Move(ms->client, origin, 0);
		EA_MoveForward(ms->client);
		/* set movement view flag so the AI can see the view is focussed */
		result.flags |= MOVERESULT_MOVEMENTVIEW;
	}
/*	else
 *      {
 *              //botimport.Print(PRT_MESSAGE, "moving towards ladder\n");
 *              subv3(reach->end, ms->origin, dir);
 *              //make sure the horizontal movement is large anough
 *              copyv3(dir, hordir);
 *              hordir[2] = 0;
 *              dist = normv3(hordir);
 *              //
 *              dir[0] = hordir[0];
 *              dir[1] = hordir[1];
 *              if (dir[2] > 0) dir[2] = 1;
 *              else dir[2] = -1;
 *              if (dist > 50) dist = 50;
 *              speed = 400 - (200 - 4 * dist);
 *              EA_Move(ms->client, dir, speed);
 *      } */
	/* save the movement direction */
	copyv3(dir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Teleport(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir;
	float	dist;
	bot_moveresult_t_cleared(result);

	/* if the bot is being teleported */
	if(ms->moveflags & MFL_TELEPORTED) return result;

	/* walk straight to center of the teleporter */
	subv3(reach->start, ms->origin, hordir);
	if(!(ms->moveflags & MFL_SWIMMING)) hordir[2] = 0;
	dist = normv3(hordir);
	BotCheckBlocked(ms, hordir, qtrue, &result);

	if(dist < 30) EA_Move(ms->client, hordir, 200);
	else EA_Move(ms->client, hordir, 400);

	if(ms->moveflags & MFL_SWIMMING) result.flags |= MOVERESULT_SWIMVIEW;

	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	dir, dir1, dir2, hordir, bottomcenter;
	float	dist, dist1, dist2, speed;
	bot_moveresult_t_cleared(result);

	/* if standing on the plat */
	if(BotOnMover(ms->origin, ms->entitynum, reach)){
#ifdef DEBUG_ELEVATOR
		botimport.Print(PRT_MESSAGE, "bot on elevator\n");
#endif	/* DEBUG_ELEVATOR
	 * if vertically not too far from the end point */
		if(abs(ms->origin[2] - reach->end[2]) < sv_maxbarrier->value){
#ifdef DEBUG_ELEVATOR
			botimport.Print(PRT_MESSAGE, "bot moving to end\n");
#endif	/* DEBUG_ELEVATOR
	 * move to the end point */
			subv3(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			normv3(hordir);
			if(!BotCheckBarrierJump(ms, hordir, 100))
				EA_Move(ms->client, hordir, 400);
			copyv3(hordir, result.movedir);
		}
		/* if not really close to the center of the elevator */
		else{
			MoverBottomCenter(reach, bottomcenter);
			subv3(bottomcenter, ms->origin, hordir);
			hordir[2] = 0;
			dist = normv3(hordir);
			if(dist > 10){
#ifdef DEBUG_ELEVATOR
				botimport.Print(PRT_MESSAGE,
					"bot moving to center\n");
#endif	/* DEBUG_ELEVATOR
	 * move to the center of the plat */
				if(dist > 100) dist = 100;
				speed = 400 - (400 - 4 * dist);
				EA_Move(ms->client, hordir, speed);
				copyv3(hordir, result.movedir);
			}
		}
	}else{
#ifdef DEBUG_ELEVATOR
		botimport.Print(PRT_MESSAGE, "bot not on elevator\n");
#endif	/* DEBUG_ELEVATOR
	 * if very near the reachability end */
		subv3(reach->end, ms->origin, dir);
		dist = lenv3(dir);
		if(dist < 64){
			if(dist > 60) dist = 60;
			speed = 360 - (360 - 6 * dist);
			if((ms->moveflags & MFL_SWIMMING) ||
			   !BotCheckBarrierJump(ms, dir, 50))
				if(speed > 5) EA_Move(ms->client, dir, speed);
			copyv3(dir, result.movedir);
			if(ms->moveflags & MFL_SWIMMING) result.flags |=
					MOVERESULT_SWIMVIEW;
			/* stop using this reachability */
			ms->reachability_time = 0;
			return result;
		}
		/* get direction and distance to reachability start */
		subv3(reach->start, ms->origin, dir1);
		if(!(ms->moveflags & MFL_SWIMMING)) dir1[2] = 0;
		dist1 = normv3(dir1);
		/* if the elevator isn't down */
		if(!MoverDown(reach)){
#ifdef DEBUG_ELEVATOR
			botimport.Print(PRT_MESSAGE, "elevator not down\n");
#endif	/* DEBUG_ELEVATOR */
			dist = dist1;
			copyv3(dir1, dir);
			BotCheckBlocked(ms, dir, qfalse, &result);
			if(dist > 60) dist = 60;
			speed = 360 - (360 - 6 * dist);
			if(!(ms->moveflags & MFL_SWIMMING) &&
			   !BotCheckBarrierJump(ms, dir, 50))
				if(speed > 5) EA_Move(ms->client, dir, speed);
			copyv3(dir, result.movedir);
			if(ms->moveflags & MFL_SWIMMING) result.flags |=
					MOVERESULT_SWIMVIEW;
			/* this isn't a failure... just wait till the elevator comes down */
			result.type = RESULTTYPE_ELEVATORUP;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}
		/* get direction and distance to elevator bottom center */
		MoverBottomCenter(reach, bottomcenter);
		subv3(bottomcenter, ms->origin, dir2);
		if(!(ms->moveflags & MFL_SWIMMING)) dir2[2] = 0;
		dist2 = normv3(dir2);
		/* if very close to the reachability start or
		 * closer to the elevator center or
		 * between reachability start and elevator center */
		if(dist1 < 20 || dist2 < dist1 || dotv3(dir1, dir2) < 0){
#ifdef DEBUG_ELEVATOR
			botimport.Print(PRT_MESSAGE, "bot moving to center\n");
#endif	/* DEBUG_ELEVATOR */
			dist = dist2;
			copyv3(dir2, dir);
		}else{	/* closer to the reachability start */
#ifdef DEBUG_ELEVATOR
			botimport.Print(PRT_MESSAGE, "bot moving to start\n");
#endif	/* DEBUG_ELEVATOR */
			dist = dist1;
			copyv3(dir1, dir);
		}
		BotCheckBlocked(ms, dir, qfalse, &result);
		if(dist > 60) dist = 60;
		speed = 400 - (400 - 6 * dist);
		if(!(ms->moveflags & MFL_SWIMMING) &&
		   !BotCheckBarrierJump(ms, dir, 50))
			EA_Move(ms->client, dir, speed);
		copyv3(dir, result.movedir);
		if(ms->moveflags & MFL_SWIMMING) result.flags |=
				MOVERESULT_SWIMVIEW;
	}
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3 bottomcenter, bottomdir, topdir;
	bot_moveresult_t_cleared(result);

	MoverBottomCenter(reach, bottomcenter);
	subv3(bottomcenter, ms->origin, bottomdir);
	subv3(reach->end, ms->origin, topdir);
	if(fabs(bottomdir[2]) < fabs(topdir[2])){
		normv3(bottomdir);
		EA_Move(ms->client, bottomdir, 300);
	}else{
		normv3(topdir);
		EA_Move(ms->client, topdir, 300);
	}
	return result;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
void
BotFuncBobStartEnd(aas_reachability_t *reach, Vec3 start, Vec3 end,
		   Vec3 origin)
{
	int	spawnflags, modelnum;
	Vec3 mins, maxs, mid, angles = {0, 0, 0};
	int	num0, num1;

	modelnum = reach->facenum & 0x0000FFFF;
	if(!AAS_OriginOfMoverWithModelNum(modelnum, origin)){
		botimport.Print(PRT_MESSAGE,
			"BotFuncBobStartEnd: no entity with model %d\n",
			modelnum);
		setv3(start, 0, 0, 0);
		setv3(end, 0, 0, 0);
		return;
	}
	AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, NULL);
	addv3(mins, maxs, mid);
	scalev3(mid, 0.5, mid);
	copyv3(mid, start);
	copyv3(mid, end);
	spawnflags = reach->facenum >> 16;
	num0 = reach->edgenum >> 16;
	if(num0 > 0x00007FFF) num0 |= 0xFFFF0000;
	num1 = reach->edgenum & 0x0000FFFF;
	if(num1 > 0x00007FFF) num1 |= 0xFFFF0000;
	if(spawnflags & 1){
		start[0] = num0;
		end[0] = num1;
		origin[0] += mid[0];
		origin[1] = mid[1];
		origin[2] = mid[2];
	}else if(spawnflags & 2){
		start[1] = num0;
		end[1] = num1;
		origin[0] = mid[0];
		origin[1] += mid[1];
		origin[2] = mid[2];
	}
	else{
		start[2] = num0;
		end[2] = num1;
		origin[0] = mid[0];
		origin[1] = mid[1];
		origin[2] += mid[2];
	}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_FuncBobbing(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	dir, dir1, dir2, hordir, bottomcenter, bob_start, bob_end,
		bob_origin;
	float	dist, dist1, dist2, speed;
	bot_moveresult_t_cleared(result);

	BotFuncBobStartEnd(reach, bob_start, bob_end, bob_origin);
	/* if standing ontop of the func_bobbing */
	if(BotOnMover(ms->origin, ms->entitynum, reach)){
#ifdef DEBUG_FUNCBOB
		botimport.Print(PRT_MESSAGE, "bot on func_bobbing\n");
#endif
		/* if near end point of reachability */
		subv3(bob_origin, bob_end, dir);
		if(lenv3(dir) < 24){
#ifdef DEBUG_FUNCBOB
			botimport.Print(PRT_MESSAGE,
				"bot moving to reachability end\n");
#endif
			/* move to the end point */
			subv3(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			normv3(hordir);
			if(!BotCheckBarrierJump(ms, hordir, 100))
				EA_Move(ms->client, hordir, 400);
			copyv3(hordir, result.movedir);
		}
		/* if not really close to the center of the elevator */
		else{
			MoverBottomCenter(reach, bottomcenter);
			subv3(bottomcenter, ms->origin, hordir);
			hordir[2] = 0;
			dist = normv3(hordir);
			if(dist > 10){
#ifdef DEBUG_FUNCBOB
				botimport.Print(
					PRT_MESSAGE,
					"bot moving to func_bobbing center\n");
#endif
				/* move to the center of the plat */
				if(dist > 100) dist = 100;
				speed = 400 - (400 - 4 * dist);
				EA_Move(ms->client, hordir, speed);
				copyv3(hordir, result.movedir);
			}
		}
	}else{
#ifdef DEBUG_FUNCBOB
		botimport.Print(PRT_MESSAGE, "bot not ontop of func_bobbing\n");
#endif
		/* if very near the reachability end */
		subv3(reach->end, ms->origin, dir);
		dist = lenv3(dir);
		if(dist < 64){
#ifdef DEBUG_FUNCBOB
			botimport.Print(PRT_MESSAGE, "bot moving to end\n");
#endif
			if(dist > 60) dist = 60;
			speed = 360 - (360 - 6 * dist);
			/* if swimming or no barrier jump */
			if((ms->moveflags & MFL_SWIMMING) ||
			   !BotCheckBarrierJump(ms, dir, 50))
				if(speed > 5) EA_Move(ms->client, dir, speed);
			copyv3(dir, result.movedir);
			if(ms->moveflags & MFL_SWIMMING) result.flags |=
					MOVERESULT_SWIMVIEW;
			/* stop using this reachability */
			ms->reachability_time = 0;
			return result;
		}
		/* get direction and distance to reachability start */
		subv3(reach->start, ms->origin, dir1);
		if(!(ms->moveflags & MFL_SWIMMING)) dir1[2] = 0;
		dist1 = normv3(dir1);
		/* if func_bobbing is Not its start position */
		subv3(bob_origin, bob_start, dir);
		if(lenv3(dir) > 16){
#ifdef DEBUG_FUNCBOB
			botimport.Print(PRT_MESSAGE,
				"func_bobbing not at start\n");
#endif
			dist = dist1;
			copyv3(dir1, dir);
			BotCheckBlocked(ms, dir, qfalse, &result);
			if(dist > 60) dist = 60;
			speed = 360 - (360 - 6 * dist);
			if(!(ms->moveflags & MFL_SWIMMING) &&
			   !BotCheckBarrierJump(ms, dir, 50))
				if(speed > 5) EA_Move(ms->client, dir, speed);
			copyv3(dir, result.movedir);
			if(ms->moveflags & MFL_SWIMMING) result.flags |=
					MOVERESULT_SWIMVIEW;
			/* this isn't a failure... just wait till the func_bobbing arrives */
			result.type = RESULTTYPE_WAITFORFUNCBOBBING;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}
		/* get direction and distance to func_bob bottom center */
		MoverBottomCenter(reach, bottomcenter);
		subv3(bottomcenter, ms->origin, dir2);
		if(!(ms->moveflags & MFL_SWIMMING)) dir2[2] = 0;
		dist2 = normv3(dir2);
		/* if very close to the reachability start or
		 * closer to the elevator center or
		 * between reachability start and func_bobbing center */
		if(dist1 < 20 || dist2 < dist1 || dotv3(dir1, dir2) < 0){
#ifdef DEBUG_FUNCBOB
			botimport.Print(PRT_MESSAGE,
				"bot moving to func_bobbing center\n");
#endif
			dist = dist2;
			copyv3(dir2, dir);
		}else{	/* closer to the reachability start */
#ifdef DEBUG_FUNCBOB
			botimport.Print(PRT_MESSAGE,
				"bot moving to reachability start\n");
#endif
			dist = dist1;
			copyv3(dir1, dir);
		}
		BotCheckBlocked(ms, dir, qfalse, &result);
		if(dist > 60) dist = 60;
		speed = 400 - (400 - 6 * dist);
		if(!(ms->moveflags & MFL_SWIMMING) &&
		   !BotCheckBarrierJump(ms, dir, 50))
			EA_Move(ms->client, dir, speed);
		copyv3(dir, result.movedir);
		if(ms->moveflags & MFL_SWIMMING) result.flags |=
				MOVERESULT_SWIMVIEW;
	}
	return result;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_FuncBobbing(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	bob_origin, bob_start, bob_end, dir, hordir, bottomcenter;
	bot_moveresult_t_cleared(result);
	float	dist, speed;

	BotFuncBobStartEnd(reach, bob_start, bob_end, bob_origin);
	subv3(bob_origin, bob_end, dir);
	dist = lenv3(dir);
	/* if the func_bobbing is near the end */
	if(dist < 16){
		subv3(reach->end, ms->origin, hordir);
		if(!(ms->moveflags & MFL_SWIMMING)) hordir[2] = 0;
		dist = normv3(hordir);
		if(dist > 60) dist = 60;
		speed = 360 - (360 - 6 * dist);
		if(speed > 5) EA_Move(ms->client, dir, speed);
		copyv3(dir, result.movedir);
		if(ms->moveflags & MFL_SWIMMING) result.flags |=
				MOVERESULT_SWIMVIEW;
	}else{
		MoverBottomCenter(reach, bottomcenter);
		subv3(bottomcenter, ms->origin, hordir);
		if(!(ms->moveflags & MFL_SWIMMING)) hordir[2] = 0;
		dist = normv3(hordir);
		if(dist > 5){
			/* move to the center of the plat */
			if(dist > 100) dist = 100;
			speed = 400 - (400 - 4 * dist);
			EA_Move(ms->client, hordir, speed);
			copyv3(hordir, result.movedir);
		}
	}
	return result;
}
/* ===========================================================================
 * 0  no valid grapple hook visible
 * 1  the grapple hook is still flying
 * 2  the grapple hooked into a wall
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
int
GrappleState(bot_movestate_t *ms, aas_reachability_t *reach)
{
	int i;
	aas_entityinfo_t entinfo;

	/* if the grapple hook is pulling */
	if(ms->moveflags & MFL_GRAPPLEPULL)
		return 2;
	/* check for a visible grapple missile entity
	 * or visible grapple entity */
	for(i = AAS_NextEntity(0); i; i = AAS_NextEntity(i))
		if(AAS_EntityType(i) == (int)entitytypemissile->value){
			AAS_EntityInfo(i, &entinfo);
			if(entinfo.weapon == (int)weapindex_grapple->value)
				return 1;
		}
	/* no valid grapple at all */
	return 0;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
void
BotResetGrapple(bot_movestate_t *ms)
{
	aas_reachability_t reach;

	AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
	/* if not using the grapple hook reachability anymore */
	if((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_GRAPPLEHOOK){
		if((ms->moveflags & MFL_ACTIVEGRAPPLE) ||
		   ms->grapplevisible_time){
			if(offhandgrapple->value)
				EA_Command(ms->client, cmd_grappleoff->string);
			ms->moveflags &= ~MFL_ACTIVEGRAPPLE;
			ms->grapplevisible_time = 0;
#ifdef DEBUG_GRAPPLE
			botimport.Print(PRT_MESSAGE, "reset grapple\n");
#endif	/* DEBUG_GRAPPLE */
		}
	}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_Grapple(bot_movestate_t *ms, aas_reachability_t *reach)
{
	bot_moveresult_t_cleared(result);
	float	dist, speed;
	Vec3	dir, viewdir, org;
	int	state, areanum;
	bsp_Trace trace;

#ifdef DEBUG_GRAPPLE
	static int debugline;
	if(!debugline) debugline = botimport.DebugLineCreate();
	botimport.DebugLineShow(debugline, reach->start, reach->end,
		LINECOLOR_BLUE);
#endif	/* DEBUG_GRAPPLE */

	if(ms->moveflags & MFL_GRAPPLERESET){
		if(offhandgrapple->value)
			EA_Command(ms->client, cmd_grappleoff->string);
		ms->moveflags &= ~MFL_ACTIVEGRAPPLE;
		return result;
	}
	if(!(int)offhandgrapple->value){
		result.weapon	= weapindex_grapple->value;
		result.flags	|= MOVERESULT_MOVEMENTWEAPON;
	}
	if(ms->moveflags & MFL_ACTIVEGRAPPLE){
#ifdef DEBUG_GRAPPLE
		botimport.Print(PRT_MESSAGE,
			"BotTravel_Grapple: active grapple\n");
#endif	/* DEBUG_GRAPPLE
	 *  */
		state = GrappleState(ms, reach);
		subv3(reach->end, ms->origin, dir);
		dir[2]	= 0;
		dist	= lenv3(dir);
		/* if very close to the grapple end or the grappled is hooked and
		 * the bot doesn't get any closer */
		if(state && dist < 48){
			if(ms->lastgrappledist - dist < 1){
#ifdef DEBUG_GRAPPLE
				botimport.Print(PRT_ERROR,
					"grapple normal end\n");
#endif	/* DEBUG_GRAPPLE */
				if(offhandgrapple->value)
					EA_Command(ms->client,
						cmd_grappleoff->string);
				ms->moveflags	&= ~MFL_ACTIVEGRAPPLE;
				ms->moveflags	|= MFL_GRAPPLERESET;
				ms->reachability_time = 0;	/* end the reachability */
				return result;
			}
		}
		/* if no valid grapple at all, or the grapple hooked and the bot */
		/* isn't moving anymore */
		else if(!state ||
			(state == 2 && dist > ms->lastgrappledist - 2)){
			if(ms->grapplevisible_time < AAS_Time() - 0.4){
#ifdef DEBUG_GRAPPLE
				botimport.Print(PRT_ERROR,
					"grapple not visible\n");
#endif	/* DEBUG_GRAPPLE */
				if(offhandgrapple->value)
					EA_Command(ms->client,
						cmd_grappleoff->string);
				ms->moveflags	&= ~MFL_ACTIVEGRAPPLE;
				ms->moveflags	|= MFL_GRAPPLERESET;
				ms->reachability_time = 0;	/* end the reachability */
				return result;
			}
		}else
			ms->grapplevisible_time = AAS_Time();
		if(!(int)offhandgrapple->value)
			EA_Attack(ms->client);
		/* remember the current grapple distance */
		ms->lastgrappledist = dist;
	}else{
#ifdef DEBUG_GRAPPLE
		botimport.Print(PRT_MESSAGE,
			"BotTravel_Grapple: inactive grapple\n");
#endif	/* DEBUG_GRAPPLE
	 *  */
		ms->grapplevisible_time = AAS_Time();
		subv3(reach->start, ms->origin, dir);
		if(!(ms->moveflags & MFL_SWIMMING)) dir[2] = 0;
		addv3(ms->origin, ms->viewoffset, org);
		subv3(reach->end, org, viewdir);
		dist = normv3(dir);
		Vector2Angles(viewdir, result.ideal_viewangles);
		result.flags |= MOVERESULT_MOVEMENTVIEW;
		if(dist < 5 &&
		   fabs(AngleDiff(result.ideal_viewangles[0],
				   ms->viewangles[0])) < 2 &&
		   fabs(AngleDiff(result.ideal_viewangles[1],
				   ms->viewangles[1])) < 2){
#ifdef DEBUG_GRAPPLE
			botimport.Print(
				PRT_MESSAGE,
				"BotTravel_Grapple: activating grapple\n");
#endif	/* DEBUG_GRAPPLE
	 * check if the grapple missile path is clear */
			addv3(ms->origin, ms->viewoffset, org);
			trace =
				AAS_Trace(org, NULL, NULL, reach->end,
					ms->entitynum,
					CONTENTS_SOLID);
			subv3(reach->end, trace.endpos, dir);
			if(lenv3(dir) > 16){
				result.failure = qtrue;
				return result;
			}
			/* activate the grapple */
			if(offhandgrapple->value)
				EA_Command(ms->client, cmd_grappleon->string);
			else
				EA_Attack(ms->client);
			ms->moveflags |= MFL_ACTIVEGRAPPLE;
			ms->lastgrappledist = 999999;
		}else{
			if(dist < 70) speed = 300 - (300 - 4 * dist);
			else speed = 400;
			BotCheckBlocked(ms, dir, qtrue, &result);
			/* elemantary action move in direction */
			EA_Move(ms->client, dir, speed);
			copyv3(dir, result.movedir);
		}
		/* if in another area before actually grappling */
		areanum = AAS_PointAreaNum(ms->origin);
		if(areanum && areanum !=
		   ms->reachareanum) ms->reachability_time = 0;
	}
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:			-
 * =========================================================================== */
bot_moveresult_t
BotTravel_RocketJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir;
	float	dist, speed;
	bot_moveresult_t_cleared(result);

	/* botimport.Print(PRT_MESSAGE, "BotTravel_RocketJump: bah\n");
	 *  */
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	/* look in the movement direction */
	Vector2Angles(hordir, result.ideal_viewangles);
	/* look straight down */
	result.ideal_viewangles[PITCH] = 90;
	if(dist < 5 &&
	   fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 5 &&
	   fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 5){
		/* botimport.Print(PRT_MESSAGE, "between jump start and run start point\n"); */
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		normv3(hordir);
		/* elemantary action jump */
		EA_Jump(ms->client);
		EA_Attack(ms->client);
		EA_Move(ms->client, hordir, 800);
		ms->jumpreach = ms->lastreachnum;
	}else{
		if(dist > 80) dist = 80;
		speed = 400 - (400 - 5 * dist);
		EA_Move(ms->client, hordir, speed);
	}
	/* look in the movement direction */
	Vector2Angles(hordir, result.ideal_viewangles);
	/* look straight down */
	result.ideal_viewangles[PITCH] = 90;
	/* set the view angles directly */
	EA_View(ms->client, result.ideal_viewangles);
	/* view is important for the movment */
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	/* select the rocket launcher */
	EA_SelectWeapon(ms->client, (int)weapindex_rocketlauncher->value);
	/* weapon is used for movement */
	result.weapon	= (int)weapindex_rocketlauncher->value;
	result.flags	|= MOVERESULT_MOVEMENTWEAPON;
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_BFGJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir;
	float	dist, speed;
	bot_moveresult_t_cleared(result);

	/* botimport.Print(PRT_MESSAGE, "BotTravel_BFGJump: bah\n");
	 *  */
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = normv3(hordir);
	if(dist < 5 &&
	   fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 5 &&
	   fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 5){
		/* botimport.Print(PRT_MESSAGE, "between jump start and run start point\n"); */
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		normv3(hordir);
		/* elemantary action jump */
		EA_Jump(ms->client);
		EA_Attack(ms->client);
		EA_Move(ms->client, hordir, 800);
		ms->jumpreach = ms->lastreachnum;
	}else{
		if(dist > 80) dist = 80;
		speed = 400 - (400 - 5 * dist);
		EA_Move(ms->client, hordir, speed);
	}
	/* look in the movement direction */
	Vector2Angles(hordir, result.ideal_viewangles);
	/* look straight down */
	result.ideal_viewangles[PITCH] = 90;
	/* set the view angles directly */
	EA_View(ms->client, result.ideal_viewangles);
	/* view is important for the movment */
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	/* select the rocket launcher */
	EA_SelectWeapon(ms->client, (int)weapindex_bfg10k->value);
	/* weapon is used for movement */
	result.weapon	= (int)weapindex_bfg10k->value;
	result.flags	|= MOVERESULT_MOVEMENTWEAPON;
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_WeaponJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3	hordir;
	float	speed;
	bot_moveresult_t_cleared(result);

	/* if not jumped yet */
	if(!ms->jumpreach) return result;
	/*
	 * //go straight to the reachability end
	 * hordir[0] = reach->end[0] - ms->origin[0];
	 * hordir[1] = reach->end[1] - ms->origin[1];
	 * hordir[2] = 0;
	 * normv3(hordir);
	 * //always use max speed when traveling through the air
	 * EA_Move(ms->client, hordir, 800);
	 * copyv3(hordir, result.movedir);
	 */
	if(!BotAirControl(ms->origin, ms->velocity, reach->end, hordir,
		   &speed)){
		/* go straight to the reachability end */
		subv3(reach->end, ms->origin, hordir);
		hordir[2] = 0;
		normv3(hordir);
		speed = 400;
	}
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotTravel_JumpPad(bot_movestate_t *ms, aas_reachability_t *reach)
{
	Vec3 hordir;
	bot_moveresult_t_cleared(result);

	/* first walk straight to the reachability start */
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	BotCheckBlocked(ms, hordir, qtrue, &result);
	/* elemantary action move in direction */
	EA_Move(ms->client, hordir, 400);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotFinishTravel_JumpPad(bot_movestate_t *ms, aas_reachability_t *reach)
{
	float	speed;
	Vec3	hordir;
	bot_moveresult_t_cleared(result);

	if(!BotAirControl(ms->origin, ms->velocity, reach->end, hordir,
		   &speed)){
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		normv3(hordir);
		speed = 400;
	}
	BotCheckBlocked(ms, hordir, qtrue, &result);
	/* elemantary action move in direction */
	EA_Move(ms->client, hordir, speed);
	copyv3(hordir, result.movedir);
	return result;
}
/* ===========================================================================
 * time before the reachability times out
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
int
BotReachabilityTime(aas_reachability_t *reach)
{
	switch(reach->traveltype & TRAVELTYPE_MASK){
	case TRAVEL_WALK: return 5;
	case TRAVEL_CROUCH: return 5;
	case TRAVEL_BARRIERJUMP: return 5;
	case TRAVEL_LADDER: return 6;
	case TRAVEL_WALKOFFLEDGE: return 5;
	case TRAVEL_JUMP: return 5;
	case TRAVEL_SWIM: return 5;
	case TRAVEL_WATERJUMP: return 5;
	case TRAVEL_TELEPORT: return 5;
	case TRAVEL_ELEVATOR: return 10;
	case TRAVEL_GRAPPLEHOOK: return 8;
	case TRAVEL_ROCKETJUMP: return 6;
	case TRAVEL_BFGJUMP: return 6;
	case TRAVEL_JUMPPAD: return 10;
	case TRAVEL_FUNCBOB: return 10;
	default:
	{
		botimport.Print(PRT_ERROR,
			"travel type %d not implemented yet\n",
			reach->traveltype);
		return 8;
	}	/* end case */
	}	/* end switch */
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
bot_moveresult_t
BotMoveInGoalArea(bot_movestate_t *ms, bot_goal_t *goal)
{
	bot_moveresult_t_cleared(result);
	Vec3	dir;
	float	dist, speed;

#ifdef DEBUG
	/* botimport.Print(PRT_MESSAGE, "%s: moving straight to goal\n", ClientName(ms->entitynum-1));
	 * AAS_ClearShownDebugLines();
	 * AAS_DebugLine(ms->origin, goal->origin, LINECOLOR_RED); */
#endif	/* DEBUG */
	/* walk straight to the goal origin */
	dir[0]	= goal->origin[0] - ms->origin[0];
	dir[1]	= goal->origin[1] - ms->origin[1];
	if(ms->moveflags & MFL_SWIMMING){
		dir[2] = goal->origin[2] - ms->origin[2];
		result.traveltype = TRAVEL_SWIM;
	}else{
		dir[2] = 0;
		result.traveltype = TRAVEL_WALK;
	}	/* endif */
	dist = normv3(dir);
	if(dist > 100) dist = 100;
	speed = 400 - (400 - 4 * dist);
	if(speed < 10) speed = 0;
	BotCheckBlocked(ms, dir, qtrue, &result);
	/* elemantary action move in direction */
	EA_Move(ms->client, dir, speed);
	copyv3(dir, result.movedir);
	if(ms->moveflags & MFL_SWIMMING){
		Vector2Angles(dir, result.ideal_viewangles);
		result.flags |= MOVERESULT_SWIMVIEW;
	}
	/* if (!debugline) debugline = botimport.DebugLineCreate(); */
	/* botimport.DebugLineShow(debugline, ms->origin, goal->origin, LINECOLOR_BLUE); */
	ms->lastreachnum = 0;
	ms->lastareanum = 0;
	ms->lastgoalareanum = goal->areanum;
	copyv3(ms->origin, ms->lastorigin);
	return result;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
void
BotMoveToGoal(bot_moveresult_t *result, int movestate, bot_goal_t *goal,
	      int travelflags)
{
	int reachnum, lastreachnum, foundjumppad, ent, resultflags;
	aas_reachability_t reach, lastreach;
	bot_movestate_t *ms;
	/* Vec3 mins, maxs, up = {0, 0, 1};
	 * bsp_Trace trace;
	 * static int debugline; */

	result->failure = qfalse;
	result->type = 0;
	result->blocked = qfalse;
	result->blockentity = 0;
	result->traveltype = 0;
	result->flags = 0;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return;
	/* reset the grapple before testing if the bot has a valid goal
	 * because the bot could lose all its goals when stuck to a wall */
	BotResetGrapple(ms);
	if(!goal){
#ifdef DEBUG
		botimport.Print(PRT_MESSAGE,
			"client %d: movetogoal -> no goal\n",
			ms->client);
#endif	/* DEBUG */
		result->failure = qtrue;
		return;
	}
	/* botimport.Print(PRT_MESSAGE, "numavoidreach = %d\n", ms->numavoidreach); */
	/* remove some of the move flags */
	ms->moveflags &= ~(MFL_SWIMMING|MFL_AGAINSTLADDER);
	/* set some of the move flags
	 * NOTE: the MFL_ONGROUND flag is also set in the higher AI */
	if(AAS_OnGround(ms->origin, ms->presencetype,
		   ms->entitynum)) ms->moveflags |= MFL_ONGROUND;
	if(ms->moveflags & MFL_ONGROUND){
		int modeltype, modelnum;

		ent = BotOnTopOfEntity(ms);

		if(ent != -1){
			modelnum = AAS_EntityModelindex(ent);
			if(modelnum >= 0 && modelnum < MAX_MODELS){
				modeltype = modeltypes[modelnum];

				if(modeltype == MODELTYPE_FUNC_PLAT){
					AAS_ReachabilityFromNum(ms->lastreachnum,
						&reach);
					/* if the bot is Not using the elevator */
					if((reach.traveltype &
					    TRAVELTYPE_MASK) !=
					   TRAVEL_ELEVATOR ||
						/* NOTE: the face number is the plat model number */
					   (reach.facenum & 0x0000FFFF) !=
					   modelnum){
						reachnum =
							AAS_NextModelReachability(
								0, modelnum);
						if(reachnum){
							/* botimport.Print(PRT_MESSAGE, "client %d: accidentally ended up on func_plat\n", ms->client); */
							AAS_ReachabilityFromNum(
								reachnum, &reach);
							ms->lastreachnum =
								reachnum;
							ms->reachability_time =
								AAS_Time() +
								BotReachabilityTime(
									&reach);
						}else{
							if(botDeveloper)
								botimport.Print(
									PRT_MESSAGE,
									"client %d: on func_plat without reachability\n",
									ms->
									client);
							result->blocked = qtrue;
							result->blockentity =
								ent;
							result->flags |=
								MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}
					result->flags |=
						MOVERESULT_ONTOPOF_ELEVATOR;
				}else if(modeltype == MODELTYPE_FUNC_BOB){
					AAS_ReachabilityFromNum(ms->lastreachnum,
						&reach);
					/* if the bot is Not using the func bobbing */
					if((reach.traveltype &
					    TRAVELTYPE_MASK) !=
					   TRAVEL_FUNCBOB ||
						/* NOTE: the face number is the func_bobbing model number */
					   (reach.facenum & 0x0000FFFF) !=
					   modelnum){
						reachnum =
							AAS_NextModelReachability(
								0, modelnum);
						if(reachnum){
							/* botimport.Print(PRT_MESSAGE, "client %d: accidentally ended up on func_bobbing\n", ms->client); */
							AAS_ReachabilityFromNum(
								reachnum, &reach);
							ms->lastreachnum =
								reachnum;
							ms->reachability_time =
								AAS_Time() +
								BotReachabilityTime(
									&reach);
						}else{
							if(botDeveloper)
								botimport.Print(
									PRT_MESSAGE,
									"client %d: on func_bobbing without reachability\n",
									ms->
									client);
							result->blocked = qtrue;
							result->blockentity =
								ent;
							result->flags |=
								MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}
					result->flags |=
						MOVERESULT_ONTOPOF_FUNCBOB;
				}else if(modeltype == MODELTYPE_FUNC_STATIC ||
					 modeltype == MODELTYPE_FUNC_DOOR){
					/* check if ontop of a door bridge ? */
					ms->areanum =
						BotFuzzyPointReachabilityArea(
							ms->origin);
					/* if not in a reachability area */
					if(!AAS_AreaReachability(ms->areanum)){
						result->blocked = qtrue;
						result->blockentity = ent;
						result->flags |=
							MOVERESULT_ONTOPOFOBSTACLE;
						return;
					}
				}
				else{
					result->blocked = qtrue;
					result->blockentity = ent;
					result->flags |=
						MOVERESULT_ONTOPOFOBSTACLE;
					return;
				}
			}
		}
	}
	/* if swimming */
	if(AAS_Swimming(ms->origin)) ms->moveflags |= MFL_SWIMMING;
	/* if against a ladder */
	if(AAS_AgainstLadder(ms->origin)) ms->moveflags |= MFL_AGAINSTLADDER;
	/* if the bot is on the ground, swimming or against a ladder */
	if(ms->moveflags & (MFL_ONGROUND|MFL_SWIMMING|MFL_AGAINSTLADDER)){
		/* botimport.Print(PRT_MESSAGE, "%s: onground, swimming or against ladder\n", ClientName(ms->entitynum-1));
		 *  */
		AAS_ReachabilityFromNum(ms->lastreachnum, &lastreach);
		/* reachability area the bot is in
		 * ms->areanum = BotReachabilityArea(ms->origin, ((lastreach.traveltype & TRAVELTYPE_MASK) != TRAVEL_ELEVATOR)); */
		ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
		if(!ms->areanum){
			result->failure = qtrue;
			result->blocked = qtrue;
			result->blockentity = 0;
			result->type = RESULTTYPE_INSOLIDAREA;
			return;
		}
		/* if the bot is in the goal area */
		if(ms->areanum == goal->areanum){
			*result = BotMoveInGoalArea(ms, goal);
			return;
		}
		/* assume we can use the reachability from the last frame */
		reachnum = ms->lastreachnum;
		/* if there is a last reachability */
		if(reachnum){
			AAS_ReachabilityFromNum(reachnum, &reach);
			/* check if the reachability is still valid */
			if(!(AAS_TravelFlagForType(reach.traveltype) &
			     travelflags))
				reachnum = 0;
			/* special grapple hook case */
			else if((reach.traveltype & TRAVELTYPE_MASK) ==
				TRAVEL_GRAPPLEHOOK){
				if(ms->reachability_time < AAS_Time() ||
				   (ms->moveflags & MFL_GRAPPLERESET))
					reachnum = 0;
			}
			/* special elevator case */
			else if((reach.traveltype & TRAVELTYPE_MASK) ==
				TRAVEL_ELEVATOR ||
				(reach.traveltype & TRAVELTYPE_MASK) ==
				TRAVEL_FUNCBOB){
				if((result->flags &
				    MOVERESULT_ONTOPOF_ELEVATOR) ||
				   (result->flags & MOVERESULT_ONTOPOF_FUNCBOB))
					ms->reachability_time = AAS_Time() + 5;
				/* if the bot was going for an elevator and reached the reachability area */
				if(ms->areanum == reach.areanum ||
				   ms->reachability_time < AAS_Time())
					reachnum = 0;
			}else{
#ifdef DEBUG
				if(botDeveloper)
					if(ms->reachability_time < AAS_Time()){
						botimport.Print(
							PRT_MESSAGE,
							"client %d: reachability timeout in ",
							ms->client);
						AAS_PrintTravelType(
							reach.traveltype &
							TRAVELTYPE_MASK);
						botimport.Print(PRT_MESSAGE,
							"\n");
					}
				/*
				 * if (ms->lastareanum != ms->areanum)
				 * {
				 *      botimport.Print(PRT_MESSAGE, "changed from area %d to %d\n", ms->lastareanum, ms->areanum);
				 * } */
#endif	/* DEBUG
	 * if the goal area changed or the reachability timed out */
				/* or the area changed */
				if(ms->lastgoalareanum != goal->areanum ||
				   ms->reachability_time < AAS_Time() ||
				   ms->lastareanum != ms->areanum)
					reachnum = 0;
				/* botimport.Print(PRT_MESSAGE, "area change or timeout\n"); */

			}
		}
		resultflags = 0;
		/* if the bot needs a new reachability */
		if(!reachnum){
			/* if the area has no reachability links */
			if(!AAS_AreaReachability(ms->areanum)){
#ifdef DEBUG
				if(botDeveloper)
					botimport.Print(
						PRT_MESSAGE,
						"area %d no reachability\n",
						ms->areanum);
#endif	/* DEBUG */
			}
			/* get a new reachability leading towards the goal */
			reachnum =
				BotGetReachabilityToGoal(ms->origin, ms->areanum,
					ms->lastgoalareanum, ms->lastareanum,
					ms->avoidreach, ms->avoidreachtimes,
					ms->avoidreachtries,
					goal, travelflags, travelflags,
					ms->avoidspots, ms->numavoidspots,
					&resultflags);
			/* the area number the reachability starts in */
			ms->reachareanum = ms->areanum;
			/* reset some state variables */
			ms->jumpreach	= 0;			/* for TRAVEL_JUMP */
			ms->moveflags	&= ~MFL_GRAPPLERESET;	/* for TRAVEL_GRAPPLEHOOK */
			/* if there is a reachability to the goal */
			if(reachnum){
				AAS_ReachabilityFromNum(reachnum, &reach);
				/* set a timeout for this reachability */
				ms->reachability_time = AAS_Time() +
							BotReachabilityTime(
					&reach);
#ifdef AVOIDREACH
				/* add the reachability to the reachabilities to avoid for a while */
				BotAddToAvoidReach(ms, reachnum, AVOIDREACH_TIME);
#endif	/* AVOIDREACH */
			}
#ifdef DEBUG

			else if(botDeveloper){
				botimport.Print(PRT_MESSAGE,
					"goal not reachable\n");
				Q_Memset(&reach, 0, sizeof(aas_reachability_t));	/* make compiler happy */
			}
			if(botDeveloper)
				/* if still going for the same goal */
				if(ms->lastgoalareanum == goal->areanum)
					if(ms->lastareanum == reach.areanum)
						botimport.Print(
							PRT_MESSAGE,
							"same goal, going back to previous area\n");
#endif	/* DEBUG */
		}
		ms->lastreachnum = reachnum;
		ms->lastgoalareanum = goal->areanum;
		ms->lastareanum = ms->areanum;
		/* if the bot has a reachability */
		if(reachnum){
			/* get the reachability from the number */
			AAS_ReachabilityFromNum(reachnum, &reach);
			result->traveltype = reach.traveltype;
#ifdef DEBUG_AI_MOVE
			AAS_ClearShownDebugLines();
			AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
			AAS_ShowReachability(&reach);
#endif	/* DEBUG_AI_MOVE
	 *  */
#ifdef DEBUG
			/* botimport.Print(PRT_MESSAGE, "client %d: ", ms->client);
			 * AAS_PrintTravelType(reach.traveltype);
			 * botimport.Print(PRT_MESSAGE, "\n"); */
#endif	/* DEBUG */
			switch(reach.traveltype & TRAVELTYPE_MASK){
			case TRAVEL_WALK: *result = BotTravel_Walk(ms,
					&reach); break;
			case TRAVEL_CROUCH: *result = BotTravel_Crouch(
					ms, &reach); break;
			case TRAVEL_BARRIERJUMP: *result = BotTravel_BarrierJump(
					ms, &reach); break;
			case TRAVEL_LADDER: *result = BotTravel_Ladder(ms,
					&reach); break;
			case TRAVEL_WALKOFFLEDGE: *result =
				BotTravel_WalkOffLedge(ms, &reach); break;
			case TRAVEL_JUMP: *result = BotTravel_Jump(ms,
					&reach); break;
			case TRAVEL_SWIM: *result = BotTravel_Swim(ms,
					&reach); break;
			case TRAVEL_WATERJUMP: *result =
				BotTravel_WaterJump(ms, &reach); break;
			case TRAVEL_TELEPORT: *result =
				BotTravel_Teleport(ms, &reach); break;
			case TRAVEL_ELEVATOR: *result =
				BotTravel_Elevator(ms, &reach); break;
			case TRAVEL_GRAPPLEHOOK: *result =
				BotTravel_Grapple(ms, &reach); break;
			case TRAVEL_ROCKETJUMP: *result =
				BotTravel_RocketJump(ms, &reach); break;
			case TRAVEL_BFGJUMP: *result = BotTravel_BFGJump(
					ms, &reach); break;
			case TRAVEL_JUMPPAD: *result = BotTravel_JumpPad(
					ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotTravel_FuncBobbing(
					ms, &reach); break;
			default:
			{
				botimport.Print(
					PRT_FATAL,
					"travel type %d not implemented yet\n",
					(reach.traveltype & TRAVELTYPE_MASK));
				break;
			}	/* end case */
			}	/* end switch */
			result->traveltype = reach.traveltype;
			result->flags |= resultflags;
		}else{
			result->failure = qtrue;
			result->flags	|= resultflags;
			Q_Memset(&reach, 0, sizeof(aas_reachability_t));
		}
#ifdef DEBUG
		if(botDeveloper)
			if(result->failure){
				botimport.Print(
					PRT_MESSAGE,
					"client %d: movement failure in ",
					ms->client);
				AAS_PrintTravelType(
					reach.traveltype & TRAVELTYPE_MASK);
				botimport.Print(PRT_MESSAGE, "\n");
			}
#endif	/* DEBUG */
	}else{
		int i, numareas, areas[16];
		Vec3 end;

		/* special handling of jump pads when the bot uses a jump pad without knowing it */
		foundjumppad = qfalse;
		maddv3(ms->origin, -2 * ms->thinktime, ms->velocity, end);
		numareas = AAS_TraceAreas(ms->origin, end, areas, NULL, 16);
		for(i = numareas-1; i >= 0; i--)
			if(AAS_AreaJumpPad(areas[i])){
				/* botimport.Print(PRT_MESSAGE, "client %d used a jumppad without knowing, area %d\n", ms->client, areas[i]); */
				foundjumppad = qtrue;
				lastreachnum = BotGetReachabilityToGoal(
					end, areas[i],
					ms->lastgoalareanum, ms->lastareanum,
					ms->avoidreach, ms->avoidreachtimes,
					ms->avoidreachtries,
					goal, travelflags, TFL_JUMPPAD,
					ms->avoidspots, ms->numavoidspots, NULL);
				if(lastreachnum){
					ms->lastreachnum = lastreachnum;
					ms->lastareanum = areas[i];
					/* botimport.Print(PRT_MESSAGE, "found jumppad reachability\n"); */
					break;
				}else{
					for(lastreachnum =
						    AAS_NextAreaReachability(
							    areas[i], 0);
					    lastreachnum;
					    lastreachnum =
						    AAS_NextAreaReachability(
							    areas[i],
							    lastreachnum)){
						/* get the reachability from the number */
						AAS_ReachabilityFromNum(
							lastreachnum, &reach);
						if((reach.traveltype &
						    TRAVELTYPE_MASK) ==
						   TRAVEL_JUMPPAD){
							ms->lastreachnum
								= lastreachnum;
							ms->lastareanum
								= areas[i];
							/* botimport.Print(PRT_MESSAGE, "found jumppad reachability hard!!\n"); */
							break;
						}
					}
					if(lastreachnum) break;
				}
			}
		if(botDeveloper)
			/* if a jumppad is found with the trace but no reachability is found */
			if(foundjumppad && !ms->lastreachnum)
				botimport.Print(
					PRT_MESSAGE,
					"client %d didn't find jumppad reachability\n",
					ms->client);
		if(ms->lastreachnum){
			/* botimport.Print(PRT_MESSAGE, "%s: NOT onground, swimming or against ladder\n", ClientName(ms->entitynum-1)); */
			AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
			result->traveltype = reach.traveltype;
#ifdef DEBUG
			/* botimport.Print(PRT_MESSAGE, "client %d finish: ", ms->client);
			 * AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
			 * botimport.Print(PRT_MESSAGE, "\n"); */
#endif	/* DEBUG
	 *  */
			switch(reach.traveltype & TRAVELTYPE_MASK){
			case TRAVEL_WALK: *result = BotTravel_Walk(ms, &reach);
				break;	/* BotFinishTravel_Walk(ms, &reach); break; */
			case TRAVEL_CROUCH: /*do nothing*/ break;
			case TRAVEL_BARRIERJUMP: *result =
				BotFinishTravel_BarrierJump(ms, &reach); break;
			case TRAVEL_LADDER: *result = BotTravel_Ladder(ms,
					&reach); break;
			case TRAVEL_WALKOFFLEDGE: *result =
				BotFinishTravel_WalkOffLedge(ms, &reach); break;
			case TRAVEL_JUMP: *result = BotFinishTravel_Jump(
					ms, &reach); break;
			case TRAVEL_SWIM: *result = BotTravel_Swim(ms,
					&reach); break;
			case TRAVEL_WATERJUMP: *result =
				BotFinishTravel_WaterJump(ms, &reach); break;
			case TRAVEL_TELEPORT: /*do nothing*/ break;
			case TRAVEL_ELEVATOR: *result = BotFinishTravel_Elevator(
					ms, &reach); break;
			case TRAVEL_GRAPPLEHOOK: *result = BotTravel_Grapple(
					ms, &reach); break;
			case TRAVEL_ROCKETJUMP:
			case TRAVEL_BFGJUMP: *result =
				BotFinishTravel_WeaponJump(ms, &reach); break;
			case TRAVEL_JUMPPAD: *result =
				BotFinishTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result =
				BotFinishTravel_FuncBobbing(ms, &reach); break;
			default:
			{
				botimport.Print(
					PRT_FATAL,
					"(last) travel type %d not implemented yet\n",
					(reach.traveltype & TRAVELTYPE_MASK));
				break;
			}	/* end case */
			}	/* end switch */
			result->traveltype = reach.traveltype;
#ifdef DEBUG
			if(botDeveloper)
				if(result->failure){
					botimport.Print(
						PRT_MESSAGE,
						"client %d: movement failure in finish ",
						ms->client);
					AAS_PrintTravelType(
						reach.traveltype &
						TRAVELTYPE_MASK);
					botimport.Print(PRT_MESSAGE, "\n");
				}
#endif	/* DEBUG */
		}
	}
	/* FIXME: is it right to do this here? */
	if(result->blocked) ms->reachability_time -= 10 * ms->thinktime;
	/* copy the last origin */
	copyv3(ms->origin, ms->lastorigin);
	/* return the movement result */
	return;
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
void
BotResetAvoidReach(int movestate)
{
	bot_movestate_t *ms;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return;
	Q_Memset(ms->avoidreach, 0, MAX_AVOIDREACH * sizeof(int));
	Q_Memset(ms->avoidreachtimes, 0, MAX_AVOIDREACH * sizeof(float));
	Q_Memset(ms->avoidreachtries, 0, MAX_AVOIDREACH * sizeof(int));
}
/* ===========================================================================
 *
 * Parameter:				-
 * Returns:					-
 * Changes Globals:		-
 * =========================================================================== */
void
BotResetLastAvoidReach(int movestate)
{
	int i, latest;
	float latesttime;
	bot_movestate_t *ms;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return;
	latesttime = 0;
	latest = 0;
	for(i = 0; i < MAX_AVOIDREACH; i++)
		if(ms->avoidreachtimes[i] > latesttime){
			latesttime = ms->avoidreachtimes[i];
			latest = i;
		}
	if(latesttime){
		ms->avoidreachtimes[latest] = 0;
		if(ms->avoidreachtries[latest] >
		   0) ms->avoidreachtries[latest]--;
	}
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
void
BotResetMoveState(int movestate)
{
	bot_movestate_t *ms;

	ms = BotMoveStateFromHandle(movestate);
	if(!ms) return;
	Q_Memset(ms, 0, sizeof(bot_movestate_t));
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
int
BotSetupMoveAI(void)
{
	BotSetBrushModelTypes();
	sv_maxstep = LibVar("sv_step", "18");
	sv_maxbarrier = LibVar("sv_maxbarrier", "32");
	sv_gravity = LibVar("sv_gravity", "800");
	weapindex_rocketlauncher	= LibVar("weapindex_rocketlauncher", "5");
	weapindex_bfg10k		= LibVar("weapindex_bfg10k", "9");
	weapindex_grapple		= LibVar("weapindex_grapple", "10");
	entitytypemissile		= LibVar("entitytypemissile", "3");
	offhandgrapple	= LibVar("offhandgrapple", "0");
	cmd_grappleon	= LibVar("cmd_grappleon", "grappleon");
	cmd_grappleoff	= LibVar("cmd_grappleoff", "grappleoff");
	return BLERR_NOERROR;
}
/* ===========================================================================
 *
 * Parameter:			-
 * Returns:				-
 * Changes Globals:		-
 * =========================================================================== */
void
BotShutdownMoveAI(void)
{
	int i;

	for(i = 1; i <= MAX_CLIENTS; i++)
		if(botmovestates[i]){
			FreeMemory(botmovestates[i]);
			botmovestates[i] = NULL;
		}
}
