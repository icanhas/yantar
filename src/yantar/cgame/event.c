/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* handle entity events at snapshot or playerstate transitions */

#include "local.h"

/*
 * CG_PlaceString
 *
 * Also called by scoreboard drawing
 */
const char      *
CG_PlaceString(int rank)
{
	static char str[64];
	char *s, *t;

	if(rank & RANK_TIED_FLAG){
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	}else
		t = "";

	if(rank == 1)
		s = S_COLOR_BLUE "1st" S_COLOR_WHITE;	/* draw in blue */
	else if(rank == 2)
		s = S_COLOR_RED "2nd" S_COLOR_WHITE;	/* draw in red */
	else if(rank == 3)
		s = S_COLOR_YELLOW "3rd" S_COLOR_WHITE;		/* draw in yellow */
	else if(rank == 11)
		s = "11th";
	else if(rank == 12)
		s = "12th";
	else if(rank == 13)
		s = "13th";
	else if(rank % 10 == 1)
		s = va("%ist", rank);
	else if(rank % 10 == 2)
		s = va("%ind", rank);
	else if(rank % 10 == 3)
		s = va("%ird", rank);
	else
		s = va("%ith", rank);

	Q_sprintf(str, sizeof(str), "%s%s", t, s);
	return str;
}

/*
 * CG_Obituary
 */
