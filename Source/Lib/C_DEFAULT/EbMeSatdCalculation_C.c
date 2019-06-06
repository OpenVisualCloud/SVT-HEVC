/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbMeSatdCalculation_C.h"

#define BITS_PER_SUM (8 * sizeof(EB_U16))

static inline EB_U32 abs2(EB_U32 a)
{
	EB_U32 s = ((a >> (BITS_PER_SUM - 1)) & (((EB_U32)1 << BITS_PER_SUM) + 1)) * ((EB_U32)-1);

	return (a + s) ^ s;
}

#define HADAMARD4(d0, d1, d2, d3, s0, s1, s2, s3) { \
        EB_U32 t0 = s0 + s1; \
        EB_U32 t1 = s0 - s1; \
        EB_U32 t2 = s2 + s3; \
        EB_U32 t3 = s2 - s3; \
        d0 = t0 + t2; \
        d2 = t0 - t2; \
        d1 = t1 + t3; \
        d3 = t1 - t3; \
}

/*******************************************
Calcualte SATD for 8x4 sublcoks.
*******************************************/
EB_U32 SatdCalculation_8x4(
	EB_U8   *src,
	EB_U32   srcStride,
	EB_U8   *ref,
	EB_U32   refStride)
{
	EB_U32 tmp[4][4];
	EB_U32 a0, a1, a2, a3;
	EB_U32 sum = 0;

	for (EB_U64 i = 0; i < 4; i++, src += srcStride, ref += refStride)
	{
		a0 = (src[0] - ref[0]) + ((EB_U32)(src[4] - ref[4]) << BITS_PER_SUM);
		a1 = (src[1] - ref[1]) + ((EB_U32)(src[5] - ref[5]) << BITS_PER_SUM);
		a2 = (src[2] - ref[2]) + ((EB_U32)(src[6] - ref[6]) << BITS_PER_SUM);
		a3 = (src[3] - ref[3]) + ((EB_U32)(src[7] - ref[7]) << BITS_PER_SUM);
		HADAMARD4(tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0, a1, a2, a3);
	}

	for (EB_U64 i = 0; i < 4; i++)
	{
		HADAMARD4(a0, a1, a2, a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i]);
		sum += abs2(a0) + abs2(a1) + abs2(a2) + abs2(a3);
	}

	return (((EB_U16)sum) + (sum >> BITS_PER_SUM)) >> 1;
}

/*******************************************
Calcualte SATD for 16x16 sublcoks.
*******************************************/
EB_U32 SatdCalculation_16x16(
	EB_U8   *src,
	EB_U32   srcStride,
	EB_U8   *ref,
	EB_U32   refStride)
{
	EB_U32 satd = 0;

	for (EB_U64 row = 0; row < 16; row += 4)
	{
		for (EB_U64 col = 0; col < 16; col += 8)
		{
			satd += SatdCalculation_8x4(src + row * srcStride + col, srcStride,
				ref + row * refStride + col, refStride);
		}
	}

	return satd;
}
