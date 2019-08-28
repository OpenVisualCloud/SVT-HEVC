/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <stdio.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbTransformUnit.h"
#include "EbRateDistortionCost.h"
#include "EbFullLoop.h"
#include "EbPictureOperators.h"

#include "EbModeDecisionProcess.h"
#include "EbEncDecProcess.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"
#include "EbComputeSAD.h"
#include "EbTransforms.h"

#include "EbInterPrediction.h"
#include "emmintrin.h"

#define TH_NFL_BIAS             7

EB_U8  MDSCAN_TO_QUADTREE_ID[] =
{
	1,
	1,
	1,
	1,
	2,
	3,
	4,
	2,
	1,
	2,
	3,
	4,
	3,
	1,
	2,
	3,
	4,
	4,
	1,
	2,
	3,
	4,
	2,
	1,
	1,
	2,
	3,
	4,
	2,
	1,
	2,
	3,
	4,
	3,
	1,
	2,
	3,
	4,
	4,
	1,
	2,
	3,
	4,
	3,
	1,
	1,
	2,
	3,
	4,
	2,
	1,
	2,
	3,
	4,
	3,
	1,
	2,
	3,
	4,
	4,
	1,
	2,
	3,
	4,
	4,
	1,
	1,
	2,
	3,
	4,
	2,
	1,
	2,
	3,
	4,
	3,
	1,
	2,
	3,
	4,
	4,
	1,
	2,
	3,
	4,
};

EB_U8 CHILD_MDSCAN_TO_PARENT_MDSCAN[] =
{
	0,
	0,
	1,
	2,
	2,
	2,
	2,
	1,
	7,
	7,
	7,
	7,
	1,
	12,
	12,
	12,
	12,
	1,
	17,
	17,
	17,
	17,
	0,
	22,
	23,
	23,
	23,
	23,
	22,
	28,
	28,
	28,
	28,
	22,
	33,
	33,
	33,
	33,
	22,
	38,
	38,
	38,
	38,
	0,
	43,
	44,
	44,
	44,
	44,
	43,
	49,
	49,
	49,
	49,
	43,
	54,
	54,
	54,
	54,
	43,
	59,
	59,
	59,
	59,
	0,
	64,
	65,
	65,
	65,
	65,
	64,
	70,
	70,
	70,
	70,
	64,
	75,
	75,
	75,
	75,
	64,
	80,
	80,
	80,
	80,
};


static const EB_U8 CuOffset[4] = { 85, 21, 5, 1 };



const EB_FULL_COST_FUNC   fullCostFuncTable[3][3] =
{
	/*      */{ NULL, NULL, NULL },
	/*INTER */{ InterFullCost, InterFullCost, NULL },
	/*INTRA */{ IntraFullCostPslice, IntraFullCostPslice, IntraFullCostIslice },
};

const EB_PREDICTION_FUNC  PredictionFunTableOl[2][3] = {

    { NULL, Inter2Nx2NPuPredictionInterpolationFree,    IntraPredictionOl },  //  Interpolation-free path  
    { NULL, Inter2Nx2NPuPredictionHevc,                 IntraPredictionOl }   //  HEVC Interpolation path
};

const EB_PREDICTION_FUNC  PredictionFunTableCl[2][3] = {
    { NULL, Inter2Nx2NPuPredictionInterpolationFree ,   IntraPredictionCl }, //  Interpolation-free path  
    { NULL, Inter2Nx2NPuPredictionHevc,                 IntraPredictionCl }  //  HEVC Interpolation path
};

const EB_FAST_COST_FUNC   ProductFastCostFuncOptTable[3][3] =
{
	/*      */{ NULL, NULL, NULL },
	/*INTER */{ InterFastCostBsliceOpt, InterFastCostPsliceOpt, NULL },
	/*INTRA */{ Intra2Nx2NFastCostPsliceOpt, Intra2Nx2NFastCostPsliceOpt, Intra2Nx2NFastCostIsliceOpt },
};
const EB_FULL_LUMA_COST_FUNC   ProductFullLumaCostFuncTable[3][3] =
{
	/*      */{ NULL, NULL, NULL },
	/*INTER */{ InterFullLumaCost, InterFullLumaCost, NULL },
	/*INTRA */{ IntraFullLumaCostPslice, IntraFullLumaCostPslice, IntraFullLumaCostIslice },
};

const EB_INTRA_4x4_FAST_LUMA_COST_FUNC Intra4x4FastCostFuncTable[3] =
{
    Intra4x4FastCostPslice,
    Intra4x4FastCostPslice,
    Intra4x4FastCostIslice
};

const EB_INTRA_4x4_FULL_LUMA_COST_FUNC Intra4x4FullCostFuncTable[3] =
{
    Intra4x4FullCostPslice,
    Intra4x4FullCostPslice,
    Intra4x4FullCostIslice
};

const EB_INTRA_NxN_FAST_COST_FUNC IntraNxNFastCostFuncTable[3] =
{
	IntraNxNFastCostPslice,
	IntraNxNFastCostPslice,
	IntraNxNFastCostIslice
};
const EB_INTRA_NxN_FULL_COST_FUNC IntraNxNFullCostFuncTable[3] =
{
	IntraNxNFullCostPslice,
	IntraNxNFullCostPslice,
	IntraNxNFullCostIslice
};

extern void GenerateIntraLumaReferenceSamplesMd(
	ModeDecisionContext_t      *contextPtr,
	EbPictureBufferDesc_t      *inputPicturePtr) {


    EB_BOOL pictureLeftBoundary = (contextPtr->lcuPtr->lcuEdgeInfoPtr->tileLeftEdgeFlag == EB_TRUE && ((contextPtr->cuOriginX & (contextPtr->lcuPtr->size - 1)) == 0)) ? EB_TRUE : EB_FALSE;
    EB_BOOL pictureTopBoundary = (contextPtr->lcuPtr->lcuEdgeInfoPtr->tileTopEdgeFlag == EB_TRUE && ((contextPtr->cuOriginY & (contextPtr->lcuPtr->size - 1)) == 0)) ? EB_TRUE : EB_FALSE;
    EB_BOOL pictureRightBoundary = (contextPtr->lcuPtr->lcuEdgeInfoPtr->tileRightEdgeFlag == EB_TRUE && (((contextPtr->cuOriginX + contextPtr->cuStats->size) & (contextPtr->lcuPtr->size - 1)) == 0)) ? EB_TRUE : EB_FALSE;

	if (contextPtr->intraMdOpenLoopFlag == EB_FALSE) {

        GenerateLumaIntraReferenceSamplesEncodePass(
            EB_FALSE,
            EB_TRUE,
            contextPtr->cuOriginX,
            contextPtr->cuOriginY,
            contextPtr->cuStats->size,
            MAX_LCU_SIZE,
            contextPtr->cuStats->depth,
            contextPtr->modeTypeNeighborArray,
            contextPtr->lumaReconNeighborArray,
            (NeighborArrayUnit_t *) EB_NULL,
            (NeighborArrayUnit_t *) EB_NULL,
            contextPtr->intraRefPtr,
            pictureLeftBoundary,
            pictureTopBoundary,
            pictureRightBoundary);

		contextPtr->lumaIntraRefSamplesGenDone = EB_TRUE;

	}
	else{
		//TODO: Change this function to take care of neighbour validity.
		//      then create a generic function that takes recon or input frame as generic input for both open and close loop case.
		UpdateNeighborSamplesArrayOL(
			contextPtr->intraRefPtr,
			inputPicturePtr,
			inputPicturePtr->strideY,
			contextPtr->cuOriginX,
			contextPtr->cuOriginY,
			contextPtr->cuStats->size,
			contextPtr->lcuPtr);

		contextPtr->lumaIntraRefSamplesGenDone = EB_TRUE;

	}
}

void ApplyMvOverBoundariesBias(
	SequenceControlSet_t                *sequenceControlSetPtr,
	ModeDecisionContext_t               *contextPtr,
	EB_U32                                cuSize,
	ModeDecisionCandidateBuffer_t       *candidateBuffer
	) {

	if (candidateBuffer->candidatePtr->type == INTER_MODE) {

		if (candidateBuffer->candidatePtr->predictionDirection[0] == UNI_PRED_LIST_0) {
			if (
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L0 >> 2) + contextPtr->cuOriginX + cuSize)) > sequenceControlSetPtr->lumaWidth) ||
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L0 >> 2) + contextPtr->cuOriginY + cuSize)) > sequenceControlSetPtr->lumaHeight) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L0 >> 2) + contextPtr->cuOriginX) < 0) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L0 >> 2) + contextPtr->cuOriginY) < 0)

				){
				*candidateBuffer->fastCostPtr = 0xFFFFFFFFFFFFFF;


			}
		}
		else if (candidateBuffer->candidatePtr->predictionDirection[0] == UNI_PRED_LIST_1) {
			if (
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L1 >> 2) + contextPtr->cuOriginX + cuSize)) > sequenceControlSetPtr->lumaWidth) ||
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L1 >> 2) + contextPtr->cuOriginY + cuSize)) > sequenceControlSetPtr->lumaHeight) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L1 >> 2) + contextPtr->cuOriginX) < 0) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L1 >> 2) + contextPtr->cuOriginY) < 0)
				){
				//*candidateBuffer->fastCostPtr += (*candidateBuffer->fastCostPtr * 100) / 100;
				*candidateBuffer->fastCostPtr = 0xFFFFFFFFFFFFFF;

			}
		}
		else {
			if (
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L0 >> 2) + contextPtr->cuOriginX + cuSize)) > sequenceControlSetPtr->lumaWidth) ||
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L0 >> 2) + contextPtr->cuOriginY + cuSize)) > sequenceControlSetPtr->lumaHeight) ||
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L1 >> 2) + contextPtr->cuOriginX + cuSize)) > sequenceControlSetPtr->lumaWidth) ||
				((unsigned)ABS((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L1 >> 2) + contextPtr->cuOriginY + cuSize)) > sequenceControlSetPtr->lumaHeight) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L0 >> 2) + contextPtr->cuOriginX) < 0) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L0 >> 2) + contextPtr->cuOriginY) < 0) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_x_L1 >> 2) + contextPtr->cuOriginX) < 0) ||
				((signed int)((signed int)(candidateBuffer->candidatePtr->motionVector_y_L1 >> 2) + contextPtr->cuOriginY) < 0)
				){
				*candidateBuffer->fastCostPtr = 0xFFFFFFFFFFFFFF;
			}
		}
	}
};

/***************************************************
* Update Recon Samples Neighbor Arrays
***************************************************/
void ModeDecisionUpdateNeighborArrays(
	NeighborArrayUnit_t     *leafDepthNeighborArray,
	NeighborArrayUnit_t     *modeTypeNeighborArray,
	NeighborArrayUnit_t     *intraLumaModeNeighborArray,
	NeighborArrayUnit_t     *mvNeighborArray,
	NeighborArrayUnit_t     *skipNeighborArray,
	NeighborArrayUnit_t     *lumaReconSampleNeighborArray,
	NeighborArrayUnit_t     *cbReconSampleNeighborArray,
	NeighborArrayUnit_t     *crReconSampleNeighborArray,
	EbPictureBufferDesc_t   *reconBuffer,
	EB_U32                   lcuSize,
	EB_BOOL                  intraMdOpenLoop,
    EB_BOOL                  intra4x4Selected,
	EB_U8                   *depth,
	EB_U8                   *modeType,
	EB_U8                   *lumaMode,
	MvUnit_t                *mvUnit,
	EB_U8                   *skipFlag,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size,
	EB_BOOL                  useIntraChromaflag)
{

	NeighborArrayUnitDepthSkipWrite(
		leafDepthNeighborArray,
		(EB_U8*)depth,
		originX,
		originY,
		size);

    if (intra4x4Selected == EB_FALSE){
        NeighborArrayUnitModeTypeWrite(
            modeTypeNeighborArray,
            (EB_U8*)modeType,
            originX,
            originY,
            size);

        NeighborArrayUnitIntraWrite(
            intraLumaModeNeighborArray,
            (EB_U8*)lumaMode,
            originX,
            originY,
            size);
    }

	// *Note - this has to be changed for non-square PU support -- JMJ
	NeighborArrayUnitMvWrite(
		mvNeighborArray,
		(EB_U8*)mvUnit,
		originX,
		originY,
		size);

	NeighborArrayUnitDepthSkipWrite(
		skipNeighborArray,
		(EB_U8*)skipFlag,
		originX,
		originY,
		size);

    if (intraMdOpenLoop == EB_FALSE && intra4x4Selected == EB_FALSE) {
        // Recon Samples - Luma
        NeighborArrayUnitSampleWrite(
            lumaReconSampleNeighborArray,
            reconBuffer->bufferY,
            reconBuffer->strideY,
            originX & (lcuSize - 1),
            originY & (lcuSize - 1),
            originX,
            originY,
            size,//CHKN width,
            size,//CHKN height,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
    }

    if (intraMdOpenLoop == EB_FALSE && useIntraChromaflag) {
        // Recon Samples - Cb
        NeighborArrayUnitSampleWrite(
            cbReconSampleNeighborArray,
            reconBuffer->bufferCb,
            reconBuffer->strideCb,
            (originX & (lcuSize - 1)) >> 1,
            (originY & (lcuSize - 1)) >> 1,
            originX >> 1,
            originY >> 1,
            size >> 1,
            size >> 1,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        // Recon Samples - Cr
        NeighborArrayUnitSampleWrite(
            crReconSampleNeighborArray,
            reconBuffer->bufferCr,
            reconBuffer->strideCr,
            (originX & (lcuSize - 1)) >> 1,
            (originY & (lcuSize - 1)) >> 1,
            originX >> 1,
            originY >> 1,
            size >> 1,
            size >> 1,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
    }


	return;
}

void MvMergePassUpdateNeighborArrays(
    PictureControlSet_t     *pictureControlSetPtr,
	NeighborArrayUnit_t     *leafDepthNeighborArray,
	NeighborArrayUnit_t     *modeTypeNeighborArray,
	NeighborArrayUnit_t     *intraLumaModeNeighborArray,
	NeighborArrayUnit_t     *mvNeighborArray,
	NeighborArrayUnit_t     *skipNeighborArray,
	NeighborArrayUnit_t     *lumaReconSampleNeighborArray,
	NeighborArrayUnit_t     *cbReconSampleNeighborArray,
	NeighborArrayUnit_t     *crReconSampleNeighborArray,
	EbPictureBufferDesc_t   *reconBuffer,
	EB_U32                   lcuSize,
	EB_BOOL                  intraMdOpenLoop,
	EB_U8                   *depth,
	EB_U8                   *modeType,
	EB_U8                   *lumaMode,
	MvUnit_t                *mvUnit,
	EB_U8                   *skipFlag,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size,
    EB_U16                   tileIdx,
	EB_BOOL                 useIntraChromaflag)
{
    

	NeighborArrayUnitDepthSkipWrite(
		leafDepthNeighborArray,
		(EB_U8*)depth,
		originX,
		originY,
		size);

    NeighborArrayUnitModeTypeWrite(
        modeTypeNeighborArray,
        (EB_U8*)modeType,
        originX,
        originY,
        size);

    NeighborArrayUnitIntraWrite(
        intraLumaModeNeighborArray,
        (EB_U8*)lumaMode,
        originX,
        originY,
        size); 

	// *Note - this has to be changed for non-square PU support -- JMJ
	NeighborArrayUnitMvWrite(
		mvNeighborArray,
		(EB_U8*)mvUnit,
		originX,
		originY,
		size);

	NeighborArrayUnitDepthSkipWrite(
		skipNeighborArray,
		(EB_U8*)skipFlag,
		originX,
		originY,
		size);

    if (intraMdOpenLoop == EB_FALSE) {
		// Recon Samples - Luma
		NeighborArrayUnitSampleWrite(
			lumaReconSampleNeighborArray,
			reconBuffer->bufferY,
			reconBuffer->strideY,
			originX & (lcuSize - 1),
			originY & (lcuSize - 1),
			originX,
			originY,
			size,//CHKN width,
			size,//CHKN height,
			NEIGHBOR_ARRAY_UNIT_FULL_MASK);

		if (useIntraChromaflag){
			// Recon Samples - Cb
			NeighborArrayUnitSampleWrite(
				cbReconSampleNeighborArray,
				reconBuffer->bufferCb,
				reconBuffer->strideCb,
				(originX & (lcuSize - 1)) >> 1,
				(originY & (lcuSize - 1)) >> 1,
				originX >> 1,
				originY >> 1,
				size >> 1,
				size >> 1,
				NEIGHBOR_ARRAY_UNIT_FULL_MASK);

			// Recon Samples - Cr
			NeighborArrayUnitSampleWrite(
				crReconSampleNeighborArray,
				reconBuffer->bufferCr,
				reconBuffer->strideCr,
				(originX & (lcuSize - 1)) >> 1,
				(originY & (lcuSize - 1)) >> 1,
				originX >> 1,
				originY >> 1,
				size >> 1,
				size >> 1,
				NEIGHBOR_ARRAY_UNIT_FULL_MASK);
		}
	}



    EB_U8 depthIndex;
    for (depthIndex = PILLAR_NEIGHBOR_ARRAY_INDEX; depthIndex <= REFINEMENT_NEIGHBOR_ARRAY_INDEX; depthIndex++) {

        NeighborArrayUnitDepthSkipWrite(
            pictureControlSetPtr->mdLeafDepthNeighborArray[depthIndex][tileIdx],
            (EB_U8*)depth,
            originX,
            originY,
            size);

        NeighborArrayUnitModeTypeWrite(
            pictureControlSetPtr->mdModeTypeNeighborArray[depthIndex][tileIdx],
            (EB_U8*)modeType,
            originX,
            originY,
            size);

        NeighborArrayUnitIntraWrite(
            pictureControlSetPtr->mdIntraLumaModeNeighborArray[depthIndex][tileIdx],
            (EB_U8*)lumaMode,
            originX,
            originY,
            size);

        // *Note - this has to be changed for non-square PU support -- JMJ
        NeighborArrayUnitMvWrite(
            pictureControlSetPtr->mdMvNeighborArray[depthIndex][tileIdx],
            (EB_U8*)mvUnit,
            originX,
            originY,
            size);

        NeighborArrayUnitDepthSkipWrite(
             pictureControlSetPtr->mdSkipFlagNeighborArray[depthIndex][tileIdx],
            (EB_U8*)skipFlag,
            originX,
            originY,
            size);

        if (intraMdOpenLoop == EB_FALSE) {
            // Recon Samples - Luma
            NeighborArrayUnitSampleWrite(
                pictureControlSetPtr->mdLumaReconNeighborArray[depthIndex][tileIdx],
                reconBuffer->bufferY,
                reconBuffer->strideY,
                originX & (lcuSize - 1),
                originY & (lcuSize - 1),
                originX,
                originY,
                size,//CHKN width,
                size,//CHKN height,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            if (useIntraChromaflag){
                // Recon Samples - Cb
                NeighborArrayUnitSampleWrite(
                    pictureControlSetPtr->mdCbReconNeighborArray[depthIndex][tileIdx],
                    reconBuffer->bufferCb,
                    reconBuffer->strideCb,
                    (originX & (lcuSize - 1)) >> 1,
                    (originY & (lcuSize - 1)) >> 1,
                    originX >> 1,
                    originY >> 1,
                    size >> 1,
                    size >> 1,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                // Recon Samples - Cr
                NeighborArrayUnitSampleWrite(
                    pictureControlSetPtr->mdCrReconNeighborArray[depthIndex][tileIdx],
                    reconBuffer->bufferCr,
                    reconBuffer->strideCr,
                    (originX & (lcuSize - 1)) >> 1,
                    (originY & (lcuSize - 1)) >> 1,
                    originX >> 1,
                    originY >> 1,
                    size >> 1,
                    size >> 1,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);
            }
        }

    }

	return;
}



void Bdp16x16vs8x8RefinementUpdateNeighborArrays(
    PictureControlSet_t     *pictureControlSetPtr,
    NeighborArrayUnit_t     *leafDepthNeighborArray,
    NeighborArrayUnit_t     *modeTypeNeighborArray,
    NeighborArrayUnit_t     *intraLumaModeNeighborArray,
    NeighborArrayUnit_t     *mvNeighborArray,
    NeighborArrayUnit_t     *skipNeighborArray,
    NeighborArrayUnit_t     *lumaReconSampleNeighborArray,
    NeighborArrayUnit_t     *cbReconSampleNeighborArray,
    NeighborArrayUnit_t     *crReconSampleNeighborArray,
    EbPictureBufferDesc_t   *reconBuffer,
    EB_U32                   lcuSize,
    EB_BOOL                  intraMdOpenLoop,
    EB_BOOL                  intra4x4Selected,
    EB_U8                   *depth,
    EB_U8                   *modeType,
    EB_U8                   *lumaMode,
    MvUnit_t                *mvUnit,
    EB_U8                   *skipFlag,
    EB_U32                   originX,
    EB_U32                   originY,
    EB_U32                   size,
    EB_U16                   tileIdx,
    EB_BOOL                 useIntraChromaflag)
{

    NeighborArrayUnitDepthSkipWrite(
        leafDepthNeighborArray,
        (EB_U8*)depth,
        originX,
        originY,
        size);

    if (intra4x4Selected == EB_FALSE){
        NeighborArrayUnitModeTypeWrite(
            modeTypeNeighborArray,
            (EB_U8*)modeType,
            originX,
            originY,
            size);

        NeighborArrayUnitIntraWrite(
            intraLumaModeNeighborArray,
            (EB_U8*)lumaMode,
            originX,
            originY,
            size);
    }

    // *Note - this has to be changed for non-square PU support -- JMJ
    NeighborArrayUnitMvWrite(
        mvNeighborArray,
        (EB_U8*)mvUnit,
        originX,
        originY,
        size);

    NeighborArrayUnitDepthSkipWrite(
        skipNeighborArray,
        (EB_U8*)skipFlag,
        originX,
        originY,
        size);


    if (intraMdOpenLoop == EB_FALSE && intra4x4Selected == EB_FALSE){
        // Recon Samples - Luma
        NeighborArrayUnitSampleWrite(
            lumaReconSampleNeighborArray,
            reconBuffer->bufferY,
            reconBuffer->strideY,
            originX & (lcuSize - 1),
            originY & (lcuSize - 1),
            originX,
            originY,
            size,//CHKN width,
            size,//CHKN height,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (useIntraChromaflag){
            // Recon Samples - Cb
            NeighborArrayUnitSampleWrite(
                cbReconSampleNeighborArray,
                reconBuffer->bufferCb,
                reconBuffer->strideCb,
                (originX & (lcuSize - 1)) >> 1,
                (originY & (lcuSize - 1)) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            // Recon Samples - Cr
            NeighborArrayUnitSampleWrite(
                crReconSampleNeighborArray,
                reconBuffer->bufferCr,
                reconBuffer->strideCr,
                (originX & (lcuSize - 1)) >> 1,
                (originY & (lcuSize - 1)) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        }
    }

    // For P & B SLICES the Pillar Neighbor Arrays update happens @ the MvMerge Pass
    if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {

        NeighborArrayUnitDepthSkipWrite(
            pictureControlSetPtr->mdLeafDepthNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
            (EB_U8*)depth,
            originX,
            originY,
            size);

        if (intra4x4Selected == EB_FALSE){
            NeighborArrayUnitModeTypeWrite(
                pictureControlSetPtr->mdModeTypeNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
                (EB_U8*)modeType,
                originX,
                originY,
                size);

            NeighborArrayUnitIntraWrite(
                pictureControlSetPtr->mdIntraLumaModeNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
                (EB_U8*)lumaMode,
                originX,
                originY,
                size);
        }

        // *Note - this has to be changed for non-square PU support -- JMJ
        NeighborArrayUnitMvWrite(
            pictureControlSetPtr->mdMvNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
            (EB_U8*)mvUnit,
            originX,
            originY,
            size);

        NeighborArrayUnitDepthSkipWrite(
            pictureControlSetPtr->mdSkipFlagNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
            (EB_U8*)skipFlag,
            originX,
            originY,
            size);

        if (intraMdOpenLoop == EB_FALSE && intra4x4Selected == EB_FALSE){
            // Recon Samples - Luma
            NeighborArrayUnitSampleWrite(
                pictureControlSetPtr->mdLumaReconNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
                reconBuffer->bufferY,
                reconBuffer->strideY,
                originX & (lcuSize - 1),
                originY & (lcuSize - 1),
                originX,
                originY,
                size,//CHKN width,
                size,//CHKN height,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            if (useIntraChromaflag){
                // Recon Samples - Cb
                NeighborArrayUnitSampleWrite(
                    pictureControlSetPtr->mdCbReconNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
                    reconBuffer->bufferCb,
                    reconBuffer->strideCb,
                    (originX & (lcuSize - 1)) >> 1,
                    (originY & (lcuSize - 1)) >> 1,
                    originX >> 1,
                    originY >> 1,
                    size >> 1,
                    size >> 1,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                // Recon Samples - Cr
                NeighborArrayUnitSampleWrite(
                    pictureControlSetPtr->mdCrReconNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx],
                    reconBuffer->bufferCr,
                    reconBuffer->strideCr,
                    (originX & (lcuSize - 1)) >> 1,
                    (originY & (lcuSize - 1)) >> 1,
                    originX >> 1,
                    originY >> 1,
                    size >> 1,
                    size >> 1,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);
            }
        }
    }

    return;
}

void DeriveMpmModes(
    ModeDecisionContext_t	*contextPtr,
    CodingUnit_t			* const cuPtr) {
	EB_U32 leftNeighborMode = (&cuPtr->predictionUnitArray[0])->intraLumaLeftMode;
	EB_U32 topNeighborMode  = (&cuPtr->predictionUnitArray[0])->intraLumaTopMode;

	if (leftNeighborMode == topNeighborMode) {
		if (leftNeighborMode > 1) { // For angular modes
			contextPtr->mostProbableModeArray[0] = leftNeighborMode;
			contextPtr->mostProbableModeArray[1] = ((leftNeighborMode + 29) & 0x1F) + 2;
			contextPtr->mostProbableModeArray[2] = ((leftNeighborMode - 1) & 0x1F) + 2;
		}
		else { // Non Angular modes
			contextPtr->mostProbableModeArray[0] = EB_INTRA_PLANAR;
			contextPtr->mostProbableModeArray[1] = EB_INTRA_DC;
			contextPtr->mostProbableModeArray[2] = EB_INTRA_VERTICAL;
		}
	}
	else {
		contextPtr->mostProbableModeArray[0] = leftNeighborMode;
		contextPtr->mostProbableModeArray[1] = topNeighborMode;

		if (leftNeighborMode && topNeighborMode) {
			contextPtr->mostProbableModeArray[2] = EB_INTRA_PLANAR; // when both modes are non planar
		}
		else {
			contextPtr->mostProbableModeArray[2] = (leftNeighborMode + topNeighborMode) < 2 ? EB_INTRA_VERTICAL : EB_INTRA_DC;
		}
	}
}
//*************************//
// SetNfl
// Based on the MDStage and the encodeMode
// the NFL candidates numbers are set
//*************************//
void SetNfl(
	LargestCodingUnit_t     *lcuPtr,
	ModeDecisionContext_t	*contextPtr,
	PictureControlSet_t		*pictureControlSetPtr,
	EB_MD_STAGE				 mdStage)
{
	switch (mdStage) {
	case MDC_STAGE:
        // NFL Level MD         Settings
        // 0                    4
        // 1                    3 if 32x32, 2 otherwise
        // 2                    2
        // 3                    2 if Detectors, 1 otherwise
        // 4                    2 if 64x64 or 32x32 or 16x16, 1 otherwise
        // 5                    2 if 64x64 or 332x32, 1 otherwise
        // 6                    1        
        if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuPtr->index] == LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE) {
            contextPtr->fullReconSearchCount = 1;
        }
        else {
            if (contextPtr->nflLevelMd == 0) {
                contextPtr->fullReconSearchCount = 4;
            }
            else if (contextPtr->nflLevelMd == 1) {
                contextPtr->fullReconSearchCount = (contextPtr->cuSize == 32) ? 3 : 2;
            }
            else if (contextPtr->nflLevelMd == 2) {
                contextPtr->fullReconSearchCount = 2;
            }
            else if (contextPtr->nflLevelMd == 3) {

                if ((pictureControlSetPtr->ParentPcsPtr->lcuHomogeneousAreaArray[lcuPtr->index]  &&
                    pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 75  &&
                    pictureControlSetPtr->ParentPcsPtr->isLcuHomogeneousOverTime[lcuPtr->index]) ||
                    (pictureControlSetPtr->ParentPcsPtr->lowMotionContentFlag                    &&
                        pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArray[lcuPtr->index] == EB_FALSE)) {

                    contextPtr->fullReconSearchCount = 2;
                }
                else {
                    contextPtr->fullReconSearchCount = 1;
                }

            }
            else if (contextPtr->nflLevelMd == 4) {
                contextPtr->fullReconSearchCount = (contextPtr->cuSize >= 16) ? 2 : 1;
            }
            else if (contextPtr->nflLevelMd == 5) {
                contextPtr->fullReconSearchCount = (contextPtr->cuSize >= 32) ? 2 : 1;
            }
            else {
                contextPtr->fullReconSearchCount = 1;
            }
        }
        break;

	case BDP_PILLAR_STAGE:
    case BDP_16X16_8X8_REF_STAGE:      
        // NFL Level Pillar/8x8 Refinement         Settings
        // 0                                       4
        // 1                                       4 if depthRefinment, 3 if 32x32, 2 otherwise
        // 2                                       3 
        // 3                                       3 if depthRefinment or 32x32, 2 otherwise
        // 4                                       3 if 32x32, 2 otherwise
        // 5                                       2    
        // 6                                       2 if Detectors, 1 otherwise
        // 7                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
        // 8                                       2 if 64x64 or 332x32, 1 otherwise
        // 9                                       1      
        if (contextPtr->nflLevelPillar8x8ref == 0) {
            contextPtr->fullReconSearchCount = 4;
        }
        else if (contextPtr->nflLevelPillar8x8ref == 1) {
            if (contextPtr->depthRefinment > 0) {
                contextPtr->fullReconSearchCount = 4;
            }
            else if (contextPtr->cuSize == 32) {
                contextPtr->fullReconSearchCount = 3;
            }
            else {
                contextPtr->fullReconSearchCount = 2;
            }
        }
        else if (contextPtr->nflLevelPillar8x8ref == 2) {
            contextPtr->fullReconSearchCount = 3;
        }
        else if (contextPtr->nflLevelPillar8x8ref == 3) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize == 32 || contextPtr->depthRefinment) ? 3 : 2;
        }
        else if (contextPtr->nflLevelPillar8x8ref == 4) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize == 32) ? 3 : 2;
        }
        else if (contextPtr->nflLevelPillar8x8ref == 5) {
            contextPtr->fullReconSearchCount = 2;
        }
        else if (contextPtr->nflLevelPillar8x8ref == 6) {
            if ((pictureControlSetPtr->ParentPcsPtr->lcuHomogeneousAreaArray[lcuPtr->index] &&
                pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 75 &&
                pictureControlSetPtr->ParentPcsPtr->isLcuHomogeneousOverTime[lcuPtr->index]) ||
                (pictureControlSetPtr->ParentPcsPtr->lowMotionContentFlag                    &&
                    pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArray[lcuPtr->index] == EB_FALSE)) {

                contextPtr->fullReconSearchCount = 2;
            }
            else {
                contextPtr->fullReconSearchCount = 1;
            }
        }
        else if (contextPtr->nflLevelPillar8x8ref == 7) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize >= 16) ? 2 : 1;
        }
        else if (contextPtr->nflLevelPillar8x8ref == 8) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize >= 32) ? 2 : 1;
        }
        else {
            contextPtr->fullReconSearchCount = 1;
        }

        break;

    case BDP_MVMERGE_STAGE:
	case BDP_64X64_32X32_REF_STAGE:

        // NFL Level MvMerge/64x64 Refinement      Settings
        // 0                                       4
        // 1                                       3 
        // 2                                       3 if depthRefinment or 32x32, 2 otherwise
        // 3                                       3 if 32x32, 2 otherwise
        // 4                                       2    
        // 5                                       2 if Detectors, 1 otherwise
        // 6                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
        // 7                                       2 if 64x64 or 332x32, 1 otherwise
        // 8                                       1      
        if (contextPtr->nflLevelMvMerge64x64ref == 0) {
            contextPtr->fullReconSearchCount = 4;
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 1) {
            contextPtr->fullReconSearchCount = 3;
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 2) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize == 32 || contextPtr->depthRefinment) ? 3 : 2;
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 3) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize == 32) ? 3 : 2;
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 4) {
            contextPtr->fullReconSearchCount = 2;
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 5) {
            if ((pictureControlSetPtr->ParentPcsPtr->lcuHomogeneousAreaArray[lcuPtr->index] &&
                pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 75 &&
                pictureControlSetPtr->ParentPcsPtr->isLcuHomogeneousOverTime[lcuPtr->index]) ||
                (pictureControlSetPtr->ParentPcsPtr->lowMotionContentFlag                    &&
                    pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArray[lcuPtr->index] == EB_FALSE)) {

                contextPtr->fullReconSearchCount = 2;
            }
            else {
                contextPtr->fullReconSearchCount = 1;
            }
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 6) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize >= 16) ? 2 : 1;
        }
        else if (contextPtr->nflLevelMvMerge64x64ref == 7) {
            contextPtr->fullReconSearchCount = (contextPtr->cuSize >= 32) ? 2 : 1;
        }
        else {
            contextPtr->fullReconSearchCount = 1;
        }

        break;
    
    default:
        break;
    }

}

