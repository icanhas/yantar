/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "client.h"
#include "keycodes.h"
#include "keys.h"
#include "botlib.h"
#include "ui.h"

extern botlib_export_t *botlib_export;

Vm *uivm;

/*
 * GetClientState
 */
static void
GetClientState(UIclientstate *state)
{
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = clc.state;
	Q_strncpyz(state->servername, clc.servername, sizeof(state->servername));
	Q_strncpyz(state->updateInfoString, cls.updateInfoString,
		sizeof(state->updateInfoString));
	Q_strncpyz(state->messageString, clc.serverMessage,
		sizeof(state->messageString));
	state->clientNum = cl.snap.ps.clientNum;
}

/*
 * LAN_LoadCachedServers
 */
void
LAN_LoadCachedServers(void)
{
	int size;
	Fhandle fileIn;
	cls.numglobalservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;
	if(fssvopenr("servercache.dat", &fileIn)){
		fsread(&cls.numglobalservers, sizeof(int), fileIn);
		fsread(&cls.numfavoriteservers, sizeof(int), fileIn);
		fsread(&size, sizeof(int), fileIn);
		if(size == sizeof(cls.globalServers) +
		   sizeof(cls.favoriteServers)){
			fsread(&cls.globalServers, sizeof(cls.globalServers),
				fileIn);
			fsread(&cls.favoriteServers, sizeof(cls.favoriteServers),
				fileIn);
		}else{
			cls.numglobalservers = cls.numfavoriteservers = 0;
			cls.numGlobalServerAddresses = 0;
		}
		fsclose(fileIn);
	}
}

/*
 * LAN_SaveServersToCache
 */
void
LAN_SaveServersToCache(void)
{
	int size;
	Fhandle fileOut = fssvopenw("servercache.dat");
	fswrite(&cls.numglobalservers, sizeof(int), fileOut);
	fswrite(&cls.numfavoriteservers, sizeof(int), fileOut);
	size = sizeof(cls.globalServers) + sizeof(cls.favoriteServers);
	fswrite(&size, sizeof(int), fileOut);
	fswrite(&cls.globalServers, sizeof(cls.globalServers), fileOut);
	fswrite(&cls.favoriteServers, sizeof(cls.favoriteServers), fileOut);
	fsclose(fileOut);
}


/*
 * LAN_ResetPings
 */
static void
LAN_ResetPings(int source)
{
	int count,i;
	Servinfo *servers = NULL;
	count = 0;

	switch(source){
	case AS_LOCAL:
		servers = &cls.localServers[0];
		count	= MAX_OTHER_SERVERS;
		break;
	case AS_GLOBAL:
		servers = &cls.globalServers[0];
		count	= MAX_GLOBAL_SERVERS;
		break;
	case AS_FAVORITES:
		servers = &cls.favoriteServers[0];
		count	= MAX_OTHER_SERVERS;
		break;
	}
	if(servers)
		for(i = 0; i < count; i++)
			servers[i].ping = -1;
}

/*
 * LAN_AddServer
 */
static int
LAN_AddServer(int source, const char *name, const char *address)
{
	int max, *count, i;
	Netaddr adr;
	Servinfo *servers = NULL;
	max = MAX_OTHER_SERVERS;
	count = NULL;

	switch(source){
	case AS_LOCAL:
		count	= &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_GLOBAL:
		max = MAX_GLOBAL_SERVERS;
		count	= &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		count	= &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}
	if(servers && *count < max){
		strtoaddr(address, &adr, NA_IP);
		for(i = 0; i < *count; i++)
			if(equaladdr(servers[i].adr, adr))
				break;
		if(i >= *count){
			servers[*count].adr = adr;
			Q_strncpyz(servers[*count].hostName, name,
				sizeof(servers[*count].hostName));
			servers[*count].visible = qtrue;
			(*count)++;
			return 1;
		}
		return 0;
	}
	return -1;
}

/*
 * LAN_RemoveServer
 */
