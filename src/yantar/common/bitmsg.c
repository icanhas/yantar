/*
 * message IO functions -- handles byte ordering and avoids 
 * alignment errors
 */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "common.h"

static Huffman msgHuff;
static qbool msgInit = qfalse;
int oldsize = 0;

void MSG_initHuffman(void);

void
MSG_Init(Bitmsg *buf, byte *data, int length)
{
	if(!msgInit)
		MSG_initHuffman();
	Q_Memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void
MSG_InitOOB(Bitmsg *buf, byte *data, int length)
{
	if(!msgInit)
		MSG_initHuffman();
	Q_Memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
	buf->oob = qtrue;
}

void
MSG_Clear(Bitmsg *buf)
{
	buf->cursize = 0;
	buf->overflowed = qfalse;
	buf->bit = 0;	/* <- in bits */
}


void
MSG_Bitstream(Bitmsg *buf)
{
	buf->oob = qfalse;
}

void
MSG_BeginReading(Bitmsg *msg)
{
	msg->readcount	= 0;
	msg->bit	= 0;
	msg->oob	= qfalse;
}

void
MSG_BeginReadingOOB(Bitmsg *msg)
{
	msg->readcount	= 0;
	msg->bit	= 0;
	msg->oob	= qtrue;
}

void
MSG_Copy(Bitmsg *buf, byte *data, int length, Bitmsg *src)
{
	if(length<src->cursize)
		comerrorf(ERR_DROP,
			"MSG_Copy: can't copy into a smaller Bitmsg buffer");
	Q_Memcpy(buf, src, sizeof(Bitmsg));
	buf->data = data;
	Q_Memcpy(buf->data, src->data, src->cursize);
}

/*
 * bit functions
 */

int overflows;

/* negative bit values include signs */
void
MSG_WriteBits(Bitmsg *msg, int value, int bits)
{
	int i;

	oldsize += bits;
	/* this isn't an exact overflow check, but close enough */
	if(msg->maxsize - msg->cursize < 4){
		msg->overflowed = qtrue;
		return;
	}
	if(bits == 0 || bits < -31 || bits > 32)
		comerrorf(ERR_DROP, "MSG_WriteBits: bad bits %i", bits);
	/* check for overflows */
	if(bits != 32){
		if(bits > 0){
			if(value > ((1 << bits) - 1) || value < 0)
				overflows++;
		}else{
			int r;

			r = 1 << (bits-1);

			if(value >  r - 1 || value < -r)
				overflows++;
		}
	}
	if(bits < 0)
		bits = -bits;
	if(msg->oob){
		if(bits==8){
			msg->data[msg->cursize] = value;
			msg->cursize += 1;
			msg->bit += 8;
		}else if(bits==16){
			short temp = value;

			CopyLittleShort(&msg->data[msg->cursize], &temp);
			msg->cursize += 2;
			msg->bit += 16;
		}else if(bits==32){
			CopyLittleLong(&msg->data[msg->cursize], &value);
			msg->cursize += 4;
			msg->bit += 32;
		}else
			comerrorf(ERR_DROP, "can't write %d bits", bits);
	}else{
		value &= (0xffffffff>>(32-bits));
		if(bits&7){
			int nbits;
			nbits = bits&7;
			for(i=0; i<nbits; i++){
				huffputbit((value&1), msg->data, &msg->bit);
				value = (value>>1);
			}
			bits = bits - nbits;
		}
		if(bits)
			for(i=0; i<bits; i+=8){
				huffoffsettransmit (&msgHuff.compressor,
					(value&0xff), msg->data, &msg->bit);
				value = (value>>8);
			}
		msg->cursize = (msg->bit>>3)+1;
	}
}

int
MSG_ReadBits(Bitmsg *msg, int bits)
{
	int value;
	int get;
	qbool sgn;
	int i, nbits;

	value = 0;
	if(bits < 0){
		bits = -bits;
		sgn = qtrue;
	}else
		sgn = qfalse;
	if(msg->oob){
		if(bits==8){
			value = msg->data[msg->readcount];
			msg->readcount += 1;
			msg->bit += 8;
		}else if(bits==16){
			short temp;

			CopyLittleShort(&temp, &msg->data[msg->readcount]);
			value = temp;
			msg->readcount += 2;
			msg->bit += 16;
		}else if(bits==32){
			CopyLittleLong(&value, &msg->data[msg->readcount]);
			msg->readcount += 4;
			msg->bit += 32;
		}else
			comerrorf(ERR_DROP, "can't read %d bits", bits);
	}else{
		nbits = 0;
		if(bits&7){
			nbits = bits&7;
			for(i=0; i<nbits; i++)
				value |= (huffgetbit(msg->data, &msg->bit)<<i);
			bits = bits - nbits;
		}
		if(bits)
			for(i=0; i<bits; i+=8){
				huffoffsetrecv (msgHuff.decompressor.tree,
					&get, msg->data,
					&msg->bit);
				value |= (get<<(i+nbits));
			}
		msg->readcount = (msg->bit>>3)+1;
	}
	if(sgn)
		if(value & (1 << (bits - 1)))
			value |= -1 ^ ((1 << bits) - 1);
	return value;
}

/*
 * writing functions
 */

void
MSG_WriteChar(Bitmsg *sb, int c)
{
#ifdef PARANOID
	if(c < -128 || c > 127)
		comerrorf (ERR_FATAL, "MSG_WriteChar: range error");
#endif
	MSG_WriteBits(sb, c, 8);
}

void
MSG_WriteByte(Bitmsg *sb, int c)
{
#ifdef PARANOID
	if(c < 0 || c > 255)
		comerrorf (ERR_FATAL, "MSG_WriteByte: range error");
#endif
	MSG_WriteBits(sb, c, 8);
}

void
MSG_WriteData(Bitmsg *buf, const void *data, int length)
{
	int i;

	for(i=0; i<length; i++)
		MSG_WriteByte(buf, ((byte*)data)[i]);
}

void
MSG_WriteShort(Bitmsg *sb, int c)
{
#ifdef PARANOID
	if(c < ((short)0x8000) || c > (short)0x7fff)
		comerrorf (ERR_FATAL, "MSG_WriteShort: range error");
#endif
	MSG_WriteBits(sb, c, 16);
}

void
MSG_WriteLong(Bitmsg *sb, int c)
{
	MSG_WriteBits(sb, c, 32);
}

void
MSG_WriteFloat(Bitmsg *sb, float f)
{
	Flint dat;

	dat.f = f;
	MSG_WriteBits(sb, dat.i, 32);
}

void
MSG_WriteString(Bitmsg *sb, const char *s)
{
	if(!s)
		MSG_WriteData(sb, "", 1);
	else{
		int	l,i;
		char	string[MAX_STRING_CHARS];

		l = strlen(s);
		if(l >= MAX_STRING_CHARS){
			comprintf("MSG_WriteString: MAX_STRING_CHARS");
			MSG_WriteData (sb, "", 1);
			return;
		}
		Q_strncpyz(string, s, sizeof(string));
		/* get rid of 0x80+ and '%' chars, because old clients don't like them */
		for(i = 0; i < l; i++)
			if(((byte*)string)[i] > 127 || string[i] == '%')
				string[i] = '.';

		MSG_WriteData (sb, string, l+1);
	}
}

void
MSG_WriteBigString(Bitmsg *sb, const char *s)
{
	if(!s)
		MSG_WriteData (sb, "", 1);
	else{
		int l,i;
		char	string[BIG_INFO_STRING];

		l = strlen(s);
		if(l >= BIG_INFO_STRING){
			comprintf("MSG_WriteString: BIG_INFO_STRING");
			MSG_WriteData (sb, "", 1);
			return;
		}
		Q_strncpyz(string, s, sizeof(string));
		/* get rid of 0x80+ and '%' chars, because old clients don't like them */
		for(i = 0; i < l; i++)
			if(((byte*)string)[i] > 127 || string[i] == '%')
				string[i] = '.';

		MSG_WriteData (sb, string, l+1);
	}
}

void
MSG_WriteAngle(Bitmsg *sb, float f)
{
	MSG_WriteByte (sb, (int)(f*256/360) & 255);
}

void
MSG_WriteAngle16(Bitmsg *sb, float f)
{
	MSG_WriteShort (sb, ANGLE2SHORT(f));
}

/* returns -1 if no more characters are available */
int
MSG_ReadChar(Bitmsg *msg)
{
	int c;

	c = (signed char)MSG_ReadBits(msg, 8);
	if(msg->readcount > msg->cursize)
		c = -1;

	return c;
}

int
MSG_ReadByte(Bitmsg *msg)
{
	int c;

	c = (unsigned char)MSG_ReadBits(msg, 8);
	if(msg->readcount > msg->cursize)
		c = -1;
	return c;
}

int
MSG_LookaheadByte(Bitmsg *msg)
{
	int bloc, readcount, bit;
	int c;
	
	bloc = huffgetbloc();
	readcount = msg->readcount;
	bit = msg->bit;
	c = MSG_ReadByte(msg);
	huffsetbloc(bloc);
	msg->readcount = readcount;
	msg->bit = bit;
	return c;
}

int
MSG_ReadShort(Bitmsg *msg)
{
	int c;

	c = (short)MSG_ReadBits(msg, 16);
	if(msg->readcount > msg->cursize)
		c = -1;

	return c;
}

int
MSG_ReadLong(Bitmsg *msg)
{
	int c;

	c = MSG_ReadBits(msg, 32);
	if(msg->readcount > msg->cursize)
		c = -1;
	return c;
}

float
MSG_ReadFloat(Bitmsg *msg)
{
	Flint dat;

	dat.i = MSG_ReadBits(msg, 32);
	if(msg->readcount > msg->cursize)
		dat.f = -1;
	return dat.f;
}

char *
MSG_ReadString(Bitmsg *msg)
{
	static char str[MAX_STRING_CHARS];
	int c;
	size_t l;

	l = 0;
	do {
		c = MSG_ReadByte(msg);	/* use ReadByte so -1 is out of bounds */
		if(c == -1 || c == 0)
			break;
		/* translate all fmt spec to avoid crash bugs */
		if(c == '%')
			c = '.';
		/* don't allow higher ascii values */
		if(c > 127)
			c = '.';
		str[l] = c;
		l++;
	} while(l < sizeof(str)-1);
	str[l] = 0;
	return str;
}

char *
MSG_ReadBigString(Bitmsg *msg)
{
	static char str[BIG_INFO_STRING];
	int c;
	size_t l;

	l = 0;
	do {
		c = MSG_ReadByte(msg);	/* use ReadByte so -1 is out of bounds */
		if(c == -1 || c == 0)
			break;
		/* translate all fmt spec to avoid crash bugs */
		if(c == '%')
			c = '.';
		/* don't allow higher ascii values */
		if(c > 127)
			c = '.';
		str[l] = c;
		l++;
	} while(l < sizeof(str)-1);
	str[l] = 0;
	return str;
}

char *
MSG_ReadStringLine(Bitmsg *msg)
{
	static char str[MAX_STRING_CHARS];
	int c;
	size_t l;

	l = 0;
	do {
		c = MSG_ReadByte(msg);	/* use ReadByte so -1 is out of bounds */
		if(c == -1 || c == 0 || c == '\n')
			break;
		/* translate all fmt spec to avoid crash bugs */
		if(c == '%')
			c = '.';
		/* don't allow higher ascii values */
		if(c > 127)
			c = '.';
		str[l] = c;
		l++;
	} while(l < sizeof(str)-1);
	str[l] = 0;
	return str;
}

float
MSG_ReadAngle16(Bitmsg *msg)
{
	return SHORT2ANGLE(MSG_ReadShort(msg));
}

void
MSG_ReadData(Bitmsg *msg, void *data, int len)
{
	int i;

	for(i=0; i<len; i++)
		((byte*)data)[i] = MSG_ReadByte (msg);
}

/*
 * a string hasher which gives the same hash value even if the
 * string is later modified via the legacy MSG read/write code
 */
int
MSG_HashKey(const char *s, int maxlen)
{
	int hash, i;

	hash = 0;
	for(i = 0; i < maxlen && s[i] != '\0'; i++){
		if(s[i] & 0x80 || s[i] == '%')
			hash += '.' * (119 + i);
		else
			hash += s[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

/*
 * delta functions
 */

extern Cvar *cl_shownet;
#define LOG(x) if(cl_shownet->integer == 4){ comprintf("%s ", x); };

void
MSG_WriteDelta(Bitmsg *msg, int oldV, int newV, int bits)
{
	if(oldV == newV){
		MSG_WriteBits(msg, 0, 1);
		return;
	}
	MSG_WriteBits(msg, 1, 1);
	MSG_WriteBits(msg, newV, bits);
}

int
MSG_ReadDelta(Bitmsg *msg, int oldV, int bits)
{
	if(MSG_ReadBits(msg, 1))
		return MSG_ReadBits(msg, bits);
	return oldV;
}

void
MSG_WriteDeltaFloat(Bitmsg *msg, float oldV, float newV)
{
	Flint fi;
	
	if(oldV == newV){
		MSG_WriteBits(msg, 0, 1);
		return;
	}
	fi.f = newV;
	MSG_WriteBits(msg, 1, 1);
	MSG_WriteBits(msg, fi.i, 32);
}

float
MSG_ReadDeltaFloat(Bitmsg *msg, float oldV)
{
	if(MSG_ReadBits(msg, 1)){
		Flint fi;

		fi.i = MSG_ReadBits(msg, 32);
		return fi.f;
	}
	return oldV;
}

/*
 * delta functions with keys
 */

int kbitmask[32] = {
	0x00000001, 0x00000003, 0x00000007, 0x0000000F,
	0x0000001F,     0x0000003F,     0x0000007F,     0x000000FF,
	0x000001FF,     0x000003FF,     0x000007FF,     0x00000FFF,
	0x00001FFF,     0x00003FFF,     0x00007FFF,     0x0000FFFF,
	0x0001FFFF,     0x0003FFFF,     0x0007FFFF,     0x000FFFFF,
	0x001FFFFf,     0x003FFFFF,     0x007FFFFF,     0x00FFFFFF,
	0x01FFFFFF,     0x03FFFFFF,     0x07FFFFFF,     0x0FFFFFFF,
	0x1FFFFFFF,     0x3FFFFFFF,     0x7FFFFFFF,     0xFFFFFFFF,
};

void
MSG_WriteDeltaKey(Bitmsg *msg, int key, int oldV, int newV, int bits)
{
	if(oldV == newV){
		MSG_WriteBits(msg, 0, 1);
		return;
	}
	MSG_WriteBits(msg, 1, 1);
	MSG_WriteBits(msg, newV ^ key, bits);
}

int
MSG_ReadDeltaKey(Bitmsg *msg, int key, int oldV, int bits)
{
	if(MSG_ReadBits(msg, 1))
		return MSG_ReadBits(msg, bits) ^ (key & kbitmask[bits]);
	return oldV;
}

void
MSG_WriteDeltaKeyFloat(Bitmsg *msg, int key, float oldV, float newV)
{
	Flint fi;
	
	if(oldV == newV){
		MSG_WriteBits(msg, 0, 1);
		return;
	}
	fi.f = newV;
	MSG_WriteBits(msg, 1, 1);
	MSG_WriteBits(msg, fi.i ^ key, 32);
}

float
MSG_ReadDeltaKeyFloat(Bitmsg *msg, int key, float oldV)
{
	if(MSG_ReadBits(msg, 1)){
		Flint fi;

		fi.i = MSG_ReadBits(msg, 32) ^ key;
		return fi.f;
	}
	return oldV;
}


/*
 * Usrcmd communication
 */

/* ms is allways sent, the others are optional */
#define CM_ANGLE1	(1<<0)
#define CM_ANGLE2	(1<<1)
#define CM_ANGLE3	(1<<2)
#define CM_FORWARD	(1<<3)
#define CM_SIDE		(1<<4)
#define CM_UP		(1<<5)
#define CM_BUTTONS	(1<<6)
#define CM_WEAPON	(1<<7)

void
MSG_WriteDeltaUsercmd(Bitmsg *msg, Usrcmd *from, Usrcmd *to)
{
	if(to->serverTime - from->serverTime < 256){
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, to->serverTime - from->serverTime, 8);
	}else{
		MSG_WriteBits(msg, 0, 1);
		MSG_WriteBits(msg, to->serverTime, 32);
	}
	MSG_WriteDelta(msg, from->angles[0], to->angles[0], 16);
	MSG_WriteDelta(msg, from->angles[1], to->angles[1], 16);
	MSG_WriteDelta(msg, from->angles[2], to->angles[2], 16);
	MSG_WriteDelta(msg, from->forwardmove, to->forwardmove, 8);
	MSG_WriteDelta(msg, from->rightmove, to->rightmove, 8);
	MSG_WriteDelta(msg, from->upmove, to->upmove, 8);
	MSG_WriteDelta(msg, from->buttons, to->buttons, 16);
	MSG_WriteDelta(msg, from->weap[WSpri], to->weap[WSpri], 8);
	MSG_WriteDelta(msg, from->weap[WSsec], to->weap[WSsec], 8);
	MSG_WriteDelta(msg, from->weap[WShook], to->weap[WShook], 8);
}

void
MSG_ReadDeltaUsercmd(Bitmsg *msg, Usrcmd *from, Usrcmd *to)
{
	if(MSG_ReadBits(msg, 1))
		to->serverTime = from->serverTime + MSG_ReadBits(msg, 8);
	else
		to->serverTime = MSG_ReadBits(msg, 32);
	to->angles[0]	= MSG_ReadDelta(msg, from->angles[0], 16);
	to->angles[1]	= MSG_ReadDelta(msg, from->angles[1], 16);
	to->angles[2]	= MSG_ReadDelta(msg, from->angles[2], 16);
	to->forwardmove = MSG_ReadDelta(msg, from->forwardmove, 8);
	if(to->forwardmove == -128)
		to->forwardmove = -127;
	to->rightmove = MSG_ReadDelta(msg, from->rightmove, 8);
	if(to->rightmove == -128)
		to->rightmove = -127;
	to->upmove = MSG_ReadDelta(msg, from->upmove, 8);
	if(to->upmove == -128)
		to->upmove = -127;
	to->brakefrac = MSG_ReadDelta(msg, from->brakefrac, 8);
	to->buttons = MSG_ReadDelta(msg, from->buttons, 16);
	to->weap[WSpri] = MSG_ReadDelta(msg, from->weap[WSpri], 8);
	to->weap[WSsec] = MSG_ReadDelta(msg, from->weap[WSsec], 8);
	to->weap[WShook] = MSG_ReadDelta(msg, from->weap[WShook], 8);
}

void
MSG_WriteDeltaUsercmdKey(Bitmsg *msg, int key, Usrcmd *from, Usrcmd *to)
{
	if(to->serverTime - from->serverTime < 256){
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, to->serverTime - from->serverTime, 8);
	}else{
		MSG_WriteBits(msg, 0, 1);
		MSG_WriteBits(msg, to->serverTime, 32);
	}
	if(from->angles[0] == to->angles[0] &&
	   from->angles[1] == to->angles[1] &&
	   from->angles[2] == to->angles[2] &&
	   from->forwardmove == to->forwardmove &&
	   from->rightmove == to->rightmove &&
	   from->upmove == to->upmove &&
	   from->brakefrac == to->brakefrac &&
	   from->buttons == to->buttons &&
	   from->weap[WSpri] == to->weap[WSpri] &&
	   from->weap[WSsec] == to->weap[WSsec] &&
	   from->weap[WShook] == to->weap[WShook])
	then{
		MSG_WriteBits(msg, 0, 1);	/* no change */
		oldsize += 7;
		return;
	}
	key ^= to->serverTime;
	MSG_WriteBits(msg, 1, 1);
	MSG_WriteDeltaKey(msg, key, from->angles[0], to->angles[0], 16);
	MSG_WriteDeltaKey(msg, key, from->angles[1], to->angles[1], 16);
	MSG_WriteDeltaKey(msg, key, from->angles[2], to->angles[2], 16);
	MSG_WriteDeltaKey(msg, key, from->forwardmove, to->forwardmove, 8);
	MSG_WriteDeltaKey(msg, key, from->rightmove, to->rightmove, 8);
	MSG_WriteDeltaKey(msg, key, from->upmove, to->upmove, 8);
	MSG_WriteDeltaKey(msg, key, from->brakefrac, to->brakefrac, 8);
	MSG_WriteDeltaKey(msg, key, from->buttons, to->buttons, 16);
	MSG_WriteDeltaKey(msg, key, from->weap[WSpri], to->weap[WSpri], 8);
	MSG_WriteDeltaKey(msg, key, from->weap[WSsec], to->weap[WSsec], 8);
	MSG_WriteDeltaKey(msg, key, from->weap[WShook], to->weap[WShook], 8);
}

void
MSG_ReadDeltaUsercmdKey(Bitmsg *msg, int key, Usrcmd *from, Usrcmd *to)
{
	if(MSG_ReadBits(msg, 1))
		to->serverTime = from->serverTime + MSG_ReadBits(msg, 8);
	else
		to->serverTime = MSG_ReadBits(msg, 32);
	if(MSG_ReadBits(msg, 1)){
		key ^= to->serverTime;
		to->angles[0] = MSG_ReadDeltaKey(msg, key, from->angles[0], 16);
		to->angles[1] = MSG_ReadDeltaKey(msg, key, from->angles[1], 16);
		to->angles[2] = MSG_ReadDeltaKey(msg, key, from->angles[2], 16);
		to->forwardmove = MSG_ReadDeltaKey(msg, key, from->forwardmove,
			8);
		to->rightmove = MSG_ReadDeltaKey(msg, key, from->rightmove, 8);
		to->upmove = MSG_ReadDeltaKey(msg, key, from->upmove, 8);
		to->brakefrac = MSG_ReadDeltaKey(msg, key, from->brakefrac, 8);
		to->buttons = MSG_ReadDeltaKey(msg, key, from->buttons, 16);
		to->weap[WSpri] = MSG_ReadDeltaKey(msg, key, from->weap[WSpri], 8);
		to->weap[WSsec] = MSG_ReadDeltaKey(msg, key, from->weap[WSsec], 8);
		to->weap[WShook] = MSG_ReadDeltaKey(msg, key, from->weap[WShook], 8);
	}else{
		to->angles[0] = from->angles[0];
		to->angles[1] = from->angles[1];
		to->angles[2] = from->angles[2];
		to->forwardmove = from->forwardmove;
		to->rightmove = from->rightmove;
		to->upmove = from->upmove;
		to->buttons = from->buttons;
		to->weap[WSpri] = from->weap[WSpri];
		to->weap[WSsec] = from->weap[WSsec];
		to->weap[WShook] = from->weap[WShook];
	}
}

/*
 * Entstate communication
 */

typedef struct {
	char	*name;
	int	offset;
	int	bits;	/* 0 = float */
} Netfield;

/* using the stringizing operator to save typing... */
#define NETF(x) # x,(size_t)&((Entstate*)0)->x

Netfield entityStateFields[] =
{
	{ NETF(traj.time), 32 },
	{ NETF(traj.base[0]), 0 },
	{ NETF(traj.base[1]), 0 },
	{ NETF(traj.delta[0]), 0 },
	{ NETF(traj.delta[1]), 0 },
	{ NETF(traj.base[2]), 0 },
	{ NETF(apos.base[1]), 0 },
	{ NETF(traj.delta[2]), 0 },
	{ NETF(apos.base[0]), 0 },
	{ NETF(event), 10 },
	{ NETF(angles2[1]), 0 },
	{ NETF(eType), 8 },
	{ NETF(torsoAnim), 8 },
	{ NETF(eventParm), 8 },
	{ NETF(legsAnim), 8 },
	{ NETF(groundEntityNum), GENTITYNUM_BITS },
	{ NETF(traj.type), 8 },
	{ NETF(eFlags), 19 },
	{ NETF(otherEntityNum), GENTITYNUM_BITS },
	{ NETF(weap[WSpri]), 8 },
	{ NETF(weap[WSsec]), 8 },
	{ NETF(weap[WShook]), 8 },
	{ NETF(parentweap), 8 },
	{ NETF(clientNum), 8 },
	{ NETF(angles[1]), 0 },
	{ NETF(traj.duration), 32 },
	{ NETF(apos.type), 8 },
	{ NETF(origin[0]), 0 },
	{ NETF(origin[1]), 0 },
	{ NETF(origin[2]), 0 },
	{ NETF(solid), 24 },
	{ NETF(powerups), MAX_POWERUPS },
	{ NETF(modelindex), 8 },
	{ NETF(otherEntityNum2), GENTITYNUM_BITS },
	{ NETF(loopSound), 8 },
	{ NETF(generic1), 8 },
	{ NETF(origin2[2]), 0 },
	{ NETF(origin2[0]), 0 },
	{ NETF(origin2[1]), 0 },
	{ NETF(modelindex2), 8 },
	{ NETF(angles[0]), 0 },
	{ NETF(time), 32 },
	{ NETF(apos.time), 32 },
	{ NETF(apos.duration), 32 },
	{ NETF(apos.base[2]), 0 },
	{ NETF(apos.delta[0]), 0 },
	{ NETF(apos.delta[1]), 0 },
	{ NETF(apos.delta[2]), 0 },
	{ NETF(time2), 32 },
	{ NETF(angles[2]), 0 },
	{ NETF(angles2[0]), 0 },
	{ NETF(angles2[2]), 0 },
	{ NETF(constantLight), 32 },
	{ NETF(frame), 16 }
};

/* if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
 * the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent */
#define FLOAT_INT_BITS	13
#define FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))

/*
 * Writes part of a packetentities message, including the entity number.
 * Can delta from either a baseline or a previous packet_entity
 * If to is NULL, a remove entity update will be sent
 * If force is not set, then nothing at all will be generated if the entity is
 * identical, under the assumption that the in-order delta code will catch it.
 */
void
MSG_WriteDeltaEntity(Bitmsg *msg, struct Entstate *from,
	struct Entstate *to, qbool force)
{
	int i, lc;
	int numFields;
	Netfield *field;
	int trunc;
	float fullFloat;
	int *fromF, *toF;

	/*
	 * all fields should be 32 bits to avoid any compiler packing issues
	 * the "number" field is not part of the field list
	 * if this assert fails, someone added a field to the Entstate
	 * struct without updating the message fields
	 */
	numFields = ARRAY_LEN(entityStateFields);
	assert(numFields + 1 == sizeof(*from)/4);

	/* a NULL to is a delta remove message */
	if(to == NULL){
		if(from == NULL)
			return;
		MSG_WriteBits(msg, from->number, GENTITYNUM_BITS);
		MSG_WriteBits(msg, 1, 1);
		return;
	}
	if(to->number < 0 || to->number >= MAX_GENTITIES)
		comerrorf (ERR_FATAL,
			"MSG_WriteDeltaEntity: Bad entity number: %i",
			to->number);
	lc = 0;
	/* build the change vector as bytes so it is endien independent */
	for(i = 0, field = entityStateFields; i < numFields; i++, field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);
		if(*fromF != *toF)
			lc = i+1;
	}
	if(lc == 0){
		/* nothing at all changed */
		if(!force)
			return;		/* nothing at all */
		/* write two bits for no change */
		MSG_WriteBits(msg, to->number, GENTITYNUM_BITS);
		MSG_WriteBits(msg, 0, 1);	/* not removed */
		MSG_WriteBits(msg, 0, 1);	/* no delta */
		return;
	}
	MSG_WriteBits(msg, to->number, GENTITYNUM_BITS);
	MSG_WriteBits(msg, 0, 1);	/* not removed */
	MSG_WriteBits(msg, 1, 1);	/* we have a delta */
	MSG_WriteByte(msg, lc);		/* # of changes */
	oldsize += numFields;

	for(i = 0, field = entityStateFields; i < lc; i++, field++){
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);
		if(*fromF == *toF){
			MSG_WriteBits(msg, 0, 1);	/* no change */
			continue;
		}
		MSG_WriteBits(msg, 1, 1);	/* changed */
		if(field->bits == 0){
			/* float */
			fullFloat = *(float*)toF;
			trunc = (int)fullFloat;
			if(fullFloat == 0.0f){
				MSG_WriteBits(msg, 0, 1);
				oldsize += FLOAT_INT_BITS;
			}else{
				MSG_WriteBits(msg, 1, 1);
				if(trunc == fullFloat && trunc +
				   FLOAT_INT_BIAS >= 0 &&
				   trunc + FLOAT_INT_BIAS <
				   (1 << FLOAT_INT_BITS)){
					/* send as small integer */
					MSG_WriteBits(msg, 0, 1);
					MSG_WriteBits(msg, trunc +
						FLOAT_INT_BIAS,
						FLOAT_INT_BITS);
				}else{
					/* send as full floating point value */
					MSG_WriteBits(msg, 1, 1);
					MSG_WriteBits(msg, *toF, 32);
				}
			}
		}else{
			if(*toF == 0)
				MSG_WriteBits(msg, 0, 1);
			else{
				MSG_WriteBits(msg, 1, 1);
				/* integer */
				MSG_WriteBits(msg, *toF, field->bits);
			}
		}
	}
}

