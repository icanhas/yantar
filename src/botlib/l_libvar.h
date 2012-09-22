/* botlib vars */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
typedef struct  libvar_t libvar_t;

struct libvar_t {
	char		*name;
	char                    *string;
	int		flags;
	qbool		modified;	/* set each time the cvar is changed */
	float		value;
	struct  libvar_s *next;
};

void		LibVarDeAllocAll(void);
libvar_t*	LibVarGet(char *var_name);
char*	LibVarGetString(char *var_name);
float		LibVarGetValue(char *var_name);
libvar_t*	LibVar(char *var_name, char *value);
float		LibVarValue(char *var_name, char *value);
char*	LibVarString(char *var_name, char *value);
void		LibVarSet(char *var_name, char *value);
qbool	LibVarChanged(char *var_name);
void		LibVarSetNotModified(char *var_name);
