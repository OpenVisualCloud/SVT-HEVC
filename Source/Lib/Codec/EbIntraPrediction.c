/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>
#include "EbIntraPrediction.h"
#include "EbUtility.h"
#include "EbAvailability.h"
#include "EbModeDecision.h"
#include "EbCodingUnit.h"
#include "EbModeDecisionProcess.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

static const EB_U32 smoothingFilterShift[] = {
    3, // 4x4
    4, // 8x8

    5, // 16x16
    6, // 32x32
    7  // 64x64
};
static const EB_U32 intraModeAngularTable[] = {
    0,
    2,
    5,
    9,
    13,
    17,
    21,
    26,
    32
};
static const EB_S32 intraModeAngularTableNegative[] = {
    0,
    -2,
    -5,
    -9,
    -13,
    -17,
    -21,
    -26,
    -32
};

static const EB_U32 invIntraModeAngularTable[] = {
    0,
    4096,
    1638,
    910,
    630,
    482,
    390,
    315,
    256
};

static const EB_S32 intraLumaFilterTable[] = {
    35, //4x4   no filtering
    7,  //8x8
    1,  //16x16
    0,  //32x32
    10,  //64x64
};

#define MIDRANGE_VALUE_8BIT    128
#define MIDRANGE_VALUE_10BIT   512



/**********************************************
 * Intra Reference Samples Ctor
 **********************************************/
EB_ERRORTYPE IntraReferenceSamplesCtor(IntraReferenceSamples_t **contextDblPtr, EB_COLOR_FORMAT colorFormat)
{
    IntraReferenceSamples_t *contextPtr;
    EB_MALLOC(IntraReferenceSamples_t*, contextPtr, sizeof(IntraReferenceSamples_t), EB_N_PTR);
    *contextDblPtr = contextPtr;

    EB_MALLOC(EB_U8*, contextPtr->yIntraReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U8*, contextPtr->cbIntraReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U8*, contextPtr->crIntraReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U8*, contextPtr->yIntraFilteredReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);
   
    EB_MALLOC(EB_U8*, contextPtr->yIntraReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    EB_MALLOC(EB_U8*, contextPtr->yIntraFilteredReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    EB_MALLOC(EB_U8*, contextPtr->cbIntraReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    EB_MALLOC(EB_U8*, contextPtr->crIntraReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    contextPtr->yIntraReferenceArrayReverse++; //Jing: mem leak? bad here
    contextPtr->yIntraFilteredReferenceArrayReverse++;
    contextPtr->cbIntraReferenceArrayReverse++;
    contextPtr->crIntraReferenceArrayReverse++;

    if (colorFormat == EB_YUV444) {
        EB_MALLOC(EB_U8*, contextPtr->cbIntraFilteredReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);
        EB_MALLOC(EB_U8*, contextPtr->crIntraFilteredReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);
        EB_MALLOC(EB_U8*, contextPtr->cbIntraFilteredReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);
        EB_MALLOC(EB_U8*, contextPtr->crIntraFilteredReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);
        contextPtr->cbIntraFilteredReferenceArrayReverse++;
        contextPtr->crIntraFilteredReferenceArrayReverse++;
    } else {
        contextPtr->cbIntraFilteredReferenceArray = NULL;
        contextPtr->crIntraFilteredReferenceArray = NULL;
        contextPtr->cbIntraFilteredReferenceArrayReverse = NULL;
        contextPtr->crIntraFilteredReferenceArrayReverse = NULL;
    }

    return EB_ErrorNone;
}

/**********************************************
 * Intra Reference Samples Ctor
 **********************************************/
EB_ERRORTYPE IntraReference16bitSamplesCtor(
    IntraReference16bitSamples_t **contextDblPtr,
    EB_COLOR_FORMAT colorFormat)
{
    IntraReference16bitSamples_t *contextPtr;
    EB_MALLOC(IntraReference16bitSamples_t*, contextPtr, sizeof(IntraReference16bitSamples_t), EB_N_PTR);
    *contextDblPtr = contextPtr;
 
    EB_MALLOC(EB_U16*, contextPtr->yIntraReferenceArray, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->cbIntraReferenceArray, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->crIntraReferenceArray, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->yIntraFilteredReferenceArray, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->yIntraReferenceArrayReverse, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->yIntraFilteredReferenceArrayReverse, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->cbIntraReferenceArrayReverse, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    EB_MALLOC(EB_U16*, contextPtr->crIntraReferenceArrayReverse, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    contextPtr->yIntraReferenceArrayReverse++;
    contextPtr->yIntraFilteredReferenceArrayReverse++;
    contextPtr->cbIntraReferenceArrayReverse++;
    contextPtr->crIntraReferenceArrayReverse++;

    if (colorFormat == EB_YUV444) {
        EB_MALLOC(EB_U16*, contextPtr->cbIntraFilteredReferenceArray, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);
        EB_MALLOC(EB_U16*, contextPtr->crIntraFilteredReferenceArray, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);
        EB_MALLOC(EB_U16*, contextPtr->cbIntraFilteredReferenceArrayReverse, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);
        EB_MALLOC(EB_U16*, contextPtr->crIntraFilteredReferenceArrayReverse, sizeof(EB_U16) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);
        contextPtr->cbIntraFilteredReferenceArrayReverse++;
        contextPtr->crIntraFilteredReferenceArrayReverse++;
    } else {
        contextPtr->cbIntraFilteredReferenceArray = NULL;
        contextPtr->crIntraFilteredReferenceArray = NULL;
        contextPtr->cbIntraFilteredReferenceArrayReverse = NULL;
        contextPtr->crIntraFilteredReferenceArrayReverse = NULL;
    }

    return EB_ErrorNone;
}



/*******************************************
 * Generate Intra Reference Samples
 *******************************************/
EB_ERRORTYPE GenerateIntraReferenceSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary)
{
    EB_ERRORTYPE          return_error = EB_ErrorNone;
    IntraReferenceSamples_t          *intraRefPtr = (IntraReferenceSamples_t          *)refWrapperPtr;
    EB_U8                *yBorder                 = intraRefPtr->yIntraReferenceArray;
    EB_U8                *cbBorder                = intraRefPtr->cbIntraReferenceArray;
    EB_U8                *crBorder                = intraRefPtr->crIntraReferenceArray;
    EB_U8                *yBorderFilt             = intraRefPtr->yIntraFilteredReferenceArray;

    EB_U8                *yBorderReverse          = intraRefPtr->yIntraReferenceArrayReverse;
    EB_U8                *yBorderFiltReverse      = intraRefPtr->yIntraFilteredReferenceArrayReverse;
    EB_U8                *cbBorderReverse         = intraRefPtr->cbIntraReferenceArrayReverse;
    EB_U8                *crBorderReverse         = intraRefPtr->crIntraReferenceArrayReverse;
      
    const EB_U32          sizeLog2      = Log2f(size);
    const EB_U32          puChromaSize    = size >> 1;

    const EB_U16 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    EB_U32                yLoadCounter;
    EB_U8                *sampleReadLoc;
    EB_U8                *sampleWriteLoc;
    EB_U8                *sampleWriteLocCb;
    EB_U8                *sampleWriteLocCr;
    EB_U32                i;
    EB_U8                *sampleWriteLocFilt;
       
    // This internal LCU availability check will be performed for top right and bottom left neighbors only.
    // It is always set to true for top, left and top left neighbors
    EB_BOOL               bottomLeftAvailabilityPreCalc;
    EB_BOOL               topRightAvailabilityPreCalc;
    const EB_U32          cuIndex = ((originY & (lcuSize-1)) >> sizeLog2) * (1 << cuDepth) + ((originX & (lcuSize-1)) >> sizeLog2);
    EB_U32                blockIndex;           // 4x4 (Min PU size) granularity
    EB_BOOL               neighborAvailable;

    const EB_U32          bottomLeftEnd         =     (size >> LOG_MIN_PU_SIZE);
    const EB_U32          leftBlockEnd          = 2 * (size >> LOG_MIN_PU_SIZE);
    const EB_U32          topLeftBlockEnd       = 2 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32          topRightBlockBegin    = 3 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32          topBlockEnd           = 4 * (size >> LOG_MIN_PU_SIZE) + 1;
    

    EB_U32                reconArrayIndex;
    EB_U32                modeArrayIndex;
    
    EB_U8                 lumaPadValue  = 0;
    EB_U8                 cbPadValue    = 0;
    EB_U8                 crPadValue    = 0;

    EB_U8                *lumaWritePtr = yBorder;
    EB_U8                *cbWritePtr   = cbBorder;
    EB_U8                *crWritePtr   = crBorder;
    
    EB_U32                writeCountLuma;
    EB_U32                writeCountChroma;

    // Neighbor Arrays
    EB_U32                topModeNeighborArraySize      = modeTypeNeighborArray->topArraySize;
    EB_U8                *topModeNeighborArray          = modeTypeNeighborArray->topArray;
    EB_U32                leftModeNeighborArraySize     = modeTypeNeighborArray->leftArraySize;
    EB_U8                *leftModeNeighborArray         = modeTypeNeighborArray->leftArray;
    EB_U8                *topLeftModeNeighborArray      = modeTypeNeighborArray->topLeftArray;
    EB_U8                *topLumaReconNeighborArray     = lumaReconNeighborArray->topArray;
    EB_U8                *leftLumaReconNeighborArray    = lumaReconNeighborArray->leftArray;
    EB_U8                *topLeftLumaReconNeighborArray = lumaReconNeighborArray->topLeftArray;
    EB_U8                *topCbReconNeighborArray       = cbReconNeighborArray->topArray;
    EB_U8                *leftCbReconNeighborArray      = cbReconNeighborArray->leftArray;
    EB_U8                *topLeftCbReconNeighborArray   = cbReconNeighborArray->topLeftArray;
    EB_U8                *topCrReconNeighborArray       = crReconNeighborArray->topArray;
    EB_U8                *leftCrReconNeighborArray      = crReconNeighborArray->leftArray;
    EB_U8                *topLeftCrReconNeighborArray   = crReconNeighborArray->topLeftArray;

    // The Generate Intra Reference sample process is a single pass algorithm
    //   that runs through the neighbor arrays from the bottom left to top right
    //   and analyzes which samples are available via a spatial availability 
    //   check and various mode checks. Un-available samples at the beginning
    //   of the run (top-right side) are padded with the first valid sample and
    //   all other missing samples are padded with the last valid sample.
    //
    //   *  - valid sample
    //   x  - missing sample
    //   |  - sample used for padding
    //   <- - padding (copy) operation 
    //       
    //                              TOP
    //                                                          0
    //  TOP-LEFT                |------->       |--------------->                     
    //          * * * * * * * * * x x x x * * * * x x x x x x x x 
    //          *                                               
    //          *                                  
    //          *                                  
    //          *                                  
    //       ^  x                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       -  *                                  
    //  LEFT    *                                  
    //          *                                  
    //       -  *                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       v  x END                                
    //  
    //  Skeleton:
    //    1. Start at position 0
    //    2. Loop until first valid position
    //       a. Separate loop for Left, Top-left, and Top neighbor arrays
    //    3. If no valid samples found, write mid-range value (128 for 8-bit)
    //    4. Else, write the first valid sample into the invalid range
    //    5. Left Loop 
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    6. Top-left Sample (no loop)                            
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    7. Top Loop
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value

    // Pre-calculate bottom left availability
    bottomLeftAvailabilityPreCalc = isBottomLeftAvailable(
        cuDepth,
        cuIndex);

    // Pre-calculate top right availability
    topRightAvailabilityPreCalc = isUpperRightAvailable(
        cuDepth,
        cuIndex);

    //*************************************************
    // Part 1: Initial Invalid Sample Loops
    //*************************************************
    
    // Left Block Loop
    blockIndex = 0;
    reconArrayIndex = originY + 2 * size - MIN_PU_SIZE;
    neighborAvailable = EB_FALSE;
    while(blockIndex < leftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :         // picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Set pad value (end of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
            cbPadValue   = leftCbReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> subHeightCMinus1];
            crPadValue   = leftCrReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> subHeightCMinus1];
        }
        else {
            ++blockIndex;
            reconArrayIndex -= MIN_PU_SIZE;
        }
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)               ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (end of block)
            lumaPadValue = topLeftLumaReconNeighborArray[reconArrayIndex];
            cbPadValue   = topLeftCbReconNeighborArray[((MAX_PICTURE_HEIGHT_SIZE- originY)>>subHeightCMinus1) + (originX>>subWidthCMinus1)];
            crPadValue   = topLeftCrReconNeighborArray[((MAX_PICTURE_HEIGHT_SIZE- originY)>>subHeightCMinus1) + (originX>>subWidthCMinus1)];
        }
        else {
            ++blockIndex;
        }
    }

    // Top Block Loop
	reconArrayIndex = originX;
    while(blockIndex < topBlockEnd && neighborAvailable == EB_FALSE) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (beginning of block)
            lumaPadValue = topLumaReconNeighborArray[reconArrayIndex];
            cbPadValue   = topCbReconNeighborArray[reconArrayIndex >> subWidthCMinus1];
            crPadValue   = topCrReconNeighborArray[reconArrayIndex >> subWidthCMinus1];
        }
        else {
            ++blockIndex;
            reconArrayIndex += MIN_PU_SIZE;
        }
       
    }

    // Check for no valid border samples
    if (blockIndex == topBlockEnd && neighborAvailable == EB_FALSE) {

        writeCountLuma   = 4*size + 1;
        writeCountChroma = 4*puChromaSize + 1;

        // Write Midrange
        EB_MEMSET(lumaWritePtr, MIDRANGE_VALUE_8BIT, writeCountLuma);
        EB_MEMSET(cbWritePtr,   MIDRANGE_VALUE_8BIT, writeCountChroma);
        EB_MEMSET(crWritePtr,   MIDRANGE_VALUE_8BIT, writeCountChroma);
    } 
    else {
        
        // Write Pad Value - adjust for the TopLeft block being 1-sample 
        writeCountLuma = (blockIndex >= topLeftBlockEnd) ?
            (blockIndex-1) * MIN_PU_SIZE + 1 :
            blockIndex * MIN_PU_SIZE;
        
        writeCountChroma = (blockIndex >= topLeftBlockEnd) ?
            (((blockIndex-1) * MIN_PU_SIZE) >> 1) + 1 :
             ((blockIndex    * MIN_PU_SIZE) >> 1);
            
        EB_MEMSET(lumaWritePtr, lumaPadValue, writeCountLuma);
        EB_MEMSET(cbWritePtr,   cbPadValue,   writeCountChroma);
        EB_MEMSET(crWritePtr,   crPadValue,   writeCountChroma);
    }

    lumaWritePtr += writeCountLuma;
    cbWritePtr   += writeCountChroma;
    crWritePtr   += writeCountChroma;

    //*************************************************
    // Part 2: Copy the remainder of the samples
    //*************************************************

    // Left Block Loop
    reconArrayIndex = originY + (2 * size - MIN_PU_SIZE) - (blockIndex * MIN_PU_SIZE);
    while(blockIndex < leftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Copy samples (Reverse the order)
            lumaWritePtr[0] = leftLumaReconNeighborArray[reconArrayIndex + 3];
            lumaWritePtr[1] = leftLumaReconNeighborArray[reconArrayIndex + 2];
            lumaWritePtr[2] = leftLumaReconNeighborArray[reconArrayIndex + 1];
            lumaWritePtr[3] = leftLumaReconNeighborArray[reconArrayIndex + 0];
            
            cbWritePtr[0] = leftCbReconNeighborArray[(reconArrayIndex >> subHeightCMinus1) + 1];
            cbWritePtr[1] = leftCbReconNeighborArray[(reconArrayIndex >> subHeightCMinus1) + 0];
            crWritePtr[0] = leftCrReconNeighborArray[(reconArrayIndex >> subHeightCMinus1) + 1];
            crWritePtr[1] = leftCrReconNeighborArray[(reconArrayIndex >> subHeightCMinus1) + 0];
            
            // Set pad value (beginning of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex];
            cbPadValue   = leftCbReconNeighborArray[reconArrayIndex >> subHeightCMinus1];
            crPadValue   = leftCrReconNeighborArray[reconArrayIndex >> subHeightCMinus1];
        }
        else {

            // Copy pad value
            EB_MEMSET(lumaWritePtr, lumaPadValue, MIN_PU_SIZE);
            EB_MEMSET(cbWritePtr,   cbPadValue,   MIN_PU_SIZE >> 1);
            EB_MEMSET(crWritePtr,   crPadValue,   MIN_PU_SIZE >> 1);
        }

        lumaWritePtr += MIN_PU_SIZE;
        cbWritePtr   += MIN_PU_SIZE >> 1;//Jing: works fine for both 420 and 422
        crWritePtr   += MIN_PU_SIZE >> 1;

        ++blockIndex;
        reconArrayIndex -= MIN_PU_SIZE;
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Copy sample
            *lumaWritePtr = topLeftLumaReconNeighborArray[reconArrayIndex];
            *cbWritePtr   = topLeftCbReconNeighborArray[((MAX_PICTURE_HEIGHT_SIZE- originY)>>subHeightCMinus1) + (originX>>subWidthCMinus1)];
            *crWritePtr   = topLeftCrReconNeighborArray[((MAX_PICTURE_HEIGHT_SIZE- originY)>>subHeightCMinus1) + (originX>>subWidthCMinus1)];

            // Set Pad Value
            lumaPadValue  = topLeftLumaReconNeighborArray[reconArrayIndex];
            cbPadValue    = topLeftCbReconNeighborArray[((MAX_PICTURE_HEIGHT_SIZE- originY)>>subHeightCMinus1) + (originX>>subWidthCMinus1)];
            crPadValue    = topLeftCrReconNeighborArray[((MAX_PICTURE_HEIGHT_SIZE- originY)>>subHeightCMinus1) + (originX>>subWidthCMinus1)];
        }
        else {
            
            // Copy pad value
            *lumaWritePtr = lumaPadValue;
            *cbWritePtr   = cbPadValue;
            *crWritePtr   = crPadValue;
        }

        ++lumaWritePtr;
        ++cbWritePtr;
        ++crWritePtr;

        ++blockIndex;
    }

    // Top Block Loop
	reconArrayIndex = originX + (blockIndex - topLeftBlockEnd) * MIN_PU_SIZE;
    while(blockIndex < topBlockEnd) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
			EB_MEMCPY(lumaWritePtr, &topLumaReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);
			EB_MEMCPY(cbWritePtr, &topCbReconNeighborArray[reconArrayIndex >> subWidthCMinus1], MIN_PU_SIZE >> subWidthCMinus1);
			EB_MEMCPY(crWritePtr, &topCrReconNeighborArray[reconArrayIndex >> subWidthCMinus1], MIN_PU_SIZE >> subWidthCMinus1);

            // Set pad value (end of block)
            lumaPadValue    = topLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
            cbPadValue      = topCbReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> subWidthCMinus1];
            crPadValue      = topCrReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> subWidthCMinus1];
        }
        else {
            
            // Copy pad value
            EB_MEMSET(lumaWritePtr,    lumaPadValue, MIN_PU_SIZE);
            EB_MEMSET(cbWritePtr,      cbPadValue,   MIN_PU_SIZE >> subWidthCMinus1);
            EB_MEMSET(crWritePtr,      crPadValue,   MIN_PU_SIZE >> subWidthCMinus1);
        }
    
        lumaWritePtr += MIN_PU_SIZE;
        cbWritePtr   += MIN_PU_SIZE >> subWidthCMinus1;
        crWritePtr   += MIN_PU_SIZE >> subWidthCMinus1;
    
        ++blockIndex;
        reconArrayIndex += MIN_PU_SIZE;
    }
    
    //*************************************************
    // Part 3: Strong Intra Filter Samples
    //*************************************************
    sampleReadLoc = yBorder;
    sampleWriteLoc = yBorderFilt;
    // Loop is only over the non-edge samples (TotalCount - 2)
    yLoadCounter = (size << 2) - 1;

    if(strongIntraSmoothingFlag == EB_TRUE) {
        EB_U32 filterShift;
        EB_U32 index;
        EB_U32 bottomLeftSample = sampleReadLoc[0];
        EB_U32 topLeftSample    = sampleReadLoc[(size << 1)];
        EB_U32 topRightSample   = sampleReadLoc[(size << 2)];
        EB_U32 twicePuSize      = (size << 1);

        EB_BOOL bilinearLeft = (ABS((EB_S32)bottomLeftSample + (EB_S32)topLeftSample  - 2 * sampleReadLoc[size])             < SMOOTHING_THRESHOLD) ? EB_TRUE : EB_FALSE;
        EB_BOOL bilinearTop  = (ABS((EB_S32)topLeftSample    + (EB_S32)topRightSample - 2 * sampleReadLoc[size+(size << 1)]) < SMOOTHING_THRESHOLD) ? EB_TRUE : EB_FALSE;

        if(size >= STRONG_INTRA_SMOOTHING_BLOCKSIZE && bilinearLeft && bilinearTop) {
            filterShift = smoothingFilterShift[Log2f(size)-2];
            sampleWriteLoc[0]           = sampleReadLoc[0];
            sampleWriteLoc[(size << 1)] = sampleReadLoc[(size << 1)];
            sampleWriteLoc[(size << 2)] = sampleReadLoc[(size << 2)];

            for(index = 1; index < twicePuSize; index++) {
                sampleWriteLoc[index]             = (EB_U8)(((twicePuSize - index) * bottomLeftSample + index * topLeftSample  + size) >> filterShift);
                sampleWriteLoc[twicePuSize+index] = (EB_U8)(((twicePuSize - index) * topLeftSample    + index * topRightSample + size) >> filterShift);
            }
        }

        else {
            // First Sample
            *sampleWriteLoc++ = *sampleReadLoc++;

            // Internal Filtered Samples
            do {
                *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
                ++sampleReadLoc;
            } while(--yLoadCounter);

            // Last Sample
            *sampleWriteLoc = *sampleReadLoc;
        }
    }
    else {

        // First Sample
        *sampleWriteLoc++ = *sampleReadLoc++;

        // Internal Filtered Samples
        do {
            *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
            ++sampleReadLoc;
        } while(--yLoadCounter);

        // Last Sample
        *sampleWriteLoc = *sampleReadLoc;
    }

    //*************************************************
    // Part 4: Create Reversed Reference Samples
    //*************************************************
    
    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagY  = EB_FALSE;
    intraRefPtr->AboveReadyFlagCb = EB_FALSE;
    intraRefPtr->AboveReadyFlagCr = EB_FALSE;
    
    intraRefPtr->LeftReadyFlagY   = EB_FALSE;
    intraRefPtr->LeftReadyFlagCb  = EB_FALSE;
    intraRefPtr->LeftReadyFlagCr  = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                     =======>    L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  
   
    //Luma
	EB_MEMCPY(yBorderReverse     + (size<<1),  yBorder     + (size<<1),  (size<<1)+1);
	EB_MEMCPY(yBorderFiltReverse + (size<<1),  yBorderFilt + (size<<1),  (size<<1)+1);

    sampleWriteLoc     = yBorderReverse      + (size<<1) - 1 ;
    sampleWriteLocFilt = yBorderFiltReverse  + (size<<1) - 1 ;
    for(i=0; i<(size<<1)   ;i++){
        
       *sampleWriteLoc     = yBorder[i];
       *sampleWriteLocFilt = yBorderFilt[i] ;
        sampleWriteLoc--;
        sampleWriteLocFilt--;
    }

    //Chroma    
	EB_MEMCPY(cbBorderReverse + (puChromaSize<<1),  cbBorder + (puChromaSize<<1),  (puChromaSize<<1)+1);
	EB_MEMCPY(crBorderReverse + (puChromaSize<<1),  crBorder + (puChromaSize<<1),  (puChromaSize<<1)+1);

    sampleWriteLocCb     = cbBorderReverse      + (puChromaSize<<1) - 1 ;
    sampleWriteLocCr     = crBorderReverse      + (puChromaSize<<1) - 1 ;
     
    for(i=0; i<(puChromaSize<<1)   ;i++){
        
       *sampleWriteLocCb     = cbBorder[i];
       *sampleWriteLocCr     = crBorder[i];
        sampleWriteLocCb--;
        sampleWriteLocCr--;
    }

    return return_error;
}

