/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* 
 * This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list 
 */

#include "shared.h"
#include "common.h"

static int bloc = 0;

/* Receive one bit from the input file (buffered) */
static int
get_bit(byte *fin)
{
	int t;
	t = (fin[(bloc>>3)] >> (bloc&7)) & 0x1;
	bloc++;
	return t;
}

/* Add a bit to the output file (buffered) */
static void
add_bit(char bit, byte *fout)
{
	if((bloc&7) == 0)
		fout[(bloc>>3)] = 0;
	fout[(bloc>>3)] |= bit << (bloc&7);
	bloc++;
}

static Node **
get_ppnode(Huff* huff)
{
	Node **tppnode;
	if(!huff->freelist)
		return &(huff->nodePtrs[huff->blocPtrs++]);
	else{
		tppnode = huff->freelist;
		huff->freelist = (Node**)*tppnode;
		return tppnode;
	}
}

static void
free_ppnode(Huff* huff, Node **ppnode)
{
	*ppnode = (Node*)huff->freelist;
	huff->freelist = ppnode;
}

/* Swap the location of these two nodes in the tree */
static void
swap(Huff* huff, Node *node1, Node *node2)
{
	Node *par1, *par2;

	par1 = node1->parent;
	par2 = node2->parent;

	if(par1){
		if(par1->left == node1)
			par1->left = node2;
		else
			par1->right = node2;
	}else
		huff->tree = node2;

	if(par2){
		if(par2->left == node2)
			par2->left = node1;
		else
			par2->right = node1;
	}else
		huff->tree = node1;

	node1->parent = par2;
	node2->parent = par1;
}

/* Swap these two nodes in the linked list (update ranks) */
static void
swaplist(Node *node1, Node *node2)
{
	Node *par1;

	par1 = node1->next;
	node1->next = node2->next;
	node2->next = par1;

	par1 = node1->prev;
	node1->prev = node2->prev;
	node2->prev = par1;

	if(node1->next == node1)
		node1->next = node2;
	if(node2->next == node2)
		node2->next = node1;
	if(node1->next)
		node1->next->prev = node1;
	if(node2->next)
		node2->next->prev = node2;
	if(node1->prev)
		node1->prev->next = node1;
	if(node2->prev)
		node2->prev->next = node2;
}

/* Do the increments */
static void
increment(Huff* huff, Node *n)
{
	Node *ln;

	if(n == nil)
		return;

	if(n->next != nil && n->next->weight == n->weight){
		ln = *n->head;
		if(ln != n->parent)
			swap(huff, ln, n);
		swaplist(ln, n);
	}
	if(n->prev && n->prev->weight == n->weight)
		*n->head = n->prev;
	else{
		*n->head = nil;
		free_ppnode(huff, n->head);
	}
	n->weight++;
	if(n->next && n->next->weight == n->weight)
		n->head = n->next->head;
	else{
		n->head = get_ppnode(huff);
		*n->head = n;
	}
	if(n->parent){
		increment(huff, n->parent);
		if(n->prev == n->parent){
			swaplist(n, n->parent);
			if(*n->head == n)
				*n->head = n->parent;
		}
	}
}

/* Send the prefix code for this node */
static void
send(Node *node, Node *child, byte *fout)
{
	if(node->parent)
		send(node->parent, node, fout);
	if(child){
		if(node->right == child)
			add_bit(1, fout);
		else
			add_bit(0, fout);
	}
}