/*
 * The entity number has already been read from the message, which
 * is how the from state is identified.
 *
 * If the delta removes the entity, Entstate->number will be set to MAX_GENTITIES-1
 *
 * Can go from either a baseline or a previous packet_entity
 */
void
MSG_ReadDeltaEntity(Bitmsg *msg, Entstate *from, Entstate *to,
	int number)
{
	int i, lc;
	int numFields;
	Netfield	*field;
	int *fromF, *toF;
	int print;
	int trunc;
	int startBit, endBit;

	if(number < 0 || number >= MAX_GENTITIES)
		comerrorf(ERR_DROP, "Bad delta entity number: %i", number);
		
	if(msg->bit == 0)
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	else
		startBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS;
	/* check for a remove */
	if(MSG_ReadBits(msg, 1) == 1){
		Q_Memset(to, 0, sizeof(*to));
		to->number = MAX_GENTITIES - 1;
		if(cl_shownet->integer >= 2 || cl_shownet->integer == -1)
			comprintf("%3i: #%-3i remove\n", msg->readcount, number);
		return;
	}
	/* check for no delta */
	if(MSG_ReadBits(msg, 1) == 0){
		*to = *from;
		to->number = number;
		return;
	}

	numFields = ARRAY_LEN(entityStateFields);
	lc = MSG_ReadByte(msg);
	if(lc > numFields || lc < 0)
		comerrorf(ERR_DROP, "invalid entityState field count");

	/*
	 * shownet 2/3 will interleave with other printed info, -1 will
	 * just print the delta records`
	 */
	if(cl_shownet->integer >= 2 || cl_shownet->integer == -1){
		print = 1;
		comprintf("%3i: #%-3i ", msg->readcount, to->number);
	}else
		print = 0;
	to->number = number;
	for(i = 0, field = entityStateFields; i < lc; i++, field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);
		if(!MSG_ReadBits(msg, 1))
			/* no change */
			*toF = *fromF;
		else{
			if(field->bits == 0){
				/* float */
				if(MSG_ReadBits(msg, 1) == 0)
					*(float*)toF = 0.0f;
				else{
					if(MSG_ReadBits(msg, 1) == 0){
						/* integral float */
						trunc = MSG_ReadBits(
							msg, FLOAT_INT_BITS);
						/* bias to allow equal parts positive and negative */
						trunc -= FLOAT_INT_BIAS;
						*(float*)toF = trunc;
						if(print)
							comprintf("%s:%i ",
								field->name,
								trunc);
					}else{
						/* full floating point value */
						*toF = MSG_ReadBits(msg, 32);
						if(print)
							comprintf(
								"%s:%f ",
								field->name,
								*(float*)toF);
					}
				}
			}else{
				if(MSG_ReadBits(msg, 1) == 0)
					*toF = 0;
				else{
					/* integer */
					*toF = MSG_ReadBits(msg, field->bits);
					if(print)
						comprintf("%s:%i ", field->name,
							*toF);
				}
			}
		}
	}
	for(i = lc, field = &entityStateFields[lc]; i < numFields; i++,
	    field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);
		/* no change */
		*toF = *fromF;
	}

	if(print){
		if(msg->bit == 0)
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		else
			endBit =
				(msg->readcount -
				 1) * 8 + msg->bit - GENTITYNUM_BITS;
		comprintf(" (%i bits)\n", endBit - startBit);
	}
}


