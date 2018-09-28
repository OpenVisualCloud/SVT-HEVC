/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSampleAdaptiveOffset_SSSE3_h
#define EbSampleAdaptiveOffset_SSSE3_h

#include "EbTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

EB_ERRORTYPE SAOApplyBO_BT_SSSE3(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U32                    saoBandPosition,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

EB_ERRORTYPE SAOApplyEO_0_BT_SSSE3(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U8                    *temporalBufferLeft,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

EB_ERRORTYPE SAOApplyEO_90_BT_SSSE3(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U8                    *temporalBufferUpper,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

EB_ERRORTYPE SAOApplyEO_135_BT_SSSE3(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U8                    *temporalBufferLeft,
    EB_U8                    *temporalBufferUpper,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

EB_ERRORTYPE SAOApplyEO_45_BT_SSSE3(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U8                    *temporalBufferLeft,
    EB_U8                    *temporalBufferUpper,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

#ifdef __cplusplus
}
#endif
#endif