void
huffaddref(Huff* h, byte ch)
{
	Node *tn, *tn2;

	if(h->loc[ch] != nil){
		increment(h, h->loc[ch]);
		return;
	}

	/* else, this is the first transmission of this node */
	tn = &(h->nodeList[h->blocNode++]);
	tn2 = &(h->nodeList[h->blocNode++]);

	tn2->symbol = INTERNAL_NODE;
	tn2->weight = 1;
	tn2->next = h->lhead->next;
	if(h->lhead->next){
		h->lhead->next->prev = tn2;
		if(h->lhead->next->weight == 1)
			tn2->head = h->lhead->next->head;
		else{
			tn2->head = get_ppnode(h);
			*tn2->head = tn2;
		}
	}else{
		tn2->head = get_ppnode(h);
		*tn2->head = tn2;
	}
	h->lhead->next = tn2;
	tn2->prev = h->lhead;

	tn->symbol = ch;
	tn->weight = 1;
	tn->next = h->lhead->next;
	if(h->lhead->next){
		h->lhead->next->prev = tn;
		if(h->lhead->next->weight == 1)
			tn->head = h->lhead->next->head;
		else{
			/* this should never happen */
			tn->head = get_ppnode(h);
			*tn->head = tn2;
		}
	}else{
		/* this should never happen */
		tn->head = get_ppnode(h);
		*tn->head = tn;
	}
	h->lhead->next = tn;
	tn->prev = h->lhead;
	tn->left = tn->right = nil;

	if(h->lhead->parent){
		if(h->lhead->parent->left == h->lhead)	/* lhead is guaranteed to be the NYT */
			h->lhead->parent->left = tn2;
		else
			h->lhead->parent->right = tn2;
	}else
		h->tree = tn2;

	tn2->right = tn;
	tn2->left = h->lhead;

	tn2->parent = h->lhead->parent;
	h->lhead->parent = tn->parent = tn2;

	h->loc[ch] = tn;

	increment(h, tn2->parent);
}

/* Get a symbol */
int
huffrecv(Node *node, int *ch, byte *fin)
{
	while(node && node->symbol == INTERNAL_NODE){
		if(get_bit(fin))
			node = node->right;
		else
			node = node->left;
	}
	if(!node){
		return 0;
		/* comerrorf(ERR_DROP, "Illegal tree!"); */
	}
	return (*ch = node->symbol);
}

/* Get a symbol */
void
huffoffsetrecv(Node *node, int *ch, byte *fin, int *offset)
{
	bloc = *offset;
	while(node && node->symbol == INTERNAL_NODE){
		if(get_bit(fin))
			node = node->right;
		else
			node = node->left;
	}
	if(!node){
		*ch = 0;
		return;
/*		comerrorf(ERR_DROP, "Illegal tree!"); */
	}
	*ch = node->symbol;
	*offset = bloc;
}

void
huffputbit(int bit, byte *fout, int *offset)
{
	bloc = *offset;
	if((bloc&7) == 0)
		fout[(bloc>>3)] = 0;
	fout[(bloc>>3)] |= bit << (bloc&7);
	bloc++;
	*offset = bloc;
}

int
huffgetbloc(void)
{
	return bloc;
}

void
huffsetbloc(int _bloc)
{
	bloc = _bloc;
}

int
huffgetbit(byte *fin, int *offset)
{
	int t;
	bloc = *offset;
	t = (fin[(bloc>>3)] >> (bloc&7)) & 0x1;
	bloc++;
	*offset = bloc;
	return t;
}

/* Send a symbol */
void
hufftransmit(Huff *huff, int ch, byte *fout)
{
	int i;
	if(huff->loc[ch] == nil){
		/* Node hasn't been transmitted, send a NYT, then the symbol */
		hufftransmit(huff, NYT, fout);
		for(i = 7; i >= 0; i--)
			add_bit((char)((ch >> i) & 0x1), fout);
	}else
		send(huff->loc[ch], nil, fout);
}

void
huffoffsettransmit(Huff *huff, int ch, byte *fout, int *offset)
{
	bloc = *offset;
	send(huff->loc[ch], nil, fout);
	*offset = bloc;
}

