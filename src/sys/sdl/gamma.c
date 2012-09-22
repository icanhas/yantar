/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifdef USE_LOCAL_HEADERS
#       include "SDL.h"
#else
#       include <SDL.h>
#endif

#include "../../renderer/local.h"
#include "qcommon.h"

/*
 * GLimp_SetGamma
 */
void
GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
	Uint16	table[3][256];
	int	i, j;

	if(!glConfig.deviceSupportsGamma || r_ignorehwgamma->integer > 0)
		return;

	for(i = 0; i < 256; i++){
		table[0][i]	= ((( Uint16 )red[i]) << 8) | red[i];
		table[1][i]	= ((( Uint16 )green[i]) << 8) | green[i];
		table[2][i]	= ((( Uint16 )blue[i]) << 8) | blue[i];
	}

#ifdef _WIN32
#include <windows.h>

	/* Win2K and newer put this odd restriction on gamma ramps... */
	{
		OSVERSIONINFO vinfo;

		vinfo.dwOSVersionInfoSize = sizeof(vinfo);
		GetVersionEx(&vinfo);
		if(vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT){
			ri.Printf(PRINT_DEVELOPER, "performing gamma clamp.\n");
			for(j = 0; j < 3; j++){
				for(i = 0; i < 128; i++)
					if(table[ j ] [ i] > ((128 + i) << 8))
						table[ j ][ i ] = (128 + i) << 8;

				if(table[ j ] [127 ] > 254 << 8)
					table[ j ][ 127 ] = 254 << 8;
			}
		}
	}
#endif

	/* enforce constantly increasing */
	for(j = 0; j < 3; j++)
		for(i = 1; i < 256; i++)
			if(table[j][i] < table[j][i-1])
				table[j][i] = table[j][i-1];

	SDL_SetGammaRamp(table[0], table[1], table[2]);
}