/*******************************************
 * Generate Luma Intra Reference Samples
 *******************************************/
EB_ERRORTYPE GenerateLumaIntraReferenceSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary)
{
    EB_ERRORTYPE          return_error = EB_ErrorNone;
    IntraReferenceSamples_t          *intraRefPtr = (IntraReferenceSamples_t          *)refWrapperPtr;
    EB_U8                *yBorder                 = intraRefPtr->yIntraReferenceArray;
    EB_U8                *yBorderFilt             = intraRefPtr->yIntraFilteredReferenceArray;

    EB_U8                *yBorderReverse          = intraRefPtr->yIntraReferenceArrayReverse;
    EB_U8                *yBorderFiltReverse      = intraRefPtr->yIntraFilteredReferenceArrayReverse;
      
    const EB_U32          sizeLog2      = Log2f(size);

    EB_U32                yLoadCounter;
    EB_U8                *sampleReadLoc;
    EB_U8                *sampleWriteLoc;
    EB_U32                i;
    EB_U8                *sampleWriteLocFilt;
       
    // This internal LCU availability check will be performed for top right and bottom left neighbors only.
    // It is always set to true for top, left and top left neighbors
    EB_BOOL               bottomLeftAvailabilityPreCalc;
    EB_BOOL               topRightAvailabilityPreCalc;
    
    EB_U32                partitionDepth    =   (size == MIN_PU_SIZE) ? cuDepth + 1 : cuDepth;
    const EB_U32          cuIndex           =   ((originY & (lcuSize - 1)) >> sizeLog2) * (1 << partitionDepth) + ((originX & (lcuSize - 1)) >> sizeLog2);

    EB_U32                blockIndex;           // 4x4 (Min PU size) granularity
    EB_BOOL               neighborAvailable;

    const EB_U32          bottomLeftEnd         =     (size >> LOG_MIN_PU_SIZE);
    const EB_U32          leftBlockEnd          = 2 * (size >> LOG_MIN_PU_SIZE);
    const EB_U32          topLeftBlockEnd       = 2 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32          topRightBlockBegin    = 3 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32          topBlockEnd           = 4 * (size >> LOG_MIN_PU_SIZE) + 1;
    

    EB_U32                reconArrayIndex;
    EB_U32                modeArrayIndex;
    
    EB_U8                 lumaPadValue  = 0;

    EB_U8                *lumaWritePtr = yBorder;
    
    EB_U32                writeCountLuma;

    // Neighbor Arrays
    EB_U32                topModeNeighborArraySize      = modeTypeNeighborArray->topArraySize;
    EB_U8                *topModeNeighborArray          = modeTypeNeighborArray->topArray;
    EB_U32                leftModeNeighborArraySize     = modeTypeNeighborArray->leftArraySize;
    EB_U8                *leftModeNeighborArray         = modeTypeNeighborArray->leftArray;
    EB_U8                *topLeftModeNeighborArray      = modeTypeNeighborArray->topLeftArray;
    EB_U8                *topLumaReconNeighborArray     = lumaReconNeighborArray->topArray;
    EB_U8                *leftLumaReconNeighborArray    = lumaReconNeighborArray->leftArray;
    EB_U8                *topLeftLumaReconNeighborArray = lumaReconNeighborArray->topLeftArray;

    (void) cbReconNeighborArray;
    (void) crReconNeighborArray;

    // The Generate Intra Reference sample process is a single pass algorithm
    //   that runs through the neighbor arrays from the bottom left to top right
    //   and analyzes which samples are available via a spatial availability 
    //   check and various mode checks. Un-available samples at the beginning
    //   of the run (top-right side) are padded with the first valid sample and
    //   all other missing samples are padded with the last valid sample.
    //
    //   *  - valid sample
    //   x  - missing sample
    //   |  - sample used for padding
    //   <- - padding (copy) operation 
    //       
    //                              TOP
    //                                                          0
    //  TOP-LEFT                |------->       |--------------->                     
    //          * * * * * * * * * x x x x * * * * x x x x x x x x 
    //          *                                               
    //          *                                  
    //          *                                  
    //          *                                  
    //       ^  x                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       -  *                                  
    //  LEFT    *                                  
    //          *                                  
    //       -  *                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       v  x END                                
    //  
    //  Skeleton:
    //    1. Start at position 0
    //    2. Loop until first valid position
    //       a. Separate loop for Left, Top-left, and Top neighbor arrays
    //    3. If no valid samples found, write mid-range value (128 for 8-bit)
    //    4. Else, write the first valid sample into the invalid range
    //    5. Left Loop 
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    6. Top-left Sample (no loop)                            
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    7. Top Loop
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value

    // Pre-calculate bottom left availability
    bottomLeftAvailabilityPreCalc = isBottomLeftAvailable(
        partitionDepth,
        cuIndex);

    // Pre-calculate top right availability
    topRightAvailabilityPreCalc = isUpperRightAvailable(
        partitionDepth,
        cuIndex);

    //*************************************************
    // Part 1: Initial Invalid Sample Loops
    //*************************************************
    
    // Left Block Loop
    blockIndex = 0;
    reconArrayIndex = originY + 2 * size - MIN_PU_SIZE;
    neighborAvailable = EB_FALSE;
    while(blockIndex < leftBlockEnd && neighborAvailable == EB_FALSE) {
        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Set pad value (end of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];

        }
        else {
            ++blockIndex;
            reconArrayIndex -= MIN_PU_SIZE;
        }
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)               ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (end of block)
            lumaPadValue = topLeftLumaReconNeighborArray[reconArrayIndex];

        }
        else {
            ++blockIndex;
        }
    }

    // Top Block Loop
    reconArrayIndex = originX;
    while(blockIndex < topBlockEnd && neighborAvailable == EB_FALSE) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (beginning of block)
            lumaPadValue = topLumaReconNeighborArray[reconArrayIndex];
        }
        else {
            ++blockIndex;
            reconArrayIndex += MIN_PU_SIZE;
        }
       
    }

    // Check for no valid border samples
    if (blockIndex == topBlockEnd && neighborAvailable == EB_FALSE) {

        writeCountLuma   = 4*size + 1;

        // Write Midrange
        EB_MEMSET(lumaWritePtr, MIDRANGE_VALUE_8BIT, writeCountLuma);
    } 
    else {
        
        // Write Pad Value - adjust for the TopLeft block being 1-sample 
        writeCountLuma = (blockIndex >= topLeftBlockEnd) ?
            (blockIndex-1) * MIN_PU_SIZE + 1 :
            blockIndex * MIN_PU_SIZE;
            
        EB_MEMSET(lumaWritePtr, lumaPadValue, writeCountLuma);
    }

    lumaWritePtr += writeCountLuma;

    //*************************************************
    // Part 2: Copy the remainder of the samples
    //*************************************************

    // Left Block Loop
    reconArrayIndex = originY + (2 * size - MIN_PU_SIZE) - (blockIndex * MIN_PU_SIZE);
    while(blockIndex < leftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Copy samples (Reverse the order)
            lumaWritePtr[0] = leftLumaReconNeighborArray[reconArrayIndex + 3];
            lumaWritePtr[1] = leftLumaReconNeighborArray[reconArrayIndex + 2];
            lumaWritePtr[2] = leftLumaReconNeighborArray[reconArrayIndex + 1];
            lumaWritePtr[3] = leftLumaReconNeighborArray[reconArrayIndex + 0];
            
            // Set pad value (beginning of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex];
        }
        else {

            // Copy pad value
            EB_MEMSET(lumaWritePtr, lumaPadValue, MIN_PU_SIZE);
        }

        lumaWritePtr += MIN_PU_SIZE;

        ++blockIndex;
        reconArrayIndex -= MIN_PU_SIZE;
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Copy sample
            *lumaWritePtr = topLeftLumaReconNeighborArray[reconArrayIndex];

            // Set Pad Value
            lumaPadValue  = topLeftLumaReconNeighborArray[reconArrayIndex];
        }
        else {
            
            // Copy pad value
            *lumaWritePtr = lumaPadValue;
        }

        ++lumaWritePtr;

        ++blockIndex;
    }

    // Top Block Loop
    reconArrayIndex = originX +  (blockIndex - topLeftBlockEnd)*MIN_PU_SIZE;

    while(blockIndex < topBlockEnd) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Copy samples in reverse order
			EB_MEMCPY(lumaWritePtr, &topLumaReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);

            // Set pad value (end of block)
            lumaPadValue    = topLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
    
        }
        else {
            
            // Copy pad value
            EB_MEMSET(lumaWritePtr,    lumaPadValue, MIN_PU_SIZE);
        }
    
        lumaWritePtr += MIN_PU_SIZE;
    
        ++blockIndex;
        reconArrayIndex += MIN_PU_SIZE;
    }
    
    //*************************************************
    // Part 3: Strong Intra Filter Samples
    //*************************************************
    sampleReadLoc = yBorder;
    sampleWriteLoc = yBorderFilt;
    // Loop is only over the non-edge samples (TotalCount - 2)
    yLoadCounter = (size << 2) - 1;

    if(strongIntraSmoothingFlag == EB_TRUE) {
        EB_U32 filterShift;
        EB_U32 index;
        EB_U32 bottomLeftSample = sampleReadLoc[0];
        EB_U32 topLeftSample    = sampleReadLoc[(size << 1)];
        EB_U32 topRightSample   = sampleReadLoc[(size << 2)];
        EB_U32 twicePuSize      = (size << 1);

        EB_BOOL bilinearLeft = (ABS((EB_S32)bottomLeftSample + (EB_S32)topLeftSample  - 2 * sampleReadLoc[size])             < SMOOTHING_THRESHOLD) ? EB_TRUE : EB_FALSE;
        EB_BOOL bilinearTop  = (ABS((EB_S32)topLeftSample    + (EB_S32)topRightSample - 2 * sampleReadLoc[size+(size << 1)]) < SMOOTHING_THRESHOLD) ? EB_TRUE : EB_FALSE;

        if(size >= STRONG_INTRA_SMOOTHING_BLOCKSIZE && bilinearLeft && bilinearTop) {
            filterShift = smoothingFilterShift[Log2f(size)-2];
            sampleWriteLoc[0]           = sampleReadLoc[0];
            sampleWriteLoc[(size << 1)] = sampleReadLoc[(size << 1)];
            sampleWriteLoc[(size << 2)] = sampleReadLoc[(size << 2)];

            for(index = 1; index < twicePuSize; index++) {
                sampleWriteLoc[index]             = (EB_U8)(((twicePuSize - index) * bottomLeftSample + index * topLeftSample  + size) >> filterShift);
                sampleWriteLoc[twicePuSize+index] = (EB_U8)(((twicePuSize - index) * topLeftSample    + index * topRightSample + size) >> filterShift);
            }
        }
        else {
            // First Sample
            *sampleWriteLoc++ = *sampleReadLoc++;

            // Internal Filtered Samples
            do {
                *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
                ++sampleReadLoc;
            } while(--yLoadCounter);

            // Last Sample
            *sampleWriteLoc = *sampleReadLoc;
        }
    }
    else {

        // First Sample
        *sampleWriteLoc++ = *sampleReadLoc++;

        // Internal Filtered Samples
        do {
            *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
            ++sampleReadLoc;
        } while(--yLoadCounter);

        // Last Sample
        *sampleWriteLoc = *sampleReadLoc;
    }

    //*************************************************
    // Part 4: Create Reversed Reference Samples
    //*************************************************
    
    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagY  = EB_FALSE;
    intraRefPtr->LeftReadyFlagY   = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                     =======>    L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	EB_MEMCPY(yBorderReverse     + (size<<1),  yBorder     + (size<<1),  (size<<1)+1);
	EB_MEMCPY(yBorderFiltReverse + (size<<1),  yBorderFilt + (size<<1),  (size<<1)+1);

    sampleWriteLoc     = yBorderReverse      + (size<<1) - 1 ;
    sampleWriteLocFilt = yBorderFiltReverse  + (size<<1) - 1 ;
    for(i=0; i<(size<<1)   ;i++){
        
       *sampleWriteLoc     = yBorder[i];
       *sampleWriteLocFilt = yBorderFilt[i] ;
        sampleWriteLoc--;
        sampleWriteLocFilt--;
    }

    return return_error;
}

/*******************************************
 * Generate Chroma Intra Reference Samples
 *******************************************/
