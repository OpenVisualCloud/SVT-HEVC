/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlProcess.h"
#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"
#include "EbUtility.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

#include "EbRateControlResults.h"
#include "EbRateControlTasks.h"

/*****************************
* Internal Typedefs
*****************************/
void RateControlLayerReset(
    RateControlLayerContext_t   *rateControlLayerPtr,
    PictureControlSet_t         *pictureControlSetPtr,
    RateControlContext_t        *rateControlContextPtr,
    EB_U32                       pictureAreaInPixel,
    EB_BOOL                      wasUsed)
{

    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
    EB_U32                sliceNum;
    EB_U32                temporalLayerIndex;
    EB_U64                totalFrameInInterval;
    EB_U64                sumBitsPerSw = 0;

    rateControlLayerPtr->targetBitRate = pictureControlSetPtr->ParentPcsPtr->targetBitRate*(EB_U64)RATE_PERCENTAGE_LAYER_ARRAY[sequenceControlSetPtr->staticConfig.hierarchicalLevels][rateControlLayerPtr->temporalIndex] / 100;
    // update this based on temporal layers
    rateControlLayerPtr->frameRate = sequenceControlSetPtr->frameRate;

    totalFrameInInterval = sequenceControlSetPtr->staticConfig.intraPeriodLength + 1;

    if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0 && sequenceControlSetPtr->intraPeriodLength != -1){
        if (pictureControlSetPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0){
            totalFrameInInterval = 0;
            for (temporalLayerIndex = 0; temporalLayerIndex< EB_MAX_TEMPORAL_LAYERS; temporalLayerIndex++){
                rateControlContextPtr->framesInInterval[temporalLayerIndex] = pictureControlSetPtr->ParentPcsPtr->framesInInterval[temporalLayerIndex];
                totalFrameInInterval += pictureControlSetPtr->ParentPcsPtr->framesInInterval[temporalLayerIndex];
                sumBitsPerSw += pictureControlSetPtr->ParentPcsPtr->bitsPerSwPerLayer[temporalLayerIndex];
            }
#if ADAPTIVE_PERCENTAGE
            rateControlLayerPtr->targetBitRate = pictureControlSetPtr->ParentPcsPtr->targetBitRate* pictureControlSetPtr->ParentPcsPtr->bitsPerSwPerLayer[rateControlLayerPtr->temporalIndex] / sumBitsPerSw;
#endif
        }
    }

    if (sequenceControlSetPtr->staticConfig.intraPeriodLength != -1){
        rateControlLayerPtr->frameRate = sequenceControlSetPtr->frameRate * rateControlContextPtr->framesInInterval[rateControlLayerPtr->temporalIndex] / totalFrameInInterval;
    }
    else{
        switch (pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels) {
        case 0:
            break;

        case 1:
            if (sequenceControlSetPtr->staticConfig.intraPeriodLength == -1){
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> 1;
            }

            break;

        case 2:
            if (rateControlLayerPtr->temporalIndex == 0){
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> 2;
            }
            else{
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> (3 - rateControlLayerPtr->temporalIndex);
            }
            break;

        case 3:
            if (rateControlLayerPtr->temporalIndex == 0){
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> 3;
            }
            else {
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> (4 - rateControlLayerPtr->temporalIndex);
            }

            break;
        case 4:
            if (rateControlLayerPtr->temporalIndex == 0){
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> 4;
            }
            else {
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> (5 - rateControlLayerPtr->temporalIndex);
            }

            break;
        case 5:
            if (rateControlLayerPtr->temporalIndex == 0){
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> 5;
            }
            else {
                rateControlLayerPtr->frameRate = rateControlLayerPtr->frameRate >> (6 - rateControlLayerPtr->temporalIndex);
            }

            break;

        default:
            break;
        }
    }

    rateControlLayerPtr->coeffAveragingWeight1 = 5;

    rateControlLayerPtr->coeffAveragingWeight2 = 16 - rateControlLayerPtr->coeffAveragingWeight1;
    if (rateControlLayerPtr->frameRate == 0){ // no frame in that layer
        rateControlLayerPtr->frameRate = 1 << RC_PRECISION;
    }
    rateControlLayerPtr->channelBitRate = (((rateControlLayerPtr->targetBitRate << (2 * RC_PRECISION)) / rateControlLayerPtr->frameRate) + RC_PRECISION_OFFSET) >> RC_PRECISION;
    rateControlLayerPtr->channelBitRate = (EB_U64)MAX((EB_S64)1, (EB_S64)rateControlLayerPtr->channelBitRate);
    rateControlLayerPtr->ecBitConstraint = rateControlLayerPtr->channelBitRate;


    // This is only for the initial frame, because the feedback is from packetization now and all of these are considered
    // considering the bits for slice header
    // *Note - only one-slice-per picture is supported for UHD
    sliceNum = 1;

    rateControlLayerPtr->ecBitConstraint -= SLICE_HEADER_BITS_NUM*sliceNum;

    rateControlLayerPtr->ecBitConstraint = MAX(1, rateControlLayerPtr->ecBitConstraint);

    rateControlLayerPtr->previousBitConstraint = rateControlLayerPtr->channelBitRate;
    rateControlLayerPtr->bitConstraint = rateControlLayerPtr->channelBitRate;
    rateControlLayerPtr->difTotalAndEcBits = 0;

    rateControlLayerPtr->frameSameSADMinQpCount = 0;
    rateControlLayerPtr->maxQp = pictureControlSetPtr->pictureQp;

    rateControlLayerPtr->alpha = 1 << (RC_PRECISION - 1);
    {
        if (!wasUsed){


            rateControlLayerPtr->sameSADCount = 0;

            rateControlLayerPtr->kCoeff = 3 << RC_PRECISION;
            rateControlLayerPtr->previousKCoeff = 3 << RC_PRECISION;

            rateControlLayerPtr->cCoeff = (rateControlLayerPtr->channelBitRate << (2 * RC_PRECISION)) / pictureAreaInPixel / CCOEFF_INIT_FACT;
            rateControlLayerPtr->previousCCoeff = (rateControlLayerPtr->channelBitRate << (2 * RC_PRECISION)) / pictureAreaInPixel / CCOEFF_INIT_FACT;

            // These are for handling Pred structure 2, when for higher temporal layer, frames can arrive in different orders
            // They should be modifed in a way that gets these from previous layers
            rateControlLayerPtr->previousFrameQp = 32;
            rateControlLayerPtr->previousFrameBitActual = 1200;
            rateControlLayerPtr->previousFrameQuantizedCoeffBitActual = 1000;
            rateControlLayerPtr->previousFrameSadMe = 10000000;
            pictureControlSetPtr->ParentPcsPtr->targetBits = 10000;
			rateControlLayerPtr->previousFrameQp = pictureControlSetPtr->pictureQp;
            rateControlLayerPtr->deltaQpFraction = 0;
			rateControlLayerPtr->previousFrameAverageQp = pictureControlSetPtr->pictureQp;
			rateControlLayerPtr->previousCalculatedFrameQp = pictureControlSetPtr->pictureQp; 
			rateControlLayerPtr->calculatedFrameQp = pictureControlSetPtr->pictureQp; 
            rateControlLayerPtr->criticalStates = 0;
        }
        else{
            rateControlLayerPtr->sameSADCount = 0;
            rateControlLayerPtr->criticalStates = 0;
        }
    }
}


void RateControlLayerResetPart2(
    RateControlLayerContext_t   *rateControlLayerPtr,
    PictureControlSet_t         *pictureControlSetPtr)
{

    // update this based on temporal layers
    rateControlLayerPtr->maxQp = (EB_U32)CLIP3(0, 51, (EB_S32)(pictureControlSetPtr->pictureQp + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][rateControlLayerPtr->temporalIndex]));


    // These are for handling Pred structure 2, when for higher temporal layer, frames can arrive in different orders
    // They should be modifed in a way that gets these from previous layers
    rateControlLayerPtr->previousFrameQp = rateControlLayerPtr->maxQp;
    rateControlLayerPtr->previousFrameAverageQp = rateControlLayerPtr->maxQp;
    rateControlLayerPtr->previousCalculatedFrameQp = rateControlLayerPtr->maxQp;
    rateControlLayerPtr->calculatedFrameQp = rateControlLayerPtr->maxQp;
    
}

EB_ERRORTYPE HighLevelRateControlContextCtor(
    HighLevelRateControlContext_t   **entryDblPtr){

    HighLevelRateControlContext_t *entryPtr;
    EB_MALLOC(HighLevelRateControlContext_t*, entryPtr, sizeof(HighLevelRateControlContext_t), EB_N_PTR);
    *entryDblPtr = entryPtr;

    return EB_ErrorNone;
}


EB_ERRORTYPE RateControlLayerContextCtor(
    RateControlLayerContext_t   **entryDblPtr){

    RateControlLayerContext_t *entryPtr;
    EB_MALLOC(RateControlLayerContext_t*, entryPtr, sizeof(RateControlLayerContext_t), EB_N_PTR);

    *entryDblPtr = entryPtr;

    entryPtr->firstFrame = 1;
    entryPtr->firstNonIntraFrame = 1;
    entryPtr->feedbackArrived = EB_FALSE;

    return EB_ErrorNone;
}



