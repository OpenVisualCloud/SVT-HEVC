/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbFullLoop_h
#define EbFullLoop_h

#include "EbModeDecisionProcess.h"
#ifdef __cplusplus
extern "C" {
#endif

    void FullLoop_R(
        LargestCodingUnit_t            *lcuPtr,
        ModeDecisionCandidateBuffer_t  *candidateBuffer,
        ModeDecisionContext_t          *contextPtr,
        const CodedUnitStats_t         *cuStatsPtr,
        EbPictureBufferDesc_t          *inputPicturePtr,
        PictureControlSet_t            *pictureControlSetPtr,
        EB_U32                          componentMask,
        EB_U32                          cbQp,
        EB_U32                          crQp,
        EB_U32                           *cbCountNonZeroCoeffs,
        EB_U32                           *crCountNonZeroCoeffs);

    void CuFullDistortionFastTuMode_R(
        EbPictureBufferDesc_t          *inputPicturePtr,
        EB_U32                          inputCbOriginIndex,
        LargestCodingUnit_t            *lcuPtr,
        ModeDecisionCandidateBuffer_t  *candidateBuffer,
        ModeDecisionContext_t            *contextPtr,
        ModeDecisionCandidate_t           *candidatePtr,
        const CodedUnitStats_t           *cuStatsPtr,
        EB_U64                          cbFullDistortion[DIST_CALC_TOTAL],
        EB_U64                          crFullDistortion[DIST_CALC_TOTAL],
        EB_U32                          countNonZeroCoeffs[3][MAX_NUM_OF_TU_PER_CU],
        EB_U32                            componentMask,
        EB_U64                         *cbCoeffBits,
        EB_U64                         *crCoeffBits);

    void ProductFullLoop(
        EbPictureBufferDesc_t         *inputPicturePtr,
        EB_U32                         inputOriginIndex,
        ModeDecisionCandidateBuffer_t  *candidateBuffer,
        ModeDecisionContext_t          *contextPtr,
        const CodedUnitStats_t         *cuStatsPtr,
        PictureControlSet_t            *pictureControlSetPtr,
        EB_U32                          qp,
        EB_U32                           *yCountNonZeroCoeffs,
        EB_U64                         *yCoeffBits,
        EB_U64                         *yFullDistortion);

    extern EB_U32 ProductPerformInterDepthDecision(
        ModeDecisionContext_t          *contextPtr,
        EB_U32                          leafIndex,
        LargestCodingUnit_t            *tbPtr,
        EB_U32                          lcuAddr,
        EB_U32                          tbOriginX,
        EB_U32                          tbOriginY,
        EB_U64                          fullLambda,
        MdRateEstimationContext_t      *mdRateEstimationPtr,
        PictureControlSet_t            *pictureControlSetPtr);

    EB_U32 ExitInterDepthDecision(
        ModeDecisionContext_t          *contextPtr,
        EB_U32                          leafIndex,
        LargestCodingUnit_t            *tbPtr,
        EB_U32                          lcuAddr,
        EB_U32                          tbOriginX,
        EB_U32                          tbOriginY,
        EB_U64                          fullLambda,
        MdRateEstimationContext_t      *mdRateEstimationPtr,
        PictureControlSet_t            *pictureControlSetPtr);

    extern EB_U32 PillarInterDepthDecision(
        ModeDecisionContext_t          *contextPtr,
        EB_U32                          leafIndex,
        LargestCodingUnit_t            *tbPtr,
        EB_U32                          tbOriginX,
        EB_U32                          tbOriginY,
        EB_U64                          fullLambda,
        MdRateEstimationContext_t      *mdRateEstimationPtr,
        PictureControlSet_t            *pictureControlSetPtr);

#ifdef __cplusplus
}
#endif
#endif // EbFullLoop_h