//*************************//
// SetNmm
// Based on the MDStage and the encodeMode
// the NMM candidates numbers are set
//*************************//
void SetNmm(
	ModeDecisionContext_t	*contextPtr,
	EB_MD_STAGE				 mdStage)
{

    // NMM Level MD           Settings
    // 0                      5
    // 1                      3 if 32x32, 2 otherwise
    // 2                      2


    // NMM Level BDP          Settings
    // 0                      5
    // 1                      3 if 32x32 or depth refinment true, 2 otherwise
    // 2                      3 if 32x32, 2 otherwise
    // 3                      2


	switch (mdStage) {
	case MDC_STAGE:
        if (contextPtr->nmmLevelMd == 0) {
             contextPtr->mvMergeSkipModeCount = 5;
        }
        else if (contextPtr->nmmLevelMd == 1) {
            if (contextPtr->cuSize == 32)
                contextPtr->mvMergeSkipModeCount = 3;
            else
                contextPtr->mvMergeSkipModeCount = 2;
        }
        else {
            contextPtr->mvMergeSkipModeCount = 2;
        }

		break;

	case BDP_PILLAR_STAGE:
    case BDP_64X64_32X32_REF_STAGE:
    case BDP_16X16_8X8_REF_STAGE:
    case BDP_MVMERGE_STAGE:
        if (contextPtr->nmmLevelBdp == 0) {
            contextPtr->mvMergeSkipModeCount = 5;
        }
        else if (contextPtr->nmmLevelBdp == 1) {
            contextPtr->mvMergeSkipModeCount = 3;
        }
        else if (contextPtr->nmmLevelBdp == 2)  {
            if (contextPtr->depthRefinment > 0) {
                contextPtr->mvMergeSkipModeCount = 3;
            }
            else {
                if (contextPtr->cuSize == 32)
                    contextPtr->mvMergeSkipModeCount = 3;
                else
                    contextPtr->mvMergeSkipModeCount = 2;
            }
        }
        else if (contextPtr->nmmLevelBdp == 3) {
            if (contextPtr->cuSize == 32)
                contextPtr->mvMergeSkipModeCount = 3;
            else
                contextPtr->mvMergeSkipModeCount = 2;           
        }
        else {
            contextPtr->mvMergeSkipModeCount = 2;
        }

        break;

	default:
		break;
	}
}
 
void CheckHighCostPartition(
	SequenceControlSet_t       *sequenceControlSetPtr,
	ModeDecisionContext_t      *contextPtr,
	LargestCodingUnit_t        *lcuPtr,
	EB_U8                       leafIndex,
	EB_U8                      *parentLeafIndexPtr,       
    EB_BOOL                     enableExitPartitioning,
	EB_U8                      *exitPartitionPtr          
	)
{

	EB_U8  parentLeafIndex;

	if (sequenceControlSetPtr->lcuParamsArray[lcuPtr->index].isCompleteLcu && contextPtr->cuStats->depth>0)
	{

		const EB_U8 totChildren = MDSCAN_TO_QUADTREE_ID[leafIndex];
		EB_BOOL areGrandChildrenDone = EB_FALSE;
		if (contextPtr->cuPtr->splitFlag == EB_FALSE)
			areGrandChildrenDone = EB_TRUE;

		if (enableExitPartitioning)
            areGrandChildrenDone = (lcuPtr->pictureControlSetPtr->sliceType != EB_I_PICTURE) ? EB_TRUE : areGrandChildrenDone;

		if (totChildren<4 && areGrandChildrenDone)
		{

			parentLeafIndex = CHILD_MDSCAN_TO_PARENT_MDSCAN[leafIndex];
			if (contextPtr->mdLocalCuUnit[parentLeafIndex].testedCuFlag)

			{
				//get parent cost.                
				EB_U64 depthNRate = 0;
				SplitFlagRate(
					contextPtr,
					lcuPtr->codedLeafArrayPtr[parentLeafIndex],
					0,
					&depthNRate,
					contextPtr->fullLambda,
					contextPtr->mdRateEstimationPtr,
					sequenceControlSetPtr->maxLcuDepth);

				EB_U64 parentCost = contextPtr->mdLocalCuUnit[parentLeafIndex].cost + depthNRate;

				//get partition cost so far
				EB_U64  totalChildrenCost = 0;
				EB_U8   cuIt = leafIndex;
				EB_U8   it;
				for (it = 0; it < totChildren; it++)
				{
					totalChildrenCost += contextPtr->mdLocalCuUnit[cuIt].cost;
					cuIt = cuIt - CuOffset[contextPtr->cuStats->depth];
				}

				if (enableExitPartitioning)
					totalChildrenCost = lcuPtr->pictureControlSetPtr->temporalLayerIndex == 0 || contextPtr->edgeBlockNumFlag ? totalChildrenCost : (totalChildrenCost / totChildren) * 4;

				if (totalChildrenCost > parentCost)
				{
					*exitPartitionPtr = EB_TRUE;
					*parentLeafIndexPtr = parentLeafIndex;
				}

		}

		}
	}
}

EB_U32 BdpCalculateNextCuIndex(
	EB_U8 *               leafDataArray,
	EB_U32                cuIndex,
	EB_U32                leafCount,
	EB_U32                cuDepth
	)
{
	EB_U32 cuIdx;
	EB_U32 stepSplitFalse = 1;
	EB_U8 nextLeafIndex = cuDepth == 0 ? leafDataArray[leafCount - 1] + 1 : leafDataArray[cuIndex] + DepthOffset[cuDepth];

	for (cuIdx = cuIndex + 1; cuIdx < leafCount; ++cuIdx){
		// Assign CU data
		const EB_U8 leafIndex = leafDataArray[cuIdx];

		if (leafIndex < nextLeafIndex){
			stepSplitFalse++;
		}

		else{
			break;
		}

	}
	return stepSplitFalse;
}

// Calculate the next CU index
EB_U32 CalculateNextCuIndex(
	const EbMdcLeafData_t *const    leafDataArray,
	EB_U32                cuIndex,
	EB_U32                leafCount,
	EB_U32                cuDepth
	)
{
	EB_U32 cuIdx;
	EB_U32 stepSplitFalse = 1;
	EB_U8 nextLeafIndex = cuDepth == 0 ? leafDataArray[leafCount - 1].leafIndex + 1 : leafDataArray[cuIndex].leafIndex + DepthOffset[cuDepth];

	for (cuIdx = cuIndex + 1; cuIdx < leafCount; ++cuIdx){
		// Assign CU data
        const EB_U8 leafIndex = leafDataArray[cuIdx].leafIndex;
		//const EB_BOOL splitFlag = leafDataArray[cuIdx].splitFlag;
		//const CodedUnitStats_t *cuStatsPtr = GetCodedUnitStats(leafIndex);

		if (leafIndex < nextLeafIndex){
			stepSplitFalse++;
		}

		else{
			break;
		}

		
	}
	return stepSplitFalse;
}
void ConstructMdCuArray(
	ModeDecisionContext_t   *contextPtr,
	SequenceControlSet_t    *sequenceControlSetPtr,
	LargestCodingUnit_t		*lcuPtr,
	const MdcLcuData_t		* const mdcResultTbPtr) {
	EB_U32 cuIdx = 0;
	EB_U32 maxCuIndex = 0;
    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuPtr->index];
	for (cuIdx = 0; cuIdx < mdcResultTbPtr->leafCount; ++cuIdx) {
		maxCuIndex = MAX(maxCuIndex, mdcResultTbPtr->leafDataArray[cuIdx].leafIndex);
	}
    // CU Loop
	if (lcuParams->isCompleteLcu)

	{

        cuIdx = 0;
		do
		{
			CodingUnit_t * const cuPtr = lcuPtr->codedLeafArrayPtr[cuIdx];
			contextPtr->mdLocalCuUnit[cuIdx].testedCuFlag = EB_FALSE; 
			cuPtr->splitFlag = EB_TRUE;

			++cuIdx;
        } while (cuIdx < maxCuIndex);

	}
	else {
		// Initialize the cost & skipFlag for each CU belonging to this tree block for the future usage by inter-depth decision
		CodingUnit_t ** const codedLeafArrayPtr = lcuPtr->codedLeafArrayPtr;
        const unsigned codedLeafCount = CU_MAX_COUNT;
		cuIdx = 0;
		do
		{
			CodingUnit_t * const cuPtr = codedLeafArrayPtr[cuIdx];
			contextPtr->mdLocalCuUnit[cuIdx].testedCuFlag = EB_FALSE;
			cuPtr->splitFlag = EB_TRUE;

			++cuIdx;

		} while (cuIdx < codedLeafCount);
	}
}

void PerformInverseTransformRecon(
    ModeDecisionContext_t		   *contextPtr,
    ModeDecisionCandidateBuffer_t  *candidateBuffer,
    CodingUnit_t                   *cuPtr,
    const CodedUnitStats_t		   *cuStatsPtr) {

    const TransformUnitStats_t         *tuStatPtr;
    EB_U32                              tuSize;
    EB_U32                              tuOriginX;
    EB_U32                              tuOriginY;
    EB_U32                              tuOriginIndex;
    EB_U32                              tuTotalCount;

    EB_U32                              tuIndex;
    EB_U32								tuItr;
    TransformUnit_t                    *tuPtr;

    // Recon not needed if Open-Loop INTRA, or if spatialSseFullLoop ON (where T-1 performed @ Full-Loop)
    if (contextPtr->intraMdOpenLoopFlag == EB_FALSE && contextPtr->spatialSseFullLoop == EB_FALSE) {

        tuTotalCount = (cuStatsPtr->size == MAX_LCU_SIZE) ? 4 : 1;
        tuIndex = (cuStatsPtr->size == MAX_LCU_SIZE) ? 1 : 0;
        tuItr = 0;
        do {
            tuStatPtr = GetTransformUnitStats(tuIndex);
            tuPtr = &cuPtr->transformUnitArray[tuIndex];
            tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatPtr->offsetX);
            tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatPtr->offsetY);
            tuSize = cuStatsPtr->size >> tuStatPtr->depth;
            tuOriginIndex = tuOriginX + tuOriginY * 64;

            // Skip T-1 if 8x8 and INTRA4x4 is the winner as T-1 already performed @ INTRA4x4 search 
            if (!(cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == MIN_CU_SIZE && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4))
            {

                if (tuPtr->lumaCbf) {
                    //since we are missing PF-N2 version for 16x16 and 8x8 iT, do zero out.
                    if (tuSize < 32 && contextPtr->pfMdMode == PF_N2) {
                        PfZeroOutUselessQuadrants(
                            &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[tuOriginIndex]),
                            candidateBuffer->reconCoeffPtr->strideY,
                            (tuSize >> 1));
                    }

                    EstimateInvTransform(
                        &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[tuOriginIndex]),
                        candidateBuffer->reconCoeffPtr->strideY,
                        &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY)[tuOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
                        tuSize,
                        contextPtr->transformInnerArrayPtr,
                        BIT_INCREMENT_8BIT,
                        EB_FALSE,
                        tuSize < 32 ? PF_OFF : contextPtr->pfMdMode);

                    AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][tuSize >> 3](
                        &(candidateBuffer->predictionPtr->bufferY[tuOriginIndex]),
                        64,
                        &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY))[tuOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
                        &(candidateBuffer->reconPtr->bufferY[tuOriginIndex]),
                        candidateBuffer->reconPtr->strideY,
                        tuSize,
                        tuSize);

                }
                else {

                    PictureCopy8Bit(
                        candidateBuffer->predictionPtr,
                        tuOriginIndex,
                        0,//tuChromaOriginIndex,
                        candidateBuffer->reconPtr,
                        tuOriginIndex,
                        0,//tuChromaOriginIndex,
                        tuSize,
                        tuSize,
                        0,//chromaTuSize,
                        0,//chromaTuSize,
                        PICTURE_BUFFER_DESC_Y_FLAG);
                }
            }

            if (contextPtr->useChromaInformationInFullLoop) {

                EB_U32 chromaTuSize = tuSize >> 1;
                EB_U32 cbTuChromaOriginIndex = ((tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideCb) >> 1);
                EB_U32 crTuChromaOriginIndex = ((tuOriginX + tuOriginY * candidateBuffer->residualQuantCoeffPtr->strideCr) >> 1);


                // Skip T-1 if 8x8 and INTRA4x4 is the winner and INTRA4x4 Chroma performed as T-1 already performed @ INTRA4x4 search 
                //if (!(cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == MIN_CU_SIZE && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4 && contextPtr->use4x4ChromaInformationInFullLoop)) 
                {
                    if (tuPtr->cbCbf) {

                        EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;
                        EB_U32 chromatTuSize = (tuSize >> 1);
                        if (chromatTuSize == 4)
                            correctedPFMode = PF_OFF;
                        else if (chromatTuSize == 8 && contextPtr->pfMdMode == PF_N4)
                            correctedPFMode = PF_N2;

                        if (correctedPFMode) {
                            PfZeroOutUselessQuadrants(
                                &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[cbTuChromaOriginIndex]),
                                candidateBuffer->reconCoeffPtr->strideCb,
                                (chromatTuSize >> 1));
                        }

                        EstimateInvTransform(
                            &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[cbTuChromaOriginIndex]),
                            candidateBuffer->reconCoeffPtr->strideCb,
                            &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb)[cbTuChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                            chromaTuSize,
                            contextPtr->transformInnerArrayPtr,
                            BIT_INCREMENT_8BIT,
                            EB_FALSE,
                            EB_FALSE);

                        PictureAddition(
                            &(candidateBuffer->predictionPtr->bufferCb[cbTuChromaOriginIndex]),
                            candidateBuffer->predictionPtr->strideCb,
                            &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb))[cbTuChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                            &(candidateBuffer->reconPtr->bufferCb[cbTuChromaOriginIndex]),
                            candidateBuffer->reconPtr->strideCb,
                            chromaTuSize,
                            chromaTuSize);

                    }
                    else {

                        PictureCopy8Bit(
                            candidateBuffer->predictionPtr,
                            tuOriginIndex,
                            cbTuChromaOriginIndex,
                            candidateBuffer->reconPtr,
                            tuOriginIndex,
                            cbTuChromaOriginIndex,
                            tuSize,
                            tuSize,
                            chromaTuSize,
                            chromaTuSize,
                            PICTURE_BUFFER_DESC_Cb_FLAG);
                    }


                    if (tuPtr->crCbf) {

                        EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;
                        EB_U32 chromatTuSize = (tuSize >> 1);
                        if (chromatTuSize == 4)
                            correctedPFMode = PF_OFF;
                        else if (chromatTuSize == 8 && contextPtr->pfMdMode == PF_N4)
                            correctedPFMode = PF_N2;

                        if (correctedPFMode) {
                            PfZeroOutUselessQuadrants(
                                &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[crTuChromaOriginIndex]),
                                candidateBuffer->reconCoeffPtr->strideCr,
                                (chromatTuSize >> 1));
                        }

                        EstimateInvTransform(
                            &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[crTuChromaOriginIndex]),
                            candidateBuffer->reconCoeffPtr->strideCr,
                            &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr)[crTuChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                            chromaTuSize,
                            contextPtr->transformInnerArrayPtr,
                            BIT_INCREMENT_8BIT,
                            EB_FALSE,
                            EB_FALSE);

                        PictureAddition(
                            &(candidateBuffer->predictionPtr->bufferCr[crTuChromaOriginIndex]),
                            candidateBuffer->predictionPtr->strideCr,
                            &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr))[crTuChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                            &(candidateBuffer->reconPtr->bufferCr[crTuChromaOriginIndex]),
                            candidateBuffer->reconPtr->strideCr,
                            chromaTuSize,
                            chromaTuSize);

                    }
                    else {

                        PictureCopy8Bit(
                            candidateBuffer->predictionPtr,
                            tuOriginIndex,
                            crTuChromaOriginIndex,
                            candidateBuffer->reconPtr,
                            tuOriginIndex,
                            crTuChromaOriginIndex,
                            tuSize,
                            tuSize,
                            chromaTuSize,
                            chromaTuSize,
                            PICTURE_BUFFER_DESC_Cr_FLAG);
                    }
                }
            }




            ++tuItr;
            tuIndex = tuIndexList[tuStatPtr->depth][tuItr];



        } while (tuItr < tuTotalCount);
    }

}

/*******************************************
* Coding Loop - Fast Loop Initialization
*******************************************/
void ProductCodingLoopInitFastLoop(
	ModeDecisionContext_t      *contextPtr,
	NeighborArrayUnit_t   *intraLumaNeighborArray,
	NeighborArrayUnit_t   *skipFlagNeighborArray,
	NeighborArrayUnit_t   *modeTypeNeighborArray,
	NeighborArrayUnit_t   *leafDepthNeighborArray)
 {
	// Keep track of the LCU Ptr
	contextPtr->lumaIntraRefSamplesGenDone = EB_FALSE;
	contextPtr->chromaIntraRefSamplesGenDone = EB_FALSE;

	// Generate Split, Skip and intra mode contexts for the rate estimation
	CodingLoopContextGeneration(
		contextPtr,
		contextPtr->cuPtr,
		contextPtr->cuOriginX,
		contextPtr->cuOriginY,
        MAX_LCU_SIZE,
        intraLumaNeighborArray,
        skipFlagNeighborArray,
        modeTypeNeighborArray,
        leafDepthNeighborArray);

	// *Notes
	// -Instead of creating function pointers in a static array, put the func pointers in a queue
	// -This function should also do the SAD calc for each mode
	// -The best independent intra chroma mode should be determined here
	// -Modify PictureBufferDesc to be able to create Luma, Cb, and/or Cr only buffers via flags
	// -Have one PictureBufferDesc that points to the intra chroma buffer to be used.
	// -Properly signal the DM mode at this point

	// Initialize the candidate buffer costs
	{
       EB_U32 bufferDepthIndexStart = contextPtr->bufferDepthIndexStart[contextPtr->cuStats->depth];
	   EB_U32 bufferDepthIndexWidth = contextPtr->bufferDepthIndexWidth[contextPtr->cuStats->depth];
		EB_U32 index = 0;

		for (index = 0; index < bufferDepthIndexWidth; ++index){
			contextPtr->fastCostArray[bufferDepthIndexStart + index] = 0xFFFFFFFFFFFFFFFFull;
			contextPtr->fullCostArray[bufferDepthIndexStart + index] = 0xFFFFFFFFFFFFFFFFull;
		}

	}
	return;
}

static inline EB_ERRORTYPE ChromaPrediction(
	PictureControlSet_t                 *pictureControlSetPtr,
	ModeDecisionCandidateBuffer_t       *candidateBuffer,
	EB_U32                               cuChromaOriginIndex,
	ModeDecisionContext_t               *contextPtr)
{
	EB_ERRORTYPE                        return_error = EB_ErrorNone;

    (void) cuChromaOriginIndex;

     if (candidateBuffer->candidatePtr->predictionIsReady == EB_FALSE) 
    {
		const EB_U8 type = candidateBuffer->candidatePtr->type;

		contextPtr->puOriginX = contextPtr->cuOriginX;

		contextPtr->puOriginY = contextPtr->cuOriginY;

		contextPtr->puWidth = contextPtr->cuSize;

		contextPtr->puHeight = contextPtr->cuSize;

		contextPtr->puItr = 0;

		if (contextPtr->intraMdOpenLoopFlag){

            PredictionFunTableOl[contextPtr->interpolationMethod][type](
				contextPtr,
				PICTURE_BUFFER_DESC_CHROMA_MASK,
				pictureControlSetPtr,
				candidateBuffer);			
		}
		else{
            PredictionFunTableCl[contextPtr->interpolationMethod][type](
				contextPtr,
				PICTURE_BUFFER_DESC_CHROMA_MASK,
				pictureControlSetPtr,
				candidateBuffer);
		}
	}

	return return_error;
}

void ProductMdFastPuPrediction(
	PictureControlSet_t                 *pictureControlSetPtr,
	ModeDecisionCandidateBuffer_t       *candidateBuffer,
	ModeDecisionContext_t               *contextPtr,
	EB_U32                               useChromaInformationInFastLoop,
	EB_U32                               modeType,
	ModeDecisionCandidate_t				*const candidatePtr,
	EB_U32								 fastLoopCandidateIndex,
	EB_U32								 bestFirstFastCostSearchCandidateIndex)
{
	contextPtr->puItr = 0;

	// Prediction
	if (contextPtr->intraMdOpenLoopFlag){	

		EB_U32 predMask = useChromaInformationInFastLoop ? PICTURE_BUFFER_DESC_FULL_MASK : PICTURE_BUFFER_DESC_LUMA_MASK;
		if (fastLoopCandidateIndex == bestFirstFastCostSearchCandidateIndex && candidatePtr->type == INTRA_MODE)
			predMask = predMask & (~PICTURE_BUFFER_DESC_Y_FLAG);

		candidateBuffer->candidatePtr->predictionIsReadyLuma = (EB_BOOL)(predMask & PICTURE_BUFFER_DESC_Y_FLAG);

        PredictionFunTableOl[contextPtr->interpolationMethod][modeType](
			contextPtr,
			predMask,
			pictureControlSetPtr,
			candidateBuffer);

	}
	else{

		candidateBuffer->candidatePtr->predictionIsReadyLuma = EB_TRUE;

        PredictionFunTableCl[contextPtr->interpolationMethod][modeType](
			contextPtr,
			useChromaInformationInFastLoop ? PICTURE_BUFFER_DESC_FULL_MASK : PICTURE_BUFFER_DESC_LUMA_MASK,
			pictureControlSetPtr,
			candidateBuffer);

	}

}

void ModeDecisionPreFetchRef(
    PictureControlSet_t             *pictureControlSetPtr,
    ModeDecisionContext_t           *contextPtr,
    ModeDecisionCandidateBuffer_t   *candidateBuffer,
    EB_BOOL                          is16bit)
{

    if (candidateBuffer->candidatePtr->type == 1){
        const EB_U32 puOriginX = contextPtr->cuOriginX;
        const EB_U32 puOriginY = contextPtr->cuOriginY;
        const EB_U32 puHeight = contextPtr->cuStats->size;
        const EB_U32 puWidth = contextPtr->cuStats->size;

        EB_S16 motionVector_x;
        EB_S16 motionVector_y;


        if (contextPtr->cuUseRefSrcFlag)
            is16bit = EB_FALSE;

        if (is16bit)
        {
            if ((candidateBuffer->candidatePtr->predictionDirection[0] == UNI_PRED_LIST_0) || (candidateBuffer->candidatePtr->predictionDirection[0] == BI_PRED))
            {
                EbPictureBufferDesc_t  *refPicList0 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U32                  refList0PosX = 0;
                EB_U32                  refList0PosY = 0;
                EB_U32					integPosL0x;
                EB_U32					integPosL0y;
                EB_U8					counter;
                EB_U16				   *src0Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
                refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

                motionVector_x = candidateBuffer->candidatePtr->motionVector_x_L0;
                motionVector_y = candidateBuffer->candidatePtr->motionVector_y_L0;
                if (contextPtr->roundMvToInteger) {
                    RoundMvOnTheFly(
                        &motionVector_x,
                        &motionVector_y);
                }

                refList0PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList0->width) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
                    );
                refList0PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList0->height) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
                    );

                //uni-prediction List0 luma
                //compute the luma fractional position
                integPosL0x = (refList0PosX >> 2);
                integPosL0y = (refList0PosY >> 2);

                src0Ptr = (EB_U16 *)refPicList0->bufferY + integPosL0x + integPosL0y*(refPicList0->strideY);
                for (counter = 0; counter < puHeight; counter++)
                {
                    char const* p0 = (char const*)(src0Ptr + counter*refPicList0->strideY);
                    _mm_prefetch(p0, _MM_HINT_T2);
                    char const* p1 = (char const*)(src0Ptr + counter*refPicList0->strideY + (puWidth >> 1));
                    _mm_prefetch(p1, _MM_HINT_T2);
                }
            }
            if ((candidateBuffer->candidatePtr->predictionDirection[0] == UNI_PRED_LIST_1) || (candidateBuffer->candidatePtr->predictionDirection[0] == BI_PRED))
            {
                EbPictureBufferDesc_t  *refPicList1 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U32                  refList1PosX = 0;
                EB_U32                  refList1PosY = 0;
                EB_U32					integPosL1x;
                EB_U32					integPosL1y;
                EB_U8					counter;
                EB_U16				   *src1Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
                refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

                motionVector_x = candidateBuffer->candidatePtr->motionVector_x_L1;
                motionVector_y = candidateBuffer->candidatePtr->motionVector_y_L1;
                if (contextPtr->roundMvToInteger) {
                    RoundMvOnTheFly(
                        &motionVector_x,
                        &motionVector_y);
                }

                refList1PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList1->width) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
                    );
                refList1PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList1->height) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
                    );

                //uni-prediction List0 luma
                //compute the luma fractional position
                integPosL1x = (refList1PosX >> 2);
                integPosL1y = (refList1PosY >> 2);

                src1Ptr = (EB_U16 *)refPicList1->bufferY + integPosL1x + integPosL1y*(refPicList1->strideY);
                for (counter = 0; counter < puHeight; counter++)
                {
                    char const* p0 = (char const*)(src1Ptr + counter*refPicList1->strideY);
                    _mm_prefetch(p0, _MM_HINT_T2);
                    char const* p1 = (char const*)(src1Ptr + counter*refPicList1->strideY + (puWidth >> 1));
                    _mm_prefetch(p1, _MM_HINT_T2);
                }
            }


        }
        else
        {
            if ((candidateBuffer->candidatePtr->predictionDirection[0] == UNI_PRED_LIST_0) || (candidateBuffer->candidatePtr->predictionDirection[0] == BI_PRED))
            {
                EbPictureBufferDesc_t  *refPicList0 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U32                  refList0PosX = 0;
                EB_U32                  refList0PosY = 0;
                EB_U32					integPosL0x;
                EB_U32					integPosL0y;
                EB_U8					counter;
                EB_U8				   *src0Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
                refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

                if (contextPtr->cuUseRefSrcFlag)

                    refPicList0 = referenceObject->refDenSrcPicture;
                else
                    refPicList0 = referenceObject->referencePicture;


                motionVector_x = candidateBuffer->candidatePtr->motionVector_x_L0;
                motionVector_y = candidateBuffer->candidatePtr->motionVector_y_L0;
                if (contextPtr->roundMvToInteger) {
                    RoundMvOnTheFly(
                        &motionVector_x,
                        &motionVector_y);
                }

                refList0PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList0->width) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
                    );
                refList0PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList0->height) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
                    );


                //uni-prediction List0 luma
                //compute the luma fractional position
                integPosL0x = (refList0PosX >> 2);
                integPosL0y = (refList0PosY >> 2);

                src0Ptr = refPicList0->bufferY + integPosL0x + integPosL0y*refPicList0->strideY;
                for (counter = 0; counter < puHeight; counter++)
                {
                    char const* p0 = (char const*)(src0Ptr + counter*refPicList0->strideY);
                    _mm_prefetch(p0, _MM_HINT_T2);
                }

            }
            if ((candidateBuffer->candidatePtr->predictionDirection[0] == UNI_PRED_LIST_1) || (candidateBuffer->candidatePtr->predictionDirection[0] == BI_PRED))
            {
                EbPictureBufferDesc_t  *refPicList1 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U32                  refList1PosX = 0;
                EB_U32                  refList1PosY = 0;
                EB_U32					integPosL1x;
                EB_U32					integPosL1y;
                EB_U8					counter;
                EB_U8				   *src1Ptr;

                // List1
                referenceObject = (EbReferenceObject_t *)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
                refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

                if (contextPtr->cuUseRefSrcFlag)
                    refPicList1 = referenceObject->refDenSrcPicture;
                else
                    refPicList1 = referenceObject->referencePicture;

                motionVector_x = candidateBuffer->candidatePtr->motionVector_x_L1;
                motionVector_y = candidateBuffer->candidatePtr->motionVector_y_L1;
                if (contextPtr->roundMvToInteger) {
                    RoundMvOnTheFly(
                        &motionVector_x,
                        &motionVector_y);
                }

                refList1PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList1->width) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
                    );
                refList1PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                    (EB_S32)(((refPicList1->height) << 2) + MVBOUNDHIGH),
                    (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
                    );

                //uni-prediction List1 luma
                //compute the luma fractional position
                integPosL1x = (refList1PosX >> 2);
                integPosL1y = (refList1PosY >> 2);


                src1Ptr = refPicList1->bufferY + integPosL1x + integPosL1y*refPicList1->strideY;
                for (counter = 0; counter < puHeight; counter++)
                {
                    char const* p1 = (char const*)(src1Ptr + counter*refPicList1->strideY);
                    _mm_prefetch(p1, _MM_HINT_T2);
                }

            }
        }

    }
}

