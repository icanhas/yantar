/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */


void S_Init(void);
void S_Shutdown(void);

/* if origin is NULL, the sound will be dynamically sourced from the entity */
void S_StartSound(Vec3 origin, int entnum, int entchannel, sfxHandle_t sfx);
void S_StartLocalSound(sfxHandle_t sfx, int channelNum);

void S_StartBackgroundTrack(const char *intro, const char *loop);
void S_StopBackgroundTrack(void);

/* cinematics and voice-over-network will send raw samples
 * 1.0 volume will be direct output of source samples */
void S_RawSamples(int stream, int samples, int rate, int width, int channels,
		  const byte *data, float volume, int entityNum);

/* stop all sounds and the background track */
void S_StopAllSounds(void);

/* all continuous looping sounds must be added before calling S_Update */
void S_ClearLoopingSounds(qbool killall);
void S_AddLoopingSound(int entityNum, const Vec3 origin, const Vec3 velocity,
		       sfxHandle_t sfx);
void S_AddRealLoopingSound(int entityNum, const Vec3 origin,
			   const Vec3 velocity,
			   sfxHandle_t sfx);
void S_StopLoopingSound(int entityNum);

/* recompute the reletive volumes for all running sounds
 * reletive to the given entityNum / orientation */
void S_Respatialize(int entityNum, const Vec3 origin, Vec3 axis[3],
		    int inwater);

/* let the sound system know where an entity currently is */
void S_UpdateEntityPosition(int entityNum, const Vec3 origin);

void S_Update(void);

void S_DisableSounds(void);

void S_BeginRegistration(void);

/* RegisterSound will allways return a valid sample, even if it
 * has to create a placeholder.  This prevents continuous filesystem
 * checks for missing files */
sfxHandle_t     S_RegisterSound(const char *sample, qbool compressed);

void S_DisplayFreeMemory(void);

void S_ClearSoundBuffer(void);

void SNDDMA_Activate(void);

void S_UpdateBackgroundTrack(void);


#ifdef USE_VOIP
void S_StartCapture(void);
int S_AvailableCaptureSamples(void);
void S_Capture(int samples, byte *data);
void S_StopCapture(void);
void S_MasterGain(float gain);
#endif
