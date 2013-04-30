/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"

/* debugging on */
#define AAS_DEBUG

#define DF_AASENTNUMBER(x)	(x - aasworld.entities)
#define DF_NUMBERAASENT(x)	(&aasworld.entities[x])
#define DF_AASENTCLIENT(x)	(x - aasworld.entities - 1)
#define DF_CLIENTAASENT(x)	(&aasworld.entities[x + 1])

#ifndef MAX_PATH
	#define MAX_PATH MAX_QPATH
#endif

/* structure to link entities to areas and areas to entities */
typedef struct aas_link_s {
	int	entnum;
	int	areanum;
	struct aas_link_s	*next_ent, *prev_ent;
	struct aas_link_s	*next_area, *prev_area;
} aas_link_t;

/* structure to link entities to leaves and leaves to entities */
typedef struct bsp_link_s {
	int	entnum;
	int	leafnum;
	struct bsp_link_s	*next_ent, *prev_ent;
	struct bsp_link_s	*next_leaf, *prev_leaf;
} bsp_link_t;

typedef struct bsp_entdata_s {
	Vec3	origin;
	Vec3	angles;
	Vec3	absmins;
	Vec3	absmaxs;
	int	solid;
	int	modelnum;
} bsp_entdata_t;

/* entity */
typedef struct aas_entity_s {
	/* entity info */
	aas_entityinfo_t	i;
	/* links into the AAS areas */
	aas_link_t		*areas;
	/* links into the BSP leaves */
	bsp_link_t		*leaves;
} aas_entity_t;

typedef struct aas_settings_s {
	Vec3	phys_gravitydirection;
	float	phys_friction;
	float	phys_stopspeed;
	float	phys_gravity;
	float	phys_waterfriction;
	float	phys_watergravity;
	float	phys_maxvelocity;
	float	phys_maxwalkvelocity;
	float	phys_maxcrouchvelocity;
	float	phys_maxswimvelocity;
	float	phys_walkaccelerate;
	float	phys_airaccelerate;
	float	phys_swimaccelerate;
	float	phys_maxstep;
	float	phys_maxsteepness;
	float	phys_maxwaterjump;
	float	phys_maxbarrier;
	float	phys_jumpvel;
	float	phys_falldelta5;
	float	phys_falldelta10;
	float	rs_waterjump;
	float	rs_teleport;
	float	rs_barrierjump;
	float	rs_startcrouch;
	float	rs_startgrapple;
	float	rs_startwalkoffledge;
	float	rs_startjump;
	float	rs_rocketjump;
	float	rs_jumppad;
	float	rs_aircontrolledjumppad;
	float	rs_funcbob;
	float	rs_startelevator;
	float	rs_falldamage5;
	float	rs_falldamage10;
	float	rs_maxfallheight;
	float	rs_maxjumpfallheight;
} aas_settings_t;

#define CACHETYPE_PORTAL	0
#define CACHETYPE_AREA		1

/* routing cache */
typedef struct aas_routingcache_s {
	byte	type;			/* portal or area cache */
	float	time;			/* last time accessed or updated */
	int	size;			/* size of the routing cache */
	int	cluster;		/* cluster the cache is for */
	int	areanum;		/* area the cache is created for */
	Vec3	origin;			/* origin within the area */
	float	starttraveltime;	/* travel time to start with */
	int	travelflags;		/* combinations of the travel flags */
	struct aas_routingcache_s	*prev, *next;
	struct aas_routingcache_s	*time_prev, *time_next;
	unsigned char			*reachabilities;	/* reachabilities used for routing */
	unsigned short int		traveltimes[1];		/* travel time for every area (variable sized) */
} aas_routingcache_t;

/* fields for the routing algorithm */
typedef struct aas_routingupdate_s {
	int				cluster;
	int				areanum;		/* area number of the update */
	Vec3				start;			/* start point the area was entered */
	unsigned short int		tmptraveltime;		/* temporary travel time */
	unsigned short int		*areatraveltimes;	/* travel times within the area */
	qbool				inlist;			/* true if the update is in the list */
	struct aas_routingupdate_s	*next;
	struct aas_routingupdate_s	*prev;
} aas_routingupdate_t;

