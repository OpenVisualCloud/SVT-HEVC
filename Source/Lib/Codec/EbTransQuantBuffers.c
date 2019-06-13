/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbDefinitions.h"
#include "EbTransQuantBuffers.h"
#include "EbPictureBufferDesc.h"


EB_ERRORTYPE EbTransQuantBuffersCtor(
    EbTransQuantBuffers_t          *transQuantBuffersPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EbPictureBufferDescInitData_t transCoeffInitArray;

    transCoeffInitArray.maxWidth            = MAX_LCU_SIZE;
    transCoeffInitArray.maxHeight           = MAX_LCU_SIZE;
    transCoeffInitArray.bitDepth            = EB_16BIT;
    transCoeffInitArray.colorFormat         = EB_YUV420;
    transCoeffInitArray.bufferEnableMask    = PICTURE_BUFFER_DESC_FULL_MASK;
	transCoeffInitArray.leftPadding			= 0;
	transCoeffInitArray.rightPadding		= 0;
	transCoeffInitArray.topPadding			= 0;
	transCoeffInitArray.botPadding			= 0;
    transCoeffInitArray.splitMode           = EB_FALSE;

    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(transQuantBuffersPtr->tuTransCoeff2Nx2NPtr),
        (EB_PTR ) &transCoeffInitArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(transQuantBuffersPtr->tuTransCoeffNxNPtr),
        (EB_PTR ) &transCoeffInitArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(transQuantBuffersPtr->tuTransCoeffN2xN2Ptr),
        (EB_PTR ) &transCoeffInitArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(transQuantBuffersPtr->tuQuantCoeffNxNPtr),
        (EB_PTR ) &transCoeffInitArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(transQuantBuffersPtr->tuQuantCoeffN2xN2Ptr),
        (EB_PTR ) &transCoeffInitArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}

