/* Quake script command processing module */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"

enum {
	Maxcmdbuf	= 16384,
	Maxcmdline	= 1024
};

typedef struct Cmd		Cmd;
typedef struct Cmdfunc	Cmdfunc;

struct Cmd {
	byte	*data;
	int	maxsize;
	int	cursize;
};

struct Cmdfunc {
	Cmdfunc			*next;
	char				*name;
	xcommand_t		function;
	Completionfunc	complete;
};

int cmd_wait;
Cmd cmd_text;
byte cmd_text_buf[Maxcmdbuf];

/*
 * Causes execution of the remainder of the command buffer to be delayed until
 * next frame.  This allows commands like:
 * bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
 */
void
Cmd_Wait_f(void)
{
	if(cmdargc() == 2){
		cmd_wait = atoi(cmdargv(1));
		if(cmd_wait < 0)
			cmd_wait = 1;	/* ignore the argument */
	}else
		cmd_wait = 1;
}

/*
 * Command buffer
 */

/* allocates an initial text buffer that will grow as needed */
void
cbufinit(void)
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = Maxcmdbuf;
	cmd_text.cursize = 0;
}

/* Adds command text at the end of the buffer, does NOT add a final \n */
void
cbufaddstr(const char *text)
{
	int l;

	l = strlen(text);

	if(cmd_text.cursize + l >= cmd_text.maxsize){
		Com_Printf("cbufaddstr: overflow\n");
		return;
	}
	Q_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;
}

/*
 * Adds command text immediately after the current command
 * Adds a \n to the text
 */
void
cbufinsertstr(const char *text)
{
	int	len;
	int	i;

	len = strlen(text) + 1;
	if(len + cmd_text.cursize > cmd_text.maxsize){
		Com_Printf("cbufinsertstr overflowed\n");
		return;
	}

	/* move the existing command text */
	for(i = cmd_text.cursize - 1; i >= 0; i--)
		cmd_text.data[i + len] = cmd_text.data[i];

	/* copy the new text in */
	Q_Memcpy(cmd_text.data, text, len - 1);

	/* add a \n */
	cmd_text.data[len - 1] = '\n';

	cmd_text.cursize += len;
}

/* this can be used in place of either cbufaddstr or cbufinsertstr */
void
cbufexecstr(int exec_when, const char *text)
{
	switch(exec_when){
	case EXEC_NOW:
		if(text && strlen(text) > 0){
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", text);
			cmdexecstr(text);
		}else{
			cbufflush();
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n",
				cmd_text.data);
		}
		break;
	case EXEC_INSERT:
		cbufinsertstr(text);
		break;
	case EXEC_APPEND:
		cbufaddstr(text);
		break;
	default:
		Com_Errorf(ERR_FATAL, "cbufexecstr: bad exec_when");
	}
}

/* 
 * Pulls off \n-terminated lines of text from the command buffer and sends
 * them through cmdexecstr.  Stops when the buffer is empty.
 * Normally called once per frame, but may be explicitly invoked.
 * Do not call inside a command function, or current args will be destroyed. 
 */
