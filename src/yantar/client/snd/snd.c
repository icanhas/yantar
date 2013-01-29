/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "../client.h"
#include "codec.h"
#include "local.h"
#include "snd.h"

Cvar	*s_volume;
Cvar	*s_muted;
Cvar	*s_musicVolume;
Cvar	*s_doppler;
Cvar	*s_backend;
Cvar	*s_muteWhenMinimized;
Cvar	*s_muteWhenUnfocused;

static Sndinterface si;

static qbool
S_ValidSoundInterface(Sndinterface *si)
{
	if(!si->Shutdown) return qfalse;
	if(!si->StartSound) return qfalse;
	if(!si->StartLocalSound) return qfalse;
	if(!si->StartBackgroundTrack) return qfalse;
	if(!si->StopBackgroundTrack) return qfalse;
	if(!si->RawSamples) return qfalse;
	if(!si->StopAllSounds) return qfalse;
	if(!si->ClearLoopingSounds) return qfalse;
	if(!si->AddLoopingSound) return qfalse;
	if(!si->AddRealLoopingSound) return qfalse;
	if(!si->StopLoopingSound) return qfalse;
	if(!si->Respatialize) return qfalse;
	if(!si->UpdateEntityPosition) return qfalse;
	if(!si->Update) return qfalse;
	if(!si->DisableSounds) return qfalse;
	if(!si->BeginRegistration) return qfalse;
	if(!si->RegisterSound) return qfalse;
	if(!si->ClearSoundBuffer) return qfalse;
	if(!si->SoundInfo) return qfalse;
	if(!si->SoundList) return qfalse;

#ifdef USE_VOIP
	if(!si->StartCapture) return qfalse;
	if(!si->AvailableCaptureSamples) return qfalse;
	if(!si->Capture) return qfalse;
	if(!si->StopCapture) return qfalse;
	if(!si->MasterGain) return qfalse;
#endif

	return qtrue;
}

void
S_StartSound(Vec3 origin, int entnum, int entchannel, Sfxhandle sfx)
{
	if(si.StartSound)
		si.StartSound(origin, entnum, entchannel, sfx);
}

void
S_StartLocalSound(Sfxhandle sfx, int channelNum)
{
	if(si.StartLocalSound)
		si.StartLocalSound(sfx, channelNum);
}

void
S_StartBackgroundTrack(const char *intro, const char *loop)
{
	if(si.StartBackgroundTrack)
		si.StartBackgroundTrack(intro, loop);
}

void
S_StopBackgroundTrack(void)
{
	if(si.StopBackgroundTrack)
		si.StopBackgroundTrack( );
}

void
S_RawSamples(int stream, int samples, int rate, int width, int channels,
	     const byte *data, float volume, int entityNum)
{
	if(si.RawSamples)
		si.RawSamples(stream, samples, rate, width, channels, data,
			volume,
			entityNum);
}

void
S_StopAllSounds(void)
{
	if(si.StopAllSounds)
		si.StopAllSounds( );
}

void
S_ClearLoopingSounds(qbool killall)
{
	if(si.ClearLoopingSounds)
		si.ClearLoopingSounds(killall);
}

void
S_AddLoopingSound(int entityNum, const Vec3 origin,
		  const Vec3 velocity, Sfxhandle sfx)
{
	if(si.AddLoopingSound)
		si.AddLoopingSound(entityNum, origin, velocity, sfx);
}

void
S_AddRealLoopingSound(int entityNum, const Vec3 origin,
		      const Vec3 velocity, Sfxhandle sfx)
{
	if(si.AddRealLoopingSound)
		si.AddRealLoopingSound(entityNum, origin, velocity, sfx);
}

void
S_StopLoopingSound(int entityNum)
{
	if(si.StopLoopingSound)
		si.StopLoopingSound(entityNum);
}

void
S_Respatialize(int entityNum, const Vec3 origin,
	       Vec3 axis[3], int inwater)
{
	if(si.Respatialize)
		si.Respatialize(entityNum, origin, axis, inwater);
}

void
S_UpdateEntityPosition(int entityNum, const Vec3 origin)
{
	if(si.UpdateEntityPosition)
		si.UpdateEntityPosition(entityNum, origin);
}

void
S_Update(void)
{
	if(s_muted->integer){
		if(!(s_muteWhenMinimized->integer && com_minimized->integer) &&
		   !(s_muteWhenUnfocused->integer && com_unfocused->integer)){
			s_muted->integer	= qfalse;
			s_muted->modified	= qtrue;
		}
	}else if((s_muteWhenMinimized->integer && com_minimized->integer) ||
		 (s_muteWhenUnfocused->integer && com_unfocused->integer)){
		s_muted->integer	= qtrue;
		s_muted->modified	= qtrue;
	}

	if(si.Update)
		si.Update( );
}

