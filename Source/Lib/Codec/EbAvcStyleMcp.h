/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAVCSTYLEMCP_H
#define EBAVCSTYLEMCP_H

#include "EbAvcStyleMcp_C.h"

#include "EbAvcStyleMcp_SSE2.h"
#include "EbAvcStyleMcp_SSSE3.h"

#include "EbPictureOperators.h"

#include "EbMcp.h"

#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureControlSet.h"

#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX512.h"
#ifdef __cplusplus
extern "C" {
#endif


void UnpackUniPredRef10Bit(
    EbPictureBufferDesc_t *refFramePicList0, 
    EB_U32                 posX,
    EB_U32                 posY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *dst,
    EB_U32                 dstLumaIndex, 
    EB_U32                 dstChromaIndex,          //input parameter, please refer to the detailed explanation above.
    EB_U32                 componentMask,
    EB_BYTE                tempBuf);

void UnpackBiPredRef10Bit(
    EbPictureBufferDesc_t *refFramePicList0,
    EbPictureBufferDesc_t *refFramePicList1,
    EB_U32                 refList0PosX,
    EB_U32                 refList0PosY,
    EB_U32                 refList1PosX,
    EB_U32                 refList1PosY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *biDst,
    EB_U32                 dstLumaIndex, 
	EB_U32                 dstChromaIndex,
	EB_U32                 componentMask,
    EB_BYTE                refList0TempDst,
    EB_BYTE                refList1TempDst,
    EB_BYTE                firstPassIFTempDst);

void UniPredIFreeRef8Bit(
    EbPictureBufferDesc_t *refPic,
    EB_U32                 posX,
    EB_U32                 posY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *dst,
    EB_U32                 dstLumaIndex, 
    EB_U32                 dstChromaIndex,          //input parameter, please refer to the detailed explanation above.
    EB_U32                 componentMask,
    EB_BYTE                tempBuf);
void BiPredIFreeRef8Bit(
    EbPictureBufferDesc_t *refPicList0,
    EbPictureBufferDesc_t *refPicList1,
    EB_U32                 refList0PosX,
    EB_U32                 refList0PosY,
    EB_U32                 refList1PosX,
    EB_U32                 refList1PosY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *biDst,
    EB_U32                 dstLumaIndex, 
	EB_U32                 dstChromaIndex,
	EB_U32                 componentMask,
    EB_BYTE                refList0TempDst,
    EB_BYTE                refList1TempDst,
    EB_BYTE                firstPassIFTempDst);


typedef void(*AvcStyleInterpolationFilterNew)(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos);

typedef void(*PictureAverage)(
    EB_BYTE                  src0,
    EB_U32                   src0Stride,
    EB_BYTE                  src1,
    EB_U32                   src1Stride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);


/***************************************
* Function Tables
***************************************/
static const AvcStyleInterpolationFilterNew FUNC_TABLE AvcStyleUniPredLumaIFFunctionPtrArray[EB_ASM_TYPE_TOTAL][3] = {
    // C_DEFAULT
    {
        AvcStyleCopyNew,                                     //copy
        AvcStyleLumaInterpolationFilterHorizontal,           //a
        AvcStyleLumaInterpolationFilterVertical,             //d
    },
    // AVX2
    {
        AvcStyleCopy_SSE2,                                          //copy
        AvcStyleLumaInterpolationFilterHorizontal_SSSE3_INTRIN,     //a
        AvcStyleLumaInterpolationFilterVertical_SSSE3_INTRIN,       //d

    }
};



static const PictureAverage FUNC_TABLE PictureAverageArray[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
    PictureAverageKernel,
	// AVX2
	PictureAverageKernel_SSE2_INTRIN,
};

typedef void(*PictureAverage1Line)(
    EB_BYTE                  src0,   
    EB_BYTE                  src1,   
    EB_BYTE                  dst,  
    EB_U32                   areaWidth);  

static const PictureAverage1Line FUNC_TABLE PictureAverage1LineArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    PictureAverageKernel1Line_C,
    // AVX2
    PictureAverageKernel1Line_SSE2_INTRIN,
};

#ifdef __cplusplus
}
#endif
#endif //EBAVCSTYLEMCP_H
