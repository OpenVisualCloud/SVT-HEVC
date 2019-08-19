/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbPacketizationProcess.h"
#include "EbEntropyCodingResults.h"

#include "EbSystemResourceManager.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"

#include "EbEntropyCoding.h"
#include "EbRateControlTasks.h"
#include "EbRateControlProcess.h"
#include "EbTime.h"
#include "EbPictureDemuxResults.h"

void HrdFullness(SequenceControlSet_t *sequenceControlSetPtr, PictureControlSet_t *pictureControlSetPtr, AppBufferingPeriodSei_t *seiBP)
{
    EB_U32 i;
    const AppVideoUsabilityInfo_t* vui = sequenceControlSetPtr->videoUsabilityInfoPtr;
    const AppHrdParameters_t* hrd = vui->hrdParametersPtr;
    EB_U32 num = 90000;
    EB_U32 denom = (hrd->bitRateValueMinus1[pictureControlSetPtr->temporalLayerIndex][0][hrd->cpbCountMinus1[0]] + 1) << (hrd->bitRateScale + BR_SHIFT);
    EB_U64 cpbState = sequenceControlSetPtr->encodeContextPtr->bufferFill;
    EB_U64 cpbSize = (hrd->cpbSizeValueMinus1[pictureControlSetPtr->temporalLayerIndex][0][hrd->cpbCountMinus1[0]] + 1) << (hrd->cpbSizeScale + CPB_SHIFT);

    for (i = 0; i < hrd->cpbCountMinus1[0]+1; i++)
    {
        seiBP->initialCpbRemovalDelay[0][i] = (EB_U32)(num * cpbState / denom);
        seiBP->initialCpbRemovalDelayOffset[0][i] = (EB_U32)(num * cpbSize / denom - seiBP->initialCpbRemovalDelay[0][i]);
    }
}

static inline EB_S32 calcScale(EB_U32 x)
{
    EB_U8 lut[16] = { 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };
    EB_S32 y, z = (((x & 0xffff) - 1) >> 27) & 16;
    x >>= z;
    z += y = (((x & 0xff) - 1) >> 28) & 8;
    x >>= y;
    z += y = (((x & 0xf) - 1) >> 29) & 4;
    x >>= y;
    return z + lut[x & 0xf];
}

