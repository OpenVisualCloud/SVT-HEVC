/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "EbModeDecisionProcess.h"
#include "EbTransforms.h"
#include "EbFullLoop.h"
#include "EbRateDistortionCost.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

static const EB_U64 depth0Th[2][MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = {
	{
		{ 1000 },
		{ 1000, 4000 },
		{ 1000, 4000, 9500 },
		{ 1000, 4000, 9500, 3000 },
		{ 1000, 4000, 9500, 3000, 3000 },
		{ 1000, 4000, 9500, 3000, 3000, 3000 }
	},

	{
		{ 0 },
		{ 0, 1000 },
		{ 0, 1000, 7000 },
		{ 0, 1000, 7000, 9500 },
		{ 0, 1000, 7000, 9500, 9500 },
		{ 0, 1000, 7000, 9500, 9500, 9500 }
	}
};
static const EB_U64 depth1Th[2][MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = {
	{
		{ 0 },
		{ 0, 2000 },
		{ 0, 2000, 5500 },
		{ 0, 2000, 5500, 9500 },
		{ 0, 2000, 5500, 9500, 9500 },
		{ 0, 2000, 5500, 9500, 9500, 9500 }
	},

	{
		{ 0 },
		{ 0, 1500 },
		{ 0, 1500, 1500 },
		{ 0, 1500, 1500, 1500 },
		{ 0, 1500, 1500, 1500, 1500 },
		{ 0, 1500, 1500, 1500, 1500, 1500 }
	}
};
static const EB_U64 depth2Th[2][MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = {
	{
		{ 0 },
		{ 0, 500 },
		{ 0, 500, 2000 },
		{ 0, 500, 2000, 2500 },
		{ 0, 500, 2000, 2500, 2500 },
		{ 0, 500, 2000, 2500, 2500, 2500 }
	},

	{
		{ 0 },
		{ 0, 1500 },
		{ 0, 1500, 1000 },
		{ 0, 1500, 1000, 4500 },
		{ 0, 1500, 1000, 4500, 4500 },
		{ 0, 1500, 1000, 4500, 4500, 4500 }
	}
};

/*********************************************************************
 * UnifiedQuantizeInvQuantize
 *
 *  Unified Quant +iQuant
 *********************************************************************/
void ProductUnifiedQuantizeInvQuantizeMd(
	PictureControlSet_t  *pictureControlSetPtr,
	EB_S16               *coeff,
	const EB_U32          coeffStride,
	EB_S16               *quantCoeff,
	EB_S16               *reconCoeff,
	EB_U32                qp,
	EB_U32                areaSize,
	EB_U32               *yCountNonZeroCoeffs,
	EB_PF_MODE		      pfMode,
	EB_U8                 enableContouringQCUpdateFlag,
	EB_U32                componentType,
    EB_RDOQ_PMCORE_TYPE   rdoqPmCoreMethod,
	CabacEncodeContext_t *cabacEncodeCtxPtr,
	EB_U64                lambda,
	EB_MODETYPE           type,                
	EB_U32                intraLumaMode,
	EB_U32                intraChromaMode,
	CabacCost_t          *CabacCost)

{
    EB_PICTURE          sliceType                          = pictureControlSetPtr->sliceType;
    EB_U32            temporalLayerIndex                 = pictureControlSetPtr->temporalLayerIndex;
	//for the Quant
	const EB_S32 qpRem = (EB_S32)QpModSix[qp]; //the output is between 0-5
	const EB_S32 qpPer = (EB_S32)QpDivSix[qp] + TRANS_BIT_INCREMENT; //the output is between 0 and 8+TRANS_BIT_INCREMENT   (CHKN TRANS_BIT_INCREMENT =   0)
	const EB_U32 qFunc = QFunc[qpRem]; // 15 bits

	const EB_U32 transformShiftNum = 7 - Log2f(areaSize);
	const EB_S32 shiftedQBits = QUANT_SHIFT + qpPer + transformShiftNum;
	const EB_U32 q_offset = ((sliceType == EB_I_PICTURE || sliceType == EB_IDR_PICTURE) ? QUANT_OFFSET_I : QUANT_OFFSET_P) << (shiftedQBits - 9);

	//for the iQuant
	const EB_S32 shiftedFFunc = (qpPer > 8) ? (EB_S32)FFunc[qpRem] << (qpPer - 2) : (EB_S32)FFunc[qpRem] << qpPer; // this is 6+8+TRANS_BIT_INCREMENT
	const EB_S32 shiftNum = (qpPer > 8) ? QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShiftNum - 2 : QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShiftNum;
	const EB_S32 iq_offset = 1 << (shiftNum - 1);


	if (pfMode == PF_N2) {
		areaSize = areaSize >> 1;
	} else if (pfMode == PF_N4) {
		areaSize = areaSize >> 2;
	}

	if (rdoqPmCoreMethod){
		DecoupledQuantizeInvQuantizeLoops(
			coeff,
			coeffStride,
			quantCoeff,
			reconCoeff,
			cabacEncodeCtxPtr,
			lambda,
			type,
			intraLumaMode,
			intraChromaMode,
			componentType,
			pictureControlSetPtr->temporalLayerIndex,
			pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag,
            (EB_U8) 0,
            (EB_U16)qp,
            (EB_U32)EB_8BIT,
			CabacCost,
			qFunc,
			q_offset,
			shiftedQBits,
			shiftedFFunc,
			iq_offset,
			shiftNum,
			areaSize,
			&(*yCountNonZeroCoeffs),
            rdoqPmCoreMethod);
	}
	else{

	    QiQ_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][areaSize >> 3](
		    coeff,
		    coeffStride,
		    quantCoeff,
		    reconCoeff,
		    qFunc,
		    q_offset,
		    shiftedQBits,
		    shiftedFFunc,
		    iq_offset,
		    shiftNum,
		    areaSize,
		    &(*yCountNonZeroCoeffs));

	    UpdateQiQCoef(
		    quantCoeff,
		    reconCoeff,
		    coeffStride,
		    shiftedFFunc,
		    iq_offset,
		    shiftNum,
		    areaSize,
		    &(*yCountNonZeroCoeffs),
		    componentType,
		    sliceType,
		    temporalLayerIndex,
		    0,
		    enableContouringQCUpdateFlag);
	}
}

/****************************************
 ************  Full loop ****************
 ****************************************/
void ProductFullLoop(
	EbPictureBufferDesc_t         *inputPicturePtr,
	EB_U32                         inputOriginIndex,
	ModeDecisionCandidateBuffer_t  *candidateBuffer,
	ModeDecisionContext_t          *contextPtr,
	const CodedUnitStats_t         *cuStatsPtr,
	PictureControlSet_t            *pictureControlSetPtr,
	EB_U32                          qp,
	EB_U32						   *yCountNonZeroCoeffs,
	EB_U64                         *yCoeffBits,
	EB_U64                         *yFullDistortion)
{
	EB_U32                       tuOriginIndex;

	EB_U32   currentTuIndex,tuIt;
	EB_U64   yTuCoeffBits;
	EB_U64   tuFullDistortion[3][DIST_CALC_TOTAL];
	candidateBuffer->yDc[0] = 0;
	candidateBuffer->yDc[1] = 0;
	candidateBuffer->yDc[2] = 0;
	candidateBuffer->yDc[3] = 0;
	candidateBuffer->yCountNonZeroCoeffs[0] = 0;
	candidateBuffer->yCountNonZeroCoeffs[1] = 0;
	candidateBuffer->yCountNonZeroCoeffs[2] = 0;
	candidateBuffer->yCountNonZeroCoeffs[3] = 0;

	if (cuStatsPtr->size == MAX_LCU_SIZE){

		for (tuIt = 0; tuIt < 4; tuIt++)
		{

			tuOriginIndex = ((tuIt & 1) << 5) + ((tuIt>1) << 11);
			currentTuIndex  = tuIt + 1;
			yTuCoeffBits = 0;
			EstimateTransform(
				&(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[tuOriginIndex]),
				MAX_LCU_SIZE,
				&(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeffNxNPtr->bufferY)[tuOriginIndex]),
				MAX_LCU_SIZE,
				32,
				contextPtr->transformInnerArrayPtr,
				0,
				EB_FALSE,
				contextPtr->pfMdMode);


			ProductUnifiedQuantizeInvQuantizeMd(
				pictureControlSetPtr,
				&(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeffNxNPtr->bufferY)[tuOriginIndex]),
				MAX_LCU_SIZE,
				&(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[tuOriginIndex]),
				&(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[tuOriginIndex]),
				qp,
				32,
				&(yCountNonZeroCoeffs[currentTuIndex]),
				contextPtr->pfMdMode,
				0,
				COMPONENT_LUMA,
                contextPtr->rdoqPmCoreMethod,
				(CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
				contextPtr->fullLambda,
				candidateBuffer->candidatePtr->type,                 // Input: CU type (INTRA, INTER)
				candidateBuffer->candidatePtr->intraLumaMode,
				EB_INTRA_CHROMA_DM,
				pictureControlSetPtr->cabacCost);

			PictureFullDistortionLuma(
				contextPtr->transQuantBuffersPtr->tuTransCoeffNxNPtr,
				tuOriginIndex,
				candidateBuffer->reconCoeffPtr,
				tuOriginIndex,
				(32 >> contextPtr->pfMdMode),
				tuFullDistortion[0],
				yCountNonZeroCoeffs[currentTuIndex],
				candidateBuffer->candidatePtr->type);


			tuFullDistortion[0][DIST_CALC_RESIDUAL] = (tuFullDistortion[0][DIST_CALC_RESIDUAL] + 8) >> 4;
			tuFullDistortion[0][DIST_CALC_PREDICTION] = (tuFullDistortion[0][DIST_CALC_PREDICTION] + 8) >> 4;

			TuEstimateCoeffBitsLuma(
				tuOriginIndex,
				contextPtr->coeffEstEntropyCoderPtr,
				candidateBuffer->residualQuantCoeffPtr,
				yCountNonZeroCoeffs[currentTuIndex],
				&yTuCoeffBits,
				32,
				candidateBuffer->candidatePtr->type,
				candidateBuffer->candidatePtr->intraLumaMode,
				contextPtr->pfMdMode,
				contextPtr->coeffCabacUpdate,
				&(candidateBuffer->candBuffCoeffCtxModel),
				contextPtr->CabacCost);

			TuCalcCostLuma(
				MAX_LCU_SIZE,
				candidateBuffer->candidatePtr,
				currentTuIndex,
				32,
				yCountNonZeroCoeffs[currentTuIndex],
				tuFullDistortion[0],
				&yTuCoeffBits,
				contextPtr->qp,
				contextPtr->fullLambda,
				contextPtr->fullChromaLambda);

			(*yCoeffBits)                         += yTuCoeffBits;
			yFullDistortion[DIST_CALC_RESIDUAL]   += tuFullDistortion[0][DIST_CALC_RESIDUAL];
			yFullDistortion[DIST_CALC_PREDICTION] += tuFullDistortion[0][DIST_CALC_PREDICTION];
			candidateBuffer->yDc[tuIt] = ABS(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[tuOriginIndex]);
			candidateBuffer->yCountNonZeroCoeffs[tuIt] = (EB_U16)yCountNonZeroCoeffs[currentTuIndex];

		}  

	}else{

		tuOriginIndex = cuStatsPtr->originX + (cuStatsPtr->originY<<6);
		yTuCoeffBits    = 0;
		EstimateTransform(
			&(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[tuOriginIndex]),
			MAX_LCU_SIZE,
			&(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY)[tuOriginIndex]),
			MAX_LCU_SIZE,
			cuStatsPtr->size,
			contextPtr->transformInnerArrayPtr,
			0,
			EB_FALSE,
			contextPtr->pfMdMode);

		ProductUnifiedQuantizeInvQuantizeMd(
			pictureControlSetPtr,
			&(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY)[tuOriginIndex]),
			MAX_LCU_SIZE,
			&(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[tuOriginIndex]),
			&(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[tuOriginIndex]),
			qp,
			cuStatsPtr->size,
			&(yCountNonZeroCoeffs[0]),
			contextPtr->pfMdMode,
			0,
			COMPONENT_LUMA,
            contextPtr->rdoqPmCoreMethod,
			(CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
			contextPtr->fullLambda,
			candidateBuffer->candidatePtr->type,                 // Input: CU type (INTRA, INTER)
			candidateBuffer->candidatePtr->intraLumaMode,
			EB_INTRA_CHROMA_DM,
			pictureControlSetPtr->cabacCost);

		if (contextPtr->spatialSseFullLoop == EB_TRUE) {

			if (yCountNonZeroCoeffs[0]) {
				//since we are missing PF-N2 version for 16x16 and 8x8 iT, do zero out.
				if (cuStatsPtr->size < 32 && contextPtr->pfMdMode == PF_N2) {
					PfZeroOutUselessQuadrants(
						&(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[tuOriginIndex]),
						candidateBuffer->reconCoeffPtr->strideY,
						(cuStatsPtr->size >> 1));
				}

				EstimateInvTransform(
					&(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[tuOriginIndex]),
					candidateBuffer->reconCoeffPtr->strideY,
					&(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY)[tuOriginIndex]),
					contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
					cuStatsPtr->size,
					contextPtr->transformInnerArrayPtr,
					BIT_INCREMENT_8BIT,
					EB_FALSE,
					cuStatsPtr->size < 32 ? PF_OFF : contextPtr->pfMdMode);

                if ((cuStatsPtr->size >> 3) < 9)
				    AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][cuStatsPtr->size >> 3](
					    &(candidateBuffer->predictionPtr->bufferY[tuOriginIndex]),
					    64,
					    &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY))[tuOriginIndex]),
					    contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
					    &(candidateBuffer->reconPtr->bufferY[tuOriginIndex]),
					    candidateBuffer->reconPtr->strideY,
					    cuStatsPtr->size,
					    cuStatsPtr->size);

			}
			else {

				PictureCopy8Bit(
					candidateBuffer->predictionPtr,
					tuOriginIndex,
					0,
					candidateBuffer->reconPtr,
					tuOriginIndex,
					0,
					cuStatsPtr->size,
					cuStatsPtr->size,
					0,
					0,
					PICTURE_BUFFER_DESC_Y_FLAG);
			}

			tuFullDistortion[0][DIST_CALC_RESIDUAL] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(cuStatsPtr->size) - 2](
				&(inputPicturePtr->bufferY[inputOriginIndex]),
				inputPicturePtr->strideY,
				&(candidateBuffer->reconPtr->bufferY[tuOriginIndex]),
				candidateBuffer->reconPtr->strideY,
				cuStatsPtr->size,
				cuStatsPtr->size);

			tuFullDistortion[0][DIST_CALC_PREDICTION] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(cuStatsPtr->size) - 2](
				&(inputPicturePtr->bufferY[inputOriginIndex]),
				inputPicturePtr->strideY,
				&(candidateBuffer->predictionPtr->bufferY[tuOriginIndex]),
				candidateBuffer->predictionPtr->strideY,
				cuStatsPtr->size,
				cuStatsPtr->size);
		}
		else {

			PictureFullDistortionLuma(
				contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr,
				tuOriginIndex,
				candidateBuffer->reconCoeffPtr,
				tuOriginIndex,
				(contextPtr->cuStats->size >> contextPtr->pfMdMode),
				tuFullDistortion[0],
				yCountNonZeroCoeffs[0],
				candidateBuffer->candidatePtr->type);

			const EB_U32 lumaShift = 2 * (7 - Log2f(cuStatsPtr->size));
			tuFullDistortion[0][DIST_CALC_RESIDUAL] = (tuFullDistortion[0][DIST_CALC_RESIDUAL] + (EB_U64)(1 << (lumaShift - 1))) >> lumaShift;
			tuFullDistortion[0][DIST_CALC_PREDICTION] = (tuFullDistortion[0][DIST_CALC_PREDICTION] + (EB_U64)(1 << (lumaShift - 1))) >> lumaShift;  
		}

		TuEstimateCoeffBitsLuma(
			tuOriginIndex,
			contextPtr->coeffEstEntropyCoderPtr,
			candidateBuffer->residualQuantCoeffPtr,
			yCountNonZeroCoeffs[0],
			&yTuCoeffBits,
			contextPtr->cuStats->size,
			candidateBuffer->candidatePtr->type,
			candidateBuffer->candidatePtr->intraLumaMode,
			contextPtr->pfMdMode,
			contextPtr->coeffCabacUpdate,
			&(candidateBuffer->candBuffCoeffCtxModel),
			contextPtr->CabacCost);

		TuCalcCostLuma(
			cuStatsPtr->size,
			candidateBuffer->candidatePtr,
			0,
			cuStatsPtr->size,
			yCountNonZeroCoeffs[0],
			tuFullDistortion[0],
			&yTuCoeffBits,
			contextPtr->qp,
			contextPtr->fullLambda,
			contextPtr->fullChromaLambda);

		(*yCoeffBits)  += yTuCoeffBits;
		yFullDistortion[DIST_CALC_RESIDUAL]   = tuFullDistortion[0][DIST_CALC_RESIDUAL];
		yFullDistortion[DIST_CALC_PREDICTION] = tuFullDistortion[0][DIST_CALC_PREDICTION];
		candidateBuffer->yDc[0] = ABS(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[tuOriginIndex]);
		candidateBuffer->yCountNonZeroCoeffs[0] = (EB_U16)yCountNonZeroCoeffs[0];
	}
}


