/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbMcp.h"
#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureOperators.h"


#if (InternalBitDepthIncrement == 0)
#define ChromaOffset4 (1 << (Shift4 - 1))
#else
#define ChromaOffset4 Offset4
#endif
#if (InternalBitDepthIncrement == 0)
#define ChromaMinusOffset1 0
#else
#define ChromaMinusOffset1 MinusOffset1
#endif

EB_ERRORTYPE MotionCompensationPredictionContextCtor(
    MotionCompensationPredictionContext_t **contextDblPtr,
	EB_U16                                  maxCUWidth,
    EB_U16                                  maxCUHeight,
    EB_BOOL                                 is16bit)

{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    MotionCompensationPredictionContext_t *contextPtr;
    EB_MALLOC(MotionCompensationPredictionContext_t *, contextPtr, sizeof(MotionCompensationPredictionContext_t), EB_N_PTR);
    *(contextDblPtr) = contextPtr;

    // Jing: increase the size for 422/444
    EB_MALLOC(EB_S16*, contextPtr->motionCompensationIntermediateResultBuf0, sizeof(EB_S16)*(maxCUWidth*maxCUHeight * 3 + 8), EB_N_PTR);        //Y + U + V

    EB_MALLOC(EB_S16*, contextPtr->motionCompensationIntermediateResultBuf1, sizeof(EB_S16)*(maxCUWidth*maxCUHeight * 3 + 8), EB_N_PTR);        //Y + U + V

    EB_MALLOC(EB_BYTE, contextPtr->avcStyleMcpIntermediateResultBuf0, sizeof(EB_U8)*maxCUWidth*maxCUHeight * 6 * 3 + 16, EB_N_PTR);        //Y + U + V;

    EB_MALLOC(EB_BYTE, contextPtr->avcStyleMcpIntermediateResultBuf1, sizeof(EB_U8)*maxCUWidth*maxCUHeight * 6 * 3 + 16, EB_N_PTR);        //Y + U + V;

#if !USE_PRE_COMPUTE
    EB_MALLOC(EB_S16*, contextPtr->TwoDInterpolationFirstPassFilterResultBuf, sizeof(EB_S16)*(maxCUWidth + MaxHorizontalLumaFliterTag - 1)*(maxCUHeight + MaxVerticalLumaFliterTag - 1), EB_N_PTR);   // to be modified

    EB_MALLOC(EB_BYTE, contextPtr->avcStyleMcpTwoDInterpolationFirstPassFilterResultBuf, sizeof(EB_U8)*(6 * maxCUWidth + MaxHorizontalLumaFliterTag - 1)*(maxCUHeight + MaxVerticalLumaFliterTag - 1), EB_N_PTR);

#endif

    if (is16bit)
    {
        EbPictureBufferDescInitData_t initData;

        initData.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK;
        initData.maxWidth = maxCUWidth + 16;    // +8 needed for interpolation; the rest to accommodate MCP assembly kernels
        initData.maxHeight = maxCUHeight + 16;  // +8 needed for interpolation; the rest to accommodate MCP assembly kernels
        initData.bitDepth = EB_16BIT;
        initData.colorFormat = EB_YUV420;
		initData.leftPadding = 0;
		initData.rightPadding = 0;
		initData.topPadding = 0;
		initData.botPadding = 0;

        initData.splitMode = EB_FALSE;

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&contextPtr->localReferenceBlockL0,
            (EB_PTR)&initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&contextPtr->localReferenceBlockL1,
            (EB_PTR)&initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        initData.bitDepth = EB_8BIT;

        return_error = EbPictureBufferDescCtor( (EB_PTR*)&contextPtr->localReferenceBlock8BITL0,  (EB_PTR)&initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = EbPictureBufferDescCtor( (EB_PTR*)&contextPtr->localReferenceBlock8BITL1,  (EB_PTR)&initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }


    }
    return EB_ErrorNone;
}

void UniPredHevcInterpolationMd(
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
	EB_U32                 componentMask)
{
	EB_U32   integPosx;
	EB_U32   integPosy;
	EB_U8    fracPosx;
	EB_U8    fracPosy;
	EB_U32   chromaPuWidth = puWidth >> 1;
	EB_U32   chromaPuHeight = puHeight >> 1;


	(void)tempBuf1;

	//luma
	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
		//compute the luma fractional position
		integPosx = (posX >> 2);
		integPosy = (posY >> 2);
		fracPosx = posX & 0x03;
		fracPosy = posY & 0x03;

		uniPredLumaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
			is16bit ? refPic->bufferY + 4 + 4 * refPic->strideY : refPic->bufferY + integPosx + integPosy*refPic->strideY,
			refPic->strideY,
			dst->bufferY + dstLumaIndex,
			dst->strideY,
			puWidth,
			puHeight,
			tempBuf0);
	}

	//chroma
	if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {
		//compute the chroma fractional position
		integPosx = (posX >> 3);
		integPosy = (posY >> 3);
		fracPosx = posX & 0x07;
		fracPosy = posY & 0x07;


		uniPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
			is16bit ? refPic->bufferCb + 2 + 2 * refPic->strideCb : refPic->bufferCb + integPosx + integPosy * refPic->strideCb,
			refPic->strideCb,
			dst->bufferCb + dstChromaIndex,
			dst->strideCb,
			chromaPuWidth,
			chromaPuHeight,
			tempBuf0,
			fracPosx,
			fracPosy);

		//doing the chroma Cr interpolation
		uniPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
			is16bit ? refPic->bufferCr + 2 + 2 * refPic->strideCr : refPic->bufferCr + integPosx + integPosy * refPic->strideCr,
			refPic->strideCr,
			dst->bufferCr + dstChromaIndex,
			dst->strideCr,
			chromaPuWidth,
			chromaPuHeight,
			tempBuf0,
			fracPosx,
			fracPosy);
	}	    //input parameter, please refer to the detailed explanation above.
}