static void
LAN_RemoveServer(int source, const char *addr)
{
	int *count, i;
	Servinfo *servers = NULL;
	count = NULL;
	switch(source){
	case AS_LOCAL:
		count	= &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_GLOBAL:
		count	= &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		count	= &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}
	if(servers){
		Netaddr comp;
		strtoaddr(addr, &comp, NA_IP);
		for(i = 0; i < *count; i++)
			if(equaladdr(comp, servers[i].adr)){
				int j = i;
				while(j < *count - 1){
					Q_Memcpy(&servers[j], &servers[j+1],
						sizeof(servers[j]));
					j++;
				}
				(*count)--;
				break;
			}
	}
}


/*
 * LAN_GetServerCount
 */
static int
LAN_GetServerCount(int source)
{
	switch(source){
	case AS_LOCAL:
		return cls.numlocalservers;
		break;
	case AS_GLOBAL:
		return cls.numglobalservers;
		break;
	case AS_FAVORITES:
		return cls.numfavoriteservers;
		break;
	}
	return 0;
}

/*
 * LAN_GetLocalServerAddressString
 */
static void
LAN_GetServerAddressString(int source, int n, char *buf, int buflen)
{
	switch(source){
	case AS_LOCAL:
		if(n >= 0 && n < MAX_OTHER_SERVERS){
			Q_strncpyz(buf,
				addrporttostr(
					cls.localServers[n].adr), buflen);
			return;
		}
		break;
	case AS_GLOBAL:
		if(n >= 0 && n < MAX_GLOBAL_SERVERS){
			Q_strncpyz(buf,
				addrporttostr(
					cls.globalServers[n].adr), buflen);
			return;
		}
		break;
	case AS_FAVORITES:
		if(n >= 0 && n < MAX_OTHER_SERVERS){
			Q_strncpyz(buf,
				addrporttostr(
					cls.favoriteServers[n].adr), buflen);
			return;
		}
		break;
	}
	buf[0] = '\0';
}

/*
 * LAN_GetServerInfo
 */
static void
LAN_GetServerInfo(int source, int n, char *buf, int buflen)
{
	char info[MAX_STRING_CHARS];
	Servinfo *server = NULL;
	info[0] = '\0';
	switch(source){
	case AS_LOCAL:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			server = &cls.localServers[n];
		break;
	case AS_GLOBAL:
		if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			server = &cls.globalServers[n];
		break;
	case AS_FAVORITES:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			server = &cls.favoriteServers[n];
		break;
	}
	if(server && buf){
		buf[0] = '\0';
		Info_SetValueForKey(info, "hostname", server->hostName);
		Info_SetValueForKey(info, "mapname", server->mapName);
		Info_SetValueForKey(info, "clients", va("%i",server->clients));
		Info_SetValueForKey(info, "sv_maxclients",
			va("%i",server->maxClients));
		Info_SetValueForKey(info, "ping", va("%i",server->ping));
		Info_SetValueForKey(info, "minping", va("%i",server->minPing));
		Info_SetValueForKey(info, "maxping", va("%i",server->maxPing));
		Info_SetValueForKey(info, "game", server->game);
		Info_SetValueForKey(info, "gametype", va("%i",server->gameType));
		Info_SetValueForKey(info, "nettype", va("%i",server->netType));
		Info_SetValueForKey(info, "addr",
			addrporttostr(server->adr));
		Info_SetValueForKey(info, "g_needpass",
			va("%i", server->g_needpass));
		Info_SetValueForKey(info, "g_humanplayers",
			va("%i", server->g_humanplayers));
		Q_strncpyz(buf, info, buflen);
	}else if(buf)
		buf[0] = '\0';

}

/*
 * LAN_GetServerPing
 */
static int
LAN_GetServerPing(int source, int n)
{
	Servinfo *server = NULL;
	switch(source){
	case AS_LOCAL:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			server = &cls.localServers[n];
		break;
	case AS_GLOBAL:
		if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			server = &cls.globalServers[n];
		break;
	case AS_FAVORITES:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			server = &cls.favoriteServers[n];
		break;
	}
	if(server)
		return server->ping;
	return -1;
}

