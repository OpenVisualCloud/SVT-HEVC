/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBMCP_SSSE3_H
#define EBMCP_SSSE3_H

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VNNI_SUPPORT
#define LumaInterpolationFilterOneDOutRawHorizontal LumaInterpolationFilterOneDOutRawHorizontal_SSSE3
#define EbHevcLumaInterpolationFilterTwoDInRaw7 EbHevcLumaInterpolationFilterTwoDInRaw7_VNNI
#define EbHevcLumaInterpolationFilterTwoDInRawOutRaw7 EbHevcLumaInterpolationFilterTwoDInRawOutRaw7_VNNI
#define EbHevcLumaInterpolationFilterTwoDInRawM EbHevcLumaInterpolationFilterTwoDInRawM_VNNI
#else
#define EbHevcLumaInterpolationFilterTwoDInRaw7 EbHevcLumaInterpolationFilterTwoDInRaw7_SSSE3
#define LumaInterpolationFilterOneDOutRawHorizontal LumaInterpolationFilterOneDOutRawHorizontal_SSSE3
#define EbHevcLumaInterpolationFilterTwoDInRawOutRaw7 EbHevcLumaInterpolationFilterTwoDInRawOutRaw7_SSSE3
#define EbHevcLumaInterpolationFilterTwoDInRawM EbHevcLumaInterpolationFilterTwoDInRawM_SSSE3
#endif

// SSSE3 functions
void ChromaInterpolationCopy_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneDHorizontal_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneDVertical_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterOneDOutRawVertical_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoD_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationCopyOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void LumaInterpolationCopy_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosa_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosb_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosc_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosd_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPose_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosf_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosg_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosh_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosi_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosj_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosk_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosn_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosp_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosq_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosr_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);

void LumaInterpolationCopyOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosaOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosbOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoscOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosdOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoseOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosfOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosgOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoshOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosiOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosjOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPoskOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosnOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPospOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosqOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void LumaInterpolationFilterPosrOutRaw_SSSE3(EB_BYTE refPic, EB_U32 srcStride, EB_S16* dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst);
void BiPredClipping_SSSE3(EB_U32 puWidth, EB_U32 puHeight, EB_S16 *list0Src, EB_S16 *list1Src, EB_BYTE dst, EB_U32 dstStride, EB_S32 offset);

void BiPredClippingOnTheFly_SSSE3(
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

#ifdef __cplusplus
}
#endif
#endif //EBMCP_SSSE3_H
