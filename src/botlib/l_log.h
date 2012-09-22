/* log file */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

void Log_Open(char *filename);
void Log_Close(void);
void Log_Shutdown(void);
void QDECL Log_Write(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void QDECL Log_WriteTimeStamped(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
FILE*Log_FilePointer(void);
void Log_Flush(void);
