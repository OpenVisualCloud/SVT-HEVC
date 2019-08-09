/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbDeblockingFilter_SSE2_h
#define EbDeblockingFilter_SSE2_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void Chroma2SampleEdgeDLFCore16bit_SSE2_INTRIN(
    EB_U16                  *edgeStartSampleCb,
    EB_U16                  *edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc);


#ifdef __cplusplus
}
#endif
#endif
