/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbCodingLoop_h
#define EbCodingLoop_h

#include "EbDefinitions.h"
#include "EbCodingUnit.h"
#include "EbSequenceControlSet.h"
#include "EbModeDecisionProcess.h"
#include "EbEncDecProcess.h"
#ifdef __cplusplus
extern "C" {
#endif
/*******************************************
 * ModeDecisionLCU
 *   performs CL (LCU)
 *******************************************/

typedef EB_ERRORTYPE(*EB_MODE_DECISION)(
        SequenceControlSet_t                *sequenceControlSetPtr,
        PictureControlSet_t                 *pictureControlSetPtr,
        const MdcLcuData_t * const           mdcResultTbPtr,
        LargestCodingUnit_t                 *lcuPtr,
        EB_U32                               lcuOriginX,
        EB_U32                               lcuOriginY,
        EB_U32                               lcuAddr,
        ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE ModeDecisionLcu(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    const MdcLcuData_t * const           mdcResultTbPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuOriginX,
    EB_U16                               lcuOriginY,
    EB_U32                               lcuAddr,
    ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE  BdpConfiguration(
    PictureControlSet_t                 *pictureControlSetPtr,
    LargestCodingUnit_t                 *lcuPtr,
    LcuParams_t                         *lcuParamPtr,
    ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE BdpPillar(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE Bdp64x64vs32x32RefinementProcess(
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE Bdp16x16vs8x8RefinementProcess(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE BdpMvMergePass(
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr);


extern EB_ERRORTYPE LinkMdtoBdp(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    ModeDecisionContext_t   *contextPtr);

extern EB_ERRORTYPE LinkBdptoMd(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    ModeDecisionContext_t   *contextPtr);


extern EB_ERRORTYPE ModeDecisionRefinementLcu(
    PictureControlSet_t                 *pictureControlSetPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U32                               lcuOriginX,
    EB_U32                               lcuOriginY,
    ModeDecisionContext_t               *contextPtr);

extern EB_ERRORTYPE QpmDeriveWeightsMinAndMax(
    PictureControlSet_t                    *pictureControlSetPtr,
    EncDecContext_t                        *contextPtr);

extern void EncodePass(
    SequenceControlSet_t    *sequenceControlSetPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    EB_U32                   tbAddr,
    EB_U32                   lcuOriginX,
    EB_U32                   lcuOriginY,
    EB_U32                   lcuQp,
    EB_BOOL                  enableSaoFlag,
    EncDecContext_t         *contextPtr);
#ifdef __cplusplus
}
#endif
#endif // EbCodingLoop_h
