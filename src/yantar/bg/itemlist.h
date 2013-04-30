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

Gitem bg_itemlist[] =
{
	{
		nil,		/* classname */
		nil,		/* pickupsound */
		{ nil },	/* worldmodel[] */
		nil,		/* icon */
		nil,		/* pickupname */
		0,		/* quantity */
		0,		/* type */
		0,		/* tag */
		"",		/* precache */
		""		/* sounds */
	},	/* leave index 0 alone */

	/*
	 * shield
	 */
	/* QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_armor_shard",
		Pmiscsounds "/ar1_pkup",
		{ Parmormodels "/shard",
		Parmormodels "/shard_sphere" },
		Picons "/shieldcellssmall",
		"5 Shield Cells",
		50,
		IT_SHIELD,
		0,
		"",
		""
	},

	/* QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_armor_combat",
		Pmiscsounds "/ar2_pkup",
		{ Parmormodels "/armor_yel" },
		 Picons "/shieldcellslarge",
		 "50 Shield Cells",
		500,
		IT_SHIELD,
		0,
		 "",
		 ""
	},

	/* QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_armor_body",
		Pmiscsounds "/ar2_pkup",
		{ Parmormodels "/armor_red" },
		 Picons "/heavyshieldgen",
		 "Heavy Shield Gen",
		1000,
		IT_SHIELD,
		0,
		 "",
		 ""
	},

	/*
	 * health
	 */
	/* QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_health_small",
		Pitemsounds "/s_health",
		{ Phealthmodels "/small_cross",
		  Phealthmodels "/small_sphere" },
		 Picons "/armourshard",
		 "5 Armor",
		50,
		IT_HEALTH,
		0,
		 "",
		 ""
	},

	/* QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_health",
		Pitemsounds "/n_health",
		{ Phealthmodels "/medium_cross",
		  Phealthmodels "/medium_sphere" },
		 Picons "/armourmedium",
		 "25 Armor",
		250,
		IT_HEALTH,
		0,
		 "",
		 ""
	},

	/* QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_health_large",
		Pitemsounds "/l_health",
		{ Phealthmodels "/large_cross",
		  Phealthmodels "/large_sphere" },
		 Picons "/armourlarge",
		 "50 Armor",
		500,
		IT_HEALTH,
		0,
		 "",
		 ""
	},

	/* QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_health_mega",
		Pitemsounds "/m_health",
		{ Phealthmodels "/mega_cross",
		  Phealthmodels "/mega_sphere" },
		 Picons "/armourmega",
		 "Heavy Armor",
		1000,
		IT_HEALTH,
		0,
		 "",
		 ""
	},

	/*
	 * weapons
	 */
	/* QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_gauntlet",
		Pmiscsounds "/w_pkup",
		{ Pmeleemodels "/melee" },
		 Picons "/melee",
		 "Gauntlet",
		0,
		IT_PRIWEAP,
		Wmelee,
		 "",
		 ""
	},

	/* QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_shotgun",
		Pmiscsounds "/w_pkup",
		{ Pshotgunmodels "/shotgun" },
		 Picons "/shotgun",
		 "Shotgun",
		10,
		IT_PRIWEAP,
		Wshotgun,
		 "",
		 ""
	},

	/* QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_machinegun",
		Pmiscsounds "/w_pkup",
		{ Pmgmodels "/machinegun" },
		 Picons "/machinegun",
		 "Machinegun",
		200,
		IT_PRIWEAP,
		Wmachinegun,
		 "",
		 ""
	},

	/* QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_grenadelauncher",
		Pmiscsounds "/w_pkup",
		{ Pgrenademodels "/nadelauncher" },
		 Picons "/nadelauncher",
		 "Grenade Launcher",
		10,
		IT_SECWEAP,
		Wgrenadelauncher,
		 "",
		
		(Pgrenadesounds "/hgrenb1a " Pgrenadesounds "/hgrenb2a")
	},

	/* QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_rocketlauncher",
		Pmiscsounds "/w_pkup",
		{ Prlmodels "/rocketpodstrong" },
		 Picons "/rocket",
		 "Rocket Launcher",
		10,
		IT_SECWEAP,
		Wrocketlauncher,
		 "",
		 ""
	},

	/* QUAKED weapon_hominglauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_hominglauncher",
		Pweapsounds "/pickup",
		{ Prlmodels "/rocketpodweak" },
		 Picons "/homing",
		 "Homing Rocket Launcher",
		30,
		IT_SECWEAP,
		Whominglauncher,
		 "",
		 ""
	},

	/* QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_lightning",
		Pmiscsounds "/w_pkup",
		{ Plgmodels "/lightning" },
		Picons "/lightning",
		"Lightning Gun",
		100,
		IT_PRIWEAP,
		Wlightning,
		"",
		""
	},

	/* QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_railgun",
		Pmiscsounds "/w_pkup",
		{ Prailmodels "/railgun" },
		 Picons "/railgun",
		 "Railgun",
		10,
		IT_PRIWEAP,
		Wrailgun,
		 "",
		 ""
	},

	/* QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_plasmagun",
		Pmiscsounds "/w_pkup",
		{ Pplasmamodels "/plasma" },
		 Picons "/plasmagun",
		 "Plasma Gun",
		50,
		IT_PRIWEAP,
		Wplasmagun,
		 "",
		 ""
	},

	/* QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_grapplinghook",
		Pmiscsounds "/w_pkup",
		{ Phookmodels "/grapple" },
		 Picons "/hook",
		 "Grappling Hook",
		0,
		IT_PRIWEAP,
		Whook,
		 "",
		 ""
	},

	/*
	 * ammo
	 */
	/* QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_shells",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/shotgunshells" },
		 Picons "/shotgunshells",
		 "Shells",
		10,
		IT_AMMO,
		Wshotgun,
		 "",
		 ""
	},

	/* QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_bullets",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/rounds" },
		 Picons "/rounds",
		 "Rounds",
		50,
		IT_AMMO,
		Wmachinegun,
		 "",
		 ""
	},

	/* QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_grenades",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/grenades" },
		 Picons "/grenades",
		 "Grenades",
		5,
		IT_AMMO,
		Wgrenadelauncher,
		 "",
		 ""
	},

	/* QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_cells",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/plasma" },
		 Picons "/plasma",
		 "Cells",
		30,
		IT_AMMO,
		Wplasmagun,
		 "",
		 ""
	},

	/* QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_lightning",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/lightningcells" },
		 Picons "/lightningcells",
		 "Lightning Cells",
		60,
		IT_AMMO,
		Wlightning,
		 "",
		 ""
	},

	/* QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_rockets",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/rockets" },
		 Picons "/rockets",
		 "Rockets",
		5,
		IT_AMMO,
		Wrocketlauncher,
		 "",
		 ""
	},

	/* QUAKED ammo_homingrockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_homingrockets",
		Pweapsounds "/pickup",
		{ Pammomodels "/homingrockets" },
		 Picons "/homingrockets",
		 "Homing Rockets",
		5,
		IT_AMMO,
		Whominglauncher,
		 "",
		 ""
	},

	/* QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_slugs",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/slugs" },
		 Picons "/slugs",
		 "Slugs",
		10,
		IT_AMMO,
		Wrailgun,
		 "",
		 ""
	},

	/*
	 * holdable items
	 */
	/* QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"holdable_teleporter",
		Pitemsounds "/holdable",
		{ Pholdablemodels "/teleporter" },
		 Picons "/teleporter",
		 "Personal Teleporter",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
		 "",
		 ""
	},
	/* QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"holdable_medkit",
		Pitemsounds "/holdable",
		{ Pholdablemodels "/medkit",
		  Pholdablemodels "/medkit_sphere" },
		 Picons "/medkit",
		 "Medkit",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
		 "",
		 Pitemsounds "/use_medkit"
	},

	/*
	 * POWERUP ITEMS
	 *  */
	/* QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_quad",
		Pitemsounds "/quaddamage",
		{ Pinstantmodels "/quad",
		  Pinstantmodels "/quad_ring" },
		 Picons "/quad",
		 "Quad Damage",
		30,
		IT_POWERUP,
		PW_QUAD,
		 "",
		 (Pitemsounds "/damage2 " Pitemsounds "/damage3")
	},

	/* QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_enviro",
		Pitemsounds "/protect",
		{ Pinstantmodels "/enviro",
		  Pinstantmodels "/enviro_ring" },
		 Picons "/envirosuit",
		 "Battle Suit",
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
		 "",
		 (Pitemsounds "/airout " Pitemsounds "/protect3")
	},

	/* QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_haste",
		Pitemsounds "/haste",
		{ Pinstantmodels "/haste",
		  Pinstantmodels "/haste_ring" },
		 Picons "/haste",
		 "Speed",
		30,
		IT_POWERUP,
		PW_HASTE,
		 "",
		 ""
	},

	/* QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_invis",
		Pitemsounds "/invisibility",
		{ Pinstantmodels "/invis",
		  Pinstantmodels "/invis_ring" },
		 Picons "/invis",
		 "Invisibility",
		30,
		IT_POWERUP,
		PW_INVIS,
		 "",
		 ""
	},

	/* QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_regen",
		Pitemsounds "/regeneration",
		{ Pinstantmodels "/regen",
		  Pinstantmodels "/regen_ring" },
		 Picons "/regen",
		 "Regeneration",
		30,
		IT_POWERUP,
		PW_REGEN,
		 "",
		 Pitemsounds "/regen"
	},

	/* QUAKED item_flight (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"item_flight",
		Pitemsounds "/flight",
		{ Pinstantmodels "/flight",
		  Pinstantmodels "/flight_ring" },
		 Picons "/flight",
		 "Flight",
		60,
		IT_POWERUP,
		PW_FLIGHT,
		 "",
		 Pitemsounds "/flight"
	},

	/* QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
	 * Only in CTF games
	 */
	{
		"team_CTF_redflag",
		nil,
		{ Pflagmodels "/redflag" },
		 Picons "/redflag",
		 "Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
		 "",
		 ""
	},

	/* QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
	 * Only in CTF games
	 */
	{
		"team_CTF_blueflag",
		nil,
		{ Pflagmodels "/blueflag" },
		 Picons "/blueflag",
		 "Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
		 "",
		 ""
	},

	/* QUAKED ammo_nanoids (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_nanoids",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/nanoids" },
		 Picons "/nanoids",
		 "Nanoids",
		20,
		IT_AMMO,
		Wnanoidcannon,
		 "",
		 ""
	},

	/* QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_mines",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/proxmines" },
		 Picons "/proxmines",
		 "Proximity Mines",
		10,
		IT_AMMO,
		Wproxlauncher,
		 "",
		 ""
	},

	/* QUAKED ammo_belt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"ammo_belt",
		Pmiscsounds "/am_pkup",
		{ Pammomodels "/herounds" },
		 Picons "/herounds",
		 "HE Rounds",
		100,
		IT_AMMO,
		Wchaingun,
		 "",
		 ""
	},

	/* QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
	 * Only in One Flag CTF games
	 */
	{
		"team_CTF_neutralflag",
		nil,
		{ Pflagmodels "/neutralflag" },
		 Picons "/neutralflag",
		 "Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
		 "",
		 ""
	},

	/* QUAKED weapon_nanoidcannon (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_nanoidcannon",
		Pmiscsounds "/w_pkup",
		{ Pnanoidmodels "/nanoidcannon" },
		 Picons "/nanoidcannon",
		 "Nanoid Cannon",
		10,
		IT_PRIWEAP,
		Wnanoidcannon,
		 "",
		 ""
	},

	/* QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_prox_launcher",
		Pmiscsounds "/w_pkup",
		{ Pproxmodels "/prox" },
		 Picons "/proxlauncher",
		 "Prox Launcher",
		5,
		IT_SECWEAP,
		Wproxlauncher,
		 "",
		 Pproxsounds "/tick "
	     Pproxsounds "/actv "
	     Pproxsounds "/impl "
	     Pproxsounds "/impm "
	     Pproxsounds "/bimpd "
	},

	/* QUAKED weapon_heminigun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended */
	{
		"weapon_heminigun",
		Pmiscsounds "/w_pkup",
		{ Phemgmodels "/heminigun" },
		 Picons "/heminigun",
		 "HE Minigun",
		80,
		IT_PRIWEAP,
		Wchaingun,
		 "",
		 Phemgsounds "/wvulwind"
	},
	
	{nil, nil, {nil}, nil, nil, 0, 0, 0, "", ""}	/* end of list marker */
};

int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;