/*
 * LAN_GetServerPtr
 */
static Servinfo *
LAN_GetServerPtr(int source, int n)
{
	switch(source){
	case AS_LOCAL:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			return &cls.localServers[n];
		break;
	case AS_GLOBAL:
		if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			return &cls.globalServers[n];
		break;
	case AS_FAVORITES:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			return &cls.favoriteServers[n];
		break;
	}
	return NULL;
}

/*
 * LAN_CompareServers
 */
static int
LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	int res;
	Servinfo *server1, *server2;

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if(!server1 || !server2)
		return 0;

	res = 0;
	switch(sortKey){
	case SORT_HOST:
		res = Q_stricmp(server1->hostName, server2->hostName);
		break;

	case SORT_MAP:
		res = Q_stricmp(server1->mapName, server2->mapName);
		break;
	case SORT_CLIENTS:
		if(server1->clients < server2->clients)
			res = -1;
		else if(server1->clients > server2->clients)
			res = 1;
		else
			res = 0;
		break;
	case SORT_GAME:
		if(server1->gameType < server2->gameType)
			res = -1;
		else if(server1->gameType > server2->gameType)
			res = 1;
		else
			res = 0;
		break;
	case SORT_PING:
		if(server1->ping < server2->ping)
			res = -1;
		else if(server1->ping > server2->ping)
			res = 1;
		else
			res = 0;
		break;
	}

	if(sortDir){
		if(res < 0)
			return 1;
		if(res > 0)
			return -1;
		return 0;
	}
	return res;
}

/*
 * LAN_GetPingQueueCount
 */
static int
LAN_GetPingQueueCount(void)
{
	return (CL_GetPingQueueCount());
}

/*
 * LAN_ClearPing
 */
static void
LAN_ClearPing(int n)
{
	CL_ClearPing(n);
}

/*
 * LAN_GetPing
 */
static void
LAN_GetPing(int n, char *buf, int buflen, int *pingtime)
{
	CL_GetPing(n, buf, buflen, pingtime);
}

/*
 * LAN_GetPingInfo
 */
static void
LAN_GetPingInfo(int n, char *buf, int buflen)
{
	CL_GetPingInfo(n, buf, buflen);
}

/*
 * LAN_MarkServerVisible
 */
static void
LAN_MarkServerVisible(int source, int n, qbool visible)
{
	if(n == -1){
		int count = MAX_OTHER_SERVERS;
		Servinfo *server = NULL;
		switch(source){
		case AS_LOCAL:
			server = &cls.localServers[0];
			break;
		case AS_GLOBAL:
			server	= &cls.globalServers[0];
			count	= MAX_GLOBAL_SERVERS;
			break;
		case AS_FAVORITES:
			server = &cls.favoriteServers[0];
			break;
		}
		if(server)
			for(n = 0; n < count; n++)
				server[n].visible = visible;

	}else{
		switch(source){
		case AS_LOCAL:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
				cls.localServers[n].visible = visible;
			break;
		case AS_GLOBAL:
			if(n >= 0 && n < MAX_GLOBAL_SERVERS)
				cls.globalServers[n].visible = visible;
			break;
		case AS_FAVORITES:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
				cls.favoriteServers[n].visible = visible;
			break;
		}
	}
}


/*
 * LAN_ServerIsVisible
 */
static int
LAN_ServerIsVisible(int source, int n)
{
	switch(source){
	case AS_LOCAL:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			return cls.localServers[n].visible;
		break;
	case AS_GLOBAL:
		if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			return cls.globalServers[n].visible;
		break;
	case AS_FAVORITES:
		if(n >= 0 && n < MAX_OTHER_SERVERS)
			return cls.favoriteServers[n].visible;
		break;
	}
	return qfalse;
}

/*
 * LAN_UpdateVisiblePings
 */
