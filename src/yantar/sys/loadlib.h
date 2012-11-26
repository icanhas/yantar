/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef DEDICATED
#       ifdef _WIN32
#               include <windows.h>
#               define Sys_LoadLibrary(f)	(void*)LoadLibrary(f)
#               define Sys_UnloadLibrary(h)	FreeLibrary((HMODULE)h)
#               define Sys_LoadFunction(h,fn)	(void*)GetProcAddress( \
	(HMODULE)h,fn)
#               define Sys_LibraryError()	"unknown"
#       else
#       include <dlfcn.h>
#               define Sys_LoadLibrary(f)	dlopen(f,RTLD_NOW)
#               define Sys_UnloadLibrary(h)	dlclose(h)
#               define Sys_LoadFunction(h,fn)	dlsym(h,fn)
#               define Sys_LibraryError()	dlerror()
#       endif
#else
#       ifdef USE_LOCAL_HEADERS
#               include "SDL.h"
#               include "SDL_loadso.h"
#       else
#               include <SDL.h>
#               include <SDL_loadso.h>
#       endif
#       define Sys_LoadLibrary(f)	SDL_LoadObject(f)
#       define Sys_UnloadLibrary(h)	SDL_UnloadObject(h)
#       define Sys_LoadFunction(h,fn)	SDL_LoadFunction(h,fn)
#       define Sys_LibraryError()	SDL_GetError()
#endif

void *QDECL Sys_LoadDll(const char *name, qbool useSystemLib);