/*
 * Playerstate communication
 */

/* using the stringizing operator to save typing... */
#define PSF(x) # x,(size_t)&((Playerstate*)0)->x

Netfield playerStateFields[] =
{
	{ PSF(commandTime), 32 },
	{ PSF(origin[0]), 0 },
	{ PSF(origin[1]), 0 },
	{ PSF(origin[2]), 0 },
	{ PSF(bobCycle), 8 },
	{ PSF(velocity[0]), 0 },
	{ PSF(velocity[1]), 0 },
	{ PSF(velocity[2]), 0 },
	{ PSF(viewangles[0]), 0 },
	{ PSF(viewangles[1]), 0 },
	{ PSF(viewangles[2]), 0 },
	{ PSF(weaptime[WSpri]), -16 },
	{ PSF(weaptime[WSsec]), -16 },
	{ PSF(weaptime[WShook]), -16 },
	{ PSF(legsTimer), 8 },
	{ PSF(pm_time), -16 },
	{ PSF(eventSequence), 16 },
	{ PSF(torsoAnim), 8 },
	{ PSF(movementDir), 4 },
	{ PSF(events[0]), 8 },
	{ PSF(legsAnim), 8 },
	{ PSF(events[1]), 8 },
	{ PSF(pm_flags), 16 },
	{ PSF(groundEntityNum), GENTITYNUM_BITS },
	{ PSF(weapstate[WSpri]), 4 },
	{ PSF(weapstate[WSsec]), 4 },
	{ PSF(weapstate[WShook]), 4},
	{ PSF(eFlags), 16 },
	{ PSF(externalEvent), 10 },
	{ PSF(gravity), 16 },
	{ PSF(speed), 16 },
	{ PSF(swingstrength), 0 },
	{ PSF(delta_angles[1]), 16 },
	{ PSF(externalEventParm), 8 },
	{ PSF(viewheight), -8 },
	{ PSF(damageEvent), 8 },
	{ PSF(damageYaw), 8 },
	{ PSF(damagePitch), 8 },
	{ PSF(damageCount), 8 },
	{ PSF(generic1), 8 },
	{ PSF(pm_type), 8 },
	{ PSF(delta_angles[0]), 16 },
	{ PSF(delta_angles[2]), 16 },
	{ PSF(torsoTimer), 12 },
	{ PSF(eventParms[0]), 8 },
	{ PSF(eventParms[1]), 8 },
	{ PSF(clientNum), 8 },
	{ PSF(weap[WSpri]), 5 },
	{ PSF(weap[WSsec]), 5 },
	{ PSF(weap[WShook]), 5},
	{ PSF(grapplePoint[0]), 0 },
	{ PSF(grapplePoint[1]), 0 },
	{ PSF(grapplePoint[2]), 0 },
	{ PSF(jumppad_ent), 10 },
	{ PSF(loopSound), 16 }
};

