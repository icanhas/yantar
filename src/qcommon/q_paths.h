/* 
 * Direct folder paths should not appear in modules' code; they should 
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

#define Pannounce	Pplayersounds "/announce"

#define Pbfgsounds		Pweapsounds "/bfg"
#define Pgattlingsounds	Pweapsounds "/vulcan"
#define Pgrenadesounds	Pweapsounds "/grenade"
#define Plgsounds		Pweapsounds "/lightning"
#define Pmgsounds		Pweapsounds "/machinegun"
#define Pmeleesounds	Pweapsounds "/melee"
#define Pplasmasounds	Pweapsounds "/plasma"
#define Pproxsounds	Pweapsounds "/proxmine"
#define Prailsounds		Pweapsounds "/rail"
#define Prlsounds		Pweapsounds "/rocket"
#define Pshotgunsounds	Pweapsounds "/shotgun"

/* 
 * vis (textures, models, etc.) 
 */
#define Pvis		"vis"

#define Pgraphics	Pvis "/graphics"
#define Pmodels	Pvis "/models"

#define Pmenuart	Pgraphics "/menu"

#define Pplayermodels	Pmodels "/players"
#define Pammomodels	Pmodels "/ammo"
#define Pweapmodels	Pmodels "/weaps"
#define Pweaphitmodels	Pmodels "/weaphits"

#define Pgattlingmodels	Pweapmodels "/vulcan"
#define Plgmodels		Pweapmodels "/lightning"
#define Pmgmodels		Pweapmodels "/machinegun"
#define Pmeleemodels	Pweapmodels "/melee"
#define Pnademodels	Pweapmodels "/grenade"
#define Pproxmodels	Pweapmodels "/proxmine"
#define Prlmodels		Pweapmodels "/rocket"
#define Pshotgunmodels	Pweapmodels "/shotgun"
