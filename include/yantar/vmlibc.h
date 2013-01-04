/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* 
 * stdlib replacement routines used by code compiled for the virtual 
 * machine 
 */

/* This file is NOT included on native builds */
#if !defined(BG_LIB_H) && defined(Q3_VM)
#define BG_LIB_H

/* Ignore __attribute__ on non-gcc platforms */
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;
typedef char* va_list;
typedef int cmp_t (const void *, const void *);

#define _INTSIZEOF(n)	((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define va_start(ap,v)	(ap = (va_list)&v + _INTSIZEOF(v))
#define va_arg(ap,t)	(*(t*)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define va_end(ap)	(ap = (va_list)0)

#define CHAR_BIT	8			/* number of bits in a char */
#define SCHAR_MAX	0x7f			/* maximum signed char value */
#define SCHAR_MIN	(-SCHAR_MAX - 1)	/* minimum signed char value */
#define UCHAR_MAX	0xff			/* maximum unsigned char value */

#define SHRT_MAX	0x7fff		/* maximum (signed) short value */
#define SHRT_MIN	(-SHRT_MAX - 1)	/* minimum (signed) short value */
#define USHRT_MAX	0xffff		/* maximum unsigned short value */
#define INT_MAX		0x7fffffff	/* maximum (signed) int value */
#define INT_MIN		(-INT_MAX - 1)	/* minimum (signed) int value */
#define UINT_MAX	0xffffffff	/* maximum unsigned int value */
#define LONG_MAX	0x7fffffffL	/* maximum (signed) long value */
#define LONG_MIN	(-LONG_MAX - 1)	/* minimum (signed) long value */
#define ULONG_MAX	0xffffffffUL	/* maximum unsigned long value */

#define isalnum(c)	(isalpha(c) || isdigit(c))
#define isalpha(c)	(isupper(c) || islower(c))
#define isascii(c)	((c) > 0 && (c) <= 0x7f)
#define iscntrl(c)	(((c) >= 0) && (((c) <= 0x1f) || ((c) == 0x7f)))
#define isdigit(c)	((c) >= '0' && (c) <= '9')
#define isgraph(c)	((c) != ' ' && isprint(c))
#define islower(c)	((c) >=  'a' && (c) <= 'z')
#define isprint(c)	((c) >= ' ' && (c) <= '~')
#define ispunct(c)	(((c) > ' ' && (c) <= '~') && !isalnum(c))
#define isspace(c)	((c) ==  ' ' || (c) == '\f' || (c) == '\n' || (c) == \
			 '\r' || \
			 (c) == '\t' || (c) == '\v')
#define isupper(c)	((c) >=  'A' && (c) <= 'Z')
#define isxdigit(c)	(isxupper(c) || isxlower(c))
#define isxlower(c)	(isdigit(c) || (c >= 'a' && c <= 'f'))
#define isxupper(c)	(isdigit(c) || (c >= 'A' && c <= 'F'))

void	qsort(void*, size_t, size_t, cmp_t*);
void	srand(unsigned);
int 	rand(void);

size_t	strlen(const char*);
char*	strcat(char*, const char*);
char*	strcpy(char*, const char*);
int	strcmp(const char*, const char*);
char*	strchr(const char*, int);
char*	strrchr(const char*, int);
char*	strstr(const char*, const char*);
char*	strncpy(char*, const char*, size_t);
int	tolower(int);
int	toupper(int);

double	atof(const char*);
double	_atof(const char**);
double	strtod(const char*r, char**);
int	atoi(const char*);
int	_atoi(const char**);
long	strtol(const char*, char**, int);
int	Q_vsnprintf(char*, size_t, const char*, va_list);
int	sscanf(const char*, const char*, ...) __attribute__ ((format (scanf, 2, 3)));

void*	memmove(void*, const void*, size_t);
void*	memset(void*, int, size_t);
void*	memcpy(void*, const void*, size_t);

double	ceil(double);
double	floor(double);
double	sqrt(double);
double	sin(double);
double	cos(double);
double	acos(double);
double	atan2(double, double);
double	atan(double);
double	asin(double);
double	tan(double);
int	abs(int);
double	fabs(double);

#endif	/* BG_LIB_H */

