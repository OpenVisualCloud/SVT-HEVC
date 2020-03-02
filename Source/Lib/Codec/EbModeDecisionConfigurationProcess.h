/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbModeDecisionConfigurationProcess_h
#define EbModeDecisionConfigurationProcess_h

#include "EbSystemResourceManager.h"
#include "EbMdRateEstimation.h"
#include "EbDefinitions.h"
#include "EbRateControlProcess.h"
#include "EbSequenceControlSet.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Defines
 **************************************/

typedef struct MdcpLocalCodingUnit_s
{
    EB_U64                          earlyCost;
    EB_BOOL                         earlySplitFlag;
    EB_U32                          splitContext;
	EB_BOOL                         slectedCu;
    EB_BOOL                         stopSplit;
} MdcpLocalCodingUnit_t;

typedef struct ModeDecisionConfigurationContext_s
{
    EbDctor                              dctor;
    EbFifo_t                            *rateControlInputFifoPtr;
    EbFifo_t                            *modeDecisionConfigurationOutputFifoPtr;

    MdRateEstimationContext_t           *mdRateEstimationPtr;

    EB_U8                               qp;
    EB_U64                              lambda;
    MdcpLocalCodingUnit_t               localCuArray[CU_MAX_COUNT];

    // Inter depth decision
    EB_U8                               groupOf8x8BlocksCount;
    EB_U8                               groupOf16x16BlocksCount;
	EB_U64                              interComplexityMinimum;
	EB_U64                              interComplexityMaximum;
	EB_U64                              interComplexityAverage;
	EB_U64                              intraComplexityMinimum;
	EB_U64                              intraComplexityMaximum;
	EB_U64                              intraComplexityAverage;
	EB_S16                              minDeltaQpWeight;
	EB_S16                              maxDeltaQpWeight;
	EB_S8                               minDeltaQp[4];
	EB_S8                               maxDeltaQp[4];

    // Budgeting
    EB_U32                             *lcuScoreArray;
    EB_U8	                            costDepthMode[LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE];
    EB_U8                              *lcuCostArray;
    EB_U32                              predictedCost;
    EB_U32                              budget;
    EB_S8                               scoreTh[MAX_SUPPORTED_SEGMENTS];
    EB_U8                               intervalCost[MAX_SUPPORTED_SEGMENTS];
    EB_U8                               numberOfSegments;
    EB_U32                              lcuMinScore;
    EB_U32                              lcuMaxScore;

    EB_ADP_DEPTH_SENSITIVE_PIC_CLASS    adpDepthSensitivePictureClass;

    EB_ADP_REFINEMENT_MODE              adpRefinementMode;

    // Multi - Mode signal(s)
    EB_U8                               adpLevel;
    EB_U8                               chromaQpOffsetLevel;

} ModeDecisionConfigurationContext_t;



/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE ModeDecisionConfigurationContextCtor(
    ModeDecisionConfigurationContext_t  *contextPtr,
    EbFifo_t                            *rateControlInputFifoPtr,

    EbFifo_t                            *modeDecisionConfigurationOutputFifoPtr,
    EB_U16						         lcuTotalCount);


extern void* ModeDecisionConfigurationKernel(void *inputPtr);
#ifdef __cplusplus
}
#endif
#endif // EbModeDecisionConfigurationProcess_h