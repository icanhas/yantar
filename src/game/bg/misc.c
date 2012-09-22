/* both games' misc functions, all completely stateless */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "q_shared.h"
#include "public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
 * DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
 * The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.
 *
 * If an item is the target of another entity, it will not spawn in until fired.
 *
 * An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.
 *
 * "notfree" if set to 1, don't spawn in free for all games
 * "notteam" if set to 1, don't spawn in team games
 * "notsingle" if set to 1, don't spawn in single player games
 * "wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
 * "random" random number of plus or minus seconds varied from the respawn time
 * "count" override quantity or duration on most items.
 */

gitem_t bg_itemlist[] =
{
	{
		NULL,
		NULL,
		{ NULL,
		  NULL,
		  NULL, NULL},
/* icon */ NULL,
/* pickup */ NULL,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ ""
	},	/* leave index 0 alone */

	/*
	 * ARMOR
	 *  */

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_armor_shard",
		Pmiscsounds "/ar1_pkup",
		{ Parmormodels "/shard",
		  Parmormodels "/shard_sphere",
		  NULL, NULL},
/* icon */ Picons "/iconr_shard",
/* pickup */ "Armor Shard",
		5,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_armor_combat",
		Pmiscsounds "/ar2_pkup",
		{ Parmormodels "/armor_yel",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconr_yellow",
/* pickup */ "Armor",
		50,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_armor_body",
		Pmiscsounds "/ar2_pkup",
		{ Parmormodels "/armor_red",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconr_red",
/* pickup */ "Heavy Armor",
		100,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

	/*
	 * health
	 *  */
/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_health_small",
		Pitemsounds "/s_health",
		{ Phealthmodels "/small_cross",
		  Phealthmodels "/small_sphere",
		  NULL, NULL },
/* icon */ Picons "/iconh_green",
/* pickup */ "5 Health",
		5,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_health",
		Pitemsounds "/n_health",
		{ Phealthmodels "/medium_cross",
		  Phealthmodels "/medium_sphere",
		  NULL, NULL },
/* icon */ Picons "/iconh_yellow",
/* pickup */ "25 Health",
		25,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_health_large",
		Pitemsounds "/l_health",
		{ Phealthmodels "/large_cross",
		  Phealthmodels "/large_sphere",
		  NULL, NULL },
/* icon */ Picons "/iconh_red",
/* pickup */ "50 Health",
		50,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_health_mega",
		Pitemsounds "/m_health",
		{ Phealthmodels "/mega_cross",
		  Phealthmodels "/mega_sphere",
		  NULL, NULL },
/* icon */ Picons "/iconh_mega",
/* pickup */ "Mega Health",
		100,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},


	/*
	 * WEAPONS
	 *  */

/*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_gauntlet",
		Pmiscsounds "/w_pkup",
		{ Pmeleemodels "/gauntlet",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_gauntlet",
/* pickup */ "Gauntlet",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_shotgun",
		Pmiscsounds "/w_pkup",
		{ Pshotgunmodels "/shotgun",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_shotgun",
/* pickup */ "Shotgun",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_machinegun",
		Pmiscsounds "/w_pkup",
		{ Pmgmodels "/machinegun",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_machinegun",
/* pickup */ "Machinegun",
		200,
		IT_WEAPON,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_grenadelauncher",
		Pmiscsounds "/w_pkup",
		{ Pgrenademodels "/grenadel",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_grenade",
/* pickup */ "Grenade Launcher",
		10,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */
		(Pgrenadesounds "/hgrenb1a " Pgrenadesounds "/hgrenb2a")
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_rocketlauncher",
		Pmiscsounds "/w_pkup",
		{ Prlmodels "/rocketl",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_rocket",
/* pickup */ "Rocket Launcher",
		10,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_lightning",
		Pmiscsounds "/w_pkup",
		{ Plgmodels "/lightning",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_lightning",
/* pickup */ "Lightning Gun",
		100,
		IT_WEAPON,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_railgun",
		Pmiscsounds "/w_pkup",
		{ Prailmodels "/railgun",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_railgun",
/* pickup */ "Railgun",
		10,
		IT_WEAPON,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_plasmagun",
		Pmiscsounds "/w_pkup",
		{ Pplasmamodels "/plasma",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_plasma",
/* pickup */ "Plasma Gun",
		50,
		IT_WEAPON,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_bfg",
		Pmiscsounds "/w_pkup",
		{ Pbfgmodels "/bfg",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_bfg",
/* pickup */ "BFG10K",
		20,
		IT_WEAPON,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_grapplinghook",
		Pmiscsounds "/w_pkup",
		{ Phookmodels "/grapple",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_grapple",
/* pickup */ "Grappling Hook",
		0,
		IT_WEAPON,
		WP_GRAPPLING_HOOK,
/* precache */ "",
/* sounds */ ""
	},

	/*
	 * AMMO ITEMS
	 *  */

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_shells",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/shotgunam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_shotgun",
/* pickup */ "Shells",
		10,
		IT_AMMO,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_bullets",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/machinegunam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_machinegun",
/* pickup */ "Bullets",
		50,
		IT_AMMO,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_grenades",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/grenadeam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_grenade",
/* pickup */ "Grenades",
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_cells",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/plasmaam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_plasma",
/* pickup */ "Cells",
		30,
		IT_AMMO,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_lightning",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/lightningam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_lightning",
/* pickup */ "Lightning",
		60,
		IT_AMMO,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_rockets",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/rocketam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_rocket",
/* pickup */ "Rockets",
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_slugs",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/railgunam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_railgun",
/* pickup */ "Slugs",
		10,
		IT_AMMO,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_bfg",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/bfgam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_bfg",
/* pickup */ "Bfg Ammo",
		15,
		IT_AMMO,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

	/*
	 * HOLDABLE ITEMS
	 *  */
/*QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"holdable_teleporter",
		Pitemsounds "/holdable",
		{ Pholdablemodels "/teleporter",
		  NULL, NULL, NULL},
/* icon */ Picons "/teleporter",
/* pickup */ "Personal Teleporter",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"holdable_medkit",
		Pitemsounds "/holdable",
		{
			Pholdablemodels "/medkit",
			Pholdablemodels "/medkit_sphere",
			NULL, NULL
		},
/* icon */ Picons "/medkit",
/* pickup */ "Medkit",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
/* precache */ "",
/* sounds */ Pitemsounds "/use_medkit"
	},

	/*
	 * POWERUP ITEMS
	 *  */
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_quad",
		Pitemsounds "/quaddamage",
		{ Pinstantmodels "/quad",
		  Pinstantmodels "/quad_ring",
		  NULL, NULL },
/* icon */ Picons "/quad",
/* pickup */ "Quad Damage",
		30,
		IT_POWERUP,
		PW_QUAD,
/* precache */ "",
/* sounds */ (Pitemsounds "/damage2 " Pitemsounds "/damage3")
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_enviro",
		Pitemsounds "/protect",
		{ Pinstantmodels "/enviro",
		  Pinstantmodels "/enviro_ring",
		  NULL, NULL },
/* icon */ Picons "/envirosuit",
/* pickup */ "Battle Suit",
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
/* precache */ "",
/* sounds */ (Pitemsounds "/airout " Pitemsounds "/protect3")
	},

/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_haste",
		Pitemsounds "/haste",
		{ Pinstantmodels "/haste",
		  Pinstantmodels "/haste_ring",
		  NULL, NULL },
/* icon */ Picons "/haste",
/* pickup */ "Speed",
		30,
		IT_POWERUP,
		PW_HASTE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_invis",
		Pitemsounds "/invisibility",
		{ Pinstantmodels "/invis",
		  Pinstantmodels "/invis_ring",
		  NULL, NULL },
/* icon */ Picons "/invis",
/* pickup */ "Invisibility",
		30,
		IT_POWERUP,
		PW_INVIS,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_regen",
		Pitemsounds "/regeneration",
		{ Pinstantmodels "/regen",
		  Pinstantmodels "/regen_ring",
		  NULL, NULL },
/* icon */ Picons "/regen",
/* pickup */ "Regeneration",
		30,
		IT_POWERUP,
		PW_REGEN,
/* precache */ "",
/* sounds */ Pitemsounds "/regen"
	},

/*QUAKED item_flight (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"item_flight",
		Pitemsounds "/flight",
		{ Pinstantmodels "/flight",
		  Pinstantmodels "/flight_ring",
		  NULL, NULL },
/* icon */ Picons "/flight",
/* pickup */ "Flight",
		60,
		IT_POWERUP,
		PW_FLIGHT,
/* precache */ "",
/* sounds */ Pitemsounds "/flight"
	},

/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
 * Only in CTF games
 */
	{
		"team_CTF_redflag",
		NULL,
		{ Pflagmodels "/r_flag",
		  NULL, NULL, NULL },
/* icon */ Picons "/iconf_red1",
/* pickup */ "Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
 * Only in CTF games
 */
	{
		"team_CTF_blueflag",
		NULL,
		{ Pflagmodels "/b_flag",
		  NULL, NULL, NULL },
/* icon */ Picons "/iconf_blu1",
/* pickup */ "Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* precache */ "",
/* sounds */ ""
	},

#ifdef MISSIONPACK
/*QUAKED holdable_kamikaze (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"holdable_kamikaze",
		Pitemsounds "/holdable",
		{ Pholdablemodels "/kamikazi",
		  NULL, NULL, NULL},
/* icon */ Picons "/kamikaze",
/* pickup */ "Kamikaze",
		60,
		IT_HOLDABLE,
		HI_KAMIKAZE,
/* precache */ "",
/* sounds */ Pitemsounds "/kamikazerespawn"
	},

/*QUAKED holdable_portal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"holdable_portal",
		Pitemsounds "/holdable",
		{ Pholdablemodels "/porter",
		  NULL, NULL, NULL},
/* icon */ Picons "/portal",
/* pickup */ "Portal",
		60,
		IT_HOLDABLE,
		HI_PORTAL,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"holdable_invulnerability",
		Pitemsounds "/holdable",
		{ Pholdablemodels "/invulnerability",
		  NULL, NULL, NULL},
/* icon */ Picons "/invulnerability",
/* pickup */ "Invulnerability",
		60,
		IT_HOLDABLE,
		HI_INVULNERABILITY,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_nails (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_nails",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/nailgunam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_nailgun",
/* pickup */ "Nails",
		20,
		IT_AMMO,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_mines",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/proxmineam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_proxlauncher",
/* pickup */ "Proximity Mines",
		10,
		IT_AMMO,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_belt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_belt",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/chaingunam",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_chaingun",
/* pickup */ "Chaingun Belt",
		100,
		IT_AMMO,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ ""
	},

	/*
	 * PERSISTANT POWERUP ITEMS
	 *  */
/*QUAKED item_scout (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
 */
	{
		"item_scout",
		Pitemsounds "/scout",
		{ Pinstantmodels "/scout",
		  NULL, NULL, NULL },
/* icon */ Picons "/scout",
/* pickup */ "Scout",
		30,
		IT_PERSISTANT_POWERUP,
		PW_SCOUT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_guard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
 */
	{
		"item_guard",
		Pitemsounds "/guard",
		{ Pinstantmodels "/guard",
		  NULL, NULL, NULL },
/* icon */ Picons "/guard",
/* pickup */ "Guard",
		30,
		IT_PERSISTANT_POWERUP,
		PW_GUARD,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
 */
	{
		"item_doubler",
		Pitemsounds "/doubler",
		{ Pinstantmodels "/doubler",
		  NULL, NULL, NULL },
/* icon */ Picons "/doubler",
/* pickup */ "Doubler",
		30,
		IT_PERSISTANT_POWERUP,
		PW_DOUBLER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
 */
	{
		"item_ammoregen",
		Pitemsounds "/ammoregen",
		{ Pinstantmodels "/ammo",
		  NULL, NULL, NULL },
/* icon */ Picons "/ammo_regen",
/* pickup */ "Ammo Regen",
		30,
		IT_PERSISTANT_POWERUP,
		PW_AMMOREGEN,
/* precache */ "",
/* sounds */ ""
	},

	/*QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
	 * Only in One Flag CTF games
	 */
	{
		"team_CTF_neutralflag",
		NULL,
		{ Pflagmodels "/n_flag",
		  NULL, NULL, NULL },
/* icon */ Picons "/iconf_neutral1",
/* pickup */ "Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_redcube",
		Pmiscsounds "/am_pkup",
		{ Ppowerupmodels "s/orb/r_orb",
		  NULL, NULL, NULL },
/* icon */ Picons "/iconh_rorb",
/* pickup */ "Red Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_bluecube",
		Pmiscsounds "/am_pkup",
		{ Ppowerupmodels "/orb/b_orb",
		  NULL, NULL, NULL },
/* icon */ Picons "/iconh_borb",
/* pickup */ "Blue Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED weapon_nailgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_nailgun",
		Pmiscsounds "/w_pkup",
		{ "models/weapons/nailgun/nailgun",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_nailgun",
/* pickup */ "Nailgun",
		10,
		IT_WEAPON,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_prox_launcher",
		Pmiscsounds "/w_pkup",
		{ Pproxmodels "/proxmine",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_proxlauncher",
/* pickup */ "Prox Launcher",
		5,
		IT_WEAPON,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ Pproxsounds "/wstbtick "
	     Pproxsounds "/wstbactv "
	     Pproxsounds "/wstbimpl "
	     Pproxsounds "/wstbimpm "
	     Pproxsounds "/wstbimpd "
	     Pproxsounds "/wstbactv"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_chaingun",
		Pmiscsounds "/w_pkup",
		{ Pgattlingmodels "/vulcan",
		  NULL, NULL, NULL},
/* icon */ Picons "/iconw_chaingun",
/* pickup */ "Chaingun",
		80,
		IT_WEAPON,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ Pgattlingsounds "/wvulwind"
	},
#endif

	/* end of list marker */
	{NULL}
};

int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;


/*
 * BG_FindItemForPowerup
 */
gitem_t *
BG_FindItemForPowerup(powerup_t pw)
{
	int i;

	for(i = 0; i < bg_numItems; i++)
		if((bg_itemlist[i].giType == IT_POWERUP ||
		    bg_itemlist[i].giType == IT_TEAM ||
		    bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) &&
		   bg_itemlist[i].giTag == pw)
			return &bg_itemlist[i];

	return NULL;
}


/*
 * BG_FindItemForHoldable
 */
gitem_t *
BG_FindItemForHoldable(holdable_t pw)
{
	int i;

	for(i = 0; i < bg_numItems; i++)
		if(bg_itemlist[i].giType == IT_HOLDABLE &&
		   bg_itemlist[i].giTag == pw)
			return &bg_itemlist[i];

	Com_Errorf(ERR_DROP, "HoldableItem not found");

	return NULL;
}


/*
 * BG_FindItemForWeapon
 *
 */
gitem_t *
BG_FindItemForWeapon(weapon_t weapon)
{
	gitem_t *it;

	for(it = bg_itemlist + 1; it->classname; it++)
		if(it->giType == IT_WEAPON && it->giTag == weapon)
			return it;

	Com_Errorf(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
 * BG_FindItem
 *
 */
gitem_t *
BG_FindItem(const char *pickupName)
{
	gitem_t *it;

	for(it = bg_itemlist + 1; it->classname; it++)
		if(!Q_stricmp(it->pickup_name, pickupName))
			return it;

	return NULL;
}

/*
 * BG_PlayerTouchesItem
 *
 * Items can be picked up without actually touching their physical bounds to make
 * grabbing them easier
 */
qbool
BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime)
{
	Vec3 origin;

	BG_EvaluateTrajectory(&item->pos, atTime, origin);

	/* we are ignoring ducked differences here */
	if(ps->origin[0] - origin[0] > 44
	   || ps->origin[0] - origin[0] < -50
	   || ps->origin[1] - origin[1] > 36
	   || ps->origin[1] - origin[1] < -36
	   || ps->origin[2] - origin[2] > 36
	   || ps->origin[2] - origin[2] < -36)
		return qfalse;

	return qtrue;
}



/*
 * BG_CanItemBeGrabbed
 *
 * Returns false if the item should not be picked up.
 * This needs to be the same for client side prediction and server use.
 */
qbool
BG_CanItemBeGrabbed(int gametype, const entityState_t *ent,
		    const playerState_t *ps)
{
	gitem_t *item;
#ifdef MISSIONPACK
	int upperBound;
#endif

	if(ent->modelindex < 1 || ent->modelindex >= bg_numItems)
		Com_Errorf(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");

	item = &bg_itemlist[ent->modelindex];

	switch(item->giType){
	case IT_WEAPON:
		return qtrue;	/* weapons are always picked up */

	case IT_AMMO:
		if(ps->ammo[ item->giTag ] >= 200)
			return qfalse;	/* can't hold any more */
		return qtrue;

	case IT_ARMOR:
#ifdef MISSIONPACK
		if(bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag ==
		   PW_SCOUT)
			return qfalse;

		/* we also clamp armor to the maxhealth for handicapping */
		if(bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag ==
		   PW_GUARD)
			upperBound = ps->stats[STAT_MAX_HEALTH];
		else
			upperBound = ps->stats[STAT_MAX_HEALTH] * 2;

		if(ps->stats[STAT_ARMOR] >= upperBound)
			return qfalse;

#else
		if(ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2)
			return qfalse;

#endif
		return qtrue;

	case IT_HEALTH:
		/* small and mega healths will go over the max, otherwise
		 * don't pick up if already at max */
#ifdef MISSIONPACK
		if(bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag ==
		   PW_GUARD)
			upperBound = ps->stats[STAT_MAX_HEALTH];
		else
#endif
		if(item->quantity == 5 || item->quantity == 100){
			if(ps->stats[STAT_HEALTH] >=
			   ps->stats[STAT_MAX_HEALTH] * 2)
				return qfalse;
			return qtrue;
		}

		if(ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
			return qfalse;
		return qtrue;

	case IT_POWERUP:
		return qtrue;	/* powerups are always picked up */

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		/* can only hold one item at a time */
		if(ps->stats[STAT_PERSISTANT_POWERUP])
			return qfalse;

		/* check team only */
		if((ent->generic1 & 2) && (ps->persistant[PERS_TEAM] != TEAM_RED))
			return qfalse;
		if((ent->generic1 & 4) &&
		   (ps->persistant[PERS_TEAM] != TEAM_BLUE))
			return qfalse;

		return qtrue;
#endif

	case IT_TEAM:	/* team items, such as flags */
#ifdef MISSIONPACK
		if(gametype == GT_1FCTF){
			/* neutral flag can always be picked up */
			if(item->giTag == PW_NEUTRALFLAG)
				return qtrue;
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->giTag == PW_BLUEFLAG  &&
				   ps->powerups[PW_NEUTRALFLAG])
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
				if(item->giTag == PW_REDFLAG  &&
				   ps->powerups[PW_NEUTRALFLAG])
					return qtrue;
		}
#endif
		if(gametype == GT_CTF){
			/* ent->modelindex2 is non-zero on items if they are dropped
			 * we need to know this because we can pick up our dropped flag (and return it)
			 * but we can't pick up our flag at base */
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->giTag == PW_BLUEFLAG ||
				   (item->giTag == PW_REDFLAG &&
				    ent->modelindex2) ||
				   (item->giTag == PW_REDFLAG &&
				    ps->powerups[PW_BLUEFLAG]))
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
				if(item->giTag == PW_REDFLAG ||
				   (item->giTag == PW_BLUEFLAG &&
				    ent->modelindex2) ||
				   (item->giTag == PW_BLUEFLAG &&
				    ps->powerups[PW_REDFLAG]))
					return qtrue;
		}

#ifdef MISSIONPACK
		if(gametype == GT_HARVESTER)
			return qtrue;

#endif
		return qfalse;

	case IT_HOLDABLE:
		/* can only hold one item at a time */
		if(ps->stats[STAT_HOLDABLE_ITEM])
			return qfalse;
		return qtrue;

	case IT_BAD:
		Com_Errorf(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
	default:
#ifndef Q3_VM
#ifndef NDEBUG
		Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n",
			item->giType);
#endif
#endif
		break;
	}

	return qfalse;
}

/* ====================================================================== */

/*
 * BG_EvaluateTrajectory
 *
 */
void
BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, Vec3 result)
{
	float	deltaTime;
	float	phase;

	switch(tr->trType){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		copyv3(tr->trBase, result);
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->trTime) * 0.001;	/* milliseconds to seconds */
		maddv3(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = sin(deltaTime * M_PI * 2);
		maddv3(tr->trBase, phase, tr->trDelta, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->trTime + tr->trDuration)
			atTime = tr->trTime + tr->trDuration;
		deltaTime = (atTime - tr->trTime) * 0.001;	/* milliseconds to seconds */
		if(deltaTime < 0)
			deltaTime = 0;
		maddv3(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001;	/* milliseconds to seconds */
		maddv3(tr->trBase, deltaTime, tr->trDelta, result);
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;	/* FIXME: local gravity... */
		break;
	default:
		Com_Errorf(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i",
			tr->trTime);
		break;
	}
}

/*
 * BG_EvaluateTrajectoryDelta
 *
 * For determining velocity at a given time
 */
void
BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, Vec3 result)
{
	float	deltaTime;
	float	phase;

	switch(tr->trType){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		clearv3(result);
		break;
	case TR_LINEAR:
		copyv3(tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime	= (atTime - tr->trTime) / (float)tr->trDuration;
		phase		= cos(deltaTime * M_PI * 2);	/* derivative of sin = cos */
		phase		*= 0.5;
		scalev3(tr->trDelta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->trTime + tr->trDuration){
			clearv3(result);
			return;
		}
		copyv3(tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001;	/* milliseconds to seconds */
		copyv3(tr->trDelta, result);
		result[2] -= DEFAULT_GRAVITY * deltaTime;	/* FIXME: local gravity... */
		break;
	default:
		Com_Errorf(ERR_DROP,
			"BG_EvaluateTrajectoryDelta: unknown trType: %i",
			tr->trTime);
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",	/* boing sound at origin", jump sound on player */

	"EV_JUMP",
	"EV_WATER_TOUCH",	/* foot touches */
	"EV_WATER_LEAVE",	/* foot leaves */
	"EV_WATER_UNDER",	/* head touches */
	"EV_WATER_CLEAR",	/* head leaves */

	"EV_ITEM_PICKUP",		/* normal item pickups are predictable */
	"EV_GLOBAL_ITEM_PICKUP",	/* powerup / team sounds are broadcast to everyone */

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",	/* eventParm will be the soundindex */

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",	/* no attenuation */
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",	/* otherEntity is the shooter */

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",	/* gib a previously living player */
	"EV_SCOREPLUM",		/* score plum */

/* #ifdef MISSIONPACK */
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",		/* kamikaze explodes */
	"EV_OBELISKEXPLODE",	/* obelisk explodes */
	"EV_OBELISKPAIN",	/* obelisk pain */
	"EV_INVUL_IMPACT",	/* invulnerability sphere impact */
	"EV_JUICED",		/* invulnerability juiced effect */
	"EV_LIGHTNINGBOLT",	/* lightning bolt bounced of invulnerability sphere */
/* #endif */

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL"

};

/*
 * BG_AddPredictableEventToPlayerstate
 *
 * Handles the sequence numbers
 */

void    trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer,
				       int bufsize);

void
BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm,
				    playerState_t *ps)
{

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if(atof(buf) != 0){
#ifdef QAGAME
			Com_Printf(
				" game event svt %5d -> %5d: num = %20s parm %d\n",
				ps->pmove_framecount /*ps->commandTime*/,
				ps->eventSequence, eventnames[newEvent],
				eventParm);
#else
			Com_Printf(
				"Cgame event svt %5d -> %5d: num = %20s parm %d\n",
				ps->pmove_framecount /*ps->commandTime*/,
				ps->eventSequence, eventnames[newEvent],
				eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
 * BG_TouchJumpPad
 */
void
BG_TouchJumpPad(playerState_t *ps, entityState_t *jumppad)
{
	Vec3	angles;
	float	p;
	int	effectNum;

	/* spectators don't use jump pads */
	if(ps->pm_type != PM_NORMAL)
		return;

	/* flying characters don't hit bounce pads */
	if(ps->powerups[PW_FLIGHT])
		return;

	/* if we didn't hit this same jumppad the previous frame
	 * then don't play the event sound again if we are in a fat trigger */
	if(ps->jumppad_ent != jumppad->number){

		v3toeuler(jumppad->origin2, angles);
		p = fabs(norm180euler(angles[PITCH]));
		if(p < 45)
			effectNum = 0;
		else
			effectNum = 1;
		BG_AddPredictableEventToPlayerstate(EV_JUMP_PAD, effectNum, ps);
	}
	/* remember hitting this jumppad this frame */
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	/* give the player the velocity from the jumppad */
	copyv3(jumppad->origin2, ps->velocity);
}

/*
 * BG_PlayerStateToEntityState
 *
 * This is done after each set of usercmd_t on the server,
 * and after local prediction on the client
 */
void
BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qbool snap)
{
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR)
		s->eType = ET_INVISIBLE;
	else if(ps->stats[STAT_HEALTH] <= GIB_HEALTH)
		s->eType = ET_INVISIBLE;
	else
		s->eType = ET_PLAYER;

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	copyv3(ps->origin, s->pos.trBase);
	if(snap)
		snapv3(s->pos.trBase);
	/* set the trDelta for flag direction */
	copyv3(ps->velocity, s->pos.trDelta);

	s->apos.trType = TR_INTERPOLATE;
	copyv3(ps->viewangles, s->apos.trBase);
	if(snap)
		snapv3(s->apos.trBase);

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim	= ps->legsAnim;
	s->torsoAnim	= ps->torsoAnim;
	s->clientNum	= ps->clientNum;	/* ET_PLAYER looks here instead of at number */
	/* so corpses can also reference the proper config */
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0)
		s->eFlags |= EF_DEAD;
	else
		s->eFlags &= ~EF_DEAD;

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS)
			ps->entityEventSequence = ps->eventSequence -
						  MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] |
			   ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ps->powerups[ i ])
			s->powerups |= 1 << i;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
 * BG_PlayerStateToEntityStateExtraPolate
 *
 * This is done after each set of usercmd_t on the server,
 * and after local prediction on the client
 */
void
BG_PlayerStateToEntityStateExtraPolate(playerState_t *ps, entityState_t *s,
				       int time,
				       qbool snap)
{
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR)
		s->eType = ET_INVISIBLE;
	else if(ps->stats[STAT_HEALTH] <= GIB_HEALTH)
		s->eType = ET_INVISIBLE;
	else
		s->eType = ET_PLAYER;

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	copyv3(ps->origin, s->pos.trBase);
	if(snap)
		snapv3(s->pos.trBase);
	/* set the trDelta for flag direction and linear prediction */
	copyv3(ps->velocity, s->pos.trDelta);
	/* set the time for linear prediction */
	s->pos.trTime = time;
	/* set maximum extra polation time */
	s->pos.trDuration = 50;	/* 1000 / sv_fps (default = 20) */

	s->apos.trType = TR_INTERPOLATE;
	copyv3(ps->viewangles, s->apos.trBase);
	if(snap)
		snapv3(s->apos.trBase);

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim	= ps->legsAnim;
	s->torsoAnim	= ps->torsoAnim;
	s->clientNum	= ps->clientNum;	/* ET_PLAYER looks here instead of at number */
	/* so corpses can also reference the proper config */
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0)
		s->eFlags |= EF_DEAD;
	else
		s->eFlags &= ~EF_DEAD;

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS)
			ps->entityEventSequence = ps->eventSequence -
						  MAX_PS_EVENTS;
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] |
			   ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ps->powerups[ i ])
			s->powerups |= 1 << i;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