void ProductPerformFastLoop(
	PictureControlSet_t			   *pictureControlSetPtr,
	LargestCodingUnit_t            *lcuPtr,
	ModeDecisionContext_t		   *contextPtr,
	ModeDecisionCandidateBuffer_t **candidateBufferPtrArrayBase,
	ModeDecisionCandidate_t		   *fastCandidateArray,
	EB_U32							fastCandidateTotalCount,
	EbPictureBufferDesc_t          *inputPicturePtr,
	EB_U32							inputOriginIndex,
	EB_U32							inputCbOriginIndex,
	EB_U32							inputCrOriginIndex,
	CodingUnit_t                   *cuPtr,
	EB_U32							cuOriginIndex,
	EB_U32							cuChromaOriginIndex,
	EB_U32							maxBuffers,
	EB_U32                         *secondFastCostSearchCandidateTotalCount) {

	EB_S32                          fastLoopCandidateIndex;
	EB_U64                          lumaFastDistortion;
	EB_U64                          chromaFastDistortion;
	ModeDecisionCandidateBuffer_t  *candidateBuffer;
	const EB_PICTURE                  sliceType = pictureControlSetPtr->sliceType;
	EB_U32                          highestCostIndex;
	EB_U64                          highestCost;
	EB_U32							isCandzz = 0;
	const EB_U32 cuDepth = contextPtr->cuStats->depth;
	const EB_U32 cuSize = contextPtr->cuStats->size;
	EB_U32 firstFastCandidateTotalCount;
	// Initialize first fast cost loop variables
	EB_U64 bestFirstFastCostSearchCandidateCost = 0xFFFFFFFFFFFFFFFFull;
	EB_S32 bestFirstFastCostSearchCandidateIndex = INVALID_FAST_CANDIDATE_INDEX;
	SequenceControlSet_t           *sequenceControlSetPtr = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr);
	LcuBasedDetectors_t* mdPicLcuDetect = contextPtr->mdPicLcuDetect;

	if (contextPtr->singleFastLoopFlag == EB_FALSE) {

		firstFastCandidateTotalCount = 0;
		// First Fast-Cost Search Candidate Loop
		fastLoopCandidateIndex = fastCandidateTotalCount - 1;
		do
		{
			lumaFastDistortion = 0;

			// Set the Candidate Buffer
			candidateBuffer = candidateBufferPtrArrayBase[0];
			ModeDecisionCandidate_t *const candidatePtr = candidateBuffer->candidatePtr = &fastCandidateArray[fastLoopCandidateIndex];
			const unsigned distortionReady = candidatePtr->distortionReady;

			// Only check (src - src) candidates (Tier0 candidates)
			if (!!distortionReady)
			{
				const EB_U32 type = candidatePtr->type;

				lumaFastDistortion = candidatePtr->meDistortion;
				firstFastCandidateTotalCount++;
                               
                // Fast Cost Calc
                ProductFastCostFuncOptTable[type][sliceType](
                    contextPtr,
                    cuPtr,
                    candidateBuffer,
                    cuPtr->qp,
                    lumaFastDistortion,
                    0,
                    contextPtr->fastLambda,
                    pictureControlSetPtr);

				// Keep track of the candidate index of the best  (src - src) candidate
				if (*(candidateBuffer->fastCostPtr) <= bestFirstFastCostSearchCandidateCost) {
					bestFirstFastCostSearchCandidateIndex = fastLoopCandidateIndex;
					bestFirstFastCostSearchCandidateCost = *(candidateBuffer->fastCostPtr);
				}

				// Initialize Fast Cost - to do not interact with the second Fast-Cost Search
				*(candidateBuffer->fastCostPtr) = 0xFFFFFFFFFFFFFFFFull;				
			}
		} while (--fastLoopCandidateIndex >= 0);
	}

	// Second Fast-Cost Search Candidate Loop
	*secondFastCostSearchCandidateTotalCount = 0;
	highestCostIndex = contextPtr->bufferDepthIndexStart[cuDepth];
	fastLoopCandidateIndex = fastCandidateTotalCount - 1;

	EB_U16 lcuAddr = lcuPtr->index;

    do
    {
        candidateBuffer = candidateBufferPtrArrayBase[highestCostIndex];
        ModeDecisionCandidate_t *const  candidatePtr = candidateBuffer->candidatePtr = &fastCandidateArray[fastLoopCandidateIndex];
        const unsigned                  distortionReady = candidatePtr->distortionReady;
        EbPictureBufferDesc_t * const   predictionPtr = candidateBuffer->predictionPtr;

        candidatePtr->predictionIsReady = EB_FALSE;
        candidatePtr->predictionIsReadyLuma = EB_TRUE;

        if ((!distortionReady) || fastLoopCandidateIndex == bestFirstFastCostSearchCandidateIndex || contextPtr->singleFastLoopFlag) {

            contextPtr->roundMvToInteger = (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == EB_FALSE && candidatePtr->type == INTER_MODE &&  candidatePtr->mergeFlag == EB_TRUE) ?
                EB_TRUE :
                EB_FALSE;

            lumaFastDistortion = 0;
            chromaFastDistortion = 0;
            // Set the Candidate Buffer
            candidatePtr->predictionIsReady = contextPtr->useChromaInformationInFastLoop ? EB_TRUE : EB_FALSE;


            ModeDecisionPreFetchRef(
                pictureControlSetPtr,
                contextPtr,
                candidateBuffer,
                (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT));

            ProductMdFastPuPrediction(
                pictureControlSetPtr,
                candidateBuffer,
                contextPtr,
                contextPtr->useChromaInformationInFastLoop,
                candidatePtr->type,
                candidatePtr,
                fastLoopCandidateIndex,
                bestFirstFastCostSearchCandidateIndex);

            //Distortion
            EB_U8 * const inputBufferY = inputPicturePtr->bufferY + inputOriginIndex;
            const unsigned inputStrideY = inputPicturePtr->strideY;
            EB_U8 * const predBufferY = predictionPtr->bufferY + cuOriginIndex;
            // Skip distortion computation if the candidate is MPM
            if (candidateBuffer->candidatePtr->mpmFlag == EB_FALSE) {

				if (fastLoopCandidateIndex == bestFirstFastCostSearchCandidateIndex && candidatePtr->type == INTRA_MODE)
					lumaFastDistortion = candidatePtr->meDistortion;
				else
					// Y
					lumaFastDistortion += (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3] ( 
						inputBufferY,
						inputStrideY,
						predBufferY,
						MAX_LCU_SIZE ,
						cuSize,
						cuSize));

                // Cb
                if (!!contextPtr->useChromaInformationInFastLoop)
                {
                    EB_U8 * const inputBufferCb = inputPicturePtr->bufferCb + inputCbOriginIndex;
                    EB_U8 *  const predBufferCb = candidateBuffer->predictionPtr->bufferCb + cuChromaOriginIndex;

					chromaFastDistortion += NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 4] ( 
						inputBufferCb,
						inputPicturePtr->strideCb,
						predBufferCb,
						(MAX_LCU_SIZE >> 1) ,
						(cuSize >> 1) ,
						cuSize >> 1);


                    EB_U8 * const inputBufferCr = inputPicturePtr->bufferCr + inputCrOriginIndex;
                    EB_U8 * const predBufferCr = candidateBuffer->predictionPtr->bufferCr + cuChromaOriginIndex;

                    chromaFastDistortion += NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 4] (
                        inputBufferCr,
                        inputPicturePtr->strideCb ,
                        predBufferCr,
                        (MAX_LCU_SIZE >> 1),
                        (cuSize >> 1),
                        cuSize >> 1) ;
                }
            }
            if (pictureControlSetPtr->ParentPcsPtr->cmplxStatusLcu[lcuAddr] == CMPLX_NOISE) {

                candidateBuffer->weightChromaDistortion = EB_FALSE;
                if (cuSize == MAX_LCU_SIZE){
                    EB_U32  predDirection = (EB_U32)candidatePtr->predictionDirection[0];
                    EB_BOOL list0ZZ = (predDirection & 1) ? EB_TRUE : (EB_BOOL)(candidatePtr->motionVector_x_L0 == 0 && candidatePtr->motionVector_y_L0 == 0);
                    EB_BOOL list1ZZ = (predDirection > 0) ? (EB_BOOL)(candidatePtr->motionVector_x_L1 == 0 && candidatePtr->motionVector_y_L1 == 0) : EB_TRUE;

                    isCandzz = (list0ZZ && list1ZZ) ? 1 : 0;
                    chromaFastDistortion = isCandzz ? chromaFastDistortion >> 2 : chromaFastDistortion;
                }

            }
            else{
                candidateBuffer->weightChromaDistortion = EB_TRUE;
            }

			ProductFastCostFuncOptTable[candidatePtr->type][sliceType](
				contextPtr,
				cuPtr,
				candidateBuffer,
				cuPtr->qp,
				lumaFastDistortion,
				chromaFastDistortion,
				contextPtr->fastLambda,
				pictureControlSetPtr);
            
            candidateBuffer->candidatePtr->fastLoopLumaDistortion = (EB_U32)lumaFastDistortion;
            if (contextPtr->useIntraInterBias){
                if (candidatePtr->type == INTRA_MODE)
                    ApplyIntraInterModeBias(
                    mdPicLcuDetect->intraInterCond1,
                    mdPicLcuDetect->intraInterCond2,
                    mdPicLcuDetect->intraInterCond3,
                    candidateBuffer->fastCostPtr);

                // Bias against bipred merge candidates to reduce blurriness in layer 0 pictures
                if (pictureControlSetPtr->sliceType != EB_I_PICTURE){
                    if (pictureControlSetPtr->temporalLayerIndex <= 1){
                        if (candidateBuffer->candidatePtr->type == INTER_MODE && (EB_U32)candidatePtr->predictionDirection[0] == BI_PRED) {
                            // Myanmar_5&6 flickering on the Grass
                            *candidateBuffer->fastCostPtr += mdPicLcuDetect->biPredCond1 ? (*candidateBuffer->fastCostPtr * 20) / 100 : 0;

                        }
                    }
				}
			}           

            if (sequenceControlSetPtr->staticConfig.improveSharpness)
                ApplyMvOverBoundariesBias(
                    sequenceControlSetPtr,
                    contextPtr,
                    cuSize,
                    candidateBuffer);

            // Zero out cost if the candidate is MPM
            if (candidatePtr->mpmFlag == EB_TRUE) {
                *candidateBuffer->fastCostPtr = 0;
            }

            (*secondFastCostSearchCandidateTotalCount)++;
        }

        // Find the buffer with the highest cost  
        if (fastLoopCandidateIndex)
        {
            // maxCost is volatile to prevent the compiler from loading 0xFFFFFFFFFFFFFF
            //   as a const at the early-out. Loading a large constant on intel x64 processors
            //   clogs the i-cache/intstruction decode. This still reloads the variable from
            //   the stack each pass, so a better solution would be to register the variable,
            //   but this might require asm.

            volatile EB_U64 maxCost = ~0ull;
            const EB_U64 *fastCostArray = contextPtr->fastCostArray;
            const EB_U32 bufferIndexStart = contextPtr->bufferDepthIndexStart[cuDepth];
            const EB_U32 bufferIndexEnd = bufferIndexStart + maxBuffers;

            EB_U32 bufferIndex;

            highestCostIndex = bufferIndexStart;
            bufferIndex = bufferIndexStart + 1;

            do {

                highestCost = fastCostArray[highestCostIndex];

                if (highestCost == maxCost) {
                    break;
                }

                if (fastCostArray[bufferIndex] > highestCost)
                {
                    highestCostIndex = bufferIndex;
                }

            } while (++bufferIndex < bufferIndexEnd);

        }
    } while (--fastLoopCandidateIndex >= 0);// End Second FastLoop

}


void  ApplyIntraInterModeBias(
    EB_BOOL                        intraInterCond1,
    EB_BOOL                        intraInterCond2,
	EB_BOOL                        intraInterCond3,
	EB_U64                         *fullCostPtr)
{

    *fullCostPtr += intraInterCond1 ? (*fullCostPtr * 30) / 100 : 0;
    *fullCostPtr += intraInterCond2 ? (*fullCostPtr * 30) / 100 : 0;
	*fullCostPtr = intraInterCond3 ? *fullCostPtr * 3 : *fullCostPtr;
}

extern void GenerateIntraChromaReferenceSamplesMd(
	ModeDecisionContext_t      *contextPtr,
	EbPictureBufferDesc_t      *inputPicturePtr) {


    if (contextPtr->useChromaInformationInFullLoop && contextPtr->intraMdOpenLoopFlag == EB_FALSE) {

		GenerateChromaIntraReferenceSamplesEncodePass(
			EB_FALSE,
			EB_TRUE,
			contextPtr->cuOriginX,
			contextPtr->cuOriginY,
			contextPtr->cuSize,
			contextPtr->lcuSize,
			contextPtr->cuDepth,
			contextPtr->modeTypeNeighborArray,
			contextPtr->lumaReconNeighborArray,
			contextPtr->cbReconNeighborArray,
			contextPtr->crReconNeighborArray,
			contextPtr->intraRefPtr,
            EB_YUV420,
            EB_FALSE,
			EB_FALSE,
			EB_FALSE,
			EB_FALSE);

		contextPtr->chromaIntraRefSamplesGenDone = EB_TRUE;

	}
    else {
		UpdateChromaNeighborSamplesArrayOL(
			contextPtr->intraRefPtr,
			inputPicturePtr,
			inputPicturePtr->strideY,
			contextPtr->cuOriginX,
			contextPtr->cuOriginY,
			inputPicturePtr->strideCb,
			inputPicturePtr->strideCr,
			contextPtr->cuChromaOriginX,
			contextPtr->cuChromaOriginY,
			contextPtr->cuSize >> 1,
			contextPtr->lcuPtr);

		contextPtr->chromaIntraRefSamplesGenDone = EB_TRUE;
	}
}

void DerivePartialFrequencyN2Flag(
	PictureControlSet_t				*pictureControlSetPtr,
	ModeDecisionContext_t			*contextPtr)
{
    if (contextPtr->pfMdLevel == 0) {
        contextPtr->pfMdMode = PF_OFF;
    }
    else  if (contextPtr->pfMdLevel == 1) {
        contextPtr->pfMdMode = PF_N2;
    }
    else  if (contextPtr->pfMdLevel == 2) {
        contextPtr->pfMdMode = ((contextPtr->cuSize <= 16) || (pictureControlSetPtr->ParentPcsPtr->failingMotionLcuFlag[contextPtr->lcuPtr->index]) || (pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[contextPtr->lcuPtr->index].edgeBlockNum > 0) || (pictureControlSetPtr->ParentPcsPtr->isPan)) ? PF_N2 : PF_N4;
    }
    else  {
        contextPtr->pfMdMode = (contextPtr->cuSize <= 8) ? PF_N2 : PF_N4;
    }
}



void PerformIntraPrediction(
    PictureControlSet_t				*pictureControlSetPtr,
    ModeDecisionContext_t			*contextPtr,
    ModeDecisionCandidateBuffer_t	*candidateBuffer,
    ModeDecisionCandidate_t         *candidatePtr) {

    if (contextPtr->intraMdOpenLoopFlag){

        EB_U8 puItr = 0;
        {

            // Set the PU Loop Variables
            contextPtr->puItr = puItr;
            contextPtr->puPtr = &contextPtr->cuPtr->predictionUnitArray[puItr];

            PredictionFunTableOl[contextPtr->interpolationMethod][candidatePtr->type](
				contextPtr,
				PICTURE_BUFFER_DESC_LUMA_MASK,
				pictureControlSetPtr,
				candidateBuffer);
        }
    }
    else{
        SVT_LOG("ERR: prediction not ready");
    }
   
}

// Skip smaller CU sizes if the current tested CU has the same depth as the TOP and LEFT CU
void SkipSmallCu(
    PictureControlSet_t    *pictureControlSetPtr,
    ModeDecisionContext_t  *contextPtr,
    CodingUnit_t * const    cuPtr)
{
    EB_U8 cuDepth   = contextPtr->cuStats->depth;
    EB_U16 lcuAddr  = contextPtr->lcuPtr->index;

    if (pictureControlSetPtr->lcuPtrArray[lcuAddr]->auraStatus == AURA_STATUS_0 && (pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuAddr]).stationaryEdgeOverTimeFlag == 0) {
		if (contextPtr->mdLocalCuUnit[cuPtr->leafIndex].leftNeighborDepth == cuDepth  &&   contextPtr->mdLocalCuUnit[cuPtr->leafIndex].topNeighborDepth == cuDepth  && cuPtr->splitFlag == EB_TRUE){
            cuPtr->splitFlag = EB_FALSE;
        }
    }

}

void Intra4x4CodingLoopContextGeneration(
	ModeDecisionContext_t   *contextPtr,
	EB_U32                   puIndex,
	EB_U32                   puOriginX,
	EB_U32                   puOriginY,
	EB_U32                   lcuSize,
	NeighborArrayUnit_t     *intraLumaNeighborArray,
	NeighborArrayUnit_t     *modeTypeNeighborArray)
{
	EB_U32 modeTypeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		modeTypeNeighborArray,
		puOriginY);
	EB_U32 modeTypeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		modeTypeNeighborArray,
		puOriginX);
	EB_U32 intraLumaModeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		intraLumaNeighborArray,
		puOriginY);
	EB_U32 intraLumaModeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		intraLumaNeighborArray,
		puOriginX);

    contextPtr->intraLumaLeftModeArray[puIndex] = (EB_U32)(
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		(EB_U32)intraLumaNeighborArray->leftArray[intraLumaModeLeftNeighborIndex]);

    contextPtr->intraLumaTopModeArray[puIndex] = (EB_U32)(
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		((puOriginY & (lcuSize - 1)) == 0) ? EB_INTRA_DC :      // If we are at the top of the LCU boundary, then
		(EB_U32)intraLumaNeighborArray->topArray[intraLumaModeTopNeighborIndex]);                      //   use DC. This seems like we could use a LCU-width

	return;
}


void Intra4x4VsIntra8x8(
	ModeDecisionContext_t *contextPtr,
    LargestCodingUnit_t *lcuPtr,
    EB_U8                *intra4x4LumaMode,
    EB_U64				 *intra4x4Cost,
    EB_U64               *intra4x4FullDistortion,
    EB_BOOL              useChromaInformationInFullLoop,
    CodingUnit_t	     *cuPtr)
{
    EB_U32                  puIndex;

    if(cuPtr->predictionModeFlag!=INTRA_MODE)
        SVT_LOG("SVT [WARNING]: cuPtr->costLuma needs to be filled in inter case");

		if (*intra4x4Cost < contextPtr->mdLocalCuUnit[cuPtr->leafIndex].costLuma) {

			if (useChromaInformationInFullLoop) {
				contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost = *intra4x4Cost;
			}
			else {
				contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost = contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost - (contextPtr->mdLocalCuUnit[cuPtr->leafIndex].costLuma - *intra4x4Cost);
			}

		    contextPtr->mdLocalCuUnit[cuPtr->leafIndex].fullDistortion = (EB_U32)*intra4x4FullDistortion;

            cuPtr->predictionUnitArray->intraLumaMode = EB_INTRA_MODE_4x4;
            for (puIndex = 0; puIndex < 4; puIndex++) {
            
                lcuPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuPtr->leafIndex] - 21) << 2) + puIndex] = intra4x4LumaMode[puIndex];
        }
    }
}

EB_U32 Intra4x4FullModeDecision(
    EB_U8                          *intra4x4LumaMode,
    EB_U64                         *intra4x4Cost,
    EB_U64                         *intra4x4FullDistortion,
    ModeDecisionCandidateBuffer_t **bufferPtrArray,
    EB_U32                          candidateTotalCount,
    EB_U8                          *bestCandidateIndexArray)
{
    EB_U32                  candidateIndex;
    EB_U64                  lowestCost = 0xFFFFFFFFFFFFFFFFull;
    EB_U32                  lowestCostIndex = 0;

    EB_U32                  fullLoopCandidateIndex;

    ModeDecisionCandidate_t *candidatePtr;

    // Find the candidate with the lowest cost
    for (fullLoopCandidateIndex = 0; fullLoopCandidateIndex < candidateTotalCount; ++fullLoopCandidateIndex) {

        candidateIndex = bestCandidateIndexArray[fullLoopCandidateIndex];

        if (*(bufferPtrArray[candidateIndex]->fullCostPtr) < lowestCost){
            lowestCostIndex = candidateIndex;
            lowestCost = *(bufferPtrArray[candidateIndex]->fullCostPtr);
        }

    }

    candidatePtr = bufferPtrArray[lowestCostIndex]->candidatePtr;



    *intra4x4Cost += lowestCost;

    *intra4x4LumaMode = candidatePtr->intraLumaMode;

    *intra4x4FullDistortion += candidatePtr->fullDistortion;

    return lowestCostIndex;
}

EB_ERRORTYPE Intra4x4ModeDecisionControl(
    ModeDecisionContext_t          *contextPtr,
    EB_U32                         *bufferTotalCountPtr,
    EB_U32                         *candidateTotalCountPtr,
    ModeDecisionCandidate_t       **candidatePtrArray,
    EB_U32							bestIntraMode)
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_U32          canTotalCnt = 0;

    EB_U32          candidateIndex;

    *bufferTotalCountPtr = contextPtr->intra4x4Nfl;

    if (contextPtr->intra4x4IntraInjection == 0) {
        EB_U32  intraCandidateIndex;
        for (intraCandidateIndex = 0; intraCandidateIndex < MAX_INTRA_MODES; ++intraCandidateIndex) {

            candidatePtrArray[canTotalCnt]->type = INTRA_MODE;
            candidatePtrArray[canTotalCnt]->intraLumaMode = (EB_U32)intraCandidateIndex;
            candidatePtrArray[canTotalCnt]->distortionReady = 0;
            canTotalCnt++;
        }
    } 
    else {
        // DC
        candidatePtrArray[canTotalCnt]->type = INTRA_MODE;
        candidatePtrArray[canTotalCnt]->intraLumaMode = (EB_U32)EB_INTRA_DC;
        candidatePtrArray[canTotalCnt]->distortionReady = 0;
        canTotalCnt++;

        if (bestIntraMode != EB_INTRA_MODE_INVALID &&  bestIntraMode != EB_INTRA_DC) {

            candidatePtrArray[canTotalCnt]->type = INTRA_MODE;
            candidatePtrArray[canTotalCnt]->intraLumaMode = (EB_U32)bestIntraMode;
            candidatePtrArray[canTotalCnt]->distortionReady = 0;
            canTotalCnt++;
        }

        if (bestIntraMode > EB_INTRA_MODE_4) {
            candidatePtrArray[canTotalCnt]->type = INTRA_MODE;
            candidatePtrArray[canTotalCnt]->intraLumaMode = (EB_U32)bestIntraMode - 3;
            candidatePtrArray[canTotalCnt]->distortionReady = 0;
            canTotalCnt++;
        }

        if ((bestIntraMode >= EB_INTRA_MODE_2) && (bestIntraMode < EB_INTRA_MODE_32)) {
            candidatePtrArray[canTotalCnt]->type = INTRA_MODE;
            candidatePtrArray[canTotalCnt]->intraLumaMode = (EB_U32)bestIntraMode + 3;
            candidatePtrArray[canTotalCnt]->distortionReady = 0;
            canTotalCnt++;
        }
    }

    // Loop over fast loop candidates
    for (candidateIndex = 0; candidateIndex < canTotalCnt; ++candidateIndex) {
        candidatePtrArray[candidateIndex]->mpmFlag = EB_FALSE;
    }

    *candidateTotalCountPtr = canTotalCnt;

    // Make sure bufferTotalCount is not larger than the number of fast modes
    *bufferTotalCountPtr = MIN(*candidateTotalCountPtr, *bufferTotalCountPtr);

    return return_error;
}



EB_ERRORTYPE Intra4x4PreModeDecision(
	EB_U32                          bufferTotalCount,
	ModeDecisionCandidateBuffer_t **bufferPtrArray,
	EB_U32                         *fullCandidateTotalCountPtr,
	EB_U8                          *bestCandidateIndexArray)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 fullCandidateIndex;
	EB_U32 fullReconCandidateCount;
	EB_U32                          highestCostIndex;
	EB_U64                          highestCost;
	EB_U32                          candIndx = 0, i;

	*fullCandidateTotalCountPtr = bufferTotalCount;

	//Second,  We substract one, because with N buffers we can determine the best N-1 candidates.
	//Note/TODO: in the case number of fast candidate is less or equal to the number of buffers, N buffers would be enough
	fullReconCandidateCount = MAX(1, (*fullCandidateTotalCountPtr) - 1);

	//With N buffers, we get here with the best N-1, plus the last candidate. We need to exclude the worst, and keep the best N-1.  
	highestCost = *(bufferPtrArray[0]->fastCostPtr);
	highestCostIndex = 0;

	if (bufferTotalCount > 1){
		for (i = 1; i < bufferTotalCount; i++) {

			if (*(bufferPtrArray[i]->fastCostPtr) >= highestCost){
				highestCost = *(bufferPtrArray[i]->fastCostPtr);
				highestCostIndex = i;
			}
		}
		for (i = 0; i < bufferTotalCount; i++) {

			if (i != highestCostIndex){
				bestCandidateIndexArray[candIndx++] = (EB_U8)i;
			}
		}
	}
	else
		bestCandidateIndexArray[0] = 0;


	// Set (*fullCandidateTotalCountPtr) to fullReconCandidateCount
	(*fullCandidateTotalCountPtr) = fullReconCandidateCount;

	for (i = 0; i < fullReconCandidateCount; ++i) {
		fullCandidateIndex = bestCandidateIndexArray[i];

		// Next, Set the transformSize
		bufferPtrArray[fullCandidateIndex]->candidatePtr->transformSize = TRANSFORM_MIN_SIZE;
		bufferPtrArray[fullCandidateIndex]->candidatePtr->transformChromaSize = TRANSFORM_MIN_SIZE;

	}

	return return_error;
}

static void Intra4x4InitFastLoop(
    ModeDecisionContext_t      *contextPtr,
    EbPictureBufferDesc_t      *inputPicturePtr,
    LargestCodingUnit_t        *lcuPtr,
    EB_U32                      partitionOriginX,
    EB_U32                      partitionOriginY,
	EB_BOOL                     useIntraChromaflag,
    EB_U32                      puIndex)
{


    if (contextPtr->intraMdOpenLoopFlag == EB_FALSE) {
        GenerateLumaIntraReferenceSamplesEncodePass(
            EB_FALSE,
            EB_TRUE,
            partitionOriginX,
            partitionOriginY,
            MIN_PU_SIZE,
            MAX_LCU_SIZE,
            3,
			contextPtr->modeTypeNeighborArray,
			contextPtr->lumaReconNeighborArray,
            (NeighborArrayUnit_t *) EB_NULL,
            (NeighborArrayUnit_t *) EB_NULL,
            contextPtr->intraRefPtr,
            EB_FALSE,
            EB_FALSE,
            EB_FALSE);

    }
    else{

        //TODO: Change this function to take care of neighbour validity.
        //      then create a generic function that takes recon or input frame as generic input for both open and close loop case.
        UpdateNeighborSamplesArrayOL(
            contextPtr->intraRefPtr,
            inputPicturePtr,
            inputPicturePtr->strideY,
            partitionOriginX,
            partitionOriginY,
            MIN_PU_SIZE,
            lcuPtr);
    }

	if (useIntraChromaflag){
		GenerateChromaIntraReferenceSamplesEncodePass(
			EB_FALSE,
			EB_TRUE,
			contextPtr->cuOriginX,
			contextPtr->cuOriginY,
			contextPtr->cuSize,
			contextPtr->lcuSize,
			contextPtr->cuDepth,
			contextPtr->modeTypeNeighborArray,
			contextPtr->lumaReconNeighborArray,
			contextPtr->cbReconNeighborArray,
			contextPtr->crReconNeighborArray,
			contextPtr->intraRefPtr,
            EB_YUV420,
            EB_FALSE,
			EB_FALSE,
			EB_FALSE,
			EB_FALSE);
	}
    // Generate Split, Skip and intra mode contexts for the rate estimation
    Intra4x4CodingLoopContextGeneration(
        contextPtr,
        puIndex,
        partitionOriginX,
        partitionOriginY,
        MAX_LCU_SIZE,
        contextPtr->intraLumaModeNeighborArray,
        contextPtr->modeTypeNeighborArray);

    // *Notes
    // -Instead of creating function pointers in a static array, put the func pointers in a queue
    // -This function should also do the SAD calc for each mode
    // -The best independent intra chroma mode should be determined here
    // -Modify PictureBufferDesc to be able to create Luma, Cb, and/or Cr only buffers via flags
    // -Have one PictureBufferDesc that points to the intra chroma buffer to be used.
    // -Properly signal the DM mode at this point

    // Initialize the candidate buffer costs
    {
        EB_U32 index = 0;
        for (index = 0; index < contextPtr->bufferDepthIndexWidth[MAX_LEVEL_COUNT - 1]; ++index){

            contextPtr->fastCostArray[(contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1]) + index] = 0xFFFFFFFFFFFFFFFFull;
            contextPtr->fullCostArray[(contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1]) + index] = 0xFFFFFFFFFFFFFFFFull;
            contextPtr->fullCostSkipPtr[(contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1]) + index] = 0xFFFFFFFFFFFFFFFFull;
            contextPtr->fullCostMergePtr[(contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1]) + index] = 0xFFFFFFFFFFFFFFFFull;

        }
    }
    return;
}


EB_U64 ComputeNxMSatd4x4Units(
    EB_S16 *diff,       // input parameter, diff samples Ptr
    EB_U32  diffStride, // input parameter, source stride
    EB_U32  width,      // input parameter, block width (N)
    EB_U32  height)     // input parameter, block height (M)
{

    EB_U64 satd = 0;
    EB_U32 blockIndexInWidth;
    EB_U32 blockIndexInHeight;
    EB_S16 subBlock[16];

    for (blockIndexInHeight = 0; blockIndexInHeight < height >> 2; ++blockIndexInHeight) {
        for (blockIndexInWidth = 0; blockIndexInWidth < width >> 2; ++blockIndexInWidth) {

            EB_MEMCPY(subBlock + 0x0, &(diff[(blockIndexInWidth << 2) + ((blockIndexInHeight << 2) + 0x0) * diffStride]), sizeof(EB_S16) << 2);
            EB_MEMCPY(subBlock + 0x4, &(diff[(blockIndexInWidth << 2) + ((blockIndexInHeight << 2) + 0x1) * diffStride]), sizeof(EB_S16) << 2);
            EB_MEMCPY(subBlock + 0x8, &(diff[(blockIndexInWidth << 2) + ((blockIndexInHeight << 2) + 0x2) * diffStride]), sizeof(EB_S16) << 2);
            EB_MEMCPY(subBlock + 0xC, &(diff[(blockIndexInWidth << 2) + ((blockIndexInHeight << 2) + 0x3) * diffStride]), sizeof(EB_S16) << 2);

            satd += Compute4x4Satd(subBlock);
        }
    }

    return satd;
}

