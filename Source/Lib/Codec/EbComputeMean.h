/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeMean_h
#define EbComputeMean_h

#include "EbComputeMean_SSE2.h"
#include "EbComputeMean_C.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef EB_U64(*EB_COMPUTE_MEAN_FUNC)(
    EB_U8 *inputSamples,
    EB_U32 inputStride,
    EB_U32 inputAreaWidth,
    EB_U32 inputAreaHeight);

static const EB_COMPUTE_MEAN_FUNC ComputeMeanFunc[2][EB_ASM_TYPE_TOTAL] = {
    {
        // C_DEFAULT
        ComputeMean,
        // AVX2
		ComputeMean8x8_AVX2_INTRIN
    },
    {
        // C_DEFAULT
        ComputeMeanOfSquaredValues,
        // AVX2
        ComputeMeanOfSquaredValues8x8_SSE2_INTRIN
    }
};

#ifdef __cplusplus
}
#endif

#endif