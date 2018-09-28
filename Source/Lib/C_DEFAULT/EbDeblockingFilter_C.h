/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbDeblockingFilter_C_h
#define EbDeblockingFilter_C_h

#include "EbTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CHROMA_SAMPLE_VALUE_10BIT   1023
#define MAX_CHROMA_SAMPLE_VALUE   255

void Luma4SampleEdgeDLFCore(
    EB_BYTE                edgeStartSample,
    EB_U32                 reconLumaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_S32                 tc,
    EB_S32                 beta);

void Luma4SampleEdgeDLFCore16bit(
    EB_U16         *edgeStartFilteredSamplePtr,
    EB_U32          reconLumaPicStride,
    EB_BOOL         isVerticalEdge,
    EB_S32          tc,
    EB_S32          beta);

void Chroma2SampleEdgeDLFCore(
    EB_BYTE                edgeStartSampleCb,
    EB_BYTE                edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc);

void Chroma2SampleEdgeDLFCore16bit(
    EB_U16				  *edgeStartSampleCb,
    EB_U16				  *edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc);


#ifdef __cplusplus
}
#endif
#endif