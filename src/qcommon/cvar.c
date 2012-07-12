/* Dynamic variable tracking */
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

#include "q_shared.h"
#include "qcommon.h"

/*
 * cvar_t variables are used to hold scalar or string variables that can be changed
 * or displayed at the console or prog code as well as accessed directly
 * in C code.
 *
 * The user can access cvars from the console in three ways:
 * r_draworder			prints the current value
 * r_draworder 0		sets the current value to 0
 * set r_draworder 0	as above, but creates the cvar if not present
 *
 * Cvars are restricted from having the same names as commands to keep this
 * interface from being ambiguous.
 *
 * The are also occasionally used to communicated information between different
 * modules of the program.
 */

enum {
	MAX_CVARS = 1024,
	HASH_SIZE = 512
};

int cvar_modifiedFlags;

static cvar_t	*cvar_vars = nil;
static cvar_t   *cvar_cheats;
static cvar_t	cvar_indexes[MAX_CVARS];
static int	cvar_numIndexes;
static cvar_t *hashTable[HASH_SIZE];

static qbool
Cvar_ValidateString(const char *s)
{
	if(s == nil)
		return qfalse;
	if(strchr(s, '\\'))
		return qfalse;
	if(strchr(s, '\"'))
		return qfalse;
	if(strchr(s, ';'))
		return qfalse;
	return qtrue;
}

static cvar_t *
Cvar_FindVar(const char *var_name)
{
	cvar_t	*var;
	long	hash;

	hash = Q_HashString(var_name, HASH_SIZE);

	for(var=hashTable[hash]; var != nil; var=var->hashNext)
		if(!Q_stricmp(var_name, var->name))
			return var;

	return nil;
}

/* returns 0 if not defined or non numeric */
float
Cvar_VariableValue(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if(var == nil)
		return 0;
	return var->value;
}

int
Cvar_VariableIntegerValue(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if(var == nil)
		return 0;
	return var->integer;
}

/* returns an empty string if not defined */
char *
Cvar_VariableString(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if(var == nil)
		return "";
	return var->string;
}

void
Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if(var == nil)
		*buffer = 0;
	else
		Q_strncpyz(buffer, var->string, bufsize);
}

int
Cvar_Flags(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if(var == nil)
		return CVAR_NONEXISTENT;
	else{
		if(var->modified)
			return var->flags | CVAR_MODIFIED;
		else
			return var->flags;
	}
}

void
Cvar_CommandCompletion(void (*callback)(const char *s))
{
	cvar_t *cvar;

	for(cvar = cvar_vars; cvar; cvar = cvar->next)
		if(cvar->name != nil)
			callback(cvar->name);
}

static const char *
Cvar_Validate(cvar_t *var, const char *value, qbool warn)
{
	static char	s[MAX_CVAR_VALUE_STRING];
	float valuef;
	qbool		changed = qfalse;

	if(!var->validate)
		return value;

	if(value == nil)
		return value;

	if(Q_isanumber(value)){
		valuef = atof(value);
		if(var->integral)
			if(!Q_isintegral(valuef)){
				if(warn)
					Q_Printf("WARNING: cvar '%s' must"
						   " be integral", var->name);

				valuef	= (int)valuef;
				changed = qtrue;
			}
	}else{
		if(warn)
			Q_Printf("WARNING: cvar '%s' must be numeric",
				var->name);

		valuef	= atof(var->resetString);
		changed = qtrue;
	}

	if(valuef < var->min){
		if(warn){
			if(changed)
				Q_Printf(" and is");
			else
				Q_Printf("WARNING: cvar '%s'", var->name);

			if(Q_isintegral(var->min))
				Q_Printf(" out of range (min %d)",
					(int)var->min);
			else
				Q_Printf(" out of range (min %f)", var->min);
		}

		valuef	= var->min;
		changed = qtrue;
	}else if(valuef > var->max){
		if(warn){
			if(changed)
				Q_Printf(" and is");
			else
				Q_Printf("WARNING: cvar '%s'", var->name);

			if(Q_isintegral(var->max))
				Q_Printf(" out of range (max %d)",
					(int)var->max);
			else
				Q_Printf(" out of range (max %f)", var->max);
		}

		valuef	= var->max;
		changed = qtrue;
	}

	if(changed){
		if(Q_isintegral(valuef)){
			Q_sprintf(s, sizeof(s), "%d", (int)valuef);

			if(warn)
				Q_Printf(", setting to %d\n", (int)valuef);
		}else{
			Q_sprintf(s, sizeof(s), "%f", valuef);

			if(warn)
				Q_Printf(", setting to %f\n", valuef);
		}

		return s;
	}else
		return value;
}

