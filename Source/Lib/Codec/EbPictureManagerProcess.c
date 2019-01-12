/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#include "EbPictureManagerProcess.h"
#include "EbReferenceObject.h"

#include "EbPictureDemuxResults.h"
#include "EbPictureManagerQueue.h"
#include "EbPredictionStructure.h"
#include "EbRateControlTasks.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

/************************************************
 * Defines
 ************************************************/
#define POC_CIRCULAR_ADD(base, offset)             (((base) + (offset)))

/************************************************
 * Configure Picture edges
 ************************************************/
static void ConfigurePictureEdges(
    SequenceControlSet_t *scsPtr,
    PictureControlSet_t  *ppsPtr)
{
    // Tiles Initialisation
    const EB_U16 pictureWidthInLcu   = (scsPtr->lumaWidth + scsPtr->lcuSize - 1) / scsPtr->lcuSize;
    const EB_U16 pictureHeightInLcu  = (scsPtr->lumaHeight + scsPtr->lcuSize - 1) / scsPtr->lcuSize;
    
    unsigned xLcuIndex, yLcuIndex, lcuIndex;
      
    // LCU-loops
	for (yLcuIndex = 0; yLcuIndex <  pictureHeightInLcu; ++yLcuIndex) {
		for (xLcuIndex = 0; xLcuIndex < pictureWidthInLcu; ++xLcuIndex) {
            lcuIndex = (EB_U16)(xLcuIndex + yLcuIndex * pictureWidthInLcu);
            ppsPtr->lcuPtrArray[lcuIndex]->pictureLeftEdgeFlag  = (xLcuIndex == 0) ? EB_TRUE : EB_FALSE;
            ppsPtr->lcuPtrArray[lcuIndex]->pictureTopEdgeFlag   = (yLcuIndex == 0) ? EB_TRUE : EB_FALSE;
			ppsPtr->lcuPtrArray[lcuIndex]->pictureRightEdgeFlag = (xLcuIndex == (unsigned)(pictureWidthInLcu - 1)) ? EB_TRUE : EB_FALSE;
        }
    }

    return;
}

#if TILES
/************************************************
 * Configure Tiles
 ************************************************/
static void ConfigureTiles(
    SequenceControlSet_t *scsPtr,
    PictureControlSet_t  *ppsPtr)
{
    // Tiles Initialisation
    const unsigned pictureWidthInLcu = (scsPtr->lumaWidth + scsPtr->lcuSize - 1) / scsPtr->lcuSize;
    const unsigned pictureHeightInLcu = (scsPtr->lumaHeight + scsPtr->lcuSize - 1) / scsPtr->lcuSize;

    const unsigned tileColumns = scsPtr->tileColumnCount;
    const unsigned tileRows = scsPtr->tileRowCount;

    unsigned lastColumnWidth = pictureWidthInLcu;
    unsigned lastRowHeight = pictureHeightInLcu;

    unsigned rowIndex, columnIndex;
    unsigned xLcuIndex, yLcuIndex, lcuIndex;
    unsigned xLcuStart, yLcuStart;

    ppsPtr->tileColumnCount = tileColumns;
    ppsPtr->tileRowCount = tileRows;
    ppsPtr->tileTotalCount = tileColumns * tileRows;

    if (scsPtr->tileUniformSpacing == 1)
    {
        for (columnIndex = 0; columnIndex < tileColumns - 1; ++columnIndex) {
            ppsPtr->tileColumnArray[columnIndex] = (EB_U16)((columnIndex + 1) * pictureWidthInLcu / tileColumns -
                columnIndex * pictureWidthInLcu / tileColumns);
            lastColumnWidth -= ppsPtr->tileColumnArray[columnIndex];
        }
        ppsPtr->tileColumnArray[columnIndex] = (EB_U16)lastColumnWidth;

        for (rowIndex = 0; rowIndex < tileRows - 1; ++rowIndex) {
            ppsPtr->tileRowArray[rowIndex] = (EB_U16)((rowIndex + 1) * pictureHeightInLcu / tileRows -
                rowIndex * pictureHeightInLcu / tileRows);
            lastRowHeight -= ppsPtr->tileRowArray[rowIndex];
        }
        ppsPtr->tileRowArray[rowIndex] = (EB_U16)lastRowHeight;

    }
    else
    {
        for (columnIndex = 0; columnIndex < tileColumns - 1; ++columnIndex) {
            ppsPtr->tileColumnArray[columnIndex] = (EB_U16)scsPtr->tileColumnWidthArray[columnIndex];
            lastColumnWidth -= ppsPtr->tileColumnArray[columnIndex];
        }
        ppsPtr->tileColumnArray[columnIndex] = (EB_U16)lastColumnWidth;

        for (rowIndex = 0; rowIndex < tileRows - 1; ++rowIndex) {
            ppsPtr->tileRowArray[rowIndex] = (EB_U16)scsPtr->tileRowHeightArray[rowIndex];
            lastRowHeight -= ppsPtr->tileRowArray[rowIndex];
        }
        ppsPtr->tileRowArray[rowIndex] = (EB_U16)lastRowHeight;
    }

    // Tile-loops
    yLcuStart = 0;
    for (rowIndex = 0; rowIndex < tileRows; ++rowIndex) {
        xLcuStart = 0;
        for (columnIndex = 0; columnIndex < tileColumns; ++columnIndex) {

            // LCU-loops
            for (yLcuIndex = yLcuStart; yLcuIndex < yLcuStart + ppsPtr->tileRowArray[rowIndex]; ++yLcuIndex) {
                for (xLcuIndex = xLcuStart; xLcuIndex < xLcuStart + ppsPtr->tileColumnArray[columnIndex]; ++xLcuIndex) {

                    lcuIndex = xLcuIndex + yLcuIndex * pictureWidthInLcu;
                    ppsPtr->lcuPtrArray[lcuIndex]->tileLeftEdgeFlag = (xLcuIndex == xLcuStart) ? EB_TRUE : EB_FALSE;
                    ppsPtr->lcuPtrArray[lcuIndex]->tileTopEdgeFlag = (yLcuIndex == yLcuStart) ? EB_TRUE : EB_FALSE;
                    ppsPtr->lcuPtrArray[lcuIndex]->tileRightEdgeFlag =
                        (xLcuIndex == xLcuStart + ppsPtr->tileColumnArray[columnIndex] - 1) ? EB_TRUE : EB_FALSE;
                    ppsPtr->lcuPtrArray[lcuIndex]->tileOriginX = xLcuStart * scsPtr->lcuSize;
                    ppsPtr->lcuPtrArray[lcuIndex]->tileOriginY = yLcuStart * scsPtr->lcuSize;
                }
            }
            xLcuStart += ppsPtr->tileColumnArray[columnIndex];
        }
        yLcuStart += ppsPtr->tileRowArray[rowIndex];
    }

    return;
}
#endif