void UnifiedQuantizeInvQuantize_R(
	EB_S16                *coeff,
	const EB_U32           coeffStride,
	EB_S16                *quantCoeff,
	EB_S16                *reconCoeff,
	EB_U32                 qp,
	EB_U32                 bitDepth,
	EB_U32                 areaSize,
	EB_PICTURE               sliceType,
	EB_U32                *yCountNonZeroCoeffs,
	EB_S8                  mdNonZeroCoeff,
	EB_PF_MODE		       pfMode,
	EB_U32                 tuOriginX,
	EB_U32                 tuOriginY,
	EB_U32                 lcuOriginY,
	EB_U32                 enableCbflag,
	EB_U8                  enableContouringQCUpdateFlag,
	EB_MODETYPE		       type,
	EB_U32                 componentType,
	EB_U32                 temporalLayerIndex,
	EB_BOOL                encDecFlag,
	EB_U32                 dZoffset,
    EB_RDOQ_PMCORE_TYPE    rdoqPmCoreMethod,
    CabacEncodeContext_t  *cabacEncodeCtxPtr,
	EB_U64                 lambda,
	EB_U32                 intraLumaMode,
	EB_U32                 intraChromaMode,
	CabacCost_t           *CabacCost)

{

	//for the Quant
	const EB_S32 qpRem = (EB_S32)QpModSix[qp]; //the output is between 0-5
	const EB_S32 qpPer = (EB_S32)QpDivSix[qp] + TRANS_BIT_INCREMENT; //the output is between 0 and 8+TRANS_BIT_INCREMENT   (CHKN TRANS_BIT_INCREMENT =   0)
	const EB_U32 qFunc = QFunc[qpRem]; // 15 bits

	const EB_U32 internalBitDepth = (EB_U32)bitDepth + TRANS_BIT_INCREMENT;  //CHKN always 8 for 8 bit

	const EB_U32 transformShiftNum = MAX_TR_DYNAMIC_RANGE - internalBitDepth - Log2f(areaSize);
	const EB_S32 shiftedQBits = QUANT_SHIFT + qpPer + transformShiftNum;
	const EB_U32 q_offset = ((sliceType == EB_I_PICTURE || sliceType == EB_IDR_PICTURE) ? QUANT_OFFSET_I : QUANT_OFFSET_P) << (shiftedQBits - 9);

	//for the iQuant
	const EB_S32 shiftedFFunc = (qpPer > 8) ? (EB_S32)FFunc[qpRem] << (qpPer - 2) : (EB_S32)FFunc[qpRem] << qpPer; // this is 6+8+TRANS_BIT_INCREMENT
	const EB_S32 shiftNum = (qpPer > 8) ? QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShiftNum - 2 : QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShiftNum;
	const EB_S32 iq_offset = 1 << (shiftNum - 1);
	EB_U32 adptive_qp_offset;

	adptive_qp_offset = q_offset;

	(void)(encDecFlag);
	(void)(mdNonZeroCoeff);
	adptive_qp_offset = dZoffset ? (dZoffset * (1 << shiftedQBits) / 20) : adptive_qp_offset;

	if (pfMode == PF_N2) {
		areaSize = areaSize >> 1;
	} else if (pfMode == PF_N4) {
		areaSize = areaSize >> 2;
	}

	if (rdoqPmCoreMethod){

		DecoupledQuantizeInvQuantizeLoops(
			coeff,
			coeffStride,
			quantCoeff,
			reconCoeff,
			cabacEncodeCtxPtr,
			lambda,
			type,
			intraLumaMode,
			intraChromaMode,
			componentType,
            (EB_U8)temporalLayerIndex,
			temporalLayerIndex < 3 ? EB_TRUE : EB_FALSE,
            (EB_U8)0,
            (EB_U16)qp,
            bitDepth,
			CabacCost,
			qFunc,
			q_offset,
			shiftedQBits,
			shiftedFFunc,
			iq_offset,
			shiftNum,
			areaSize,
			&(*yCountNonZeroCoeffs),
            rdoqPmCoreMethod);
	}else{

		QiQ_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][areaSize >> 3](
			coeff,
			coeffStride,
			quantCoeff,
			reconCoeff,
			qFunc,
			adptive_qp_offset,
			shiftedQBits,
			shiftedFFunc,
			iq_offset,
			shiftNum,
			areaSize,
			&(*yCountNonZeroCoeffs));

		UpdateQiQCoef_R(
			quantCoeff,
			reconCoeff,
			coeffStride,
			shiftedFFunc,
			iq_offset,
			shiftNum,
			areaSize,
			&(*yCountNonZeroCoeffs),
			componentType,
			sliceType,
			temporalLayerIndex,
			enableCbflag,
			enableContouringQCUpdateFlag);
	}

	(void)tuOriginX;
	(void)tuOriginY;
	(void)lcuOriginY;
    (void)type;
}
/****************************************
 ************  Full loop ****************
****************************************/
void FullLoop_R (
	LargestCodingUnit_t           *lcuPtr,
   ModeDecisionCandidateBuffer_t  *candidateBuffer,
   ModeDecisionContext_t          *contextPtr,
   const CodedUnitStats_t         *cuStatsPtr,
   EbPictureBufferDesc_t          *inputPicturePtr,
   PictureControlSet_t            *pictureControlSetPtr,
   EB_U32                          componentMask,
   EB_U32                          cbQp,
   EB_U32                          crQp,
   EB_U32						  *cbCountNonZeroCoeffs,
   EB_U32						  *crCountNonZeroCoeffs)
{
	(void)lcuPtr;

    EB_S16                *chromaResidualPtr;
    EB_U32                 tuIndex;
    EB_U32                 tuOriginIndex;
    EB_U32                 tuCbOriginIndex;
	EB_U32                 tuCrOriginIndex;
	EB_U32                 tuCount;
    const TransformUnitStats_t  *tuStatPtr;
    EB_U32                 tuItr;
    EB_U32                 tuSize;
    EB_U32                 chromatTuSize;
    EB_U32                 tuOriginX;
    EB_U32                 tuOriginY;
    
    EbPictureBufferDesc_t         * tuTransCoeffTmpPtr;
    EbPictureBufferDesc_t         * tuQuantCoeffTmpPtr;
    
    if (cuStatsPtr->size == MAX_LCU_SIZE) {
        tuCount = 4;
        tuIndex = 1;
        tuTransCoeffTmpPtr = contextPtr->transQuantBuffersPtr->tuTransCoeffNxNPtr;
        tuQuantCoeffTmpPtr = candidateBuffer->residualQuantCoeffPtr;

    }
    else {
        tuCount = 1;
        tuIndex = 0;
        tuTransCoeffTmpPtr = contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr;
        tuQuantCoeffTmpPtr = candidateBuffer->residualQuantCoeffPtr;
    }

    tuItr = 0;
    do {
        tuStatPtr = GetTransformUnitStats(tuIndex);
        tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatPtr->offsetX);
        tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatPtr->offsetY);
        tuSize = cuStatsPtr->size >> tuStatPtr->depth;
        chromatTuSize = tuSize == 4 ? tuSize : (tuSize >> 1);
        tuOriginIndex = tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideY;
        tuCbOriginIndex = tuSize == 4 ?
            tuOriginIndex :
            ((tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideCb) >> 1);
        tuCrOriginIndex = tuSize == 4 ?
            tuOriginIndex :
            ((tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideCr) >> 1);

        //    This function replaces the previous Intra Chroma mode if the LM fast
        //    cost is better.
        //    *Note - this might require that we have inv transform in the loop
        EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;

        if (chromatTuSize == 4)
            correctedPFMode = PF_OFF;
        else if (chromatTuSize == 8 && contextPtr->pfMdMode == PF_N4)
            correctedPFMode = PF_N2;

        if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
            // Configure the Chroma Residual Ptr
            chromaResidualPtr = //(candidateBuffer->candidatePtr->type  == INTRA_MODE )?
                 //&(((EB_S16*) candidateBuffer->intraChromaResidualPtr->bufferCb)[tuChromaOriginIndex]):
                &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCb)[tuCbOriginIndex]);

            // Cb Transform 
            EstimateTransform(
                chromaResidualPtr,
                candidateBuffer->residualQuantCoeffPtr->strideCb,
                &(((EB_S16*)tuTransCoeffTmpPtr->bufferCb)[tuCbOriginIndex]),
                tuTransCoeffTmpPtr->strideCb,
                chromatTuSize,
                contextPtr->transformInnerArrayPtr,
                0,
                EB_FALSE,
                correctedPFMode);

			UnifiedQuantizeInvQuantize_R(
				&(((EB_S16*)tuTransCoeffTmpPtr->bufferCb)[tuCbOriginIndex]),
				tuTransCoeffTmpPtr->strideCb,
				&(((EB_S16*)tuQuantCoeffTmpPtr->bufferCb)[tuCbOriginIndex]),
				&(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[tuCbOriginIndex]),
				cbQp,
				inputPicturePtr->bitDepth,
				chromatTuSize,
				pictureControlSetPtr->sliceType,
				&(cbCountNonZeroCoeffs[tuIndex]),
				-1,
				correctedPFMode,
				0,
				0,
				0,
				0,
				0,
				candidateBuffer->candidatePtr->type,
				COMPONENT_CHROMA,
				pictureControlSetPtr->temporalLayerIndex,
				EB_FALSE,
				0,
                contextPtr->rdoqPmCoreMethod,
				(CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
				contextPtr->fullLambda,
				candidateBuffer->candidatePtr->intraLumaMode,
				EB_INTRA_CHROMA_DM,
				pictureControlSetPtr->cabacCost);

            if (contextPtr->spatialSseFullLoop == EB_TRUE) {
                if (cbCountNonZeroCoeffs[tuIndex]) {

                    EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;
                    EB_U32 chromatTuSize = (tuSize >> 1);
                    if (chromatTuSize == 4)
                        correctedPFMode = PF_OFF;
                    else if (chromatTuSize == 8 && contextPtr->pfMdMode == PF_N4)
                        correctedPFMode = PF_N2;

                    if (correctedPFMode) {
                        PfZeroOutUselessQuadrants(
                            &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[tuCbOriginIndex]),
                            candidateBuffer->reconCoeffPtr->strideCb,
                            (chromatTuSize >> 1));
                    }

                    EstimateInvTransform(
                        &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[tuCbOriginIndex]),
                        candidateBuffer->reconCoeffPtr->strideCb,
                        &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb)[tuCbOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                        chromatTuSize,
                        contextPtr->transformInnerArrayPtr,
                        BIT_INCREMENT_8BIT,
                        EB_FALSE,
                        EB_FALSE);

                    PictureAddition(
                        &(candidateBuffer->predictionPtr->bufferCb[tuCbOriginIndex]),
                        candidateBuffer->predictionPtr->strideCb,
                        &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb))[tuCbOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                        &(candidateBuffer->reconPtr->bufferCb[tuCbOriginIndex]),
                        candidateBuffer->reconPtr->strideCb,
                        chromatTuSize,
                        chromatTuSize);

                }
                else {

                    PictureCopy8Bit(
                        candidateBuffer->predictionPtr,
                        tuOriginIndex,
                        tuCbOriginIndex,
                        candidateBuffer->reconPtr,
                        tuOriginIndex,
                        tuCbOriginIndex,
                        tuSize,
                        tuSize,
                        chromatTuSize,
                        chromatTuSize,
                        PICTURE_BUFFER_DESC_Cb_FLAG);
                }
            }

        }


         if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
             // Configure the Chroma Residual Ptr 
             chromaResidualPtr = //(candidateBuffer->candidatePtr->type  == INTRA_MODE )?
                  //&(((EB_S16*) candidateBuffer->intraChromaResidualPtr->bufferCr)[tuChromaOriginIndex]):
                 &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCr)[tuCrOriginIndex]);

             // Cr Transform 
             EstimateTransform(
                 chromaResidualPtr,
                 candidateBuffer->residualQuantCoeffPtr->strideCr,
                 &(((EB_S16*)tuTransCoeffTmpPtr->bufferCr)[tuCrOriginIndex]),
                 tuTransCoeffTmpPtr->strideCr,
                 chromatTuSize,
                 contextPtr->transformInnerArrayPtr,
                 0,
                 EB_FALSE,
                 correctedPFMode);

             UnifiedQuantizeInvQuantize_R(
                 &(((EB_S16*)tuTransCoeffTmpPtr->bufferCr)[tuCrOriginIndex]),
                 tuTransCoeffTmpPtr->strideCr,
                 &(((EB_S16*)tuQuantCoeffTmpPtr->bufferCr)[tuCrOriginIndex]),
                 &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[tuCrOriginIndex]),
                 crQp,
                 inputPicturePtr->bitDepth,
                 chromatTuSize,
                 pictureControlSetPtr->sliceType,
                 &(crCountNonZeroCoeffs[tuIndex]),
                 -1,
                 correctedPFMode,
                 0,
                 0,
                 0,
                 0,
                 0,
                 candidateBuffer->candidatePtr->type,
                 COMPONENT_CHROMA,
                 pictureControlSetPtr->temporalLayerIndex,
                 EB_FALSE,
                 0,
                 contextPtr->rdoqPmCoreMethod,
				 (CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
				 contextPtr->fullLambda,
				 candidateBuffer->candidatePtr->intraLumaMode,
				 EB_INTRA_CHROMA_DM,
				 pictureControlSetPtr->cabacCost);

             if (contextPtr->spatialSseFullLoop == EB_TRUE) {
                 if (crCountNonZeroCoeffs[tuIndex]) {

                     EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;
                     EB_U32 chromatTuSize = (tuSize >> 1);
                     if (chromatTuSize == 4)
                         correctedPFMode = PF_OFF;
                     else if (chromatTuSize == 8 && contextPtr->pfMdMode == PF_N4)
                         correctedPFMode = PF_N2;

                     if (correctedPFMode) {
                         PfZeroOutUselessQuadrants(
                             &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[tuCbOriginIndex]),
                             candidateBuffer->reconCoeffPtr->strideCr,
                             (chromatTuSize >> 1));
                     }

                     EstimateInvTransform(
                         &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[tuCbOriginIndex]),
                         candidateBuffer->reconCoeffPtr->strideCr,
                         &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr)[tuCbOriginIndex]),
                         contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                         chromatTuSize,
                         contextPtr->transformInnerArrayPtr,
                         BIT_INCREMENT_8BIT,
                         EB_FALSE,
                         EB_FALSE);

                     PictureAddition(
                         &(candidateBuffer->predictionPtr->bufferCr[tuCbOriginIndex]),
                         candidateBuffer->predictionPtr->strideCr,
                         &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr))[tuCbOriginIndex]),
                         contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                         &(candidateBuffer->reconPtr->bufferCr[tuCbOriginIndex]),
                         candidateBuffer->reconPtr->strideCr,
                         chromatTuSize,
                         chromatTuSize);

                 }
                 else {

                     PictureCopy8Bit(
                         candidateBuffer->predictionPtr,
                         tuOriginIndex,
                         tuCbOriginIndex,
                         candidateBuffer->reconPtr,
                         tuOriginIndex,
                         tuCbOriginIndex,
                         tuSize,
                         tuSize,
                         chromatTuSize,
                         chromatTuSize,
                         PICTURE_BUFFER_DESC_Cr_FLAG);
                 }
             }

         }
        
        ++tuItr;
		tuIndex = tuIndexList[tuStatPtr->depth][tuItr];

    } while (tuItr < tuCount);

}

