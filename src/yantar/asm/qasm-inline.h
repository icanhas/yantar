/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#ifndef __ASM_INLINE_I386__
#define __ASM_INLINE_I386__

#include "platform.h"

#if idx64
  #define EAX	"%%rax"
  #define EBX	"%%rbx"
  #define ESP	"%%rsp"
  #define EDI	"%%rdi"
#else
  #define EAX	"%%eax"
  #define EBX	"%%ebx"
  #define ESP	"%%esp"
  #define EDI	"%%edi"
#endif

#endif