void
cbufflush(void)
{
	int i, quotes;
	char *text;
	char line[Maxcmdline];
	/* 
	 * This will keep / / style comments all on one line by not breaking on
	 * a semicolon.  It will keep / * ... * / style comments all on one line by not
	 * breaking it for semicolon or newline. 
	 */
	qbool in_star_comment = qfalse;
	qbool in_slash_comment = qfalse;

	while(cmd_text.cursize){
		if(cmd_wait > 0){
			/* 
			 * skip out while text still remains in buffer, leaving it
			 * for next frame 
			 */
			cmd_wait--;
			break;
		}

		/* find a \n or ; line break or comment: // or / * * / */
		text = (char*)cmd_text.data;

		quotes = 0;
		for(i=0; i< cmd_text.cursize; i++){
			if(text[i] == '"')
				quotes++;

			if(!(quotes&1)){
				if(i < cmd_text.cursize - 1){
					if(!in_star_comment && text[i] == '/' 
					   && text[i+1] == '/')
					then{
						in_slash_comment = qtrue;
					}else if(!in_slash_comment && text[i] == '/' 
					   && text[i+1] == '*')
					then{
						in_star_comment = qtrue;
					}else if(in_star_comment && text[i] == '*' 
					   && text[i+1] == '/')
					then{
						in_star_comment = qfalse;
						/* 
						 * If we are in a star comment, then the part after it is valid
						 * Note: This will cause it to NUL out the terminating '/'
						 * but ExecuteString doesn't require it anyway. 
						 */
						i++;
						break;
					}
				}
				if(!in_slash_comment && !in_star_comment && text[i] == ';')
					break;
			}
			if(!in_star_comment && (text[i] == '\n' || text[i] == '\r')){
				in_slash_comment = qfalse;
				break;
			}
		}

		if(i >= (Maxcmdline - 1))
			i = Maxcmdline - 1;
		Q_Memcpy(line, text, i);
		line[i] = 0;
		
		/* 
		 * Delete the text from the command buffer and move
		 * remaining commands down.  This is necessary because
		 * commands (exec) can insert data at the beginning of
		 * the text buffer.
		 */
		if(i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else{
			i++;
			cmd_text.cursize -= i;
			memmove(text, text+i, cmd_text.cursize);
		}
		/* execute the command line */
		cmdexecstr(line);
	}
}

/*
 * Script commands
 */
 
void
Cmd_Exec_f(void)
{
	union {
		char	*c;
		void	*v;
	} f;
	char filename[MAX_QPATH];

	if(cmdargc() != 2){
		Com_Printf("exec <filename> : execute a script file\n");
		return;
	}

	Q_strncpyz(filename, cmdargv(1), sizeof(filename));
	Q_defaultext(filename, sizeof(filename), ".cfg");
	fsreadfile(filename, &f.v);
	if(!f.c){
		Com_Printf("couldn't exec %s\n",cmdargv(1));
		return;
	}
	Com_Printf("execing %s\n",cmdargv(1));
	cbufinsertstr(f.c);

	fsfreefile(f.v);
}

/*
 * Inserts the current value of a variable as command text
 */
void
Cmd_Vstr_f(void)
{
	char *v;

	if(cmdargc() != 2){
		Com_Printf("vstr <variablename> : execute a variable command\n");
		return;
	}

	v = cvargetstr(cmdargv(1));
	cbufinsertstr(va("%s\n", v));
}

/*
 * Just prints the rest of the line to the console
 */
void
Cmd_Echo_f(void)
{
	Com_Printf("%s\n", cmdargs());
}

/*
 * Command execution
 */

static int cmd_argc;
static char *cmd_argv[MAX_STRING_TOKENS];				/* points into cmd_tokenized */
static char cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	/* will have 0 bytes inserted */
static char cmd_cmd[BIG_INFO_STRING];				/* the original command we received (no token processing) */
static Cmdfunc *cmd_functions;	/* possible commands to execute */

int
cmdargc(void)
{
	return cmd_argc;
}

char*
cmdargv(int arg)
{
	if(arg >= cmd_argc)
		return "";
	return cmd_argv[arg];
}

/*
 * The interpreted versions use this because
 * they can't have pointers returned to them
 */
void
cmdargvbuf(int arg, char *buffer, int bufferLength)
{
	Q_strncpyz(buffer, cmdargv(arg), bufferLength);
}

/*
 * Returns a single string containing argv(1) to argv(argc()-1)
 */
char*
cmdargs(void)
{
	static char cmd_args[MAX_STRING_CHARS];
	int i;

	cmd_args[0] = 0;
	for(i = 1; i < cmd_argc; i++){
		strcat(cmd_args, cmd_argv[i]);
		if(i != cmd_argc-1)
			strcat(cmd_args, " ");
	}

	return cmd_args;
}

/*
 * Returns a single string containing argv(arg) to argv(argc()-1)
 */
char *
cmdargsfrom(int arg)
{
	static char cmd_args[BIG_INFO_STRING];
	int i;

	cmd_args[0] = 0;
	if(arg < 0)
		arg = 0;
	for(i = arg; i < cmd_argc; i++){
		strcat(cmd_args, cmd_argv[i]);
		if(i != cmd_argc-1)
			strcat(cmd_args, " ");
	}

	return cmd_args;
}

/*
 * The interpreted versions use this because
 * they can't have pointers returned to them
 */
void
cmdargsbuf(char *buffer, int bufferLength)
{
	Q_strncpyz(buffer, cmdargs(), bufferLength);
}

/*
 * Retrieve the unmodified command string
 * For rcon use when you want to transmit without altering quoting
 */
char *
cmdcmd(void)
{
	return cmd_cmd;
}

/*
 * The functions that execute commands get their parameters with these
 * functions. cmdargv() will return an empty string, not a NULL
 * if arg > argc, so string operations are allways safe. 
 */

/*
 * Replace command separators with space to prevent interpretation
 * This is a hack to protect buggy qvms
 */
void
cmdsanitizeargs(void)
{
	int i;

	for(i = 1; i < cmd_argc; i++){
		char *c = cmd_argv[i];

		if(strlen(c) > MAX_CVAR_VALUE_STRING - 1)
			c[MAX_CVAR_VALUE_STRING - 1] = '\0';

		while((c = strpbrk(c, "\n\r;"))){
			*c = ' ';
			++c;
		}
	}
}

/*
 * Parses the given string into command line tokens.
 * The text is copied to a seperate buffer and 0 characters
 * are inserted in the apropriate place, The argv array
 * will point into this temporary buffer.
 */
static void
cmdstrtok2(const char *text_in, qbool ignoreQuotes)
{
	const char *text;
	char *textOut;

#ifdef TKN_DBG
	/* FIXME TTimo blunt hook to try to find the tokenization of userinfo */
	Com_DPrintf("cmdstrtok: %s\n", text_in);
#endif
	cmd_argc = 0;	/* clear prev args */

	if(!text_in)
		return;

	Q_strncpyz(cmd_cmd, text_in, sizeof(cmd_cmd));

	text = text_in;
	textOut = cmd_tokenized;

	while(1){
		if(cmd_argc >= MAX_STRING_TOKENS)
			return;	/* this is usually something malicious */

		while(1){
			/* skip whitespace */
			while(*text && *text <= ' ')
				text++;
			if(!*text)
				return;	/* all tokens parsed */

			/* skip / / comments */
			if(text[0] == '/' && text[1] == '/')
				return;	/* all tokens parsed */

			/* skip / * * / comments */
			if(text[0] == '/' && text[1] =='*'){
				while(*text && (text[0] != '*' || text[1] != '/'))
					text++;
				if(!*text)
					return;	/* all tokens parsed */
				text += 2;
			}else
				break;	/* we are ready to parse a token */
		}

		/* handle quoted strings; this doesn't handle \" escaping */
		if(!ignoreQuotes && *text == '"'){
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while(*text && *text != '"')
				*textOut++ = *text++;
			*textOut++ = 0;
			if(!*text)
				return;	/* all tokens parsed */
			text++;
			continue;
		}

		/* regular token */
		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		/* skip until whitespace, quote, or command */
		while(*text > ' '){
			if(!ignoreQuotes && text[0] == '"')
				break;
			if(text[0] == '/' && text[1] == '/')
				break;
			if(text[0] == '/' && text[1] =='*')
				break;	/* skip / * * / comments */
			*textOut++ = *text++;
		}
		*textOut++ = 0;
		if(!*text)
			return;	/* all tokens parsed */
	}

}

void
cmdstrtok(const char *text_in)
{
	cmdstrtok2(text_in, qfalse);
}

void
cmdstrtokignorequotes(const char *text_in)
{
	cmdstrtok2(text_in, qtrue);
}

Cmdfunc*
Cmd_FindCommand(const char *cmd_name)
{
	Cmdfunc *cmd;
	for(cmd = cmd_functions; cmd; cmd = cmd->next)
		if(!Q_stricmp(cmd_name, cmd->name))
			return cmd;
	return NULL;
}

/* 
 * called by the init functions of other parts of the program to
 * register commands and functions to call for them.
 * The cmd_name is referenced later, so it should not be in temp memory
 * if function is NULL, the command will be forwarded to the server
 * as a clc_clientCommand instead of executed locally 
 */
void
cmdadd(const char *cmd_name, xcommand_t function)
{
	Cmdfunc *cmd;

	/* fail if the command already exists */
	if(Cmd_FindCommand(cmd_name)){
		/* allow completion-only commands to be silently doubled */
		if(function != NULL)
			Com_Printf("cmdadd: %s already defined\n", cmd_name);
		return;
	}
	/* use a small malloc to avoid zone fragmentation */
	cmd = salloc(sizeof(*cmd));
	cmd->name = Copystr(cmd_name);
	cmd->function = function;
	cmd->complete = NULL;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

/* callback with each valid string */
void
cmdsetcompletion(const char *command, Completionfunc complete)
{
	Cmdfunc *cmd;

	for(cmd = cmd_functions; cmd; cmd = cmd->next)
		if(!Q_stricmp(command, cmd->name))
			cmd->complete = complete;
}

void
cmdremove(const char *cmd_name)
{
	Cmdfunc *cmd, **back;

	back = &cmd_functions;
	for(;;){
		cmd = *back;
		if(!cmd)
			return;	/* command wasn't active */
		if(!strcmp(cmd_name, cmd->name)){
			*back = cmd->next;
			if(cmd->name)
				zfree(cmd->name);
			zfree(cmd);
			return;
		}
		back = &cmd->next;
	}
}

/* Only remove commands with no associated function */
void
cmdsaferemove(const char *cmd_name)
{
	Cmdfunc *cmd;

	if(!(cmd = Cmd_FindCommand(cmd_name)))
		return;
	if(cmd->function){
		Com_Errorf(ERR_DROP, "Restricted source tried to remove "
				    "system command \"%s\"", cmd_name);
		return;
	}
	cmdremove(cmd_name);
}

void
cmdcompletion(void (*callback)(const char *s))
{
	Cmdfunc *cmd;

	for(cmd=cmd_functions; cmd; cmd=cmd->next)
		callback(cmd->name);
}

void
cmdcompletearg(const char *command, char *args, int argNum)
{
	Cmdfunc *cmd;

	for(cmd = cmd_functions; cmd; cmd = cmd->next)
		if(!Q_stricmp(command, cmd->name) && cmd->complete)
			cmd->complete(args, argNum);
}

/* Parses a single line of text into arguments and tries to execute it
 * as if it was typed at the console */
/* A complete command line has been parsed, so try to execute it */
void
cmdexecstr(const char *text)
{
	Cmdfunc *cmd, **prev;

	/* execute the command line */
	cmdstrtok(text);
	if(!cmdargc())
		return;	/* no tokens */

	/* check registered command functions */
	for(prev = &cmd_functions; *prev; prev = &cmd->next){
		cmd = *prev;
		if(!Q_stricmp(cmd_argv[0],cmd->name)){
			/* 
			 * rearrange the links so that the command will be
			 * near the head of the list next time it is used 
			 */
			*prev = cmd->next;
			cmd->next = cmd_functions;
			cmd_functions = cmd;

			/* perform the action */
			if(!cmd->function)
				/* let the cgame or game handle it */
				break;
			else
				cmd->function();
			return;
		}
	}
	/* check cvars */
	if(cvariscmd())
		return;
	/* check client game commands */
	if(com_cl_running && com_cl_running->integer && CL_GameCommand())
		return;
	/* check server game commands */
	if(com_sv_running && com_sv_running->integer && SV_GameCommand())
		return;
	/* check ui commands */
	if(com_cl_running && com_cl_running->integer && UI_GameCommand())
		return;
	/* 
	 * send it as a server command if we are connected
	 * this will usually result in a chat message 
	 */
	CL_ForwardCommandToServer(text);
}

void
Cmd_List_f(void)
{
	Cmdfunc *cmd;
	char *match;
	int i;

	if(cmdargc() > 1)
		match = cmdargv(1);
	else
		match = NULL;

	i = 0;
	for(cmd=cmd_functions; cmd; cmd=cmd->next){
		if(match && !Q_Filter(match, cmd->name, qfalse)) 
			continue;
		Com_Printf("%s\n", cmd->name);
		i++;
	}
	Com_Printf("%i commands\n", i);
}

void
cmdcompletecfgname(char *args, int argNum)
{
	UNUSED(args);
	if(argNum == 2)
		fieldcompletefilename("", "cfg", qfalse, qtrue);
}

void
cmdinit(void)
{
	cmdadd("cmdlist",Cmd_List_f);
	cmdadd("exec",Cmd_Exec_f);
	cmdsetcompletion("exec", cmdcompletecfgname);
	cmdadd("vstr",Cmd_Vstr_f);
	cmdsetcompletion("vstr", cvarcompletename);
	cmdadd("echo",Cmd_Echo_f);
	cmdadd("wait", Cmd_Wait_f);
}