//****************************************
// ************ CuFullDistortionFastTuMode ****************
//****************************************/
void CuFullDistortionFastTuMode_R (
    EbPictureBufferDesc_t          *inputPicturePtr,
    EB_U32                          inputCbOriginIndex,
	LargestCodingUnit_t            *lcuPtr,
	ModeDecisionCandidateBuffer_t  *candidateBuffer,
	ModeDecisionContext_t            *contextPtr ,
	ModeDecisionCandidate_t		   *candidatePtr,
	const CodedUnitStats_t		   *cuStatsPtr,
	EB_U64                          cbFullDistortion[DIST_CALC_TOTAL] , 
	EB_U64                          crFullDistortion[DIST_CALC_TOTAL] , 
	EB_U32                          countNonZeroCoeffs[3][MAX_NUM_OF_TU_PER_CU],
	EB_U32							componentMask,
    EB_U64                         *cbCoeffBits,
    EB_U64                         *crCoeffBits)
{
	(void)lcuPtr;

    EB_U64                          yTuCoeffBits;
    EB_U64                          cbTuCoeffBits;
    EB_U64                          crTuCoeffBits;
	EB_U32                          tuOriginIndex;
	EB_U32                          tuOriginX;
	EB_U32                          tuOriginY;
	EB_U32                          currentTuIndex;
	EB_U32                          chromaShift;
    EB_U32                          tuChromaOriginIndex;
	EB_U64                          tuFullDistortion[3][DIST_CALC_TOTAL];
	EbPictureBufferDesc_t          *transformBuffer;
	EB_U32                          tuTotalCount;
	EB_U32							tuSize;
	EB_U32							chromaTuSize;
	const TransformUnitStats_t     *tuStatPtr;
	EB_U32                          tuItr = 0;

    if (cuStatsPtr->size == MAX_LCU_SIZE){
        currentTuIndex = 1;
        transformBuffer = contextPtr->transQuantBuffersPtr->tuTransCoeffNxNPtr;
        tuTotalCount = 4;

    }
    else{
        currentTuIndex = 0;
        transformBuffer = contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr;
        tuTotalCount = 1;
    }

	do { 
            tuStatPtr = GetTransformUnitStats(currentTuIndex);

            tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatPtr->offsetX);
            tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatPtr->offsetY);
            tuSize    = cuStatsPtr->size >> tuStatPtr->depth;
            chromaTuSize = tuSize == 4 ? tuSize : (tuSize >> 1);
            tuOriginIndex = tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideY ; 
            tuChromaOriginIndex = tuSize == 4 ?
                tuOriginIndex : 
                ((tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideCb) >> 1);

            // Reset the Bit Costs
            yTuCoeffBits  = 0;
			cbTuCoeffBits = 0;
            crTuCoeffBits = 0;

			if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK){

                EB_U32 countNonZeroCoeffsAll[3];
                countNonZeroCoeffsAll[0] = countNonZeroCoeffs[0][currentTuIndex];
                countNonZeroCoeffsAll[1] = countNonZeroCoeffs[1][currentTuIndex];
                countNonZeroCoeffsAll[2] = countNonZeroCoeffs[2][currentTuIndex];

            EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;
       
            if(chromaTuSize == 4)
                correctedPFMode = PF_OFF;
            else if(chromaTuSize == 8 && contextPtr->pfMdMode == PF_N4)
                correctedPFMode = PF_N2;

            if (contextPtr->spatialSseFullLoop == EB_TRUE) {

                tuFullDistortion[1][DIST_CALC_RESIDUAL] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(chromaTuSize) - 2](
                    &(inputPicturePtr->bufferCb[inputCbOriginIndex]),
                    inputPicturePtr->strideCb,
                    &(candidateBuffer->reconPtr->bufferCb[tuChromaOriginIndex]),
                    candidateBuffer->reconPtr->strideCb,
                    chromaTuSize,
                    chromaTuSize);


                tuFullDistortion[1][DIST_CALC_PREDICTION] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(chromaTuSize) - 2](
                    &(inputPicturePtr->bufferCb[inputCbOriginIndex]),
                    inputPicturePtr->strideCb,
                    &(candidateBuffer->predictionPtr->bufferCb[tuChromaOriginIndex]),
                    candidateBuffer->predictionPtr->strideCb,
                    chromaTuSize,
                    chromaTuSize);

                tuFullDistortion[2][DIST_CALC_RESIDUAL] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(chromaTuSize) - 2](
                    &(inputPicturePtr->bufferCr[inputCbOriginIndex]),
                    inputPicturePtr->strideCr,
                    &(candidateBuffer->reconPtr->bufferCr[tuChromaOriginIndex]),
                    candidateBuffer->reconPtr->strideCr,
                    chromaTuSize,
                    chromaTuSize);

                tuFullDistortion[2][DIST_CALC_PREDICTION] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(chromaTuSize) - 2](
                    &(inputPicturePtr->bufferCr[inputCbOriginIndex]),
                    inputPicturePtr->strideCr,
                    &(candidateBuffer->predictionPtr->bufferCr[tuChromaOriginIndex]),
                    candidateBuffer->predictionPtr->strideCr,
                    chromaTuSize,
                    chromaTuSize);

            }
            else {
                // *Full Distortion (SSE)
                // *Note - there are known issues with how this distortion metric is currently
                //    calculated.  The amount of scaling between the two arrays is not 
                //    equivalent.
                PictureFullDistortion_R(
                    transformBuffer,
                    tuOriginIndex,
                    tuChromaOriginIndex,
                    candidateBuffer->reconCoeffPtr,
                    (tuSize >> contextPtr->pfMdMode),
                    (chromaTuSize >> correctedPFMode),
                    PICTURE_BUFFER_DESC_CHROMA_MASK,//componentMask, 
                    tuFullDistortion[0],
                    tuFullDistortion[1],
                    tuFullDistortion[2],
                    countNonZeroCoeffsAll,
                    candidateBuffer->candidatePtr->type);


                chromaShift = 2 * (7 - Log2f(chromaTuSize));
                tuFullDistortion[1][DIST_CALC_RESIDUAL] = (tuFullDistortion[1][DIST_CALC_RESIDUAL] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
                tuFullDistortion[1][DIST_CALC_PREDICTION] = (tuFullDistortion[1][DIST_CALC_PREDICTION] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
                tuFullDistortion[2][DIST_CALC_RESIDUAL] = (tuFullDistortion[2][DIST_CALC_RESIDUAL] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
                tuFullDistortion[2][DIST_CALC_PREDICTION] = (tuFullDistortion[2][DIST_CALC_PREDICTION] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
            
            }

             TuEstimateCoeffBits_R(
                 tuOriginIndex,
                 tuChromaOriginIndex,
                 PICTURE_BUFFER_DESC_CHROMA_MASK,//componentMask,
                 contextPtr->coeffEstEntropyCoderPtr,
                 candidateBuffer->residualQuantCoeffPtr,
                 countNonZeroCoeffs[0][currentTuIndex],
                 countNonZeroCoeffs[1][currentTuIndex],
                 countNonZeroCoeffs[2][currentTuIndex],
                 &yTuCoeffBits,
                 &cbTuCoeffBits,
                 &crTuCoeffBits,
                 candidateBuffer->candidatePtr->transformSize,
                 candidateBuffer->candidatePtr->transformChromaSize,
                 candidateBuffer->candidatePtr->type,
                 candidateBuffer->candidatePtr->intraLumaMode,
                 EB_INTRA_CHROMA_DM,
                 correctedPFMode,
                 contextPtr->coeffCabacUpdate,
                 &(candidateBuffer->candBuffCoeffCtxModel),
                 contextPtr->CabacCost);

			TuCalcCost(
                contextPtr->cuSize,
                candidatePtr,
                currentTuIndex,
                tuSize,
                chromaTuSize,
                countNonZeroCoeffs[0][currentTuIndex],
			    countNonZeroCoeffs[1][currentTuIndex],
			    countNonZeroCoeffs[2][currentTuIndex],
                tuFullDistortion[0],
                tuFullDistortion[1],
                tuFullDistortion[2],
                PICTURE_BUFFER_DESC_CHROMA_MASK,//componentMask, 
                &yTuCoeffBits,
                &cbTuCoeffBits,
                &crTuCoeffBits,
                contextPtr->qp,
                contextPtr->fullLambda,
                contextPtr->fullChromaLambda);   

			 *cbCoeffBits += cbTuCoeffBits;
             *crCoeffBits += crTuCoeffBits;
             cbFullDistortion[DIST_CALC_RESIDUAL] += tuFullDistortion[1][DIST_CALC_RESIDUAL];
             crFullDistortion[DIST_CALC_RESIDUAL] += tuFullDistortion[2][DIST_CALC_RESIDUAL];
             cbFullDistortion[DIST_CALC_PREDICTION] += tuFullDistortion[1][DIST_CALC_PREDICTION];
             crFullDistortion[DIST_CALC_PREDICTION] += tuFullDistortion[2][DIST_CALC_PREDICTION];

			}

            ++tuItr;
			currentTuIndex = tuIndexList[tuStatPtr->depth][tuItr];

	} while (tuItr < tuTotalCount);
}


EB_U32 ExitInterDepthDecision(
	ModeDecisionContext_t          *contextPtr,
	EB_U32                          leafIndex,
	LargestCodingUnit_t            *tbPtr,
	EB_U32                          lcuAddr,
	EB_U32                          tbOriginX,
	EB_U32                          tbOriginY,
	EB_U64                          fullLambda,
	MdRateEstimationContext_t      *mdRateEstimationPtr,
	PictureControlSet_t            *pictureControlSetPtr)
{
	EB_U32                     lastCuIndex;
	EB_U32                     leftCuIndex;
	EB_U32                     topCuIndex;
	EB_U32                     topLeftCuIndex;
	EB_U32                     depthZeroCandidateCuIndex;
	EB_U32                     depthOneCandidateCuIndex = leafIndex;
	EB_U32                     depthTwoCandidateCuIndex = leafIndex;
	EB_U64                     depthNRate = 0;
	EB_U64                     depthNPlusOneRate = 0;
	EB_U64                     depthNCost = 0;
	EB_U64                     depthNPlusOneCost = 0;
	EB_U32                     cuOriginX;
	EB_U32                     cuOriginY;
	EB_U32                     tbMaxDepth = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->maxLcuDepth;

	EncodeContext_t           *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;
	SequenceControlSet_t      *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	const CodedUnitStats_t    *curCuStatsPtr;
	const CodedUnitStats_t    *depthTwoCuStatsPtr;
	const CodedUnitStats_t    *depthOneCuStatsPtr;
	const CodedUnitStats_t    *depthZeroCuStatsPtr;

	(void)lcuAddr;
	lastCuIndex = leafIndex;
	curCuStatsPtr = GetCodedUnitStats(leafIndex);
	cuOriginX = tbOriginX + curCuStatsPtr->originX;
	cuOriginY = tbOriginY + curCuStatsPtr->originY;



	//Parent is winner, update its cost, and trigger and inter-depth check-point.
	EB_U64 SplitRate = 0;
	SplitFlagRate(
		contextPtr,
		tbPtr->codedLeafArrayPtr[leafIndex],
		0,
		&SplitRate,
		contextPtr->fullLambda,
		contextPtr->mdRateEstimationPtr,
		sequenceControlSetPtr->maxLcuDepth);

	contextPtr->mdLocalCuUnit[leafIndex].cost += SplitRate;

	if (curCuStatsPtr->depth == 0) {
		contextPtr->groupOf16x16BlocksCount = 0;
	}
	else if (curCuStatsPtr->depth == 1) {
		contextPtr->groupOf16x16BlocksCount++;
		contextPtr->groupOf8x8BlocksCount = 0;
	}
	else if (curCuStatsPtr->depth == 2) {
		contextPtr->groupOf8x8BlocksCount++;
	}


	/*** Stage 0: Inter depth decision: depth 2 vs depth 3 ***/

	// Walks to the last coded 8x8 block for merging
	if ((GROUP_OF_4_8x8_BLOCKS(cuOriginX, cuOriginY))) {

		depthTwoCandidateCuIndex = leafIndex - DEPTH_THREE_STEP - DEPTH_THREE_STEP - DEPTH_THREE_STEP - 1;

		contextPtr->groupOf8x8BlocksCount++;

		// From the last coded cu index, get the indices of the left, top, and top left cus
		leftCuIndex = leafIndex - DEPTH_THREE_STEP;
		topCuIndex = leftCuIndex - DEPTH_THREE_STEP;
		topLeftCuIndex = topCuIndex - DEPTH_THREE_STEP;

		// From the top left index, get the index of the candidate pu for merging
		depthTwoCandidateCuIndex = topLeftCuIndex - 1;

		// Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
		//   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

		// Compute depth N cost
		SplitFlagRate(
			contextPtr,
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex],
			0,
			&depthNRate,
			fullLambda,
			mdRateEstimationPtr,
			tbMaxDepth); 
		if (contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].testedCuFlag == EB_FALSE)
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = MAX_CU_COST;

		depthNCost = contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost + depthNRate;
		// Compute depth N+1 cost
		SplitFlagRate(
			contextPtr,
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex],
			1,
			&depthNPlusOneRate,
			fullLambda,
			mdRateEstimationPtr,
			tbMaxDepth);
		depthNPlusOneCost =
			contextPtr->mdLocalCuUnit[leafIndex].cost +
			contextPtr->mdLocalCuUnit[leftCuIndex].cost +
			contextPtr->mdLocalCuUnit[topCuIndex].cost +
			contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
			depthNPlusOneRate;


		// Inter depth comparison: depth 2 vs depth 3
		if (depthNCost <= depthNPlusOneCost){

			// If the cost is low enough to warrant not spliting further:
			// 1. set the split flag of the candidate pu for merging to false
			// 2. update the last pu index
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex]->splitFlag = EB_FALSE;
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = depthNCost;
			lastCuIndex = depthTwoCandidateCuIndex;
		}
		else {
			// If the cost is not low enough:
			// update the cost of the candidate pu for merging
			// this update is required for the next inter depth decision
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = depthNPlusOneCost;
		}


	}

	// Stage 1: Inter depth decision: depth 1 vs depth 2 

	// Walks to the last coded 16x16 block for merging
	depthTwoCuStatsPtr = GetCodedUnitStats(depthTwoCandidateCuIndex);
	cuOriginX = tbOriginX + depthTwoCuStatsPtr->originX;
	cuOriginY = tbOriginY + depthTwoCuStatsPtr->originY;

	if (GROUP_OF_4_16x16_BLOCKS(cuOriginX, cuOriginY) &&
		(contextPtr->groupOf8x8BlocksCount == 4)){


		depthOneCandidateCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP - DEPTH_TWO_STEP - DEPTH_TWO_STEP - 1;

		contextPtr->groupOf8x8BlocksCount = 0;
		contextPtr->groupOf16x16BlocksCount++;

		// From the last coded pu index, get the indices of the left, top, and top left pus
		leftCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP;
		topCuIndex = leftCuIndex - DEPTH_TWO_STEP;
		topLeftCuIndex = topCuIndex - DEPTH_TWO_STEP;

		// Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
		//   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

		// From the top left index, get the index of the candidate pu for merging
		depthOneCandidateCuIndex = topLeftCuIndex - 1;

		depthOneCuStatsPtr = GetCodedUnitStats(depthOneCandidateCuIndex);
		if (depthOneCuStatsPtr->depth == 1) {

			// Compute depth N cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex],
				0,
				&depthNRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			if (contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].testedCuFlag == EB_FALSE)
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = MAX_CU_COST;
			depthNCost = contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost + depthNRate;

			// Compute depth N+1 cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex],
				1,
				&depthNPlusOneRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			depthNPlusOneCost =
				contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost +
				contextPtr->mdLocalCuUnit[leftCuIndex].cost +
				contextPtr->mdLocalCuUnit[topCuIndex].cost +
				contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
				depthNPlusOneRate;
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[leftCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[topCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[topLeftCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);


			// Inter depth comparison: depth 1 vs depth 2
			if (depthNCost <= depthNPlusOneCost){

				// If the cost is low enough to warrant not spliting further:
				// 1. set the split flag of the candidate pu for merging to false
				// 2. update the last pu index
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex]->splitFlag = EB_FALSE;
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = depthNCost;
				lastCuIndex = depthOneCandidateCuIndex;
			}
			else {
				// If the cost is not low enough:
				// update the cost of the candidate pu for merging
				// this update is required for the next inter depth decision
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = depthNPlusOneCost;
			}


		}
	}

	// Stage 2: Inter depth decision: depth 0 vs depth 1 

	// Walks to the last coded 32x32 block for merging
	// Stage 2 isn't performed in I slices since the abcense of 64x64 candidates
	depthOneCuStatsPtr = GetCodedUnitStats(depthOneCandidateCuIndex);
	cuOriginX = tbOriginX + depthTwoCuStatsPtr->originX;
	cuOriginY = tbOriginY + depthTwoCuStatsPtr->originY;
	if ((pictureControlSetPtr->sliceType == EB_P_PICTURE || pictureControlSetPtr->sliceType == EB_B_PICTURE)
		&& GROUP_OF_4_32x32_BLOCKS(cuOriginX, cuOriginY) &&
		(contextPtr->groupOf16x16BlocksCount == 4)) {

		depthZeroCandidateCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP - DEPTH_ONE_STEP - DEPTH_ONE_STEP - 1;

		contextPtr->groupOf16x16BlocksCount = 0;

		// From the last coded pu index, get the indices of the left, top, and top left pus
		leftCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP;
		topCuIndex = leftCuIndex - DEPTH_ONE_STEP;
		topLeftCuIndex = topCuIndex - DEPTH_ONE_STEP;

		// Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
		//   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

		// From the top left index, get the index of the candidate pu for merging
		depthZeroCandidateCuIndex = topLeftCuIndex - 1;

		depthZeroCuStatsPtr = GetCodedUnitStats(depthZeroCandidateCuIndex);
		if (depthZeroCuStatsPtr->depth == 0) {

			// Compute depth N cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex],
				0,
				&depthNRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			if (contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].testedCuFlag == EB_FALSE)
				contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].cost = MAX_CU_COST;
			depthNCost = contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].cost + depthNRate;
			// Compute depth N+1 cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex],
				1,
				&depthNPlusOneRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			depthNPlusOneCost =
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost +
				contextPtr->mdLocalCuUnit[leftCuIndex].cost +
				contextPtr->mdLocalCuUnit[topCuIndex].cost +
				contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
				depthNPlusOneRate;

			// Inter depth comparison: depth 0 vs depth 1
			if (depthNCost <= depthNPlusOneCost){

				// If the cost is low enough to warrant not spliting further:
				// 1. set the split flag of the candidate pu for merging to false
				// 2. update the last pu index
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex]->splitFlag = EB_FALSE;
				lastCuIndex = depthZeroCandidateCuIndex;
			}


		}
	}

	return lastCuIndex;
}

