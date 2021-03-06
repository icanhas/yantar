/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "local.h"
#include "codec.h"
#include "../client.h"

static void S_Update_(void);
static void S_Base_StopAllSounds(void);
static void S_Base_StopBackgroundTrack(void);

static Sndstream	*s_backgroundStream = NULL;
static char	s_backgroundLoop[MAX_QPATH];
/* static char	s_backgroundMusic[MAX_QPATH]; //TTimo: unused */

/* Internal sound data & structures */

/* only begin attenuating sound volumes when outside the FULLVOLUME range */
#define SOUND_FULLVOLUME	80
#define SOUND_ATTENUATE		0.0008f
#define LOOP_HASH 128

static int s_soundStarted;
static qbool s_soundMuted;
static int listener_number;
static Vec3 listener_origin;
static Vec3 listener_axis[3];
static Sfx *sfxHash[LOOP_HASH];
static Loopsnd loopSounds[MAX_GENTITIES];
static Channel *freelist = NULL;
Dma dma;
Channel s_channels[MAX_CHANNELS];
Channel	loop_channels[MAX_CHANNELS];
int numLoopChannels;
int s_soundtime;	/* sample PAIRS */
int s_paintedtime;	/* sample PAIRS */
/* 
 * MAX_SFX may be larger than MAX_SOUNDS because
 * of custom player sounds 
 */
#define MAX_SFX 4096
Sfx	s_knownSfx[MAX_SFX];
int	s_numSfx = 0;
int s_rawend[MAX_RAW_STREAMS];
Samppair s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];

Cvar *s_testsound;
Cvar *s_show;
Cvar *s_mixahead;
Cvar *s_mixPreStep;

static void
S_Base_SoundInfo(void)
{
	comprintf("----- Sound Info -----\n");
	if(!s_soundStarted)
		comprintf ("sound system not started\n");
	else{
		comprintf("%5d stereo\n", dma.channels - 1);
		comprintf("%5d samples\n", dma.samples);
		comprintf("%5d samplebits\n", dma.samplebits);
		comprintf("%5d submission_chunk\n", dma.submission_chunk);
		comprintf("%5d speed\n", dma.speed);
		comprintf("%p dma buffer\n", dma.buffer);
		if(s_backgroundStream)
			comprintf("Background file: %s\n", s_backgroundLoop);
		else
			comprintf("No background file.\n");
	}
	comprintf("----------------------\n");
}

#ifdef USE_VOIP
static void
S_Base_StartCapture(void)
{
	/* !!! FIXME: write me. */
}

static int
S_Base_AvailableCaptureSamples(void)
{
	/* !!! FIXME: write me. */
	return 0;
}

static void
S_Base_Capture(int samples, byte *data)
{
	/* !!! FIXME: write me. */
}

static void
S_Base_StopCapture(void)
{
	/* !!! FIXME: write me. */
}

static void
S_Base_MasterGain(float val)
{
	/* !!! FIXME: write me. */
}
#endif

static void
S_Base_SoundList(void)
{
	int	i;
	Sfx *sfx;
	int	size, total;
	char	type[4][16];
	char	mem[2][16];

	strcpy(type[0], "16bit");
	strcpy(type[1], "adpcm");
	strcpy(type[2], "daub4");
	strcpy(type[3], "mulaw");
	strcpy(mem[0], "paged out");
	strcpy(mem[1], "resident ");
	total = 0;
	for(sfx=s_knownSfx, i=0; i<s_numSfx; i++, sfx++){
		size = sfx->soundLength;
		total += size;
		comprintf("%6i[%s] : %s[%s]\n", size,
			type[sfx->soundCompressionMethod],
			sfx->soundName, mem[sfx->inMemory]);
	}
	comprintf ("Total resident: %i\n", total);
	sndshowfreemem();
}

static void
S_ChannelFree(Channel *v)
{
	v->thesfx = NULL;
	*(Channel**)v = freelist;
	freelist = (Channel*)v;
}

static Channel*
S_ChannelMalloc(void)
{
	Channel *v;

	if(freelist == NULL)
		return NULL;
	v = freelist;
	freelist = *(Channel**)freelist;
	v->allocTime = commillisecs();
	return v;
}

static void
S_ChannelSetup(void)
{
	Channel *p, *q;

	Q_Memset(s_channels, 0, sizeof(s_channels));
	p = s_channels;;
	q = p + MAX_CHANNELS;
	while(--q > p)
		*(Channel**)q = q-1;
	*(Channel**)q = NULL;
	freelist = p + MAX_CHANNELS - 1;
	comdprintf("Channel memory manager started\n");
}

/* Load a sound */

/*
 * return a hash value for the sfx name
 */
