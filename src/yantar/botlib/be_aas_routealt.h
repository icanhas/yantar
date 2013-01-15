/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef AASINTERN

void AAS_InitAlternativeRouting(void);
void AAS_ShutdownAlternativeRouting(void);

#endif	/* AASINTERN */

int AAS_AlternativeRouteGoals(Vec3 start, int startareanum, Vec3 goal,
			      int goalareanum, int travelflags,
			      aas_altroutegoal_t *altroutegoals,
			      int maxaltroutegoals,
			      int type);
