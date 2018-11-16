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
#include "EbErrorCodes.h"
#include "EbTime.h"

static EB_BOOL IsPassthroughData(EbLinkedListNode* dataNode)
{   return dataNode->passthrough;   }

// Extracts passthrough data from a linked list. The extracted data nodes are removed from the original linked list and
// returned as a linked list. Does not gaurantee the original order of the nodes.
static EbLinkedListNode* ExtractPassthroughData(EbLinkedListNode** llPtrPtr)
{
    EbLinkedListNode* llRestPtr = NULL;
    EbLinkedListNode* llPassPtr = splitEbLinkedList(*llPtrPtr, &llRestPtr, IsPassthroughData);
    *llPtrPtr = llRestPtr;
    return llPassPtr;
}


EB_ERRORTYPE PacketizationContextCtor(
    PacketizationContext_t **contextDblPtr,
    EbFifo_t                *entropyCodingInputFifoPtr,
    EbFifo_t                *rateControlTasksOutputFifoPtr)
{
    PacketizationContext_t *contextPtr;
    EB_MALLOC(PacketizationContext_t*, contextPtr, sizeof(PacketizationContext_t), EB_N_PTR);
    *contextDblPtr = contextPtr;
        
    contextPtr->entropyCodingInputFifoPtr      = entropyCodingInputFifoPtr;
    contextPtr->rateControlTasksOutputFifoPtr  = rateControlTasksOutputFifoPtr;

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
       
    EB_SLICE                        sliceType;
    
    EbLinkedListNode               *appDataLLHeadTempPtr;

    for(;;) {
    
        // Get EntropyCoding Results
        EbGetFullObject(
            contextPtr->entropyCodingInputFifoPtr,
            &entropyCodingResultsWrapperPtr);
        entropyCodingResultsPtr = (EntropyCodingResults_t*) entropyCodingResultsWrapperPtr->objectPtr;
        pictureControlSetPtr    = (PictureControlSet_t*)    entropyCodingResultsPtr->pictureControlSetWrapperPtr->objectPtr;
        sequenceControlSetPtr   = (SequenceControlSet_t*)   pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
        encodeContextPtr        = (EncodeContext_t*)        sequenceControlSetPtr->encodeContextPtr;
#if DEADLOCK_DEBUG
        printf("POC %lld PK IN \n", pictureControlSetPtr->pictureNumber);
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
        outputStreamPtr->nFlags |= (encodeContextPtr->terminatingSequenceFlagReceived == EB_TRUE && pictureControlSetPtr->ParentPcsPtr->decodeOrder == encodeContextPtr->terminatingPictureNumber) ? EB_BUFFERFLAG_EOS : 0;
        outputStreamPtr->nFilledLen = 0;
        outputStreamPtr->nOffset = 0;
        outputStreamPtr->pts = pictureControlSetPtr->ParentPcsPtr->ebInputPtr->pts;
        outputStreamPtr->dts = pictureControlSetPtr->ParentPcsPtr->decodeOrder - (1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 1;
        outputStreamPtr->sliceType = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag ? 
                                     pictureControlSetPtr->ParentPcsPtr->idrFlag ? IDR_SLICE :
                                     pictureControlSetPtr->sliceType : NON_REF_SLICE;
        // Get Empty Rate Control Input Tasks
        EbGetEmptyObject(
            contextPtr->rateControlTasksOutputFifoPtr,
            &rateControlTasksWrapperPtr);
        rateControlTasksPtr                                 = (RateControlTasks_t*) rateControlTasksWrapperPtr->objectPtr; 
        rateControlTasksPtr->pictureControlSetWrapperPtr    = pictureControlSetPtr->PictureParentControlSetWrapperPtr;
        rateControlTasksPtr->taskType                       = RC_PACKETIZATION_FEEDBACK_RESULT;
        
        sliceType = pictureControlSetPtr->sliceType;
        
        if(pictureControlSetPtr->pictureNumber == 0 && sequenceControlSetPtr->staticConfig.codeVpsSpsPps == 1) {

            // Reset the bitstream before writing to it
            ResetBitstream(
                pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

            if(sequenceControlSetPtr->staticConfig.accessUnitDelimiter) {

                EncodeAUD(
                    pictureControlSetPtr->bitstreamPtr,
                    sliceType,
                    pictureControlSetPtr->temporalId);
            }

            // Compute Profile Tier and Level Information
            ComputeProfileTierLevelInfo(
                sequenceControlSetPtr);
                            
			ComputeMaxDpbBuffer(
                sequenceControlSetPtr);

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

            // Flush the Bitstream
            FlushBitstream(
                pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);
            
            // Copy SPS & PPS to the Output Bitstream
            CopyRbspBitstreamToPayload(
                pictureControlSetPtr->bitstreamPtr,
                outputStreamPtr->pBuffer,
                (EB_U32*) &(outputStreamPtr->nFilledLen),
                (EB_U32*) &(outputStreamPtr->nAllocLen),
				encodeContextPtr);
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
                        if(pictureControlSetPtr->sliceType == I_SLICE){

                       
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
                        if(pictureControlSetPtr->sliceType == I_SLICE){   
                         //   printf("Update After: %d\n", pictureControlSetPtr->pictureNumber);
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
                sliceType,
                pictureControlSetPtr->temporalId);
        }

        // Parsing the linked list and find the user data SEI msgs and code them
        sequenceControlSetPtr->picTimingSei.picStruct = 0;
        appDataLLHeadTempPtr =  pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr;
        while (appDataLLHeadTempPtr != EB_NULL)
        {
            switch(appDataLLHeadTempPtr->type){

                case EB_TYPE_UNREG_USER_DATA_SEI:
                    if(sequenceControlSetPtr->staticConfig.unregisteredUserDataSeiFlag){
                        EncodeUnregUserDataSEI(
                            pictureControlSetPtr->bitstreamPtr,
                            (UnregistedUserData_t*)(appDataLLHeadTempPtr->data),
                            sequenceControlSetPtr->encodeContextPtr);  
                    }
                    break;

                case EB_TYPE_REG_USER_DATA_SEI:
                    if(sequenceControlSetPtr->staticConfig.registeredUserDataSeiFlag){
                        EncodeRegUserDataSEI(
                            pictureControlSetPtr->bitstreamPtr,
                            (RegistedUserData_t*)(appDataLLHeadTempPtr->data));  
                    }
                    break;

                case EB_TYPE_PIC_STRUCT:
                    sequenceControlSetPtr->picTimingSei.picStruct = *(EB_U32*)(appDataLLHeadTempPtr->data);
                    break;

                default:
                    break;
            }

            appDataLLHeadTempPtr = appDataLLHeadTempPtr->next;

        }

        if( sequenceControlSetPtr->staticConfig.bufferingPeriodSEI && 
            pictureControlSetPtr->sliceType == I_SLICE && 
            sequenceControlSetPtr->staticConfig.videoUsabilityInfo &&
            (sequenceControlSetPtr->videoUsabilityInfoPtr->hrdParametersPtr->nalHrdParametersPresentFlag || sequenceControlSetPtr->videoUsabilityInfoPtr->hrdParametersPtr->vclHrdParametersPresentFlag)) 
        {                           
            EncodeBufferingPeriodSEI(
                pictureControlSetPtr->bitstreamPtr,
                &sequenceControlSetPtr->bufferingPeriod,
                sequenceControlSetPtr->videoUsabilityInfoPtr,
                sequenceControlSetPtr->encodeContextPtr);    
        }

        if(sequenceControlSetPtr->staticConfig.pictureTimingSEI) {

            EncodePictureTimingSEI(
                pictureControlSetPtr->bitstreamPtr,
                &sequenceControlSetPtr->picTimingSei,
                sequenceControlSetPtr->videoUsabilityInfoPtr,
                sequenceControlSetPtr->encodeContextPtr,
                pictureControlSetPtr->ParentPcsPtr->pictStruct);

        }

        if(sequenceControlSetPtr->staticConfig.recoveryPointSeiFlag){
            EncodeRecoveryPointSEI(
                pictureControlSetPtr->bitstreamPtr,
                &sequenceControlSetPtr->recoveryPoint);
        }
    
        EncodeSliceHeader(
            0,
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
			encodeContextPtr);

        // Reset the bitstream
        ResetBitstream(pictureControlSetPtr->bitstreamPtr->outputBitstreamPtr);

        // Write the slice data into the bitstream
        bitstream.outputBitstreamPtr = EntropyCoderGetBitstreamPtr(pictureControlSetPtr->entropyCoderPtr);

        FlushBitstream(bitstream.outputBitstreamPtr);

        CopyRbspBitstreamToPayload(
            &bitstream,
            outputStreamPtr->pBuffer,
            (EB_U32*) &(outputStreamPtr->nFilledLen),
            (EB_U32*) &(outputStreamPtr->nAllocLen),
			encodeContextPtr);
        
        // Send the number of bytes per frame to RC
        pictureControlSetPtr->ParentPcsPtr->totalNumBits = outputStreamPtr->nFilledLen << 3;    

        // Code EOS NUT
        if (outputStreamPtr->nFlags & EB_BUFFERFLAG_EOS) 
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
                ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr);
        }
        
        //Store the buffer in the Queue
        queueEntryPtr->outputStreamWrapperPtr = outputStreamWrapperPtr;               

        // Note: last chance here to add more output meta data for an encoded picture -->
            
        // collect output meta data 
        queueEntryPtr->outMetaData = concatEbLinkedList(ExtractPassthroughData(&(pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr)),
                                                        pictureControlSetPtr->ParentPcsPtr->appOutDataLLHeadPtr);
        pictureControlSetPtr->ParentPcsPtr->appOutDataLLHeadPtr = (EbLinkedListNode *)EB_NULL;

        // Calling callback functions to release the memory allocated for data linked list in the application
        while (pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr != EB_NULL){
        appDataLLHeadTempPtr = pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr->next;
        if (pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr->releaseCbFncPtr != EB_NULL){
            pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr->releaseCbFncPtr(pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr);
            }

            pictureControlSetPtr->ParentPcsPtr->dataLLHeadPtr = appDataLLHeadTempPtr;
        }

        if (sequenceControlSetPtr->staticConfig.speedControlFlag){
            // update speed control variables
            EbBlockOnMutex(encodeContextPtr->scBufferMutex);
            encodeContextPtr->scFrameOut++;
            EbReleaseMutex(encodeContextPtr->scBufferMutex);
        }

        // Post Rate Control Taks            
        EbPostFullObject(rateControlTasksWrapperPtr);    

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
            FinishTime((unsigned long long*)&finishTimeSeconds, (unsigned long long*)&finishTimeuSeconds);

            ComputeOverallElapsedTimeMs(
                queueEntryPtr->startTimeSeconds,
                queueEntryPtr->startTimeuSeconds,
                finishTimeSeconds,
                finishTimeuSeconds,
                &latency);

            outputStreamPtr->nTickCount = (EB_U32)latency;
            outputStreamPtr->pAppPrivate = queueEntryPtr->outMetaData;
			EbPostFullObject(outputStreamWrapperPtr);
            queueEntryPtr->outMetaData = (EbLinkedListNode *)EB_NULL;

            // Reset the Reorder Queue Entry
            queueEntryPtr->pictureNumber    += PACKETIZATION_REORDER_QUEUE_MAX_DEPTH;            
            queueEntryPtr->outputStreamWrapperPtr = (EbObjectWrapper_t *)EB_NULL;

            // Increment the Reorder Queue head Ptr
            encodeContextPtr->packetizationReorderQueueHeadIndex = 
                (encodeContextPtr->packetizationReorderQueueHeadIndex == PACKETIZATION_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->packetizationReorderQueueHeadIndex + 1;
            
            queueEntryPtr           = encodeContextPtr->packetizationReorderQueue[encodeContextPtr->packetizationReorderQueueHeadIndex];
 

        }
#if DEADLOCK_DEBUG
        printf("POC %lld PK OUT \n", pictureControlSetPtr->pictureNumber);
#endif     
    }
return EB_NULL;
}
