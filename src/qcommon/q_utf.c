/*
 * UTF-8 decoding and Unicode string routines.
 * May be linked into any module.
 */
/*
 * This file is part of Quake III Arena source code.
 *
 * Quake III Arena source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Quake III Arena source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include "q_shared.h"

enum {
	Bit1		= 7,
	Bitx		= 6,
	Bit2		= 5,
	Bit3		= 4,
	Bit4		= 3,
	Bit5		= 2,
	
	T1		= ((1<<(Bit1+1))-1) ^ 0xff,	/* 00000000 */
	Tx		= ((1<<(Bitx+1))-1) ^ 0xff,	/* 10000000 */
	T2		= ((1<<(Bit2+1))-1) ^ 0xff,	/* 11000000 */
	T3		= ((1<<(Bit3+1))-1) ^ 0xff,	/* 11100000 */
	T4		= ((1<<(Bit4+1))-1) ^ 0xff,	/* 11110000 */
	T5		= ((1<<(Bit5+1))-1) ^ 0xff,	/* 11111000 */
	
	Rune1	= (1<<(Bit1 + 0*Bitx))-1,		/* 00000000 00000000 01111111 */
	Rune2	= (1<<(Bit2 + 1*Bitx))-1,		/* 00000000 00000111 11111111 */
	Rune3	= (1<<(Bit3 + 2*Bitx))-1,		/* 00000000 11111111 11111111 */
	Rune4	= (1<<(Bit4 + 2*Bitx))-1,		/* 00111111 11111111 11111111 */
	
	Maskx	= (1<<Bitx)-1,				/* 00111111 */
	Testx	= (Maskx ^ 0xff)			/* 11000000 */
};

/*
 * UTF encoding & decoding
 */

int
Q_runetochar(char *s, rune_t *rune)
{
	rune_t c;
	
	c = *rune;
	/* one char sequence (0000-007f => 00-7f) */
	if(c <= Rune1){
		s[0] = c;
		return 1;
	}
	
	/* two char sequence (0080-07ff => T2 Tx) */
	if(c <= Rune2){
		s[0] = T2 | (c >> 1*Bitx);
		s[1] = Tx | (c & Maskx);
		return 2;
	}
		
	if(c > Runemax)
		c = Runeerror;
	/* three char sequence (0800-ffff => T3 Tx Tx) */
	if(c <= Rune3){
		s[0] = T3 | (c >> 2*Bitx);
		s[1] = Tx | ((c >> 1*Bitx) & Maskx);
		s[2] = Tx | (c & Maskx);
		return 3;
	}

	/* four char sequence (010000-1fffff => T4 Tx Tx Tx) */
	s[0] = T4 | (c >> 3*Bitx);
	s[1] = Tx | ((c >> 2*Bitx) & Maskx);
	s[2] = Tx | ((c >> 1*Bitx) & Maskx);
	s[3] = Tx | (c & Maskx);
	return 4;	
}

int Q_chartorune(rune_t *rune, char *s)
{
	int c, c1, c2, c3;
	rune_t r;
	
	c = *(byte*)s;
	/* one char sequence (0000-007f => T1) */
	if(c < Tx){
		*rune = c;
		return 1;
	}
	
	c1 = *(byte*)(s+1) ^ Tx;
	/* two char sequence (0080-07ff => T2 Tx) */
	if(c1 & Testx)
		goto bad;
	if(c < T3){
		if(c < T2)
			goto bad;
		r = ((c << Bitx) | c1) & Rune2;
		if(r <= Rune1)
			goto bad;
		*rune = r;
		return 2;
	}
	
	c2 = *(byte*)(s+2) ^ Tx;
	/* three char sequence (0800-ffff => T3 Tx Tx) */
	if(c2 & Testx)
		goto bad;
	if(c < T4){
		r = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if(r <= Rune2)
			goto bad;
		*rune = r;
		return 3;
	}
	
	c3 = *(byte*)(s+3) ^ Tx;
	/* four char sequence (010000-1fffff => T4 Tx Tx Tx) */
	if(c3 & Testx)
		goto bad;
	if(c < T5){
		r = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
		if(r <= Rune3)
			goto bad;
		if(r > Runemax)
			goto bad;
		*rune = r;
		return 4;
	} 
	
bad:
	/* bad sequence */
	*rune = Runeerror;
	return 1;
}

