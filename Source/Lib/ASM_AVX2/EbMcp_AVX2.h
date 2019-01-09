/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#ifndef EBMCP_AVX2_H
#define EBMCP_AVX2_H

#include "EbDefinitions.h"

#define USE_PRE_COMPUTE             0

// extern EB_ALIGN(16) const EB_S16 IntraPredictionConst_AVX2[344];

void ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);

void BiPredClippingOnTheFly_AVX2(
	EB_BYTE    list0Src,
	EB_U32     list0SrcStride,
	EB_BYTE    list1Src,
	EB_U32     list1SrcStride,
	EB_BYTE    dst,
	EB_U32     dstStride,
	EB_U32     puWidth,
	EB_U32     puHeight,
	EB_S32     offset,
	EB_BOOL	   isLuma);
#endif // EBMCP_AVX2_H