qbool
LAN_UpdateVisiblePings(int source)
{
	return CL_UpdateVisiblePings_f(source);
}

/*
 * LAN_GetServerStatus
 */
int
LAN_GetServerStatus(char *serverAddress, char *serverStatus, int maxLen)
{
	return CL_ServerStatus(serverAddress, serverStatus, maxLen);
}

/*
 * CL_GetGlConfig
 */
static void
CL_GetGlconfig(Glconfig *config)
{
	*config = cls.glconfig;
}

/*
 * CL_GetClipboardData
 */
static void
CL_GetClipboardData(char *buf, int buflen)
{
	char *cbd;

	cbd = sysgetclipboarddata();

	if(!cbd){
		*buf = 0;
		return;
	}

	Q_strncpyz(buf, cbd, buflen);

	zfree(cbd);
}

/*
 * Key_KeynumToStringBuf
 */
static void
Key_KeynumToStringBuf(int keynum, char *buf, int buflen)
{
	Q_strncpyz(buf, Key_KeynumToString(keynum), buflen);
}

/*
 * Key_GetBindingBuf
 */
static void
Key_GetBindingBuf(int keynum, char *buf, int buflen)
{
	char *value;

	value = Key_GetBinding(keynum);
	if(value)
		Q_strncpyz(buf, value, buflen);
	else
		*buf = 0;
}

/*
 * GetConfigString
 */
static int
GetConfigString(int index, char *buf, int size)
{
	int offset;

	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	offset = cl.gameState.stringOffsets[index];
	if(!offset){
		if(size)
			buf[0] = 0;
		return qfalse;
	}

	Q_strncpyz(buf, cl.gameState.stringData+offset, size);

	return qtrue;
}

/*
 * FloatAsInt
 */
static int
FloatAsInt(float f)
{
	Flint fi;
	fi.f = f;
	return fi.i;
}

/*
 * CL_UISystemCalls
 *
 * The ui module is making a system call
 */
