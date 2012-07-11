/* botlib vars */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
