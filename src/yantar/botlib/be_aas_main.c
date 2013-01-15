/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_log.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

aas_t aasworld;
libvar_t *saveroutingcache;

void QDECL
AAS_Error(char *fmt, ...)
{
	char str[1024];
	va_list arglist;

	va_start(arglist, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, arglist);
	va_end(arglist);
	botimport.Print(PRT_FATAL, "%s", str);
}

char*
AAS_StringFromIndex(char *indexname, char *stringindex[], int numindexes,
		    int index)
{
	if(!aasworld.indexessetup){
		botimport.Print(PRT_ERROR, "%s: index %d not setup\n", indexname,
			index);
		return "";
	}
	if(index < 0 || index >= numindexes){
		botimport.Print(PRT_ERROR, "%s: index %d out of range\n",
			indexname,
			index);
		return "";
	}
	if(!stringindex[index]){
		if(index)
			botimport.Print(PRT_ERROR,
				"%s: reference to unused index %d\n", indexname,
				index);
		return "";
	}
	return stringindex[index];
}

int
AAS_IndexFromString(char *indexname, char *stringindex[], int numindexes,
		    char *string)
{
	int i;
	if(!aasworld.indexessetup){
		botimport.Print(PRT_ERROR, "%s: index not setup \"%s\"\n",
			indexname,
			string);
		return 0;
	}
	for(i = 0; i < numindexes; i++){
		if(!stringindex[i]) continue;
		if(!Q_stricmp(stringindex[i], string)) return i;
	}
	return 0;
}

char*
AAS_ModelFromIndex(int index)
{
	return AAS_StringFromIndex("ModelFromIndex",
		&aasworld.configstrings[CS_MODELS], MAX_MODELS, index);
}

int
AAS_IndexFromModel(char *modelname)
{
	return AAS_IndexFromString("IndexFromModel",
		&aasworld.configstrings[CS_MODELS], MAX_MODELS, modelname);
}

void
AAS_UpdateStringIndexes(int numconfigstrings, char *configstrings[])
{
	int i;
	/* set string pointers and copy the strings */
	for(i = 0; i < numconfigstrings; i++)
		if(configstrings[i]){
			/* if (aasworld.configstrings[i]) FreeMemory(aasworld.configstrings[i]); */
			aasworld.configstrings[i] =
				(char*)GetMemory(strlen(configstrings[i]) + 1);
			strcpy(aasworld.configstrings[i], configstrings[i]);
		}
	aasworld.indexessetup = qtrue;
}

int
AAS_Loaded(void)
{
	return aasworld.loaded;
}

int
AAS_Initialized(void)
{
	return aasworld.initialized;
}

void
AAS_SetInitialized(void)
{
	aasworld.initialized = qtrue;
	botimport.Print(PRT_MESSAGE, "AAS initialized.\n");
#ifdef DEBUG
	/* create all the routing cache
	 * AAS_CreateAllRoutingCache();
	 *
	 * AAS_RoutingInfo(); */
#endif
}

void
AAS_ContinueInit(float time)
{
	if(!aasworld.loaded) return;
	if(aasworld.initialized) return;
	/* calculate reachability, if not finished return */
	if(AAS_ContinueInitReachability(time)) return;
	AAS_InitClustering();
	/* if reachability has been calculated and an AAS file should be written
	 * or there is a forced data optimization */
	if(aasworld.savefile || ((int)LibVarGetValue("forcewrite"))){
		if((int)LibVarValue("aasoptimize", "0")) AAS_Optimize();
		/* save the AAS file */
		if(AAS_WriteAASFile(aasworld.filename))
			botimport.Print(PRT_MESSAGE, "%s written successfully\n",
				aasworld.filename);
		else
			botimport.Print(PRT_ERROR, "couldn't write %s\n",
				aasworld.filename);
	}
	AAS_InitRouting();
	AAS_SetInitialized();
}

/*
 * called at the start of every frame
 */