/* set value of an existing cvar */
static void
setNewVal(cvar_t *var, const char *var_name, const char *var_value, int flags)
{
	var_value = Cvar_Validate(var, var_value, qfalse);
	/*
	 * if the C code is now specifying a variable that the user already
	 * set a value for, take the new value as the reset value
	 */
	if(var->flags & CVAR_USER_CREATED){
		var->flags &= ~CVAR_USER_CREATED;
		Z_Free(var->resetString);
		var->resetString = CopyString(var_value);

		if(flags & CVAR_ROM){
			/*
			 * this variable was set by the user,
			 * so force it to value given by the engine.
			 */
			if(var->latchedString != '\0')
				Z_Free(var->latchedString);
			var->latchedString = CopyString(var_value);
		}
	}

	/* make sure the game code cannot mark engine-added variables as gamecode vars */
	if(var->flags & CVAR_VM_CREATED){
		if(!(flags & CVAR_VM_CREATED))
			var->flags &= ~CVAR_VM_CREATED;
	}else if(flags & CVAR_VM_CREATED)
		flags &= ~CVAR_VM_CREATED;

	/* make sure servers cannot mark engine-added variables as SERVER_CREATED */
	if(var->flags & CVAR_SERVER_CREATED){
		if(!(flags & CVAR_SERVER_CREATED))
			var->flags &= ~CVAR_SERVER_CREATED;
	}else if(flags & CVAR_SERVER_CREATED)
		flags &= ~CVAR_SERVER_CREATED;

	var->flags |= flags;

	/* only allow one non-empty reset string without a warning */
	if(var->resetString[0] == '\0'){
		/* we don't have a reset string yet */
		Z_Free(var->resetString);
		var->resetString = CopyString(var_value);
	}else if(var_value[0] && strcmp(var->resetString, var_value))
		Q_DPrintf("Warning: cvar \"%s\" given initial values: \"%s\""
			    " and \"%s\"\n", var_name, var->resetString, var_value);
	/* if we have a latched string, take that value now */
	if(var->latchedString != nil){
		char *s;

		s = var->latchedString;
		var->latchedString = nil;	/* otherwise cvar_set2 would free it */
		Cvar_Set2(var_name, s, qtrue);
		Z_Free(s);
	}
	/* ZOID--needs to be set so that cvars the game sets as
	 * SERVERINFO get sent to clients */
	cvar_modifiedFlags |= flags;
}

/*
 * If the variable already exists, the value will not be set unless
 * CVAR_ROM. The flags will be or'ed in if the variable exists.
 */
