/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "ui.h"
#include "ref.h"
#include "local.h"

/* this file is only included when building a dll
 * syscalls.asm is included instead when building a qvm */
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

static intptr_t (QDECL *syscall)(intptr_t arg,
				 ...) = (intptr_t (QDECL*)(intptr_t, ...))-1;

Q_EXPORT void
dllEntry(intptr_t (QDECL *syscallptr)(intptr_t arg,...))
{
	syscall = syscallptr;
}

int
PASSFLOAT(float x)
{
	Flint fi;
	fi.f = x;
	return fi.i;
}

void
trap_Print(const char *string)
{
	syscall(UI_PRINT, string);
}

void
trap_Error(const char *string)
{
	syscall(UI_ERROR, string);
	exit(1);
}

int
trap_Milliseconds(void)
{
	return syscall(UI_MILLISECONDS);
}

void
trap_Cvar_Register(Vmcvar *cvar, const char *var_name, const char *value,
		   int flags)
{
	syscall(UI_CVAR_REGISTER, cvar, var_name, value, flags);
}

void
trap_Cvar_Update(Vmcvar *cvar)
{
	syscall(UI_CVAR_UPDATE, cvar);
}

void
trap_Cvar_Set(const char *var_name, const char *value)
{
	syscall(UI_CVAR_SET, var_name, value);
}

float
trap_Cvar_VariableValue(const char *var_name)
{
	Flint fi;
	fi.i = syscall(UI_CVAR_VARIABLEVALUE, var_name);
	return fi.f;
}

void
trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	syscall(UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}

void
trap_Cvar_SetValue(const char *var_name, float value)
{
	syscall(UI_CVAR_SETVALUE, var_name, PASSFLOAT(value));
}

void
trap_Cvar_Reset(const char *name)
{
	syscall(UI_CVAR_RESET, name);
}

void
trap_Cvar_Create(const char *var_name, const char *var_value, int flags)
{
	syscall(UI_CVAR_CREATE, var_name, var_value, flags);
}

void
trap_Cvar_InfoStringBuffer(int bit, char *buffer, int bufsize)
{
	syscall(UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize);
}

int
trap_Argc(void)
{
	return syscall(UI_ARGC);
}

void
trap_Argv(int n, char *buffer, int bufferLength)
{
	syscall(UI_ARGV, n, buffer, bufferLength);
}

void
trap_Cmd_ExecuteText(int exec_when, const char *text)
{
	syscall(UI_CMD_EXECUTETEXT, exec_when, text);
}

int
trap_FS_FOpenFile(const char *qpath, Fhandle *f, Fsmode mode)
{
	return syscall(UI_FS_FOPENFILE, qpath, f, mode);
}

void
trap_FS_Read(void *buffer, int len, Fhandle f)
{
	syscall(UI_FS_READ, buffer, len, f);
}

void
trap_FS_Write(const void *buffer, int len, Fhandle f)
{
	syscall(UI_FS_WRITE, buffer, len, f);
}

void
trap_FS_FCloseFile(Fhandle f)
{
	syscall(UI_FS_FCLOSEFILE, f);
}