void
MSG_WriteDeltaPlayerstate(Bitmsg *msg, struct Playerstate *from,
			  struct Playerstate *to)
{
	int i;
	Playerstate dummy;
	int statsbits;
	int persistantbits;
	int ammobits;
	int powerupbits;
	int numFields;
	Netfield *field;
	int *fromF, *toF;
	float fullFloat;
	int trunc, lc;

	if(!from){
		from = &dummy;
		Q_Memset(&dummy, 0, sizeof(dummy));
	}
	numFields = ARRAY_LEN(playerStateFields);
	lc = 0;
	for(i = 0, field = playerStateFields; i < numFields; i++, field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);
		if(*fromF != *toF)
			lc = i+1;
	}
	MSG_WriteByte(msg, lc);	/* # of changes */
	oldsize += numFields - lc;
	for(i = 0, field = playerStateFields; i < lc; i++, field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);

		if(*fromF == *toF){
			MSG_WriteBits(msg, 0, 1);	/* no change */
			continue;
		}

		MSG_WriteBits(msg, 1, 1);	/* changed */

		if(field->bits == 0){
			/* float */
			fullFloat = *(float*)toF;
			trunc = (int)fullFloat;
			if(trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
			   trunc + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS)){
				/* send as small integer */
				MSG_WriteBits(msg, 0, 1);
				MSG_WriteBits(msg, trunc + FLOAT_INT_BIAS,
					FLOAT_INT_BITS);
			}else{
				/* send as full floating point value */
				MSG_WriteBits(msg, 1, 1);
				MSG_WriteBits(msg, *toF, 32);
			}
		}else
			/* integer */
			MSG_WriteBits(msg, *toF, field->bits);
	}


	/* send the arrays */
	statsbits = 0;
	for(i=0; i<MAX_STATS; i++)
		if(to->stats[i] != from->stats[i])
			statsbits |= 1<<i;
	persistantbits = 0;
	for(i=0; i<MAX_PERSISTANT; i++)
		if(to->persistant[i] != from->persistant[i])
			persistantbits |= 1<<i;
	ammobits = 0;
	for(i=0; i<MAX_WEAPONS; i++)
		if(to->ammo[i] != from->ammo[i])
			ammobits |= 1<<i;
	powerupbits = 0;
	for(i=0; i<MAX_POWERUPS; i++)
		if(to->powerups[i] != from->powerups[i])
			powerupbits |= 1<<i;

	if(!statsbits && !persistantbits && !ammobits && !powerupbits){
		MSG_WriteBits(msg, 0, 1);	/* no change */
		oldsize += 4;
		return;
	}
	MSG_WriteBits(msg, 1, 1);	/* changed */

	if(statsbits){
		MSG_WriteBits(msg, 1, 1);	/* changed */
		MSG_WriteBits(msg, statsbits, MAX_STATS);
		for(i=0; i<MAX_STATS; i++)
			if(statsbits & (1<<i))
				MSG_WriteShort (msg, to->stats[i]);
	}else
		MSG_WriteBits(msg, 0, 1);	/* no change */

	if(persistantbits){
		MSG_WriteBits(msg, 1, 1);	/* changed */
		MSG_WriteBits(msg, persistantbits, MAX_PERSISTANT);
		for(i=0; i<MAX_PERSISTANT; i++)
			if(persistantbits & (1<<i))
				MSG_WriteShort (msg, to->persistant[i]);
	}else
		MSG_WriteBits(msg, 0, 1);	/* no change */

	if(ammobits){
		MSG_WriteBits(msg, 1, 1);	/* changed */
		MSG_WriteBits(msg, ammobits, MAX_WEAPONS);
		for(i=0; i<MAX_WEAPONS; i++)
			if(ammobits & (1<<i))
				MSG_WriteShort (msg, to->ammo[i]);
	}else
		MSG_WriteBits(msg, 0, 1);	/* no change */

	if(powerupbits){
		MSG_WriteBits(msg, 1, 1);	/* changed */
		MSG_WriteBits(msg, powerupbits, MAX_POWERUPS);
		for(i=0; i<MAX_POWERUPS; i++)
			if(powerupbits & (1<<i))
				MSG_WriteLong(msg, to->powerups[i]);
	}else
		MSG_WriteBits(msg, 0, 1);	/* no change */
}

