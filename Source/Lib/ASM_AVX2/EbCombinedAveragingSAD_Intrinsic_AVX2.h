/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbCombinedAveragingSAD_AVX2_h
#define EbCombinedAveragingSAD_AVX2_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

 EB_U64 ComputeMean8x8_AVX2_INTRIN(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight);

  void ComputeIntermVarFour8x8_AVX2_INTRIN(
        EB_U8 *  inputSamples,
        EB_U16   inputStride,
        EB_U64 * meanOf8x8Blocks,      // mean of four  8x8
        EB_U64 * meanOfSquared8x8Blocks);

#ifdef __cplusplus
}
#endif
#endif