int
trap_FS_GetFileList(const char *path, const char *extension, char *listbuf,
		    int bufsize)
{
	return syscall(UI_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

int
trap_FS_Seek(Fhandle f, long offset, int origin)
{
	return syscall(UI_FS_SEEK, f, offset, origin);
}

Handle
trap_R_RegisterModel(const char *name)
{
	return syscall(UI_R_REGISTERMODEL, name);
}

Handle
trap_R_RegisterSkin(const char *name)
{
	return syscall(UI_R_REGISTERSKIN, name);
}

void
trap_R_RegisterFont(const char *fontName, int pointSize, Fontinfo *font)
{
	syscall(UI_R_REGISTERFONT, fontName, pointSize, font);
}

Handle
trap_R_RegisterShaderNoMip(const char *name)
{
	return syscall(UI_R_REGISTERSHADERNOMIP, name);
}

void
trap_R_ClearScene(void)
{
	syscall(UI_R_CLEARSCENE);
}

void
trap_R_AddRefEntityToScene(const Refent *re)
{
	syscall(UI_R_ADDREFENTITYTOSCENE, re);
}

void
trap_R_AddPolyToScene(Handle hShader, int numVerts, const Polyvert *verts)
{
	syscall(UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts);
}

void
trap_R_AddLightToScene(const Vec3 org, float intensity, float r, float g,
		       float b)
{
	syscall(UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(
			r), PASSFLOAT(g), PASSFLOAT(b));
}

void
trap_R_RenderScene(const Refdef *fd)
{
	syscall(UI_R_RENDERSCENE, fd);
}

void
trap_R_SetColor(const float *rgba)
{
	syscall(UI_R_SETCOLOR, rgba);
}

void
trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1,
		      float s2, float t2,
		      Handle hShader)
{
	syscall(UI_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(
			w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(
			t1), PASSFLOAT(s2),
		PASSFLOAT(t2), hShader);
}

void
trap_R_ModelBounds(Cliphandle model, Vec3 mins, Vec3 maxs)
{
	syscall(UI_R_MODELBOUNDS, model, mins, maxs);
}

void
trap_UpdateScreen(void)
{
	syscall(UI_UPDATESCREEN);
}

int
trap_CM_LerpTag(Orient *tag, Cliphandle mod, int startFrame,
		int endFrame, float frac,
		const char *tagName)
{
	return syscall(UI_CM_LERPTAG, tag, mod, startFrame, endFrame,
		PASSFLOAT(frac), tagName);
}

void
trap_S_StartLocalSound(Sfxhandle sfx, int channelNum)
{
	syscall(UI_S_STARTLOCALSOUND, sfx, channelNum);
}

Sfxhandle
trap_S_RegisterSound(const char *sample, qbool compressed)
{
	return syscall(UI_S_REGISTERSOUND, sample, compressed);
}

void
trap_S_StopBackgroundTrack(void)
{
	syscall(UI_S_STOPBACKGROUNDTRACK);
}

void
trap_S_StartBackgroundTrack(const char *intro, const char *loop)
{
	syscall(UI_S_STARTBACKGROUNDTRACK, intro, loop);
}

void
trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen)
{
	syscall(UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen);
}

void
trap_Key_GetBindingBuf(int keynum, char *buf, int buflen)
{
	syscall(UI_KEY_GETBINDINGBUF, keynum, buf, buflen);
}

void
trap_Key_SetBinding(int keynum, const char *binding)
{
	syscall(UI_KEY_SETBINDING, keynum, binding);
}

qbool
trap_Key_IsDown(int keynum)
{
	return syscall(UI_KEY_ISDOWN, keynum);
}

qbool
trap_Key_GetOverstrikeMode(void)
{
	return syscall(UI_KEY_GETOVERSTRIKEMODE);
}

void
trap_Key_SetOverstrikeMode(qbool state)
{
	syscall(UI_KEY_SETOVERSTRIKEMODE, state);
}

void
trap_Key_ClearStates(void)
{
	syscall(UI_KEY_CLEARSTATES);
}

int
trap_Key_GetCatcher(void)
{
	return syscall(UI_KEY_GETCATCHER);
}

void
trap_Key_SetCatcher(int catcher)
{
	syscall(UI_KEY_SETCATCHER, catcher);
}

void
trap_GetClipboardData(char *buf, int bufsize)
{
	syscall(UI_GETCLIPBOARDDATA, buf, bufsize);
}

void
trap_GetClientState(uiClientState_t *state)
{
	syscall(UI_GETCLIENTSTATE, state);
}

void
trap_GetGlconfig(Glconfig *glconfig)
{
	syscall(UI_GETGLCONFIG, glconfig);
}

int
trap_GetConfigString(int index, char* buff, int buffsize)
{
	return syscall(UI_GETCONFIGSTRING, index, buff, buffsize);
}

int
trap_LAN_GetServerCount(int source)
{
	return syscall(UI_LAN_GETSERVERCOUNT, source);
}

void
trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen)
{
	syscall(UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen);
}