void
MSG_ReadDeltaPlayerstate(Bitmsg *msg, Playerstate *from, Playerstate *to)
{
	int i, lc;
	int bits;
	Netfield *field;
	int numFields;
	int startBit, endBit;
	int print;
	int *fromF, *toF;
	int trunc;
	Playerstate dummy;

	if(!from){
		from = &dummy;
		Q_Memset(&dummy, 0, sizeof(dummy));
	}
	*to = *from;
	if(msg->bit == 0)
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	else
		startBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS;

	/* 
	 * shownet 2/3 will interleave with other printed info, -2 will
	 * just print the delta records
	 */
	if(cl_shownet->integer >= 2 || cl_shownet->integer == -2){
		print = 1;
		comprintf("%3i: playerstate ", msg->readcount);
	}else
		print = 0;

	numFields = ARRAY_LEN(playerStateFields);
	lc = MSG_ReadByte(msg);
	if(lc > numFields || lc < 0)
		comerrorf(ERR_DROP, "invalid playerState field count");

	for(i = 0, field = playerStateFields; i < lc; i++, field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);

		if(!MSG_ReadBits(msg, 1))
			/* no change */
			*toF = *fromF;
		else{
			if(field->bits == 0){
				/* float */
				if(MSG_ReadBits(msg, 1) == 0){
					/* integral float */
					trunc = MSG_ReadBits(msg, FLOAT_INT_BITS);
					/* bias to allow equal parts positive and negative */
					trunc -= FLOAT_INT_BIAS;
					*(float*)toF = trunc;
					if(print)
						comprintf("%s:%i ", field->name,
							trunc);
				}else{
					/* full floating point value */
					*toF = MSG_ReadBits(msg, 32);
					if(print)
						comprintf("%s:%f ", field->name,
							*(float*)toF);
				}
			}else{
				/* integer */
				*toF = MSG_ReadBits(msg, field->bits);
				if(print)
					comprintf("%s:%i ", field->name, *toF);
			}
		}
	}
	for(i=lc,field = &playerStateFields[lc]; i<numFields; i++, field++){
		fromF	= (int*)((byte*)from + field->offset);
		toF	= (int*)((byte*)to + field->offset);
		/* no change */
		*toF = *fromF;
	}

	/* read the arrays */
	if(MSG_ReadBits(msg, 1)){
		/* parse stats */
		if(MSG_ReadBits(msg, 1)){
			LOG("PS_STATS");
			bits = MSG_ReadBits (msg, MAX_STATS);
			for(i=0; i<MAX_STATS; i++)
				if(bits & (1<<i))
					to->stats[i] = MSG_ReadShort(msg);
		}
		/* parse persistant stats */
		if(MSG_ReadBits(msg, 1)){
			LOG("PS_PERSISTANT");
			bits = MSG_ReadBits (msg, MAX_PERSISTANT);
			for(i=0; i<MAX_PERSISTANT; i++)
				if(bits & (1<<i))
					to->persistant[i] = MSG_ReadShort(msg);
		}
		/* parse ammo */
		if(MSG_ReadBits(msg, 1)){
			LOG("PS_AMMO");
			bits = MSG_ReadBits (msg, MAX_WEAPONS);
			for(i=0; i<MAX_WEAPONS; i++)
				if(bits & (1<<i))
					to->ammo[i] = MSG_ReadShort(msg);
		}
		/* parse powerups */
		if(MSG_ReadBits(msg, 1)){
			LOG("PS_POWERUPS");
			bits = MSG_ReadBits (msg, MAX_POWERUPS);
			for(i=0; i<MAX_POWERUPS; i++)
				if(bits & (1<<i))
					to->powerups[i] = MSG_ReadLong(msg);
		}
	}

	if(print){
		if(msg->bit == 0)
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		else
			endBit =(msg->readcount -
				 1) * 8 + msg->bit - GENTITYNUM_BITS;
		comprintf(" (%i bits)\n", endBit - startBit);
	}
}

