/* Definitions local to snd */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "snd.h"

#define PAINTBUFFER_SIZE	4096	/* this is in samples */

#define SND_CHUNK_SIZE		1024			/* samples */
#define SND_CHUNK_SIZE_FLOAT	(SND_CHUNK_SIZE/2)	/* floats */
#define SND_CHUNK_SIZE_BYTE	(SND_CHUNK_SIZE*2)	/* floats */

typedef struct {
	int	left;	/* the final values will be clamped to +/- 0x00ffff00 and shifted down */
	int	right;
} Samppair;

typedef struct sndBuffer_s {
	short		sndChunk[SND_CHUNK_SIZE];
	struct sndBuffer_s              *next;
	int		size;
} sndBuffer;

typedef struct Sfx {
	sndBuffer	*soundData;
	qbool		defaultSound;		/* couldn't be loaded, so use buzz */
	qbool		inMemory;		/* not in Memory */
	qbool		soundCompressed;	/* not in Memory */
	int		soundCompressionMethod;
	int		soundLength;
	char		soundName[MAX_QPATH];
	int		lastTimeUsed;
	struct Sfx	*next;
} Sfx;

typedef struct {
	int	channels;
	int	samples;		/* mono samples in buffer */
	int	submission_chunk;	/* don't mix less than this # */
	int	samplebits;
	int	speed;
	byte	*buffer;
} Dma;

#define START_SAMPLE_IMMEDIATE	0x7fffffff

#define MAX_DOPPLER_SCALE	50.0f	/* arbitrary */

typedef struct Loopsnd {
	Vec3		origin;
	Vec3		velocity;
	Sfx		*sfx;
	int		mergeFrame;
	qbool		active;
	qbool		kill;
	qbool		doppler;
	float		dopplerScale;
	float		oldDopplerScale;
	int		framenum;
} Loopsnd;

typedef struct {
	int		allocTime;
	int		startSample;	/* START_SAMPLE_IMMEDIATE = set immediately on next mix */
	int		entnum;		/* to allow overriding a specific sound */
	int		entchannel;	/* to allow overriding a specific sound */
	int		leftvol;	/* 0-255 volume after spatialization */
	int		rightvol;	/* 0-255 volume after spatialization */
	int		master_vol;	/* 0-255 volume before spatialization */
	float		dopplerScale;
	float		oldDopplerScale;
	Vec3		origin;		/* only use if fixed_origin is set */
	qbool		fixed_origin;	/* use origin instead of fetching entnum's origin */
	Sfx		*thesfx;	/* sfx structure */
	qbool		doppler;
} Channel;


#define WAV_FORMAT_PCM 1


typedef struct {
	int	format;
	int	rate;
	int	width;
	int	channels;
	int	samples;
	int	dataofs;	/* chunk starts this many bytes from file start */
} Wavinfo;

/* Interface between Q3 sound "api" and the sound backend */
typedef struct {
	void (*Shutdown)(void);
	void (*StartSound)(Vec3 origin, int entnum, int entchannel,
			   Sfxhandle sfx);
	void (*StartLocalSound)(Sfxhandle sfx, int channelNum);
	void (*StartBackgroundTrack)(const char *intro, const char *loop);
	void (*StopBackgroundTrack)(void);
	void (*RawSamples)(int stream, int samples, int rate, int width,
			   int channels, const byte *data, float volume,
			   int entityNum);
	void (*StopAllSounds)(void);
	void (*ClearLoopingSounds)(qbool killall);
	void (*AddLoopingSound)(int entityNum, const Vec3 origin,
				const Vec3 velocity, Sfxhandle sfx);
	void (*AddRealLoopingSound)(int entityNum, const Vec3 origin,
				    const Vec3 velocity, Sfxhandle sfx);
	void (*StopLoopingSound)(int entityNum);
	void (*Respatialize)(int entityNum, const Vec3 origin, Vec3 axis[3],
			     int inwater);
	void (*UpdateEntityPosition)(int entityNum, const Vec3 origin);
	void (*Update)(void);
	void (*DisableSounds)(void);
	void (*BeginRegistration)(void);
	Sfxhandle (*RegisterSound)(const char *sample, qbool compressed);
	void (*ClearSoundBuffer)(void);
	void (*SoundInfo)(void);
	void (*SoundList)(void);
#ifdef USE_VOIP
	void (*StartCapture)(void);
	int (*AvailableCaptureSamples)(void);
	void (*Capture)(int samples, byte *data);
	void (*StopCapture)(void);
	void (*MasterGain)(float gain);
#endif
} Sndinterface;


/*
 *
 * SYSTEM SPECIFIC FUNCTIONS
 *
 */

/* initializes cycling through a DMA buffer and returns information on it */
qbool SNDDMA_Init(void);

/* gets the current DMA position */
int             SNDDMA_GetDMAPos(void);

/* shutdown the DMA xfer. */
void    SNDDMA_Shutdown(void);

void    SNDDMA_BeginPainting(void);

void    SNDDMA_Submit(void);

/* ==================================================================== */

#define MAX_CHANNELS 96

extern Channel	s_channels[MAX_CHANNELS];
extern Channel	loop_channels[MAX_CHANNELS];
extern int	numLoopChannels;

extern int	s_paintedtime;
extern Vec3	listener_forward;
extern Vec3	listener_right;
extern Vec3	listener_up;
extern Dma	dma;

#define MAX_RAW_SAMPLES 16384
#define MAX_RAW_STREAMS (MAX_CLIENTS * 2 + 1)
extern Samppair s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];
extern int s_rawend[MAX_RAW_STREAMS];

extern Cvar *s_volume;
extern Cvar *s_musicVolume;
extern Cvar *s_muted;
extern Cvar *s_doppler;

extern Cvar *s_testsound;

qbool S_LoadSound(Sfx *sfx);

void            SND_free(sndBuffer *v);
sndBuffer*SND_malloc(void);
void            SND_setup(void);
void            SND_shutdown(void);

void S_PaintChannels(int endtime);

void S_memoryLoad(Sfx *sfx);

/* spatializes a channel */
void S_Spatialize(Channel *ch);

/* wavelet function */

#define SENTINEL_MULAW_ZERO_RUN		127
#define SENTINEL_MULAW_FOUR_BIT_RUN	126

void S_FreeOldestSound(void);

#define NXStream byte

void encodeWavelet(Sfx *sfx, short *packets);
void decodeWavelet(sndBuffer *stream, short *packets);

void encodeMuLaw(Sfx *sfx, short *packets);
extern short mulawToShort[256];

extern short	*sfxScratchBuffer;
extern Sfx    *sfxScratchPointer;
extern int	sfxScratchIndex;

qbool S_Base_Init(Sndinterface *si);