void EncodeUniPredInterpolation(
    EbPictureBufferDesc_t *refPic,                  //input parameter, please refer to the detailed explanation above.
    EB_U32                 posX,                    //input parameter, please refer to the detailed explanation above.
    EB_U32                 posY,                    //input parameter, please refer to the detailed explanation above.
    EB_U32                 puWidth,                 //input parameter
    EB_U32                 puHeight,                //input parameter
    EbPictureBufferDesc_t *dst,                     //output parameter, please refer to the detailed explanation above.
    EB_U32                 dstLumaIndex,            //input parameter, please refer to the detailed explanation above.
    EB_U32                 dstChromaIndex,          //input parameter, please refer to the detailed explanation above.
    EB_S16                *tempBuf0,                //input parameter, please refer to the detailed explanation above.
    EB_S16                *tempBuf1)                //input parameter, please refer to the detailed explanation above.
{
    EB_U32   integPosx;
    EB_U32   integPosy;
    EB_U8    fracPosx;
    EB_U8    fracPosy;

    EB_COLOR_FORMAT colorFormat=dst->colorFormat;
    EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    EB_U32 chromaPuWidth = puWidth >> subWidthCMinus1;
    EB_U32 chromaPuHeight = puHeight >> subHeightCMinus1;
    

    (void)tempBuf1;
    
    //luma
    //compute the luma fractional position
    integPosx = (posX >> 2);
    integPosy = (posY >> 2);
    fracPosx  = posX & 0x03;
    fracPosy  = posY & 0x03;

	uniPredLumaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
		refPic->bufferY + integPosx + integPosy*refPic->strideY,
		refPic->strideY,
		dst->bufferY + dstLumaIndex,
		dst->strideY,
		puWidth,
		puHeight,
		tempBuf0);

    //chroma
    //compute the chroma fractional position
    integPosx = (posX >> (2 + subWidthCMinus1));
    integPosy = (posY >> (2 + subHeightCMinus1));
    fracPosx  = (posX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
    fracPosy  = (posY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);

        
	uniPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
		refPic->bufferCb + integPosx + integPosy*refPic->strideCb,
		refPic->strideCb,
		dst->bufferCb + dstChromaIndex,
		dst->strideCb,
		chromaPuWidth,
		chromaPuHeight,
		tempBuf0,
		fracPosx,
		fracPosy);

	//doing the chroma Cr interpolation
	uniPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
		refPic->bufferCr + integPosx + integPosy*refPic->strideCr,
		refPic->strideCr,
		dst->bufferCr + dstChromaIndex,
		dst->strideCr,
		chromaPuWidth,
		chromaPuHeight,
		tempBuf0,
		fracPosx,
		fracPosy);
}

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
    EB_S16                *tempBuf0)                //input parameter, please refer to the detailed explanation above.
{
    const EB_COLOR_FORMAT colorFormat=dst->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    //EB_U32   integPosx;
    //EB_U32   integPosy;
    EB_U8 fracPosx;
    EB_U8 fracPosy;
    EB_U32 chromaPuWidth = puWidth >> subWidthCMinus1;
    EB_U32 chromaPuHeight = puHeight >> subHeightCMinus1;

    //EB_U32   position;
    (void)refPic;

    //luma
    //compute the luma fractional position
    // integPosx = (posX >> 2);
    // integPosy = (posY >> 2);
    fracPosx = posX & 0x03;
    fracPosy = posY & 0x03;

	uniPredLuma16bitIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
		(EB_U16 *)fullPelBlock->bufferY + 4 + 4 * fullPelBlock->strideY,
		fullPelBlock->strideY,
		(EB_U16*)(dst->bufferY) + dstLumaIndex,
		dst->strideY,
		puWidth,
		puHeight,
		tempBuf0);

    //chroma
    //compute the chroma fractional position
    //integPosx = (posX >> 3);
    //integPosy = (posY >> 3);
    fracPosx  = (posX & (0x07 >> (1 - subWidthCMinus1))) << (1 - subWidthCMinus1);
    fracPosy  = (posY & (0x07 >> (1 - subHeightCMinus1))) << (1 - subHeightCMinus1);

	uniPredChromaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & AVX2_MASK)][fracPosx + (fracPosy << 3)](
		(EB_U16 *)fullPelBlock->bufferCb + 2 + 2 * fullPelBlock->strideCb,
		fullPelBlock->strideCb,
		(EB_U16*)(dst->bufferCb) + dstChromaIndex,
		dst->strideCb,
		chromaPuWidth,
		chromaPuHeight,
		tempBuf0,
		fracPosx,
		fracPosy);

	uniPredChromaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & AVX2_MASK)][fracPosx + (fracPosy << 3)](
		(EB_U16 *)fullPelBlock->bufferCr + 2 + 2 * fullPelBlock->strideCr,
		fullPelBlock->strideCr,
		(EB_U16*)(dst->bufferCr) + dstChromaIndex,
		dst->strideCr,
		chromaPuWidth,
		chromaPuHeight,
		tempBuf0,
		fracPosx,
		fracPosy);
  
}