EB_ERRORTYPE RateControlIntervalParamContextCtor(
    RateControlIntervalParamContext_t   **entryDblPtr){

    EB_U32 temporalIndex;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    RateControlIntervalParamContext_t *entryPtr;
    EB_MALLOC(RateControlIntervalParamContext_t*, entryPtr, sizeof(RateControlIntervalParamContext_t), EB_N_PTR);

    *entryDblPtr = entryPtr;

    entryPtr->inUse = EB_FALSE;
    entryPtr->wasUsed = EB_FALSE;
    entryPtr->lastGop = EB_FALSE;
    entryPtr->processedFramesNumber = 0;
    EB_MALLOC(RateControlLayerContext_t**, entryPtr->rateControlLayerArray, sizeof(RateControlLayerContext_t*)*EB_MAX_TEMPORAL_LAYERS, EB_N_PTR);

    for (temporalIndex = 0; temporalIndex < EB_MAX_TEMPORAL_LAYERS; temporalIndex++){
        return_error = RateControlLayerContextCtor(&entryPtr->rateControlLayerArray[temporalIndex]);
        entryPtr->rateControlLayerArray[temporalIndex]->temporalIndex = temporalIndex;
        entryPtr->rateControlLayerArray[temporalIndex]->frameRate = 1 << RC_PRECISION;
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    entryPtr->minTargetRateAssigned = EB_FALSE;

    entryPtr->intraFramesQp = 0;
    entryPtr->nextGopIntraFrameQp = 0;
    entryPtr->firstPicPredBits   = 0;
    entryPtr->firstPicActualBits = 0;
    entryPtr->firstPicPredQp     = 0;
    entryPtr->firstPicActualQp   = 0;
    entryPtr->firstPicActualQpAssigned = EB_FALSE;
    entryPtr->sceneChangeInGop = EB_FALSE;
    entryPtr->extraApBitRatioI = 0;

    return EB_ErrorNone;
}

EB_ERRORTYPE RateControlCodedFramesStatsContextCtor(
    CodedFramesStatsEntry_t   **entryDblPtr,
    EB_U64                      pictureNumber){

    CodedFramesStatsEntry_t *entryPtr;
    EB_MALLOC(CodedFramesStatsEntry_t*, entryPtr, sizeof(CodedFramesStatsEntry_t), EB_N_PTR);

    *entryDblPtr = entryPtr;

    entryPtr->pictureNumber = pictureNumber;
    entryPtr->frameTotalBitActual = -1;

    return EB_ErrorNone;
}


EB_ERRORTYPE RateControlContextCtor(
    RateControlContext_t   **contextDblPtr,
    EbFifo_t                *rateControlInputTasksFifoPtr,
    EbFifo_t                *rateControlOutputResultsFifoPtr,
    EB_S32                   intraPeriodLength)
{
    EB_U32 temporalIndex;
    EB_U32 intervalIndex;

#if OVERSHOOT_STAT_PRINT
    EB_U32 pictureIndex;
#endif

    EB_ERRORTYPE return_error = EB_ErrorNone;
    RateControlContext_t *contextPtr;
    EB_MALLOC(RateControlContext_t*, contextPtr, sizeof(RateControlContext_t), EB_N_PTR);

    *contextDblPtr = contextPtr;

    contextPtr->rateControlInputTasksFifoPtr = rateControlInputTasksFifoPtr;
    contextPtr->rateControlOutputResultsFifoPtr = rateControlOutputResultsFifoPtr;

    // High level RC
    return_error = HighLevelRateControlContextCtor(
        &contextPtr->highLevelRateControlPtr);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    for (temporalIndex = 0; temporalIndex < EB_MAX_TEMPORAL_LAYERS; temporalIndex++){
        contextPtr->framesInInterval[temporalIndex] = 0;
    }

    EB_MALLOC(RateControlIntervalParamContext_t**, contextPtr->rateControlParamQueue, sizeof(RateControlIntervalParamContext_t*)*PARALLEL_GOP_MAX_NUMBER, EB_N_PTR);

    contextPtr->rateControlParamQueueHeadIndex = 0;
    for (intervalIndex = 0; intervalIndex < PARALLEL_GOP_MAX_NUMBER; intervalIndex++){
        return_error = RateControlIntervalParamContextCtor(
            &contextPtr->rateControlParamQueue[intervalIndex]);
        contextPtr->rateControlParamQueue[intervalIndex]->firstPoc = (intervalIndex*(EB_U32)(intraPeriodLength + 1));
        contextPtr->rateControlParamQueue[intervalIndex]->lastPoc = ((intervalIndex + 1)*(EB_U32)(intraPeriodLength + 1)) - 1;
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

#if OVERSHOOT_STAT_PRINT
    contextPtr->codedFramesStatQueueHeadIndex = 0;
    contextPtr->codedFramesStatQueueTailIndex = 0;
    EB_MALLOC(CodedFramesStatsEntry_t**, contextPtr->codedFramesStatQueue, sizeof(CodedFramesStatsEntry_t*)*CODED_FRAMES_STAT_QUEUE_MAX_DEPTH, EB_N_PTR);

    for (pictureIndex = 0; pictureIndex < CODED_FRAMES_STAT_QUEUE_MAX_DEPTH; ++pictureIndex) {
        return_error = RateControlCodedFramesStatsContextCtor(
            &contextPtr->codedFramesStatQueue[pictureIndex],
            pictureIndex);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    contextPtr->maxBitActualPerSw = 0;
    contextPtr->maxBitActualPerGop = 0;
#endif

    contextPtr->baseLayerFramesAvgQp = 0;
    contextPtr->baseLayerIntraFramesAvgQp = 0;


    contextPtr->intraCoefRate = 4;
    contextPtr->extraBits = 0;
    contextPtr->extraBitsGen = 0;
    contextPtr->maxRateAdjustDeltaQP = 0;


    return EB_ErrorNone;
}
void HighLevelRcInputPictureMode2(
    PictureParentControlSet_t         *pictureControlSetPtr,
    SequenceControlSet_t              *sequenceControlSetPtr,
    EncodeContext_t                   *encodeContextPtr,
    RateControlContext_t              *contextPtr,
    HighLevelRateControlContext_t     *highLevelRateControlPtr)
{

    EB_BOOL                             endOfSequenceFlag = EB_TRUE;

    HlRateControlHistogramEntry_t      *hlRateControlHistogramPtrTemp;
    // Queue variables
    EB_U32                             queueEntryIndexTemp;
    EB_U32                             queueEntryIndexTemp2;
    EB_U32                             queueEntryIndexHeadTemp;


    EB_U64                              minLaBitDistance;
    EB_U32                              selectedRefQpTableIndex;
    EB_U32                              selectedRefQp;
#if RC_UPDATE_TARGET_RATE
    EB_U32                              selectedOrgRefQp;
#endif
    EB_U32								previousSelectedRefQp = encodeContextPtr->previousSelectedRefQp;
    EB_U64								maxCodedPoc = encodeContextPtr->maxCodedPoc;
    EB_U32								maxCodedPocSelectedRefQp = encodeContextPtr->maxCodedPocSelectedRefQp;


    EB_U32                              refQpIndex;
    EB_U32                              refQpIndexTemp;
    EB_U32                              refQpTableIndex;

    EB_U32                              areaInPixel;
    EB_U32                              numOfFullLcus;
    EB_U32                              qpSearchMin;
    EB_U32                              qpSearchMax;
    EB_S32                              qpStep = 1;
    EB_BOOL                             bestQpFound;
    EB_U32                              temporalLayerIndex;
    EB_BOOL                             tablesUpdated;

    EB_U64                              bitConstraintPerSw= 0;

    RateControlTables_t					*rateControlTablesPtr;
    EB_Bit_Number						*sadBitsArrayPtr;
    EB_Bit_Number						*intraSadBitsArrayPtr;
    EB_U32                               predBitsRefQp;

    for (temporalLayerIndex = 0; temporalLayerIndex< EB_MAX_TEMPORAL_LAYERS; temporalLayerIndex++){
        pictureControlSetPtr->bitsPerSwPerLayer[temporalLayerIndex] = 0;
    }
    pictureControlSetPtr->totalBitsPerGop = 0;

    areaInPixel = sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight;;

    EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->rateTableUpdateMutex);

    tablesUpdated = sequenceControlSetPtr->encodeContextPtr->rateControlTablesArrayUpdated;
    pictureControlSetPtr->percentageUpdated = EB_FALSE;

    if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){

        // Increamenting the head of the hlRateControlHistorgramQueue and clean up the entores
        hlRateControlHistogramPtrTemp = (encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]);
        while ((hlRateControlHistogramPtrTemp->lifeCount == 0) && hlRateControlHistogramPtrTemp->passedToHlrc){

            EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
            // Reset the Reorder Queue Entry
            hlRateControlHistogramPtrTemp->pictureNumber += INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH;
            hlRateControlHistogramPtrTemp->lifeCount = -1;
            hlRateControlHistogramPtrTemp->passedToHlrc = EB_FALSE;
            hlRateControlHistogramPtrTemp->isCoded = EB_FALSE;
            hlRateControlHistogramPtrTemp->totalNumBitsCoded = 0;

            // Increment the Reorder Queue head Ptr
            encodeContextPtr->hlRateControlHistorgramQueueHeadIndex =
                (encodeContextPtr->hlRateControlHistorgramQueueHeadIndex == HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->hlRateControlHistorgramQueueHeadIndex + 1;
            EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
            hlRateControlHistogramPtrTemp = encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex];

        }
        // For the case that number of frames in the sliding window is less than size of the look ahead or intra Refresh. i.e. end of sequence
        if ((pictureControlSetPtr->framesInSw <  MIN(sequenceControlSetPtr->staticConfig.lookAheadDistance + 1, (EB_U32)sequenceControlSetPtr->intraPeriodLength + 1))) {

            selectedRefQp = maxCodedPocSelectedRefQp;

            // Update the QP for the sliding window based on the status of RC    
            if ((contextPtr->extraBitsGen >(EB_S64)(contextPtr->virtualBufferSize << 3))){
                selectedRefQp = (EB_U32)MAX((EB_S32)selectedRefQp -2, 0);
            }
            else if ((contextPtr->extraBitsGen >(EB_S64)(contextPtr->virtualBufferSize << 2))){
                selectedRefQp = (EB_U32)MAX((EB_S32)selectedRefQp - 1, 0);
            }
            if ((contextPtr->extraBitsGen < -(EB_S64)(contextPtr->virtualBufferSize << 2))){
                selectedRefQp += 2;
            }
            else if ((contextPtr->extraBitsGen < -(EB_S64)(contextPtr->virtualBufferSize << 1))){
                selectedRefQp += 1;
            }

            if ((pictureControlSetPtr->framesInSw < (EB_U32)(sequenceControlSetPtr->intraPeriodLength + 1)) &&
                (pictureControlSetPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0)){
                selectedRefQp = (EB_U32)CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    selectedRefQp + 1);
            }

            queueEntryIndexHeadTemp = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
            queueEntryIndexHeadTemp += encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
            queueEntryIndexHeadTemp = (queueEntryIndexHeadTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                queueEntryIndexHeadTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                queueEntryIndexHeadTemp;

            queueEntryIndexTemp = queueEntryIndexHeadTemp;
            {

                hlRateControlHistogramPtrTemp = (encodeContextPtr->hlRateControlHistorgramQueue[queueEntryIndexTemp]);

                refQpIndexTemp = selectedRefQp + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->hierarchicalLevels][hlRateControlHistogramPtrTemp->temporalLayerIndex];

                refQpIndexTemp = (EB_U32)CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    refQpIndexTemp);

                if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                    refQpIndexTemp = (EB_U32)MAX((EB_S32)refQpIndexTemp + RC_INTRA_QP_OFFSET,0);
                }

                hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = 0;
                rateControlTablesPtr = &encodeContextPtr->rateControlTablesArray[refQpIndexTemp];
                sadBitsArrayPtr = rateControlTablesPtr->sadBitsArray[hlRateControlHistogramPtrTemp->temporalLayerIndex];
                intraSadBitsArrayPtr = rateControlTablesPtr->intraSadBitsArray[hlRateControlHistogramPtrTemp->temporalLayerIndex];
                predBitsRefQp = 0;
                numOfFullLcus = 0;

                if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    {
                        unsigned i;
                        EB_U32 accum = 0;
                        for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                        {
                            accum += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);
                        }

                        predBitsRefQp = accum;
                        numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
                    }
                    hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] += predBitsRefQp;
                }

                else{
                    {
                        unsigned i;
                        EB_U32 accum = 0;
                        for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                        {
                            accum += (EB_U32)(hlRateControlHistogramPtrTemp->meDistortionHistogram[i] * sadBitsArrayPtr[i]);
                        }

                        predBitsRefQp = accum;
                        numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;

                    }
                    hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] += predBitsRefQp;
                }

                // Scale for in complete
                //  predBitsRefQp is normalized based on the area because of the LCUs at the picture boundries
                hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] * (EB_U64)areaInPixel / (numOfFullLcus << 12);

                // Store the predBitsRefQp for the first frame in the window to PCS
                pictureControlSetPtr->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];

            }
        }
        else{
            // Loop over the QPs and find the best QP
            minLaBitDistance = MAX_UNSIGNED_VALUE;
            qpSearchMin = (EB_U8)CLIP3(
                sequenceControlSetPtr->staticConfig.minQpAllowed,
                sequenceControlSetPtr->staticConfig.maxQpAllowed,
                (EB_U32)MAX((EB_S32)sequenceControlSetPtr->qp - 20, 0));

            qpSearchMax = (EB_U8)CLIP3(
                sequenceControlSetPtr->staticConfig.minQpAllowed,
                sequenceControlSetPtr->staticConfig.maxQpAllowed,
                sequenceControlSetPtr->qp + 20);

            for (refQpTableIndex = qpSearchMin; refQpTableIndex < qpSearchMax; refQpTableIndex++){
                highLevelRateControlPtr->predBitsRefQpPerSw[refQpTableIndex] = 0;
            }

            bitConstraintPerSw = highLevelRateControlPtr->bitConstraintPerSw * pictureControlSetPtr->framesInSw / (sequenceControlSetPtr->staticConfig.lookAheadDistance + 1);

            // Update the target rate for the sliding window based on the status of RC    
            if ((contextPtr->extraBitsGen >(EB_S64)(contextPtr->virtualBufferSize * 10))){
                bitConstraintPerSw = bitConstraintPerSw * 130 / 100;
            }
            else if((contextPtr->extraBitsGen >(EB_S64)(contextPtr->virtualBufferSize << 3))){
                bitConstraintPerSw = bitConstraintPerSw * 120 / 100;
            }
            else if ((contextPtr->extraBitsGen >(EB_S64)(contextPtr->virtualBufferSize << 2))){
                bitConstraintPerSw = bitConstraintPerSw * 110 / 100;
            }
            if ((contextPtr->extraBitsGen < -(EB_S64)(contextPtr->virtualBufferSize << 3))){
                bitConstraintPerSw = bitConstraintPerSw * 80 / 100;
            }
            else if ((contextPtr->extraBitsGen < -(EB_S64)(contextPtr->virtualBufferSize << 2))){
                bitConstraintPerSw = bitConstraintPerSw * 90 / 100;
            }

            // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
            previousSelectedRefQp = CLIP3(
                qpSearchMin,
                qpSearchMax,
                previousSelectedRefQp);
            refQpTableIndex  = previousSelectedRefQp;
            selectedRefQpTableIndex = refQpTableIndex;
            selectedRefQp = refQpListTable[selectedRefQpTableIndex];
            bestQpFound = EB_FALSE;
            while (refQpTableIndex >= qpSearchMin && refQpTableIndex <= qpSearchMax && !bestQpFound){

                refQpIndex = CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    refQpListTable[refQpTableIndex]);
                highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] = 0;

                // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
                queueEntryIndexHeadTemp = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
                queueEntryIndexHeadTemp += encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
                queueEntryIndexHeadTemp = (queueEntryIndexHeadTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queueEntryIndexHeadTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queueEntryIndexHeadTemp;

                queueEntryIndexTemp = queueEntryIndexHeadTemp;
                // This is set to false, so the last frame would go inside the loop
                endOfSequenceFlag = EB_FALSE;

                while (!endOfSequenceFlag &&
                    queueEntryIndexTemp <= queueEntryIndexHeadTemp + sequenceControlSetPtr->staticConfig.lookAheadDistance){

                    queueEntryIndexTemp2 = (queueEntryIndexTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queueEntryIndexTemp;
                    hlRateControlHistogramPtrTemp = (encodeContextPtr->hlRateControlHistorgramQueue[queueEntryIndexTemp2]);

                    refQpIndexTemp = refQpIndex + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->hierarchicalLevels][hlRateControlHistogramPtrTemp->temporalLayerIndex];

                    refQpIndexTemp = (EB_U32)CLIP3(
                        sequenceControlSetPtr->staticConfig.minQpAllowed,
                        sequenceControlSetPtr->staticConfig.maxQpAllowed,
                        refQpIndexTemp);

                    if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                        refQpIndexTemp = (EB_U32)MAX((EB_S32)refQpIndexTemp + RC_INTRA_QP_OFFSET, 0);
                    }

                    hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = 0;

                    if (refQpTableIndex == previousSelectedRefQp){
                        EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
                        hlRateControlHistogramPtrTemp->lifeCount--;
                        EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
                    }
                    if (hlRateControlHistogramPtrTemp->isCoded){
                        // If the frame is already coded, use the actual number of bits
                        hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->totalNumBitsCoded;
                    }
                    else{
                        rateControlTablesPtr = &encodeContextPtr->rateControlTablesArray[refQpIndexTemp];
                        sadBitsArrayPtr = rateControlTablesPtr->sadBitsArray[hlRateControlHistogramPtrTemp->temporalLayerIndex];
                        intraSadBitsArrayPtr = rateControlTablesPtr->intraSadBitsArray[0];
                        predBitsRefQp = 0;
                        numOfFullLcus = 0;

                        if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                            // Loop over block in the frame and calculated the predicted bits at reg QP
                            unsigned i;
                            EB_U32 accum = 0;
                            for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                            {
                                accum += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);
                            }

                            predBitsRefQp = accum;
                            numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
                            hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] += predBitsRefQp;
                        }
                        else{
                            unsigned i;
                            EB_U32 accum = 0;
                            EB_U32 accumIntra = 0;
                            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                            {
                                accum += (EB_U32)(hlRateControlHistogramPtrTemp->meDistortionHistogram[i] * sadBitsArrayPtr[i]);
                                accumIntra += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);

                            }
                            if (accum > accumIntra*3)
                                predBitsRefQp = accumIntra;
                            else
                                predBitsRefQp = accum;
                            numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
                            hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] += predBitsRefQp;
                        }

                        // Scale for in complete LCSs
                        //  predBitsRefQp is normalized based on the area because of the LCUs at the picture boundries
                        hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] * (EB_U64)areaInPixel / (numOfFullLcus << 12);

                    }
                    highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] += hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];
                    // Store the predBitsRefQp for the first frame in the window to PCS
                    if (queueEntryIndexHeadTemp == queueEntryIndexTemp2)
                        pictureControlSetPtr->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];

                    endOfSequenceFlag = hlRateControlHistogramPtrTemp->endOfSequenceFlag;
                    queueEntryIndexTemp++;
            }

                if (minLaBitDistance >= (EB_U64)ABS((EB_S64)highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] - (EB_S64)bitConstraintPerSw)){
                    minLaBitDistance = (EB_U64)ABS((EB_S64)highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] - (EB_S64)bitConstraintPerSw);
                    selectedRefQpTableIndex = refQpTableIndex;
                    selectedRefQp = refQpIndex;
                }
                else{
                    bestQpFound = EB_TRUE;
                }

                if (refQpTableIndex == previousSelectedRefQp){
                    if (highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex]  > bitConstraintPerSw){
                        qpStep = +1;
                    }
                    else{
                        qpStep = -1;
                    }
                }
                refQpTableIndex = (EB_U32)(refQpTableIndex + qpStep);

            }
        }

#if RC_UPDATE_TARGET_RATE
        selectedOrgRefQp = selectedRefQp;
        if (sequenceControlSetPtr->intraPeriodLength != -1 && pictureControlSetPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0 &&
            (EB_S32)pictureControlSetPtr->framesInSw >  sequenceControlSetPtr->intraPeriodLength){
            if (pictureControlSetPtr->pictureNumber > 0){
                pictureControlSetPtr->intraSelectedOrgQp = (EB_U8)selectedRefQp;
            }
            else{
                selectedOrgRefQp = selectedRefQp + 1;
                selectedRefQp = selectedRefQp + 1;
            }
            refQpIndex = selectedRefQp;
            highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] = 0;

            if (highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] == 0){

                // Finding the predicted bits for each frame in the sliding window at the reference Qp(s)
                //queueEntryIndexTemp = encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
                queueEntryIndexHeadTemp = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
                queueEntryIndexHeadTemp += encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
                queueEntryIndexHeadTemp = (queueEntryIndexHeadTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queueEntryIndexHeadTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queueEntryIndexHeadTemp;

                queueEntryIndexTemp = queueEntryIndexHeadTemp;

                // This is set to false, so the last frame would go inside the loop
                endOfSequenceFlag = EB_FALSE;

                while (!endOfSequenceFlag &&
                    //queueEntryIndexTemp <= encodeContextPtr->hlRateControlHistorgramQueueHeadIndex+sequenceControlSetPtr->staticConfig.lookAheadDistance){
                    queueEntryIndexTemp <= queueEntryIndexHeadTemp + sequenceControlSetPtr->staticConfig.lookAheadDistance){

                    queueEntryIndexTemp2 = (queueEntryIndexTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queueEntryIndexTemp;
                    hlRateControlHistogramPtrTemp = (encodeContextPtr->hlRateControlHistorgramQueue[queueEntryIndexTemp2]);

                    refQpIndexTemp = refQpIndex + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->hierarchicalLevels][hlRateControlHistogramPtrTemp->temporalLayerIndex];

                    refQpIndexTemp = (EB_U32)CLIP3(
                        sequenceControlSetPtr->staticConfig.minQpAllowed,
                        sequenceControlSetPtr->staticConfig.maxQpAllowed,
                        refQpIndexTemp);

                    if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                        refQpIndexTemp = (EB_U32)MAX((EB_S32)refQpIndexTemp + RC_INTRA_QP_OFFSET, 0);
                    }

                    hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = 0;

                    if (hlRateControlHistogramPtrTemp->isCoded){
                        hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->totalNumBitsCoded;
                    }
                    else{
                        rateControlTablesPtr = &encodeContextPtr->rateControlTablesArray[refQpIndexTemp];
                        sadBitsArrayPtr = rateControlTablesPtr->sadBitsArray[hlRateControlHistogramPtrTemp->temporalLayerIndex];
                        intraSadBitsArrayPtr = rateControlTablesPtr->intraSadBitsArray[hlRateControlHistogramPtrTemp->temporalLayerIndex];
                        predBitsRefQp = 0;

                        numOfFullLcus = 0;

                        if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                            // Loop over block in the frame and calculated the predicted bits at reg QP
        
                            {
                                unsigned i;
                                EB_U32 accum = 0;
                                for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
                                {
                                    accum += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);
                                }

                                predBitsRefQp = accum;
                                numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
                            }
                            hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] += predBitsRefQp;
                        }

                        else{
                            unsigned i;
                            EB_U32 accum = 0;
                            EB_U32 accumIntra = 0;
                            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
                            {
                                accum += (EB_U32)(hlRateControlHistogramPtrTemp->meDistortionHistogram[i] * sadBitsArrayPtr[i]);
                                accumIntra += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);

                            }
                            if (accum > accumIntra * 3)
                                predBitsRefQp = accumIntra;
                            else
                                predBitsRefQp = accum;
                            numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
                            hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] += predBitsRefQp;
                        }

                        // Scale for in complete
                        //  predBitsRefQp is normalized based on the area because of the LCUs at the picture boundries
                        hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp] * (EB_U64)areaInPixel / (numOfFullLcus << 12);

                    }
                    highLevelRateControlPtr->predBitsRefQpPerSw[refQpIndex] += hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];
                    // Store the predBitsRefQp for the first frame in the window to PCS
                    //  if(encodeContextPtr->hlRateControlHistorgramQueueHeadIndex == queueEntryIndexTemp2)
                    if (queueEntryIndexHeadTemp == queueEntryIndexTemp2)
                        pictureControlSetPtr->predBitsRefQp[refQpIndexTemp] = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];

                    endOfSequenceFlag = hlRateControlHistogramPtrTemp->endOfSequenceFlag;
                    queueEntryIndexTemp++;
                }
            }
        }
