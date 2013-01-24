/* Dynamic variable tracking */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/*
 * Cvars are used to hold scalar or string variables that can be changed
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

#include "shared.h"
#include "common.h"

enum {
	/*
	 * FIXME: having a hard limit defeats one advantage of hash tables
	 * straight away
	 */
	MAX_CVARS	= 1024,
	HASH_SIZE	= 512
};

int cvar_modifiedFlags;

static Cvar *cvarlist = nil;
static Cvar *cheatsenabled;
static Cvar indices[MAX_CVARS];
static int numindices;
static Cvar *hashtab[HASH_SIZE];

static qbool
validatestr(const char *s)
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

static Cvar*
look(const char *var_name)
{
	Cvar	*var;
	long	hash;

	hash = Q_hashstr(var_name, HASH_SIZE);

	for(var=hashtab[hash]; var != nil; var=var->hashNext)
		if(!Q_stricmp(var_name, var->name))
			return var;

	return nil;
}

/* returns 0 if not defined or non numeric */
float
cvargetf(const char *var_name)
{
	Cvar *var;

	var = look(var_name);
	if(var == nil)
		return 0;
	return var->value;
}

int
cvargeti(const char *var_name)
{
	Cvar *var;

	var = look(var_name);
	if(var == nil)
		return 0;
	return var->integer;
}

/* returns an empty string if not defined */
char*
cvargetstr(const char *var_name)
{
	Cvar *var;

	var = look(var_name);
	if(var == nil)
		return "";
	return var->string;
}

void
cvargetstrbuf(const char *var_name, char *buffer, int bufsize)
{
	Cvar *var;

	var = look(var_name);
	if(var == nil)
		*buffer = 0;
	else
		Q_strncpyz(buffer, var->string, bufsize);
}

int
cvarflags(const char *var_name)
{
	Cvar *var;

	var = look(var_name);
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
cvarcmdcompletion(void (*callback)(const char *s))
{
	Cvar *cvar;

	for(cvar = cvarlist; cvar; cvar = cvar->next)
		if(cvar->name != nil)
			callback(cvar->name);
}

static const char *
validate(Cvar *var, const char *value, qbool warn)
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
					comprintf("WARNING: cvar '%s' must"
						   " be integral", var->name);

				valuef	= (int)valuef;
				changed = qtrue;
			}
	}else{
		if(warn)
			comprintf("WARNING: cvar '%s' must be numeric",
				var->name);

		valuef	= atof(var->resetString);
		changed = qtrue;
	}

	if(valuef < var->min){
		if(warn){
			if(changed)
				comprintf(" and is");
			else
				comprintf("WARNING: cvar '%s'", var->name);

			if(Q_isintegral(var->min))
				comprintf(" out of range (min %d)",
					(int)var->min);
			else
				comprintf(" out of range (min %f)", var->min);
		}

		valuef	= var->min;
		changed = qtrue;
	}else if(valuef > var->max){
		if(warn){
			if(changed)
				comprintf(" and is");
			else
				comprintf("WARNING: cvar '%s'", var->name);

			if(Q_isintegral(var->max))
				comprintf(" out of range (max %d)",
					(int)var->max);
			else
				comprintf(" out of range (max %f)", var->max);
		}

		valuef	= var->max;
		changed = qtrue;
	}

	if(changed){
		if(Q_isintegral(valuef)){
			Q_sprintf(s, sizeof(s), "%d", (int)valuef);

			if(warn)
				comprintf(", setting to %d\n", (int)valuef);
		}else{
			Q_sprintf(s, sizeof(s), "%f", valuef);

			if(warn)
				comprintf(", setting to %f\n", valuef);
		}

		return s;
	}else
		return value;
}

