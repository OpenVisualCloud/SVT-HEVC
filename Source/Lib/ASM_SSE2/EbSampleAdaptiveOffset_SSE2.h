/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSampleAdaptiveOffset_SSE2_h
#define EbSampleAdaptiveOffset_SSE2_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

EB_ERRORTYPE GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_INTRIN(
    EB_U16                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U16                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],    // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1]);   // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]

extern EB_ERRORTYPE SAOApplyBO16bit_SSE2_INTRIN(
	EB_U16                          *reconSamplePtr,
	EB_U32                           reconStride,
	EB_U32                           saoBandPosition,
	EB_S8                           *saoOffsetPtr,
	EB_U32                           lcuHeight,
	EB_U32                           lcuWidth);

extern EB_ERRORTYPE SAOApplyEO_45_16bit_SSE2_INTRIN(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                           *temporalBufferLeft,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

extern EB_ERRORTYPE SAOApplyEO_135_16bit_SSE2_INTRIN(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                          *temporalBufferLeft,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

extern EB_ERRORTYPE SAOApplyEO_90_16bit_SSE2_INTRIN(
    EB_U16                          *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                          *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

extern EB_ERRORTYPE SAOApplyEO_0_16bit_SSE2_INTRIN(
    EB_U16                          *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                          *temporalBufferLeft,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

#ifdef __cplusplus
}
#endif
#endif