static void
CG_Obituary(Entstate *ent)
{
	int	mod;
	int	target, attacker;
	char	*message;
	char	*message2;
	const char	*targetInfo;
	const char	*attackerInfo;
	char	targetName[32];
	char	attackerName[32];
	Clientinfo *ci;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if(target < 0 || target >= MAX_CLIENTS)
		CG_Error("CG_Obituary: target out of range");
	ci = &cgs.clientinfo[target];

	if(attacker < 0 || attacker >= MAX_CLIENTS){
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
	}else
		attackerInfo = CG_ConfigString(CS_PLAYERS + attacker);

	targetInfo = CG_ConfigString(CS_PLAYERS + target);
	if(!targetInfo)
		return;
	Q_strncpyz(targetName,
		Info_ValueForKey(targetInfo, "n"), sizeof(targetName) - 2);
	strcat(targetName, S_COLOR_WHITE);

	message2 = "";

	/* check for single client messages */

	switch(mod){
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	default:
		message = NULL;
		break;
	}

	if(attacker == target){
		switch(mod){
		case MOD_GRENADE_SPLASH:
			message = "caught its own grenade";
			break;
		case MOD_ROCKET_SPLASH:
			message = "blew itself up";
			break;
		case MOD_PLASMA_SPLASH:
			message = "melted itself";
			break;
		case MOD_PROXIMITY_MINE:
			message = "found its prox mine";
			break;
		default:
			message = "killed itself";
			break;
		}
	}

	if(message){
		CG_Printf("%s %s.\n", targetName, message);
		return;
	}

	/* check for kill messages from the current clientNum */
	if(attacker == cg.snap->ps.clientNum){
		char *s;

		if(cgs.gametype < GT_TEAM)
			s = va("You fragged %s\n%s place with %i", targetName,
				CG_PlaceString(cg.snap->ps.persistant[PERS_RANK]
					+ 1),
				cg.snap->ps.persistant[PERS_SCORE]);
		else
			s = va("You fragged %s", targetName);

#ifdef MISSIONPACK
		if(!(cg_singlePlayerActive.integer && cg_cameraOrbit.integer))
			CG_CenterPrint(s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);

#else
		CG_CenterPrint(s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
#endif

		/* print the text message as well */
	}

	/* check for double client messages */
	if(!attackerInfo){
		attacker = ENTITYNUM_WORLD;
		strcpy(attackerName, "noname");
	}else{
		Q_strncpyz(attackerName, Info_ValueForKey(attackerInfo,
				"n"), sizeof(attackerName) - 2);
		strcat(attackerName, S_COLOR_WHITE);
		/* check for kill messages about the current clientNum */
		if(target == cg.snap->ps.clientNum)
			Q_strncpyz(cg.killerName, attackerName,
				sizeof(cg.killerName));
	}

	if(attacker != ENTITYNUM_WORLD){
		switch(mod){
		case MOD_GRAPPLE:
			message = "was caught by";
			break;
		case MOD_GAUNTLET:
			message = "was pummeled by";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_GRENADE:
			message = "ate";
			message2 = "'s grenade";
			break;
		case MOD_GRENADE_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_ROCKET_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_PLASMA:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_PLASMA_SPLASH:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_LIGHTNING:
			message = "was electrocuted by";
			break;
		case MOD_NANOID:
			message = "was wracked by";
			message2= "'s Nanoid Cannon";
			break;
		case MOD_CHAINGUN:
			message = "got lead poisoning from";
			message2 = "'s Chaingun";
			break;
		case MOD_PROXIMITY_MINE:
			message = "was too close to";
			message2 = "'s Prox Mine";
			break;
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
		default:
			message = "was killed by";
			break;
		}

		if(message){
			CG_Printf("%s %s %s%s\n",
				targetName, message, attackerName, message2);
			return;
		}
	}

	/* we don't know what it was */
	CG_Printf("%s died.\n", targetName);
}

/* ========================================================================== */

/*
 * CG_UseItem
 */
static void
CG_UseItem(Centity *cent)
{
	Clientinfo *ci;
	int itemNum, clientNum;
	Gitem *item;
	Entstate *es;

	es = &cent->currentState;

	itemNum = (es->event & ~EV_EVENT_BITS) - EV_USE_ITEM0;
	if(itemNum < 0 || itemNum > HI_NUM_HOLDABLE)
		itemNum = 0;

	/* print a message if the local player */
	if(es->number == cg.snap->ps.clientNum){
		if(!itemNum)
			CG_CenterPrint("No item to use", SCREEN_HEIGHT * 0.30,
				BIGCHAR_WIDTH);
		else{
			item = BG_FindItemForHoldable(itemNum);
			CG_CenterPrint(va("Use %s",
					item->pickupname), SCREEN_HEIGHT * 0.30,
				BIGCHAR_WIDTH);
		}
	}

	switch(itemNum){
	default:
	case HI_NONE:
		trap_sndstartsound (NULL, es->number, CHAN_BODY,
			cgs.media.useNothingSound);
		break;

	case HI_TELEPORTER:
		break;

	case HI_MEDKIT:
		clientNum = cent->currentState.clientNum;
		if(clientNum >= 0 && clientNum < MAX_CLIENTS){
			ci = &cgs.clientinfo[ clientNum ];
			ci->medkitUsageTime = cg.time;
		}
		trap_sndstartsound (NULL, es->number, CHAN_BODY,
			cgs.media.medkitSound);
		break;
	}

}

/*
 * A new item was picked up this frame
 */
static void
CG_ItemPickup(int itemNum)
{
	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;
	/* see if it should be the grabbed weapon */
	switch(bg_itemlist[itemNum].type){
	case IT_PRIWEAP:
		/* select it immediately */
		if(cg_autoswitch.integer && bg_itemlist[itemNum].tag 
		  != Wmachinegun)
		then{
			cg.weapseltime[WSpri] = cg.time;
			cg.weapsel[WSpri] = bg_itemlist[itemNum].tag;
		}
		break;
	case IT_SECWEAP:
		/* select it immediately */
		if(cg_autoswitch.integer && bg_itemlist[itemNum].tag 
		  != Wmachinegun)
		then{
			cg.weapseltime[WSsec] = cg.time;
			cg.weapsel[WSsec] = bg_itemlist[itemNum].tag;
		}
		break;
	default:
		break;
	}
}

/*
 * Returns waterlevel for entity origin
 */
int
CG_WaterLevel(Centity *cent)
{
	Vec3	point;
	int	contents, sample1, sample2, anim, waterlevel;

	/* get waterlevel, accounting for ducking */
	waterlevel = 0;
	copyv3(cent->lerpOrigin, point);
	point[2] += MinsZ + 1;
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		point[2] += CROUCH_VIEWHEIGHT;
	else
		point[2] += DEFAULT_VIEWHEIGHT;

	contents = CG_PointContents(point, -1);

	if(contents & MASK_WATER){
		sample2 = point[2] - MinsZ;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point[2] = cent->lerpOrigin[2] + MinsZ + sample1;
		contents = CG_PointContents(point, -1);

		if(contents & MASK_WATER){
			waterlevel = 2;
			point[2] = cent->lerpOrigin[2] + MinsZ + sample2;
			contents = CG_PointContents(point, -1);

			if(contents & MASK_WATER)
				waterlevel = 3;
		}
	}

	return waterlevel;
}

/*
 * CG_PainEvent
 *
 * Also called by playerstate transition
 */
void
CG_PainEvent(Centity *cent, int health)
{
	char *snd;

	/* don't do more than two pain sounds a second */
	if(cg.time - cent->pe.painTime < 500)
		return;

	if(health < 25)
		snd = "*pain25_1";
	else if(health < 50)
		snd = "*pain50_1";
	else if(health < 75)
		snd = "*pain75_1";
	else
		snd = "*pain100_1";
	/* play a gurp sound instead of a normal pain sound */
	if(CG_WaterLevel(cent) >= 1){
		if(rand()&1)
			trap_sndstartsound(
				NULL, cent->currentState.number, CHAN_VOICE,
				CG_CustomSound(cent->currentState.number,
					Pplayersounds "/gurp1"));
		else
			trap_sndstartsound(
				NULL, cent->currentState.number, CHAN_VOICE,
				CG_CustomSound(cent->currentState.number,
					Pplayersounds "/gurp2"));
	}else
		trap_sndstartsound(NULL, cent->currentState.number, CHAN_VOICE,
			CG_CustomSound(cent->currentState.number,
				snd));
	/* save pain time for programitic twitch animation */
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}



/*
 * CG_EntityEvent
 *
 * An entity has an event value
 * also called by CG_CheckPlayerstateEvents
 */
#define DEBUGNAME(x) if(cg_debugEvents.integer){CG_Printf(x "\n"); }
void
CG_EntityEvent(Centity *cent, Vec3 position)
{
	Entstate *es;
	int event;
	Vec3	dir;
	const char              *s;
	int	clientNum;
	Clientinfo *ci;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if(cg_debugEvents.integer)
		CG_Printf("ent:%3i  event:%3i ", es->number, event);

	if(!event){
		DEBUGNAME("ZEROEVENT");
		return;
	}

	clientNum = es->clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS)
		clientNum = 0;
	ci = &cgs.clientinfo[ clientNum ];

	switch(event){
	/*
	 * movement generated events
	 *  */
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if(cg_footsteps.integer)
			trap_sndstartsound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ ci->footsteps ][rand()&3]);
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if(cg_footsteps.integer)
			trap_sndstartsound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_METAL ][rand()&3]);
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if(cg_footsteps.integer)
			trap_sndstartsound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3]);
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if(cg_footsteps.integer)
			trap_sndstartsound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3]);
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if(cg_footsteps.integer)
			trap_sndstartsound (NULL, es->number, CHAN_BODY,
				cgs.media.footsteps[ FOOTSTEP_SPLASH ][rand()&3]);
		break;


	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.landSound);
		if(clientNum == cg.predictedPlayerState.clientNum){
			/* smooth landing z changes */
			cg.landChange	= -8;
			cg.landTime	= cg.time;
		}
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		/* use normal pain sound */
		trap_sndstartsound(NULL, es->number, CHAN_VOICE,
			CG_CustomSound(es->number, "*pain100_1"));
		if(clientNum == cg.predictedPlayerState.clientNum){
			/* smooth landing z changes */
			cg.landChange	= -16;
			cg.landTime	= cg.time;
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			CG_CustomSound(es->number, "*fall1"));
		cent->pe.painTime = cg.time;	/* don't play a pain sound right after this */
		if(clientNum == cg.predictedPlayerState.clientNum){
			/* smooth landing z changes */
			cg.landChange	= -24;
			cg.landTime	= cg.time;
		}
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:	/* smooth out step up transitions */
		DEBUGNAME("EV_STEP");
		{
			float	oldStep;
			int	delta;
			int	step;

			if(clientNum != cg.predictedPlayerState.clientNum)
				break;
			/* if we are interpolating, we don't need to smooth steps */
			if(cg.demoPlayback ||
			   (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
			   cg_nopredict.integer ||
			   cg_synchronousClients.integer)
				break;
			/* check for stepping up before a previous step is completed */
			delta = cg.time - cg.stepTime;
			if(delta < STEP_TIME)
				oldStep = cg.stepChange *
					  (STEP_TIME - delta) / STEP_TIME;
			else
				oldStep = 0;

			/* add this amount */
			step = 4 * (event - EV_STEP_4 + 1);
			cg.stepChange = oldStep + step;
			if(cg.stepChange > MAX_STEP_CHANGE)
				cg.stepChange = MAX_STEP_CHANGE;
			cg.stepTime = cg.time;
			break;
		}

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
/*		CG_Printf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm ); */
		{
			Vec3 up = {0, 0, 1};


			CG_SmokePuff(cent->lerpOrigin, up,
				32,
				1, 1, 1, 0.33f,
				1000,
				cg.time, 0,
				LEF_PUFF_DONT_SCALE,
				cgs.media.smokePuffShader);
		}

		/* boing sound at origin, jump sound on player */
		trap_sndstartsound (cent->lerpOrigin, -1, CHAN_VOICE,
			cgs.media.jumpPadSound);
		trap_sndstartsound (NULL, es->number, CHAN_VOICE,
			CG_CustomSound(es->number, "*jump1"));
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		trap_sndstartsound (NULL, es->number, CHAN_VOICE,
			CG_CustomSound(es->number, "*jump1"));
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		trap_sndstartsound (NULL, es->number, CHAN_VOICE,
			CG_CustomSound(es->number, "*taunt"));
		break;
	case EV_TAUNT_YES:
		DEBUGNAME("EV_TAUNT_YES");
		break;
	case EV_TAUNT_NO:
		DEBUGNAME("EV_TAUNT_NO");
		break;
	case EV_TAUNT_FOLLOWME:
		DEBUGNAME("EV_TAUNT_FOLLOWME");
		break;
	case EV_TAUNT_GETFLAG:
		DEBUGNAME("EV_TAUNT_GETFLAG");
		break;
	case EV_TAUNT_GUARDBASE:
		DEBUGNAME("EV_TAUNT_GUARDBASE");
		break;
	case EV_TAUNT_PATROL:
		DEBUGNAME("EV_TAUNT_PATROL");
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.watrInSound);
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.watrOutSound);
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.watrUnSound);
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			CG_CustomSound(es->number, "*gasp"));
		break;
	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			Gitem *item;
			int index;

			index = es->eventParm;	/* player predicted */

			if(index < 1 || index >= bg_numItems)
				break;
			item = &bg_itemlist[ index ];

			/* powerups and team items will have a separate global sound, this one
			 * will be played at prediction time */
			if(item->type == IT_POWERUP || item->type ==
			   IT_TEAM)
				trap_sndstartsound (NULL, es->number, CHAN_AUTO,
					cgs.media.n_healthSound);
			else if(item->type == IT_PERSISTANT_POWERUP){
				switch(item->tag){
				/* we have no persistent powerups in the game at present */
				default:
					break;
				}
			}else
				trap_sndstartsound (NULL, es->number, CHAN_AUTO,
					trap_sndregister(item->pickupsound,
						qfalse));

			/* show icon and name on status bar */
			if(es->number == cg.snap->ps.clientNum)
				CG_ItemPickup(index);
		}
		break;
	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			Gitem *item;
			int index;

			index = es->eventParm;	/* player predicted */

			if(index < 1 || index >= bg_numItems)
				break;
			item = &bg_itemlist[ index ];
			/* powerup pickups are global */
			if(item->pickupsound)
				trap_sndstartsound (
					NULL, cg.snap->ps.clientNum, CHAN_AUTO,
					trap_sndregister(item->pickupsound,
						qfalse));

			/* show icon and name on status bar */
			if(es->number == cg.snap->ps.clientNum)
				CG_ItemPickup(index);
		}
		break;
	/*
	 * weapon events
	 */
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
/*		trap_sndstartsound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound ); */
		if(es->number == cg.snap->ps.clientNum)
			CG_OutOfAmmoChange(WSpri);
		break;
	case EV_NOSECAMMO:
		DEBUGNAME("EV_NOSECAMMO");
		if(es->number == cg.snap->ps.clientNum)
			CG_OutOfAmmoChange(WSsec);
		break;
	case EV_CHANGE_WEAPON:
	case EV_CHANGESECWEAP:
		DEBUGNAME("EV_CHANGE_WEAPON");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.selectSound);
		break;
	case EV_FIREPRIWEAP:
		DEBUGNAME("EV_FIREPRIWEAP");
		CG_FireWeapon(cent, WSpri);
		break;
	case EV_FIRESECWEAP:
		DEBUGNAME("EV_FIRESECWEAP");
		CG_FireWeapon(cent, WSsec);
		break;
	case EV_FIREHOOK:
		DEBUGNAME("EV_FIREHOOK");
		CG_FireWeapon(cent, WShook);
		break;
	case EV_USE_ITEM0:
		DEBUGNAME("EV_USE_ITEM0");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM1:
		DEBUGNAME("EV_USE_ITEM1");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM2:
		DEBUGNAME("EV_USE_ITEM2");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM3:
		DEBUGNAME("EV_USE_ITEM3");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM4:
		DEBUGNAME("EV_USE_ITEM4");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM5:
		DEBUGNAME("EV_USE_ITEM5");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM6:
		DEBUGNAME("EV_USE_ITEM6");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM7:
		DEBUGNAME("EV_USE_ITEM7");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM8:
		DEBUGNAME("EV_USE_ITEM8");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM9:
		DEBUGNAME("EV_USE_ITEM9");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM10:
		DEBUGNAME("EV_USE_ITEM10");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM11:
		DEBUGNAME("EV_USE_ITEM11");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM12:
		DEBUGNAME("EV_USE_ITEM12");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM13:
		DEBUGNAME("EV_USE_ITEM13");
		CG_UseItem(cent);
		break;
	case EV_USE_ITEM14:
		DEBUGNAME("EV_USE_ITEM14");
		CG_UseItem(cent);
		break;
	/*
	 * other events
	 */
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.teleInSound);
		CG_SpawnEffect(position);
		break;
	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.teleOutSound);
		CG_SpawnEffect(position);
		break;
	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.respawnSound);
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->miscTime = cg.time;	/* scale up from this */
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.respawnSound);
		break;
	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		if(rand() & 1)
			trap_sndstartsound (NULL, es->number, CHAN_AUTO,
				cgs.media.hgrenb1aSound);
		else
			trap_sndstartsound (NULL, es->number, CHAN_AUTO,
				cgs.media.hgrenb2aSound);
		break;
	case EV_PROXIMITY_MINE_STICK:
		DEBUGNAME("EV_PROXIMITY_MINE_STICK");
		if(es->eventParm & SURF_FLESH)
			trap_sndstartsound (NULL, es->number, CHAN_AUTO,
				cgs.media.wstbimplSound);
		else if(es->eventParm & SURF_METALSTEPS)
			trap_sndstartsound (NULL, es->number, CHAN_AUTO,
				cgs.media.wstbimpmSound);
		else
			trap_sndstartsound (NULL, es->number, CHAN_AUTO,
				cgs.media.wstbimpdSound);
		break;
	case EV_PROXIMITY_MINE_TRIGGER:
		DEBUGNAME("EV_PROXIMITY_MINE_TRIGGER");
		trap_sndstartsound (NULL, es->number, CHAN_AUTO,
			cgs.media.wstbactvSound);
		break;
	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		CG_ScorePlum(cent->currentState.otherEntityNum, cent->lerpOrigin,
			cent->currentState.time);
		break;
	/*
	 * missile impacts
	 */
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir(es->eventParm, dir);
		CG_MissileHitPlayer(es->parentweap, position, dir,
			es->otherEntityNum);
		break;
	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir(es->eventParm, dir);
		CG_MissileHitWall(es->parentweap, 0, position, dir,
			IMPACTSOUND_DEFAULT);
		break;
	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		ByteToDir(es->eventParm, dir);
		CG_MissileHitWall(es->parentweap, 0, position, dir,
			IMPACTSOUND_METAL);
		break;
	case EV_RAILTRAIL:
		DEBUGNAME("EV_RAILTRAIL");
		cent->currentState.parentweap = Wrailgun;

		if(es->clientNum == cg.snap->ps.clientNum &&
		   !cg.renderingThirdPerson){
			if(cg_drawGun.integer == 2)
				saddv3(es->origin2, 8, cg.refdef.viewaxis[1],
					es->origin2);
			else if(cg_drawGun.integer == 3)
				saddv3(es->origin2, 4, cg.refdef.viewaxis[1],
					es->origin2);
		}

		CG_RailTrail(ci, es->origin2, es->traj.base);

		/* if the end was on a nomark surface, don't make an explosion */
		if(es->eventParm != 255){
			ByteToDir(es->eventParm, dir);
			CG_MissileHitWall(es->parentweap, es->clientNum, position,
				dir, IMPACTSOUND_DEFAULT);
		}
		break;
	case EV_BULLET_HIT_WALL:
		DEBUGNAME("EV_BULLET_HIT_WALL");
		ByteToDir(es->eventParm, dir);
		CG_Bullet(es->traj.base, es->otherEntityNum, dir, qfalse,
			ENTITYNUM_WORLD);
		break;
	case EV_BULLET_HIT_FLESH:
		DEBUGNAME("EV_BULLET_HIT_FLESH");
		CG_Bullet(es->traj.base, es->otherEntityNum, dir, qtrue,
			es->eventParm);
		break;
	case EV_SHOTGUN:
		DEBUGNAME("EV_SHOTGUN");
		CG_ShotgunFire(es);
		break;
	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if(cgs.gameSounds[es->eventParm])
			trap_sndstartsound(NULL, es->number, CHAN_VOICE,
				cgs.gameSounds[es->eventParm]);
		else{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			trap_sndstartsound(NULL, es->number, CHAN_VOICE,
				CG_CustomSound(es->number,
					s));
		}
		break;
	/* play from the player's head so it never diminishes */
	case EV_GLOBAL_SOUND:	
		DEBUGNAME("EV_GLOBAL_SOUND");
		if(cgs.gameSounds[es->eventParm])
			trap_sndstartsound(NULL, cg.snap->ps.clientNum,
				CHAN_AUTO,
				cgs.gameSounds[es->eventParm]);
		else{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			trap_sndstartsound(NULL, cg.snap->ps.clientNum,
				CHAN_AUTO, CG_CustomSound(es->number, s));
		}
		break;
	/* play from the player's head so it never diminishes */
	case EV_GLOBAL_TEAM_SOUND:	
	{
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		switch(es->eventParm){
		/* CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag */
		case GTS_RED_CAPTURE:	
			if(cgs.clientinfo[cg.clientNum].team == TEAM_RED)
				CG_AddBufferedSound(cgs.media.captureYourTeamSound);
			else
				CG_AddBufferedSound(cgs.media.captureOpponentSound);
			break;
		/* CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag */
		case GTS_BLUE_CAPTURE:	
			if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
				CG_AddBufferedSound(cgs.media.captureYourTeamSound);
			else
				CG_AddBufferedSound(cgs.media.captureOpponentSound);
			break;
		/* CTF: blue flag returned, 1FCTF: never used */
		case GTS_RED_RETURN:	
			if(cgs.clientinfo[cg.clientNum].team == TEAM_RED)
				CG_AddBufferedSound(cgs.media.returnYourTeamSound);
			else
				CG_AddBufferedSound(cgs.media.returnOpponentSound);
			CG_AddBufferedSound(cgs.media.blueFlagReturnedSound);
			break;
		/* CTF red flag returned, 1FCTF: neutral flag returned */
		case GTS_BLUE_RETURN:	
			if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
				CG_AddBufferedSound(cgs.media.returnYourTeamSound);
			else
				CG_AddBufferedSound(cgs.media.returnOpponentSound);
			CG_AddBufferedSound(cgs.media.redFlagReturnedSound);
			break;
		/* CTF: red team took blue flag, 1FCTF: blue team took the neutral flag */
		case GTS_RED_TAKEN:
			/* if this player picked up the flag then a sound is played in CG_CheckLocalSounds */
			if(cg.snap->ps.powerups[PW_BLUEFLAG] ||
			   cg.snap->ps.powerups[PW_NEUTRALFLAG]){
			}else{
				if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE){
					if(cgs.gametype == GT_1FCTF)
						CG_AddBufferedSound(cgs.media.yourTeamTookTheFlagSound);
					else
						CG_AddBufferedSound(cgs.media.enemyTookYourFlagSound);
				}else if(cgs.clientinfo[cg.clientNum].team == TEAM_RED){
					if(cgs.gametype == GT_1FCTF)
						CG_AddBufferedSound(cgs.media.enemyTookTheFlagSound);
					else
						CG_AddBufferedSound(cgs.media.yourTeamTookEnemyFlagSound);
				}
			}
			break;
		/* CTF: blue team took the red flag, 1FCTF red team took the neutral flag */
		case GTS_BLUE_TAKEN:
			/* if this player picked up the flag then a sound is played in CG_CheckLocalSounds */
			if(cg.snap->ps.powerups[PW_REDFLAG] ||
			   cg.snap->ps.powerups[PW_NEUTRALFLAG]){
			}else{
				if(cgs.clientinfo[cg.clientNum].team == TEAM_RED){
					if(cgs.gametype == GT_1FCTF)
						CG_AddBufferedSound(cgs.media.yourTeamTookTheFlagSound);
					else
					CG_AddBufferedSound(cgs.media.enemyTookYourFlagSound);
				}else if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE){
					if(cgs.gametype == GT_1FCTF)
						CG_AddBufferedSound(cgs.media.enemyTookTheFlagSound);
					else
					CG_AddBufferedSound(cgs.media.yourTeamTookEnemyFlagSound);
				}
			}
		case GTS_REDTEAM_SCORED:
			CG_AddBufferedSound(cgs.media.redScoredSound);
			break;
		case GTS_BLUETEAM_SCORED:
			CG_AddBufferedSound(cgs.media.blueScoredSound);
			break;
		case GTS_REDTEAM_TOOK_LEAD:
			CG_AddBufferedSound(cgs.media.redLeadsSound);
			break;
		case GTS_BLUETEAM_TOOK_LEAD:
			CG_AddBufferedSound(cgs.media.blueLeadsSound);
			break;
		case GTS_TEAMS_ARE_TIED:
			CG_AddBufferedSound(cgs.media.teamsTiedSound);
			break;
		default:
			break;
		}
		break;
	}

	case EV_PAIN:
		/* local player sounds are triggered in CG_CheckLocalSounds,
		 * so ignore events on the player */
		DEBUGNAME("EV_PAIN");
		if(cent->currentState.number != cg.snap->ps.clientNum)
			CG_PainEvent(cent, es->eventParm);
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");

		if(CG_WaterLevel(cent) >= 1)
			trap_sndstartsound(NULL, es->number, CHAN_VOICE,
				CG_CustomSound(es->number,
					"*drown"));
		else
			trap_sndstartsound(NULL, es->number, CHAN_VOICE,
				CG_CustomSound(es->number, va("*death%i",
						event - EV_DEATH1 + 1)));

		break;


	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		CG_Obituary(es);
		break;

	/*
	 * powerup events
	 *  */
	case EV_POWERUP_QUAD:
		DEBUGNAME("EV_POWERUP_QUAD");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupActive = PW_QUAD;
			cg.powerupTime = cg.time;
		}
		trap_sndstartsound (NULL, es->number, CHAN_ITEM,
			cgs.media.quadSound);
		break;
	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		trap_sndstartsound (NULL, es->number, CHAN_ITEM,
			cgs.media.protectSound);
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupActive = PW_REGEN;
			cg.powerupTime = cg.time;
		}
		trap_sndstartsound (NULL, es->number, CHAN_ITEM,
			cgs.media.regenSound);
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		/* don't play gib sound when using the kamikaze because it interferes
		 * with the kamikaze sound, downside is that the gib sound will also
		 * not be played when someone is gibbed while just carrying the kamikaze */
		if(!(es->eFlags & EF_KAMIKAZE))
			trap_sndstartsound(NULL, es->number, CHAN_BODY,
				cgs.media.gibSound);
		CG_GibPlayer(cent->lerpOrigin);
		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		trap_sndstoploop(es->number);
		es->loopSound = 0;
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_Beam(cent);
		break;

	default:
		DEBUGNAME("UNKNOWN");
		CG_Error("Unknown event: %i", event);
		break;
	}

}


/*
 * CG_CheckEvents
 *
 */
void
CG_CheckEvents(Centity *cent)
{
	/* check for event-only entities */
	if(cent->currentState.eType > ET_EVENTS){
		if(cent->previousEvent)
			return;		/* already fired */
		/* if this is a player event set the entity number of the client entity number */
		if(cent->currentState.eFlags & EF_PLAYER_EVENT)
			cent->currentState.number =
				cent->currentState.otherEntityNum;

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	}else{
		/* check for events riding with another entity */
		if(cent->currentState.event == cent->previousEvent)
			return;
		cent->previousEvent = cent->currentState.event;
		if((cent->currentState.event & ~EV_EVENT_BITS) == 0)
			return;
	}

	/* calculate the position at exactly the frame time */
	BG_EvaluateTrajectory(&cent->currentState.traj, cg.snap->serverTime,
		cent->lerpOrigin);
	CG_SetEntitySoundPosition(cent);

	CG_EntityEvent(cent, cent->lerpOrigin);
}
