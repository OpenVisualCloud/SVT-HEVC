/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbEntropyCodingProcess.h"
#include "EbTransforms.h"
#include "EbEncDecResults.h"
#include "EbEntropyCodingResults.h"
#include "EbRateControlTasks.h"

/******************************************************
 * Enc Dec Context Constructor
 ******************************************************/
EB_ERRORTYPE EntropyCodingContextCtor(
    EntropyCodingContext_t **contextDblPtr,
    EbFifo_t                *encDecInputFifoPtr,
    EbFifo_t                *packetizationOutputFifoPtr,
    EbFifo_t                *rateControlOutputFifoPtr,
    EB_BOOL                  is16bit)
{
    EntropyCodingContext_t *contextPtr;
    EB_MALLOC(EntropyCodingContext_t*, contextPtr, sizeof(EntropyCodingContext_t), EB_N_PTR);
    *contextDblPtr = contextPtr;

    contextPtr->is16bit = is16bit;

    // Input/Output System Resource Manager FIFOs
    contextPtr->encDecInputFifoPtr          = encDecInputFifoPtr;
    contextPtr->entropyCodingOutputFifoPtr  = packetizationOutputFifoPtr;
    contextPtr->rateControlOutputFifoPtr    = rateControlOutputFifoPtr;

    return EB_ErrorNone;
}

/***********************************************
 * Entropy Coding Reset Neighbor Arrays
 ***********************************************/
static void EntropyCodingResetNeighborArrays(PictureControlSet_t *pictureControlSetPtr)
{
    NeighborArrayUnitReset(pictureControlSetPtr->modeTypeNeighborArray);
    NeighborArrayUnitReset(pictureControlSetPtr->leafDepthNeighborArray);
    NeighborArrayUnitReset(pictureControlSetPtr->skipFlagNeighborArray);
    NeighborArrayUnitReset(pictureControlSetPtr->intraLumaModeNeighborArray);

    return;
}

/**************************************************
 * Reset Entropy Coding Picture
 **************************************************/
static void ResetEntropyCodingPicture(
	EntropyCodingContext_t  *contextPtr,
	PictureControlSet_t     *pictureControlSetPtr,
	SequenceControlSet_t    *sequenceControlSetPtr)
{
    ResetBitstream(EntropyCoderGetBitstreamPtr(pictureControlSetPtr->entropyCoderPtr));

	EB_U32                       entropyCodingQp;

	contextPtr->is16bit = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

	// SAO
	pictureControlSetPtr->saoFlag[0] = EB_TRUE;
	pictureControlSetPtr->saoFlag[1] = EB_TRUE;

	// QP
	contextPtr->qp = pictureControlSetPtr->pictureQp;
	// Asuming cb and cr offset to be the same for chroma QP in both slice and pps for lambda computation

	EB_U8	qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
	contextPtr->chromaQp = MapChromaQp(qpScaled);


	if (pictureControlSetPtr->useDeltaQp) {
		entropyCodingQp = pictureControlSetPtr->pictureQp;
	}
	else {
		entropyCodingQp = pictureControlSetPtr->pictureQp;
	}


	// Reset CABAC Contexts
	// Reset QP Assignement
	pictureControlSetPtr->prevCodedQp = pictureControlSetPtr->pictureQp;
	pictureControlSetPtr->prevQuantGroupCodedQp = pictureControlSetPtr->pictureQp;

	ResetEntropyCoder(
		sequenceControlSetPtr->encodeContextPtr,
		pictureControlSetPtr->entropyCoderPtr,
		entropyCodingQp,
		pictureControlSetPtr->sliceType);

	EntropyCodingResetNeighborArrays(pictureControlSetPtr);

    return;
}


/******************************************************
 * EncDec Configure LCU
 ******************************************************/
static void EntropyCodingConfigureLcu(
    EntropyCodingContext_t  *contextPtr,
    LargestCodingUnit_t     *lcuPtr,
    PictureControlSet_t     *pictureControlSetPtr)
{
    contextPtr->qp = pictureControlSetPtr->pictureQp;

	// Asuming cb and cr offset to be the same for chroma QP in both slice and pps for lambda computation

	EB_U8	qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
	contextPtr->chromaQp = MapChromaQp(qpScaled);

    lcuPtr->qp = contextPtr->qp;

    return;
}