intptr_t
CL_UISystemCalls(intptr_t *args)
{
	switch(args[0]){
	case UI_ERROR:
		comerrorf(ERR_DROP, "%s", (const char*)VMA(1));
		return 0;

	case UI_PRINT:
		comprintf("%s", (const char*)VMA(1));
		return 0;

	case UI_MILLISECONDS:
		return sysmillisecs();

	case UI_CVAR_REGISTER:
		cvarregister(VMA(1), VMA(2), VMA(3), args[4]);
		return 0;

	case UI_CVAR_UPDATE:
		cvarupdate(VMA(1));
		return 0;

	case UI_CVAR_SET:
		cvarsetstrsafe(VMA(1), VMA(2));
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt(cvargetf(VMA(1)));

	case UI_CVAR_VARIABLESTRINGBUFFER:
		cvargetstrbuf(VMA(1), VMA(2), args[3]);
		return 0;

	case UI_CVAR_SETVALUE:
		cvarsetfsafe(VMA(1), VMF(2));
		return 0;

	case UI_CVAR_RESET:
		cvarreset(VMA(1));
		return 0;

	case UI_CVAR_CREATE:
		cvarregister(nil, VMA(1), VMA(2), args[3]);
		return 0;

	case UI_CVAR_INFOSTRINGBUFFER:
		cvargetinfostrbuf(args[1], VMA(2), args[3]);
		return 0;

	case UI_ARGC:
		return cmdargc();

	case UI_ARGV:
		cmdargvbuf(args[1], VMA(2), args[3]);
		return 0;

	case UI_CMD_EXECUTETEXT:
		if(args[1] == EXEC_NOW
		   && (!strncmp(VMA(2), "snd_restart", 11)
		       || !strncmp(VMA(2), "vid_restart", 11)
		       || !strncmp(VMA(2), "quit", 5))){
			comprintf (
				S_COLOR_YELLOW
				"turning EXEC_NOW '%.11s' into EXEC_INSERT\n",
				(const char*)VMA(2));
			args[1] = EXEC_INSERT;
		}
		cbufexecstr(args[1], VMA(2));
		return 0;

	case UI_FS_FOPENFILE:
		return fsopenmode(VMA(1), VMA(2), args[3]);

	case UI_FS_READ:
		fsread2(VMA(1), args[2], args[3]);
		return 0;

	case UI_FS_WRITE:
		fswrite(VMA(1), args[2], args[3]);
		return 0;

	case UI_FS_FCLOSEFILE:
		fsclose(args[1]);
		return 0;

	case UI_FS_GETFILELIST:
		return fsgetfilelist(VMA(1), VMA(2), VMA(3), args[4]);

	case UI_FS_SEEK:
		return fsseek(args[1], args[2], args[3]);

	case UI_R_REGISTERMODEL:
		return re.RegisterModel(VMA(1));

	case UI_R_REGISTERSKIN:
		return re.RegisterSkin(VMA(1));

	case UI_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip(VMA(1));

	case UI_R_CLEARSCENE:
		re.ClearScene();
		return 0;

	case UI_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene(VMA(1));
		return 0;

	case UI_R_ADDPOLYTOSCENE:
		re.AddPolyToScene(args[1], args[2], VMA(3), 1);
		return 0;

	case UI_R_ADDLIGHTTOSCENE:
		re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5));
		return 0;

	case UI_R_RENDERSCENE:
		re.RenderScene(VMA(1));
		return 0;

	case UI_R_SETCOLOR:
		re.SetColor(VMA(1));
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(
				6), VMF(7), VMF(8), args[9]);
		return 0;

	case UI_R_MODELBOUNDS:
		re.ModelBounds(args[1], VMA(2), VMA(3));
		return 0;

	case UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case UI_CM_LERPTAG:
		re.LerpTag(VMA(1), args[2], args[3], args[4], VMF(5), VMA(6));
		return 0;

	case UI_S_REGISTERSOUND:
		return sndregister(VMA(1), args[2]);

	case UI_S_STARTLOCALSOUND:
		sndstartlocalsound(args[1], args[2]);
		return 0;

	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
		return 0;

	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], VMA(2), args[3]);
		return 0;

	case UI_KEY_SETBINDING:
		Key_SetBinding(args[1], VMA(2));
		return 0;

	case UI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case UI_KEY_SETCATCHER:
		/* Don't allow the ui module to close the console */
		Key_SetCatcher(args[1] | (Key_GetCatcher( ) & KEYCATCH_CONSOLE));
		return 0;

	case UI_GETCLIPBOARDDATA:
		CL_GetClipboardData(VMA(1), args[2]);
		return 0;

	case UI_GETCLIENTSTATE:
		GetClientState(VMA(1));
		return 0;

	case UI_GETGLCONFIG:
		CL_GetGlconfig(VMA(1));
		return 0;

	case UI_GETCONFIGSTRING:
		return GetConfigString(args[1], VMA(2), args[3]);

	case UI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case UI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case UI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], VMA(2), VMA(3));

	case UI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], VMA(2));
		return 0;

	case UI_LAN_GETPINGQUEUECOUNT:
		return LAN_GetPingQueueCount();

	case UI_LAN_CLEARPING:
		LAN_ClearPing(args[1]);
		return 0;

	case UI_LAN_GETPING:
		LAN_GetPing(args[1], VMA(2), args[3], VMA(4));
		return 0;

	case UI_LAN_GETPINGINFO:
		LAN_GetPingInfo(args[1], VMA(2), args[3]);
		return 0;

	case UI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case UI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], VMA(3), args[4]);
		return 0;

	case UI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], VMA(3), args[4]);
		return 0;

	case UI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);

	case UI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], args[3]);
		return 0;

	case UI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);

	case UI_LAN_UPDATEVISIBLEPINGS:
		return LAN_UpdateVisiblePings(args[1]);

	case UI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;

	case UI_LAN_SERVERSTATUS:
		return LAN_GetServerStatus(VMA(1), VMA(2), args[3]);

	case UI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4],
			args[5]);

	case UI_MEMORY_REMAINING:
		return hunkmemremaining();

	case UI_R_REGISTERFONT:
		re.RegisterFont(VMA(1), args[2], VMA(3));
		return 0;

	case UI_S_STOPBACKGROUNDTRACK:
		sndstopbackgroundtrack();
		return 0;
	case UI_S_STARTBACKGROUNDTRACK:
		sndstartbackgroundtrack(VMA(1), VMA(2));
		return 0;

	case UI_REAL_TIME:
		return comrealtime(VMA(1));

	case UI_CIN_PLAYCINEMATIC:
		comdprintf("UI_CIN_PlayCinematic\n");
		return CIN_PlayCinematic(VMA(
				1), args[2], args[3], args[4], args[5], args[6]);

	case UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case UI_R_REMAP_SHADER:
		re.RemapShader(VMA(1), VMA(2), VMA(3));
		return 0;

	case UI_MEMSET:
		Q_Memset(VMA(1), args[2], args[3]);
		return 0;
	case UI_MEMCPY:
		Q_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;
	case UI_STRNCPY:
		strncpy(VMA(1), VMA(2), args[3]);
		return args[1];
	case UI_SIN:
		return FloatAsInt(sin(VMF(1)));
	case UI_COS:
		return FloatAsInt(cos(VMF(1)));
	case UI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));
	case UI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));
	case UI_FLOOR:
		return FloatAsInt(floor(VMF(1)));
	case UI_CEIL:
		return FloatAsInt(ceil(VMF(1)));
	case UI_ACOS:
		return FloatAsInt(Q_acos(VMF(1)));
	case UI_ASIN:
		return FloatAsInt(asin(VMF(1)));
	case UI_ATAN:
		return FloatAsInt(atan(VMF(1)));
	default:
		comerrorf(ERR_DROP, "Bad UI system trap: %ld",
			(long int)args[0]);
	}

	return 0;
}

