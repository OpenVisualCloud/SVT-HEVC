/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbEncDecSegments.h"
#include "EbThreads.h"
#include "EbDefinitions.h"

EB_ERRORTYPE EncDecSegmentsCtor(
    EncDecSegments_t **segmentsDblPtr,
    EB_U32             segmentColCount,
    EB_U32             segmentRowCount)
{
    EB_U32 rowIndex;
    EncDecSegments_t *segmentsPtr;
    EB_MALLOC(EncDecSegments_t*, segmentsPtr, sizeof(EncDecSegments_t), EB_N_PTR);
    
    *segmentsDblPtr = segmentsPtr;

    segmentsPtr->segmentMaxRowCount = segmentRowCount;
    segmentsPtr->segmentMaxBandCount = segmentRowCount + segmentColCount;
    segmentsPtr->segmentMaxTotalCount = segmentsPtr->segmentMaxRowCount * segmentsPtr->segmentMaxBandCount;

    // Start Arrays
    EB_MALLOC(EB_U16*, segmentsPtr->xStartArray, sizeof(EB_U16) * segmentsPtr->segmentMaxTotalCount, EB_N_PTR);

    EB_MALLOC(EB_U16*, segmentsPtr->yStartArray, sizeof(EB_U16) * segmentsPtr->segmentMaxTotalCount, EB_N_PTR);
    
    EB_MALLOC(EB_U16*, segmentsPtr->validLcuCountArray, sizeof(EB_U16) * segmentsPtr->segmentMaxTotalCount, EB_N_PTR);
    
    // Dependency map
    EB_MALLOC(EB_U8*, segmentsPtr->depMap.dependencyMap, sizeof(EB_U8) * segmentsPtr->segmentMaxTotalCount, EB_N_PTR);
    
    EB_CREATEMUTEX(EB_HANDLE, segmentsPtr->depMap.updateMutex, sizeof(EB_HANDLE), EB_MUTEX);
    
    // Segment rows
    EB_MALLOC(EncDecSegSegmentRow_t*, segmentsPtr->rowArray, sizeof(EncDecSegSegmentRow_t) * segmentsPtr->segmentMaxRowCount, EB_N_PTR)
    
    for(rowIndex=0; rowIndex < segmentsPtr->segmentMaxRowCount; ++rowIndex) {
        EB_CREATEMUTEX(EB_HANDLE, segmentsPtr->rowArray[rowIndex].assignmentMutex, sizeof(EB_HANDLE), EB_MUTEX);
    }

    return EB_ErrorNone;
}