/* reversed reachability link */
typedef struct aas_reversedlink_s {
	int	linknum;			/* the aas_areareachability_t */
	int	areanum;			/* reachable from this area */
	struct aas_reversedlink_s *next;	/* next link */
} aas_reversedlink_t;

/* reversed area reachability */
typedef struct aas_reversedreachability_s {
	int numlinks;
	aas_reversedlink_t *first;
} aas_reversedreachability_t;

/* areas a reachability goes through */
typedef struct aas_reachabilityareas_s {
	int firstarea, numareas;
} aas_reachabilityareas_t;

typedef struct aas_s {
	int	loaded;		/* true when an AAS file is loaded */
	int	initialized;	/* true when AAS has been initialized */
	int	savefile;	/* set true when file should be saved */
	int	bspchecksum;
	float	time;	/* current time */
	int	numframes;
	char	filename[MAX_PATH];	/* name of the aas file */
	char	mapname[MAX_PATH];
	int	numbboxes;
	aas_bbox_t	*bboxes;
	int		numvertexes;
	aas_vertex_t	*vertexes;
	int		numplanes;
	aas_plane_t	*planes;
	int		numedges;
	aas_edge_t	*edges;
	int		edgeindexsize;
	aas_edgeindex_t *edgeindex;
	int		numfaces;
	aas_face_t	*faces;
	int		faceindexsize;
	aas_faceindex_t *faceindex;
	/* convex areas */
	int		numareas;
	aas_area_t	*areas;
	int		numareasettings;	/* convex area settings */
	aas_areasettings_t		*areasettings;
	int				reachabilitysize;	/* reachablity list */
	aas_reachability_t		*reachability;
	int				numnodes;	/* nodes of the bsp tree */
	aas_node_t			*nodes;
	int				numportals;	/* cluster portals */
	aas_portal_t			*portals;
	int				portalindexsize;	/* cluster portal index */
	aas_portalindex_t		*portalindex;
	int				numclusters;	/* clusters */
	aas_cluster_t			*clusters;
	int				numreachabilityareas;
	float				reachabilitytime;
	/* enities linked in the areas */
	aas_link_t			*linkheap;		/* heap with link structures */
	int				linkheapsize;		/* size of the link heap */
	aas_link_t			*freelinks;		/* first free link */
	aas_link_t			**arealinkedentities;	/* entities linked into areas */
	int				maxentities;	/* entities */
	int				maxclients;
	aas_entity_t			*entities;
	/* index to retrieve travel flag for a travel type */
	int				travelflagfortype[MAX_TRAVELTYPES];
	/* travel flags for each area based on contents */
	int				*areacontentstravelflags;
	aas_routingupdate_t		*areaupdate;	/* routing update */
	aas_routingupdate_t		*portalupdate;
	/* number of routing updates during a frame (reset every frame) */
	int				frameroutingupdates;
	aas_reversedreachability_t	*reversedreachability;	/* reversed reachability links */
	ushort			***areatraveltimes;	/* travel times within the areas */
	/* array of size numclusters with cluster cache */
	aas_routingcache_t		***clusterareacache;
	aas_routingcache_t		**portalcache;
	/* cache list sorted on time */
	aas_routingcache_t		*oldestcache;	/* start of cache list sorted on time */
	aas_routingcache_t		*newestcache;	/* end of cache list sorted on time */
	int		*portalmaxtraveltimes;	/* maximum travel time through portal areas */
	int		*reachabilityareaindex;	/* areas the reachabilities go through */
	aas_reachabilityareas_t		*reachabilityareas;
} aas_t;

#define AASINTERN

#ifndef BSPCINCLUDE

#include "be_aas_main.h"
#include "be_aas_entity.h"
#include "be_aas_sample.h"
#include "be_aas_cluster.h"
#include "be_aas_reach.h"
#include "be_aas_route.h"
#include "be_aas_routealt.h"
#include "be_aas_debug.h"
#include "be_aas_file.h"
#include "be_aas_optimize.h"
#include "be_aas_bsp.h"
#include "be_aas_move.h"

#endif	/* BSPCINCLUDE */
