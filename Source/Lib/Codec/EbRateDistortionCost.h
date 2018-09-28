/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateDistortionCost_h
#define EbRateDistortionCost_h


/***************************************
 * Includes
 ***************************************/
#include "EbIntraPrediction.h"
#include "EbInterPrediction.h"
#include "EbLambdaRateTables.h"
#include "EbTransforms.h"
#include "EbModeDecisionProcess.h"
#include "EbEncDecProcess.h"  


#ifdef __cplusplus
extern "C" {
#endif
	extern void CodingLoopContextGeneration(
	ModeDecisionContext_t      *contextPtr,
    CodingUnit_t            *cuPtr,
    EB_U32                   cuOriginX,
    EB_U32                   cuOriginY,
    EB_U32                   lcuSize,
    NeighborArrayUnit_t     *intraLumaNeighborArray,
    NeighborArrayUnit_t     *skipFlagNeighborArray,
    NeighborArrayUnit_t     *modeTypeNeighborArray,
    NeighborArrayUnit_t     *leafDepthNeighborArray);

	extern void ModeDecisionRefinementContextGeneration(
	ModeDecisionContext_t      *contextPtr,
    CodingUnit_t            *cuPtr,
    EB_U32                   cuOriginX,
    EB_U32                   cuOriginY,
    EB_U32                   lcuSize,
    NeighborArrayUnit_t     *intraLumaNeighborArray,
    NeighborArrayUnit_t     *modeTypeNeighborArray);

extern EB_ERRORTYPE TuCalcCost(
    EB_U32                   cuSize,
    ModeDecisionCandidate_t *candidatePtr,
    EB_U32                   tuIndex,
    EB_U32                   transformSize,
    EB_U32                   transformChromaSize,
    EB_U32                   yCountNonZeroCoeffs,
    EB_U32                   cbCountNonZeroCoeffs,
    EB_U32                   crCountNonZeroCoeffs,
    EB_U64                   yTuDistortion[DIST_CALC_TOTAL],
    EB_U64                   cbTuDistortion[DIST_CALC_TOTAL],    
    EB_U64                   crTuDistortion[DIST_CALC_TOTAL],   
    EB_U32                   componentMask,
    EB_U64                  *yTuCoeffBits,                        
    EB_U64                  *cbTuCoeffBits,                       
    EB_U64                  *crTuCoeffBits,                       
    EB_U32                   qp,                                  
    EB_U64                   lambda,                              
	EB_U64                   lambdaChroma);


extern EB_ERRORTYPE TuCalcCostLuma(
    EB_U32                   cuSize,
    ModeDecisionCandidate_t *candidatePtr,
    EB_U32                   tuIndex,
    EB_U32                   transformSize,
    EB_U32                   yCountNonZeroCoeffs,
    EB_U64                   yTuDistortion[DIST_CALC_TOTAL],
    EB_U64                  *yTuCoeffBits,                        
    EB_U32                   qp,                                  
    EB_U64                   lambda,                              
    EB_U64                   lambdaChroma);

extern EB_ERRORTYPE IntraLumaModeContext(
    CodingUnit_t                        *cuPtr,
    EB_U32                                  lumaMode,
    EB_S32                                 *predictionIndex);

extern EB_ERRORTYPE Intra2Nx2NFastCostIsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t						  *cuPtr,
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr);

extern EB_ERRORTYPE Intra2Nx2NFastCostIslice(
    CodingUnit_t						  *cuPtr,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
    EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
    EB_U64                                  lambda,
    PictureControlSet_t                    *pictureControlSetPtr);


extern EB_ERRORTYPE Intra2Nx2NFastCostPsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                           *cuPtr,
struct ModeDecisionCandidateBuffer_s        *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr);

extern EB_ERRORTYPE IntraFullCostIslice(
	LargestCodingUnit_t                    *lcuPtr,
    CodingUnit_t                           *cuPtr, 
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
    EB_U32                                  qp,
    EB_U64                                 *yDistortion,
    EB_U64                                 *cbDistortion,
    EB_U64                                 *crDistortion,
    EB_U64                                  lambda,
    EB_U64                                  lambdaChroma,
    EB_U64                                 *yCoeffBits,
    EB_U64                                 *cbCoeffBits,
    EB_U64                                 *crCoeffBits,
    EB_U32                                  transformSize,
    EB_U32                                  transformChromaSize,
	PictureControlSet_t                    *pictureControlSetPtr);

extern EB_ERRORTYPE IntraFullCostPslice(
	LargestCodingUnit_t                    *lcuPtr,
    CodingUnit_t                           *cuPtr, 
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
    EB_U32                                  qp,
    EB_U64                                 *yDistortion,
    EB_U64                                 *cbDistortion,
    EB_U64                                 *crDistortion,
    EB_U64                                  lambda,
    EB_U64                                  lambdaChroma,
    EB_U64                                 *yCoeffBits,
    EB_U64                                 *cbCoeffBits,
    EB_U64                                 *crCoeffBits,
    EB_U32                                  transformSize,
    EB_U32                                  transformChromaSize,
	PictureControlSet_t                    *pictureControlSetPtr);

EB_ERRORTYPE IntraFullLumaCostIslice(
    CodingUnit_t                           *cuPtr,
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
    EB_U64                                 *yDistortion,
    EB_U64                                  lambda,
    EB_U64                                 *yCoeffBits,
    EB_U32                                  transformSize);

