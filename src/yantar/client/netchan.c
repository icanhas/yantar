/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "shared.h"
#include "common.h"
#include "client.h"


/*
 * CL_Netchan_TransmitNextFragment
 */
qbool
CL_Netchan_TransmitNextFragment(netchan_t *chan)
{
	if(chan->unsentFragments){
		Netchan_TransmitNextFragment(chan);
		return qtrue;
	}

	return qfalse;
}

/*
 * CL_Netchan_Transmit
 */
void
CL_Netchan_Transmit(netchan_t *chan, msg_t* msg)
{
	MSG_WriteByte(msg, clc_EOF);


	Netchan_Transmit(chan, msg->cursize, msg->data);

	/* Transmit all fragments without delay */
	while(CL_Netchan_TransmitNextFragment(chan))
		Com_DPrintf(
			"WARNING: #462 unsent fragments (not supposed to happen!)\n");
}

/*
 * CL_Netchan_Process
 */
qbool
CL_Netchan_Process(netchan_t *chan, msg_t *msg)
{
	int ret;

	ret = Netchan_Process(chan, msg);
	if(!ret)
		return qfalse;


	return qtrue;
}
