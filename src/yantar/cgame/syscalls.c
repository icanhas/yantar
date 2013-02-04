/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 * cg_syscalls.c -- this file is only included when building a dll
 * cg_syscalls.asm is included instead when building a qvm */
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

#include "local.h"

static intptr_t (QDECL *syscall)(intptr_t arg,
				 ...) = (intptr_t (QDECL*)(intptr_t, ...))-1;


Q_EXPORT void
dllEntry(intptr_t (QDECL  *syscallptr)(intptr_t arg,...))
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
trap_Print(const char *fmt)
{
	syscall(CG_PRINT, fmt);
}

void
trap_Error(const char *fmt)
{
	syscall(CG_ERROR, fmt);
	/* shut up GCC warning about returning functions, because we know better */
	exit(1);
}

int
trap_Milliseconds(void)
{
	return syscall(CG_MILLISECONDS);
}

void
trap_cvarregister(Vmcvar *vmCvar, const char *varName,
		   const char *defaultValue,
		   int flags)
{
	syscall(CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags);
}

void
trap_cvarupdate(Vmcvar *vmCvar)
{
	syscall(CG_CVAR_UPDATE, vmCvar);
}

void
trap_cvarsetstr(const char *var_name, const char *value)
{
	syscall(CG_CVAR_SET, var_name, value);
}

