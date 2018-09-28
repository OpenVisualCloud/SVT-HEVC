/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeMean_SS2_h
#define EbComputeMean_SS2_h
#ifdef __cplusplus
extern "C" {
#endif

#include "EbTypes.h"

EB_U64 ComputeSubMean8x8_SSE2_INTRIN(
	EB_U8 *  inputSamples,      // input parameter, input samples Ptr
	EB_U16   inputStride);
EB_U64 ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(
	EB_U8 *  inputSamples,      // input parameter, input samples Ptr
	EB_U16   inputStride);

EB_U64 ComputeMeanOfSquaredValues8x8_SSE2_INTRIN(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight);   // input parameter, input area height


#ifdef __cplusplus
}
#endif

#endif