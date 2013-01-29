/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
 /*
  * FIXME: this is total bollocks
  */

#include "shared.h"
#include "local.h"
#include "codec.h"

#define DEF_COMSOUNDMEGS "8"

static sndBuffer *buffer = nil;
static sndBuffer *freelist = nil;
static int	inUse = 0;
static int	totalInUse = 0;

short *sfxScratchBuffer = nil;
Sfx *sfxScratchPointer = nil;
int sfxScratchIndex = 0;

void
SND_free(sndBuffer *v)
{
	*(sndBuffer**)v = freelist;
	freelist = (sndBuffer*)v;
	inUse += sizeof(sndBuffer);
}

sndBuffer*
SND_malloc(void)
{
	sndBuffer *v;
	
	while(freelist == nil)
		S_FreeOldestSound();
	inUse -= sizeof(sndBuffer);
	totalInUse += sizeof(sndBuffer);
	v = freelist;
	freelist = *(sndBuffer**)freelist;
	v->next = nil;
	return v;
}

void
SND_setup(void)
{
	sndBuffer	*p, *q;
	Cvar		*cv;
	int scs;

	cv = cvarget("com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE);
	scs = (cv->integer*1536);

	buffer = malloc(scs*sizeof(sndBuffer));
	/* allocate the stack based hunk allocator */
	sfxScratchBuffer = malloc(SND_CHUNK_SIZE * sizeof(short) * 4);	/* hunkalloc(SND_CHUNK_SIZE * sizeof(short) * 4); */
	sfxScratchPointer = nil;

	inUse	= scs*sizeof(sndBuffer);
	p	= buffer;;
	q	= p + scs;
	while(--q > p)
		*(sndBuffer**)q = q-1;
	*(sndBuffer**)q = nil;
	freelist = p + scs - 1;
	comprintf("Sound memory manager started\n");
}

void
SND_shutdown(void)
{
	free(sfxScratchBuffer);
	free(buffer);
}

/* resample/decimate to the current source rate */
static void
ResampleSfx(Sfx *sfx, int inrate, int inwidth, byte *data)
{
	int outcount, srcsample;
	int i, sample, samplefrac, fracstep, part;
	float stepscale;
	sndBuffer *chunk;

	stepscale = (float)inrate / dma.speed;	/* this is usually 0.5, 1, or 2 */

	outcount = sfx->soundLength / stepscale;
	sfx->soundLength = outcount;

	samplefrac = 0;
	fracstep = stepscale * 256;
	chunk = sfx->soundData;

	for(i=0; i<outcount; i++){
		srcsample = samplefrac >> 8;
		samplefrac += fracstep;
		if(inwidth == 2)
			sample = (((short*)data)[srcsample]);
		else
			sample = (int)((uchar)(data[srcsample]) - 128) << 8;
		part = (i & (SND_CHUNK_SIZE-1));
		if(part == 0){
			sndBuffer *newchunk;
			newchunk = SND_malloc();
			if(chunk == nil)
				sfx->soundData = newchunk;
			else
				chunk->next = newchunk;
			chunk = newchunk;
		}
		chunk->sndChunk[part] = sample;
	}
}

/*
 * The filename may be different than sfx->name in the case
 * of a forced fallback of a player specific sound
 */
qbool
S_LoadSound(Sfx *sfx)
{
	byte *data;
	Sndinfo info;
/*	int		size; */

	/* player specific sounds are never directly loaded */
	if(sfx->soundName[0] == '*')
		return qfalse;

	/* load it in */
	data = S_CodecLoad(sfx->soundName, &info);
	if(!data)
		return qfalse;

	if(info.width == 1)
		comdprintf(S_COLOR_YELLOW "WARNING: %s is a 8 bit sound file\n",
			sfx->soundName);

	if(info.rate != 22050)
		comdprintf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz sound file\n",
			sfx->soundName);

	sfx->lastTimeUsed = commillisecs()+1;
	
	sfx->soundCompressionMethod = 0;
	sfx->soundLength = info.samples;
	sfx->soundData = nil;
	ResampleSfx(sfx, info.rate, info.width, data + info.dataofs);

	hunkfreetemp(data);

	return qtrue;
}

void
S_DisplayFreeMemory(void)
{
	comprintf("%d bytes free sound buffer memory, %d total used\n", inUse,
		totalInUse);
}