void
S_DisableSounds(void)
{
	if(si.DisableSounds)
		si.DisableSounds( );
}

void
S_BeginRegistration(void)
{
	if(si.BeginRegistration)
		si.BeginRegistration( );
}

Sfxhandle
S_RegisterSound(const char *sample, qbool compressed)
{
	if(si.RegisterSound)
		return si.RegisterSound(sample, compressed);
	else
		return 0;
}

void
sndclearbuffer(void)
{
	if(si.ClearSoundBuffer)
		si.ClearSoundBuffer( );
}

void
S_SoundInfo(void)
{
	if(si.SoundInfo)
		si.SoundInfo( );
}

void
S_SoundList(void)
{
	if(si.SoundList)
		si.SoundList( );
}

#ifdef USE_VOIP
void
S_StartCapture(void)
{
	if(si.StartCapture)
		si.StartCapture( );
}

int
S_AvailableCaptureSamples(void)
{
	if(si.AvailableCaptureSamples)
		return si.AvailableCaptureSamples( );
	return 0;
}

void
S_Capture(int samples, byte *data)
{
	if(si.Capture)
		si.Capture(samples, data);
}

void
S_StopCapture(void)
{
	if(si.StopCapture)
		si.StopCapture( );
}

void
S_MasterGain(float gain)
{
	if(si.MasterGain)
		si.MasterGain(gain);
}
#endif

void
S_Play_f(void)
{
	int	i;
	Sfxhandle h;
	char	name[256];

	if(!si.RegisterSound || !si.StartLocalSound)
		return;

	i = 1;
	while(i<cmdargc()){
		Q_strncpyz(name, cmdargv(i), sizeof(name));
		h = si.RegisterSound(name, qfalse);

		if(h)
			si.StartLocalSound(h, CHAN_LOCAL_SOUND);
		i++;
	}
}

void
S_Music_f(void)
{
	int c;

	if(!si.StartBackgroundTrack)
		return;

	c = cmdargc();

	if(c == 2)
		si.StartBackgroundTrack(cmdargv(1), NULL);
	else if(c == 3)
		si.StartBackgroundTrack(cmdargv(1), cmdargv(2));
	else{
		comprintf ("music <musicfile> [loopfile]\n");
		return;
	}

}

void
S_StopMusic_f(void)
{
	if(!si.StopBackgroundTrack)
		return;

	si.StopBackgroundTrack();
}

void
S_Init(void)
{
	Cvar *cv;
	qbool started = qfalse;

	comprintf("------ Initializing Sound ------\n");

	s_volume = cvarget("s_volume", "0.8", CVAR_ARCHIVE);
	s_musicVolume	= cvarget("s_musicvolume", "0.25", CVAR_ARCHIVE);
	s_muted		= cvarget("s_muted", "0", CVAR_ROM);
	s_doppler	= cvarget("s_doppler", "1", CVAR_ARCHIVE);
	s_backend	= cvarget("s_backend", "", CVAR_ROM);
	s_muteWhenMinimized = cvarget("s_muteWhenMinimized", "0",
		CVAR_ARCHIVE);
	s_muteWhenUnfocused = cvarget("s_muteWhenUnfocused", "0",
		CVAR_ARCHIVE);

	cv = cvarget("s_initsound", "1", 0);
	if(!cv->integer)
		comprintf("Sound disabled.\n");
	else{

		S_CodecInit( );

		cmdadd("play", S_Play_f);
		cmdadd("music", S_Music_f);
		cmdadd("stopmusic", S_StopMusic_f);
		cmdadd("s_list", S_SoundList);
		cmdadd("s_stop", S_StopAllSounds);
		cmdadd("s_info", S_SoundInfo);

		if(!started){
			started = S_Base_Init(&si);
			cvarsetstr("s_backend", "base");
		}

		if(started){
			if(!S_ValidSoundInterface(&si))
				comerrorf(ERR_FATAL, "Sound interface invalid");

			S_SoundInfo( );
			comprintf("Sound initialized.\n");
		}else
			comprintf("Sound initialization failed.\n");
	}

	comprintf("--------------------------------\n");
}

void
S_Shutdown(void)
{
	if(si.Shutdown)
		si.Shutdown( );
	Q_Memset(&si, 0, sizeof(Sndinterface));
	cmdremove("play");
	cmdremove("music");
	cmdremove("stopmusic");
	cmdremove("s_list");
	cmdremove("s_stop");
	cmdremove("s_info");
	S_CodecShutdown( );
}
