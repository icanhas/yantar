/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*****************************************************************************
* name:		be_aas_cluster.h
*
* desc:		AAS
*
* $Archive: /source/code/botlib/be_aas_cluster.h $
*
*****************************************************************************/

#ifdef AASINTERN
/* initialize the AAS clustering */
void AAS_InitClustering(void);
/*  */
void AAS_SetViewPortalsAsClusterPortals(void);
#endif	/* AASINTERN */