static long
S_HashSFXName(const char *name)
{
	int	i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while(name[i] != '\0'){
		letter = tolower(name[i]);
		if(letter =='.') break;			/* don't include extension */
		if(letter =='\\') letter = '/';		/* damn path names */
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (LOOP_HASH-1);
	return hash;
}

/* Will allocate a new sfx if it isn't found */
static Sfx *
S_FindName(const char *name)
{
	int	i;
	int	hash;
	Sfx *sfx;

	if(!name)
		comerrorf (ERR_FATAL, "S_FindName: NULL");
	if(!name[0])
		comerrorf (ERR_FATAL, "S_FindName: empty name");
	if(strlen(name) >= MAX_QPATH)
		comerrorf (ERR_FATAL, "Sound name too long: %s", name);

	hash = S_HashSFXName(name);
	sfx = sfxHash[hash];
	/* see if already loaded */
	while(sfx){
		if(!Q_stricmp(sfx->soundName, name))
			return sfx;
		sfx = sfx->next;
	}

	/* find a free sfx */
	for(i=0; i < s_numSfx; i++)
		if(!s_knownSfx[i].soundName[0])
			break;
	if(i == s_numSfx){
		if(s_numSfx == MAX_SFX)
			comerrorf (ERR_FATAL, "S_FindName: out of Sfx");
		s_numSfx++;
	}

	sfx = &s_knownSfx[i];
	Q_Memset(sfx, 0, sizeof(*sfx));
	strcpy(sfx->soundName, name);
	sfx->next = sfxHash[hash];
	sfxHash[hash] = sfx;
	return sfx;
}

/*
 * Disables sounds until the next sndbeginreg.
 * This is called when the hunk is cleared and the sounds
 * are no longer valid.
 */
static void
S_Base_DisableSounds(void)
{
	S_Base_StopAllSounds();
	s_soundMuted = qtrue;
}

/* Creates a default buzz sound if the file can't be loaded */
static Handle
S_Base_RegisterSound(const char *name, qbool compressed)
{
	Sfx *sfx;

	compressed = qfalse;
	if(!s_soundStarted)
		return 0;
	if(strlen(name) >= MAX_QPATH){
		comprintf("Sound name exceeds MAX_QPATH\n");
		return 0;
	}

	sfx = S_FindName(name);
	if(sfx->soundData){
		if(sfx->defaultSound){
			comprintf(S_COLOR_YELLOW
				"WARNING: could not find %s - using default\n",
				sfx->soundName);
			return 0;
		}
		return sfx - s_knownSfx;
	}

	sfx->inMemory = qfalse;
	sfx->soundCompressed = compressed;
	S_memoryLoad(sfx);
	if(sfx->defaultSound){
		comprintf(
			S_COLOR_YELLOW
			"WARNING: could not find %s - using default\n",
			sfx->soundName);
		return 0;
	}
	return sfx - s_knownSfx;
}

static void
S_Base_BeginRegistration(void)
{
	s_soundMuted = qfalse;	/* we can play again */

	if(s_numSfx == 0){
		SND_setup();
		Q_Memset(s_knownSfx, '\0', sizeof(s_knownSfx));
		Q_Memset(sfxHash, '\0', sizeof(Sfx *) * LOOP_HASH);
		S_Base_RegisterSound(Psound "/default", qfalse);
	}
}

void
S_memoryLoad(Sfx *sfx)
{
	/* load the sound file */
	if(!S_LoadSound (sfx))
/*		comprintf( S_COLOR_YELLOW "WARNING: couldn't load sound: %s\n", sfx->soundName ); */
		sfx->defaultSound = qtrue;
	sfx->inMemory = qtrue;
}

/* Used for spatializing s_channels */
static void
S_SpatializeOrigin(Vec3 origin, int master_vol, int *left_vol, int *right_vol)
{
	Scalar	dot;
	Scalar	dist;
	Scalar	lscale, rscale, scale;
	Vec3	source_vec;
	Vec3	vec;
	const float dist_mult = SOUND_ATTENUATE;

	/* calculate stereo seperation and distance attenuation */
	subv3(origin, listener_origin, source_vec);
	dist = normv3(source_vec);
	dist -= SOUND_FULLVOLUME;
	if(dist < 0)
		dist = 0;	/* close enough to be at full volume */
	dist *= dist_mult;	/* different attenuation levels */
	rotv3(source_vec, listener_axis, vec);
	dot = -vec[1];

	if(dma.channels == 1){	/* no attenuation = no spatialization */
		rscale	= 1.0;
		lscale	= 1.0;
	}else{
		rscale	= 0.5 * (1.0 + dot);
		lscale	= 0.5 * (1.0 - dot);
		if(rscale < 0)
			rscale = 0;
		if(lscale < 0)
			lscale = 0;
	}

	/* add in distance effect */
	scale = (1.0 - dist) * rscale;
	*right_vol = (master_vol * scale);
	if(*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (master_vol * scale);
	if(*left_vol < 0)
		*left_vol = 0;
}

/* Start a sound effect */

/*
 * Validates the parms and ques the sound up
 * if pos is NULL, the sound will be dynamically sourced from the entity
 * Entchannel 0 will never override a playing sound
 */
static void
S_Base_StartSound(Vec3 origin, int entityNum, int entchannel,
		  Handle sfxHandle)
{
	Channel	*ch;
	Sfx		*sfx;
	int	i, oldest, chosen, time;
	int	inplay, allowed;

	if(!s_soundStarted || s_soundMuted)
		return;
	if(!origin && (entityNum < 0 || entityNum > MAX_GENTITIES))
		comerrorf(ERR_DROP, "sndstartsound: bad entitynum %i", entityNum);

	if(sfxHandle < 0 || sfxHandle >= s_numSfx){
		comprintf(S_COLOR_YELLOW "sndstartsound: handle %i out of range\n",
			sfxHandle);
		return;
	}

	sfx = &s_knownSfx[sfxHandle];
	if(sfx->inMemory == qfalse)
		S_memoryLoad(sfx);

	if(s_show->integer == 1)
		comprintf("%i : %s\n", s_paintedtime, sfx->soundName);

	time = commillisecs();

/*	comprintf("playing %s\n", sfx->soundName); */
	/* pick a channel to play on */

	allowed = 4;
	if(entityNum == listener_number)
		allowed = 8;

	ch = s_channels;
	inplay = 0;
	for(i = 0; i < MAX_CHANNELS; i++, ch++)
		if(ch->entnum == entityNum && ch->thesfx == sfx){
			if(time - ch->allocTime < 50)
/*                      if (cvargetf( "cg_showmiss" )) {
 *                              comprintf("double sound start\n");
 *                      } */
				return;
			inplay++;
		}

	if(inplay>allowed)
		return;

	sfx->lastTimeUsed = time;

	ch = S_ChannelMalloc();	/* entityNum, entchannel); */
	if(!ch){
		ch = s_channels;

		oldest	= sfx->lastTimeUsed;
		chosen	= -1;
		for(i = 0; i < MAX_CHANNELS; i++, ch++)
			if(ch->entnum != listener_number && ch->entnum ==
			   entityNum && ch->allocTime<oldest &&
			   ch->entchannel !=
			   CHAN_ANNOUNCER){
				oldest	= ch->allocTime;
				chosen	= i;
			}
		if(chosen == -1){
			ch = s_channels;
			for(i = 0; i < MAX_CHANNELS; i++, ch++)
				if(ch->entnum != listener_number &&
				   ch->allocTime<oldest && ch->entchannel !=
				   CHAN_ANNOUNCER){
					oldest	= ch->allocTime;
					chosen	= i;
				}
			if(chosen == -1){
				ch = s_channels;
				if(ch->entnum == listener_number){
					for(i = 0; i < MAX_CHANNELS; i++, ch++)
						if(ch->allocTime<oldest){
							oldest	= ch->allocTime;
							chosen	= i;
						}
				}
				if(chosen == -1){
					comprintf("dropping sound\n");
					return;
				}
			}
		}
		ch = &s_channels[chosen];
		ch->allocTime = sfx->lastTimeUsed;
	}

	if(origin){
		copyv3 (origin, ch->origin);
		ch->fixed_origin = qtrue;
	}else
		ch->fixed_origin = qfalse;

	ch->master_vol	= 127;
	ch->entnum	= entityNum;
	ch->thesfx	= sfx;
	ch->startSample = START_SAMPLE_IMMEDIATE;
	ch->entchannel	= entchannel;
	ch->leftvol	= ch->master_vol;	/* these will get calced at next spatialize */
	ch->rightvol	= ch->master_vol;	/* unless the game isn't running */
	ch->doppler	= qfalse;
}

static void
S_Base_StartLocalSound(Handle sfxHandle, int channelNum)
{
	if(!s_soundStarted || s_soundMuted)
		return;

	if(sfxHandle < 0 || sfxHandle >= s_numSfx){
		comprintf(
			S_COLOR_YELLOW
			"sndstartlocalsound: handle %i out of range\n",
			sfxHandle);
		return;
	}

	S_Base_StartSound (NULL, listener_number, channelNum, sfxHandle);
}

/*
 * If we are about to perform file access, clear the buffer
 * so sound doesn't stutter.
 */
static void
S_Base_ClearSoundBuffer(void)
{
	int clear;

	if(!s_soundStarted)
		return;

	/* stop looping sounds */
	Q_Memset(loopSounds, 0, MAX_GENTITIES*sizeof(Loopsnd));
	Q_Memset(loop_channels, 0, MAX_CHANNELS*sizeof(Channel));
	numLoopChannels = 0;

	S_ChannelSetup();
	Q_Memset(s_rawend, '\0', sizeof(s_rawend));

	if(dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	SNDDMA_BeginPainting();
	if(dma.buffer)
		Q_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit();
}

static void
S_Base_StopAllSounds(void)
{
	if(!s_soundStarted)
		return;

	/* stop the background music */
	S_Base_StopBackgroundTrack();

	S_Base_ClearSoundBuffer ();
}

/* continuous looping sounds are added each frame */

static void
S_Base_StopLoopingSound(int entityNum)
{
	loopSounds[entityNum].active = qfalse;
/*	loopSounds[entityNum].sfx = 0; */
	loopSounds[entityNum].kill = qfalse;
}

static void
S_Base_ClearLoopingSounds(qbool killall)
{
	int i;
	for(i = 0; i < MAX_GENTITIES; i++)
		if(killall || loopSounds[i].kill == qtrue ||
		   (loopSounds[i].sfx 
		   && loopSounds[i].sfx->soundLength == 0))
			S_Base_StopLoopingSound(i);
	numLoopChannels = 0;
}

/*
 * Called during entity generation for a frame
 * Include velocity in case I get around to doing doppler...
 */
static void
S_Base_AddLoopingSound(int entityNum, const Vec3 origin, const Vec3 velocity,
		       Handle sfxHandle)
{
	Sfx *sfx;

	if(!s_soundStarted || s_soundMuted)
		return;
	if(sfxHandle < 0 || sfxHandle >= s_numSfx){
		comprintf(
			S_COLOR_YELLOW
			"sndaddloop: handle %i out of range\n",
			sfxHandle);
		return;
	}
	sfx = &s_knownSfx[sfxHandle];
	if(sfx->inMemory == qfalse)
		S_memoryLoad(sfx);
	if(!sfx->soundLength)
		comerrorf(ERR_DROP, "%s has length 0", sfx->soundName);

	copyv3(origin, loopSounds[entityNum].origin);
	copyv3(velocity, loopSounds[entityNum].velocity);
	loopSounds[entityNum].active = qtrue;
	loopSounds[entityNum].kill = qtrue;
	loopSounds[entityNum].doppler = qfalse;
	loopSounds[entityNum].oldDopplerScale = 1.0;
	loopSounds[entityNum].dopplerScale = 1.0;
	loopSounds[entityNum].sfx = sfx;

	if(s_doppler->integer && lensqrv3(velocity)>0.0){
		Vec3	out;
		float	lena, lenb;

		loopSounds[entityNum].doppler = qtrue;
		lena = distsqrv3(loopSounds[listener_number].origin,
			loopSounds[entityNum].origin);
		addv3(loopSounds[entityNum].origin,
			loopSounds[entityNum].velocity,
			out);
		lenb = distsqrv3(loopSounds[listener_number].origin, out);
		if((loopSounds[entityNum].framenum+1) != cls.framecount)
			loopSounds[entityNum].oldDopplerScale = 1.0;
		else
			loopSounds[entityNum].oldDopplerScale =
				loopSounds[entityNum].dopplerScale;
		loopSounds[entityNum].dopplerScale = lenb/(lena*100);
		if(loopSounds[entityNum].dopplerScale<=1.0)
			loopSounds[entityNum].doppler = qfalse;		/* don't bother doing the math */
		else if(loopSounds[entityNum].dopplerScale>MAX_DOPPLER_SCALE)
			loopSounds[entityNum].dopplerScale = MAX_DOPPLER_SCALE;
	}

	loopSounds[entityNum].framenum = cls.framecount;
}

/*
 * Called during entity generation for a frame
 * Include velocity in case I get around to doing doppler...
 */
static void
S_Base_AddRealLoopingSound(int entityNum, const Vec3 origin,
			   const Vec3 velocity,
			   Handle sfxHandle)
{
	Sfx *sfx;

	if(!s_soundStarted || s_soundMuted)
		return;
	if(sfxHandle < 0 || sfxHandle >= s_numSfx){
		comprintf(
			S_COLOR_YELLOW
			"sndaddrealloop: handle %i out of range\n",
			sfxHandle);
		return;
	}
	sfx = &s_knownSfx[ sfxHandle ];
	if(sfx->inMemory == qfalse)
		S_memoryLoad(sfx);

	if(!sfx->soundLength)
		comerrorf(ERR_DROP, "%s has length 0", sfx->soundName);
	copyv3(origin, loopSounds[entityNum].origin);
	copyv3(velocity, loopSounds[entityNum].velocity);
	loopSounds[entityNum].sfx = sfx;
	loopSounds[entityNum].active = qtrue;
	loopSounds[entityNum].kill = qfalse;
	loopSounds[entityNum].doppler = qfalse;
}

/*
 * Spatialize all of the looping sounds.
 * All sounds are on the same cycle, so any duplicates can just
 * sum up the channel multipliers.
 */
static void
S_AddLoopSounds(void)
{
	int	i, j, time;
	int	left_total, right_total, left, right;
	Channel	*ch;
	Loopsnd *loop, *loop2;
	static int	loopFrame;


	numLoopChannels = 0;
	time = commillisecs();

	loopFrame++;
	for(i = 0; i < MAX_GENTITIES; i++){
		loop = &loopSounds[i];
		if(!loop->active || loop->mergeFrame == loopFrame)
			continue;	/* already merged into an earlier sound */

		if(loop->kill)
			S_SpatializeOrigin(loop->origin, 127, &left_total,
				&right_total);	/* 3d */
		else
			S_SpatializeOrigin(loop->origin, 90,  &left_total,
				&right_total);	/* sphere */

		loop->sfx->lastTimeUsed = time;

		for(j=(i+1); j< MAX_GENTITIES; j++){
			loop2 = &loopSounds[j];
			if(!loop2->active || loop2->doppler || loop2->sfx !=
			   loop->sfx)
				continue;
			loop2->mergeFrame = loopFrame;

			if(loop2->kill)
				S_SpatializeOrigin(loop2->origin, 127, &left,
					&right);	/* 3d */
			else
				S_SpatializeOrigin(loop2->origin, 90,  &left,
					&right);	/* sphere */

			loop2->sfx->lastTimeUsed = time;
			left_total += left;
			right_total += right;
		}
		if(left_total == 0 && right_total == 0)
			continue;	/* not audible */

		/* allocate a channel */
		ch = &loop_channels[numLoopChannels];

		if(left_total > 255)
			left_total = 255;
		if(right_total > 255)
			right_total = 255;

		ch->master_vol = 127;
		ch->leftvol	= left_total;
		ch->rightvol	= right_total;
		ch->thesfx	= loop->sfx;
		ch->doppler	= loop->doppler;
		ch->dopplerScale = loop->dopplerScale;
		ch->oldDopplerScale = loop->oldDopplerScale;
		numLoopChannels++;
		if(numLoopChannels == MAX_CHANNELS)
			return;
	}
}

/* Music streaming */
static void
S_Base_RawSamples(int stream, int samples, int rate, int width, int s_channels,
	const byte *data, float volume,
	 int entityNum)
{
	int	i;
	int	src, dst;
	float scale;
	int	intVolume;
	Samppair *rawsamples;

	if(!s_soundStarted || s_soundMuted)
		return;
	if(entityNum >= 0)
		return;
	if((stream < 0) || (stream >= MAX_RAW_STREAMS))
		return;

	rawsamples = s_rawsamples[stream];

	if(s_muted->integer)
		intVolume = 0;
	else
		intVolume = 256 * volume * s_volume->value;

	if(s_rawend[stream] < s_soundtime){
		comdprintf("S_Base_RawSamples: resetting minimum: %i < %i\n",
			s_rawend[stream],
			s_soundtime);
		s_rawend[stream] = s_soundtime;
	}

	scale = (float)rate / dma.speed;

/* comprintf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend[stream]); */
	if(s_channels == 2 && width == 2){
		if(scale == 1.0)	/* optimized case */
			for(i=0; i<samples; i++){
				dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
				s_rawend[stream]++;
				rawsamples[dst].left =
					((short*)data)[i*2] * intVolume;
				rawsamples[dst].right =
					((short*)data)[i*2+1] * intVolume;
			}
		else
			for(i=0;; i++){
				src = i*scale;
				if(src >= samples)
					break;
				dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
				s_rawend[stream]++;
				rawsamples[dst].left =
					((short*)data)[src*2] * intVolume;
				rawsamples[dst].right =
					((short*)data)[src*2+1] * intVolume;
			}
	}else if(s_channels == 1 && width == 2)
		for(i=0;; i++){
			src = i*scale;
			if(src >= samples)
				break;
			dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
			s_rawend[stream]++;
			rawsamples[dst].left = ((short*)data)[src] *
					       intVolume;
			rawsamples[dst].right = ((short*)data)[src] *
						intVolume;
		}
	else if(s_channels == 2 && width == 1){
		intVolume *= 256;

		for(i=0;; i++){
			src = i*scale;
			if(src >= samples)
				break;
			dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
			s_rawend[stream]++;
			rawsamples[dst].left =
				((char*)data)[src*2] * intVolume;
			rawsamples[dst].right =
				((char*)data)[src*2+1] * intVolume;
		}
	}else if(s_channels == 1 && width == 1){
		intVolume *= 256;

		for(i=0;; i++){
			src = i*scale;
			if(src >= samples)
				break;
			dst = s_rawend[stream]&(MAX_RAW_SAMPLES-1);
			s_rawend[stream]++;
			rawsamples[dst].left =
				(((byte*)data)[src]-128) * intVolume;
			rawsamples[dst].right =
				(((byte*)data)[src]-128) * intVolume;
		}
	}

	if(s_rawend[stream] > s_soundtime + MAX_RAW_SAMPLES)
		comdprintf("S_Base_RawSamples: overflowed %i > %i\n",
			s_rawend[stream], s_soundtime);
}

/* let the sound system know where an entity currently is */
static void
S_Base_UpdateEntityPosition(int entityNum, const Vec3 origin)
{
	if((entityNum < 0) || (entityNum >= MAX_GENTITIES))
		comerrorf(ERR_DROP, "sndupdateentpos: bad entitynum %i",
			entityNum);
	copyv3(origin, loopSounds[entityNum].origin);
}

/* Change the volumes of all the playing sounds for changes in their positions */
static void
S_Base_Respatialize(int entityNum, const Vec3 head, Vec3 axis[3],
		    int inwater)
{
	int i;
	Channel	*ch;
	Vec3		origin;
	
	UNUSED(inwater);

	if(!s_soundStarted || s_soundMuted)
		return;
	listener_number = entityNum;
	copyv3(head, listener_origin);
	copyv3(axis[0], listener_axis[0]);
	copyv3(axis[1], listener_axis[1]);
	copyv3(axis[2], listener_axis[2]);

	/* update spatialization for dynamic sounds */
	ch = s_channels;
	for(i = 0; i < MAX_CHANNELS; i++, ch++){
		if(!ch->thesfx)
			continue;
		/* anything coming from the view entity will always be full volume */
		if(ch->entnum == listener_number){
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		}else{
			if(ch->fixed_origin)
				copyv3(ch->origin, origin);
			else
				copyv3(loopSounds[ ch->entnum ].origin,
					origin);

			S_SpatializeOrigin (origin, ch->master_vol, &ch->leftvol,
				&ch->rightvol);
		}
	}
	S_AddLoopSounds ();
}

/* Returns qtrue if any new sounds were started since the last mix */
static qbool
S_ScanChannelStarts(void)
{
	Channel	*ch;
	int i;
	qbool		newSamples;

	newSamples = qfalse;
	ch = s_channels;
	for(i=0; i<MAX_CHANNELS; i++, ch++){
		if(!ch->thesfx)
			continue;
		/* if this channel was just started this frame,
		 * set the sample count to it begins mixing
		 * into the very first sample */
		if(ch->startSample == START_SAMPLE_IMMEDIATE){
			ch->startSample = s_paintedtime;
			newSamples = qtrue;
			continue;
		}
		/* if it is completely finished by now, clear it */
		if(ch->startSample + (ch->thesfx->soundLength) <= s_paintedtime)
			S_ChannelFree(ch);
	}

	return newSamples;
}

/* Called once each time through the main loop */
static void
S_Base_Update(void)
{
	int	i;
	int	total;
	Channel *ch;

	if(!s_soundStarted || s_soundMuted)
/*		comdprintf ("not started or muted\n"); */
		return;

	/*
	 * debugging output
	 */
	if(s_show->integer == 2){
		total = 0;
		ch = s_channels;
		for(i=0; i<MAX_CHANNELS; i++, ch++)
			if(ch->thesfx && (ch->leftvol || ch->rightvol)){
				comprintf ("%d %d %s\n", ch->leftvol,
					ch->rightvol,
					ch->thesfx->soundName);
				total++;
			}

		comprintf ("----(%i)---- painted: %i\n", total, s_paintedtime);
	}
	/* add raw data from streamed samples */
	sndupdatebackgroundtrack();
	/* mix some sound */
	S_Update_();
}

static void
S_GetSoundtime(void)
{
	int samplepos;
	static int	buffers;
	static int	oldsamplepos;
	int fullsamples;

	fullsamples = dma.samples / dma.channels;
	if(CL_VideoRecording( )){
		s_soundtime += (int)ceil(dma.speed / cl_aviFrameRate->value);
		return;
	}

	/* 
	 * it is possible to miscount buffers if it has wrapped twice between
	 * calls to sndupdate.  Oh well. 
	 */
	samplepos = SNDDMA_GetDMAPos();
	if(samplepos < oldsamplepos){
		buffers++;	/* buffer wrapped */

		if(s_paintedtime > 0x40000000){	/* time to chop things off to avoid 32 bit limits */
			buffers = 0;
			s_paintedtime = fullsamples;
			S_Base_StopAllSounds ();
		}
	}
	oldsamplepos = samplepos;
	s_soundtime = buffers*fullsamples + samplepos/dma.channels;
#if 0
	/* check to make sure that we haven't overshot */
	if(s_paintedtime < s_soundtime){
		comdprintf ("S_Update_ : overflow\n");
		s_paintedtime = s_soundtime;
	}
#endif
	if(dma.submission_chunk < 256)
		s_paintedtime = s_soundtime + s_mixPreStep->value * dma.speed;
	else
		s_paintedtime = s_soundtime + dma.submission_chunk;
}

static void
S_Update_(void)
{
	unsigned endtime;
	int samps;
	static float lastTime = 0.0f;
	float	ma, op;
	float	thisTime, sane;
	static int ot = -1;

	if(!s_soundStarted || s_soundMuted)
		return;
	thisTime = commillisecs();
	/* Updates s_soundtime */
	S_GetSoundtime();
	if(s_soundtime == ot)
		return;
	ot = s_soundtime;

	/* clear any sound effects that end before the current time,
	 * and start any new sounds */
	S_ScanChannelStarts();

	sane = thisTime - lastTime;
	if(sane<11)
		sane = 11;	/* 85hz */

	ma = s_mixahead->value * dma.speed;
	op = s_mixPreStep->value + sane*dma.speed*0.01;

	if(op < ma)
		ma = op;
	/* mix ahead of current position */
	endtime = s_soundtime + ma;
	/* mix to an even submission block size */
	endtime = (endtime + dma.submission_chunk-1)
		  & ~(dma.submission_chunk-1);
	/* never mix more than the complete buffer */
	samps = dma.samples >> (dma.channels-1);
	if(endtime - s_soundtime > samps)
		endtime = s_soundtime + samps;

	SNDDMA_BeginPainting();
	S_PaintChannels(endtime);
	SNDDMA_Submit();
	lastTime = thisTime;
}

/* background music functions */

static void
S_Base_StopBackgroundTrack(void)
{
	if(!s_backgroundStream)
		return;
	S_CodecCloseStream(s_backgroundStream);
	s_backgroundStream = NULL;
	s_rawend[0] = 0;
}

static void
S_Base_StartBackgroundTrack(const char *intro, const char *loop)
{
	if(!intro)
		intro = "";
	if(!loop || !loop[0])
		loop = intro;
	comdprintf("sndstartbackgroundtrack( %s, %s )\n", intro, loop);

	if(!*intro){
		S_Base_StopBackgroundTrack();
		return;
	}

	if(!loop)
		s_backgroundLoop[0] = 0;
	else
		Q_strncpyz(s_backgroundLoop, loop, sizeof(s_backgroundLoop));

	/* close the background track, but DON'T reset s_rawend
	 * if restarting the same back ground track */
	if(s_backgroundStream){
		S_CodecCloseStream(s_backgroundStream);
		s_backgroundStream = NULL;
	}

	/* Open stream */
	s_backgroundStream = S_CodecOpenStream(intro);
	if(!s_backgroundStream){
		comprintf(S_COLOR_YELLOW "WARNING: couldn't open music file %s\n",
			intro);
		return;
	}

	if(s_backgroundStream->info.channels != 2 ||
	   s_backgroundStream->info.rate != 22050)
		comprintf(S_COLOR_YELLOW
			"WARNING: music file %s is not 22k stereo\n",
			intro);
}

void
sndupdatebackgroundtrack(void)
{
	int	bufferSamples;
	int	fileSamples;
	byte	raw[30000];	/* just enough to fit in a mac stack frame */
	int	fileBytes;
	int	r;

	if(!s_backgroundStream)
		return;
	if(s_musicVolume->value <= 0)
		return;

	/* see how many samples should be copied into the raw buffer */
	if(s_rawend[0] < s_soundtime)
		s_rawend[0] = s_soundtime;
	while(s_rawend[0] < s_soundtime + MAX_RAW_SAMPLES){
		bufferSamples = MAX_RAW_SAMPLES - (s_rawend[0] - s_soundtime);

		/* decide how much data needs to be read from the file */
		fileSamples = bufferSamples * s_backgroundStream->info.rate /
			      dma.speed;

		if(!fileSamples)
			return;

		/* our max buffer size */
		fileBytes = fileSamples *
			    (s_backgroundStream->info.width *
			     s_backgroundStream->info.channels);
		if(fileBytes > sizeof(raw)){
			fileBytes = sizeof(raw);
			fileSamples = fileBytes /
				      (s_backgroundStream->info.width *
					   s_backgroundStream->info.channels);
		}

		r = S_CodecReadStream(s_backgroundStream, fileBytes, raw);
		if(r < fileBytes){
			fileBytes = r;
			fileSamples = r / (s_backgroundStream->info.width *
					   s_backgroundStream->info.channels);
		}

		if(r > 0)
			/* add to raw buffer */
			S_Base_RawSamples(0, fileSamples,
				s_backgroundStream->info.rate,
				s_backgroundStream->info.width,
				s_backgroundStream->info.channels, raw,
				s_musicVolume->value,
				-1);
		else{
			/* loop */
			if(s_backgroundLoop[0]){
				S_CodecCloseStream(s_backgroundStream);
				s_backgroundStream = NULL;
				S_Base_StartBackgroundTrack(s_backgroundLoop,
					s_backgroundLoop);
				if(!s_backgroundStream)
					return;
			}else{
				S_Base_StopBackgroundTrack();
				return;
			}
		}

	}
}

void
S_FreeOldestSound(void)
{
	int i, oldest, used;
	Sfx *sfx;
	sndBuffer *buffer, *nbuffer;

	oldest = commillisecs();
	used = 0;
	for(i=1; i < s_numSfx; i++){
		sfx = &s_knownSfx[i];
		if(sfx->inMemory && sfx->lastTimeUsed<oldest){
			used = i;
			oldest = sfx->lastTimeUsed;
		}
	}
	sfx = &s_knownSfx[used];
	comdprintf("S_FreeOldestSound: freeing sound %s\n", sfx->soundName);
	buffer = sfx->soundData;
	while(buffer != NULL){
		nbuffer = buffer->next;
		SND_free(buffer);
		buffer = nbuffer;
	}
	sfx->inMemory	= qfalse;
	sfx->soundData	= NULL;
}

/* shutdown sound engine */
static void
S_Base_Shutdown(void)
{
	if(!s_soundStarted)
		return;
	SNDDMA_Shutdown();
	SND_shutdown();
	s_soundStarted = 0;
	s_numSfx = 0;
	cmdremove("s_info");
}

qbool
S_Base_Init(Sndinterface *si)
{
	qbool r;

	if(!si)
		return qfalse;
	s_mixahead = cvarget("s_mixahead", "0.2", CVAR_ARCHIVE);
	s_mixPreStep = cvarget("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	s_show = cvarget("s_show", "0", CVAR_CHEAT);
	s_testsound = cvarget("s_testsound", "0", CVAR_CHEAT);
	r = SNDDMA_Init();

	if(r){
		s_soundStarted	= 1;
		s_soundMuted	= 1;
/*		s_numSfx = 0; */
		Q_Memset(sfxHash, 0, sizeof(Sfx *)*LOOP_HASH);
		s_soundtime = 0;
		s_paintedtime = 0;
		S_Base_StopAllSounds( );
	}else
		return qfalse;

	si->shutdown = S_Base_Shutdown;
	si->startsnd = S_Base_StartSound;
	si->startlocalsnd = S_Base_StartLocalSound;
	si->startbackgroundtrack = S_Base_StartBackgroundTrack;
	si->stopbackgroundtrack = S_Base_StopBackgroundTrack;
	si->rawsamps = S_Base_RawSamples;
	si->stopall = S_Base_StopAllSounds;
	si->clearloops = S_Base_ClearLoopingSounds;
	si->addloop = S_Base_AddLoopingSound;
	si->addrealloop = S_Base_AddRealLoopingSound;
	si->stoploops = S_Base_StopLoopingSound;
	si->respatialize = S_Base_Respatialize;
	si->updateentpos = S_Base_UpdateEntityPosition;
	si->update = S_Base_Update;
	si->disablesnds = S_Base_DisableSounds;
	si->beginreg = S_Base_BeginRegistration;
	si->registersnd = S_Base_RegisterSound;
	si->clearsndbuf = S_Base_ClearSoundBuffer;
	si->sndinfo	= S_Base_SoundInfo;
	si->sndlist	= S_Base_SoundList;
#ifdef USE_VOIP
	si->startcapture = S_Base_StartCapture;
	si->availcapturesamps = S_Base_AvailableCaptureSamples;
	si->capture = S_Base_Capture;
	si->stopcapture = S_Base_StopCapture;
	si->mastergain	= S_Base_MasterGain;
#endif
	return qtrue;
}
