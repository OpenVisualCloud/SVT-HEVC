/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMcp_C_h
#define EbMcp_C_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif


void LumaInterpolationCopy(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosaNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosbNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoscNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoseNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosfNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosgNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoshNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosiNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosjNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoskNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosnNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPospNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosqNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosrNew(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);

void LumaInterpolationCopyOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosaOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosbOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoscOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoseOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosfOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosgOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoshOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosiOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosjOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoskOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosnOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPospOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosqOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosrOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);

void LumaInterpolationFilterPosaOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosbOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoscOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoshOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosnOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoseOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosfOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosgOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosiOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosjOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoskOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPospOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosqOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosrOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);


void LumaInterpolationCopyOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationCopy16bit(EB_U16* refPic, EB_U32 srcStride, EB_U16* dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosaNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosbNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoscNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoseNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosfNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosgNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoshNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosiNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosjNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoskNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosnNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPospNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosqNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosrNew16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);

void ChromaInterpolationCopy16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationCopyOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationCopyOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneD16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneDOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoD16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDInRaw16bit(EB_S16 *firstPassIFDst, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDInRawOutRaw16bit(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDOutRaw16bit(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);

void ChromaInterpolationCopy(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneD(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoD(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneDOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);

void BiPredClipping(EB_U32 puWidth, EB_U32 puHeight, EB_S16 *list0Src, EB_S16 *list1Src, EB_BYTE dst, EB_U32 dstStride, EB_S32 offset);
void BiPredClipping16bit(EB_U32 puWidth, EB_U32 puHeight, EB_S16 *list0Src, EB_S16 *list1Src, EB_U16 *dst, EB_U32 dstStride);
void BiPredClippingOnTheFly(EB_BYTE list0Src, EB_U32 list0SrcStride, EB_BYTE list1Src, EB_U32 list1SrcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S32 offset, EB_BOOL isLuma);

void WpLumaInterpolationCopy(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosa(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosb(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosc(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosd(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPose(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosf(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosg(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosh(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosi(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosj(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosk(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosn(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosp(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosq(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosr(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);


void WpChromaInterpolationCopy(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpChromaInterpolationFilterOneD(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpChromaInterpolationFilterTwoD(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);

void WpChromaInterpolationFilterOneDOutRaw(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);

void WpLumaInterpolationCopy16bit(EB_U16* refPic, EB_U32 srcStride, EB_U16* dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosa16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosb16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosc16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosd16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPose16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosf16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosg16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosh16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosi16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosj16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosk16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosn16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosp16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosq16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpLumaInterpolationFilterPosr16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);

void WpChromaInterpolationCopy16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpChromaInterpolationFilterOneD16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);
void WpChromaInterpolationFilterTwoD16bit(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy, EB_S16 wpWeight, EB_S16 wpOffset, EB_U8 wpDenominator);

void WpBiPredClipping(EB_U32 puWidth, EB_U32 puHeight, EB_S16 *list0Src, EB_S16 *list1Src, EB_BYTE dst, EB_U32 dstStride, EB_S32 offset, EB_S16 wpWeightList0, EB_S16 wpWeightList1, EB_U8 wpDenominator, EB_S16 wpOffset);
void WpBiPredClipping16bit(EB_U32 puWidth, EB_U32 puHeight, EB_S16 *list0Src, EB_S16 *list1Src, EB_U16 *dst, EB_U32 dstStride, EB_S16 wpWeightList0, EB_S16 wpWeightList1, EB_U8 wpDenominator, EB_S16 wpOffset);


void uniPredCopy16bit(
    EB_U16*               refPic,
    EB_U32                srcStride,            
    EB_U16*               dst,                 
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                *firstPassIFDst);



#ifdef __cplusplus
}
#endif        
#endif // EbMcp_C_h