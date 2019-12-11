/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureBufferDesc.h"
#include "EbDefinitions.h"
/*****************************************
 * EbPictureBufferDescCtor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbPictureBufferDescCtor(
    EB_PTR  *objectDblPtr,
    EB_PTR   objectInitDataPtr,
    EB_HANDLE encHandle)
{
    EbPictureBufferDesc_t          *pictureBufferDescPtr;
    EbPictureBufferDescInitData_t  *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*) objectInitDataPtr;

    EB_U32 bytesPerPixel = (pictureBufferDescInitDataPtr->bitDepth == EB_8BIT) ? 1 : 2;
    
    EB_MALLOC(EbPictureBufferDesc_t*, pictureBufferDescPtr, sizeof(EbPictureBufferDesc_t), EB_N_PTR, encHandle);

    // Allocate the PictureBufferDesc Object
    *objectDblPtr       = (EB_PTR) pictureBufferDescPtr;

    if (pictureBufferDescInitDataPtr->colorFormat < EB_YUV420 || pictureBufferDescInitDataPtr->colorFormat > EB_YUV444) {
        pictureBufferDescInitDataPtr->colorFormat = EB_YUV420;
    }
    EB_U32 subWidthCMinus1 = (pictureBufferDescInitDataPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;
    // Set the Picture Buffer Static variables
    pictureBufferDescPtr->maxWidth      = pictureBufferDescInitDataPtr->maxWidth;
    pictureBufferDescPtr->maxHeight     = pictureBufferDescInitDataPtr->maxHeight;
    pictureBufferDescPtr->width         = pictureBufferDescInitDataPtr->maxWidth;
    pictureBufferDescPtr->height        = pictureBufferDescInitDataPtr->maxHeight;
    pictureBufferDescPtr->bitDepth      = pictureBufferDescInitDataPtr->bitDepth;
    pictureBufferDescPtr->colorFormat   = pictureBufferDescInitDataPtr->colorFormat;
	pictureBufferDescPtr->strideY		= pictureBufferDescInitDataPtr->maxWidth + pictureBufferDescInitDataPtr->leftPadding + pictureBufferDescInitDataPtr->rightPadding;
    pictureBufferDescPtr->strideCb      = pictureBufferDescPtr->strideCr = pictureBufferDescPtr->strideY >> subWidthCMinus1;
	pictureBufferDescPtr->originX		= pictureBufferDescInitDataPtr->leftPadding;
	pictureBufferDescPtr->originY		= pictureBufferDescInitDataPtr->topPadding;

	pictureBufferDescPtr->lumaSize		= (pictureBufferDescInitDataPtr->maxWidth + pictureBufferDescInitDataPtr->leftPadding + pictureBufferDescInitDataPtr->rightPadding) *
										  (pictureBufferDescInitDataPtr->maxHeight + pictureBufferDescInitDataPtr->topPadding + pictureBufferDescInitDataPtr->botPadding);
    pictureBufferDescPtr->chromaSize    = pictureBufferDescPtr->lumaSize >> (3 - pictureBufferDescInitDataPtr->colorFormat);
    pictureBufferDescPtr->packedFlag    = EB_FALSE;

    if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE) {
        pictureBufferDescPtr->strideBitIncY  = pictureBufferDescPtr->strideY;
        pictureBufferDescPtr->strideBitIncCb = pictureBufferDescPtr->strideCb;
        pictureBufferDescPtr->strideBitIncCr = pictureBufferDescPtr->strideCr;
    }
    else {
        pictureBufferDescPtr->strideBitIncY  = 0;
        pictureBufferDescPtr->strideBitIncCb = 0;
        pictureBufferDescPtr->strideBitIncCr = 0;
    }

    // Allocate the Picture Buffers (luma & chroma)
    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_ALLIGN_MALLOC(EB_BYTE, pictureBufferDescPtr->bufferY, pictureBufferDescPtr->lumaSize      * bytesPerPixel * sizeof(EB_U8), EB_A_PTR, encHandle);
        //pictureBufferDescPtr->bufferY = (EB_BYTE) EB_aligned_malloc( pictureBufferDescPtr->lumaSize      * bytesPerPixel * sizeof(EB_U8),ALVALUE);
        pictureBufferDescPtr->bufferBitIncY = 0;
        if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE ) {
            EB_ALLIGN_MALLOC(EB_BYTE, pictureBufferDescPtr->bufferBitIncY, pictureBufferDescPtr->lumaSize      * bytesPerPixel * sizeof(EB_U8), EB_A_PTR, encHandle);
            //pictureBufferDescPtr->bufferBitIncY = (EB_BYTE) EB_aligned_malloc( pictureBufferDescPtr->lumaSize      * bytesPerPixel * sizeof(EB_U8),ALVALUE);
        }
    }
    else {
        pictureBufferDescPtr->bufferY   = 0;
        pictureBufferDescPtr->bufferBitIncY = 0;
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_ALLIGN_MALLOC(EB_BYTE, pictureBufferDescPtr->bufferCb, pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8), EB_A_PTR, encHandle);
        //pictureBufferDescPtr->bufferCb = (EB_BYTE) EB_aligned_malloc(pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8),ALVALUE);
        pictureBufferDescPtr->bufferBitIncCb = 0;
        if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE ) {
            EB_ALLIGN_MALLOC(EB_BYTE, pictureBufferDescPtr->bufferBitIncCb, pictureBufferDescPtr->chromaSize      * bytesPerPixel * sizeof(EB_U8), EB_A_PTR, encHandle);
            //pictureBufferDescPtr->bufferBitIncCb = (EB_BYTE) EB_aligned_malloc(pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8),ALVALUE);
        }
    }
    else {
        pictureBufferDescPtr->bufferCb   = 0;
        pictureBufferDescPtr->bufferBitIncCb= 0;
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
        EB_ALLIGN_MALLOC(EB_BYTE, pictureBufferDescPtr->bufferCr, pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8), EB_A_PTR, encHandle);
        //pictureBufferDescPtr->bufferCr = (EB_BYTE) EB_aligned_malloc(pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8),ALVALUE);
        pictureBufferDescPtr->bufferBitIncCr = 0;
        if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE ) {
            EB_ALLIGN_MALLOC(EB_BYTE, pictureBufferDescPtr->bufferBitIncCr, pictureBufferDescPtr->chromaSize      * bytesPerPixel * sizeof(EB_U8), EB_A_PTR, encHandle);
            //pictureBufferDescPtr->bufferBitIncCr = (EB_BYTE) EB_aligned_malloc(pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8),ALVALUE);
        }
    }
    else {
        pictureBufferDescPtr->bufferCr   = 0;
        pictureBufferDescPtr->bufferBitIncCr = 0;
    }

    return EB_ErrorNone;
}


/*****************************************
 * EbReconPictureBufferDescCtor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbReconPictureBufferDescCtor(
    EB_PTR  *objectDblPtr,
    EB_PTR   objectInitDataPtr,
    EB_HANDLE encHandle)
{
    EbPictureBufferDesc_t          *pictureBufferDescPtr;
    EbPictureBufferDescInitData_t  *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*) objectInitDataPtr;

    EB_U32 bytesPerPixel = (pictureBufferDescInitDataPtr->bitDepth == EB_8BIT) ? 1 : 2;

    EB_MALLOC(EbPictureBufferDesc_t*, pictureBufferDescPtr, sizeof(EbPictureBufferDesc_t), EB_N_PTR, encHandle);

    // Allocate the PictureBufferDesc Object
    *objectDblPtr       = (EB_PTR) pictureBufferDescPtr;

    EB_U32 subWidthCMinus1 = (pictureBufferDescInitDataPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;
    // Set the Picture Buffer Static variables
    pictureBufferDescPtr->maxWidth      = pictureBufferDescInitDataPtr->maxWidth;
    pictureBufferDescPtr->maxHeight     = pictureBufferDescInitDataPtr->maxHeight;
    pictureBufferDescPtr->width         = pictureBufferDescInitDataPtr->maxWidth;
    pictureBufferDescPtr->height        = pictureBufferDescInitDataPtr->maxHeight;
    pictureBufferDescPtr->bitDepth      = pictureBufferDescInitDataPtr->bitDepth;
    pictureBufferDescPtr->colorFormat   = pictureBufferDescInitDataPtr->colorFormat;
	pictureBufferDescPtr->strideY		= pictureBufferDescInitDataPtr->maxWidth + pictureBufferDescInitDataPtr->leftPadding + pictureBufferDescInitDataPtr->rightPadding;
    pictureBufferDescPtr->strideCb      = pictureBufferDescPtr->strideCr = pictureBufferDescPtr->strideY >> subWidthCMinus1;
	pictureBufferDescPtr->originX		= pictureBufferDescInitDataPtr->leftPadding;
	pictureBufferDescPtr->originY		= pictureBufferDescInitDataPtr->topPadding;

	pictureBufferDescPtr->lumaSize		= (pictureBufferDescInitDataPtr->maxWidth + pictureBufferDescInitDataPtr->leftPadding + pictureBufferDescInitDataPtr->rightPadding) *
									      (pictureBufferDescInitDataPtr->maxHeight + pictureBufferDescInitDataPtr->topPadding + pictureBufferDescInitDataPtr->botPadding);
    pictureBufferDescPtr->chromaSize    = pictureBufferDescPtr->lumaSize >> (3 - pictureBufferDescInitDataPtr->colorFormat);
    pictureBufferDescPtr->packedFlag    = EB_FALSE;

    pictureBufferDescPtr->strideBitIncY  = 0;
    pictureBufferDescPtr->strideBitIncCb = 0;
    pictureBufferDescPtr->strideBitIncCr = 0;

    // Allocate the Picture Buffers (luma & chroma)
    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_CALLOC(EB_BYTE, pictureBufferDescPtr->bufferY, pictureBufferDescPtr->lumaSize      * bytesPerPixel * sizeof(EB_U8) + (pictureBufferDescPtr->width + 1) * 2 * bytesPerPixel*sizeof(EB_U8), 1, EB_N_PTR, encHandle);
        pictureBufferDescPtr->bufferY += (pictureBufferDescPtr->width+1) * bytesPerPixel;
    }
    else {
        pictureBufferDescPtr->bufferY   = 0;
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_CALLOC(EB_BYTE, pictureBufferDescPtr->bufferCb, pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8) + ((pictureBufferDescPtr->width >> 1) + 1) * 2 * bytesPerPixel*sizeof(EB_U8), 1, EB_N_PTR, encHandle);
        pictureBufferDescPtr->bufferCb += ((pictureBufferDescPtr->width >> 1) +1) * bytesPerPixel;
    }
    else {
        pictureBufferDescPtr->bufferCb   = 0;
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
        EB_CALLOC(EB_BYTE, pictureBufferDescPtr->bufferCr, pictureBufferDescPtr->chromaSize    * bytesPerPixel * sizeof(EB_U8) + ((pictureBufferDescPtr->width >> 1) + 1) * 2 * bytesPerPixel*sizeof(EB_U8), 1, EB_N_PTR, encHandle);
        pictureBufferDescPtr->bufferCr += ((pictureBufferDescPtr->width >> 1)+1) * bytesPerPixel;
    }
    else {
        pictureBufferDescPtr->bufferCr   = 0;
    }

    return EB_ErrorNone;
}
