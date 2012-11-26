/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		be_aas_file.h
*
* desc:		AAS
*
* $Archive: /source/code/botlib/be_aas_file.h $
*
*****************************************************************************/

#ifdef AASINTERN
/* loads the AAS file with the given name */
int AAS_LoadAASFile(char *filename);
/* writes an AAS file with the given name */
qbool AAS_WriteAASFile(char *filename);
/* dumps the loaded AAS data */
void AAS_DumpAASData(void);
/* print AAS file information */
void AAS_FileInfo(void);
#endif	/* AASINTERN */
