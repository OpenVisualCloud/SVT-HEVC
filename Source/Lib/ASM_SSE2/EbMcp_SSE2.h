/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBMCP_SSE2_H
#define EBMCP_SSE2_H

#include "EbTypes.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
#define USE_PRE_COMPUTE             0
extern EB_ALIGN(16) const EB_S16 IntraPredictionConst_SSE2[344];
/**************************************************
* Assembly Declarations
**************************************************/
extern void PictureCopyKernel_SSE2(EB_BYTE src, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 areaWidth, EB_U32 areaHeight); 
void PictureAverageKernel_SSE2(EB_BYTE src0, EB_U32 src0Stride, EB_BYTE src1, EB_U32 src1Stride, EB_BYTE dst, EB_U32 dstStride, EB_U32 areaWidth, EB_U32 areaHeight);
void PictureAverageKernel_SSE2_INTRIN(EB_BYTE src0, EB_U32 src0Stride, EB_BYTE src1, EB_U32 src1Stride, EB_BYTE dst, EB_U32 dstStride, EB_U32 areaWidth, EB_U32 areaHeight);
void PictureAverageKernel1Line_SSE2_INTRIN(
	EB_BYTE                  src0,
	EB_BYTE                  src1,
	EB_BYTE                  dst,
	EB_U32                   areaWidth);


extern void BiPredClipping16bit_SSE2_INTRIN(EB_U32 puWidth, EB_U32 puHeight, EB_S16 *list0Src, EB_S16 *list1Src, EB_U16 *dst, EB_U32 dstStride);

void BiPredClippingOnTheFly16bit_SSE2(
	EB_U16    *list0Src,
	EB_U32     list0SrcStride,
	EB_U16    *list1Src,
	EB_U32     list1SrcStride,
	EB_U16    *dst,
	EB_U32     dstStride,
	EB_U32     puWidth,
	EB_U32     puHeight);

// 16Bit
extern void LumaInterpolationCopy16bit_SSE2(EB_U16* refPic, EB_U32 srcStride, EB_U16* dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);

void LumaInterpolationCopyOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosa16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosb16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosc16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosd16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPose16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosf16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosg16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosh16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosi16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosj16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosk16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosn16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosp16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosq16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosr16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(EB_S16 *firstPassIFDst, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 coeffIndex);
void LumaInterpolationFilterPoshInRaw16bit_SSE2_INTRIN(EB_S16 *firstPassIFDst, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight);
void LumaInterpolationFilterPosnInRaw16bit_SSE2_INTRIN(EB_S16 *firstPassIFDst, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight);
void LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoshOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosnOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoseOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosfOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosgOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosiOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosjOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoskOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPospOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosqOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosrOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 coeffIndex);
void LumaInterpolationFilterPoshInRawOutRaw_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight);

extern void ChromaInterpolationCopyOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterOneDOutRaw16bitHorizontal_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterTwoD16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterTwoDOutRaw16bit_SSE2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterTwoDInRaw16bit_SSE2_INTRIN(EB_S16 *firstPassIFDst, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy);

extern void ChromaInterpolationCopy16bit_SSE2(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
extern void ChromaInterpolationFilterTwoDInRawOutRaw_SSE2_INTRIN(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy);


#ifdef __cplusplus
}
#endif
#endif //EBMCP_SSE2_H