void
trap_cvargetstrbuf(const char *var_name, char *buffer, int bufsize)
{
	syscall(CG_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}

int
trap_Argc(void)
{
	return syscall(CG_ARGC);
}

void
trap_Argv(int n, char *buffer, int bufferLength)
{
	syscall(CG_ARGV, n, buffer, bufferLength);
}

void
trap_Args(char *buffer, int bufferLength)
{
	syscall(CG_ARGS, buffer, bufferLength);
}

int
trap_fsopen(const char *qpath, Fhandle *f, Fsmode mode)
{
	return syscall(CG_FS_FOPENFILE, qpath, f, mode);
}

void
trap_fsread(void *buffer, int len, Fhandle f)
{
	syscall(CG_FS_READ, buffer, len, f);
}

void
trap_fswrite(const void *buffer, int len, Fhandle f)
{
	syscall(CG_FS_WRITE, buffer, len, f);
}

void
trap_fsclose(Fhandle f)
{
	syscall(CG_FS_FCLOSEFILE, f);
}

int
trap_fsseek(Fhandle f, long offset, int origin)
{
	return syscall(CG_FS_SEEK, f, offset, origin);
}

void
trap_SendConsoleCommand(const char *text)
{
	syscall(CG_SENDCONSOLECOMMAND, text);
}

void
trap_AddCommand(const char *cmdName)
{
	syscall(CG_ADDCOMMAND, cmdName);
}

void
trap_RemoveCommand(const char *cmdName)
{
	syscall(CG_REMOVECOMMAND, cmdName);
}

void
trap_SendClientCommand(const char *s)
{
	syscall(CG_SENDCLIENTCOMMAND, s);
}

void
trap_UpdateScreen(void)
{
	syscall(CG_UPDATESCREEN);
}

void
trap_CM_LoadMap(const char *mapname)
{
	syscall(CG_CM_LOADMAP, mapname);
}

int
trap_CM_NumInlineModels(void)
{
	return syscall(CG_CM_NUMINLINEMODELS);
}

Cliphandle
trap_CM_InlineModel(int index)
{
	return syscall(CG_CM_INLINEMODEL, index);
}

Cliphandle
trap_CM_TempBoxModel(const Vec3 mins, const Vec3 maxs)
{
	return syscall(CG_CM_TEMPBOXMODEL, mins, maxs);
}

Cliphandle
trap_CM_TempCapsuleModel(const Vec3 mins, const Vec3 maxs)
{
	return syscall(CG_CM_TEMPCAPSULEMODEL, mins, maxs);
}

int
trap_CM_PointContents(const Vec3 p, Cliphandle model)
{
	return syscall(CG_CM_POINTCONTENTS, p, model);
}

int
trap_CM_TransformedPointContents(const Vec3 p, Cliphandle model,
				 const Vec3 origin,
				 const Vec3 angles)
{
	return syscall(CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles);
}

void
trap_CM_BoxTrace(Trace *results, const Vec3 start, const Vec3 end,
		 const Vec3 mins, const Vec3 maxs,
		 Cliphandle model, int brushmask)
{
	syscall(CG_CM_BOXTRACE, results, start, end, mins, maxs, model,
		brushmask);
}

void
trap_CM_CapsuleTrace(Trace *results, const Vec3 start, const Vec3 end,
		     const Vec3 mins, const Vec3 maxs,
		     Cliphandle model, int brushmask)
{
	syscall(CG_CM_CAPSULETRACE, results, start, end, mins, maxs, model,
		brushmask);
}

void
trap_CM_TransformedBoxTrace(Trace *results, const Vec3 start,
			    const Vec3 end,
			    const Vec3 mins, const Vec3 maxs,
			    Cliphandle model, int brushmask,
			    const Vec3 origin,
			    const Vec3 angles)
{
	syscall(CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs,
		model, brushmask, origin,
		angles);
}

void
trap_CM_TransformedCapsuleTrace(Trace *results, const Vec3 start,
				const Vec3 end,
				const Vec3 mins, const Vec3 maxs,
				Cliphandle model, int brushmask,
				const Vec3 origin,
				const Vec3 angles)
{
	syscall(CG_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs,
		model, brushmask, origin,
		angles);
}

int
trap_CM_MarkFragments(int numPoints, const Vec3 *points,
		      const Vec3 projection,
		      int maxPoints, Vec3 pointBuffer,
		      int maxFragments, Markfrag *fragmentBuffer)
{
	return syscall(CG_CM_MARKFRAGMENTS, numPoints, points, projection,
		maxPoints, pointBuffer, maxFragments,
		fragmentBuffer);
}

void
trap_sndstartsound(Vec3 origin, int entityNum, int entchannel, Handle sfx)
{
	syscall(CG_S_STARTSOUND, origin, entityNum, entchannel, sfx);
}

void
trap_sndstartlocalsound(Handle sfx, int channelNum)
{
	syscall(CG_S_STARTLOCALSOUND, sfx, channelNum);
}

void
trap_sndclearloops(qbool killall)
{
	syscall(CG_S_CLEARLOOPINGSOUNDS, killall);
}

void
trap_sndaddloop(int entityNum, const Vec3 origin, const Vec3 velocity,
		       Handle sfx)
{
	syscall(CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, sfx);
}

void
trap_sndaddrealloop(int entityNum, const Vec3 origin,
			   const Vec3 velocity,
			   Handle sfx)
{
	syscall(CG_S_ADDREALLOOPINGSOUND, entityNum, origin, velocity, sfx);
}

void
trap_sndstoploop(int entityNum)
{
	syscall(CG_S_STOPLOOPINGSOUND, entityNum);
}

void
trap_sndupdateentpos(int entityNum, const Vec3 origin)
{
	syscall(CG_S_UPDATEENTITYPOSITION, entityNum, origin);
}

void
trap_sndrespatialize(int entityNum, const Vec3 origin, Vec3 axis[3],
		    int inwater)
{
	syscall(CG_S_RESPATIALIZE, entityNum, origin, axis, inwater);
}

Handle
trap_sndregister(const char *sample, qbool compressed)
{
	return syscall(CG_S_REGISTERSOUND, sample, compressed);
}

void
trap_sndstartbackgroundtrack(const char *intro, const char *loop)
{
	syscall(CG_S_STARTBACKGROUNDTRACK, intro, loop);
}

void
trap_R_LoadWorldMap(const char *mapname)
{
	syscall(CG_R_LOADWORLDMAP, mapname);
}

Handle
trap_R_RegisterModel(const char *name)
{
	return syscall(CG_R_REGISTERMODEL, name);
}

Handle
trap_R_RegisterSkin(const char *name)
{
	return syscall(CG_R_REGISTERSKIN, name);
}

Handle
trap_R_RegisterShader(const char *name)
{
	return syscall(CG_R_REGISTERSHADER, name);
}

Handle
trap_R_RegisterShaderNoMip(const char *name)
{
	return syscall(CG_R_REGISTERSHADERNOMIP, name);
}

void
trap_R_RegisterFont(const char *fontName, int pointSize, Fontinfo *font)
{
	syscall(CG_R_REGISTERFONT, fontName, pointSize, font);
}

void
trap_R_ClearScene(void)
{
	syscall(CG_R_CLEARSCENE);
}

void
trap_R_AddRefEntityToScene(const Refent *re)
{
	syscall(CG_R_ADDREFENTITYTOSCENE, re);
}

void
trap_R_AddPolyToScene(Handle hShader, int numVerts, const Polyvert *verts)
{
	syscall(CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts);
}

void
trap_R_AddPolysToScene(Handle hShader, int numVerts, const Polyvert *verts,
		       int num)
{
	syscall(CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, num);
}

int
trap_R_LightForPoint(Vec3 point, Vec3 ambientLight, Vec3 directedLight,
		     Vec3 lightDir)
{
	return syscall(CG_R_LIGHTFORPOINT, point, ambientLight, directedLight,
		lightDir);
}

void
trap_R_AddLightToScene(const Vec3 org, float intensity, float r, float g,
		       float b)
{
	syscall(CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(
			r), PASSFLOAT(g), PASSFLOAT(b));
}

void
trap_R_AddAdditiveLightToScene(const Vec3 org, float intensity, float r,
			       float g,
			       float b)
{
	syscall(CG_R_ADDADDITIVELIGHTTOSCENE, org, PASSFLOAT(
			intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b));
}

void
trap_R_RenderScene(const Refdef *fd)
{
	syscall(CG_R_RENDERSCENE, fd);
}

void
trap_R_SetColor(const float *rgba)
{
	syscall(CG_R_SETCOLOR, rgba);
}

void
trap_R_DrawStretchPic(float x, float y, float w, float h,
		      float s1, float t1, float s2, float t2, Handle hShader)
{
	syscall(CG_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(
			w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(
			t1), PASSFLOAT(s2),
		PASSFLOAT(t2), hShader);
}

void
trap_R_ModelBounds(Cliphandle model, Vec3 mins, Vec3 maxs)
{
	syscall(CG_R_MODELBOUNDS, model, mins, maxs);
}

int
trap_R_LerpTag(Orient *tag, Cliphandle mod, int startFrame,
	       int endFrame,
	       float frac,
	       const char *tagName)
{
	return syscall(CG_R_LERPTAG, tag, mod, startFrame, endFrame,
		PASSFLOAT(frac), tagName);
}

void
trap_R_RemapShader(const char *oldShader, const char *newShader,
		   const char *timeOffset)
{
	syscall(CG_R_REMAP_SHADER, oldShader, newShader, timeOffset);
}

void
trap_GetGlconfig(Glconfig *glconfig)
{
	syscall(CG_GETGLCONFIG, glconfig);
}

void
trap_GetGameState(Gamestate *gamestate)
{
	syscall(CG_GETGAMESTATE, gamestate);
}

void
trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime)
{
	syscall(CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime);
}

qbool
trap_GetSnapshot(int snapshotNumber, Snap *snapshot)
{
	return syscall(CG_GETSNAPSHOT, snapshotNumber, snapshot);
}

qbool
trap_GetServerCommand(int serverCommandNumber)
{
	return syscall(CG_GETSERVERCOMMAND, serverCommandNumber);
}

int
trap_GetCurrentCmdNumber(void)
{
	return syscall(CG_GETCURRENTCMDNUMBER);
}

qbool
trap_GetUserCmd(int cmdNumber, Usrcmd *ucmd)
{
	return syscall(CG_GETUSERCMD, cmdNumber, ucmd);
}

void
trap_SetUserCmdValue(int weap1, int weap2, float sensitivityscale)
{
	syscall(CG_SETUSERCMDVALUE, weap1, weap2, PASSFLOAT(sensitivityscale));
}

void
testPrintInt(char *string, int i)
{
	syscall(CG_TESTPRINTINT, string, i);
}

void
testPrintFloat(char *string, float f)
{
	syscall(CG_TESTPRINTFLOAT, string, PASSFLOAT(f));
}

int
trap_MemoryRemaining(void)
{
	return syscall(CG_MEMORY_REMAINING);
}

qbool
trap_Key_IsDown(int keynum)
{
	return syscall(CG_KEY_ISDOWN, keynum);
}

int
trap_Key_GetCatcher(void)
{
	return syscall(CG_KEY_GETCATCHER);
}

void
trap_Key_SetCatcher(int catcher)
{
	syscall(CG_KEY_SETCATCHER, catcher);
}

int
trap_Key_GetKey(const char *binding)
{
	return syscall(CG_KEY_GETKEY, binding);
}

void
trap_sndstopbackgroundtrack(void)
{
	syscall(CG_S_STOPBACKGROUNDTRACK);
}

int
trap_RealTime(Qtime *qtime)
{
	return syscall(CG_REAL_TIME, qtime);
}

void
trap_snapv3(float *v)
{
	syscall(CG_SNAPVECTOR, v);
}

/* this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate) */
int
trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width,
		       int height,
		       int bits)
{
	return syscall(CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height,
		bits);
}

