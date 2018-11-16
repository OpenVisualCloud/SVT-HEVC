/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureManager_h
#define EbPictureManager_h

#include "EbTypes.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/***************************************
 * Context
 ***************************************/
typedef struct PictureManagerContext_s
{      
    EbFifo_t                 *pictureInputFifoPtr;
    EbFifo_t                 *pictureManagerOutputFifoPtr;
    EbFifo_t                **pictureControlSetFifoPtrArray;
     
} PictureManagerContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE PictureManagerContextCtor(
    PictureManagerContext_t **contextDblPtr,
    EbFifo_t                 *pictureInputFifoPtr,
    EbFifo_t                 *pictureManagerOutputFifoPtr,   
    EbFifo_t                **pictureControlSetFifoPtrArray);
    
   

extern void* PictureManagerKernel(void *inputPtr);
#ifdef __cplusplus
}
#endif
#endif // EbPictureManager_h