EB_EXTERN EB_ERRORTYPE PerformIntra4x4Search(
    PictureControlSet_t   *pictureControlSetPtr,
    LargestCodingUnit_t   *lcuPtr,
    CodingUnit_t          *cuPtr,
    ModeDecisionContext_t *contextPtr,
    EB_U32				   bestIntraMode)

{
	EB_ERRORTYPE    return_error = EB_ErrorNone;

	// Candidate Buffer
	ModeDecisionCandidateBuffer_t **candidateBufferPtrArray;
	ModeDecisionCandidateBuffer_t  *candidateBuffer;
	ModeDecisionCandidate_t        *candidatePtr;
	EB_U32  candidateIndex = 0;
	EB_U32  highestCostIndex;
	EB_U32  bufferIndex;
	EB_U32  bufferIndexEnd;
	EB_U32  maxBuffers;
	EB_U32  bufferTotalCount;


	// Fast loop    
	EB_S32  fastLoopCandidateIndex;
	EB_U32  fastCandidateTotalCount;
	EB_U64  lumaFastDistortion;
	//EB_U64  chromaFastDistortion;
	EB_U64  highestCost;

	// Full Loop 
	EB_U32  fullLoopCandidateIndex;
	EB_U32  fullCandidateTotalCount;
	EB_U64  yFullDistortion[DIST_CALC_TOTAL];
	EB_U32  countNonZeroCoeffs;
	EB_U64  yCoeffBits;
	EB_U32  lumaShift;

	EbPictureBufferDesc_t   *tuTransCoeffTmpPtr;
	EbPictureBufferDesc_t   *tuQuantCoeffTmpPtr;
	EbPictureBufferDesc_t   *transformBuffer;
    EbPictureBufferDesc_t   *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->chromaDownSamplePicturePtr;


    EB_U32                   inputOriginIndex;
	EB_U32                   puOriginIndex;

	EB_U8                    intra4x4LumaMode[4];
	EB_U64                   intra4x4Cost = 0;
	EB_U64                   intra4x4FullDistortion = 0;

	EncodeContext_t         *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;
    EB_U8 partitionIndex;

	EB_BOOL                  use4x4ChromaInformation = EB_FALSE;
	EB_U32                   inputChromaOriginIndex;
	EB_U32                   puChromaOriginIndex;
	EB_U64                   chromaFastDistortion;
	EB_U64                   cbFullDistortion[DIST_CALC_TOTAL];
	EB_U32                   cbCountNonZeroCoeffs;
	EB_U64                   cbCoeffBits;
	EB_U64                   crFullDistortion[DIST_CALC_TOTAL];
	EB_U32                   crCountNonZeroCoeffs;
	EB_U64                   crCoeffBits;
	EB_U32                   chromaShift;
    
    if (contextPtr->coeffCabacUpdate)
        EB_MEMCPY(&(contextPtr->i4x4CoeffCtxModel), &(contextPtr->latestValidCoeffCtxModel), sizeof(CoeffCtxtMdl_t));

    for (partitionIndex = 0; partitionIndex < 4; partitionIndex++) {

        contextPtr->puPtr = &contextPtr->cuPtr->predictionUnitArray[contextPtr->puItr];

        EB_U32 partitionOriginX = contextPtr->cuOriginX + INTRA_4x4_OFFSET_X[partitionIndex];
        EB_U32 partitionOriginY = contextPtr->cuOriginY + INTRA_4x4_OFFSET_Y[partitionIndex];

        inputOriginIndex = (partitionOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (partitionOriginX + inputPicturePtr->originX);
        puOriginIndex = ((partitionOriginY & (MAX_LCU_SIZE - 1)) * MAX_LCU_SIZE) + (partitionOriginX & (MAX_LCU_SIZE - 1));

        candidateBufferPtrArray = &(contextPtr->candidateBufferPtrArray[contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1]]);

        inputChromaOriginIndex = (contextPtr->cuChromaOriginY + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCr + (contextPtr->cuChromaOriginX + (inputPicturePtr->originX >> 1));
        puChromaOriginIndex = (((contextPtr->cuOriginY & (contextPtr->lcuSize - 1)) * contextPtr->lcuChromaSize) + (contextPtr->cuOriginX & (contextPtr->lcuSize - 1))) >> 1;
        candidateBufferPtrArray = &(contextPtr->candidateBufferPtrArray[contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1]]);

        use4x4ChromaInformation = ((contextPtr->use4x4ChromaInformationInFullLoop == EB_TRUE) && (partitionIndex == 0)) ? EB_TRUE : EB_FALSE;


        // Initialize Fast Loop
        Intra4x4InitFastLoop(
            contextPtr,
            inputPicturePtr,
            lcuPtr,
            partitionOriginX,
            partitionOriginY,
            use4x4ChromaInformation,
            partitionIndex);

        // To move out of the PU loop in case all PUs are going to share same number of fast/full loop candidates 
        Intra4x4ModeDecisionControl(
            contextPtr,
            &bufferTotalCount,
            &fastCandidateTotalCount,
            contextPtr->fastCandidatePtrArray,
            bestIntraMode);

        CHECK_REPORT_ERROR(
            (bufferTotalCount <= contextPtr->bufferDepthIndexWidth[MAX_LEVEL_COUNT - 1]),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_CL_ERROR3);

        CHECK_REPORT_ERROR(
            (fastCandidateTotalCount <= MODE_DECISION_CANDIDATE_MAX_COUNT),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_CL_ERROR4);

        //if we want to recon N candidate, we would need N+1 buffers
        maxBuffers = MIN((bufferTotalCount + 1), contextPtr->bufferDepthIndexWidth[MAX_LEVEL_COUNT - 1]);

        // Fast-Cost Search Candidate Loop
        //      -Prediction 
        //      -(Input - Prediction) & SAD
        //      -Fast cost calc

        for (fastLoopCandidateIndex = fastCandidateTotalCount - 1; fastLoopCandidateIndex >= 0; --fastLoopCandidateIndex) {

            lumaFastDistortion = 0;

            chromaFastDistortion = 0;

            // Find the buffer with the highest cost         
            highestCostIndex = contextPtr->bufferDepthIndexStart[MAX_LEVEL_COUNT - 1];
            bufferIndex = highestCostIndex + 1;
            bufferIndexEnd = highestCostIndex + maxBuffers;

            highestCost = *(contextPtr->candidateBufferPtrArray[highestCostIndex]->fastCostPtr);
            do {

                if ((*(contextPtr->candidateBufferPtrArray[bufferIndex]->fastCostPtr) > highestCost)){
                    highestCostIndex = bufferIndex;
                    highestCost = *(contextPtr->candidateBufferPtrArray[bufferIndex]->fastCostPtr);
                }

                ++bufferIndex;
            } while ((bufferIndex < bufferIndexEnd) && (highestCost < 0xFFFFFFFFFFFFFFFFull));

            // Set the Candidate Buffer
            candidateBuffer = contextPtr->candidateBufferPtrArray[highestCostIndex];
            candidateBuffer->candidatePtr = contextPtr->fastCandidatePtrArray[fastLoopCandidateIndex];

            // Prediction
            if (contextPtr->intraMdOpenLoopFlag){

                Intra4x4IntraPredictionOl(
                    contextPtr->puItr,
                    partitionOriginX,
                    partitionOriginY,
                    MIN_PU_SIZE,
                    MIN_PU_SIZE,
                    MAX_LCU_SIZE,
                    use4x4ChromaInformation ? PICTURE_BUFFER_DESC_FULL_MASK : PICTURE_BUFFER_DESC_LUMA_MASK,
                    pictureControlSetPtr,
                    candidateBuffer,
                    contextPtr);

            }
            else{

                Intra4x4IntraPredictionCl(
                    contextPtr->puItr,
                    partitionOriginX,
                    partitionOriginY,
                    MIN_PU_SIZE,
                    MIN_PU_SIZE,
                    MAX_LCU_SIZE,
                    use4x4ChromaInformation ? PICTURE_BUFFER_DESC_FULL_MASK : PICTURE_BUFFER_DESC_LUMA_MASK,
                    pictureControlSetPtr,
                    candidateBuffer,
                    contextPtr);
            }

            // Skip distortion computation if the candidate is MPM
            if (candidateBuffer->candidatePtr->mpmFlag == EB_FALSE)
            {
                //Distortion
                if (use4x4ChromaInformation){
                    //Distortion
                    PictureFastDistortion(
                        inputPicturePtr,
                        inputOriginIndex,
                        inputChromaOriginIndex,
                        candidateBuffer->predictionPtr,
                        puOriginIndex,
                        puChromaOriginIndex,
                        MIN_PU_SIZE,
                        PICTURE_BUFFER_DESC_FULL_MASK,
                        &lumaFastDistortion,
                        &chromaFastDistortion);

                    // Fast Cost Calc
                    IntraNxNFastCostFuncTable[pictureControlSetPtr->sliceType](
                        cuPtr,
                        candidateBuffer,
                        contextPtr->qp,
                        lumaFastDistortion,
                        chromaFastDistortion,
                        contextPtr->fastLambda,
                        pictureControlSetPtr);
                }
                else{
                    PictureFastDistortion(
                        inputPicturePtr,
                        inputOriginIndex,
                        0,
                        candidateBuffer->predictionPtr,
                        puOriginIndex,
                        0,
                        MIN_PU_SIZE,
                        PICTURE_BUFFER_DESC_LUMA_MASK,
                        &lumaFastDistortion,
                        NULL);


                    // Fast Cost Calc
                    Intra4x4FastCostFuncTable[pictureControlSetPtr->sliceType](
                        contextPtr,
                        partitionIndex,
                        candidateBuffer,
                        lumaFastDistortion,
                        contextPtr->fastLambda);
                }
            }
        }

        // PreModeDecision
        // -Input is the buffers
        // -Output is list of buffers for full reconstruction

        Intra4x4PreModeDecision(
            (fastCandidateTotalCount == bufferTotalCount) ? bufferTotalCount : maxBuffers,
            candidateBufferPtrArray,
            &fullCandidateTotalCount,
            contextPtr->bestCandidateIndexArray);

        // Set fullCandidateTotalCount to number of buffers to process
        fullCandidateTotalCount = MIN(fullCandidateTotalCount, bufferTotalCount);

        CHECK_REPORT_ERROR(
            (fullCandidateTotalCount <= contextPtr->bufferDepthIndexWidth[MAX_LEVEL_COUNT - 1]),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_CL_ERROR5);

        // Full-Cost Search Luma Candidate Loop
        for (fullLoopCandidateIndex = 0; fullLoopCandidateIndex < fullCandidateTotalCount; ++fullLoopCandidateIndex) {
            if (fullLoopCandidateIndex < MAX_FULL_LOOP_CANIDATES_PER_DEPTH)
                candidateIndex = contextPtr->bestCandidateIndexArray[fullLoopCandidateIndex];

            yFullDistortion[DIST_CALC_RESIDUAL] = 0;
            yFullDistortion[DIST_CALC_PREDICTION] = 0;
            yCoeffBits = 0;
            countNonZeroCoeffs = 0;

            cbFullDistortion[DIST_CALC_RESIDUAL] = 0;
            cbFullDistortion[DIST_CALC_PREDICTION] = 0;
            cbCoeffBits = 0;
            cbCountNonZeroCoeffs = 0;
            crFullDistortion[DIST_CALC_RESIDUAL] = 0;
            crFullDistortion[DIST_CALC_PREDICTION] = 0;
            crCoeffBits = 0;
            crCountNonZeroCoeffs = 0;

            // Set the Candidate Buffer      
            candidateBuffer = candidateBufferPtrArray[candidateIndex];
            candidatePtr = candidateBuffer->candidatePtr;//this is the FastCandidateStruct 

            //4x4CandBuff <-- latest4x4
            if (contextPtr->coeffCabacUpdate)
                EB_MEMCPY(&(candidateBuffer->candBuffCoeffCtxModel), &(contextPtr->i4x4CoeffCtxModel), sizeof(CoeffCtxtMdl_t));


            //Y Residual
            contextPtr->puWidth = MIN_PU_SIZE;
            contextPtr->puHeight = MIN_PU_SIZE;
            PictureResidual(
                &(inputPicturePtr->bufferY[inputOriginIndex]),
                inputPicturePtr->strideY,
                &(candidateBuffer->predictionPtr->bufferY[puOriginIndex]),
                candidateBuffer->predictionPtr->strideY,
                &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[puOriginIndex]),
                candidateBuffer->residualQuantCoeffPtr->strideY,
                MIN_PU_SIZE,
                MIN_PU_SIZE);

            if (use4x4ChromaInformation){
                //Cb Residual
                PictureResidual(
                    &(inputPicturePtr->bufferCb[inputChromaOriginIndex]),
                    inputPicturePtr->strideCb,
                    &(candidateBuffer->predictionPtr->bufferCb[puChromaOriginIndex]),
                    candidateBuffer->predictionPtr->strideCb,
                    &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCb)[puChromaOriginIndex]),
                    candidateBuffer->residualQuantCoeffPtr->strideCb,
                    contextPtr->puWidth,
                    contextPtr->puHeight);

                //Cr Residual
                PictureResidual(
                    &(inputPicturePtr->bufferCr[inputChromaOriginIndex]),
                    inputPicturePtr->strideCr,
                    &(candidateBuffer->predictionPtr->bufferCr[puChromaOriginIndex]),
                    candidateBuffer->predictionPtr->strideCr,
                    &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCr)[puChromaOriginIndex]),
                    candidateBuffer->residualQuantCoeffPtr->strideCr,
                    contextPtr->puWidth,
                    contextPtr->puHeight);
            }

            // Y Transform
            tuTransCoeffTmpPtr = contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr;
            tuQuantCoeffTmpPtr = candidateBuffer->residualQuantCoeffPtr;
            EstimateTransform(
                &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[puOriginIndex]),
                candidateBuffer->residualQuantCoeffPtr->strideY,
                &(((EB_S16*)tuTransCoeffTmpPtr->bufferY)[puOriginIndex]),
                tuTransCoeffTmpPtr->strideY,
                candidateBuffer->candidatePtr->transformSize,
                contextPtr->transformInnerArrayPtr,
                0,
                EB_TRUE, // DST
                PF_OFF);

            // Y Quantize + Y InvQuantize
            UnifiedQuantizeInvQuantize(
                contextPtr->encDecContextPtr,
                lcuPtr->pictureControlSetPtr,
                &(((EB_S16*)tuTransCoeffTmpPtr->bufferY)[puOriginIndex]),
                tuTransCoeffTmpPtr->strideY,
                &(((EB_S16*)tuQuantCoeffTmpPtr->bufferY)[puOriginIndex]),
                &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[puOriginIndex]),
                contextPtr->cuPtr->qp,
                inputPicturePtr->bitDepth,
                candidateBuffer->candidatePtr->transformSize,
                pictureControlSetPtr->sliceType,
                &countNonZeroCoeffs,
                PF_OFF,
                0,
                0,
                0,
                0,
                0,
                COMPONENT_LUMA,
                pictureControlSetPtr->temporalLayerIndex,
                EB_FALSE,
                (CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
                contextPtr->fullLambda,
                candidateBuffer->candidatePtr->intraLumaMode,
                EB_INTRA_CHROMA_DM,
                pictureControlSetPtr->cabacCost);

            // Initialize luma CBF
            candidatePtr->yCbf = 0;

            transformBuffer = contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr;
            // *Full Distortion (SSE)
            // *Note - there are known issues with how this distortion metric is currently
            //    calculated.  The amount of scaling between the two arrays is not 
            //    equivalent.
            PictureFullDistortionLuma(
                transformBuffer,
                puOriginIndex,
                candidateBuffer->reconCoeffPtr,
                puOriginIndex,
                candidateBuffer->candidatePtr->transformSize,
                yFullDistortion,
                countNonZeroCoeffs,
                candidateBuffer->candidatePtr->type);

            lumaShift = 2 * (7 - Log2f(candidateBuffer->candidatePtr->transformSize));
            // Note: for square Transform, the scale is 1/(2^(7-Log2(Transform size)))
            // For NSQT the scale would be 1/ (2^(7-(Log2(first Transform size)+Log2(second Transform size))/2))
            // Add Log2 of Transform size in order to calculating it multiple time in this function
            yFullDistortion[DIST_CALC_RESIDUAL] = (yFullDistortion[DIST_CALC_RESIDUAL] + (EB_U64)(1 << (lumaShift - 1))) >> lumaShift;
            yFullDistortion[DIST_CALC_PREDICTION] = (yFullDistortion[DIST_CALC_PREDICTION] + (EB_U64)(1 << (lumaShift - 1))) >> lumaShift;

            EB_U8 intraLumaMode = candidateBuffer->candidatePtr->intraLumaMode;

            if (use4x4ChromaInformation){

                // Initialize chroma CBF
                candidatePtr->cbCbf = 0;
                candidatePtr->crCbf = 0;

                tuTransCoeffTmpPtr = contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr;
                tuQuantCoeffTmpPtr = candidateBuffer->residualQuantCoeffPtr;
                // Cb Transform
                EstimateTransform(
                    &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCb)[puChromaOriginIndex]),
                    candidateBuffer->residualQuantCoeffPtr->strideCb,
                    &(((EB_S16*)tuTransCoeffTmpPtr->bufferCb)[puChromaOriginIndex]),
                    tuTransCoeffTmpPtr->strideCb,
                    candidateBuffer->candidatePtr->transformSize,
                    contextPtr->transformInnerArrayPtr,
                    0,
                    EB_FALSE,//DCT
                    EB_FALSE);

                // Cb Quantize + Cb InvQuantize

                UnifiedQuantizeInvQuantize(
                    contextPtr->encDecContextPtr,
                    lcuPtr->pictureControlSetPtr,
                    &(((EB_S16*)tuTransCoeffTmpPtr->bufferCb)[puChromaOriginIndex]),
                    tuTransCoeffTmpPtr->strideCb,
                    &(((EB_S16*)tuQuantCoeffTmpPtr->bufferCb)[puChromaOriginIndex]),
                    &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[puChromaOriginIndex]),
                    contextPtr->cuPtr->qp,
                    inputPicturePtr->bitDepth,
                    candidateBuffer->candidatePtr->transformSize,
                    pictureControlSetPtr->sliceType,
                    &cbCountNonZeroCoeffs,
                    PF_OFF,
                    0,
                    0,
                    0,
                    0,
                    0,
                    COMPONENT_CHROMA,
                    pictureControlSetPtr->temporalLayerIndex,
                    EB_FALSE,
                    (CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
                    contextPtr->fullLambda,
                    candidateBuffer->candidatePtr->intraLumaMode,
                    EB_INTRA_CHROMA_DM,
                    pictureControlSetPtr->cabacCost);

                // Cr Transform
                EstimateTransform(
                    &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCr)[puChromaOriginIndex]),
                    candidateBuffer->residualQuantCoeffPtr->strideCr,
                    &(((EB_S16*)tuTransCoeffTmpPtr->bufferCr)[puChromaOriginIndex]),
                    tuTransCoeffTmpPtr->strideCr,
                    candidateBuffer->candidatePtr->transformSize,
                    contextPtr->transformInnerArrayPtr,
                    0,
                    EB_FALSE,//DCT
                    EB_FALSE);

                // Cr Quantize + Cr InvQuantize
                UnifiedQuantizeInvQuantize(
                    contextPtr->encDecContextPtr,
                    lcuPtr->pictureControlSetPtr,
                    &(((EB_S16*)tuTransCoeffTmpPtr->bufferCr)[puChromaOriginIndex]),
                    tuTransCoeffTmpPtr->strideCr,
                    &(((EB_S16*)tuQuantCoeffTmpPtr->bufferCr)[puChromaOriginIndex]),
                    &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[puChromaOriginIndex]),
                    contextPtr->cuPtr->qp,
                    inputPicturePtr->bitDepth,
                    candidateBuffer->candidatePtr->transformSize,
                    pictureControlSetPtr->sliceType,
                    &crCountNonZeroCoeffs,
                    PF_OFF,
                    0,
                    0,
                    0,
                    0,
                    0,
                    COMPONENT_CHROMA,
                    pictureControlSetPtr->temporalLayerIndex,
                    EB_FALSE,
                    (CabacEncodeContext_t*)contextPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
                    contextPtr->fullLambda,
                    candidateBuffer->candidatePtr->intraLumaMode,
                    EB_INTRA_CHROMA_DM,
                    pictureControlSetPtr->cabacCost);

                // Initialize luma CBF
                candidatePtr->cbCbf = 0;
                candidatePtr->crCbf = 0;

                transformBuffer = contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr;
                // *Full Distortion (SSE)
                // *Note - there are known issues with how this distortion metric is currently
                //    calculated.  The amount of scaling between the two arrays is not 

                EB_U32 nonZCoef[3];
                EB_U64 zeroDis[2] = { 0 };
                nonZCoef[0] = 0;
                nonZCoef[1] = cbCountNonZeroCoeffs;
                nonZCoef[2] = crCountNonZeroCoeffs;
                PictureFullDistortion_R(
                    transformBuffer,
                    puOriginIndex,
                    puChromaOriginIndex,
                    candidateBuffer->reconCoeffPtr,
                    candidateBuffer->candidatePtr->transformSize,
                    candidateBuffer->candidatePtr->transformChromaSize,
                    PICTURE_BUFFER_DESC_CHROMA_MASK,
                    zeroDis,
                    cbFullDistortion,
                    crFullDistortion,
                    nonZCoef,
                    INTRA_MODE);

                cbCountNonZeroCoeffs = nonZCoef[1];
                crCountNonZeroCoeffs = nonZCoef[2];


                chromaShift = 2 * (7 - Log2f(candidateBuffer->candidatePtr->transformSize));
                // Note: for square Transform, the scale is 1/(2^(7-Log2(Transform size)))
                // For NSQT the scale would be 1/ (2^(7-(Log2(first Transform size)+Log2(second Transform size))/2))
                // Add Log2 of Transform size in order to calculating it multiple time in this function
                cbFullDistortion[DIST_CALC_RESIDUAL] = (cbFullDistortion[DIST_CALC_RESIDUAL] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
                cbFullDistortion[DIST_CALC_PREDICTION] = (cbFullDistortion[DIST_CALC_PREDICTION] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;

                crFullDistortion[DIST_CALC_RESIDUAL] = (crFullDistortion[DIST_CALC_RESIDUAL] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
                crFullDistortion[DIST_CALC_PREDICTION] = (crFullDistortion[DIST_CALC_PREDICTION] + (EB_U64)(1 << (chromaShift - 1))) >> chromaShift;
            }

            /*intra 4x4 Luma*/
            TuEstimateCoeffBitsLuma(
                puOriginIndex,
                contextPtr->coeffEstEntropyCoderPtr,
                candidateBuffer->residualQuantCoeffPtr,
                countNonZeroCoeffs,
                &yCoeffBits,
                MIN_TU_SIZE,
                candidateBuffer->candidatePtr->type,
                intraLumaMode,
                EB_FALSE,
                contextPtr->coeffCabacUpdate,
                &(candidateBuffer->candBuffCoeffCtxModel),
                contextPtr->CabacCost);

            TuCalcCostLuma(
                MIN_CU_SIZE,
                candidatePtr,
                0,
                candidateBuffer->candidatePtr->transformSize,
                countNonZeroCoeffs,
                &(yFullDistortion[DIST_CALC_RESIDUAL]),
                &yCoeffBits,
                contextPtr->qp,
                contextPtr->fullLambda,
                contextPtr->fullChromaLambda);

            if (use4x4ChromaInformation){

                EB_PF_MODE    correctedPFMode = contextPtr->pfMdMode;

                correctedPFMode = PF_OFF;

                TuEstimateCoeffBits_R(
                    puOriginIndex,
                    puChromaOriginIndex,
                    PICTURE_BUFFER_DESC_CHROMA_MASK,
                    contextPtr->coeffEstEntropyCoderPtr,
                    candidateBuffer->residualQuantCoeffPtr,
                    countNonZeroCoeffs,
                    cbCountNonZeroCoeffs,
                    crCountNonZeroCoeffs,
                    &yCoeffBits,
                    &cbCoeffBits,
                    &crCoeffBits,
                    candidateBuffer->candidatePtr->transformSize,
                    candidateBuffer->candidatePtr->transformChromaSize,
                    candidateBuffer->candidatePtr->type,
                    candidateBuffer->candidatePtr->intraLumaMode,
                    EB_INTRA_CHROMA_DM,
                    correctedPFMode,
                    contextPtr->coeffCabacUpdate,
                    &(candidateBuffer->candBuffCoeffCtxModel),
                    contextPtr->CabacCost);

                candidatePtr->cbCbf = ((cbCountNonZeroCoeffs != 0));
                candidatePtr->crCbf = ((crCountNonZeroCoeffs != 0));

            }

            if (contextPtr->spatialSseFullLoop == EB_TRUE) {

                if (candidateBuffer->candidatePtr->yCbf) {

                    EstimateInvTransform(
                        &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[puOriginIndex]),
                        candidateBuffer->reconCoeffPtr->strideY,
                        &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY)[puOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
                        candidateBuffer->candidatePtr->transformSize,
                        contextPtr->transformInnerArrayPtr,
                        BIT_INCREMENT_8BIT,
                        EB_TRUE,
                        EB_FALSE);

                    AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][candidateBuffer->candidatePtr->transformSize >> 3](
                        &(candidateBuffer->predictionPtr->bufferY[puOriginIndex]),
                        candidateBuffer->predictionPtr->strideY,
                        &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY))[puOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
                        &(candidateBuffer->reconPtr->bufferY[puOriginIndex]),
                        candidateBuffer->reconPtr->strideY,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize);
                }
                else {

                    PictureCopy8Bit(
                        candidateBuffer->predictionPtr,
                        puOriginIndex,
                        0,
                        candidateBuffer->reconPtr,
                        puOriginIndex,
                        0,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        0,
                        0,
                        PICTURE_BUFFER_DESC_Y_FLAG);
                }

                yFullDistortion[0] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(MIN_PU_SIZE) - 2](
                    &(inputPicturePtr->bufferY[inputOriginIndex]),
                    inputPicturePtr->strideY,
                    &(candidateBuffer->reconPtr->bufferY[puOriginIndex]),
                    candidateBuffer->reconPtr->strideY,
                    MIN_PU_SIZE,
                    MIN_PU_SIZE);

                if (use4x4ChromaInformation){
                    // Cb

                    if (candidateBuffer->candidatePtr->cbCbf) {

                        EstimateInvTransform(
                            &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[puChromaOriginIndex]),
                            candidateBuffer->reconCoeffPtr->strideCb,
                            &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb)[puChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                            candidateBuffer->candidatePtr->transformSize,
                            contextPtr->transformInnerArrayPtr,
                            BIT_INCREMENT_8BIT,
                            EB_FALSE, // DCT
                            EB_FALSE);

                        AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][candidateBuffer->candidatePtr->transformSize >> 3](
                            &(candidateBuffer->predictionPtr->bufferCb[puChromaOriginIndex]),
                            candidateBuffer->predictionPtr->strideCb,
                            &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb))[puChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                            &(candidateBuffer->reconPtr->bufferCb[puChromaOriginIndex]),
                            candidateBuffer->reconPtr->strideCb,
                            candidateBuffer->candidatePtr->transformSize,
                            candidateBuffer->candidatePtr->transformSize);
                    }
                    else {

                        PictureCopy8Bit(
                            candidateBuffer->predictionPtr,
                            0,
                            puChromaOriginIndex,
                            candidateBuffer->reconPtr,
                            0,
                            puChromaOriginIndex,
                            0,
                            0,
                            candidateBuffer->candidatePtr->transformSize,
                            candidateBuffer->candidatePtr->transformSize,
                            PICTURE_BUFFER_DESC_Cb_FLAG);
                    }

                    cbFullDistortion[0] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(MIN_PU_SIZE) - 2](
                        &(inputPicturePtr->bufferCb[inputChromaOriginIndex]),
                        inputPicturePtr->strideCb,
                        &(candidateBuffer->reconPtr->bufferCb[puChromaOriginIndex]),
                        candidateBuffer->reconPtr->strideCb,
                        MIN_PU_SIZE,
                        MIN_PU_SIZE);

                    if (candidateBuffer->candidatePtr->crCbf) {


                        EstimateInvTransform(
                            &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[puChromaOriginIndex]),
                            candidateBuffer->reconCoeffPtr->strideCr,
                            &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr)[puChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                            candidateBuffer->candidatePtr->transformSize,
                            contextPtr->transformInnerArrayPtr,
                            BIT_INCREMENT_8BIT,
                            EB_FALSE, // DCT
                            EB_FALSE);

                        AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][candidateBuffer->candidatePtr->transformSize >> 3](
                            &(candidateBuffer->predictionPtr->bufferCr[puChromaOriginIndex]),
                            candidateBuffer->predictionPtr->strideCr,
                            &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr))[puChromaOriginIndex]),
                            contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                            &(candidateBuffer->reconPtr->bufferCr[puChromaOriginIndex]),
                            candidateBuffer->reconPtr->strideCr,
                            candidateBuffer->candidatePtr->transformSize,
                            candidateBuffer->candidatePtr->transformSize);
                    }
                    else {

                        PictureCopy8Bit(
                            candidateBuffer->predictionPtr,
                            0,
                            puChromaOriginIndex,
                            candidateBuffer->reconPtr,
                            0,
                            puChromaOriginIndex,
                            0,
                            0,
                            candidateBuffer->candidatePtr->transformSize,
                            candidateBuffer->candidatePtr->transformSize,
                            PICTURE_BUFFER_DESC_Cr_FLAG);
                    }

                    crFullDistortion[0] = SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(MIN_PU_SIZE) - 2](
                        &(inputPicturePtr->bufferCr[inputChromaOriginIndex]),
                        inputPicturePtr->strideCr,
                        &(candidateBuffer->reconPtr->bufferCr[puChromaOriginIndex]),
                        candidateBuffer->reconPtr->strideCr,
                        MIN_PU_SIZE,
                        MIN_PU_SIZE);
                }
            }

            // Full cost calc (SSD of Diff Transform Coeff & Recon Transform Coeff)
            //   plus all available mode bit costs
            // Note : this functions doesn't support different TU sizes within one CU
            //candidateBuffer->candidatePtr->fullLumaCostFuncPtr(

            if (use4x4ChromaInformation){

                IntraNxNFullCostFuncTable[pictureControlSetPtr->sliceType](
                    pictureControlSetPtr,
                    candidateBuffer,
                    contextPtr->cuPtr->qp,
                    yFullDistortion,
                    cbFullDistortion,
                    crFullDistortion,
                    contextPtr->fullLambda,
                    contextPtr->fullChromaLambda,
                    &yCoeffBits,
                    &cbCoeffBits,
                    &crCoeffBits,
                    candidatePtr->transformSize);

                candidatePtr->fullDistortion = (EB_U32)(yFullDistortion[0]) + (EB_U32)(cbFullDistortion[0]) + (EB_U32)(crFullDistortion[0]);
            }
            else{
                Intra4x4FullCostFuncTable[pictureControlSetPtr->sliceType](
                    candidateBuffer,
                    yFullDistortion,
                    contextPtr->fullLambda,
                    &yCoeffBits,
                    candidatePtr->transformSize);

                candidatePtr->fullDistortion = (EB_U32)(yFullDistortion[0]);
            }
        }

        // Full Mode Decision (choose the best mode)
        if (fullCandidateTotalCount < MAX_FULL_LOOP_CANIDATES_PER_DEPTH)
            candidateIndex = Intra4x4FullModeDecision(
                &(intra4x4LumaMode[partitionIndex]),
                &intra4x4Cost,
                &intra4x4FullDistortion,
                candidateBufferPtrArray,
                fullCandidateTotalCount,
                contextPtr->bestCandidateIndexArray);

        // Set Candidate Buffer to the selected mode        
        candidateBuffer = candidateBufferPtrArray[candidateIndex];

        //latest4x4  <-- 4x4CandBuff
        if (contextPtr->coeffCabacUpdate)
            EB_MEMCPY(&(contextPtr->i4x4CoeffCtxModel), &(candidateBuffer->candBuffCoeffCtxModel), sizeof(CoeffCtxtMdl_t));

        // Recon not needed if Open-Loop INTRA, or if spatialSseFullLoop ON (where T-1 performed @ Full-Loop)
        if (contextPtr->intraMdOpenLoopFlag == EB_FALSE && contextPtr->spatialSseFullLoop == EB_FALSE) {

            if (candidateBuffer->candidatePtr->yCbf) {

                EstimateInvTransform(
                    &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferY)[puOriginIndex]),
                    candidateBuffer->reconCoeffPtr->strideY,
                    &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY)[puOriginIndex]),
                    contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
                    candidateBuffer->candidatePtr->transformSize,
                    contextPtr->transformInnerArrayPtr,
                    BIT_INCREMENT_8BIT,
                    EB_TRUE, // DST
                    EB_FALSE);
                AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][candidateBuffer->candidatePtr->transformSize >> 3](
                    &(candidateBuffer->predictionPtr->bufferY[puOriginIndex]),
                    candidateBuffer->predictionPtr->strideY,
                    &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferY))[puOriginIndex]),
                    contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideY,
                    &(candidateBuffer->reconPtr->bufferY[puOriginIndex]),
                    candidateBuffer->reconPtr->strideY,
                    candidateBuffer->candidatePtr->transformSize,
                    candidateBuffer->candidatePtr->transformSize);
            }
            else {

                PictureCopy8Bit(
                    candidateBuffer->predictionPtr,
                    puOriginIndex,
                    0,//tuChromaOriginIndex,
                    candidateBuffer->reconPtr,
                    puOriginIndex,
                    0,//tuChromaOriginIndex,
                    candidateBuffer->candidatePtr->transformSize,
                    candidateBuffer->candidatePtr->transformSize,
                    0,//chromaTuSize,
                    0,//chromaTuSize,
                    PICTURE_BUFFER_DESC_Y_FLAG);
            }

            if (use4x4ChromaInformation) {
                if (candidateBuffer->candidatePtr->cbCbf) {

                    EstimateInvTransform(
                        &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCb)[puChromaOriginIndex]),
                        candidateBuffer->reconCoeffPtr->strideCb,
                        &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb)[puChromaOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                        candidateBuffer->candidatePtr->transformSize,
                        contextPtr->transformInnerArrayPtr,
                        BIT_INCREMENT_8BIT,
                        EB_FALSE,
                        EB_FALSE);

                    PictureAddition(
                        &(candidateBuffer->predictionPtr->bufferCb[puChromaOriginIndex]),
                        candidateBuffer->predictionPtr->strideCb,
                        &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCb))[puChromaOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCb,
                        &(candidateBuffer->reconPtr->bufferCb[puChromaOriginIndex]),
                        candidateBuffer->reconPtr->strideCb,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize);

                }
                else {

                    PictureCopy8Bit(
                        candidateBuffer->predictionPtr,
                        puOriginIndex,
                        puChromaOriginIndex,
                        candidateBuffer->reconPtr,
                        puOriginIndex,
                        puChromaOriginIndex,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        PICTURE_BUFFER_DESC_Cb_FLAG);
                }

                if (candidateBuffer->candidatePtr->crCbf) {

                    EstimateInvTransform(
                        &(((EB_S16*)candidateBuffer->reconCoeffPtr->bufferCr)[puChromaOriginIndex]),
                        candidateBuffer->reconCoeffPtr->strideCr,
                        &(((EB_S16*)contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr)[puChromaOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                        candidateBuffer->candidatePtr->transformSize,
                        contextPtr->transformInnerArrayPtr,
                        BIT_INCREMENT_8BIT,
                        EB_FALSE,
                        EB_FALSE);

                    PictureAddition(
                        &(candidateBuffer->predictionPtr->bufferCr[puChromaOriginIndex]),
                        candidateBuffer->predictionPtr->strideCr,
                        &(((EB_S16*)(contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->bufferCr))[puChromaOriginIndex]),
                        contextPtr->transQuantBuffersPtr->tuTransCoeff2Nx2NPtr->strideCr,
                        &(candidateBuffer->reconPtr->bufferCr[puChromaOriginIndex]),
                        candidateBuffer->reconPtr->strideCr,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize);
                }
                else {

                    PictureCopy8Bit(
                        candidateBuffer->predictionPtr,
                        puOriginIndex,
                        puChromaOriginIndex,
                        candidateBuffer->reconPtr,
                        puOriginIndex,
                        puChromaOriginIndex,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        candidateBuffer->candidatePtr->transformSize,
                        PICTURE_BUFFER_DESC_Cr_FLAG);
                }
            }
        }
		// Update Neighbor Arrays
		EB_U8 predictionModeFlag = (EB_U8)candidateBuffer->candidatePtr->type;
		EB_U8 intraLumaMode = (EB_U8)candidateBuffer->candidatePtr->intraLumaMode;

		NeighborArrayUnitModeTypeWrite(
            contextPtr->modeTypeNeighborArray,
			&predictionModeFlag,
			partitionOriginX,
			partitionOriginY,
			MIN_PU_SIZE);

		NeighborArrayUnitIntraWrite(
            contextPtr->intraLumaModeNeighborArray,
			&intraLumaMode,
			partitionOriginX,
			partitionOriginY,
			MIN_PU_SIZE);

        if (contextPtr->intraMdOpenLoopFlag == EB_FALSE) {

            NeighborArrayUnitSampleWrite(
                contextPtr->lumaReconNeighborArray,
                candidateBuffer->reconPtr->bufferY,
                candidateBuffer->reconPtr->strideY,
                partitionOriginX & (MAX_LCU_SIZE - 1),
                partitionOriginY & (MAX_LCU_SIZE - 1),
                partitionOriginX,
                partitionOriginY,
                MIN_PU_SIZE,
                MIN_PU_SIZE,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

			if (use4x4ChromaInformation) {
				// Recon Samples - Cb
				NeighborArrayUnitSampleWrite(
					contextPtr->cbReconNeighborArray,
					candidateBuffer->reconPtr->bufferCb,
					candidateBuffer->reconPtr->strideCb,
					partitionOriginX & (MAX_LCU_SIZE - 1) >> 1,
					partitionOriginY & (MAX_LCU_SIZE - 1) >> 1,
					partitionOriginX >> 1,
					partitionOriginY >> 1,
					MIN_PU_SIZE,
					MIN_PU_SIZE,
					NEIGHBOR_ARRAY_UNIT_FULL_MASK);

				// Recon Samples - Cr
				NeighborArrayUnitSampleWrite(
					contextPtr->crReconNeighborArray,
					candidateBuffer->reconPtr->bufferCr,
					candidateBuffer->reconPtr->strideCr,
					partitionOriginX & (MAX_LCU_SIZE - 1) >> 1,
					partitionOriginY & (MAX_LCU_SIZE - 1) >> 1,
					partitionOriginX >> 1,
					partitionOriginY >> 1,
					MIN_PU_SIZE,
					MIN_PU_SIZE,
					NEIGHBOR_ARRAY_UNIT_FULL_MASK);
			}

        }

	}

    if (contextPtr->useIntraInterBias){
        ApplyIntraInterModeBias(
            contextPtr->mdPicLcuDetect->intraInterCond1,
            contextPtr->mdPicLcuDetect->intraInterCond2,
            contextPtr->mdPicLcuDetect->intraInterCond3,
            &intra4x4Cost);
    }

	Intra4x4VsIntra8x8(
		contextPtr,
        lcuPtr,
		&(intra4x4LumaMode[0]),
		&intra4x4Cost,
		&intra4x4FullDistortion,
		contextPtr->use4x4ChromaInformationInFullLoop,
		cuPtr);

	return return_error;

}