/******************************************************
 * Entropy Coding Lcu
 ******************************************************/
static void EntropyCodingLcu(
    LargestCodingUnit_t               *lcuPtr,
    PictureControlSet_t               *pictureControlSetPtr,
    SequenceControlSet_t              *sequenceControlSetPtr,
    EB_U32                             lcuOriginX,
    EB_U32                             lcuOriginY,
    EB_BOOL                            terminateSliceFlag,
    EB_U32                             pictureOriginX,
    EB_U32                             pictureOriginY)
{

	EbPictureBufferDesc_t *coeffPicturePtr = lcuPtr->quantizedCoeff;

    //rate Control
    EB_U32                       writtenBitsBeforeQuantizedCoeff;
    EB_U32                       writtenBitsAfterQuantizedCoeff;

    //store the number of written bits before coding quantized coeffs (flush is not called yet): 
    // The total number of bits is 
    // number of written bits
    // + 32  - bits remaining in interval Low Value
    // + number of buffered byte * 8
    // This should be only for coeffs not any flag
    writtenBitsBeforeQuantizedCoeff =  ((OutputBitstreamUnit_t*)EntropyCoderGetBitstreamPtr(pictureControlSetPtr->entropyCoderPtr))->writtenBitsCount +
                                        32 - ((CabacEncodeContext_t*) pictureControlSetPtr->entropyCoderPtr->cabacEncodeContextPtr)->bacEncContext.bitsRemainingNum +
                                        (((CabacEncodeContext_t*) pictureControlSetPtr->entropyCoderPtr->cabacEncodeContextPtr)->bacEncContext.tempBufferedBytesNum <<3);

    if(sequenceControlSetPtr->staticConfig.enableSaoFlag && (pictureControlSetPtr->saoFlag[0] || pictureControlSetPtr->saoFlag[1])) {

        // Code SAO parameters
        EncodeLcuSaoParameters(
            lcuPtr,
            pictureControlSetPtr->entropyCoderPtr,
            pictureControlSetPtr->saoFlag[0],
            pictureControlSetPtr->saoFlag[1],
            (EB_U8)sequenceControlSetPtr->staticConfig.encoderBitDepth);
    }

    EncodeLcu(
        lcuPtr,
        lcuOriginX,
        lcuOriginY,
        pictureControlSetPtr,
        sequenceControlSetPtr->lcuSize,
        pictureControlSetPtr->entropyCoderPtr,
        coeffPicturePtr,
        pictureControlSetPtr->modeTypeNeighborArray,
        pictureControlSetPtr->leafDepthNeighborArray,
        pictureControlSetPtr->intraLumaModeNeighborArray,
        pictureControlSetPtr->skipFlagNeighborArray,
		pictureOriginX,
		pictureOriginY);

    //store the number of written bits after coding quantized coeffs (flush is not called yet): 
    // The total number of bits is 
    // number of written bits
    // + 32  - bits remaining in interval Low Value
    // + number of buffered byte * 8
    writtenBitsAfterQuantizedCoeff =   ((OutputBitstreamUnit_t*)EntropyCoderGetBitstreamPtr(pictureControlSetPtr->entropyCoderPtr))->writtenBitsCount +
                                        32 - ((CabacEncodeContext_t*) pictureControlSetPtr->entropyCoderPtr->cabacEncodeContextPtr)->bacEncContext.bitsRemainingNum +
                                        (((CabacEncodeContext_t*) pictureControlSetPtr->entropyCoderPtr->cabacEncodeContextPtr)->bacEncContext.tempBufferedBytesNum <<3);

    lcuPtr->totalBits = writtenBitsAfterQuantizedCoeff - writtenBitsBeforeQuantizedCoeff;

    pictureControlSetPtr->ParentPcsPtr->quantizedCoeffNumBits += lcuPtr->quantizedCoeffsBits;

    /*********************************************************
    *Note - At the end of each LCU, HEVC adds 1 bit to indicate that
    if the current LCU is the end of a slice, where 0x1 means it is the
    end of slice and 0x0 means not. Currently we assume that each slice
    contains integer number of tiles, thus the trailing 1 bit for the
    non-ending LCU of a tile is always 0x0 and the trailing 1 bit for the
    tile ending LCU can be 0x0 or 0x1. In the entropy coding process, we
    can not decide the slice boundary so we can not write the trailing 1 bit
    for the tile ending LCU.
    *********************************************************/
    EncodeTerminateLcu(
        pictureControlSetPtr->entropyCoderPtr,
        terminateSliceFlag);

    return;
}