void BiPredHevcInterpolationMd(
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
	EB_U32				   componentMask)
{
	EB_U32   integPosx;
	EB_U32   integPosy;
	EB_U8    fracPosx;
	EB_U8    fracPosy;
	EB_U32   chromaPuWidth = puWidth >> 1;
	EB_U32   chromaPuHeight = puHeight >> 1;
	EB_U32   lumaTempBufSize = puWidth * puHeight;
	EB_U32   chromaTempBufSize = chromaPuWidth * chromaPuHeight;

	EB_U32   integPosL0x, integPosL1x;
	EB_U32   integPosL0y, integPosL1y;
	EB_U8    fracPosL0x, fracPosL1x;
	EB_U8    fracPosL0y, fracPosL1y;

	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
		//compute the luma fractional position
		integPosL0x = (refList0PosX >> 2);
		integPosL0y = (refList0PosY >> 2);
		fracPosL0x = refList0PosX & 0x03;
		fracPosL0y = refList0PosY & 0x03;


		//uni-prediction List1 luma

			//compute the luma fractional position
		integPosL1x = (refList1PosX >> 2);
		integPosL1y = (refList1PosY >> 2);
		fracPosL1x = refList1PosX & 0x03;
		fracPosL1y = refList1PosY & 0x03;

		if (((fracPosL0x + (fracPosL0y << 2)) == 0) && ((fracPosL1x + (fracPosL1y << 2)) == 0))
		{
			// bi-pred luma clipping
			BiPredClippingOnTheFly_SSSE3(
				is16Bit ? refPicList0->bufferY + 4 + 4 * refPicList0->strideY : refPicList0->bufferY + integPosL0x + integPosL0y*refPicList0->strideY,
				refPicList0->strideY,
				is16Bit ? refPicList1->bufferY + 4 + 4 * refPicList1->strideY : refPicList1->bufferY + integPosL1x + integPosL1y*refPicList1->strideY,
				refPicList1->strideY,
				biDst->bufferY + dstLumaIndex,
				biDst->strideY,
				puWidth,
				puHeight,
				Offset5,
				EB_TRUE);
		}
		else
		{
			//uni-prediction List0 luma
			//compute the luma fractional position
			integPosx = (refList0PosX >> 2);
			integPosy = (refList0PosY >> 2);
			fracPosx = refList0PosX & 0x03;
			fracPosy = refList0PosY & 0x03;

			// Note: SSSE3 Interpolation can only be enabled if 
			//       SSSE3 clipping functions are enabled
			//doing the luma interpolation
			biPredLumaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
				is16Bit ? refPicList0->bufferY + 4 + 4 * refPicList0->strideY : refPicList0->bufferY + integPosx + integPosy*refPicList0->strideY,
				refPicList0->strideY,
				refList0TempDst,
				puWidth,
				puHeight,
				fistPassIFTempDst);

			//uni-prediction List1 luma
			//compute the luma fractional position
			integPosx = (refList1PosX >> 2);
			integPosy = (refList1PosY >> 2);
			fracPosx = refList1PosX & 0x03;
			fracPosy = refList1PosY & 0x03;

			//doing the luma interpolation
			biPredLumaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
				is16Bit ? refPicList1->bufferY + 4 + 4 * refPicList1->strideY : refPicList1->bufferY + integPosx + integPosy*refPicList1->strideY,
				refPicList1->strideY,
				refList1TempDst,
				puWidth,
				puHeight,
				fistPassIFTempDst);

			// bi-pred luma clipping
			biPredClippingFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
				puWidth,
				puHeight,
				refList0TempDst,
				refList1TempDst,
				biDst->bufferY + dstLumaIndex,
				biDst->strideY,
				Offset5);
		}
	}

	if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {
		//uni-prediction List0 chroma
		//compute the chroma fractional position
		integPosL0x = (refList0PosX >> 3);
		integPosL0y = (refList0PosY >> 3);
		fracPosL0x = refList0PosX & 0x07;
		fracPosL0y = refList0PosY & 0x07;

		//uni-prediction List1 chroma
		//compute the chroma fractional position
		integPosL1x = (refList1PosX >> 3);
		integPosL1y = (refList1PosY >> 3);
		fracPosL1x = refList1PosX & 0x07;
		fracPosL1y = refList1PosY & 0x07;

		if (((fracPosL0x + (fracPosL0y << 3)) == 0) && ((fracPosL1x + (fracPosL1y << 3)) == 0))
		{
			// bi-pred luma clipping
			BiPredClippingOnTheFly_SSSE3(
				is16Bit ? refPicList0->bufferCb + 2 + 2 * refPicList0->strideCb : refPicList0->bufferCb + integPosL0x + integPosL0y*refPicList0->strideCb,
				refPicList0->strideCb,
				is16Bit ? refPicList1->bufferCb + 2 + 2 * refPicList1->strideCb : refPicList1->bufferCb + integPosL1x + integPosL1y*refPicList1->strideCb,
				refPicList1->strideCb,
				biDst->bufferCb + dstChromaIndex,
				biDst->strideCb,
				chromaPuWidth,
				chromaPuHeight,
				ChromaOffset5,
				EB_FALSE);

			// bi-pred luma clipping
			BiPredClippingOnTheFly_SSSE3(
				is16Bit ? refPicList0->bufferCr + 2 + 2 * refPicList0->strideCr : refPicList0->bufferCr + integPosL0x + integPosL0y*refPicList0->strideCr,
				refPicList0->strideCr,
				is16Bit ? refPicList1->bufferCr + 2 + 2 * refPicList1->strideCr : refPicList1->bufferCr + integPosL1x + integPosL1y*refPicList1->strideCr,
				refPicList1->strideCr,
				biDst->bufferCr + dstChromaIndex,
				biDst->strideCr,
				chromaPuWidth,
				chromaPuHeight,
				ChromaOffset5,
				EB_FALSE);
		}
		else
		{
			//uni-prediction List0 chroma
			//compute the chroma fractional position
			integPosx = (refList0PosX >> 3);
			integPosy = (refList0PosY >> 3);
			fracPosx = refList0PosX & 0x07;
			fracPosy = refList0PosY & 0x07;

			//doing the chroma Cb interpolation
			biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
				is16Bit ? refPicList0->bufferCb + 2 + 2 * refPicList0->strideCb : refPicList0->bufferCb + integPosx + integPosy*refPicList0->strideCb,
				refPicList0->strideCb,
				refList0TempDst + lumaTempBufSize,
				chromaPuWidth,
				chromaPuHeight,
				fistPassIFTempDst,
				fracPosx,
				fracPosy);

			//doing the chroma Cr interpolation
			biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
				is16Bit ? refPicList0->bufferCr + 2 + 2 * refPicList0->strideCr : refPicList0->bufferCr + integPosx + integPosy*refPicList0->strideCr,
				refPicList0->strideCr,
				refList0TempDst + lumaTempBufSize + chromaTempBufSize,
				chromaPuWidth,
				chromaPuHeight,
				fistPassIFTempDst,
				fracPosx,
				fracPosy);


			//uni-prediction List1 chroma
			//compute the chroma fractional position
			integPosx = (refList1PosX >> 3);
			integPosy = (refList1PosY >> 3);
			fracPosx = refList1PosX & 0x07;
			fracPosy = refList1PosY & 0x07;

			//doing the chroma Cb interpolation
			biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
				is16Bit ? refPicList1->bufferCb + 2 + 2 * refPicList1->strideCb : refPicList1->bufferCb + integPosx + integPosy*refPicList1->strideCb,
				refPicList1->strideCb,
				refList1TempDst + lumaTempBufSize,
				chromaPuWidth,
				chromaPuHeight,
				fistPassIFTempDst,
				fracPosx,
				fracPosy);

			//doing the chroma Cr interpolation
			biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
				is16Bit ? refPicList1->bufferCr + 2 + 2 * refPicList1->strideCr : refPicList1->bufferCr + integPosx + integPosy*refPicList1->strideCr,
				refPicList1->strideCr,
				refList1TempDst + lumaTempBufSize + chromaTempBufSize,
				chromaPuWidth,
				chromaPuHeight,
				fistPassIFTempDst,
				fracPosx,
				fracPosy);

			// bi-pred chroma clipping
			biPredClippingFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
				chromaPuWidth,
				chromaPuHeight,
				refList0TempDst + lumaTempBufSize,
				refList1TempDst + lumaTempBufSize,
				biDst->bufferCb + dstChromaIndex,
				biDst->strideCb,
				ChromaOffset5);
			biPredClippingFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
				chromaPuWidth,
				chromaPuHeight,
				refList0TempDst + lumaTempBufSize + chromaTempBufSize,
				refList1TempDst + lumaTempBufSize + chromaTempBufSize,
				biDst->bufferCr + dstChromaIndex,
				biDst->strideCr,
				ChromaOffset5);
		}
	}


	return;
}



