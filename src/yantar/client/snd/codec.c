/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "../client.h"
#include "codec.h"

static Sndcodec *codecs;

/*
 * Opens/loads a sound, tries codec based on the sound's file extension
 * then tries all supported codecs.
 */
static void *
S_CodecGetSound(const char *filename, snd_info_t *info)
{
	Sndcodec	*codec;
	Sndcodec	*orgCodec = NULL;
	qbool		orgNameFailed = qfalse;
	char	localName[ MAX_QPATH ];
	const char      *ext;
	char	altName[ MAX_QPATH ];
	void *rtn = NULL;

	Q_strncpyz(localName, filename, MAX_QPATH);

	ext = Q_getext(localName);

	if(*ext){
		/* Look for the correct loader and use it */
		for(codec = codecs; codec; codec = codec->next)
			if(!Q_stricmp(ext, codec->ext)){
				/* Load */
				if(info)
					rtn = codec->load(localName, info);
				else
					rtn = codec->open(localName);
				break;
			}

		/* A loader was found */
		if(codec){
			if(!rtn){
				/* Loader failed, most likely because the file isn't there;
				 * try again without the extension */
				orgNameFailed = qtrue;
				orgCodec = codec;
				Q_stripext(filename, localName,
					MAX_QPATH);
			}else
				/* Something loaded */
				return rtn;
		}
	}

	/* Try and find a suitable match using all
	 * the sound codecs supported */
	for(codec = codecs; codec; codec = codec->next){
		if(codec == orgCodec)
			continue;

		Q_sprintf(altName, sizeof(altName), "%s.%s", localName,
			codec->ext);

		/* Load */
		if(info)
			rtn = codec->load(altName, info);
		else
			rtn = codec->open(altName);

		if(rtn){
			if(orgNameFailed)
				Com_DPrintf(
					S_COLOR_YELLOW
					"WARNING: %s not present, using %s instead\n",
					filename, altName);

			return rtn;
		}
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: Failed to %s sound %s!\n",
		info ? "load" : "open",
		filename);

	return NULL;
}

void
S_CodecInit()
{
	codecs = NULL;

#ifdef USE_CODEC_VORBIS
	S_CodecRegister(&ogg_codec);
#endif

/* Register wav codec last so that it is always tried first when a file extension was not found */
	S_CodecRegister(&wav_codec);
}

void
S_CodecShutdown()
{
	codecs = NULL;
}

void
S_CodecRegister(Sndcodec *codec)
{
	codec->next = codecs;
	codecs = codec;
}

void *
S_CodecLoad(const char *filename, snd_info_t *info)
{
	return S_CodecGetSound(filename, info);
}

Sndstream *
S_CodecOpenStream(const char *filename)
{
	return S_CodecGetSound(filename, NULL);
}

void
S_CodecCloseStream(Sndstream *stream)
{
	stream->codec->close(stream);
}

int
S_CodecReadStream(Sndstream *stream, int bytes, void *buffer)
{
	return stream->codec->read(stream, bytes, buffer);
}

/* Util functions (used by codecs) */

Sndstream *
S_CodecUtilOpen(const char *filename, Sndcodec *codec)
{
	Sndstream	*stream;
	Fhandle	hnd;
	int length;

	/* Try to open the file */
	length = FS_FOpenFileRead(filename, &hnd, qtrue);
	if(!hnd){
		Com_DPrintf("Can't read sound file %s\n", filename);
		return NULL;
	}

	/* Allocate a stream */
	stream = Z_Malloc(sizeof(Sndstream));
	if(!stream){
		FS_FCloseFile(hnd);
		return NULL;
	}

	/* Copy over, return */
	stream->codec	= codec;
	stream->file	= hnd;
	stream->length	= length;
	return stream;
}

void
S_CodecUtilClose(Sndstream **stream)
{
	FS_FCloseFile((*stream)->file);
	Z_Free(*stream);
	*stream = NULL;
}