EB_ERRORTYPE GenerateChromaIntraReferenceSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      cuOriginX,
    EB_U32                      cuOriginY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     secondChroma,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    IntraReferenceSamples_t *intraRefPtr = (IntraReferenceSamples_t*)refWrapperPtr;

    EB_U8 *cbBorder = intraRefPtr->cbIntraReferenceArray;
    EB_U8 *cbBorderFilt = intraRefPtr->cbIntraFilteredReferenceArray;
    EB_U8 *crBorder = intraRefPtr->crIntraReferenceArray;
    EB_U8 *crBorderFilt = intraRefPtr->crIntraFilteredReferenceArray;

    EB_U8 *cbBorderReverse = intraRefPtr->cbIntraReferenceArrayReverse;
    EB_U8 *cbBorderFiltReverse = intraRefPtr->cbIntraFilteredReferenceArrayReverse;
    EB_U8 *crBorderReverse = intraRefPtr->crIntraReferenceArrayReverse;
    EB_U8 *crBorderFiltReverse = intraRefPtr->crIntraFilteredReferenceArrayReverse;
      
    const EB_U32 sizeLog2 = Log2f(size);
    const EB_U32 puChromaSize = size >> ((colorFormat==EB_YUV420 || colorFormat==EB_YUV422) ? 1 : 0);
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    const EB_U32 chromaOffset = (colorFormat == EB_YUV422 && secondChroma) ? puChromaSize : 0;

    EB_U32 i;
    EB_U32 yLoadCounter;
    EB_U8 *sampleReadLocCb;
    EB_U8 *sampleWriteLocCb;
    EB_U8 *sampleReadLocCr;
    EB_U8 *sampleWriteLocCr;
    EB_U8 *sampleWriteLocCbFilt;
    EB_U8 *sampleWriteLocCrFilt;
       
    // This internal LCU availability check will be performed for top right and bottom left neighbors only.
    // It is always set to true for top, left and top left neighbors
    EB_BOOL bottomLeftAvailabilityPreCalc;
    EB_BOOL topRightAvailabilityPreCalc;

    const EB_U32 partitionDepth = (size == MIN_PU_SIZE) ? cuDepth + 1 : cuDepth;
    const EB_U32 cuIndex = ((cuOriginY & (lcuSize - 1)) >> sizeLog2) * (1 << partitionDepth) + ((cuOriginX & (lcuSize - 1)) >> sizeLog2);

    EB_U32 blockIndex;           // 4x4 (Min PU size) granularity
    EB_BOOL neighborAvailable;

    EB_U32 bottomLeftEnd = (puChromaSize >> LOG_MIN_PU_SIZE);
    EB_U32 leftBlockEnd = 2 * (puChromaSize >> LOG_MIN_PU_SIZE);
    EB_U32 topLeftBlockEnd = 2 * (puChromaSize >> LOG_MIN_PU_SIZE) + 1;
    EB_U32 topRightBlockBegin = 3 * (puChromaSize >> LOG_MIN_PU_SIZE) + 1;
    EB_U32 topBlockEnd = 4 * (puChromaSize >> LOG_MIN_PU_SIZE) + 1;
    

    EB_U32 reconArrayIndex;
    EB_U32 modeArrayIndex;

    EB_U8 cbPadValue = 0;
    EB_U8 crPadValue = 0;

    EB_U8 *cbWritePtr = cbBorder;
    EB_U8 *crWritePtr = crBorder;

    EB_U32 writeCountChroma;

    // Neighbor Arrays
    EB_U32                topModeNeighborArraySize      = modeTypeNeighborArray->topArraySize;
    EB_U8                *topModeNeighborArray          = modeTypeNeighborArray->topArray;
    EB_U32                leftModeNeighborArraySize     = modeTypeNeighborArray->leftArraySize;
    EB_U8                *leftModeNeighborArray         = modeTypeNeighborArray->leftArray;
    EB_U8                *topLeftModeNeighborArray      = modeTypeNeighborArray->topLeftArray;
    EB_U8                *topCbReconNeighborArray       = cbReconNeighborArray->topArray;
    EB_U8                *leftCbReconNeighborArray      = cbReconNeighborArray->leftArray;
    EB_U8                *topLeftCbReconNeighborArray   = cbReconNeighborArray->topLeftArray;
    EB_U8                *topCrReconNeighborArray       = crReconNeighborArray->topArray;
    EB_U8                *leftCrReconNeighborArray      = crReconNeighborArray->leftArray;
    EB_U8                *topLeftCrReconNeighborArray   = crReconNeighborArray->topLeftArray;

    (void) strongIntraSmoothingFlag;
    (void) lumaReconNeighborArray;

    // The Generate Intra Reference sample process is a single pass algorithm
    //   that runs through the neighbor arrays from the bottom left to top right
    //   and analyzes which samples are available via a spatial availability 
    //   check and various mode checks. Un-available samples at the beginning
    //   of the run (top-right side) are padded with the first valid sample and
    //   all other missing samples are padded with the last valid sample.
    //
    //   *  - valid sample
    //   x  - missing sample
    //   |  - sample used for padding
    //   <- - padding (copy) operation 
    //       
    //                              TOP
    //                                                          0
    //  TOP-LEFT                |------->       |--------------->                     
    //          * * * * * * * * * x x x x * * * * x x x x x x x x 
    //          *                                               
    //          *                                  
    //          *                                  
    //          *                                  
    //       ^  x                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       -  *                                  
    //  LEFT    *                                  
    //          *                                  
    //       -  *                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       v  x END                                
    //  
    //  Skeleton:
    //    1. Start at position 0
    //    2. Loop until first valid position
    //       a. Separate loop for Left, Top-left, and Top neighbor arrays
    //    3. If no valid samples found, write mid-range value (128 for 8-bit)
    //    4. Else, write the first valid sample into the invalid range
    //    5. Left Loop 
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    6. Top-left Sample (no loop)                            
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    7. Top Loop
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value

    // Pre-calculate bottom left availability per luma wise
    bottomLeftAvailabilityPreCalc = isBottomLeftAvailable(
        partitionDepth,
        cuIndex);


    // Pre-calculate top right availability
    topRightAvailabilityPreCalc = isUpperRightAvailable(
        partitionDepth,
        cuIndex);

    if (colorFormat == EB_YUV422 && !secondChroma) {
        bottomLeftAvailabilityPreCalc = EB_TRUE;
    }

    if (colorFormat == EB_YUV422 && secondChroma) {
        topRightAvailabilityPreCalc = EB_FALSE;
    }

    //*************************************************
    // Part 1: Initial Invalid Sample Loops
    //*************************************************
    
    // Left Block Loop
    blockIndex = 0;

    //chroma pel position
    reconArrayIndex = (cuOriginY >> subHeightCMinus1) + chromaOffset +
        2 * puChromaSize - MIN_PU_SIZE;

    neighborAvailable = EB_FALSE;
    while(blockIndex < leftBlockEnd && neighborAvailable == EB_FALSE) {
        //Jing: mode is for luma, reconArrayIndex is pel of chroma, needs to convert to luma axis to get the mode
        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex << subHeightCMinus1); //mode is stored as luma, so convert to luma axis

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            // Set pad value (end of block)
            cbPadValue = leftCbReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1)];
            crPadValue = leftCrReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1)];
        }
        else {
            ++blockIndex;
            reconArrayIndex -= MIN_PU_SIZE;
        }
    }

    // Top Left Block
    while(blockIndex < topLeftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            cuOriginX,
            cuOriginY + chromaOffset);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)   ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)               ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            int pos = ((MAX_PICTURE_HEIGHT_SIZE - (cuOriginY + chromaOffset)) >> subHeightCMinus1) + (cuOriginX >> subWidthCMinus1);
            cbPadValue = topLeftCbReconNeighborArray[pos];
            crPadValue = topLeftCrReconNeighborArray[pos];
            //crPadValue = topLeftCrReconNeighborArray[MAX_PICTURE_HEIGHT_SIZE + (cuOriginX >> subWidthCMinus1) - (cuOriginY + chromaOffset)];
        }
        else {
            ++blockIndex;
        }
    }

    // Top Block Loop
    reconArrayIndex = cuOriginX >> subWidthCMinus1;
    while(blockIndex < topBlockEnd && neighborAvailable == EB_FALSE) {
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex << subWidthCMinus1);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == INTER_MODE &&
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (beginning of block)
            cbPadValue = topCbReconNeighborArray[reconArrayIndex];
            crPadValue = topCrReconNeighborArray[reconArrayIndex];
        }
        else {
            ++blockIndex;
            reconArrayIndex += MIN_PU_SIZE;
        }
       
    }

    // Check for no valid border samples
    if (blockIndex == topBlockEnd && neighborAvailable == EB_FALSE) {
        writeCountChroma = 4 * puChromaSize + 1;

        // Write Midrange
        EB_MEMSET(cbWritePtr,   MIDRANGE_VALUE_8BIT, writeCountChroma);
        EB_MEMSET(crWritePtr,   MIDRANGE_VALUE_8BIT, writeCountChroma);
    } 
    else {
        // Write Pad Value - adjust for the TopLeft block being 1-sample         
        writeCountChroma = (blockIndex >= topLeftBlockEnd) ?
            ((blockIndex-1) * MIN_PU_SIZE) + 1 :
             (blockIndex    * MIN_PU_SIZE);

        EB_MEMSET(cbWritePtr, cbPadValue, writeCountChroma);
        EB_MEMSET(crWritePtr, crPadValue, writeCountChroma);
    }

    cbWritePtr += writeCountChroma;
    crWritePtr += writeCountChroma;

    //*************************************************
    // Part 2: Copy the remainder of the samples
    //*************************************************

    // Left Block Loop
    reconArrayIndex = (cuOriginY>>subHeightCMinus1) + chromaOffset + (2 * puChromaSize - MIN_PU_SIZE) - (blockIndex * MIN_PU_SIZE);

    while(blockIndex < leftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex << subHeightCMinus1);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Copy samples (Reverse the order)            
            cbWritePtr[0] = leftCbReconNeighborArray[(reconArrayIndex) + 3];
            cbWritePtr[1] = leftCbReconNeighborArray[(reconArrayIndex) + 2];
            cbWritePtr[2] = leftCbReconNeighborArray[(reconArrayIndex) + 1];
            cbWritePtr[3] = leftCbReconNeighborArray[(reconArrayIndex) + 0];

            crWritePtr[0] = leftCrReconNeighborArray[(reconArrayIndex) + 3];
            crWritePtr[1] = leftCrReconNeighborArray[(reconArrayIndex) + 2];
            crWritePtr[2] = leftCrReconNeighborArray[(reconArrayIndex) + 1];
            crWritePtr[3] = leftCrReconNeighborArray[(reconArrayIndex) + 0];

            // Set pad value (beginning of block)
            cbPadValue   = leftCbReconNeighborArray[reconArrayIndex];
            crPadValue   = leftCrReconNeighborArray[reconArrayIndex];
        }
        else {
            // Copy pad value
            EB_MEMSET(cbWritePtr,   cbPadValue,   MIN_PU_SIZE);
            EB_MEMSET(crWritePtr,   crPadValue,   MIN_PU_SIZE);
        }
        cbWritePtr   += MIN_PU_SIZE;
        crWritePtr   += MIN_PU_SIZE;

        ++blockIndex;
        reconArrayIndex -= MIN_PU_SIZE;
    }

    // Top Left Block
    while(blockIndex < topLeftBlockEnd) {
        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            cuOriginX,
            cuOriginY+chromaOffset);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            // Copy sample
            int pos = ((MAX_PICTURE_HEIGHT_SIZE - (cuOriginY + chromaOffset)) >> subHeightCMinus1) + (cuOriginX >> subWidthCMinus1);
            *cbWritePtr = topLeftCbReconNeighborArray[pos];
            *crWritePtr = topLeftCrReconNeighborArray[pos];

            // Set Pad Value
            cbPadValue = topLeftCbReconNeighborArray[pos];
            crPadValue = topLeftCrReconNeighborArray[pos];
        } else {
            // Copy pad value
            *cbWritePtr   = cbPadValue;
            *crWritePtr   = crPadValue;
        }
        ++cbWritePtr;
        ++crWritePtr;
        ++blockIndex;
    }

    // Top Block Loop
    reconArrayIndex = (cuOriginX >> subWidthCMinus1) + (blockIndex - topLeftBlockEnd) * MIN_PU_SIZE;

    while(blockIndex < topBlockEnd) {
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex << subWidthCMinus1);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == INTER_MODE &&
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
			EB_MEMCPY(cbWritePtr, &topCbReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);
			EB_MEMCPY(crWritePtr, &topCrReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);

            // Set pad value (end of block)
            cbPadValue = topCbReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
            crPadValue = topCrReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
        } else {
            // Copy pad value
            EB_MEMSET(cbWritePtr, cbPadValue, MIN_PU_SIZE);
            EB_MEMSET(crWritePtr, crPadValue, MIN_PU_SIZE);
        }
        cbWritePtr   += MIN_PU_SIZE;
        crWritePtr   += MIN_PU_SIZE;
        ++blockIndex;
        reconArrayIndex += MIN_PU_SIZE;
    }

    if (colorFormat == EB_YUV444) {
        //*************************************************
        // Part 2.5: Intra Filter Samples for ChromaArrayType==3
        //*************************************************
        sampleReadLocCb = cbBorder;
        sampleWriteLocCb = cbBorderFilt;
        sampleReadLocCr = crBorder;
        sampleWriteLocCr = crBorderFilt;
        // Loop is only over the non-edge samples (TotalCount - 2)
        yLoadCounter = (size << 2) - 1;

        // First Sample
        *sampleWriteLocCb++ = *sampleReadLocCb++;
        *sampleWriteLocCr++ = *sampleReadLocCr++;

        // Internal Filtered Samples
        do {
            *sampleWriteLocCb++ = (sampleReadLocCb[-1] + (sampleReadLocCb[0] << 1) + sampleReadLocCb[1] + 2) >> 2;
            *sampleWriteLocCr++ = (sampleReadLocCr[-1] + (sampleReadLocCr[0] << 1) + sampleReadLocCr[1] + 2) >> 2;
            ++sampleReadLocCb;
            ++sampleReadLocCr;
        } while(--yLoadCounter);

        // Last Sample
        *sampleWriteLocCb = *sampleReadLocCb;
        *sampleWriteLocCr = *sampleReadLocCr;
    }
    //*************************************************
    // Part 3: Create Reversed Reference Samples
    //*************************************************
    
    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagCb = EB_FALSE;
    intraRefPtr->AboveReadyFlagCr = EB_FALSE;

    intraRefPtr->LeftReadyFlagCb  = EB_FALSE;
    intraRefPtr->LeftReadyFlagCr  = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                     =======>    L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	EB_MEMCPY(cbBorderReverse + (puChromaSize<<1),  cbBorder + (puChromaSize<<1),  (puChromaSize<<1)+1);
	EB_MEMCPY(crBorderReverse + (puChromaSize<<1),  crBorder + (puChromaSize<<1),  (puChromaSize<<1)+1);

    sampleWriteLocCb = cbBorderReverse + (puChromaSize << 1) - 1 ;
    sampleWriteLocCr = crBorderReverse + (puChromaSize << 1) - 1 ;
     
    for(i = 0; i < (puChromaSize << 1) ;i++){
       *sampleWriteLocCb = cbBorder[i];
       *sampleWriteLocCr = crBorder[i];
        sampleWriteLocCb--;
        sampleWriteLocCr--;
    }

    if (colorFormat == EB_YUV444) {
        EB_MEMCPY(cbBorderFiltReverse + (puChromaSize<<1),  cbBorderFilt + (puChromaSize<<1), (puChromaSize<<1)+1);
        EB_MEMCPY(crBorderFiltReverse + (puChromaSize<<1),  crBorderFilt + (puChromaSize<<1), (puChromaSize<<1)+1);
        sampleWriteLocCbFilt = cbBorderFiltReverse + (puChromaSize << 1) - 1 ;
        sampleWriteLocCrFilt = crBorderFiltReverse + (puChromaSize << 1) - 1 ;
        for(i = 0; i < (puChromaSize << 1) ;i++){
            *sampleWriteLocCbFilt = cbBorderFilt[i];
            *sampleWriteLocCrFilt = crBorderFilt[i];
            sampleWriteLocCbFilt--;
            sampleWriteLocCrFilt--;
        }
    }

    return return_error;
}

/*******************************************
 * Generate Intra Reference Samples - 16 bit
 *******************************************/
EB_ERRORTYPE GenerateIntraReference16bitSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary)
{
    EB_ERRORTYPE          return_error = EB_ErrorNone;
    IntraReference16bitSamples_t       *intraRefPtr = (IntraReference16bitSamples_t*)refWrapperPtr;
    EB_U16                *yBorder                 = intraRefPtr->yIntraReferenceArray;
    EB_U16                *cbBorder                = intraRefPtr->cbIntraReferenceArray;
    EB_U16                *crBorder                = intraRefPtr->crIntraReferenceArray;
    EB_U16                *yBorderFilt             = intraRefPtr->yIntraFilteredReferenceArray;

    EB_U16                *yBorderReverse          = intraRefPtr->yIntraReferenceArrayReverse;
    EB_U16                *yBorderFiltReverse      = intraRefPtr->yIntraFilteredReferenceArrayReverse;
    EB_U16                *cbBorderReverse         = intraRefPtr->cbIntraReferenceArrayReverse;
    EB_U16                *crBorderReverse         = intraRefPtr->crIntraReferenceArrayReverse;
      
    const EB_U32          sizeLog2      = Log2f(size);
    const EB_U32          chromaRatio = (colorFormat==EB_YUV420 || colorFormat==EB_YUV422)?1:0;
    const EB_U32          puChromaSize    = size >> chromaRatio;

    EB_U32                yLoadCounter;
    EB_U16                *sampleReadLoc;
    EB_U16                *sampleWriteLoc;
    EB_U16                *sampleWriteLocCb;
    EB_U16                *sampleWriteLocCr;
    EB_U32                i;
    EB_U16                *sampleWriteLocFilt;
       
    // This internal LCU availability check will be performed for top right and bottom left neighbors only.
    // It is always set to true for top, left and top left neighbors
    EB_BOOL               bottomLeftAvailabilityPreCalc;
    EB_BOOL               topRightAvailabilityPreCalc;
    const EB_U32          cuIndex = ((originY & (lcuSize-1)) >> sizeLog2) * (1 << cuDepth) + ((originX & (lcuSize-1)) >> sizeLog2);

    EB_U32                blockIndex;           // 4x4 (Min PU size) granularity
    EB_BOOL               neighborAvailable;

    const EB_U32          bottomLeftEnd         =     (size >> LOG_MIN_PU_SIZE);
    const EB_U32          leftBlockEnd          = 2 * (size >> LOG_MIN_PU_SIZE);
    const EB_U32          topLeftBlockEnd       = 2 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32          topRightBlockBegin    = 3 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32          topBlockEnd           = 4 * (size >> LOG_MIN_PU_SIZE) + 1;
    

    EB_U32                reconArrayIndex;
    EB_U32                modeArrayIndex;
    
    EB_U16                 lumaPadValue  = 0;
    EB_U16                 cbPadValue    = 0;
    EB_U16                 crPadValue    = 0;

    EB_U16                *lumaWritePtr = yBorder;
    EB_U16                *cbWritePtr   = cbBorder;
    EB_U16                *crWritePtr   = crBorder;
    
    EB_U32                writeCountLuma;
    EB_U32                writeCountChroma;

    // Neighbor Arrays
    EB_U32                topModeNeighborArraySize      = modeTypeNeighborArray->topArraySize;
    EB_U8                *topModeNeighborArray          = modeTypeNeighborArray->topArray;
    EB_U32                leftModeNeighborArraySize     = modeTypeNeighborArray->leftArraySize;
    EB_U8                *leftModeNeighborArray         = modeTypeNeighborArray->leftArray;
    EB_U8                *topLeftModeNeighborArray      = modeTypeNeighborArray->topLeftArray;
  
    EB_U16                *topLumaReconNeighborArray     = (EB_U16*)lumaReconNeighborArray->topArray;
    EB_U16                *leftLumaReconNeighborArray    = (EB_U16*)lumaReconNeighborArray->leftArray;
    EB_U16                *topLeftLumaReconNeighborArray = (EB_U16*)lumaReconNeighborArray->topLeftArray;
    EB_U16                *topCbReconNeighborArray       = (EB_U16*)cbReconNeighborArray->topArray;
    EB_U16                *leftCbReconNeighborArray      = (EB_U16*)cbReconNeighborArray->leftArray;
    EB_U16                *topLeftCbReconNeighborArray   = (EB_U16*)cbReconNeighborArray->topLeftArray;
    EB_U16                *topCrReconNeighborArray       = (EB_U16*)crReconNeighborArray->topArray;
    EB_U16                *leftCrReconNeighborArray      = (EB_U16*)crReconNeighborArray->leftArray;
    EB_U16                *topLeftCrReconNeighborArray   = (EB_U16*)crReconNeighborArray->topLeftArray;

    // The Generate Intra Reference sample process is a single pass algorithm
    //   that runs through the neighbor arrays from the bottom left to top right
    //   and analyzes which samples are available via a spatial availability 
    //   check and various mode checks. Un-available samples at the beginning
    //   of the run (top-right side) are padded with the first valid sample and
    //   all other missing samples are padded with the last valid sample.
    //
    //   *  - valid sample
    //   x  - missing sample
    //   |  - sample used for padding
    //   <- - padding (copy) operation 
    //       
    //                              TOP
    //                                                          0
    //  TOP-LEFT                |------->       |--------------->                     
    //          * * * * * * * * * x x x x * * * * x x x x x x x x 
    //          *                                               
    //          *                                  
    //          *                                  
    //          *                                  
    //       ^  x                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       -  *                                  
    //  LEFT    *                                  
    //          *                                  
    //       -  *                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       v  x END                                
    //  
    //  Skeleton:
    //    1. Start at position 0
    //    2. Loop until first valid position
    //       a. Separate loop for Left, Top-left, and Top neighbor arrays
    //    3. If no valid samples found, write mid-range value (128 for 8-bit)
    //    4. Else, write the first valid sample into the invalid range
    //    5. Left Loop 
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    6. Top-left Sample (no loop)                            
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    7. Top Loop
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value

    // Pre-calculate bottom left availability
    bottomLeftAvailabilityPreCalc = isBottomLeftAvailable(
        cuDepth,
        cuIndex);

    // Pre-calculate top right availability
    topRightAvailabilityPreCalc = isUpperRightAvailable(
        cuDepth,
        cuIndex);

    //*************************************************
    // Part 1: Initial Invalid Sample Loops
    //*************************************************
    
    // Left Block Loop
    blockIndex = 0;
    reconArrayIndex = originY + 2 * size - MIN_PU_SIZE;
    neighborAvailable = EB_FALSE;
    while(blockIndex < leftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Set pad value (end of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
            cbPadValue   = leftCbReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> chromaRatio];
            crPadValue   = leftCrReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> chromaRatio];

        }
        else {
            ++blockIndex;
            reconArrayIndex -= MIN_PU_SIZE;
        }
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)   ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)          ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)        ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (end of block)
            lumaPadValue = topLeftLumaReconNeighborArray[reconArrayIndex];
            cbPadValue   = topLeftCbReconNeighborArray[reconArrayIndex >> chromaRatio];
            crPadValue   = topLeftCrReconNeighborArray[reconArrayIndex >> chromaRatio];
        }
        else {
            ++blockIndex;
        }
    }

    // Top Block Loop
    reconArrayIndex = originX;
    while(blockIndex < topBlockEnd && neighborAvailable == EB_FALSE) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8)  INVALID_MODE)  ? EB_FALSE :   // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (beginning of block)
            lumaPadValue = topLumaReconNeighborArray[reconArrayIndex];
            cbPadValue   = topCbReconNeighborArray[reconArrayIndex >> chromaRatio];
            crPadValue   = topCrReconNeighborArray[reconArrayIndex >> chromaRatio];
        }
        else {
            ++blockIndex;
            reconArrayIndex += MIN_PU_SIZE;
        }
       
    }

    // Check for no valid border samples
    if (blockIndex == topBlockEnd && neighborAvailable == EB_FALSE) {

        writeCountLuma   = 4*size + 1;
        writeCountChroma = 4*(size>>chromaRatio) + 1;

        // Write Midrange    
        memset16bit(lumaWritePtr, MIDRANGE_VALUE_10BIT, writeCountLuma);
        memset16bit(cbWritePtr,   MIDRANGE_VALUE_10BIT, writeCountChroma);
        memset16bit(crWritePtr,   MIDRANGE_VALUE_10BIT, writeCountChroma);
    } 
    else {
        
        // Write Pad Value - adjust for the TopLeft block being 1-sample 
        writeCountLuma = (blockIndex >= topLeftBlockEnd) ?
            (blockIndex-1) * MIN_PU_SIZE + 1 :
            blockIndex * MIN_PU_SIZE;
        
        writeCountChroma = (blockIndex >= topLeftBlockEnd) ?
            (((blockIndex-1) * MIN_PU_SIZE) >> chromaRatio) + 1 :
             ((blockIndex    * MIN_PU_SIZE) >> chromaRatio);
            
        memset16bit(lumaWritePtr, lumaPadValue, writeCountLuma);
        memset16bit(cbWritePtr,   cbPadValue,   writeCountChroma);
        memset16bit(crWritePtr,   crPadValue,   writeCountChroma);
    }

    lumaWritePtr += writeCountLuma;
    cbWritePtr   += writeCountChroma;
    crWritePtr   += writeCountChroma;

    //*************************************************
    // Part 2: Copy the remainder of the samples
    //*************************************************

    // Left Block Loop
    reconArrayIndex = originY + (2 * size - MIN_PU_SIZE) - (blockIndex * MIN_PU_SIZE);
    while(blockIndex < leftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Copy samples (Reverse the order)
            lumaWritePtr[0] = leftLumaReconNeighborArray[reconArrayIndex + 3];
            lumaWritePtr[1] = leftLumaReconNeighborArray[reconArrayIndex + 2];
            lumaWritePtr[2] = leftLumaReconNeighborArray[reconArrayIndex + 1];
            lumaWritePtr[3] = leftLumaReconNeighborArray[reconArrayIndex + 0];
            
            cbWritePtr[0] = leftCbReconNeighborArray[(reconArrayIndex >> chromaRatio) + 1];
            cbWritePtr[1] = leftCbReconNeighborArray[(reconArrayIndex >> chromaRatio) + 0];
            
            crWritePtr[0] = leftCrReconNeighborArray[(reconArrayIndex >> chromaRatio) + 1];
            crWritePtr[1] = leftCrReconNeighborArray[(reconArrayIndex >> chromaRatio) + 0];
            
            // Set pad value (beginning of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex];
            cbPadValue   = leftCbReconNeighborArray[reconArrayIndex >> chromaRatio];
            crPadValue   = leftCrReconNeighborArray[reconArrayIndex >> chromaRatio];
        }
        else {

            // Copy pad value
            memset16bit(lumaWritePtr, lumaPadValue, MIN_PU_SIZE);
            memset16bit(cbWritePtr,   cbPadValue,   MIN_PU_SIZE >> chromaRatio);
            memset16bit(crWritePtr,   crPadValue,   MIN_PU_SIZE >> chromaRatio);
        }

        lumaWritePtr += MIN_PU_SIZE;
        cbWritePtr   += MIN_PU_SIZE >> chromaRatio;
        crWritePtr   += MIN_PU_SIZE >> chromaRatio;

        ++blockIndex;
        reconArrayIndex -= MIN_PU_SIZE;
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :     // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :     // picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Copy sample
            *lumaWritePtr = topLeftLumaReconNeighborArray[reconArrayIndex];
            *cbWritePtr   = topLeftCbReconNeighborArray[reconArrayIndex >> chromaRatio];
            *crWritePtr   = topLeftCrReconNeighborArray[reconArrayIndex >> chromaRatio];

            // Set Pad Value
            lumaPadValue  = topLeftLumaReconNeighborArray[reconArrayIndex];
            cbPadValue    = topLeftCbReconNeighborArray[reconArrayIndex >> chromaRatio];
            crPadValue    = topLeftCrReconNeighborArray[reconArrayIndex >> chromaRatio];
        }
        else {
            
            // Copy pad value
            *lumaWritePtr = lumaPadValue;
            *cbWritePtr   = cbPadValue;
            *crWritePtr   = crPadValue;
        }

        ++lumaWritePtr;
        ++cbWritePtr;
        ++crWritePtr;

        ++blockIndex;
    }

    // Top Block Loop
	reconArrayIndex = originX + (blockIndex - topLeftBlockEnd) * MIN_PU_SIZE;
    while(blockIndex < topBlockEnd) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Copy samples in reverse order
            memcpy16bit(lumaWritePtr, &topLumaReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);
            memcpy16bit(cbWritePtr, &topCbReconNeighborArray[reconArrayIndex >> chromaRatio], MIN_PU_SIZE >> chromaRatio);
            memcpy16bit(crWritePtr, &topCrReconNeighborArray[reconArrayIndex >> chromaRatio], MIN_PU_SIZE >> chromaRatio);

            // Set pad value (end of block)
            lumaPadValue    = topLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
            cbPadValue      = topCbReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> chromaRatio];
            crPadValue      = topCrReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1) >> chromaRatio];
        }
        else {
            
            // Copy pad value
            memset16bit(lumaWritePtr,    lumaPadValue, MIN_PU_SIZE);
            memset16bit(cbWritePtr,      cbPadValue,   MIN_PU_SIZE >> chromaRatio);
            memset16bit(crWritePtr,      crPadValue,   MIN_PU_SIZE >> chromaRatio);
        }
    
        lumaWritePtr += MIN_PU_SIZE;
        cbWritePtr   += MIN_PU_SIZE >> chromaRatio;
        crWritePtr   += MIN_PU_SIZE >> chromaRatio;
    
        ++blockIndex;
        reconArrayIndex += MIN_PU_SIZE;
    }
    
    //*************************************************
    // Part 3: Strong Intra Filter Samples
    //*************************************************
    sampleReadLoc = yBorder;
    sampleWriteLoc = yBorderFilt;
    // Loop is only over the non-edge samples (TotalCount - 2)
    yLoadCounter = (size << 2) - 1;

    if(strongIntraSmoothingFlag == EB_TRUE) {
        EB_U32 filterShift;
        EB_U32 index;
        EB_U32 bottomLeftSample = sampleReadLoc[0];
        EB_U32 topLeftSample    = sampleReadLoc[(size << 1)];
        EB_U32 topRightSample   = sampleReadLoc[(size << 2)];
        EB_U32 twicePuSize      = (size << 1);
        //TODO CLEAN UP THIS MESS, this works only for 32x32 blocks
        EB_BOOL bilinearLeft = (ABS((EB_S32)bottomLeftSample + (EB_S32)topLeftSample  - 2 * sampleReadLoc[size])             < SMOOTHING_THRESHOLD_10BIT) ? EB_TRUE : EB_FALSE;
        EB_BOOL bilinearTop  = (ABS((EB_S32)topLeftSample    + (EB_S32)topRightSample - 2 * sampleReadLoc[size+(size << 1)]) < SMOOTHING_THRESHOLD_10BIT) ? EB_TRUE : EB_FALSE;

        if(size >= STRONG_INTRA_SMOOTHING_BLOCKSIZE && bilinearLeft && bilinearTop) {
            filterShift = smoothingFilterShift[Log2f(size)-2];
            sampleWriteLoc[0]           = sampleReadLoc[0];
            sampleWriteLoc[(size << 1)] = sampleReadLoc[(size << 1)];
            sampleWriteLoc[(size << 2)] = sampleReadLoc[(size << 2)];

            for(index = 1; index < twicePuSize; index++) {
                sampleWriteLoc[index]             = (EB_U16)(((twicePuSize - index) * bottomLeftSample + index * topLeftSample  + size) >> filterShift);
                sampleWriteLoc[twicePuSize+index] = (EB_U16)(((twicePuSize - index) * topLeftSample    + index * topRightSample + size) >> filterShift);
            }
        }
        else {
            // First Sample
            *sampleWriteLoc++ = *sampleReadLoc++;

            // Internal Filtered Samples
            do {
                *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
                ++sampleReadLoc;
            } while(--yLoadCounter);

            // Last Sample
            *sampleWriteLoc = *sampleReadLoc;
        }
    }
    else {

        // First Sample
        *sampleWriteLoc++ = *sampleReadLoc++;

        // Internal Filtered Samples
        do {
            *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
            ++sampleReadLoc;
        } while(--yLoadCounter);

        // Last Sample
        *sampleWriteLoc = *sampleReadLoc;
    }

    //*************************************************
    // Part 4: Create Reversed Reference Samples
    //*************************************************
    
    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagY  = EB_FALSE;
    intraRefPtr->AboveReadyFlagCb = EB_FALSE;
    intraRefPtr->AboveReadyFlagCr = EB_FALSE;
    
    intraRefPtr->LeftReadyFlagY   = EB_FALSE;
    intraRefPtr->LeftReadyFlagCb  = EB_FALSE;
    intraRefPtr->LeftReadyFlagCr  = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                     =======>    L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  
   
    //Luma
    memcpy16bit(yBorderReverse     + (size<<1),  yBorder     + (size<<1),  (size<<1)+1);
    memcpy16bit(yBorderFiltReverse + (size<<1),  yBorderFilt + (size<<1),  (size<<1)+1);

    sampleWriteLoc     = yBorderReverse      + (size<<1) - 1 ;
    sampleWriteLocFilt = yBorderFiltReverse  + (size<<1) - 1 ;
    for(i=0; i<(size<<1)   ;i++){
        
       *sampleWriteLoc     = yBorder[i];
       *sampleWriteLocFilt = yBorderFilt[i] ;
        sampleWriteLoc--;
        sampleWriteLocFilt--;
    }

    //Chroma    
    memcpy16bit(cbBorderReverse + (puChromaSize<<1),  cbBorder + (puChromaSize<<1),  (puChromaSize<<1)+1);
    memcpy16bit(crBorderReverse + (puChromaSize<<1),  crBorder + (puChromaSize<<1),  (puChromaSize<<1)+1);

    sampleWriteLocCb     = cbBorderReverse      + (puChromaSize<<1) - 1 ;
    sampleWriteLocCr     = crBorderReverse      + (puChromaSize<<1) - 1 ;
     
    for(i=0; i<(puChromaSize<<1)   ;i++){
        
       *sampleWriteLocCb     = cbBorder[i];
       *sampleWriteLocCr     = crBorder[i];
        sampleWriteLocCb--;
        sampleWriteLocCr--;
    }

    return return_error;
}