#endif
        pictureControlSetPtr->tablesUpdated = tablesUpdated;
        EB_BOOL expensiveISlice = EB_FALSE;
        // Looping over the window to find the percentage of bit allocation in each layer
        if ((sequenceControlSetPtr->intraPeriodLength != -1) &&
            ((EB_S32)pictureControlSetPtr->framesInSw >  sequenceControlSetPtr->intraPeriodLength) &&
            ((EB_S32)pictureControlSetPtr->framesInSw >  sequenceControlSetPtr->intraPeriodLength)){
            EB_U64 iSliceBits = 0;

            if (pictureControlSetPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0){

                queueEntryIndexHeadTemp = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
                queueEntryIndexHeadTemp += encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
                queueEntryIndexHeadTemp = (queueEntryIndexHeadTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                    queueEntryIndexHeadTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                    queueEntryIndexHeadTemp;

                queueEntryIndexTemp = queueEntryIndexHeadTemp;

                // This is set to false, so the last frame would go inside the loop
                endOfSequenceFlag = EB_FALSE;

                while (!endOfSequenceFlag &&
                    queueEntryIndexTemp <= queueEntryIndexHeadTemp + sequenceControlSetPtr->intraPeriodLength){

                    queueEntryIndexTemp2 = (queueEntryIndexTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH : queueEntryIndexTemp;
                    hlRateControlHistogramPtrTemp = (encodeContextPtr->hlRateControlHistorgramQueue[queueEntryIndexTemp2]);

                    refQpIndexTemp = selectedRefQp + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->hierarchicalLevels][hlRateControlHistogramPtrTemp->temporalLayerIndex];

                    refQpIndexTemp = (EB_U32)CLIP3(
                        sequenceControlSetPtr->staticConfig.minQpAllowed,
                        sequenceControlSetPtr->staticConfig.maxQpAllowed,
                        refQpIndexTemp);

                    if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE){
                        refQpIndexTemp = (EB_U32)MAX((EB_S32)refQpIndexTemp + RC_INTRA_QP_OFFSET, 0);
                    }
                    if (queueEntryIndexTemp == queueEntryIndexHeadTemp){
                        iSliceBits = hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];
                    }
                    pictureControlSetPtr->totalBitsPerGop += hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];
                    pictureControlSetPtr->bitsPerSwPerLayer[hlRateControlHistogramPtrTemp->temporalLayerIndex] += hlRateControlHistogramPtrTemp->predBitsRefQp[refQpIndexTemp];
                    pictureControlSetPtr->percentageUpdated = EB_TRUE;

                    endOfSequenceFlag = hlRateControlHistogramPtrTemp->endOfSequenceFlag;
                    queueEntryIndexTemp++;
                }
                if (iSliceBits*100 > 85*pictureControlSetPtr->totalBitsPerGop){
                    expensiveISlice = EB_TRUE;
                }
                if (pictureControlSetPtr->totalBitsPerGop == 0){
                    for (temporalLayerIndex = 0; temporalLayerIndex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIndex++){
                        pictureControlSetPtr->bitsPerSwPerLayer[temporalLayerIndex] = RATE_PERCENTAGE_LAYER_ARRAY[sequenceControlSetPtr->staticConfig.hierarchicalLevels][temporalLayerIndex];
                    }
                }
            }
        }
        else{
            for (temporalLayerIndex = 0; temporalLayerIndex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIndex++){
                pictureControlSetPtr->bitsPerSwPerLayer[temporalLayerIndex] = RATE_PERCENTAGE_LAYER_ARRAY[sequenceControlSetPtr->staticConfig.hierarchicalLevels][temporalLayerIndex];
            }
        }
        if (expensiveISlice){
            if (tablesUpdated){
                selectedRefQp = (EB_U32)MAX((EB_S32)selectedRefQp - 1,0);
            }
            else{
                selectedRefQp = (EB_U32)MAX((EB_S32)selectedRefQp - 3,0);
            }
            selectedRefQp = (EB_U32)CLIP3(
                sequenceControlSetPtr->staticConfig.minQpAllowed,
                sequenceControlSetPtr->staticConfig.maxQpAllowed,
                selectedRefQp);
        }
        // Set the QP
        previousSelectedRefQp = selectedRefQp;
        if (pictureControlSetPtr->pictureNumber > maxCodedPoc && pictureControlSetPtr->temporalLayerIndex < 2 && !pictureControlSetPtr->endOfSequenceRegion){

            maxCodedPoc = pictureControlSetPtr->pictureNumber;
            maxCodedPocSelectedRefQp = previousSelectedRefQp;
            encodeContextPtr->previousSelectedRefQp = previousSelectedRefQp;
            encodeContextPtr->maxCodedPoc = maxCodedPoc;
            encodeContextPtr->maxCodedPocSelectedRefQp = maxCodedPocSelectedRefQp;

        }

        pictureControlSetPtr->bestPredQp = (EB_U8)CLIP3(
            sequenceControlSetPtr->staticConfig.minQpAllowed,
            sequenceControlSetPtr->staticConfig.maxQpAllowed,
            selectedRefQp + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]);

        if (pictureControlSetPtr->sliceType == EB_I_PICTURE){
            pictureControlSetPtr->bestPredQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->bestPredQp + RC_INTRA_QP_OFFSET,0);
        }
#if RC_UPDATE_TARGET_RATE
        if (pictureControlSetPtr->pictureNumber == 0){
            highLevelRateControlPtr->prevIntraSelectedRefQp = selectedRefQp;
            highLevelRateControlPtr->prevIntraOrgSelectedRefQp = selectedRefQp;
        }
        if (sequenceControlSetPtr->intraPeriodLength != -1){
            if (pictureControlSetPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0){
                highLevelRateControlPtr->prevIntraSelectedRefQp = selectedRefQp;
                highLevelRateControlPtr->prevIntraOrgSelectedRefQp = selectedOrgRefQp;
            }
        }