EB_BOOL  StopSplitCondition(
    SequenceControlSet_t    *sequenceControlSetPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    ModeDecisionContext_t   *contextPtr,
    const CodedUnitStats_t  *curCuStatsPtr,
    EB_U32                   lcuAddr,
    EB_U32                   leafIndex)
{

    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuAddr];

    EB_BOOL stopSplitFlag = EB_TRUE;

    if ( pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL85_DEPTH_MODE ||
         pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL84_DEPTH_MODE ||     
         (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_FULL85_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_FULL84_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_AVC_DEPTH_MODE))
        ) {

        stopSplitFlag = EB_FALSE;
    }
    else if (pictureControlSetPtr->temporalLayerIndex == 0) {
        stopSplitFlag = EB_FALSE;
    }
    else{
        if (sequenceControlSetPtr->staticConfig.qp >= 20    && 
            pictureControlSetPtr->sliceType != EB_I_PICTURE      && 
            pictureControlSetPtr->temporalLayerIndex == 0   && 
            pictureControlSetPtr->ParentPcsPtr->logoPicFlag &&
            pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum) {

            stopSplitFlag = EB_FALSE;
        }


        if (stopSplitFlag != EB_FALSE)
        {

            EB_U32 lcuEdgeFlag = pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum == 0 ? 0 : 1;
            EB_U64 d0Th;
            EB_U64 d1Th;
            EB_U64 d2Th;

            d0Th = depth0Th[lcuEdgeFlag][pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex];
            d1Th = depth1Th[lcuEdgeFlag][pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex];
            d2Th = depth2Th[lcuEdgeFlag][pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex];

            EB_BOOL interSlice =    (pictureControlSetPtr->sliceType == EB_P_PICTURE) || (pictureControlSetPtr->sliceType == EB_B_PICTURE) ? EB_TRUE : EB_FALSE;
            EB_BOOL stopAtDepth0 = ((curCuStatsPtr->depth == 0) && (contextPtr->mdLocalCuUnit[leafIndex].fullDistortion < d0Th)) ? EB_TRUE : EB_FALSE;
            EB_BOOL stopAtDepth1 = ((curCuStatsPtr->depth == 1) && (contextPtr->mdLocalCuUnit[leafIndex].fullDistortion < d1Th)) ? EB_TRUE : EB_FALSE;
            EB_BOOL stopAtDepth2 = ((curCuStatsPtr->depth == 2) && (contextPtr->mdLocalCuUnit[leafIndex].fullDistortion < d2Th)) ? EB_TRUE : EB_FALSE;

            stopSplitFlag = (interSlice && (stopAtDepth0 || stopAtDepth1 || stopAtDepth2)) ? EB_TRUE : EB_FALSE;

            if  (!lcuParams->isCompleteLcu                                                        ||
                 pictureControlSetPtr->ParentPcsPtr->lcuIsolatedNonHomogeneousAreaArray[lcuAddr] || 
                 (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE && pictureControlSetPtr->lcuPtrArray[lcuAddr]->auraStatus == AURA_STATUS_1)) { 

                stopSplitFlag = EB_FALSE;
            }

        }

    }
    return stopSplitFlag;
}

