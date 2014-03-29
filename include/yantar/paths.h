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
#define Pweapsounds	Psound "/weaps"
#define Pmiscsounds	Psound "/misc"
#define Pitemsounds	Psound "/items"
#define Pteamsounds	Psound "/teamplay"
#define Pmoversounds	Psound "/movers"
#define Pworldsounds	Psound "/world"

#define Pannounce	Pplayersounds "/announce"

#define Pdoorsounds	Pmoversounds "/doors"
#define Pswitchsounds	Pmoversounds "/switches"
#define Pplatformsounds	Pmoversounds "/plats"

#define Phemgsounds	Pweapsounds "/heminigun"
#define Pgrenadesounds	Pweapsounds "/nadelauncher"
#define Plgsounds	Pweapsounds "/lightning"
#define Pmgsounds	Pweapsounds "/machinegun"
#define Pmeleesounds	Pweapsounds "/melee"
#define Pplasmasounds	Pweapsounds "/plasma"
#define Pproxsounds	Pweapsounds "/prox"
#define Pnanoidsounds	Pweapsounds "/nanoid"
#define Prailsounds	Pweapsounds "/rail"
#define Prlsounds	Pweapsounds "/rocket"
#define Pshotgunsounds	Pweapsounds "/shotgun"
#define Phooksounds	Pweapsounds "/hook"

/* 
 * vis (textures, models, etc.) 
 */
#define Pvis		"vis"

#define Pmodels		Pvis "/models"
#define Psprites	Pvis "/sprites"
#define Ptex		Pvis "/textures"

#define Pmenuart	Ptex "/menu"
#define P2dart		Ptex "/2d"
#define Pdmgart	Ptex "/dmg"
#define Pmiscart	Ptex "/misc"
#define Pmedalart	Ptex	"/medals"
#define Pxhairs	Ptex	"/xhairs"

#define Picons		Ptex "/icons"

#define Pplayermodels	Pmodels "/ships"
#define Pammomodels	Pmodels "/ammo"
#define Pweapmodels	Pmodels "/weaps"
#define Pweaphitmodels	Pmodels "/weaphits"
#define Ppowerupmodels	Pmodels "/powerups"
#define Pflagmodels		Pmodels "/flags"
#define Pobjectmodels	Pmodels "/mapobjects"
#define Pgibmodels		Pmodels "/gibs"
#define Pmiscmodels	Pmodels "/misc"
#define Pprojectilemodels	Pmodels "/projectiles"

#define Phemgmodels	Pweapmodels "/heminigun"
#define Plgmodels	Pweapmodels "/lightning"
#define Pmgmodels	Pweapmodels "/minigun"
#define Pmeleemodels	Pweapmodels "/melee"
#define Pgrenademodels	Pweapmodels "/nadelauncher"
#define Pproxmodels	Pweapmodels "/prox"
#define Pnanoidmodels	Pweapmodels "/nanoid"
#define Procketpodmodels	Pweapmodels "/rocketpod"
#define Pshotgunmodels	Pweapmodels "/shotgun"
#define Pshellmodels	Pweapmodels "/shells"
#define Phookmodels	Pweapmodels "/hook"
#define Pplasmamodels	Pweapmodels "/plasma"
#define Prailmodels		Pweapmodels "/rail"
#define Phookmodels	Pweapmodels "/hook"

#define Pflagbasemodels	Pobjectmodels "/flagbase"

#define Ptelemodels		Pmiscmodels ""

#define Parmormodels		Ppowerupmodels "/armor"
#define Pshieldmodels		Ppowerupmodels "/shield"
#define Phealthmodels		Ppowerupmodels "/health"
#define Pholdablemodels		Ppowerupmodels "/holdable"
#define Pinstantmodels		Ppowerupmodels "/instant"

/*
 * etc.
 */
#define Pvmfiles			"vm"
#define Pmaps				"maps"
#define Pvids				Pvis "/video"