#endif
        pictureControlSetPtr->targetBitsBestPredQp = pictureControlSetPtr->predBitsRefQp[pictureControlSetPtr->bestPredQp];
        //if (pictureControlSetPtr->sliceType == 2)
        // {
        //SVT_LOG("\nTID: %d\t", pictureControlSetPtr->temporalLayerIndex);
        //SVT_LOG("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
        //    pictureControlSetPtr->pictureNumber,
        //    pictureControlSetPtr->bestPredQp,
        //    (int)pictureControlSetPtr->targetBitsBestPredQp,
        //    (int)highLevelRateControlPtr->predBitsRefQpPerSw[selectedRefQp - 1],
        //    (int)highLevelRateControlPtr->predBitsRefQpPerSw[selectedRefQp],
        //    (int)highLevelRateControlPtr->predBitsRefQpPerSw[selectedRefQp + 1],
        //    (int)highLevelRateControlPtr->bitConstraintPerSw,
        //    (int)bitConstraintPerSw,
        //    (int)highLevelRateControlPtr->virtualBufferLevel);
        //}
    }
    EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->rateTableUpdateMutex);
}
void FrameLevelRcInputPictureMode2(
    PictureControlSet_t               *pictureControlSetPtr,
    SequenceControlSet_t              *sequenceControlSetPtr,
    RateControlContext_t              *contextPtr,
    RateControlLayerContext_t         *rateControlLayerPtr,
    RateControlIntervalParamContext_t *rateControlParamPtr,
    EB_U32                             bestOisCuIndex)
{

    RateControlLayerContext_t   *rateControlLayerTempPtr;

    // Tiles
    EB_U32                       pictureAreaInPixel;
    EB_U32                       areaInPixel;

    // LCU Loop variables
    EB_U32                       lcuTotalCount;
    LargestCodingUnit_t         *lcuPtr;
    EB_U32                       lcuCodingOrder;
    EB_U32                       lcuHeight;
    EB_U32                       lcuWidth;
    EB_U64                       tempQp;
    EB_U32                       areaInLcus;
    (void) bestOisCuIndex;
    lcuTotalCount = pictureControlSetPtr->lcuTotalCount;
    pictureAreaInPixel = sequenceControlSetPtr->lumaHeight*sequenceControlSetPtr->lumaWidth;

    if (rateControlLayerPtr->firstFrame == 1){
        rateControlLayerPtr->firstFrame = 0;
        pictureControlSetPtr->ParentPcsPtr->firstFrameInTemporalLayer = 1;
    }
    else{
        pictureControlSetPtr->ParentPcsPtr->firstFrameInTemporalLayer = 0;
    }
	if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
        if (rateControlLayerPtr->firstNonIntraFrame == 1){
            rateControlLayerPtr->firstNonIntraFrame = 0;
            pictureControlSetPtr->ParentPcsPtr->firstNonIntraFrameInTemporalLayer = 1;
        }
        else{
            pictureControlSetPtr->ParentPcsPtr->firstNonIntraFrameInTemporalLayer = 0;
        }
    }

    pictureControlSetPtr->ParentPcsPtr->targetBitsRc = 0;

    // ***Rate Control***
    areaInLcus = 0;
    areaInPixel = 0;

    for (lcuCodingOrder = 0; lcuCodingOrder < pictureControlSetPtr->lcuTotalCount; ++lcuCodingOrder) {

        lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];
        lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuPtr->originX >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaWidth - lcuPtr->originX;
        lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuPtr->originY >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaHeight - lcuPtr->originY;

        // This is because of the tile boundry LCUs which do not have correct SAD from ME. 
        if ((lcuWidth == MAX_LCU_SIZE) && (lcuHeight == MAX_LCU_SIZE)){
            // add the area of one LCU (64x64=4096) to the area of the tile
            areaInPixel += 4096;
            areaInLcus++;
        }
        else{
            // add the area of the LCU to the area of the tile
            areaInPixel += lcuWidth*lcuHeight;
        }
    }
    rateControlLayerPtr->areaInPixel = areaInPixel;

    if (pictureControlSetPtr->ParentPcsPtr->firstFrameInTemporalLayer || (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc)){
        if (sequenceControlSetPtr->enableQpScalingFlag && (pictureControlSetPtr->pictureNumber != rateControlParamPtr->firstPoc)) {

            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                (EB_S32)sequenceControlSetPtr->staticConfig.minQpAllowed,
                (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed,
                (EB_S32)(rateControlParamPtr->intraFramesQp + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]) - 1 - (RC_INTRA_QP_OFFSET));
        }

        if (pictureControlSetPtr->pictureNumber == 0){
            rateControlParamPtr->intraFramesQp = sequenceControlSetPtr->qp;
        }

        if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc){
            EB_U32 temporalLayerIdex;
            rateControlParamPtr->previousVirtualBufferLevel = contextPtr->virtualBufferLevelInitialValue;
            rateControlParamPtr->virtualBufferLevel = contextPtr->virtualBufferLevelInitialValue;
            rateControlParamPtr->extraApBitRatioI = 0;
            if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
                rateControlParamPtr->lastPoc = MAX(rateControlParamPtr->firstPoc + pictureControlSetPtr->ParentPcsPtr->framesInSw - 1, rateControlParamPtr->firstPoc);
                rateControlParamPtr->lastGop = EB_TRUE;
            }
            
            if ((contextPtr->extraBits > (EB_S64)(contextPtr->virtualBufferSize >> 8)) ||
                (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize >> 8))){

                EB_S64 extraBitsPerGop = 0;

                if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
                    if ((contextPtr->extraBits >(EB_S64)(contextPtr->virtualBufferSize << 4)) ||
                        (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize << 4))){
                        extraBitsPerGop = contextPtr->extraBits;
                        extraBitsPerGop = CLIP3(
                            -(EB_S64)(contextPtr->vbFillThreshold2 << 3),
                            (EB_S64)(contextPtr->vbFillThreshold2 << 3),
                            extraBitsPerGop);
                    }
                    else
                    if ((contextPtr->extraBits >(EB_S64)(contextPtr->virtualBufferSize << 3)) ||
                        (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize << 3))){
                        extraBitsPerGop = contextPtr->extraBits;
                        extraBitsPerGop = CLIP3(
                            -(EB_S64)(contextPtr->vbFillThreshold2 << 2),
                            (EB_S64)(contextPtr->vbFillThreshold2 << 2),
                            extraBitsPerGop);
                    }
                    else if ((contextPtr->extraBits >(EB_S64)(contextPtr->virtualBufferSize << 2)) ||
                        (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize << 2))){

                        extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->vbFillThreshold2 << 1,
                            (EB_S64)contextPtr->vbFillThreshold2 << 1,
                            extraBitsPerGop);
                    }
                    else{
                        extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->vbFillThreshold1,
                            (EB_S64)contextPtr->vbFillThreshold1,
                            extraBitsPerGop);
                    }
                }
                else{
                    if ((contextPtr->extraBits >(EB_S64)(contextPtr->virtualBufferSize << 3)) ||
                        (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize << 3))){
                        extraBitsPerGop = contextPtr->extraBits;
                        extraBitsPerGop = CLIP3(
                            -(EB_S64)(contextPtr->vbFillThreshold2 << 2),
                            (EB_S64)(contextPtr->vbFillThreshold2 << 2),
                            extraBitsPerGop);
                    }
                    else if ((contextPtr->extraBits >(EB_S64)(contextPtr->virtualBufferSize << 2)) ||
                        (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize << 2))){

                        extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->vbFillThreshold2 << 1,
                            (EB_S64)contextPtr->vbFillThreshold2 << 1,
                            extraBitsPerGop);
                    }
                }

                rateControlParamPtr->virtualBufferLevel -= extraBitsPerGop;
                rateControlParamPtr->previousVirtualBufferLevel -= extraBitsPerGop;
                contextPtr->extraBits -= extraBitsPerGop;
            }

            for (temporalLayerIdex = 0; temporalLayerIdex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIdex++){

                rateControlLayerTempPtr = rateControlParamPtr->rateControlLayerArray[temporalLayerIdex];
                RateControlLayerReset(
                    rateControlLayerTempPtr,
                    pictureControlSetPtr,
                    contextPtr,
                    pictureAreaInPixel,
                    rateControlParamPtr->wasUsed);
            }
        }

        pictureControlSetPtr->ParentPcsPtr->sadMe = 0;
        // Finding the QP of the Intra frame by using variance tables
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            EB_U32         selectedRefQp;

            if (sequenceControlSetPtr->staticConfig.lookAheadDistance == 0){
                EB_U32         selectedRefQpTableIndex;
                EB_U32         intraSadIntervalIndex;
                EB_U32         refQpIndex;
                EB_U32         refQpTableIndex;
                EB_U32         qpSearchMin;
                EB_U32         qpSearchMax;
                EB_U32         numOfFullLcus;
                EB_U64         minLaBitDistance;

                minLaBitDistance = MAX_UNSIGNED_VALUE;
                selectedRefQpTableIndex = 0;
                selectedRefQp = refQpListTable[selectedRefQpTableIndex];
                qpSearchMin = (EB_U32)CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    (EB_U32)MAX((EB_S32)sequenceControlSetPtr->qp - 30, 0));

                qpSearchMax = CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    sequenceControlSetPtr->qp + 30);

                if (!sequenceControlSetPtr->encodeContextPtr->rateControlTablesArrayUpdated) {
                    contextPtr->intraCoefRate = CLIP3(
                        1,
                        2,
                        (EB_U32)(rateControlLayerPtr->frameRate >> 16) / 4);
                }
                else{
                    if (contextPtr->baseLayerFramesAvgQp > contextPtr->baseLayerIntraFramesAvgQp + 3)
                        contextPtr->intraCoefRate--;
                    else if (contextPtr->baseLayerFramesAvgQp <= contextPtr->baseLayerIntraFramesAvgQp + 2)
                        contextPtr->intraCoefRate += 2;
                    else if (contextPtr->baseLayerFramesAvgQp <= contextPtr->baseLayerIntraFramesAvgQp)
                        contextPtr->intraCoefRate++;

                    contextPtr->intraCoefRate = CLIP3(
                        1,
                        15,//(EB_U32) (contextPtr->framesInInterval[0]+1) / 2,
                        contextPtr->intraCoefRate);
                }

                // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
                for (refQpTableIndex = qpSearchMin; refQpTableIndex < qpSearchMax; refQpTableIndex++){
                    refQpIndex = refQpListTable[refQpTableIndex];

                    pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] = 0;

                    numOfFullLcus = 0;
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {

                        lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];
                        lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuPtr->originX >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaWidth - lcuPtr->originX;
                        lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuPtr->originY >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaHeight - lcuPtr->originY;
                        // This is because of the tile boundry LCUs which do not have correct SAD from ME. 
                        // ME doesn't know about Tile Boundries
                        if ((lcuWidth == MAX_LCU_SIZE) && (lcuHeight == MAX_LCU_SIZE)){
                            numOfFullLcus++;
                            intraSadIntervalIndex = pictureControlSetPtr->ParentPcsPtr->intraSadIntervalIndex[lcuCodingOrder];
                            pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] += sequenceControlSetPtr->encodeContextPtr->rateControlTablesArray[refQpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][intraSadIntervalIndex];

                        }
                    }

                    // Scale for in complete LCUs
                    //  predBitsRefQp is normalized based on the area because of the LCUs at the tile boundries
                    pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] = pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] * rateControlLayerPtr->areaInPixel / (numOfFullLcus << 12);

                    if (minLaBitDistance > (EB_U64)ABS((EB_S64)rateControlLayerPtr->ecBitConstraint*contextPtr->intraCoefRate - (EB_S64)pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex])){
                        minLaBitDistance = (EB_U64)ABS((EB_S64)rateControlLayerPtr->ecBitConstraint*contextPtr->intraCoefRate - (EB_S64)pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex]);

                        selectedRefQpTableIndex = refQpTableIndex;
                        selectedRefQp = refQpIndex;
                    }

                }

                if (!sequenceControlSetPtr->encodeContextPtr->rateControlTablesArrayUpdated) {
					pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)selectedRefQp - (EB_S32)1, 0);
                    rateControlLayerPtr->calculatedFrameQp              = (EB_U8)MAX((EB_S32)selectedRefQp - (EB_S32)1, 0);
					pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->pictureQp;
                }
                else{
					pictureControlSetPtr->pictureQp  = (EB_U8)selectedRefQp;
                    rateControlLayerPtr->calculatedFrameQp = (EB_U8)selectedRefQp;
					pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->pictureQp;
					pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        (EB_U32)MAX((EB_S32)contextPtr->baseLayerFramesAvgQp - (EB_S32)3, 0),
                        contextPtr->baseLayerFramesAvgQp,
						pictureControlSetPtr->pictureQp);
					pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        (EB_U32)MAX((EB_S32)contextPtr->baseLayerIntraFramesAvgQp - (EB_S32)5, 0),
                        contextPtr->baseLayerIntraFramesAvgQp + 2,
						pictureControlSetPtr->pictureQp);
                }
            }
            else{
                selectedRefQp = pictureControlSetPtr->ParentPcsPtr->bestPredQp;
				pictureControlSetPtr->pictureQp = (EB_U8)selectedRefQp;
				pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->pictureQp;
                if (rateControlParamPtr->firstPoc == 0){
					pictureControlSetPtr->pictureQp++;
                }
            }

            // Update the QP based on the VB
            if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
                if (rateControlParamPtr->virtualBufferLevel >= contextPtr->vbFillThreshold2 << 1){
					pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 2;
                }
                else if (rateControlParamPtr->virtualBufferLevel >= contextPtr->vbFillThreshold2){
					pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE;
                }
                else if (rateControlParamPtr->virtualBufferLevel >= contextPtr->vbFillThreshold1 &&
                    rateControlParamPtr->virtualBufferLevel < contextPtr->vbFillThreshold2){
					pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD1QPINCREASE;
                }
                if (rateControlParamPtr->virtualBufferLevel <= -(contextPtr->vbFillThreshold2 << 2))
					pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE - (EB_S32)2, 0);
                else
                if (rateControlParamPtr->virtualBufferLevel <= -(contextPtr->vbFillThreshold2 << 1))
					pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE - (EB_S32)1, 0);
                else if (rateControlParamPtr->virtualBufferLevel <= 0)
					pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE, 0);
            }
            else{

               if (rateControlParamPtr->virtualBufferLevel >= contextPtr->vbFillThreshold2){
				   pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE;
                }
               if (rateControlParamPtr->virtualBufferLevel <= -(contextPtr->vbFillThreshold2 << 2))
				   pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp - (EB_U8)THRESHOLD2QPINCREASE - (EB_S32)2;
               else if (rateControlParamPtr->virtualBufferLevel <= - (contextPtr->vbFillThreshold2<<1))
				   pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp - (EB_U8)THRESHOLD2QPINCREASE - (EB_S32)1;
               else
               if (rateControlParamPtr->virtualBufferLevel <= 0)
				   pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE, 0);
            }
			pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                sequenceControlSetPtr->staticConfig.minQpAllowed,
                sequenceControlSetPtr->staticConfig.maxQpAllowed,
				pictureControlSetPtr->pictureQp);
        }
        else{

            // LCU Loop                       
            for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {

                lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];
                lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuPtr->originX >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaWidth - lcuPtr->originX;
                lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuPtr->originY >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaHeight - lcuPtr->originY;
                // This is because of the tile boundry LCUs which do not have correct SAD from ME. 
                // ME doesn't know about Tile Boundries
                if ((lcuWidth == MAX_LCU_SIZE) && (lcuHeight == MAX_LCU_SIZE)){
					pictureControlSetPtr->ParentPcsPtr->sadMe += pictureControlSetPtr->ParentPcsPtr->rcMEdistortion[lcuCodingOrder];
                }
            }

            //  tileSadMe is normalized based on the area because of the LCUs at the tile boundries
            pictureControlSetPtr->ParentPcsPtr->sadMe = MAX((pictureControlSetPtr->ParentPcsPtr->sadMe*rateControlLayerPtr->areaInPixel / (areaInLcus << 12)), 1);

            // totalSquareMad has RC_PRECISION precision
            pictureControlSetPtr->ParentPcsPtr->sadMe <<= RC_PRECISION;

        }

		tempQp = pictureControlSetPtr->pictureQp;

        if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc){
            EB_U32 temporalLayerIdex;
            for (temporalLayerIdex = 0; temporalLayerIdex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIdex++){
                rateControlLayerTempPtr = rateControlParamPtr->rateControlLayerArray[temporalLayerIdex];
                RateControlLayerResetPart2(
                    rateControlLayerTempPtr,
                    pictureControlSetPtr);
            }
        }

        if (pictureControlSetPtr->pictureNumber == 0){
			contextPtr->baseLayerFramesAvgQp = pictureControlSetPtr->pictureQp + 1;
			contextPtr->baseLayerIntraFramesAvgQp = pictureControlSetPtr->pictureQp;
        }
    }
    else{

        pictureControlSetPtr->ParentPcsPtr->sadMe = 0;

        // if the pixture is an I slice, for now we set the QP as the QP of the previous frame
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            EB_U32         selectedRefQp;

            if (sequenceControlSetPtr->staticConfig.lookAheadDistance == 0)
            {
                EB_U32         selectedRefQpTableIndex;
                EB_U32         intraSadIntervalIndex;
                EB_U32         refQpIndex;
                EB_U32         refQpTableIndex;
                EB_U32         qpSearchMin;
                EB_U32         qpSearchMax;
                EB_U32         numOfFullLcus;
                EB_U64         minLaBitDistance;

                minLaBitDistance = MAX_UNSIGNED_VALUE;
                selectedRefQpTableIndex = 0;
                selectedRefQp = refQpListTable[selectedRefQpTableIndex];
                qpSearchMin = (EB_U8)CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    (EB_U32)MAX((EB_S32)sequenceControlSetPtr->qp - 20, 0));

                qpSearchMax = (EB_U8)CLIP3(
                    sequenceControlSetPtr->staticConfig.minQpAllowed,
                    sequenceControlSetPtr->staticConfig.maxQpAllowed,
                    sequenceControlSetPtr->qp + 20);

                contextPtr->intraCoefRate = CLIP3(
                    1,
                    (EB_U32)(rateControlLayerPtr->frameRate >> 16) / 4,
                    contextPtr->intraCoefRate);
                // Loop over proper QPs and find the Predicted bits for that QP. Find the QP with the closest total predicted rate to target bits for the sliding window.
                for (refQpTableIndex = qpSearchMin; refQpTableIndex < qpSearchMax; refQpTableIndex++){
                    refQpIndex = refQpListTable[refQpTableIndex];
                    pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] = 0;
                    numOfFullLcus = 0;
                    // Loop over block in the frame and calculated the predicted bits at reg QP
                    for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {

                        lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];
                        lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuPtr->originX >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaWidth - lcuPtr->originX;
                        lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuPtr->originY >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaHeight - lcuPtr->originY;
                        // This is because of the tile boundry LCUs which do not have correct SAD from ME. 
                        // ME doesn't know about Tile Boundries
                        if ((lcuWidth == MAX_LCU_SIZE) && (lcuHeight == MAX_LCU_SIZE)){
                            numOfFullLcus++;
                            intraSadIntervalIndex = pictureControlSetPtr->ParentPcsPtr->intraSadIntervalIndex[lcuCodingOrder];
                            pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] += sequenceControlSetPtr->encodeContextPtr->rateControlTablesArray[refQpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][intraSadIntervalIndex];
                        }
                    }

                    // Scale for in complete LCUs
                    // predBitsRefQp is normalized based on the area because of the LCUs at the tile boundries
                    pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] = pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex] * rateControlLayerPtr->areaInPixel / (numOfFullLcus << 12);
                    if (minLaBitDistance > (EB_U64)ABS((EB_S64)rateControlLayerPtr->ecBitConstraint*contextPtr->intraCoefRate - (EB_S64)pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex])){
                        minLaBitDistance = (EB_U64)ABS((EB_S64)rateControlLayerPtr->ecBitConstraint*contextPtr->intraCoefRate - (EB_S64)pictureControlSetPtr->ParentPcsPtr->predBitsRefQp[refQpIndex]);

                        selectedRefQpTableIndex = refQpTableIndex;
                        selectedRefQp = refQpIndex;
                    }
                }
                if (!sequenceControlSetPtr->encodeContextPtr->rateControlTablesArrayUpdated) {
					pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)selectedRefQp - (EB_S32)1, 0);
                    rateControlLayerPtr->calculatedFrameQp              = (EB_U8)MAX((EB_S32)selectedRefQp - (EB_S32)1, 0);
					pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->pictureQp;
                }
                else{
					pictureControlSetPtr->pictureQp = (EB_U8)selectedRefQp;
                    rateControlLayerPtr->calculatedFrameQp = (EB_U8)selectedRefQp;
					pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->pictureQp;
					pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        (EB_U32)MAX((EB_S32)contextPtr->baseLayerFramesAvgQp - (EB_S32)3, 0),
                        contextPtr->baseLayerFramesAvgQp + 1,
						pictureControlSetPtr->pictureQp);
					pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        (EB_U32)MAX((EB_S32)contextPtr->baseLayerIntraFramesAvgQp - (EB_S32)5, 0),
                        contextPtr->baseLayerIntraFramesAvgQp + 2,
						pictureControlSetPtr->pictureQp);
                }
            }
            else{
                selectedRefQp = pictureControlSetPtr->ParentPcsPtr->bestPredQp;
				pictureControlSetPtr->pictureQp = (EB_U8)selectedRefQp;
				pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->pictureQp;
            }

			pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                sequenceControlSetPtr->staticConfig.minQpAllowed,
                sequenceControlSetPtr->staticConfig.maxQpAllowed,
				pictureControlSetPtr->pictureQp);

			tempQp = pictureControlSetPtr->pictureQp;

        }

        else{ // Not an I slice
            // combining the target rate from initial RC and frame level RC
            if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){
                pictureControlSetPtr->ParentPcsPtr->targetBitsRc = rateControlLayerPtr->bitConstraint;
                rateControlLayerPtr->ecBitConstraint = (rateControlLayerPtr->alpha * pictureControlSetPtr->ParentPcsPtr->targetBitsBestPredQp +
                    ((1 << RC_PRECISION) - rateControlLayerPtr->alpha) * pictureControlSetPtr->ParentPcsPtr->targetBitsRc + RC_PRECISION_OFFSET) >> RC_PRECISION;

                rateControlLayerPtr->ecBitConstraint = (EB_U64)MAX((EB_S64)rateControlLayerPtr->ecBitConstraint - (EB_S64)rateControlLayerPtr->difTotalAndEcBits, 1);

                pictureControlSetPtr->ParentPcsPtr->targetBitsRc = rateControlLayerPtr->ecBitConstraint;
            }

            // LCU Loop                       
            for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {

                lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];
                lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuPtr->originX >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaWidth - lcuPtr->originX;
                lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuPtr->originY >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaHeight - lcuPtr->originY;
                // This is because of the tile boundry LCUs which do not have correct SAD from ME. 
                // ME doesn't know about Tile Boundries
                if ((lcuWidth == MAX_LCU_SIZE) && (lcuHeight == MAX_LCU_SIZE)){
					pictureControlSetPtr->ParentPcsPtr->sadMe += pictureControlSetPtr->ParentPcsPtr->rcMEdistortion[lcuCodingOrder];
                }
            }

            //  tileSadMe is normalized based on the area because of the LCUs at the tile boundries
            pictureControlSetPtr->ParentPcsPtr->sadMe = MAX((pictureControlSetPtr->ParentPcsPtr->sadMe*rateControlLayerPtr->areaInPixel / (areaInLcus << 12)), 1);
            pictureControlSetPtr->ParentPcsPtr->sadMe <<= RC_PRECISION;

            rateControlLayerPtr->totalMad = MAX((pictureControlSetPtr->ParentPcsPtr->sadMe / rateControlLayerPtr->areaInPixel), 1);

            if (!rateControlLayerPtr->feedbackArrived){
                rateControlLayerPtr->previousFrameSadMe = pictureControlSetPtr->ParentPcsPtr->sadMe;
            }

            {
                EB_U64 qpCalcTemp1, qpCalcTemp2, qpCalcTemp3;

                qpCalcTemp1 = pictureControlSetPtr->ParentPcsPtr->sadMe *rateControlLayerPtr->totalMad;
                qpCalcTemp2 =
                    MAX((EB_S64)(rateControlLayerPtr->ecBitConstraint << (2 * RC_PRECISION)) - (EB_S64)rateControlLayerPtr->cCoeff*(EB_S64)rateControlLayerPtr->areaInPixel,
                    (EB_S64)(rateControlLayerPtr->ecBitConstraint << (2 * RC_PRECISION - 2)));

                // This is a more complex but with higher precision implementation
                if (qpCalcTemp1 > qpCalcTemp2)
                    qpCalcTemp3 = (EB_U64)((qpCalcTemp1 / qpCalcTemp2)*rateControlLayerPtr->kCoeff);
                else
                    qpCalcTemp3 = (EB_U64)(qpCalcTemp1*rateControlLayerPtr->kCoeff / qpCalcTemp2);
                tempQp = (EB_U64)(Log2fHighPrecision(MAX(((qpCalcTemp3 + RC_PRECISION_OFFSET) >> RC_PRECISION)*((qpCalcTemp3 + RC_PRECISION_OFFSET) >> RC_PRECISION)*((qpCalcTemp3 + RC_PRECISION_OFFSET) >> RC_PRECISION), 1), RC_PRECISION));

                rateControlLayerPtr->calculatedFrameQp = (EB_U8)(CLIP3(1, 51, (EB_U32)(tempQp + RC_PRECISION_OFFSET) >> RC_PRECISION));
                pictureControlSetPtr->ParentPcsPtr->calculatedQp = (EB_U8)(CLIP3(1, 51, (EB_U32)(tempQp + RC_PRECISION_OFFSET) >> RC_PRECISION));
            }

            tempQp += rateControlLayerPtr->deltaQpFraction;
			pictureControlSetPtr->pictureQp = (EB_U8)((tempQp + RC_PRECISION_OFFSET) >> RC_PRECISION);
            // Use the QP of HLRC instead of calculated one in FLRC
            if (pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels > 1){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->ParentPcsPtr->bestPredQp;
                pictureControlSetPtr->ParentPcsPtr->calculatedQp = pictureControlSetPtr->ParentPcsPtr->bestPredQp;
            }
            if (sequenceControlSetPtr->intraPeriodLength != -1 && rateControlParamPtr->firstPoc == 0){
                pictureControlSetPtr->ParentPcsPtr->bestPredQp++;
				pictureControlSetPtr->pictureQp++;
                pictureControlSetPtr->ParentPcsPtr->calculatedQp++;                
            }
        }
        if (pictureControlSetPtr->ParentPcsPtr->firstNonIntraFrameInTemporalLayer && pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){
			pictureControlSetPtr->pictureQp = (EB_U8)rateControlParamPtr->intraFramesQp + 1;
        }

        if (!rateControlLayerPtr->feedbackArrived && pictureControlSetPtr->sliceType != EB_I_PICTURE){

            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                (EB_S32)sequenceControlSetPtr->staticConfig.minQpAllowed,
                (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed,
                (EB_S32)(rateControlParamPtr->intraFramesQp + QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex] - RC_INTRA_QP_OFFSET));
        }

        if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
            if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold2 << 2){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 4;
            }
            else if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold2 << 1){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 3;
            }
            else if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold2){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 2;
            }
            else if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold1 &&
                rateControlParamPtr->virtualBufferLevel < contextPtr->vbFillThreshold2){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD1QPINCREASE + 2;
            }
        }
        else{


            if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold2 << 2){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 2 ;

            }
            else if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold2 << 1){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 1;


            }
            else if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold2){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD2QPINCREASE + 1;
            }
            else if (rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold1 &&
                rateControlParamPtr->virtualBufferLevel < contextPtr->vbFillThreshold2){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + (EB_U8)THRESHOLD1QPINCREASE;
            }

        }
        if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
            if (rateControlParamPtr->virtualBufferLevel < -(contextPtr->vbFillThreshold2 << 2))
                pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE - 2, 0);
            else if (rateControlParamPtr->virtualBufferLevel < -(contextPtr->vbFillThreshold2 << 1))
                pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE - 1, 0);
            else if (rateControlParamPtr->virtualBufferLevel < 0)
                pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE, 0);
        }
        else{

            if (rateControlParamPtr->virtualBufferLevel < -(contextPtr->vbFillThreshold2 << 2))
                pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE - 1, 0);
            else if (rateControlParamPtr->virtualBufferLevel < -contextPtr->vbFillThreshold2)
                pictureControlSetPtr->pictureQp = (EB_U8)MAX((EB_S32)pictureControlSetPtr->pictureQp - (EB_S32)THRESHOLD2QPINCREASE, 0);
        }
        // limiting the QP based on the predicted QP
        if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){
            if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
                pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                    (EB_U32)MAX((EB_S32)pictureControlSetPtr->ParentPcsPtr->bestPredQp - 8, 0),
                    (EB_U32)pictureControlSetPtr->ParentPcsPtr->bestPredQp + 8,
                    (EB_U32)pictureControlSetPtr->pictureQp);
            }
            else{
                pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                    (EB_U32)MAX((EB_S32)pictureControlSetPtr->ParentPcsPtr->bestPredQp - 8, 0), 
                    (EB_U32)pictureControlSetPtr->ParentPcsPtr->bestPredQp + 8,
                    (EB_U32)pictureControlSetPtr->pictureQp);

            }
        }
        if (pictureControlSetPtr->pictureNumber != rateControlParamPtr->firstPoc && 
            pictureControlSetPtr->pictureQp == pictureControlSetPtr->ParentPcsPtr->bestPredQp && rateControlParamPtr->virtualBufferLevel > contextPtr->vbFillThreshold1){
            if (rateControlParamPtr->extraApBitRatioI > 200){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + 3;
            }
            else if (rateControlParamPtr->extraApBitRatioI > 100){
                pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + 2;
            }
            else if (rateControlParamPtr->extraApBitRatioI > 50){
                pictureControlSetPtr->pictureQp++;
            }
        }
        //Limiting the QP based on the QP of the Reference frame
        {
            EB_U32 refQp;
                
            if (!pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
                if (contextPtr->framesInInterval[pictureControlSetPtr->temporalLayerIndex]< 5){
                    if ((EB_S32)pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){
                        if (pictureControlSetPtr->refSliceTypeArray[0] == EB_I_PICTURE){
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)pictureControlSetPtr->refPicQpArray[0],
                                (EB_U32)pictureControlSetPtr->refPicQpArray[0] + 2,
                                pictureControlSetPtr->pictureQp);
                        }
                        else{
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)pictureControlSetPtr->refPicQpArray[0] - 1, 0),
                                (EB_U32)pictureControlSetPtr->refPicQpArray[0] + 1,
                                pictureControlSetPtr->pictureQp);

                        }
                    }
                    if ((EB_S32)pictureControlSetPtr->temporalLayerIndex == 1){
                        refQp = 0;
                        if (pictureControlSetPtr->refSliceTypeArray[0] != EB_I_PICTURE){
                            refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[0]);
                        }

                        if ((pictureControlSetPtr->sliceType == EB_B_PICTURE) && (pictureControlSetPtr->refSliceTypeArray[1] != EB_I_PICTURE)){
                            refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[1]);
                        }
                        if (refQp > 0){
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)refQp - 1,
                                (EB_U32)refQp + 1,
                                pictureControlSetPtr->pictureQp);
                        }
                    }
                }
                else{
                    if ((EB_S32)pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){
                        if (pictureControlSetPtr->refSliceTypeArray[0] == EB_I_PICTURE){
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)pictureControlSetPtr->refPicQpArray[0],
                                (EB_U32)pictureControlSetPtr->refPicQpArray[0] + 2,
                                pictureControlSetPtr->pictureQp);
                        }
                        else{
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)pictureControlSetPtr->refPicQpArray[0] - 1, 0),
                                (EB_U32)pictureControlSetPtr->refPicQpArray[0] + 1,
                                pictureControlSetPtr->pictureQp);
                        }
                    }
                    if ((EB_S32)pictureControlSetPtr->temporalLayerIndex == 1){
                        refQp = 0;
                        if (pictureControlSetPtr->refSliceTypeArray[0] != EB_I_PICTURE){
                            refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[0]);
                        }
                        if ((pictureControlSetPtr->sliceType == EB_B_PICTURE) && (pictureControlSetPtr->refSliceTypeArray[1] != EB_I_PICTURE)){
                            refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[1]);
                        }
                        if (refQp > 0){
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)refQp - 1,
                                (EB_U32)refQp + 1,
                                pictureControlSetPtr->pictureQp);
                        }
                    }
                }
                    
                if ((EB_S32)pictureControlSetPtr->temporalLayerIndex == 2){
                    refQp = 0;
                    if (pictureControlSetPtr->refSliceTypeArray[0] != EB_I_PICTURE){
                        refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[0]);
                    }

                    if ((pictureControlSetPtr->sliceType == EB_B_PICTURE) && (pictureControlSetPtr->refSliceTypeArray[1] != EB_I_PICTURE)){
                        refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[1]);
                    }
                    if (refQp > 0){
                        if (pictureControlSetPtr->sliceType == EB_P_PICTURE){
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)refQp - 1,0),
                                (EB_U32)refQp + 1,
                                pictureControlSetPtr->pictureQp);
                        }
                        else{
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)refQp - 1, 0),
                                (EB_U32)refQp + 2,
                                pictureControlSetPtr->pictureQp);
                        }
                    }
                }

                if ((EB_S32)pictureControlSetPtr->temporalLayerIndex >= 3){

                    refQp = 0;
                    if (pictureControlSetPtr->refSliceTypeArray[0] != EB_I_PICTURE){
                        refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[0]);
                    }
                    if ((pictureControlSetPtr->sliceType == EB_B_PICTURE) && (pictureControlSetPtr->refSliceTypeArray[1] != EB_I_PICTURE)){
                        refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[1]);
                    }
                    if (refQp > 0){
                        if (pictureControlSetPtr->sliceType == EB_P_PICTURE){
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)refQp - 1, 0),
                                (EB_U32)refQp + 2,
                                pictureControlSetPtr->pictureQp);
                        }
                        else{
                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)refQp - 1, 0),
                                (EB_U32)refQp + 3,
                                pictureControlSetPtr->pictureQp);
                        }
                    }
                }
            }
            else{
                if ((EB_S32)pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){
                    if (pictureControlSetPtr->refSliceTypeArray[0] == EB_I_PICTURE){
                        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                            (EB_U32)MAX((EB_S32)pictureControlSetPtr->refPicQpArray[0] - 2,0),
                            (EB_U32)pictureControlSetPtr->refPicQpArray[0] + 3,
                            pictureControlSetPtr->pictureQp);
                    }
                    else{
                        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                            (EB_U32)MAX((EB_S32)pictureControlSetPtr->refPicQpArray[0] - 2, 0),
                            (EB_U32)pictureControlSetPtr->refPicQpArray[0] + 3,
                            pictureControlSetPtr->pictureQp);
                    }
                }

                if ((EB_S32)pictureControlSetPtr->temporalLayerIndex >= 1){
                    refQp = 0;
                    if (pictureControlSetPtr->refSliceTypeArray[0] != EB_I_PICTURE){
                        refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[0]);
                    }
                    if ((pictureControlSetPtr->sliceType == EB_B_PICTURE) && (pictureControlSetPtr->refSliceTypeArray[1] != EB_I_PICTURE)){
                        refQp = MAX(refQp, pictureControlSetPtr->refPicQpArray[1]);
                    }
                    if (refQp > 0){
                        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                            (EB_U32)MAX((EB_S32)refQp - 2, 0),
                            (EB_U32)refQp + 3,
                            pictureControlSetPtr->pictureQp);
                    }
                }
            }
        }
        // limiting the QP between min Qp allowed and max Qp allowed
        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
            sequenceControlSetPtr->staticConfig.minQpAllowed,
            sequenceControlSetPtr->staticConfig.maxQpAllowed,
            pictureControlSetPtr->pictureQp);

        rateControlLayerPtr->deltaQpFraction = CLIP3(-RC_PRECISION_OFFSET, RC_PRECISION_OFFSET, -((EB_S64)tempQp - (EB_S64)(pictureControlSetPtr->pictureQp << RC_PRECISION)));

        if (pictureControlSetPtr->ParentPcsPtr->sadMe == rateControlLayerPtr->previousFrameSadMe &&
            (rateControlLayerPtr->previousFrameSadMe != 0))
            rateControlLayerPtr->sameSADCount++;
        else
            rateControlLayerPtr->sameSADCount = 0;
    }

    rateControlLayerPtr->previousCCoeff = rateControlLayerPtr->cCoeff;
    rateControlLayerPtr->previousKCoeff = rateControlLayerPtr->kCoeff;
    rateControlLayerPtr->previousCalculatedFrameQp = rateControlLayerPtr->calculatedFrameQp;
}
void FrameLevelRcFeedbackPictureMode2(
    PictureParentControlSet_t         *parentPictureControlSetPtr,
    SequenceControlSet_t              *sequenceControlSetPtr,
    RateControlContext_t              *contextPtr)
{



    RateControlLayerContext_t           *rateControlLayerTempPtr;
    RateControlIntervalParamContext_t   *rateControlParamPtr;
    RateControlLayerContext_t           *rateControlLayerPtr;
    // LCU Loop variables
    EB_U32                       sliceNum;
    EB_U64                       previousFrameBitActual;

    if (sequenceControlSetPtr->intraPeriodLength == -1)
        rateControlParamPtr = contextPtr->rateControlParamQueue[0];
    else{
        EB_U32 intervalIndexTemp = 0;
        while ((!(parentPictureControlSetPtr->pictureNumber >= contextPtr->rateControlParamQueue[intervalIndexTemp]->firstPoc &&
            parentPictureControlSetPtr->pictureNumber <= contextPtr->rateControlParamQueue[intervalIndexTemp]->lastPoc)) &&
            (intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER)){
            intervalIndexTemp++;
        }
        CHECK_REPORT_ERROR(
            intervalIndexTemp != PARALLEL_GOP_MAX_NUMBER,
            sequenceControlSetPtr->encodeContextPtr->appCallbackPtr,
            EB_ENC_RC_ERROR2);
        rateControlParamPtr = contextPtr->rateControlParamQueue[intervalIndexTemp];
    }

    rateControlLayerPtr = rateControlParamPtr->rateControlLayerArray[parentPictureControlSetPtr->temporalLayerIndex];

    rateControlLayerPtr->maxQp = 0;

    rateControlLayerPtr->feedbackArrived = EB_TRUE;
    rateControlLayerPtr->maxQp = MAX(rateControlLayerPtr->maxQp, parentPictureControlSetPtr->pictureQp);

    rateControlLayerPtr->previousFrameQp = parentPictureControlSetPtr->pictureQp;
    rateControlLayerPtr->previousFrameBitActual = parentPictureControlSetPtr->totalNumBits;
    if (parentPictureControlSetPtr->quantizedCoeffNumBits == 0)
        parentPictureControlSetPtr->quantizedCoeffNumBits = 1;
    rateControlLayerPtr->previousFrameQuantizedCoeffBitActual = parentPictureControlSetPtr->quantizedCoeffNumBits;

    // Setting Critical states for adjusting the averaging weights on C and K
    if ((parentPictureControlSetPtr->sadMe  > (3 * rateControlLayerPtr->previousFrameSadMe) >> 1) &&
        (rateControlLayerPtr->previousFrameSadMe != 0)){
        rateControlLayerPtr->criticalStates = 3;
    }
    else if (rateControlLayerPtr->criticalStates){
        rateControlLayerPtr->criticalStates--;
    }
    else{
        rateControlLayerPtr->criticalStates = 0;
    }

    if (parentPictureControlSetPtr->sliceType != EB_I_PICTURE){
        // Updating CCoeff
        rateControlLayerPtr->cCoeff = (((EB_S64)rateControlLayerPtr->previousFrameBitActual - (EB_S64)rateControlLayerPtr->previousFrameQuantizedCoeffBitActual) << (2 * RC_PRECISION))
            / rateControlLayerPtr->areaInPixel;
        rateControlLayerPtr->cCoeff = MAX(rateControlLayerPtr->cCoeff, 1);

        // Updating KCoeff
        if ((parentPictureControlSetPtr->sadMe + RC_PRECISION_OFFSET) >> RC_PRECISION > 5){
            {
                EB_U64 test1, test2, test3;
                test1 = rateControlLayerPtr->previousFrameQuantizedCoeffBitActual*(TWO_TO_POWER_QP_OVER_THREE[parentPictureControlSetPtr->pictureQp]);
                test2 = MAX(parentPictureControlSetPtr->sadMe / rateControlLayerPtr->areaInPixel, 1);
                test3 = test1 * 65536 / test2 * 65536 / parentPictureControlSetPtr->sadMe;

                rateControlLayerPtr->kCoeff = test3;
            }
        }

        if (rateControlLayerPtr->criticalStates){
            rateControlLayerPtr->kCoeff = (8 * rateControlLayerPtr->kCoeff + 8 * rateControlLayerPtr->previousKCoeff + 8) >> 4;
            rateControlLayerPtr->cCoeff = (8 * rateControlLayerPtr->cCoeff + 8 * rateControlLayerPtr->previousCCoeff + 8) >> 4;
        }
        else{
            rateControlLayerPtr->kCoeff = (rateControlLayerPtr->coeffAveragingWeight1*rateControlLayerPtr->kCoeff + rateControlLayerPtr->coeffAveragingWeight2*rateControlLayerPtr->previousKCoeff + 8) >> 4;
            rateControlLayerPtr->cCoeff = (rateControlLayerPtr->coeffAveragingWeight1*rateControlLayerPtr->cCoeff + rateControlLayerPtr->coeffAveragingWeight2*rateControlLayerPtr->previousCCoeff + 8) >> 4;
        }
        rateControlLayerPtr->kCoeff = MIN(rateControlLayerPtr->kCoeff, rateControlLayerPtr->previousKCoeff * 4);
        rateControlLayerPtr->cCoeff = MIN(rateControlLayerPtr->cCoeff, rateControlLayerPtr->previousCCoeff * 4);
		if (parentPictureControlSetPtr->sliceType != EB_I_PICTURE) {
            rateControlLayerPtr->previousFrameSadMe = parentPictureControlSetPtr->sadMe;
        }
        else{
            rateControlLayerPtr->previousFrameSadMe = 0;
        }
    }

    if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){
        if (parentPictureControlSetPtr->sliceType == EB_I_PICTURE){
            if (parentPictureControlSetPtr->totalNumBits < parentPictureControlSetPtr->targetBitsBestPredQp << 1)
                contextPtr->baseLayerIntraFramesAvgQp = (3 * contextPtr->baseLayerIntraFramesAvgQp + parentPictureControlSetPtr->pictureQp + 2) >> 2;
            else if (parentPictureControlSetPtr->totalNumBits > parentPictureControlSetPtr->targetBitsBestPredQp << 2)
                contextPtr->baseLayerIntraFramesAvgQp = (3 * contextPtr->baseLayerIntraFramesAvgQp + parentPictureControlSetPtr->pictureQp + 4 + 2) >> 2;
            else if (parentPictureControlSetPtr->totalNumBits > parentPictureControlSetPtr->targetBitsBestPredQp << 1)
                contextPtr->baseLayerIntraFramesAvgQp = (3 * contextPtr->baseLayerIntraFramesAvgQp + parentPictureControlSetPtr->pictureQp + 2 + 2) >> 2;
        }
    }

  
    {
        EB_U64 previousFrameEcBits = 0;
        EB_BOOL pictureMinQpAllowed = EB_TRUE;
        rateControlLayerPtr->previousFrameAverageQp = 0;
        rateControlLayerPtr->previousFrameAverageQp += rateControlLayerPtr->previousFrameQp;
        previousFrameEcBits += rateControlLayerPtr->previousFrameBitActual;
        if (rateControlLayerPtr->sameSADCount == 0 ||
            parentPictureControlSetPtr->pictureQp != sequenceControlSetPtr->staticConfig.minQpAllowed){
			pictureMinQpAllowed = EB_FALSE;
        }
		if (pictureMinQpAllowed)
            rateControlLayerPtr->frameSameSADMinQpCount++;
        else
            rateControlLayerPtr->frameSameSADMinQpCount = 0;

        rateControlLayerPtr->previousEcBits = previousFrameEcBits;
        previousFrameBitActual = parentPictureControlSetPtr->totalNumBits;
        if (parentPictureControlSetPtr->firstFrameInTemporalLayer){
            rateControlLayerPtr->difTotalAndEcBits = (previousFrameBitActual - previousFrameEcBits);
        }
        else{
            rateControlLayerPtr->difTotalAndEcBits = ((previousFrameBitActual - previousFrameEcBits) + rateControlLayerPtr->difTotalAndEcBits) >> 1;
        }

        // update bitrate of different layers in the interval based on the rate of the I frame
        if (parentPictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc &&
            (parentPictureControlSetPtr->sliceType == EB_I_PICTURE) &&
            sequenceControlSetPtr->staticConfig.intraPeriodLength != -1){
            EB_U32 temporalLayerIdex;
            EB_U64 targetBitRate;
            EB_U64 channelBitRate;
            EB_U64 sumBitsPerSw = 0;
#if ADAPTIVE_PERCENTAGE
            if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){
                if (parentPictureControlSetPtr->tablesUpdated && parentPictureControlSetPtr->percentageUpdated){
                    parentPictureControlSetPtr->bitsPerSwPerLayer[0] =
                        (EB_U64)MAX((EB_S64)parentPictureControlSetPtr->bitsPerSwPerLayer[0] + (EB_S64)parentPictureControlSetPtr->totalNumBits - (EB_S64)parentPictureControlSetPtr->targetBitsBestPredQp, 1);
                }
            }
#endif

            if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0 && sequenceControlSetPtr->intraPeriodLength != -1){
                for (temporalLayerIdex = 0; temporalLayerIdex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIdex++){
                    sumBitsPerSw += parentPictureControlSetPtr->bitsPerSwPerLayer[temporalLayerIdex];
                }
            }

            for (temporalLayerIdex = 0; temporalLayerIdex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIdex++){
                rateControlLayerTempPtr = rateControlParamPtr->rateControlLayerArray[temporalLayerIdex];

                targetBitRate = (EB_U64)((EB_S64)parentPictureControlSetPtr->targetBitRate -
                    MIN((EB_S64)parentPictureControlSetPtr->targetBitRate * 3 / 4, (EB_S64)(parentPictureControlSetPtr->totalNumBits*contextPtr->frameRate / (sequenceControlSetPtr->staticConfig.intraPeriodLength + 1)) >> RC_PRECISION))
                    *RATE_PERCENTAGE_LAYER_ARRAY[sequenceControlSetPtr->staticConfig.hierarchicalLevels][temporalLayerIdex] / 100;

#if ADAPTIVE_PERCENTAGE
                if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0 && sequenceControlSetPtr->intraPeriodLength != -1){
                    targetBitRate = (EB_U64)((EB_S64)parentPictureControlSetPtr->targetBitRate -
                        MIN((EB_S64)parentPictureControlSetPtr->targetBitRate * 3 / 4, (EB_S64)(parentPictureControlSetPtr->totalNumBits*contextPtr->frameRate / (sequenceControlSetPtr->staticConfig.intraPeriodLength + 1)) >> RC_PRECISION))
                        *parentPictureControlSetPtr->bitsPerSwPerLayer[temporalLayerIdex] / sumBitsPerSw;
                }
