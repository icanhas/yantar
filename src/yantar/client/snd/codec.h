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

typedef struct Sndinfo Sndinfo;
typedef struct Sndstream Sndstream;
typedef struct Sndcodec Sndcodec;

struct Sndinfo {
	int	rate;
	int	width;
	int	channels;
	int	samples;
	int	size;
	int	dataofs;
};

struct Sndstream {
	Sndcodec*	codec;
	Fhandle		file;
	Sndinfo		info;
	int		length;
	int		pos;
	void*		ptr;
};

/* Codec data structure */
struct Sndcodec {
	char*		ext;
	void*		(*load)(const char* fname, Sndinfo*);
	Sndstream*	(*open)(const char* fname);
	int		(*read)(Sndstream*, int, void*);
	void		(*close)(Sndstream*);
	Sndcodec*	next;
};

/* Codec management */
void S_CodecInit(void);
void S_CodecShutdown(void);
void S_CodecRegister(Sndcodec *codec);
void*S_CodecLoad(const char *filename, Sndinfo *info);
Sndstream*S_CodecOpenStream(const char *filename);
void S_CodecCloseStream(Sndstream *stream);
int S_CodecReadStream(Sndstream *stream, int bytes, void *buffer);

/* Util functions (used by codecs) */
Sndstream*S_CodecUtilOpen(const char *filename, Sndcodec *codec);
void S_CodecUtilClose(Sndstream **stream);

/* WAV Codec */
extern Sndcodec wav_codec;
void*S_WAV_CodecLoad(const char *filename, Sndinfo *info);
Sndstream*S_WAV_CodecOpenStream(const char *filename);
void S_WAV_CodecCloseStream(Sndstream *stream);
int S_WAV_CodecReadStream(Sndstream *stream, int bytes, void *buffer);

/* Ogg Vorbis codec */
#ifdef USE_CODEC_VORBIS
extern Sndcodec ogg_codec;
void*S_OGG_CodecLoad(const char *filename, Sndinfo *info);
Sndstream*S_OGG_CodecOpenStream(const char *filename);
void S_OGG_CodecCloseStream(Sndstream *stream);
int S_OGG_CodecReadStream(Sndstream *stream, int bytes, void *buffer);
#endif	/* USE_CODEC_VORBIS */

#endif	/* !_SND_CODEC_H_ */
