/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncHandle_h
#define EbEncHandle_h

#include "EbDefinitions.h"
#include "EbApi.h"

#include "EbPictureBufferDesc.h"
#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"
#include "EbResourceCoordinationProcess.h"
#include "EbPictureAnalysisProcess.h"
#include "EbResourceCoordinationResults.h"
#include "EbPictureDemuxResults.h"
#include "EbRateControlResults.h"
#include "EbPictureDecisionProcess.h"
#include "EbMotionEstimationProcess.h"
#include "EbInitialRateControlProcess.h"
#include "EbSourceBasedOperationsProcess.h"
#include "EbPictureManagerProcess.h"
#include "EbRateControlProcess.h"
#include "EbModeDecisionConfigurationProcess.h"
#include "EbEncDecProcess.h"
#include "EbEntropyCodingProcess.h"
#include "EbPacketizationProcess.h"
#include "EbUnPackProcess.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Component Private Data
 **************************************/  
typedef struct EbEncHandle_s
{
    EbDctor                                 dctor;
    // Encode Instances & Compute Segments
    EB_U32                                  encodeInstanceTotalCount;
    EB_U32                                 *computeSegmentsTotalCountArray;
    
    // Config Set Counts
    EB_U32                                  sequenceControlSetPoolTotalCount; 
    
    // Full Results Count
    EB_U32                                  pictureControlSetPoolTotalCount;   
        
    // Picture Buffer Counts
    EB_U32                                  reconPicturePoolTotalCount;
    EB_U32                                  coeffPicturePoolTotalCount;
    EB_U32                                  referencePicturePoolTotalCount;
    EB_U32                                  paReferencePicturPooleBufferInitCount;
    
    // Config Set Pool & Active Array
    EbSystemResource_t                     *sequenceControlSetPoolPtr;
    EbFifo_t                              **sequenceControlSetPoolProducerFifoPtrArray;
    EbSequenceControlSetInstance_t        **sequenceControlSetInstanceArray;
    
    // Full Results 
    EbSystemResource_t                    **pictureControlSetPoolPtrArray;
    EbFifo_t                             ***pictureControlSetPoolProducerFifoPtrDblArray;

    //ParentControlSet
    EbSystemResource_t                    **pictureParentControlSetPoolPtrArray;
    EbFifo_t                             ***pictureParentControlSetPoolProducerFifoPtrDblArray;
        
    // Picture Buffers
    EbSystemResource_t                    **referencePicturePoolPtrArray;
    EbSystemResource_t                    **paReferencePicturePoolPtrArray;
    
    // Picture Buffer Producer Fifos   
    EbFifo_t                             ***referencePicturePoolProducerFifoPtrDblArray;
    EbFifo_t                             ***paReferencePicturePoolProducerFifoPtrDblArray;
    
    // Thread Handles
    EB_HANDLE                               resourceCoordinationThreadHandle;
    EB_HANDLE                               pictureEnhancementThreadHandle;
    EB_HANDLE                              *pictureAnalysisThreadHandleArray;
    EB_HANDLE                               pictureDecisionThreadHandle;
    EB_HANDLE                              *motionEstimationThreadHandleArray;
    EB_HANDLE                               initialRateControlThreadHandle;
	EB_HANDLE                              *sourceBasedOperationsThreadHandleArray;
    EB_HANDLE                               pictureManagerThreadHandle;
    EB_HANDLE                               rateControlThreadHandle;
    EB_HANDLE                              *modeDecisionConfigurationThreadHandleArray;
    EB_HANDLE                              *encDecThreadHandleArray;
    EB_HANDLE                              *entropyCodingThreadHandleArray;
    EB_HANDLE                               packetizationThreadHandle;
    EB_HANDLE                              *unpackThreadHandleArray;

    // Contexts
    ResourceCoordinationContext_t          *resourceCoordinationContextPtr;
    PictureAnalysisContext_t              **pictureAnalysisContextPtrArray;
    PictureDecisionContext_t               *pictureDecisionContextPtr;
    MotionEstimationContext_t             **motionEstimationContextPtrArray;
    InitialRateControlContext_t            *initialRateControlContextPtr;
    SourceBasedOperationsContext_t        **sourceBasedOperationsContextPtrArray;
    PictureManagerContext_t                *pictureManagerContextPtr;
    RateControlContext_t                   *rateControlContextPtr;
    ModeDecisionConfigurationContext_t    **modeDecisionConfigurationContextPtrArray;
    EncDecContext_t                       **encDecContextPtrArray;
    EntropyCodingContext_t                **entropyCodingContextPtrArray;
    PacketizationContext_t                 *packetizationContextPtr;
    UnPackContext_t                       **unpackContextPtrArray;

    // System Resource Managers
    EbSystemResource_t                     *inputBufferResourcePtr;
    EbSystemResource_t                    **outputStreamBufferResourcePtrArray;
    EbSystemResource_t                    **outputReconBufferResourcePtrArray;
    EbSystemResource_t                     *resourceCoordinationResultsResourcePtr;
    EbSystemResource_t                     *pictureAnalysisResultsResourcePtr;
    EbSystemResource_t                     *pictureDecisionResultsResourcePtr;
    EbSystemResource_t                     *motionEstimationResultsResourcePtr;
	EbSystemResource_t                     *initialRateControlResultsResourcePtr;
    EbSystemResource_t                     *pictureDemuxResultsResourcePtr;
    EbSystemResource_t                     *rateControlTasksResourcePtr;
    EbSystemResource_t                     *rateControlResultsResourcePtr;
    EbSystemResource_t                     *encDecTasksResourcePtr;
    EbSystemResource_t                     *encDecResultsResourcePtr;
    EbSystemResource_t                     *entropyCodingResultsResourcePtr;
    EbSystemResource_t                     *unpackTasksResourcePtr;
    EbSystemResource_t                     *unpackSyncResourcePtr;
    
    // Inter-Process Producer Fifos
    EbFifo_t                              **inputBufferProducerFifoPtrArray;
    EbFifo_t                             ***outputStreamBufferProducerFifoPtrDblArray;
    EbFifo_t                             ***outputReconBufferProducerFifoPtrDblArray;
    EbFifo_t                              **resourceCoordinationResultsProducerFifoPtrArray;
    EbFifo_t                              **pictureAnalysisResultsProducerFifoPtrArray;
    EbFifo_t                              **pictureDecisionResultsProducerFifoPtrArray;
    EbFifo_t                              **motionEstimationResultsProducerFifoPtrArray;
    EbFifo_t                              **initialRateControlResultsProducerFifoPtrArray;
    EbFifo_t                              **pictureDemuxResultsProducerFifoPtrArray;
    EbFifo_t                              **pictureManagerResultsProducerFifoPtrArray;
    EbFifo_t                              **rateControlTasksProducerFifoPtrArray;
    EbFifo_t                              **rateControlResultsProducerFifoPtrArray;
    EbFifo_t                              **encDecTasksProducerFifoPtrArray;
    EbFifo_t                              **encDecResultsProducerFifoPtrArray;
    EbFifo_t                              **entropyCodingResultsProducerFifoPtrArray;
    EbFifo_t                              **unpackTasksProducerFifoPtrArray;
    EbFifo_t                              **unpackSyncProducerFifoPtrArray;
    
    // Inter-Process Consumer Fifos
    EbFifo_t                              **inputBufferConsumerFifoPtrArray;
    EbFifo_t                             ***outputStreamBufferConsumerFifoPtrDblArray;
    EbFifo_t                             ***outputReconBufferConsumerFifoPtrDblArray;
    EbFifo_t                              **resourceCoordinationResultsConsumerFifoPtrArray;
    EbFifo_t                              **pictureAnalysisResultsConsumerFifoPtrArray;
    EbFifo_t                              **pictureDecisionResultsConsumerFifoPtrArray;
    EbFifo_t                              **motionEstimationResultsConsumerFifoPtrArray;
    EbFifo_t                              **initialRateControlResultsConsumerFifoPtrArray;
    EbFifo_t                              **pictureDemuxResultsConsumerFifoPtrArray;
    EbFifo_t                              **rateControlTasksConsumerFifoPtrArray;
    EbFifo_t                              **rateControlResultsConsumerFifoPtrArray;
    EbFifo_t                              **encDecTasksConsumerFifoPtrArray;
    EbFifo_t                              **encDecResultsConsumerFifoPtrArray;
    EbFifo_t                              **entropyCodingResultsConsumerFifoPtrArray;
    EbFifo_t                              **unpackTasksConsumerFifoPtrArray;
    EbFifo_t                              **unpackSyncConsumerFifoPtrArray;
    // Callbacks
    EbCallback_t                          **appCallbackPtrArray;

    // Output Recon Video Port
    // *Note - Recon Video buffers will have distortion data appended to 
    // the end of the buffer.  This distortion data aids in the calculation
    // of PSNR.
} EbEncHandle_t;

/**************************************
 * EB_BUFFERHEADERTYPE Constructor/Destoryer
 **************************************/
EB_ERRORTYPE EbInputBufferHeaderCreator(
     EB_PTR *objectDblPtr,
     EB_PTR objectInitDataPtr);

EB_ERRORTYPE EbOutputBufferHeaderCreator(
     EB_PTR *objectDblPtr,
     EB_PTR objectInitDataPtr);

EB_ERRORTYPE EbOutputReconBufferHeaderCreator(
     EB_PTR *objectDblPtr,
     EB_PTR objectInitDataPtr);

void EbInputBufferHeaderDestroyer(EB_PTR p);
void EbOutputBufferHeaderDestroyer(EB_PTR p);
void EbOutputReconBufferHeaderDestroyer(EB_PTR p);

#ifdef __cplusplus
}
#endif
#endif // EbEncHandle_h