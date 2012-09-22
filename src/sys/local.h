/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "q_shared.h"
#include "qcommon.h"

/* Require a minimum version of SDL */
#define MINSDL_MAJOR	1
#define MINSDL_MINOR	2
#define MINSDL_PATCH	10

/* Input subsystem */
void IN_Init(void);
void IN_Frame(void);
void IN_Shutdown(void);
void IN_Restart(void);

/* Console */
void CON_Shutdown(void);
void CON_Init(void);
char*CON_Input(void);
void CON_Print(const char *message);

unsigned int CON_LogSize(void);
unsigned int CON_LogWrite(const char *in);
unsigned int CON_LogRead(char *out, unsigned int outSize);

#ifdef MACOS_X
char*Sys_StripAppBundle(char *pwd);
#endif

void Sys_GLimpSafeInit(void);
void Sys_GLimpInit(void);
void Sys_PlatformInit(void);
void Sys_PlatformExit(void);
void Sys_SigHandler(int signal) __attribute__ ((noreturn));
void Sys_ErrorDialog(const char *error);
void Sys_AnsiColorPrint(const char *msg);

int Sys_PID(void);
qbool Sys_PIDIsRunning(int pid);