/******************************************************
 * Update Entropy Coding Rows 
 *
 * This function is responsible for synchronizing the
 *   processing of Entropy Coding LCU-rows and starts 
 *   processing of LCU-rows as soon as their inputs are 
 *   available and the previous LCU-row has completed.  
 *   At any given time, only one segment row per picture
 *   is being processed.
 *
 * The function has two parts:
 *
 * (1) Update the available row index which tracks
 *   which LCU Row-inputs are available.
 *
 * (2) Increment the lcu-row counter as the segment-rows
 *   are completed.
 *
 * Since there is the potentential for thread collusion,
 *   a MUTEX a used to protect the sensitive data and
 *   the execution flow is separated into two paths
 *
 * (A) Initial update.
 *  -Update the Completion Mask [see (1) above]
 *  -If the picture is not currently being processed,
 *     check to see if the next segment-row is available
 *     and start processing.
 * (B) Continued processing
 *  -Upon the completion of a segment-row, check
 *     to see if the next segment-row's inputs have
 *     become available and begin processing if so.
 *
 * On last important point is that the thread-safe
 *   code section is kept minimally short. The MUTEX
 *   should NOT be locked for the entire processing
 *   of the segment-row (B) as this would block other
 *   threads from performing an update (A).
 ******************************************************/
static EB_BOOL UpdateEntropyCodingRows(
    PictureControlSet_t *pictureControlSetPtr,
    EB_U32              *rowIndex,
    EB_U32               rowCount,
    EB_BOOL             *initialProcessCall)
{
    EB_BOOL processNextRow = EB_FALSE;

    // Note, any writes & reads to status variables (e.g. inProgress) in MD-CTRL must be thread-safe
    EbBlockOnMutex(pictureControlSetPtr->entropyCodingMutex);

    // Update availability mask
    if (*initialProcessCall == EB_TRUE) {
        unsigned i;

        for(i=*rowIndex; i < *rowIndex + rowCount; ++i) {
            pictureControlSetPtr->entropyCodingRowArray[i] = EB_TRUE;
        }
        
        while(pictureControlSetPtr->entropyCodingRowArray[pictureControlSetPtr->entropyCodingCurrentAvailableRow] == EB_TRUE &&
              pictureControlSetPtr->entropyCodingCurrentAvailableRow < pictureControlSetPtr->entropyCodingRowCount)
        {
            ++pictureControlSetPtr->entropyCodingCurrentAvailableRow;
        }
    }

    // Release inProgress token
    if(*initialProcessCall == EB_FALSE && pictureControlSetPtr->entropyCodingInProgress == EB_TRUE) {
        pictureControlSetPtr->entropyCodingInProgress = EB_FALSE;
    }

    // Test if the picture is not already complete AND not currently being worked on by another ENCDEC process
    if(pictureControlSetPtr->entropyCodingCurrentRow < pictureControlSetPtr->entropyCodingRowCount && 
       pictureControlSetPtr->entropyCodingRowArray[pictureControlSetPtr->entropyCodingCurrentRow] == EB_TRUE &&
       pictureControlSetPtr->entropyCodingInProgress == EB_FALSE)
    {
        // Test if the next LCU-row is ready to go
        if(pictureControlSetPtr->entropyCodingCurrentRow <= pictureControlSetPtr->entropyCodingCurrentAvailableRow)
        {
            pictureControlSetPtr->entropyCodingInProgress = EB_TRUE;
            *rowIndex = pictureControlSetPtr->entropyCodingCurrentRow++;
            processNextRow = EB_TRUE;
        }
    }

    *initialProcessCall = EB_FALSE;

    EbReleaseMutex(pictureControlSetPtr->entropyCodingMutex);

    return processNextRow;
}
#if TILES

