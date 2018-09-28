/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControl_h
#define EbInitialRateControl_h

#include "EbTypes.h"
#include "EbSystemResourceManager.h"
#include "EbRateControlProcess.h"



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

#endif // EbInitialRateControl_h