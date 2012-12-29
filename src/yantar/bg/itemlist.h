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
	 * SHIELD
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
		IT_SHIELD,
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
		IT_SHIELD,
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
		IT_SHIELD,
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
		IT_PRIWEAP,
		W1melee,
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
		IT_PRIWEAP,
		W1shotgun,
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
		IT_PRIWEAP,
		W1machinegun,
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
		IT_SECWEAP,
		W2grenadelauncher,
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
		IT_SECWEAP,
		W2rocketlauncher,
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
		IT_PRIWEAP,
		W1lightning,
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
		IT_PRIWEAP,
		W1railgun,
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
		IT_PRIWEAP,
		W1plasmagun,
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
		IT_SECWEAP,
		W2bfg,
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
		IT_PRIWEAP,
		W1_GRAPPLING_HOOK,
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
		W1shotgun,
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
		W1machinegun,
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
		W2grenadelauncher,
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
		W1plasmagun,
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
		W1lightning,
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
		W2rocketlauncher,
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
		W1railgun,
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
		W2bfg,
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

/*QUAKED ammo_nanoids (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_nanoids",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/nanoids",
		  NULL, NULL, NULL},
/* icon */ Picons "/nanoidcannon",
/* pickup */ "Nanoids",
		20,
		IT_AMMO,
		W1nanoidcannon,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"ammo_mines",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/proxmines",
		  NULL, NULL, NULL},
/* icon */ Picons "/icona_proxlauncher",
/* pickup */ "Proximity Mines",
		10,
		IT_AMMO,
		W2proxlauncher,
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
		W1chaingun,
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

/*QUAKED weapon_nanoidcannon (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_nanoidcannon",
		Pmiscsounds "/w_pkup",
		{ Pnailgunmodels "/nanoidcannon",
		  NULL, NULL, NULL},
/* icon */ Picons "/nanoidcannon",
/* pickup */ "Nanoid Cannon",
		10,
		IT_PRIWEAP,
		W1nanoidcannon,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
 */
	{
		"weapon_prox_launcher",
		Pmiscsounds "/w_pkup",
		{ Pproxmodels "/prox",
		  NULL, NULL, NULL},
/* icon */ Picons "/icon_proxlauncher",
/* pickup */ "Prox Launcher",
		5,
		IT_SECWEAP,
		W2proxlauncher,
/* precache */ "",
/* sounds */ Pproxsounds "/tick "
	     Pproxsounds "/actv "
	     Pproxsounds "/impl "
	     Pproxsounds "/impm "
	     Pproxsounds "/bimpd "
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
		IT_PRIWEAP,
		W1chaingun,
/* precache */ "",
/* sounds */ Pgattlingsounds "/wvulwind"
	},
	
	{NULL}	/* end of list marker */
};

int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;