static inline EB_S32 calcLength(EB_U32 x)
{
    EB_U8 lut[16] = { 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    EB_S32 y, z = (((x >> 16) - 1) >> 27) & 16;
    x >>= z ^ 16;
    z += y = ((x - 0x100) >> 28) & 8;
    x >>= y ^ 8;
    z += y = ((x - 0x10) >> 29) & 4;
    x >>= y ^ 4;
    return z + lut[x];
}

void InitHRD(SequenceControlSet_t *scsPtr)
{
    EB_U32 i, j, k;
    AppHrdParameters_t *hrd = scsPtr->videoUsabilityInfoPtr->hrdParametersPtr;

    // normalize HRD size and rate to the value / scale notation
    hrd->bitRateScale = (EB_U32)CLIP3(0, 15, calcScale(scsPtr->staticConfig.vbvMaxrate) - BR_SHIFT);
    hrd->cpbSizeScale = (EB_U32)CLIP3(0, 15, calcScale(scsPtr->staticConfig.vbvBufsize) - CPB_SHIFT);
    for (i = 0; i < MAX_TEMPORAL_LAYERS; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < MAX_CPB_COUNT; k++)
            {
                hrd->bitRateValueMinus1[i][j][k] = (scsPtr->staticConfig.vbvMaxrate >> (hrd->bitRateScale + BR_SHIFT))-1;
                hrd->cpbSizeValueMinus1[i][j][k] = (scsPtr->staticConfig.vbvBufsize >> (hrd->cpbSizeScale + CPB_SHIFT))-1;
                if (scsPtr->staticConfig.vbvMaxrate == scsPtr->staticConfig.targetBitRate)
                    hrd->cbrFlag[i][j][k] = 1;
            }
    EB_U32 bitRateUnscale = ((scsPtr->staticConfig.vbvMaxrate >> (hrd->bitRateScale + BR_SHIFT)) << (hrd->bitRateScale + BR_SHIFT));
    EB_U32 cpbSizeUnscale = ((scsPtr->staticConfig.vbvBufsize >> (hrd->cpbSizeScale + CPB_SHIFT)) << (hrd->cpbSizeScale + CPB_SHIFT));

    // arbitrary
#define MAX_DURATION 0.5

    EB_U32 maxCpbOutputDelay = (EB_U32)MIN((EB_U32)scsPtr->staticConfig.intraPeriodLength* MAX_DURATION *scsPtr->videoUsabilityInfoPtr->vuiTimeScale / scsPtr->videoUsabilityInfoPtr->vuiNumUnitsInTick, MAX_UNSIGNED_VALUE);
    EB_U32 maxDpbOutputDelay = (EB_U32)(scsPtr->maxDpbSize* MAX_DURATION *scsPtr->videoUsabilityInfoPtr->vuiTimeScale / scsPtr->videoUsabilityInfoPtr->vuiNumUnitsInTick);
    EB_U32 maxDelay = (EB_U32)(90000.0 * cpbSizeUnscale / bitRateUnscale + 0.5);
    hrd->initialCpbRemovalDelayLengthMinus1 = (EB_U32)((2 + CLIP3(4, 22, 32 - calcLength(maxDelay)))-1);
    hrd->auCpbRemovalDelayLengthMinus1 = (EB_U32)((CLIP3(4, 31, 32 - calcLength(maxCpbOutputDelay)))-1);
    hrd->dpbOutputDelayLengthMinus1 = (EB_U32)((CLIP3(4, 31, 32 - calcLength(maxDpbOutputDelay)))-1);

#undef MAX_DURATION
}

EB_ERRORTYPE PacketizationContextCtor(
    PacketizationContext_t **contextDblPtr,
    EbFifo_t                *entropyCodingInputFifoPtr,
    EbFifo_t                *rateControlTasksOutputFifoPtr,
    EbFifo_t                *pictureManagerOutputFifoPtr
)
{
    PacketizationContext_t *contextPtr;
    EB_MALLOC(PacketizationContext_t*, contextPtr, sizeof(PacketizationContext_t), EB_N_PTR);
    *contextDblPtr = contextPtr;
        
    contextPtr->entropyCodingInputFifoPtr      = entropyCodingInputFifoPtr;
    contextPtr->rateControlTasksOutputFifoPtr  = rateControlTasksOutputFifoPtr;
    contextPtr->pictureManagerOutputFifoPtr    = pictureManagerOutputFifoPtr;

    EB_MALLOC(EbPPSConfig_t*, contextPtr->ppsConfig, sizeof(EbPPSConfig_t), EB_N_PTR);
    
	return EB_ErrorNone;
}

void* PacketizationKernel(void *inputPtr)
{
    // Context
    PacketizationContext_t         *contextPtr = (PacketizationContext_t*) inputPtr;
    
    PictureControlSet_t            *pictureControlSetPtr;
    
    // Config
    SequenceControlSet_t           *sequenceControlSetPtr;
    
    // Encoding Context
    EncodeContext_t                *encodeContextPtr;
    
    // Input
    EbObjectWrapper_t              *entropyCodingResultsWrapperPtr;
    EntropyCodingResults_t         *entropyCodingResultsPtr;

    // Output
    EbObjectWrapper_t              *outputStreamWrapperPtr;
    EB_BUFFERHEADERTYPE           *outputStreamPtr;
    EbObjectWrapper_t              *rateControlTasksWrapperPtr;
    RateControlTasks_t             *rateControlTasksPtr;
    EbObjectWrapper_t               *pictureManagerResultsWrapperPtr;
    PictureDemuxResults_t       	*pictureManagerResultPtr;
    // Bitstream copy to output buffer
    Bitstream_t                     bitstream;
    
    // Queue variables
    EB_S32                          queueEntryIndex;
    PacketizationReorderEntry_t    *queueEntryPtr;

    EB_U32                          lcuHeight;
    EB_U32                          lcuWidth;
    EB_U32                          intraSadIntervalIndex;
    EB_U32                          sadIntervalIndex;
    EB_U32                          refQpIndex = 0;       
    EB_U32                          packetizationQp;
       
    EB_U64                          refDecOrder = 0;
    EB_U64                          filler;
    EB_U32                          fillerBytes;
    EB_U64                          bufferRate;
    EB_U16                          tileIdx;
    EB_U16                          tileCnt;

    EB_BOOL                         toInsertHeaders;
    
    for(;;) {
    
        // Get EntropyCoding Results
        EbGetFullObject(
            contextPtr->entropyCodingInputFifoPtr,
            &entropyCodingResultsWrapperPtr);
        EB_CHECK_END_OBJ(entropyCodingResultsWrapperPtr);
        entropyCodingResultsPtr = (EntropyCodingResults_t*) entropyCodingResultsWrapperPtr->objectPtr;
        pictureControlSetPtr    = (PictureControlSet_t*)    entropyCodingResultsPtr->pictureControlSetWrapperPtr->objectPtr;
        sequenceControlSetPtr   = (SequenceControlSet_t*)   pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
        encodeContextPtr        = (EncodeContext_t*)        sequenceControlSetPtr->encodeContextPtr;
        tileCnt = pictureControlSetPtr->ParentPcsPtr->tileRowCount * pictureControlSetPtr->ParentPcsPtr->tileColumnCount; 
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld PK IN \n", pictureControlSetPtr->pictureNumber);
#endif

        //****************************************************
        // Input Entropy Results into Reordering Queue
        //****************************************************
               
        //get a new entry spot
        queueEntryIndex = pictureControlSetPtr->ParentPcsPtr->decodeOrder % PACKETIZATION_REORDER_QUEUE_MAX_DEPTH;          
        queueEntryPtr    = encodeContextPtr->packetizationReorderQueue[queueEntryIndex];
        queueEntryPtr->startTimeSeconds = pictureControlSetPtr->ParentPcsPtr->startTimeSeconds;
        queueEntryPtr->startTimeuSeconds = pictureControlSetPtr->ParentPcsPtr->startTimeuSeconds;

        //TODO: buffer should be big enough to avoid a deadlock here. Add an assert that make the warning       
        // Get Output Bitstream buffer
        outputStreamWrapperPtr   = pictureControlSetPtr->ParentPcsPtr->outputStreamWrapperPtr;
        outputStreamPtr          = (EB_BUFFERHEADERTYPE*) outputStreamWrapperPtr->objectPtr;
        outputStreamPtr->nFlags  = 0; 
        EbBlockOnMutex(encodeContextPtr->terminatingConditionsMutex);
        outputStreamPtr->nFlags |= (encodeContextPtr->terminatingSequenceFlagReceived == EB_TRUE && pictureControlSetPtr->ParentPcsPtr->decodeOrder == encodeContextPtr->terminatingPictureNumber) ? EB_BUFFERFLAG_EOS : 0;
        EbReleaseMutex(encodeContextPtr->terminatingConditionsMutex);
        outputStreamPtr->nFilledLen = 0;
        outputStreamPtr->pts = pictureControlSetPtr->ParentPcsPtr->ebInputPtr->pts;
        outputStreamPtr->dts = pictureControlSetPtr->ParentPcsPtr->decodeOrder - (EB_U64)(1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 1;
        outputStreamPtr->sliceType = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag ? 
                                     pictureControlSetPtr->ParentPcsPtr->idrFlag ? EB_IDR_PICTURE :
                                     pictureControlSetPtr->sliceType : EB_NON_REF_PICTURE;

        outputStreamPtr->pAppPrivate = pictureControlSetPtr->ParentPcsPtr->ebInputPtr->pAppPrivate;
        // Get Empty Rate Control Input Tasks
        EbGetEmptyObject(
            contextPtr->rateControlTasksOutputFifoPtr,
            &rateControlTasksWrapperPtr);
        rateControlTasksPtr                                 = (RateControlTasks_t*) rateControlTasksWrapperPtr->objectPtr; 
        rateControlTasksPtr->pictureControlSetWrapperPtr    = pictureControlSetPtr->PictureParentControlSetWrapperPtr;
        rateControlTasksPtr->taskType                       = RC_PACKETIZATION_FEEDBACK_RESULT;

        if (sequenceControlSetPtr->staticConfig.rateControlMode) {
            // Get Empty Results Object
            EbGetEmptyObject(
                contextPtr->pictureManagerOutputFifoPtr,
                &pictureManagerResultsWrapperPtr);

            pictureManagerResultPtr = (PictureDemuxResults_t*)pictureManagerResultsWrapperPtr->objectPtr;
            pictureManagerResultPtr->pictureNumber = pictureControlSetPtr->pictureNumber;
            pictureManagerResultPtr->pictureType = EB_PIC_FEEDBACK;
            pictureManagerResultPtr->sequenceControlSetWrapperPtr = pictureControlSetPtr->sequenceControlSetWrapperPtr;
        }
        else {
            pictureManagerResultsWrapperPtr = EB_NULL;
            (void) pictureManagerResultPtr;
            (void)pictureManagerResultsWrapperPtr;
        }

        if (sequenceControlSetPtr->profileIdc == 0)
        {
            // Compute Profile Tier and Level Information
            ComputeProfileTierLevelInfo(
                sequenceControlSetPtr);

            ComputeMaxDpbBuffer(
                sequenceControlSetPtr);

            if (sequenceControlSetPtr->staticConfig.hrdFlag == 1)
                InitHRD(sequenceControlSetPtr);
        }

        toInsertHeaders = EB_FALSE;
        if (pictureControlSetPtr->pictureNumber == 0) {
            toInsertHeaders = EB_TRUE;
        } else if ((pictureControlSetPtr->sliceType == EB_I_PICTURE) &&
                   (sequenceControlSetPtr->intraRefreshType >= IDR_REFRESH)) {
            if (sequenceControlSetPtr->staticConfig.rateControlMode) {
                EB_U32 idrCount = pictureControlSetPtr->pictureNumber /
                                  (sequenceControlSetPtr->intraPeriodLength + 1);
                if ((idrCount % (sequenceControlSetPtr->intraRefreshType + 1)) == 0)
                    toInsertHeaders = EB_TRUE;
            } else if (sequenceControlSetPtr->intraRefreshType == IDR_REFRESH) {
                toInsertHeaders = EB_TRUE;
            }
        }

        if (sequenceControlSetPtr->staticConfig.codeVpsSpsPps && toInsertHeaders) {
            // Reset the bitstream before writing to it
            ResetBitstream(
                pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

            if (sequenceControlSetPtr->staticConfig.accessUnitDelimiter) {

                EncodeAUD(
                    pictureControlSetPtr->bitstreamPtr,
                    pictureControlSetPtr->sliceType,
                    pictureControlSetPtr->temporalId);
            }

            // Code the VPS
            EncodeVPS(
                pictureControlSetPtr->bitstreamPtr,
                sequenceControlSetPtr);

            // Code the SPS
            EncodeSPS(
                pictureControlSetPtr->bitstreamPtr,
                sequenceControlSetPtr);
                
           // Code the PPS 
           // *Note - when tiles are enabled, we send a separate PPS for each
           //   temporal layer since Tiles vary across temporal layers

           //  Configure first pps
            contextPtr->ppsConfig->ppsId           = 0;
            contextPtr->ppsConfig->constrainedFlag = 0;
            EncodePPS(
               pictureControlSetPtr->bitstreamPtr,
                sequenceControlSetPtr,
               contextPtr->ppsConfig);

            if (sequenceControlSetPtr->staticConfig.constrainedIntra == EB_TRUE){
                // Configure second pps
                contextPtr->ppsConfig->ppsId = 1;
                contextPtr->ppsConfig->constrainedFlag = 1;

                EncodePPS(
                    pictureControlSetPtr->bitstreamPtr,
                    sequenceControlSetPtr,
                    contextPtr->ppsConfig);
            }

            if (sequenceControlSetPtr->staticConfig.maxCLL || sequenceControlSetPtr->staticConfig.maxFALL) {
                sequenceControlSetPtr->contentLightLevel.maxContentLightLevel = sequenceControlSetPtr->staticConfig.maxCLL;
                sequenceControlSetPtr->contentLightLevel.maxPicAverageLightLevel = sequenceControlSetPtr->staticConfig.maxFALL;
                EncodeContentLightLevelSEI(
                    pictureControlSetPtr->bitstreamPtr,
                    &sequenceControlSetPtr->contentLightLevel);
            }

            if (sequenceControlSetPtr->staticConfig.useMasteringDisplayColorVolume) {
                EncodeMasteringDisplayColorVolumeSEI(
                    pictureControlSetPtr->bitstreamPtr,
                    &sequenceControlSetPtr->masteringDisplayColorVolume);
            }

            if (sequenceControlSetPtr->staticConfig.hrdFlag == 1)
            {
                sequenceControlSetPtr->activeParameterSet.selfContainedCvsFlag = EB_TRUE;
                sequenceControlSetPtr->activeParameterSet.noParameterSetUpdateFlag = EB_TRUE;
                EncodeActiveParameterSetsSEI(
                    pictureControlSetPtr->bitstreamPtr,
                    &sequenceControlSetPtr->activeParameterSet);
            }
            // Flush the Bitstream
            FlushBitstream(
                pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);
            
            // Copy SPS & PPS to the Output Bitstream
            CopyRbspBitstreamToPayload(
                pictureControlSetPtr->bitstreamPtr,
                outputStreamPtr->pBuffer,
                (EB_U32*) &(outputStreamPtr->nFilledLen),
                (EB_U32*) &(outputStreamPtr->nAllocLen),
                encodeContextPtr,
				NAL_UNIT_INVALID);
        }
        
         
        // Bitstream Written Loop
        // This loop writes the result of entropy coding into the bitstream
        {
            EB_U32                       lcuTotalCount;
            LargestCodingUnit_t         *lcuPtr;
            EB_U32                       lcuCodingOrder;
			EB_S32                       qpIndex;

            lcuTotalCount               = pictureControlSetPtr->lcuTotalCount;

            // LCU Loop
            if (sequenceControlSetPtr->staticConfig.rateControlMode > 0){
                EB_U64  sadBits[NUMBER_OF_SAD_INTERVALS]= {0};
                EB_U32  count[NUMBER_OF_SAD_INTERVALS] = {0};

                sequenceControlSetPtr->encodeContextPtr->rateControlTablesArrayUpdated = EB_TRUE;
                
		        for(lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {

                    lcuPtr   = pictureControlSetPtr->lcuPtrArray[lcuCodingOrder];               
            

                    // updating initial rate control tables based on the bits used for encoding LCUs
                    lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuPtr->originX >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaWidth - lcuPtr->originX;
                    lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuPtr->originY >= (EB_U16)MAX_LCU_SIZE) ? lcuPtr->size : sequenceControlSetPtr->lumaHeight - lcuPtr->originY;
                   // refQpIndex = lcuPtr->qp;
                    refQpIndex = pictureControlSetPtr->ParentPcsPtr->pictureQp;

                    //Code for using 64x64 Block
                    if (( lcuWidth == MAX_LCU_SIZE) && (lcuHeight == MAX_LCU_SIZE)){
                        if(pictureControlSetPtr->sliceType == EB_I_PICTURE){

                       
                            intraSadIntervalIndex = pictureControlSetPtr->ParentPcsPtr->intraSadIntervalIndex[lcuCodingOrder];

							sadBits[intraSadIntervalIndex] += lcuPtr->totalBits;
                            count[intraSadIntervalIndex] ++;
                            
                        }
                        else{
                            sadIntervalIndex = pictureControlSetPtr->ParentPcsPtr->interSadIntervalIndex[lcuCodingOrder];

                            sadBits[sadIntervalIndex] += lcuPtr->totalBits;//encodeContextPtr->rateControlTablesArray[qpIndex].sadBits[sadIntervalIndex];//encodeContextPtr->rateControlTablesArray[qpIndex].sadBits[sadIntervalIndex];////;
                            count[sadIntervalIndex] ++;

                        }
					}
				}
                {
                    EB_U32 blkSize = BLKSIZE;
                    EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->rateTableUpdateMutex);
                    {
                        if(pictureControlSetPtr->sliceType == EB_I_PICTURE){
                         //   SVT_LOG("Update After: %d\n", pictureControlSetPtr->pictureNumber);
                            if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE){
                                for (sadIntervalIndex = 0; sadIntervalIndex < NUMBER_OF_INTRA_SAD_INTERVALS; sadIntervalIndex++){
                                    if (count[sadIntervalIndex] > (5 * 64 * 64 / blkSize / blkSize)){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((8 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1 < 15)) >> 16)
                                                + 2 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                    else if (count[sadIntervalIndex] > (1 * 64 * 64 / blkSize / blkSize)){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((5 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1 < 15)) >> 16)
                                                + 5 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);

                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                    else if (count[sadIntervalIndex] == (1 * 64 * 64 / blkSize / blkSize)){//=1
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((2 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1 < 15)) >> 16)
                                                + 8 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);

                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                }
                            }
                            else{
                                for (sadIntervalIndex = 0; sadIntervalIndex < NUMBER_OF_INTRA_SAD_INTERVALS; sadIntervalIndex++){
                                    if(count[sadIntervalIndex]> (10*64*64/blkSize/blkSize) ){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for(qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] = 
                                                (EB_Bit_Number)(((( 8*sadBits[sadIntervalIndex]* TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex-qpIndex+51] +(1<15))>>16)
                                                + 2*(EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] +5)/10);
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                    else if (count[sadIntervalIndex]> (5 * 64 * 64 / blkSize / blkSize)){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((5 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1<15)) >> 16)
                                                + 5 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);

                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                    else if (count[sadIntervalIndex]>= (1 * 64 * 64 / blkSize / blkSize)){//=1
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((1 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1<15)) >> 16)
                                                + 9 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);

                                            encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].intraSadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                }
                            }

                        }
                        else{

                            if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE){
                                for (sadIntervalIndex = 0; sadIntervalIndex < NUMBER_OF_SAD_INTERVALS; sadIntervalIndex++){

                                    if (count[sadIntervalIndex] > (5 * 64 * 64 / blkSize / blkSize)){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((8 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1 < 15)) >> 16)
                                                + 2 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);
                                            // intrinsics used in initial RC are assuming signed 16 bits is the maximum
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));

                                        }
                                    }
                                    else if (count[sadIntervalIndex] > (1 * 64 * 64 / blkSize / blkSize)){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((5 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1 < 15)) >> 16)
                                                + 5 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);
                                            // intrinsics used in initial RC are assuming signed 16 bits is the maximum
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));

                                        }
                                    } 
                                    else if ((sadIntervalIndex > ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) && count[sadIntervalIndex] > (1 * 64 * 64 / blkSize / blkSize)) || (count[sadIntervalIndex] == (1 * 64 * 64 / blkSize / blkSize))){

                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for (qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                (EB_Bit_Number)((((1 * sadBits[sadIntervalIndex] * TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex - qpIndex + 51] + (1 < 15)) >> 16)
                                                + 9 * (EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] + 5) / 10);

                                            // intrinsics used in initial RC are assuming signed 16 bits is the maximum
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1 << 15) - 1));
                                        }
                                    }
                                }
                            }
                            else{
                                for (sadIntervalIndex = 0; sadIntervalIndex < NUMBER_OF_SAD_INTERVALS; sadIntervalIndex++){
                                    if(count[sadIntervalIndex]> (10*64*64/blkSize/blkSize) ){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for(qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] = 
                                                (EB_Bit_Number)(((( 7*sadBits[sadIntervalIndex]* TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex-qpIndex+51] +(1<15))>>16)
                                                + 3*(EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] +5)/10);
                                            // intrinsics used in initial RC are assuming signed 16 bits is the maximum
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =  
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1<<15)-1));

                                        }
                                    }
                                    else if(count[sadIntervalIndex]>(5*64*64/blkSize/blkSize)){
                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for(qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] = 
                                                (EB_Bit_Number)(((( 5*sadBits[sadIntervalIndex]* TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex-qpIndex+51] +(1<15))>>16)
                                                + 5*(EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] +5)/10);
                                            // intrinsics used in initial RC are assuming signed 16 bits is the maximum
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =  
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1<<15)-1));

                                        }
                                    }
                                    else if (sadIntervalIndex > ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) && count[sadIntervalIndex]>(1 * 64 * 64 / blkSize / blkSize)){

                                        sadBits[sadIntervalIndex] /= count[sadIntervalIndex];
                                        for(qpIndex = sequenceControlSetPtr->staticConfig.minQpAllowed; qpIndex <= (EB_S32)sequenceControlSetPtr->staticConfig.maxQpAllowed; qpIndex++){
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] = 
                                                (EB_Bit_Number)(((( 1*sadBits[sadIntervalIndex]* TWO_TO_POWER_X_OVER_SIX[(EB_S32)refQpIndex-qpIndex+51] +(1<15))>>16)
                                                + 9*(EB_U32)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] +5)/10);

                                            // intrinsics used in initial RC are assuming signed 16 bits is the maximum
                                            encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex] =  
                                                MIN((EB_U16)encodeContextPtr->rateControlTablesArray[qpIndex].sadBitsArray[pictureControlSetPtr->temporalLayerIndex][sadIntervalIndex], (EB_U16)((1<<15)-1));
                                        }
                                    }
                                }
                            }

                        }
                    }

                    EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->rateTableUpdateMutex);
                }

			}
        }

        // Reset the bitstream
        ResetBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

        // Encode slice header and write it into the bitstream.
        packetizationQp = pictureControlSetPtr->pictureQp;
     
        if(sequenceControlSetPtr->staticConfig.accessUnitDelimiter && (pictureControlSetPtr->pictureNumber > 0)) 
        {
            EncodeAUD(
                pictureControlSetPtr->bitstreamPtr,
                pictureControlSetPtr->sliceType,
                pictureControlSetPtr->temporalId);
        }

        // Parsing the linked list and find the user data SEI msgs and code them
        sequenceControlSetPtr->picTimingSei.picStruct = 0;
        if( sequenceControlSetPtr->staticConfig.bufferingPeriodSEI && 
            pictureControlSetPtr->sliceType == EB_I_PICTURE && 
            sequenceControlSetPtr->staticConfig.videoUsabilityInfo &&
            (sequenceControlSetPtr->videoUsabilityInfoPtr->hrdParametersPtr->nalHrdParametersPresentFlag || sequenceControlSetPtr->videoUsabilityInfoPtr->hrdParametersPtr->vclHrdParametersPresentFlag)) 
        {
            //Calculating the hrdfullness based on the vbv buffer fill status
            if (sequenceControlSetPtr->staticConfig.hrdFlag == 1)
            {
                HrdFullness(sequenceControlSetPtr, pictureControlSetPtr, &sequenceControlSetPtr->bufferingPeriod);
            }
            EncodeBufferingPeriodSEI(
                pictureControlSetPtr->bitstreamPtr,
                &sequenceControlSetPtr->bufferingPeriod,
                sequenceControlSetPtr->videoUsabilityInfoPtr,
                sequenceControlSetPtr->encodeContextPtr);    
        }

        // Flush the Bitstream
        FlushBitstream(
            pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

        // Copy Buffering Period SEI to the Output Bitstream
        CopyRbspBitstreamToPayload(
            pictureControlSetPtr->bitstreamPtr,
            outputStreamPtr->pBuffer,
            (EB_U32*) &(outputStreamPtr->nFilledLen),
            (EB_U32*) &(outputStreamPtr->nAllocLen),
            encodeContextPtr,
			NAL_UNIT_INVALID);
        queueEntryPtr->startSplicing = outputStreamPtr->nFilledLen;
        if (sequenceControlSetPtr->staticConfig.pictureTimingSEI) {
            if (sequenceControlSetPtr->staticConfig.hrdFlag == 1)
            {
                queueEntryPtr->picTimingEntry->decodeOrder = pictureControlSetPtr->ParentPcsPtr->decodeOrder;
                queueEntryPtr->picTimingEntry->picStruct = pictureControlSetPtr->ParentPcsPtr->pictStruct;
                queueEntryPtr->picTimingEntry->temporalId = pictureControlSetPtr->temporalId;
                queueEntryPtr->sliceType = pictureControlSetPtr->sliceType;
                queueEntryPtr->picTimingEntry->poc = pictureControlSetPtr->pictureNumber;
            }

        }
        // Reset the bitstream
        ResetBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

        if(sequenceControlSetPtr->staticConfig.recoveryPointSeiFlag){
            EncodeRecoveryPointSEI(
                pictureControlSetPtr->bitstreamPtr,
                &sequenceControlSetPtr->recoveryPoint);
        }

        if (sequenceControlSetPtr->staticConfig.useNaluFile && pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payloadSize) {
            if (pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payloadType == USER_DATA_REGISTERED_ITU_T_T35) {
                sequenceControlSetPtr->regUserDataSeiPtr.userDataSize = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payloadSize;
                sequenceControlSetPtr->regUserDataSeiPtr.userData = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payload;
                EncodeRegUserDataSEI(
                    pictureControlSetPtr->bitstreamPtr,
                    &sequenceControlSetPtr->regUserDataSeiPtr);
            }
            if (pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payloadType == USER_DATA_UNREGISTERED) {
                sequenceControlSetPtr->unRegUserDataSeiPtr.userDataSize = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payloadSize;
                sequenceControlSetPtr->unRegUserDataSeiPtr.userData = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->userSeiMsg.payload;
                EncodeUnregUserDataSEI(
                    pictureControlSetPtr->bitstreamPtr,
                    &sequenceControlSetPtr->unRegUserDataSeiPtr,
                    encodeContextPtr);
            }
        }


        // Jing: process multiple tiles
        for (tileIdx = 0; tileIdx < tileCnt; tileIdx++) {
            EB_U16 xLcuStart = pictureControlSetPtr->ParentPcsPtr->tileColStartLcu[tileIdx % pictureControlSetPtr->ParentPcsPtr->tileColumnCount];
            EB_U16 yLcuStart = pictureControlSetPtr->ParentPcsPtr->tileRowStartLcu[tileIdx / pictureControlSetPtr->ParentPcsPtr->tileColumnCount];
            EB_U16 lcuIndex = xLcuStart + yLcuStart * pictureControlSetPtr->ParentPcsPtr->pictureWidthInLcu;

            // Encode slice header
            if (tileIdx == 0 || sequenceControlSetPtr->staticConfig.tileSliceMode == 1) {
                EncodeSliceHeader(
                        lcuIndex,
                        packetizationQp,
                        pictureControlSetPtr,
                        (OutputBitstreamUnit_t*) pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

                // Flush the Bitstream
                FlushBitstream(
                        pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);      

                // Copy Slice Header to the Output Bitstream
                CopyRbspBitstreamToPayload(
                        pictureControlSetPtr->bitstreamPtr,
                        outputStreamPtr->pBuffer,
                        (EB_U32*) &(outputStreamPtr->nFilledLen),
                        (EB_U32*) &(outputStreamPtr->nAllocLen),
                        encodeContextPtr,
                        NAL_UNIT_INVALID);

                // Reset the bitstream
                ResetBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);
            }

            // Write the slice data into the bitstream
            bitstream.outputBitstreamPtr = EntropyCoderGetBitstreamPtr(pictureControlSetPtr->entropyCodingInfo[tileIdx]->entropyCoderPtr);

            FlushBitstream(bitstream.outputBitstreamPtr);

            CopyRbspBitstreamToPayload(
                &bitstream,
                outputStreamPtr->pBuffer,
                (EB_U32*) &(outputStreamPtr->nFilledLen),
                (EB_U32*) &(outputStreamPtr->nAllocLen),
			    encodeContextPtr,
			    NAL_UNIT_INVALID);

            bufferRate = encodeContextPtr->vbvMaxrate / (sequenceControlSetPtr->staticConfig.frameRate >> 16);
            queueEntryPtr->fillerBitsSent = 0;
            if ((sequenceControlSetPtr->staticConfig.vbvBufsize && sequenceControlSetPtr->staticConfig.vbvMaxrate) && (sequenceControlSetPtr->staticConfig.vbvMaxrate == sequenceControlSetPtr->staticConfig.targetBitRate))
            {
                pictureControlSetPtr->ParentPcsPtr->totalNumBits = outputStreamPtr->nFilledLen << 3;
                EB_S64 buffer = (EB_S64)(encodeContextPtr->bufferFill);

                buffer -= pictureControlSetPtr->ParentPcsPtr->totalNumBits;
                buffer = MAX(buffer, 0);
                buffer = (EB_S64)(buffer + bufferRate);
                //Block to write filler data to prevent vbv overflow
                if ((EB_U64)buffer > encodeContextPtr->vbvBufsize)
                {
                    filler = (EB_U64)buffer - encodeContextPtr->vbvBufsize;
                    queueEntryPtr->fillerBitsSent = filler;
                }
            }

        }
        
        // Send the number of bytes per frame to RC
        pictureControlSetPtr->ParentPcsPtr->totalNumBits = outputStreamPtr->nFilledLen << 3;    

        queueEntryPtr->actualBits = pictureControlSetPtr->ParentPcsPtr->totalNumBits;
        pictureControlSetPtr->ParentPcsPtr->totalNumBits += queueEntryPtr->fillerBitsSent;
        // Copy Dolby Vision RPU metadata to the output bitstream
        if (sequenceControlSetPtr->staticConfig.dolbyVisionProfile == 81 && pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->dolbyVisionRpu.payloadSize) {
            // Reset the bitstream
            ResetBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

            CodeDolbyVisionRpuMetadata(
                pictureControlSetPtr->bitstreamPtr,
                pictureControlSetPtr
            );

            // Flush the Bitstream
            FlushBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

            // Copy payload to the Output Bitstream
            CopyRbspBitstreamToPayload(
                pictureControlSetPtr->bitstreamPtr,
                outputStreamPtr->pBuffer,
                (EB_U32*) &(outputStreamPtr->nFilledLen),
                (EB_U32*) &(outputStreamPtr->nAllocLen),
                ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr,
                NAL_UNIT_UNSPECIFIED_62);
        }

        // Code EOS NUT
        if (outputStreamPtr->nFlags & EB_BUFFERFLAG_EOS && sequenceControlSetPtr->staticConfig.codeEosNal == 1) 
        {
            // Reset the bitstream
            ResetBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

            CodeEndOfSequenceNalUnit(pictureControlSetPtr->bitstreamPtr);

            // Flush the Bitstream
            FlushBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

            // Copy SPS & PPS to the Output Bitstream
            CopyRbspBitstreamToPayload(
                pictureControlSetPtr->bitstreamPtr,
                outputStreamPtr->pBuffer,
                (EB_U32*) &(outputStreamPtr->nFilledLen),
                (EB_U32*) &(outputStreamPtr->nAllocLen),
                ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr,
				NAL_UNIT_INVALID);
        }
        
        //Store the buffer in the Queue
        queueEntryPtr->outputStreamWrapperPtr = outputStreamWrapperPtr;               

        if (sequenceControlSetPtr->staticConfig.speedControlFlag){
            // update speed control variables
            EbBlockOnMutex(encodeContextPtr->scBufferMutex);
            encodeContextPtr->scFrameOut++;
            EbReleaseMutex(encodeContextPtr->scBufferMutex);
        }

        // Post Rate Control Taks            
        EbPostFullObject(rateControlTasksWrapperPtr);    

        if (sequenceControlSetPtr->staticConfig.rateControlMode) {
            // Post the Full Results Object
            EbPostFullObject(pictureManagerResultsWrapperPtr);
        }
        //Release the Parent PCS then the Child PCS
        EbReleaseObject(entropyCodingResultsPtr->pictureControlSetWrapperPtr);//Child

        // Release the Entropy Coding Result
        EbReleaseObject(entropyCodingResultsWrapperPtr);


        //****************************************************
        // Process the head of the queue
        //****************************************************      
        // Look at head of queue and see if any picture is ready to go
        queueEntryPtr           = encodeContextPtr->packetizationReorderQueue[encodeContextPtr->packetizationReorderQueueHeadIndex];

        while(queueEntryPtr->outputStreamWrapperPtr != EB_NULL) {

            outputStreamWrapperPtr = queueEntryPtr->outputStreamWrapperPtr;
            outputStreamPtr          = (EB_BUFFERHEADERTYPE*) outputStreamWrapperPtr->objectPtr;

            // Calculate frame latency in milliseconds
            double latency = 0.0;
            EB_U64 finishTimeSeconds = 0;
            EB_U64 finishTimeuSeconds = 0;
            EB_U32  bufferWrittenBytesCount = 0;
            EB_U32  startinBytes = 0;
            EB_U32  totalBytes = 0;
            EbFinishTime((uint64_t*)&finishTimeSeconds, (uint64_t*)&finishTimeuSeconds);

            EbComputeOverallElapsedTimeMs(
                queueEntryPtr->startTimeSeconds,
                queueEntryPtr->startTimeuSeconds,
                finishTimeSeconds,
                finishTimeuSeconds,
                &latency);
            //printf("POC %d, PAK out, dts %d, Packetization latency %3.3f\n", outputStreamPtr->pts, outputStreamPtr->dts, latency);
#if LATENCY_PROFILE
            SVT_LOG("POC %lld PAK OUT, latency %3.3f\n", outputStreamPtr->pts, latency);
#endif

            outputStreamPtr->nTickCount = (EB_U32)latency;
            if (sequenceControlSetPtr->staticConfig.pictureTimingSEI) {
                if (sequenceControlSetPtr->staticConfig.hrdFlag == 1)
                {
                    // The aucpbremoval delay specifies how many clock ticks the
                    // access unit associated with the picture timing SEI message has to
                    // wait after removal of the access unit with the most recent
                    // buffering period SEI message
                    const AppVideoUsabilityInfo_t* vui = sequenceControlSetPtr->videoUsabilityInfoPtr;
                    const AppHrdParameters_t* hrd = vui->hrdParametersPtr;
                    sequenceControlSetPtr->picTimingSei.auCpbRemovalDelayMinus1 = (EB_U32)((MIN(MAX(1, (EB_S32)(queueEntryPtr->picTimingEntry->decodeOrder - refDecOrder)), (1 << hrd->auCpbRemovalDelayLengthMinus1))) - 1);
                    sequenceControlSetPtr->picTimingSei.picDpbOutputDelay = (EB_U32)((sequenceControlSetPtr->maxDpbSize - 1) + queueEntryPtr->picTimingEntry->poc - queueEntryPtr->picTimingEntry->decodeOrder);
                }
                // Reset the bitstream
                ResetBitstream(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);

                EncodePictureTimingSEI(
                    queueEntryPtr->bitStreamPtr2,
                    &sequenceControlSetPtr->picTimingSei,
                    sequenceControlSetPtr->videoUsabilityInfoPtr,
                    sequenceControlSetPtr->encodeContextPtr,
                    queueEntryPtr->picTimingEntry->picStruct,
                    queueEntryPtr->picTimingEntry->temporalId);

                // Flush the Bitstream
                FlushBitstream(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)queueEntryPtr->bitStreamPtr2->outputBitstreamPtr;
                bufferWrittenBytesCount = outputBitstreamPtr->writtenBitsCount >> 3;
                startinBytes = queueEntryPtr->startSplicing;
                totalBytes = outputStreamPtr->nFilledLen;
                //Shift the bitstream by size of picture timing SEI
                memmove(outputStreamPtr->pBuffer + startinBytes + bufferWrittenBytesCount, outputStreamPtr->pBuffer + startinBytes, totalBytes - startinBytes);
                // Copy Picture Timing SEI to the Output Bitstream
                CopyRbspBitstreamToPayload(
                    queueEntryPtr->bitStreamPtr2,
                    outputStreamPtr->pBuffer,
                    (EB_U32*) &(queueEntryPtr->startSplicing),
                    (EB_U32*) &(outputStreamPtr->nAllocLen),
                    sequenceControlSetPtr->encodeContextPtr,
					NAL_UNIT_INVALID);
                outputStreamPtr->nFilledLen += bufferWrittenBytesCount;
            }

            if (queueEntryPtr->sliceType == EB_I_PICTURE)
            {
                refDecOrder = queueEntryPtr->pictureNumber;
            }
            /* update VBV plan */
            if (encodeContextPtr->vbvMaxrate && encodeContextPtr->vbvBufsize)
            {   
                EbBlockOnMutex(encodeContextPtr->bufferFillMutex);
                EB_S64 bufferfill_temp = (EB_S64)(encodeContextPtr->bufferFill);
                bufferfill_temp -= queueEntryPtr->actualBits;
                bufferfill_temp = MAX(bufferfill_temp, 0);
                queueEntryPtr->fillerBitsFinal = 0;
                EB_U64 buffer=bufferfill_temp = (EB_S64)(bufferfill_temp + (encodeContextPtr->vbvMaxrate * (1.0 / (sequenceControlSetPtr->frameRate >> RC_PRECISION))));
                //Block to write filler data to prevent cpb overflow
                if ((sequenceControlSetPtr->staticConfig.vbvMaxrate == sequenceControlSetPtr->staticConfig.targetBitRate)&& !(outputStreamPtr->nFlags & EB_BUFFERFLAG_EOS))
                {
                    if (buffer > encodeContextPtr->vbvBufsize)
                    {
                        filler = buffer - encodeContextPtr->vbvBufsize;
                        queueEntryPtr->fillerBitsFinal = filler;
                        fillerBytes = ((EB_U32)(filler >> 3)) - FILLER_DATA_OVERHEAD;
                        // Reset the bitstream
                        ResetBitstream(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);

                        EncodeFillerData(queueEntryPtr->bitStreamPtr2, queueEntryPtr->picTimingEntry->temporalId);

                        // Flush the Bitstream
                        FlushBitstream(
                            queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);

                        // Copy filler bits to the Output Bitstream
                        CopyRbspBitstreamToPayload(
                            queueEntryPtr->bitStreamPtr2,
                            outputStreamPtr->pBuffer,
                            (EB_U32*) &(outputStreamPtr->nFilledLen),
                            (EB_U32*) &(outputStreamPtr->nAllocLen),
                            encodeContextPtr, NAL_UNIT_INVALID);

                        for (EB_U32 i = 0; i < fillerBytes; i++)
                        {
                            ResetBitstream(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                            OutputBitstreamWrite(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr, 0xff, 8);
                            FlushBitstream(
                                queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                            CopyRbspBitstreamToPayload(
                                queueEntryPtr->bitStreamPtr2,
                                outputStreamPtr->pBuffer,
                                (EB_U32*) &(outputStreamPtr->nFilledLen),
                                (EB_U32*) &(outputStreamPtr->nAllocLen),
                                encodeContextPtr, NAL_UNIT_INVALID);
                        }
                        ResetBitstream(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                        // Byte Align the Bitstream: rbsp_trailing_bits
                        OutputBitstreamWrite(
                            queueEntryPtr->bitStreamPtr2->outputBitstreamPtr,
                            1,
                            1);
                        FlushBitstream(
                            queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                        CopyRbspBitstreamToPayload(
                            queueEntryPtr->bitStreamPtr2,
                            outputStreamPtr->pBuffer,
                            (EB_U32*) &(outputStreamPtr->nFilledLen),
                            (EB_U32*) &(outputStreamPtr->nAllocLen),
                            encodeContextPtr, NAL_UNIT_INVALID);
                        ResetBitstream(queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                        OutputBitstreamWriteAlignZero(
                            queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                        FlushBitstream(
                            queueEntryPtr->bitStreamPtr2->outputBitstreamPtr);
                        CopyRbspBitstreamToPayload(
                            queueEntryPtr->bitStreamPtr2,
                            outputStreamPtr->pBuffer,
                            (EB_U32*) &(outputStreamPtr->nFilledLen),
                            (EB_U32*) &(outputStreamPtr->nAllocLen),
                            encodeContextPtr, NAL_UNIT_INVALID);
                    }
                }
                bufferfill_temp -= queueEntryPtr->fillerBitsFinal;
                bufferfill_temp = MIN(bufferfill_temp, encodeContextPtr->vbvBufsize);
                encodeContextPtr->bufferFill = (EB_U64)(bufferfill_temp);
                encodeContextPtr->fillerBitError = (EB_S64)(queueEntryPtr->fillerBitsFinal - queueEntryPtr->fillerBitsSent);
                EbReleaseMutex(encodeContextPtr->bufferFillMutex);
            }
            EbPostFullObject(outputStreamWrapperPtr);
            // Reset the Reorder Queue Entry
            queueEntryPtr->pictureNumber    += PACKETIZATION_REORDER_QUEUE_MAX_DEPTH;            
            queueEntryPtr->outputStreamWrapperPtr = (EbObjectWrapper_t *)EB_NULL;

            // Increment the Reorder Queue head Ptr
            encodeContextPtr->packetizationReorderQueueHeadIndex = 
                (encodeContextPtr->packetizationReorderQueueHeadIndex == PACKETIZATION_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->packetizationReorderQueueHeadIndex + 1;
            
            queueEntryPtr           = encodeContextPtr->packetizationReorderQueue[encodeContextPtr->packetizationReorderQueueHeadIndex];
 

        }
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld PK OUT \n", pictureControlSetPtr->pictureNumber);
#endif     
    }
return EB_NULL;
}