void EncodeBiPredInterpolation(
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
    EB_S16                *fistPassIFTempDst)
{
    EB_U32   integPosx;
    EB_U32   integPosy;
    EB_U8    fracPosx;
    EB_U8    fracPosy;

    const EB_COLOR_FORMAT colorFormat = biDst->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    EB_U32   chromaPuWidth      = puWidth >> subWidthCMinus1;
    EB_U32   chromaPuHeight     = puHeight >> subHeightCMinus1;
    EB_U32   lumaTempBufSize    = puWidth * puHeight;
    EB_U32   chromaTempBufSize = chromaPuWidth * chromaPuHeight;

	EB_U32   integPosL0x, integPosL1x;
	EB_U32   integPosL0y, integPosL1y;
	EB_U8    fracPosL0x, fracPosL1x;
	EB_U8    fracPosL0y, fracPosL1y;
	//compute the luma fractional position
	integPosL0x = (refList0PosX >> 2);
	integPosL0y = (refList0PosY >> 2);
	fracPosL0x = refList0PosX & 0x03;
	fracPosL0y = refList0PosY & 0x03;


	//uni-prediction List1 luma
	//compute the luma fractional position
	integPosL1x = (refList1PosX >> 2);
	integPosL1y = (refList1PosY >> 2);
	fracPosL1x = refList1PosX & 0x03;
	fracPosL1y = refList1PosY & 0x03;

	if (((fracPosL0x + (fracPosL0y << 2)) == 0) && ((fracPosL1x + (fracPosL1y << 2)) == 0))
	{
		// bi-pred luma clipping
		BiPredClippingOnTheFly_SSSE3(
			refPicList0->bufferY + integPosL0x + integPosL0y*refPicList0->strideY,
			refPicList0->strideY,
			refPicList1->bufferY + integPosL1x + integPosL1y*refPicList1->strideY,
			refPicList1->strideY,
			biDst->bufferY + dstLumaIndex,
			biDst->strideY,
			puWidth,
			puHeight,
			Offset5,
			EB_TRUE);
	}
	else
	{
		//uni-prediction List0 luma
		//compute the luma fractional position
		integPosx = (refList0PosX >> 2);
		integPosy = (refList0PosY >> 2);
		fracPosx = refList0PosX & 0x03;
		fracPosy = refList0PosY & 0x03;

		// Note: SSSE3 Interpolation can only be enabled if 
		//       SSSE3 clipping functions are enabled
		//doing the luma interpolation
		biPredLumaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
			refPicList0->bufferY + integPosx + integPosy*refPicList0->strideY,
			refPicList0->strideY,
			refList0TempDst,
			puWidth,
			puHeight,
			fistPassIFTempDst);

		//uni-prediction List1 luma
		//compute the luma fractional position
		integPosx = (refList1PosX >> 2);
		integPosy = (refList1PosY >> 2);
		fracPosx = refList1PosX & 0x03;
		fracPosy = refList1PosY & 0x03;

		//doing the luma interpolation
		biPredLumaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
			refPicList1->bufferY + integPosx + integPosy*refPicList1->strideY,
			refPicList1->strideY,
			refList1TempDst,
			puWidth,
			puHeight,
			fistPassIFTempDst);

		// bi-pred luma clipping
		biPredClippingFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
			puWidth,
			puHeight,
			refList0TempDst,
			refList1TempDst,
			biDst->bufferY + dstLumaIndex,
			biDst->strideY,
			Offset5);
	}

	//uni-prediction List0 chroma
	//compute the chroma fractional position
	integPosL0x = (refList0PosX >> (2 + subWidthCMinus1));
	integPosL0y = (refList0PosY >> (2 + subHeightCMinus1));
	fracPosL0x = (refList0PosX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
	fracPosL0y = (refList0PosY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);


	//uni-prediction List1 chroma
	//compute the chroma fractional position
	integPosL1x = (refList1PosX >> (2 + subWidthCMinus1));
	integPosL1y = (refList1PosY >> (2 + subHeightCMinus1));
	fracPosL1x = (refList1PosX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
	fracPosL1y = (refList1PosY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);

	if (((fracPosL0x + (fracPosL0y << 3)) == 0) && ((fracPosL1x + (fracPosL1y << 3)) == 0))
	{
		// bi-pred luma clipping
		BiPredClippingOnTheFly_SSSE3(
			refPicList0->bufferCb + integPosL0x + integPosL0y*refPicList0->strideCb,
			refPicList0->strideCb,
			refPicList1->bufferCb + integPosL1x + integPosL1y*refPicList1->strideCb,
			refPicList1->strideCb,
			biDst->bufferCb + dstChromaIndex,
			biDst->strideCb,
			chromaPuWidth,
			chromaPuHeight,
			ChromaOffset5,
			EB_FALSE);

		// bi-pred luma clipping
		BiPredClippingOnTheFly_SSSE3(
			refPicList0->bufferCr + integPosL0x + integPosL0y*refPicList0->strideCr,
			refPicList0->strideCr,
			refPicList1->bufferCr + integPosL1x + integPosL1y*refPicList1->strideCr,
			refPicList1->strideCr,
			biDst->bufferCr + dstChromaIndex,
			biDst->strideCr,
			chromaPuWidth,
			chromaPuHeight,
			ChromaOffset5,
			EB_FALSE);
	}
	else
	{
		//uni-prediction List0 chroma
		//compute the chroma fractional position
		integPosx = (refList0PosX >> (2 + subWidthCMinus1));
		integPosy = (refList0PosY >> (2 + subHeightCMinus1));
		fracPosx = (refList0PosX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
		fracPosy = (refList0PosY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);

		//doing the chroma Cb interpolation
		biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
			refPicList0->bufferCb + integPosx + integPosy*refPicList0->strideCb,
			refPicList0->strideCb,
			refList0TempDst + lumaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);

		//doing the chroma Cr interpolation
		biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
			refPicList0->bufferCr + integPosx + integPosy*refPicList0->strideCr,
			refPicList0->strideCr,
			refList0TempDst + lumaTempBufSize + chromaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);


		//uni-prediction List1 chroma
		//compute the chroma fractional position
		//integPosx = (refList1PosX >> 3);
		//integPosy = (refList1PosY >> 3);
		//fracPosx = refList1PosX & 0x07;
		//fracPosy = refList1PosY & 0x07;

		integPosx = (refList1PosX >> (2 + subWidthCMinus1));
		integPosy = (refList1PosY >> (2 + subHeightCMinus1));
		fracPosx = (refList1PosX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
		fracPosy = (refList1PosY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);

		//doing the chroma Cb interpolation
		biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
			refPicList1->bufferCb + integPosx + integPosy*refPicList1->strideCb,
			refPicList1->strideCb,
			refList1TempDst + lumaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);

		//doing the chroma Cr interpolation
		biPredChromaIFFunctionPtrArrayNew[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 3)](
			refPicList1->bufferCr + integPosx + integPosy*refPicList1->strideCr,
			refPicList1->strideCr,
			refList1TempDst + lumaTempBufSize + chromaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);

		// bi-pred chroma clipping
		biPredClippingFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
			chromaPuWidth,
			chromaPuHeight,
			refList0TempDst + lumaTempBufSize,
			refList1TempDst + lumaTempBufSize,
			biDst->bufferCb + dstChromaIndex,
			biDst->strideCb,
			ChromaOffset5);
		biPredClippingFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
			chromaPuWidth,
			chromaPuHeight,
			refList0TempDst + lumaTempBufSize + chromaTempBufSize,
			refList1TempDst + lumaTempBufSize + chromaTempBufSize,
			biDst->bufferCr + dstChromaIndex,
			biDst->strideCr,
			ChromaOffset5);
	}


    return;
}

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
    EB_S16                *fistPassIFTempDst)
{
	//EB_U32   integPosx;
	//EB_U32   integPosy;
	EB_U8    fracPosx;
	EB_U8    fracPosy;
    const EB_COLOR_FORMAT colorFormat = biDst->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
	EB_U32   chromaPuWidth = puWidth >> subWidthCMinus1;
	EB_U32   chromaPuHeight = puHeight >> subHeightCMinus1;
	EB_U32   lumaTempBufSize = puWidth * puHeight;
	EB_U32   chromaTempBufSize = chromaPuWidth * chromaPuHeight;

	EB_U8    fracPosL0x, fracPosL1x;
	EB_U8    fracPosL0y, fracPosL1y;
	//compute the luma fractional position

	fracPosL0x = refList0PosX & 0x03;
	fracPosL0y = refList0PosY & 0x03;


	//uni-prediction List1 luma
	//compute the luma fractional position

	fracPosL1x = refList1PosX & 0x03;
	fracPosL1y = refList1PosY & 0x03;

	if (((fracPosL0x + (fracPosL0y << 2)) == 0) && ((fracPosL1x + (fracPosL1y << 2)) == 0))
	{
		// bi-pred luma clipping
		BiPredClippingOnTheFly16bit_SSE2(
			(EB_U16 *)fullPelBlockL0->bufferY + 4 + 4 * fullPelBlockL0->strideY,
			fullPelBlockL0->strideY,
			(EB_U16 *)fullPelBlockL1->bufferY + 4 + 4 * fullPelBlockL1->strideY,
			fullPelBlockL1->strideY,
			(EB_U16*)biDst->bufferY + dstLumaIndex,
			biDst->strideY,
			puWidth,
			puHeight);

	}
	else
	{
		//List0 luma
		//integPosx = (refList0PosX >> 2);
		//integPosy = (refList0PosY >> 2);
		fracPosx = refList0PosX & 0x03;
		fracPosy = refList0PosY & 0x03;

		biPredLumaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
			(EB_U16 *)fullPelBlockL0->bufferY + 4 + 4 * fullPelBlockL0->strideY,
			fullPelBlockL0->strideY,
			refList0TempDst,
			puWidth,
			puHeight,
			fistPassIFTempDst);

		//List1 luma
		//integPosx = (refList1PosX >> 2);
		//integPosy = (refList1PosY >> 2);
		fracPosx = refList1PosX & 0x03;
		fracPosy = refList1PosY & 0x03;

		biPredLumaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & PREAVX2_MASK)][fracPosx + (fracPosy << 2)](
			(EB_U16 *)fullPelBlockL1->bufferY + 4 + 4 * fullPelBlockL1->strideY,
			fullPelBlockL1->strideY,
			refList1TempDst,
			puWidth,
			puHeight,
			fistPassIFTempDst);

		biPredClipping16bitFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
			puWidth,
			puHeight,
			refList0TempDst,
			refList1TempDst,
			(EB_U16*)biDst->bufferY + dstLumaIndex,
			biDst->strideY);
	}

	fracPosL0x = (refList0PosX & (0x07 >> (1 - subWidthCMinus1))) << (1 - subWidthCMinus1);
	fracPosL0y = (refList0PosY & (0x07 >> (1 - subHeightCMinus1))) << (1 - subHeightCMinus1);

	fracPosL1x = (refList1PosX & (0x07 >> (1 - subWidthCMinus1))) << (1 - subWidthCMinus1);
	fracPosL1y = (refList1PosY & (0x07 >> (1 - subHeightCMinus1))) << (1 - subHeightCMinus1);

	if (((fracPosL0x + (fracPosL0y << 2)) == 0) && ((fracPosL1x + (fracPosL1y << 2)) == 0))
	{

		{
		BiPredClippingOnTheFly16bit_SSE2(
			(EB_U16 *)fullPelBlockL0->bufferCb + 2 + 2 * fullPelBlockL0->strideCb,
			fullPelBlockL0->strideCb,
			(EB_U16 *)fullPelBlockL1->bufferCb + 2 + 2 * fullPelBlockL1->strideCb,
			fullPelBlockL1->strideCb,
			(EB_U16*)biDst->bufferCb + dstChromaIndex,
			biDst->strideCb,
			chromaPuWidth,
			chromaPuHeight);


		BiPredClippingOnTheFly16bit_SSE2(
			(EB_U16 *)fullPelBlockL0->bufferCr + 2 + 2 * fullPelBlockL0->strideCr,
			fullPelBlockL0->strideCr,
			(EB_U16 *)fullPelBlockL1->bufferCr + 2 + 2 * fullPelBlockL1->strideCr,
			fullPelBlockL1->strideCr,
			(EB_U16*)biDst->bufferCr + dstChromaIndex,
			biDst->strideCr,
			chromaPuWidth,
			chromaPuHeight);
		}
	}
	else
	{
		//List0 chroma
		//integPosx = (refList0PosX >> 3);
		//integPosy = (refList0PosY >> 3);
		fracPosx = (refList0PosX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
		fracPosy = (refList0PosY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);

		biPredChromaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & AVX2_MASK)][fracPosx + (fracPosy << 3)](
			(EB_U16 *)fullPelBlockL0->bufferCb + 2 + 2 * fullPelBlockL0->strideCb,
			fullPelBlockL0->strideCb,
			refList0TempDst + lumaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);

		biPredChromaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & AVX2_MASK)][fracPosx + (fracPosy << 3)](
			(EB_U16 *)fullPelBlockL0->bufferCr + 2 + 2 * fullPelBlockL0->strideCr,
			fullPelBlockL0->strideCr,
			refList0TempDst + lumaTempBufSize + chromaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);


		//***********************
		//      L1: Y+Cb+Cr
		//***********************

		//List1 chroma
		//integPosx = (refList1PosX >> 3);
		//integPosy = (refList1PosY >> 3);
		fracPosx = (refList1PosX & (0x07 >> (1-subWidthCMinus1))) << (1-subWidthCMinus1);
		fracPosy = (refList1PosY & (0x07 >> (1-subHeightCMinus1))) << (1-subHeightCMinus1);

		biPredChromaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & AVX2_MASK)][fracPosx + (fracPosy << 3)](
			(EB_U16 *)fullPelBlockL1->bufferCb + 2 + 2 * fullPelBlockL1->strideCb,
			fullPelBlockL1->strideCb,
			refList1TempDst + lumaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);

		biPredChromaIFFunctionPtrArrayNew16bit[!!(ASM_TYPES & AVX2_MASK)][fracPosx + (fracPosy << 3)](
			(EB_U16 *)fullPelBlockL1->bufferCr + 2 + 2 * fullPelBlockL1->strideCr,
			fullPelBlockL1->strideCr,
			refList1TempDst + lumaTempBufSize + chromaTempBufSize,
			chromaPuWidth,
			chromaPuHeight,
			fistPassIFTempDst,
			fracPosx,
			fracPosy);


		//***********************
		//      L0+L1
		//***********************    
		biPredClipping16bitFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
			chromaPuWidth,
			chromaPuHeight,
			refList0TempDst + lumaTempBufSize,
			refList1TempDst + lumaTempBufSize,
			(EB_U16*)biDst->bufferCb + dstChromaIndex,
			biDst->strideCb);

		biPredClipping16bitFuncPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
			chromaPuWidth,
			chromaPuHeight,
			refList0TempDst + lumaTempBufSize + chromaTempBufSize,
			refList1TempDst + lumaTempBufSize + chromaTempBufSize,
			(EB_U16*)biDst->bufferCr + dstChromaIndex,
			biDst->strideCr);

	}

	return;
}
/** GeneratePadding()
        is used to pad the target picture. The horizontal padding happens first and then the vertical padding.
 */