/************************************************
 * Picture Manager Context Constructor
 ************************************************/
EB_ERRORTYPE PictureManagerContextCtor(
    PictureManagerContext_t **contextDblPtr,
    EbFifo_t                 *pictureInputFifoPtr,
    EbFifo_t                 *pictureManagerOutputFifoPtr,   
    EbFifo_t                **pictureControlSetFifoPtrArray)
{
    PictureManagerContext_t *contextPtr;
    EB_MALLOC(PictureManagerContext_t*, contextPtr, sizeof(PictureManagerContext_t), EB_N_PTR);

    *contextDblPtr = contextPtr;
    
    contextPtr->pictureInputFifoPtr         = pictureInputFifoPtr;
    contextPtr->pictureManagerOutputFifoPtr   = pictureManagerOutputFifoPtr;   
    contextPtr->pictureControlSetFifoPtrArray = pictureControlSetFifoPtrArray;
    
    return EB_ErrorNone;
}



 
/***************************************************************************************************
 * Picture Manager Kernel
 *                                                                                                        
 * Notes on the Picture Manager:                                                                                                       
 *
 * The Picture Manager Process performs the function of managing both the Input Picture buffers and
 * the Reference Picture buffers and subdividing the Input Picture into Tiles. Both the Input Picture 
 * and Reference Picture buffers particular management depends on the GoP structure already implemented in 
 * the Picture decision. Also note that the Picture Manager sets up the RPS for Entropy Coding as well.  
 *
 * Inputs:
 * Input Picture                                                                                                       
 *   -Input Picture Data                                                                                                                                                                                      
 *                                                                                                        
 *  Reference Picture                                                                                                       
 *   -Reference Picture Data                                                                                     
 *                                                                                                        
 *  Outputs:                                                                                                      
 *   -Picture Control Set with fully available Reference List                                                                                              
 *                                                                                                  
 ***************************************************************************************************/