void ModeDecisionRefinementUpdateNeighborArrays(
    ModeDecisionContext_t   *contextPtr,
	NeighborArrayUnit_t     *modeTypeNeighborArray,
	NeighborArrayUnit_t     *intraLumaModeNeighborArray,
	NeighborArrayUnit_t     *lumaReconSampleNeighborArray,
	EB_BOOL                  intra4x4Selected,
	EB_U8                   *modeType,
	EB_U8                   *lumaMode,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size)

{

	if (intra4x4Selected == EB_FALSE){

		NeighborArrayUnitModeTypeWrite(
			modeTypeNeighborArray,
			(EB_U8*)modeType,
			originX,
			originY,
			size);

		NeighborArrayUnitIntraWrite(
			intraLumaModeNeighborArray,
			(EB_U8*)lumaMode,
			originX,
			originY,
			size);

		// Recon Samples - Luma
		NeighborArrayUnitSampleWrite(
			lumaReconSampleNeighborArray,
			contextPtr->mdReconBuffer->bufferY,
			contextPtr->mdReconBuffer->strideY,
			originX & (MAX_LCU_SIZE - 1),
			originY & (MAX_LCU_SIZE - 1),
			originX,
			originY,
			size,
			size,
			NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        		
	}

	return;
}


void UpdateMdNeighborArrays(
    PictureControlSet_t     *pictureControlSetPtr,
    ModeDecisionContext_t   *contextPtr,
	EbPictureBufferDesc_t   *reconBuffer,
	EB_U32                   lcuSize,
	EB_U8                   *depth,
    EB_U8                   *modeType,
    EB_U8                   *lumaMode,
    MvUnit_t                *mvUnit,
    EB_U8                   *skipFlag,
    EB_U32                   originX,
    EB_U32                   originY,
    EB_U32                   size,
    EB_BOOL                  chromaModeFull)
{

    EB_U16 tileIdx = contextPtr->tileIndex;
    NeighborArrayUnitDepthSkipWrite(
        pictureControlSetPtr->mdLeafDepthNeighborArray[0][tileIdx],
        (EB_U8*)depth,
        originX,
        originY,
        size);

    NeighborArrayUnitModeTypeWrite(
        pictureControlSetPtr->mdModeTypeNeighborArray[0][tileIdx],
        (EB_U8*)modeType,
        originX,
        originY,
        size);

    NeighborArrayUnitIntraWrite(
        pictureControlSetPtr->mdIntraLumaModeNeighborArray[0][tileIdx],
        (EB_U8*)lumaMode,
        originX,
        originY,
        size);


    // *Note - this has to be changed for non-square PU support -- JMJ
    NeighborArrayUnitMvWrite(
        pictureControlSetPtr->mdMvNeighborArray[0][tileIdx],
        (EB_U8*)mvUnit,
        originX,
        originY,
        size);

    NeighborArrayUnitDepthSkipWrite(
        pictureControlSetPtr->mdSkipFlagNeighborArray[0][tileIdx],
        (EB_U8*)skipFlag,
        originX,
        originY,
        size);

    if (contextPtr->intraMdOpenLoopFlag == EB_FALSE){
        // Recon Samples - Luma
        NeighborArrayUnitSampleWrite(
            pictureControlSetPtr->mdLumaReconNeighborArray[0][tileIdx],
            reconBuffer->bufferY,
            reconBuffer->strideY,
            originX & (lcuSize - 1),
            originY & (lcuSize - 1),
            originX,
            originY,
            size,//CHKN width,
            size,//CHKN height,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (chromaModeFull){
            // Recon Samples - Cb
            NeighborArrayUnitSampleWrite(
                pictureControlSetPtr->mdCbReconNeighborArray[0][tileIdx],
                reconBuffer->bufferCb,
                reconBuffer->strideCb,
                (originX & (lcuSize - 1)) >> 1,
                (originY & (lcuSize - 1)) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            // Recon Samples - Cr
            NeighborArrayUnitSampleWrite(
                pictureControlSetPtr->mdCrReconNeighborArray[0][tileIdx],
                reconBuffer->bufferCr,
                reconBuffer->strideCr,
                (originX & (lcuSize - 1)) >> 1,
                (originY & (lcuSize - 1)) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        }
    }

    return;
}


EB_EXTERN EB_ERRORTYPE LinkBdptoMd(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    ModeDecisionContext_t   *contextPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U8 leafIndex = 0;

    while (leafIndex < CU_MAX_COUNT) {

        CodingUnit_t * const cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

        if (cuPtr->splitFlag == EB_FALSE) {

            // Update the CU Stats
            contextPtr->cuStats = GetCodedUnitStats(leafIndex);
            contextPtr->cuOriginX = (EB_U16) lcuPtr->originX + contextPtr->cuStats->originX;
            contextPtr->cuOriginY = (EB_U16) lcuPtr->originY + contextPtr->cuStats->originY;

            // Set the MvUnit
            contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
            contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
            contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

            // Set the candidate buffer
            contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

            EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;

            // intraLumaMode 36 is used to signal INTRA4x4, and when INTRA4x4 is selected intra4x4Mode should be read from intra4x4Mode array 
            // the upper right INTRA4x4 mode (partition index 0) is used to update the intra mode neighbor array
            EB_U8 intraLumaMode = (cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == MIN_CU_SIZE && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4) ?
                (EB_U8)lcuPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[leafIndex] - 21) << 2)] :
                (EB_U8)contextPtr->cuPtr->predictionModeFlag;

            EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

            UpdateMdNeighborArrays(
                pictureControlSetPtr,
                contextPtr,
                contextPtr->mdReconBuffer,
                MAX_LCU_SIZE,
                (EB_U8*)&contextPtr->cuStats->depth,
                &predictionModeFlag,
                &intraLumaMode,
                &contextPtr->mvUnit,
                &skipFlag,
                contextPtr->cuOriginX,
                contextPtr->cuOriginY,
                contextPtr->cuStats->size,
				lcuPtr->chromaEncodeMode == CHROMA_MODE_FULL);


            leafIndex += DepthOffset[contextPtr->cuStats->depth];
        }
        else {
            leafIndex++;
        }
    }

    return return_error;
}

void UpdateBdpNeighborArrays(
    PictureControlSet_t     *pictureControlSetPtr,
    ModeDecisionContext_t   *contextPtr,
	EbPictureBufferDesc_t   *reconBuffer,
	EB_U32                   lcuSize,
	EB_U8                   *depth,
    EB_U8                   *modeType,
    EB_U8                   *lumaMode,
    MvUnit_t                *mvUnit,
    EB_U8                   *skipFlag,
    EB_U32                   originX,
    EB_U32                   originY,
    EB_U32                   size,
    EB_BOOL                  chromaModeFull)
{

    EB_U8 depthIndex;
    EB_U16 tileIdx = contextPtr->tileIndex;

    for (depthIndex = PILLAR_NEIGHBOR_ARRAY_INDEX; depthIndex < NEIGHBOR_ARRAY_TOTAL_COUNT; depthIndex++) {

        NeighborArrayUnitDepthSkipWrite(
            pictureControlSetPtr->mdLeafDepthNeighborArray[depthIndex][tileIdx],
            (EB_U8*)depth,
            originX,
            originY,
            size);

        NeighborArrayUnitModeTypeWrite(
            pictureControlSetPtr->mdModeTypeNeighborArray[depthIndex][tileIdx],
            (EB_U8*)modeType,
            originX,
            originY,
            size);

        NeighborArrayUnitIntraWrite(
            pictureControlSetPtr->mdIntraLumaModeNeighborArray[depthIndex][tileIdx],
            (EB_U8*)lumaMode,
            originX,
            originY,
            size);

        // *Note - this has to be changed for non-square PU support -- JMJ
        NeighborArrayUnitMvWrite(
            pictureControlSetPtr->mdMvNeighborArray[depthIndex][tileIdx],
            (EB_U8*)mvUnit,
            originX,
            originY,
            size);

        NeighborArrayUnitDepthSkipWrite(
            pictureControlSetPtr->mdSkipFlagNeighborArray[depthIndex][tileIdx],
            (EB_U8*)skipFlag,
            originX,
            originY,
            size);

        if (contextPtr->intraMdOpenLoopFlag == EB_FALSE){
            // Recon Samples - Luma
            NeighborArrayUnitSampleWrite(
                pictureControlSetPtr->mdLumaReconNeighborArray[depthIndex][tileIdx],
                reconBuffer->bufferY,
                reconBuffer->strideY,
                originX & (lcuSize - 1),
                originY & (lcuSize - 1),
                originX,
                originY,
                size,//CHKN width,
                size,//CHKN height,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            if (chromaModeFull){
                // Recon Samples - Cb
                NeighborArrayUnitSampleWrite(
                    pictureControlSetPtr->mdCbReconNeighborArray[depthIndex][tileIdx],
                    reconBuffer->bufferCb,
                    reconBuffer->strideCb,
                    (originX & (lcuSize - 1)) >> 1,
                    (originY & (lcuSize - 1)) >> 1,
                    originX >> 1,
                    originY >> 1,
                    size >> 1,
                    size >> 1,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                // Recon Samples - Cr
                NeighborArrayUnitSampleWrite(
                    pictureControlSetPtr->mdCrReconNeighborArray[depthIndex][tileIdx],
                    reconBuffer->bufferCr,
                    reconBuffer->strideCr,
                    (originX & (lcuSize - 1)) >> 1,
                    (originY & (lcuSize - 1)) >> 1,
                    originX >> 1,
                    originY >> 1,
                    size >> 1,
                    size >> 1,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);
            }
        }
    }

    return;
}

EB_EXTERN EB_ERRORTYPE LinkMdtoBdp(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    ModeDecisionContext_t   *contextPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U8 leafIndex = 0;

    while (leafIndex < CU_MAX_COUNT) {

        CodingUnit_t * const cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

        if (cuPtr->splitFlag == EB_FALSE) {

            // Update the CU Stats
            contextPtr->cuStats = GetCodedUnitStats(leafIndex);
            contextPtr->cuOriginX = (EB_U16) lcuPtr->originX + contextPtr->cuStats->originX;
            contextPtr->cuOriginY = (EB_U16) lcuPtr->originY + contextPtr->cuStats->originY;

            // Set the MvUnit
            contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
            contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
            contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

            // Set the candidate buffer
            contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

            EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;

            // intraLumaMode 36 is used to signal INTRA4x4, and when INTRA4x4 is selected intra4x4Mode should be read from intra4x4Mode array 
            // the upper right INTRA4x4 mode (partition index 0) is used to update the intra mode neighbor array
            EB_U8 intraLumaMode = (cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == MIN_CU_SIZE && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4) ?
                (EB_U8)lcuPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[leafIndex] - 21) << 2)] :
                (EB_U8)contextPtr->cuPtr->predictionModeFlag;

            EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

            UpdateBdpNeighborArrays(
                pictureControlSetPtr,
                contextPtr,
                contextPtr->mdReconBuffer,
                MAX_LCU_SIZE,
                (EB_U8*)&contextPtr->cuStats->depth,
                &predictionModeFlag,
                &intraLumaMode,
                &contextPtr->mvUnit,
                &skipFlag,
                contextPtr->cuOriginX,
                contextPtr->cuOriginY,
                contextPtr->cuStats->size,
				lcuPtr->chromaEncodeMode == CHROMA_MODE_FULL);

            leafIndex += DepthOffset[contextPtr->cuStats->depth];
        }
        else {
            leafIndex++;
        }
    }

    return return_error;
}


EB_EXTERN EB_ERRORTYPE ModeDecisionRefinementLcu(
	PictureControlSet_t                 *pictureControlSetPtr,
	LargestCodingUnit_t                 *lcuPtr,
    EB_U32                               lcuOriginX,
    EB_U32                               lcuOriginY,
	ModeDecisionContext_t               *contextPtr)
{
	EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_U16 tileIdx = contextPtr->tileIndex;
    contextPtr->intraLumaModeNeighborArray  = pictureControlSetPtr->mdRefinementIntraLumaModeNeighborArray[tileIdx];
    contextPtr->modeTypeNeighborArray       = pictureControlSetPtr->mdRefinementModeTypeNeighborArray[tileIdx];
    contextPtr->lumaReconNeighborArray      = pictureControlSetPtr->mdRefinementLumaReconNeighborArray[tileIdx];


    contextPtr->lcuPtr = lcuPtr;
    
    EB_U8 leafIndex = 0;

    while (leafIndex < CU_MAX_COUNT) {

        CodingUnit_t * const cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

        if (cuPtr->splitFlag == EB_FALSE) {

            const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(leafIndex);
            
            // Initialize CU info
            contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;
            contextPtr->cuOriginX = (EB_U16) (lcuOriginX + cuStatsPtr->originX);
            contextPtr->cuOriginY = (EB_U16) (lcuOriginY + cuStatsPtr->originY);

            // Initialize reference samples generation DONE signal
            contextPtr->lumaIntraRefSamplesGenDone = EB_FALSE;
            contextPtr->chromaIntraRefSamplesGenDone = EB_FALSE;

            // Generate Split, Skip and intra mode contexts for the rate estimation
            ModeDecisionRefinementContextGeneration(
				contextPtr,
                contextPtr->cuPtr,
                contextPtr->cuOriginX,
                contextPtr->cuOriginY,
                MAX_LCU_SIZE,
                contextPtr->intraLumaModeNeighborArray,
                contextPtr->modeTypeNeighborArray);

            if (cuStatsPtr->size == MIN_CU_SIZE && cuPtr->predictionModeFlag == INTRA_MODE && lcuPtr->intra4x4SearchMethod == INTRA4x4_REFINEMENT_SEARCH) {
                PerformIntra4x4Search(
                    pictureControlSetPtr,
                    lcuPtr,
                    cuPtr,
                    contextPtr,
                    cuPtr->predictionUnitArray[0].intraLumaMode);
            }


            // Update Neighbor Arrays
            EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
            EB_U8 intraLumaMode = (EB_U8)contextPtr->cuPtr->predictionUnitArray[0].intraLumaMode;


            ModeDecisionRefinementUpdateNeighborArrays(
                contextPtr,
                contextPtr->modeTypeNeighborArray,
                contextPtr->intraLumaModeNeighborArray,
                contextPtr->lumaReconNeighborArray,
                (cuPtr->predictionModeFlag == INTRA_MODE && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4),
                &predictionModeFlag,
                &intraLumaMode,
                contextPtr->cuOriginX,
                contextPtr->cuOriginY,
                cuStatsPtr->size);

            leafIndex += DepthOffset[cuStatsPtr->depth];
        }
        else {
            leafIndex++;
        }
    }
    
    return return_error;
}

void UpdateMdReconBuffer(
    EbPictureBufferDesc_t           *reconSrcPtr,
    EbPictureBufferDesc_t           *reconDstPtr,
    ModeDecisionContext_t           *contextPtr,
	LargestCodingUnit_t				*lcuPtr)
{
	if ((contextPtr->cuStats->size >> 3) < 9) {
		PicCopyKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][contextPtr->cuStats->size >> 3](
			&(reconSrcPtr->bufferY[contextPtr->cuStats->originX + contextPtr->cuStats->originY * reconSrcPtr->strideY]),
			reconSrcPtr->strideY,
			&(reconDstPtr->bufferY[contextPtr->cuStats->originX + contextPtr->cuStats->originY * reconDstPtr->strideY]),
			reconDstPtr->strideY,
			contextPtr->cuStats->size,
			contextPtr->cuStats->size);

		if (lcuPtr->chromaEncodeMode == CHROMA_MODE_FULL) {
			EB_U8   chromaSize = contextPtr->cuStats->size >> 1;
			EB_U16  chromaOriginX = contextPtr->cuStats->originX >> 1;
			EB_U16  chromaOriginY = contextPtr->cuStats->originY >> 1;

			PicCopyKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][chromaSize >> 3](
				&(reconSrcPtr->bufferCb[chromaOriginX + chromaOriginY * reconSrcPtr->strideCb]),
				reconSrcPtr->strideCb,
				&(reconDstPtr->bufferCb[chromaOriginX + chromaOriginY * reconDstPtr->strideCb]),
				reconDstPtr->strideCb,
				chromaSize,
				chromaSize);

			PicCopyKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][chromaSize >> 3](
				&(reconSrcPtr->bufferCr[chromaOriginX + chromaOriginY * reconSrcPtr->strideCr]),
				reconSrcPtr->strideCr,
				&(reconDstPtr->bufferCr[chromaOriginX + chromaOriginY * reconDstPtr->strideCr]),
				reconDstPtr->strideCr,
				chromaSize,
				chromaSize);
		}
	}
}

void AddChromaEncDec(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    CodingUnit_t            *cuPtr,
    ModeDecisionContext_t   *contextPtr,
    EncDecContext_t         *contextPtrED,
    EbPictureBufferDesc_t   *inputPicturePtr,
    EB_U32                   inputCbOriginIndex,
    EB_U32					 cuChromaOriginIndex,
    EB_U32                   candIdxInput)
{
    
    EB_U64      yFullDistortion[DIST_CALC_TOTAL];
    EB_U32      countNonZeroCoeffs[3][MAX_NUM_OF_TU_PER_CU];

    EB_U64      cbFullDistortion[DIST_CALC_TOTAL];
    EB_U64      crFullDistortion[DIST_CALC_TOTAL];

    EB_U64      yCoeffBits;
    EB_U64	    cbCoeffBits = 0;
    EB_U64	    crCoeffBits = 0;


    const CodedUnitStats_t                *cuStatsPtr = contextPtrED->cuStats;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);
    ModeDecisionCandidateBuffer_t          *candidateBuffer;
    ModeDecisionCandidate_t                *candidatePtr;    

    // Set the Candidate Buffer  
    candidateBuffer = candidateBufferPtrArray[candIdxInput];
    candidatePtr = candidateBuffer->candidatePtr;

    candidatePtr->type = INTER_MODE;
    candidatePtr->mergeFlag = EB_TRUE;
    candidatePtr->predictionIsReady = EB_FALSE;
   

     PredictionUnit_t       *puPtr = & cuPtr->predictionUnitArray[0];

     candidatePtr->predictionDirection[0] = puPtr->interPredDirectionIndex;
     candidatePtr->mergeIndex             = puPtr->mergeIndex;

     candidatePtr->transformSize       = contextPtr->cuSize == 64 ? 32 : contextPtr->cuSize;
     candidatePtr->transformChromaSize = candidatePtr->transformSize >> 1;


    if (puPtr->interPredDirectionIndex == UNI_PRED_LIST_0)
    {        
         //EB_MEMCPY(&candidatePtr->MVsL0,&puPtr->mv[REF_LIST_0].x,4);
		 candidatePtr->motionVector_x_L0 = puPtr->mv[REF_LIST_0].x;
		 candidatePtr->motionVector_y_L0 = puPtr->mv[REF_LIST_0].y;

	}

	if (puPtr->interPredDirectionIndex == UNI_PRED_LIST_1)
	{
		//EB_MEMCPY(&candidatePtr->MVsL1,&puPtr->mv[REF_LIST_1].x,4);
		candidatePtr->motionVector_x_L1 = puPtr->mv[REF_LIST_1].x;
		candidatePtr->motionVector_y_L1 = puPtr->mv[REF_LIST_1].y;
	}

	if (puPtr->interPredDirectionIndex == BI_PRED)
	{
		candidatePtr->motionVector_x_L0 = puPtr->mv[REF_LIST_0].x;
		candidatePtr->motionVector_y_L0 = puPtr->mv[REF_LIST_0].y;
		candidatePtr->motionVector_x_L1 = puPtr->mv[REF_LIST_1].x;
		candidatePtr->motionVector_y_L1 = puPtr->mv[REF_LIST_1].y;

	}

    // Set PF Mode - should be done per TU (and not per CU) to avoid the correction
	DerivePartialFrequencyN2Flag(
		pictureControlSetPtr,
		contextPtr);

    candidatePtr->yCbf =  contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCbf;

    candidatePtr->mdRateEstimationPtr =  contextPtr->mdRateEstimationPtr;
    candidatePtr->fastLumaRate =  contextPtr->mdEpPipeLcu[cuPtr->leafIndex].fastLumaRate;

    contextPtr->roundMvToInteger = (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == EB_FALSE && candidatePtr->type == INTER_MODE &&  candidatePtr->mergeFlag == EB_TRUE) ?
        EB_TRUE :
        EB_FALSE;

    //restore  Luma information for final cost calc
    yCoeffBits                            = contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits;
    yFullDistortion[DIST_CALC_RESIDUAL]   = contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_RESIDUAL];
    yFullDistortion[DIST_CALC_PREDICTION] = contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_PREDICTION];

    cbFullDistortion[DIST_CALC_RESIDUAL] = 0;
    crFullDistortion[DIST_CALC_RESIDUAL] = 0;
    cbFullDistortion[DIST_CALC_PREDICTION] = 0;
    crFullDistortion[DIST_CALC_PREDICTION] = 0;

    cbCoeffBits = 0;
    crCoeffBits = 0;

    ChromaPrediction(
        pictureControlSetPtr,
        candidateBuffer,
        cuChromaOriginIndex,
        contextPtr);


    PictureResidual(
        &(inputPicturePtr->bufferCb[inputCbOriginIndex]),
        inputPicturePtr->strideCb,
        &(candidateBuffer->predictionPtr->bufferCb[cuChromaOriginIndex]),
        candidateBuffer->predictionPtr->strideCb,
        &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCb)[cuChromaOriginIndex]),
        candidateBuffer->residualQuantCoeffPtr->strideCb,
        contextPtr->cuSize >> 1,
        contextPtr->cuSize >> 1);

    PictureResidual(
        &(inputPicturePtr->bufferCr[inputCbOriginIndex]),
        inputPicturePtr->strideCr,
        &(candidateBuffer->predictionPtr->bufferCr[cuChromaOriginIndex]),
        candidateBuffer->predictionPtr->strideCr,
        &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCr)[cuChromaOriginIndex]),
        candidateBuffer->residualQuantCoeffPtr->strideCr,
        contextPtr->cuSize >> 1,
        contextPtr->cuSize >> 1);

   

    EB_U8 qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
    EB_U8 cbQp = MapChromaQp(qpScaled);

    qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->qp + pictureControlSetPtr->crQpOffset + pictureControlSetPtr->sliceCrQpOffset));
    EB_U8 crQp = MapChromaQp(qpScaled);

    FullLoop_R(
        lcuPtr,
        candidateBuffer,
        contextPtr,
        cuStatsPtr,
        inputPicturePtr,
        pictureControlSetPtr,      
        PICTURE_BUFFER_DESC_CHROMA_MASK,
        cbQp,
        crQp,
        &(*countNonZeroCoeffs[1]),
        &(*countNonZeroCoeffs[2]));

    candidatePtr->cbCbf = 0;
    candidatePtr->crCbf = 0;

    CuFullDistortionFastTuMode_R(
        inputPicturePtr,
        inputCbOriginIndex,
        lcuPtr,
        candidateBuffer,
        contextPtr,
        candidatePtr,
        cuStatsPtr,
        cbFullDistortion,
        crFullDistortion,
        countNonZeroCoeffs,
        PICTURE_BUFFER_DESC_CHROMA_MASK,
        &cbCoeffBits,
        &crCoeffBits);


    candidatePtr->rootCbf = (candidatePtr->yCbf || candidatePtr->cbCbf || candidatePtr->crCbf) ? 1 : 0;

    EB_U32 transChromaformSize = candidatePtr->transformSize >> 1;

    fullCostFuncTable[candidateBuffer->candidatePtr->type][pictureControlSetPtr->sliceType](
        lcuPtr,
        cuPtr,
        contextPtr->cuSize,
        contextPtr->cuSizeLog2,
        candidateBuffer,
        contextPtr->qp,
        yFullDistortion,
        cbFullDistortion,
        crFullDistortion,
        contextPtr->fullLambda,
        contextPtr->fullChromaLambda,
        &yCoeffBits,
        &cbCoeffBits,
        &crCoeffBits,
        candidatePtr->transformSize,
        transChromaformSize,
        pictureControlSetPtr);

	contextPtr->mdEpPipeLcu[cuPtr->leafIndex].mergeCost = *candidateBuffer->fullCostMergePtr;
	contextPtr->mdEpPipeLcu[cuPtr->leafIndex].skipCost = *candidateBuffer->fullCostSkipPtr;

}

