/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEstimationProcess_h
#define EbEstimationProcess_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"
#include "EbMotionEstimationContext.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Context
 **************************************/
typedef struct MotionEstimationContext_s
{      
    EbFifo_t                        *pictureDecisionResultsInputFifoPtr;
    EbFifo_t                        *motionEstimationResultsOutputFifoPtr;

    MeContext_t                     *meContextPtr;

    IntraReferenceSamplesOpenLoop_t *intraRefPtr;

    // Multi-Mode signal(s)
    EB_BOOL                          oisKernelLevel;                // used by P/B Slices
    EB_U8                            oisThSet;                      // used by P/B Slices
    EB_BOOL                          setBestOisDistortionToValid;   // used by I/P/B Slices

} MotionEstimationContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE MotionEstimationContextCtor(
	MotionEstimationContext_t   **contextDblPtr,
	EbFifo_t                     *pictureDecisionResultsInputFifoPtr,
	EbFifo_t                     *motionEstimationResultsOutputFifoPtr);


extern void* MotionEstimationKernel(void *inputPtr);  

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationProcess_h