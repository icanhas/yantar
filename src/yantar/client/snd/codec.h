/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifndef _SND_CODEC_H_
#define _SND_CODEC_H_

#include "shared.h"
#include "common.h"

typedef struct snd_info_t snd_info_t;
typedef struct Sndstream Sndstream;
typedef struct Sndcodec Sndcodec;

/* Codec op types */
typedef void *(*CODEC_LOAD)(const char *filename, snd_info_t *info);
typedef Sndstream *(*CODEC_OPEN)(const char *filename);
typedef int (*CODEC_READ)(Sndstream *stream, int bytes, void *buffer);
typedef void (*CODEC_CLOSE)(Sndstream *stream);

typedef struct snd_info_t {
	int	rate;
	int	width;
	int	channels;
	int	samples;
	int	size;
	int	dataofs;
};

struct Sndstream {
	Sndcodec	*codec;
	Fhandle	file;
	snd_info_t	info;
	int		length;
	int		pos;
	void		*ptr;
};

/* Codec data structure */
struct Sndcodec {
	char		*ext;
	CODEC_LOAD	load;
	CODEC_OPEN	open;
	CODEC_READ	read;
	CODEC_CLOSE	close;
	Sndcodec	*next;
};

/* Codec management */
void S_CodecInit(void);
void S_CodecShutdown(void);
void S_CodecRegister(Sndcodec *codec);
void*S_CodecLoad(const char *filename, snd_info_t *info);
Sndstream*S_CodecOpenStream(const char *filename);
void S_CodecCloseStream(Sndstream *stream);
int S_CodecReadStream(Sndstream *stream, int bytes, void *buffer);

/* Util functions (used by codecs) */
Sndstream*S_CodecUtilOpen(const char *filename, Sndcodec *codec);
void S_CodecUtilClose(Sndstream **stream);

/* WAV Codec */
extern Sndcodec wav_codec;
void*S_WAV_CodecLoad(const char *filename, snd_info_t *info);
Sndstream*S_WAV_CodecOpenStream(const char *filename);
void S_WAV_CodecCloseStream(Sndstream *stream);
int S_WAV_CodecReadStream(Sndstream *stream, int bytes, void *buffer);

/* Ogg Vorbis codec */
#ifdef USE_CODEC_VORBIS
extern Sndcodec ogg_codec;
void*S_OGG_CodecLoad(const char *filename, snd_info_t *info);
Sndstream*S_OGG_CodecOpenStream(const char *filename);
void S_OGG_CodecCloseStream(Sndstream *stream);
int S_OGG_CodecReadStream(Sndstream *stream, int bytes, void *buffer);
#endif	/* USE_CODEC_VORBIS */

#endif	/* !_SND_CODEC_H_ */
