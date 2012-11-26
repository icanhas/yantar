/*
 * Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "qasm-inline.h"
#include "shared.h"

/*
 * GNU inline asm version of qsnapvector
 * See MASM snapvector.asm for commentary
 */

static unsigned char ssemask[16] __attribute__((aligned(16))) =
{
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00"
};

void
qsnapv3sse(Vec3 vec)
{
	__asm__ volatile
	(
		"movaps (%0), %%xmm1\n"
		"movups (%1), %%xmm0\n"
		"movaps %%xmm0, %%xmm2\n"
		"andps %%xmm1, %%xmm0\n"
		"andnps %%xmm2, %%xmm1\n"
		"cvtps2dq %%xmm0, %%xmm0\n"
		"cvtdq2ps %%xmm0, %%xmm0\n"
		"orps %%xmm1, %%xmm0\n"
		"movups %%xmm0, (%1)\n"
		:
		: "r" (ssemask), "r" (vec)
		: "memory", "%xmm0", "%xmm1", "%xmm2"
	);

}

#define QROUNDX87(src) \
	"flds " src "\n" \
		    "fistps " src "\n" \
				  "filds " src "\n" \
					       "fstps " src "\n"

void
qsnapv3x87(Vec3 vec)
{
	__asm__ volatile
	(
		QROUNDX87("(%0)")
		QROUNDX87("4(%0)")
		QROUNDX87("8(%0)")
			:
			: "r" (vec)
			: "memory"
	);
}
