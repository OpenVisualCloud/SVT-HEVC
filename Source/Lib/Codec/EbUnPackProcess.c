/*
* Copyright(c) 2020 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbDefinitions.h"
#include "EbUnPackProcess.h"

/******************************************************
 * Enc Dec Context Constructor
 ******************************************************/
EB_ERRORTYPE UnPackContextCtor(
    UnPackContext_t    *contextPtr,
    EbFifo_t           *copyFrameInputFifoPtr,
    EbFifo_t           *copyFrameOutPutFifoPtr,
    EbFifo_t           *unPackInputFifoPtr,
    EbFifo_t           *unPackOutPutFifoPtr,
    EB_U8               nbLumaThreads,
    EB_U8               nbChromaThreads)
{
    contextPtr->copyFrameInputFifoPtr = copyFrameInputFifoPtr;
    contextPtr->copyFrameOutputFifoPtr = copyFrameOutPutFifoPtr;
    contextPtr->unPackInputFifoPtr = unPackInputFifoPtr;
    contextPtr->unPackOutPutFifoPtr = unPackOutPutFifoPtr;
    contextPtr->nbLumaThreads = nbLumaThreads;
    contextPtr->nbChromaThreads = nbChromaThreads;

    return EB_ErrorNone;
}

EB_ERRORTYPE UnPackCreator(
    EB_PTR  *objectDblPtr,
    EB_PTR   objectInitDataPtr)
{
    EBUnPack2DType_t* obj;

    *objectDblPtr = NULL;
    EB_CALLOC(obj, 1, sizeof(EBUnPack2DType_t));
    *objectDblPtr = obj;

    return EB_ErrorNone;
}

void UnPackDestoryer(EB_PTR p)
{
    EBUnPack2DType_t *obj = (EBUnPack2DType_t*)p;
    EB_FREE(obj);
}