#endif                            
                // update this based on temporal layers    
                if (temporalLayerIdex == 0)
                    channelBitRate = (((targetBitRate << (2 * RC_PRECISION)) / MAX(1, rateControlLayerTempPtr->frameRate - (1 * contextPtr->frameRate / (sequenceControlSetPtr->staticConfig.intraPeriodLength + 1)))) + RC_PRECISION_OFFSET) >> RC_PRECISION;
                else
                    channelBitRate = (((targetBitRate << (2 * RC_PRECISION)) / rateControlLayerTempPtr->frameRate) + RC_PRECISION_OFFSET) >> RC_PRECISION;
                channelBitRate = (EB_U64)MAX((EB_S64)1, (EB_S64)channelBitRate);
                rateControlLayerTempPtr->ecBitConstraint = channelBitRate;

                sliceNum = 1;
                rateControlLayerTempPtr->ecBitConstraint -= SLICE_HEADER_BITS_NUM*sliceNum;

                rateControlLayerTempPtr->previousBitConstraint = channelBitRate;
                rateControlLayerTempPtr->bitConstraint = channelBitRate;
                rateControlLayerTempPtr->channelBitRate = channelBitRate;
            }
            if ((EB_S64)parentPictureControlSetPtr->targetBitRate * 3 / 4 < (EB_S64)(parentPictureControlSetPtr->totalNumBits*contextPtr->frameRate / (sequenceControlSetPtr->staticConfig.intraPeriodLength + 1)) >> RC_PRECISION){
                rateControlParamPtr->previousVirtualBufferLevel += (EB_S64)((parentPictureControlSetPtr->totalNumBits*contextPtr->frameRate / (sequenceControlSetPtr->staticConfig.intraPeriodLength + 1)) >> RC_PRECISION) - (EB_S64)parentPictureControlSetPtr->targetBitRate * 3 / 4;
                contextPtr->extraBitsGen -= (EB_S64)((parentPictureControlSetPtr->totalNumBits*contextPtr->frameRate / (sequenceControlSetPtr->staticConfig.intraPeriodLength + 1)) >> RC_PRECISION) - (EB_S64)parentPictureControlSetPtr->targetBitRate * 3 / 4;
            }
        }

        if (previousFrameBitActual ){

            EB_U64 bitChangesRate;
            // Updating virtual buffer level and it can be negative
            if ((parentPictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc) &&
                (parentPictureControlSetPtr->sliceType == EB_I_PICTURE) &&
                (rateControlParamPtr->lastGop == EB_FALSE) &&
                sequenceControlSetPtr->staticConfig.intraPeriodLength != -1){
                rateControlParamPtr->virtualBufferLevel =
                    (EB_S64)rateControlParamPtr->previousVirtualBufferLevel;
            }
            else{
                rateControlParamPtr->virtualBufferLevel =
                    (EB_S64)rateControlParamPtr->previousVirtualBufferLevel +
                    (EB_S64)previousFrameBitActual - (EB_S64)rateControlLayerPtr->channelBitRate;
                contextPtr->extraBitsGen -= (EB_S64)previousFrameBitActual - (EB_S64)rateControlLayerPtr->channelBitRate;
            }

            if (parentPictureControlSetPtr->hierarchicalLevels > 1 && rateControlLayerPtr->frameSameSADMinQpCount > 10){
                rateControlLayerPtr->previousBitConstraint = (EB_S64)rateControlLayerPtr->channelBitRate;
                rateControlParamPtr->virtualBufferLevel = ((EB_S64)contextPtr->virtualBufferSize >> 1);
            }
            // Updating bit difference                      
            rateControlLayerPtr->bitDiff = (EB_S64)rateControlParamPtr->virtualBufferLevel
                //- ((EB_S64)contextPtr->virtualBufferSize>>1);
                - ((EB_S64)rateControlLayerPtr->channelBitRate >> 1);

            // Limit the bit difference
            rateControlLayerPtr->bitDiff = CLIP3(-(EB_S64)(rateControlLayerPtr->channelBitRate), (EB_S64)(rateControlLayerPtr->channelBitRate >> 1), rateControlLayerPtr->bitDiff);
            bitChangesRate = rateControlLayerPtr->frameRate;

            // Updating bit Constraint
            rateControlLayerPtr->bitConstraint = MAX((EB_S64)rateControlLayerPtr->previousBitConstraint - ((rateControlLayerPtr->bitDiff << RC_PRECISION) / ((EB_S64)bitChangesRate)), 1);

            // Limiting the bitConstraint
            if (parentPictureControlSetPtr->temporalLayerIndex == 0){
                rateControlLayerPtr->bitConstraint = CLIP3(rateControlLayerPtr->channelBitRate >> 2, 
                    rateControlLayerPtr->channelBitRate * 200 / 100,
                    rateControlLayerPtr->bitConstraint);
            }
            else{
                rateControlLayerPtr->bitConstraint = CLIP3(rateControlLayerPtr->channelBitRate >> 1,
                    rateControlLayerPtr->channelBitRate * 200 / 100,
                    rateControlLayerPtr->bitConstraint);
            }
            rateControlLayerPtr->ecBitConstraint = (EB_U64)MAX((EB_S64)rateControlLayerPtr->bitConstraint - (EB_S64)rateControlLayerPtr->difTotalAndEcBits, 1);
            rateControlParamPtr->previousVirtualBufferLevel = rateControlParamPtr->virtualBufferLevel;
            rateControlLayerPtr->previousBitConstraint = rateControlLayerPtr->bitConstraint;
        }

        rateControlParamPtr->processedFramesNumber++;
        rateControlParamPtr->inUse = EB_TRUE;
        // check if all the frames in the interval have arrived
        if (rateControlParamPtr->processedFramesNumber == (rateControlParamPtr->lastPoc - rateControlParamPtr->firstPoc + 1) &&
            sequenceControlSetPtr->intraPeriodLength != -1){

            EB_U32 temporalIndex;
            EB_S64 extraBits;
            rateControlParamPtr->firstPoc += PARALLEL_GOP_MAX_NUMBER*(EB_U32)(sequenceControlSetPtr->intraPeriodLength + 1);
            rateControlParamPtr->lastPoc += PARALLEL_GOP_MAX_NUMBER*(EB_U32)(sequenceControlSetPtr->intraPeriodLength + 1);
            rateControlParamPtr->processedFramesNumber = 0;
            rateControlParamPtr->extraApBitRatioI = 0;
            rateControlParamPtr->inUse = EB_FALSE;
            rateControlParamPtr->wasUsed = EB_TRUE;
            rateControlParamPtr->lastGop = EB_FALSE;
            rateControlParamPtr->firstPicActualQpAssigned = EB_FALSE;
            for (temporalIndex = 0; temporalIndex < EB_MAX_TEMPORAL_LAYERS; temporalIndex++){
                rateControlParamPtr->rateControlLayerArray[temporalIndex]->firstFrame = 1;
                rateControlParamPtr->rateControlLayerArray[temporalIndex]->firstNonIntraFrame = 1;
                rateControlParamPtr->rateControlLayerArray[temporalIndex]->feedbackArrived = EB_FALSE;
            }
            extraBits = ((EB_S64)contextPtr->virtualBufferSize >> 1) - (EB_S64)rateControlParamPtr->virtualBufferLevel;

            rateControlParamPtr->virtualBufferLevel = contextPtr->virtualBufferSize >> 1;
            contextPtr->extraBits += extraBits;

        }
        // Allocate the extraBits among other GOPs
        if ((parentPictureControlSetPtr->temporalLayerIndex <= 2) &&
            ((contextPtr->extraBits > (EB_S64)(contextPtr->virtualBufferSize >> 8)) ||
            (contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize >> 8)))){
            EB_U32 intervalIndexTemp, intervalInUseCount;
            EB_S64 extraBitsPerGop;
            EB_S64 extraBits = contextPtr->extraBits;
            EB_S32 clipCoef1, clipCoef2;
            if (parentPictureControlSetPtr->endOfSequenceRegion){
                clipCoef1 = -1;
                clipCoef2 = -1;
            }
            else{
                if (contextPtr->extraBits > (EB_S64)(contextPtr->virtualBufferSize << 3) ||
                    contextPtr->extraBits < -(EB_S64)(contextPtr->virtualBufferSize << 3)){
                    clipCoef1 = 0;
                    clipCoef2 = 0;
                }
                else{
                    clipCoef1 = 2;
                    clipCoef2 = 4;
                }
            }

            intervalInUseCount = 0;

            if (extraBits > 0){
                // Extra bits to be distributed
                // Distribute it among those that are consuming more
                for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){
                    if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                        contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel >((EB_S64)contextPtr->virtualBufferSize >> 1)){
                        intervalInUseCount++;
                    }
                }
                // Distribute the rate among them
                if (intervalInUseCount){
                    extraBitsPerGop = extraBits / intervalInUseCount;
                    if (clipCoef1 > 0)
                        extraBitsPerGop = CLIP3(
                        -(EB_S64)contextPtr->virtualBufferSize >> clipCoef1,
                        (EB_S64)contextPtr->virtualBufferSize >> clipCoef1,
                        extraBitsPerGop);
                    else
                        extraBitsPerGop = CLIP3(
                        -(EB_S64)contextPtr->virtualBufferSize << (-clipCoef1),
                        (EB_S64)contextPtr->virtualBufferSize << (-clipCoef1),
                        extraBitsPerGop);

                    for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){
                        if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel >((EB_S64)contextPtr->virtualBufferSize >> 1)){
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel -= extraBitsPerGop;
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->previousVirtualBufferLevel -= extraBitsPerGop;
                            contextPtr->extraBits -= extraBitsPerGop;
                        }
                    }
                }
                // if no interval with more consuming was found, allocate it to ones with consuming less
                else{
                    intervalInUseCount = 0;
                    // Distribute it among those that are consuming less
                    for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){

                        if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel <= ((EB_S64)contextPtr->virtualBufferSize >> 1)){
                            intervalInUseCount++;
                        }
                    }
                    if (intervalInUseCount){
                        extraBitsPerGop = extraBits / intervalInUseCount;
                        if (clipCoef2 > 0)
                            extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->virtualBufferSize >> clipCoef2,
                            (EB_S64)contextPtr->virtualBufferSize >> clipCoef2,
                            extraBitsPerGop);
                        else
                            extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->virtualBufferSize << (-clipCoef2),
                            (EB_S64)contextPtr->virtualBufferSize << (-clipCoef2),
                            extraBitsPerGop);
                        // Distribute the rate among them
                        for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){

                            if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                                contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel <= ((EB_S64)contextPtr->virtualBufferSize >> 1)){
                                contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel -= extraBitsPerGop;
                                contextPtr->rateControlParamQueue[intervalIndexTemp]->previousVirtualBufferLevel -= extraBitsPerGop;
                                contextPtr->extraBits -= extraBitsPerGop;
                            }
                        }
                    }
                }
            }
            else{
                // Distribute it among those that are consuming less
                for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){

                    if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                        contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel < ((EB_S64)contextPtr->virtualBufferSize >> 1)){
                        intervalInUseCount++;
                    }
                }
                if (intervalInUseCount){
                    extraBitsPerGop = extraBits / intervalInUseCount;
                    if (clipCoef1 > 0)
                        extraBitsPerGop = CLIP3(
                        -(EB_S64)contextPtr->virtualBufferSize >> clipCoef1,
                        (EB_S64)contextPtr->virtualBufferSize >> clipCoef1,
                        extraBitsPerGop);
                    else
                        extraBitsPerGop = CLIP3(
                        -(EB_S64)contextPtr->virtualBufferSize << (-clipCoef1),
                        (EB_S64)contextPtr->virtualBufferSize << (-clipCoef1),
                        extraBitsPerGop);
                    // Distribute the rate among them
                    for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){
                        if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel < ((EB_S64)contextPtr->virtualBufferSize >> 1)){
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel -= extraBitsPerGop;
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->previousVirtualBufferLevel -= extraBitsPerGop;
                            contextPtr->extraBits -= extraBitsPerGop;
                        }
                    }
                }
                // if no interval with less consuming was found, allocate it to ones with consuming more
                else{
                    intervalInUseCount = 0;
                    for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){

                        if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                            contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel < (EB_S64)(contextPtr->virtualBufferSize)){
                            intervalInUseCount++;
                        }
                    }
                    if (intervalInUseCount){
                        extraBitsPerGop = extraBits / intervalInUseCount;
                        if (clipCoef2 > 0)
                            extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->virtualBufferSize >> clipCoef2,
                            (EB_S64)contextPtr->virtualBufferSize >> clipCoef2,
                            extraBitsPerGop);
                        else
                            extraBitsPerGop = CLIP3(
                            -(EB_S64)contextPtr->virtualBufferSize << (-clipCoef2),
                            (EB_S64)contextPtr->virtualBufferSize << (-clipCoef2),
                            extraBitsPerGop);
                        // Distribute the rate among them
                        for (intervalIndexTemp = 0; intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER; intervalIndexTemp++){

                            if (contextPtr->rateControlParamQueue[intervalIndexTemp]->inUse &&
                                contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel < (EB_S64)(contextPtr->virtualBufferSize)){
                                contextPtr->rateControlParamQueue[intervalIndexTemp]->virtualBufferLevel -= extraBitsPerGop;
                                contextPtr->rateControlParamQueue[intervalIndexTemp]->previousVirtualBufferLevel -= extraBitsPerGop;
                                contextPtr->extraBits -= extraBitsPerGop;
                            }
                        }
                    }
                }
            }
        }
    }
}