cvar_t *
Cvar_Get(const char *var_name, const char *var_value, int flags)
{
	cvar_t	*var;
	long	hash;
	int	index;

	if(var_name == nil || var_value == nil)
		Q_Error(ERR_FATAL, "Cvar_Get: nil parameter");

	if(!Cvar_ValidateString(var_name)){
		Q_Printf("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

#if 0	/* FIXME: values with backslash happen */
	if(!Cvar_ValidateString(var_value)){
		Q_Printf("invalid cvar value string: %s\n", var_value);
		var_value = "BADVALUE";
	}
#endif

	var = Cvar_FindVar(var_name);
	if(var != nil){
		setNewVal(var, var_name, var_value, flags);
		return var;
	}

	/* allocate a new cvar... */
	/* find a free cvar */
	for(index = 0; index < MAX_CVARS; index++)
		if(cvar_indexes[index].name == nil)
			break;

	if(index >= MAX_CVARS){
		if(!com_errorEntered)
			Q_Error(
				ERR_FATAL,
				"Error: Too many cvars, cannot create a new one!");
		return nil;
	}

	var = &cvar_indexes[index];

	if(index >= cvar_numIndexes)
		cvar_numIndexes = index + 1;

	var->name	= CopyString (var_name);
	var->string	= CopyString (var_value);
	var->modified	= qtrue;
	var->modificationCount = 1;
	var->value	= atof(var->string);
	var->integer	= atoi(var->string);
	var->resetString = CopyString(var_value);
	var->validate = qfalse;

	/* link the variable in */
	var->next = cvar_vars;
	if(cvar_vars != nil)
		cvar_vars->prev = var;

	var->prev	= nil;
	cvar_vars	= var;

	var->flags = flags;
	/* note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo) */
	cvar_modifiedFlags |= var->flags;

	hash = Q_HashString(var_name, HASH_SIZE);
	var->hashIndex = hash;

	var->hashNext = hashTable[hash];
	if(hashTable[hash])
		hashTable[hash]->hashPrev = var;

	var->hashPrev	= nil;
	hashTable[hash] = var;

	return var;
}

/*
 * Prints the description, value, default, and latched string of
 * the given variable
 */
void
Cvar_Print(cvar_t *v)
{
	Q_Printf("%s: ", v->name);
	if(v->desc != nil)
		Q_Printf("%s\n", v->desc);
	else
		Q_Printf("no description\n");

	Q_Printf("%s is \"%s" S_COLOR_WHITE "\"",
		v->name, v->string);
	if(!(v->flags & CVAR_ROM)){
		if(Q_stricmp(v->string, v->resetString) == 0)
			Q_Printf(", the default");
		else
			Q_Printf(", default: \"%s" S_COLOR_WHITE "\"",
				v->resetString);
	}
	Q_Printf("\n");
	if(v->latchedString != nil)
		Q_Printf("latched: \"%s\"\n", v->latchedString);
}

/* sets the description string of the named cvar, if it exists */
void
Cvar_SetDesc(const char *name, const char *desc)
{
	cvar_t *cv;

	cv = Cvar_FindVar(name);
	if((cv == nil) || (desc == nil))
		return;
	if(cv->desc != nil)
		Z_Free(cv->desc);
	cv->desc = CopyString(desc);
}

/* same as Cvar_Set, but allows more control over setting of cvar */
cvar_t *
Cvar_Set2(const char *var_name, const char *value, qbool force)
{
	cvar_t *var;

/*	Q_DPrintf( "Cvar_Set2: %s %s\n", var_name, value ); */

	if(!Cvar_ValidateString(var_name)){
		Q_Printf("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

#if 0	/* FIXME */
	if(value && !Cvar_ValidateString(value)){
		Q_Printf("invalid cvar value string: %s\n", value);
		var_value = "BADVALUE";
	}
#endif

	var = Cvar_FindVar(var_name);
	if(var == nil){
		if(value == nil)
			return nil;
		/* create it */
		if(!force)
			return Cvar_Get(var_name, value, CVAR_USER_CREATED);
		else
			return Cvar_Get(var_name, value, 0);
	}

	if(value == nil)
		value = var->resetString;

	value = Cvar_Validate(var, value, qtrue);

	if((var->flags & CVAR_LATCH) && (var->latchedString != nil)){
		if(!strcmp(value, var->string)){
			Z_Free(var->latchedString);
			var->latchedString = nil;
			return var;
		}

		if(!strcmp(value, var->latchedString))
			return var;
	}else if(!strcmp(value, var->string))
		return var;

	/* note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo) */
	cvar_modifiedFlags |= var->flags;

	if(!force){
		if(var->flags & CVAR_ROM){
			Q_Printf("%s is read only.\n", var_name);
			return var;
		}

		if(var->flags & CVAR_INIT){
			Q_Printf("%s is write protected.\n", var_name);
			return var;
		}

		if(var->flags & CVAR_LATCH){
			if(var->latchedString){
				if(strcmp(value, var->latchedString) == 0)
					return var;
				Z_Free(var->latchedString);
			}else if(strcmp(value, var->string) == 0)
				return var;

			Q_Printf("%s will be changed upon restarting.\n",
				var_name);
			var->latchedString = CopyString(value);
			var->modified = qtrue;
			var->modificationCount++;
			return var;
		}

		if((var->flags & CVAR_CHEAT) && (cvar_cheats->integer <= 0)){
			Q_Printf ("%s is cheat protected.\n", var_name);
			return var;
		}

	}else if(var->latchedString != nil){
		Z_Free(var->latchedString);
		var->latchedString = nil;
	}

	if(!strcmp(value, var->string))
		return var;	/* not changed */

	var->modified = qtrue;
	var->modificationCount++;

	Z_Free(var->string);	/* free the old value string */

	var->string	= CopyString(value);
	var->value	= atof (var->string);
	var->integer	= atoi (var->string);

	return var;
}

/* will create the variable with no flags if it doesn't exist */
void
Cvar_Set(const char *var_name, const char *value)
{
	Cvar_Set2(var_name, value, qtrue);
}

/* sometimes we set variables from an untrusted source: fail if flags & CVAR_PROTECTED */
void
Cvar_SetSafe(const char *var_name, const char *value)
{
	int flags = Cvar_Flags(var_name);

	if((flags != CVAR_NONEXISTENT) && (flags & CVAR_PROTECTED)){
		if(value != nil)
			Q_Error(ERR_DROP, "Restricted source tried to set "
					    "\"%s\" to \"%s\"", var_name, value);
		else
			Q_Error(ERR_DROP, "Restricted source tried to "
					    "modify \"%s\"", var_name);
		return;
	}
	Cvar_Set(var_name, value);
}

/* don't set the cvar immediately */
void
Cvar_SetLatched(const char *var_name, const char *value)
{
	Cvar_Set2(var_name, value, qfalse);
}

void
Cvar_SetValue(const char *var_name, float value)
{
	char val[32];

	if(value == (int)value)
		Q_sprintf(val, sizeof(val), "%i", (int)value);
	else
		Q_sprintf(val, sizeof(val), "%f", value);
	Cvar_Set(var_name, val);
}

/* expands value to a string and calls Cvar_Set/Cvar_SetSafe */
void
Cvar_SetValueSafe(const char *var_name, float value)
{
	char val[32];

	if(Q_isintegral(value))
		Q_sprintf(val, sizeof(val), "%i", (int)value);
	else
		Q_sprintf(val, sizeof(val), "%f", value);
	Cvar_SetSafe(var_name, val);
}

void
Cvar_Reset(const char *var_name)
{
	Cvar_Set2(var_name, nil, qfalse);
}

void
Cvar_ForceReset(const char *var_name)
{
	Cvar_Set2(var_name, nil, qtrue);
}

/* Any testing variables will be reset to the safe values */
void
Cvar_SetCheatState(void)
{
	cvar_t *var;

	/* set all default vars to the safe value */
	for(var = cvar_vars; var != nil; var = var->next)
		if(var->flags & CVAR_CHEAT){
			/*
			 * the CVAR_LATCHED|CVAR_CHEAT vars might escape the
			 * reset here because of a
			 * different var->latchedString
			 * */
			if(var->latchedString != nil){
				Z_Free(var->latchedString);
				var->latchedString = nil;
			}
			if(strcmp(var->resetString, var->string))
				Cvar_Set(var->name, var->resetString);
		}
}

/*
 * Handles variable inspection and changing from the console
 *
 * called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
 * command. Returns true if the command was a variable reference that
 * was handled. (print or change)
 */
qbool
Cvar_Command(void)
{
	cvar_t *v;

	/* check variables */
	v = Cvar_FindVar(Cmd_Argv(0));
	if(v == nil)
		return qfalse;

	/* perform a variable print or set */
	if(Cmd_Argc() == 1){
		Cvar_Print(v);
		return qtrue;
	}

	/* set the value if forcing isn't required */
	Cvar_Set2(v->name, Cmd_Args(), qfalse);
	return qtrue;
}

/*
 * Cvar_Print_f: Prints the contents of a cvar
 * (preferred over Cvar_Command where cvar names and commands conflict)
 */
void
Cvar_Print_f(void)
{
	char *name;
	cvar_t *cv;

	if(Cmd_Argc() != 2){
		Q_Printf ("usage: print <variable>\n");
		return;
	}

	name = Cmd_Argv(1);

	cv = Cvar_FindVar(name);

	if(cv != nil)
		Cvar_Print(cv);
	else
		Q_Printf("Cvar %s does not exist.\n", name);
}

/*
 * Toggles a cvar for easy single key binding, optionally
 * through a list of given values.
 */
void
Cvar_Toggle_f(void)
{
	int i, c = Cmd_Argc();
	char *curval;

	if(c < 2){
		Q_Printf("usage: toggle <variable> [value1, value2, ...]\n");
		return;
	}

	if(c == 2){
		Cvar_Set2(Cmd_Argv(1), va("%d",
				!Cvar_VariableValue(Cmd_Argv(1))),
			qfalse);
		return;
	}

	if(c == 3){
		Q_Printf("toggle: nothing to toggle to\n");
		return;
	}

	curval = Cvar_VariableString(Cmd_Argv(1));

	/* don't bother checking the last arg for a match since the desired
	 * behaviour is the same as no match (set to the first argument) */
	for(i = 2; i+1 < c; i++)
		if(strcmp(curval, Cmd_Argv(i)) == 0){
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(i + 1), qfalse);
			return;
		}
	/* fallback */
	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), qfalse);
}

/*
 * Allows setting and defining of arbitrary cvars from console,
 * even if they weren't declared in C code.
 */
void
Cvar_Set_f(void)
{
	int c;
	char *cmd;
	cvar_t *v;

	c	= Cmd_Argc();
	cmd	= Cmd_Argv(0);

	if(c < 2){
		Q_Printf("usage: %s <variable> <value>\n", cmd);
		return;
	}
	if(c == 2){
		Cvar_Print_f();
		return;
	}

	v = Cvar_Set2 (Cmd_Argv(1), Cmd_ArgsFrom(2), qfalse);
	if(v == nil)
		return;
	switch(cmd[3]){
	case 'a':
		if(!(v->flags & CVAR_ARCHIVE)){
			v->flags |= CVAR_ARCHIVE;
			cvar_modifiedFlags |= CVAR_ARCHIVE;
		}
		break;
	case 'u':
		if(!(v->flags & CVAR_USERINFO)){
			v->flags |= CVAR_USERINFO;
			cvar_modifiedFlags |= CVAR_USERINFO;
		}
		break;
	case 's':
		if(!(v->flags & CVAR_SERVERINFO)){
			v->flags |= CVAR_SERVERINFO;
			cvar_modifiedFlags |= CVAR_SERVERINFO;
		}
		break;
	}
}

void
Cvar_Reset_f(void)
{
	if(Cmd_Argc() != 2){
		Q_Printf("usage: reset <variable>\n");
		return;
	}
	Cvar_Reset(Cmd_Argv(1));
}

/*
 * Appends lines containing "set variable value" for
 * all variables with the archive flag set.
 */
void
Cvar_WriteVariables(fileHandle_t f)
{
	int	len;	/* total length of name + archival string */
	char	buffer[1024];
	cvar_t *var;

	for(var = cvar_vars; var != nil; var = var->next){
		if((var->name == nil) || Q_stricmp(var->name, "cl_cdkey") == 0)
			continue;

		if(!(var->flags & CVAR_ARCHIVE)){
			continue;
		}

		/* write the latched value, even if it hasn't taken effect yet */
		if(var->latchedString != nil){
			len = strlen(var->name) + strlen(var->latchedString);
			if(len+10 > sizeof(buffer)){
				Q_Printf(S_COLOR_YELLOW
					"WARNING: value of variable "
					"\"%s\" too long to write to file\n",
					var->name);
				continue;
			}
			Q_sprintf (buffer, sizeof(buffer),
				"seta %s \"%s\"\n", var->name,
				var->latchedString);
		}else{
			len = strlen(var->name) + strlen(var->string);
			if(len+10 > sizeof(buffer)){
				Q_Printf(S_COLOR_YELLOW
					"WARNING: value of variable "
					"\"%s\" too long to write to file\n",
					var->name);
				continue;
			}
			Q_sprintf (buffer, sizeof(buffer),
				"seta %s \"%s\"\n", var->name,
				var->string);
		}
		FS_Write(buffer, strlen(buffer), f);
	}
}

void
Cvar_List_f(void)
{
	cvar_t	*var;
	int	i;
	char *match;

	if(Cmd_Argc() > 1)
		match = Cmd_Argv(1);
	else
		match = nil;

	for(var = cvar_vars, i = 0; var != nil; var = var->next, i++){
		if((var->name == nil) ||
		   (match && !Q_Filter(match, var->name, qfalse))){
			continue;
		}

		if(var->flags & CVAR_SERVERINFO)
			Q_Printf("S");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_SYSTEMINFO)
			Q_Printf("s");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_USERINFO)
			Q_Printf("U");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_ROM)
			Q_Printf("R");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_INIT)
			Q_Printf("I");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_ARCHIVE)
			Q_Printf("A");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_LATCH)
			Q_Printf("L");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_CHEAT)
			Q_Printf("C");
		else
			Q_Printf(" ");

		if(var->flags & CVAR_USER_CREATED)
			Q_Printf("?");
		else
			Q_Printf(" ");

		Q_Printf(" %s \"%s\"\n", var->name, var->string);
	}
	Q_Printf("\n%i total cvars\n", i);
	Q_Printf("%i cvar indexes\n", cvar_numIndexes);
}

