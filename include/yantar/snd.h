/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

void	sndinit(void);
void	sndshutdown(void);
/* if origin is NULL, the sound will be dynamically sourced from the entity */
void	sndstartsound(Vec3 origin, int entnum, int entchannel, Sfxhandle sfx);
void	sndstartlocalsound(Sfxhandle sfx, int channelNum);
void	sndstartbackgroundtrack(const char *intro, const char *loop);
void	sndstopbackgroundtrack(void);
/* 
 * cinematics and voice-over-network will send raw samples
 * 1.0 volume will be direct output of source samples 
 */
void	sndrawsamps(int stream, int samples, int rate, int width, int channels,
		const byte *data, float volume, int entityNum);
/* stop all sounds and the background track */
void	sndstopall(void);
/* all continuous looping sounds must be added before calling sndupdate */
void	sndclearloops(qbool killall);
void	sndaddloop(int entityNum, const Vec3 origin, const Vec3 velocity,
		Sfxhandle sfx);
void	sndaddrealloop(int entityNum, const Vec3 origin,
		const Vec3 velocity,
		Sfxhandle sfx);
void	sndstoploop(int entityNum);
/* 
 * recompute the volumes for all running sounds
 * relative to the given entityNum/orientation 
 */
void	sndrespatialize(int entityNum, const Vec3 origin, Vec3 axis[3],
		int inwater);
/* let the sound system know where an entity currently is */
void	sndupdateentpos(int entityNum, const Vec3 origin);
void	sndupdate(void);
void	snddisablesounds(void);
void	sndbeginreg(void);
/* 
 * RegisterSound will allways return a valid sample, even if it
 * has to create a placeholder.  This prevents continuous filesystem
 * checks for missing files 
 */
Sfxhandle	sndregister(const char *sample, qbool compressed);
void	sndshowfreemem(void);
void	sndclearbuf(void);
void	SNDDMA_Activate(void);
void	sndupdatebackgroundtrack(void);
#ifdef USE_VOIP
void	sndstartcapture(void);
int	sndavailcapturesamps(void);
void	sndcapture(int samples, byte *data);
void	sndstopcapture(void);
void	sndmastergain(float gain);
#endif