EB_ERRORTYPE IntraFullLumaCostPslice(
    CodingUnit_t                           *cuPtr,
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                 *yDistortion,
    EB_U64                                  lambda,
    EB_U64                                 *yCoeffBits,
    EB_U32                                  transformSize);

extern EB_ERRORTYPE InterFastCostPsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                        *cuPtr,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr);

extern EB_ERRORTYPE InterFastCostBsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                           *cuPtr,
struct ModeDecisionCandidateBuffer_s	   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr);

extern EB_ERRORTYPE InterFullCost(
	LargestCodingUnit_t                    *lcuPtr,
    CodingUnit_t                           *cuPtr, 
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
    EB_U32                                  qp,
    EB_U64                                 *yDistortion,
    EB_U64                                 *cbDistortion,
    EB_U64                                 *crDistortion,
    EB_U64                                  lambda,
    EB_U64                                  lambdaChroma,
    EB_U64                                 *yCoeffBits,
    EB_U64                                 *cbCoeffBits,
    EB_U64                                 *crCoeffBits,
    EB_U32                                  transformSize,
    EB_U32                                  transformChromaSize,
	PictureControlSet_t                    *pictureControlSetPtr);

EB_ERRORTYPE InterFullLumaCost(
    CodingUnit_t                           *cuPtr,
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U64                                 *yDistortion,
    EB_U64                                  lambda,
    EB_U64                                 *yCoeffBits,
    EB_U32                                  transformSize);

extern EB_ERRORTYPE  MergeSkipFullCost(

	LargestCodingUnit_t                    *lcuPtr,
    CodingUnit_t                           *cuPtr, 
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
    EB_U32                                  qp,
    EB_U64                                 *yDistortion,
    EB_U64                                 *cbDistortion,
    EB_U64                                 *crDistortion,
    EB_U64                                  lambda,
    EB_U64                                  lambdaChroma,
    EB_U64                                 *yCoeffBits,
    EB_U64                                 *cbCoeffBits,
    EB_U64                                 *crCoeffBits,
    EB_U32                                  transformSize,
    EB_U32                                  transformChromaSize,
	PictureControlSet_t                    *pictureControlSetPtr);

EB_ERRORTYPE  MergeSkipFullLumaCost(
    CodingUnit_t                           *cuPtr, 
    EB_U32                                  cuSize,
    EB_U32                                  cuSizeLog2,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U64                                 *yDistortion,
    EB_U64                                  lambda,
    EB_U64                                 *yCoeffBits,
    EB_U32                                  transformSize);

extern EB_ERRORTYPE SplitFlagRate(
	ModeDecisionContext_t               *contextPtr,
    CodingUnit_t                           *cuPtr,
    EB_U32                                  splitFlag,
    EB_U64                                 *splitRate,
    EB_U64                                  lambda,
    MdRateEstimationContext_t              *mdRateEstimationPtr,
    EB_U32                                  tbMaxDepth);

extern EB_ERRORTYPE EncodeTuCalcCost(	
    EncDecContext_t          *contextPtr,
	EB_U32                   *countNonZeroCoeffs,
	EB_U64                    yTuDistortion[DIST_CALC_TOTAL],
	EB_U64                   *yTuCoeffBits,
	EB_U32                    componentMask
	);



extern EB_ERRORTYPE Intra4x4FastCostIslice(
    ModeDecisionContext_t                  *contextPtr,
    EB_U32                                  puIndex,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
    EB_U64                                  lumaDistortion,
    EB_U64                                  lambda);
    
extern EB_ERRORTYPE Intra4x4FastCostPslice(
    ModeDecisionContext_t                  *contextPtr,
    EB_U32                                  puIndex,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
    EB_U64                                  lumaDistortion,
    EB_U64                                  lambda);

extern EB_ERRORTYPE Intra4x4FullCostIslice(
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
    EB_U64                                 *yDistortion,
    EB_U64                                  lambda,
    EB_U64                                 *yCoeffBits,
    EB_U32                                  transformSize);
    
extern EB_ERRORTYPE Intra4x4FullCostPslice(
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
    EB_U64                                 *yDistortion,
    EB_U64                                  lambda,
    EB_U64                                 *yCoeffBits,
    EB_U32                                  transformSize);

EB_ERRORTYPE IntraNxNFastCostIslice(
	CodingUnit_t                           *cuPtr,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr);
EB_ERRORTYPE IntraNxNFastCostPslice(
	CodingUnit_t                           *cuPtr,
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr);

EB_ERRORTYPE IntraNxNFullCostIslice(
	PictureControlSet_t                    *pictureControlSetPtr,
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                 *yDistortion,
	EB_U64                                 *cbDistortion,
	EB_U64                                 *crDistortion,
	EB_U64                                  lambda,
	EB_U64                                  lambdaChroma,
	EB_U64                                 *yCoeffBits,
	EB_U64                                 *cbCoeffBits,
	EB_U64                                 *crCoeffBits,
	EB_U32                                  transformSize);

EB_ERRORTYPE IntraNxNFullCostPslice(
	PictureControlSet_t                    *pictureControlSetPtr,
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                 *yDistortion,
	EB_U64                                 *cbDistortion,
	EB_U64                                 *crDistortion,
	EB_U64                                  lambda,
	EB_U64                                  lambdaChroma,
	EB_U64                                 *yCoeffBits,
	EB_U64                                 *cbCoeffBits,
	EB_U64                                 *crCoeffBits,
	EB_U32                                  transformSize);


#ifdef __cplusplus
}
#endif
#endif //EbRateDistortionCost_h