void GeneratePadding(
    EB_BYTE  srcPic,                    //output paramter, pointer to the source picture to be padded.
    EB_U32   srcStride,                 //input paramter, the stride of the source picture to be padded.
    EB_U32   originalSrcWidth,          //input paramter, the width of the source picture which excludes the padding.
    EB_U32   originalSrcHeight,         //input paramter, the height of the source picture which excludes the padding.
    EB_U32   paddingWidth,              //input paramter, the padding width.
    EB_U32   paddingHeight)             //input paramter, the padding height.
{
    EB_U32   verticalIdx = originalSrcHeight;
    EB_BYTE  tempSrcPic0;
    EB_BYTE  tempSrcPic1;
    EB_BYTE  tempSrcPic2;
    EB_BYTE  tempSrcPic3;

    tempSrcPic0 = srcPic + paddingWidth + paddingHeight*srcStride;
    while(verticalIdx)
    {
        // horizontal padding
        EB_MEMSET(tempSrcPic0-paddingWidth, *tempSrcPic0, paddingWidth);
        EB_MEMSET(tempSrcPic0+originalSrcWidth, *(tempSrcPic0+originalSrcWidth-1), paddingWidth);

        tempSrcPic0 += srcStride;
        --verticalIdx;
    }

    // vertical padding
    verticalIdx = paddingHeight;
    tempSrcPic0 = srcPic + paddingHeight*srcStride;
    tempSrcPic1 = srcPic + (paddingHeight+originalSrcHeight-1)*srcStride;
    tempSrcPic2 = tempSrcPic0;
    tempSrcPic3 = tempSrcPic1;
    while(verticalIdx)
    {
        // top part data copy
        tempSrcPic2 -= srcStride;
		EB_MEMCPY(tempSrcPic2, tempSrcPic0, sizeof(EB_U8)*srcStride);        // EB_U8 to be modified
        // bottom part data copy
        tempSrcPic3 += srcStride;
		EB_MEMCPY(tempSrcPic3, tempSrcPic1, sizeof(EB_U8)*srcStride);        // EB_U8 to be modified
        --verticalIdx;
    }

    return;
}
/** GeneratePadding16Bit()
is used to pad the target picture. The horizontal padding happens first and then the vertical padding.
*/
void GeneratePadding16Bit(
	EB_BYTE  srcPic,                    //output paramter, pointer to the source picture to be padded.
	EB_U32   srcStride,                 //input paramter, the stride of the source picture to be padded.
	EB_U32   originalSrcWidth,          //input paramter, the width of the source picture which excludes the padding.
	EB_U32   originalSrcHeight,         //input paramter, the height of the source picture which excludes the padding.
	EB_U32   paddingWidth,              //input paramter, the padding width.
	EB_U32   paddingHeight)             //input paramter, the padding height.
{
	EB_U32   verticalIdx = originalSrcHeight;
	EB_BYTE  tempSrcPic0;
	EB_BYTE  tempSrcPic1;
	EB_BYTE  tempSrcPic2;
	EB_BYTE  tempSrcPic3;

	tempSrcPic0 = srcPic + paddingWidth + paddingHeight*srcStride;
	while (verticalIdx)
	{
		// horizontal padding
		//EB_MEMSET(tempSrcPic0 - paddingWidth, tempSrcPic0, paddingWidth);
		memset16bit((EB_U16*)(tempSrcPic0 - paddingWidth), ((EB_U16*)(tempSrcPic0))[0], paddingWidth >> 1);
		memset16bit((EB_U16*)(tempSrcPic0 + originalSrcWidth), ((EB_U16*)(tempSrcPic0 + originalSrcWidth - 2/*1*/))[0], paddingWidth >> 1);

		tempSrcPic0 += srcStride;
		--verticalIdx;
	}

	// vertical padding
	verticalIdx = paddingHeight;
	tempSrcPic0 = srcPic + paddingHeight*srcStride;
	tempSrcPic1 = srcPic + (paddingHeight + originalSrcHeight - 1)*srcStride;
	tempSrcPic2 = tempSrcPic0;
	tempSrcPic3 = tempSrcPic1;
	while (verticalIdx)
	{
		// top part data copy
		tempSrcPic2 -= srcStride;
		EB_MEMCPY(tempSrcPic2, tempSrcPic0, sizeof(EB_U8)*srcStride);        // EB_U8 to be modified
		// bottom part data copy
		tempSrcPic3 += srcStride;
		EB_MEMCPY(tempSrcPic3, tempSrcPic1, sizeof(EB_U8)*srcStride);        // EB_U8 to be modified
		--verticalIdx;
	}

	return;
}


