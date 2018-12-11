/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_SSSE3_h
#define EbIntraPrediction_SSSE3_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void IntraModeDCChroma16bit_SSSE3_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);

#ifdef __cplusplus
}
#endif
#endif // EbIntraPrediction_SSSE3_h