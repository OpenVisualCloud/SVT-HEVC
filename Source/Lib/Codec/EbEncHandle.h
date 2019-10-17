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

#include "EbResourceCoordinationResults.h"
#include "EbPictureDemuxResults.h"
#include "EbRateControlResults.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Component Private Data
 **************************************/  
typedef struct EbEncHandle_s
{
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
        
    // Contexts
    EB_PTR                                  resourceCoordinationContextPtr;
    EB_PTR                                  pictureEnhancementContextPtr;
    EB_PTR                                 *pictureAnalysisContextPtrArray;
    EB_PTR                                  pictureDecisionContextPtr;
    EB_PTR                                 *motionEstimationContextPtrArray;
    EB_PTR                                  initialRateControlContextPtr;
	EB_PTR                                 *sourceBasedOperationsContextPtrArray;
    EB_PTR                                  pictureManagerContextPtr;
    EB_PTR                                  rateControlContextPtr;
    EB_PTR                                 *modeDecisionConfigurationContextPtrArray;
    EB_PTR                                 *encDecContextPtrArray;
    EB_PTR                                 *entropyCodingContextPtrArray;
    EB_PTR                                  packetizationContextPtr;
    
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
                                                   
    // Callbacks
    EbCallback_t                          **appCallbackPtrArray;
        
    // Input Video Ports
    EB_PARAM_PORTDEFINITIONTYPE           **inputVideoPortPtrArray;
        
    // Output Bitstream Port
    EB_PARAM_PORTDEFINITIONTYPE           **outputStreamPortPtrArray;
    
    // Output Recon Video Port
    // *Note - Recon Video buffers will have distortion data appended to 
    // the end of the buffer.  This distortion data aids in the calculation
    // of PSNR.
    // Memory Map
    EbMemoryMapEntry                       *memoryMap; 
    EB_U32                                  memoryMapIndex;
    EB_U64                                  totalLibMemory;

} EbEncHandle_t;

/**************************************
 * EB_BUFFERHEADERTYPE Constructor
 **************************************/  
extern EB_ERRORTYPE EbInputBufferHeaderCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

extern EB_ERRORTYPE EbOutputBufferHeaderCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);

extern EB_ERRORTYPE EbOutputReconBufferHeaderCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif // EbEncHandle_h