static void PerformFullLoop(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    CodingUnit_t            *cuPtr,
    ModeDecisionContext_t   *contextPtr,
    EbPictureBufferDesc_t   *inputPicturePtr,
    EB_U32                   inputOriginIndex,
    EB_U32                   inputCbOriginIndex,
    EB_U32					 cuOriginIndex,
    EB_U32					 cuChromaOriginIndex,
    EB_U32                   fullCandidateTotalCount)
{

	EB_U32      prevRootCbf;
    EB_U64      bestfullCost;
	EB_U32      fullLoopCandidateIndex;
    EB_U8       candidateIndex;

	EB_U64      yFullDistortion[DIST_CALC_TOTAL];
	EB_U32      countNonZeroCoeffs[3][MAX_NUM_OF_TU_PER_CU];

    EB_U64      cbFullDistortion[DIST_CALC_TOTAL];
	EB_U64      crFullDistortion[DIST_CALC_TOTAL];

	EB_U64      yCoeffBits;
    EB_U64	    cbCoeffBits = 0;
	EB_U64	    crCoeffBits = 0;  

    LcuStat_t  *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuPtr->index]);
    const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(contextPtr->cuPtr->leafIndex);

    prevRootCbf = 1;
    bestfullCost = 0xFFFFFFFFull;

	ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray     = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);
    ModeDecisionCandidateBuffer_t          *candidateBuffer;
	ModeDecisionCandidate_t                *candidatePtr;

    for (fullLoopCandidateIndex = 0; fullLoopCandidateIndex < fullCandidateTotalCount; ++fullLoopCandidateIndex) {

        candidateIndex = contextPtr->bestCandidateIndexArray[fullLoopCandidateIndex];

        // initialize TU Split
        yFullDistortion[DIST_CALC_RESIDUAL] = 0;
        yFullDistortion[DIST_CALC_PREDICTION] = 0;
        yCoeffBits = 0;

        // Set the Candidate Buffer      
        candidateBuffer = candidateBufferPtrArray[candidateIndex];
        candidatePtr = candidateBuffer->candidatePtr;//this is the FastCandidateStruct 
        candidatePtr->fullDistortion = 0;

        //CandBuff <-- CU
        if (contextPtr->coeffCabacUpdate)
            EB_MEMCPY(&(candidateBuffer->candBuffCoeffCtxModel), &(contextPtr->latestValidCoeffCtxModel), sizeof(CoeffCtxtMdl_t));

        candidatePtr->chromaDistortion = 0;
        candidatePtr->chromaDistortionInterDepth = 0;
        
        if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
            if (candidatePtr->type == INTRA_MODE && prevRootCbf == 0) {
                continue;
            }
        }
        // Set Skip Flag
        candidatePtr->skipFlag = EB_FALSE;

        // Do Prediction in the case it is not done in the fast Loop.
        if (candidatePtr->predictionIsReadyLuma == EB_FALSE) {
            PerformIntraPrediction(
                pictureControlSetPtr,
                contextPtr,
                candidateBuffer,
                candidatePtr);
        }

        candidatePtr->chromaDistortion = 0;
        candidatePtr->chromaDistortionInterDepth = 0;

        //Y Residual
        PictureResidual(
            &(inputPicturePtr->bufferY[inputOriginIndex]),
            inputPicturePtr->strideY,
            &(candidateBuffer->predictionPtr->bufferY[cuOriginIndex]),
            64,
            &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferY)[cuOriginIndex]),
            candidateBuffer->residualQuantCoeffPtr->strideY,
            contextPtr->cuStats->size,
            contextPtr->cuStats->size);

        // initialize luma CBF
        candidatePtr->yCbf = 0;

        ProductFullLoop(
            inputPicturePtr,
            inputOriginIndex,
            candidateBuffer,
            contextPtr,
            cuStatsPtr,
            pictureControlSetPtr,
            contextPtr->cuPtr->qp,
            &(*countNonZeroCoeffs[0]),
            &yCoeffBits,
            &yFullDistortion[0]);

        {
            candidatePtr->rootCbf = (candidatePtr->yCbf) ? 1 : 0;
            // exclude chroma from cost calculation
            candidatePtr->cbCbf = 1;
            candidatePtr->crCbf = 1;
        }

        if (cuStatsPtr->size == 32 && pictureControlSetPtr->sliceType == EB_I_PICTURE)
            candidatePtr->countNonZeroCoeffs = (EB_U16)countNonZeroCoeffs[0][0];
        else
            candidatePtr->countNonZeroCoeffs = 0;

        candidatePtr->chromaDistortionInterDepth = 0;
        candidatePtr->chromaDistortion = 0;

        if (contextPtr->useChromaInformationInFullLoop) {


            cbFullDistortion[DIST_CALC_RESIDUAL] = 0;
            crFullDistortion[DIST_CALC_RESIDUAL] = 0;
            cbFullDistortion[DIST_CALC_PREDICTION] = 0;
            crFullDistortion[DIST_CALC_PREDICTION] = 0;

            cbCoeffBits = 0;
            crCoeffBits = 0;

            //do the prediction here
            ChromaPrediction(
                pictureControlSetPtr,
                candidateBuffer,
                cuChromaOriginIndex,
                contextPtr);

            PictureResidual(
                &(inputPicturePtr->bufferCb[inputCbOriginIndex]),
                inputPicturePtr->strideCb,
                &(candidateBuffer->predictionPtr->bufferCb[cuChromaOriginIndex]),
                candidateBuffer->predictionPtr->strideCb,
                &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCb)[cuChromaOriginIndex]),
                candidateBuffer->residualQuantCoeffPtr->strideCb,
                contextPtr->cuSize >> 1,
                contextPtr->cuSize >> 1);

            PictureResidual(
                &(inputPicturePtr->bufferCr[inputCbOriginIndex]),
                inputPicturePtr->strideCr,
                &(candidateBuffer->predictionPtr->bufferCr[cuChromaOriginIndex]),
                candidateBuffer->predictionPtr->strideCr,
                &(((EB_S16*)candidateBuffer->residualQuantCoeffPtr->bufferCr)[cuChromaOriginIndex]),
                candidateBuffer->residualQuantCoeffPtr->strideCr,
                contextPtr->cuSize >> 1,
                contextPtr->cuSize >> 1);

            // FullLoop and TU search
            EB_U8 qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->cuPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
            EB_U8 cbQp = MapChromaQp(qpScaled);

            qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->cuPtr->qp + pictureControlSetPtr->crQpOffset + pictureControlSetPtr->sliceCrQpOffset));
            EB_U8 crQp = MapChromaQp(qpScaled);


            FullLoop_R(
                lcuPtr,
                candidateBuffer,
                contextPtr,
                cuStatsPtr,
                inputPicturePtr,
                pictureControlSetPtr,
                //CHKN contextPtr->useChromaInformationInFullLoop ? PICTURE_BUFFER_DESC_FULL_MASK : PICTURE_BUFFER_DESC_LUMA_MASK,
                PICTURE_BUFFER_DESC_CHROMA_MASK,
                cbQp,
                crQp,
                &(*countNonZeroCoeffs[1]),
                &(*countNonZeroCoeffs[2]));

            candidatePtr->cbCbf = 0;
            candidatePtr->crCbf = 0;

            CuFullDistortionFastTuMode_R(
                inputPicturePtr,
                inputCbOriginIndex,
                lcuPtr,
                candidateBuffer,
                contextPtr,
                candidatePtr,
                cuStatsPtr,
                cbFullDistortion,
                crFullDistortion,
                countNonZeroCoeffs,
                PICTURE_BUFFER_DESC_CHROMA_MASK,
                &cbCoeffBits,
                &crCoeffBits);

            EB_U32 N4_TH[NUM_QPS] = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
            EB_U32 N2_TH[NUM_QPS] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

            //			EB_U32 tuSize = candidatePtr->transformSize;
            //			EB_U32 chromatTuSize = candidatePtr->transformChromaSize;
            EB_U32 correctedPFMode = contextPtr->pfMdMode;

            EB_U8 Weight;
            Weight = pictureControlSetPtr->temporalLayerIndex == 0 || pictureControlSetPtr->highIntraSlection || pictureControlSetPtr->ParentPcsPtr->lcuBlockPercentage > 40 || pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuPtr->index].isolatedHighIntensityLcu ? 1 : pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag && pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuPtr->index].edgeBlockNum ? 3 : pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuPtr->index].edgeBlockNum ? 2 : 1;
            if (correctedPFMode == PF_N4) {
                yCoeffBits *= Weight*N4_TH[contextPtr->cuPtr->qp];
            }
            else if (correctedPFMode == PF_N2) {
                yCoeffBits *= N2_TH[contextPtr->cuPtr->qp];
            }


            candidatePtr->rootCbf = (candidatePtr->yCbf || candidatePtr->cbCbf || candidatePtr->crCbf) ? 1 : 0;

            EB_U32 transChromaformSize = candidatePtr->transformSize >> 1;

            fullCostFuncTable[candidateBuffer->candidatePtr->type][pictureControlSetPtr->sliceType](
                lcuPtr,
                cuPtr,
                contextPtr->cuSize,
                contextPtr->cuSizeLog2,
                candidateBuffer,
                contextPtr->cuPtr->qp,
                yFullDistortion,
                cbFullDistortion,
                crFullDistortion,
                contextPtr->fullLambda,
                contextPtr->fullChromaLambda,
                &yCoeffBits,
                &cbCoeffBits,
                &crCoeffBits,
                candidatePtr->transformSize,
                transChromaformSize,
                pictureControlSetPtr);

            candidateBuffer->cbDistortion[DIST_CALC_RESIDUAL] = cbFullDistortion[DIST_CALC_RESIDUAL];
            candidateBuffer->cbDistortion[DIST_CALC_PREDICTION] = cbFullDistortion[DIST_CALC_PREDICTION];
            candidateBuffer->cbCoeffBits = cbCoeffBits;

            candidateBuffer->crDistortion[DIST_CALC_RESIDUAL] = crFullDistortion[DIST_CALC_RESIDUAL];
            candidateBuffer->crDistortion[DIST_CALC_PREDICTION] = crFullDistortion[DIST_CALC_PREDICTION];
            candidateBuffer->crCoeffBits = crCoeffBits;
            candidateBuffer->candidatePtr->fullDistortion = (EB_U32)(yFullDistortion[0]);
            candidateBuffer->candidatePtr->lumaDistortion = (EB_U32)(yFullDistortion[0]);

        }
        else {
            if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST/*Add MODE1 here too?*/)
            {
                EB_U32 N4_TH[NUM_QPS] = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
                EB_U32 N2_TH[NUM_QPS] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

                //			EB_U32 tuSize = candidatePtr->transformSize;
                //			EB_U32 chromatTuSize = candidatePtr->transformChromaSize;
                EB_U32 correctedPFMode = contextPtr->pfMdMode;

                EB_U8 Weight;
                Weight = pictureControlSetPtr->temporalLayerIndex == 0 || pictureControlSetPtr->highIntraSlection || pictureControlSetPtr->ParentPcsPtr->lcuBlockPercentage > 40 || pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuPtr->index].isolatedHighIntensityLcu ? 1 : pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag && pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuPtr->index].edgeBlockNum ? 3 : pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuPtr->index].edgeBlockNum ? 2 : 1;
                if (correctedPFMode == PF_N4) {
                    yCoeffBits *= Weight*N4_TH[contextPtr->cuPtr->qp];
                }
                else if (correctedPFMode == PF_N2) {
                    yCoeffBits *= N2_TH[contextPtr->cuPtr->qp];
                }
            }

            // Full cost calc (SSD of Diff Transform Coeff & Recon Transform Coeff)
            //   plus all available mode bit costs
            // Note : this functions doesn't support different TU sizes within one CU
            //candidateBuffer->candidatePtr->fullLumaCostFuncPtr(
            {
                ProductFullLumaCostFuncTable[candidatePtr->type][pictureControlSetPtr->sliceType](
                    cuPtr,
                    contextPtr->cuStats->size,
                    contextPtr->cuSizeLog2,
                    candidateBuffer,
                    yFullDistortion,
                    contextPtr->fullLambda,
                    &yCoeffBits,
                    candidatePtr->transformSize);
            }
            candidateBuffer->fullCostLuma = *candidateBuffer->fullCostPtr;
        }
        candidateBuffer->yCoeffBits = yCoeffBits;
        if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST)
        {
            candidateBuffer->yCoeffBits = yCoeffBits;
            candidateBuffer->yFullDistortion[DIST_CALC_RESIDUAL] = yFullDistortion[DIST_CALC_RESIDUAL];
            candidateBuffer->yFullDistortion[DIST_CALC_PREDICTION] = yFullDistortion[DIST_CALC_PREDICTION];
        }


        candidatePtr->fullDistortion = (EB_U32)(yFullDistortion[0]);

        if (contextPtr->useIntraInterBias) {
            if (candidatePtr->type == INTRA_MODE)
                ApplyIntraInterModeBias(
                    contextPtr->mdPicLcuDetect->intraInterCond1,
                    contextPtr->mdPicLcuDetect->intraInterCond2,
                    contextPtr->mdPicLcuDetect->intraInterCond3,
                    candidateBuffer->fullCostPtr);



            if (pictureControlSetPtr->sliceType != EB_I_PICTURE && pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 90) {
                if (pictureControlSetPtr->temporalLayerIndex <= 1) {
                    if (candidatePtr->type == INTER_MODE && (EB_U32)candidatePtr->predictionDirection[0] == BI_PRED) {

                        *candidateBuffer->fullCostPtr += lcuPtr->pictureControlSetPtr->ParentPcsPtr->lcuHomogeneousAreaArray[lcuPtr->index] || lcuStatPtr->cuStatArray[cuPtr->leafIndex].grassArea ? (*candidateBuffer->fullCostPtr * 20) / 100 : 0;


                    }
                }
            }
        }

		if (contextPtr->fullLoopEscape)

            if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
                if (candidatePtr->type == INTER_MODE) {
                    if (*candidateBuffer->fullCostPtr < bestfullCost) {
                        prevRootCbf = candidatePtr->yCbf;
                        bestfullCost = *candidateBuffer->fullCostPtr;
                    }
                }
            }


    }//end for( full loop)
}


/*******************************************
* ModeDecision LCU
*   performs CL (LCU)
*******************************************/
EB_EXTERN EB_ERRORTYPE ModeDecisionLcu(
	SequenceControlSet_t                *sequenceControlSetPtr,
	PictureControlSet_t                 *pictureControlSetPtr,
	const MdcLcuData_t * const           mdcResultTbPtr,
	LargestCodingUnit_t                 *lcuPtr,
	EB_U16                               lcuOriginX,
	EB_U16                               lcuOriginY,
	EB_U32                               lcuAddr,
	ModeDecisionContext_t               *contextPtr)
{
	EB_ERRORTYPE                            return_error = EB_ErrorNone;

	// CU                                   
	EB_U32                                  cuIdx;

	// Input  
    EbPictureBufferDesc_t                  *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->chromaDownSamplePicturePtr;


    EB_U16 tileIdx = contextPtr->tileIndex;

	// Mode Decision Candidate Buffers
	EB_U32                                  bufferTotalCount;
	ModeDecisionCandidateBuffer_t          *candidateBuffer;
	ModeDecisionCandidateBuffer_t          *bestCandidateBuffers[EB_MAX_LCU_DEPTH];

	// Mode Decision Search Candidates 
	EB_U8                                   candidateIndex;
	EB_U32                                  fastCandidateTotalCount;
	EB_U32                                  fullCandidateTotalCount;

	EB_U32                                  secondFastCostSearchCandidateTotalCount;

	// CTB merge                          
	EB_U32                                  lastCuIndex;

	// Pre Intra Search                   
	const EB_U32                            lcuHeight = MIN(MAX_LCU_SIZE, (EB_U32)(sequenceControlSetPtr->lumaHeight - lcuOriginY));
	const EB_PICTURE                          sliceType = pictureControlSetPtr->sliceType;
	const EB_U32                            leafCount = mdcResultTbPtr->leafCount;
	const EbMdcLeafData_t *const            leafDataArray = mdcResultTbPtr->leafDataArray;
	ModeDecisionCandidate_t                *fastCandidateArray = contextPtr->fastCandidateArray;
	ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
	EB_U32							        bestIntraMode = EB_INTRA_MODE_INVALID;
	ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray;

	EB_U32                                  maxBuffers;

	LcuParams_t                            *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuAddr];

    // enableExitPartitioning derivation @ ModeDecisionLcu()
    EB_BOOL enableExitPartitioning;
    if (contextPtr->enableExitPartitioning && (
        pictureControlSetPtr->ParentPcsPtr->depthMode >= PICT_OPEN_LOOP_DEPTH_MODE ||
       (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_OPEN_LOOP_DEPTH_MODE ||
        pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_LIGHT_OPEN_LOOP_DEPTH_MODE ||
        pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_PRED_OPEN_LOOP_DEPTH_MODE ||
        pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE)))) {
        enableExitPartitioning = EB_TRUE;
    }
    else {
        enableExitPartitioning = EB_FALSE;
    }

    // Keep track of the LCU Ptr
	contextPtr->lcuPtr                  = lcuPtr;
    
	contextPtr->groupOf8x8BlocksCount   = 0;
	contextPtr->groupOf16x16BlocksCount = 0;

	ConstructMdCuArray(
		contextPtr,
		sequenceControlSetPtr,
		lcuPtr,
		mdcResultTbPtr);

    // Mode Decision Neighbor Arrays
    contextPtr->intraLumaModeNeighborArray  = pictureControlSetPtr->mdIntraLumaModeNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->mvNeighborArray             = pictureControlSetPtr->mdMvNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->skipFlagNeighborArray       = pictureControlSetPtr->mdSkipFlagNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->modeTypeNeighborArray       = pictureControlSetPtr->mdModeTypeNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->leafDepthNeighborArray      = pictureControlSetPtr->mdLeafDepthNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->lumaReconNeighborArray      = pictureControlSetPtr->mdLumaReconNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->cbReconNeighborArray        = pictureControlSetPtr->mdCbReconNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->crReconNeighborArray        = pictureControlSetPtr->mdCrReconNeighborArray[MD_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->edgeBlockNumFlag = (EB_BOOL)pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum;
    // First CU Loop  
	cuIdx = 0;
	do
	{
		// Assign CU data
		const EB_U8 leafIndex = leafDataArray[cuIdx].leafIndex;

		CodingUnit_t *  cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

		const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(leafIndex);
        contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;


        contextPtr->cuOriginX = lcuOriginX + cuStatsPtr->originX;
		contextPtr->cuOriginY = lcuOriginY + cuStatsPtr->originY;
		const EB_U32 inputOriginIndex = (contextPtr->cuOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (contextPtr->cuOriginX + inputPicturePtr->originX);
		const EB_U32 inputCbOriginIndex = ((contextPtr->cuOriginY>>1) + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCb + ((contextPtr->cuOriginX >>1) + (inputPicturePtr->originX >> 1));
		const EB_U32 cuOriginIndex       = ((contextPtr->cuOriginY & 63)  * 64) + (contextPtr->cuOriginX & 63);
		const EB_U32 cuChromaOriginIndex = (((contextPtr->cuOriginY & 63) * 32) + (contextPtr->cuOriginX & 63)) >> 1;
		const EbMdcLeafData_t * const leafDataPtr = &mdcResultTbPtr->leafDataArray[cuIdx];

        //need these for L7 Chroma: TBCleaned.
        contextPtr->cuDepth = cuStatsPtr->depth;
        contextPtr->cuSize  = cuStatsPtr->size;
        contextPtr->lcuSize = MAX_LCU_SIZE;
        contextPtr->lcuChromaSize = contextPtr->lcuSize >> 1;
        contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;
        contextPtr->cuChromaOriginX = contextPtr->cuOriginX >> 1;
		contextPtr->cuChromaOriginY = contextPtr->cuOriginY >> 1;
		contextPtr->mdLocalCuUnit[leafIndex].testedCuFlag = EB_TRUE;
		cuPtr->leafIndex = leafIndex;
		cuPtr->splitFlag = (EB_U16)(
            (sliceType == EB_I_PICTURE) && (cuStatsPtr->depth == 0) ? EB_TRUE :
            leafDataPtr->splitFlag);		
        cuPtr->qp = contextPtr->qp;

        candidateBufferPtrArray = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);

        // Set PF Mode - should be done per TU (and not per CU) to avoid the correction
		DerivePartialFrequencyN2Flag(
			pictureControlSetPtr,
			contextPtr);

		// Initialize Fast Loop
		ProductCodingLoopInitFastLoop( // HT to be rechecked especially for fullCostArray
			contextPtr,
            contextPtr->intraLumaModeNeighborArray,
		    contextPtr->skipFlagNeighborArray,
		    contextPtr->modeTypeNeighborArray,
		    contextPtr->leafDepthNeighborArray);

        if (
            (pictureControlSetPtr->ParentPcsPtr->depthMode >= PICT_OPEN_LOOP_DEPTH_MODE || 
            (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_LIGHT_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_AVC_DEPTH_MODE))) && 
             lcuParams->isCompleteLcu
            ){

            SkipSmallCu(
                pictureControlSetPtr,
                contextPtr,
                cuPtr);
        }


        // Perform MPM search
        if (contextPtr->mpmSearch) {
            DeriveMpmModes( 
                contextPtr,
                cuPtr);
        }

        SetNmm(
            contextPtr,
            MDC_STAGE);

        SetNfl(
            lcuPtr,
            contextPtr,
            pictureControlSetPtr,
            MDC_STAGE);

        contextPtr->conformantMvMergeTable = EB_TRUE;

        ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
            lcuPtr,
            contextPtr,
			leafIndex,
            lcuAddr,
            &bufferTotalCount,
            &fastCandidateTotalCount,
            (void*)contextPtr->interPredictionContext,
            pictureControlSetPtr,
            contextPtr->mpmSearch,
            contextPtr->mpmSearchCandidate,
            contextPtr->mostProbableModeArray);

        //if we want to recon N candidate, we would need N+1 buffers
		maxBuffers = MIN((bufferTotalCount + 1), contextPtr->bufferDepthIndexWidth[cuStatsPtr->depth]);

		ProductPerformFastLoop(
			pictureControlSetPtr,
			lcuPtr,
			contextPtr,
			candidateBufferPtrArrayBase,
			fastCandidateArray,
			fastCandidateTotalCount,
			inputPicturePtr,
			inputOriginIndex,
			inputCbOriginIndex,
            inputCbOriginIndex,
            cuPtr,
			cuOriginIndex,
			cuChromaOriginIndex,
			maxBuffers,
			&secondFastCostSearchCandidateTotalCount);

		// Make sure bufferTotalCount is not larger than the number of fast modes
		bufferTotalCount = MIN(secondFastCostSearchCandidateTotalCount, bufferTotalCount);

		// PreModeDecision
		// -Input is the buffers
		// -Output is list of buffers for full reconstruction
		EB_U8  disableMergeIndex = 0;


        PreModeDecision(
            cuPtr,
            (secondFastCostSearchCandidateTotalCount == bufferTotalCount) ? bufferTotalCount : maxBuffers,
            candidateBufferPtrArray,
            &fullCandidateTotalCount,
            contextPtr->bestCandidateIndexArray,
			&disableMergeIndex,
            (EB_BOOL)(secondFastCostSearchCandidateTotalCount == bufferTotalCount)); // The fast loop bug fix is now added to 4K only

        PerformFullLoop(
            pictureControlSetPtr,
            lcuPtr,
            cuPtr,
            contextPtr,
            inputPicturePtr,
            inputOriginIndex,
            inputCbOriginIndex,
            cuOriginIndex,
            cuChromaOriginIndex,
            MIN(fullCandidateTotalCount, bufferTotalCount)); // fullCandidateTotalCount to number of buffers to process  

		// Full Mode Decision (choose the best mode)
		candidateIndex = ProductFullModeDecision(
            contextPtr,
			cuPtr,
            contextPtr->cuStats->size,
			candidateBufferPtrArray,
			fullCandidateTotalCount,
			contextPtr->bestCandidateIndexArray,
			&bestIntraMode);

		// Set Candidate Buffer to the selected mode        
		// If Intra 4x4 is selected then candidateBuffer for depth 3 is not going to get used
		// No MD-Recon: iTransform Loop + Recon, and no lumaReconSampleNeighborArray update

        if (lcuPtr->intra4x4SearchMethod == INTRA4x4_INLINE_SEARCH)
        {

            if (cuStatsPtr->size == MIN_CU_SIZE && cuPtr->predictionModeFlag == INTRA_MODE) {
                PerformIntra4x4Search(
                    pictureControlSetPtr,
                    lcuPtr,
                    cuPtr,
                    contextPtr,
                    cuPtr->predictionUnitArray[0].intraLumaMode);

                if (contextPtr->coeffCabacUpdate && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4)
                    EB_MEMCPY(&(candidateBufferPtrArray[candidateIndex]->candBuffCoeffCtxModel), &(contextPtr->i4x4CoeffCtxModel), sizeof(CoeffCtxtMdl_t));

            }
        }


		candidateBuffer = candidateBufferPtrArray[candidateIndex];
		
        bestCandidateBuffers[contextPtr->cuStats->depth] = candidateBuffer;
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                = candidateBuffer->yCoeffBits;
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[0]                    = candidateBuffer->yDc[0];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[1]                    = candidateBuffer->yDc[1];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[2]                    = candidateBuffer->yDc[2];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[3]                    = candidateBuffer->yDc[3];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[0]    = candidateBuffer->yCountNonZeroCoeffs[0];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[1]    = candidateBuffer->yCountNonZeroCoeffs[1];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[2]    = candidateBuffer->yCountNonZeroCoeffs[2];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[3]    = candidateBuffer->yCountNonZeroCoeffs[3];

	   if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST && candidateBuffer->candidatePtr->type == INTER_MODE && candidateBuffer->candidatePtr->mergeFlag == EB_TRUE)
       {
             contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                            = candidateBuffer->yCoeffBits;
             contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_RESIDUAL]   = candidateBuffer->yFullDistortion[DIST_CALC_RESIDUAL];
             contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_PREDICTION] = candidateBuffer->yFullDistortion[DIST_CALC_PREDICTION];
             contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCbf                                  = candidateBuffer->candidatePtr->yCbf ;
             contextPtr->mdEpPipeLcu[cuPtr->leafIndex].fastLumaRate                          = candidateBuffer->candidatePtr->fastLumaRate;
       }


	   EB_U8    parentLeafIndex;

	   EB_BOOL  exitPartition = EB_FALSE;
       contextPtr->mdLocalCuUnit[leafIndex].mdcArrayIndex = (EB_U8)cuIdx;

        CheckHighCostPartition(
            sequenceControlSetPtr,
            contextPtr,
            lcuPtr,
            leafIndex,
            &parentLeafIndex,
            enableExitPartitioning,
            &exitPartition);

	   if (exitPartition)
	   {
		   //Reset the CU-Loop to the parent CU.
		   cuStatsPtr = GetCodedUnitStats(parentLeafIndex);
		   cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[parentLeafIndex];
		   cuIdx = contextPtr->mdLocalCuUnit[parentLeafIndex].mdcArrayIndex;
		   cuPtr->splitFlag = EB_FALSE;

		   lastCuIndex = ExitInterDepthDecision(
			   contextPtr,
			   parentLeafIndex,
			   lcuPtr,
			   lcuAddr,
			   lcuOriginX,
			   lcuOriginY,
			   contextPtr->fullLambda,
			   contextPtr->mdRateEstimationPtr,
			   pictureControlSetPtr);

	   }
       else {

           PerformInverseTransformRecon(
               contextPtr,
               candidateBuffer,
               cuPtr,
               contextPtr->cuStats);


           // Inter Depth Decision
           lastCuIndex = ProductPerformInterDepthDecision(
               contextPtr,
               leafIndex,
               lcuPtr,
               lcuAddr,
               lcuOriginX,
               lcuOriginY,
               contextPtr->fullLambda,
               contextPtr->mdRateEstimationPtr,
               pictureControlSetPtr);


       }


		if (lcuPtr->codedLeafArrayPtr[lastCuIndex]->splitFlag == EB_FALSE)
		{
			// Update the CU Stats
			contextPtr->cuStats = GetCodedUnitStats(lastCuIndex);
			contextPtr->cuOriginX = lcuOriginX + contextPtr->cuStats->originX;
			contextPtr->cuOriginY = lcuOriginY + contextPtr->cuStats->originY;

			// Set the MvUnit
			contextPtr->mvUnit.predDirection   = (EB_U8)(&lcuPtr->codedLeafArrayPtr[lastCuIndex]->predictionUnitArray[0])->interPredDirectionIndex;
			contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[lastCuIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
			contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[lastCuIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

			// Set the candidate buffer
			contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[lastCuIndex];
			candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];

            if (contextPtr->coeffCabacUpdate)
                EB_MEMCPY(&(contextPtr->latestValidCoeffCtxModel), &(candidateBuffer->candBuffCoeffCtxModel), sizeof(CoeffCtxtMdl_t));

			// Update Neighbor Arrays
			{
				EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
				EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
				EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

				ModeDecisionUpdateNeighborArrays(
					contextPtr->leafDepthNeighborArray,
					contextPtr->modeTypeNeighborArray,
					contextPtr->intraLumaModeNeighborArray,
					contextPtr->mvNeighborArray,
					contextPtr->skipFlagNeighborArray,
					contextPtr->lumaReconNeighborArray,
					contextPtr->cbReconNeighborArray,
					contextPtr->crReconNeighborArray,
					candidateBuffer->reconPtr,
                    MAX_LCU_SIZE,
                    contextPtr->intraMdOpenLoopFlag,				
                    (cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == MIN_CU_SIZE && cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4),
                    (EB_U8*)&contextPtr->cuStats->depth,
					&predictionModeFlag,
					&intraLumaMode,
					&contextPtr->mvUnit,
					&skipFlag,
					contextPtr->cuOriginX,
					contextPtr->cuOriginY,
					contextPtr->cuStats->size,
                    contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);

			}

			if (contextPtr->intraMdOpenLoopFlag == EB_FALSE)
			{
				UpdateMdReconBuffer(
					candidateBuffer->reconPtr,
					contextPtr->mdReconBuffer,
					contextPtr,
					lcuPtr);
			}

		}
	
		if (cuPtr->splitFlag == EB_TRUE){
			cuIdx++;
		}
		else if ( lcuHeight < MAX_LCU_SIZE) {
			cuIdx++;
		}
		else{
			cuIdx += CalculateNextCuIndex(
				leafDataArray,
				cuIdx,
				leafCount,
                cuStatsPtr->depth);
        }

	} while (cuIdx < leafCount);// End of CU loop
	return return_error;
}