/* Unsets a cvar */
cvar_t *
Cvar_Unset(cvar_t *cv)
{
	cvar_t *next = cv->next;

	if(cv->name)
		Z_Free(cv->name);
	if(cv->string)
		Z_Free(cv->string);
	if(cv->latchedString)
		Z_Free(cv->latchedString);
	if(cv->resetString)
		Z_Free(cv->resetString);

	if(cv->prev)
		cv->prev->next = cv->next;
	else
		cvar_vars = cv->next;

	if(cv->next)
		cv->next->prev = cv->prev;

	if(cv->hashPrev)
		cv->hashPrev->hashNext = cv->hashNext;
	else
		hashTable[cv->hashIndex] = cv->hashNext;

	if(cv->hashNext)
		cv->hashNext->hashPrev = cv->hashPrev;

	Q_Memset(cv, '\0', sizeof(*cv));

	return next;
}

/*
 * Unsets a user-defined cvar
 */
void
Cvar_Unset_f(void)
{
	cvar_t *cv;

	if(Cmd_Argc() != 2){
		Q_Printf("Usage: %s <varname>\n", Cmd_Argv(0));
		return;
	}

	cv = Cvar_FindVar(Cmd_Argv(1));

	if(cv == nil)
		return;

	if(cv->flags & CVAR_USER_CREATED)
		Cvar_Unset(cv);
	else
		Q_Printf("Error: %s: Variable %s is not user created.\n",
			Cmd_Argv(0), cv->name);
}