int
AAS_StartFrame(float time)
{
	aasworld.time = time;
	AAS_UnlinkInvalidEntities();
	AAS_InvalidateEntities();
	AAS_ContinueInit(time);
	aasworld.frameroutingupdates = 0;
	if(botDeveloper){
		if(LibVarGetValue("showcacheupdates")){
			AAS_RoutingInfo();
			LibVarSet("showcacheupdates", "0");
		}
		if(LibVarGetValue("showmemoryusage")){
			PrintUsedMemorySize();
			LibVarSet("showmemoryusage", "0");
		}
		if(LibVarGetValue("memorydump")){
			PrintMemoryLabels();
			LibVarSet("memorydump", "0");
		}
	}
	if(saveroutingcache->value){
		AAS_WriteRouteCache();
		LibVarSet("saveroutingcache", "0");
	}
	aasworld.numframes++;
	return BLERR_NOERROR;
}

float
AAS_Time(void)
{
	return aasworld.time;
}

void
AAS_ProjectPointOntoVector(Vec3 point, Vec3 vStart, Vec3 vEnd,
			   Vec3 vProj)
{
	Vec3 pVec, vec;

	subv3(point, vStart, pVec);
	subv3(vEnd, vStart, vec);
	normv3(vec);
	/* project onto the directional vector for this segment */
	maddv3(vStart, dotv3(pVec, vec), vec, vProj);
}

int
AAS_LoadFiles(const char *mapname)
{
	int	errnum;
	char	aasfile[MAX_PATH];
/*	char bspfile[MAX_PATH]; */

	strcpy(aasworld.mapname, mapname);
	/* NOTE: first reset the entity links into the AAS areas and BSP leaves
	 * the AAS link heap and BSP link heap are reset after respectively the
	 * AAS file and BSP file are loaded */
	AAS_ResetEntityLinks();
	/* load bsp info */
	AAS_LoadBSPFile();

	/* load the aas file */
	Q_sprintf(aasfile, MAX_PATH, Pmaps "/%s.aas", mapname);
	errnum = AAS_LoadAASFile(aasfile);
	if(errnum != BLERR_NOERROR)
		return errnum;

	botimport.Print(PRT_MESSAGE, "loaded %s\n", aasfile);
	strncpy(aasworld.filename, aasfile, MAX_PATH);
	return BLERR_NOERROR;
}

/*
 * called every time a map changes
 */
int
AAS_LoadMap(const char *mapname)
{
	int errnum;

	/* if no mapname is provided then the string indexes are updated */
	if(!mapname)
		return 0;
	aasworld.initialized = qfalse;
	/* NOTE: free the routing caches before loading a new map because
	 * to free the caches the old number of areas, number of clusters
	 * and number of areas in a clusters must be available */
	AAS_FreeRoutingCaches();
	errnum = AAS_LoadFiles(mapname);
	if(errnum != BLERR_NOERROR){
		aasworld.loaded = qfalse;
		return errnum;
	}
	AAS_InitSettings();
	AAS_InitAASLinkHeap();
	AAS_InitAASLinkedEntities();
	AAS_InitReachability();
	AAS_InitAlternativeRouting();
	return 0;
}

/*
 * called when the library is first loaded
 */
int
AAS_Setup(void)
{
	aasworld.maxclients	= (int)LibVarValue("maxclients", "128");
	aasworld.maxentities	= (int)LibVarValue("maxentities", "1024");
	/* as soon as it's set to 1 the routing cache will be saved */
	saveroutingcache = LibVar("saveroutingcache", "0");
	/* allocate memory for the entities */
	if(aasworld.entities) FreeMemory(aasworld.entities);
	aasworld.entities = (aas_entity_t*)GetClearedHunkMemory(
		aasworld.maxentities * sizeof(aas_entity_t));
	AAS_InvalidateEntities();
	/* force some recalculations
	 * LibVarSet("forceclustering", "1");			//force clustering calculation
	 * LibVarSet("forcereachability", "1");		//force reachability calculation */
	aasworld.numframes = 0;
	return BLERR_NOERROR;
}

void
AAS_Shutdown(void)
{
	AAS_ShutdownAlternativeRouting();
	AAS_DumpBSPData();
	AAS_FreeRoutingCaches();
	AAS_FreeAASLinkHeap();
	AAS_FreeAASLinkedEntities();
	AAS_DumpAASData();
	if(aasworld.entities) FreeMemory(aasworld.entities);
	Q_Memset(&aasworld, 0, sizeof(aas_t));
	aasworld.initialized = qfalse;
	/* NOTE: as soon as a new .bsp file is loaded the .bsp file memory is
	 * freed an reallocated, so there's no need to free that memory here
	 * print shutdown */
	botimport.Print(PRT_MESSAGE, "AAS shutdown.\n");
}