/*******************************************
 * Generate Luma Intra Reference Samples - 16 bit
 *******************************************/
EB_ERRORTYPE GenerateLumaIntraReference16bitSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    IntraReference16bitSamples_t *intraRefPtr = (IntraReference16bitSamples_t*)refWrapperPtr;
    EB_U16 *yBorder = intraRefPtr->yIntraReferenceArray;
    EB_U16 *yBorderFilt = intraRefPtr->yIntraFilteredReferenceArray;


    EB_U16 *yBorderReverse = intraRefPtr->yIntraReferenceArrayReverse;
    EB_U16 *yBorderFiltReverse = intraRefPtr->yIntraFilteredReferenceArrayReverse;

    const EB_U32          sizeLog2      = Log2f(size);

    EB_U32 yLoadCounter;
    EB_U16 *sampleReadLoc;
    EB_U16 *sampleWriteLoc;
    EB_U32 i;
    EB_U16 *sampleWriteLocFilt;
    // This internal LCU availability check will be performed for top right and bottom left neighbors only.
    // It is always set to true for top, left and top left neighbors
    EB_BOOL bottomLeftAvailabilityPreCalc;
    EB_BOOL topRightAvailabilityPreCalc;
    
    EB_U32 partitionDepth = (size == MIN_PU_SIZE) ? cuDepth + 1 : cuDepth;
    const EB_U32 cuIndex = ((originY & (lcuSize - 1)) >> sizeLog2) * (1 << partitionDepth) + ((originX & (lcuSize - 1)) >> sizeLog2);

    EB_U32 blockIndex;// 4x4 (Min PU size) granularity
    EB_BOOL neighborAvailable;

    const EB_U32 bottomLeftEnd = (size >> LOG_MIN_PU_SIZE);
    const EB_U32 leftBlockEnd = 2 * (size >> LOG_MIN_PU_SIZE);
    const EB_U32 topLeftBlockEnd = 2 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32 topRightBlockBegin = 3 * (size >> LOG_MIN_PU_SIZE) + 1;
    const EB_U32 topBlockEnd = 4 * (size >> LOG_MIN_PU_SIZE) + 1;
    

    EB_U32 reconArrayIndex;
    EB_U32 modeArrayIndex;
    
    EB_U16 lumaPadValue  = 0;
    EB_U16 *lumaWritePtr = yBorder;
    
    EB_U32 writeCountLuma;

    // Neighbor Arrays
    EB_U32 topModeNeighborArraySize = modeTypeNeighborArray->topArraySize;
    EB_U8 *topModeNeighborArray = modeTypeNeighborArray->topArray;
    EB_U32 leftModeNeighborArraySize = modeTypeNeighborArray->leftArraySize;
    EB_U8 *leftModeNeighborArray = modeTypeNeighborArray->leftArray;
    EB_U8 *topLeftModeNeighborArray = modeTypeNeighborArray->topLeftArray;
    EB_U16 *topLumaReconNeighborArray = (EB_U16*)lumaReconNeighborArray->topArray;
    EB_U16 *leftLumaReconNeighborArray = (EB_U16*)lumaReconNeighborArray->leftArray;
    EB_U16 *topLeftLumaReconNeighborArray = (EB_U16*)lumaReconNeighborArray->topLeftArray;

    (void) cbReconNeighborArray;
    (void) crReconNeighborArray;

    // The Generate Intra Reference sample process is a single pass algorithm
    //   that runs through the neighbor arrays from the bottom left to top right
    //   and analyzes which samples are available via a spatial availability 
    //   check and various mode checks. Un-available samples at the beginning
    //   of the run (top-right side) are padded with the first valid sample and
    //   all other missing samples are padded with the last valid sample.
    //
    //   *  - valid sample
    //   x  - missing sample
    //   |  - sample used for padding
    //   <- - padding (copy) operation 
    //       
    //                              TOP
    //                                                          0
    //  TOP-LEFT                |------->       |--------------->                     
    //          * * * * * * * * * x x x x * * * * x x x x x x x x 
    //          *                                               
    //          *                                  
    //          *                                  
    //          *                                  
    //       ^  x                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       -  *                                  
    //  LEFT    *                                  
    //          *                                  
    //       -  *                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       v  x END                                
    //  
    //  Skeleton:
    //    1. Start at position 0
    //    2. Loop until first valid position
    //       a. Separate loop for Left, Top-left, and Top neighbor arrays
    //    3. If no valid samples found, write mid-range value (128 for 8-bit)
    //    4. Else, write the first valid sample into the invalid range
    //    5. Left Loop 
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    6. Top-left Sample (no loop)                            
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    7. Top Loop
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value

    // Pre-calculate bottom left availability
    bottomLeftAvailabilityPreCalc = isBottomLeftAvailable(
        partitionDepth,
        cuIndex);

    // Pre-calculate top right availability
    topRightAvailabilityPreCalc = isUpperRightAvailable(
        partitionDepth,
        cuIndex);

    //*************************************************
    // Part 1: Initial Invalid Sample Loops
    //*************************************************
    
    // Left Block Loop
    blockIndex = 0;
    reconArrayIndex = originY + 2 * size - MIN_PU_SIZE;
    neighborAvailable = EB_FALSE;
    while(blockIndex < leftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Set pad value (end of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];

        }
        else {
            ++blockIndex;
            reconArrayIndex -= MIN_PU_SIZE;
        }
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd && neighborAvailable == EB_FALSE) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)               ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (end of block)
            lumaPadValue = topLeftLumaReconNeighborArray[reconArrayIndex];

        }
        else {
            ++blockIndex;
        }
    }

    // Top Block Loop
    reconArrayIndex = originX;
    while(blockIndex < topBlockEnd && neighborAvailable == EB_FALSE) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (beginning of block)
            lumaPadValue = topLumaReconNeighborArray[reconArrayIndex];
        }
        else {
            ++blockIndex;
            reconArrayIndex += MIN_PU_SIZE;
        }
       
    }

    // Check for no valid border samples
    if (blockIndex == topBlockEnd && neighborAvailable == EB_FALSE) {

        writeCountLuma   = 4*size + 1;

        // Write Midrange
        memset16bit(lumaWritePtr, MIDRANGE_VALUE_10BIT, writeCountLuma);
    } 
    else {
        
        // Write Pad Value - adjust for the TopLeft block being 1-sample 
        writeCountLuma = (blockIndex >= topLeftBlockEnd) ?
            (blockIndex-1) * MIN_PU_SIZE + 1 :
            blockIndex * MIN_PU_SIZE;
            
        memset16bit(lumaWritePtr, lumaPadValue, writeCountLuma);
    }

    lumaWritePtr += writeCountLuma;

    //*************************************************
    // Part 2: Copy the remainder of the samples
    //*************************************************

    // Left Block Loop
    reconArrayIndex = originY + (2 * size - MIN_PU_SIZE) - (blockIndex * MIN_PU_SIZE);
    while(blockIndex < leftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Copy samples (Reverse the order)
            lumaWritePtr[0] = leftLumaReconNeighborArray[reconArrayIndex + 3];
            lumaWritePtr[1] = leftLumaReconNeighborArray[reconArrayIndex + 2];
            lumaWritePtr[2] = leftLumaReconNeighborArray[reconArrayIndex + 1];
            lumaWritePtr[3] = leftLumaReconNeighborArray[reconArrayIndex + 0];
            
            // Set pad value (beginning of block)
            lumaPadValue = leftLumaReconNeighborArray[reconArrayIndex];
        }
        else {

            // Copy pad value
            memset16bit(lumaWritePtr, lumaPadValue, MIN_PU_SIZE);
        }

        lumaWritePtr += MIN_PU_SIZE;

        ++blockIndex;
        reconArrayIndex -= MIN_PU_SIZE;
    }

    // Top Left Block
    reconArrayIndex = MAX_PICTURE_HEIGHT_SIZE + originX - originY;
    while(blockIndex < topLeftBlockEnd) {

        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            originX,
            originY);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Copy sample
            *lumaWritePtr = topLeftLumaReconNeighborArray[reconArrayIndex];

            // Set Pad Value
            lumaPadValue  = topLeftLumaReconNeighborArray[reconArrayIndex];
        }
        else {
            
            // Copy pad value
            *lumaWritePtr = lumaPadValue;
        }

        ++lumaWritePtr;

        ++blockIndex;
    }

    // Top Block Loop
    reconArrayIndex = originX +  (blockIndex - topLeftBlockEnd)*MIN_PU_SIZE;

    while(blockIndex < topBlockEnd) {
    
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (topModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check
    
        if(neighborAvailable == EB_TRUE) {
            
            // Copy samples in reverse order
            memcpy16bit(lumaWritePtr, &topLumaReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);

            // Set pad value (end of block)
            lumaPadValue    = topLumaReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
    
        }
        else {
            // Copy pad value
            memset16bit(lumaWritePtr, lumaPadValue, MIN_PU_SIZE);
        }
    
        lumaWritePtr += MIN_PU_SIZE;
    
        ++blockIndex;
        reconArrayIndex += MIN_PU_SIZE;
    }
    
    //*************************************************
    // Part 3: Strong Intra Filter Samples
    //*************************************************
    sampleReadLoc = yBorder;
    sampleWriteLoc = yBorderFilt;
    // Loop is only over the non-edge samples (TotalCount - 2)
    yLoadCounter = (size << 2) - 1;

    if(strongIntraSmoothingFlag == EB_TRUE) {
        EB_U32 filterShift;
        EB_U32 index;
        EB_U32 bottomLeftSample = sampleReadLoc[0];
        EB_U32 topLeftSample    = sampleReadLoc[(size << 1)];
        EB_U32 topRightSample   = sampleReadLoc[(size << 2)];
        EB_U32 twicePuSize      = (size << 1);

        //TODO CLEAN UP THIS MESS, this works only for 32x32 blocks
        EB_BOOL bilinearLeft = (ABS((EB_S32)bottomLeftSample + (EB_S32)topLeftSample  - 2 * sampleReadLoc[size])             < SMOOTHING_THRESHOLD_10BIT) ? EB_TRUE : EB_FALSE;
        EB_BOOL bilinearTop  = (ABS((EB_S32)topLeftSample    + (EB_S32)topRightSample - 2 * sampleReadLoc[size+(size << 1)]) < SMOOTHING_THRESHOLD_10BIT) ? EB_TRUE : EB_FALSE;

        if(size >= STRONG_INTRA_SMOOTHING_BLOCKSIZE && bilinearLeft && bilinearTop) {
            filterShift = smoothingFilterShift[Log2f(size)-2];
            sampleWriteLoc[0] = sampleReadLoc[0];
            sampleWriteLoc[(size << 1)] = sampleReadLoc[(size << 1)];
            sampleWriteLoc[(size << 2)] = sampleReadLoc[(size << 2)];

            for(index = 1; index < twicePuSize; index++) {
                sampleWriteLoc[index] = (EB_U16)(((twicePuSize - index) * bottomLeftSample + index * topLeftSample  + size) >> filterShift);
                sampleWriteLoc[twicePuSize+index] = (EB_U16)(((twicePuSize - index) * topLeftSample + index * topRightSample + size) >> filterShift);
            }
        }
        else {
            // First Sample
            *sampleWriteLoc++ = *sampleReadLoc++;

            // Internal Filtered Samples
            do {
                *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
                ++sampleReadLoc;
            } while(--yLoadCounter);

            // Last Sample
            *sampleWriteLoc = *sampleReadLoc;
        }
    }
    else {

        // First Sample
        *sampleWriteLoc++ = *sampleReadLoc++;

        // Internal Filtered Samples
        do {
            *sampleWriteLoc++ = (sampleReadLoc[-1] + (sampleReadLoc[0] << 1) + sampleReadLoc[1] + 2) >> 2;
            ++sampleReadLoc;
        } while(--yLoadCounter);

        // Last Sample
        *sampleWriteLoc = *sampleReadLoc;
    }

    //*************************************************
    // Part 4: Create Reversed Reference Samples
    //*************************************************
    
    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagY  = EB_FALSE;
    intraRefPtr->LeftReadyFlagY   = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                     =======>    L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	memcpy16bit(yBorderReverse + (size<<1), yBorder + (size<<1), (size<<1)+1);
	memcpy16bit(yBorderFiltReverse + (size<<1), yBorderFilt + (size<<1), (size<<1)+1);

    sampleWriteLoc = yBorderReverse + (size<<1) - 1 ;
    sampleWriteLocFilt = yBorderFiltReverse + (size<<1) - 1 ;
    for(i=0; i<(size<<1) ;i++) {
       *sampleWriteLoc = yBorder[i];
       *sampleWriteLocFilt = yBorderFilt[i] ;
        sampleWriteLoc--;
        sampleWriteLocFilt--;
    }

    return return_error;
}

/*******************************************
 * Generate Chroma Intra Reference Samples - 16 bit
 *******************************************/