static EB_BOOL WaitForAllEntropyCodingRows(
    PictureControlSet_t *pictureControlSetPtr,
    EB_U32              *rowIndex,
    EB_U32               rowCount,
    EB_BOOL             *initialProcessCall)
{
    EB_BOOL processNextRow = EB_TRUE;
    unsigned i;

    // Note, any writes & reads to status variables (e.g. inProgress) in MD-CTRL must be thread-safe
    EbBlockOnMutex(pictureControlSetPtr->entropyCodingMutex);

    // Update availability mask
    if (*initialProcessCall == EB_TRUE) {

        for (i = *rowIndex; i < *rowIndex + rowCount; ++i) {
            pictureControlSetPtr->entropyCodingRowArray[i] = EB_TRUE;
        }

        //pictureControlSetPtr->entropyCodingCurrentAvailableRow = MAX(((EB_S32) (*rowIndex)),pictureControlSetPtr->entropyCodingCurrentAvailableRow);
        while (pictureControlSetPtr->entropyCodingRowArray[pictureControlSetPtr->entropyCodingCurrentAvailableRow] == EB_TRUE &&
            pictureControlSetPtr->entropyCodingCurrentAvailableRow < pictureControlSetPtr->entropyCodingRowCount)
        {
            ++pictureControlSetPtr->entropyCodingCurrentAvailableRow;
        }
    }

    // Release inProgress token
    if (*initialProcessCall == EB_FALSE && pictureControlSetPtr->entropyCodingInProgress == EB_TRUE) {
        pictureControlSetPtr->entropyCodingInProgress = EB_FALSE;
    }

    // Test if all rows are available
    for (i = 0; i < (unsigned)pictureControlSetPtr->entropyCodingRowCount; ++i) {
        processNextRow = (pictureControlSetPtr->entropyCodingRowArray[i] == EB_FALSE) ? EB_FALSE : processNextRow;
    }

    // Test if the picture is already being processed
    processNextRow = (pictureControlSetPtr->entropyCodingInProgress) ? EB_FALSE : processNextRow;

    *initialProcessCall = EB_FALSE;

    EbReleaseMutex(pictureControlSetPtr->entropyCodingMutex);

    return processNextRow;
}
/**************************************************
 * Reset Entropy Coding Tiles
 **************************************************/
static void ResetEntropyCodingTile(
    EntropyCodingContext_t  *contextPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr,
    EB_U32                   lcuRowIndex,
    EB_BOOL                  tilesActiveFlag)
{
    EB_U32                       entropyCodingQp;

    contextPtr->is16bit = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

    // SAO
    pictureControlSetPtr->saoFlag[0] = EB_TRUE;
    pictureControlSetPtr->saoFlag[1] = EB_TRUE;

    // QP
    contextPtr->qp = pictureControlSetPtr->pictureQp;
    contextPtr->chromaQp = MapChromaQp(contextPtr->qp);

    if (pictureControlSetPtr->useDeltaQp) {
        entropyCodingQp = pictureControlSetPtr->pictureQp;
    }
    else {
        entropyCodingQp = pictureControlSetPtr->pictureQp;
    }



    // Reset CABAC Contexts
    if (lcuRowIndex == 0 || tilesActiveFlag)
    {
        // Reset QP Assignement
        pictureControlSetPtr->prevCodedQp = pictureControlSetPtr->pictureQp;
        pictureControlSetPtr->prevQuantGroupCodedQp = pictureControlSetPtr->pictureQp;

        ResetEntropyCoder(
            sequenceControlSetPtr->encodeContextPtr,
            pictureControlSetPtr->entropyCoderPtr,
            entropyCodingQp,
            pictureControlSetPtr->sliceType);

        EntropyCodingResetNeighborArrays(pictureControlSetPtr);
    }

    return;
}
#endif
/******************************************************
 * Entropy Coding Kernel
 ******************************************************/