void ConstructPillarCuArray(
    PictureControlSet_t   *pictureControlSetPtr,
    LargestCodingUnit_t   *lcuPtr,
    LcuParams_t           *lcuParamPtr,
    ModeDecisionContext_t *contextPtr)
{
    contextPtr->pillarCuArray.leafCount = 0;

    EB_U8 cuIndex = 0;
    EB_BOOL splitFlag;

    while (cuIndex < CU_MAX_COUNT) {

        const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(cuIndex);

        CodingUnit_t * const cuPtr = lcuPtr->codedLeafArrayPtr[cuIndex];
        splitFlag           = EB_TRUE;
        cuPtr->splitFlag    = EB_TRUE;	

		contextPtr->mdLocalCuUnit[cuIndex].testedCuFlag = EB_FALSE;
        if (lcuParamPtr->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
        {

            if (cuStatsPtr->depth == 0) {
                splitFlag            = EB_TRUE;
				cuPtr->splitFlag = EB_TRUE;
				contextPtr->mdLocalCuUnit[cuIndex].testedCuFlag = EB_FALSE;
            }
            else if (cuStatsPtr->depth == 1 && contextPtr->depthRefinment < 2 && pictureControlSetPtr->ParentPcsPtr->complexLcuArray[lcuPtr->index] != LCU_COMPLEXITY_STATUS_2) {
                    splitFlag            = EB_TRUE;
					cuPtr->splitFlag = EB_TRUE;
					contextPtr->mdLocalCuUnit[cuIndex].testedCuFlag = EB_FALSE;
                    contextPtr->pillarCuArray.leafDataArray[contextPtr->pillarCuArray.leafCount] = cuIndex;
                    contextPtr->pillarCuArray.leafCount++;

            }
            else if (cuStatsPtr->depth == 2) {
                    
                splitFlag           = EB_FALSE;
				cuPtr->splitFlag = EB_FALSE;
				contextPtr->mdLocalCuUnit[cuIndex].testedCuFlag = EB_FALSE;
                contextPtr->pillarCuArray.leafDataArray[contextPtr->pillarCuArray.leafCount] = cuIndex;
                contextPtr->pillarCuArray.leafCount++;

            }
            else if (cuStatsPtr->depth == 3) {
                splitFlag           = EB_FALSE;
				cuPtr->splitFlag = EB_FALSE;
				contextPtr->mdLocalCuUnit[cuIndex].testedCuFlag = EB_FALSE;
                contextPtr->pillarCuArray.leafDataArray[contextPtr->pillarCuArray.leafCount] = cuIndex;
                contextPtr->pillarCuArray.leafCount++;

            }
        } // End CU Loop

        cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

    }
}


EB_EXTERN EB_ERRORTYPE BdpPillar(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr, 
    LcuParams_t                         *lcuParamPtr, 
    LargestCodingUnit_t                 *lcuPtr,
	EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr)
{
	EB_ERRORTYPE                            return_error = EB_ErrorNone;
                                            
	// CU                                   
	EB_U32                                  cuIdx;

	// Input   
    EbPictureBufferDesc_t                  *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->chromaDownSamplePicturePtr;
    EB_U16                                  tileIdx = lcuPtr->lcuEdgeInfoPtr->tileIndexInRaster;


	// Mode Decision Candidate Buffers
	EB_U32                                  bufferTotalCount;
	ModeDecisionCandidateBuffer_t          *candidateBuffer;
	ModeDecisionCandidateBuffer_t          *bestCandidateBuffers[EB_MAX_LCU_DEPTH];

	// Mode Decision Search Candidates 
	EB_U8                                   candidateIndex;
	EB_U32                                  fastCandidateTotalCount;
	EB_U32                                  fullCandidateTotalCount;

	EB_U32                                  secondFastCostSearchCandidateTotalCount;
                                          
	// CTB merge                          
	EB_U32                                  lastCuIndex;
                                          
	// Pre Intra Search                   

	ModeDecisionCandidate_t                *fastCandidateArray          = contextPtr->fastCandidateArray;
	ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
	EB_U32							        bestIntraMode               = EB_INTRA_MODE_INVALID;
    
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray;

	EB_U32                                  maxBuffers;


    // enableExitPartitioning derivation @ BdpPillar()
    EB_BOOL enableExitPartitioning = contextPtr->enableExitPartitioning;

    ConstructPillarCuArray(
        pictureControlSetPtr,
        lcuPtr,
        lcuParamPtr,
        contextPtr);

    contextPtr->cu8x8RefinementOnFlag = EB_FALSE;

    // Keep track of the LCU Ptr
	contextPtr->lcuPtr                  = lcuPtr;
    
	contextPtr->groupOf8x8BlocksCount   = 0;
	contextPtr->groupOf16x16BlocksCount = 0;

    // Mode Decision Neighbor Arrays
    contextPtr->intraLumaModeNeighborArray  =   pictureControlSetPtr->mdIntraLumaModeNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->mvNeighborArray             =   pictureControlSetPtr->mdMvNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->skipFlagNeighborArray       =   pictureControlSetPtr->mdSkipFlagNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->modeTypeNeighborArray       =   pictureControlSetPtr->mdModeTypeNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->leafDepthNeighborArray      =   pictureControlSetPtr->mdLeafDepthNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->lumaReconNeighborArray      =   pictureControlSetPtr->mdLumaReconNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->cbReconNeighborArray        =   pictureControlSetPtr->mdCbReconNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->crReconNeighborArray        =   pictureControlSetPtr->mdCrReconNeighborArray[PILLAR_NEIGHBOR_ARRAY_INDEX][tileIdx];

    contextPtr->edgeBlockNumFlag = (EB_BOOL)pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum;

    // First CU Loop  
	cuIdx = 0;
	do
	{
		// Assign CU data

        const EB_U8 leafIndex = contextPtr->pillarCuArray.leafDataArray[cuIdx];

		CodingUnit_t *  cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

		const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(leafIndex);
        contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;


        contextPtr->cuOriginX = lcuParamPtr->originX + cuStatsPtr->originX;
		contextPtr->cuOriginY = lcuParamPtr->originY + cuStatsPtr->originY;
		const EB_U32 inputOriginIndex = (contextPtr->cuOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (contextPtr->cuOriginX + inputPicturePtr->originX);
		const EB_U32 inputCbOriginIndex = ((contextPtr->cuOriginY>>1) + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCb + ((contextPtr->cuOriginX >>1) + (inputPicturePtr->originX >> 1));
		const EB_U32 cuOriginIndex       = ((contextPtr->cuOriginY & 63)  * 64) + (contextPtr->cuOriginX & 63);
		const EB_U32 cuChromaOriginIndex = (((contextPtr->cuOriginY & 63) * 32) + (contextPtr->cuOriginX & 63)) >> 1;

        //need these for L7 Chroma: TBCleaned.
        contextPtr->cuDepth = cuStatsPtr->depth;
        contextPtr->cuSize  = cuStatsPtr->size;
        contextPtr->lcuSize = MAX_LCU_SIZE;
        contextPtr->lcuChromaSize = contextPtr->lcuSize >> 1;
        contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;
        contextPtr->cuChromaOriginX = contextPtr->cuOriginX >> 1;
		contextPtr->cuChromaOriginY = contextPtr->cuOriginY >> 1;
		contextPtr->mdLocalCuUnit[leafIndex].testedCuFlag = EB_TRUE;
		cuPtr->leafIndex = leafIndex;	
        cuPtr->qp = contextPtr->qp;

        candidateBufferPtrArray = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);

        // Set PF Mode - should be done per TU (and not per CU) to avoid the correction
		DerivePartialFrequencyN2Flag(
			pictureControlSetPtr,
			contextPtr);
  
		// Initialize Fast Loop
		ProductCodingLoopInitFastLoop( // HT to be rechecked especially for fullCostArray
			contextPtr,
            contextPtr->intraLumaModeNeighborArray,
		    contextPtr->skipFlagNeighborArray,
		    contextPtr->modeTypeNeighborArray,
		    contextPtr->leafDepthNeighborArray);

		// Perform MPM search
		if (contextPtr->mpmSearch) {
            DeriveMpmModes( // HT done
                contextPtr,
                cuPtr);
		}

        SetNmm(
            contextPtr,
            BDP_PILLAR_STAGE);

        SetNfl(
            lcuPtr,
            contextPtr,
            pictureControlSetPtr,
            BDP_PILLAR_STAGE);

        contextPtr->conformantMvMergeTable = EB_TRUE;

		ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
			lcuPtr,
			contextPtr,
			leafIndex,
			lcuAddr,
			&bufferTotalCount,
			&fastCandidateTotalCount,
			(void*)contextPtr->interPredictionContext,
			pictureControlSetPtr,
			contextPtr->mpmSearch,
			contextPtr->mpmSearchCandidate,
			contextPtr->mostProbableModeArray);


        //if we want to recon N candidate, we would need N+1 buffers
		maxBuffers = MIN((bufferTotalCount + 1), contextPtr->bufferDepthIndexWidth[cuStatsPtr->depth]);

		ProductPerformFastLoop(
			pictureControlSetPtr,
			lcuPtr,
			contextPtr,
			candidateBufferPtrArrayBase,
			fastCandidateArray,
			fastCandidateTotalCount,
			inputPicturePtr,
			inputOriginIndex,
			inputCbOriginIndex,
            inputCbOriginIndex,
            cuPtr,
			cuOriginIndex,
			cuChromaOriginIndex,
			maxBuffers,
			&secondFastCostSearchCandidateTotalCount);

		// Make sure bufferTotalCount is not larger than the number of fast modes
		bufferTotalCount = MIN(secondFastCostSearchCandidateTotalCount, bufferTotalCount);

		// PreModeDecision
		// -Input is the buffers
		// -Output is list of buffers for full reconstruction
		EB_U8 disableMergeIndex = 0;

        PreModeDecision(
            cuPtr,
            (secondFastCostSearchCandidateTotalCount == bufferTotalCount) ? bufferTotalCount : maxBuffers,
            candidateBufferPtrArray,
            &fullCandidateTotalCount,
            contextPtr->bestCandidateIndexArray,
			&disableMergeIndex,
            (EB_BOOL)(secondFastCostSearchCandidateTotalCount == bufferTotalCount)); // The fast loop bug fix is now added to 4K only

		PerformFullLoop(
            pictureControlSetPtr,
            lcuPtr,
            cuPtr,
            contextPtr,
            inputPicturePtr,
            inputOriginIndex,
            inputCbOriginIndex,
            cuOriginIndex,
            cuChromaOriginIndex,
            MIN(fullCandidateTotalCount, bufferTotalCount)); // fullCandidateTotalCount to number of buffers to process  

		// Full Mode Decision (choose the best mode)
		candidateIndex = ProductFullModeDecision(
            contextPtr,
			cuPtr,
            contextPtr->cuStats->size,
			candidateBufferPtrArray,
			fullCandidateTotalCount,
			contextPtr->bestCandidateIndexArray,
			&bestIntraMode);

		candidateBuffer = candidateBufferPtrArray[candidateIndex];
		
        bestCandidateBuffers[contextPtr->cuStats->depth] = candidateBuffer;
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                = candidateBuffer->yCoeffBits;
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[0]                    = candidateBuffer->yDc[0];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[1]                    = candidateBuffer->yDc[1];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[2]                    = candidateBuffer->yDc[2];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[3]                    = candidateBuffer->yDc[3];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[0]    = candidateBuffer->yCountNonZeroCoeffs[0];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[1]    = candidateBuffer->yCountNonZeroCoeffs[1];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[2]    = candidateBuffer->yCountNonZeroCoeffs[2];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[3]    = candidateBuffer->yCountNonZeroCoeffs[3];

        if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST && candidateBuffer->candidatePtr->type == INTER_MODE && candidateBuffer->candidatePtr->mergeFlag == EB_TRUE)
        {
            contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                            = candidateBuffer->yCoeffBits;
            contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_RESIDUAL]   = candidateBuffer->yFullDistortion[DIST_CALC_RESIDUAL];
            contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_PREDICTION] = candidateBuffer->yFullDistortion[DIST_CALC_PREDICTION];
            contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCbf                                  = candidateBuffer->candidatePtr->yCbf ;
            contextPtr->mdEpPipeLcu[cuPtr->leafIndex].fastLumaRate                          = candidateBuffer->candidatePtr->fastLumaRate;
        }

		EB_U8          parentLeafIndex;
		EB_BOOL        exitPartition = EB_FALSE; 
		contextPtr->mdLocalCuUnit[leafIndex].mdcArrayIndex = (EB_U8)cuIdx;
		
		CheckHighCostPartition(
            sequenceControlSetPtr,
            contextPtr,
            lcuPtr,
            leafIndex,
            &parentLeafIndex,
            enableExitPartitioning,
            &exitPartition);

	   if (exitPartition)
	   {
		   //Reset the CU-Loop to the parent CU.
		   cuStatsPtr = GetCodedUnitStats(parentLeafIndex);
		   cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[parentLeafIndex];
		   cuIdx = contextPtr->mdLocalCuUnit[parentLeafIndex].mdcArrayIndex;
		   cuPtr->splitFlag = EB_FALSE;

		   lastCuIndex = ExitInterDepthDecision(
			   contextPtr,
			   parentLeafIndex,
			   lcuPtr,
			   lcuAddr,
			   lcuParamPtr->originX,
			   lcuParamPtr->originY,
			   contextPtr->fullLambda,
			   contextPtr->mdRateEstimationPtr,
			   pictureControlSetPtr);

	   }
	   else{
            PerformInverseTransformRecon(
                contextPtr,
                candidateBuffer,
                cuPtr,
                contextPtr->cuStats);

		// Inter Depth Decision
		lastCuIndex = PillarInterDepthDecision(
			contextPtr,
			leafIndex,
			lcuPtr,
			lcuParamPtr->originX,
			lcuParamPtr->originY,
			contextPtr->fullLambda,
			contextPtr->mdRateEstimationPtr,
			pictureControlSetPtr);
	   }

		if (lcuPtr->codedLeafArrayPtr[lastCuIndex]->splitFlag == EB_FALSE)
		{
			// Update the CU Stats
			contextPtr->cuStats = GetCodedUnitStats(lastCuIndex);
			contextPtr->cuOriginX = lcuParamPtr->originX + contextPtr->cuStats->originX;
			contextPtr->cuOriginY = lcuParamPtr->originY + contextPtr->cuStats->originY;

			// Set the MvUnit
			contextPtr->mvUnit.predDirection   = (EB_U8)(&lcuPtr->codedLeafArrayPtr[lastCuIndex]->predictionUnitArray[0])->interPredDirectionIndex;
			contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[lastCuIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
			contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[lastCuIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

			// Set the candidate buffer
			contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[lastCuIndex];
			candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];

			// Update Neighbor Arrays
			{
				EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
				EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
				EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

				ModeDecisionUpdateNeighborArrays(
					contextPtr->leafDepthNeighborArray,
					contextPtr->modeTypeNeighborArray,
					contextPtr->intraLumaModeNeighborArray,
					contextPtr->mvNeighborArray,
					contextPtr->skipFlagNeighborArray,
					contextPtr->lumaReconNeighborArray,
					contextPtr->cbReconNeighborArray,
					contextPtr->crReconNeighborArray,
					candidateBuffer->reconPtr,
                    MAX_LCU_SIZE,
                    contextPtr->intraMdOpenLoopFlag,
                    EB_FALSE,
					(EB_U8*)&contextPtr->cuStats->depth,
					&predictionModeFlag,
					&intraLumaMode,
					&contextPtr->mvUnit,
					&skipFlag,
					contextPtr->cuOriginX,
					contextPtr->cuOriginY,
					contextPtr->cuStats->size, 
                    contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);

			}

			if (contextPtr->intraMdOpenLoopFlag == EB_FALSE)
			{
				UpdateMdReconBuffer(
					candidateBuffer->reconPtr,
					contextPtr->pillarReconBuffer,
					contextPtr,
					lcuPtr);
			}
		}

		if (exitPartition == EB_FALSE){
			cuIdx++;
		}
		else{
			cuIdx += BdpCalculateNextCuIndex(
				contextPtr->pillarCuArray.leafDataArray,
				cuIdx,
				contextPtr->pillarCuArray.leafCount,
				cuStatsPtr->depth);
		}



	} while (cuIdx < contextPtr->pillarCuArray.leafCount);

	return return_error;
}	


void SplitParentCu(
	ModeDecisionContext_t               *contextPtr,
    LargestCodingUnit_t         *lcuPtr,
    EB_U8                        parentLeafIndex,
    EB_U64                       totalChildCost,
    EB_U64                       fullLambda,
    MdRateEstimationContext_t    *mdRateEstimationPtr,
    EB_U64                       *depthNCost,
    EB_U64                       *depthNPlusOneCost)
{

    EB_U64                     depthNRate = 0;
    EB_U64                     depthNPlusOneRate = 0;


    // Copy the Mode & Depth of the Top-Left N+1 block to the N block for the SplitContext calculation
    //   This needs to be done in the case that the N block was initially not calculated.
	contextPtr->mdLocalCuUnit[parentLeafIndex].leftNeighborMode = contextPtr->mdLocalCuUnit[parentLeafIndex + 1].leftNeighborMode;
	contextPtr->mdLocalCuUnit[parentLeafIndex].leftNeighborDepth = contextPtr->mdLocalCuUnit[parentLeafIndex + 1].leftNeighborDepth;
	contextPtr->mdLocalCuUnit[parentLeafIndex].topNeighborMode = contextPtr->mdLocalCuUnit[parentLeafIndex + 1].topNeighborMode;
	contextPtr->mdLocalCuUnit[parentLeafIndex].topNeighborDepth = contextPtr->mdLocalCuUnit[parentLeafIndex + 1].topNeighborDepth;
    // Compute depth N cost
	SplitFlagRate(
		contextPtr,
        lcuPtr->codedLeafArrayPtr[parentLeafIndex],
        0,
        &depthNRate,
        fullLambda,
        mdRateEstimationPtr,
        3); //tbMaxDepth);

	*depthNCost = contextPtr->mdLocalCuUnit[parentLeafIndex].cost + depthNRate;
    // Compute depth N+1 cost
	SplitFlagRate(
		contextPtr,
        lcuPtr->codedLeafArrayPtr[parentLeafIndex],
        1,
        &depthNPlusOneRate,
        fullLambda,
        mdRateEstimationPtr,
        3); //tbMaxDepth);

    *depthNPlusOneCost = totalChildCost + depthNPlusOneRate;
}

EB_EXTERN EB_ERRORTYPE Bdp64x64vs32x32RefinementProcess(
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr)
{
    EB_ERRORTYPE                            return_error = EB_ErrorNone;

    // Input      
    EbPictureBufferDesc_t                  *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->chromaDownSamplePicturePtr;

    // Mode Decision Candidate Buffers
    EB_U32                                  bufferTotalCount;
    ModeDecisionCandidateBuffer_t          *candidateBuffer;
    ModeDecisionCandidateBuffer_t          *bestCandidateBuffers[EB_MAX_LCU_DEPTH];

    // Mode Decision Search Candidates 
    EB_U8                                   candidateIndex;
    EB_U32                                  fastCandidateTotalCount;
    EB_U32                                  fullCandidateTotalCount;

    EB_U32                                  secondFastCostSearchCandidateTotalCount;

    // Pre Intra Search                   

    ModeDecisionCandidate_t                *fastCandidateArray = contextPtr->fastCandidateArray;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
    EB_U32							        bestIntraMode = EB_INTRA_MODE_INVALID;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray;

    EB_U32                                  maxBuffers;
    EB_U16                                  tileIdx = lcuPtr->lcuEdgeInfoPtr->tileIndexInRaster;

    // Keep track of the LCU Ptr
    contextPtr->lcuPtr = lcuPtr;

    contextPtr->intraLumaModeNeighborArray  = pictureControlSetPtr->mdIntraLumaModeNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->mvNeighborArray             = pictureControlSetPtr->mdMvNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->skipFlagNeighborArray       = pictureControlSetPtr->mdSkipFlagNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->modeTypeNeighborArray       = pictureControlSetPtr->mdModeTypeNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->leafDepthNeighborArray      = pictureControlSetPtr->mdLeafDepthNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->lumaReconNeighborArray      = pictureControlSetPtr->mdLumaReconNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->cbReconNeighborArray        = pictureControlSetPtr->mdCbReconNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->crReconNeighborArray        = pictureControlSetPtr->mdCrReconNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];

    EB_U8 leafIndex = 0;

    CodingUnit_t * const cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];
    const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(leafIndex);
    contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;

    contextPtr->cuOriginX = lcuParamPtr->originX + cuStatsPtr->originX;
    contextPtr->cuOriginY = lcuParamPtr->originY + cuStatsPtr->originY;
    const EB_U32 inputOriginIndex = (contextPtr->cuOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (contextPtr->cuOriginX + inputPicturePtr->originX);
    const EB_U32 inputCbOriginIndex = ((contextPtr->cuOriginY >> 1) + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCb + ((contextPtr->cuOriginX >> 1) + (inputPicturePtr->originX >> 1));
    const EB_U32 cuOriginIndex = ((contextPtr->cuOriginY & 63) * 64) + (contextPtr->cuOriginX & 63);
    const EB_U32 cuChromaOriginIndex = (((contextPtr->cuOriginY & 63) * 32) + (contextPtr->cuOriginX & 63)) >> 1;

    //need these for L7 Chroma: TBCleaned.
    contextPtr->cuDepth = cuStatsPtr->depth;
    contextPtr->cuSize = cuStatsPtr->size;
    contextPtr->lcuSize = MAX_LCU_SIZE;
    contextPtr->lcuChromaSize = contextPtr->lcuSize >> 1;
    contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;
    contextPtr->cuChromaOriginX = contextPtr->cuOriginX >> 1;
    contextPtr->cuChromaOriginY = contextPtr->cuOriginY >> 1;
    cuPtr->leafIndex = leafIndex;
    cuPtr->qp = contextPtr->qp;
    candidateBufferPtrArray = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);

    // Set PF Mode - should be done per TU (and not per CU) to avoid the correction
	DerivePartialFrequencyN2Flag(
		pictureControlSetPtr,
		contextPtr);

    // Initialize Fast Loop
    ProductCodingLoopInitFastLoop( // HT to be rechecked especially for fullCostArray
        contextPtr,
        contextPtr->intraLumaModeNeighborArray,
        contextPtr->skipFlagNeighborArray,
        contextPtr->modeTypeNeighborArray,
        contextPtr->leafDepthNeighborArray);

    // Perform MPM search
    if (contextPtr->mpmSearch) {
        DeriveMpmModes( // HT done
            contextPtr,
            cuPtr);
    }

    SetNmm(
        contextPtr,
        BDP_64X64_32X32_REF_STAGE);

    SetNfl(
        lcuPtr,
        contextPtr,
        pictureControlSetPtr,
        BDP_64X64_32X32_REF_STAGE);

    contextPtr->conformantMvMergeTable = EB_TRUE;

    ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
        lcuPtr,
        contextPtr,
		leafIndex,
        lcuAddr,
        &bufferTotalCount,
        &fastCandidateTotalCount,
        (void*)contextPtr->interPredictionContext,
        pictureControlSetPtr,
        contextPtr->mpmSearch,
        contextPtr->mpmSearchCandidate,
        contextPtr->mostProbableModeArray);

    //if we want to recon N candidate, we would need N+1 buffers
    maxBuffers = MIN((bufferTotalCount + 1), contextPtr->bufferDepthIndexWidth[cuStatsPtr->depth]);

    ProductPerformFastLoop(
        pictureControlSetPtr,
        lcuPtr,
        contextPtr,
        candidateBufferPtrArrayBase,
        fastCandidateArray,
        fastCandidateTotalCount,
        inputPicturePtr,
        inputOriginIndex,
        inputCbOriginIndex,
        inputCbOriginIndex,
        cuPtr,
        cuOriginIndex,
        cuChromaOriginIndex,
        maxBuffers,
        &secondFastCostSearchCandidateTotalCount);

    // Make sure bufferTotalCount is not larger than the number of fast modes
    bufferTotalCount = MIN(secondFastCostSearchCandidateTotalCount, bufferTotalCount);

    // PreModeDecision
    // -Input is the buffers
    // -Output is list of buffers for full reconstruction
	EB_U8 disableMergeIndex = 0;

    PreModeDecision(
        cuPtr,
        (secondFastCostSearchCandidateTotalCount == bufferTotalCount) ? bufferTotalCount : maxBuffers,
        candidateBufferPtrArray,
        &fullCandidateTotalCount,
        contextPtr->bestCandidateIndexArray,
		&disableMergeIndex,
        (EB_BOOL)(secondFastCostSearchCandidateTotalCount == bufferTotalCount)); // The fast loop bug fix is now added to 4K only

    PerformFullLoop(
        pictureControlSetPtr,
        lcuPtr,
        cuPtr,
        contextPtr,
        inputPicturePtr,
        inputOriginIndex,
        inputCbOriginIndex,
        cuOriginIndex,
        cuChromaOriginIndex,
        MIN(fullCandidateTotalCount, bufferTotalCount)); // fullCandidateTotalCount to number of buffers to process  

    // Full Mode Decision (choose the best mode)
    candidateIndex = ProductFullModeDecision(
        contextPtr,
        cuPtr,
        contextPtr->cuStats->size,
        candidateBufferPtrArray,
        fullCandidateTotalCount,
        contextPtr->bestCandidateIndexArray,
        &bestIntraMode);
    candidateBuffer = candidateBufferPtrArray[candidateIndex];

    bestCandidateBuffers[contextPtr->cuStats->depth] = candidateBuffer;
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                = candidateBuffer->yCoeffBits;
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[0]                    = candidateBuffer->yDc[0];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[1]                    = candidateBuffer->yDc[1];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[2]                    = candidateBuffer->yDc[2];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[3]                    = candidateBuffer->yDc[3];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[0]    = candidateBuffer->yCountNonZeroCoeffs[0];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[1]    = candidateBuffer->yCountNonZeroCoeffs[1];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[2]    = candidateBuffer->yCountNonZeroCoeffs[2];
    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[3]    = candidateBuffer->yCountNonZeroCoeffs[3];

	if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST && candidateBuffer->candidatePtr->type == INTER_MODE && candidateBuffer->candidatePtr->mergeFlag == EB_TRUE)
    {
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                            = candidateBuffer->yCoeffBits;
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_RESIDUAL]   = candidateBuffer->yFullDistortion[DIST_CALC_RESIDUAL];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_PREDICTION] = candidateBuffer->yFullDistortion[DIST_CALC_PREDICTION];
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCbf                                  = candidateBuffer->candidatePtr->yCbf ;
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].fastLumaRate                          = candidateBuffer->candidatePtr->fastLumaRate;
    }

    PerformInverseTransformRecon(
        contextPtr,
        candidateBuffer,
        cuPtr,
        contextPtr->cuStats);

    EB_U64 depthNCost = 0;
    EB_U64 depthNPlusOneCost = 0;
	EB_U64 totalChildCost = contextPtr->mdLocalCuUnit[1].cost + contextPtr->mdLocalCuUnit[22].cost + contextPtr->mdLocalCuUnit[43].cost + contextPtr->mdLocalCuUnit[64].cost;

	SplitParentCu(
		contextPtr,
        lcuPtr,
        0,
        totalChildCost,
        contextPtr->fullLambda,
        contextPtr->mdRateEstimationPtr,
        &depthNCost,
        &depthNPlusOneCost);

    if (depthNCost < depthNPlusOneCost) {

        lcuPtr->codedLeafArrayPtr[0]->splitFlag = EB_FALSE;

        // Update the CU Stats
        contextPtr->cuStats = GetCodedUnitStats(leafIndex);
        contextPtr->cuOriginX = lcuParamPtr->originX + contextPtr->cuStats->originX;
        contextPtr->cuOriginY = lcuParamPtr->originY + contextPtr->cuStats->originY;

        // Set the MvUnit
        contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
        contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
        contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

        // Set the candidate buffer
        contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];
        candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];

        // Update Neighbor Arrays
        EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
        EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
        EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

        ModeDecisionUpdateNeighborArrays(
            contextPtr->leafDepthNeighborArray,
            contextPtr->modeTypeNeighborArray,
            contextPtr->intraLumaModeNeighborArray,
            contextPtr->mvNeighborArray,
            contextPtr->skipFlagNeighborArray,
            contextPtr->lumaReconNeighborArray,
            contextPtr->cbReconNeighborArray,
            contextPtr->crReconNeighborArray,
            candidateBuffer->reconPtr,
            MAX_LCU_SIZE,
            contextPtr->intraMdOpenLoopFlag,
            EB_FALSE,
            (EB_U8*)&contextPtr->cuStats->depth,
            &predictionModeFlag,
            &intraLumaMode,
            &contextPtr->mvUnit,
            &skipFlag,
            contextPtr->cuOriginX,
            contextPtr->cuOriginY,
            contextPtr->cuStats->size,
            contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);

		if (contextPtr->intraMdOpenLoopFlag == EB_FALSE)
		{
			UpdateMdReconBuffer(
				candidateBuffer->reconPtr,
				contextPtr->pillarReconBuffer,
				contextPtr,
				lcuPtr);
		}
	}


    return return_error;
}


