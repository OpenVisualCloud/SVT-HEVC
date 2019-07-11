/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
* Includes
***************************************/

#include "EbRateDistortionCost.h"

#include "EbModeDecisionConfiguration.h"

static const EB_U32 skipFlagBits[NUMBER_OF_SKIP_FLAG_CASES] = { 17878, 62157, 86651, 54723, 14816, 8254 };
static const EB_U32 mergeIndexBits[6] = { 10350, 109741, 142509, 175277, 175277 };
static const EB_U32 interBiDirBits[8] = { 29856, 36028, 15752, 59703, 8692, 84420, 2742, 136034 };
static const EB_U32 interUniDirBits[2] = { 2742, 136034 };
static const EB_U32 mvpIndexBits[2] = { 23196, 44891 };

#define WEIGHT_FACTOR_FOR_AURA_CU   4 

EB_ERRORTYPE  MergeSkipFullLumaCost(
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuSize,
	EB_U32                                  cuSizeLog2,
	ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize);

/*********************************************************************************
* apply Weight on Chroma Distortion
**********************************************************************************/
EB_U64 getWeightedChromaDistortion(
    PictureControlSet_t                    *pictureControlSetPtr,
    EB_U64                                  chromaDistortion,
    EB_U32                                  qp)
{

    EB_U64 weightedDistortion;
    if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
        // Random Access
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            weightedDistortion = (((chromaDistortion * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            weightedDistortion = (((chromaDistortion * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else {
            weightedDistortion = (((chromaDistortion * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
    }
    else {
        // Low delay
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            weightedDistortion = (((chromaDistortion * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else {
            weightedDistortion = (((chromaDistortion * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
    }

    return weightedDistortion;
}

/************************************************************
* Coding Loop Context Generation
************************************************************/
void CodingLoopContextGeneration(
	ModeDecisionContext_t      *contextPtr,
	CodingUnit_t            *cuPtr,
	EB_U32                   cuOriginX,
	EB_U32                   cuOriginY,
	EB_U32                   lcuSize,
	NeighborArrayUnit_t     *intraLumaNeighborArray,
	NeighborArrayUnit_t     *skipFlagNeighborArray,
	NeighborArrayUnit_t     *modeTypeNeighborArray,
	NeighborArrayUnit_t     *leafDepthNeighborArray)
{
	EB_U32 modeTypeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		modeTypeNeighborArray,
		cuOriginY);
	EB_U32 modeTypeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		modeTypeNeighborArray,
		cuOriginX);
	EB_U32 leafDepthLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		leafDepthNeighborArray,
		cuOriginY);
	EB_U32 leafDepthTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		leafDepthNeighborArray,
		cuOriginX);
	EB_U32 skipFlagLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		skipFlagNeighborArray,
		cuOriginY);
	EB_U32 skipFlagTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		skipFlagNeighborArray,
		cuOriginX);
	EB_U32 intraLumaModeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		intraLumaNeighborArray,
		cuOriginY);
	EB_U32 intraLumaModeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		intraLumaNeighborArray,
		cuOriginX);

	// Intra Luma Neighbor Modes
	(&cuPtr->predictionUnitArray[0])->intraLumaLeftMode = (EB_U32)(
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		(EB_U32)intraLumaNeighborArray->leftArray[intraLumaModeLeftNeighborIndex]);
	(&cuPtr->predictionUnitArray[0])->intraLumaTopMode = (EB_U32)(
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		((cuOriginY & (lcuSize - 1)) == 0) ? EB_INTRA_DC :      // If we are at the top of the LCU boundary, then
		(EB_U32)intraLumaNeighborArray->topArray[intraLumaModeTopNeighborIndex]);                      //   use DC. This seems like we could use a LCU-width
	//   Top Intra Mode Neighbor Array instead of a Full
	// Skip Flag Context
	cuPtr->skipFlagContext =
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] == (EB_U8)INVALID_MODE) ? 0 :
		(skipFlagNeighborArray->leftArray[skipFlagLeftNeighborIndex] == EB_TRUE) ? 1 : 0;
	cuPtr->skipFlagContext +=
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] == (EB_U8)INVALID_MODE) ? 0 :
		(skipFlagNeighborArray->topArray[skipFlagTopNeighborIndex] == EB_TRUE) ? 1 : 0;

	// Split Flag Context (neighbor info)
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].leftNeighborMode = modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex];
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].leftNeighborDepth = leafDepthNeighborArray->leftArray[leafDepthLeftNeighborIndex];
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].topNeighborMode = modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex];
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].topNeighborDepth     = leafDepthNeighborArray->topArray[leafDepthTopNeighborIndex];

	return;
}

void ModeDecisionRefinementContextGeneration(
	ModeDecisionContext_t               *contextPtr,
	CodingUnit_t            *cuPtr,
	EB_U32                   cuOriginX,
	EB_U32                   cuOriginY,
	EB_U32                   lcuSize,
	NeighborArrayUnit_t     *intraLumaNeighborArray,
	NeighborArrayUnit_t     *modeTypeNeighborArray)
{
	EB_U32 modeTypeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		modeTypeNeighborArray,
		cuOriginY);
	EB_U32 modeTypeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		modeTypeNeighborArray,
		cuOriginX);

	EB_U32 intraLumaModeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		intraLumaNeighborArray,
		cuOriginY);
	EB_U32 intraLumaModeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		intraLumaNeighborArray,
		cuOriginX);

	// Intra Luma Neighbor Modes
	cuPtr->predictionUnitArray[0].intraLumaLeftMode = (EB_U32)(
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		(EB_U32)intraLumaNeighborArray->leftArray[intraLumaModeLeftNeighborIndex]);
	cuPtr->predictionUnitArray[0].intraLumaTopMode = (EB_U32)(
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		((cuOriginY & (lcuSize - 1)) == 0) ? EB_INTRA_DC :      // If we are at the top of the LCU boundary, then
		(EB_U32)intraLumaNeighborArray->topArray[intraLumaModeTopNeighborIndex]);                      //   use DC. This seems like we could use a LCU-width

	// Split Flag Context (neighbor info)
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].leftNeighborMode = modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex];
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].topNeighborMode = modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex];

	return;
}

/********************************************
* TuCalcCost
*   computes TU Cost and generetes TU Cbf
********************************************/