void HighLevelRcFeedBackPicture(
    PictureParentControlSet_t         *pictureControlSetPtr,
    SequenceControlSet_t              *sequenceControlSetPtr)
{

    // Queue variables
    HlRateControlHistogramEntry_t      *hlRateControlHistogramPtrTemp;
    EB_U32                             queueEntryIndexHeadTemp;


    //SVT_LOG("\nOut:%d Slidings: ",pictureControlSetPtr->pictureNumber);        
    if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){

        // Update the coded rate in the histogram queue   
        if (pictureControlSetPtr->pictureNumber >= sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueue[sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber){
            queueEntryIndexHeadTemp = (EB_S32)(pictureControlSetPtr->pictureNumber - sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueue[sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
            queueEntryIndexHeadTemp += sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
            queueEntryIndexHeadTemp = (queueEntryIndexHeadTemp > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
                queueEntryIndexHeadTemp - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
                queueEntryIndexHeadTemp;

            hlRateControlHistogramPtrTemp = (sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueue[queueEntryIndexHeadTemp]);
            if (hlRateControlHistogramPtrTemp->pictureNumber == pictureControlSetPtr->pictureNumber &&
                hlRateControlHistogramPtrTemp->passedToHlrc){
                EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
                hlRateControlHistogramPtrTemp->totalNumBitsCoded = pictureControlSetPtr->totalNumBits;
                hlRateControlHistogramPtrTemp->isCoded = EB_TRUE;
                EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
            }
        }

    }
}
void* RateControlKernel(void *inputPtr)
{
    // Context
    RateControlContext_t        *contextPtr = (RateControlContext_t*)inputPtr;
    // EncodeContext_t             *encodeContextPtr;

    RateControlIntervalParamContext_t *rateControlParamPtr;

    RateControlIntervalParamContext_t *prevGopRateControlParamPtr;
    RateControlIntervalParamContext_t *nextGopRateControlParamPtr;

    PictureControlSet_t         *pictureControlSetPtr;
    PictureParentControlSet_t   *parentPictureControlSetPtr;

    // Config
    SequenceControlSet_t        *sequenceControlSetPtr;

    // Input
    EbObjectWrapper_t           *rateControlTasksWrapperPtr;
    RateControlTasks_t          *rateControlTasksPtr;

    // Output
    EbObjectWrapper_t           *rateControlResultsWrapperPtr;
    RateControlResults_t        *rateControlResultsPtr;

    RateControlLayerContext_t   *rateControlLayerPtr;

    // LCU Loop variables
    EB_U32                       lcuTotalCount;
    LargestCodingUnit_t         *lcuPtr;
    EB_U32                       lcuCodingOrder;
    EB_U64                       totalNumberOfFbFrames = 0;
    EB_U32                       bestOisCuIndex = 0;

    RATE_CONTROL_TASKTYPES       taskType;

    for (;;) {

        // Get RateControl Task
        EbGetFullObject(
            contextPtr->rateControlInputTasksFifoPtr,
            &rateControlTasksWrapperPtr);

        rateControlTasksPtr = (RateControlTasks_t*)rateControlTasksWrapperPtr->objectPtr;
        taskType = rateControlTasksPtr->taskType;

        // Modify these for different temporal layers later
        switch (taskType){

        case RC_PICTURE_MANAGER_RESULT:

            pictureControlSetPtr = (PictureControlSet_t*)rateControlTasksPtr->pictureControlSetWrapperPtr->objectPtr;
            sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
#if DEADLOCK_DEBUG
            SVT_LOG("POC %lld RC IN \n", pictureControlSetPtr->pictureNumber);
#endif
            // High level RC
            if (pictureControlSetPtr->pictureNumber == 0){

                contextPtr->highLevelRateControlPtr->targetBitRate                  = sequenceControlSetPtr->staticConfig.targetBitRate;
                contextPtr->highLevelRateControlPtr->frameRate                      = sequenceControlSetPtr->frameRate;
                contextPtr->highLevelRateControlPtr->channelBitRatePerFrame         = (EB_U64)MAX((EB_S64)1, (EB_S64)((contextPtr->highLevelRateControlPtr->targetBitRate << RC_PRECISION) / contextPtr->highLevelRateControlPtr->frameRate));

                contextPtr->highLevelRateControlPtr->channelBitRatePerSw            = contextPtr->highLevelRateControlPtr->channelBitRatePerFrame * (sequenceControlSetPtr->staticConfig.lookAheadDistance + 1);
                contextPtr->highLevelRateControlPtr->bitConstraintPerSw             = contextPtr->highLevelRateControlPtr->channelBitRatePerSw;

#if RC_UPDATE_TARGET_RATE
                contextPtr->highLevelRateControlPtr->previousUpdatedBitConstraintPerSw = contextPtr->highLevelRateControlPtr->channelBitRatePerSw;
#endif

                EB_S32 totalFrameInInterval = sequenceControlSetPtr->intraPeriodLength;
                EB_U32 gopPeriod = (1 << pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels);
                contextPtr->frameRate = sequenceControlSetPtr->frameRate;
                while (totalFrameInInterval >= 0){
                    if (totalFrameInInterval % (gopPeriod) == 0)
                        contextPtr->framesInInterval[0] ++;
                    else if (totalFrameInInterval % (gopPeriod >> 1) == 0)
                        contextPtr->framesInInterval[1] ++;
                    else if (totalFrameInInterval % (gopPeriod >> 2) == 0)
                        contextPtr->framesInInterval[2] ++;
                    else if (totalFrameInInterval % (gopPeriod >> 3) == 0)
                        contextPtr->framesInInterval[3] ++;
                    else if (totalFrameInInterval % (gopPeriod >> 4) == 0)
                        contextPtr->framesInInterval[4] ++;
                    else if (totalFrameInInterval % (gopPeriod >> 5) == 0)
                        contextPtr->framesInInterval[5] ++;
                    totalFrameInInterval--;
                }
                contextPtr->virtualBufferSize               = (((EB_U64)sequenceControlSetPtr->staticConfig.targetBitRate*3) << RC_PRECISION) / (contextPtr->frameRate);
                contextPtr->rateAveragePeriodinFrames       = (EB_U64)sequenceControlSetPtr->staticConfig.intraPeriodLength + 1;
                contextPtr->virtualBufferLevelInitialValue  = contextPtr->virtualBufferSize >> 1;
                contextPtr->virtualBufferLevel              = contextPtr->virtualBufferSize >> 1;
                contextPtr->previousVirtualBufferLevel      = contextPtr->virtualBufferSize >> 1;
                contextPtr->vbFillThreshold1                = (contextPtr->virtualBufferSize * 6) >> 3;
                contextPtr->vbFillThreshold2                = (contextPtr->virtualBufferSize << 3) >> 3;
                contextPtr->baseLayerFramesAvgQp            = sequenceControlSetPtr->qp;
                contextPtr->baseLayerIntraFramesAvgQp       = sequenceControlSetPtr->qp;
            }
            if (sequenceControlSetPtr->staticConfig.rateControlMode)
            {
                pictureControlSetPtr->ParentPcsPtr->intraSelectedOrgQp = 0;
                HighLevelRcInputPictureMode2(
                    pictureControlSetPtr->ParentPcsPtr,
                    sequenceControlSetPtr,
                    sequenceControlSetPtr->encodeContextPtr,
                    contextPtr,
                    contextPtr->highLevelRateControlPtr);


            }

            // Frame level RC
            if (sequenceControlSetPtr->intraPeriodLength == -1 || sequenceControlSetPtr->staticConfig.rateControlMode == 0){
                rateControlParamPtr = contextPtr->rateControlParamQueue[0];
                prevGopRateControlParamPtr = contextPtr->rateControlParamQueue[0];
                nextGopRateControlParamPtr = contextPtr->rateControlParamQueue[0];
            }
            else{
                EB_U32 intervalIndexTemp = 0;
                EB_BOOL intervalFound = EB_FALSE;
                while ((intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER) && !intervalFound){

                    if (pictureControlSetPtr->pictureNumber >= contextPtr->rateControlParamQueue[intervalIndexTemp]->firstPoc &&
                        pictureControlSetPtr->pictureNumber <= contextPtr->rateControlParamQueue[intervalIndexTemp]->lastPoc) {
                        intervalFound = EB_TRUE;
                    }
                    else{
                        intervalIndexTemp++;
                    }
                }
                CHECK_REPORT_ERROR(
                    intervalIndexTemp != PARALLEL_GOP_MAX_NUMBER,
                    sequenceControlSetPtr->encodeContextPtr->appCallbackPtr,
                    EB_ENC_RC_ERROR2);

                rateControlParamPtr = contextPtr->rateControlParamQueue[intervalIndexTemp];

                prevGopRateControlParamPtr = (intervalIndexTemp == 0) ?
                    contextPtr->rateControlParamQueue[PARALLEL_GOP_MAX_NUMBER - 1] :
                    contextPtr->rateControlParamQueue[intervalIndexTemp - 1];
                nextGopRateControlParamPtr = (intervalIndexTemp == PARALLEL_GOP_MAX_NUMBER-1) ?
                    contextPtr->rateControlParamQueue[0] :
                    contextPtr->rateControlParamQueue[intervalIndexTemp + 1];
            }

            rateControlLayerPtr = rateControlParamPtr->rateControlLayerArray[pictureControlSetPtr->temporalLayerIndex];

            lcuTotalCount = pictureControlSetPtr->lcuTotalCount;

            // ***Rate Control***
            //SVT_LOG("\nRate Control Thread %d\n", (int)  pictureControlSetPtr->ParentPcsPtr->pictureNumber);
            if (sequenceControlSetPtr->staticConfig.rateControlMode == 0){
                // if RC mode is 0,  fixed QP is used                   
                // QP scaling based on POC number for Flat IPPP structure

                if (sequenceControlSetPtr->enableQpScalingFlag && pictureControlSetPtr->ParentPcsPtr->qpOnTheFly == EB_FALSE){

                    if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
                        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3((EB_S32)sequenceControlSetPtr->staticConfig.minQpAllowed, (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed, (EB_S32)(sequenceControlSetPtr->qp) + contextPtr->maxRateAdjustDeltaQP);
                    }
                    else{
                        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3((EB_S32)sequenceControlSetPtr->staticConfig.minQpAllowed, (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed, (EB_S32)(sequenceControlSetPtr->qp + MOD_QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]));

                    }
                }

                else if (pictureControlSetPtr->ParentPcsPtr->qpOnTheFly == EB_TRUE){

                    pictureControlSetPtr->pictureQp = (EB_U8)CLIP3((EB_S32)sequenceControlSetPtr->staticConfig.minQpAllowed, (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed,pictureControlSetPtr->ParentPcsPtr->pictureQp);
                }

            }
            else{
                    FrameLevelRcInputPictureMode2(
                        pictureControlSetPtr,
                        sequenceControlSetPtr,
                        contextPtr,
                        rateControlLayerPtr,
                        rateControlParamPtr,
                        bestOisCuIndex);

                    if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc && pictureControlSetPtr->pictureNumber != 0 && !prevGopRateControlParamPtr->sceneChangeInGop){
                        EB_S16 deltaApQp = (EB_S16)prevGopRateControlParamPtr->firstPicActualQp - (EB_S16)prevGopRateControlParamPtr->firstPicPredQp;
                        EB_S64 extraApBitRatio = (prevGopRateControlParamPtr->firstPicPredBits != 0) ?
                            (((EB_S64)prevGopRateControlParamPtr->firstPicActualBits - (EB_S64)prevGopRateControlParamPtr->firstPicPredBits) * 100) / ((EB_S64)prevGopRateControlParamPtr->firstPicPredBits) :
                            0;
                        extraApBitRatio += (EB_S64)deltaApQp * 15;
                        if (extraApBitRatio > 200){
                            pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + 3;
                        }
                        else if (extraApBitRatio > 100){
                            pictureControlSetPtr->pictureQp = pictureControlSetPtr->pictureQp + 2;
                        }
                        else if (extraApBitRatio > 50){
                            pictureControlSetPtr->pictureQp++;
                        }
                    }

                    if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc && pictureControlSetPtr->pictureNumber != 0){
                        EB_U8 qpIncAllowed = 3;
                        EB_U8 qpDecAllowed = 4;
                        if (pictureControlSetPtr->ParentPcsPtr->intraSelectedOrgQp + 10 <= prevGopRateControlParamPtr->firstPicActualQp)
                        {
                            qpDecAllowed = (EB_U8)(prevGopRateControlParamPtr->firstPicActualQp - pictureControlSetPtr->ParentPcsPtr->intraSelectedOrgQp) >> 1;
                        }

                        if (pictureControlSetPtr->ParentPcsPtr->intraSelectedOrgQp >= prevGopRateControlParamPtr->firstPicActualQp + 10)
                        {
                            qpIncAllowed = (EB_U8)(pictureControlSetPtr->ParentPcsPtr->intraSelectedOrgQp - prevGopRateControlParamPtr->firstPicActualQp) * 2 / 3;
                            if (prevGopRateControlParamPtr->firstPicActualQp <= 15)
                                qpIncAllowed += 5;
                            else if (prevGopRateControlParamPtr->firstPicActualQp <= 20)
                                qpIncAllowed += 4;
                            else if (prevGopRateControlParamPtr->firstPicActualQp <= 25)
                                qpIncAllowed += 3;
                        }
                        else if (prevGopRateControlParamPtr->sceneChangeInGop){
                            qpIncAllowed = 5;
                        }
                        if (pictureControlSetPtr->ParentPcsPtr->endOfSequenceRegion){
                            qpIncAllowed += 2;
                            qpDecAllowed += 4;
                        }
                        pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                            (EB_U32)MAX((EB_S32)prevGopRateControlParamPtr->firstPicActualQp - (EB_S32)qpDecAllowed, 0),
                            (EB_U32)prevGopRateControlParamPtr->firstPicActualQp + qpIncAllowed,
                            pictureControlSetPtr->pictureQp);
                    }

                    // Scene change
                    if (pictureControlSetPtr->sliceType == EB_I_PICTURE && pictureControlSetPtr->pictureNumber != rateControlParamPtr->firstPoc){
                        if (nextGopRateControlParamPtr->firstPicActualQpAssigned){

                            pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                (EB_U32)MAX((EB_S32)nextGopRateControlParamPtr->firstPicActualQp - (EB_S32)1, 0),
                                (EB_U32)nextGopRateControlParamPtr->firstPicActualQp + 8,
                                pictureControlSetPtr->pictureQp);
                        }
                        else{
                            if (rateControlParamPtr->firstPicActualQp < 20){
                                pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                    (EB_U32)MAX((EB_S32)rateControlParamPtr->firstPicActualQp - (EB_S32)4, 0),
                                    (EB_U32)rateControlParamPtr->firstPicActualQp + 10,
                                    pictureControlSetPtr->pictureQp);
                            }
                            else{
                                pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                                    (EB_U32)MAX((EB_S32)rateControlParamPtr->firstPicActualQp - (EB_S32)4, 0),
                                    (EB_U32)rateControlParamPtr->firstPicActualQp + 8,
                                    pictureControlSetPtr->pictureQp);
                            
                            }

                        }
                    }
                    pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        sequenceControlSetPtr->staticConfig.minQpAllowed,
                        sequenceControlSetPtr->staticConfig.maxQpAllowed,
                        pictureControlSetPtr->pictureQp);
            }
            if (sequenceControlSetPtr->intraPeriodLength != -1 && pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels < 2 && (EB_S32)pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){
                if (nextGopRateControlParamPtr->firstPicActualQpAssigned || nextGopRateControlParamPtr->wasUsed){

                    pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        (EB_U32)MAX((EB_S32)nextGopRateControlParamPtr->firstPicActualQp - (EB_S32)4, 0),
                        (EB_U32)pictureControlSetPtr->pictureQp,
                        pictureControlSetPtr->pictureQp);
                }
                else{
                    pictureControlSetPtr->pictureQp = (EB_U8)CLIP3(
                        (EB_U32)MAX((EB_S32)rateControlParamPtr->firstPicActualQp - (EB_S32)4, 0),
                        (EB_U32)pictureControlSetPtr->pictureQp,
                        pictureControlSetPtr->pictureQp);
                }
            }
            pictureControlSetPtr->ParentPcsPtr->pictureQp = pictureControlSetPtr->pictureQp;
            if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0 && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){
                contextPtr->baseLayerFramesAvgQp = (3 * contextPtr->baseLayerFramesAvgQp + pictureControlSetPtr->pictureQp + 2) >> 2;
            }
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE){
                if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc){
                    rateControlParamPtr->firstPicPredQp = (EB_U16) pictureControlSetPtr->ParentPcsPtr->bestPredQp;
                    rateControlParamPtr->firstPicActualQp = (EB_U16) pictureControlSetPtr->pictureQp;
                    rateControlParamPtr->sceneChangeInGop = pictureControlSetPtr->ParentPcsPtr->sceneChangeInGop;
                    rateControlParamPtr->firstPicActualQpAssigned = EB_TRUE;
                }
                {
                    if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc){
                        if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){
                            contextPtr->baseLayerIntraFramesAvgQp = (3 * contextPtr->baseLayerIntraFramesAvgQp + pictureControlSetPtr->pictureQp + 2) >> 2;
                        }
                    }

                    if (pictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc){
                        rateControlParamPtr->intraFramesQp = pictureControlSetPtr->pictureQp;
                        rateControlParamPtr->nextGopIntraFrameQp = pictureControlSetPtr->pictureQp;

                    }
                }
            }
            pictureControlSetPtr->ParentPcsPtr->averageQp = 0;
            for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {

                lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];
                lcuPtr->qp = (EB_U8)pictureControlSetPtr->pictureQp;
                pictureControlSetPtr->ParentPcsPtr->averageQp += lcuPtr->qp;
            }
            pictureControlSetPtr->ParentPcsPtr->averageQp = pictureControlSetPtr->ParentPcsPtr->averageQp / lcuTotalCount;

            // Get Empty Rate Control Results Buffer
            EbGetEmptyObject(
                contextPtr->rateControlOutputResultsFifoPtr,
                &rateControlResultsWrapperPtr);
            rateControlResultsPtr = (RateControlResults_t*)rateControlResultsWrapperPtr->objectPtr;
            rateControlResultsPtr->pictureControlSetWrapperPtr = rateControlTasksPtr->pictureControlSetWrapperPtr;

