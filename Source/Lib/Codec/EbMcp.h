/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBMCP_H
#define EBMCP_H

#include "EbMcp_SSE2.h"
#include "EbMcp_SSSE3.h"
#include "EbMcp_AVX2.h"
#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbObject.h"

#ifdef __cplusplus
extern "C" {
#endif
#define USE_PRE_COMPUTE             0


typedef struct MotionCompensationPredictionContext_s
{
    EbDctor dctor;
    EB_S16 *motionCompensationIntermediateResultBuf0;            //this 64x64(Y)+32x32(U)+32x32(V) buffer is used to store the MCP intermediate result for ref0.
    EB_S16 *motionCompensationIntermediateResultBuf1;            //this 64x64(Y)+32x32(U)+32x32(V) buffer is used to store the MCP intermediate result for ref1.

	EB_BYTE avcStyleMcpIntermediateResultBuf0;                    // For short filter in MD
    EB_BYTE avcStyleMcpIntermediateResultBuf1;                    // For short filter in MD

#if !USE_PRE_COMPUTE
    EB_S16 *TwoDInterpolationFirstPassFilterResultBuf;           //this (64+MaxLumaFliterTag-1)x(64+MaxLumaFliterTag-1) buffer is used to store the result of 1st pass filtering of 2D interpolation filter.
    EB_BYTE avcStyleMcpTwoDInterpolationFirstPassFilterResultBuf; // For short filter in MD
#endif

    EbPictureBufferDesc_t *localReferenceBlockL0;                //used to pre-load reference L0 full pel block in local memory in 16bit mode
    EbPictureBufferDesc_t *localReferenceBlockL1;                //used to pre-load reference L1 full pel block in local memory in 16bit mode
    EbPictureBufferDesc_t *localReferenceBlock8BITL0;                //used to pre-load reference L0 full pel block in local memory in 16bit mode
    EbPictureBufferDesc_t *localReferenceBlock8BITL1;                //used to pre-load reference L1 full pel block in local memory in 16bit mode

}MotionCompensationPredictionContext_t;

/** InterpolationFilter()
        is generally defined interpolation filter function.
        There is a whole group of these functions, each of which corresponds to a particular
        integer/fractional sample, and the function is indexed in a function pointer array
        in terms of the fracPosx and fracPosy.

    @param *refPic (8-bits input)
        refPic is the pointer to the reference picture data that was chosen by
        the integer pixel precision MV.
    @param srcStride (input)
    @param fracPosx (input)
        fracPosx is the horizontal fractional position of the predicted sample
    @param fracPosy (input)
        fracPosy is the veritcal fractional position of the predicted sample
    @param puWidth (input)
    @param puHeight (input)
    @param *dst (16-bits output)
        dst is the pointer to the destination where the prediction result will
        be stored.
    @param dstStride (input)
    @param *firstPassIFDst (16-bits input)
        firstPassIFDst is the pointer to the buffer where the result of the first
        pass filtering of the 2D interpolation filter will be stored.
    @param isLast (input)
        isLast indicates if there is any further filtering (interpolation filtering)
		afterwards.
 */
typedef void (*InterpolationFilter)(
    EB_BYTE               refPic,               //8-bits input parameter, please refer to the detailed explanation above.
    EB_U32                srcStride,            //input parameter
    EB_U32                fracPosx,             //input parameter, please refer to the detailed explanation above.
    EB_U32                fracPosy,             //input parameter, please refer to the detailed explanation above.
    EB_U32                puWidth,              //input parameter
    EB_U32                puHeight,             //input parameter
    EB_S16               *dst,                  //output parameter, please refer to the detailed explanation above.
    EB_U32                dstStride,            //input parameter
    EB_S16               *firstPassIFDst,       //input parameter, please refer to the detailed explanation above.
    EB_BOOL               isLast);              //input parameter, please refer to the detailed explanation above.

typedef void (*InterpolationFilterNew)(
    EB_BYTE               refPic,               //8-bits input parameter, please refer to the detailed explanation above.
    EB_U32                srcStride,            //input parameter
    EB_BYTE               dst,                  //output parameter, please refer to the detailed explanation above.
    EB_U32                dstStride,            //input parameter
    EB_U32                puWidth,              //input parameter
    EB_U32                puHeight,             //input parameter
    EB_S16               *firstPassIFDst);      //input parameter, please refer to the detailed explanation above.

typedef void (*InterpolationFilterNew16bit)(
    EB_U16*               refPic,               //input parameter
    EB_U32                srcStride,            //input parameter
    EB_U16*               dst,                  //output parameter, please refer to the detailed explanation above.
    EB_U32                dstStride,            //input parameter
    EB_U32                puWidth,              //input parameter
    EB_U32                puHeight,             //input parameter
    EB_S16               *firstPassIFDst);      //input parameter, please refer to the detailed explanation above.

typedef void (*InterpolationFilterChromaNew16bit)(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

typedef void (*ChromaFilterOutRaw16bit)(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

typedef void (*InterpolationFilterOutRaw)(
    EB_BYTE               refPic,               //8-bits input parameter, please refer to the detailed explanation above.
    EB_U32                srcStride,            //input parameter
    EB_S16               *dst,                  //output parameter, please refer to the detailed explanation above.
    EB_U32                puWidth,              //input parameter
    EB_U32                puHeight,             //input parameter
    EB_S16               *firstPassIFDst);      //input parameter, please refer to the detailed explanation above.

typedef void (*InterpolationFilterOutRaw16bit)(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst);

typedef void (*ChromaFilterNew)(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

typedef void (*ChromaFilterOutRaw)(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

typedef void (*sampleBiPredClipping)(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_BYTE    dst,
    EB_U32     dstStride,
    EB_S32     offset);

typedef void (*sampleBiPredClipping16bit)(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_U16	  *dst,
    EB_U32     dstStride);

typedef void (*lumaSampleUniPredClipping)(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *src,
    EB_U32     srcStride,
    EB_BYTE    dst,
    EB_U32     dstStride);

typedef void (*lumaSampleBiPredClipping)(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_U32     list0SrcStride,
    EB_S16    *list1Src,
    EB_U32     list1SrcStride,
    EB_BYTE    dst,
    EB_U32     dstStride);

typedef void (*chromaSampleUniPredClipping)(
    EB_U32     chromaPuWidth,
    EB_U32     chromaPuHeight,
    EB_S16    *cbSrc,
    EB_U32     cbSrcStride,
    EB_S16    *crSrc,
    EB_U32     crSrcStride,
    EB_BYTE    cbDst,
    EB_U32     cbDstStride,
    EB_BYTE    crDst,
    EB_U32     crDstStride);

typedef void (*chromaSampleBiPredClipping)(
    EB_U32     chromaPuWidth,
    EB_U32     chromaPuHeight,
    EB_S16    *list0CbSrc,
    EB_U32     list0CbSrcStride,
    EB_S16    *list0CrSrc,
    EB_U32     list0CrSrcStride,
    EB_S16    *list1CbSrc,
    EB_U32     list1CbSrcStride,
    EB_S16    *list1CrSrc,
    EB_U32     list1CrSrcStride,
    EB_BYTE    cbDst,
    EB_U32     cbDstStride,
    EB_BYTE    crDst,
    EB_U32     crDstStride);

typedef void (*AvcStyleInterpolationFilter)(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

typedef void (*lumaSampleBiPredAverage)(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_BYTE    list0Src,
    EB_U32     list0SrcDoubleStride,
    EB_BYTE    list1Src,
    EB_U32     list1SrcDoubleStride,
    EB_BYTE    dst,
    EB_U32     dstDoubleStride);

typedef void (*chromaSampleBiPredAverage)(
    EB_U32     chromaPuWidth,
    EB_U32     chromaPuHeight,
    EB_BYTE    list0CbSrc,
    EB_U32     list0CbSrcDoubleStride,
    EB_BYTE    list0CrSrc,
    EB_U32     list0CrSrcDoubleStride,
    EB_BYTE    list1CbSrc,
    EB_U32     list1CbSrcDoubleStride,
    EB_BYTE    list1CrSrc,
    EB_U32     list1CrSrcDoubleStride,
    EB_BYTE    cbDst,
    EB_U32     cbDstDoubleStride,
    EB_BYTE    crDst,
    EB_U32     crDstDoubleStride);

extern EB_ERRORTYPE MotionCompensationPredictionContextCtor(
	MotionCompensationPredictionContext_t  *contextPtr,
	EB_U16                                  maxCUWidth,
    EB_U16                                  maxCUHeight,
    EB_BOOL                                 is16bit);

extern void UniPredHevcInterpolationMd(
	EbPictureBufferDesc_t *refPic,
	EB_U32                 posX,
	EB_U32                 posY,
	EB_U32                 puWidth,
	EB_U32                 puHeight,
	EbPictureBufferDesc_t *dst,
	EB_U32                 dstLumaIndex,
	EB_U32                 dstChromaIndex,
	EB_S16                *tempBuf0,
	EB_S16                *tempBuf1,
	EB_BOOL				   is16bit,
	EB_U32				   componentMask);

extern void EncodeUniPredInterpolation(
    EbPictureBufferDesc_t *refPic,
    EB_U32                 posX,
    EB_U32                 posY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *dst,
    EB_U32                 dstLumaIndex,
    EB_U32                 dstChromaIndex,
    EB_S16                *tempBuf0,
    EB_S16                *tempBuf1);

void UniPredInterpolation16bit(
    EbPictureBufferDesc_t *fullPelBlock,
    EbPictureBufferDesc_t *refPic,                  //input parameter, please refer to the detailed explanation above.
    EB_U32                 posX,                    //input parameter, please refer to the detailed explanation above.
    EB_U32                 posY,                    //input parameter, please refer to the detailed explanation above.
    EB_U32                 puWidth,                 //input parameter
    EB_U32                 puHeight,                //input parameter
    EbPictureBufferDesc_t *dst,                     //output parameter, please refer to the detailed explanation above.
    EB_U32                 dstLumaIndex,            //input parameter, please refer to the detailed explanation above.
    EB_U32                 dstChromaIndex,          //input parameter, please refer to the detailed explanation above.
    EB_S16                *tempBuf0);                //input parameter, please refer to the detailed explanation above.

extern void BiPredHevcInterpolationMd(
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
	EB_S16                *refList0TempDst,
	EB_S16                *refList1TempDst,
	EB_S16                *fistPassIFTempDst,
	EB_BOOL				   is16Bit,
	EB_U32				   componentMask);

extern void EncodeBiPredInterpolation(
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
    EB_S16                *refList0TempDst,
    EB_S16                *refList1TempDst,
    EB_S16                *fistPassIFTempDst);

void BiPredInterpolation16bit(
    EbPictureBufferDesc_t *fullPelBlockL0,
    EbPictureBufferDesc_t *fullPelBlockL1,
    EB_U32                 refList0PosX,
    EB_U32                 refList0PosY,
    EB_U32                 refList1PosX,
    EB_U32                 refList1PosY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *biDst,
    EB_U32                 dstLumaIndex,
    EB_U32                 dstChromaIndex,
    EB_S16                *refList0TempDst,
    EB_S16                *refList1TempDst,
    EB_S16                *fistPassIFTempDst);

extern void GeneratePadding(
    EB_BYTE  srcPic,
    EB_U32   srcStride,
    EB_U32   originalSrcWidth,
    EB_U32   originalSrcHeight,
    EB_U32   paddingWidth,
    EB_U32   paddingHeight);
extern void GeneratePadding16Bit(
	EB_BYTE  srcPic,
	EB_U32   srcStride,
	EB_U32   originalSrcWidth,
	EB_U32   originalSrcHeight,
	EB_U32   paddingWidth,
	EB_U32   paddingHeight);
extern void PadInputPicture(
    EB_BYTE  srcPic,
    EB_U32   srcStride,
    EB_U32   originalSrcWidth,
    EB_U32   originalSrcHeight,
	EB_U32   padRight,
	EB_U32   padBottom);

void AvcStyleCopy(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateOneDFilterChroma(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateTwoDFilterChroma(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosb(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosh(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosj(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosa(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosc(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosd(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPose(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosf(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosg(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosi(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosk(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosn(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosp(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosq(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void EstimateLumaInterpolationFilterPosr(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_BYTE               firstPassIFDst,
    EB_BYTE               secondPassIFDst,
    EB_BOOL               isLast);

void InterlacedBiPredAverageLumaIfResult(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_BYTE    list0Src,
    EB_U32     list0SrcDoubleStride,
    EB_BYTE    list1Src,
    EB_U32     list1SrcDoubleStride,
    EB_BYTE    dst,
    EB_U32     dstDoubleStride);

void InterlacedBiPredAverageChromaIfResult(
    EB_U32     chromaPuWidth,
    EB_U32     chromaPuHeight,
    EB_BYTE    list0CbSrc,
    EB_U32     list0CbSrcDoubleStride,
    EB_BYTE    list0CrSrc,
    EB_U32     list0CrSrcDoubleStride,
    EB_BYTE    list1CbSrc,
    EB_U32     list1CbSrcDoubleStride,
    EB_BYTE    list1CrSrc,
    EB_U32     list1CrSrcDoubleStride,
    EB_BYTE    cbDst,
    EB_U32     cbDstDoubleStride,
    EB_BYTE    crDst,
    EB_U32     crDstDoubleStride);


void EstimateUniPredInterpolation(
    EbPictureBufferDesc_t *refPic,
    EB_U32                 posX,
    EB_U32                 posY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *dst,
    EB_U32                 dstLumaIndex,
    EB_U32                 dstChromaIndex,
    EB_BYTE                tempBuf0,
    EB_BYTE                tempBuf1,
    EB_BYTE                tempBuf2,
    EB_U32                 componentMask);


// Function Tables (Super-long, declared in EbMcpTables.c)
extern const InterpolationFilterNew16bit uniPredLuma16bitIFFunctionPtrArray[EB_ASM_TYPE_TOTAL][16];
extern const InterpolationFilterChromaNew16bit uniPredChromaIFFunctionPtrArrayNew16bit[EB_ASM_TYPE_TOTAL][64];
extern const InterpolationFilterOutRaw16bit biPredLumaIFFunctionPtrArrayNew16bit[EB_ASM_TYPE_TOTAL][16];
extern const ChromaFilterOutRaw16bit biPredChromaIFFunctionPtrArrayNew16bit[EB_ASM_TYPE_TOTAL][64];
extern const sampleBiPredClipping biPredClippingFuncPtrArray[EB_ASM_TYPE_TOTAL];

extern const InterpolationFilterNew     uniPredLumaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][16];
extern const InterpolationFilterOutRaw  biPredLumaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][16];
extern const ChromaFilterNew            uniPredChromaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][64];
extern const ChromaFilterOutRaw         biPredChromaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][64];
extern const sampleBiPredClipping16bit biPredClipping16bitFuncPtrArray[EB_ASM_TYPE_TOTAL];


#ifdef __cplusplus
}
#endif
#endif // EBMCP_H
