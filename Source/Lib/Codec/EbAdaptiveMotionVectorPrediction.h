/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAdaptiveMotionVectorPrediction_h
#define EbAdaptiveMotionVectorPrediction_h

#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbNeighborArrays.h"
#include "EbMvMerge.h"
#ifdef __cplusplus
extern "C" {
#endif

struct ModeDecisionContext_s;
struct InterPredictionContext_s;

typedef enum TmvpPos {
    TmvpColocatedBottomRight = 0,
    TmvpColocatedCenter      = 1
} TmvpPos;

// TMVP items corresponding to one LCU
typedef struct TmvpUnit_s {
    Mv_t              mv[MAX_NUM_OF_REF_PIC_LIST][MAX_TMVP_CAND_PER_LCU];
    EB_U64            refPicPOC[MAX_NUM_OF_REF_PIC_LIST][MAX_TMVP_CAND_PER_LCU];
    EB_PREDDIRECTION  predictionDirection[MAX_TMVP_CAND_PER_LCU];
    EB_BOOL              availabilityFlag[MAX_TMVP_CAND_PER_LCU];

    //*Note- list 1 motion info will be added when B-slices are ready

} TmvpUnit_t;

extern EB_ERRORTYPE ClipMV(
    EB_U32                   cuOriginX,
    EB_U32                   cuOriginY,
    EB_S16                  *MVx,
    EB_S16                  *MVy,
    EB_U32                   pictureWidth,
    EB_U32                   pictureHeight,
    EB_U32                   tbSize);

extern EB_BOOL GetTemporalMVP(
    EB_U32                 puPicWiseLocX,
    EB_U32                 puPicWiseLocY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EB_REFLIST             targetRefPicList,
    EB_U64                 targetRefPicPOC,
    TmvpUnit_t            *tmvpMapPtr,
    EB_U64                 colocatedPuPOC,
    EB_REFLIST             preDefinedColocatedPuRefList,
    EB_BOOL                isLowDelay,
    EB_U32                 tbSize,
    EB_S16                *MVPCandx,
    EB_S16                *MVPCandy,
    PictureControlSet_t   *pictureControlSetPtr);

extern EB_BOOL GetTemporalMVPBPicture(
    EB_U32                 puPicWiseLocX,
    EB_U32                 puPicWiseLocY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    PictureControlSet_t  *pictureControlSetPtr,
    TmvpUnit_t             *tmvpMapPtr,
    EB_U64                  colocatedPuPOC,
    EB_REFLIST              preDefinedColocatedPuRefList,
    EB_BOOL                  isLowDelay,
    EB_U32                 tbSize,
    EB_S16                 *MVPCand);

extern EB_ERRORTYPE FillAMVPCandidates(
    NeighborArrayUnit_t    *mvNeighborArray,
    NeighborArrayUnit_t    *modeNeighborArray,
    EB_U32                  originX,
    EB_U32                  originY,
    EB_U32                  width,
    EB_U32                  height,
    EB_U32                  cuSize,
    EB_U32                  cuDepth,
    EB_U32                  lcuSize,
    PictureControlSet_t    *pictureControlSetPtr,
    EB_BOOL                 tmvpEnableFlag,
    EB_U32                  tbAddr,
    EB_REFLIST              targetRefPicList,
    EB_S16                  *xMvAmvpArray,
    EB_S16                  *yMvAmvpArray,
    EB_U8                    *amvpCandidateCount);

extern EB_ERRORTYPE GenerateL0L1AmvpMergeLists(
    struct ModeDecisionContext_s    *contextPtr,
    struct InterPredictionContext_s    *interPredictionPtr,
    PictureControlSet_t                *pictureControlSetPtr,
    EB_BOOL                             tmvpEnableFlag,
    EB_U32                             tbAddr,
    EB_S16                             xMvAmvpArray[2][2],
    EB_S16                             yMvAmvpArray[2][2],
    EB_U32                             amvpCandidateCount[2],
    EB_S16                           firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2],
    EB_S16                           firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2]
    );

extern EB_ERRORTYPE NonConformantGenerateL0L1AmvpMergeLists(
    struct ModeDecisionContext_s    *contextPtr,
    struct InterPredictionContext_s    *interPredictionPtr,
    PictureControlSet_t                *pictureControlSetPtr,
    EB_U32                             tbAddr);

#ifdef __cplusplus
}
#endif
#endif // EbAdaptiveMotionVectorPrediction_h