int msg_hData[256] = {
	250315,	/* 0 */
	41193,	/* 1 */
	6292,	/* 2 */
	7106,	/* 3 */
	3730,	/* 4 */
	3750,	/* 5 */
	6110,	/* 6 */
	23283,	/* 7 */
	33317,	/* 8 */
	6950,	/* 9 */
	7838,	/* 10 */
	9714,	/* 11 */
	9257,	/* 12 */
	17259,	/* 13 */
	3949,	/* 14 */
	1778,	/* 15 */
	8288,	/* 16 */
	1604,	/* 17 */
	1590,	/* 18 */
	1663,	/* 19 */
	1100,	/* 20 */
	1213,	/* 21 */
	1238,	/* 22 */
	1134,	/* 23 */
	1749,	/* 24 */
	1059,	/* 25 */
	1246,	/* 26 */
	1149,	/* 27 */
	1273,	/* 28 */
	4486,	/* 29 */
	2805,	/* 30 */
	3472,	/* 31 */
	21819,	/* 32 */
	1159,	/* 33 */
	1670,	/* 34 */
	1066,	/* 35 */
	1043,	/* 36 */
	1012,	/* 37 */
	1053,	/* 38 */
	1070,	/* 39 */
	1726,	/* 40 */
	888,		/* 41 */
	1180,	/* 42 */
	850,		/* 43 */
	960,		/* 44 */
	780,		/* 45 */
	1752,	/* 46 */
	3296,	/* 47 */
	10630,	/* 48 */
	4514,	/* 49 */
	5881,	/* 50 */
	2685,	/* 51 */
	4650,	/* 52 */
	3837,	/* 53 */
	2093,	/* 54 */
	1867,	/* 55 */
	2584,	/* 56 */
	1949,	/* 57 */
	1972,	/* 58 */
	940,		/* 59 */
	1134,	/* 60 */
	1788,	/* 61 */
	1670,	/* 62 */
	1206,	/* 63 */
	5719,	/* 64 */
	6128,	/* 65 */
	7222,	/* 66 */
	6654,	/* 67 */
	3710,	/* 68 */
	3795,	/* 69 */
	1492,	/* 70 */
	1524,	/* 71 */
	2215,	/* 72 */
	1140,	/* 73 */
	1355,	/* 74 */
	971,		/* 75 */
	2180,	/* 76 */
	1248,	/* 77 */
	1328,	/* 78 */
	1195,	/* 79 */
	1770,	/* 80 */
	1078,	/* 81 */
	1264,	/* 82 */
	1266,	/* 83 */
	1168,	/* 84 */
	965,		/* 85 */
	1155,	/* 86 */
	1186,	/* 87 */
	1347,	/* 88 */
	1228,	/* 89 */
	1529,	/* 90 */
	1600,	/* 91 */
	2617,	/* 92 */
	2048,	/* 93 */
	2546,	/* 94 */
	3275,	/* 95 */
	2410,	/* 96 */
	3585,	/* 97 */
	2504,	/* 98 */
	2800,	/* 99 */
	2675,	/* 100 */
	6146,	/* 101 */
	3663,	/* 102 */
	2840,	/* 103 */
	14253,	/* 104 */
	3164,	/* 105 */
	2221,	/* 106 */
	1687,	/* 107 */
	3208,	/* 108 */
	2739,	/* 109 */
	3512,	/* 110 */
	4796,	/* 111 */
	4091,	/* 112 */
	3515,	/* 113 */
	5288,	/* 114 */
	4016,	/* 115 */
	7937,	/* 116 */
	6031,	/* 117 */
	5360,	/* 118 */
	3924,	/* 119 */
	4892,	/* 120 */
	3743,	/* 121 */
	4566,	/* 122 */
	4807,	/* 123 */
	5852,	/* 124 */
	6400,	/* 125 */
	6225,	/* 126 */
	8291,	/* 127 */
	23243,	/* 128 */
	7838,	/* 129 */
	7073,	/* 130 */
	8935,	/* 131 */
	5437,	/* 132 */
	4483,	/* 133 */
	3641,	/* 134 */
	5256,	/* 135 */
	5312,	/* 136 */
	5328,	/* 137 */
	5370,	/* 138 */
	3492,	/* 139 */
	2458,	/* 140 */
	1694,	/* 141 */
	1821,	/* 142 */
	2121,	/* 143 */
	1916,	/* 144 */
	1149,	/* 145 */
	1516,	/* 146 */
	1367,	/* 147 */
	1236,	/* 148 */
	1029,	/* 149 */
	1258,	/* 150 */
	1104,	/* 151 */
	1245,	/* 152 */
	1006,	/* 153 */
	1149,	/* 154 */
	1025,	/* 155 */
	1241,	/* 156 */
	952,		/* 157 */
	1287,	/* 158 */
	997,		/* 159 */
	1713,	/* 160 */
	1009,	/* 161 */
	1187,	/* 162 */
	879,		/* 163 */
	1099,	/* 164 */
	929,		/* 165 */
	1078,	/* 166 */
	951,		/* 167 */
	1656,	/* 168 */
	930,		/* 169 */
	1153,	/* 170 */
	1030,	/* 171 */
	1262,	/* 172 */
	1062,	/* 173 */
	1214,	/* 174 */
	1060,	/* 175 */
	1621,	/* 176 */
	930,		/* 177 */
	1106,	/* 178 */
	912,		/* 179 */
	1034,	/* 180 */
	892,		/* 181 */
	1158,	/* 182 */
	990,		/* 183 */
	1175,	/* 184 */
	850,		/* 185 */
	1121,	/* 186 */
	903,		/* 187 */
	1087,	/* 188 */
	920,		/* 189 */
	1144,	/* 190 */
	1056,	/* 191 */
	3462,	/* 192 */
	2240,	/* 193 */
	4397,	/* 194 */
	12136,	/* 195 */
	7758,	/* 196 */
	1345,	/* 197 */
	1307,	/* 198 */
	3278,	/* 199 */
	1950,	/* 200 */
	886,		/* 201 */
	1023,	/* 202 */
	1112,	/* 203 */
	1077,	/* 204 */
	1042,	/* 205 */
	1061,	/* 206 */
	1071,	/* 207 */
	1484,	/* 208 */
	1001,	/* 209 */
	1096,	/* 210 */
	915,		/* 211 */
	1052,	/* 212 */
	995,		/* 213 */
	1070,	/* 214 */
	876,		/* 215 */
	1111,	/* 216 */
	851,		/* 217 */
	1059,	/* 218 */
	805,		/* 219 */
	1112,	/* 220 */
	923,		/* 221 */
	1103,	/* 222 */
	817,		/* 223 */
	1899,	/* 224 */
	1872,	/* 225 */
	976,		/* 226 */
	841,		/* 227 */
	1127,	/* 228 */
	956,		/* 229 */
	1159,	/* 230 */
	950,		/* 231 */
	7791,	/* 232 */
	954,		/* 233 */
	1289,	/* 234 */
	933,		/* 235 */
	1127,	/* 236 */
	3207,	/* 237 */
	1020,	/* 238 */
	927,		/* 239 */
	1355,	/* 240 */
	768,		/* 241 */
	1040,	/* 242 */
	745,		/* 243 */
	952,		/* 244 */
	805,		/* 245 */
	1073,	/* 246 */
	740,		/* 247 */
	1013,	/* 248 */
	805,		/* 249 */
	1008,	/* 250 */
	796,		/* 251 */
	996,		/* 252 */
	1057,	/* 253 */
	11457,	/* 254 */
	13504,	/* 255 */
};

void
MSG_initHuffman(void)
{
	int i, j;

	msgInit = qtrue;
	huffinit(&msgHuff);
	for(i=0; i<256; i++)
		for(j=0; j<msg_hData[i]; j++){
			huffaddref(&msgHuff.compressor, (byte)i);	/* Do update */
			huffaddref(&msgHuff.decompressor, (byte)i);	/* Do update */
		}
}
