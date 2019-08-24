/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControl_h
#define EbInitialRateControl_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbRateControlProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Context
 **************************************/
typedef struct InitialRateControlContext_s
{      
    EbFifo_t                    *motionEstimationResultsInputFifoPtr;
	EbFifo_t                    *initialrateControlResultsOutputFifoPtr;

} InitialRateControlContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE InitialRateControlContextCtor(
    InitialRateControlContext_t **contextDblPtr,
    EbFifo_t                     *motionEstimationResultsInputFifoPtr,
    EbFifo_t                     *pictureDemuxResultsOutputFifoPtr);
    
extern void* InitialRateControlKernel(void *inputPtr);

extern void MeBasedGlobalMotionDetection(
    SequenceControlSet_t         *sequenceControlSetPtr,
    PictureParentControlSet_t    *pictureControlSetPtr);

#ifdef __cplusplus
}
#endif
#endif // EbInitialRateControl_h