/* set value of an existing cvar */
static void
setnewval(Cvar *var, const char *var_name, const char *var_value, int flags)
{
	var_value = validate(var, var_value, qfalse);
	/*
	 * if the C code is now specifying a variable that the user already
	 * set a value for, take the new value as the reset value
	 */
	if(var->flags & CVAR_USER_CREATED){
		var->flags &= ~CVAR_USER_CREATED;
		zfree(var->resetString);
		var->resetString = copystr(var_value);

		if(flags & CVAR_ROM){
			/*
			 * this variable was set by the user,
			 * so force it to value given by the engine.
			 */
			if(var->latchedString != '\0')
				zfree(var->latchedString);
			var->latchedString = copystr(var_value);
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
		zfree(var->resetString);
		var->resetString = copystr(var_value);
	}else if(var_value[0] && strcmp(var->resetString, var_value))
		comdprintf("Warning: cvar \"%s\" given initial values: \"%s\""
			    " and \"%s\"\n", var_name, var->resetString, var_value);
	/* if we have a latched string, take that value now */
	if(var->latchedString != nil){
		char *s;

		s = var->latchedString;
		var->latchedString = nil;	/* otherwise cvar_set2 would free it */
		cvarsetstr2(var_name, s, qtrue);
		zfree(s);
	}
	/* ZOID--needs to be set so that cvars the game sets as
	 * SERVERINFO get sent to clients */
	cvar_modifiedFlags |= flags;
}

/*
 * If the variable already exists, the value will not be set unless
 * CVAR_ROM. The flags will be or'ed in if the variable exists.
 */
Cvar*
cvarget(const char *var_name, const char *var_value, int flags)
{
	Cvar *var;
	long hash;
	int index;

	if(var_name == nil || var_value == nil)
		comerrorf(ERR_FATAL, "cvarget: nil parameter");

	if(!validatestr(var_name)){
		comprintf("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

#if 0	/* FIXME: values with backslash happen */
	if(!validatestr(var_value)){
		comprintf("invalid cvar value string: %s\n", var_value);
		var_value = "BADVALUE";
	}
#endif

	var = look(var_name);
	if(var != nil){
		setnewval(var, var_name, var_value, flags);
		return var;
	}

	/* allocate a new cvar... */
	/* find a free cvar */
	for(index = 0; index < MAX_CVARS; index++)
		if(indices[index].name == nil)
			break;

	if(index >= MAX_CVARS){
		if(!com_errorEntered)
			comerrorf(
				ERR_FATAL,
				"Error: Too many cvars, cannot create a new one!");
		return nil;
	}

	var = &indices[index];

	if(index >= numindices)
		numindices = index + 1;

	var->name = copystr(var_name);
	var->string = copystr(var_value);
	var->modified = qtrue;
	var->modificationCount = 1;
	var->value = atof(var->string);
	var->integer = atoi(var->string);
	var->resetString = copystr(var_value);
	var->validate = qfalse;

	/* link the variable in */
	var->next = cvarlist;
	if(cvarlist != nil)
		cvarlist->prev = var;

	var->prev = nil;
	cvarlist = var;

	var->flags = flags;
	/* note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo) */
	cvar_modifiedFlags |= var->flags;

	hash = Q_hashstr(var_name, HASH_SIZE);
	var->hashIndex = hash;

	var->hashNext = hashtab[hash];
	if(hashtab[hash])
		hashtab[hash]->hashPrev = var;

	var->hashPrev = nil;
	hashtab[hash] = var;

	return var;
}

/*
 * Prints the description, value, default, and latched string of
 * the given variable
 */
static void
printcvar(Cvar *v)
{
	comprintf("%s: ", v->name);
	if(v->desc != nil)
		comprintf("%s\n", v->desc);
	else
		comprintf("no description\n");

	comprintf("%s is \"%s" S_COLOR_WHITE "\"",
		v->name, v->string);
	if(!(v->flags & CVAR_ROM)){
		if(Q_stricmp(v->string, v->resetString) == 0)
			comprintf(", the default");
		else
			comprintf(", default: \"%s" S_COLOR_WHITE "\"",
				v->resetString);
	}
	comprintf("\n");
	if(v->latchedString != nil)
		comprintf("latched: \"%s\"\n", v->latchedString);
}

/* sets the description string of the named cvar, if it exists */
void
cvarsetdesc(const char *name, const char *desc)
{
	Cvar *cv;

	cv = look(name);
	if((cv == nil) || (desc == nil))
		return;
	if(cv->desc != nil)
		zfree(cv->desc);
	cv->desc = copystr(desc);
}

/* same as cvarsetstr, but allows more control over setting of cvar */
Cvar*
cvarsetstr2(const char *var_name, const char *value, qbool force)
{
	Cvar *var;

	/* comdprintf( "cvarsetstr2: %s %s\n", var_name, value ); */

	if(!validatestr(var_name)){
		comprintf("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

#if 0	/* FIXME */
	if(value && !validatestr(value)){
		comprintf("invalid cvar value string: %s\n", value);
		var_value = "BADVALUE";
	}
#endif

	var = look(var_name);
	if(var == nil){
		if(value == nil)
			return nil;
		/* create it */
		if(!force)
			return cvarget(var_name, value, CVAR_USER_CREATED);
		else
			return cvarget(var_name, value, 0);
	}

	if(value == nil)
		value = var->resetString;

	value = validate(var, value, qtrue);

	if((var->flags & CVAR_LATCH) && (var->latchedString != nil)){
		if(!strcmp(value, var->string)){
			zfree(var->latchedString);
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
			comprintf("%s is read only.\n", var_name);
			return var;
		}

		if(var->flags & CVAR_INIT){
			comprintf("%s is write protected.\n", var_name);
			return var;
		}

		if(var->flags & CVAR_LATCH){
			if(var->latchedString){
				if(strcmp(value, var->latchedString) == 0)
					return var;
				zfree(var->latchedString);
			}else if(strcmp(value, var->string) == 0)
				return var;

			comprintf("%s will be changed upon restarting.\n",
				var_name);
			var->latchedString = copystr(value);
			var->modified = qtrue;
			var->modificationCount++;
			return var;
		}

		if((var->flags & CVAR_CHEAT) && (cheatsenabled->integer <= 0)){
			comprintf ("%s is cheat protected.\n", var_name);
			return var;
		}

	}else if(var->latchedString != nil){
		zfree(var->latchedString);
		var->latchedString = nil;
	}

	if(!strcmp(value, var->string))
		return var;	/* not changed */

	var->modified = qtrue;
	var->modificationCount++;

	zfree(var->string);	/* free the old value string */

	var->string	= copystr(value);
	var->value	= atof (var->string);
	var->integer	= atoi (var->string);

	return var;
}

/* will create the variable with no flags if it doesn't exist */
void
cvarsetstr(const char *var_name, const char *value)
{
	cvarsetstr2(var_name, value, qtrue);
}

/* sometimes we set variables from an untrusted source: fail if flags & CVAR_PROTECTED */
void
cvarsetstrsafe(const char *var_name, const char *value)
{
	int flags = cvarflags(var_name);

	if((flags != CVAR_NONEXISTENT) && (flags & CVAR_PROTECTED)){
		if(value != nil)
			comerrorf(ERR_DROP, "Restricted source tried to set "
					    "\"%s\" to \"%s\"", var_name, value);
		else
			comerrorf(ERR_DROP, "Restricted source tried to "
					    "modify \"%s\"", var_name);
		return;
	}
	cvarsetstr(var_name, value);
}

/* don't set the cvar immediately */
void
cvarsetstrlatched(const char *var_name, const char *value)
{
	cvarsetstr2(var_name, value, qfalse);
}

void
cvarsetf(const char *var_name, float value)
{
	char val[32];

	if(value == (int)value)
		Q_sprintf(val, sizeof(val), "%i", (int)value);
	else
		Q_sprintf(val, sizeof(val), "%f", value);
	cvarsetstr(var_name, val);
}

/* expands value to a string and calls cvarsetstr/cvarsetstrsafe */
void
cvarsetfsafe(const char *var_name, float value)
{
	char val[32];

	if(Q_isintegral(value))
		Q_sprintf(val, sizeof(val), "%i", (int)value);
	else
		Q_sprintf(val, sizeof(val), "%f", value);
	cvarsetstrsafe(var_name, val);
}

void
cvarreset(const char *var_name)
{
	cvarsetstr2(var_name, nil, qfalse);
}

void
cvarforcereset(const char *var_name)
{
	cvarsetstr2(var_name, nil, qtrue);
}

/* Any testing variables will be reset to the safe values */
void
cvarsetcheatstate(void)
{
	Cvar *var;

	/* set all default vars to the safe value */
	for(var = cvarlist; var != nil; var = var->next)
		if(var->flags & CVAR_CHEAT){
			/*
			 * the CVAR_LATCHED|CVAR_CHEAT vars might escape the
			 * reset here because of a
			 * different var->latchedString
			 * */
			if(var->latchedString != nil){
				zfree(var->latchedString);
				var->latchedString = nil;
			}
			if(strcmp(var->resetString, var->string))
				cvarsetstr(var->name, var->resetString);
		}
}

/*
 * Handles variable inspection and changing from the console
 *
 * called by cmdexecstr when cmdargv(0) doesn't match a known
 * command. Returns true if the command was a variable reference that
 * was handled. (print or change)
 */
qbool
cvariscmd(void)
{
	Cvar *v;

	/* check variables */
	v = look(cmdargv(0));
	if(v == nil)
		return qfalse;

	/* perform a variable print or set */
	if(cmdargc() == 1){
		printcvar(v);
		return qtrue;
	}

	/* set the value if forcing isn't required */
	cvarsetstr2(v->name, cmdargs(), qfalse);
	return qtrue;
}

/*
 * printcvar_f: Prints the contents of a cvar
 * (preferred over cvariscmd where cvar names and commands conflict)
 */
static void
printcvar_f(void)
{
	char *name;
	Cvar *cv;

	if(cmdargc() != 2){
		comprintf ("usage: print <variable>\n");
		return;
	}

	name = cmdargv(1);

	cv = look(name);

	if(cv != nil)
		printcvar(cv);
	else
		comprintf("Cvar %s does not exist.\n", name);
}

/*
 * Toggles a cvar for easy single key binding, optionally
 * through a list of given values.
 */
static void
toggle_f(void)
{
	int i, c = cmdargc();
	char *curval;

	if(c < 2){
		comprintf("usage: toggle <variable> [value1, value2, ...]\n");
		return;
	}

	if(c == 2){
		cvarsetstr2(cmdargv(1), va("%d",
				!cvargetf(cmdargv(1))),
			qfalse);
		return;
	}

	if(c == 3){
		comprintf("toggle: nothing to toggle to\n");
		return;
	}

	curval = cvargetstr(cmdargv(1));

	/* don't bother checking the last arg for a match since the desired
	 * behaviour is the same as no match (set to the first argument) */
	for(i = 2; i+1 < c; i++)
		if(strcmp(curval, cmdargv(i)) == 0){
			cvarsetstr2(cmdargv(1), cmdargv(i + 1), qfalse);
			return;
		}
	/* fallback */
	cvarsetstr2(cmdargv(1), cmdargv(2), qfalse);
}

/*
 * Allows setting and defining of arbitrary cvars from console,
 * even if they weren't declared in C code.
 */
static void
set_f(void)
{
	int c;
	char *cmd;
	Cvar *v;

	c = cmdargc();
	cmd = cmdargv(0);

	if(c < 2){
		comprintf("usage: %s <variable> <value>\n", cmd);
		return;
	}
	if(c == 2){
		printcvar_f();
		return;
	}

	v = cvarsetstr2(cmdargv(1), cmdargsfrom(2), qfalse);
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

static void
reset_f(void)
{
	if(cmdargc() != 2){
		comprintf("usage: reset <variable>\n");
		return;
	}
	cvarreset(cmdargv(1));
}

/*
 * Appends lines containing "set variable value" for
 * all variables with the archive flag set.
 */
void
cvarwritevars(Fhandle f)
{
	size_t len;	/* total length of name + archival string */
	char buffer[1024];
	Cvar *var;

	for(var = cvarlist; var != nil; var = var->next){
		if(var->name == nil)
			continue;

		if(!(var->flags & CVAR_ARCHIVE)){
			continue;
		}

		/* write the latched value, even if it hasn't taken effect yet */
		if(var->latchedString != nil){
			len = strlen(var->name) + strlen(var->latchedString);
			if(len+10 > sizeof(buffer)){
				comprintf(S_COLOR_YELLOW
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
				comprintf(S_COLOR_YELLOW
					"WARNING: value of variable "
					"\"%s\" too long to write to file\n",
					var->name);
				continue;
			}
			Q_sprintf (buffer, sizeof(buffer),
				"seta %s \"%s\"\n", var->name,
				var->string);
		}
		fswrite(buffer, strlen(buffer), f);
	}
}

static void
list_f(void)
{
	Cvar *var;
	int i;
	char *match;

	if(cmdargc() > 1)
		match = cmdargv(1);
	else
		match = nil;

	for(var = cvarlist, i = 0; var != nil; var = var->next, i++){
		if((var->name == nil) ||
		   (match && !filterstr(match, var->name, qfalse))){
			continue;
		}

		if(var->flags & CVAR_SERVERINFO)
			comprintf("S");
		else
			comprintf(" ");

		if(var->flags & CVAR_SYSTEMINFO)
			comprintf("s");
		else
			comprintf(" ");

		if(var->flags & CVAR_USERINFO)
			comprintf("U");
		else
			comprintf(" ");

		if(var->flags & CVAR_ROM)
			comprintf("R");
		else
			comprintf(" ");

		if(var->flags & CVAR_INIT)
			comprintf("I");
		else
			comprintf(" ");

		if(var->flags & CVAR_ARCHIVE)
			comprintf("A");
		else
			comprintf(" ");

		if(var->flags & CVAR_LATCH)
			comprintf("L");
		else
			comprintf(" ");

		if(var->flags & CVAR_CHEAT)
			comprintf("C");
		else
			comprintf(" ");

		if(var->flags & CVAR_USER_CREATED)
			comprintf("?");
		else
			comprintf(" ");

		comprintf(" %s \"%s\"\n", var->name, var->string);
	}
	comprintf("\n%i total cvars\n", i);
	comprintf("%i cvar indexes\n", numindices);
}

/* Unsets a cvar */
static Cvar*
unset(Cvar *cv)
{
	Cvar *next = cv->next;

	if(cv->name)
		zfree(cv->name);
	if(cv->string)
		zfree(cv->string);
	if(cv->latchedString)
		zfree(cv->latchedString);
	if(cv->resetString)
		zfree(cv->resetString);

	if(cv->prev)
		cv->prev->next = cv->next;
	else
		cvarlist = cv->next;

	if(cv->next)
		cv->next->prev = cv->prev;

	if(cv->hashPrev)
		cv->hashPrev->hashNext = cv->hashNext;
	else
		hashtab[cv->hashIndex] = cv->hashNext;

	if(cv->hashNext)
		cv->hashNext->hashPrev = cv->hashPrev;

	Q_Memset(cv, '\0', sizeof(*cv));

	return next;
}

/*
 * Unsets a user-defined cvar
 */
static void
unset_f(void)
{
	Cvar *cv;

	if(cmdargc() != 2){
		comprintf("Usage: %s <varname>\n", cmdargv(0));
		return;
	}

	cv = look(cmdargv(1));

	if(cv == nil)
		return;

	if(cv->flags & CVAR_USER_CREATED)
		unset(cv);
	else
		comprintf("Error: %s: Variable %s is not user created.\n",
			cmdargv(0), cv->name);
}

/*
 * Resets all cvars to their hardcoded values and removes
 * user-defined variables and variables added via the VMs if requested.
 */
void
cvarrestart(qbool unsetVM)
{
	Cvar *var;

	for(var = cvarlist; var != nil; var = var->next){
		if((var->flags & CVAR_USER_CREATED) ||
		   (unsetVM && (var->flags & CVAR_VM_CREATED))){
			/* throw out any variables the user/vm created */
			var = unset(var);
			continue;
		}

		if(!(var->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)))
			/* Just reset the rest to their default values. */
			cvarsetstr2(var->name, var->resetString, qfalse);
	}
}

/*
 * Resets all cvars to their hardcoded values
 */
void
cvarrestart_f(void)
{
	cvarrestart(qfalse);
}

/* returns an info string containing all the cvars that have the given bit set
 * in their flags ( CVAR_USERINFO, CVAR_SERVERINFO, CVAR_SYSTEMINFO, etc ) */
char *
cvargetinfostr(int bit)
{
	static char info[MAX_INFO_STRING];
	Cvar *var;

	info[0] = 0;

	for(var = cvarlist; var != nil; var = var->next)
		if(var->name && (var->flags & bit))
			Info_SetValueForKey(info, var->name, var->string);
	return info;
}

/*
 * handles large info strings ( CS_SYSTEMINFO )
 */
char *
cvargetbiginfostr(int bit)
{
	static char info[BIG_INFO_STRING];
	Cvar *var;

	info[0] = 0;

	for(var = cvarlist; var != nil; var = var->next)
		if((var->name != nil) && (var->flags & bit))
			Info_SetValueForKey_Big(info, var->name, var->string);
	return info;
}

void
cvargetinfostrbuf(int bit, char* buff, int buffsize)
{
	Q_strncpyz(buff, cvargetinfostr(bit), buffsize);
}

void
cvarcheckrange(Cvar *var, float min, float max, qbool integral)
{
	var->validate = qtrue;
	var->min = min;
	var->max = max;
	var->integral = integral;
	/* Force an initial range check */
	cvarsetstr(var->name, var->string);
}

/*
 * basically a slightly modified cvarget for the interpreted modules
 */
void
cvarregister(Vmcvar *vmCvar, const char *varName, const char *defaultValue,
	      int flags)
{
	Cvar *cv;

	/* 
	 * There is code in cvarget to prevent CVAR_ROM cvars being changed by the
	 * user. In other words CVAR_ARCHIVE and CVAR_ROM are mutually exclusive
	 * flags.  Unfortunately some historical game code (including single player
	 * baseq3) sets both flags. We unset CVAR_ROM for such cvars. 
	 */
	if((flags & (CVAR_ARCHIVE | CVAR_ROM)) == (CVAR_ARCHIVE | CVAR_ROM)){
		comdprintf(
			S_COLOR_YELLOW "WARNING: Unsetting CVAR_ROM cvar '%s', "
				       "since it is also CVAR_ARCHIVE\n",
			varName);
		flags &= ~CVAR_ROM;
	}

	cv = cvarget(varName, defaultValue, flags | CVAR_VM_CREATED);

	if(vmCvar == nil)
		return;

	vmCvar->handle = cv - indices;
	vmCvar->modificationCount = -1;
	cvarupdate(vmCvar);
}

/* updates an interpreted modules' version of a cvar */
void
cvarupdate(Vmcvar *vmCvar)
{
	Cvar *cv = nil;

	assert(vmCvar != nil);
	if((unsigned)vmCvar->handle >= numindices)
		comerrorf(ERR_DROP, "cvarupdate: handle out of range");

	cv = indices + vmCvar->handle;
	if(cv->modificationCount == vmCvar->modificationCount)
		return;
	if(cv->string == nil)
		return;		/* variable might have been cleared by a cvar_restart */

	vmCvar->modificationCount = cv->modificationCount;
	if(strlen(cv->string)+1 > MAX_CVAR_VALUE_STRING)
		comerrorf(ERR_DROP,
			"cvarupdate: src %s length %u exceeds MAX_CVAR_VALUE_STRING",
			cv->string,
			(uint)strlen(cv->string));
	Q_strncpyz(vmCvar->string, cv->string,  MAX_CVAR_VALUE_STRING);
	vmCvar->value = cv->value;
	vmCvar->integer = cv->integer;
}

void
cvarcompletename(char *args, int argNum)
{
	if(argNum == 2){
		/* Skip "<cmd> " */
		char *p;

		p = Q_skiptoks(args, 1, " ");
		if(p > args)
			fieldcompletecmd(p, qfalse, qtrue);
	}
}

/* Reads in all archived cvars */
void
cvarinit(void)
{
	Q_Memset(indices, '\0', sizeof(indices));
	Q_Memset(hashtab, '\0', sizeof(hashtab));

	cheatsenabled = cvarget("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);

	cmdadd("print", printcvar_f);
	cmdadd("toggle", toggle_f);
	cmdsetcompletion("toggle", cvarcompletename);
	cmdadd("set", set_f);
	cmdsetcompletion("set", cvarcompletename);
	cmdadd("sets", set_f);
	cmdsetcompletion("sets", cvarcompletename);
	cmdadd("setu", set_f);
	cmdsetcompletion("setu", cvarcompletename);
	cmdadd("seta", set_f);
	cmdsetcompletion("seta", cvarcompletename);
	cmdadd("reset", reset_f);
	cmdsetcompletion("reset", cvarcompletename);
	cmdadd("unset", unset_f);
	cmdsetcompletion("unset", cvarcompletename);
	cmdadd("cvarlist", list_f);
	cmdadd("find", list_f);
	cmdadd("cvar_reset", cvarrestart_f);
}
