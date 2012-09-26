/* bot library variables */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "l_memory.h"
#include "l_libvar.h"

libvar_t *libvarlist = NULL;	/* list with library variables */

float
LibVarStringValue(char *string)
{
	int dotfound	= 0;
	float value	= 0;

	while(*string){
		if(*string < '0' || *string > '9'){
			if(dotfound || *string != '.')
				return 0;
			else{
				dotfound = 10;
				string++;
			}
		}
		if(dotfound){
			value = value +
				(float)(*string - '0') / (float)dotfound;
			dotfound *= 10;
		}else
			value = value * 10.0 + (float)(*string - '0');
		string++;
	}
	return value;
}

libvar_t *
LibVarAlloc(char *var_name)
{
	libvar_t *v;

	v = (libvar_t*)GetMemory(sizeof(libvar_t));
	Q_Memset(v, 0, sizeof(libvar_t));
	v->name = (char*)GetMemory(strlen(var_name)+1);
	strcpy(v->name, var_name);
	v->next = libvarlist;
	libvarlist = v;
	return v;
}

void
LibVarDeAlloc(libvar_t *v)
{
	if(v->string) FreeMemory(v->string);
	FreeMemory(v->name);
	FreeMemory(v);
}

/* removes all library variables */
void
LibVarDeAllocAll(void)
{
	libvar_t *v;

	for(v = libvarlist; v; v = libvarlist){
		libvarlist = libvarlist->next;
		LibVarDeAlloc(v);
	}
	libvarlist = NULL;
}

/* gets the library variable with the given name */
libvar_t *
LibVarGet(char *var_name)
{
	libvar_t *v;

	for(v = libvarlist; v; v = v->next)
		if(!Q_stricmp(v->name, var_name))
			return v;
	return NULL;
}

/* gets the string of the library variable with the given name */
char *
LibVarGetString(char *var_name)
{
	libvar_t *v;

	v = LibVarGet(var_name);
	if(v)
		return v->string;
	else
		return "";
}

/* gets the value of the library variable with the given name */
float
LibVarGetValue(char *var_name)
{
	libvar_t *v;

	v = LibVarGet(var_name);
	if(v)
		return v->value;
	else
		return 0;
}

/* creates the library variable if not existing already and returns it */
libvar_t *
LibVar(char *var_name, char *value)
{
	libvar_t *v;
	v = LibVarGet(var_name);
	if(v)
		return v;
	/* create new variable */
	v = LibVarAlloc(var_name);
	v->string = (char*)GetMemory(strlen(value) + 1);
	strcpy(v->string, value);
	v->value = LibVarStringValue(v->string);
	v->modified = qtrue;
	return v;
}

/* creates the library variable if not existing already and returns the value string */
char *
LibVarString(char *var_name, char *value)
{
	libvar_t *v;

	v = LibVar(var_name, value);
	return v->string;
}

/* creates the library variable if not existing already and returns the value */
float
LibVarValue(char *var_name, char *value)
{
	libvar_t *v;

	v = LibVar(var_name, value);
	return v->value;
}

/* sets the library variable */
void
LibVarSet(char *var_name, char *value)
{
	libvar_t *v;

	v = LibVarGet(var_name);
	if(v)
		FreeMemory(v->string);
	else
		v = LibVarAlloc(var_name);
	v->string = (char*)GetMemory(strlen(value) + 1);
	strcpy(v->string, value);
	v->value = LibVarStringValue(v->string);
	v->modified = qtrue;
}

/* returns true if the library variable has been modified */
qbool
LibVarChanged(char *var_name)
{
	libvar_t *v;

	v = LibVarGet(var_name);
	if(v)
		return v->modified;
	else
		return qfalse;
}

/* sets the library variable to unmodified */
void
LibVarSetNotModified(char *var_name)
{
	libvar_t *v;

	v = LibVarGet(var_name);
	if(v)
		v->modified = qfalse;
}