EB_EXTERN EB_ERRORTYPE Bdp16x16vs8x8RefinementProcess(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr, 
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr)
{
    EB_ERRORTYPE                            return_error = EB_ErrorNone;

    // Input    
    EbPictureBufferDesc_t                  *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->chromaDownSamplePicturePtr;

    EB_U16                                  tileIdx = contextPtr->tileIndex;

    // Mode Decision Candidate Buffers
    EB_U32                                  bufferTotalCount;
    ModeDecisionCandidateBuffer_t          *candidateBuffer;
    ModeDecisionCandidateBuffer_t          *bestCandidateBuffers[EB_MAX_LCU_DEPTH] = {NULL};
	
    // Mode Decision Search Candidates 
    EB_U8                                   candidateIndex;
    EB_U32                                  fastCandidateTotalCount;
    EB_U32                                  fullCandidateTotalCount;

    EB_U32                                  secondFastCostSearchCandidateTotalCount;

    // Pre Intra Search                   

    ModeDecisionCandidate_t                *fastCandidateArray = contextPtr->fastCandidateArray;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
    EB_U32							        bestIntraMode = EB_INTRA_MODE_INVALID;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray;

    EB_U32                                  maxBuffers;


	LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuPtr->index];

    // enableExitPartitioning derivation @ Bdp16x16vs8x8RefinementProcess()
    EB_BOOL enableExitPartitioning = (contextPtr->enableExitPartitioning && lcuParams->isCompleteLcu) ? EB_TRUE : EB_FALSE;


    // Keep track of the LCU Ptr
    contextPtr->lcuPtr = lcuPtr;  

    contextPtr->intraLumaModeNeighborArray  = pictureControlSetPtr->mdIntraLumaModeNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->mvNeighborArray             = pictureControlSetPtr->mdMvNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->skipFlagNeighborArray       = pictureControlSetPtr->mdSkipFlagNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->modeTypeNeighborArray       = pictureControlSetPtr->mdModeTypeNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->leafDepthNeighborArray      = pictureControlSetPtr->mdLeafDepthNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->lumaReconNeighborArray      = pictureControlSetPtr->mdLumaReconNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->cbReconNeighborArray        = pictureControlSetPtr->mdCbReconNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->crReconNeighborArray        = pictureControlSetPtr->mdCrReconNeighborArray[REFINEMENT_NEIGHBOR_ARRAY_INDEX][tileIdx];

    EB_U8 parentLeafIndex = 0;
    
    while (parentLeafIndex < CU_MAX_COUNT) {

        if (lcuPtr->codedLeafArrayPtr[parentLeafIndex]->splitFlag == EB_FALSE) {

            EB_U8  parentDepthOffset = DepthOffset[GetCodedUnitStats(parentLeafIndex)->depth];
            EB_U8  childDepthOffset = DepthOffset[GetCodedUnitStats(parentLeafIndex)->depth + 1];
            EB_BOOL cu16x16RefinementFlag;  

			if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LIGHT_BDP_DEPTH_MODE || (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE  && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuAddr] == LCU_LIGHT_BDP_DEPTH_MODE))){

                cu16x16RefinementFlag = GetCodedUnitStats(parentLeafIndex)->size == 16 && (lcuPtr->codedLeafArrayPtr[parentLeafIndex]->predictionModeFlag == INTRA_MODE || contextPtr->depthRefinment > 0);


            }else{
                cu16x16RefinementFlag = (GetCodedUnitStats(parentLeafIndex)->size == 16) ;
            }

            if (cu16x16RefinementFlag) {

                EB_U64 totalChildCost = 0;

                EB_U8 totalLeafCount;
                EB_U8 leafCount;

                EB_U8 leafIndexArray[5];

                totalLeafCount = 4;
                leafIndexArray[0] = parentLeafIndex + 1;
                leafIndexArray[1] = parentLeafIndex + 1 + childDepthOffset;
                leafIndexArray[2] = parentLeafIndex + 1 + childDepthOffset * 2;
                leafIndexArray[3] = parentLeafIndex + 1 + childDepthOffset * 3;

 
				EB_BOOL exitPartition = EB_FALSE;
				EB_U64  cu16x16Cost = MAX_CU_COST;
				if (enableExitPartitioning)
				{
					EB_U64  depth2Rate;
					SplitFlagRate(
						contextPtr,
						lcuPtr->codedLeafArrayPtr[parentLeafIndex],
						0,
						&depth2Rate,
						contextPtr->fullLambda,
						contextPtr->mdRateEstimationPtr,
						3); //tbMaxDepth);     //CHKN  THIS SHOULD BE 4 - keep 3 to be consistent with BDP usage

					cu16x16Cost = contextPtr->mdLocalCuUnit[parentLeafIndex].cost + depth2Rate;
				}


                for (leafCount = 0; leafCount < totalLeafCount; leafCount++) {

                    EB_U8 leafIndex = leafIndexArray[leafCount];

                    EB_U8 neighborLeafIndex = 0;

                    CodingUnit_t * const cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];
                    const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(leafIndex);
                    contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;

                    contextPtr->cuOriginX = lcuParamPtr->originX + cuStatsPtr->originX;
                    contextPtr->cuOriginY = lcuParamPtr->originY + cuStatsPtr->originY;
                    const EB_U32 inputOriginIndex = (contextPtr->cuOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (contextPtr->cuOriginX + inputPicturePtr->originX);
                    const EB_U32 inputCbOriginIndex = ((contextPtr->cuOriginY >> 1) + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCb + ((contextPtr->cuOriginX >> 1) + (inputPicturePtr->originX >> 1));
                    const EB_U32 cuOriginIndex = ((contextPtr->cuOriginY & 63) * 64) + (contextPtr->cuOriginX & 63);
                    const EB_U32 cuChromaOriginIndex = (((contextPtr->cuOriginY & 63) * 32) + (contextPtr->cuOriginX & 63)) >> 1;

                    //need these for L7 Chroma: TBCleaned.
                    contextPtr->cuDepth = cuStatsPtr->depth;
                    contextPtr->cuSize = cuStatsPtr->size;
                    contextPtr->lcuSize = MAX_LCU_SIZE;
                    contextPtr->lcuChromaSize = contextPtr->lcuSize >> 1;
                    contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;
                    contextPtr->cuChromaOriginX = contextPtr->cuOriginX >> 1;
                    contextPtr->cuChromaOriginY = contextPtr->cuOriginY >> 1;
					contextPtr->mdLocalCuUnit[leafIndex].testedCuFlag = EB_TRUE;
                    cuPtr->leafIndex = leafIndex;
                    cuPtr->qp = contextPtr->qp;
                    candidateBufferPtrArray = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);

					// Set PF Mode - should be done per TU (and not per CU) to avoid the correction
					DerivePartialFrequencyN2Flag(
						pictureControlSetPtr,
						contextPtr);				

                    // Initialize Fast Loop
                    ProductCodingLoopInitFastLoop( // HT to be rechecked especially for fullCostArray
                        contextPtr,
                        contextPtr->intraLumaModeNeighborArray,
                        contextPtr->skipFlagNeighborArray,
                        contextPtr->modeTypeNeighborArray,
                        contextPtr->leafDepthNeighborArray);

                    // Perform MPM search
                    if (contextPtr->mpmSearch) {
                        DeriveMpmModes( // HT done
                            contextPtr,
                            cuPtr);
                    }

                    SetNmm(
                        contextPtr,
                        BDP_16X16_8X8_REF_STAGE);

                    SetNfl(
                        lcuPtr,
                        contextPtr,
                        pictureControlSetPtr,
                        BDP_16X16_8X8_REF_STAGE);

                    contextPtr->conformantMvMergeTable = EB_FALSE;
                    ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
                        lcuPtr,
                        contextPtr,
						leafIndex,
                        lcuAddr,
                        &bufferTotalCount,
                        &fastCandidateTotalCount,
                        (void*)contextPtr->interPredictionContext,
                        pictureControlSetPtr,
                        contextPtr->mpmSearch,
                        contextPtr->mpmSearchCandidate,
                        contextPtr->mostProbableModeArray);


                    //if we want to recon N candidate, we would need N+1 buffers
                    maxBuffers = MIN((bufferTotalCount + 1), contextPtr->bufferDepthIndexWidth[cuStatsPtr->depth]);

                    ProductPerformFastLoop(
                        pictureControlSetPtr,
                        lcuPtr,
                        contextPtr,
                        candidateBufferPtrArrayBase,
                        fastCandidateArray,
                        fastCandidateTotalCount,
                        inputPicturePtr,
                        inputOriginIndex,
                        inputCbOriginIndex,
                        inputCbOriginIndex,
                        cuPtr,
                        cuOriginIndex,
                        cuChromaOriginIndex,
                        maxBuffers,
                        &secondFastCostSearchCandidateTotalCount);

                    // Make sure bufferTotalCount is not larger than the number of fast modes
                    bufferTotalCount = MIN(secondFastCostSearchCandidateTotalCount, bufferTotalCount);

                    // PreModeDecision
                    // -Input is the buffers
                    // -Output is list of buffers for full reconstruction
					EB_U8 disableMergeIndex = 0;

                    PreModeDecision(
                        cuPtr,
                        (secondFastCostSearchCandidateTotalCount == bufferTotalCount) ? bufferTotalCount : maxBuffers,
                        candidateBufferPtrArray,
                        &fullCandidateTotalCount,
                        contextPtr->bestCandidateIndexArray,
						&disableMergeIndex,
                        (EB_BOOL)(secondFastCostSearchCandidateTotalCount == bufferTotalCount)); // The fast loop bug fix is now added to 4K only

                    PerformFullLoop(
                        pictureControlSetPtr,
                        lcuPtr,
                        cuPtr,
                        contextPtr,
                        inputPicturePtr,
                        inputOriginIndex,
                        inputCbOriginIndex,
                        cuOriginIndex,
                        cuChromaOriginIndex,
                        MIN(fullCandidateTotalCount, bufferTotalCount)); // fullCandidateTotalCount to number of buffers to process  

                    // Full Mode Decision (choose the best mode)
                    candidateIndex = ProductFullModeDecision(
                        contextPtr,
                        cuPtr,
                        contextPtr->cuStats->size,
                        candidateBufferPtrArray,
                        fullCandidateTotalCount,
                        contextPtr->bestCandidateIndexArray,
                        &bestIntraMode);
                    candidateBuffer = candidateBufferPtrArray[candidateIndex];

                    bestCandidateBuffers[contextPtr->cuStats->depth] = candidateBuffer;
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                = candidateBuffer->yCoeffBits;
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[0]                    = candidateBuffer->yDc[0];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[1]                    = candidateBuffer->yDc[1];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[2]                    = candidateBuffer->yDc[2];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[3]                    = candidateBuffer->yDc[3];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[0]    = candidateBuffer->yCountNonZeroCoeffs[0];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[1]    = candidateBuffer->yCountNonZeroCoeffs[1];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[2]    = candidateBuffer->yCountNonZeroCoeffs[2];
                    contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[3]    = candidateBuffer->yCountNonZeroCoeffs[3];

				   if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST && candidateBuffer->candidatePtr->type == INTER_MODE && candidateBuffer->candidatePtr->mergeFlag == EB_TRUE)
				   {
						 contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                            = candidateBuffer->yCoeffBits;
						 contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_RESIDUAL]   = candidateBuffer->yFullDistortion[DIST_CALC_RESIDUAL];
						 contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_PREDICTION] = candidateBuffer->yFullDistortion[DIST_CALC_PREDICTION];
						 contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCbf                                  = candidateBuffer->candidatePtr->yCbf ;
						 contextPtr->mdEpPipeLcu[cuPtr->leafIndex].fastLumaRate                          = candidateBuffer->candidatePtr->fastLumaRate;
				   }

					PerformInverseTransformRecon(
						contextPtr,
						candidateBuffer,
						cuPtr,
						contextPtr->cuStats);

                    if (leafCount < 4) {

						totalChildCost += contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost;
                        neighborLeafIndex = leafIndex;
                    }

    
					if (enableExitPartitioning){
						if (leafCount < 3 && totalChildCost > cu16x16Cost  && lcuParams->isCompleteLcu)
						{
							exitPartition = EB_TRUE;
							neighborLeafIndex = parentLeafIndex;
						}
					}


                    if (leafCount == (totalLeafCount - 1)){

                        EB_U64 depthNCost = 0;
                        EB_U64 depthNPlusOneCost = 0;

						parentLeafIndex = (parentLeafIndex == (CU_MAX_COUNT - 1)) ? CU_MAX_COUNT - 5 : parentLeafIndex;
						
						SplitParentCu(
							contextPtr,
                            lcuPtr,
                            parentLeafIndex,
                            totalChildCost,
                            contextPtr->fullLambda,
                            contextPtr->mdRateEstimationPtr,
                            &depthNCost,
                            &depthNPlusOneCost);

                        if (depthNPlusOneCost < depthNCost) {
                            lcuPtr->codedLeafArrayPtr[parentLeafIndex]->splitFlag = EB_TRUE;

                            lcuPtr->codedLeafArrayPtr[parentLeafIndex + 1]->splitFlag = EB_FALSE;
                            lcuPtr->codedLeafArrayPtr[parentLeafIndex + 1 + childDepthOffset]->splitFlag = EB_FALSE;
                            lcuPtr->codedLeafArrayPtr[parentLeafIndex + 1 + childDepthOffset * 2]->splitFlag = EB_FALSE;
                            lcuPtr->codedLeafArrayPtr[parentLeafIndex + 1 + childDepthOffset * 3]->splitFlag = EB_FALSE;
                            
                            contextPtr->cu8x8RefinementOnFlag = EB_TRUE;
                            
                        }
                        else {
                            neighborLeafIndex = parentLeafIndex;
                        }
                    }

                    if (leafCount < 4 || neighborLeafIndex == parentLeafIndex) {

                        // Update the CU Stats
                        contextPtr->cuStats = GetCodedUnitStats(neighborLeafIndex);
                        contextPtr->cuOriginX = lcuParamPtr->originX + contextPtr->cuStats->originX;
                        contextPtr->cuOriginY = lcuParamPtr->originY + contextPtr->cuStats->originY;

                        // Set the MvUnit
                        contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[neighborLeafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
                        contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[neighborLeafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
                        contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[neighborLeafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

                        // Set the candidate buffer
                        contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[neighborLeafIndex];
                        candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];

                        // Update Neighbor Arrays
                        EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
                        EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
                        EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

                        // Set recon buffer
                        EbPictureBufferDesc_t *reconBuffer = (neighborLeafIndex == parentLeafIndex) ?
                            contextPtr->pillarReconBuffer :
                            candidateBuffer->reconPtr     ;

                        ModeDecisionUpdateNeighborArrays(                           
                            contextPtr->leafDepthNeighborArray,
                            contextPtr->modeTypeNeighborArray,
                            contextPtr->intraLumaModeNeighborArray,
                            contextPtr->mvNeighborArray,
                            contextPtr->skipFlagNeighborArray,
                            contextPtr->lumaReconNeighborArray,
                            contextPtr->cbReconNeighborArray,
                            contextPtr->crReconNeighborArray,
                            reconBuffer,
                            MAX_LCU_SIZE,
                            contextPtr->intraMdOpenLoopFlag,
                            EB_FALSE,
                            (EB_U8*)&contextPtr->cuStats->depth,
                            &predictionModeFlag,
                            &intraLumaMode,
                            &contextPtr->mvUnit,
                            &skipFlag,
                            contextPtr->cuOriginX,
                            contextPtr->cuOriginY,
                            contextPtr->cuStats->size,
                            contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);

						if (contextPtr->intraMdOpenLoopFlag == EB_FALSE)
						{
							UpdateMdReconBuffer(
								reconBuffer,
								contextPtr->mdReconBuffer,
								contextPtr,
								lcuPtr);
						}
					}


					if (exitPartition == EB_TRUE){
						break;
					}


                }
            }
            else {

                // Update the CU Stats
                contextPtr->cuStats = GetCodedUnitStats(parentLeafIndex);
                contextPtr->cuOriginX = lcuParamPtr->originX + contextPtr->cuStats->originX;
                contextPtr->cuOriginY = lcuParamPtr->originY + contextPtr->cuStats->originY;

                // Set the MvUnit
                contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[parentLeafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
                contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[parentLeafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
                contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[parentLeafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

                // Set the candidate buffer
                contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[parentLeafIndex];
                candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];

                // Update Neighbor Arrays
                EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
                EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
                EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

                Bdp16x16vs8x8RefinementUpdateNeighborArrays(    
                    pictureControlSetPtr,

                    contextPtr->leafDepthNeighborArray,
                    contextPtr->modeTypeNeighborArray,
                    contextPtr->intraLumaModeNeighborArray,
                    contextPtr->mvNeighborArray,
                    contextPtr->skipFlagNeighborArray,
                    contextPtr->lumaReconNeighborArray,
                    contextPtr->cbReconNeighborArray,
                    contextPtr->crReconNeighborArray,
                    contextPtr->pillarReconBuffer,
                    MAX_LCU_SIZE,
                    contextPtr->intraMdOpenLoopFlag,
                    EB_FALSE,
                    (EB_U8*)&contextPtr->cuStats->depth,
                    &predictionModeFlag,
                    &intraLumaMode,
                    &contextPtr->mvUnit,
                    &skipFlag,
                    contextPtr->cuOriginX,
                    contextPtr->cuOriginY,
                    contextPtr->cuStats->size,
                    tileIdx,
                    contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);

				if (contextPtr->intraMdOpenLoopFlag == EB_FALSE)
				{
					UpdateMdReconBuffer(
						contextPtr->pillarReconBuffer,
						contextPtr->mdReconBuffer,
						contextPtr,
						lcuPtr);
				}
			}

            parentLeafIndex += parentDepthOffset;

        } else {

            parentLeafIndex++;
        }
    }   
    return return_error;
}


EB_EXTERN EB_ERRORTYPE BdpMvMergePass(
    PictureControlSet_t                 *pictureControlSetPtr,
    LcuParams_t                         *lcuParamPtr,
    LargestCodingUnit_t                 *lcuPtr,
    EB_U16                               lcuAddr,
    ModeDecisionContext_t               *contextPtr)

{
    EB_ERRORTYPE                            return_error = EB_ErrorNone;
    // Input       
    EbPictureBufferDesc_t                  *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->chromaDownSamplePicturePtr;

    // Mode Decision Candidate Buffers
    EB_U32                                  bufferTotalCount;
    ModeDecisionCandidateBuffer_t          *candidateBuffer;
    ModeDecisionCandidateBuffer_t          *bestCandidateBuffers[EB_MAX_LCU_DEPTH] = {NULL};
	
    // Mode Decision Search Candidates 
    EB_U8                                   candidateIndex;
    EB_U32                                  fastCandidateTotalCount;
    EB_U32                                  fullCandidateTotalCount;

    EB_U32                                  secondFastCostSearchCandidateTotalCount;

    // Pre Intra Search                   

    ModeDecisionCandidate_t                *fastCandidateArray = contextPtr->fastCandidateArray;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArrayBase = contextPtr->candidateBufferPtrArray;
    EB_U32							        bestIntraMode = EB_INTRA_MODE_INVALID;
    ModeDecisionCandidateBuffer_t         **candidateBufferPtrArray;

    EB_U32                                  maxBuffers;
    EB_U16                                  tileIdx = contextPtr->tileIndex;

    // Keep track of the LCU Ptr
    contextPtr->lcuPtr = lcuPtr;

    contextPtr->intraLumaModeNeighborArray  = pictureControlSetPtr->mdIntraLumaModeNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->mvNeighborArray             = pictureControlSetPtr->mdMvNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->skipFlagNeighborArray       = pictureControlSetPtr->mdSkipFlagNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->modeTypeNeighborArray       = pictureControlSetPtr->mdModeTypeNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->leafDepthNeighborArray      = pictureControlSetPtr->mdLeafDepthNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->lumaReconNeighborArray      = pictureControlSetPtr->mdLumaReconNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->cbReconNeighborArray        = pictureControlSetPtr->mdCbReconNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];
    contextPtr->crReconNeighborArray        = pictureControlSetPtr->mdCrReconNeighborArray[MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX][tileIdx];

    // First CU Loop  
    EB_U8 leafIndex = 0;
    while (leafIndex < CU_MAX_COUNT) {

        // Don not touch cuPtr (just check on it)
        CodingUnit_t * const cuPtr = contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

        if (cuPtr->splitFlag == EB_FALSE) {

            if (cuPtr->predictionUnitArray->mergeFlag && contextPtr->cu8x8RefinementOnFlag) {

                const CodedUnitStats_t *cuStatsPtr = contextPtr->cuStats = GetCodedUnitStats(leafIndex);

                contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;


                contextPtr->cuOriginX = lcuParamPtr->originX + cuStatsPtr->originX;
                contextPtr->cuOriginY = lcuParamPtr->originY + cuStatsPtr->originY;
                const EB_U32 inputOriginIndex = (contextPtr->cuOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (contextPtr->cuOriginX + inputPicturePtr->originX);
                const EB_U32 inputCbOriginIndex = ((contextPtr->cuOriginY >> 1) + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCb + ((contextPtr->cuOriginX >> 1) + (inputPicturePtr->originX >> 1));
                const EB_U32 cuOriginIndex = ((contextPtr->cuOriginY & 63) * 64) + (contextPtr->cuOriginX & 63);
                const EB_U32 cuChromaOriginIndex = (((contextPtr->cuOriginY & 63) * 32) + (contextPtr->cuOriginX & 63)) >> 1;

                //need these for L7 Chroma: TBCleaned.
                contextPtr->cuDepth = cuStatsPtr->depth;
                contextPtr->cuSize = cuStatsPtr->size;
                contextPtr->lcuSize = MAX_LCU_SIZE;
                contextPtr->lcuChromaSize = contextPtr->lcuSize >> 1;
                contextPtr->cuSizeLog2 = cuStatsPtr->sizeLog2;
                contextPtr->cuChromaOriginX = contextPtr->cuOriginX >> 1;
		        contextPtr->cuChromaOriginY = contextPtr->cuOriginY >> 1;
                cuPtr->leafIndex = leafIndex;
                cuPtr->qp = contextPtr->qp;
                candidateBufferPtrArray = &(candidateBufferPtrArrayBase[contextPtr->bufferDepthIndexStart[cuStatsPtr->depth]]);

				// Set PF Mode - should be done per TU (and not per CU) to avoid the correction
				DerivePartialFrequencyN2Flag(
					pictureControlSetPtr,
					contextPtr);

                // Initialize Fast Loop
                ProductCodingLoopInitFastLoop( // HT to be rechecked especially for fullCostArray
                    contextPtr,
                    contextPtr->intraLumaModeNeighborArray,
                    contextPtr->skipFlagNeighborArray,
                    contextPtr->modeTypeNeighborArray,
                    contextPtr->leafDepthNeighborArray);

                // Perform MPM search
                if (contextPtr->mpmSearch) {
                    DeriveMpmModes( // HT done
                        contextPtr,
                        cuPtr);
                }

				SetNmm(
					contextPtr,
                    BDP_MVMERGE_STAGE);

                SetNfl(
                    lcuPtr,
                    contextPtr,
                    pictureControlSetPtr,
                    BDP_MVMERGE_STAGE);

                contextPtr->conformantMvMergeTable = EB_TRUE;

                ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
                    lcuPtr,
                    contextPtr,
					leafIndex,
                    lcuAddr,
                    &bufferTotalCount,
                    &fastCandidateTotalCount,
                    (void*)contextPtr->interPredictionContext,
                    pictureControlSetPtr,
                    contextPtr->mpmSearch,
                    contextPtr->mpmSearchCandidate,
                    contextPtr->mostProbableModeArray);


                //if we want to recon N candidate, we would need N+1 buffers
                maxBuffers = MIN((bufferTotalCount + 1), contextPtr->bufferDepthIndexWidth[cuStatsPtr->depth]);

                ProductPerformFastLoop(
                    pictureControlSetPtr,
                    lcuPtr,
                    contextPtr,
                    candidateBufferPtrArrayBase,
                    fastCandidateArray,
                    fastCandidateTotalCount,
                    inputPicturePtr,
                    inputOriginIndex,
                    inputCbOriginIndex,
                    inputCbOriginIndex,
                    cuPtr,
                    cuOriginIndex,
                    cuChromaOriginIndex,
                    maxBuffers,
                    &secondFastCostSearchCandidateTotalCount);

                // Make sure bufferTotalCount is not larger than the number of fast modes
                bufferTotalCount = MIN(secondFastCostSearchCandidateTotalCount, bufferTotalCount);

                // PreModeDecision
                // -Input is the buffers
                // -Output is list of buffers for full reconstruction
				EB_U8 disableMergeIndex = 0;

                PreModeDecision(
                    cuPtr,
                    (secondFastCostSearchCandidateTotalCount == bufferTotalCount) ? bufferTotalCount : maxBuffers,
                    candidateBufferPtrArray,
                    &fullCandidateTotalCount,
                    contextPtr->bestCandidateIndexArray,
					&disableMergeIndex,
                    (EB_BOOL)(secondFastCostSearchCandidateTotalCount == bufferTotalCount)); // The fast loop bug fix is now added to 4K only

                PerformFullLoop(
                    pictureControlSetPtr,
                    lcuPtr,
                    cuPtr,
                    contextPtr,
                    inputPicturePtr,
                    inputOriginIndex,
                    inputCbOriginIndex,
                    cuOriginIndex,
                    cuChromaOriginIndex,
                    MIN(fullCandidateTotalCount, bufferTotalCount)); // fullCandidateTotalCount to number of buffers to process  

                // Full Mode Decision (choose the best mode)
                candidateIndex = ProductFullModeDecision(
                    contextPtr,
                    cuPtr,
                    contextPtr->cuStats->size,
                    candidateBufferPtrArray,
                    fullCandidateTotalCount,
                    contextPtr->bestCandidateIndexArray,
                    &bestIntraMode);

                candidateBuffer = candidateBufferPtrArray[candidateIndex];

                bestCandidateBuffers[contextPtr->cuStats->depth] = candidateBuffer;

                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                = candidateBuffer->yCoeffBits;
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[0]                    = candidateBuffer->yDc[0];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[1]                    = candidateBuffer->yDc[1];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[2]                    = candidateBuffer->yDc[2];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yDc[3]                    = candidateBuffer->yDc[3];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[0]    = candidateBuffer->yCountNonZeroCoeffs[0];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[1]    = candidateBuffer->yCountNonZeroCoeffs[1];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[2]    = candidateBuffer->yCountNonZeroCoeffs[2];
                contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[3]    = candidateBuffer->yCountNonZeroCoeffs[3];

				if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST && candidateBuffer->candidatePtr->type == INTER_MODE && candidateBuffer->candidatePtr->mergeFlag == EB_TRUE)
				{
						contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits                            = candidateBuffer->yCoeffBits;
						contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_RESIDUAL]   = candidateBuffer->yFullDistortion[DIST_CALC_RESIDUAL];
						contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yFullDistortion[DIST_CALC_PREDICTION] = candidateBuffer->yFullDistortion[DIST_CALC_PREDICTION];
						contextPtr->mdEpPipeLcu[cuPtr->leafIndex].yCbf                                  = candidateBuffer->candidatePtr->yCbf ;
						contextPtr->mdEpPipeLcu[cuPtr->leafIndex].fastLumaRate                          = candidateBuffer->candidatePtr->fastLumaRate;
				}

                PerformInverseTransformRecon(
                    contextPtr,
                    candidateBuffer,
                    cuPtr,
                    contextPtr->cuStats);

                // Update the CU Stats
                contextPtr->cuStats = GetCodedUnitStats(leafIndex);
                contextPtr->cuOriginX = lcuParamPtr->originX + contextPtr->cuStats->originX;
                contextPtr->cuOriginY = lcuParamPtr->originY + contextPtr->cuStats->originY;

                // Set the MvUnit
                contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
                contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
                contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

                // Set the candidate buffer
                contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];
                candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];

                // Update Neighbor Arrays
                EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
                EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
                EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;
                    
                MvMergePassUpdateNeighborArrays(
                    pictureControlSetPtr,
                    contextPtr->leafDepthNeighborArray,
                    contextPtr->modeTypeNeighborArray,
                    contextPtr->intraLumaModeNeighborArray,
                    contextPtr->mvNeighborArray,
                    contextPtr->skipFlagNeighborArray,
                    contextPtr->lumaReconNeighborArray,
                    contextPtr->cbReconNeighborArray,
                    contextPtr->crReconNeighborArray,
                    candidateBuffer->reconPtr,
                    MAX_LCU_SIZE,
                    contextPtr->intraMdOpenLoopFlag,
                    (EB_U8*)&contextPtr->cuStats->depth,
                    &predictionModeFlag,
                    &intraLumaMode,
                    &contextPtr->mvUnit,
                    &skipFlag,
                    contextPtr->cuOriginX,
                    contextPtr->cuOriginY,
                    contextPtr->cuStats->size,
                    tileIdx,
                    contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);

					if (contextPtr->intraMdOpenLoopFlag == EB_FALSE)
					{

						UpdateMdReconBuffer(
							candidateBuffer->reconPtr,
							contextPtr->mdReconBuffer,
							contextPtr,
							lcuPtr);

					}

			}
            else {

                // Update the CU Stats
                contextPtr->cuStats = GetCodedUnitStats(leafIndex);
                contextPtr->cuOriginX = lcuParamPtr->originX + contextPtr->cuStats->originX;
                contextPtr->cuOriginY = lcuParamPtr->originY + contextPtr->cuStats->originY;

                // Set the MvUnit
                contextPtr->mvUnit.predDirection = (EB_U8)(&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->interPredDirectionIndex;
                contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_0].mvUnion;
                contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = (&lcuPtr->codedLeafArrayPtr[leafIndex]->predictionUnitArray[0])->mv[REF_LIST_1].mvUnion;

                // Set the candidate buffer
                contextPtr->cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];
                candidateBuffer = bestCandidateBuffers[contextPtr->cuStats->depth];


                // Update Neighbor Arrays
                EB_U8 predictionModeFlag = (EB_U8)contextPtr->cuPtr->predictionModeFlag;
                EB_U8 intraLumaMode = (EB_U8)(&contextPtr->cuPtr->predictionUnitArray[0])->intraLumaMode;
                EB_U8 skipFlag = (EB_U8)contextPtr->cuPtr->skipFlag;

                MvMergePassUpdateNeighborArrays(
                    pictureControlSetPtr,
                    contextPtr->leafDepthNeighborArray,
                    contextPtr->modeTypeNeighborArray,
                    contextPtr->intraLumaModeNeighborArray,
                    contextPtr->mvNeighborArray,
                    contextPtr->skipFlagNeighborArray,
                    contextPtr->lumaReconNeighborArray,
                    contextPtr->cbReconNeighborArray,
                    contextPtr->crReconNeighborArray,
                    contextPtr->mdReconBuffer,
                    MAX_LCU_SIZE,
                    contextPtr->intraMdOpenLoopFlag,
                    (EB_U8*)&contextPtr->cuStats->depth,
                    &predictionModeFlag,
                    &intraLumaMode,
                    &contextPtr->mvUnit,
                    &skipFlag,
                    contextPtr->cuOriginX,
                    contextPtr->cuOriginY,
                    contextPtr->cuStats->size,
                    tileIdx,
                    contextPtr->useChromaInformationInFullLoop ? EB_TRUE : EB_FALSE);
            }
            leafIndex += DepthOffset[contextPtr->cuStats->depth];
        }
        else {
            leafIndex++;
        }
    }
    return return_error;
}

