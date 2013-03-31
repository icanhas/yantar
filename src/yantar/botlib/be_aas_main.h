/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef AASINTERN

extern aas_t aasworld;

void QDECL	AAS_Error(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void	AAS_SetInitialized(void);
int	AAS_Setup(void);
void	AAS_Shutdown(void);
int	AAS_LoadMap(const char *mapname);
int	AAS_StartFrame(float time);

#endif	/* AASINTERN */

/* returns true if AAS is initialized */
int	AAS_Initialized(void);
/* returns true if the AAS file is loaded */
int	AAS_Loaded(void);
/* returns the current time */
float	AAS_Time(void);
void	AAS_ProjectPointOntoVector(Vec3 point, Vec3 vStart, Vec3 vEnd,
		Vec3 vProj);
