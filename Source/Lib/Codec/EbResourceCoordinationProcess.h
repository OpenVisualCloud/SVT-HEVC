/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbResourceCoordination_h
#define EbResourceCoordination_h

#include "EbSystemResourceManager.h"
#include "EbDefinitions.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/***************************************
 * Context
 ***************************************/
typedef struct ResourceCoordinationContext_s
{      
    EbDctor                              dctor;
    EbFifo_t                            *inputBufferFifoPtr;
    EbFifo_t                            *resourceCoordinationResultsOutputFifoPtr;
    EbFifo_t                           **pictureControlSetFifoPtrArray;
    EbSequenceControlSetInstance_t     **sequenceControlSetInstanceArray;
    EbObjectWrapper_t                  **sequenceControlSetActiveArray;
    EbFifo_t                            *sequenceControlSetEmptyFifoPtr;
    EbCallback_t                       **appCallbackPtrArray;
     
    // Compute Segments
    EB_U32                              *computeSegmentsTotalCountArray;
    EB_U32                               encodeInstancesTotalCount;
    
    // Picture Number Array
    EB_U64                              *pictureNumberArray;

	EB_U64                               averageEncMod;
	EB_U8                                prevEncMod;
	EB_S8                                prevEncModeDelta;
	EB_U8                                prevChangeCond;

	EB_S64                               previousModeChangeBuffer;
	EB_S64                               previousModeChangeFrameIn;
	EB_S64                               previousBufferCheck1;
	EB_S64                               previousFrameInCheck1;
	EB_S64                               previousFrameInCheck2;
	EB_S64                               previousFrameInCheck3;


	EB_U64                               curSpeed; // speed x 1000
    EB_U64                               prevsTimeSeconds;
    EB_U64                               prevsTimeuSeconds;
	EB_S64                               prevFrameOut;

    EB_U64                               firstInPicArrivedTimeSeconds;
    EB_U64                               firstInPicArrivedTimeuSeconds;
	EB_BOOL                              startFlag;

} ResourceCoordinationContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE ResourceCoordinationContextCtor(
    ResourceCoordinationContext_t       *contextPtr,
    EbFifo_t                            *inputBufferFifoPtr,
    EbFifo_t                            *resourceCoordinationResultsOutputFifoPtr,
    EbFifo_t                           **pictureControlSetFifoPtrArray,
    EbSequenceControlSetInstance_t     **sequenceControlSetInstanceArray,
    EbFifo_t                            *sequenceControlSetEmptyFifoPtr,
    EbCallback_t                       **appCallbackPtrArray,
    EB_U32                              *computeSegmentsTotalCountArray,
    EB_U32                               encodeInstancesTotalCount);
    
  

extern void* ResourceCoordinationKernel(void *inputPtr);
#ifdef __cplusplus
}
#endif
#endif // EbResourceCoordination_h