void* PictureManagerKernel(void *inputPtr)
{
    PictureManagerContext_t         *contextPtr = (PictureManagerContext_t*) inputPtr;
    
    EbObjectWrapper_t               *ChildPictureControlSetWrapperPtr;
    PictureControlSet_t             *ChildPictureControlSetPtr;
    PictureParentControlSet_t       *pictureControlSetPtr;
    SequenceControlSet_t            *sequenceControlSetPtr;
    EncodeContext_t                 *encodeContextPtr;


    EbObjectWrapper_t               *inputPictureDemuxWrapperPtr;
    PictureDemuxResults_t           *inputPictureDemuxPtr;

    EbObjectWrapper_t               *outputWrapperPtr;
    RateControlTasks_t              *rateControlTasksPtr;
    
    // LCU
    EB_U32                          lcuAddr;

    EB_BOOL                         availabilityFlag;
    
    PredictionStructureEntry_t     *predPositionPtr;
    
    // Dynamic GOP
    PredictionStructure_t          *nextPredStructPtr;
    PredictionStructureEntry_t     *nextBaseLayerPredPositionPtr;

    EB_U32                          dependantListPositiveEntries;
    EB_U32                          dependantListRemovedEntries;

    InputQueueEntry_t               *inputEntryPtr;
    EB_U32                          inputQueueIndex;
    EB_U64                          currentInputPoc;
        
    ReferenceQueueEntry_t           *referenceEntryPtr;
    EB_U32                          referenceQueueIndex;
    EB_U64                          refPoc;
        
    EB_U32                          depIdx;
    EB_U64                          depPoc;
    EB_U32                          depListCount;

    PictureParentControlSet_t       *entryPictureControlSetPtr;
    SequenceControlSet_t            *entrySequenceControlSetPtr;
    
    // Initialization
    EB_U8                           pictureWidthInLcu;
    EB_U8                           pictureHeightInLcu;
    
	PictureManagerReorderEntry_t    *queueEntryPtr;
	EB_S32                           queueEntryIndex;

    // Debug
    EB_U32 loopCount = 0;
    
    for(;;) {
    
        // Get Input Full Object
        EbGetFullObject(
            contextPtr->pictureInputFifoPtr,
            &inputPictureDemuxWrapperPtr);

        inputPictureDemuxPtr = (PictureDemuxResults_t*) inputPictureDemuxWrapperPtr->objectPtr;
        
        // *Note - This should be overhauled and/or replaced when we
        //   need hierarchical support. 
        loopCount++;
        
        switch(inputPictureDemuxPtr->pictureType) {
        
        case EB_PIC_INPUT:      

            pictureControlSetPtr            = (PictureParentControlSet_t*)  inputPictureDemuxPtr->pictureControlSetWrapperPtr->objectPtr;
            sequenceControlSetPtr           = (SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
            encodeContextPtr                = sequenceControlSetPtr->encodeContextPtr; 

#if DEADLOCK_DEBUG
            SVT_LOG("POC %lld PM IN \n", pictureControlSetPtr->pictureNumber);
#endif
		   queueEntryIndex = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->pictureManagerReorderQueue[encodeContextPtr->pictureManagerReorderQueueHeadIndex]->pictureNumber);
		   queueEntryIndex += encodeContextPtr->pictureManagerReorderQueueHeadIndex;
		   queueEntryIndex = (queueEntryIndex > PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH - 1) ? queueEntryIndex - PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH : queueEntryIndex;
		   queueEntryPtr = encodeContextPtr->pictureManagerReorderQueue[queueEntryIndex];
		   if (queueEntryPtr->parentPcsWrapperPtr != NULL){
			   CHECK_REPORT_ERROR_NC(
				   encodeContextPtr->appCallbackPtr,
				   EB_ENC_PD_ERROR8);
		   }
		   else{
			   queueEntryPtr->parentPcsWrapperPtr = inputPictureDemuxPtr->pictureControlSetWrapperPtr;
			   queueEntryPtr->pictureNumber = pictureControlSetPtr->pictureNumber;
		   }
		   // Process the head of the Picture Manager Reorder Queue
		   queueEntryPtr = encodeContextPtr->pictureManagerReorderQueue[encodeContextPtr->pictureManagerReorderQueueHeadIndex];

		   while (queueEntryPtr->parentPcsWrapperPtr != EB_NULL) {

			   pictureControlSetPtr = (PictureParentControlSet_t*)queueEntryPtr->parentPcsWrapperPtr->objectPtr;

			   predPositionPtr = pictureControlSetPtr->predStructPtr->predStructEntryPtrArray[pictureControlSetPtr->predStructIndex];

			   // If there was a change in the number of temporal layers, then cleanup the Reference Queue's Dependent Counts
			   if (pictureControlSetPtr->hierarchicalLayersDiff != 0) {

				   referenceQueueIndex = encodeContextPtr->referencePictureQueueHeadIndex;

				   while (referenceQueueIndex != encodeContextPtr->referencePictureQueueTailIndex) {

					   referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];

					   if (referenceEntryPtr->pictureNumber == (pictureControlSetPtr->pictureNumber - 1)) { // Picture where the change happened 

						   // Get the prediction struct entry of the next GOP structure
						   nextPredStructPtr = GetPredictionStructure(
							   encodeContextPtr->predictionStructureGroupPtr,
							   pictureControlSetPtr->predStructure,
							   1,
							   pictureControlSetPtr->hierarchicalLevels);

						   // Get the prediction struct of a picture in temporal layer 0 (from the new GOP structure)
						   nextBaseLayerPredPositionPtr = nextPredStructPtr->predStructEntryPtrArray[nextPredStructPtr->predStructEntryCount - 1];


						   // Remove all positive entries from the dependant lists
						   dependantListPositiveEntries = 0;
						   for (depIdx = 0; depIdx < referenceEntryPtr->list0.listCount; ++depIdx) {
							   if (referenceEntryPtr->list0.list[depIdx] >= 0) {
								   dependantListPositiveEntries++;
							   }
						   }
						   referenceEntryPtr->list0.listCount = referenceEntryPtr->list0.listCount - dependantListPositiveEntries;

						   dependantListPositiveEntries = 0;
						   for (depIdx = 0; depIdx < referenceEntryPtr->list1.listCount; ++depIdx) {
							   if (referenceEntryPtr->list1.list[depIdx] >= 0) {
								   dependantListPositiveEntries++;
							   }
						   }
						   referenceEntryPtr->list1.listCount = referenceEntryPtr->list1.listCount - dependantListPositiveEntries;

						   for (depIdx = 0; depIdx < nextBaseLayerPredPositionPtr->depList0.listCount; ++depIdx) {
							   if (nextBaseLayerPredPositionPtr->depList0.list[depIdx] >= 0) {
								   referenceEntryPtr->list0.list[referenceEntryPtr->list0.listCount++] = nextBaseLayerPredPositionPtr->depList0.list[depIdx];
							   }
						   }


						   for (depIdx = 0; depIdx < nextBaseLayerPredPositionPtr->depList1.listCount; ++depIdx) {
							   if (nextBaseLayerPredPositionPtr->depList1.list[depIdx] >= 0) {
								   referenceEntryPtr->list1.list[referenceEntryPtr->list1.listCount++] = nextBaseLayerPredPositionPtr->depList1.list[depIdx];
							   }
						   }

						   // Update the dependant count update
						   dependantListRemovedEntries = referenceEntryPtr->depList0Count + referenceEntryPtr->depList1Count - referenceEntryPtr->dependentCount;

						   referenceEntryPtr->depList0Count = referenceEntryPtr->list0.listCount;
						   referenceEntryPtr->depList1Count = referenceEntryPtr->list1.listCount;
						   referenceEntryPtr->dependentCount = referenceEntryPtr->depList0Count + referenceEntryPtr->depList1Count - dependantListRemovedEntries;

					   }
					   else {

						   // Modify Dependent List0
						   depListCount = referenceEntryPtr->list0.listCount;
						   for (depIdx = 0; depIdx < depListCount; ++depIdx) {


							   // Adjust the latest currentInputPoc in case we're in a POC rollover scenario 
							   // currentInputPoc += (currentInputPoc < referenceEntryPtr->pocNumber) ? (1 << sequenceControlSetPtr->bitsForPictureOrderCount) : 0;

							   depPoc = POC_CIRCULAR_ADD(
								   referenceEntryPtr->pictureNumber, // can't use a value that gets reset
								   referenceEntryPtr->list0.list[depIdx]/*,
								   sequenceControlSetPtr->bitsForPictureOrderCount*/);

							   // If Dependent POC is greater or equal to the IDR POC
							   if (depPoc >= pictureControlSetPtr->pictureNumber && referenceEntryPtr->list0.list[depIdx]) {

								   referenceEntryPtr->list0.list[depIdx] = 0;

								   // Decrement the Reference's referenceCount
								   --referenceEntryPtr->dependentCount;

								   CHECK_REPORT_ERROR(
									   (referenceEntryPtr->dependentCount != ~0u),
									   encodeContextPtr->appCallbackPtr,
									   EB_ENC_PD_ERROR3);
							   }
						   }

						   // Modify Dependent List1
						   depListCount = referenceEntryPtr->list1.listCount;
						   for (depIdx = 0; depIdx < depListCount; ++depIdx) {

							   // Adjust the latest currentInputPoc in case we're in a POC rollover scenario 
							   // currentInputPoc += (currentInputPoc < referenceEntryPtr->pocNumber) ? (1 << sequenceControlSetPtr->bitsForPictureOrderCount) : 0;

							   depPoc = POC_CIRCULAR_ADD(
								   referenceEntryPtr->pictureNumber,
								   referenceEntryPtr->list1.list[depIdx]/*,
								   sequenceControlSetPtr->bitsForPictureOrderCount*/);

							   // If Dependent POC is greater or equal to the IDR POC
							   if ((depPoc >= pictureControlSetPtr->pictureNumber) && referenceEntryPtr->list1.list[depIdx]) {
								   referenceEntryPtr->list1.list[depIdx] = 0;

								   // Decrement the Reference's referenceCount
								   --referenceEntryPtr->dependentCount;

								   CHECK_REPORT_ERROR(
									   (referenceEntryPtr->dependentCount != ~0u),
									   encodeContextPtr->appCallbackPtr,
									   EB_ENC_PD_ERROR3);
							   }
						   }
					   }

					   // Increment the referenceQueueIndex Iterator
					   referenceQueueIndex = (referenceQueueIndex == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : referenceQueueIndex + 1;
				   }
			   }

			   // If there was an I-frame or Scene Change, then cleanup the Reference Queue's Dependent Counts
			   if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
			   {

				   referenceQueueIndex = encodeContextPtr->referencePictureQueueHeadIndex;
				   while (referenceQueueIndex != encodeContextPtr->referencePictureQueueTailIndex) {

					   referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];

					   // Modify Dependent List0
					   depListCount = referenceEntryPtr->list0.listCount;
					   for (depIdx = 0; depIdx < depListCount; ++depIdx) {

						   currentInputPoc = pictureControlSetPtr->pictureNumber;

						   // Adjust the latest currentInputPoc in case we're in a POC rollover scenario 
						   // currentInputPoc += (currentInputPoc < referenceEntryPtr->pictureNumber) ? (1 << sequenceControlSetPtr->bitsForPictureOrderCount) : 0;

						   depPoc = POC_CIRCULAR_ADD(
							   referenceEntryPtr->pictureNumber, // can't use a value that gets reset
							   referenceEntryPtr->list0.list[depIdx]/*,
							   sequenceControlSetPtr->bitsForPictureOrderCount*/);

						   // If Dependent POC is greater or equal to the IDR POC
						   if (depPoc >= currentInputPoc && referenceEntryPtr->list0.list[depIdx]) {

							   referenceEntryPtr->list0.list[depIdx] = 0;

							   // Decrement the Reference's referenceCount
							   --referenceEntryPtr->dependentCount;
							   CHECK_REPORT_ERROR(
								   (referenceEntryPtr->dependentCount != ~0u),
								   encodeContextPtr->appCallbackPtr,
								   EB_ENC_PM_ERROR3);
						   }
					   }

					   // Modify Dependent List1
					   depListCount = referenceEntryPtr->list1.listCount;
					   for (depIdx = 0; depIdx < depListCount; ++depIdx) {

						   currentInputPoc = pictureControlSetPtr->pictureNumber;

						   // Adjust the latest currentInputPoc in case we're in a POC rollover scenario 
						   // currentInputPoc += (currentInputPoc < referenceEntryPtr->pictureNumber) ? (1 << sequenceControlSetPtr->bitsForPictureOrderCount) : 0;

						   depPoc = POC_CIRCULAR_ADD(
							   referenceEntryPtr->pictureNumber,
							   referenceEntryPtr->list1.list[depIdx]/*,
							   sequenceControlSetPtr->bitsForPictureOrderCount*/);

						   // If Dependent POC is greater or equal to the IDR POC or if we inserted trailing Ps
						   if (((depPoc >= currentInputPoc) || (((pictureControlSetPtr->preAssignmentBufferCount != pictureControlSetPtr->predStructPtr->predStructPeriod) || (pictureControlSetPtr->idrFlag == EB_TRUE)) && (depPoc > (currentInputPoc - pictureControlSetPtr->preAssignmentBufferCount)))) && referenceEntryPtr->list1.list[depIdx]) {

							   referenceEntryPtr->list1.list[depIdx] = 0;

							   // Decrement the Reference's referenceCount
							   --referenceEntryPtr->dependentCount;
							   CHECK_REPORT_ERROR(
								   (referenceEntryPtr->dependentCount != ~0u),
								   encodeContextPtr->appCallbackPtr,
								   EB_ENC_PM_ERROR3);
						   }

					   }

					   // Increment the referenceQueueIndex Iterator
					   referenceQueueIndex = (referenceQueueIndex == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : referenceQueueIndex + 1;
				   }

			   }
			   else if (pictureControlSetPtr->idrFlag == EB_TRUE) {

				   // Set Reference Entry pointer
				   referenceEntryPtr = (ReferenceQueueEntry_t*)EB_NULL;
			   }

			   // Check if the EnhancedPictureQueue is full.  
			   // *Note - Having the number of Enhanced Pictures less than the InputQueueSize should ensure this never gets hit

			   CHECK_REPORT_ERROR(
				   (((encodeContextPtr->inputPictureQueueHeadIndex != encodeContextPtr->inputPictureQueueTailIndex) || (encodeContextPtr->inputPictureQueue[encodeContextPtr->inputPictureQueueHeadIndex]->inputObjectPtr == EB_NULL))),
				   encodeContextPtr->appCallbackPtr,
				   EB_ENC_PM_ERROR4);

			   // Place Picture in input queue
			   inputEntryPtr = encodeContextPtr->inputPictureQueue[encodeContextPtr->inputPictureQueueTailIndex];
			   inputEntryPtr->inputObjectPtr = queueEntryPtr->parentPcsWrapperPtr;
			   inputEntryPtr->referenceEntryIndex = encodeContextPtr->referencePictureQueueTailIndex;
			   encodeContextPtr->inputPictureQueueTailIndex =
				   (encodeContextPtr->inputPictureQueueTailIndex == INPUT_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->inputPictureQueueTailIndex + 1;

			   // Copy the reference lists into the inputEntry and 
			   // set the Reference Counts Based on Temporal Layer and how many frames are active
			   pictureControlSetPtr->refList0Count = (pictureControlSetPtr->sliceType == EB_I_PICTURE) ? 0 : (EB_U8)predPositionPtr->refList0.referenceListCount;
			   pictureControlSetPtr->refList1Count = (pictureControlSetPtr->sliceType == EB_I_PICTURE) ? 0 : (EB_U8)predPositionPtr->refList1.referenceListCount;
			   inputEntryPtr->list0Ptr = &predPositionPtr->refList0;
			   inputEntryPtr->list1Ptr = &predPositionPtr->refList1;

			   // Check if the ReferencePictureQueue is full. 
			   CHECK_REPORT_ERROR(
				   (((encodeContextPtr->referencePictureQueueHeadIndex != encodeContextPtr->referencePictureQueueTailIndex) || (encodeContextPtr->referencePictureQueue[encodeContextPtr->referencePictureQueueHeadIndex]->referenceObjectPtr == EB_NULL))),
				   encodeContextPtr->appCallbackPtr,
				   EB_ENC_PM_ERROR5);

			   // Create Reference Queue Entry even if picture will not be referenced
			   referenceEntryPtr = encodeContextPtr->referencePictureQueue[encodeContextPtr->referencePictureQueueTailIndex];
			   referenceEntryPtr->pictureNumber = pictureControlSetPtr->pictureNumber;
			   referenceEntryPtr->referenceObjectPtr = (EbObjectWrapper_t*)EB_NULL;
			   referenceEntryPtr->releaseEnable = EB_TRUE;
			   referenceEntryPtr->referenceAvailable = EB_FALSE;
			   referenceEntryPtr->isUsedAsReferenceFlag = pictureControlSetPtr->isUsedAsReferenceFlag;
			   encodeContextPtr->referencePictureQueueTailIndex =
				   (encodeContextPtr->referencePictureQueueTailIndex == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->referencePictureQueueTailIndex + 1;

               // Copy the Dependent Lists
			   // *Note - we are removing any leading picture dependencies for now
			   referenceEntryPtr->list0.listCount = 0;
			   for (depIdx = 0; depIdx < predPositionPtr->depList0.listCount; ++depIdx) {
				   if (predPositionPtr->depList0.list[depIdx] >= 0) {
					   referenceEntryPtr->list0.list[referenceEntryPtr->list0.listCount++] = predPositionPtr->depList0.list[depIdx];
				   }
			   }

			   referenceEntryPtr->list1.listCount = predPositionPtr->depList1.listCount;
			   for (depIdx = 0; depIdx < predPositionPtr->depList1.listCount; ++depIdx) {
				   referenceEntryPtr->list1.list[depIdx] = predPositionPtr->depList1.list[depIdx];
			   }

			   referenceEntryPtr->depList0Count = referenceEntryPtr->list0.listCount;
			   referenceEntryPtr->depList1Count = referenceEntryPtr->list1.listCount;
			   referenceEntryPtr->dependentCount = referenceEntryPtr->depList0Count + referenceEntryPtr->depList1Count;

			   CHECK_REPORT_ERROR(
				   (pictureControlSetPtr->predStructPtr->predStructPeriod < MAX_ELAPSED_IDR_COUNT),
				   encodeContextPtr->appCallbackPtr,
				   EB_ENC_PM_ERROR6);

			   // Release the Reference Buffer once we know it is not a reference
			   if (pictureControlSetPtr->isUsedAsReferenceFlag == EB_FALSE){
				   // Release the nominal liveCount value
				   EbReleaseObject(pictureControlSetPtr->referencePictureWrapperPtr);
				   pictureControlSetPtr->referencePictureWrapperPtr = (EbObjectWrapper_t*)EB_NULL;
			   }
#if DEADLOCK_DEBUG
               SVT_LOG("POC %lld PM OUT \n", pictureControlSetPtr->pictureNumber);
#endif
			   // Release the Picture Manager Reorder Queue
			   queueEntryPtr->parentPcsWrapperPtr = (EbObjectWrapper_t*)EB_NULL;
			   queueEntryPtr->pictureNumber += PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH;

			   // Increment the Picture Manager Reorder Queue
			   encodeContextPtr->pictureManagerReorderQueueHeadIndex = (encodeContextPtr->pictureManagerReorderQueueHeadIndex == PICTURE_MANAGER_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->pictureManagerReorderQueueHeadIndex + 1;

			   // Get the next entry from the Picture Manager Reorder Queue (Entry N+1) 
			   queueEntryPtr = encodeContextPtr->pictureManagerReorderQueue[encodeContextPtr->pictureManagerReorderQueueHeadIndex];

		   }
		   break;
            
        case EB_PIC_REFERENCE:
        
            sequenceControlSetPtr   = (SequenceControlSet_t*) inputPictureDemuxPtr->sequenceControlSetWrapperPtr->objectPtr;
            encodeContextPtr        = sequenceControlSetPtr->encodeContextPtr;
            
            // Check if Reference Queue is full
            CHECK_REPORT_ERROR(
                (encodeContextPtr->referencePictureQueueHeadIndex != encodeContextPtr->referencePictureQueueTailIndex),
                encodeContextPtr->appCallbackPtr, 
                EB_ENC_PM_ERROR7);
           
            referenceQueueIndex = encodeContextPtr->referencePictureQueueHeadIndex;
            
            // Find the Reference in the Reference Queue 
            do {
                
                referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];                
                
                if(referenceEntryPtr->pictureNumber == inputPictureDemuxPtr->pictureNumber) {
                
                    // Assign the reference object if there is a match
                    referenceEntryPtr->referenceObjectPtr = inputPictureDemuxPtr->referencePictureWrapperPtr;
                    
                    // Set the reference availability
                    referenceEntryPtr->referenceAvailable = EB_TRUE;
                }
                
                // Increment the referenceQueueIndex Iterator
                referenceQueueIndex = (referenceQueueIndex == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : referenceQueueIndex + 1;
            
            } while ((referenceQueueIndex != encodeContextPtr->referencePictureQueueTailIndex) && (referenceEntryPtr->pictureNumber != inputPictureDemuxPtr->pictureNumber));
            

            CHECK_REPORT_ERROR(
                (referenceEntryPtr->pictureNumber == inputPictureDemuxPtr->pictureNumber),
                encodeContextPtr->appCallbackPtr, 
                EB_ENC_PM_ERROR8);   
                  
            //keep the relase of SCS here because we still need the encodeContext strucutre here
            // Release the Reference's SequenceControlSet    
            EbReleaseObject(inputPictureDemuxPtr->sequenceControlSetWrapperPtr);
                
            break;
            
        default:
           
            sequenceControlSetPtr   = (SequenceControlSet_t*) inputPictureDemuxPtr->sequenceControlSetWrapperPtr->objectPtr;
            encodeContextPtr        = sequenceControlSetPtr->encodeContextPtr;

            CHECK_REPORT_ERROR_NC(
                encodeContextPtr->appCallbackPtr, 
                EB_ENC_PM_ERROR9);

            pictureControlSetPtr    = (PictureParentControlSet_t*) EB_NULL;
            encodeContextPtr        = (EncodeContext_t*) EB_NULL;
            
            break;
        } 
        
        // ***********************************
        //  Common Code
        // *************************************
        
        // Walk the input queue and start all ready pictures.  Mark entry as null after started.  Increment the head as you go.
        if (encodeContextPtr != (EncodeContext_t*)EB_NULL) {
        inputQueueIndex = encodeContextPtr->inputPictureQueueHeadIndex;
        while (inputQueueIndex != encodeContextPtr->inputPictureQueueTailIndex) {
            
            inputEntryPtr = encodeContextPtr->inputPictureQueue[inputQueueIndex];
            
            if(inputEntryPtr->inputObjectPtr != EB_NULL) {
                
                entryPictureControlSetPtr   = (PictureParentControlSet_t*)   inputEntryPtr->inputObjectPtr->objectPtr;
                entrySequenceControlSetPtr  = (SequenceControlSet_t*)  entryPictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
                
                availabilityFlag = EB_TRUE;
                
                // Check RefList0 Availability
				 if (entryPictureControlSetPtr->refList0Count){
                    referenceQueueIndex = (EB_U32) CIRCULAR_ADD(
                        ((EB_S32) inputEntryPtr->referenceEntryIndex) -     // Base
                        inputEntryPtr->list0Ptr->referenceList,     // Offset
                        REFERENCE_QUEUE_MAX_DEPTH);                         // Max
                        
                    referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];
                    
                    CHECK_REPORT_ERROR(
                        (referenceEntryPtr),
                        encodeContextPtr->appCallbackPtr, 
                        EB_ENC_PM_ERROR10);
                    
                    refPoc = POC_CIRCULAR_ADD(
                        entryPictureControlSetPtr->pictureNumber,
                        -inputEntryPtr->list0Ptr->referenceList/*,
                        entrySequenceControlSetPtr->bitsForPictureOrderCount*/);
                        
                    // Increment the currentInputPoc is the case of POC rollover    
                    currentInputPoc = encodeContextPtr->currentInputPoc;
                                                
                    availabilityFlag =
                        (availabilityFlag == EB_FALSE)          ? EB_FALSE  :   // Don't update if already False 
                        (refPoc > currentInputPoc)              ? EB_FALSE  :   // The Reference has not been received as an Input Picture yet, then its availability is false
                        (referenceEntryPtr->referenceAvailable) ? EB_TRUE   :   // The Reference has been completed
                                                                  EB_FALSE;     // The Reference has not been completed
                }
                
                // Check RefList1 Availability
                if(entryPictureControlSetPtr->sliceType == EB_B_PICTURE) {
					if (entryPictureControlSetPtr->refList1Count){
                        // If Reference is valid (non-zero), update the availability
                        if(inputEntryPtr->list1Ptr->referenceList != (EB_S32) INVALID_POC) {
                    
                            referenceQueueIndex = (EB_U32) CIRCULAR_ADD(
                                ((EB_S32) inputEntryPtr->referenceEntryIndex) -     // Base
                                inputEntryPtr->list1Ptr->referenceList,     // Offset
                                REFERENCE_QUEUE_MAX_DEPTH);                         // Max
                            
                            referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];

                            CHECK_REPORT_ERROR(
                                (referenceEntryPtr),
                                encodeContextPtr->appCallbackPtr, 
                                EB_ENC_PM_ERROR10);
                        
                            refPoc = POC_CIRCULAR_ADD(
                                entryPictureControlSetPtr->pictureNumber,
                                -inputEntryPtr->list1Ptr->referenceList/*,
                                entrySequenceControlSetPtr->bitsForPictureOrderCount*/);
                            
                            // Increment the currentInputPoc is the case of POC rollover        
                            currentInputPoc = encodeContextPtr->currentInputPoc;
                            //currentInputPoc += ((currentInputPoc < refPoc && inputEntryPtr->list1Ptr->referenceList[refIdx] > 0)) ?
                            //    (1 << entrySequenceControlSetPtr->bitsForPictureOrderCount) :
                            //    0;
                        
                            availabilityFlag =
                                (availabilityFlag == EB_FALSE)          ? EB_FALSE  :   // Don't update if already False 
                                (refPoc > currentInputPoc)              ? EB_FALSE  :   // The Reference has not been received as an Input Picture yet, then its availability is false
                                (referenceEntryPtr->referenceAvailable) ? EB_TRUE   :   // The Reference has been completed
                                                                          EB_FALSE;     // The Reference has not been completed
                        }
                    }
                }
                
                if(availabilityFlag == EB_TRUE) { 

                    // Get New  Empty Child PCS from PCS Pool
                    EbGetEmptyObject(
                        contextPtr->pictureControlSetFifoPtrArray[0],
                        &ChildPictureControlSetWrapperPtr);
                    
                    // Child PCS is released by Packetization
                    EbObjectIncLiveCount(
                        ChildPictureControlSetWrapperPtr,
                        1);

                    ChildPictureControlSetPtr     = (PictureControlSet_t*) ChildPictureControlSetWrapperPtr->objectPtr;

                    //1.Link The Child PCS to its Parent
                    ChildPictureControlSetPtr->PictureParentControlSetWrapperPtr = inputEntryPtr->inputObjectPtr;
                    ChildPictureControlSetPtr->ParentPcsPtr                      = entryPictureControlSetPtr;
                    
                    
                    //2. Have some common information between  ChildPCS and ParentPCS.
                    ChildPictureControlSetPtr->sequenceControlSetWrapperPtr             = entryPictureControlSetPtr->sequenceControlSetWrapperPtr;                  
                    ChildPictureControlSetPtr->pictureQp                                = entryPictureControlSetPtr->pictureQp;
                    ChildPictureControlSetPtr->pictureNumber                            = entryPictureControlSetPtr->pictureNumber; 
                    ChildPictureControlSetPtr->sliceType                                = entryPictureControlSetPtr->sliceType ;
                    ChildPictureControlSetPtr->temporalLayerIndex                       = entryPictureControlSetPtr->temporalLayerIndex ;

                    ChildPictureControlSetPtr->ParentPcsPtr->totalNumBits               = 0;
                    ChildPictureControlSetPtr->ParentPcsPtr->pictureQp                  = entryPictureControlSetPtr->pictureQp;
                    ChildPictureControlSetPtr->ParentPcsPtr->sadMe                      = 0;
                    ChildPictureControlSetPtr->ParentPcsPtr->quantizedCoeffNumBits      = 0;      
                    ChildPictureControlSetPtr->encMode                                  = entryPictureControlSetPtr->encMode; 



                    // Update temporal ID
                    if(entrySequenceControlSetPtr->staticConfig.enableTemporalId) { 
                        ChildPictureControlSetPtr->temporalId =  (entryPictureControlSetPtr->nalUnit == NAL_UNIT_CODED_SLICE_IDR_W_RADL) ? 0 :
                                                                 (entryPictureControlSetPtr->nalUnit == NAL_UNIT_CODED_SLICE_CRA) ? 0 : 
                                                                                                                                     entryPictureControlSetPtr->temporalLayerIndex;
                    }

                    //3.make all  init for ChildPCS
                    pictureWidthInLcu  = (EB_U8)((entrySequenceControlSetPtr->lumaWidth + entrySequenceControlSetPtr->lcuSize - 1) / entrySequenceControlSetPtr->lcuSize);
                    pictureHeightInLcu = (EB_U8)((entrySequenceControlSetPtr->lumaHeight + entrySequenceControlSetPtr->lcuSize - 1) / entrySequenceControlSetPtr->lcuSize); 

#if TILES
                    ConfigureTiles(
                        entrySequenceControlSetPtr,
                        ChildPictureControlSetPtr);
#endif
                    // EncDec Segments 
                    EncDecSegmentsInit(
                        ChildPictureControlSetPtr->encDecSegmentCtrl,
                        entrySequenceControlSetPtr->encDecSegmentColCountArray[entryPictureControlSetPtr->temporalLayerIndex],
                        entrySequenceControlSetPtr->encDecSegmentRowCountArray[entryPictureControlSetPtr->temporalLayerIndex],
                        pictureWidthInLcu,
                        pictureHeightInLcu);

                    // Entropy Coding Rows
                    {
                        unsigned rowIndex;

                        ChildPictureControlSetPtr->entropyCodingCurrentRow = 0;
                        ChildPictureControlSetPtr->entropyCodingCurrentAvailableRow = 0;
                        ChildPictureControlSetPtr->entropyCodingRowCount = pictureHeightInLcu;
                        ChildPictureControlSetPtr->entropyCodingInProgress = EB_FALSE;

                        for(rowIndex=0; rowIndex < MAX_LCU_ROWS; ++rowIndex) {
                            ChildPictureControlSetPtr->entropyCodingRowArray[rowIndex] = EB_FALSE;
                        }
                    }
                    // Picture edges
					ConfigurePictureEdges(entrySequenceControlSetPtr, ChildPictureControlSetPtr);
                    
                    // Reset the qp array for DLF
                    EB_MEMSET(ChildPictureControlSetPtr->qpArray, 0, sizeof(EB_U8)*ChildPictureControlSetPtr->qpArraySize);
                    // Set all the elements in the vertical/horizontal edge bS arraies to 0 for DLF
                    for(lcuAddr = 0; lcuAddr < ChildPictureControlSetPtr->lcuTotalCount; ++lcuAddr){
                        EB_MEMSET(ChildPictureControlSetPtr->verticalEdgeBSArray[lcuAddr], 0, VERTICAL_EDGE_BS_ARRAY_SIZE*sizeof(EB_U8));
                        EB_MEMSET(ChildPictureControlSetPtr->horizontalEdgeBSArray[lcuAddr], 0, HORIZONTAL_EDGE_BS_ARRAY_SIZE*sizeof(EB_U8));
                    }

                    // Error resilience related
                    ChildPictureControlSetPtr->colocatedPuRefList  = REF_LIST_0;     // to be modified

                    ChildPictureControlSetPtr->isLowDelay          = (EB_BOOL)(
                        ChildPictureControlSetPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[ChildPictureControlSetPtr->ParentPcsPtr->predStructIndex]->positiveRefPicsTotalCount == 0);

                    // Rate Control 

                    ChildPictureControlSetPtr->useDeltaQp =  (EB_U8)(entrySequenceControlSetPtr->staticConfig.improveSharpness || entrySequenceControlSetPtr->staticConfig.bitRateReduction);

                    // Check resolution
                    if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_1080p_RANGE)
                        ChildPictureControlSetPtr->difCuDeltaQpDepth = 2;
                    else
                        ChildPictureControlSetPtr->difCuDeltaQpDepth = 3;
                                      
                    // Reset the Reference Lists
                    EB_MEMSET(ChildPictureControlSetPtr->refPicPtrArray, 0, 2 * sizeof(EbObjectWrapper_t*));

                    EB_MEMSET(ChildPictureControlSetPtr->refPicQpArray, 0,  2 * sizeof(EB_U8));

                    EB_MEMSET(ChildPictureControlSetPtr->refSliceTypeArray, 0,  2 * sizeof(EB_PICTURE));
                   
                    // Configure List0
                    if ((entryPictureControlSetPtr->sliceType == EB_P_PICTURE) || (entryPictureControlSetPtr->sliceType == EB_B_PICTURE)) {
                    
						if (entryPictureControlSetPtr->refList0Count){
                            referenceQueueIndex = (EB_U32) CIRCULAR_ADD(
                                ((EB_S32) inputEntryPtr->referenceEntryIndex) - inputEntryPtr->list0Ptr->referenceList,      
                                REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max
                                
                            referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];

                            // Set the Reference Object
                            ChildPictureControlSetPtr->refPicPtrArray[REF_LIST_0] = referenceEntryPtr->referenceObjectPtr;

                            ChildPictureControlSetPtr->refPicQpArray[REF_LIST_0]  = ((EbReferenceObject_t*) referenceEntryPtr->referenceObjectPtr->objectPtr)->qp;
                            ChildPictureControlSetPtr->refSliceTypeArray[REF_LIST_0] = ((EbReferenceObject_t*) referenceEntryPtr->referenceObjectPtr->objectPtr)->sliceType;

                            // Increment the Reference's liveCount by the number of tiles in the input picture
                            EbObjectIncLiveCount(
                                referenceEntryPtr->referenceObjectPtr,
                                1);
                            
                            // Decrement the Reference's dependentCount Count
                            --referenceEntryPtr->dependentCount;

                            CHECK_REPORT_ERROR(
                                (referenceEntryPtr->dependentCount != ~0u),
                                encodeContextPtr->appCallbackPtr, 
                                EB_ENC_PM_ERROR1);  
                            
                        }
                    }
                    
                    // Configure List1
                    if (entryPictureControlSetPtr->sliceType == EB_B_PICTURE) {
                        
						if (entryPictureControlSetPtr->refList1Count){
                            referenceQueueIndex = (EB_U32) CIRCULAR_ADD(
                                ((EB_S32) inputEntryPtr->referenceEntryIndex) - inputEntryPtr->list1Ptr->referenceList,      
                                REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max
                                
                            referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];

                            // Set the Reference Object
                            ChildPictureControlSetPtr->refPicPtrArray[REF_LIST_1] = referenceEntryPtr->referenceObjectPtr;

                            ChildPictureControlSetPtr->refPicQpArray[REF_LIST_1]  = ((EbReferenceObject_t*) referenceEntryPtr->referenceObjectPtr->objectPtr)->qp;
                            ChildPictureControlSetPtr->refSliceTypeArray[REF_LIST_1] = ((EbReferenceObject_t*) referenceEntryPtr->referenceObjectPtr->objectPtr)->sliceType;
                            


                            // Increment the Reference's liveCount by the number of tiles in the input picture
                            EbObjectIncLiveCount(
                                referenceEntryPtr->referenceObjectPtr,
                                1);
                            
                            // Decrement the Reference's dependentCount Count
                            --referenceEntryPtr->dependentCount;

                            CHECK_REPORT_ERROR(
                                (referenceEntryPtr->dependentCount != ~0u),
                                encodeContextPtr->appCallbackPtr, 
                                EB_ENC_PM_ERROR1);  
                        }
                    }

                    // Adjust the Slice-type if the Lists are Empty, but don't reset the Prediction Structure
                    entryPictureControlSetPtr->sliceType =
                        (entryPictureControlSetPtr->refList1Count > 0) ? EB_B_PICTURE :
                        (entryPictureControlSetPtr->refList0Count > 0) ? EB_P_PICTURE : 
                                                                         EB_I_PICTURE;


                    // Increment the sequenceControlSet Wrapper's live count by 1 for only the pictures which are used as reference
                    if(ChildPictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) { 
                        EbObjectIncLiveCount(
                            ChildPictureControlSetPtr->ParentPcsPtr->sequenceControlSetWrapperPtr,
                            1);
                    }

                    
                    // Get Empty Results Object
                    EbGetEmptyObject(
                        contextPtr->pictureManagerOutputFifoPtr,
                        &outputWrapperPtr);
                        
                    rateControlTasksPtr                              = (RateControlTasks_t*) outputWrapperPtr->objectPtr;
                    rateControlTasksPtr->pictureControlSetWrapperPtr = ChildPictureControlSetWrapperPtr;
                    rateControlTasksPtr->taskType                    = RC_PICTURE_MANAGER_RESULT;
                                        
                    // Post the Full Results Object
                    EbPostFullObject(outputWrapperPtr);

                    // Remove the Input Entry from the Input Queue
                    inputEntryPtr->inputObjectPtr = (EbObjectWrapper_t*) EB_NULL;
                  
                }
            }
            
            // Increment the HeadIndex if the head is null
            encodeContextPtr->inputPictureQueueHeadIndex = 
                (encodeContextPtr->inputPictureQueue[encodeContextPtr->inputPictureQueueHeadIndex]->inputObjectPtr) ? encodeContextPtr->inputPictureQueueHeadIndex :
                (encodeContextPtr->inputPictureQueueHeadIndex == INPUT_QUEUE_MAX_DEPTH - 1)                         ? 0
                                                                                                                    : encodeContextPtr->inputPictureQueueHeadIndex + 1;
            
            // Increment the inputQueueIndex Iterator
            inputQueueIndex = (inputQueueIndex == INPUT_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;
            
        }
        
        // Walk the reference queue and remove entries that have been completely referenced.
        referenceQueueIndex = encodeContextPtr->referencePictureQueueHeadIndex;
        while(referenceQueueIndex != encodeContextPtr->referencePictureQueueTailIndex) {
            
            referenceEntryPtr = encodeContextPtr->referencePictureQueue[referenceQueueIndex];
            
            // Remove the entry & release the reference if there are no remaining references
            if((referenceEntryPtr->dependentCount == 0) &&
               (referenceEntryPtr->referenceAvailable)  &&
               (referenceEntryPtr->releaseEnable)       &&
               (referenceEntryPtr->referenceObjectPtr)) 
            {   
                // Release the nominal liveCount value
                EbReleaseObject(referenceEntryPtr->referenceObjectPtr);
            
                referenceEntryPtr->referenceObjectPtr = (EbObjectWrapper_t*) EB_NULL;
                referenceEntryPtr->referenceAvailable = EB_FALSE;
                referenceEntryPtr->isUsedAsReferenceFlag = EB_FALSE;
            }
                
            // Increment the HeadIndex if the head is empty          
            encodeContextPtr->referencePictureQueueHeadIndex = 
                (encodeContextPtr->referencePictureQueue[encodeContextPtr->referencePictureQueueHeadIndex]->releaseEnable         == EB_FALSE)? encodeContextPtr->referencePictureQueueHeadIndex:
                (encodeContextPtr->referencePictureQueue[encodeContextPtr->referencePictureQueueHeadIndex]->referenceAvailable    == EB_FALSE &&
                 encodeContextPtr->referencePictureQueue[encodeContextPtr->referencePictureQueueHeadIndex]->isUsedAsReferenceFlag == EB_TRUE) ? encodeContextPtr->referencePictureQueueHeadIndex:
                (encodeContextPtr->referencePictureQueue[encodeContextPtr->referencePictureQueueHeadIndex]->dependentCount > 0)               ? encodeContextPtr->referencePictureQueueHeadIndex:
                (encodeContextPtr->referencePictureQueueHeadIndex == REFERENCE_QUEUE_MAX_DEPTH - 1)                                           ? 0
                                                                                                                                              : encodeContextPtr->referencePictureQueueHeadIndex + 1;
            // Increment the referenceQueueIndex Iterator
            referenceQueueIndex = (referenceQueueIndex == REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : referenceQueueIndex + 1;
        }
        }
        
        // Release the Input Picture Demux Results
        EbReleaseObject(inputPictureDemuxWrapperPtr);  
        
    }
return EB_NULL;    
}
