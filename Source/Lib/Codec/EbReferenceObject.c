/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbPictureBufferDesc.h"
#include "EbReferenceObject.h"

void InitializeSamplesNeighboringReferencePicture16Bit(
    EB_BYTE  reconSamplesBufferPtr,
    EB_U16   stride,
    EB_U16   reconWidth,
    EB_U16   reconHeight,
    EB_U16   leftPadding,
    EB_U16   topPadding) {

    EB_U16  *reconSamplesPtr;
    EB_U16   sampleCount;

    // 1. Zero out the top row
    reconSamplesPtr = (EB_U16*)reconSamplesBufferPtr + (topPadding - 1) * stride + leftPadding - 1;
    EB_MEMSET((EB_U8*)reconSamplesPtr, 0, sizeof(EB_U16)*(1 + reconWidth + 1));

    // 2. Zero out the bottom row
    reconSamplesPtr = (EB_U16*)reconSamplesBufferPtr + (topPadding + reconHeight) * stride + leftPadding - 1; 
    EB_MEMSET((EB_U8*)reconSamplesPtr, 0, sizeof(EB_U16)*(1 + reconWidth + 1));
    
    // 3. Zero out the left column
    reconSamplesPtr = (EB_U16*)reconSamplesBufferPtr + topPadding * stride + leftPadding - 1;
    for (sampleCount = 0; sampleCount < reconHeight; sampleCount++) {
        reconSamplesPtr[sampleCount * stride] = 0;
    }

    // 4. Zero out the right column
    reconSamplesPtr = (EB_U16*)reconSamplesBufferPtr + topPadding * stride + leftPadding + reconWidth;
    for (sampleCount = 0; sampleCount < reconHeight; sampleCount++) {
        reconSamplesPtr[sampleCount * stride] = 0;
    }
}

void InitializeSamplesNeighboringReferencePicture8Bit(
    EB_BYTE  reconSamplesBufferPtr,
    EB_U16   stride,
    EB_U16   reconWidth,
    EB_U16   reconHeight,
    EB_U16   leftPadding,
    EB_U16   topPadding) {

    EB_U8   *reconSamplesPtr;
    EB_U16   sampleCount;

    // 1. Zero out the top row
    reconSamplesPtr = reconSamplesBufferPtr + (topPadding - 1) * stride + leftPadding - 1;
    EB_MEMSET(reconSamplesPtr, 0, sizeof(EB_U8)*(1 + reconWidth + 1));

    // 2. Zero out the bottom row
    reconSamplesPtr = reconSamplesBufferPtr + (topPadding + reconHeight) * stride + leftPadding - 1; 
    EB_MEMSET(reconSamplesPtr, 0, sizeof(EB_U8)*(1 + reconWidth + 1));
    
    // 3. Zero out the left column
    reconSamplesPtr = reconSamplesBufferPtr + topPadding * stride + leftPadding - 1;
    for (sampleCount = 0; sampleCount < reconHeight; sampleCount++) {
        reconSamplesPtr[sampleCount * stride] = 0;
    }

    // 4. Zero out the right column
    reconSamplesPtr = reconSamplesBufferPtr + topPadding * stride + leftPadding + reconWidth;
    for (sampleCount = 0; sampleCount < reconHeight; sampleCount++) {
        reconSamplesPtr[sampleCount * stride] = 0;
    }
}

void InitializeSamplesNeighboringReferencePicture(
    EbReferenceObject_t              *referenceObject,
    EbPictureBufferDescInitData_t    *pictureBufferDescInitDataPtr,
    EB_BITDEPTH                       bitDepth) {

    if (bitDepth == EB_10BIT){

        InitializeSamplesNeighboringReferencePicture16Bit(
            referenceObject->referencePicture16bit->bufferY,
            referenceObject->referencePicture16bit->strideY,
            referenceObject->referencePicture16bit->width,
            referenceObject->referencePicture16bit->height,
            pictureBufferDescInitDataPtr->leftPadding,
            pictureBufferDescInitDataPtr->topPadding);

        InitializeSamplesNeighboringReferencePicture16Bit(
            referenceObject->referencePicture16bit->bufferCb,
            referenceObject->referencePicture16bit->strideCb,
            referenceObject->referencePicture16bit->width >> 1,
            referenceObject->referencePicture16bit->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);

        InitializeSamplesNeighboringReferencePicture16Bit(
            referenceObject->referencePicture16bit->bufferCr,
            referenceObject->referencePicture16bit->strideCr,
            referenceObject->referencePicture16bit->width >> 1,
            referenceObject->referencePicture16bit->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);
    }
    else {
    
        InitializeSamplesNeighboringReferencePicture8Bit(
            referenceObject->referencePicture->bufferY,
            referenceObject->referencePicture->strideY,
            referenceObject->referencePicture->width,
            referenceObject->referencePicture->height,
            pictureBufferDescInitDataPtr->leftPadding,
            pictureBufferDescInitDataPtr->topPadding);

        InitializeSamplesNeighboringReferencePicture8Bit(
            referenceObject->referencePicture->bufferCb,
            referenceObject->referencePicture->strideCb,
            referenceObject->referencePicture->width >> 1,
            referenceObject->referencePicture->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);

        InitializeSamplesNeighboringReferencePicture8Bit(
            referenceObject->referencePicture->bufferCr,
            referenceObject->referencePicture->strideCr,
            referenceObject->referencePicture->width >> 1,
            referenceObject->referencePicture->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);
    }
}


