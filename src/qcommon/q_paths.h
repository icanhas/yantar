/* 
 * Folder paths should not appear directly in modules' code; they should 
 * appear here instead. 
 */

/* 
 * sound 
 */
#define Psound		"sound"

#define Pmusic		Psound "/music"
#define Pfeedback	Psound "/feedback"
#define Pplayersounds	Psound "/player"
#define Pweapsounds	Psound "/weapons"
#define Pmiscsounds	Psound "/misc"
#define Pitemsounds	Psound "/items"
#define Pteamsounds	Psound "/teamplay"
#define Pmoversounds	Psound "/movers"
#define Pworldsounds	Psound "/world"

#define Pannounce	Pplayersounds "/announce"

#define Pdoorsounds	Pmoversounds "/doors"
#define Pswitchsounds	Pmoversounds "/switches"
#define Pplatformsounds	Pmoversounds "/plats"

#define Pbfgsounds	Pweapsounds "/bfg"
#define Pgattlingsounds	Pweapsounds "/vulcan"
#define Pgrenadesounds	Pweapsounds "/grenade"
#define Plgsounds	Pweapsounds "/lightning"
#define Pmgsounds	Pweapsounds "/machinegun"
#define Pmeleesounds	Pweapsounds "/melee"
#define Pplasmasounds	Pweapsounds "/plasma"
#define Pproxsounds	Pweapsounds "/proxmine"
#define Prailsounds	Pweapsounds "/rail"
#define Prlsounds	Pweapsounds "/rocket"
#define Pshotgunsounds	Pweapsounds "/shotgun"

/* 
 * vis (textures, models, etc.) 
 */
#define Pvis		"vis"

#define Pgraphics	Pvis ""
#define Pmodels		Pvis "/models"
#define Picons		Pvis "/icons"
#define Psprites	Pvis "/sprites"

#define Pmenuart	Pgraphics "/menu"
#define P2dart		Pgraphics "/2d"
#define Pdmgart	Pgraphics "/dmg"
#define Pmiscart	Pgraphics "/misc"
#define Pmedalart	Pgraphics	"/medals"

#define Pplayermodels	Pmodels "/players"
#define Pammomodels	Pmodels "/ammo"
#define Pweapmodels	Pmodels "/weaps"
#define Pweaphitmodels	Pmodels "/weaphits"
#define Ppowerupmodels	Pmodels "/powerups"
#define Pflagmodels		Pmodels "/flags"
#define Pobjectmodels	Pmodels "/mapobjects"
#define Pgibmodels		Pmodels "/gibs"
#define Pmiscmodels	Pmodels "/misc"

#define Pgattlingmodels	Pweapmodels "/vulcan"
#define Plgmodels	Pweapmodels "/lightning"
#define Pmgmodels	Pweapmodels "/machinegun"
#define Pmeleemodels	Pweapmodels "/melee"
#define Pgrenademodels	Pweapmodels "/grenade"
#define Pproxmodels	Pweapmodels "/proxmine"
#define Prlmodels	Pweapmodels "/rocket"
#define Pshotgunmodels	Pweapmodels "/shotgun"
#define Pshellmodels	Pweapmodels "/shells"
#define Phookmodels	Pweapmodels "/hook"
#define Pplasmamodels	Pweapmodels "/plasma"
#define Prailmodels		Pweapmodels "/rail"
#define Pbfgmodels		Pweapmodels "/bfg"

#define Pflagbasemodels	Pobjectmodels "/flagbase"

#define Ptelemodels		Pmiscmodels ""

#define Pharvestermodels	Ppowerupmodels "/harvester"
#define Pobeliskmodels		Ppowerupmodels "/obelisk"
#define Parmormodels		Ppowerupmodels "/armor"
#define Pshieldmodels		Ppowerupmodels "/shield"
#define Phealthmodels		Ppowerupmodels "/health"
#define Pholdablemodels		Ppowerupmodels "/holdable"
#define Pinstantmodels		Ppowerupmodels "/instant"

/*
 * bytecode
 */
#define Pvmfiles			"vm"

