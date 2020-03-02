/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbPictureBufferDesc.h"
#include "EbReferenceObject.h"

void EbHevcInitializeSamplesNeighboringReferencePicture16Bit(
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

void EbHevcInitializeSamplesNeighboringReferencePicture8Bit(
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

void EbHevcInitializeSamplesNeighboringReferencePicture(
    EbReferenceObject_t              *referenceObject,
    EbPictureBufferDescInitData_t    *pictureBufferDescInitDataPtr,
    EB_BITDEPTH                       bitDepth) {

    if (bitDepth == EB_10BIT){

        EbHevcInitializeSamplesNeighboringReferencePicture16Bit(
            referenceObject->referencePicture16bit->bufferY,
            referenceObject->referencePicture16bit->strideY,
            referenceObject->referencePicture16bit->width,
            referenceObject->referencePicture16bit->height,
            pictureBufferDescInitDataPtr->leftPadding,
            pictureBufferDescInitDataPtr->topPadding);

        EbHevcInitializeSamplesNeighboringReferencePicture16Bit(
            referenceObject->referencePicture16bit->bufferCb,
            referenceObject->referencePicture16bit->strideCb,
            referenceObject->referencePicture16bit->width >> 1,
            referenceObject->referencePicture16bit->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);

        EbHevcInitializeSamplesNeighboringReferencePicture16Bit(
            referenceObject->referencePicture16bit->bufferCr,
            referenceObject->referencePicture16bit->strideCr,
            referenceObject->referencePicture16bit->width >> 1,
            referenceObject->referencePicture16bit->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);
    }
    else {

        EbHevcInitializeSamplesNeighboringReferencePicture8Bit(
            referenceObject->referencePicture->bufferY,
            referenceObject->referencePicture->strideY,
            referenceObject->referencePicture->width,
            referenceObject->referencePicture->height,
            pictureBufferDescInitDataPtr->leftPadding,
            pictureBufferDescInitDataPtr->topPadding);

        EbHevcInitializeSamplesNeighboringReferencePicture8Bit(
            referenceObject->referencePicture->bufferCb,
            referenceObject->referencePicture->strideCb,
            referenceObject->referencePicture->width >> 1,
            referenceObject->referencePicture->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);

        EbHevcInitializeSamplesNeighboringReferencePicture8Bit(
            referenceObject->referencePicture->bufferCr,
            referenceObject->referencePicture->strideCr,
            referenceObject->referencePicture->width >> 1,
            referenceObject->referencePicture->height >> 1,
            pictureBufferDescInitDataPtr->leftPadding >> 1,
            pictureBufferDescInitDataPtr->topPadding >> 1);
    }
}

static void EbReferenceObjectDctor(EB_PTR p)
{
    EbReferenceObject_t *obj = (EbReferenceObject_t*)p;
    EB_DELETE(obj->refDenSrcPicture);
    EB_FREE_ARRAY(obj->tmvpMap);
    EB_DELETE(obj->referencePicture);
    EB_DELETE(obj->referencePicture16bit);
}

/*****************************************
 * EbPictureBufferDescCtor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbReferenceObjectCtor(
    EbReferenceObject_t  *referenceObject,
    EB_PTR   objectInitDataPtr)
{
    EbPictureBufferDescInitData_t    *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*)  objectInitDataPtr;
    EbPictureBufferDescInitData_t    pictureBufferDescInitData16BitPtr = *pictureBufferDescInitDataPtr;

    referenceObject->dctor = EbReferenceObjectDctor;
    if (pictureBufferDescInitData16BitPtr.bitDepth == EB_10BIT){
        EB_NEW(
            referenceObject->referencePicture16bit,
            EbPictureBufferDescCtor,
            (EB_PTR)&pictureBufferDescInitData16BitPtr);

        EbHevcInitializeSamplesNeighboringReferencePicture(
            referenceObject,
            &pictureBufferDescInitData16BitPtr,
            pictureBufferDescInitData16BitPtr.bitDepth);
    }
    else{
        EB_NEW(
            referenceObject->referencePicture,
            EbPictureBufferDescCtor,
            (EB_PTR)pictureBufferDescInitDataPtr);

        EbHevcInitializeSamplesNeighboringReferencePicture(
            referenceObject,
            pictureBufferDescInitDataPtr,
            pictureBufferDescInitData16BitPtr.bitDepth);
    }

    // Allocate LCU based TMVP map
    EB_MALLOC_ARRAY(referenceObject->tmvpMap, ((pictureBufferDescInitDataPtr->maxWidth + (64 - 1)) >> 6) * ((pictureBufferDescInitDataPtr->maxHeight + (64 - 1)) >> 6));

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

        EB_NEW(
            referenceObject->refDenSrcPicture,
            EbPictureBufferDescCtor,
            (EB_PTR)&bufDesc);
    }

    return EB_ErrorNone;
}

EB_ERRORTYPE EbReferenceObjectCreator(
    EB_PTR  *objectDblPtr,
    EB_PTR   objectInitDataPtr)
{
    EbReferenceObject_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, EbReferenceObjectCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}

static void EbPaReferenceObjectDctor(EB_PTR p)
{
    EbPaReferenceObject_t* obj = (EbPaReferenceObject_t*)p;
    EB_DELETE(obj->inputPaddedPicturePtr);
    EB_DELETE(obj->quarterDecimatedPicturePtr);
    EB_DELETE(obj->sixteenthDecimatedPicturePtr);
}

/*****************************************
 * EbPaReferenceObjectCtor
 *  Initializes the Buffer Descriptor's
 *  values that are fixed for the life of
 *  the descriptor.
 *****************************************/
EB_ERRORTYPE EbPaReferenceObjectCtor(
    EbPaReferenceObject_t  *paReferenceObject,
    EB_PTR   objectInitDataPtr)
{
    EbPictureBufferDescInitData_t       *pictureBufferDescInitDataPtr = (EbPictureBufferDescInitData_t*)objectInitDataPtr;
    paReferenceObject->dctor = EbPaReferenceObjectDctor;

    // Reference picture constructor
    EB_NEW(
        paReferenceObject->inputPaddedPicturePtr,
        EbPictureBufferDescCtor,
        (EB_PTR)pictureBufferDescInitDataPtr);

    // Quarter Decim reference picture constructor
    EB_NEW(
        paReferenceObject->quarterDecimatedPicturePtr,
        EbPictureBufferDescCtor,
        (EB_PTR)(pictureBufferDescInitDataPtr + 1));

    // Sixteenth Decim reference picture constructor
    EB_NEW(
        paReferenceObject->sixteenthDecimatedPicturePtr,
        EbPictureBufferDescCtor,
        (EB_PTR)(pictureBufferDescInitDataPtr + 2));

    return EB_ErrorNone;
}

EB_ERRORTYPE EbPaReferenceObjectCreator(
    EB_PTR  *objectDblPtr,
    EB_PTR   objectInitDataPtr)
{
    EbPaReferenceObject_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, EbPaReferenceObjectCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}