EB_ERRORTYPE TuCalcCost(
	EB_U32                   cuSize,
	ModeDecisionCandidate_t *candidatePtr,                        // input parameter, prediction result Ptr
	EB_U32                   tuIndex,                             // input parameter, TU index inside the CU
	EB_U32                   transformSize,
	EB_U32                   transformChromaSize,
	EB_U32                   yCountNonZeroCoeffs,                 // input parameter, number of non zero Y quantized coefficients
	EB_U32                   cbCountNonZeroCoeffs,                // input parameter, number of non zero cb quantized coefficients
	EB_U32                   crCountNonZeroCoeffs,                // input parameter, number of non zero cr quantized coefficients
	EB_U64                   yTuDistortion[DIST_CALC_TOTAL],      // input parameter, Y distortion for both Normal and Cbf zero modes
	EB_U64                   cbTuDistortion[DIST_CALC_TOTAL],     // input parameter, Cb distortion for both Normal and Cbf zero modes
	EB_U64                   crTuDistortion[DIST_CALC_TOTAL],     // input parameter, Cr distortion for both Normal and Cbf zero modes
	EB_U32                   componentMask,
	EB_U64                  *yTuCoeffBits,                        // input parameter, Y quantized coefficients rate
	EB_U64                  *cbTuCoeffBits,                       // input parameter, Cb quantized coefficients rate
	EB_U64                  *crTuCoeffBits,                       // input parameter, Cr quantized coefficients rate
	EB_U32                   qp,                                  // input parameter, Cr quantized coefficients rate
	EB_U64                   lambda,                              // input parameter, lambda for Luma
	EB_U64                   lambdaChroma)                        // input parameter, lambda for Chroma
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbfCtx = ((cuSize == transformSize));

	// Non Zero Cbf mode variables
	EB_U64 yNonZeroCbfDistortion = yTuDistortion[DIST_CALC_RESIDUAL];

	EB_U64 yNonZeroCbfLumaFlagBitsNum = 0;

	EB_U64 yNonZeroCbfRate;

	EB_U64 yNonZeroCbfCost = 0;

	// Zero Cbf mode variables
	EB_U64 yZeroCbfDistortion = yTuDistortion[DIST_CALC_PREDICTION];

	EB_U64 yZeroCbfLumaFlagBitsNum = 0;

	EB_U64 yZeroCbfRate;

	EB_U64 yZeroCbfCost = 0;

	// Luma and chroma transform size shift for the distortion
	(void)lambdaChroma;
	(void)qp;
	(void)crTuCoeffBits;
	(void)cbTuCoeffBits;
	(void)cbTuDistortion;
	(void)crTuDistortion;
	(void)transformChromaSize;


	// **Compute distortion


	if (componentMask & PICTURE_BUFFER_DESC_Y_FLAG) {

		// Non Zero Distortion
		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
		yNonZeroCbfDistortion = LUMA_WEIGHT * (yNonZeroCbfDistortion << COST_PRECISION);

		// Zero distortion
		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
		yZeroCbfDistortion = LUMA_WEIGHT * (yZeroCbfDistortion << COST_PRECISION);

		// **Compute Rate

		// Estimate Cbf's Bits

		yNonZeroCbfLumaFlagBitsNum = candidatePtr->mdRateEstimationPtr->lumaCbfBits[(NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
		yZeroCbfLumaFlagBitsNum = candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfCtx];


		yNonZeroCbfRate = ((*yTuCoeffBits) << 15) + yNonZeroCbfLumaFlagBitsNum;
		yZeroCbfRate = yZeroCbfLumaFlagBitsNum;

		if (candidatePtr->type == INTRA_MODE) {

			yZeroCbfCost = 0xFFFFFFFFFFFFFFFFull;

		}
		else {

			yZeroCbfCost = yZeroCbfDistortion + (((lambda       * yZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);

		}

		// **Compute Cost
		yNonZeroCbfCost = yNonZeroCbfDistortion + (((lambda       * yNonZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);

		candidatePtr->yCbf |= (((yCountNonZeroCoeffs != 0) && (yNonZeroCbfCost  < yZeroCbfCost)) << tuIndex);
		*yTuCoeffBits = (yNonZeroCbfCost  < yZeroCbfCost) ? *yTuCoeffBits : 0;
		yTuDistortion[DIST_CALC_RESIDUAL] = (yNonZeroCbfCost  < yZeroCbfCost) ? yTuDistortion[DIST_CALC_RESIDUAL] : yTuDistortion[DIST_CALC_PREDICTION];
	}

	if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
		candidatePtr->cbCbf |= ((cbCountNonZeroCoeffs != 0) << tuIndex);
	}

	if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
		candidatePtr->crCbf |= ((crCountNonZeroCoeffs != 0) << tuIndex);
	}

	return return_error;
}

/********************************************
* TuCalcCost
*   computes TU Cost and generetes TU Cbf
********************************************/

EB_ERRORTYPE TuCalcCostLuma(
	EB_U32                   cuSize,
	ModeDecisionCandidate_t *candidatePtr,                        // input parameter, prediction result Ptr
	EB_U32                   tuIndex,                             // input parameter, TU index inside the CU 
	EB_U32                   transformSize,
	EB_U32                   yCountNonZeroCoeffs,                 // input parameter, number of non zero Y quantized coefficients
	EB_U64                   yTuDistortion[DIST_CALC_TOTAL],      // input parameter, Y distortion for both Normal and Cbf zero modes
	EB_U64                  *yTuCoeffBits,                        // input parameter, Y quantized coefficients rate
	EB_U32                   qp,                                  // input parameter, Cr quantized coefficients rate
	EB_U64                   lambda,                              // input parameter, lambda for Luma
	EB_U64                   lambdaChroma)                        // input parameter, lambda for Chroma
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbfCtx = ((cuSize == transformSize));

	// Non Zero Cbf mode variables
	EB_U64 yNonZeroCbfDistortion = yTuDistortion[DIST_CALC_RESIDUAL];

	EB_U64 yNonZeroCbfLumaFlagBitsNum = 0;

	EB_U64 yNonZeroCbfRate;

	EB_U64 yNonZeroCbfCost = 0;

	// Zero Cbf mode variables
	EB_U64 yZeroCbfDistortion = yTuDistortion[DIST_CALC_PREDICTION];

	EB_U64 yZeroCbfLumaFlagBitsNum = 0;

	EB_U64 yZeroCbfRate;

	EB_U64 yZeroCbfCost = 0;

	// Luma and chroma transform size shift for the distortion

	(void)lambdaChroma;
	(void)qp;

	// **Compute distortion
	// Non Zero Distortion   
	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula 
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	yNonZeroCbfDistortion = LUMA_WEIGHT * (yNonZeroCbfDistortion << COST_PRECISION);

	// Zero distortion
	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula 
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	yZeroCbfDistortion = LUMA_WEIGHT * (yZeroCbfDistortion << COST_PRECISION);

	// **Compute Rate

	// Estimate Cbf's Bits

	yNonZeroCbfLumaFlagBitsNum = candidatePtr->mdRateEstimationPtr->lumaCbfBits[(NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
	yZeroCbfLumaFlagBitsNum = candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfCtx];
	yNonZeroCbfRate = ((*yTuCoeffBits) << 15) + yNonZeroCbfLumaFlagBitsNum;
	yZeroCbfRate = yZeroCbfLumaFlagBitsNum;

	if (candidatePtr->type == INTRA_MODE){

		yZeroCbfCost = 0xFFFFFFFFFFFFFFFFull;

	}
	else{

		yZeroCbfCost = yZeroCbfDistortion + (((lambda       * yZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);

	}

	// **Compute Cost
	yNonZeroCbfCost = yNonZeroCbfDistortion + (((lambda       * yNonZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);

	candidatePtr->yCbf |= (((yCountNonZeroCoeffs != 0) && (yNonZeroCbfCost  < yZeroCbfCost)) << tuIndex);
	*yTuCoeffBits = (yNonZeroCbfCost  < yZeroCbfCost) ? *yTuCoeffBits : 0;
	yTuDistortion[DIST_CALC_RESIDUAL] = (yNonZeroCbfCost  < yZeroCbfCost) ? yTuDistortion[DIST_CALC_RESIDUAL] : yTuDistortion[DIST_CALC_PREDICTION];

	return return_error;
}

/*********************************************************************************
* IntraLumaModeContext function is used to check the intra luma mode of neighbor
* PUs and generate the required information for the intra luma mode rate estimation
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param lumaMode(input)
*       lumaMode is the candidate luma mode.
*   @param predictionIndex (output)
*       predictionIndex -1 means neither of the neighboring PUs hase the same
*       intra mode as the current PU.
**********************************************************************************/
EB_ERRORTYPE IntraLumaModeContext(
	CodingUnit_t                        *cuPtr,
	EB_U32                                  lumaMode,
	EB_S32                                 *predictionIndex)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 lumaPredictionArray[3];
	EB_U32 leftNeighborMode = (&cuPtr->predictionUnitArray[0])->intraLumaLeftMode;
	EB_U32 topNeighborMode  = (&cuPtr->predictionUnitArray[0])->intraLumaTopMode;

	if (leftNeighborMode == topNeighborMode) {
		if (leftNeighborMode > 1) { // For angular modes
			lumaPredictionArray[0] = leftNeighborMode;
			lumaPredictionArray[1] = ((leftNeighborMode + 29) & 0x1F) + 2;
			lumaPredictionArray[2] = ((leftNeighborMode - 1) & 0x1F) + 2;
		}
		else { // Non Angular modes
			lumaPredictionArray[0] = EB_INTRA_PLANAR;
			lumaPredictionArray[1] = EB_INTRA_DC;
			lumaPredictionArray[2] = EB_INTRA_VERTICAL;
		}
	}
	else {
		lumaPredictionArray[0] = leftNeighborMode;
		lumaPredictionArray[1] = topNeighborMode;

		if (leftNeighborMode && topNeighborMode) {
			lumaPredictionArray[2] = EB_INTRA_PLANAR; // when both modes are non planar
		}
		else {
			lumaPredictionArray[2] = (leftNeighborMode + topNeighborMode) < 2 ? EB_INTRA_VERTICAL : EB_INTRA_DC;
		}
	}

	*predictionIndex = (lumaMode == lumaPredictionArray[0]) ? 0 :
		(lumaMode == lumaPredictionArray[1]) ? 1 :
		(lumaMode == lumaPredictionArray[2]) ? 2 :
		-1; // luma mode is not equal to any of the predictors

	return return_error;
}

/*********************************************************************************
* Intra2Nx2NFastCost function is used to estimate the cost of an intra candidate mode
* for fast mode decision module in I slice.
* Chroma cost is excluded from fast cost functions. Only the fastChromaRate is stored
* for future use in full loop
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE Intra2Nx2NFastCostIsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                           *cuPtr,
struct ModeDecisionCandidateBuffer_s       *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	ModeDecisionCandidate_t		*candidatePtr = candidateBufferPtr->candidatePtr;
	EB_U32 lumaMode = candidatePtr->intraLumaMode;

	// Luma and chroma rate
	EB_U64 rate;
	EB_U64 lumaRate = 0;
	EB_U64 chromaRate;
	EB_U64 lumaSad, chromaSad;

	// Luma and chroma distortion
	EB_U64 totalDistortion;

    (void) pictureControlSetPtr;

	// Estimate Chroma Mode Bits
	chromaRate = 9732; // candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraChromaBits[chromaMode];
	// Estimate Partition Size Bits :
    lumaRate = contextPtr->cuStats->depth == 3 ? 24752 : ZERO_COST;
	// Estimate Luma Mode Bits for Intra
	lumaRate += (lumaMode == (&cuPtr->predictionUnitArray[0])->intraLumaLeftMode || lumaMode == (&cuPtr->predictionUnitArray[0])->intraLumaTopMode) ? 57520 : 206378;
	// Keep the Fast Luma and Chroma rate for future use
	candidatePtr->fastLumaRate = lumaRate;
	candidatePtr->fastChromaRate = chromaRate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;

	// include luma only in total distortion
	
	lumaSad = (LUMA_WEIGHT * lumaDistortion) << COST_PRECISION;
	chromaSad = (((chromaDistortion * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT); // Low delay and Random access have the same value of chroma weight
	totalDistortion = lumaSad + chromaSad;

	// include luma only in rate calculation
	rate = ((lambda * (lumaRate + chromaRate)) + MD_OFFSET) >> MD_SHIFT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	return return_error;
}

EB_ERRORTYPE Intra2Nx2NFastCostIslice(
	CodingUnit_t                           *cuPtr,
   struct ModeDecisionCandidateBuffer_s     *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 lumaMode = candidateBufferPtr->candidatePtr->intraLumaMode;
	EB_S32 predictionIndex = -1;
	// Number of bits for each synatax element
    EB_U64 partSizeIntraBitsNum = 0;
	EB_U64 intraLumaModeBitsNum = 0;

	// Luma and chroma rate
	EB_U64 rate;
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	EB_U64 lumaSad, chromaSad;

	// Luma and chroma distortion
	EB_U64 totalDistortion;
	// Estimate Chroma Mode Bits
	chromaRate = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraChromaBits[EB_INTRA_CHROMA_DM];

	// Estimate Partition Size Bits :
	partSizeIntraBitsNum = (GetCodedUnitStats(cuPtr->leafIndex)->depth == (((SequenceControlSet_t *)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->maxLcuDepth - 1)) ?
	candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraPartSizeBits[SIZE_2Nx2N] :
		ZERO_COST;

	// Estimate Luma Mode Bits for Intra
	IntraLumaModeContext(
		cuPtr,
		lumaMode,
		&predictionIndex);

	intraLumaModeBitsNum = (predictionIndex != -1) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[predictionIndex] :
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[3];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate =
		partSizeIntraBitsNum +
		intraLumaModeBitsNum;

	// Keep the Fast Luma and Chroma rate for future use
	candidateBufferPtr->candidatePtr->fastLumaRate = lumaRate;
	candidateBufferPtr->candidatePtr->fastChromaRate = chromaRate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;
    lumaSad = (LUMA_WEIGHT * lumaDistortion) << COST_PRECISION;
    chromaSad = (((chromaDistortion * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT); // Low delay and Random access have the same value of chroma weight
    totalDistortion = lumaSad + chromaSad;

	// include luma only in rate calculation
	rate = ((lambda * (lumaRate + chromaRate)) + MD_OFFSET) >> MD_SHIFT;

	// Scale the rate by the total (*Note this is experimental)
	rate *= RATE_WEIGHT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	return return_error;
}


/*********************************************************************************
* Intra2Nx2NFastCost function is used to estimate the cost of an intra candidate mode
* for fast mode decision module in P slice.
* Chroma cost is excluded from fast cost functions. Only the fastChromaRate is stored
* for future use in full loop
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE Intra2Nx2NFastCostPsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                           *cuPtr,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	ModeDecisionCandidate_t		*candidatePtr = candidateBufferPtr->candidatePtr;

	// Luma and chroma rate
	EB_U64 rate;
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	EB_U64 lumaSad, chromaSad;
	EB_U32 lumaMode = candidatePtr->intraLumaMode;
	// Luma and chroma distortion
	EB_U64 totalDistortion;


	// Estimate Chroma Mode Bits
	chromaRate = 12368; // mdRateEstimationPtr->intraChromaBits[chromaMode];
	// Estimate Partition Size Bits 
    lumaRate = contextPtr->cuStats->depth == 3 ? 31523 : ZERO_COST;

	// Estimate Pred Mode Bits
	lumaRate += 136034; // mdRateEstimationPtr->predModeBits[candidateType];
	//// Estimate Luma Mode Bits for Intra
	lumaRate += (lumaMode == (&cuPtr->predictionUnitArray[0])->intraLumaLeftMode || lumaMode == (&cuPtr->predictionUnitArray[0])->intraLumaTopMode) ? 72731 : 192228;


	// Keep the Fast Luma and Chroma rate for future use
	candidatePtr->fastLumaRate = lumaRate;
	candidatePtr->fastChromaRate = chromaRate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;

	// include luma only in total distortion
	lumaSad = (LUMA_WEIGHT * lumaDistortion) << COST_PRECISION;
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode
	if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
		// Random Access
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            chromaSad = (((chromaDistortion * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            chromaSad = (((chromaDistortion * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else {
            chromaSad = (((chromaDistortion * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }

	}
	else {
		// Low delay
		if (pictureControlSetPtr->temporalLayerIndex == 0) {
			chromaSad = (((chromaDistortion * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
		else {
			chromaSad = (((chromaDistortion * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
	}

	totalDistortion = lumaSad + chromaSad;

	// include luma only in rate calculation
	rate = ((lambda * (lumaRate + chromaRate)) + MD_OFFSET) >> MD_SHIFT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	return return_error;
}

/*********************************************************************************
* IntraFullCostIslice function is used to estimate the cost of an intra candidate mode
* for full mode decision module in EB_I_PICTURE.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE IntraFullCostIslice(
	LargestCodingUnit_t                    *lcuPtr,
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuSize,
	EB_U32                                  cuSizeLog2,
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
	EB_U32                                  transformSize,
	EB_U32                                  transformChromaSize,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
	EB_U32 yCbfBlock;
	EB_U32 yCbfCtx = ((cuSize == transformSize));
	EB_U32 crCbf = candidateBufferPtr->candidatePtr->crCbf;
	EB_U32 crCbfBlock;
	EB_U32 cbCbf = candidateBufferPtr->candidatePtr->cbCbf;
	EB_U32 cbCbfBlock;
	EB_U32 chromaCbfCtx = cuSizeLog2 - Log2f(transformSize);

	EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
	EB_U32 transSubDivFlagCtx;

	EB_U64 tranSubDivFlagBitsNum = 0;

	EB_U64 cbfLumaFlagBitsNum = 0;
	EB_U64 cbfChromaFlagBitsNum = 0;

	EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
	EB_U32 tuCount;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	EB_U64 coeffRate;
	// EB_U64 lumaCoeffRate;

	// Luma and chroma distortion
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;
	EB_U64 chromaSse;

	(void)pictureControlSetPtr;
	(void)cuPtr;
	(void)transformChromaSize;
    (void)lcuPtr;
	transSubDivFlagCtx = 5 - Log2f(transformSize);
	// Rate Estimation of each syntax element

	// Estimate the Transform Split Flag & the Cbf's Bits
	for (tuCount = 0; tuCount < transformBlockCount; ++tuCount) {

		tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

		cbCbfBlock = ((cbCbf)& (1 << tuCount)) > 0;
		crCbfBlock = ((crCbf)& (1 << tuCount)) > 0;
		yCbfBlock = ((yCbf)& (1 << tuCount)) > 0;

		if (transformBlockCount > 1)
			yCbfCtx = 0;
		else
			yCbfCtx = 1;

		cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];

		cbfChromaFlagBitsNum +=
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];

	}

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;
	chromaRate = cbfChromaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;
	chromaRate += candidateBufferPtr->candidatePtr->fastChromaRate;

	// Coeff rate
	coeffRate = (*yCoeffBits + *cbCoeffBits + *crCoeffBits) << 15;
	//lumaCoeffRate = (*yCoeffBits) << 15;

	lumaSse = yDistortion[0];
	chromaSse = cbDistortion[0] + crDistortion[0];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode
	chromaSse = (((chromaSse * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT); // Low delay and Random access have the same value of chroma weight


	distortion = lumaSse + chromaSse;

	// Assign full cost
    *candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate + lambdaChroma * chromaRate) + MD_OFFSET) >> MD_SHIFT);

	candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - distortion;
    coeffRate = (*yCoeffBits) << 15;
    candidateBufferPtr->fullCostLuma = lumaSse + (((lambda * coeffRate + lambda * lumaRate ) + MD_OFFSET) >> MD_SHIFT);

	return return_error;
}

/*********************************************************************************
* IntraFullCost function is used to estimate the cost of an intra candidate mode
* for full mode decision module.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE IntraFullCostPslice(
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
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
	EB_U32 yCbfBlock;
	EB_U32 yCbfCtx = ((cuSize == transformSize));
	EB_U32 crCbf = candidateBufferPtr->candidatePtr->crCbf;
	EB_U32 crCbfBlock;
	EB_U32 cbCbf = candidateBufferPtr->candidatePtr->cbCbf;
	EB_U32 cbCbfBlock;
	EB_U32 chromaCbfCtx = cuSizeLog2 - Log2f(transformSize);

	EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
	EB_U32 transSubDivFlagCtx;

	EB_U64 tranSubDivFlagBitsNum = 0;

	EB_U64 cbfLumaFlagBitsNum = 0;
	EB_U64 cbfChromaFlagBitsNum = 0;

	EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
	EB_U32 tuCount;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	EB_U64 coeffRate;
	//EB_U64 lumaCoeffRate;

	// Luma and chroma distortion
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;
	EB_U64 chromaSse;

	(void)cuPtr;
	(void)transformChromaSize;

	transSubDivFlagCtx = 5 - Log2f(transformSize);
	// Rate Estimation of each syntax element

	// Estimate the Transform Split Flag & the Cbf's Bits
	for (tuCount = 0; tuCount < transformBlockCount; ++tuCount) {

		tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

		cbCbfBlock = ((cbCbf)& (1 << tuCount)) > 0;
		crCbfBlock = ((crCbf)& (1 << tuCount)) > 0;
		yCbfBlock = ((yCbf)& (1 << tuCount)) > 0;

		if (transformBlockCount > 1)
			yCbfCtx = 0;
		else
			yCbfCtx = 1;

		cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];

		cbfChromaFlagBitsNum +=
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];

	}

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;
	chromaRate = cbfChromaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;
	chromaRate += candidateBufferPtr->candidatePtr->fastChromaRate;

	// Coeff rate
	coeffRate = (*yCoeffBits + *cbCoeffBits + *crCoeffBits) << 15;
	//lumaCoeffRate = (*yCoeffBits) << 15;

	lumaSse = yDistortion[0];
	chromaSse = cbDistortion[0] + crDistortion[0];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode
	if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
		// Random Access
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            chromaSse = (((chromaSse * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            chromaSse = (((chromaSse * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else {
            chromaSse = (((chromaSse * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
	}
	else {
		// Low delay
		if (pictureControlSetPtr->temporalLayerIndex == 0) {
			chromaSse = (((chromaSse * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
		else {
			chromaSse = (((chromaSse * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
	}

	distortion = lumaSse + chromaSse;

	// Assign full cost
    *candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate + lambdaChroma * chromaRate) + MD_OFFSET) >> MD_SHIFT);

	candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - distortion;
   
    (void)lcuPtr;

    coeffRate = (*yCoeffBits) << 15;
    candidateBufferPtr->fullCostLuma = lumaSse + (((lambda * coeffRate + lambda * lumaRate ) + MD_OFFSET) >> MD_SHIFT);


	return return_error;
}

/*********************************************************************************
* IntraFullCostIslice function is used to estimate the cost of an intra candidate mode
* for full mode decision module in EB_I_PICTURE.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE IntraFullLumaCostIslice(
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuSize,
	EB_U32                                  cuSizeLog2,
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
	EB_U32 yCbfBlock;
	EB_U32 yCbfCtx = ((cuSize == transformSize));
	EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
	EB_U32 transSubDivFlagCtx;

	EB_U64 tranSubDivFlagBitsNum = 0;

	EB_U64 cbfLumaFlagBitsNum = 0;

	EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
	EB_U32 tuCount;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 coeffRate;

	// Luma and chroma distortion
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;

	(void)cuPtr;

	transSubDivFlagCtx = 5 - Log2f(transformSize);
	// Rate Estimation of each syntax element

	// Estimate the Transform Split Flag & the Cbf's Bits
	for (tuCount = 0; tuCount < transformBlockCount; ++tuCount) {

		tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

		yCbfBlock = ((yCbf)& (1 << tuCount)) > 0;

		if (transformBlockCount > 1)
			yCbfCtx = 0;
		else
			yCbfCtx = 1;

		cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
	}

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;

	// Coeff rate
	coeffRate = *yCoeffBits << 15;

	// luma distortion
	lumaSse = yDistortion[0];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	distortion = lumaSse;

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - distortion;


	return return_error;
}

/*********************************************************************************
* IntraFullCost function is used to estimate the cost of an intra candidate mode
* for full mode decision module.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE IntraFullLumaCostPslice(
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuSize,
	EB_U32                                  cuSizeLog2,
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
	EB_U32 yCbfBlock;
	EB_U32 yCbfCtx = ((cuSize == transformSize));
	EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
	EB_U32 transSubDivFlagCtx;

	EB_U64 tranSubDivFlagBitsNum = 0;

	EB_U64 cbfLumaFlagBitsNum = 0;

	EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
	EB_U32 tuCount;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 coeffRate;

	// Luma and chroma distortion
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;

	(void)cuPtr;

	transSubDivFlagCtx = 5 - Log2f(transformSize);
	// Rate Estimation of each syntax element

	// Estimate the Transform Split Flag & the Cbf's Bits
	for (tuCount = 0; tuCount < transformBlockCount; ++tuCount) {

		tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

		yCbfBlock = ((yCbf)& (1 << tuCount)) > 0;

		if (transformBlockCount > 1)
			yCbfCtx = 0;
		else
			yCbfCtx = 1;

		cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
	}

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;

	// Coeff rate
	coeffRate = *yCoeffBits << 15;

	// luma distortion
	lumaSse = yDistortion[0];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	distortion = lumaSse;

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - distortion;


	return return_error;
}

/*********************************************************************************
* InterFastCostPsliceOpt function is used to estimate the cost of an inter candidate mode
* for fast mode decision module in P Slice.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the inter candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE InterFastCostPsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                           *cuPtr,
	ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE  return_error = EB_ErrorNone;
	EB_U64        lumaSad, chromaSad;
	EB_U64        lumaRate;
	EB_U64        distortion;

	ModeDecisionCandidate_t *candidatePtr = candidateBufferPtr->candidatePtr;
    (void)contextPtr;

	if (candidatePtr->mergeFlag == EB_TRUE){
		EB_U32 mergeIndex = candidatePtr->mergeIndex;
		// Rate
		EB_U64 mergeSkiplumaRate;
		mergeSkiplumaRate = skipFlagBits[(NUMBER_OF_SKIP_FLAG_CASES >> 1) + cuPtr->skipFlagContext];

		{
			mergeSkiplumaRate += mergeIndexBits[mergeIndex];
		}


		// *Note- store the fast rate to avoid the recomputation of the rate of each syntax element
		// the full cost module
		candidatePtr->fastLumaRate = mergeSkiplumaRate;
		candidatePtr->fastChromaRate = ZERO_COST;
		if (candidateBufferPtr->weightChromaDistortion == EB_TRUE){

			chromaDistortion = getWeightedChromaDistortion(pictureControlSetPtr, chromaDistortion, qp);
			distortion = (((LUMA_WEIGHT * lumaDistortion)) << COST_PRECISION) + chromaDistortion;

		}
		else{
			distortion = ((LUMA_WEIGHT * lumaDistortion) + chromaDistortion) << COST_PRECISION;
		}
		candidateBufferPtr->residualLumaSad = lumaDistortion;

		// Assign fast cost
		*candidateBufferPtr->fastCostPtr = distortion + (((lambda * mergeSkiplumaRate) + MD_OFFSET) >> MD_SHIFT);

	}
	else{


		EB_U8            amvpIdx;
		EB_S32           predRefX;
		EB_S32           predRefY;
		EB_S32           mvRefX;
		EB_S32           mvRefY;

		// Estimate Syntax Bits

		lumaRate = 86440; // mergeFlagBits + skipFlagBits + predModeBits + interPartSizeBits;


		amvpIdx = candidatePtr->motionVectorPredIdx[REF_LIST_0];
		predRefX = candidatePtr->motionVectorPred_x[REF_LIST_0];
		predRefY = candidatePtr->motionVectorPred_y[REF_LIST_0];
		mvRefX = candidatePtr->motionVector_x_L0;
		mvRefY = candidatePtr->motionVector_y_L0;

			

		EB_S32 mvdX = EB_ABS_DIFF(predRefX, mvRefX);
		EB_S32 mvdY = EB_ABS_DIFF(predRefY, mvRefY);


		mvdX = mvdX > 499 ? 499 : mvdX;
		mvdY = mvdY > 499 ? 499 : mvdY;
		lumaRate += mvBitTable[mvdX][mvdY];
		lumaRate += mvpIndexBits[amvpIdx];

	
		
		// *Note- store the fast rate to avoid the recomputation of the rate of each syntax element
		// the full cost module
		candidatePtr->fastLumaRate = lumaRate;
		candidatePtr->fastChromaRate = ZERO_COST;

		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (6 * PSNRy + PSNRu + PSNRv) / 8.  This approximate weighting
		//  should be used in the cost calc.
		lumaSad = (LUMA_WEIGHT * lumaDistortion) << COST_PRECISION;
		//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode
		if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
			// Random Access
            if (pictureControlSetPtr->temporalLayerIndex == 0) {
                chromaSad = (((chromaDistortion * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                chromaSad = (((chromaDistortion * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
            else {
                chromaSad = (((chromaDistortion * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
		}
		else {
			// Low delay
			if (pictureControlSetPtr->temporalLayerIndex == 0) {
				chromaSad = (((chromaDistortion * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
			}
			else {
				chromaSad = (((chromaDistortion * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
			}
		}

		distortion = lumaSad + chromaSad;

		candidateBufferPtr->residualLumaSad = lumaDistortion;

		// overwrite lumaDistortion with the shifted value to be used in cost function
		lumaDistortion = ((LUMA_WEIGHT * lumaDistortion) << COST_PRECISION);

		// Assign fast cost
		*candidateBufferPtr->fastCostPtr = distortion + (((lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	}
	return return_error;
}

/*********************************************************************************
* InterFastCostBslice function is used to estimate the cost of an inter candidate mode
* for fast mode decision module in B Slice.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the inter candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE InterFastCostBsliceOpt(
    struct ModeDecisionContext_s           *contextPtr,
	CodingUnit_t                           *cuPtr,
	ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE  return_error = EB_ErrorNone;
	ModeDecisionCandidate_t *candidatePtr = candidateBufferPtr->candidatePtr;
    // Luma rate
    EB_U64           lumaRate;   
	EB_U64           distortion;                                        // Luma and chroma distortion
	EB_U64           lumaSad, chromaSad;
	if (candidatePtr->mergeFlag == EB_TRUE){

		EB_U32 mergeIndex = candidatePtr->mergeIndex;
		// Rate
		EB_U64 mergeSkiplumaRate;
		mergeSkiplumaRate = skipFlagBits[(NUMBER_OF_SKIP_FLAG_CASES >> 1) + cuPtr->skipFlagContext];

		{
			mergeSkiplumaRate += mergeIndexBits[mergeIndex];
		}


		// *Note- store the fast rate to avoid the recomputation of the rate of each syntax element
		// the full cost module
		candidatePtr->fastLumaRate = mergeSkiplumaRate;
		candidatePtr->fastChromaRate = ZERO_COST;
		if (candidateBufferPtr->weightChromaDistortion == EB_TRUE){

			chromaDistortion = getWeightedChromaDistortion(pictureControlSetPtr, chromaDistortion, qp);
			distortion = (((LUMA_WEIGHT * lumaDistortion)) << COST_PRECISION) + chromaDistortion;

		}
		else{
			distortion = ((LUMA_WEIGHT * lumaDistortion) + chromaDistortion) << COST_PRECISION;
		}
		candidateBufferPtr->residualLumaSad = lumaDistortion;

		// Assign fast cost
		*candidateBufferPtr->fastCostPtr = distortion + (((lambda * mergeSkiplumaRate) + MD_OFFSET) >> MD_SHIFT);

	}
	else{

		const EB_U32     puIndex = 0;
		EB_U8            amvpIdx;
		EB_S32           predRefX;
		EB_S32           predRefY;
		EB_S32           mvRefX;
		EB_S32           mvRefY;
		EB_PREDDIRECTION predDirection = candidatePtr->predictionDirection[puIndex];
		EB_BOOL          biPred = (EB_BOOL)(predDirection == BI_PRED);

		// Estimate Syntax Bits

		lumaRate = 86440; // mergeFlagBits + skipFlagBits + predModeBits + interPartSizeBits;
        lumaRate += interBiDirBits[(contextPtr->cuStats->depth << 1) + biPred];
		
		if (predDirection < 2 && predDirection != BI_PRED) {
			
			lumaRate += interUniDirBits[predDirection];

			if (predDirection == UNI_PRED_LIST_0){
				
				amvpIdx = candidatePtr->motionVectorPredIdx[REF_LIST_0];
				predRefX = candidatePtr->motionVectorPred_x[REF_LIST_0];
				predRefY = candidatePtr->motionVectorPred_y[REF_LIST_0];
				mvRefX = candidatePtr->motionVector_x_L0;
				mvRefY = candidatePtr->motionVector_y_L0;

			}
			else{
				amvpIdx = candidatePtr->motionVectorPredIdx[REF_LIST_1];
				predRefX = candidatePtr->motionVectorPred_x[REF_LIST_1];
				predRefY = candidatePtr->motionVectorPred_y[REF_LIST_1];
				mvRefX = candidatePtr->motionVector_x_L1;
				mvRefY = candidatePtr->motionVector_y_L1;

			}

			EB_S32 mvdX = EB_ABS_DIFF(predRefX, mvRefX);
			EB_S32 mvdY = EB_ABS_DIFF(predRefY, mvRefY);


			mvdX = mvdX > 499 ? 499 : mvdX;
			mvdY = mvdY > 499 ? 499 : mvdY;
			lumaRate += mvBitTable[mvdX][mvdY];
			lumaRate += mvpIndexBits[amvpIdx];
			
		}
		else{
			
			// LIST 0 Rate Estimation
			amvpIdx = candidatePtr->motionVectorPredIdx[REF_LIST_0];
			predRefX = candidatePtr->motionVectorPred_x[REF_LIST_0];
			predRefY = candidatePtr->motionVectorPred_y[REF_LIST_0];
			mvRefX = candidatePtr->motionVector_x_L0;
			mvRefY = candidatePtr->motionVector_y_L0;

			EB_S32 mvdX = EB_ABS_DIFF(predRefX, mvRefX);
			EB_S32 mvdY = EB_ABS_DIFF(predRefY, mvRefY);
			mvdX = mvdX > 499 ? 499 : mvdX;
			mvdY = mvdY > 499 ? 499 : mvdY;
			lumaRate += mvBitTable[mvdX][mvdY];
			lumaRate += mvpIndexBits[amvpIdx];

			// LIST 1 Rate Estimation
			amvpIdx = candidatePtr->motionVectorPredIdx[REF_LIST_1];
			predRefX = candidatePtr->motionVectorPred_x[REF_LIST_1];
			predRefY = candidatePtr->motionVectorPred_y[REF_LIST_1];
			mvRefX = candidatePtr->motionVector_x_L1;
			mvRefY = candidatePtr->motionVector_y_L1;

			mvdX = EB_ABS_DIFF(predRefX, mvRefX);
			mvdY = EB_ABS_DIFF(predRefY, mvRefY);
			mvdX = mvdX > 499 ? 499 : mvdX;
			mvdY = mvdY > 499 ? 499 : mvdY;

			lumaRate += mvBitTable[mvdX][mvdY];
			lumaRate += mvpIndexBits[amvpIdx];

			
		}

		// *Note- store the fast rate to avoid the recomputation of the rate of each syntax element
		// the full cost module
		candidateBufferPtr->residualLumaSad = lumaDistortion;
		candidatePtr->fastLumaRate = lumaRate;
		candidatePtr->fastChromaRate = ZERO_COST;

		lumaSad = (LUMA_WEIGHT * lumaDistortion) << COST_PRECISION;
		//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode
		if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
			// Random Access
            if (pictureControlSetPtr->temporalLayerIndex == 0) {
                chromaSad = (((chromaDistortion * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                chromaSad = (((chromaDistortion * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
            else {
                chromaSad = (((chromaDistortion * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
		}
		else {
			// Low delay
			if (pictureControlSetPtr->temporalLayerIndex == 0) {
				chromaSad = (((chromaDistortion * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
			}
			else {
				chromaSad = (((chromaDistortion * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
			}
		}

		distortion = lumaSad + chromaSad;


		// Assign fast cost
		*candidateBufferPtr->fastCostPtr = distortion + (((lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	}
	return return_error;
}

/************************************
******* EstimateTuFlags
**************************************/
EB_ERRORTYPE EstimateTuFlags(
	CodingUnit_t                    *cuPtr,
	EB_U32                          cuSize,
	EB_U32                          cuSizeLog2,
	ModeDecisionCandidateBuffer_t   *candidateBufferPtr,
	EB_U64                          *tranSubDivFlagBitsNum,
	EB_U64                          *cbfLumaFlagBitsNum,
	EB_U64                          *cbfChromaFlagBitsNum)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32                      subDivContext;
	EB_U32                      tuIndex;
	EB_U32                      tuIndexDepth2;
	EB_U32                      tuTotalCount;


	TransformUnit_t            *tuPtr;
	const TransformUnitStats_t *tuStatPtr;
	EB_U32                      tuItr;
	EB_U32                      tuSize;
	EB_U32                      parentTuIndex;

	EB_U32                      cbfContext;

	ModeDecisionCandidate_t     *candidatePtr = candidateBufferPtr->candidatePtr;

	*tranSubDivFlagBitsNum = 0;
	*cbfLumaFlagBitsNum = 0;
	*cbfChromaFlagBitsNum = 0;

	// Set TU variables
	if (cuSize == MAX_LCU_SIZE){
		tuTotalCount = 4;
		tuIndex = 1;
		tuItr = 0;
		tuPtr = &cuPtr->transformUnitArray[0];
		tuPtr->splitFlag = EB_TRUE;
		tuPtr->cbCbf = EB_FALSE;
		tuPtr->crCbf = EB_FALSE;
		tuPtr->chromaCbfContext = 0; //at TU level 
	}
	else {
		tuTotalCount = 1;
		tuIndex = 0;
		tuItr = 0;
	}

	// Set TU
	do {
		tuStatPtr = GetTransformUnitStats(tuIndex);
		tuSize = cuSize >> tuStatPtr->depth;
		tuPtr = &cuPtr->transformUnitArray[tuIndex];
		parentTuIndex = 0;
		if (tuStatPtr->depth > 0)
			parentTuIndex = tuIndexList[tuStatPtr->depth - 1][(tuItr >> 2)];

		tuPtr->splitFlag = EB_FALSE;
		tuPtr->lumaCbf = (EB_BOOL)(((candidatePtr->yCbf)  & (1 << tuIndex))   > 0);
		tuPtr->cbCbf = (EB_BOOL)(((candidatePtr->cbCbf) & (1 << (tuIndex))) > 0);
		tuPtr->crCbf = (EB_BOOL)(((candidatePtr->crCbf) & (1 << (tuIndex))) > 0);
		tuPtr->chromaCbfContext = (tuIndex == 0) ? 0 : (cuSizeLog2 - Log2f(tuSize)); //at TU level 
		tuPtr->lumaCbfContext = (cuSizeLog2 - Log2f(tuSize)) == 0 ? 1 : 0;

		if (tuPtr->cbCbf){
			cuPtr->transformUnitArray[0].cbCbf = EB_TRUE;
			cuPtr->transformUnitArray[parentTuIndex].cbCbf = EB_TRUE;
		}
		if (tuPtr->crCbf){
			cuPtr->transformUnitArray[0].crCbf = EB_TRUE;
			cuPtr->transformUnitArray[parentTuIndex].crCbf = EB_TRUE;
		}

		++tuItr;
		tuIndex = tuIndexList[tuStatPtr->depth][tuItr];

	} while (tuItr < tuTotalCount);


	tuPtr = &cuPtr->transformUnitArray[0];
	tuStatPtr = GetTransformUnitStats(0);
	tuSize = cuSize >> tuStatPtr->depth;
	cbfContext = tuPtr->chromaCbfContext;
	subDivContext = 5 - Log2f(tuSize);

	if (cuSize != 64) {
		// Encode split flag 
		*tranSubDivFlagBitsNum +=
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[(EB_U32)(tuPtr->splitFlag) * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + subDivContext];
	}

	if (tuPtr->splitFlag) {

		*cbfChromaFlagBitsNum +=
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext] +
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
		for (tuIndex = 1; tuIndex < 5; tuIndex += 1) {
			tuPtr = &cuPtr->transformUnitArray[tuIndex];
			tuStatPtr = GetTransformUnitStats(tuIndex);
			tuSize = cuSize >> tuStatPtr->depth;


			if (GetCodedUnitStats(cuPtr->leafIndex)->size != 8) {
				subDivContext = 5 - Log2f(tuSize);
				// Encode split flag 
				*tranSubDivFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[(EB_U32)(tuPtr->splitFlag) * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + subDivContext];
			}

			if (tuPtr->splitFlag) {
				cbfContext = tuPtr->chromaCbfContext;
				if ((cuPtr->transformUnitArray[0].cbCbf) != 0){
					// Cb CBF  
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				if ((cuPtr->transformUnitArray[0].crCbf) != 0){
					// Cr CBF  
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];

				}

				// Transform depth 2
				//tuIndexDepth2 = tuIndex*4 + 1;

				tuIndexDepth2 = tuIndex + 1;
                tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				cbfContext = tuPtr->lumaCbfContext;
				// Luma CBF
				*cbfLumaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[tuPtr->lumaCbf  * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];

				tuIndexDepth2++;
                tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;
				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				cbfContext = tuPtr->lumaCbfContext;

				// Luma CBF
				*cbfLumaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[tuPtr->lumaCbf  * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];

				tuIndexDepth2++;
                tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				cbfContext = tuPtr->lumaCbfContext;

				// Luma CBF
				*cbfLumaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[tuPtr->lumaCbf  * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];


				tuIndexDepth2++;
                tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				cbfContext = tuPtr->lumaCbfContext;

				// Luma CBF
				*cbfLumaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[tuPtr->lumaCbf  * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];

			}
			else {

				cbfContext = tuPtr->chromaCbfContext;

				// Cb CBF  
				if ((cuPtr->transformUnitArray[0].cbCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[0].crCbf) && (tuSize != 8)){
					*cbfChromaFlagBitsNum +=
						candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
				}

				cbfContext = tuPtr->lumaCbfContext;

				// Luma CBF
				*cbfLumaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[tuPtr->lumaCbf  * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
			}
		}

	}
	else {

		//  Cb CBF
		*cbfChromaFlagBitsNum +=
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];

		// Cr CBF  
		*cbfChromaFlagBitsNum +=
			candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(tuPtr->crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];

		// Luma CBF

		// In the Inter case, if the RootCbf is 1 and the Chroma Cbfs are 0, then we can infer that the
		// luma Cbf is true, so there is no need to code it.
		if (tuPtr->cbCbf || tuPtr->crCbf) {

			//cbfContext = ((cuPtr->size == tuPtr->size) || (tuPtr->size == TRANSFORM_MAX_SIZE));
			cbfContext = tuPtr->lumaCbfContext;

			*cbfLumaFlagBitsNum +=
				candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[(tuPtr->lumaCbf>0)  * (NUMBER_OF_CBF_CASES >> 1) + cbfContext];
		}
	}

	return return_error;
}

/*********************************************************************************
* InterFullCost function is used to estimate the cost of an inter candidate mode
* for full mode decision module in P or B Slices.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the inter candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE InterFullCost(
	LargestCodingUnit_t					   *lcuPtr,
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
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE  return_error = EB_ErrorNone;

	if (candidateBufferPtr->candidatePtr->mergeFlag == EB_TRUE){
		MergeSkipFullCost(
			lcuPtr,
			cuPtr,
			cuSize,
			cuSizeLog2,
			candidateBufferPtr,
			qp,
			yDistortion,
			cbDistortion,
			crDistortion,
			lambda,
			lambdaChroma,
			yCoeffBits,
			cbCoeffBits,
			crCoeffBits,
			transformSize,
			transformChromaSize,
			pictureControlSetPtr);

	}
	else{

		// Full Rate Estimation
		EB_U32 rootCbf = candidateBufferPtr->candidatePtr->rootCbf;


		EB_U64 rootCbfBitsNum = 0;
		EB_U64 tranSubDivFlagBitsNum = 0;

		EB_U64 cbfLumaFlagBitsNum = 0;
		EB_U64 cbfChromaFlagBitsNum = 0;

		EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
		EB_U32 yCbfBlock;
		EB_U32 yCbfCtx;

		EB_U32 crCbf = candidateBufferPtr->candidatePtr->crCbf;
		EB_U32 crCbfBlock;
		EB_U32 cbCbf = candidateBufferPtr->candidatePtr->cbCbf;
		EB_U32 cbCbfBlock;
		EB_U32 chromaCbfCtx;


		EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
		EB_U32 transSubDivFlagCtx;

		EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
		EB_U32 tuCount;
		EB_U32 tuIndex;
		chromaCbfCtx = cuSizeLog2 - Log2f(transformSize);
		transSubDivFlagCtx = 5 - Log2f(transformSize);

		// Luma and chroma rate
		EB_U64 lumaRate;
		EB_U64 chromaRate;
		EB_U64 coeffRate;
		//EB_U64 lumaCoeffRate;

		// Luma and chroma distortion
		EB_U64 distortion;

		// Luma and chroma SSE
		EB_U64 lumaSse;
		EB_U64 chromaSse;

		// Rate Estimation of each syntax element
		// Estimate Root Cbf (Combined Y, Cb, & Cr Cbfs) Bits
		rootCbfBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->rootCbfBits[rootCbf];

		(void)cuPtr;
		(void)cuSize;
		(void)transformChromaSize;

		// If the Root Cbf is on
		if (rootCbf) {
			if (transformBlockCount > 1) { //(cuPtr->size == MAX_LCU_SIZE)

				chromaCbfCtx = 0;
				//yCbfCtx      = cuSizeLog2 - Log2f(transformSize);
				cbfChromaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];

				for (tuCount = 0, tuIndex = 1; tuCount < transformBlockCount; ++tuCount) {

					tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

					cbCbfBlock = ((cbCbf)& (1 << tuIndex)) > 0;
					crCbfBlock = ((crCbf)& (1 << tuIndex)) > 0;
					yCbfBlock = ((yCbf)& (1 << tuIndex)) > 0;
					yCbfCtx = 0;
					chromaCbfCtx = cuSizeLog2 - Log2f(transformSize);

					cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
					cbfChromaFlagBitsNum += (crCbf > 0) ? candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] : 0;
					cbfChromaFlagBitsNum += (cbCbf > 0) ? candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] : 0;
					tuIndex += 1;

				}
			}
			else {
				// Estimate the Transform Split Flag & the Cbf's Bits

				tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

				cbCbfBlock = (cbCbf > 0);
				crCbfBlock = (crCbf > 0);
				yCbfBlock = (yCbf  > 0);

				if (cbCbf || crCbf) {
					yCbfCtx = 1;
					cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
				}

				cbfChromaFlagBitsNum +=
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
					candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];
			}

		}

		// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
		lumaRate = rootCbfBitsNum + tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;
		chromaRate = cbfChromaFlagBitsNum;

		// Add fast rate to get the total rate of the subject mode
		lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;
		chromaRate += candidateBufferPtr->candidatePtr->fastChromaRate;

		// Coeff rate
		coeffRate = (*yCoeffBits + *cbCoeffBits + *crCoeffBits) << 15;
		//lumaCoeffRate = (*yCoeffBits) << 15;

		// Compute Cost
		lumaSse = yDistortion[0];
		chromaSse = cbDistortion[0] + crDistortion[0];


		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
		lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

		// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
		//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode

		if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
			// Random Access
            if (pictureControlSetPtr->temporalLayerIndex == 0) {
                chromaSse = (((chromaSse * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                chromaSse = (((chromaSse * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
            else {
                chromaSse = (((chromaSse * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
            }
		}
		else {
			// Low delay
			if (pictureControlSetPtr->temporalLayerIndex == 0) {
				chromaSse = (((chromaSse * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
			}
			else {
				chromaSse = (((chromaSse * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
			}
		}

		distortion = lumaSse + chromaSse;

		// Assign full cost
        *candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate + lambdaChroma * chromaRate) + MD_OFFSET) >> MD_SHIFT);

		candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - distortion;

	}
	return return_error;
}

/*********************************************************************************
* InterFullCost function is used to estimate the cost of an inter candidate mode
* for full mode decision module in P or B Slices.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the inter candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE InterFullLumaCost(
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuSize,
	EB_U32                                  cuSizeLog2,
	ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE  return_error = EB_ErrorNone;
	if (candidateBufferPtr->candidatePtr->mergeFlag == EB_TRUE){
		MergeSkipFullLumaCost(
			cuPtr,
			cuSize,
			cuSizeLog2,
			candidateBufferPtr,
			yDistortion,
			lambda,
			yCoeffBits,
			transformSize);

	}
	else{

		// Full Rate Estimation
		EB_U32 rootCbf = candidateBufferPtr->candidatePtr->rootCbf;

		EB_U64 rootCbfBitsNum = 0;
		EB_U64 tranSubDivFlagBitsNum = 0;

		EB_U64 cbfLumaFlagBitsNum = 0;
		EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
		EB_U32 yCbfBlock;
		EB_U32 yCbfCtx;
		EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
		EB_U32 transSubDivFlagCtx;
		EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
		EB_U32 tuCount;
		EB_U32 tuIndex;

		transSubDivFlagCtx = 5 - Log2f(transformSize);

		// Luma and chroma rate
		EB_U64 lumaRate;
		EB_U64 coeffRate;

		// Luma and chroma distortion
		EB_U64 distortion;

		// Luma and chroma SSE
		EB_U64 lumaSse;

		// Rate Estimation of each syntax element
		// Estimate Root Cbf (Combined Y, Cb, & Cr Cbfs) Bits
		rootCbfBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->rootCbfBits[rootCbf];

		(void)cuPtr;
		(void)cuSize;

		// If the Root Cbf is on
		if (rootCbf) {
			if (transformBlockCount > 1) { //(cuPtr->size == MAX_LCU_SIZE)

				for (tuCount = 0, tuIndex = 1; tuCount < transformBlockCount; ++tuCount) {

					tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

					yCbfBlock = ((yCbf)& (1 << tuIndex)) > 0;

					yCbfCtx = 0;
					cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
					tuIndex += 1 ;
				}
			}
			else {
				// Estimate the Transform Split Flag & the Cbf's Bits

				tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

				yCbfBlock = (yCbf  > 0);

				yCbfCtx = 1;
				cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];

			}

		}

		// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
		lumaRate = rootCbfBitsNum + tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

		// Add fast rate to get the total rate of the subject mode
		lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;

		// Coeff rate
		coeffRate = *yCoeffBits << 15;

		// Compute Cost
		lumaSse = yDistortion[0];

		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
		lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

		distortion = lumaSse;

		// Assign full cost
		*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

		candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - distortion;


	}
	return return_error;
}

/*********************************************************************************
* MergeSkipFullCost function is used to estimate the cost of an AMVPSkip candidate
* mode for full mode decision module.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the inter candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE  MergeSkipFullCost(
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
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE  return_error = EB_ErrorNone;


	// Pred Mode Bits Table
	// 0 for Inter
	// 1 for Intra
	EB_MODETYPE candidateType = candidateBufferPtr->candidatePtr->type == INTER_MODE ? 0 : 1;
	//EB_U32 mergeIndex = candidateBufferPtr->candidatePtr->mergeIndex[0];
	EB_U32 mergeIndex = candidateBufferPtr->candidatePtr->mergeIndex;
	EB_U32 rootCbf = candidateBufferPtr->candidatePtr->rootCbf;

	EB_U64 skipFlagBitsNum = 0;
	EB_U64 mergeFlagBitsNum = 0;
	EB_U64 partSizeIntraBitsNum = 0;
	EB_U64 predModeBitsNum = 0;
	EB_U64 mergeIndexBitsNum = 0;
	EB_U64 tranSubDivFlagBitsNum = 0;
	EB_U64 cbfLumaFlagBitsNum = 0;
	EB_U64 cbfChromaFlagBitsNum = 0;

	EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
	EB_U32 yCbfBlock;
	EB_U32 yCbfCtx;

	EB_U32 crCbf = candidateBufferPtr->candidatePtr->crCbf;
	EB_U32 crCbfBlock;
	EB_U32 cbCbf = candidateBufferPtr->candidatePtr->cbCbf;
	EB_U32 cbCbfBlock;
	EB_U32 chromaCbfCtx;

	EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
	EB_U32 transSubDivFlagCtx;
	EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
	chromaCbfCtx = cuSizeLog2 - Log2f(transformSize);
	transSubDivFlagCtx = 5 - Log2f(transformSize);
	EB_U32 tuCount;
	EB_U32 tuIndex;

	// Merge
	EB_U64 mergeLumaRate;
	EB_U64 mergeChromaRate;
	EB_U64 mergeDistortion;
	EB_U64 mergeCost;
	//EB_U64 mergeLumaCost;
	EB_U64 mergeLumaSse;
	EB_U64 mergeChromaSse;
	EB_U64 coeffRate;
	//EB_U64 lumaCoeffRate;

	// SKIP
	EB_U64 skipDistortion;
	EB_U64 skipCost;
	//EB_U64 skipLumaCost;

	// Luma and chroma transform size shift for the distortion
	EB_U64 skipLumaSse;
	EB_U64 skipChromaSse;

	(void)transformSize;
	(void)transformChromaSize;
	(void)cuSize;
    (void)lcuPtr;
	// Rate Estimation of each syntax element

	// Estimate Skip Flag Bits
	skipFlagBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->skipFlagBits[SPLIT_FLAG_ZERO * (NUMBER_OF_SKIP_FLAG_CASES >> 1) + cuPtr->skipFlagContext];

	// Estimate Merge Flag Bits
	mergeFlagBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->mergeFlagBits[1];

	// Estimate Pred Mode Bits
	predModeBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->predModeBits[candidateType];

	// Estimate Partition Size Bits :
	partSizeIntraBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->interPartSizeBits[SIZE_2Nx2N];

	 {
		 mergeIndexBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->mergeIndexBits[mergeIndex];
	}

    //mergeIndexBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->mergeIndexBits[mergeIndex];

	// Estimate Root Cbf (Combined Y, Cb, & Cr Cbfs) Bits
	// for Merge, rootCBF is not sent


	// If the Root Cbf is on
	if (rootCbf) {
		if (transformBlockCount > 1) { //(cuPtr->size == MAX_LCU_SIZE)

			chromaCbfCtx = 0;
			//yCbfCtx      = (cuSizeLog2) - Log2f(transformSize);/*((cuPtr->sizeLog2) - Log2f(transformSize)) == 0  ? 1 : 0*/;
			cbfChromaFlagBitsNum +=
				candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(crCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
				candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[(cbCbf > 0) * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];

			for (tuCount = 0, tuIndex = 1; tuCount < transformBlockCount; ++tuCount) {
				tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

				cbCbfBlock = ((cbCbf)& (1 << tuIndex)) > 0;
				crCbfBlock = ((crCbf)& (1 << tuIndex)) > 0;
				yCbfBlock = ((yCbf)& (1 << tuIndex)) > 0;
				yCbfCtx = 0;
				chromaCbfCtx = cuSizeLog2 - Log2f(transformSize);

				cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
				cbfChromaFlagBitsNum += (crCbf > 0) ? candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] : 0;
				cbfChromaFlagBitsNum += (cbCbf > 0) ? candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] : 0;
				tuIndex += 1;
				
			}
		}
		else {
			// Estimate the Transform Split Flag & the Cbf's Bits
			tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

			cbCbfBlock = (cbCbf > 0);
			crCbfBlock = (crCbf > 0);
			yCbfBlock = (yCbf  > 0);

			if (cbCbfBlock || crCbfBlock) {
				yCbfCtx = 1;
				cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
			}

			cbfChromaFlagBitsNum +=
				candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
				candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];
		}

	}


	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	mergeLumaRate = skipFlagBitsNum + mergeFlagBitsNum + predModeBitsNum + partSizeIntraBitsNum + mergeIndexBitsNum + tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

	mergeChromaRate = cbfChromaFlagBitsNum;

	// Coeff rate
	coeffRate = (*yCoeffBits + *cbCoeffBits + *crCoeffBits) << 15;
	//lumaCoeffRate = (*yCoeffBits) << 15;

	// Compute Merge Cost
	mergeLumaSse = yDistortion[0];
	mergeChromaSse = cbDistortion[0] + crDistortion[0];

	skipLumaSse = yDistortion[1];
	skipChromaSse = cbDistortion[1] + crDistortion[1];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumulas
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	mergeLumaSse = LUMA_WEIGHT * (mergeLumaSse << COST_PRECISION);

	// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode

	if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {
		// Random Access
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            mergeChromaSse = (((mergeChromaSse * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            mergeChromaSse = (((mergeChromaSse * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else {
            mergeChromaSse = (((mergeChromaSse * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
	}
	else {
		// Low delay
		if (pictureControlSetPtr->temporalLayerIndex == 0) {
			mergeChromaSse = (((mergeChromaSse * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
		else {
			mergeChromaSse = (((mergeChromaSse * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
	}

	mergeDistortion = mergeLumaSse + mergeChromaSse;
    mergeCost = mergeDistortion + (((lambda * coeffRate + lambda * mergeLumaRate + lambdaChroma * mergeChromaRate) + MD_OFFSET) >> MD_SHIFT);

	// mergeLumaCost = mergeLumaSse    + (((lambda * lumaCoeffRate + lambda * mergeLumaRate) + MD_OFFSET) >> MD_SHIFT);

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	skipLumaSse = LUMA_WEIGHT * (skipLumaSse << COST_PRECISION);

	// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode

	if (pictureControlSetPtr->ParentPcsPtr->predStructure == EB_PRED_RANDOM_ACCESS) {

        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            skipChromaSse = (((skipChromaSse * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            skipChromaSse = (((skipChromaSse * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }
        else {
            skipChromaSse = (((skipChromaSse * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
        }

	}
	else {
		// Low Delay
		if (pictureControlSetPtr->temporalLayerIndex == 0) {
			skipChromaSse = (((skipChromaSse * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
		else {
			skipChromaSse = (((skipChromaSse * ChromaWeightFactorLdQpScaling[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
		}
	}

	skipDistortion = skipLumaSse + skipChromaSse;

    skipCost = skipDistortion + (((lambda * candidateBufferPtr->candidatePtr->fastLumaRate) + MD_OFFSET) >> MD_SHIFT);
	//skipLumaCost = skipLumaSse + (((lambda * candidateBufferPtr->candidatePtr->fastLumaRate) + MD_OFFSET) >> MD_SHIFT);

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = (skipCost <= mergeCost) ? skipCost : mergeCost;

	EB_U64 tempDistortion;
	tempDistortion = (skipCost <= mergeCost) ? skipDistortion : mergeDistortion;
	candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - tempDistortion;
    *candidateBufferPtr->fullCostMergePtr = mergeCost;
    *candidateBufferPtr->fullCostSkipPtr = skipCost;
    // Assign merge flag
	candidateBufferPtr->candidatePtr->mergeFlag = EB_TRUE;
	// Assign skip flag
	candidateBufferPtr->candidatePtr->skipFlag = EB_FALSE;


	return return_error;
}

/*********************************************************************************
* MergeSkipFullCost function is used to estimate the cost of an AMVPSkip candidate
* mode for full mode decision module.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the inter candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE  MergeSkipFullLumaCost(
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuSize,
	EB_U32                                  cuSizeLog2,
	ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE  return_error = EB_ErrorNone;


	// Pred Mode Bits Table
	// 0 for Inter
	// 1 for Intra
	EB_MODETYPE   candidateType = candidateBufferPtr->candidatePtr->type == INTER_MODE ? 0 : 1;
	EB_U32 mergeIndex = candidateBufferPtr->candidatePtr->mergeIndex;

	EB_U32 rootCbf = candidateBufferPtr->candidatePtr->rootCbf;

	EB_U64 skipFlagBitsNum = 0;
	EB_U64 mergeFlagBitsNum = 0;
	EB_U64 partSizeIntraBitsNum = 0;
	EB_U64 predModeBitsNum = 0;
	EB_U64 mergeIndexBitsNum = 0;
	EB_U64 tranSubDivFlagBitsNum = 0;
	EB_U64 cbfLumaFlagBitsNum = 0;
	EB_U32 yCbf = candidateBufferPtr->candidatePtr->yCbf;
	EB_U32 yCbfBlock;
	EB_U32 yCbfCtx;

	EB_U32 transSubDivFlag = TU_SPLIT_ZERO; // *Note- hardcoded to 0
	EB_U32 transSubDivFlagCtx;
	EB_U32 transformBlockCount = SQR(cuSizeLog2 - Log2f(transformSize) + 1);
	EB_U32 tuCount;
	EB_U32 tuIndex;

	transSubDivFlagCtx = 5 - Log2f(transformSize);

	// Merge
	EB_U64 mergeLumaRate;
	EB_U64 mergeDistortion;
	EB_U64 mergeCost;
	EB_U64 mergeLumaSse;
	EB_U64 coeffRate;

	// SKIP
	EB_U64 skipDistortion;
	EB_U64 skipCost;

	// Luma and chroma transform size shift for the distortion
	EB_U64 skipLumaSse;

	(void)cuSize;
	(void)transformSize;

	// Rate Estimation of each syntax element

	// Estimate Skip Flag Bits
	skipFlagBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->skipFlagBits[SPLIT_FLAG_ZERO * (NUMBER_OF_SKIP_FLAG_CASES >> 1) + cuPtr->skipFlagContext];

	// Estimate Merge Flag Bits
	mergeFlagBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->mergeFlagBits[1];

	// Estimate Pred Mode Bits
	predModeBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->predModeBits[candidateType];

	// Estimate Partition Size Bits :
	partSizeIntraBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->interPartSizeBits[SIZE_2Nx2N];

	// Merge Index
    {
        mergeIndexBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->mergeIndexBits[mergeIndex];
    }

	//mergeIndexBitsNum = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->mergeIndexBits[mergeIndex];

	// Estimate Root Cbf (Combined Y, Cb, & Cr Cbfs) Bits
	// for Merge, rootCBF is not sent

	// If the Root Cbf is on
	if (rootCbf) {
		if (transformBlockCount > 1) { //(cuPtr->size == MAX_LCU_SIZE)

			for (tuCount = 0, tuIndex = 1; tuCount < transformBlockCount; ++tuCount) {
				tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

				yCbfBlock = ((yCbf)& (1 << tuIndex)) > 0;
				yCbfCtx = 0;

				cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
				tuIndex += 1;
			}
		}
		else {
			// Estimate the Transform Split Flag & the Cbf's Bits
			tranSubDivFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->transSubDivFlagBits[transSubDivFlag * (NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES >> 1) + transSubDivFlagCtx];

			yCbfBlock = (yCbf  > 0);

			yCbfCtx = 1;
			cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbfBlock * (NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];

		}

	}

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	mergeLumaRate = skipFlagBitsNum + mergeFlagBitsNum + predModeBitsNum + partSizeIntraBitsNum + mergeIndexBitsNum + tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

	// Coeff rate
	coeffRate = *yCoeffBits << 15;

	// Compute Merge Cost
	mergeLumaSse = yDistortion[0];

	skipLumaSse = yDistortion[1];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumulas
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	mergeLumaSse = LUMA_WEIGHT * (mergeLumaSse << COST_PRECISION);

	mergeDistortion = mergeLumaSse;

	mergeCost = mergeDistortion + (((lambda * coeffRate + lambda * mergeLumaRate) + MD_OFFSET) >> MD_SHIFT);

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	skipLumaSse = LUMA_WEIGHT * (skipLumaSse << COST_PRECISION);

	skipDistortion = skipLumaSse;

	skipCost = skipDistortion + (((lambda * candidateBufferPtr->candidatePtr->fastLumaRate) + MD_OFFSET) >> MD_SHIFT);

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = (skipCost <= mergeCost) ? skipCost : mergeCost;

	EB_U64 tempDistortion;
	tempDistortion = (skipCost <= mergeCost) ? skipDistortion : mergeDistortion;
	candidateBufferPtr->fullLambdaRate = *candidateBufferPtr->fullCostPtr - tempDistortion;


	*candidateBufferPtr->fullCostMergePtr = mergeCost;
	*candidateBufferPtr->fullCostSkipPtr = skipCost;

	// Assign merge flag
	candidateBufferPtr->candidatePtr->mergeFlag = EB_TRUE;
	// Assign skip flag
	candidateBufferPtr->candidatePtr->skipFlag = EB_FALSE;

	return return_error;
}

/*********************************************************************************
* SplitFlagRate function is used to generate the Split rate
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param splitFlag(input)
*       splitFlag is the split flag value.
*   @param splitRate(output)
*       splitRate contains rate.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
*   @param mdRateEstimationPtr(input)
*       mdRateEstimationPtr is pointer to MD rate Estimation Tables
**********************************************************************************/
EB_ERRORTYPE SplitFlagRate(
	ModeDecisionContext_t               *contextPtr,
	CodingUnit_t                           *cuPtr,
	EB_U32                                  splitFlag,
	EB_U64                                 *splitRate,
	EB_U64                                  lambda,
	MdRateEstimationContext_t              *mdRateEstimationPtr,
	EB_U32                                  tbMaxDepth)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 splitFlagContext;
	EB_U32 cuDepth = GetCodedUnitStats(cuPtr->leafIndex)->depth;

	// Calculate the context
	splitFlagContext =
		(contextPtr->mdLocalCuUnit[cuPtr->leafIndex].leftNeighborMode > INTRA_MODE) ? 0 :
		(contextPtr->mdLocalCuUnit[cuPtr->leafIndex].leftNeighborDepth > cuDepth) ? 1 : 0;
	splitFlagContext +=
		(contextPtr->mdLocalCuUnit[cuPtr->leafIndex].topNeighborMode > INTRA_MODE) ? 0 :
		(contextPtr->mdLocalCuUnit[cuPtr->leafIndex].topNeighborDepth > cuDepth) ? 1 : 0;

	cuPtr->splitFlagContext = splitFlagContext;

	// Estimate Split Flag Bits
	*splitRate = (GetCodedUnitStats(cuPtr->leafIndex)->depth < (tbMaxDepth - 1)) ?
		mdRateEstimationPtr->splitFlagBits[splitFlag * (NUMBER_OF_SPLIT_FLAG_CASES >> 1) + splitFlagContext] :
		ZERO_COST;

	// Assign rate
	*splitRate = (((lambda * *splitRate) + MD_OFFSET) >> MD_SHIFT);

	return return_error;
}

/********************************************
* TuCalcCost
*   Computes TU Cost and generetes TU Cbf
*   at the level of the encode pass
********************************************/

EB_ERRORTYPE EncodeTuCalcCost(
    EncDecContext_t          *contextPtr,
	EB_U32                   *countNonZeroCoeffs,
	EB_U64                    yTuDistortion[DIST_CALC_TOTAL],
	EB_U64                   *yTuCoeffBits,
	EB_U32                    componentMask
	)
{
    CodingUnit_t              *cuPtr                = contextPtr->cuPtr;
	EB_U32                     cuSize               = contextPtr->cuStats->size;
    EB_U32                     tuIndex              = cuSize == 64 ? contextPtr ->tuItr : 0;
	EB_U32                     transformSize        = cuSize == 64 ? 32 : cuSize;
    MdRateEstimationContext_t *mdRateEstimationPtr  = contextPtr->mdRateEstimationPtr;
    EB_U64                     lambda               = contextPtr->fullLambda;
    EB_U32                     yCountNonZeroCoeffs  = countNonZeroCoeffs[0];
    EB_U32                     cbCountNonZeroCoeffs = countNonZeroCoeffs[1];
    EB_U32                     crCountNonZeroCoeffs = countNonZeroCoeffs[2];

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbfCtx = ((cuSize == transformSize));

	// Non Zero Cbf mode variables
	EB_U64 yNonZeroCbfDistortion = yTuDistortion[DIST_CALC_RESIDUAL];

	EB_U64 yNonZeroCbfLumaFlagBitsNum = 0;

	EB_U64 yNonZeroCbfRate;

	EB_U64 yNonZeroCbfCost = 0;

	// Zero Cbf mode variables
	EB_U64 yZeroCbfDistortion = yTuDistortion[DIST_CALC_PREDICTION];

	EB_U64 yZeroCbfLumaFlagBitsNum = 0;

	EB_U64 yZeroCbfRate;

	EB_U64 yZeroCbfCost = 0;

	// Luma and chroma transform size shift for the distortion

	


	// **Compute distortion
	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
        // Non Zero Distortion
		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
		yNonZeroCbfDistortion = LUMA_WEIGHT * (yNonZeroCbfDistortion << COST_PRECISION);


		// Zero distortion
		// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
		//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
		yZeroCbfDistortion = LUMA_WEIGHT * (yZeroCbfDistortion << COST_PRECISION);


		yNonZeroCbfLumaFlagBitsNum = mdRateEstimationPtr->lumaCbfBits[(NUMBER_OF_CBF_CASES >> 1) + yCbfCtx];
		yZeroCbfLumaFlagBitsNum = mdRateEstimationPtr->lumaCbfBits[yCbfCtx];
		yNonZeroCbfRate = ((*yTuCoeffBits) << 15) + yNonZeroCbfLumaFlagBitsNum;
		yZeroCbfRate = yZeroCbfLumaFlagBitsNum;

		if ( cuPtr->predictionModeFlag == INTRA_MODE) {

			yZeroCbfCost = 0xFFFFFFFFFFFFFFFFull;

		}
		else {

			yZeroCbfCost = yZeroCbfDistortion + (((lambda       * yZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);
		}

		// **Compute Cost
		yNonZeroCbfCost = yNonZeroCbfDistortion + (((lambda       * yNonZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);

		cuPtr->transformUnitArray[tuIndex].lumaCbf = ((yCountNonZeroCoeffs != 0) && (yNonZeroCbfCost  < yZeroCbfCost)) ? EB_TRUE : EB_FALSE;
		*yTuCoeffBits = (yNonZeroCbfCost  < yZeroCbfCost) ? *yTuCoeffBits : 0;
		yTuDistortion[DIST_CALC_RESIDUAL] = (yNonZeroCbfCost  < yZeroCbfCost) ? yTuDistortion[DIST_CALC_RESIDUAL] : yTuDistortion[DIST_CALC_PREDICTION];

	}
	else{

		cuPtr->transformUnitArray[tuIndex].lumaCbf = EB_FALSE;

	}
	cuPtr->transformUnitArray[tuIndex].cbCbf = cbCountNonZeroCoeffs != 0 ? EB_TRUE : EB_FALSE;
	cuPtr->transformUnitArray[tuIndex].crCbf = crCountNonZeroCoeffs != 0 ? EB_TRUE : EB_FALSE;


	if (cuPtr->transformUnitArray[tuIndex].lumaCbf) {
		cuPtr->transformUnitArray[0].lumaCbf = EB_TRUE;
	}

	if (cuPtr->transformUnitArray[tuIndex].cbCbf) {
		cuPtr->transformUnitArray[0].cbCbf = EB_TRUE;
	}

	if (cuPtr->transformUnitArray[tuIndex].crCbf) {
		cuPtr->transformUnitArray[0].crCbf = EB_TRUE;
	}

	return return_error;
}

EB_U64 GetPMCost(
	EB_U64                   lambda,	
	EB_U64                   tuDistortion,
	EB_U64                   yTuCoeffBits	
	)
{
	
	EB_U64 yNonZeroCbfDistortion = LUMA_WEIGHT * (tuDistortion << COST_PRECISION);
    EB_U64 yNonZeroCbfRate = (yTuCoeffBits );
	EB_U64 yNonZeroCbfCost = yNonZeroCbfDistortion + (((lambda       * yNonZeroCbfRate) + MD_OFFSET) >> MD_SHIFT);

	return yNonZeroCbfCost;
}


/*********************************************************************************
* IntraLumaModeContext function is used to check the intra luma mode of neighbor
* PUs and generate the required information for the intra luma mode rate estimation
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param lumaMode(input)
*       lumaMode is the candidate luma mode.
*   @param predictionIndex (output)
*       predictionIndex -1 means neither of the neighboring PUs hase the same
*       intra mode as the current PU.
**********************************************************************************/
EB_ERRORTYPE Intra4x4LumaModeContext(
	ModeDecisionContext_t   *contextPtr,
	EB_U32                   puIndex,
	EB_U32                   lumaMode,
	EB_S32                  *predictionIndex)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 lumaPredictionArray[3];
	EB_U32 leftNeighborMode = contextPtr->intraLumaLeftModeArray[puIndex];
	EB_U32 topNeighborMode = contextPtr->intraLumaTopModeArray[puIndex];

	if (leftNeighborMode == topNeighborMode) {
		if (leftNeighborMode > 1) { // For angular modes
			lumaPredictionArray[0] = leftNeighborMode;
			lumaPredictionArray[1] = ((leftNeighborMode + 29) & 0x1F) + 2;
			lumaPredictionArray[2] = ((leftNeighborMode - 1) & 0x1F) + 2;
		}
		else { // Non Angular modes
			lumaPredictionArray[0] = EB_INTRA_PLANAR;
			lumaPredictionArray[1] = EB_INTRA_DC;
			lumaPredictionArray[2] = EB_INTRA_VERTICAL;
		}
	}
	else {
		lumaPredictionArray[0] = leftNeighborMode;
		lumaPredictionArray[1] = topNeighborMode;

		if (leftNeighborMode && topNeighborMode) {
			lumaPredictionArray[2] = EB_INTRA_PLANAR; // when both modes are non planar
		}
		else {
			lumaPredictionArray[2] = (leftNeighborMode + topNeighborMode) < 2 ? EB_INTRA_VERTICAL : EB_INTRA_DC;
		}
	}

	*predictionIndex = (lumaMode == lumaPredictionArray[0]) ? 0 :
		(lumaMode == lumaPredictionArray[1]) ? 1 :
		(lumaMode == lumaPredictionArray[2]) ? 2 :
		-1; // luma mode is not equal to any of the predictors

	return return_error;
}
/*********************************************************************************
* Intra4x4FastCostIslice function is used to estimate the cost of an intra 4x4 candidate mode
* for fast mode decision module in I slice.
* The fast cost is based on Luma Distortion and Luma Rate only
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE Intra4x4FastCostIslice(
	ModeDecisionContext_t                  *contextPtr,
	EB_U32                                  puIndex,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                  lumaDistortion,
	EB_U64                                  lambda)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 lumaMode;
	EB_S32 predictionIndex;

	// Number of bits for each synatax element
	EB_U64 partSizeIntraBitsNum;
	EB_U64 intraLumaModeBitsNum;
	// Luma and chroma rate
	EB_U64 rate;
    EB_U64 lumaRate;
	// Luma distortion
	EB_U64 totalDistortion;

	lumaMode = candidateBufferPtr->candidatePtr->intraLumaMode;
	predictionIndex = -1;

	partSizeIntraBitsNum = 0;
	intraLumaModeBitsNum = 0;

	// Estimate Partition Size Bits
	partSizeIntraBitsNum = (puIndex == 0) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraPartSizeBits[1] :
		0;

	// Estimate Luma Mode Bits for Intra
	Intra4x4LumaModeContext(
        contextPtr,
		puIndex,
		lumaMode,
		&predictionIndex);

	intraLumaModeBitsNum = (predictionIndex != -1) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[predictionIndex] :
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[3];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate =
		partSizeIntraBitsNum +
		intraLumaModeBitsNum;

	// Keep the Fast Luma and Chroma rate for future use
	candidateBufferPtr->candidatePtr->fastLumaRate = lumaRate;

	// Distortion calculation
	totalDistortion = ((LUMA_WEIGHT * lumaDistortion) << COST_PRECISION);

	// Rate calculation
	rate = (((lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	// Scale the rate by the total (*Note this is experimental)
	rate *= RATE_WEIGHT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;

	return return_error;
}

/*********************************************************************************
* Intra4x4FastCostPslice function is used to estimate the cost of an intra 4x4 candidate mode
* for fast mode decision module in P slice.
* The fast cost is based on Luma Distortion and Luma Rate only
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE Intra4x4FastCostPslice(
	ModeDecisionContext_t                  *contextPtr,
	EB_U32                                  puIndex,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                  lumaDistortion,
	EB_U64                                  lambda)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 lumaMode;
	EB_S32 predictionIndex;

	// Number of bits for each synatax element
	EB_U64 predModeBitsNum;
	EB_U64 partSizeIntraBitsNum;
	EB_U64 intraLumaModeBitsNum;
	// Luma and chroma rate
	EB_U64 rate;
	EB_U64 lumaRate;
	// Luma distortion
	EB_U64 totalDistortion;

	lumaMode = candidateBufferPtr->candidatePtr->intraLumaMode;
	predictionIndex = -1;

	predModeBitsNum = 0;
	partSizeIntraBitsNum = 0;
	intraLumaModeBitsNum = 0;

	// Estimate Partition Size Bits
	partSizeIntraBitsNum = (puIndex == 0) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraPartSizeBits[1] :
		0;

	// Estimate Pred Mode Bits

	// Pred Mode Bits Table
	// 0 for Inter
	// 1 for Intra

	predModeBitsNum = (puIndex == 0) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->predModeBits[1] :
		0;

	// Estimate Luma Mode Bits for Intra
	Intra4x4LumaModeContext(
        contextPtr,
		puIndex,
		lumaMode,
		&predictionIndex);

	intraLumaModeBitsNum = (predictionIndex != -1) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[predictionIndex] :
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[3];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate =
		predModeBitsNum +
		partSizeIntraBitsNum +
		intraLumaModeBitsNum;

	// Keep the Fast Luma and Chroma rate for future use
	candidateBufferPtr->candidatePtr->fastLumaRate = lumaRate;

	// Distortion calculation
	totalDistortion = ((LUMA_WEIGHT * lumaDistortion) << COST_PRECISION);

	// Rate calculation
	rate = (((lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	// Scale the rate by the total (*Note this is experimental)
	rate *= RATE_WEIGHT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;

	return return_error;
}

/*********************************************************************************
* Intra4x4FullCostIslice function is used to estimate the cost of an intra candidate mode
* for full mode decision module in EB_I_PICTURE.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param chromaDistortion (input)
*       chromaDistortion is the intra candidate chroma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE Intra4x4FullCostIslice(
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf;

	EB_U64 tranSubDivFlagBitsNum;
	EB_U64 cbfLumaFlagBitsNum;

	EB_U64 lumaRate;
	EB_U64 coeffRate;
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;
	(void)transformSize;
	yCbf = candidateBufferPtr->candidatePtr->yCbf;

	tranSubDivFlagBitsNum = 0;
	cbfLumaFlagBitsNum = 0;

	// Estimate the Transform Split Flag & the Cbf's Bits

	cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbf * (NUMBER_OF_CBF_CASES >> 1)];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;

	// Coeff rate
	coeffRate = (*yCoeffBits) << 15;

	lumaSse = yDistortion[0];

	distortion = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	return return_error;
}

/*********************************************************************************
* Intra4x4FullCostPslice function is used to estimate the cost of an intra candidate mode
* for full mode decision module.
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param chromaDistortion (input)
*       chromaDistortion is the intra candidate chroma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE Intra4x4FullCostPslice(
struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U64                                 *yDistortion,
	EB_U64                                  lambda,
	EB_U64                                 *yCoeffBits,
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf;

	EB_U64 tranSubDivFlagBitsNum;
	EB_U64 cbfLumaFlagBitsNum;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 coeffRate;

	EB_U64 distortion;
	EB_U64 lumaSse;
	(void)transformSize;

	yCbf = candidateBufferPtr->candidatePtr->yCbf;

	tranSubDivFlagBitsNum = 0;
	cbfLumaFlagBitsNum = 0;

	// Estimate the Transform Split Flag & the Cbf's Bits

	cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbf * (NUMBER_OF_CBF_CASES >> 1)];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;

	// Coeff rate
	coeffRate = (*yCoeffBits) << 15;

	lumaSse = yDistortion[0];

	distortion = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate) + MD_OFFSET) >> MD_SHIFT);

	return return_error;
}

/*********************************************************************************
* Intra2Nx2NFastCost function is used to estimate the cost of an intra candidate mode
* for fast mode decision module in I slice.
* Chroma cost is excluded from fast cost functions. Only the fastChromaRate is stored
* for future use in full loop
*
*   @param *cuPtr(input)
*       cuPtr is the pointer of the target CU.
*   @param *candidateBufferPtr(input)
*       chromaBufferPtr is the buffer pointer of the candidate luma mode.
*   @param qp(input)
*       qp is the quantizer parameter.
*   @param lumaDistortion (input)
*       lumaDistortion is the intra candidate luma distortion.
*   @param lambda(input)
*       lambda is the Lagrange multiplier
**********************************************************************************/
EB_ERRORTYPE IntraNxNFastCostIslice(
	CodingUnit_t                           *cuPtr,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 lumaMode = candidateBufferPtr->candidatePtr->intraLumaMode;
	EB_U32 chromaMode = EB_INTRA_CHROMA_DM;
	EB_S32 predictionIndex = -1;
	EB_U8  partitionMode = 1; // NXN partition

	(void)qp;

	// Number of bits for each synatax element
	EB_U64 partSizeIntraBitsNum = 0;
	EB_U64 intraLumaModeBitsNum = 0;

	// Luma and chroma rate
	EB_U64 rate;
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	//EB_U64 lumaSad, chromaSad;

	// Luma and chroma distortion
	EB_U64 totalDistortion;

	//EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	// Estimate Chroma Mode Bits
	chromaRate = candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraChromaBits[chromaMode];


	// Estimate Partition Size Bits :
	partSizeIntraBitsNum = (GetCodedUnitStats(cuPtr->leafIndex)->depth == (((SequenceControlSet_t *)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->maxLcuDepth - 1)) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraPartSizeBits[partitionMode] :
		ZERO_COST;

	// Estimate Luma Mode Bits for Intra
	IntraLumaModeContext(
		cuPtr,
		lumaMode,
		&predictionIndex);

	intraLumaModeBitsNum = (predictionIndex != -1) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[predictionIndex] :
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraLumaBits[3];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate =
		partSizeIntraBitsNum +
		intraLumaModeBitsNum;

	// Keep the Fast Luma and Chroma rate for future use
	candidateBufferPtr->candidatePtr->fastLumaRate = lumaRate;
	candidateBufferPtr->candidatePtr->fastChromaRate = chromaRate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;

	totalDistortion = (LUMA_WEIGHT * (lumaDistortion + chromaDistortion)) << COST_PRECISION;
	
	// include luma only in rate calculation
	rate = ((lambda * (lumaRate + chromaRate)) + MD_OFFSET) >> MD_SHIFT;

	// Scale the rate by the total (*Note this is experimental)
	rate *= RATE_WEIGHT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	return return_error;
}

EB_ERRORTYPE IntraNxNFastCostPslice(
	CodingUnit_t                           *cuPtr,
    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
	EB_U32                                  qp,
	EB_U64                                  lumaDistortion,
	EB_U64                                  chromaDistortion,
	EB_U64                                  lambda,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	ModeDecisionCandidate_t		*candidatePtr = candidateBufferPtr->candidatePtr;
	MdRateEstimationContext_t	*mdRateEstimationPtr = candidatePtr->mdRateEstimationPtr;

	// Pred Mode Bits Table
	// 0 for Inter
	// 1 for Intra
	EB_MODETYPE candidateType = candidatePtr->type == INTER_MODE ? 0 : 1;
	EB_U32 lumaMode = candidatePtr->intraLumaMode;
	EB_U32 chromaMode = EB_INTRA_CHROMA_DM;
	EB_S32 predictionIndex = -1;
	EB_U8  partitionMode =  1; // NXN partition

	// Number of bits for each synatax element
	EB_U64 predModeBitsNum = 0;
	EB_U64 partSizeIntraBitsNum = 0;
	EB_U64 intraLumaModeBitsNum = 0;
	(void)qp;
	// Luma and chroma rate
	EB_U64 rate;
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	//EB_U64 lumaSad, chromaSad;

	// Luma and chroma distortion
	EB_U64 totalDistortion;



	// Estimate Chroma Mode Bits
	chromaRate = mdRateEstimationPtr->intraChromaBits[chromaMode];

	// Estimate Partition Size Bits :
	partSizeIntraBitsNum = (GetCodedUnitStats(cuPtr->leafIndex)->depth == (((SequenceControlSet_t *)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->maxLcuDepth - 1)) ?
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->intraPartSizeBits[partitionMode] :
		ZERO_COST;

	// Estimate Pred Mode Bits
	predModeBitsNum = mdRateEstimationPtr->predModeBits[candidateType];

	// Estimate Luma Mode Bits for Intra
	IntraLumaModeContext(
		cuPtr,
		lumaMode,
		&predictionIndex);

	intraLumaModeBitsNum = (predictionIndex != -1) ?
		mdRateEstimationPtr->intraLumaBits[predictionIndex] :
		mdRateEstimationPtr->intraLumaBits[3];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate =
		predModeBitsNum +
		partSizeIntraBitsNum +
		intraLumaModeBitsNum;

	// Keep the Fast Luma and Chroma rate for future use
	candidatePtr->fastLumaRate = lumaRate;
	candidatePtr->fastChromaRate = chromaRate;

	candidateBufferPtr->residualLumaSad = lumaDistortion;

	// include luma only in total distortion
	totalDistortion = (LUMA_WEIGHT * (lumaDistortion + chromaDistortion)) << COST_PRECISION;
	

	// include luma only in rate calculation
	rate = ((lambda * (lumaRate + chromaRate)) + MD_OFFSET) >> MD_SHIFT;

	// Scale the rate by the total (*Note this is experimental)
	rate *= RATE_WEIGHT;

	// Assign fast cost
	*(candidateBufferPtr->fastCostPtr) = totalDistortion + rate;

	return return_error;
}

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
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf;
	EB_U32 crCbf;
	EB_U32 cbCbf;

	EB_U32 chromaCbfCtx;

	EB_U64 tranSubDivFlagBitsNum;
	EB_U64 cbfLumaFlagBitsNum;
	EB_U64 cbfChromaFlagBitsNum;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	EB_U64 coeffRate;

	// Luma and chroma distortion
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;
	EB_U64 chromaSse;

	(void)pictureControlSetPtr;

	yCbf = candidateBufferPtr->candidatePtr->yCbf;
	crCbf = candidateBufferPtr->candidatePtr->crCbf;
	cbCbf = candidateBufferPtr->candidatePtr->cbCbf;

	tranSubDivFlagBitsNum = 0;
	cbfLumaFlagBitsNum = 0;
	cbfChromaFlagBitsNum = 0;

	// Estimate the Transform Split Flag & the Cbf's Bits
	chromaCbfCtx = 2/*cuPtr->sizeLog2*/ - Log2f(transformSize);

	cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbf * (NUMBER_OF_CBF_CASES >> 1)];

	cbfChromaFlagBitsNum +=
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbf * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbf * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;
	chromaRate = cbfChromaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;
	chromaRate += candidateBufferPtr->candidatePtr->fastChromaRate;

	// Coeff rate
	coeffRate = (*yCoeffBits + *cbCoeffBits + *crCoeffBits) << 15;

	// Note: for square Transform, the scale is 1/(2^(7-Log2(Transform size)))
	// For NSQT the scale would be 1/ (2^(7-(Log2(first Transform size)+Log2(second Transform size))/2))
	// Add Log2 of Transform size in order to calculating it multiple time in this function
	lumaSse = yDistortion[0];
	chromaSse = cbDistortion[0] + crDistortion[0];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode
	chromaSse = (((chromaSse * ChromaWeightFactorLd[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT); // Low delay and Random access have the same value of chroma weight

	distortion = lumaSse + chromaSse;

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate + lambdaChroma * chromaRate) + MD_OFFSET) >> MD_SHIFT);

	return return_error;
}

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
	EB_U32                                  transformSize)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 yCbf;
	EB_U32 crCbf;
	EB_U32 cbCbf;

	EB_U32 chromaCbfCtx;

	EB_U64 tranSubDivFlagBitsNum;
	EB_U64 cbfLumaFlagBitsNum;
	EB_U64 cbfChromaFlagBitsNum;

	// Luma and chroma rate
	EB_U64 lumaRate;
	EB_U64 chromaRate;
	EB_U64 coeffRate;

	// Luma and chroma distortion
	EB_U64 distortion;

	// Luma and chroma SSE
	EB_U64 lumaSse;
	EB_U64 chromaSse;

	yCbf = candidateBufferPtr->candidatePtr->yCbf;
	crCbf = candidateBufferPtr->candidatePtr->crCbf;
	cbCbf = candidateBufferPtr->candidatePtr->cbCbf;

	tranSubDivFlagBitsNum = 0;
	cbfLumaFlagBitsNum = 0;
	cbfChromaFlagBitsNum = 0;

	// Estimate the Transform Split Flag & the Cbf's Bits

	chromaCbfCtx = /*cuPtr->sizeLog2*/ 2 - Log2f(transformSize);

	cbfLumaFlagBitsNum += candidateBufferPtr->candidatePtr->mdRateEstimationPtr->lumaCbfBits[yCbf * (NUMBER_OF_CBF_CASES >> 1)];

	cbfChromaFlagBitsNum +=
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[crCbf * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx] +
		candidateBufferPtr->candidatePtr->mdRateEstimationPtr->chromaCbfBits[cbCbf * (NUMBER_OF_CBF_CASES >> 1) + chromaCbfCtx];

	// Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
	lumaRate = tranSubDivFlagBitsNum + cbfLumaFlagBitsNum;
	chromaRate = cbfChromaFlagBitsNum;

	// Add fast rate to get the total rate of the subject mode
	lumaRate += candidateBufferPtr->candidatePtr->fastLumaRate;
	chromaRate += candidateBufferPtr->candidatePtr->fastChromaRate;

	// Coeff rate
	coeffRate = (*yCoeffBits + *cbCoeffBits + *crCoeffBits) << 15;
	// Note: for square Transform, the scale is 1/(2^(7-Log2(Transform size)))
	// For NSQT the scale would be 1/ (2^(7-(Log2(first Transform size)+Log2(second Transform size))/2))
	// Add Log2 of Transform size in order to calculating it multiple time in this function

	lumaSse = yDistortion[0];
	chromaSse = cbDistortion[0] + crDistortion[0];

	// *Note - As of Oct 2011, the JCT-VC uses the PSNR forumula
	//  PSNR = (LUMA_WEIGHT * PSNRy + PSNRu + PSNRv) / (2+LUMA_WEIGHT)
	lumaSse = LUMA_WEIGHT * (lumaSse << COST_PRECISION);

	// *Note - As in JCTVC-G1102, the JCT-VC uses the Mode Decision forumula where the chromaSse has been weighted
	//  CostMode = (lumaSse + wchroma * chromaSse) + lambdaSse * rateMode

	if (pictureControlSetPtr->temporalLayerIndex == 0) {
		chromaSse = (((chromaSse * ChromaWeightFactorRaBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
	}
	else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
		chromaSse = (((chromaSse * ChromaWeightFactorRaRefNonBase[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
	}
	else {
		chromaSse = (((chromaSse * ChromaWeightFactorRaNonRef[qp]) + CHROMA_WEIGHT_OFFSET) >> CHROMA_WEIGHT_SHIFT);
	}


	distortion = lumaSse + chromaSse;

	// Assign full cost
	*candidateBufferPtr->fullCostPtr = distortion + (((lambda * coeffRate + lambda * lumaRate + lambdaChroma * chromaRate) + MD_OFFSET) >> MD_SHIFT);

	return return_error;
}
