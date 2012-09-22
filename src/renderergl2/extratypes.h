/*
 * Copyright (C) 2009-2011 Andrei Drexler, Richard Allen, James Canete
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifndef __TR_EXTRATYPES_H__
#define __TR_EXTRATYPES_H__

/* extratypes.h, for mods that want to extend types.h without losing compatibility with original VMs */

/* extra renderfx flags start at 0x0400 */
#define RF_SUNFLARE 0x0400

/* extra refdef flags start at 0x0008 */
#define RDF_NOFOG	0x0008	/* don't apply fog */
#define RDF_EXTRA	0x0010	/* Makro - refdefex_t to follow after refdef_t */

typedef struct {
	float blurFactor;
} refdefex_t;

#endif