EB_ERRORTYPE GenerateChromaIntraReference16bitSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      cuOriginX,
    EB_U32                      cuOriginY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     secondChroma,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary)
{
    EB_ERRORTYPE          return_error = EB_ErrorNone;

    IntraReference16bitSamples_t       *intraRefPtr = (IntraReference16bitSamples_t*)refWrapperPtr;

    EB_U16 *cbBorder = intraRefPtr->cbIntraReferenceArray;
    EB_U16 *cbBorderFilt = intraRefPtr->cbIntraFilteredReferenceArray;
    EB_U16 *crBorder = intraRefPtr->crIntraReferenceArray;
    EB_U16 *crBorderFilt = intraRefPtr->crIntraFilteredReferenceArray;

    EB_U16 *cbBorderReverse = intraRefPtr->cbIntraReferenceArrayReverse;
    EB_U16 *cbBorderFiltReverse = intraRefPtr->cbIntraFilteredReferenceArrayReverse;
    EB_U16 *crBorderReverse = intraRefPtr->crIntraReferenceArrayReverse;
    EB_U16 *crBorderFiltReverse = intraRefPtr->crIntraFilteredReferenceArrayReverse;

      
    const EB_U32 sizeLog2 = Log2f(size);
    const EB_U32 puChromaSize = size >> ((colorFormat == EB_YUV420 || colorFormat == EB_YUV422) ? 1 : 0);
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    const EB_U32 chromaOffset = (colorFormat == EB_YUV422 && secondChroma) ? puChromaSize : 0;

    EB_U32 i;
    EB_U32 yLoadCounter;
    EB_U16 *sampleReadLocCb;
    EB_U16 *sampleWriteLocCb;
    EB_U16 *sampleReadLocCr;
    EB_U16 *sampleWriteLocCr;
    EB_U16 *sampleWriteLocCbFilt;
    EB_U16 *sampleWriteLocCrFilt;
       
    // This internal LCU availability check will be performed for top right and bottom left neighbors only.
    // It is always set to true for top, left and top left neighbors
    EB_BOOL bottomLeftAvailabilityPreCalc;
    EB_BOOL topRightAvailabilityPreCalc;

    const EB_U32 partitionDepth = (size == MIN_PU_SIZE) ? cuDepth + 1 : cuDepth;
    const EB_U32 cuIndex = ((cuOriginY & (lcuSize - 1)) >> sizeLog2) * (1 << partitionDepth) + ((cuOriginX & (lcuSize - 1)) >> sizeLog2);

    EB_U32 blockIndex;           // 4x4 (Min PU size) granularity
    EB_BOOL neighborAvailable;

    EB_U32 bottomLeftEnd = (puChromaSize >> LOG_MIN_PU_SIZE);
    EB_U32 leftBlockEnd = 2 * (puChromaSize >> LOG_MIN_PU_SIZE);
    EB_U32 topLeftBlockEnd = 2 * (puChromaSize >> LOG_MIN_PU_SIZE) + 1;
    EB_U32 topRightBlockBegin = 3 * (puChromaSize >> LOG_MIN_PU_SIZE) + 1;
    EB_U32 topBlockEnd = 4 * (puChromaSize >> LOG_MIN_PU_SIZE) + 1;
    
    EB_U32 reconArrayIndex;
    EB_U32 modeArrayIndex;

    EB_U16 cbPadValue = 0;
    EB_U16 crPadValue = 0;

    EB_U16 *cbWritePtr = cbBorder;
    EB_U16 *crWritePtr = crBorder;

    EB_U32 writeCountChroma;

    // Neighbor Arrays
    EB_U32 topModeNeighborArraySize      = modeTypeNeighborArray->topArraySize;
    EB_U8 *topModeNeighborArray          = modeTypeNeighborArray->topArray;
    EB_U32 leftModeNeighborArraySize     = modeTypeNeighborArray->leftArraySize;
    EB_U8 *leftModeNeighborArray         = modeTypeNeighborArray->leftArray;
    EB_U8 *topLeftModeNeighborArray      = modeTypeNeighborArray->topLeftArray;

    EB_U16 *topCbReconNeighborArray       = (EB_U16*)cbReconNeighborArray->topArray;
    EB_U16 *leftCbReconNeighborArray      = (EB_U16*)cbReconNeighborArray->leftArray;
    EB_U16 *topLeftCbReconNeighborArray   = (EB_U16*)cbReconNeighborArray->topLeftArray;
    EB_U16 *topCrReconNeighborArray       = (EB_U16*)crReconNeighborArray->topArray;
    EB_U16 *leftCrReconNeighborArray      = (EB_U16*)crReconNeighborArray->leftArray;
    EB_U16 *topLeftCrReconNeighborArray   = (EB_U16*)crReconNeighborArray->topLeftArray;

    (void) strongIntraSmoothingFlag;
    (void) lumaReconNeighborArray;

    // The Generate Intra Reference sample process is a single pass algorithm
    //   that runs through the neighbor arrays from the bottom left to top right
    //   and analyzes which samples are available via a spatial availability 
    //   check and various mode checks. Un-available samples at the beginning
    //   of the run (top-right side) are padded with the first valid sample and
    //   all other missing samples are padded with the last valid sample.
    //
    //   *  - valid sample
    //   x  - missing sample
    //   |  - sample used for padding
    //   <- - padding (copy) operation 
    //       
    //                              TOP
    //                                                          0
    //  TOP-LEFT                |------->       |--------------->                     
    //          * * * * * * * * * x x x x * * * * x x x x x x x x 
    //          *                                               
    //          *                                  
    //          *                                  
    //          *                                  
    //       ^  x                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       -  *                                  
    //  LEFT    *                                  
    //          *                                  
    //       -  *                                  
    //       |  x                                  
    //       |  x                                  
    //       |  x                                  
    //       v  x END                                
    //  
    //  Skeleton:
    //    1. Start at position 0
    //    2. Loop until first valid position
    //       a. Separate loop for Left, Top-left, and Top neighbor arrays
    //    3. If no valid samples found, write mid-range value (128 for 8-bit)
    //    4. Else, write the first valid sample into the invalid range
    //    5. Left Loop 
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    6. Top-left Sample (no loop)                            
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value
    //    7. Top Loop
    //       a. If block is valid, copy recon values & update pad value
    //       b. Else, copy pad value

    // Pre-calculate bottom left availability per luma wise
    bottomLeftAvailabilityPreCalc = isBottomLeftAvailable(
        partitionDepth,
        cuIndex);


    // Pre-calculate top right availability
    topRightAvailabilityPreCalc = isUpperRightAvailable(
        partitionDepth,
        cuIndex);

    if (colorFormat == EB_YUV422 && !secondChroma) {
        bottomLeftAvailabilityPreCalc = EB_TRUE;
    }

    if (colorFormat == EB_YUV422 && secondChroma) {
        topRightAvailabilityPreCalc = EB_FALSE;
    }

    //*************************************************
    // Part 1: Initial Invalid Sample Loops
    //*************************************************
    
    // Left Block Loop
    blockIndex = 0;

    //chroma pel position
    reconArrayIndex = (cuOriginY >> subHeightCMinus1) + chromaOffset +
                2 * puChromaSize - MIN_PU_SIZE;

    neighborAvailable = EB_FALSE;
    while(blockIndex < leftBlockEnd && neighborAvailable == EB_FALSE) {

        //Jing: mode is for luma, reconArrayIndex is pel of chroma, needs to convert to luma axis to get the mode
        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex<<subHeightCMinus1); //mode is stored as luma, so convert to luma axis

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {

            // Set pad value (end of block)
            cbPadValue   = leftCbReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1)];
            crPadValue   = leftCrReconNeighborArray[(reconArrayIndex + MIN_PU_SIZE - 1)];
        }
        else {
            ++blockIndex;
            reconArrayIndex -= MIN_PU_SIZE;
        }
    }

    // Top Left Block
    while(blockIndex < topLeftBlockEnd && neighborAvailable == EB_FALSE) {
        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            cuOriginX,
            cuOriginY + chromaOffset);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)               ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            // Set pad value (end of block)
            int pos = ((MAX_PICTURE_HEIGHT_SIZE - (cuOriginY + chromaOffset)) >> subHeightCMinus1) + (cuOriginX >> subWidthCMinus1);
            cbPadValue = topLeftCbReconNeighborArray[pos];
            crPadValue = topLeftCrReconNeighborArray[pos];
        } else {
            ++blockIndex;
        }
    }

    // Top Block Loop
    reconArrayIndex = cuOriginX >> subWidthCMinus1;
    while(blockIndex < topBlockEnd && neighborAvailable == EB_FALSE) {
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex<<subWidthCMinus1);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == INTER_MODE &&
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
            // Set pad value (beginning of block)
            cbPadValue   = topCbReconNeighborArray[reconArrayIndex];
            crPadValue   = topCrReconNeighborArray[reconArrayIndex];
            
        }
        else {
            ++blockIndex;
            reconArrayIndex += MIN_PU_SIZE;
        }
       
    }

    // Check for no valid border samples
    if (blockIndex == topBlockEnd && neighborAvailable == EB_FALSE) {

        writeCountChroma = 4*puChromaSize + 1;

        // Write Midrange
        memset16bit(cbWritePtr, MIDRANGE_VALUE_10BIT, writeCountChroma);
        memset16bit(crWritePtr, MIDRANGE_VALUE_10BIT, writeCountChroma);
    } 
    else {
        
        // Write Pad Value - adjust for the TopLeft block being 1-sample         
        writeCountChroma = (blockIndex >= topLeftBlockEnd) ?
            ((blockIndex-1) * MIN_PU_SIZE) + 1 :
             (blockIndex    * MIN_PU_SIZE);

        memset16bit(cbWritePtr, cbPadValue, writeCountChroma);
        memset16bit(crWritePtr, crPadValue, writeCountChroma);
    }

    cbWritePtr   += writeCountChroma;
    crWritePtr   += writeCountChroma;

    //*************************************************
    // Part 2: Copy the remainder of the samples
    //*************************************************

    // Left Block Loop
    reconArrayIndex = (cuOriginY >> subHeightCMinus1) + chromaOffset +
        (2 * puChromaSize - MIN_PU_SIZE) - (blockIndex * MIN_PU_SIZE);

    while(blockIndex < leftBlockEnd) {
        modeArrayIndex = GetNeighborArrayUnitLeftIndex(
            modeTypeNeighborArray,
            reconArrayIndex << subHeightCMinus1);

        neighborAvailable = 
            (modeArrayIndex >= leftModeNeighborArraySize)           ? EB_FALSE :            // array boundary check
            (bottomLeftAvailabilityPreCalc == EB_FALSE &&
             blockIndex < bottomLeftEnd)                            ? EB_FALSE :            // internal scan-order check
            (leftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE) ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE)                           ? EB_FALSE :            // left picture boundary check
            (leftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            // Copy samples (Reverse the order)            
            cbWritePtr[0] = leftCbReconNeighborArray[(reconArrayIndex) + 3];
            cbWritePtr[1] = leftCbReconNeighborArray[(reconArrayIndex) + 2];
            cbWritePtr[2] = leftCbReconNeighborArray[(reconArrayIndex) + 1];
            cbWritePtr[3] = leftCbReconNeighborArray[(reconArrayIndex) + 0];

            crWritePtr[0] = leftCrReconNeighborArray[(reconArrayIndex) + 3];
            crWritePtr[1] = leftCrReconNeighborArray[(reconArrayIndex) + 2];
            crWritePtr[2] = leftCrReconNeighborArray[(reconArrayIndex) + 1];
            crWritePtr[3] = leftCrReconNeighborArray[(reconArrayIndex) + 0];

            // Set pad value (beginning of block)
            cbPadValue   = leftCbReconNeighborArray[reconArrayIndex];
            crPadValue   = leftCrReconNeighborArray[reconArrayIndex];
        }
        else {
            // Copy pad value
            memset16bit(cbWritePtr,   cbPadValue,   MIN_PU_SIZE);
            memset16bit(crWritePtr,   crPadValue,   MIN_PU_SIZE);
        }

        cbWritePtr   += MIN_PU_SIZE;
        crWritePtr   += MIN_PU_SIZE;

        ++blockIndex;
        reconArrayIndex -= MIN_PU_SIZE;
    }

    // Top Left Block
    while(blockIndex < topLeftBlockEnd) {
        modeArrayIndex = GetNeighborArrayUnitTopLeftIndex(
            modeTypeNeighborArray,
            cuOriginX,
            cuOriginY + chromaOffset);

        neighborAvailable = 
            (topLeftModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE)         ? EB_FALSE :    // left picture boundary check
            (topLeftModeNeighborArray[modeArrayIndex] == INTER_MODE && 
             constrainedIntraFlag                     == EB_TRUE)       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            // Copy sample
            int pos = ((MAX_PICTURE_HEIGHT_SIZE - (cuOriginY + chromaOffset)) >> subHeightCMinus1) + (cuOriginX >> subWidthCMinus1);
            *cbWritePtr = topLeftCbReconNeighborArray[pos];
            *crWritePtr = topLeftCrReconNeighborArray[pos];

            // Set Pad Value
            cbPadValue = topLeftCbReconNeighborArray[pos];
            crPadValue = topLeftCrReconNeighborArray[pos];
        } else {
            // Copy pad value
            *cbWritePtr = cbPadValue;
            *crWritePtr = crPadValue;
        }

        ++cbWritePtr;
        ++crWritePtr;
        ++blockIndex;
    }

    // Top Block Loop
    reconArrayIndex = (cuOriginX >> subWidthCMinus1) +
        (blockIndex - topLeftBlockEnd) * MIN_PU_SIZE;

    while(blockIndex < topBlockEnd) {
        modeArrayIndex = GetNeighborArrayUnitTopIndex(
            modeTypeNeighborArray,
            reconArrayIndex<<subWidthCMinus1);
    
        neighborAvailable = 
            (modeArrayIndex >= topModeNeighborArraySize)            ? EB_FALSE :            // array boundary check
            (topRightAvailabilityPreCalc == EB_FALSE &&
             blockIndex >= topRightBlockBegin)                      ? EB_FALSE :            // internal scan-order check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == (EB_U8) INVALID_MODE)  ? EB_FALSE :    // slice boundary check
            (pictureTopBoundary == EB_TRUE)                            ? EB_FALSE :            // top picture boundary check
            (pictureRightBoundary == EB_TRUE && blockIndex >= topRightBlockBegin) ? EB_FALSE :  // right picture boundary check
            (!secondChroma && topModeNeighborArray[modeArrayIndex] == INTER_MODE &&
             constrainedIntraFlag == EB_TRUE)                       ? EB_FALSE : EB_TRUE;   // contrained intra check

        if(neighborAvailable == EB_TRUE) {
            
			memcpy16bit(cbWritePtr, &topCbReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);
			memcpy16bit(crWritePtr, &topCrReconNeighborArray[reconArrayIndex], MIN_PU_SIZE);

            // Set pad value (end of block)
            cbPadValue      = topCbReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
            crPadValue      = topCrReconNeighborArray[reconArrayIndex + MIN_PU_SIZE - 1];
        }
        else {
            
            // Copy pad value
            memset16bit(cbWritePtr,      cbPadValue,   MIN_PU_SIZE);
            memset16bit(crWritePtr,      crPadValue,   MIN_PU_SIZE);
        }

        cbWritePtr   += MIN_PU_SIZE;
        crWritePtr   += MIN_PU_SIZE;
    
        ++blockIndex;
        reconArrayIndex += MIN_PU_SIZE;
    }

    if (colorFormat == EB_YUV444) {
        //*************************************************
        // Part 2.5: Intra Filter Samples for ChromaArrayType==3
        //*************************************************
        sampleReadLocCb = cbBorder;
        sampleWriteLocCb = cbBorderFilt;
        sampleReadLocCr = crBorder;
        sampleWriteLocCr = crBorderFilt;
        // Loop is only over the non-edge samples (TotalCount - 2)
        yLoadCounter = (size << 2) - 1;

        // First Sample
        *sampleWriteLocCb++ = *sampleReadLocCb++;
        *sampleWriteLocCr++ = *sampleReadLocCr++;

        // Internal Filtered Samples
        do {
            *sampleWriteLocCb++ = (sampleReadLocCb[-1] + (sampleReadLocCb[0] << 1) + sampleReadLocCb[1] + 2) >> 2;
            *sampleWriteLocCr++ = (sampleReadLocCr[-1] + (sampleReadLocCr[0] << 1) + sampleReadLocCr[1] + 2) >> 2;
            ++sampleReadLocCb;
            ++sampleReadLocCr;
        } while(--yLoadCounter);

        // Last Sample
        *sampleWriteLocCb = *sampleReadLocCb;
        *sampleWriteLocCr = *sampleReadLocCr;
    }

    //*************************************************
    // Part 3: Create Reversed Reference Samples
    //*************************************************
    
    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagCb = EB_FALSE;
    intraRefPtr->AboveReadyFlagCr = EB_FALSE;

    intraRefPtr->LeftReadyFlagCb  = EB_FALSE;
    intraRefPtr->LeftReadyFlagCr  = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                     =======>    L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	memcpy16bit(cbBorderReverse + (puChromaSize << 1), cbBorder + (puChromaSize << 1), (puChromaSize << 1) + 1);
	memcpy16bit(crBorderReverse + (puChromaSize << 1), crBorder + (puChromaSize << 1), (puChromaSize << 1) + 1);

    sampleWriteLocCb = cbBorderReverse + (puChromaSize<<1) - 1 ;
    sampleWriteLocCr = crBorderReverse + (puChromaSize<<1) - 1 ;
     
    for(i=0; i<(puChromaSize<<1); i++){
       *sampleWriteLocCb = cbBorder[i];
       *sampleWriteLocCr = crBorder[i];
        sampleWriteLocCb--;
        sampleWriteLocCr--;
    }

    if (colorFormat == EB_YUV444) {
        memcpy16bit(cbBorderFiltReverse + (puChromaSize<<1),
                cbBorderFilt + (puChromaSize << 1), (puChromaSize << 1) + 1);
        memcpy16bit(crBorderFiltReverse + (puChromaSize<<1),
                crBorderFilt + (puChromaSize << 1), (puChromaSize << 1) + 1);

        sampleWriteLocCbFilt = cbBorderFiltReverse + (puChromaSize << 1) - 1 ;
        sampleWriteLocCrFilt = crBorderFiltReverse + (puChromaSize << 1) - 1 ;

        for(i = 0; i < (puChromaSize << 1) ;i++){
            *sampleWriteLocCbFilt = cbBorderFilt[i];
            *sampleWriteLocCrFilt = crBorderFilt[i];
            sampleWriteLocCbFilt--;
            sampleWriteLocCrFilt--;
        }
    }


    return return_error;
}


static void IntraModeAngular_27To33(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride)     //input parameter, denotes the stride for the prediction ptr
{
    EB_U8           *refSampMain;
    EB_S32           intraPredAngle = intraModeAngularTable[mode - INTRA_VERTICAL_MODE];
    refSampMain    = refSamples + (size << 1);

	IntraAngVertical_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}
static void IntraModeAngular16bit_27To33(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U16            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride)     //input parameter, denotes the stride for the prediction ptr
{
    EB_U16           *refSampMain;
    EB_S32           intraPredAngle = intraModeAngularTable[mode - INTRA_VERTICAL_MODE];
    refSampMain    = refSamples + (size << 1);
    
    IntraAngVertical_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}

/** IntraModeAngular_19To25()
        is used to compute the prediction for Intra angular modes 19-25
 */
static void IntraModeAngular_19To25(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    EB_U8            *refAbove,
    EB_BOOL          *AboveReadyFlag
    )    
{
    EB_U8        *refSampMain;
    EB_U8        *refSampSide;
    const EB_U32  numberOfSamples = size  + 1;
    EB_S32        signIndex;
    const EB_U32  refOffset = (size << 1);
    EB_S32        intraPredAngle = 0;
    EB_U32        invAngle       = 0;
    EB_U32        invAngleSum    = 128;       // rounding used for (shift by 8)
    EB_S32        idx;
    EB_U32        index;
    
	if (INTRA_VERTICAL_MODE - mode < 9 ) { // check for index range, has to be less than size of array
		intraPredAngle	= intraModeAngularTableNegative[INTRA_VERTICAL_MODE - mode];
		invAngle		= invIntraModeAngularTable[INTRA_VERTICAL_MODE - mode];
	}
	
    //We just need to copy above Reference pixels only for ONE TIME for all modes of this group
    //where Filtered or non-Filtered are always used (8x8,32x32)
    if( (*AboveReadyFlag == EB_FALSE) || (size==16) ){

        *AboveReadyFlag = EB_TRUE;
        // Copy above reference samples (inc top left)
        for(index = 0; index < numberOfSamples; index++) {
            refAbove[index+size-1] = refSamples[refOffset + index];
        }
    }  
   
    refSampMain  = refAbove + (size - 1);

    // Extend the Main reference to the left for angles with negative slope
    refSampSide = refSamples + (size << 1);
    for(signIndex = -1; signIndex > (EB_S32)((EB_S32)size*intraPredAngle >> 5); --signIndex) {
        invAngleSum += invAngle;
        idx = (-((EB_S32)invAngleSum >> 8));
        refSampMain[signIndex] = refSampSide[idx];
    }

   
	IntraAngVertical_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}
/** IntraModeAngular16bit_19To25()
        is used to compute the prediction for Intra angular modes 19-25
 */
