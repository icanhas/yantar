/*
 * Copyright (C) 2011 Andrei Drexler, Richard Allen, James Canete
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#ifndef TR_POSTPROCESS_H
#define TR_POSTPROCESS_H

void RB_ToneMap(int autoExposure);
void RB_BokehBlur(float blur);
void RB_GodRays(void);
void RB_GaussianBlur(float blur);

#endif