void EncDecSegmentsInit(
    EncDecSegments_t *segmentsPtr,
    EB_U32            segColCount,
    EB_U32            segRowCount,
    EB_U32            picWidthLcu,
    EB_U32            picHeightLcu)
{
    unsigned x, y, yLast;
    unsigned rowIndex, bandIndex, segmentIndex;

    segColCount = (segColCount <= picWidthLcu) ? segColCount : picWidthLcu;
    segRowCount = (segRowCount <= picHeightLcu) ? segRowCount : picHeightLcu;
    segmentsPtr->lcuRowCount = picHeightLcu;
    segmentsPtr->lcuBandCount = BAND_TOTAL_COUNT(picHeightLcu, picWidthLcu);
    segmentsPtr->segmentRowCount = segRowCount;
    segmentsPtr->segmentBandCount = BAND_TOTAL_COUNT(segRowCount, segColCount);
    segmentsPtr->segmentTotalCount = segmentsPtr->segmentRowCount * segmentsPtr->segmentBandCount;

    //EB_MEMSET(segmentsPtr->inputMap.inputDependencyMap, 0, sizeof(EB_U16) * segmentsPtr->segmentTotalCount);
    EB_MEMSET(segmentsPtr->validLcuCountArray, 0, sizeof(EB_U16) * segmentsPtr->segmentTotalCount);
    EB_MEMSET(segmentsPtr->xStartArray, -1, sizeof(EB_U16) * segmentsPtr->segmentTotalCount);
    EB_MEMSET(segmentsPtr->yStartArray, -1, sizeof(EB_U16) * segmentsPtr->segmentTotalCount);

    // Initialize the per-LCU input availability map & Start Arrays
    for(y=0; y < picHeightLcu; ++y) {
        for(x=0; x < picWidthLcu; ++x) {
            bandIndex = BAND_INDEX(x, y, segmentsPtr->segmentBandCount, segmentsPtr->lcuBandCount);
            rowIndex = ROW_INDEX(y, segmentsPtr->segmentRowCount, segmentsPtr->lcuRowCount);
            segmentIndex = SEGMENT_INDEX(rowIndex, bandIndex, segmentsPtr->segmentBandCount);

            //++segmentsPtr->inputMap.inputDependencyMap[segmentIndex];
            ++segmentsPtr->validLcuCountArray[segmentIndex];
            segmentsPtr->xStartArray[segmentIndex] = (segmentsPtr->xStartArray[segmentIndex] == (EB_U16) -1) ? 
                (EB_U16) x : 
                segmentsPtr->xStartArray[segmentIndex];
            segmentsPtr->yStartArray[segmentIndex] = (segmentsPtr->yStartArray[segmentIndex] == (EB_U16) -1) ? 
                (EB_U16) y : 
                segmentsPtr->yStartArray[segmentIndex]; 
        }
    }

    // Initialize the row-based controls
    for(rowIndex=0; rowIndex < segmentsPtr->segmentRowCount; ++rowIndex) {
        y = ((rowIndex * segmentsPtr->lcuRowCount) + (segmentsPtr->segmentRowCount-1)) / segmentsPtr->segmentRowCount;
        yLast = ((((rowIndex+1) * segmentsPtr->lcuRowCount) + (segmentsPtr->segmentRowCount-1)) / segmentsPtr->segmentRowCount) - 1;
        bandIndex = BAND_INDEX(0, y, segmentsPtr->segmentBandCount, segmentsPtr->lcuBandCount);

        segmentsPtr->rowArray[rowIndex].startingSegIndex = (EB_U16) SEGMENT_INDEX(rowIndex, bandIndex, segmentsPtr->segmentBandCount);
        bandIndex = BAND_INDEX(picWidthLcu-1, yLast, segmentsPtr->segmentBandCount, segmentsPtr->lcuBandCount);
        segmentsPtr->rowArray[rowIndex].endingSegIndex = (EB_U16) SEGMENT_INDEX(rowIndex, bandIndex, segmentsPtr->segmentBandCount);
        segmentsPtr->rowArray[rowIndex].currentSegIndex = segmentsPtr->rowArray[rowIndex].startingSegIndex;
    }

    // Initialize the per-segment dependency map
    EB_MEMSET(segmentsPtr->depMap.dependencyMap, 0, sizeof(EB_U8) * segmentsPtr->segmentTotalCount);
    for(rowIndex=0; rowIndex < segmentsPtr->segmentRowCount; ++rowIndex) {
        for(segmentIndex=segmentsPtr->rowArray[rowIndex].startingSegIndex; segmentIndex <= segmentsPtr->rowArray[rowIndex].endingSegIndex; ++segmentIndex) {

            // Check that segment is valid
            if(segmentsPtr->validLcuCountArray[segmentIndex]) {
                // Right Neighbor
                if(segmentIndex < segmentsPtr->rowArray[rowIndex].endingSegIndex) {
                    ++segmentsPtr->depMap.dependencyMap[segmentIndex + 1];
                }
                // Bottom Neighbor
                if(rowIndex < segmentsPtr->segmentRowCount - 1 && segmentIndex + segmentsPtr->segmentBandCount >= segmentsPtr->rowArray[rowIndex+1].startingSegIndex) {
                    ++segmentsPtr->depMap.dependencyMap[segmentIndex + segmentsPtr->segmentBandCount];
                }
            }
        }
    }

    return;
}

