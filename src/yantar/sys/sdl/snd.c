/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "shared.h"
#include "../../client/snd/local.h"	/* FIXME: leaking internals from client/snd */

qbool		snd_inited = qfalse;

Cvar		*s_sdlBits;
Cvar		*s_sdlSpeed;
Cvar		*s_sdlChannels;
Cvar		*s_sdlDevSamps;
Cvar		*s_sdlMixSamps;

/* The audio callback. All the magic happens here. */
static int	dmapos	= 0;
static int	dmasize = 0;

static void
SNDDMA_AudioCallback(void *userdata, Uint8 *stream, int len)
{
	int pos = (dmapos * (dma.samplebits/8));
	if(pos >= dmasize)
		dmapos = pos = 0;

	if(!snd_inited){	/* shouldn't happen, but just in case... */
		memset(stream, '\0', len);
		return;
	}else{
		int	tobufend = dmasize - pos;	/* bytes to buffer's end. */
		int	len1	= len;
		int	len2	= 0;

		if(len1 > tobufend){
			len1	= tobufend;
			len2	= len - len1;
		}
		memcpy(stream, dma.buffer + pos, len1);
		if(len2 <= 0)
			dmapos += (len1 / (dma.samplebits/8));
		else{	/* wraparound? */
			memcpy(stream+len1, dma.buffer, len2);
			dmapos = (len2 / (dma.samplebits/8));
		}
	}

	if(dmapos >= dmasize)
		dmapos = 0;
}

static struct {
	Uint16	enumFormat;
	char	*stringFormat;
} formatToStringTable[] =
{
	{ AUDIO_U8,     "AUDIO_U8" },
	{ AUDIO_S8,     "AUDIO_S8" },
	{ AUDIO_U16LSB, "AUDIO_U16LSB" },
	{ AUDIO_S16LSB, "AUDIO_S16LSB" },
	{ AUDIO_U16MSB, "AUDIO_U16MSB" },
	{ AUDIO_S16MSB, "AUDIO_S16MSB" }
};

static int formatToStringTableSize = ARRAY_LEN(formatToStringTable);

static void
SNDDMA_PrintAudiospec(const char *str, const SDL_AudioSpec *spec)
{
	int i;
	char *fmt = NULL;

	comprintf("%s:\n", str);

	for(i = 0; i < formatToStringTableSize; i++)
		if(spec->format == formatToStringTable[ i ].enumFormat){
			fmt = formatToStringTable[ i ].stringFormat;
		}

	if(fmt){
		comprintf("  Format:   %s\n", fmt);
	}else{
		comprintf("  Format:   " S_COLOR_RED "UNKNOWN\n");
	}

	comprintf("  Freq:     %d\n", (int)spec->freq);
	comprintf("  Samples:  %d\n", (int)spec->samples);
	comprintf("  Channels: %d\n", (int)spec->channels);
}

qbool
SNDDMA_Init(void)
{
	char drivername[128];
	SDL_AudioSpec	desired;
	SDL_AudioSpec	obtained;
	int tmp;

	if(snd_inited)
		return qtrue;

	if(!s_sdlBits){
		s_sdlBits	= cvarget("s_sdlBits", "16", CVAR_ARCHIVE);
		s_sdlSpeed	= cvarget("s_sdlSpeed", "0", CVAR_ARCHIVE);
		s_sdlChannels	= cvarget("s_sdlChannels", "2", CVAR_ARCHIVE);
		s_sdlDevSamps	= cvarget("s_sdlDevSamps", "0", CVAR_ARCHIVE);
		s_sdlMixSamps	= cvarget("s_sdlMixSamps", "0", CVAR_ARCHIVE);
	}

	comprintf("SDL_Init( SDL_INIT_AUDIO )... ");

	if(!SDL_WasInit(SDL_INIT_AUDIO)){
		if(SDL_Init(SDL_INIT_AUDIO) == -1){
			comprintf("FAILED (%s)\n", SDL_GetError( ));
			return qfalse;
		}
	}

	comprintf("OK\n");

	if(SDL_AudioDriverName(drivername, sizeof(drivername)) == NULL)
		strcpy(drivername, "(UNKNOWN)");
	comprintf("SDL audio driver is \"%s\".\n", drivername);

	memset(&desired, '\0', sizeof(desired));
	memset(&obtained, '\0', sizeof(obtained));

	tmp = ((int)s_sdlBits->value);
	if((tmp != 16) && (tmp != 8))
		tmp = 16;

	desired.freq = (int)s_sdlSpeed->value;
	if(!desired.freq) desired.freq = 44100;
	desired.format = ((tmp == 16) ? AUDIO_S16SYS : AUDIO_U8);

	/* I dunno if this is the best idea, but I'll give it a try...
	 *  should probably check a cvar for this... */
	if(s_sdlDevSamps->value)
		desired.samples = s_sdlDevSamps->value;
	else{
		/* just pick a sane default. */
		if(desired.freq <= 11025)
			desired.samples = 256;
		else if(desired.freq <= 22050)
			desired.samples = 512;
		else if(desired.freq <= 44100)
			desired.samples = 1024;
		else
			desired.samples = 2048;		/* (*shrug*) */
	}

	desired.channels	= (int)s_sdlChannels->value;
	desired.callback	= SNDDMA_AudioCallback;

	if(SDL_OpenAudio(&desired, &obtained) == -1){
		comprintf("SDL_OpenAudio() failed: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	SNDDMA_PrintAudiospec("SDL_AudioSpec", &obtained);

	/* dma.samples needs to be big, or id's mixer will just refuse to
	 *  work at all; we need to keep it significantly bigger than the
	 *  amount of SDL callback samples, and just copy a little each time
	 *  the callback runs.
	 * 32768 is what the OSS driver filled in here on my system. I don't
	 *  know if it's a good value overall, but at least we know it's
	 *  reasonable...this is why I let the user override. */
	tmp = s_sdlMixSamps->value;
	if(!tmp)
		tmp = (obtained.samples * obtained.channels) * 10;

	if(tmp & (tmp - 1)){	/* not a power of two? Seems to confuse something. */
		int val = 1;
		while(val < tmp)
			val <<= 1;

		tmp = val;
	}

	dmapos = 0;
	dma.samplebits	= obtained.format & 0xFF;	/* first byte of format is bits. */
	dma.channels	= obtained.channels;
	dma.samples = tmp;
	dma.submission_chunk = 1;
	dma.speed	= obtained.freq;
	dmasize		= (dma.samples * (dma.samplebits/8));
	dma.buffer	= calloc(1, dmasize);

	comprintf("Starting SDL audio callback...\n");
	SDL_PauseAudio(0);	/* start callback. */

	comprintf("SDL audio initialized.\n");
	snd_inited = qtrue;
	return qtrue;
}

int
SNDDMA_GetDMAPos(void)
{
	return dmapos;
}

void
SNDDMA_Shutdown(void)
{
	comprintf("Closing SDL audio device...\n");
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	free(dma.buffer);
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	comprintf("SDL audio device shut down.\n");
}

/*
 * Send sound to device if buffer isn't really the dma buffer
 */
void
SNDDMA_Submit(void)
{
	SDL_UnlockAudio();
}

void
SNDDMA_BeginPainting(void)
{
	SDL_LockAudio();
}
