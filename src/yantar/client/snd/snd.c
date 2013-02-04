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
	if(!si->shutdown) return qfalse;
	if(!si->startsnd) return qfalse;
	if(!si->startlocalsnd) return qfalse;
	if(!si->startbackgroundtrack) return qfalse;
	if(!si->stopbackgroundtrack) return qfalse;
	if(!si->rawsamps) return qfalse;
	if(!si->stopall) return qfalse;
	if(!si->clearloops) return qfalse;
	if(!si->addloop) return qfalse;
	if(!si->addrealloop) return qfalse;
	if(!si->stoploops) return qfalse;
	if(!si->respatialize) return qfalse;
	if(!si->updateentpos) return qfalse;
	if(!si->update) return qfalse;
	if(!si->disablesnds) return qfalse;
	if(!si->beginreg) return qfalse;
	if(!si->registersnd) return qfalse;
	if(!si->clearsndbuf) return qfalse;
	if(!si->sndinfo) return qfalse;
	if(!si->sndlist) return qfalse;

#ifdef USE_VOIP
	if(!si->startcapture) return qfalse;
	if(!si->availcapturesamps) return qfalse;
	if(!si->capture) return qfalse;
	if(!si->stopcapture) return qfalse;
	if(!si->mastergain) return qfalse;
#endif

	return qtrue;
}

void
sndstartsound(Vec3 origin, int entnum, int entchannel, Handle sfx)
{
	if(si.startsnd)
		si.startsnd(origin, entnum, entchannel, sfx);
}

void
sndstartlocalsound(Handle sfx, int channelNum)
{
	if(si.startlocalsnd)
		si.startlocalsnd(sfx, channelNum);
}

void
sndstartbackgroundtrack(const char *intro, const char *loop)
{
	if(si.startbackgroundtrack)
		si.startbackgroundtrack(intro, loop);
}

void
sndstopbackgroundtrack(void)
{
	if(si.stopbackgroundtrack)
		si.stopbackgroundtrack( );
}

void
sndrawsamps(int stream, int samples, int rate, int width, int channels,
	     const byte *data, float volume, int entityNum)
{
	if(si.rawsamps)
		si.rawsamps(stream, samples, rate, width, channels, data,
			volume,
			entityNum);
}

void
sndstopall(void)
{
	if(si.stopall)
		si.stopall( );
}

void
sndclearloops(qbool killall)
{
	if(si.clearloops)
		si.clearloops(killall);
}

void
sndaddloop(int entityNum, const Vec3 origin,
		  const Vec3 velocity, Handle sfx)
{
	if(si.addloop)
		si.addloop(entityNum, origin, velocity, sfx);
}

void
sndaddrealloop(int entityNum, const Vec3 origin,
		      const Vec3 velocity, Handle sfx)
{
	if(si.addrealloop)
		si.addrealloop(entityNum, origin, velocity, sfx);
}

void
sndstoploop(int entityNum)
{
	if(si.stoploops)
		si.stoploops(entityNum);
}

void
sndrespatialize(int entityNum, const Vec3 origin,
	       Vec3 axis[3], int inwater)
{
	if(si.respatialize)
		si.respatialize(entityNum, origin, axis, inwater);
}

void
sndupdateentpos(int entityNum, const Vec3 origin)
{
	if(si.updateentpos)
		si.updateentpos(entityNum, origin);
}

void
sndupdate(void)
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

	if(si.update)
		si.update( );
}

void
snddisablesounds(void)
{
	if(si.disablesnds)
		si.disablesnds( );
}

void
sndbeginreg(void)
{
	if(si.beginreg)
		si.beginreg( );
}

Handle
sndregister(const char *sample, qbool compressed)
{
	if(si.registersnd)
		return si.registersnd(sample, compressed);
	else
		return 0;
}

void
sndclearbuf(void)
{
	if(si.clearsndbuf)
		si.clearsndbuf( );
}

void
S_SoundInfo(void)
{
	if(si.sndinfo)
		si.sndinfo( );
}

void
S_SoundList(void)
{
	if(si.sndlist)
		si.sndlist( );
}

#ifdef USE_VOIP
void
sndstartcapture(void)
{
	if(si.startcapture)
		si.startcapture( );
}

int
sndavailcapturesamps(void)
{
	if(si.availcapturesamps)
		return si.availcapturesamps( );
	return 0;
}

void
sndcapture(int samples, byte *data)
{
	if(si.capture)
		si.capture(samples, data);
}

void
sndstopcapture(void)
{
	if(si.stopcapture)
		si.stopcapture( );
}

void
sndmastergain(float gain)
{
	if(si.mastergain)
		si.mastergain(gain);
}
#endif

void
S_Play_f(void)
{
	int	i;
	Handle h;
	char	name[256];

	if(!si.registersnd || !si.startlocalsnd)
		return;

	i = 1;
	while(i<cmdargc()){
		Q_strncpyz(name, cmdargv(i), sizeof(name));
		h = si.registersnd(name, qfalse);

		if(h)
			si.startlocalsnd(h, CHAN_LOCAL_SOUND);
		i++;
	}
}

void
S_Music_f(void)
{
	int c;

	if(!si.startbackgroundtrack)
		return;

	c = cmdargc();

	if(c == 2)
		si.startbackgroundtrack(cmdargv(1), NULL);
	else if(c == 3)
		si.startbackgroundtrack(cmdargv(1), cmdargv(2));
	else{
		comprintf ("music <musicfile> [loopfile]\n");
		return;
	}

}

void
S_StopMusic_f(void)
{
	if(!si.stopbackgroundtrack)
		return;

	si.stopbackgroundtrack();
}

void
sndinit(void)
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
		cmdadd("s_stop", sndstopall);
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
sndshutdown(void)
{
	if(si.shutdown)
		si.shutdown( );
	Q_Memset(&si, 0, sizeof(Sndinterface));
	cmdremove("play");
	cmdremove("music");
	cmdremove("stopmusic");
	cmdremove("s_list");
	cmdremove("s_stop");
	cmdremove("s_info");
	S_CodecShutdown( );
}
