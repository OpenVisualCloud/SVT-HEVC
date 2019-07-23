/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbEncodeContext.h"
#include "EbPictureManagerQueue.h"
#include "EbCabacContextModel.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

EB_ERRORTYPE EncodeContextCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr)
{
    EB_U32 pictureIndex;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    
    EncodeContext_t *encodeContextPtr;
    EB_MALLOC(EncodeContext_t*, encodeContextPtr, sizeof(EncodeContext_t), EB_N_PTR);
    *objectDblPtr = (EB_PTR) encodeContextPtr;
    
    objectInitDataPtr = 0;
    CHECK_REPORT_ERROR(
	    (objectInitDataPtr == 0),
	    encodeContextPtr->appCallbackPtr, 
	    EB_ENC_EC_ERROR29);

    // Callback Functions
    encodeContextPtr->appCallbackPtr                                    = (EbCallback_t*) EB_NULL;
    
    // Port Active State
    EB_CREATEMUTEX(EB_HANDLE, encodeContextPtr->terminatingConditionsMutex, sizeof(EB_HANDLE), EB_MUTEX);

    encodeContextPtr->totalNumberOfReconFrames                          = 0;
	
	
    // Output Buffer Fifos
    encodeContextPtr->streamOutputFifoPtr                            = (EbFifo_t*) EB_NULL;
    
    // Picture Buffer Fifos
    encodeContextPtr->inputPicturePoolFifoPtr                           = (EbFifo_t*) EB_NULL;
    encodeContextPtr->referencePicturePoolFifoPtr                       = (EbFifo_t*) EB_NULL;    
    encodeContextPtr->paReferencePicturePoolFifoPtr                     = (EbFifo_t*) EB_NULL;    
    
    // Picture Decision Reordering Queue
    encodeContextPtr->pictureDecisionReorderQueueHeadIndex                 = 0;
    EB_MALLOC(PictureDecisionReorderEntry_t**, encodeContextPtr->pictureDecisionReorderQueue, sizeof(PictureDecisionReorderEntry_t*) * PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);
        
    for(pictureIndex=0; pictureIndex < PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
		return_error = PictureDecisionReorderEntryCtor(
            &(encodeContextPtr->pictureDecisionReorderQueue[pictureIndex]),
            pictureIndex);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
	// Picture Manager Reordering Queue
	encodeContextPtr->pictureManagerReorderQueueHeadIndex = 0;
	EB_MALLOC(PictureManagerReorderEntry_t**, encodeContextPtr->pictureManagerReorderQueue, sizeof(PictureManagerReorderEntry_t*) * PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);

	for (pictureIndex = 0; pictureIndex < PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
		return_error = PictureManagerReorderEntryCtor(
			&(encodeContextPtr->pictureManagerReorderQueue[pictureIndex]),
			pictureIndex);
		if (return_error == EB_ErrorInsufficientResources){
			return EB_ErrorInsufficientResources;
		}
	}


    // Picture Manager Pre-Assignment Buffer
    encodeContextPtr->preAssignmentBufferIntraCount                     = 0;
    encodeContextPtr->preAssignmentBufferIdrCount                       = 0;
    encodeContextPtr->preAssignmentBufferSceneChangeCount               = 0;
    encodeContextPtr->preAssignmentBufferSceneChangeIndex               = 0;
    encodeContextPtr->preAssignmentBufferEosFlag                        = EB_FALSE;
    encodeContextPtr->decodeBaseNumber                                  = 0;

    encodeContextPtr->preAssignmentBufferCount                          = 0;
    encodeContextPtr->numberOfActivePictures                            = 0;

    EB_MALLOC(EbObjectWrapper_t**, encodeContextPtr->preAssignmentBuffer, sizeof(EbObjectWrapper_t*) * PRE_ASSIGNMENT_MAX_DEPTH, EB_N_PTR);
    
    for(pictureIndex=0; pictureIndex < PRE_ASSIGNMENT_MAX_DEPTH; ++pictureIndex) {
        encodeContextPtr->preAssignmentBuffer[pictureIndex] = (EbObjectWrapper_t*) EB_NULL;
    }

    // Picture Manager Input Queue
    encodeContextPtr->inputPictureQueueHeadIndex                        = 0;
    encodeContextPtr->inputPictureQueueTailIndex                        = 0;
    EB_MALLOC(InputQueueEntry_t**, encodeContextPtr->inputPictureQueue, sizeof(InputQueueEntry_t*) * INPUT_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(pictureIndex=0; pictureIndex < INPUT_QUEUE_MAX_DEPTH; ++pictureIndex) {
        return_error = InputQueueEntryCtor(
            &(encodeContextPtr->inputPictureQueue[pictureIndex]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Picture Manager Reference Queue
    encodeContextPtr->referencePictureQueueHeadIndex                    = 0;
    encodeContextPtr->referencePictureQueueTailIndex                    = 0;
    EB_MALLOC(ReferenceQueueEntry_t**, encodeContextPtr->referencePictureQueue, sizeof(ReferenceQueueEntry_t*) * REFERENCE_QUEUE_MAX_DEPTH, EB_N_PTR);
    
    for(pictureIndex=0; pictureIndex < REFERENCE_QUEUE_MAX_DEPTH; ++pictureIndex) {
        return_error = ReferenceQueueEntryCtor(
            &(encodeContextPtr->referencePictureQueue[pictureIndex]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Picture Decision PA Reference Queue
    encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex                     = 0;
    encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex                     = 0;
    EB_MALLOC(PaReferenceQueueEntry_t**, encodeContextPtr->pictureDecisionPaReferenceQueue, sizeof(PaReferenceQueueEntry_t*) * PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH, EB_N_PTR);

    for(pictureIndex=0; pictureIndex < PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH; ++pictureIndex) {
		return_error = PaReferenceQueueEntryCtor(
            &(encodeContextPtr->pictureDecisionPaReferenceQueue[pictureIndex]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Initial Rate Control Reordering Queue
    encodeContextPtr->initialRateControlReorderQueueHeadIndex                 = 0;
    EB_MALLOC(InitialRateControlReorderEntry_t**, encodeContextPtr->initialRateControlReorderQueue, sizeof(InitialRateControlReorderEntry_t*) * INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);

	for(pictureIndex=0; pictureIndex < INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
        return_error = InitialRateControlReorderEntryCtor(
            &(encodeContextPtr->initialRateControlReorderQueue[pictureIndex]),
            pictureIndex);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // High level Rate Control histogram Queue
    encodeContextPtr->hlRateControlHistorgramQueueHeadIndex                 = 0;
    
    EB_CALLOC(HlRateControlHistogramEntry_t**, encodeContextPtr->hlRateControlHistorgramQueue, sizeof(HlRateControlHistogramEntry_t*) * HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH, 1, EB_N_PTR);

    for(pictureIndex=0; pictureIndex < HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH; ++pictureIndex) {
        return_error = HlRateControlHistogramEntryCtor(
            &(encodeContextPtr->hlRateControlHistorgramQueue[pictureIndex]),
            pictureIndex);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    // HLRateControl Historgram Queue Mutex
    EB_CREATEMUTEX(EB_HANDLE, encodeContextPtr->hlRateControlHistorgramQueueMutex, sizeof(EB_HANDLE), EB_MUTEX);

    // Packetization Reordering Queue
    encodeContextPtr->packetizationReorderQueueHeadIndex                 = 0;
    EB_MALLOC(PacketizationReorderEntry_t**, encodeContextPtr->packetizationReorderQueue, sizeof(PacketizationReorderEntry_t*) * PACKETIZATION_REORDER_QUEUE_MAX_DEPTH, EB_N_PTR);
    
    for(pictureIndex=0; pictureIndex < PACKETIZATION_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
        return_error = PacketizationReorderEntryCtor(
            &(encodeContextPtr->packetizationReorderQueue[pictureIndex]),
            pictureIndex);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    encodeContextPtr->intraPeriodPosition                               = 0;
    encodeContextPtr->predStructPosition                                = 0;
    encodeContextPtr->currentInputPoc                                   = -1;
    encodeContextPtr->elapsedNonIdrCount                                = 0;
    encodeContextPtr->elapsedNonCraCount                                = 0;
    encodeContextPtr->initialPicture                                    = EB_TRUE;

    encodeContextPtr->lastIdrPicture                                    = 0;
    // Sequence Termination Flags
    encodeContextPtr->terminatingPictureNumber                          = ~0u;
    encodeContextPtr->terminatingSequenceFlagReceived                   = EB_FALSE;

    // Prediction Structure Group
    encodeContextPtr->predictionStructureGroupPtr                       = (PredictionStructureGroup_t*) EB_NULL;

    // Cabac Context Model Array
    EB_MALLOC(ContextModelEncContext_t*, encodeContextPtr->cabacContextModelArray, sizeof(ContextModelEncContext_t) * TOTAL_NUMBER_OF_CABAC_CONTEXT_MODEL_BUFFERS, EB_N_PTR);

    return_error = EncodeCabacContextModelCtor(encodeContextPtr->cabacContextModelArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // MD Rate Estimation Array
    EB_MALLOC(MdRateEstimationContext_t*, encodeContextPtr->mdRateEstimationArray, sizeof(MdRateEstimationContext_t) * TOTAL_NUMBER_OF_MD_RATE_ESTIMATION_CASE_BUFFERS, EB_N_PTR);

    return_error = MdRateEstimationContextCtor(encodeContextPtr->mdRateEstimationArray, encodeContextPtr->cabacContextModelArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Rate Control
    encodeContextPtr->availableTargetBitRate                            = 10000000;
    encodeContextPtr->availableTargetBitRateChanged                     = EB_FALSE;
    encodeContextPtr->bufferFill                                        = 0;
    encodeContextPtr->vbvBufsize                                        = 0;
    encodeContextPtr->vbvMaxrate                                        = 0;
    encodeContextPtr->fillerBitError                                    = 0;

    // Rate Control Bit Tables
    EB_MALLOC(RateControlTables_t*, encodeContextPtr->rateControlTablesArray, sizeof(RateControlTables_t) * TOTAL_NUMBER_OF_INITIAL_RC_TABLES_ENTRY, EB_N_PTR);

    return_error = RateControlTablesCtor(encodeContextPtr->rateControlTablesArray);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // RC Rate Table Update Mutex
    EB_CREATEMUTEX(EB_HANDLE, encodeContextPtr->rateTableUpdateMutex, sizeof(EB_HANDLE), EB_MUTEX);

    encodeContextPtr->rateControlTablesArrayUpdated = EB_FALSE;

    EB_CREATEMUTEX(EB_HANDLE, encodeContextPtr->scBufferMutex, sizeof(EB_HANDLE), EB_MUTEX);
    encodeContextPtr->scBuffer      = 0;
    encodeContextPtr->scFrameIn     = 0;
    encodeContextPtr->scFrameOut    = 0;
    encodeContextPtr->encMode = SPEED_CONTROL_INIT_MOD;

    EB_CREATEMUTEX(EB_HANDLE, encodeContextPtr->bufferFillMutex, sizeof(EB_HANDLE), EB_MUTEX);
    encodeContextPtr->previousSelectedRefQp = 32;
    encodeContextPtr->maxCodedPoc = 0;
    encodeContextPtr->maxCodedPocSelectedRefQp = 32;

     encodeContextPtr->sharedReferenceMutex = EbCreateMutex();
	if (encodeContextPtr->sharedReferenceMutex  == (EB_HANDLE) EB_NULL){
        return EB_ErrorInsufficientResources;
    }else {
        memoryMap[*(memoryMapIndex)].ptrType          = EB_MUTEX;
        memoryMap[(*(memoryMapIndex))++].ptr          = encodeContextPtr->sharedReferenceMutex ;
        *totalLibMemory                              += (sizeof(EB_HANDLE));    
    }


    return EB_ErrorNone;
}

