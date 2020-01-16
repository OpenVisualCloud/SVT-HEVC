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
        UnPackContext_t   **contextDblPtr,
        EbFifo_t           *copyFrameInputFifoPtr,
        EbFifo_t           *copyFrameOutPutFifoPtr,
        EbFifo_t           *unPackInputFifoPtr,
        EbFifo_t           *unPackOutPutFifoPtr,
        EB_U8               nbLumaThreads,
        EB_U8               nbChromaThreads)
{
        UnPackContext_t        *contextPtr;
        EB_MALLOC(UnPackContext_t*, contextPtr, sizeof(UnPackContext_t),EB_N_PTR);
        *contextDblPtr = contextPtr;
        contextPtr->copyFrameInputFifoPtr = copyFrameInputFifoPtr;
        contextPtr->copyFrameOutputFifoPtr = copyFrameOutPutFifoPtr;
        contextPtr->unPackInputFifoPtr = unPackInputFifoPtr;
        contextPtr->unPackOutPutFifoPtr = unPackOutPutFifoPtr;
        contextPtr->nbLumaThreads = nbLumaThreads;
        contextPtr->nbChromaThreads = nbChromaThreads;

        return EB_ErrorNone;
}

EB_ERRORTYPE UnPackCtor(
        EB_PTR  *Object,
        EB_PTR  data)
{
        EB_ENC_UnPack2D_TYPE_t *unpack;
        EB_MALLOC(EB_ENC_UnPack2D_TYPE_t*,unpack,sizeof(EB_ENC_UnPack2D_TYPE_t),EB_N_PTR);
        *Object = unpack;
        EB_MALLOC(EB_U16*,unpack->in16BitBuffer,sizeof(EB_U16),EB_N_PTR);
        unpack->inStride = 0;
        EB_MALLOC(EB_U8*,unpack->out8BitBuffer,sizeof(EB_U8),EB_N_PTR);
        unpack->out8Stride = 0;
        EB_MALLOC(EB_U8*,unpack->outnBitBuffer,sizeof(EB_U8),EB_N_PTR);
        EB_MALLOC(EB_U8*,unpack->outnStride,sizeof(EB_U8),EB_N_PTR);
        unpack->width = 0;
        unpack->height = 0;

        return EB_ErrorNone;
}