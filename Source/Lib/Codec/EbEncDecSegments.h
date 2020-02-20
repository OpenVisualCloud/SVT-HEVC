/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncDecSegments_h
#define EbEncDecSegments_h

#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Defines
 **************************************/
#define ENCDEC_SEGMENTS_MAX_COL_COUNT  60
#define ENCDEC_SEGMENTS_MAX_ROW_COUNT  37
#define ENCDEC_SEGMENTS_MAX_BAND_COUNT ENCDEC_SEGMENTS_MAX_COL_COUNT + ENCDEC_SEGMENTS_MAX_ROW_COUNT
#define ENCDEC_SEGMENTS_MAX_COUNT      ENCDEC_SEGMENTS_MAX_BAND_COUNT * ENCDEC_SEGMENTS_MAX_ROW_COUNT
#define ENCDEC_SEGMENT_INVALID         0xFFFF

/**************************************
 * Macros
 **************************************/
#define BAND_TOTAL_COUNT(lcuRowTotalCount, lcuColTotalCount) \
    ((lcuRowTotalCount) + (lcuColTotalCount) - 1)
#define ROW_INDEX(yLcuIndex, segmentRowCount, lcuRowTotalCount) \
    (((yLcuIndex) * (segmentRowCount)) / (lcuRowTotalCount))
#define BAND_INDEX(xLcuIndex, yLcuIndex, segmentBandCount, lcuBandTotalCount) \
    ((((xLcuIndex) + (yLcuIndex)) * (segmentBandCount)) / (lcuBandTotalCount))
#define SEGMENT_INDEX(rowIndex, bandIndex, segmentBandCount) \
    (((rowIndex) * (segmentBandCount)) + (bandIndex))

/**************************************
 * Member definitions
 **************************************/
typedef struct {
    EB_U8      *dependencyMap;
    EB_HANDLE   updateMutex;
} EncDecSegDependencyMap_t;

typedef struct {
    EB_U16      startingSegIndex;
    EB_U16      endingSegIndex;
    EB_U16      currentSegIndex;
    EB_HANDLE   assignmentMutex;
} EncDecSegSegmentRow_t;

/**************************************
 * ENCDEC Segments
 **************************************/
typedef struct
{
    EbDctor                   dctor;
    EncDecSegDependencyMap_t  depMap;
    EncDecSegSegmentRow_t    *rowArray;

    EB_U16                   *xStartArray;
    EB_U16                   *yStartArray;
    EB_U16                   *validLcuCountArray;

    EB_U32                    segmentBandCount;
    EB_U32                    segmentRowCount;
    EB_U32                    segmentTotalCount;
    EB_U32                    lcuBandCount;
    EB_U32                    lcuRowCount;

    EB_U32                    segmentMaxBandCount;
    EB_U32                    segmentMaxRowCount;
    EB_U32                    segmentMaxTotalCount;

} EncDecSegments_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EncDecSegmentsCtor(
    EncDecSegments_t   *segmentsPtr,
    EB_U32             segmentColCount,
    EB_U32             segmentRowCount);


extern void EncDecSegmentsInit(
    EncDecSegments_t *segmentsPtr,
    EB_U32            colCount,
    EB_U32            rowCount,
    EB_U32            picWidthLcu,
    EB_U32            picHeightLcu);
#ifdef __cplusplus
}
#endif
#endif // EbEncDecResults_h