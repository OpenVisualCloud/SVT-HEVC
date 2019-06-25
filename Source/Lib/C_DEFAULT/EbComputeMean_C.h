/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeMean_C_h
#define EbComputeMean_C_h
#ifdef __cplusplus
extern "C" {
#endif

#include "EbDefinitions.h"

EB_U64 ComputeMean(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight);   // input parameter, input area height

EB_U64 ComputeMeanOfSquaredValues(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight);   // input parameter, input area height

#ifdef __cplusplus
}
#endif

#endif
