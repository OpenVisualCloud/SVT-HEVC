/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInterPrediction_h
#define EbInterPrediction_h

#include "EbDefinitions.h"

#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbModeDecision.h"
#include "EbMcp.h"
#include "EbMvMerge.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MVBOUNDLOW    36    //  (80-71)<<2 // 80 = ReferencePadding ; minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
#define MVBOUNDHIGH   348   //  (80+7)<<2
#define REFPADD_QPEL  320   //  (16+64)<<2

	void RoundMvOnTheFly(
		EB_S16 *motionVector_x,
		EB_S16 *motionVector_y);

struct ModeDecisionContext_s;

typedef struct InterPredictionContext_s {
    EbDctor                                 dctor;
    // mcp context
    MotionCompensationPredictionContext_t  *mcpContext;

    // MV Merge
    EB_U32                 mvMergeCandidateCount;
    MvMergeCandidate_t     mvMergeCandidateArray[MAX_NUM_OF_MV_MERGE_CANDIDATE];

} InterPredictionContext_t;

extern EB_ERRORTYPE InterPredictionContextCtor(
	InterPredictionContext_t  *contextPtr,
	EB_U16                     maxCUWidth,
    EB_U16                     maxCUHeight,
    EB_BOOL                    is16bit);


extern EB_ERRORTYPE Inter2Nx2NPuPredictionInterpolationFree(
    struct ModeDecisionContext_s           *contextPtr,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
	ModeDecisionCandidateBuffer_t          *candidateBufferPtr);

EB_ERRORTYPE Inter2Nx2NPuPredictionHevc(
    struct ModeDecisionContext_s           *contextPtr,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr);



extern EB_ERRORTYPE EncodePassInterPrediction(
    MvUnit_t                               *mvUnit,
	EB_U16                                  puOriginX,
	EB_U16                                  puOriginY,
	EB_U8                                   puWidth,
	EB_U8                                   puHeight,
    PictureControlSet_t                    *pictureControlSetPtr,
    EbPictureBufferDesc_t                  *predictionPtr,
    MotionCompensationPredictionContext_t  *mcpContext);

extern EB_ERRORTYPE EncodePassInterPrediction16bit(
    MvUnit_t                               *mvUnit,
	EB_U16                                  puOriginX,
	EB_U16                                  puOriginY,
	EB_U8                                   puWidth,
	EB_U8                                   puHeight,
    PictureControlSet_t                    *pictureControlSetPtr,
    EbPictureBufferDesc_t                  *predictionPtr,
    MotionCompensationPredictionContext_t  *mcpContext);

EB_ERRORTYPE ChooseMVPIdx_V2(
    ModeDecisionCandidate_t  *candidatePtr,
    EB_U32                    cuOriginX,
    EB_U32                    cuOriginY,
    EB_U32                    puIndex,
    EB_U32                    tbSize,
    EB_S16                   *ref0AMVPCandArray_x,
    EB_S16                   *ref0AMVPCandArray_y,
    EB_U32                    ref0NumAvailableAMVPCand,
    EB_S16                   *ref1AMVPCandArray_x,
    EB_S16                   *ref1AMVPCandArray_y,
    EB_U32                    ref1NumAvailableAMVPCand,
    PictureControlSet_t      *pictureControlSetPtr);
#ifdef __cplusplus
}
#endif
#endif //EbInterPrediction_h