/**********************************************
 * Inter Depth Split Decision
 **********************************************/
EB_U32 ProductPerformInterDepthDecision(
    ModeDecisionContext_t          *contextPtr,
    EB_U32                          leafIndex,
    LargestCodingUnit_t            *tbPtr,
    EB_U32                          lcuAddr,
    EB_U32                          tbOriginX,
    EB_U32                          tbOriginY,
    EB_U64                          fullLambda,
    MdRateEstimationContext_t      *mdRateEstimationPtr,
    PictureControlSet_t            *pictureControlSetPtr)
{
    EB_U32                     lastCuIndex;
    EB_U32                     leftCuIndex;
    EB_U32                     topCuIndex;
    EB_U32                     topLeftCuIndex;
    EB_U32                     depthZeroCandidateCuIndex;
    EB_U32                     depthOneCandidateCuIndex = leafIndex;
    EB_U32                     depthTwoCandidateCuIndex = leafIndex;
    EB_U64                     depthNRate = 0;
    EB_U64                     depthNPlusOneRate = 0;
    EB_U64                     depthNCost = 0;
    EB_U64                     depthNPlusOneCost = 0;
    EB_U32                     cuOriginX;
    EB_U32                     cuOriginY;

    EB_U32                     tbMaxDepth = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->maxLcuDepth;
    EB_BOOL                    stopSplitFlag ;
    EB_BOOL                    lastDepthFlag = tbPtr->codedLeafArrayPtr[leafIndex]->splitFlag == EB_FALSE ? EB_TRUE : EB_FALSE;
    EncodeContext_t           *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;
    SequenceControlSet_t      *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
    const CodedUnitStats_t    *curCuStatsPtr;
    const CodedUnitStats_t    *depthTwoCuStatsPtr;
    const CodedUnitStats_t    *depthOneCuStatsPtr;
    const CodedUnitStats_t    *depthZeroCuStatsPtr;

    lastCuIndex = leafIndex;
    curCuStatsPtr = GetCodedUnitStats(leafIndex);
    cuOriginX = tbOriginX + curCuStatsPtr->originX;
    cuOriginY = tbOriginY + curCuStatsPtr->originY;
    EB_U8 interDepthW12 = 0;
    EB_U8 interDepthW01 = 0;

    stopSplitFlag = StopSplitCondition(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        contextPtr,
        curCuStatsPtr,
        lcuAddr,
        leafIndex);

    if (lastDepthFlag || stopSplitFlag) {
		tbPtr->codedLeafArrayPtr[leafIndex]->splitFlag = EB_FALSE;


        if (curCuStatsPtr->depth == 1) {
            contextPtr->groupOf16x16BlocksCount ++;
        } else if (curCuStatsPtr->depth == 2) {
            contextPtr->groupOf8x8BlocksCount ++;
        }
    }

    /*** Stage 0: Inter depth decision: depth 2 vs depth 3 ***/

    // Walks to the last coded 8x8 block for merging
    if ((GROUP_OF_4_8x8_BLOCKS(cuOriginX, cuOriginY))) {

        depthTwoCandidateCuIndex = leafIndex - DEPTH_THREE_STEP - DEPTH_THREE_STEP - DEPTH_THREE_STEP - 1;

        contextPtr->groupOf8x8BlocksCount ++;

        // From the last coded cu index, get the indices of the left, top, and top left cus
        leftCuIndex = leafIndex - DEPTH_THREE_STEP;
        topCuIndex = leftCuIndex - DEPTH_THREE_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_THREE_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depthTwoCandidateCuIndex = topLeftCuIndex - 1;

        // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
        //   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

        // Compute depth N cost
		SplitFlagRate(
			contextPtr,
            tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex],
            0,
            &depthNRate,
            fullLambda,
            mdRateEstimationPtr,
			tbMaxDepth);
		if (contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].testedCuFlag == EB_FALSE)
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = MAX_CU_COST;

		depthNCost = contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost + depthNRate;

		// Compute depth N+1 cost
		SplitFlagRate(
			contextPtr,
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex],
			1,
			&depthNPlusOneRate,
			fullLambda,
			mdRateEstimationPtr,
			tbMaxDepth);
		depthNPlusOneCost =
			contextPtr->mdLocalCuUnit[leafIndex].cost +
			contextPtr->mdLocalCuUnit[leftCuIndex].cost +
			contextPtr->mdLocalCuUnit[topCuIndex].cost +
			contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
			depthNPlusOneRate;
		// Inter depth comparison: depth 2 vs depth 3
		if (depthNCost <= depthNPlusOneCost){

			// If the cost is low enough to warrant not spliting further:
			// 1. set the split flag of the candidate pu for merging to false
			// 2. update the last pu index
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex]->splitFlag = EB_FALSE;
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = depthNCost;
			lastCuIndex = depthTwoCandidateCuIndex;
		}
		else {
			// If the cost is not low enough:
			// update the cost of the candidate pu for merging
			// this update is required for the next inter depth decision
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = depthNPlusOneCost;
		}


    }
    
    // Stage 1: Inter depth decision: depth 1 vs depth 2 
    
    // Walks to the last coded 16x16 block for merging
    depthTwoCuStatsPtr = GetCodedUnitStats(depthTwoCandidateCuIndex);
    cuOriginX  = tbOriginX + depthTwoCuStatsPtr->originX;
    cuOriginY  = tbOriginY + depthTwoCuStatsPtr->originY;
    if (GROUP_OF_4_16x16_BLOCKS(cuOriginX, cuOriginY) && 
        (contextPtr->groupOf8x8BlocksCount == 4 ) ){

        depthOneCandidateCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP - DEPTH_TWO_STEP - DEPTH_TWO_STEP - 1;

        contextPtr->groupOf8x8BlocksCount = 0;
        contextPtr->groupOf16x16BlocksCount ++;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP;
        topCuIndex = leftCuIndex - DEPTH_TWO_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_TWO_STEP;

        // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
        //   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

        // From the top left index, get the index of the candidate pu for merging
        depthOneCandidateCuIndex = topLeftCuIndex - 1;

        depthOneCuStatsPtr = GetCodedUnitStats(depthOneCandidateCuIndex);
        if (depthOneCuStatsPtr->depth == 1) {

            // Compute depth N cost
			SplitFlagRate(
				contextPtr,
                tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex],
                0,
                &depthNRate,
                fullLambda,
                mdRateEstimationPtr,
				tbMaxDepth);
			if (contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].testedCuFlag == EB_FALSE)
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = MAX_CU_COST;
			depthNCost = contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost + depthNRate;

			// Compute depth N+1 cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex],
				1,
				&depthNPlusOneRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			depthNPlusOneCost =
				contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost +
				contextPtr->mdLocalCuUnit[leftCuIndex].cost +
				contextPtr->mdLocalCuUnit[topCuIndex].cost +
				contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
				depthNPlusOneRate;
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[leftCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[topCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[topLeftCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);

			if (depthNPlusOneCost < MAX_CU_COST)
				depthNPlusOneCost = depthNPlusOneCost + ((EB_S64)depthNPlusOneCost*interDepthW12) / 100;

			// Inter depth comparison: depth 1 vs depth 2
			if (depthNCost <= depthNPlusOneCost){

				// If the cost is low enough to warrant not spliting further:
				// 1. set the split flag of the candidate pu for merging to false
				// 2. update the last pu index
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex]->splitFlag = EB_FALSE;
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = depthNCost;
				lastCuIndex = depthOneCandidateCuIndex;
			}
			else {
				// If the cost is not low enough:
				// update the cost of the candidate pu for merging
				// this update is required for the next inter depth decision
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = depthNPlusOneCost;
			}


        }
    }

    // Stage 2: Inter depth decision: depth 0 vs depth 1 

    // Walks to the last coded 32x32 block for merging
    // Stage 2 isn't performed in I slices since the abcense of 64x64 candidates
    depthOneCuStatsPtr = GetCodedUnitStats(depthOneCandidateCuIndex);
    cuOriginX  = tbOriginX + depthTwoCuStatsPtr->originX;
    cuOriginY  = tbOriginY + depthTwoCuStatsPtr->originY;
    if ((pictureControlSetPtr->sliceType == EB_P_PICTURE || pictureControlSetPtr->sliceType == EB_B_PICTURE )
        && GROUP_OF_4_32x32_BLOCKS(cuOriginX, cuOriginY) && 
        (contextPtr->groupOf16x16BlocksCount == 4)) {

        depthZeroCandidateCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP - DEPTH_ONE_STEP - DEPTH_ONE_STEP - 1;

        contextPtr->groupOf16x16BlocksCount = 0;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP;
        topCuIndex = leftCuIndex - DEPTH_ONE_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_ONE_STEP;

        // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
        //   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

        // From the top left index, get the index of the candidate pu for merging
        depthZeroCandidateCuIndex = topLeftCuIndex - 1;
        
        depthZeroCuStatsPtr = GetCodedUnitStats(depthZeroCandidateCuIndex);
        if (depthZeroCuStatsPtr->depth == 0) {
            
            // Compute depth N cost
			SplitFlagRate(
				contextPtr,
                tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex],
                0,
                &depthNRate,
                fullLambda,
                mdRateEstimationPtr,
				tbMaxDepth);
			if (contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].testedCuFlag == EB_FALSE)
				contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].cost = MAX_CU_COST;
			depthNCost = contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].cost + depthNRate;
			// Compute depth N+1 cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex],
				1,
				&depthNPlusOneRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			depthNPlusOneCost =
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost +
				contextPtr->mdLocalCuUnit[leftCuIndex].cost +
				contextPtr->mdLocalCuUnit[topCuIndex].cost +
				contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
				depthNPlusOneRate;
			if (depthNPlusOneCost < MAX_CU_COST)
				depthNPlusOneCost = depthNPlusOneCost + ((EB_S64)depthNPlusOneCost*interDepthW01) / 100;

			// Inter depth comparison: depth 0 vs depth 1
			if (depthNCost <= depthNPlusOneCost){

				// If the cost is low enough to warrant not spliting further:
				// 1. set the split flag of the candidate pu for merging to false
				// 2. update the last pu index
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex]->splitFlag = EB_FALSE;
				lastCuIndex = depthZeroCandidateCuIndex;
			}


         }
    }

    return lastCuIndex;
}

