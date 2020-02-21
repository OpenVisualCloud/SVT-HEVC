/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbDefinitions.h"
#include "EbTransQuantBuffers.h"
#include "EbPictureBufferDesc.h"


static void EbTransQuantBuffersDctor(EB_PTR p)
{
    EbTransQuantBuffers_t* obj = (EbTransQuantBuffers_t*)p;
    EB_DELETE(obj->tuTransCoeff2Nx2NPtr);
    EB_DELETE(obj->tuTransCoeffNxNPtr);
    EB_DELETE(obj->tuTransCoeffN2xN2Ptr);
    EB_DELETE(obj->tuQuantCoeffNxNPtr);
    EB_DELETE(obj->tuQuantCoeffN2xN2Ptr);
}

EB_ERRORTYPE EbTransQuantBuffersCtor(
    EbTransQuantBuffers_t          *transQuantBuffersPtr)
{
    transQuantBuffersPtr->dctor = EbTransQuantBuffersDctor;

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

    EB_NEW(
        transQuantBuffersPtr->tuTransCoeff2Nx2NPtr,
        EbPictureBufferDescCtor,
        (EB_PTR)&transCoeffInitArray);

    EB_NEW(
        transQuantBuffersPtr->tuTransCoeffNxNPtr,
        EbPictureBufferDescCtor,
        (EB_PTR)&transCoeffInitArray);

    EB_NEW(
        transQuantBuffersPtr->tuTransCoeffN2xN2Ptr,
        EbPictureBufferDescCtor,
        (EB_PTR)&transCoeffInitArray);

    EB_NEW(
        transQuantBuffersPtr->tuQuantCoeffNxNPtr,
        EbPictureBufferDescCtor,
        (EB_PTR)&transCoeffInitArray);

    EB_NEW(
        transQuantBuffersPtr->tuQuantCoeffN2xN2Ptr,
        EbPictureBufferDescCtor,
        (EB_PTR)&transCoeffInitArray);

    return EB_ErrorNone;
}
