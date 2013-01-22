/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "server.h"




/*
 * SV_Netchan_FreeQueue
 */
void
SV_Netchan_FreeQueue(Client *client)
{
	netchan_buffer_t *netbuf, *next;

	for(netbuf = client->netchan_start_queue; netbuf; netbuf = next){
		next = netbuf->next;
		zfree(netbuf);
	}

	client->netchan_start_queue	= NULL;
	client->netchan_end_queue	= &client->netchan_start_queue;
}

/*
 * SV_Netchan_TransmitNextInQueue
 */
void
SV_Netchan_TransmitNextInQueue(Client *client)
{
	netchan_buffer_t *netbuf;

	Com_DPrintf(
		"#462 Netchan_TransmitNextFragment: popping a queued message for transmit\n");
	netbuf = client->netchan_start_queue;


	Netchan_Transmit(&client->netchan, netbuf->msg.cursize, netbuf->msg.data);

	/* pop from queue */
	client->netchan_start_queue = netbuf->next;
	if(!client->netchan_start_queue){
		Com_DPrintf("#462 Netchan_TransmitNextFragment: emptied queue\n");
		client->netchan_end_queue = &client->netchan_start_queue;
	}else
		Com_DPrintf(
			"#462 Netchan_TransmitNextFragment: remaining queued message\n");

	zfree(netbuf);
}

/*
 * SV_Netchan_TransmitNextFragment
 * Transmit the next fragment and the next queued packet
 * Return number of ms until next message can be sent based on throughput given by client rate,
 * -1 if no packet was sent.
 */

int
SV_Netchan_TransmitNextFragment(Client *client)
{
	if(client->netchan.unsentFragments){
		Netchan_TransmitNextFragment(&client->netchan);
		return SV_RateMsec(client);
	}else if(client->netchan_start_queue){
		SV_Netchan_TransmitNextInQueue(client);
		return SV_RateMsec(client);
	}

	return -1;
}


/*
 * SV_Netchan_Transmit
 * TTimo
 * https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=462
 * if there are some unsent fragments (which may happen if the snapshots
 * and the gamestate are fragmenting, and collide on send for instance)
 * then buffer them and make sure they get sent in correct order
 */

void
SV_Netchan_Transmit(Client *client, Bitmsg *msg)
{
	MSG_WriteByte(msg, svc_EOF);

	if(client->netchan.unsentFragments || client->netchan_start_queue){
		netchan_buffer_t *netbuf;
		Com_DPrintf(
			"#462 SV_Netchan_Transmit: unsent fragments, stacked\n");
		netbuf = (netchan_buffer_t*)zalloc(sizeof(netchan_buffer_t));
		/* store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending */
		MSG_Copy(&netbuf->msg, netbuf->msgBuffer,
			sizeof(netbuf->msgBuffer), msg);
		netbuf->next = NULL;
		/* insert it in the queue, the message will be encoded and sent later */
		*client->netchan_end_queue	= netbuf;
		client->netchan_end_queue	=
			&(*client->netchan_end_queue)->next;
	}else{
		Netchan_Transmit(&client->netchan, msg->cursize, msg->data);
	}
}

/*
 * Netchan_SV_Process
 */
qbool
SV_Netchan_Process(Client *client, Bitmsg *msg)
{
	int ret;
	ret = Netchan_Process(&client->netchan, msg);
	if(!ret)
		return qfalse;


	return qtrue;
}
