/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureDecision_h
#define EbPictureDecision_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Context
 **************************************/
typedef struct PictureDecisionContext_s
{      
    EbFifo_t                     *pictureAnalysisResultsInputFifoPtr;
    EbFifo_t                     *pictureDecisionResultsOutputFifoPtr;
 
    EB_U64                       lastSolidColorFramePoc;

    EB_BOOL						 resetRunningAvg;

    EB_U32 **ahdRunningAvgCb;
    EB_U32 **ahdRunningAvgCr;
    EB_U32 **ahdRunningAvg;
	EB_BOOL		isSceneChangeDetected;

	// Dynamic GOP
	EB_U32	    totalRegionActivityCost[MAX_NUMBER_OF_REGIONS_IN_WIDTH][MAX_NUMBER_OF_REGIONS_IN_HEIGHT];


	EB_U32		totalNumberOfMiniGops;

	EB_U32		miniGopStartIndex[MINI_GOP_WINDOW_MAX_COUNT];
	EB_U32		miniGopEndIndex	 [MINI_GOP_WINDOW_MAX_COUNT];
	EB_U32		miniGopLenght	 [MINI_GOP_WINDOW_MAX_COUNT];
	EB_U32		miniGopIntraCount[MINI_GOP_WINDOW_MAX_COUNT];
	EB_U32		miniGopIdrCount  [MINI_GOP_WINDOW_MAX_COUNT];
	EB_U32		miniGopHierarchicalLevels[MINI_GOP_WINDOW_MAX_COUNT];
	EB_BOOL		miniGopActivityArray	 [MINI_GOP_MAX_COUNT];
	EB_U32		miniGopRegionActivityCostArray[MINI_GOP_MAX_COUNT][MAX_NUMBER_OF_REGIONS_IN_WIDTH][MAX_NUMBER_OF_REGIONS_IN_HEIGHT];

	EB_U32		miniGopGroupFadedInPicturesCount [MINI_GOP_MAX_COUNT];
	EB_U32		miniGopGroupFadedOutPicturesCount[MINI_GOP_MAX_COUNT];
} PictureDecisionContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EB_ERRORTYPE PictureDecisionContextCtor(
    PictureDecisionContext_t    **contextDblPtr,
    EbFifo_t                     *pictureAnalysisResultsInputFifoPtr,
    EbFifo_t                     *pictureDecisionResultsOutputFifoPtr);
    

extern void* PictureDecisionKernel(void *inputPtr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureDecision_h
