/* Stateless support routines that are included in all modules */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
#include "shared.h"

/*
 * Q_hashstr: Return the hashed value of string s. Will skip file extension
 * if passed a pathname.
 */
long
Q_hashstr(const char *s, int size)
{
	int	i, c;
	long	hash;

	hash = 0;
	for(i = 0; s[i] != '\0'; ++i){
		c = tolower(s[i]);
		if(c == '.')
			break;	/* don't include extension */
		if((c == PATH_SEP) && (c != '/'))
			c = '/';
		hash += c * (i + 119);
	}
	hash	^= (hash >> 10) ^ (hash >> 20);
	hash	&= size-1;
	return hash;
}

float
Q_clamp(float min, float max, float value)
{
	if(value < min)
		return min;
	if(value > max)
		return max;
	return value;
}

char *
Q_skippath(char *pathname)
{
	char *last;

	last = pathname;
	while(*pathname != '\0'){
		if(*pathname == '/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

char *
Q_getext(const char *name)
{
	char *dot = strrchr(name, '.'), *slash;
	if(dot && (!(slash = strrchr(name, '/')) || slash < dot))
		return dot + 1;
	else
		return "";
}

void
Q_stripext(const char *in, char *out, int destsize)
{
	const char *dot = strrchr(in, '.'), *slash;
	if(dot && (!(slash = strrchr(in, '/')) || slash < dot))
		Q_strncpyz(out, in, (destsize < dot-in+1 ? destsize : dot-in+1));
	else
		Q_strncpyz(out, in, destsize);
}

/*
 * Q_cmpext: string compare the end of the strings and return
 * qtrue if strings match
 */
qbool
Q_cmpext(const char *in, const char *ext)
{
	int inlen, extlen;

	inlen	= strlen(in);
	extlen	= strlen(ext);

	if(extlen <= inlen){
		in += inlen - extlen;

		if(!Q_stricmp(in, ext))
			return qtrue;
	}
	return qfalse;
}

/*
 * Q_defaultext: if path doesn't have an extension, then append the
 * specified one (which should include the .)
 */
void
Q_defaultext(char *path, int maxSize, const char *extension)
{
	const char *dot = strrchr(path, '.'), *slash;
	if(dot && (!(slash = strrchr(path, '/')) || slash < dot))
		return;
	else
		Q_strcat(path, maxSize, extension);
}

/*
 * Byte order functions
 */
/*
 * // can't just use function pointers, or dll linkage can
 * // mess up when qcommon is included in multiple places
 * static short	(*_BigShort) (short l);
 * static short	(*_LittleShort) (short l);
 * static int		(*_BigLong) (int l);
 * static int		(*_LittleLong) (int l);
 * static qint64	(*_BigLong64) (qint64 l);
 * static qint64	(*_LittleLong64) (qint64 l);
 * static float	(*_BigFloat) (const float *l);
 * static float	(*_LittleFloat) (const float *l);
 *
 * short	BigShort(short l){return _BigShort(l);}
 * short	LittleShort(short l) {return _LittleShort(l);}
 * int		BigLong (int l) {return _BigLong(l);}
 * int		LittleLong (int l) {return _LittleLong(l);}
 * qint64       BigLong64 (qint64 l) {return _BigLong64(l);}
 * qint64       LittleLong64 (qint64 l) {return _LittleLong64(l);}
 * float	BigFloat (const float *l) {return _BigFloat(l);}
 * float	LittleFloat (const float *l) {return _LittleFloat(l);}
 */

void
Q_cpshortswap(void *dest, void *src)
{
	byte *to = dest, *from = src;

	to[0]	= from[1];
	to[1]	= from[0];
}

void
Q_cplongswap(void *dest, void *src)
{
	byte *to = dest, *from = src;

	to[0]	= from[3];
	to[1]	= from[2];
	to[2]	= from[1];
	to[3]	= from[0];
}

short
Q_shortswap(short l)
{
	byte b1,b2;

	b1	= l&255;
	b2	= (l>>8)&255;

	return (b1<<8) + b2;
}

short
Q_shortnoswap(short l)
{
	return l;
}

int
Q_longswap(int l)
{
	byte b1,b2,b3,b4;

	b1	= l&255;
	b2	= (l>>8)&255;
	b3	= (l>>16)&255;
	b4	= (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int
Q_longnoswap(int l)
{
	return l;
}

qint64
Q_long64swap(qint64 ll)
{
	qint64 result;

	result.b0	= ll.b7;
	result.b1	= ll.b6;
	result.b2	= ll.b5;
	result.b3	= ll.b4;
	result.b4	= ll.b3;
	result.b5	= ll.b2;
	result.b6	= ll.b1;
	result.b7	= ll.b0;

	return result;
}

qint64
Q_long64noswap(qint64 ll)
{
	return ll;
}

float
Q_floatswap(const float *f)
{
	floatint_t out;

	out.f	= *f;
	out.ui	= Q_longswap(out.ui);

	return out.f;
}

float
Q_floatnoswap(const float *f)
{
	return *f;
}

/*
 * void Swap_Init (void)
 * {
 *      byte	swaptest[2] = {1,0};
 *
 * // set the byte swapping variables in a portable manner
 *      if ( *(short *)swaptest == 1)
 *      {
 *              _BigShort = Q_shortswap;
 *              _LittleShort = Q_shortnoswap;
 *              _BigLong = Q_longswap;
 *              _LittleLong = Q_longnoswap;
 *              _BigLong64 = Q_long64swap;
 *              _LittleLong64 = Q_long64noswap;
 *              _BigFloat = Q_floatswap;
 *              _LittleFloat = Q_floatnoswap;
 *      }
 *      else
 *      {
 *              _BigShort = Q_shortnoswap;
 *              _LittleShort = Q_shortswap;
 *              _BigLong = Q_longnoswap;
 *              _LittleLong = Q_longswap;
 *              _BigLong64 = Q_long64noswap;
 *              _LittleLong64 = Q_long64swap;
 *              _BigFloat = Q_floatnoswap;
 *              _LittleFloat = Q_floatswap;
 *      }
 *
 * }
 */

/*
 * Parsing
 */

static char	com_token[MAX_TOKEN_CHARS];
static int	com_lines;

/*
 * Q_readtok: Lex and return the next token of a string. 
 * Will never return nil, just empty strings.
 */
char *
Q_readtok(char **data_p)
{
	return Q_readtok2(data_p, qtrue);
}

static char *
Q_skipwhitespace(char *data, qbool *hasNewLines)
{
	int c;

	while((c = *data) <= ' '){
		if(c == '\0')
			return nil;

		if(c == '\n'){
			++com_lines;
			*hasNewLines = qtrue;
		}
		++data;
	}

	return data;
}

int
Q_compresstr(char *data_p)
{
	char	*in, *out;
	int	c;
	qbool newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	if(in != nil){
		while((c = *in) != '\0'){
			/* skip double slash comments */
			if(c == '/' && in[1] == '/')
				while(*in != '\0' && *in != '\n')
					in++;
			/* skip / * * / comments */
			else if(c == '/' && in[1] == '*'){
				while(*in != '\0' && (*in != '*' || in[1] != '/'))
					in++;
				if(*in != '\0')
					in += 2;
				/* record when we hit a newline */
			}else if(c == '\n' || c == '\r'){
				newline = qtrue;
				in++;
				/* record when we hit whitespace */
			}else if(c == ' ' || c == '\t'){
				whitespace = qtrue;
				in++;
				/* an actual token */
			}else{
				/*
				 * if we have a pending newline, emit it
				 * (and it counts as whitespace)
				 * */
				if(newline){
					*out++	= '\n';
					newline = qfalse;
					whitespace = qfalse;
				}
				if(whitespace){
					*out++ = ' ';
					whitespace = qfalse;
				}

				/* copy quoted strings unmolested */
				if(c == '"'){
					*out++ = c;
					in++;
					for(;; ){
						c = *in;
						if(c != '\0' && c != '"'){
							*out++ = c;
							in++;
						}else
							break;
					}
					if(c == '"'){
						*out++ = c;
						in++;
					}
				}else{
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = '\0';
	}
	return out - data_p;
}

/*
 * Q_readtok2: Lex and return the next token of a string.  
 * Will never return nil, just empty strings.
 * If "allowLineBreaks" is qtrue then an empty string will be
 * returned if the next token is a newline.
 */
char *
Q_readtok2(char **data_p, qbool allowLineBreaks)
{
	int c, len;
	qbool hasNewLines;
	char *data;

	c = 0;
	len	= 0;
	hasNewLines = qfalse;
	data	= *data_p;
	com_token[0] = '\0';
	
	/* make sure the incoming data is valid */
	if(data == nil){
		*data_p = nil;
		return com_token;
	}

	for(;;){
		data = Q_skipwhitespace(data, &hasNewLines);
		if(data == nil){
			*data_p = nil;
			return com_token;
		}
		if(hasNewLines && !allowLineBreaks){
			*data_p = data;
			return com_token;
		}

		c = *data;

		/* skip comments */
		if((c == '/') && (data[1] == '/')){
			data += 2;
			while((*data != '\0') && (*data != '\n'))
				++data;
		}else if((c == '/') && (data[1] == '*')){
			data += 2;
			while((*data != '\0') && ((*data != '*') || (data[1] != '/')))
				++data;
			if(*data != '\0')
				data += 2;
		}else
			break;
	}

	/* handle quoted strings */
	if(c == '\"'){
		data++;
		for(;;){
			c = *data++;
			if((c == '\"') || (c == '\0')){
				/* FIXME: really? */
				com_token[len] = '\0';
				*data_p = (char*)data;
				return com_token;
			}
			if(len < MAX_TOKEN_CHARS-1){
				com_token[len] = c;
				len++;
			}
		}
	}

	/* parse a regular word */
	do {
		if(len < MAX_TOKEN_CHARS - 1){
			com_token[len] = c;
			++len;
		}
		data++;
		c = *data;
		if(c == '\n')
			com_lines++;
	} while(c > 32);

	com_token[len] = 0;

	*data_p = (char*)data;
	return com_token;
}

void
Q_matchtok(char **buf_p, char *match)
{
	char *token;

	token = Q_readtok(buf_p);
	if(strcmp(token, match))
		Com_Errorf(ERR_DROP, "MatchToken: %s != %s", token, match);
}

/*
 * Q_skipblock: The next token should be an open brace. Skips until a
 * matching close brace is found. Internal brace depths are properly skipped.
 */
void
Q_skipblock(char **program)
{
	char	*token;
	int	depth;

	depth = 0;
	do {
		token = Q_readtok2(program, qtrue);
		if(token[1] == 0){
			if(token[0] == '{')
				depth++;
			else if(token[0] == '}')
				depth--;
		}
	} while(depth > 0 && *program != '\0');
}

void
Q_skipline(char **data)
{
	char	*p;
	int	c;

	p = *data;
	while((c = *p++) != 0)
		if(c == '\n'){
			com_lines++;
			break;
		}

	*data = p;
}

void
Parse1DMatrix(char **buf_p, int x, float *m)
{
	char	*token;
	int	i;

	Q_matchtok(buf_p, "(");
	for(i = 0; i < x; i++){
		token	= Q_readtok(buf_p);
		m[i]	= atof(token);
	}
	Q_matchtok(buf_p, ")");
}

void
Parse2DMatrix(char **buf_p, int y, int x, float *m)
{
	int i;

	Q_matchtok(buf_p, "(");
	for(i = 0; i < y; i++)
		Parse1DMatrix (buf_p, x, m + i * x);
	Q_matchtok(buf_p, ")");
}

void
Parse3DMatrix(char **buf_p, int z, int y, int x, float *m)
{
	int i;

	Q_matchtok(buf_p, "(");
	for(i = 0; i < z; i++)
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	Q_matchtok(buf_p, ")");
}

int
Q_hexstr2int(const char *str)
{
	if((str == nil) || (str[0] == '\0'))
		return -1;

	/* check for hex code */
	if((str[0] == '0') && (str[1] == 'x')){
		uint i, n;

		for(i = 2, n = 0; i < strlen(str); i++){
			char digit;

			n *= 16;

			digit = tolower(str[i]);

			if((digit >= '0') && (digit <= '9'))
				digit -= '0';
			else if((digit >= 'a') && (digit <= 'f'))
				digit = digit - 'a' + 10;
			else
				return -1;

			n += digit;
		}
		return n;
	}
	return -1;
}

/*
 * Library replacement functions
 */
int
Q_isprint(int c)
{
	if(c >= 0x20 && c <= 0x7E)
		return (1);
	return (0);
}

int
Q_islower(int c)
{
	if(c >= 'a' && c <= 'z')
		return (1);
	return (0);
}

int
Q_isupper(int c)
{
	if(c >= 'A' && c <= 'Z')
		return (1);
	return (0);
}

int
Q_isalpha(int c)
{
	if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return (1);
	return (0);
}

qbool
Q_isanumber(const char *s)
{
	char *p;

	if(*s == '\0')
		return qfalse;

	(void)strtod(s, &p);

	return *p == '\0';
}

qbool
Q_isintegral(float f)
{
	return (int)f == f;
}

#ifdef _MSC_VER
/*
 * Q_vsnprintf: Special wrapper function for Microsoft's broken _vsnprintf()
 * function. MinGW comes with its own snprintf() which is not broken.
 */
int
Q_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int retval;

	retval = _vsnprintf(str, size, format, ap);
	if(retval < 0 || retval == size){
		/*
		 * Microsoft doesn't adhere to the C99 standard of vsnprintf,
		 * which states that the return value must be the number of
		 * bytes written if the output string had sufficient length.
		 *
		 * Obviously we cannot determine that value from Microsoft's
		 * implementation, so we have no choice but to return size.
		 */
		str[size - 1] = '\0';
		return size;
	}
	return retval;
}
#endif

/* Q_strncpyz: Safe strncpy that ensures a trailing zero */
void
Q_strncpyz(char *dest, const char *src, int destsize)
{
	if(dest == nil)
		Com_Errorf(ERR_FATAL, "Q_strncpyz: nil dest");
	if(src == nil)
		Com_Errorf(ERR_FATAL, "Q_strncpyz: nil src");
	if(destsize < 1)
		Com_Errorf(ERR_FATAL,"Q_strncpyz: destsize < 1");
	strncpy(dest, src, destsize-1);
	dest[destsize-1] = 0;
}

int
Q_stricmpn(const char *s1, const char *s2, int n)
{
	int c1, c2;

	if(s1 == nil){
		if(s2 == nil)
			return 0;
		else
			return -1;
	}else if(s2==nil)
		return 1;



	do {
		c1	= *s1++;
		c2	= *s2++;

		if(!n--)
			return 0;	/* strings are equal until end point */

		if(c1 != c2){
			if(c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if(c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if(c1 != c2)
				return c1 < c2 ? -1 : 1;
		}
	} while(c1 != '\0');

	return 0;	/* strings are equal */
}

int
Q_strncmp(const char *s1, const char *s2, int n)
{
	int c1, c2;

	do {
		c1	= *s1++;
		c2	= *s2++;

		if(n-- <= 0)
			return 0;	/* strings are equal until end point */

		if(c1 != c2)
			return c1 < c2 ? -1 : 1;
	} while(c1 != '\0');

	return 0;	/* strings are equal */
}

int
Q_stricmp(const char *s1, const char *s2)
{
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}

char *
Q_strlwr(char *s1)
{
	char *s;

	s = s1;
	while(*s != '\0'){
		*s = tolower(*s);
		s++;
	}
	return s1;
}

char *
Q_strupr(char *s1)
{
	char *s;

	s = s1;
	while(*s != '\0'){
		*s = toupper(*s);
		s++;
	}
	return s1;
}

/* Q_strcat: never goes past bounds or leaves without a terminating 0 */
void
Q_strcat(char *dest, int size, const char *src)
{
	int l1;

	l1 = strlen(dest);
	if(l1 >= size)
		Com_Errorf(ERR_FATAL, "Q_strcat: already overflowed");
	Q_strncpyz(dest + l1, src, size - l1);
}

/* Q_Stristr: Find the first occurrence of find in s. */
const char *
Q_stristr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if((c = *find++) != 0){
		if(c >= 'a' && c <= 'z')
			c -= ('a' - 'A');
		len = strlen(find);
		do {
			do {
				if((sc = *s++) == '\0')
					return nil;
				if(sc >= 'a' && sc <= 'z')
					sc -= ('a' - 'A');
			} while(sc != c);
		} while(Q_stricmpn(s, find, len) != 0);
		--s;
	}
	return s;
}

int
Q_printablelen(const char *string)
{
	int len;
	const char *p;

	if(string == nil)
		return 0;

	len	= 0;
	p	= string;
	while(*p != '\0'){
		if(Q_IsColorString(p)){
			p += 2;
			continue;
		}
		p++;
		len++;
	}
	return len;
}

char *
Q_cleanstr(char *string)
{
	char	* d;
	char * s;
	int	c;

	s	= string;
	d	= string;
	while((c = *s) != '\0'){
		if(Q_IsColorString(s))
			s++;
		else if(c >= 0x20 && c <= 0x7E)
			*d++ = c;
		s++;
	}
	*d = '\0';
	return string;
}

int
Q_countchar(const char *string, char tocount)
{
	int count;

	for(count = 0; *string != '\0'; string++)
		if(*string == tocount)
			count++;

	return count;
}

int QDECL
Q_sprintf(char *dest, int size, const char *fmt, ...)
{
	int len;
	va_list argptr;

	va_start(argptr,fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);

	if(len >= size)
		Com_Printf("Q_sprintf: Output length %d too short, require"
			   " %d bytes.\n", size, len + 1);

	return len;
}

/*
 * va: does a varargs printf into a temp buffer, so I don't need to have
 * varargs versions of all text functions.
 */
char *QDECL
va(char *format, ...)
{
	va_list argptr;
	static char	string[2][32000];	/* in case va is called by nested functions */
	static int	index = 0;
	char *buf;

	buf = string[index & 1];
	index++;

	va_start(argptr, format);
	Q_vsnprintf(buf, sizeof(*string), format, argptr);
	va_end(argptr);

	return buf;
}

/* Q_truncstr: Assumes buffer is atleast TRUNCATE_LENGTH big */
void
Q_truncstr(char *buffer, const char *s)
{
	int length = strlen(s);

	if(length <= TRUNCATE_LENGTH)
		Q_strncpyz(buffer, s, TRUNCATE_LENGTH);
	else{
		Q_strncpyz(buffer, s, (TRUNCATE_LENGTH / 2) - 3);
		Q_strcat(buffer, TRUNCATE_LENGTH, " ... ");
		Q_strcat(buffer, TRUNCATE_LENGTH, s + length -
			(TRUNCATE_LENGTH / 2) + 3);
	}
}

/*
 * Info strings
 */

/*
 * Info_ValueForKey: Searches the string for the given key and returns the
 * associated value, or an empty string.
 *
 * FIXME: overflow check?
 */
char *
Info_ValueForKey(const char *s, const char *key)
{
	char pkey[BIG_INFO_KEY];
	/* use two buffers so compares work without stomping on each-other */
	static char	value[2][BIG_INFO_VALUE];
	static int	valueindex = 0;
	char *o;

	if(s == nil || key == nil)
		return "";

	if(strlen(s) >= BIG_INFO_STRING)
		Com_Errorf(ERR_DROP, "Info_ValueForKey: oversize infostring");

	valueindex ^= 1;
	if(*s == '\\')
		s++;
	for(;; ){
		o = pkey;
		while(*s != '\\'){
			if(*s == '\0')
				return "";
			*o++ = *s++;
		}
		*o = '\0';
		s++;

		o = value[valueindex];

		while((*s != '\\') && (*s != '\0'))
			*o++ = *s++;
		*o = '\0';

		if(!Q_stricmp(key, pkey))
			return value[valueindex];

		if(*s == '\0')
			break;
		s++;
	}

	return "";
}

/*
 * Info_NextPair: Used to itterate through all the key/value pairs in an info
 * string
 */
void
Info_NextPair(const char **head, char *key, char *value)
{
	char *o;
	const char *s;

	s = *head;

	if(*s == '\\')
		s++;
	key[0]		= 0;
	value[0]	= 0;

	o = key;
	while(*s != '\\'){
		if(*s == '\0'){
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = '\0';
	s++;

	o = value;
	while((*s != '\\') && (*s != '\0'))
		*o++ = *s++;
	*o = '\0';

	*head = s;
}

void
Info_RemoveKey(char *s, const char *key)
{
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char *o;

	if(strlen(s) >= MAX_INFO_STRING)
		Com_Errorf(ERR_DROP, "Info_RemoveKey: oversize infostring");

	if(strchr (key, '\\'))
		return;

	for(;; ){
		start = s;
		if(*s == '\\')
			s++;
		o = pkey;
		while(*s != '\\'){
			if(!*s)
				return;
			*o++ = *s++;
		}
		*o = '\0';
		s++;

		o = value;
		while(*s != '\\' && *s != '\0'){	/* FIXME */
			if(*s == '\0')
				return;
			*o++ = *s++;
		}
		*o = '\0';

		if(!strcmp (key, pkey)){
			memmove(start, s, strlen(s) + 1);	/* remove this part */
			return;
		}

		if(*s == '\0')
			return;
	}

}

void
Info_RemoveKey_Big(char *s, const char *key)
{
	char	*start;
	char	pkey[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char *o;

	if(strlen(s) >= BIG_INFO_STRING)
		Com_Errorf(ERR_DROP, "Info_RemoveKey_Big: oversize infostring");

	if(strchr(key, '\\'))
		return;

	for(;; ){
		start = s;
		if(*s == '\\')
			s++;
		o = pkey;
		while(*s != '\\'){
			if(*s == '\0')
				return;
			*o++ = *s++;
		}
		*o = '\0';
		s++;

		o = value;
		while(*s != '\\' && *s != '\0'){
			if(*s == '\0')
				return;
			*o++ = *s++;
		}
		*o = '\0';

		if(!strcmp (key, pkey)){
			strcpy(start, s);	/* remove this part */
			return;
		}

		if(*s == '\0')
			return;
	}

}

/*
 * Info_Validate: Some characters are illegal in info strings because they can
 * mess up the server's parsing
 */
qbool
Info_Validate(const char *s)
{
	if(strchr(s, '\"'))
		return qfalse;
	if(strchr(s, ';'))
		return qfalse;
	return qtrue;
}

/* Info_SetValueForKey: Changes or adds a key/value pair */
void
Info_SetValueForKey(char *s, const char *key, const char *value)
{
	char newi[MAX_INFO_STRING];
	const char *blacklist = "\\;\"";

	if(strlen(s) >= MAX_INFO_STRING)
		Com_Errorf(ERR_DROP, "Info_SetValueForKey: oversize infostring");

	for(; *blacklist != '\0'; ++blacklist)
		if(strchr(key, *blacklist) || strchr(value, *blacklist)){
			Com_Printf(S_COLOR_YELLOW "Can't use keys or values"
						  " with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}

	Info_RemoveKey(s, key);
	if(!value || !strlen(value))
		return;

	Q_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if(strlen(newi) + strlen(s) >= MAX_INFO_STRING){
		Com_Printf("Info string length exceeded\n");
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}

/*
 * Info_SetValueForKey_Big: Changes or adds a key/value pair Includes and
 * retains zero-length values
 */
void
Info_SetValueForKey_Big(char *s, const char *key, const char *value)
{
	char newi[BIG_INFO_STRING];
	const char *blacklist = "\\;\"";

	if(strlen(s) >= BIG_INFO_STRING)
		Com_Errorf(ERR_DROP, "Info_SetValueForKey: oversize infostring");

	for(; *blacklist; ++blacklist)
		if(strchr (key, *blacklist) || strchr(value, *blacklist)){
			Com_Printf(S_COLOR_YELLOW "Can't use keys or values"
						  " with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}

	Info_RemoveKey_Big(s, key);
	if(value == nil)
		return;

	Q_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);
	if(strlen(newi) + strlen(s) >= BIG_INFO_STRING){
		Com_Printf("BIG Info string length exceeded\n");
		return;
	}
	strcat (s, newi);
}

static qbool
Q_incharset(char c, char *set)
{
	uint i;

	for(i = 0; i < strlen(set); i++)
		if(set[i] == c)
			return qtrue;
	return qfalse;
}

char *
Q_skipcharset(char *s, char *sep)
{
	char *p = s;

	while(p != '\0'){
		if(Q_incharset(*p, sep))
			++p;
		else
			break;
	}
	return p;
}

char *
Q_skiptoks(char *s, int numTokens, char *sep)
{
	int sepCount = 0;
	char *p = s;

	while(sepCount < numTokens){
		if(Q_incharset(*p++, sep)){
			++sepCount;
			while(Q_incharset(*p, sep))
				++p;
		}else if(*p == '\0')
			break;
	}

	if(sepCount == numTokens)
		return p;
	else
		return s;
}