/* stops playing the cinematic and ends it.  should always return FMV_EOF
* cinematics must be stopped in reverse order of when they are started */
Cinstatus
trap_CIN_StopCinematic(int handle)
{
	return syscall(CG_CIN_STOPCINEMATIC, handle);
}


/* will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached. */
Cinstatus
trap_CIN_RunCinematic(int handle)
{
	return syscall(CG_CIN_RUNCINEMATIC, handle);
}


/* draws the current frame */
void
trap_CIN_DrawCinematic(int handle)
{
	syscall(CG_CIN_DRAWCINEMATIC, handle);
}


/* allows you to resize the animation dynamically */
void
trap_CIN_SetExtents(int handle, int x, int y, int w, int h)
{
	syscall(CG_CIN_SETEXTENTS, handle, x, y, w, h);
}

/*
 * qbool trap_loadCamera( const char *name ) {
 *      return syscall( CG_LOADCAMERA, name );
 * }
 *
 * void trap_startCamera(int time) {
 *      syscall(CG_STARTCAMERA, time);
 * }
 *
 * qbool trap_getCameraInfo( int time, Vec3 *origin, Vec3 *angles) {
 *      return syscall( CG_GETCAMERAINFO, time, origin, angles );
 * }
 */

qbool
trap_GetEntityToken(char *buffer, int bufferSize)
{
	return syscall(CG_GET_ENTITY_TOKEN, buffer, bufferSize);
}

qbool
trap_R_inPVS(const Vec3 p1, const Vec3 p2)
{
	return syscall(CG_R_INPVS, p1, p2);
}