/** PadInputPicture()
is used to pad the input picture in order to get . The horizontal padding happens first and then the vertical padding.
*/
void PadInputPicture(
	EB_BYTE  srcPic,                //output paramter, pointer to the source picture to be padded.
	EB_U32   srcStride,             //input paramter, the stride of the source picture to be padded.
	EB_U32   originalSrcWidth,      //input paramter, the width of the source picture which excludes the padding.
	EB_U32   originalSrcHeight,     //input paramter, the height of the source picture which excludes the padding.
	EB_U32   padRight,				//input paramter, the padding right.
	EB_U32   padBottom)             //input paramter, the padding bottom.
{

	EB_U32   verticalIdx;
	EB_BYTE  tempSrcPic0;
	EB_BYTE  tempSrcPic1;

	if (padRight) {

		// Add padding @ the right
		verticalIdx = originalSrcHeight;
		tempSrcPic0 = srcPic;

		while (verticalIdx)
		{

			EB_MEMSET(tempSrcPic0 + originalSrcWidth, *(tempSrcPic0 + originalSrcWidth - 1), padRight);
			tempSrcPic0 += srcStride;
			--verticalIdx;
		}
	}

	if (padBottom) {

		// Add padding @ the bottom
		verticalIdx = padBottom;
		tempSrcPic0 = srcPic + (originalSrcHeight - 1) * srcStride;
		tempSrcPic1 = tempSrcPic0;

		while (verticalIdx)
		{
			tempSrcPic1 += srcStride;
			EB_MEMCPY(tempSrcPic1, tempSrcPic0, sizeof(EB_U8)* (originalSrcWidth + padRight));
			--verticalIdx;
		}
	}

	return;
}