/*****************************************
 * EbPictureBufferDescCtor
 *  Initializes the Buffer Descriptor's 
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbReferenceObjectCtor(
    EB_PTR  *objectDblPtr, 
    EB_PTR   objectInitDataPtr)
{

    EbReferenceObject_t              *referenceObject;
    EbPictureBufferDescInitData_t    *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*)  objectInitDataPtr;
    EbPictureBufferDescInitData_t    pictureBufferDescInitData16BitPtr = *pictureBufferDescInitDataPtr;


    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_MALLOC(EbReferenceObject_t*, referenceObject, sizeof(EbReferenceObject_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) referenceObject;

    if (pictureBufferDescInitData16BitPtr.bitDepth == EB_10BIT){

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&(referenceObject->referencePicture16bit),
            (EB_PTR)&pictureBufferDescInitData16BitPtr);

        InitializeSamplesNeighboringReferencePicture(
            referenceObject,
            &pictureBufferDescInitData16BitPtr,
            pictureBufferDescInitData16BitPtr.bitDepth);

    }
    else{

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&(referenceObject->referencePicture),
            (EB_PTR)pictureBufferDescInitDataPtr);

        InitializeSamplesNeighboringReferencePicture(
            referenceObject,
            pictureBufferDescInitDataPtr,
            pictureBufferDescInitData16BitPtr.bitDepth);
    }

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}



    // Allocate LCU based TMVP map   
    EB_MALLOC(TmvpUnit_t *, referenceObject->tmvpMap, (sizeof(TmvpUnit_t) * (((pictureBufferDescInitDataPtr->maxWidth + (64 - 1)) >> 6) * ((pictureBufferDescInitDataPtr->maxHeight + (64 - 1)) >> 6))), EB_N_PTR);

    //RESTRICT THIS TO M4
    {
        EbPictureBufferDescInitData_t bufDesc;

        bufDesc.maxWidth = pictureBufferDescInitDataPtr->maxWidth;
        bufDesc.maxHeight = pictureBufferDescInitDataPtr->maxHeight;
        bufDesc.bitDepth = EB_8BIT;
        bufDesc.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK; 
        bufDesc.leftPadding  = pictureBufferDescInitDataPtr->leftPadding;
        bufDesc.rightPadding = pictureBufferDescInitDataPtr->rightPadding;
        bufDesc.topPadding   = pictureBufferDescInitDataPtr->topPadding;
        bufDesc.botPadding   = pictureBufferDescInitDataPtr->botPadding;
        bufDesc.splitMode    = 0;
        bufDesc.colorFormat  = pictureBufferDescInitDataPtr->colorFormat;


        return_error = EbPictureBufferDescCtor((EB_PTR*)&(referenceObject->refDenSrcPicture),
                                                (EB_PTR)&bufDesc);
        if (return_error == EB_ErrorInsufficientResources)
            return EB_ErrorInsufficientResources;
    }    

    return EB_ErrorNone;
}

/*****************************************
 * EbPaReferenceObjectCtor
 *  Initializes the Buffer Descriptor's 
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbPaReferenceObjectCtor(
    EB_PTR  *objectDblPtr, 
    EB_PTR   objectInitDataPtr)
{

    EbPaReferenceObject_t               *paReferenceObject;
    EbPictureBufferDescInitData_t       *pictureBufferDescInitDataPtr   = (EbPictureBufferDescInitData_t*) objectInitDataPtr;
    EB_ERRORTYPE return_error                                           = EB_ErrorNone;
    EB_MALLOC(EbPaReferenceObject_t*, paReferenceObject, sizeof(EbPaReferenceObject_t), EB_N_PTR);
    *objectDblPtr = (EB_PTR) paReferenceObject;

    // Reference picture constructor
    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(paReferenceObject->inputPaddedPicturePtr),
        (EB_PTR )   pictureBufferDescInitDataPtr);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

	// Quarter Decim reference picture constructor
	paReferenceObject->quarterDecimatedPicturePtr = (EbPictureBufferDesc_t*)EB_NULL;
        return_error = EbPictureBufferDescCtor(
            (EB_PTR*) &(paReferenceObject->quarterDecimatedPicturePtr),
            (EB_PTR )  (pictureBufferDescInitDataPtr + 1));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

    // Sixteenth Decim reference picture constructor
	paReferenceObject->sixteenthDecimatedPicturePtr = (EbPictureBufferDesc_t*)EB_NULL;
        return_error = EbPictureBufferDescCtor(
            (EB_PTR*) &(paReferenceObject->sixteenthDecimatedPicturePtr),
            (EB_PTR )  (pictureBufferDescInitDataPtr + 2));
		if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    
    return EB_ErrorNone;
}



