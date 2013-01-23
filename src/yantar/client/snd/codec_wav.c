/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "../client.h"
#include "codec.h"

static int
FGetLittleLong(Fhandle f)
{
	int v;

	fsread(&v, sizeof(v), f);
	return LittleLong(v);
}

static short
FGetLittleShort(Fhandle f)
{
	short v;

	fsread(&v, sizeof(v), f);
	return LittleShort(v);
}

static int
S_ReadChunkInfo(Fhandle f, char *name)
{
	int len, r;

	name[4] = 0;

	r = fsread(name, 4, f);
	if(r != 4)
		return -1;
	len = FGetLittleLong(f);
	if(len < 0){
		Com_Printf(S_COLOR_YELLOW "WARNING: Negative chunk length\n");
		return -1;
	}
	return len;
}

/* Returns the length of the data in the chunk, or -1 if not found */
static int
S_FindRIFFChunk(Fhandle f, char *chunk)
{
	char	name[5];
	int	len;

	while((len = S_ReadChunkInfo(f, name)) >= 0){
		/* If this is the right chunk, return */
		if(!Q_strncmp(name, chunk, 4))
			return len;
		len = PAD(len, 2);
		/* Not the right chunk - skip it */
		fsseek(f, len, FS_SEEK_CUR);
	}
	return -1;
}

static void
S_ByteSwapRawSamples(int samples, int width, int s_channels, const byte *data)
{
	int i;

	if(width != 2)
		return;
	if(LittleShort(256) == 256)
		return;

	if(s_channels == 2)
		samples <<= 1;
	for(i = 0; i < samples; i++)
		((short*)data)[i] = LittleShort(((short*)data)[i]);
}

static qbool
S_ReadRIFFHeader(Fhandle file, Sndinfo *info)
{
	char	dump[16];
	int	bits;
	int	fmtlen = 0;

	/* skip the riff wav header */
	fsread(dump, 12, file);
	/* Scan for the format chunk */
	if((fmtlen = S_FindRIFFChunk(file, "fmt ")) < 0){
		Com_Printf(S_COLOR_RED "ERROR: Couldn't find 'fmt' chunk\n");
		return qfalse;
	}
	/* Save the parameters */
	FGetLittleShort(file);	/* wav_format */
	info->channels = FGetLittleShort(file);
	info->rate = FGetLittleLong(file);
	FGetLittleLong(file);
	FGetLittleShort(file);
	bits = FGetLittleShort(file);

	if(bits < 8){
		Com_Printf(S_COLOR_RED
			"ERROR: Less than 8 bit sound is not supported\n");
		return qfalse;
	}

	info->width = bits / 8;
	info->dataofs = 0;

	/* Skip the rest of the format chunk if required */
	if(fmtlen > 16){
		fmtlen -= 16;
		fsseek(file, fmtlen, FS_SEEK_CUR);
	}
	/* Scan for the data chunk */
	if((info->size = S_FindRIFFChunk(file, "data")) < 0){
		Com_Printf(S_COLOR_RED "ERROR: Couldn't find \"data\" chunk\n");
		return qfalse;
	}
	info->samples = (info->size / info->width) / info->channels;
	return qtrue;
}

Sndcodec wav_codec =
{
	"wav",
	S_WAV_CodecLoad,
	S_WAV_CodecOpenStream,
	S_WAV_CodecReadStream,
	S_WAV_CodecCloseStream,
	NULL
};

void *
S_WAV_CodecLoad(const char *filename, Sndinfo *info)
{
	Fhandle file;
	void *buffer;

	fsopenr(filename, &file, qtrue);
	if(!file)
		return NULL;
	if(!S_ReadRIFFHeader(file, info)){
		fsclose(file);
		Com_Printf(S_COLOR_RED
			"ERROR: Incorrect/unsupported format in '%s'\n",
			filename);
		return NULL;
	}

	buffer = hunkalloctemp(info->size);
	if(!buffer){
		fsclose(file);
		Com_Printf(S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n",
			filename);
		return NULL;
	}

	fsread(buffer, info->size, file);
	S_ByteSwapRawSamples(info->samples, info->width, info->channels,
		(byte*)buffer);
	fsclose(file);
	return buffer;
}

Sndstream *
S_WAV_CodecOpenStream(const char *filename)
{
	Sndstream *rv;

	rv = S_CodecUtilOpen(filename, &wav_codec);
	if(!rv)
		return NULL;
	if(!S_ReadRIFFHeader(rv->file, &rv->info)){
		S_CodecUtilClose(&rv);
		return NULL;
	}
	return rv;
}

void
S_WAV_CodecCloseStream(Sndstream *stream)
{
	S_CodecUtilClose(&stream);
}

int
S_WAV_CodecReadStream(Sndstream *stream, int bytes, void *buffer)
{
	int	remaining = stream->info.size - stream->pos;
	int	samples;

	if(remaining <= 0)
		return 0;
	if(bytes > remaining)
		bytes = remaining;
	stream->pos += bytes;
	samples = (bytes / stream->info.width) / stream->info.channels;
	fsread(buffer, bytes, stream->file);
	S_ByteSwapRawSamples(samples, stream->info.width, stream->info.channels,
		buffer);
	return bytes;
}