int
Q_runelen(ulong c)
{
	rune_t r;
	char s[10];
	
	r = c;
	return Q_runetochar(s, &r);
}

int
Q_fullrune(char *s, int n)
{
	int c;
	
	if(n <= 0)
		return 0;
	c = *(byte*)s;
	if(c < Tx)
		return 1;
	if(c < T3)
		return n >= 2;
	if(n < T4)
		return n >= 3;
	return n >= 4;
}

int
Q_utflen(char *s)
{
	int c;
	long n;
	rune_t r;
	
	n = 0;
	for(;;){
		c = *(byte*)s;
		if(c < Runeself){
			/* single byte rune */
			if(c == '\0')
				return n;
			s++;
		}else
			s += Q_chartorune(&r, s);
		n++;
	}
}

char*
Q_utfrune(char *s, ulong c)
{
	ulong c1;
	rune_t r;
	int n;
	
	if(c < Runesync)
		/* not part of utf sequence */
		return strchr(s, c);
	for(;;){
		c1 = *(byte*)s;
		if(c1 < Runeself){	/* single byte rune */
			if(c1 == 0)
				return 0;
			if(c1 == c)
				return s;
			s++;
			continue;
		}
		n = Q_chartorune(&r, s);
		if(r == c)
			return s;
		s += n;
	}
}

/* return pointer to first occurrence of s2 in s1, or nil if none */
char*
Q_utfutf(char *s1, char *s2)
{
	char *p;
	ulong f, n1, n2;
	rune_t r;
	
	n1 = Q_chartorune(&r, s2);
	f = r;
	if(f <= Runesync)
		/* represents self */
		return strstr(s1, s2);
	n2 = strlen(s2);
	for(p = s1; (p = Q_utfrune(p, f)); p += n1)
		if(Q_strncmp(p, s2, n2) == 0)
			return p;
	return nil;
}

/*
 * rune str routines
 */

rune_t*
Q_runestrcat(rune_t *s1, rune_t *s2)
{
	Q_runestrcpy(Q_runestrchr(s1, 0), s2);
	return s1;
}

rune_t*
Q_runestrchr(rune_t *s, rune_t c)
{
	rune_t c0, c1;
	
	c0 = c;
	if(c == 0){
		while(*s++ != 0)
			;
		return s-1;
	}
	while((c1 = *s++))
		if((c1 == c0))
			return s-1;
	return 0;
}

int
Q_runestrcmp(rune_t *s1, rune_t *s2)
{
	rune_t c1, c2;
	
	for(;;){
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2){
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			return 0;
	}
}

rune_t*
Q_runestrcpy(rune_t *dst, rune_t *src)
{
	rune_t *odst;
	
	odst = dst;
	while((*dst++ = *src++))
		;
	return odst;
}

rune_t*
Q_runestrncpy(rune_t* dst, rune_t *src, size_t n)
{
	size_t i;
	rune_t *odst;
	
	odst = dst;
	for(i = 0; i < n; i++)
		if((*dst++ = *src++) == 0){
			while(++i < n)
				*dst++ = 0;
			return odst;
		}
	return odst;
}

int
Q_runestrncmp(rune_t *s1, rune_t *s2, size_t n)
{
	rune_t c1, c2;
	
	while(n > 0){
		c1 = *s1++;
		c2 = *s2++;
		n--;
		if(c1 != c2){
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}

size_t
Q_runestrlen(rune_t *s)
{
	/* FIXME */
	return (size_t)(Q_runestrchr(s, 0) - s);
}

/* return pointer to first occurrence of s2 in s1, or nil if none */
rune_t*
Q_runestrstr(rune_t *s1, rune_t *s2)
{
	rune_t *p, *pa, *pb;
	ulong c0, c;
	
	c0 = *s2;
	if(c0 == 0)
		return s1;
	s2++;
	for(p = Q_runestrchr(s1, c0); p; p = Q_runestrchr(p+1, c0)){
		pa = p;
		for(pb = s2; ; pb++){
			c = *pb;
			if(c == 0)
				return p;
			if(c != *++pa)
				break;
		}
	}
	return nil;
}
