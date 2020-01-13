/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbUnPackProcess_h
#define EbUnPackProcess_h

#include "EbSystemResourceManager.h"
#include "EbErrorCodes.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct EB_ENC_UnPack2D_TYPE
{
    EB_U16 *in16BitBuffer;
    EB_U32  inStride;
    EB_U8  *out8BitBuffer;
    EB_U32  out8Stride;
    EB_U8  *outnBitBuffer;
    EB_U32 *outnStride;
    EB_U32  width;
    EB_U32  height;
}EB_ENC_UnPack2D_TYPE_t;

/***************************************
 * Context
 ***************************************/
typedef struct context_unpack
{
        EbFifo_t       *copyFrameInputFifoPtr;
        EbFifo_t       *copyFrameOutputFifoPtr;
        EbFifo_t       *unPackInputFifoPtr;
        EbFifo_t       *unPackOutPutFifoPtr;
        EB_U8           nbLumaThreads;
        EB_U8           nbChromaThreads;
}UnPackContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE UnPackContextCtor(
    UnPackContext_t   **contextDblPtr,
    EbFifo_t           *copyFrameInputFifoPtr,
    EbFifo_t           *copyFrameOutPutFifoPtr,
    EbFifo_t           *unPackInputFifoPtr,
    EbFifo_t           *unPackOutPutFifoPtr,
    EB_U8               nbLumaThreads,
    EB_U8               nbChromaThreads);

extern EB_ERRORTYPE UnPackCtor(
    EB_PTR  *Object,
    EB_PTR  data);

#ifdef __cplusplus
}
#endif
#endif // EbUnPackProcess.h