void
trap_LAN_GetServerInfo(int source, int n, char *buf, int buflen)
{
	syscall(UI_LAN_GETSERVERINFO, source, n, buf, buflen);
}

int
trap_LAN_GetServerPing(int source, int n)
{
	return syscall(UI_LAN_GETSERVERPING, source, n);
}

int
trap_LAN_GetPingQueueCount(void)
{
	return syscall(UI_LAN_GETPINGQUEUECOUNT);
}

int
trap_LAN_ServerStatus(const char *serverAddress, char *serverStatus, int maxLen)
{
	return syscall(UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen);
}

void
trap_LAN_SaveCachedServers(void)
{
	syscall(UI_LAN_SAVECACHEDSERVERS);
}

void
trap_LAN_LoadCachedServers(void)
{
	syscall(UI_LAN_LOADCACHEDSERVERS);
}

void
trap_LAN_ResetPings(int n)
{
	syscall(UI_LAN_RESETPINGS, n);
}

void
trap_LAN_ClearPing(int n)
{
	syscall(UI_LAN_CLEARPING, n);
}

void
trap_LAN_GetPing(int n, char *buf, int buflen, int *pingtime)
{
	syscall(UI_LAN_GETPING, n, buf, buflen, pingtime);
}

void
trap_LAN_GetPingInfo(int n, char *buf, int buflen)
{
	syscall(UI_LAN_GETPINGINFO, n, buf, buflen);
}

void
trap_LAN_MarkServerVisible(int source, int n, qbool visible)
{
	syscall(UI_LAN_MARKSERVERVISIBLE, source, n, visible);
}

int
trap_LAN_ServerIsVisible(int source, int n)
{
	return syscall(UI_LAN_SERVERISVISIBLE, source, n);
}

qbool
trap_LAN_UpdateVisiblePings(int source)
{
	return syscall(UI_LAN_UPDATEVISIBLEPINGS, source);
}

int
trap_LAN_AddServer(int source, const char *name, const char *addr)
{
	return syscall(UI_LAN_ADDSERVER, source, name, addr);
}

void
trap_LAN_RemoveServer(int source, const char *addr)
{
	syscall(UI_LAN_REMOVESERVER, source, addr);
}

int
trap_LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	return syscall(UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2);
}

int
trap_MemoryRemaining(void)
{
	return syscall(UI_MEMORY_REMAINING);
}

int
trap_RealTime(Qtime *qtime)
{
	return syscall(UI_REAL_TIME, qtime);
}

/* this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate) */
int
trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width,
		       int height,
		       int bits)
{
	return syscall(UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height,
		bits);
}

/* stops playing the cinematic and ends it.  should always return FMV_EOF
* cinematics must be stopped in reverse order of when they are started */
e_status
trap_CIN_StopCinematic(int handle)
{
	return syscall(UI_CIN_STOPCINEMATIC, handle);
}


/* will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached. */
e_status
trap_CIN_RunCinematic(int handle)
{
	return syscall(UI_CIN_RUNCINEMATIC, handle);
}


/* draws the current frame */
void
trap_CIN_DrawCinematic(int handle)
{
	syscall(UI_CIN_DRAWCINEMATIC, handle);
}


/* allows you to resize the animation dynamically */
void
trap_CIN_SetExtents(int handle, int x, int y, int w, int h)
{
	syscall(UI_CIN_SETEXTENTS, handle, x, y, w, h);
}


void
trap_R_RemapShader(const char *oldShader, const char *newShader,
		   const char *timeOffset)
{
	syscall(UI_R_REMAP_SHADER, oldShader, newShader, timeOffset);
}
