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

static void EncodeContextDctor(EB_PTR p)
{
    EncodeContext_t* obj = (EncodeContext_t*)p;
    EB_DESTROY_MUTEX(obj->terminatingConditionsMutex);
    EB_DESTROY_MUTEX(obj->hlRateControlHistorgramQueueMutex);
    EB_DESTROY_MUTEX(obj->rateTableUpdateMutex);
    EB_DESTROY_MUTEX(obj->scBufferMutex);
    EB_DESTROY_MUTEX(obj->bufferFillMutex);
    EB_DESTROY_MUTEX(obj->sharedReferenceMutex);

    EB_DELETE(obj->predictionStructureGroupPtr);
    EB_DELETE_PTR_ARRAY(obj->pictureDecisionReorderQueue, PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH);
    EB_DELETE_PTR_ARRAY(obj->pictureManagerReorderQueue, PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH);
    EB_FREE(obj->preAssignmentBuffer);
    EB_DELETE_PTR_ARRAY(obj->inputPictureQueue, INPUT_QUEUE_MAX_DEPTH);
    EB_DELETE_PTR_ARRAY(obj->referencePictureQueue, REFERENCE_QUEUE_MAX_DEPTH);
    EB_DELETE_PTR_ARRAY(obj->pictureDecisionPaReferenceQueue, PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH);
    EB_DELETE_PTR_ARRAY(obj->initialRateControlReorderQueue, INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH);
    EB_DELETE_PTR_ARRAY(obj->hlRateControlHistorgramQueue, HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH);
    EB_DELETE_PTR_ARRAY(obj->packetizationReorderQueue, PACKETIZATION_REORDER_QUEUE_MAX_DEPTH);
    EB_FREE_ARRAY(obj->cabacContextModelArray);
    EB_FREE(obj->mdRateEstimationArray);
    EB_FREE_ARRAY(obj->rateControlTablesArray);
}

EB_ERRORTYPE EncodeContextCtor(
    EncodeContext_t *encodeContextPtr,
    EB_PTR objectInitDataPtr)
{
    EB_U32 pictureIndex;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    
    encodeContextPtr->dctor = EncodeContextDctor;
    
    objectInitDataPtr = 0;
    CHECK_REPORT_ERROR(
	    (objectInitDataPtr == 0),
	    encodeContextPtr->appCallbackPtr, 
	    EB_ENC_EC_ERROR29);

    // Port Active State
    EB_CREATE_MUTEX(encodeContextPtr->terminatingConditionsMutex);

    // Picture Decision Reordering Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->pictureDecisionReorderQueue, PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->pictureDecisionReorderQueue[pictureIndex],
            PictureDecisionReorderEntryCtor,
            pictureIndex);
    }

    // Picture Manager Reordering Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->pictureManagerReorderQueue, PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->pictureManagerReorderQueue[pictureIndex],
            PictureManagerReorderEntryCtor,
            pictureIndex);
    }
    // Picture Manager Pre-Assignment Buffer
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->preAssignmentBuffer, PRE_ASSIGNMENT_MAX_DEPTH);

    // Picture Manager Input Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->inputPictureQueue, INPUT_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < INPUT_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->inputPictureQueue[pictureIndex],
            InputQueueEntryCtor);
    }
    // Picture Manager Reference Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->referencePictureQueue, REFERENCE_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < REFERENCE_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->referencePictureQueue[pictureIndex],
            ReferenceQueueEntryCtor);
    }

    // Picture Decision PA Reference Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->pictureDecisionPaReferenceQueue, PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->pictureDecisionPaReferenceQueue[pictureIndex],
            PaReferenceQueueEntryCtor);
    }

    // Initial Rate Control Reordering Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->initialRateControlReorderQueue, INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->initialRateControlReorderQueue[pictureIndex],
            InitialRateControlReorderEntryCtor,
            pictureIndex);
    }

    // High level Rate Control histogram Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->hlRateControlHistorgramQueue, HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->hlRateControlHistorgramQueue[pictureIndex],
            HlRateControlHistogramEntryCtor,
            pictureIndex);
    }

    // HLRateControl Historgram Queue Mutex
    EB_CREATE_MUTEX(encodeContextPtr->hlRateControlHistorgramQueueMutex);

    // Packetization Reordering Queue
    EB_ALLOC_PTR_ARRAY(encodeContextPtr->packetizationReorderQueue, PACKETIZATION_REORDER_QUEUE_MAX_DEPTH);
    for (pictureIndex = 0; pictureIndex < PACKETIZATION_REORDER_QUEUE_MAX_DEPTH; ++pictureIndex) {
        EB_NEW(
            encodeContextPtr->packetizationReorderQueue[pictureIndex],
            PacketizationReorderEntryCtor,
            pictureIndex);
    }

    encodeContextPtr->currentInputPoc                                   = -1;
    encodeContextPtr->initialPicture                                    = EB_TRUE;

    // Sequence Termination Flags
    encodeContextPtr->terminatingPictureNumber                          = ~0u;

    // Cabac Context Model Array
    EB_MALLOC_ARRAY(encodeContextPtr->cabacContextModelArray, TOTAL_NUMBER_OF_CABAC_CONTEXT_MODEL_BUFFERS);
    EncodeCabacContextModelInit(encodeContextPtr->cabacContextModelArray);

    // MD Rate Estimation Array
    EB_CALLOC(encodeContextPtr->mdRateEstimationArray, TOTAL_NUMBER_OF_MD_RATE_ESTIMATION_CASE_BUFFERS, sizeof(MdRateEstimationContext_t));
    MdRateEstimationContextInit(encodeContextPtr->mdRateEstimationArray, encodeContextPtr->cabacContextModelArray);


    // Rate Control
    encodeContextPtr->availableTargetBitRate                            = 10000000;

    // Rate Control Bit Tables
    EB_MALLOC_ARRAY(encodeContextPtr->rateControlTablesArray, TOTAL_NUMBER_OF_INITIAL_RC_TABLES_ENTRY);
    RateControlTablesInit(encodeContextPtr->rateControlTablesArray);

    // RC Rate Table Update Mutex
    EB_CREATE_MUTEX(encodeContextPtr->rateTableUpdateMutex);
    EB_CREATE_MUTEX(encodeContextPtr->scBufferMutex);

    encodeContextPtr->encMode = SPEED_CONTROL_INIT_MOD;
    EB_CREATE_MUTEX(encodeContextPtr->bufferFillMutex);
    encodeContextPtr->previousSelectedRefQp = 32;
    encodeContextPtr->maxCodedPocSelectedRefQp = 32;
    EB_CREATE_MUTEX(encodeContextPtr->sharedReferenceMutex);

    return EB_ErrorNone;
}

