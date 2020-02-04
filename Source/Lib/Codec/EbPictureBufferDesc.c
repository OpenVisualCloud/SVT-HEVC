/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureBufferDesc.h"
#include "EbDefinitions.h"


static void EbPictureBufferDescDctor(EB_PTR p)
{
    EbPictureBufferDesc_t *obj = (EbPictureBufferDesc_t*)p;
    if (obj->bufferEnableMask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_FREE_ALIGNED_ARRAY(obj->bufferY);
        EB_FREE_ALIGNED_ARRAY(obj->bufferBitIncY);
    }
    if (obj->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_FREE_ALIGNED_ARRAY(obj->bufferCb);
        EB_FREE_ALIGNED_ARRAY(obj->bufferBitIncCb);
    }
    if (obj->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_FREE_ALIGNED_ARRAY(obj->bufferCr);
        EB_FREE_ALIGNED_ARRAY(obj->bufferBitIncCr);
    }
}
/*****************************************
 * EbPictureBufferDescCtor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbPictureBufferDescCtor(
    EbPictureBufferDesc_t  *pictureBufferDescPtr,
    EB_PTR   objectInitDataPtr)
{
    EbPictureBufferDescInitData_t  *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*)objectInitDataPtr;
    EB_U32 bytesPerPixel = (pictureBufferDescInitDataPtr->bitDepth == EB_8BIT) ? 1 : 2;

    if (pictureBufferDescInitDataPtr->colorFormat < EB_YUV420 || pictureBufferDescInitDataPtr->colorFormat > EB_YUV444) {
        pictureBufferDescInitDataPtr->colorFormat = EB_YUV420;
    }
    EB_U32 subWidthCMinus1 = (pictureBufferDescInitDataPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;

    pictureBufferDescPtr->dctor = EbPictureBufferDescDctor;
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

    if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE) {
        pictureBufferDescPtr->strideBitIncY  = pictureBufferDescPtr->strideY;
        pictureBufferDescPtr->strideBitIncCb = pictureBufferDescPtr->strideCb;
        pictureBufferDescPtr->strideBitIncCr = pictureBufferDescPtr->strideCr;
    }

    pictureBufferDescPtr->bufferEnableMask = pictureBufferDescInitDataPtr->bufferEnableMask;
    // Allocate the Picture Buffers (luma & chroma)
    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferY, pictureBufferDescPtr->lumaSize * bytesPerPixel);
        pictureBufferDescPtr->bufferBitIncY = 0;
        if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE ) {
            EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferBitIncY, pictureBufferDescPtr->lumaSize * bytesPerPixel);
        }
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferCb, pictureBufferDescPtr->chromaSize * bytesPerPixel);
        pictureBufferDescPtr->bufferBitIncCb = 0;
        if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE ) {
            EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferBitIncCb, pictureBufferDescPtr->chromaSize * bytesPerPixel);
        }
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
        EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferCr, pictureBufferDescPtr->chromaSize * bytesPerPixel);
        pictureBufferDescPtr->bufferBitIncCr = 0;
        if(pictureBufferDescInitDataPtr->splitMode == EB_TRUE ) {
            EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferBitIncCr, pictureBufferDescPtr->chromaSize * bytesPerPixel);
        }
    }

    return EB_ErrorNone;
}

static void EbReconPictureBufferDescDctor(EB_PTR p)
{
    EbPictureBufferDesc_t *obj = (EbPictureBufferDesc_t*)p;
    if (obj->bufferEnableMask & PICTURE_BUFFER_DESC_Y_FLAG)
        EB_FREE_ALIGNED_ARRAY(obj->bufferY);
    if (obj->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG)
        EB_FREE_ALIGNED_ARRAY(obj->bufferCb);
    if (obj->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG)
        EB_FREE_ALIGNED_ARRAY(obj->bufferCr);
}

/*****************************************
 * EbReconPictureBufferDescCtor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbReconPictureBufferDescCtor(
    EbPictureBufferDesc_t  *pictureBufferDescPtr,
    EB_PTR   objectInitDataPtr)
{
    EbPictureBufferDescInitData_t  *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*) objectInitDataPtr;
    EB_U32 bytesPerPixel = (pictureBufferDescInitDataPtr->bitDepth == EB_8BIT) ? 1 : 2;
    EB_U32 subWidthCMinus1 = (pictureBufferDescInitDataPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;

    pictureBufferDescPtr->dctor = EbReconPictureBufferDescDctor;
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

    pictureBufferDescPtr->bufferEnableMask = pictureBufferDescInitDataPtr->bufferEnableMask;
    // Allocate the Picture Buffers (luma & chroma)
    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Y_FLAG) {
        EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferY, pictureBufferDescPtr->lumaSize * bytesPerPixel);
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferCb, pictureBufferDescPtr->chromaSize * bytesPerPixel);
    }

    if(pictureBufferDescInitDataPtr->bufferEnableMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
        EB_CALLOC_ALIGNED_ARRAY(pictureBufferDescPtr->bufferCr, pictureBufferDescPtr->chromaSize * bytesPerPixel);
    }

    return EB_ErrorNone;
}
