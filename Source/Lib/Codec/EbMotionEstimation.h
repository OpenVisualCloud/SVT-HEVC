/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimation_h
#define EbMotionEstimation_h

#include "EbTypes.h"
#include "EbCodingUnit.h"

#include "EbMotionEstimationProcess.h"
#include "EbMotionEstimationContext.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#include "EbReferenceObject.h"
#include "EbPictureDecisionProcess.h"
#ifdef __cplusplus
extern "C" {
#endif
extern EB_ERRORTYPE MotionEstimateLcu(
    PictureParentControlSet_t   *pictureControlSetPtr, 
    EB_U32                       lcuIndex,
    EB_U32                       lcuOriginX,
    EB_U32                       lcuOriginY,
    MeContext_t                 *contextPtr,
    EbPictureBufferDesc_t       *inputPtr);

extern EB_ERRORTYPE OpenLoopIntraCandidateSearchLcu(
	PictureParentControlSet_t   *pictureControlSetPtr,
	EB_U32                       lcuIndex,
	MotionEstimationContext_t   *contextPtr,
	EbPictureBufferDesc_t       *inputPtr);

extern void Decimation2D(
    EB_U8                   *inputSamples,
    EB_U32                   inputStride,
    EB_U32                   inputAreaWidth,
    EB_U32                   inputAreaHeight,
    EB_U8                   *decimSamples,
    EB_U32                   decimStride,
    EB_U32                   decimStep);

extern EB_ERRORTYPE OpenLoopIntraSearchLcu(
	PictureParentControlSet_t   *pictureControlSetPtr,
	EB_U32                       lcuIndex,
	MotionEstimationContext_t   *contextPtr,
	EbPictureBufferDesc_t       *inputPtr);

extern void GetMv(
    PictureParentControlSet_t	*pictureControlSetPtr,
    EB_U32						 lcuIndex,
    EB_S32						*xCurrentMv,
    EB_S32						*yCurrentMv);

extern void GetMeDist(
    PictureParentControlSet_t	*pictureControlSetPtr,
    EB_U32						 lcuIndex,
    EB_U32                      *distortion);

EB_S8 Sort3Elements(EB_U32 a, EB_U32 b, EB_U32 c);
#define a_b_c  0
#define a_c_b  1
#define b_a_c  2
#define b_c_a  3
#define c_a_b  4
#define c_b_a  5

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimation_h