/*
 * Resets all cvars to their hardcoded values and removes
 * user-defined variables and variables added via the VMs if requested.
 */
void
Cvar_Restart(qbool unsetVM)
{
	cvar_t *var;

	for(var = cvar_vars; var != nil; var = var->next){
		if((var->flags & CVAR_USER_CREATED) ||
		   (unsetVM && (var->flags & CVAR_VM_CREATED))){
			/* throw out any variables the user/vm created */
			var = Cvar_Unset(var);
			continue;
		}

		if(!(var->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)))
			/* Just reset the rest to their default values. */
			Cvar_Set2(var->name, var->resetString, qfalse);
	}
}

/*
 * Resets all cvars to their hardcoded values
 */
void
Cvar_Restart_f(void)
{
	Cvar_Restart(qfalse);
}

/* returns an info string containing all the cvars that have the given bit set
 * in their flags ( CVAR_USERINFO, CVAR_SERVERINFO, CVAR_SYSTEMINFO, etc ) */
char *
Cvar_InfoString(int bit)
{
	static char info[MAX_INFO_STRING];
	cvar_t *var;

	info[0] = 0;

	for(var = cvar_vars; var != nil; var = var->next)
		if(var->name && (var->flags & bit))
			Info_SetValueForKey(info, var->name, var->string);
	return info;
}