static void IntraModeAngular16bit_19To25(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U16            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    EB_U16            *refAbove,
    EB_BOOL          *AboveReadyFlag
    )    
{
    EB_U16        *refSampMain;
    EB_U16        *refSampSide;
    const EB_U32  numberOfSamples = size  + 1;
    EB_S32        signIndex;
    const EB_U32  refOffset = (size << 1);
    EB_S32        intraPredAngle = 0;
    EB_U32        invAngle       = 0;
    EB_U32        invAngleSum    = 128;       // rounding used for (shift by 8)
    EB_S32        idx;
    EB_U32        index;
	
    if (INTRA_VERTICAL_MODE - mode < 9) { // check for index range, has to be less than size of array
		intraPredAngle = intraModeAngularTableNegative[INTRA_VERTICAL_MODE - mode];
		invAngle = invIntraModeAngularTable[INTRA_VERTICAL_MODE - mode];
	}
    
    //We just need to copy above Reference pixels only for ONE TIME for all modes of this group
    //where Filtered or non-Filtered are always used (8x8,32x32)
    if( (*AboveReadyFlag == EB_FALSE) || (size==16) ){

        *AboveReadyFlag = EB_TRUE;
        // Copy above reference samples (inc top left)
        for(index = 0; index < numberOfSamples; index++) {
            refAbove[index+size-1] = refSamples[refOffset + index];
        }
    }  
   
    refSampMain  = refAbove + (size - 1);

    // Extend the Main reference to the left for angles with negative slope
    refSampSide = refSamples + (size << 1);
    for(signIndex = -1; signIndex > (EB_S32)((EB_S32)size*intraPredAngle >> 5); --signIndex) {
        invAngleSum += invAngle;
        idx = (-((EB_S32)invAngleSum >> 8));
        refSampMain[signIndex] = refSampSide[idx];
    }

   
	IntraAngVertical_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}


 /** IntraModeAngular_11To17()
        is used to compute the prediction for Intra angular modes 11-17
 */
static void IntraModeAngular_11To17(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    EB_U8            *refLeft,
    EB_BOOL          *LeftReadyFlag)     
{
    EB_U8          *refSampMain;
    EB_U8          *refSampSide;
    const EB_U32    numberOfSamples = size + 1;
    EB_S32          signIndex;
    const EB_U32    refOffset = (size << 1);
    EB_U32          index;
   
     EB_S32          intraPredAngle = intraModeAngularTableNegative[ mode - INTRA_HORIZONTAL_MODE];
     EB_U32          invAngle       = invIntraModeAngularTable[mode - INTRA_HORIZONTAL_MODE];
     EB_U32          invAngleSum    = 128;       // rounding used for (shift by 8)

    //We just need to copy above Reference pixels only for ONE TIME for all modes of this group
    //where Filtered or non-Filtered are always used (8x8,32x32)
     if( (*LeftReadyFlag == EB_FALSE) || (size==16)){

        *LeftReadyFlag = EB_TRUE;
        // Copy left reference samples (inc top left)(DO we really need all the data including topright??)
        for(index = 0; index < numberOfSamples; index++) {
            refLeft[index+size-1] = refSamples[refOffset - index];
        }  
     }
   
    refSampMain =  refLeft + (size - 1);

    // Extend the Main reference to the left for angles with negative slope  
    refSampSide = refSamples + (size << 1);

    for(signIndex = -1; signIndex > (EB_S32)((EB_S32)size*intraPredAngle >> 5); --signIndex) {
        invAngleSum += invAngle;
        refSampMain[signIndex] = refSampSide[invAngleSum >> 8];
    } 

  
	IntraAngHorizontal_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}
/** IntraModeAngular16bit_11To17()
        is used to compute the prediction for Intra angular modes 11-17
 */
static void IntraModeAngular16bit_11To17(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U16            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    EB_U16            *refLeft,
    EB_BOOL          *LeftReadyFlag)     
{
    EB_U16         *refSampMain;
    EB_U16          *refSampSide;
    const EB_U32    numberOfSamples = size + 1;
    EB_S32          signIndex;
    const EB_U32    refOffset = (size << 1);
    EB_U32          index;
   
    EB_S32          intraPredAngle = intraModeAngularTableNegative[ mode - INTRA_HORIZONTAL_MODE];
    EB_U32          invAngle       = invIntraModeAngularTable[mode - INTRA_HORIZONTAL_MODE];
    EB_U32          invAngleSum    = 128;       // rounding used for (shift by 8)

    //We just need to copy above Reference pixels only for ONE TIME for all modes of this group
    //where Filtered or non-Filtered are always used (8x8,32x32)
     if( (*LeftReadyFlag == EB_FALSE) || (size==16)){

        *LeftReadyFlag = EB_TRUE;
        // Copy left reference samples (inc top left)(DO we really need all the data including topright??)
        for(index = 0; index < numberOfSamples; index++) {
            refLeft[index+size-1] = refSamples[refOffset - index];
        }  
     }
   
    refSampMain =  refLeft + (size - 1);

    // Extend the Main reference to the left for angles with negative slope  
    refSampSide = refSamples + (size << 1);

    for(signIndex = -1; signIndex > (EB_S32)((EB_S32)size*intraPredAngle >> 5); --signIndex) {
        invAngleSum += invAngle;
        refSampMain[signIndex] = refSampSide[invAngleSum >> 8];
    } 

  
   IntraAngHorizontal_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}

 /** IntraModeAngular_3To9()
        is used to compute the prediction for Intra angular modes 3-9
 */
static void IntraModeAngular_3To9(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride)     //input parameter, denotes the stride for the prediction ptr
{
    EB_U8        *refSampMain;
   
    EB_S32        intraPredAngle = (INTRA_HORIZONTAL_MODE - mode) < 9 ? intraModeAngularTable[INTRA_HORIZONTAL_MODE - mode]:0;
    
    refSampMain = refSamples-1;
      
	IntraAngHorizontal_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}
/** IntraModeAngular16bit_3To9()
        is used to compute the prediction for Intra angular modes 3-9
 */
static void IntraModeAngular16bit_3To9(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U16           *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride)     //input parameter, denotes the stride for the prediction ptr
{
    EB_U16        *refSampMain;
   
    EB_S32        intraPredAngle = (INTRA_HORIZONTAL_MODE - mode) < 9 ? intraModeAngularTable[INTRA_HORIZONTAL_MODE - mode] : 0;
    
    refSampMain = refSamples-1;

      
	IntraAngHorizontal_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
        size, 
        refSampMain,
        predictionPtr,              
        predictionBufferStride,
        EB_FALSE,
        intraPredAngle);

    return;
}


/** IntraModeAngular_all()
        is the main function to compute intra  angular prediction for a PU
 */
static inline void IntraModeAngular_all(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      puSize,                     //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *refSamplesReverse,          //input parameter, pointer to the reference samples,Left in reverse order
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    EB_U8            *refAbove,
    EB_BOOL          *AboveReadyFlag,
    EB_U8            *refLeft,
    EB_BOOL          *LeftReadyFlag)
{

           
    switch(mode){
        case 34:

            IntraAng34_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                EB_FALSE);

            break;
        case 33: case 32: case 31: case 30: case 29: case 28: case 27:
            IntraModeAngular_27To33(               
                mode,
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride);
            break;
        case 25: case 24: case 23: case 22: case 21: case 20: case 19:                             
            IntraModeAngular_19To25(              
                mode,
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                refAbove,
                AboveReadyFlag);
            break;
        case 18:
            IntraAng18_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                EB_FALSE);
            break;
        case 17: case 16: case 15: case 14: case 13: case 12: case 11:
            IntraModeAngular_11To17(               
                mode,
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                refLeft,
                LeftReadyFlag);
            break;
        case 9: case 8: case 7: case 6: case 5: case 4: case 3:
            IntraModeAngular_3To9(                
                mode,
                puSize,
                refSamplesReverse,
                predictionPtr,
                predictionBufferStride);
            break;
        case 2:
            
            IntraAng2_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                refSamplesReverse,
                predictionPtr,
                predictionBufferStride,
                EB_FALSE);
            break;
        }
}
/** IntraModeAngular_all()
        is the main function to compute intra  angular prediction for a PU
 */
static inline void IntraModeAngular16bit_all(
    EB_U32            mode,                       //input parameter, indicates the Intra luma mode
    const EB_U32      puSize,                     //input parameter, denotes the size of the current PU
    EB_U16          *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16            *refSamplesReverse,          //input parameter, pointer to the reference samples,Left in reverse order
    EB_U16            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    EB_U16            *refAbove,
    EB_BOOL          *AboveReadyFlag,
    EB_U16            *refLeft,
    EB_BOOL          *LeftReadyFlag)
{

           
    switch(mode){
        case 34:

            IntraAng34_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                EB_FALSE);

            break;
        case 33: case 32: case 31: case 30: case 29: case 28: case 27:
            IntraModeAngular16bit_27To33(
                mode,
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride);
            break;
        case 25: case 24: case 23: case 22: case 21: case 20: case 19:           
            IntraModeAngular16bit_19To25(  
                mode,
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                refAbove,
                AboveReadyFlag);
            break;
        case 18:
            IntraAng18_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                EB_FALSE);
            break;
        case 17: case 16: case 15: case 14: case 13: case 12: case 11:
            IntraModeAngular16bit_11To17(
                mode,
                puSize,
                refSamples,
                predictionPtr,
                predictionBufferStride,
                refLeft,
                LeftReadyFlag);
            break;
        case 9: case 8: case 7: case 6: case 5: case 4: case 3:
            IntraModeAngular16bit_3To9(   
                mode,
                puSize,
                refSamplesReverse,
                predictionPtr,
                predictionBufferStride);
            break;
        case 2:
            
            IntraAng2_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                refSamplesReverse,
                predictionPtr,
                predictionBufferStride,
                EB_FALSE);
            break;
        }
}
/** IntraPrediction()
        is the main function to compute intra prediction for a PU
 */
EB_ERRORTYPE IntraPredictionCl(
	ModeDecisionContext_t                  *mdContextPtr,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t		   *candidateBufferPtr)
{
    EB_ERRORTYPE return_error    = EB_ErrorNone;
    const EB_U32 lumaMode        = candidateBufferPtr->candidatePtr->intraLumaMode;
    EB_U32                      chromaMode;
	const EB_U32 puOriginX = mdContextPtr->cuOriginX;
	const EB_U32 puOriginY = mdContextPtr->cuOriginY;
	const EB_U32 puWidth = mdContextPtr->cuStats->size;
	const EB_U32 puHeight = mdContextPtr->cuStats->size;


	IntraReferenceSamples_t * const contextPtr = (IntraReferenceSamples_t*)(mdContextPtr->intraRefPtr);
	const EncodeContext_t * const encodeContextPtr   =   ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    // Map the mode to the function table index
    EB_U32 funcIndex =
        (lumaMode < 2)?                     lumaMode  :
        (lumaMode == INTRA_VERTICAL_MODE)?          2 :
        (lumaMode == INTRA_HORIZONTAL_MODE)?        3 :
        4;

    EB_U32 puOriginIndex;
    EB_U32 puChromaOriginIndex;
    EB_U32 puSize;
    EB_U32 chromaPuSize;

    EB_S32 diffModeA;
    EB_S32 diffModeB;
    EB_S32 diffMode;

    EB_U8 *yIntraReferenceArray;
    EB_U8 *yIntraReferenceArrayReverse;

    (void) pictureControlSetPtr;


    CHECK_REPORT_ERROR(
		(puWidth == puHeight),
		encodeContextPtr->appCallbackPtr, 
		EB_ENC_INTRA_PRED_ERROR2);


	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

		if (mdContextPtr->lumaIntraRefSamplesGenDone == EB_FALSE)
		{
			EbPictureBufferDesc_t     *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

			GenerateIntraLumaReferenceSamplesMd(
				mdContextPtr,
				inputPicturePtr);
		}
        puOriginIndex = ((puOriginY & (63)) * 64) + (puOriginX & (63));
        puSize        = puWidth;

        diffModeA = EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_HORIZONTAL_MODE);
        diffModeB = EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_VERTICAL_MODE);
        diffMode = MIN(diffModeA, diffModeB);

        contextPtr->AboveReadyFlagY  = EB_FALSE;
        contextPtr->LeftReadyFlagY   = EB_FALSE;

        switch(funcIndex) {

        case 0:

           yIntraReferenceArray =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                    contextPtr->yIntraReferenceArrayReverse;

		   IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);

            break;

        case 1:

            yIntraReferenceArray = contextPtr->yIntraReferenceArrayReverse;
      
            IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);

            break;

        case 2:
        
            yIntraReferenceArray =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                    contextPtr->yIntraReferenceArrayReverse;
              
       
            IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);
            break;

        case 3:

            yIntraReferenceArray =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                     contextPtr->yIntraReferenceArrayReverse;

            IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);

            break;
         
        case 4:
          
             yIntraReferenceArray        =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArray :
                                            contextPtr->yIntraReferenceArray;
             yIntraReferenceArrayReverse =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                            contextPtr->yIntraReferenceArrayReverse;

            IntraModeAngular_all(           
                lumaMode,
                puSize,
                yIntraReferenceArray,
                yIntraReferenceArrayReverse,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                contextPtr->ReferenceAboveLineY,
                & contextPtr->AboveReadyFlagY,
                contextPtr->ReferenceLeftLineY,
                & contextPtr->LeftReadyFlagY);

            break;

        default:
            break;
        }
    }    

    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

		if (mdContextPtr->chromaIntraRefSamplesGenDone == EB_FALSE)
		{

			EbPictureBufferDesc_t *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;
			GenerateIntraChromaReferenceSamplesMd(
				mdContextPtr,
				inputPicturePtr);

		}

        // The chromaMode is always DM
        chromaMode = (EB_U32) lumaMode;
        puChromaOriginIndex = (((puOriginY & (63)) * 32) + (puOriginX & (63)))>>1;
        chromaPuSize        = puWidth >> 1;

        contextPtr->AboveReadyFlagCb = EB_FALSE;
        contextPtr->AboveReadyFlagCr = EB_FALSE;
        contextPtr->LeftReadyFlagCb  = EB_FALSE;
        contextPtr->LeftReadyFlagCr  = EB_FALSE;

        switch(funcIndex) {

        case 0:
                
             // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }
      
            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 2:
              
            // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
                IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArray,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }
       
            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArray,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 3:
 
            // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }

            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 1:
               
            // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }
      
            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 4:

            // Cb Intra Prediction   
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
                IntraModeAngular_all(
                    chromaMode,
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArray,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    contextPtr->ReferenceAboveLineCb,
                    & contextPtr->AboveReadyFlagCb,
                    contextPtr->ReferenceLeftLineCb,
                    & contextPtr->LeftReadyFlagCb);
            }

            // Cr Intra Prediction     
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
                IntraModeAngular_all(
                    chromaMode,
                    chromaPuSize,
                    contextPtr->crIntraReferenceArray,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    contextPtr->ReferenceAboveLineCr,
                    & contextPtr->AboveReadyFlagCr,
                    contextPtr->ReferenceLeftLineCr,
                    & contextPtr->LeftReadyFlagCr);
            }

            break;

        default:
            break;
        }
    }

    return return_error;
}

EB_ERRORTYPE Intra4x4IntraPredictionCl(
    EB_U32                                  puIndex,
    EB_U32                                  puOriginX,
    EB_U32                                  puOriginY,
    EB_U32                                  puWidth,
    EB_U32                                  puHeight,
    EB_U32                                  lcuSize,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
    EB_PTR                                  predictionContextPtr)
{
    EB_ERRORTYPE                return_error    = EB_ErrorNone;
    EB_U32          lumaMode        = candidateBufferPtr->candidatePtr->intraLumaMode;
    EB_U32        chromaMode;

    IntraReferenceSamples_t *contextPtr         =   (IntraReferenceSamples_t*)(((ModeDecisionContext_t*)predictionContextPtr)->intraRefPtr);
	EncodeContext_t         *encodeContextPtr   =   ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    // Map the mode to the function table index
    EB_U32 funcIndex =
        (lumaMode < 2)?                     lumaMode  :
        (lumaMode == INTRA_VERTICAL_MODE)?          2 :
        (lumaMode == INTRA_HORIZONTAL_MODE)?        3 :
        4;

    EB_U32 puOriginIndex;
    EB_U32 puChromaOriginIndex;
    EB_U32 puSize;
    EB_U32 chromaPuSize;

    EB_S32 diffModeA;
    EB_S32 diffModeB;
    EB_S32 diffMode;

    EB_U8 *yIntraReferenceArray;
    EB_U8 *yIntraReferenceArrayReverse;

    (void) puIndex;
    (void)pictureControlSetPtr;

    CHECK_REPORT_ERROR(
		(puWidth == puHeight),
		encodeContextPtr->appCallbackPtr, 
		EB_ENC_INTRA_PRED_ERROR2);

    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
        
        lumaMode = candidateBufferPtr->candidatePtr->intraLumaMode;

        puOriginIndex = ((puOriginY & (lcuSize-1)) * candidateBufferPtr->predictionPtr->strideY) + (puOriginX & (lcuSize-1));
        puSize        = puWidth;

        diffModeA = EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_HORIZONTAL_MODE);
        diffModeB = EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_VERTICAL_MODE);
        diffMode = MIN(diffModeA, diffModeB);

        contextPtr->AboveReadyFlagY  = EB_FALSE;
        contextPtr->LeftReadyFlagY   = EB_FALSE;

        switch(funcIndex) {

        case 0:

           yIntraReferenceArray =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                    contextPtr->yIntraReferenceArrayReverse;

		   IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);

            break;

        case 1:

            yIntraReferenceArray = contextPtr->yIntraReferenceArrayReverse;
      
			IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);

            break;

        case 2:
        
            yIntraReferenceArray =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                    contextPtr->yIntraReferenceArrayReverse;
              
       
			IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);
            break;

        case 3:

            yIntraReferenceArray =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                     contextPtr->yIntraReferenceArrayReverse;

			IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArray,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                EB_FALSE);

            break;
         
        case 4:
          
             yIntraReferenceArray        =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArray :
                                            contextPtr->yIntraReferenceArray;
             yIntraReferenceArrayReverse =  (diffMode > intraLumaFilterTable[Log2f(puWidth)-2])? contextPtr->yIntraFilteredReferenceArrayReverse :
                                            contextPtr->yIntraReferenceArrayReverse;

            IntraModeAngular_all(           
                lumaMode,
                puSize,
                yIntraReferenceArray,
                yIntraReferenceArrayReverse,
                &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
                candidateBufferPtr->predictionPtr->strideY,
                contextPtr->ReferenceAboveLineY,
                & contextPtr->AboveReadyFlagY,
                contextPtr->ReferenceLeftLineY,
                & contextPtr->LeftReadyFlagY);

            break;

        default:
            break;
        }
    }    

    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

        // The chromaMode is always DM
        chromaMode = (EB_U32) lumaMode;

        puChromaOriginIndex = (((puOriginY & (lcuSize-1)) * candidateBufferPtr->predictionPtr->strideCb) + (puOriginX & (lcuSize-1))) >> 1;
        chromaPuSize        = puWidth;

        contextPtr->AboveReadyFlagCb = EB_FALSE;
        contextPtr->AboveReadyFlagCr = EB_FALSE;
        contextPtr->LeftReadyFlagCb  = EB_FALSE;
        contextPtr->LeftReadyFlagCr  = EB_FALSE;

        switch(funcIndex) {

        case 0:
                
             // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }
      
            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 2:
              
            // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArray,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }
       
            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArray,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 3:
 
            // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }

            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 1:
               
            // Cb Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    EB_FALSE);
            }
      
            // Cr Intra Prediction
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    chromaPuSize,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    EB_FALSE);
            }

            break;

        case 4:

            // Cb Intra Prediction   
            if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
                IntraModeAngular_all(
                    chromaMode,
                    chromaPuSize,
                    contextPtr->cbIntraReferenceArray,
                    contextPtr->cbIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCb,
                    contextPtr->ReferenceAboveLineCb,
                    & contextPtr->AboveReadyFlagCb,
                    contextPtr->ReferenceLeftLineCb,
                    & contextPtr->LeftReadyFlagCb);
            }

            // Cr Intra Prediction     
            if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
                IntraModeAngular_all(
                    chromaMode,
                    chromaPuSize,
                    contextPtr->crIntraReferenceArray,
                    contextPtr->crIntraReferenceArrayReverse,
                    &(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBufferPtr->predictionPtr->strideCr,
                    contextPtr->ReferenceAboveLineCr,
                    & contextPtr->AboveReadyFlagCr,
                    contextPtr->ReferenceLeftLineCr,
                    & contextPtr->LeftReadyFlagCr);
            }

            break;

        default:
            break;
        }
    }

    return return_error;
}