EB_U32 PillarInterDepthDecision(
    ModeDecisionContext_t          *contextPtr,
    EB_U32                          leafIndex,
    LargestCodingUnit_t            *tbPtr,
    EB_U32                          tbOriginX,
    EB_U32                          tbOriginY,
    EB_U64                          fullLambda,
    MdRateEstimationContext_t      *mdRateEstimationPtr,
    PictureControlSet_t            *pictureControlSetPtr)
{
    EB_U32                     lastCuIndex;
    EB_U32                     leftCuIndex;
    EB_U32                     topCuIndex;
    EB_U32                     topLeftCuIndex;
    EB_U32                     depthZeroCandidateCuIndex;
    EB_U32                     depthOneCandidateCuIndex = leafIndex;
    EB_U32                     depthTwoCandidateCuIndex = leafIndex;
    EB_U64                     depthNRate = 0;
    EB_U64                     depthNPlusOneRate = 0;
    EB_U64                     depthNCost = 0;
    EB_U64                     depthNPlusOneCost = 0;
    EB_U32                     cuOriginX;
    EB_U32                     cuOriginY;

    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
    EncodeContext_t      *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EB_U32                tbMaxDepth = sequenceControlSetPtr->maxLcuDepth;
    EB_BOOL               lastDepthFlag = tbPtr->codedLeafArrayPtr[leafIndex]->splitFlag == EB_FALSE ? EB_TRUE : EB_FALSE;

    const CodedUnitStats_t    *curCuStatsPtr;
    const CodedUnitStats_t    *depthTwoCuStatsPtr;
    const CodedUnitStats_t    *depthOneCuStatsPtr;
    const CodedUnitStats_t    *depthZeroCuStatsPtr;

    lastCuIndex = leafIndex;
    curCuStatsPtr = GetCodedUnitStats(leafIndex);
    cuOriginX = tbOriginX + curCuStatsPtr->originX;
    cuOriginY = tbOriginY + curCuStatsPtr->originY;
    EB_U8 interDepthW12 = 0;
    EB_U8 interDepthW01 = 0;

    if (lastDepthFlag) {
        tbPtr->codedLeafArrayPtr[leafIndex]->splitFlag = EB_FALSE;


        if (curCuStatsPtr->depth == 1) {
            contextPtr->groupOf16x16BlocksCount++;
        }
        else if (curCuStatsPtr->depth == 2) {
            contextPtr->groupOf8x8BlocksCount++;
        }
    }

    /*** Stage 0: Inter depth decision: depth 2 vs depth 3 ***/

    // Walks to the last coded 8x8 block for merging
    if ((GROUP_OF_4_8x8_BLOCKS(cuOriginX, cuOriginY))) {

        depthTwoCandidateCuIndex = leafIndex - DEPTH_THREE_STEP - DEPTH_THREE_STEP - DEPTH_THREE_STEP - 1;

        contextPtr->groupOf8x8BlocksCount++;

        // From the last coded cu index, get the indices of the left, top, and top left cus
        leftCuIndex = leafIndex - DEPTH_THREE_STEP;
        topCuIndex = leftCuIndex - DEPTH_THREE_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_THREE_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depthTwoCandidateCuIndex = topLeftCuIndex - 1;

        // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
        //   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

        // Compute depth N cost
		SplitFlagRate(
			contextPtr,
            tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex],
            0,
            &depthNRate,
            fullLambda,
            mdRateEstimationPtr,
			tbMaxDepth);
		if (contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].testedCuFlag == EB_FALSE)
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = MAX_CU_COST;

		depthNCost = contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost + depthNRate;
		// Compute depth N+1 cost
		SplitFlagRate(
			contextPtr,
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex],
			1,
			&depthNPlusOneRate,
			fullLambda,
			mdRateEstimationPtr,
			tbMaxDepth);
		depthNPlusOneCost =
			contextPtr->mdLocalCuUnit[leafIndex].cost +
			contextPtr->mdLocalCuUnit[leftCuIndex].cost +
			contextPtr->mdLocalCuUnit[topCuIndex].cost +
			contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
			depthNPlusOneRate;
		// Inter depth comparison: depth 2 vs depth 3
		if (depthNCost <= depthNPlusOneCost){

			// If the cost is low enough to warrant not spliting further:
			// 1. set the split flag of the candidate pu for merging to false
			// 2. update the last pu index
			tbPtr->codedLeafArrayPtr[depthTwoCandidateCuIndex]->splitFlag = EB_FALSE;
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = depthNCost;
			lastCuIndex = depthTwoCandidateCuIndex;
		}
		else {
			// If the cost is not low enough:
			// update the cost of the candidate pu for merging
			// this update is required for the next inter depth decision
			contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost = depthNPlusOneCost;
		}


    }

    // Stage 1: Inter depth decision: depth 1 vs depth 2 

    // Walks to the last coded 16x16 block for merging
    depthTwoCuStatsPtr = GetCodedUnitStats(depthTwoCandidateCuIndex);
    cuOriginX = tbOriginX + depthTwoCuStatsPtr->originX;
    cuOriginY = tbOriginY + depthTwoCuStatsPtr->originY;
    if (GROUP_OF_4_16x16_BLOCKS(cuOriginX, cuOriginY) &&
        (contextPtr->groupOf8x8BlocksCount == 4)){

        depthOneCandidateCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP - DEPTH_TWO_STEP - DEPTH_TWO_STEP - 1;

        contextPtr->groupOf8x8BlocksCount = 0;
        contextPtr->groupOf16x16BlocksCount++;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP;
        topCuIndex = leftCuIndex - DEPTH_TWO_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_TWO_STEP;

        // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
        //   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

        // From the top left index, get the index of the candidate pu for merging
        depthOneCandidateCuIndex = topLeftCuIndex - 1;

        depthOneCuStatsPtr = GetCodedUnitStats(depthOneCandidateCuIndex);
        if (depthOneCuStatsPtr->depth == 1) {

            // Compute depth N cost
			SplitFlagRate(
				contextPtr,
                tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex],
                0,
                &depthNRate,
                fullLambda,
                mdRateEstimationPtr,
				tbMaxDepth);
			if (contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].testedCuFlag == EB_FALSE)
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = MAX_CU_COST;
			depthNCost = contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost + depthNRate;

			// Compute depth N+1 cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex],
				1,
				&depthNPlusOneRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			depthNPlusOneCost =
				contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost +
				contextPtr->mdLocalCuUnit[leftCuIndex].cost +
				contextPtr->mdLocalCuUnit[topCuIndex].cost +
				contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
				depthNPlusOneRate;
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[depthTwoCandidateCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[leftCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[topCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);
			CHECK_REPORT_ERROR(
				(contextPtr->mdLocalCuUnit[topLeftCuIndex].cost != MAX_CU_COST),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_FL_ERROR4);

			if (depthNPlusOneCost < MAX_CU_COST)
				depthNPlusOneCost = depthNPlusOneCost + ((EB_S64)depthNPlusOneCost*interDepthW12) / 100;

			// Inter depth comparison: depth 1 vs depth 2
			if (depthNCost <= depthNPlusOneCost) {
				// If the cost is low enough to warrant not spliting further:
				// 1. set the split flag of the candidate pu for merging to false
				// 2. update the last pu index
				tbPtr->codedLeafArrayPtr[depthOneCandidateCuIndex]->splitFlag = EB_FALSE;
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = depthNCost;
				lastCuIndex = depthOneCandidateCuIndex;
			}
			else {
				// If the cost is not low enough:
				// update the cost of the candidate pu for merging
				// this update is required for the next inter depth decision
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost = depthNPlusOneCost;
			}


        }
    }

    // Stage 2: Inter depth decision: depth 0 vs depth 1 

    // Walks to the last coded 32x32 block for merging
    // Stage 2 isn't performed in I slices since the abcense of 64x64 candidates
    depthOneCuStatsPtr = GetCodedUnitStats(depthOneCandidateCuIndex);
    cuOriginX = tbOriginX + depthTwoCuStatsPtr->originX;
    cuOriginY = tbOriginY + depthTwoCuStatsPtr->originY;
    if ((pictureControlSetPtr->sliceType == EB_P_PICTURE || pictureControlSetPtr->sliceType == EB_B_PICTURE)
        && GROUP_OF_4_32x32_BLOCKS(cuOriginX, cuOriginY) &&
        (contextPtr->groupOf16x16BlocksCount == 4)) {

        depthZeroCandidateCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP - DEPTH_ONE_STEP - DEPTH_ONE_STEP - 1;

        contextPtr->groupOf16x16BlocksCount = 0;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP;
        topCuIndex = leftCuIndex - DEPTH_ONE_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_ONE_STEP;

        // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
        //   This needs to be done in the case that the N block was initially not calculated.

		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborMode;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].leftNeighborDepth;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].topNeighborMode = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborMode;
		contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[topLeftCuIndex].topNeighborDepth;

        // From the top left index, get the index of the candidate pu for merging
        depthZeroCandidateCuIndex = topLeftCuIndex - 1;

        depthZeroCuStatsPtr = GetCodedUnitStats(depthZeroCandidateCuIndex);
        if (depthZeroCuStatsPtr->depth == 0) {

            // Compute depth N cost
			SplitFlagRate(
				contextPtr,
                tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex],
                0,
                &depthNRate,
                fullLambda,
                mdRateEstimationPtr,
				tbMaxDepth);
			if (contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].testedCuFlag == EB_FALSE)
				contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].cost = MAX_CU_COST;
			depthNCost = contextPtr->mdLocalCuUnit[depthZeroCandidateCuIndex].cost + depthNRate;
			// Compute depth N+1 cost
			SplitFlagRate(
				contextPtr,
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex],
				1,
				&depthNPlusOneRate,
				fullLambda,
				mdRateEstimationPtr,
				tbMaxDepth);
			depthNPlusOneCost =
				contextPtr->mdLocalCuUnit[depthOneCandidateCuIndex].cost +
				contextPtr->mdLocalCuUnit[leftCuIndex].cost +
				contextPtr->mdLocalCuUnit[topCuIndex].cost +
				contextPtr->mdLocalCuUnit[topLeftCuIndex].cost +
				depthNPlusOneRate;
			if (depthNPlusOneCost < MAX_CU_COST)
				depthNPlusOneCost = depthNPlusOneCost + ((EB_S64)depthNPlusOneCost*interDepthW01) / 100;

			// Inter depth comparison: depth 0 vs depth 1
			if (depthNCost <= depthNPlusOneCost){

				// If the cost is low enough to warrant not spliting further:
				// 1. set the split flag of the candidate pu for merging to false
				// 2. update the last pu index
				tbPtr->codedLeafArrayPtr[depthZeroCandidateCuIndex]->splitFlag = EB_FALSE;
				lastCuIndex = depthZeroCandidateCuIndex;
			}

        }
    }

    return lastCuIndex;
}

