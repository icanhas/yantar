/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#ifndef __ASM_I386__
#define __ASM_I386__

#include "../qcommon/q_platform.h"

#ifdef __ELF__
.section.note.GNU-stack,"",@progbits
#endif

#if defined(__ELF__) || defined(__WIN64__)
#define C(label)	label
#else
#define C(label)	_ ## label
#endif

#endif