#if DEADLOCK_DEBUG
            SVT_LOG("POC %lld RC OUT \n", pictureControlSetPtr->pictureNumber);
#endif

            // Post Full Rate Control Results
            EbPostFullObject(rateControlResultsWrapperPtr);

            // Release Rate Control Tasks
            EbReleaseObject(rateControlTasksWrapperPtr);

            break;

        case RC_PACKETIZATION_FEEDBACK_RESULT:
            //loopCount++;
            //SVT_LOG("Rate Control Thread FeedBack %d\n", (int) loopCount);            

            parentPictureControlSetPtr = (PictureParentControlSet_t*)rateControlTasksPtr->pictureControlSetWrapperPtr->objectPtr;
            sequenceControlSetPtr = (SequenceControlSet_t*)parentPictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
            // Frame level RC
            if (sequenceControlSetPtr->intraPeriodLength == -1 || sequenceControlSetPtr->staticConfig.rateControlMode == 0){
                rateControlParamPtr = contextPtr->rateControlParamQueue[0];
                prevGopRateControlParamPtr = contextPtr->rateControlParamQueue[0];
                if (parentPictureControlSetPtr->sliceType == EB_I_PICTURE){

                    if (parentPictureControlSetPtr->totalNumBits > MAX_BITS_PER_FRAME){
                        contextPtr->maxRateAdjustDeltaQP++;
                    }
                    else if (contextPtr->maxRateAdjustDeltaQP > 0 && parentPictureControlSetPtr->totalNumBits < MAX_BITS_PER_FRAME * 85 / 100){
                        contextPtr->maxRateAdjustDeltaQP--;
                    }
                    contextPtr->maxRateAdjustDeltaQP = CLIP3(0, 51, contextPtr->maxRateAdjustDeltaQP);
                    contextPtr->maxRateAdjustDeltaQP = 0;
                }
            }
            else{
                EB_U32 intervalIndexTemp = 0;
                EB_BOOL intervalFound = EB_FALSE;
                while ((intervalIndexTemp < PARALLEL_GOP_MAX_NUMBER) && !intervalFound){

                    if (parentPictureControlSetPtr->pictureNumber >= contextPtr->rateControlParamQueue[intervalIndexTemp]->firstPoc &&
                        parentPictureControlSetPtr->pictureNumber <= contextPtr->rateControlParamQueue[intervalIndexTemp]->lastPoc) {
                        intervalFound = EB_TRUE;
                    }
                    else{
                        intervalIndexTemp++;
                    }
                }
                CHECK_REPORT_ERROR(
                    intervalIndexTemp != PARALLEL_GOP_MAX_NUMBER,
                    sequenceControlSetPtr->encodeContextPtr->appCallbackPtr,
                    EB_ENC_RC_ERROR2);

                rateControlParamPtr = contextPtr->rateControlParamQueue[intervalIndexTemp];

                prevGopRateControlParamPtr = (intervalIndexTemp == 0) ?
                    contextPtr->rateControlParamQueue[PARALLEL_GOP_MAX_NUMBER - 1] :
                    contextPtr->rateControlParamQueue[intervalIndexTemp - 1];

            }
            if (sequenceControlSetPtr->staticConfig.rateControlMode != 0){

                contextPtr->previousVirtualBufferLevel = contextPtr->virtualBufferLevel;

                contextPtr->virtualBufferLevel =
                    (EB_S64)contextPtr->previousVirtualBufferLevel +
                    (EB_S64)parentPictureControlSetPtr->totalNumBits - (EB_S64)contextPtr->highLevelRateControlPtr->channelBitRatePerFrame;

                HighLevelRcFeedBackPicture(
                    parentPictureControlSetPtr,
                    sequenceControlSetPtr);
                    FrameLevelRcFeedbackPictureMode2(
                        parentPictureControlSetPtr,
                        sequenceControlSetPtr,
                        contextPtr);
                if (parentPictureControlSetPtr->pictureNumber == rateControlParamPtr->firstPoc){
                    rateControlParamPtr->firstPicPredBits   = parentPictureControlSetPtr->targetBitsBestPredQp;  
                    rateControlParamPtr->firstPicActualBits = parentPictureControlSetPtr->totalNumBits;
                    {
                        EB_S16 deltaApQp = (EB_S16)rateControlParamPtr->firstPicActualQp - (EB_S16)rateControlParamPtr->firstPicPredQp;
                        rateControlParamPtr->extraApBitRatioI = (rateControlParamPtr->firstPicPredBits != 0) ?
                            (((EB_S64)rateControlParamPtr->firstPicActualBits - (EB_S64)rateControlParamPtr->firstPicPredBits) * 100) / ((EB_S64)rateControlParamPtr->firstPicPredBits) :
                            0;
                        rateControlParamPtr->extraApBitRatioI += (EB_S64)deltaApQp * 15;
                    }
                   

                }

            }

            // Queue variables
