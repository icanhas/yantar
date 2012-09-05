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

/*****************************************************************************
* name:		be_aas_move.h
*
* desc:		AAS
*
* $Archive: /source/code/botlib/be_aas_move.h $
*
*****************************************************************************/

#ifdef AASINTERN
extern aas_settings_t aassettings;
#endif	/* AASINTERN */

/* movement prediction */
int AAS_PredictClientMovement(struct aas_clientmove_s *move,
			      int entnum, Vec3 origin,
			      int presencetype, int onground,
			      Vec3 velocity, Vec3 cmdmove,
			      int cmdframes,
			      int maxframes, float frametime,
			      int stopevent, int stopareanum, int visualize);
/* predict movement until bounding box is hit */
int AAS_ClientMovementHitBBox(struct aas_clientmove_s *move,
			      int entnum, Vec3 origin,
			      int presencetype, int onground,
			      Vec3 velocity, Vec3 cmdmove,
			      int cmdframes,
			      int maxframes, float frametime,
			      Vec3 mins, Vec3 maxs, int visualize);
/* returns true if on the ground at the given origin */
int AAS_OnGround(Vec3 origin, int presencetype, int passent);
/* returns true if swimming at the given origin */
int AAS_Swimming(Vec3 origin);
/* returns the jump reachability run start point */
void AAS_JumpReachRunStart(struct aas_reachability_s *reach, Vec3 runstart);
/* returns true if against a ladder at the given origin */
int AAS_AgainstLadder(Vec3 origin);
/* rocket jump Z velocity when rocket-jumping at origin */
float AAS_RocketJumpZVelocity(Vec3 origin);
/* bfg jump Z velocity when bfg-jumping at origin */
float AAS_BFGJumpZVelocity(Vec3 origin);
/* calculates the horizontal velocity needed for a jump and returns true this velocity could be calculated */
int AAS_HorizontalVelocityForJump(float zvel, Vec3 start, Vec3 end,
				  float *velocity);
/*  */
void AAS_SetMovedir(Vec3 angles, Vec3 movedir);
/*  */
int AAS_DropToFloor(Vec3 origin, Vec3 mins, Vec3 maxs);
/*  */
void AAS_InitSettings(void);
