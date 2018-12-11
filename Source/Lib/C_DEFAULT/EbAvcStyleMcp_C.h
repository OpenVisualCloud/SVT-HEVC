/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAVCSTYLEMCP_C_H
#define EBAVCSTYLEMCP_C_H

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void AvcStyleCopyNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos);
void AvcStyleLumaInterpolationFilterHorizontal(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos);
void AvcStyleLumaInterpolationFilterVertical(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos);

void WpAvcStyleCopy(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterHorizontal(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterVertical(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPose(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosf(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosg(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosi(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosj(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosk(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosp(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosq(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpAvcStyleLumaInterpolationFilterPosr(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf, EB_U32 fracPos, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);



#ifdef __cplusplus
}
#endif
#endif //EBAVCSTYLEMCP_C_H