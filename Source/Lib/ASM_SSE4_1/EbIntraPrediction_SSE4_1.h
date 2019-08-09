/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_SSE4_1_h
#define EbIntraPrediction_SSE4_1_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void IntraModeDCLuma16bit_SSE4_1_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

#ifdef __cplusplus
}
#endif
#endif // EbIntraPrediction_SSE4_1_h
