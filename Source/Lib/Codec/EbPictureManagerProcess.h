/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureManager_h
#define EbPictureManager_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/***************************************
 * Context
 ***************************************/
typedef struct PictureManagerContext_s
{      
    EbDctor                   dctor;
    EbFifo_t                 *pictureInputFifoPtr;
    EbFifo_t                 *pictureManagerOutputFifoPtr;
    EbFifo_t                **pictureControlSetFifoPtrArray;
     
} PictureManagerContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE PictureManagerContextCtor(
    PictureManagerContext_t  *contextPtr,
    EbFifo_t                 *pictureInputFifoPtr,
    EbFifo_t                 *pictureManagerOutputFifoPtr,   
    EbFifo_t                **pictureControlSetFifoPtrArray);
    
   

extern void* PictureManagerKernel(void *inputPtr);
#ifdef __cplusplus
}
#endif
#endif // EbPictureManager_h