/*
 * handles large info strings ( CS_SYSTEMINFO )
 */
char *
Cvar_InfoString_Big(int bit)
{
	static char info[BIG_INFO_STRING];
	cvar_t *var;

	info[0] = 0;

	for(var = cvar_vars; var != nil; var = var->next)
		if((var->name != nil) && (var->flags & bit))
			Info_SetValueForKey_Big(info, var->name, var->string);
	return info;
}

void
Cvar_InfoStringBuffer(int bit, char* buff, int buffsize)
{
	Q_strncpyz(buff, Cvar_InfoString(bit), buffsize);
}

void
Cvar_CheckRange(cvar_t *var, float min, float max, qbool integral)
{
	var->validate	= qtrue;
	var->min	= min;
	var->max	= max;
	var->integral	= integral;
	/* Force an initial range check */
	Cvar_Set(var->name, var->string);
}

/*
 * basically a slightly modified Cvar_Get for the interpreted modules
 */
void
Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue,
	      int flags)
{
	cvar_t *cv;

	/* There is code in Cvar_Get to prevent CVAR_ROM cvars being changed by the
	 * user. In other words CVAR_ARCHIVE and CVAR_ROM are mutually exclusive
	 * flags. Unfortunately some historical game code (including single player
	 * baseq3) sets both flags. We unset CVAR_ROM for such cvars. */
	if((flags & (CVAR_ARCHIVE | CVAR_ROM)) == (CVAR_ARCHIVE | CVAR_ROM)){
		Q_DPrintf(
			S_COLOR_YELLOW "WARNING: Unsetting CVAR_ROM cvar '%s', "
				       "since it is also CVAR_ARCHIVE\n",
			varName);
		flags &= ~CVAR_ROM;
	}

	cv = Cvar_Get(varName, defaultValue, flags | CVAR_VM_CREATED);

	if(vmCvar == nil)
		return;

	vmCvar->handle = cv - cvar_indexes;
	vmCvar->modificationCount = -1;
	Cvar_Update(vmCvar);
}

