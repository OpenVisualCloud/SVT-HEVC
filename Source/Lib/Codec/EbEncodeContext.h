/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncodeContext_h
#define EbEncodeContext_h

#include <stdio.h>

#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbApi.h"
#include "EbPictureDecisionReorderQueue.h"
#include "EbPictureDecisionQueue.h"
#include "EbPictureManagerQueue.h"
#include "EbPacketizationReorderQueue.h"
#include "EbInitialRateControlReorderQueue.h"
#include "EbPictureManagerReorderQueue.h"
#include "EbCabacContextModel.h"
#include "EbMdRateEstimation.h"
#include "EbPredictionStructure.h"
#include "EbRateControlTables.h"

#ifdef __cplusplus
extern "C" {
#endif

// *Note - the queues are small for testing purposes.  They should be increased when they are done.
#define PRE_ASSIGNMENT_MAX_DEPTH                    128     // should be large enough to hold an entire prediction period
#define INPUT_QUEUE_MAX_DEPTH                       5000
#define REFERENCE_QUEUE_MAX_DEPTH                   5000
#define PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH   5000

#define PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH            2048
#define INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH        2048
#define PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH             2048
#define HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH   2048
#define PACKETIZATION_REORDER_QUEUE_MAX_DEPTH               2048

// RC Groups: They should be a power of 2, so we can replace % by &. 
// Instead of using x % y, we use x && (y-1)
#define PARALLEL_GOP_MAX_NUMBER                         256 
#define RC_GROUP_IN_GOP_MAX_NUMBER                      512
#define PICTURE_IN_RC_GROUP_MAX_NUMBER                  64

typedef struct EncodeContext_s 
{
    // Callback Functions
    EbCallback_t                     *appCallbackPtr;

    EB_HANDLE                            terminatingConditionsMutex;
    EB_U64                               totalNumberOfReconFrames;

    // Output Buffer Fifos
    EbFifo_t                            *streamOutputFifoPtr;
    EbFifo_t                            *reconOutputFifoPtr;
    
    // Picture Buffer Fifos
    EbFifo_t                            *inputPicturePoolFifoPtr;
    EbFifo_t                            *referencePicturePoolFifoPtr;
    EbFifo_t                            *paReferencePicturePoolFifoPtr;
    
    // Picture Decision Reorder Queue
    PictureDecisionReorderEntry_t      **pictureDecisionReorderQueue;
    EB_U32                               pictureDecisionReorderQueueHeadIndex;
    
	// Picture Manager Reorder Queue
	PictureManagerReorderEntry_t       **pictureManagerReorderQueue;
	EB_U32                               pictureManagerReorderQueueHeadIndex;

    // Picture Manager Pre-Assignment Buffer
    EB_U32                               preAssignmentBufferIntraCount;
    EB_U32                               preAssignmentBufferIdrCount;
    EB_U32                               preAssignmentBufferSceneChangeCount;
    EB_U32                               preAssignmentBufferSceneChangeIndex;
    EB_U32                               preAssignmentBufferEosFlag;
    EB_U64                               decodeBaseNumber;
    
    EbObjectWrapper_t                  **preAssignmentBuffer;
    EB_U32                               preAssignmentBufferCount;
    EB_U32                               numberOfActivePictures;

    // Picture Decision Circular Queues
    PaReferenceQueueEntry_t            **pictureDecisionPaReferenceQueue;
    EB_U32                               pictureDecisionPaReferenceQueueHeadIndex;
    EB_U32                               pictureDecisionPaReferenceQueueTailIndex;

    // Picture Manager Circular Queues
    InputQueueEntry_t                  **inputPictureQueue;
    EB_U32                               inputPictureQueueHeadIndex;
    EB_U32                               inputPictureQueueTailIndex;

    ReferenceQueueEntry_t              **referencePictureQueue;
    EB_U32                               referencePictureQueueHeadIndex;
    EB_U32                               referencePictureQueueTailIndex;
    
    // Initial Rate Control Reorder Queue
    InitialRateControlReorderEntry_t   **initialRateControlReorderQueue;
    EB_U32                               initialRateControlReorderQueueHeadIndex;

    // High Level Rate Control Histogram Queue
    HlRateControlHistogramEntry_t      **hlRateControlHistorgramQueue;
    EB_U32                               hlRateControlHistorgramQueueHeadIndex;
    EB_HANDLE                            hlRateControlHistorgramQueueMutex;

    // Packetization Reorder Queue
    PacketizationReorderEntry_t        **packetizationReorderQueue;
    EB_U32                               packetizationReorderQueueHeadIndex;
    
    // GOP Counters
    EB_U32                               intraPeriodPosition;        // Current position in intra period
    EB_U32                               predStructPosition;         // Current position within a prediction structure

    EB_U32                               elapsedNonIdrCount;
    EB_U32                               elapsedNonCraCount;
    EB_S64                               currentInputPoc;
    EB_BOOL                              initialPicture;

    EB_U64                               lastIdrPicture; // the most recently occured IDR picture (in decode order)
    // Sequence Termination Flags
    EB_U64							     terminatingPictureNumber;
	EB_BOOL                              terminatingSequenceFlagReceived;

    // Prediction Structure
    PredictionStructureGroup_t          *predictionStructureGroupPtr;
    
    // Cabac Context Model Array
    ContextModelEncContext_t            *cabacContextModelArray; 

    // MD Rate Estimation Table
    MdRateEstimationContext_t           *mdRateEstimationArray;

    // Rate Control Bit Tables
    RateControlTables_t                 *rateControlTablesArray;
    EB_BOOL                              rateControlTablesArrayUpdated;
    EB_HANDLE                            rateTableUpdateMutex;

    // Speed Control
    EB_S64                               scBuffer;
    EB_S64                               scFrameIn;
    EB_S64                               scFrameOut;
    EB_HANDLE                            scBufferMutex;

	EB_ENC_MODE                          encMode;

    // Rate Control
    EB_U32                               availableTargetBitRate;
    EB_BOOL                              availableTargetBitRateChanged;
    EB_U32                               vbvMaxrate;
    EB_U32                               vbvBufsize;
    EB_U64                               bufferFill;
    EB_S64                               fillerBitError;
    EB_HANDLE                            bufferFillMutex;

    EB_U32								 previousSelectedRefQp;
    EB_U64								 maxCodedPoc;
    EB_U32								 maxCodedPocSelectedRefQp;

	// Dynamic GOP
	EB_U32								 previousMiniGopHierarchicalLevels;

	EbObjectWrapper_t                   *previousPictureControlSetWrapperPtr;
    EB_HANDLE						     sharedReferenceMutex;


} EncodeContext_t;

typedef struct EncodeContextInitData_s {
    int junk;
} EncodeContextInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EncodeContextCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);
    
#ifdef __cplusplus
}
#endif
#endif // EbEncodeContext_h