void* EntropyCodingKernel(void *inputPtr)
{
    // Context & SCS & PCS
    EntropyCodingContext_t                  *contextPtr = (EntropyCodingContext_t*) inputPtr;
    PictureControlSet_t                     *pictureControlSetPtr;
    SequenceControlSet_t                    *sequenceControlSetPtr;

    // Input
    EbObjectWrapper_t                       *encDecResultsWrapperPtr;
    EncDecResults_t                         *encDecResultsPtr;

    // Output
    EbObjectWrapper_t                       *entropyCodingResultsWrapperPtr;
    EntropyCodingResults_t                  *entropyCodingResultsPtr;

    // LCU Loop variables
    LargestCodingUnit_t                     *lcuPtr;
    EB_U16                                   lcuIndex;
    EB_U8                                    lcuSize;
    EB_U8                                    lcuSizeLog2;
    EB_U32                                   xLcuIndex;
    EB_U32                                   yLcuIndex;
    EB_U32                                   lcuOriginX;
    EB_U32                                   lcuOriginY;
    EB_BOOL                                  lastLcuFlag;
    EB_U32                                   pictureWidthInLcu;
    // Variables
    EB_BOOL                                  initialProcessCall;

    for(;;) {

        // Get Mode Decision Results
        EbGetFullObject(
            contextPtr->encDecInputFifoPtr,
            &encDecResultsWrapperPtr);
        encDecResultsPtr       = (EncDecResults_t*) encDecResultsWrapperPtr->objectPtr;
        pictureControlSetPtr   = (PictureControlSet_t*) encDecResultsPtr->pictureControlSetWrapperPtr->objectPtr;
        sequenceControlSetPtr  = (SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
        lastLcuFlag            = EB_FALSE;
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld EC IN \n", pictureControlSetPtr->pictureNumber);
#endif
        // LCU Constants
        lcuSize     = sequenceControlSetPtr->lcuSize;
        lcuSizeLog2 = (EB_U8)Log2f(lcuSize);
        contextPtr->lcuSize = lcuSize;
        pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + lcuSize - 1) >> lcuSizeLog2;
#if TILES
        if (sequenceControlSetPtr->tileRowCount * sequenceControlSetPtr->tileColumnCount == 1)
#endif     
        {
            initialProcessCall = EB_TRUE;
            yLcuIndex = encDecResultsPtr->completedLcuRowIndexStart;   
            
            // LCU-loops
            while(UpdateEntropyCodingRows(pictureControlSetPtr, &yLcuIndex, encDecResultsPtr->completedLcuRowCount, &initialProcessCall) == EB_TRUE) 
            {
                EB_U32 rowTotalBits = 0;

                if(yLcuIndex == 0) {
					ResetEntropyCodingPicture(
						contextPtr, 
						pictureControlSetPtr,
						sequenceControlSetPtr);
					pictureControlSetPtr->entropyCodingPicDone = EB_FALSE;
                }

                for(xLcuIndex = 0; xLcuIndex < pictureWidthInLcu; ++xLcuIndex) 
                {

                    
                    lcuIndex = (EB_U16)(xLcuIndex + yLcuIndex * pictureWidthInLcu);
                    lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];

                    lcuOriginX = xLcuIndex << lcuSizeLog2;
                    lcuOriginY = yLcuIndex << lcuSizeLog2;
                    lastLcuFlag = (lcuIndex == pictureControlSetPtr->lcuTotalCount - 1) ? EB_TRUE : EB_FALSE;
            
                    // Configure the LCU
                    EntropyCodingConfigureLcu(
                        contextPtr,
                        lcuPtr,
                        pictureControlSetPtr);
            
                    // Entropy Coding
                    EntropyCodingLcu(
                        lcuPtr,
                        pictureControlSetPtr,
                        sequenceControlSetPtr,
                        lcuOriginX,
                        lcuOriginY,
                        lastLcuFlag,
                        0,
                        0);

                    rowTotalBits += lcuPtr->totalBits;
                }

                // At the end of each LCU-row, send the updated bit-count to Entropy Coding
                {
                    EbObjectWrapper_t *rateControlTaskWrapperPtr;
                    RateControlTasks_t *rateControlTaskPtr;

                    // Get Empty EncDec Results
                    EbGetEmptyObject(
                        contextPtr->rateControlOutputFifoPtr,
                        &rateControlTaskWrapperPtr);
                    rateControlTaskPtr = (RateControlTasks_t*) rateControlTaskWrapperPtr->objectPtr;
                    rateControlTaskPtr->taskType = RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT;
                    rateControlTaskPtr->pictureNumber = pictureControlSetPtr->pictureNumber;
                    rateControlTaskPtr->rowNumber = yLcuIndex;
                    rateControlTaskPtr->bitCount = rowTotalBits;

                    rateControlTaskPtr->pictureControlSetWrapperPtr = 0;
                    rateControlTaskPtr->segmentIndex = ~0u;
                    
                    // Post EncDec Results
                    EbPostFullObject(rateControlTaskWrapperPtr);
                }

				EbBlockOnMutex(pictureControlSetPtr->entropyCodingMutex);
				if (pictureControlSetPtr->entropyCodingPicDone == EB_FALSE) {

					// If the picture is complete, terminate the slice
					if (pictureControlSetPtr->entropyCodingCurrentRow == pictureControlSetPtr->entropyCodingRowCount)
					{
						EB_U32 refIdx;

						pictureControlSetPtr->entropyCodingPicDone = EB_TRUE;

						EncodeSliceFinish(pictureControlSetPtr->entropyCoderPtr);

						// Release the List 0 Reference Pictures
						for (refIdx = 0; refIdx < pictureControlSetPtr->ParentPcsPtr->refList0Count; ++refIdx) {
							if (pictureControlSetPtr->refPicPtrArray[0] != EB_NULL) {

								EbReleaseObject(pictureControlSetPtr->refPicPtrArray[0]);
                            }
						}

						// Release the List 1 Reference Pictures
						for (refIdx = 0; refIdx < pictureControlSetPtr->ParentPcsPtr->refList1Count; ++refIdx) {
							if (pictureControlSetPtr->refPicPtrArray[1] != EB_NULL) {

								EbReleaseObject(pictureControlSetPtr->refPicPtrArray[1]);
							}
						}

						// Get Empty Entropy Coding Results
						EbGetEmptyObject(
							contextPtr->entropyCodingOutputFifoPtr,
							&entropyCodingResultsWrapperPtr);
						entropyCodingResultsPtr = (EntropyCodingResults_t*)entropyCodingResultsWrapperPtr->objectPtr;
						entropyCodingResultsPtr->pictureControlSetWrapperPtr = encDecResultsPtr->pictureControlSetWrapperPtr;

						// Post EntropyCoding Results
						EbPostFullObject(entropyCodingResultsWrapperPtr);

					} // End if(PictureCompleteFlag)
				}
				EbReleaseMutex(pictureControlSetPtr->entropyCodingMutex);


			}
        }
#if TILES
        else
        {
        unsigned xLcuStart, yLcuStart, rowIndex, columnIndex;

        initialProcessCall = EB_TRUE;

        yLcuIndex = encDecResultsPtr->completedLcuRowIndexStart;

        // Update EC-rows, exit if picture is "in-progress" 
        if (WaitForAllEntropyCodingRows(pictureControlSetPtr, &yLcuIndex, encDecResultsPtr->completedLcuRowCount, &initialProcessCall) == EB_TRUE)
        {


            ResetEntropyCodingPicture(
                contextPtr,
                pictureControlSetPtr,
                sequenceControlSetPtr);


            // Tile-loops
            yLcuStart = 0;
            for (rowIndex = 0; rowIndex < sequenceControlSetPtr->tileRowCount; ++rowIndex)
            {
                xLcuStart = 0;
                for (columnIndex = 0; columnIndex < sequenceControlSetPtr->tileColumnCount; ++columnIndex)
                {
                    ResetEntropyCodingTile(
                        contextPtr,
                        pictureControlSetPtr,
                        sequenceControlSetPtr,
                        yLcuIndex,
                        EB_TRUE);

                    if (sequenceControlSetPtr->tileSliceMode == 1 && (xLcuStart !=0 || yLcuStart != 0)) {
                        CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)pictureControlSetPtr->entropyCoderPtr->cabacEncodeContextPtr;
                        EncodeSliceHeader(
                            xLcuStart + yLcuStart * pictureWidthInLcu,
                            pictureControlSetPtr->pictureQp,
                            pictureControlSetPtr,
                            (OutputBitstreamUnit_t*) &(cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf));
                    }

                    // LCU-loops
                    for (yLcuIndex = yLcuStart; yLcuIndex < yLcuStart + sequenceControlSetPtr->tileRowArray[rowIndex]; ++yLcuIndex)
                    {
                        for (xLcuIndex = xLcuStart; xLcuIndex < xLcuStart + sequenceControlSetPtr->tileColumnArray[columnIndex]; ++xLcuIndex)
                        {
                            lcuIndex = xLcuIndex + yLcuIndex * pictureWidthInLcu;
                            lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];
                            lcuOriginX = xLcuIndex << lcuSizeLog2;
                            lcuOriginY = yLcuIndex << lcuSizeLog2;
                            if (sequenceControlSetPtr->tileSliceMode == 0) {
                                lastLcuFlag = (lcuIndex == pictureControlSetPtr->lcuTotalCount - 1) ? EB_TRUE : EB_FALSE;
                            } else
                            {
                                lastLcuFlag = (xLcuIndex == (xLcuStart + sequenceControlSetPtr->tileColumnArray[columnIndex] -1) && yLcuIndex == (yLcuStart + sequenceControlSetPtr->tileRowArray[rowIndex] -1)) ? EB_TRUE : EB_FALSE;
                            }

                            // Configure the LCU
                            EntropyCodingConfigureLcu(
                                contextPtr,
                                lcuPtr,
                                pictureControlSetPtr);

                            // Entropy Coding
                            EntropyCodingLcu(
                                lcuPtr,
                                pictureControlSetPtr,
                                sequenceControlSetPtr,
                                lcuOriginX,
                                lcuOriginY,
                                lastLcuFlag,
                                xLcuIndex * lcuSize,
                                yLcuIndex * lcuSize);
                        }
                    }

                    if (sequenceControlSetPtr->tileSliceMode == 0 && lastLcuFlag == EB_FALSE) {
                        EncodeTileFinish(pictureControlSetPtr->entropyCoderPtr);
                    } else if (sequenceControlSetPtr->tileSliceMode) {
                        EncodeSliceFinish(pictureControlSetPtr->entropyCoderPtr);
                    }

                    xLcuStart += sequenceControlSetPtr->tileColumnArray[columnIndex];
                }
                yLcuStart += sequenceControlSetPtr->tileRowArray[rowIndex];
            }

            // If the picture is complete, terminate the slice, 2nd pass DLF, SAO application
            if (lastLcuFlag == EB_TRUE)
            {
                EB_U32 refIdx;

                if (sequenceControlSetPtr->tileSliceMode == 0) {
                    EncodeSliceFinish(pictureControlSetPtr->entropyCoderPtr);
                }

                // Release the List 0 Reference Pictures
                for (refIdx = 0; refIdx < pictureControlSetPtr->ParentPcsPtr->refList0Count; ++refIdx) {
                    if (pictureControlSetPtr->refPicPtrArray[0]/*[refIdx]*/ != EB_NULL) {
                        EbReleaseObject(pictureControlSetPtr->refPicPtrArray[0]/*[refIdx]*/);
                    }
                }

                // Release the List 1 Reference Pictures
                for (refIdx = 0; refIdx < pictureControlSetPtr->ParentPcsPtr->refList1Count; ++refIdx) {
                    if (pictureControlSetPtr->refPicPtrArray[1]/*[refIdx]*/ != EB_NULL) {
                        EbReleaseObject(pictureControlSetPtr->refPicPtrArray[1]/*[refIdx]*/);
                    }
                }

                // Get Empty Entropy Coding Results
                EbGetEmptyObject(
                    contextPtr->entropyCodingOutputFifoPtr,
                    &entropyCodingResultsWrapperPtr);
                entropyCodingResultsPtr = (EntropyCodingResults_t*)entropyCodingResultsWrapperPtr->objectPtr;
                entropyCodingResultsPtr->pictureControlSetWrapperPtr = encDecResultsPtr->pictureControlSetWrapperPtr;

                // Post EntropyCoding Results
                EbPostFullObject(entropyCodingResultsWrapperPtr);

            } // End if(PictureCompleteFlag)
        }
        }
#endif


#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld EC OUT \n", pictureControlSetPtr->pictureNumber);
#endif
        // Release Mode Decision Results
        EbReleaseObject(encDecResultsWrapperPtr);

    }
    
    return EB_NULL;
}