#if OVERSHOOT_STAT_PRINT
            if (sequenceControlSetPtr->intraPeriodLength != -1){
                
                EB_S32                       queueEntryIndex;
                EB_U32                       queueEntryIndexTemp;
                EB_U32                       queueEntryIndexTemp2;
                CodedFramesStatsEntry_t     *queueEntryPtr;
                EB_BOOL                      moveSlideWondowFlag = EB_TRUE;
                EB_BOOL                      endOfSequenceFlag = EB_TRUE;
                EB_U32                       framesInSw;

                // Determine offset from the Head Ptr          
                queueEntryIndex = (EB_S32)(parentPictureControlSetPtr->pictureNumber - contextPtr->codedFramesStatQueue[contextPtr->codedFramesStatQueueHeadIndex]->pictureNumber);
                queueEntryIndex += contextPtr->codedFramesStatQueueHeadIndex;
                queueEntryIndex = (queueEntryIndex > CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? queueEntryIndex - CODED_FRAMES_STAT_QUEUE_MAX_DEPTH : queueEntryIndex;
                queueEntryPtr   = contextPtr->codedFramesStatQueue[queueEntryIndex];

                queueEntryPtr->frameTotalBitActual  = (EB_U64)parentPictureControlSetPtr->totalNumBits;
                queueEntryPtr->pictureNumber        = parentPictureControlSetPtr->pictureNumber;
                queueEntryPtr->endOfSequenceFlag    = parentPictureControlSetPtr->endOfSequenceFlag;
                contextPtr->rateAveragePeriodinFrames = (EB_U64)sequenceControlSetPtr->staticConfig.intraPeriodLength + 1;

                //SVT_LOG("\n0_POC: %d\n",
                //    queueEntryPtr->pictureNumber);
                moveSlideWondowFlag = EB_TRUE;
                while (moveSlideWondowFlag){
                  //  SVT_LOG("\n1_POC: %d\n",
                  //      queueEntryPtr->pictureNumber);
                    // Check if the sliding window condition is valid
                    queueEntryIndexTemp = contextPtr->codedFramesStatQueueHeadIndex;
                    if (contextPtr->codedFramesStatQueue[queueEntryIndexTemp]->frameTotalBitActual != -1){
                        endOfSequenceFlag = contextPtr->codedFramesStatQueue[queueEntryIndexTemp]->endOfSequenceFlag;
                    }
                    else{
                        endOfSequenceFlag = EB_FALSE;
                    }
                    while (moveSlideWondowFlag && !endOfSequenceFlag &&
                        queueEntryIndexTemp < contextPtr->codedFramesStatQueueHeadIndex + contextPtr->rateAveragePeriodinFrames){
                       // SVT_LOG("\n2_POC: %d\n",
                       //     queueEntryPtr->pictureNumber);

                        queueEntryIndexTemp2 = (queueEntryIndexTemp > CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - CODED_FRAMES_STAT_QUEUE_MAX_DEPTH : queueEntryIndexTemp;

                        moveSlideWondowFlag = (EB_BOOL)(moveSlideWondowFlag && (contextPtr->codedFramesStatQueue[queueEntryIndexTemp2]->frameTotalBitActual != -1));

                        if (contextPtr->codedFramesStatQueue[queueEntryIndexTemp2]->frameTotalBitActual != -1){
                            // check if it is the last frame. If we have reached the last frame, we would output the buffered frames in the Queue.
                            endOfSequenceFlag = contextPtr->codedFramesStatQueue[queueEntryIndexTemp]->endOfSequenceFlag;
                        }
                        else{
                            endOfSequenceFlag = EB_FALSE;
                        }
                        queueEntryIndexTemp =
                            (queueEntryIndexTemp == CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? 0 : queueEntryIndexTemp + 1;

                    }

                    if (moveSlideWondowFlag)  {
                        //get a new entry spot
                        queueEntryPtr = (contextPtr->codedFramesStatQueue[contextPtr->codedFramesStatQueueHeadIndex]);
                        queueEntryIndexTemp = contextPtr->codedFramesStatQueueHeadIndex;
                        // This is set to false, so the last frame would go inside the loop
                        endOfSequenceFlag = EB_FALSE;
                        framesInSw = 0;
                        contextPtr->totalBitActualPerSw = 0;

                        while (!endOfSequenceFlag &&
                            queueEntryIndexTemp < contextPtr->codedFramesStatQueueHeadIndex + contextPtr->rateAveragePeriodinFrames){
                            framesInSw++;

                            queueEntryIndexTemp2 = (queueEntryIndexTemp > CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - CODED_FRAMES_STAT_QUEUE_MAX_DEPTH : queueEntryIndexTemp;

                            contextPtr->totalBitActualPerSw += contextPtr->codedFramesStatQueue[queueEntryIndexTemp2]->frameTotalBitActual;
                            endOfSequenceFlag = contextPtr->codedFramesStatQueue[queueEntryIndexTemp2]->endOfSequenceFlag;

                            queueEntryIndexTemp =
                                (queueEntryIndexTemp == CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? 0 : queueEntryIndexTemp + 1;

                        }
                        //

                        //if(framesInSw == contextPtr->rateAveragePeriodinFrames)
                        //    SVT_LOG("POC:%d\t %.3f\n", queueEntryPtr->pictureNumber, (double)contextPtr->totalBitActualPerSw*(sequenceControlSetPtr->frameRate>> RC_PRECISION)/(double)framesInSw/1000);
                        if (framesInSw == (EB_U32)sequenceControlSetPtr->intraPeriodLength + 1){
                            contextPtr->maxBitActualPerSw = MAX(contextPtr->maxBitActualPerSw, contextPtr->totalBitActualPerSw*(sequenceControlSetPtr->frameRate >> RC_PRECISION) / framesInSw / 1000);
                            if (queueEntryPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0){
                                contextPtr->maxBitActualPerGop = MAX(contextPtr->maxBitActualPerGop, contextPtr->totalBitActualPerSw*(sequenceControlSetPtr->frameRate >> RC_PRECISION) / framesInSw / 1000);
                                if (contextPtr->totalBitActualPerSw > sequenceControlSetPtr->staticConfig.maxBufferSize){
                                    SVT_LOG("\nPOC:%d\tOvershoot:%.0f%% \n",
                                        (int)queueEntryPtr->pictureNumber,
                                        (double)((EB_S64)contextPtr->totalBitActualPerSw * 100 / (EB_S64)sequenceControlSetPtr->staticConfig.maxBufferSize - 100));
                                }
                            }
                        }
                        if (framesInSw == contextPtr->rateAveragePeriodinFrames - 1){
                            SVT_LOG("\n%d MAX\n", (EB_S32)contextPtr->maxBitActualPerSw);
                            SVT_LOG("\n%d GopMa\n", (EB_S32)contextPtr->maxBitActualPerGop);
                        }

                        // Reset the Queue Entry
                        queueEntryPtr->pictureNumber += CODED_FRAMES_STAT_QUEUE_MAX_DEPTH;
                        queueEntryPtr->frameTotalBitActual = -1;

                        // Increment the Reorder Queue head Ptr
                        contextPtr->codedFramesStatQueueHeadIndex =
                            (contextPtr->codedFramesStatQueueHeadIndex == CODED_FRAMES_STAT_QUEUE_MAX_DEPTH - 1) ? 0 : contextPtr->codedFramesStatQueueHeadIndex + 1;

                        queueEntryPtr = (contextPtr->codedFramesStatQueue[contextPtr->codedFramesStatQueueHeadIndex]);

                    }
                }
            }
#endif

            totalNumberOfFbFrames++;

			// Release the SequenceControlSet
			EbReleaseObject(parentPictureControlSetPtr->sequenceControlSetWrapperPtr);
            // Release the input buffer
            EbReleaseObject(parentPictureControlSetPtr->ebInputWrapperPtr);
            // Release the ParentPictureControlSet
			EbReleaseObject(rateControlTasksPtr->pictureControlSetWrapperPtr);

			// Release Rate Control Tasks  
            EbReleaseObject(rateControlTasksWrapperPtr);
            break;

        case RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT:

            // Extract bits-per-lcu-row

            // Release Rate Control Tasks  
            EbReleaseObject(rateControlTasksWrapperPtr);

            break;

        default:
            pictureControlSetPtr = (PictureControlSet_t*)rateControlTasksPtr->pictureControlSetWrapperPtr->objectPtr;
            sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
            //encodeContextPtr            = sequenceControlSetPtr->encodeContextPtr;
            //CHECK_REPORT_ERROR_NC(
            //             encodeContextPtr->appCallbackPtr, 
            //             EB_ENC_RC_ERROR1);

            break;
        }
    }
    return EB_NULL;
}