void
huffdecompress(Bitmsg *mbuf, int offset)
{
	int ch, cch, i, j, size;
	byte seq[65536], *buffer;
	Huff huff;

	size = mbuf->cursize - offset;
	buffer = mbuf->data + offset;

	if(size <= 0)
		return;

	Q_Memset(&huff, 0, sizeof huff);
	/* Initialize the tree & list with the NYT node */
	huff.tree = huff.lhead = huff.ltail = huff.loc[NYT] = &(huff.nodeList[huff.blocNode++]);
	huff.tree->symbol = NYT;
	huff.tree->weight = 0;
	huff.lhead->next = huff.lhead->prev = nil;
	huff.tree->parent = huff.tree->left = huff.tree->right = nil;

	cch = buffer[0]*256 + buffer[1];
	/* don't overflow with bad messages */
	if(cch > mbuf->maxsize - offset)
		cch = mbuf->maxsize - offset;
	bloc = 16;

	for(j = 0; j < cch; j++){
		ch = 0;
		/* 
		 * don't overflow reading from the messages
		 * FIXME: would it be better to have a overflow check in get_bit ? 
		 */
		if((bloc >> 3) > size){
			seq[j] = 0;
			break;
		}
		huffrecv(huff.tree, &ch, buffer);	/* Get a character */
		if(ch == NYT){				/* We got a NYT, get the symbol associated with it */
			ch = 0;
			for(i = 0; i < 8; i++)
				ch = (ch<<1) + get_bit(buffer);
		}

		seq[j] = ch;	/* Write symbol */

		huffaddref(&huff, (byte)ch);	/* Increment node */
	}
	mbuf->cursize = cch + offset;
	Q_Memcpy(mbuf->data + offset, seq, cch);
}

extern int oldsize;

void
huffcompress(Bitmsg *mbuf, int offset)
{
	int i, ch, size;
	byte seq[65536], *buffer;
	Huff huff;

	size = mbuf->cursize - offset;
	buffer = mbuf->data+ +offset;

	if(size<=0)
		return;

	Q_Memset(&huff, 0, sizeof(Huff));
	/* Add the NYT (not yet transmitted) node into the tree/list * / */
	huff.tree = huff.lhead = huff.loc[NYT] = &(huff.nodeList[huff.blocNode++]);
	huff.tree->symbol = NYT;
	huff.tree->weight = 0;
	huff.lhead->next = huff.lhead->prev = nil;
	huff.tree->parent = huff.tree->left = huff.tree->right = nil;
	huff.loc[NYT] = huff.tree;

	seq[0] = (size>>8);
	seq[1] = size&0xff;
	bloc = 16;

	for(i=0; i<size; i++){
		ch = buffer[i];
		hufftransmit(&huff, ch, seq);	/* Transmit symbol */
		huffaddref(&huff, (byte)ch);	/* Do update */
	}

	bloc += 8;	/* next byte */

	mbuf->cursize = (bloc>>3) + offset;
	Q_Memcpy(mbuf->data+offset, seq, (bloc>>3));
}

void
huffinit(Huffman *huff)
{
	Node *p;

	Q_Memset(&huff->compressor, 0, sizeof huff->compressor);
	Q_Memset(&huff->decompressor, 0, sizeof huff->decompressor);
	
	p = &(huff->decompressor.nodeList[huff->decompressor.blocNode++]);
	/* Initialize the tree & list with the NYT node */
	huff->decompressor.tree = p;
	huff->decompressor.lhead = p;
	huff->decompressor.ltail = p;
	huff->decompressor.loc[NYT] = p;
	huff->decompressor.tree->symbol = NYT;
	huff->decompressor.tree->weight = 0;
	huff->decompressor.lhead->prev = nil;
	huff->decompressor.lhead->next = nil;
	huff->decompressor.tree->parent = nil;
	huff->decompressor.tree->left = nil;
	huff->decompressor.tree->right = nil;
	
	p = &(huff->compressor.nodeList[huff->compressor.blocNode++]);
	/* Add the NYT (not yet transmitted) node into the tree/list * / */
	huff->compressor.tree = p;
	huff->compressor.lhead = p;
	huff->compressor.loc[NYT] = p;
	huff->compressor.tree->symbol = NYT;
	huff->compressor.tree->weight = 0;
	huff->compressor.lhead->prev = nil;
	huff->compressor.lhead->next = nil;
	huff->compressor.tree->parent = nil;
	huff->compressor.tree->left = nil;
	huff->compressor.tree->right = nil;
}