/* updates an interpreted modules' version of a cvar */
void
Cvar_Update(vmCvar_t *vmCvar)
{
	cvar_t *cv = nil;
	assert(vmCvar != nil);

	if((unsigned)vmCvar->handle >= cvar_numIndexes)
		Q_Error(ERR_DROP, "Cvar_Update: handle out of range");

	cv = cvar_indexes + vmCvar->handle;
	if(cv->modificationCount == vmCvar->modificationCount)
		return;
	if(cv->string == nil)
		return;		/* variable might have been cleared by a cvar_restart */

	vmCvar->modificationCount = cv->modificationCount;
	if(strlen(cv->string)+1 > MAX_CVAR_VALUE_STRING)
		Q_Error(ERR_DROP,
			"Cvar_Update: src %s length %u exceeds MAX_CVAR_VALUE_STRING",
			cv->string,
			(uint)strlen(cv->string));
	Q_strncpyz(vmCvar->string, cv->string,  MAX_CVAR_VALUE_STRING);
	vmCvar->value	= cv->value;
	vmCvar->integer = cv->integer;
}

void
Cvar_CompleteCvarName(char *args, int argNum)
{
	if(argNum == 2){
		/* Skip "<cmd> " */
		char *p;

		p = Q_SkipTokens(args, 1, " ");
		if(p > args)
			Field_CompleteCommand(p, qfalse, qtrue);
	}
}

/* Reads in all archived cvars */
void
Cvar_Init(void)
{
	Q_Memset(cvar_indexes, '\0', sizeof(cvar_indexes));
	Q_Memset(hashTable, '\0', sizeof(hashTable));

	cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);

	Cmd_AddCommand("print", Cvar_Print_f);
	Cmd_AddCommand("toggle", Cvar_Toggle_f);
	Cmd_SetCommandCompletionFunc("toggle", Cvar_CompleteCvarName);
	Cmd_AddCommand("set", Cvar_Set_f);
	Cmd_SetCommandCompletionFunc("set", Cvar_CompleteCvarName);
	Cmd_AddCommand("sets", Cvar_Set_f);
	Cmd_SetCommandCompletionFunc("sets", Cvar_CompleteCvarName);
	Cmd_AddCommand("setu", Cvar_Set_f);
	Cmd_SetCommandCompletionFunc("setu", Cvar_CompleteCvarName);
	Cmd_AddCommand("seta", Cvar_Set_f);
	Cmd_SetCommandCompletionFunc("seta", Cvar_CompleteCvarName);
	Cmd_AddCommand("reset", Cvar_Reset_f);
	Cmd_SetCommandCompletionFunc("reset", Cvar_CompleteCvarName);
	Cmd_AddCommand("unset", Cvar_Unset_f);
	Cmd_SetCommandCompletionFunc("unset", Cvar_CompleteCvarName);

	Cmd_AddCommand("cvarlist", Cvar_List_f);
	Cmd_AddCommand("find", Cvar_List_f);
	Cmd_AddCommand("cvar_reset", Cvar_Restart_f);
}