void
clshutdownUI(void)
{
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_UI);
	cls.uiStarted = qfalse;
	if(uivm == NULL)
		return;
	vmcall(uivm, UI_SHUTDOWN);
	vmfree(uivm);
	uivm = NULL;
}

enum {
	UI_OLD_API_VERSION = 4
};

void
clinitUI(void)
{
	int v;
	Vmmode interpret;

	/* load the dll or bytecode */
	interpret = cvargetf("vm_ui");
	if(cl_connectedToPureServer)
		/* if sv_pure is set we only allow qvms to be loaded */
		if(interpret != VMcompiled && interpret != VMbytecode)
			interpret = VMcompiled;

	uivm = vmcreate("ui", CL_UISystemCalls, interpret);
	if(uivm == NULL)
		comerrorf(ERR_FATAL, "vmcreate on UI failed");

	/* sanity check */
	v = vmcall(uivm, UI_GETAPIVERSION);
	if(v == UI_OLD_API_VERSION)
		/* comprintf(S_COLOR_YELLOW "WARNING: loading old Quake III Arena User Interface version %d\n", v ); */
		/* init for this gamestate */
		vmcall(uivm, UI_INIT,
			(clc.state >= CA_AUTHORIZING && clc.state < CA_ACTIVE));
	else if(v != UI_API_VERSION){
		vmfree(uivm);
		uivm = NULL;
		comerrorf(ERR_DROP, "User Interface is version %d, expected %d",
			v,
			UI_API_VERSION);
		cls.uiStarted = qfalse;
	}else
		/* init for this gamestate */
		vmcall(uivm, UI_INIT,
			(clc.state >= CA_AUTHORIZING && clc.state < CA_ACTIVE));
}

/* UI_GameCommand: See if the current console command is claimed by the ui */
qbool
UI_GameCommand(void)
{
	if(uivm == NULL)
		return qfalse;
	return vmcall(uivm, UI_CONSOLE_COMMAND, cls.simtime);
}