EB_ERRORTYPE Intra4x4IntraPredictionOl(
	EB_U32                                  puIndex,
    EB_U32                                  puOriginX,
    EB_U32                                  puOriginY,
    EB_U32                                  puWidth,
    EB_U32                                  puHeight,
    EB_U32                                  lcuSize,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,   
    EB_PTR                                  predictionContextPtr)  
{
    EB_ERRORTYPE                return_error = EB_ErrorNone;
    EB_U32          openLoopIntraCandidateIndex = candidateBufferPtr->candidatePtr->intraLumaMode;
    const EB_U32 puOriginIndex = ((puOriginY & (lcuSize-1)) * candidateBufferPtr->predictionPtr->strideY) + (puOriginX & (lcuSize-1));
    const EB_U32 puSize        = puWidth;
      
    // Map the mode to the function table index
    EB_U32 funcIndex =
        (openLoopIntraCandidateIndex < 2)                        ?       openLoopIntraCandidateIndex  :
        (openLoopIntraCandidateIndex == INTRA_VERTICAL_MODE)     ?       2 :
        (openLoopIntraCandidateIndex == INTRA_HORIZONTAL_MODE)   ?       3 :
        4;

	IntraReferenceSamples_t    *intraRefPtr =    (IntraReferenceSamples_t*)(((ModeDecisionContext_t*)predictionContextPtr)->intraRefPtr);

    (void) puHeight;
	(void) componentMask;
	(void) pictureControlSetPtr;
	(void) puIndex;

    switch(funcIndex) {

    case 0:
        
		IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
            puSize,
            intraRefPtr->yIntraReferenceArrayReverse,
            &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
            candidateBufferPtr->predictionPtr->strideY,
            EB_FALSE);

        break;

    case 1:
        
		IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
            puSize,
            intraRefPtr->yIntraReferenceArrayReverse,
            &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
            candidateBufferPtr->predictionPtr->strideY,
            EB_FALSE);

        break;

    case 2:
        
		IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
            puSize,
            intraRefPtr->yIntraReferenceArrayReverse,
            &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
            candidateBufferPtr->predictionPtr->strideY,
            EB_FALSE);

        break;

    case 3:
        
		IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
            puSize,
            intraRefPtr->yIntraReferenceArrayReverse,
            &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
            candidateBufferPtr->predictionPtr->strideY,
            EB_FALSE);

        break;

    case 4:
        
        IntraModeAngular_all(           
            openLoopIntraCandidateIndex,
            puSize,
            intraRefPtr->yIntraReferenceArray,
            intraRefPtr->yIntraReferenceArrayReverse,
            &(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
            candidateBufferPtr->predictionPtr->strideY,
            intraRefPtr->ReferenceAboveLineY,
            &intraRefPtr->AboveReadyFlagY,
            intraRefPtr->ReferenceLeftLineY,
            &intraRefPtr->LeftReadyFlagY);

        break;

    default:
        break;
    }
    
    return return_error;
}


/*********************************************
 *  Encode Pass Intra Prediction
 *    Calculates a conformant H.265 prediction 
 *    for an Intra Prediction Unit
 *********************************************/
EB_ERRORTYPE EncodePassIntraPrediction(
    void                                   *refSamples,
    EB_U32                                  originX,
    EB_U32                                  originY,
    EB_U32                                  puSize,
    EB_U32                                  puChromaSize,
    EbPictureBufferDesc_t                  *predictionPtr,
    EB_COLOR_FORMAT                         colorFormat,
    EB_BOOL                                 secondChroma,
    EB_U32                                  lumaMode,
    EB_U32                                  chromaMode,
    EB_U32                                  componentMask)
{
    EB_ERRORTYPE             return_error = EB_ErrorNone;
    IntraReferenceSamples_t *referenceSamples = (IntraReferenceSamples_t*)refSamples;
    EB_U32 lumaOffset;
    EB_U32 chromaOffset;

    EB_S32 diffModeA;
    EB_S32 diffModeB;
    EB_S32 diffMode;

    EB_U8 *yIntraReferenceArray;
    EB_U8 *yIntraReferenceArrayReverse;

    EB_U8 *cbIntraReferenceArray;
    EB_U8 *cbIntraReferenceArrayReverse;
    EB_U8 *crIntraReferenceArray;
    EB_U8 *crIntraReferenceArrayReverse;

    EB_U32 chromaModeAdj;
    EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    //***********************************
    // Luma
    //***********************************
    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
        lumaOffset  =    (originY) * predictionPtr->strideY   + (originX);
        yIntraReferenceArray = referenceSamples->yIntraReferenceArray;
        yIntraReferenceArrayReverse = referenceSamples->yIntraReferenceArrayReverse;
        diffModeA   =   EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_HORIZONTAL_MODE);
        diffModeB   =   EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_VERTICAL_MODE);
        diffMode    =   MIN(diffModeA, diffModeB);

        if (diffMode > intraLumaFilterTable[Log2f(puSize) - 2] && lumaMode != EB_INTRA_DC) {
            yIntraReferenceArray = referenceSamples->yIntraFilteredReferenceArray;
            yIntraReferenceArrayReverse = referenceSamples->yIntraFilteredReferenceArrayReverse;
        }

        switch(lumaMode) {

        case EB_INTRA_PLANAR:
		IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);

            break;

        case EB_INTRA_DC:
			IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);

            break;

        case EB_INTRA_VERTICAL:
			IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);

            break;

        case EB_INTRA_HORIZONTAL:
			IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);

            break;

        default:
            IntraModeAngular_all(           
                lumaMode,
                puSize,
                yIntraReferenceArray,
                yIntraReferenceArrayReverse,
                predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                referenceSamples->ReferenceAboveLineY,
                &referenceSamples->AboveReadyFlagY,
                referenceSamples->ReferenceLeftLineY,
                &referenceSamples->LeftReadyFlagY);

            break;

        case EB_INTRA_MODE_INVALID:
            break;

        }
    }
    
    //***********************************
    // Chroma
    //***********************************
    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {
        if (secondChroma && colorFormat == EB_YUV422) {
            chromaOffset = (originX >> subWidthCMinus1) +
                ((originY + puChromaSize) >> subHeightCMinus1) * predictionPtr->strideCb;
        } else {
            chromaOffset = (originX >> subWidthCMinus1) +
                (originY >> subHeightCMinus1) * predictionPtr->strideCb;
        }

        chromaModeAdj   =    lumaMode;
        chromaModeAdj   = 
            (chromaMode == EB_INTRA_CHROMA_PLANAR)      ? EB_INTRA_PLANAR :
            (chromaMode == EB_INTRA_CHROMA_VERTICAL)    ? EB_INTRA_VERTICAL :
            (chromaMode == EB_INTRA_CHROMA_HORIZONTAL)  ? EB_INTRA_HORIZONTAL :
            (chromaMode == EB_INTRA_CHROMA_DC)          ? EB_INTRA_DC :
            (chromaMode == EB_INTRA_CHROMA_DM)          ? lumaMode : EB_INTRA_MODE_INVALID;

        cbIntraReferenceArray = referenceSamples->cbIntraReferenceArray;
        cbIntraReferenceArrayReverse = referenceSamples->cbIntraReferenceArrayReverse;
        crIntraReferenceArray = referenceSamples->crIntraReferenceArray;
        crIntraReferenceArrayReverse = referenceSamples->crIntraReferenceArrayReverse;

        if (colorFormat == EB_YUV422) {
            chromaModeAdj = intra422PredModeMap[chromaModeAdj];
        } else if (colorFormat == EB_YUV444) {
            diffModeA = EB_ABS_DIFF((EB_S32)chromaModeAdj, (EB_S32)INTRA_HORIZONTAL_MODE);
            diffModeB = EB_ABS_DIFF((EB_S32)chromaModeAdj, (EB_S32)INTRA_VERTICAL_MODE);
            diffMode = MIN(diffModeA, diffModeB);
            // Jing: diffMode > intraLumaFilterTable[Log2f(puSize) - 2] guarantee 4x4 case no filter will be done
            if (diffMode > intraLumaFilterTable[Log2f(puSize) - 2] && chromaModeAdj != EB_INTRA_DC) {
                cbIntraReferenceArray = referenceSamples->cbIntraFilteredReferenceArray;
                crIntraReferenceArray = referenceSamples->crIntraFilteredReferenceArray;
                cbIntraReferenceArrayReverse = referenceSamples->cbIntraFilteredReferenceArrayReverse;
                crIntraReferenceArrayReverse = referenceSamples->crIntraFilteredReferenceArrayReverse;
            }
        }

        switch(chromaModeAdj) {
        case EB_INTRA_PLANAR:
             // Cb Intra Prediction
			IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);

            // Cr Intra Prediction
			IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);

            break;

        case EB_INTRA_VERTICAL:
              
            // Cb Intra Prediction
			IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);
       
            // Cr Intra Prediction
			IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);

            break;

        case EB_INTRA_HORIZONTAL:
 
             // Cb Intra Prediction
			IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);
                         

            // Cr Intra Prediction
			IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);

            break;

        case EB_INTRA_DC:
               
            // Cb Intra Prediction
			IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);

      
            // Cr Intra Prediction
			IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);

            break;

        default:

            // *Note - For Chroma DM mode, use the Luma Angular mode
            //   to generate the prediction.

            // Cb Intra Prediction   
            IntraModeAngular_all(
                chromaModeAdj,                       
                puChromaSize,
                cbIntraReferenceArray,
                cbIntraReferenceArrayReverse,
                predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                referenceSamples->ReferenceAboveLineCb,
                &referenceSamples->AboveReadyFlagCb,
                referenceSamples->ReferenceLeftLineCb,
                &referenceSamples->LeftReadyFlagCb);

            // Cr Intra Prediction     
            IntraModeAngular_all(
                chromaModeAdj,
                puChromaSize,
                crIntraReferenceArray,
                crIntraReferenceArrayReverse,
                predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                referenceSamples->ReferenceAboveLineCr,
                &referenceSamples->AboveReadyFlagCr,
                referenceSamples->ReferenceLeftLineCr,
                &referenceSamples->LeftReadyFlagCr);

            break;

        case EB_INTRA_MODE_INVALID:
            break;
        }
    }

    return return_error;
}

/*********************************************
 *  Encode Pass Intra Prediction 16bit
 *    Calculates a conformant H.265 prediction 
 *    for an Intra Prediction Unit
 *********************************************/
EB_ERRORTYPE EncodePassIntraPrediction16bit(
    void                                   *refSamples,
    EB_U32                                  originX,
    EB_U32                                  originY,
    EB_U32                                  puSize,
    EB_U32                                  puChromaSize,
    EbPictureBufferDesc_t                  *predictionPtr,
    EB_COLOR_FORMAT                         colorFormat,
    EB_BOOL                                 secondChroma,
    EB_U32                                  lumaMode,
    EB_U32                                  chromaMode,
    EB_U32                                  componentMask)
{
    EB_ERRORTYPE             return_error = EB_ErrorNone;
    IntraReference16bitSamples_t *referenceSamples = (IntraReference16bitSamples_t *)refSamples;
    EB_U32 lumaOffset;
    EB_U32 chromaOffset;

    EB_S32 diffModeA;
    EB_S32 diffModeB;
    EB_S32 diffMode;

    EB_U16 *yIntraReferenceArray;
    EB_U16 *yIntraReferenceArrayReverse;

    EB_U16 *cbIntraReferenceArray;
    EB_U16 *cbIntraReferenceArrayReverse;
    EB_U16 *crIntraReferenceArray;
    EB_U16 *crIntraReferenceArrayReverse;

    EB_U32 chromaModeAdj;
    EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    //***********************************
    // Luma
    //***********************************
    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
        lumaOffset = (originY) * predictionPtr->strideY + originX;
        yIntraReferenceArray = referenceSamples->yIntraReferenceArray;
        yIntraReferenceArrayReverse = referenceSamples->yIntraReferenceArrayReverse;
        diffModeA = EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_HORIZONTAL_MODE);
        diffModeB = EB_ABS_DIFF((EB_S32) lumaMode,(EB_S32) INTRA_VERTICAL_MODE);
        diffMode = MIN(diffModeA, diffModeB);

        if (diffMode > intraLumaFilterTable[Log2f(puSize) - 2] && lumaMode != EB_INTRA_DC) {
            yIntraReferenceArray = referenceSamples->yIntraFilteredReferenceArray;
            yIntraReferenceArrayReverse = referenceSamples->yIntraFilteredReferenceArrayReverse;
        }

        switch(lumaMode) {

        case EB_INTRA_PLANAR:
        IntraPlanar_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);
            break;

        case EB_INTRA_DC:
            IntraDCLuma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);
            break;

        case EB_INTRA_VERTICAL:
            IntraVerticalLuma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);

            break;

        case EB_INTRA_HORIZONTAL:
            IntraHorzLuma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                yIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                EB_FALSE);
            break;

        default:
            IntraModeAngular16bit_all(           
                lumaMode,
                puSize,
                yIntraReferenceArray,
                yIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferY + lumaOffset,
                predictionPtr->strideY,
                referenceSamples->ReferenceAboveLineY,
                &referenceSamples->AboveReadyFlagY,
                referenceSamples->ReferenceLeftLineY,
                &referenceSamples->LeftReadyFlagY);

            break;

        case EB_INTRA_MODE_INVALID:
            break;

        }
    }
    
    //***********************************
    // Chroma
    //***********************************
    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {
        if (secondChroma && colorFormat == EB_YUV422) {
            chromaOffset = (originX >> subWidthCMinus1) +
                ((originY + puChromaSize) >> subHeightCMinus1) * predictionPtr->strideCb;
        } else {
            chromaOffset = (originX >> subWidthCMinus1) +
                (originY >> subHeightCMinus1) * predictionPtr->strideCb;
        }

        chromaModeAdj = lumaMode;
        chromaModeAdj = 
            (chromaMode == EB_INTRA_CHROMA_PLANAR)      ? EB_INTRA_PLANAR :
            (chromaMode == EB_INTRA_CHROMA_VERTICAL)    ? EB_INTRA_VERTICAL :
            (chromaMode == EB_INTRA_CHROMA_HORIZONTAL)  ? EB_INTRA_HORIZONTAL :
            (chromaMode == EB_INTRA_CHROMA_DC)          ? EB_INTRA_DC :
            (chromaMode == EB_INTRA_CHROMA_DM)          ? lumaMode : EB_INTRA_MODE_INVALID;

        cbIntraReferenceArray = referenceSamples->cbIntraReferenceArray;
        cbIntraReferenceArrayReverse = referenceSamples->cbIntraReferenceArrayReverse;
        crIntraReferenceArray = referenceSamples->crIntraReferenceArray;
        crIntraReferenceArrayReverse = referenceSamples->crIntraReferenceArrayReverse;

        if (colorFormat == EB_YUV422) {
            chromaModeAdj = intra422PredModeMap[chromaModeAdj];
        } else if (colorFormat == EB_YUV444) {
            diffModeA = EB_ABS_DIFF((EB_S32)chromaModeAdj, (EB_S32)INTRA_HORIZONTAL_MODE);
            diffModeB = EB_ABS_DIFF((EB_S32)chromaModeAdj, (EB_S32)INTRA_VERTICAL_MODE);
            diffMode = MIN(diffModeA, diffModeB);
            // Jing: diffMode > intraLumaFilterTable[Log2f(puSize) - 2] guarantee 4x4 case no filter will be done
            if (diffMode > intraLumaFilterTable[Log2f(puSize) - 2] && chromaModeAdj != EB_INTRA_DC) {
                cbIntraReferenceArray = referenceSamples->cbIntraFilteredReferenceArray;
                crIntraReferenceArray = referenceSamples->crIntraFilteredReferenceArray;
                cbIntraReferenceArrayReverse = referenceSamples->cbIntraFilteredReferenceArrayReverse;
                crIntraReferenceArrayReverse = referenceSamples->crIntraFilteredReferenceArrayReverse;
            }
        }

        switch(chromaModeAdj) {
        case EB_INTRA_PLANAR:
             IntraPlanar_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);

            IntraPlanar_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);
            break;

        case EB_INTRA_VERTICAL:
            IntraVerticalChroma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);
       
            IntraVerticalChroma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);
            break;

        case EB_INTRA_HORIZONTAL:
            IntraHorzChroma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);
                         
            IntraHorzChroma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);
            break;

        case EB_INTRA_DC:
            IntraDCChroma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                cbIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                EB_FALSE);

            IntraDCChroma_16bit_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puChromaSize,
                crIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                EB_FALSE);
            break;

        default:

            // *Note - For Chroma DM mode, use the Luma Angular mode
            //   to generate the prediction.
            IntraModeAngular16bit_all(
                chromaModeAdj,                       
                puChromaSize,
                cbIntraReferenceArray,
                cbIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCb + chromaOffset,
                predictionPtr->strideCb,
                referenceSamples->ReferenceAboveLineCb,
                &referenceSamples->AboveReadyFlagCb,
                referenceSamples->ReferenceLeftLineCb,
                &referenceSamples->LeftReadyFlagCb);

            IntraModeAngular16bit_all(
                chromaModeAdj,
                puChromaSize,
                crIntraReferenceArray,
                crIntraReferenceArrayReverse,
                (EB_U16*)predictionPtr->bufferCr + chromaOffset,
                predictionPtr->strideCr,
                referenceSamples->ReferenceAboveLineCr,
                &referenceSamples->AboveReadyFlagCr,
                referenceSamples->ReferenceLeftLineCr,
                &referenceSamples->LeftReadyFlagCr);
            break;

        case EB_INTRA_MODE_INVALID:
            break;
        }
    }

    return return_error;
}

/**********************************************
 * Intra Reference Samples Ctor
 **********************************************/
EB_ERRORTYPE IntraOpenLoopReferenceSamplesCtor(
    IntraReferenceSamplesOpenLoop_t **contextDblPtr)
{
    IntraReferenceSamplesOpenLoop_t *contextPtr;
    EB_MALLOC(IntraReferenceSamplesOpenLoop_t*, contextPtr, sizeof(IntraReferenceSamplesOpenLoop_t), EB_N_PTR);

    *contextDblPtr = contextPtr;

    EB_MALLOC(EB_U8*, contextPtr->yIntraReferenceArray, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 1), EB_N_PTR);
    
    EB_MALLOC(EB_U8*, contextPtr->yIntraReferenceArrayReverse, sizeof(EB_U8) * (4 * MAX_LCU_SIZE + 2), EB_N_PTR);

    contextPtr->yIntraReferenceArrayReverse++;
     
    return EB_ErrorNone;
}


 
EB_ERRORTYPE UpdateNeighborSamplesArrayOL(
    IntraReferenceSamples_t         *intraRefPtr,
    EbPictureBufferDesc_t           *inputPtr,          
    EB_U32                           stride,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
    EB_U32                           blockSize,
    LargestCodingUnit_t             *lcuPtr)
{

    EB_ERRORTYPE    return_error    = EB_ErrorNone;

    EB_U32 idx;
    EB_U8  *srcPtr;
    EB_U8  *dstPtr;
    EB_U8  *readPtr;

    EB_U32 count;
    EB_U8 *yBorderReverse   = intraRefPtr->yIntraReferenceArrayReverse;
    EB_U8 *yBorder          = intraRefPtr->yIntraReferenceArray;
    EB_U8 *yBorderLoc;
	EB_U32 width = inputPtr->width;
	EB_U32 height = inputPtr->height;
	EB_U32 blockSizeHalf = blockSize << 1;

    const EB_BOOL pictureLeftBoundary = (lcuPtr->pictureLeftEdgeFlag == EB_TRUE && (srcOriginX & (lcuPtr->size - 1)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureTopBoundary  = (lcuPtr->pictureTopEdgeFlag  == EB_TRUE && (srcOriginY & (lcuPtr->size - 1)) == 0) ? EB_TRUE : EB_FALSE;
    // Adjust the Source ptr to start at the origin of the block being updated
	srcPtr  = inputPtr->bufferY + (((srcOriginY + inputPtr->originY) * stride) + (srcOriginX + inputPtr->originX));

    // Adjust the Destination ptr to start at the origin of the Intra reference array
    dstPtr  = yBorderReverse;

    //Initialise the Luma Intra Reference Array to the mid range value 128 (for CUs at the picture boundaries)
    EB_MEMSET(dstPtr, MIDRANGE_VALUE_8BIT, (blockSize << 2) + 1);

    // Get the left-column
    count = blockSizeHalf;
    if (pictureLeftBoundary == EB_FALSE) {
        
        readPtr = srcPtr - 1;
        count   = ((srcOriginY + count) > height) ? count - ((srcOriginY + count) - height) : count;
        
        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;
            readPtr += stride;
            dstPtr  ++;
        }

        dstPtr += (blockSizeHalf- count);

    } else {
        
        dstPtr += count;
    }
    
    // Get the upper left sample
    if (pictureLeftBoundary == EB_FALSE && pictureTopBoundary == EB_FALSE) {

        readPtr = srcPtr - stride- 1 ;
        *dstPtr = *readPtr;
        dstPtr  ++;
    } else {

        dstPtr  ++;
    }
    
    // Get the top-row
    count = blockSizeHalf;
    if (pictureTopBoundary == EB_FALSE) {
        readPtr = srcPtr - stride;
        
        count   = ((srcOriginX + count) > width) ? count - ((srcOriginX + count) - width) : count;
		EB_MEMCPY(dstPtr, readPtr, count);
        dstPtr += (blockSizeHalf - count);

    } else {
        
        dstPtr += count;
    }
    

    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagY  = EB_FALSE;
    intraRefPtr->LeftReadyFlagY   = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                    <=======     L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	EB_MEMCPY(yBorder + blockSizeHalf, yBorderReverse + blockSizeHalf,  blockSizeHalf + 1);
    yBorderLoc = yBorder + blockSizeHalf - 1 ;

    for(count = 0; count< blockSizeHalf; count++){
        
       *yBorderLoc     = yBorderReverse[count];
        yBorderLoc--;
    }

    return return_error;
}

EB_ERRORTYPE UpdateChromaNeighborSamplesArrayOL(
    IntraReferenceSamples_t         *intraRefPtr,
    EbPictureBufferDesc_t           *inputPtr,  
    EB_U32                           stride,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
    EB_U32                           cbStride,
    EB_U32                           crStride,
    EB_U32                           cuChromaOriginX,
    EB_U32                           cuChromaOriginY,
    EB_U32                           blockSize,
    LargestCodingUnit_t             *lcuPtr)
{

    EB_ERRORTYPE    return_error    = EB_ErrorNone;

    EB_U32 idx;
    EB_U8  *cbSrcPtr;
    EB_U8  *cbDstPtr;
    EB_U8  *cbReadPtr;

	EB_U8  *crSrcPtr;
    EB_U8  *crDstPtr;
    EB_U8  *crReadPtr;

    EB_U32 count;
    EB_U8 *cbBorderReverse   = intraRefPtr->cbIntraReferenceArrayReverse;
	EB_U8 *crBorderReverse   = intraRefPtr->crIntraReferenceArrayReverse;
    EB_U8 *cbBorder          = intraRefPtr->cbIntraReferenceArray;
	EB_U8 *crBorder          = intraRefPtr->crIntraReferenceArray;
    EB_U8 *cbBorderLoc;
	EB_U8 *crBorderLoc;
	EB_U32 width = inputPtr->width >> 1;
	EB_U32 height = inputPtr->height >> 1;
	EB_U32 blockSizeHalf = blockSize << 1;

    const EB_BOOL pictureLeftBoundary = (lcuPtr->pictureLeftEdgeFlag == EB_TRUE && (srcOriginX & (lcuPtr->size - 1)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureTopBoundary  = (lcuPtr->pictureTopEdgeFlag  == EB_TRUE && (srcOriginY & (lcuPtr->size - 1)) == 0) ? EB_TRUE : EB_FALSE;

    // Adjust the Source ptr to start at the origin of the block being updated
	cbSrcPtr  = inputPtr->bufferCb + (((cuChromaOriginY + (inputPtr->originY >> 1)) * cbStride) + (cuChromaOriginX + (inputPtr->originX >> 1)));
	crSrcPtr  = inputPtr->bufferCr + (((cuChromaOriginY + (inputPtr->originY >> 1)) * crStride) + (cuChromaOriginX + (inputPtr->originX >> 1)));

    // Adjust the Destination ptr to start at the origin of the Intra reference array
	cbDstPtr  = cbBorderReverse;
    crDstPtr  = crBorderReverse;
    //Initialise the Luma Intra Reference Array to the mid range value 128 (for CUs at the picture boundaries)
	EB_MEMSET(cbDstPtr, MIDRANGE_VALUE_8BIT, (blockSize << 2) + 1);
	EB_MEMSET(crDstPtr, MIDRANGE_VALUE_8BIT, (blockSize << 2) + 1);

	(void)stride;

    // Get the left-column
    count = blockSizeHalf;
    if (pictureLeftBoundary == EB_FALSE) {
        
        cbReadPtr = cbSrcPtr - 1;
		crReadPtr = crSrcPtr - 1;
        count   = ((cuChromaOriginY + count) > height) ? count - ((cuChromaOriginY + count) - height) : count;
        
        for(idx = 0; idx < count; ++idx) {

            *cbDstPtr = *cbReadPtr;
            cbReadPtr += cbStride;
            cbDstPtr  ++;

			*crDstPtr = *crReadPtr;
            crReadPtr += crStride;
            crDstPtr  ++;
        }

        cbDstPtr += (blockSizeHalf- count);
		crDstPtr += (blockSizeHalf- count);

    } else {
        
		cbDstPtr += count;
        crDstPtr += count;
    }
    
    // Get the upper left sample
    if (pictureLeftBoundary == EB_FALSE && pictureTopBoundary == EB_FALSE) {
        cbReadPtr = cbSrcPtr - cbStride- 1 ;
        *cbDstPtr = *cbReadPtr;
        cbDstPtr  ++;

		crReadPtr = crSrcPtr - crStride- 1 ;
        *crDstPtr = *crReadPtr;
        crDstPtr  ++;
    } else {

        cbDstPtr  ++;
		crDstPtr  ++;
    }
    
    // Get the top-row
    count = blockSizeHalf;
    if (pictureTopBoundary == EB_FALSE) {

        cbReadPtr = cbSrcPtr - cbStride;
        
        count   = ((cuChromaOriginX + count) > width) ? count - ((cuChromaOriginX + count) - width) : count;
		EB_MEMCPY(cbDstPtr, cbReadPtr, count);
        cbDstPtr += (blockSizeHalf - count);


		crReadPtr = crSrcPtr - crStride;
		EB_MEMCPY(crDstPtr, crReadPtr, count);
        crDstPtr += (blockSizeHalf - count);

    } else {
        
        crDstPtr += count;
    }
    

    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagCb  = EB_FALSE;
    intraRefPtr->LeftReadyFlagCb   = EB_FALSE;

	intraRefPtr->AboveReadyFlagCr  = EB_FALSE;
    intraRefPtr->LeftReadyFlagCr   = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                    <=======     L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	EB_MEMCPY(cbBorder + blockSizeHalf, cbBorderReverse + blockSizeHalf,  blockSizeHalf + 1);
	EB_MEMCPY(crBorder + blockSizeHalf, crBorderReverse + blockSizeHalf,  blockSizeHalf + 1);
    cbBorderLoc = cbBorder + blockSizeHalf - 1 ;
	crBorderLoc = crBorder + blockSizeHalf - 1 ;

    for(count = 0; count< blockSizeHalf; count++){
        
       *cbBorderLoc     = cbBorderReverse[count];
        cbBorderLoc--;

		*crBorderLoc     = crBorderReverse[count];
        crBorderLoc--;
    }

    return return_error;
}

/** UpdateNeighborSamplesArrayOpenLoop()
        updates neighbor sample array
 */
EB_ERRORTYPE UpdateNeighborSamplesArrayOpenLoop(
    IntraReferenceSamplesOpenLoop_t *intraRefPtr,
    EbPictureBufferDesc_t           *inputPtr,          
    EB_U32                           stride,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
    EB_U32                           blockSize)
{

    EB_ERRORTYPE    return_error    = EB_ErrorNone;

    EB_U32 idx;
    EB_U8  *srcPtr;
    EB_U8  *dstPtr;
    EB_U8  *readPtr;

    EB_U32 count;
    
    EB_U8 *yBorderReverse   = intraRefPtr->yIntraReferenceArrayReverse;
    EB_U8 *yBorder          = intraRefPtr->yIntraReferenceArray;
    EB_U8 *yBorderLoc;

	EB_U32 width = inputPtr->width;
	EB_U32 height = inputPtr->height;
	EB_U32 blockSizeHalf = blockSize << 1;

    // Adjust the Source ptr to start at the origin of the block being updated
    srcPtr  = inputPtr->bufferY + (((srcOriginY + inputPtr->originY) * stride) + (srcOriginX + inputPtr->originX));

    // Adjust the Destination ptr to start at the origin of the Intra reference array
    dstPtr  = yBorderReverse;

    //Initialise the Luma Intra Reference Array to the mid range value 128 (for CUs at the picture boundaries)
    EB_MEMSET(dstPtr, MIDRANGE_VALUE_8BIT, (blockSize << 2) + 1);

    // Get the left-column
    count   = blockSizeHalf;

    if (srcOriginX != 0) {
        
        readPtr = srcPtr - 1;
        count   = ((srcOriginY + count) > height) ? count - ((srcOriginY + count) - height) : count;
        
        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;
            readPtr += stride;
            dstPtr  ++;
        }

        dstPtr += (blockSizeHalf- count);

    } else {
        
        dstPtr += count;
    }
    
    // Get the upper left sample
    if (srcOriginX != 0 && srcOriginY != 0) {

        readPtr = srcPtr - stride- 1 ;
        *dstPtr = *readPtr;
        dstPtr  ++;
    } else {

        dstPtr  ++;
    }
    
    // Get the top-row
    count   = blockSizeHalf;
    if (srcOriginY != 0) {

        readPtr = srcPtr - stride;
        
        count   = ((srcOriginX + count) > width) ? count - ((srcOriginX + count) - width) : count;
		EB_MEMCPY(dstPtr, readPtr, count);
        dstPtr += (blockSizeHalf - count);

    } else {
        
        dstPtr += count;
    }
    

    //at the begining of a CU Loop, the Above/Left scratch buffers are not ready to be used.
    intraRefPtr->AboveReadyFlagY  = EB_FALSE;
    intraRefPtr->LeftReadyFlagY   = EB_FALSE;

    //For SIMD purposes, provide a copy of the reference buffer with reverse order of Left samples 
    /*
        TL   T0   T1   T2   T3  T4  T5  T6  T7                 TL   T0   T1   T2   T3  T4  T5  T6  T7
        L0  |----------------|                                 L7  |----------------|
        L1  |                |                    <=======     L6  |                |   
        L2  |                |                                 L5  |                |
        L3  |----------------|                                 L4  |----------------|
        L4                                                     L3 
        L5                                                     L2
        L6                                                     L1
        L7 <-- pointer (Regular Order)                         L0<-- pointer     Reverse Order 
                                                               junk
    */  

	EB_MEMCPY(yBorder + blockSizeHalf,  yBorderReverse + blockSizeHalf,  blockSizeHalf + 1);
    yBorderLoc = yBorder + blockSizeHalf - 1 ;

    for(count = 0; count<blockSizeHalf; count++){
        
       *yBorderLoc     = yBorderReverse[count];
        yBorderLoc--;
    }

    return return_error;
}

/** IntraPredictionOpenLoop()
        performs Open-loop Intra candidate Search for a CU
 */
EB_ERRORTYPE IntraPredictionOpenLoop(
    EB_U32                               cuSize,                       // input parameter, pointer to the current cu
    MotionEstimationContext_t           *contextPtr,                  // input parameter, ME context
    EB_U32                   openLoopIntraCandidateIndex) // input parameter, intra mode
{
    EB_ERRORTYPE                return_error = EB_ErrorNone;
    
    // Map the mode to the function table index
    EB_U32 funcIndex =
        (openLoopIntraCandidateIndex < 2)                        ?       openLoopIntraCandidateIndex  :
        (openLoopIntraCandidateIndex == INTRA_VERTICAL_MODE)     ?       2 :
        (openLoopIntraCandidateIndex == INTRA_HORIZONTAL_MODE)   ?       3 :
        4;
   

    switch(funcIndex) {

    case 0:
        
		IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
            cuSize,
            contextPtr->intraRefPtr->yIntraReferenceArrayReverse,
            (&(contextPtr->meContextPtr->lcuBuffer[0])),
            contextPtr->meContextPtr->lcuBufferStride,
            EB_FALSE);

        break;

    case 1:
        
		IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
            cuSize,
            contextPtr->intraRefPtr->yIntraReferenceArrayReverse,
            (&(contextPtr->meContextPtr->lcuBuffer[0])),
            contextPtr->meContextPtr->lcuBufferStride,
            EB_FALSE);

        break;

    case 2:
        
		IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
            cuSize,
            contextPtr->intraRefPtr->yIntraReferenceArrayReverse,
            (&(contextPtr->meContextPtr->lcuBuffer[0])),
            contextPtr->meContextPtr->lcuBufferStride,
            EB_FALSE);

        break;

    case 3:
        
		IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
            cuSize,
            contextPtr->intraRefPtr->yIntraReferenceArrayReverse,
            (&(contextPtr->meContextPtr->lcuBuffer[0])),
            contextPtr->meContextPtr->lcuBufferStride,
            EB_FALSE);

        break;

    case 4:
        
        IntraModeAngular_all(           
            openLoopIntraCandidateIndex,
            cuSize,
            contextPtr->intraRefPtr->yIntraReferenceArray,
            contextPtr->intraRefPtr->yIntraReferenceArrayReverse,
            (&(contextPtr->meContextPtr->lcuBuffer[0])),
            contextPtr->meContextPtr->lcuBufferStride,
            contextPtr->intraRefPtr->ReferenceAboveLineY,
            & contextPtr->intraRefPtr->AboveReadyFlagY,
            contextPtr->intraRefPtr->ReferenceLeftLineY,
            & contextPtr->intraRefPtr->LeftReadyFlagY);

        break;

    default:
        break;
    }
    
    return return_error;
}


/** IntraPredictionOpenLoop()
        performs Open-loop Intra candidate Search for a CU
 */
EB_ERRORTYPE IntraPredictionOl(
	ModeDecisionContext_t                  *mdContextPtr,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
	ModeDecisionCandidateBuffer_t		   *candidateBufferPtr)
{
    EB_ERRORTYPE                return_error = EB_ErrorNone;


	EB_U32						puOriginX	= mdContextPtr->cuOriginX;
	EB_U32						puOriginY	= mdContextPtr->cuOriginY;
	EB_U32 						puWidth     = mdContextPtr->cuStats->size;
	EB_U32 						puHeight    = mdContextPtr->cuStats->size;
	const EB_U32 puOriginIndex = ((puOriginY & (63)) * 64) + (puOriginX & (63));

    EB_U32          openLoopIntraCandidateIndex = candidateBufferPtr->candidatePtr->intraLumaMode;
    
    const EB_U32 puSize        = puWidth;
    const EB_U32 puIndex = mdContextPtr->puItr;
      
    // Map the mode to the function table index
    EB_U32 funcIndex =
        (openLoopIntraCandidateIndex < 2)                        ?       openLoopIntraCandidateIndex  :
        (openLoopIntraCandidateIndex == INTRA_VERTICAL_MODE)     ?       2 :
        (openLoopIntraCandidateIndex == INTRA_HORIZONTAL_MODE)   ?       3 :
        4;

	IntraReferenceSamples_t    *intraRefPtr =    (IntraReferenceSamples_t*)(mdContextPtr->intraRefPtr);

    (void) puHeight;
	(void) pictureControlSetPtr;
	(void) puIndex;

	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

		if (mdContextPtr->lumaIntraRefSamplesGenDone == EB_FALSE)
		{
			EbPictureBufferDesc_t     *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

			GenerateIntraLumaReferenceSamplesMd(
				mdContextPtr,
				inputPicturePtr);
		}


		switch (funcIndex) {

		case 0:

			IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				puSize,
				intraRefPtr->yIntraReferenceArrayReverse,
				&(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
				64,
				EB_FALSE);

			break;

		case 1:

			IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				puSize,
				intraRefPtr->yIntraReferenceArrayReverse,
				&(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
				64,
				EB_FALSE);

			break;

		case 2:

			IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				puSize,
				intraRefPtr->yIntraReferenceArrayReverse,
				&(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
				64,
				EB_FALSE);

			break;

		case 3:

			IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
				puSize,
				intraRefPtr->yIntraReferenceArrayReverse,
				&(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
				64,
				EB_FALSE);

			break;

		case 4:

			IntraModeAngular_all(
				openLoopIntraCandidateIndex,
				puSize,
				intraRefPtr->yIntraReferenceArray,
				intraRefPtr->yIntraReferenceArrayReverse,
				&(candidateBufferPtr->predictionPtr->bufferY[puOriginIndex]),
				64,
				intraRefPtr->ReferenceAboveLineY,
				&intraRefPtr->AboveReadyFlagY,
				intraRefPtr->ReferenceLeftLineY,
				&intraRefPtr->LeftReadyFlagY);

			break;

		default:
			break;
		}
	}

	if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

		if (mdContextPtr->chromaIntraRefSamplesGenDone == EB_FALSE)
		{

			EbPictureBufferDesc_t *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

			GenerateIntraChromaReferenceSamplesMd(
				mdContextPtr,
				inputPicturePtr);

		}

		// The chromaMode is always DM
		EB_U32 chromaMode = (EB_U32)funcIndex;

        EB_U32 puChromaOriginIndex = (((puOriginY & (63)) * 32) + (puOriginX & (63)))>>1;
		EB_U32 chromaPuSize = puWidth >> 1;

		intraRefPtr->AboveReadyFlagCb = EB_FALSE;
		intraRefPtr->AboveReadyFlagCr = EB_FALSE;
		intraRefPtr->LeftReadyFlagCb = EB_FALSE;
		intraRefPtr->LeftReadyFlagCr = EB_FALSE;

		switch (funcIndex) {

		case 0:

			// Cb Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
					chromaPuSize,
					intraRefPtr->cbIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			// Cr Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
					chromaPuSize,
					intraRefPtr->crIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			break;

		case 2:

			// Cb Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
					chromaPuSize,
					intraRefPtr->cbIntraReferenceArray,
					&(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
					candidateBufferPtr->predictionPtr->strideCb,
					EB_FALSE);
			}

			// Cr Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraVerticalChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
					chromaPuSize,
					intraRefPtr->crIntraReferenceArray,
					&(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			break;

		case 3:

			// Cb Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
					chromaPuSize,
					intraRefPtr->cbIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			// Cr Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraHorzChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
					chromaPuSize,
					intraRefPtr->crIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			break;

		case 1:

			// Cb Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
					chromaPuSize,
					intraRefPtr->cbIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			// Cr Intra Prediction
			if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraDCChroma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
					chromaPuSize,
					intraRefPtr->crIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
					32,
					EB_FALSE);
			}

			break;

		case 4:

			// Cb Intra Prediction   
			if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
				IntraModeAngular_all(
					chromaMode,
					chromaPuSize,
					intraRefPtr->cbIntraReferenceArray,
					intraRefPtr->cbIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCb[puChromaOriginIndex]),
					candidateBufferPtr->predictionPtr->strideCb,
					intraRefPtr->ReferenceAboveLineCb,
					&intraRefPtr->AboveReadyFlagCb,
					intraRefPtr->ReferenceLeftLineCb,
					&intraRefPtr->LeftReadyFlagCb);
			}

			// Cr Intra Prediction     
			if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
				IntraModeAngular_all(
					chromaMode,
					chromaPuSize,
					intraRefPtr->crIntraReferenceArray,
					intraRefPtr->crIntraReferenceArrayReverse,
					&(candidateBufferPtr->predictionPtr->bufferCr[puChromaOriginIndex]),
					candidateBufferPtr->predictionPtr->strideCr,
					intraRefPtr->ReferenceAboveLineCr,
					&intraRefPtr->AboveReadyFlagCr,
					intraRefPtr->ReferenceLeftLineCr,
					&intraRefPtr->LeftReadyFlagCr);
			}

			break;

		default:
			break;
		}
	}
   
    return return_error;
}


EB_ERRORTYPE IntraPredOnSrc(
    ModeDecisionContext_t                  *mdContextPtr,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    EbPictureBufferDesc_t                  * predictionPtr,
    EB_U32                                   intraMode)
{
    EB_ERRORTYPE                return_error = EB_ErrorNone;


    EB_U32						puOriginX = mdContextPtr->cuOriginX;
    EB_U32						puOriginY = mdContextPtr->cuOriginY;
    EB_U32 						puWidth = mdContextPtr->cuStats->size;
    EB_U32 						puHeight = mdContextPtr->cuStats->size;
    const EB_U32 puOriginIndex = ((puOriginY & (63)) * 64) + (puOriginX & (63));

    EB_U32          openLoopIntraCandidateIndex = intraMode;

    const EB_U32 puSize = puWidth;
    const EB_U32 puIndex = mdContextPtr->puItr;

    // Map the mode to the function table index
    EB_U32 funcIndex =
        (openLoopIntraCandidateIndex < 2) ? openLoopIntraCandidateIndex :
        (openLoopIntraCandidateIndex == INTRA_VERTICAL_MODE) ? 2 :
        (openLoopIntraCandidateIndex == INTRA_HORIZONTAL_MODE) ? 3 :
        4;

    IntraReferenceSamples_t    *intraRefPtr = (IntraReferenceSamples_t*)(mdContextPtr->intraRefPtr);

    (void)puHeight;
    (void)pictureControlSetPtr;
    (void)puIndex;

    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

        if (mdContextPtr->lumaIntraRefSamplesGenDone == EB_FALSE)
        {
            EbPictureBufferDesc_t     *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

            GenerateIntraLumaReferenceSamplesMd(
                mdContextPtr,
                inputPicturePtr);
        }


        switch (funcIndex) {

        case 0:

			IntraPlanar_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                intraRefPtr->yIntraReferenceArrayReverse,
                &(predictionPtr->bufferY[puOriginIndex]),
                64,
                EB_FALSE);

            break;

        case 1:

			IntraDCLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                intraRefPtr->yIntraReferenceArrayReverse,
                &(predictionPtr->bufferY[puOriginIndex]),
                64,
                EB_FALSE);

            break;

        case 2:

			IntraVerticalLuma_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
                puSize,
                intraRefPtr->yIntraReferenceArrayReverse,
                &(predictionPtr->bufferY[puOriginIndex]),
                64,
                EB_FALSE);

            break;

        case 3:

			IntraHorzLuma_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](
                puSize,
                intraRefPtr->yIntraReferenceArrayReverse,
                &(predictionPtr->bufferY[puOriginIndex]),
                64,
                EB_FALSE);

            break;

        case 4:

            IntraModeAngular_all(
                openLoopIntraCandidateIndex,
                puSize,
                intraRefPtr->yIntraReferenceArray,
                intraRefPtr->yIntraReferenceArrayReverse,
                &(predictionPtr->bufferY[puOriginIndex]),
                64,
                intraRefPtr->ReferenceAboveLineY,
                &intraRefPtr->AboveReadyFlagY,
                intraRefPtr->ReferenceLeftLineY,
                &intraRefPtr->LeftReadyFlagY);

            break;

        default:
            break;
        }
    }
    return return_error;
}

