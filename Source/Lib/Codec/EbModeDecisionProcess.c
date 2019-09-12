/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>
#include "EbUtility.h"

#include "EbModeDecisionProcess.h"

#include "EbLambdaRateTables.h"
#include "EbTransforms.h"


/******************************************************
 * Mode Decision Context Constructor
 ******************************************************/
EB_ERRORTYPE ModeDecisionContextCtor(
    ModeDecisionContext_t  **contextDblPtr,
    EbFifo_t                *modeDecisionConfigurationInputFifoPtr,
    EbFifo_t                *modeDecisionOutputFifoPtr,
    EB_BOOL                  is16bit)
{
    EB_U32 bufferIndex;
    EB_U32 candidateIndex;
    EB_ERRORTYPE return_error = EB_ErrorNone;

    ModeDecisionContext_t *contextPtr;
    EB_MALLOC(ModeDecisionContext_t*, contextPtr, sizeof(ModeDecisionContext_t), EB_N_PTR);
    *contextDblPtr = contextPtr;

    // Input/Output System Resource Manager FIFOs
    contextPtr->modeDecisionConfigurationInputFifoPtr = modeDecisionConfigurationInputFifoPtr;
    contextPtr->modeDecisionOutputFifoPtr             = modeDecisionOutputFifoPtr;

    // Trasform Scratch Memory
    EB_MALLOC(EB_S16*, contextPtr->transformInnerArrayPtr, 3120, EB_N_PTR); //refer to EbInvTransform_SSE2.as. case 32x32

    // MD rate Estimation tables
    EB_MALLOC(MdRateEstimationContext_t*, contextPtr->mdRateEstimationPtr, sizeof(MdRateEstimationContext_t), EB_N_PTR);

    // Fast Candidate Array
    EB_MALLOC(ModeDecisionCandidate_t*, contextPtr->fastCandidateArray, sizeof(ModeDecisionCandidate_t) * MODE_DECISION_CANDIDATE_MAX_COUNT, EB_N_PTR);

    EB_MALLOC(ModeDecisionCandidate_t**, contextPtr->fastCandidatePtrArray, sizeof(ModeDecisionCandidate_t*) * MODE_DECISION_CANDIDATE_MAX_COUNT, EB_N_PTR);
    
    for(candidateIndex = 0; candidateIndex < MODE_DECISION_CANDIDATE_MAX_COUNT; ++candidateIndex) {
        contextPtr->fastCandidatePtrArray[candidateIndex] = &contextPtr->fastCandidateArray[candidateIndex];
        contextPtr->fastCandidatePtrArray[candidateIndex]->mdRateEstimationPtr  = contextPtr->mdRateEstimationPtr;
    }

    // Transform and Quantization Buffers
    EB_MALLOC(EbTransQuantBuffers_t*, contextPtr->transQuantBuffersPtr, sizeof(EbTransQuantBuffers_t), EB_N_PTR);

	// Cabac cost
    EB_MALLOC(CabacCost_t*, contextPtr->CabacCost, sizeof(CabacCost_t), EB_N_PTR);

    return_error = EbTransQuantBuffersCtor(
        contextPtr->transQuantBuffersPtr);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Cost Arrays
    EB_MALLOC(EB_U64*, contextPtr->fastCostArray, sizeof(EB_U64) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

    EB_MALLOC(EB_U64*, contextPtr->fullCostArray, sizeof(EB_U64) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

    EB_MALLOC(EB_U64*, contextPtr->fullCostSkipPtr, sizeof(EB_U64) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

    EB_MALLOC(EB_U64*, contextPtr->fullCostMergePtr, sizeof(EB_U64) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

    // Candidate Buffers
    EB_MALLOC(ModeDecisionCandidateBuffer_t**, contextPtr->candidateBufferPtrArray, sizeof(ModeDecisionCandidateBuffer_t*) * MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT, EB_N_PTR);

    for(bufferIndex = 0; bufferIndex < MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT; ++bufferIndex) {
        return_error = ModeDecisionCandidateBufferCtor(
            &(contextPtr->candidateBufferPtrArray[bufferIndex]),
            MAX_LCU_SIZE,
            EB_8BIT,
            &(contextPtr->fastCostArray[bufferIndex]),
            &(contextPtr->fullCostArray[bufferIndex]),
            &(contextPtr->fullCostSkipPtr[bufferIndex]),
            &(contextPtr->fullCostMergePtr[bufferIndex]));
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Inter Prediction Context
    return_error = InterPredictionContextCtor(
        &contextPtr->interPredictionContext,
        MAX_LCU_SIZE,
        MAX_LCU_SIZE,
        is16bit);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Prediction Buffer
    {
        EbPictureBufferDescInitData_t initData;

		initData.bufferEnableMask = PICTURE_BUFFER_DESC_LUMA_MASK;
        initData.maxWidth          = MAX_LCU_SIZE;
        initData.maxHeight         = MAX_LCU_SIZE;
        initData.bitDepth          = EB_8BIT;
        initData.colorFormat       = EB_YUV420;
		initData.leftPadding	   = 0;
		initData.rightPadding      = 0;
		initData.topPadding        = 0;
		initData.botPadding        = 0;
        initData.splitMode         = EB_FALSE;

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*) &contextPtr->predictionBuffer,
            (EB_PTR) &initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

    }

    // Intra Reference Samples
    return_error = IntraReferenceSamplesCtor(&contextPtr->intraRefPtr, EB_YUV420);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // MCP Context
    return_error = MotionCompensationPredictionContextCtor(
        &contextPtr->mcpContext,
        MAX_LCU_SIZE,
        MAX_LCU_SIZE,
        is16bit);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    {
        EbPictureBufferDescInitData_t initData;

        initData.bufferEnableMask  = PICTURE_BUFFER_DESC_FULL_MASK;
        initData.maxWidth          = MAX_LCU_SIZE;
        initData.maxHeight         = MAX_LCU_SIZE;
        initData.bitDepth          = EB_8BIT;
        initData.colorFormat       = EB_YUV420;
		initData.leftPadding	   = 0;
		initData.rightPadding      = 0;
		initData.topPadding        = 0;
		initData.botPadding        = 0;
        initData.splitMode         = EB_FALSE;

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&contextPtr->pillarReconBuffer,
            (EB_PTR)&initData);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*) &contextPtr->mdReconBuffer,
            (EB_PTR) &initData);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }


    EB_MALLOC(LcuBasedDetectors_t*, contextPtr->mdPicLcuDetect, sizeof(LcuBasedDetectors_t), EB_N_PTR);
    EB_MEMSET(contextPtr->mdPicLcuDetect,0,sizeof(LcuBasedDetectors_t));

    return EB_ErrorNone;
}



extern void lambdaAssignLowDelay(
    PictureParentControlSet_t *pictureControlSetPtr,
	EB_U32                    *fastLambda,
	EB_U32                    *fullLambda,
	EB_U32                    *fastChromaLambda,
	EB_U32                    *fullChromaLambda,
	EB_U32                    *fullChromaLambdaSao,
	EB_U8                      qp,
	EB_U8                      chromaQp)

{

    if (pictureControlSetPtr->temporalLayerIndex == 0) {

		*fastLambda              = lambdaModeDecisionLdSad[qp];
		*fastChromaLambda        = lambdaModeDecisionLdSad[qp];
        *fullLambda              = lambdaModeDecisionLdSse[qp];
        *fullChromaLambda        = lambdaModeDecisionLdSse[qp];
        *fullChromaLambdaSao     = lambdaModeDecisionLdSse[chromaQp];

    }
    else { // Hierarchical postions 1, 2, 3, 4, 5

		*fastLambda				 = lambdaModeDecisionLdSadQpScaling[qp];
		*fastChromaLambda		 = lambdaModeDecisionLdSadQpScaling[qp];
        *fullLambda              = lambdaModeDecisionLdSseQpScaling[qp];
        *fullChromaLambda        = lambdaModeDecisionLdSseQpScaling[qp];
        *fullChromaLambdaSao     = lambdaModeDecisionLdSseQpScaling[chromaQp];
    }

}

void lambdaAssignRandomAccess(
    PictureParentControlSet_t *pictureControlSetPtr,
	EB_U32                    *fastLambda,
	EB_U32                    *fullLambda,
	EB_U32                    *fastChromaLambda,
	EB_U32                    *fullChromaLambda,
	EB_U32                    *fullChromaLambdaSao,
	EB_U8                      qp,
	EB_U8                      chromaQp)

{
    if (pictureControlSetPtr->temporalLayerIndex == 0) {

        *fastLambda             = lambdaModeDecisionRaSadBase[qp];
        *fastChromaLambda       = lambdaModeDecisionRaSadBase[qp];
        *fullLambda             = lambdaModeDecisionRaSseBase[qp];
        *fullChromaLambda       = lambdaModeDecisionRaSseBase[qp];
        *fullChromaLambdaSao    = lambdaModeDecisionRaSseBase[chromaQp];

    }
    else if (pictureControlSetPtr->isUsedAsReferenceFlag) {

        *fastLambda             = lambdaModeDecisionRaSadRefNonBase[qp];
        *fastChromaLambda       = lambdaModeDecisionRaSadRefNonBase[qp];
        *fullLambda             = lambdaModeDecisionRaSseRefNonBase[qp];
        *fullChromaLambda       = lambdaModeDecisionRaSseRefNonBase[qp];
        *fullChromaLambdaSao    = lambdaModeDecisionRaSseRefNonBase[chromaQp];
    }
    else {

        *fastLambda             = lambdaModeDecisionRaSadNonRef[qp];
        *fastChromaLambda       = lambdaModeDecisionRaSadNonRef[qp];
        *fullLambda             = lambdaModeDecisionRaSseNonRef[qp];
        *fullChromaLambda       = lambdaModeDecisionRaSseNonRef[qp];
        *fullChromaLambdaSao    = lambdaModeDecisionRaSseNonRef[chromaQp];
    }

}

void lambdaAssignISlice(
    PictureParentControlSet_t *pictureControlSetPtr,
	EB_U32                    *fastLambda,
	EB_U32                    *fullLambda,
	EB_U32                    *fastChromaLambda,
	EB_U32                    *fullChromaLambda,
	EB_U32                    *fullChromaLambdaSao,
	EB_U8                      qp,
	EB_U8                      chromaQp)

{

    if (pictureControlSetPtr->temporalLayerIndex == 0) {

		*fastLambda              = lambdaModeDecisionISliceSad[qp];
		*fastChromaLambda        = lambdaModeDecisionISliceSad[qp];
        *fullLambda              = lambdaModeDecisionISliceSse[qp];
        *fullChromaLambda        = lambdaModeDecisionISliceSse[qp];
        *fullChromaLambdaSao     = lambdaModeDecisionISliceSse[chromaQp];

    }
    else {

    }

}
const EB_LAMBDA_ASSIGN_FUNC lambdaAssignmentFunctionTable[4]  = {
    lambdaAssignLowDelay,		// low delay P
    lambdaAssignLowDelay,		// low delay B
    lambdaAssignRandomAccess,	// Random Access
    lambdaAssignISlice			// I_SLICE
};

void ProductResetModeDecision(
    ModeDecisionContext_t   *contextPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr)
{
	EB_PICTURE                     sliceType;
	MdRateEstimationContext_t   *mdRateEstimationArray;
	
	// SAO
	pictureControlSetPtr->saoFlag[0] = EB_TRUE;
	pictureControlSetPtr->saoFlag[1] = EB_TRUE;

	// QP
	contextPtr->qp = pictureControlSetPtr->pictureQp;
	// Asuming cb and cr offset to be the same for chroma QP in both slice and pps for lambda computation

	EB_U8 qpScaled = CLIP3(MIN_QP_VALUE, MAX_CHROMA_MAP_QP_VALUE, (EB_S32)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
	contextPtr->chromaQp = MapChromaQp(qpScaled);

	if (pictureControlSetPtr->sliceType == EB_I_PICTURE && pictureControlSetPtr->temporalId == 0){

		(*lambdaAssignmentFunctionTable[3])(
            pictureControlSetPtr->ParentPcsPtr,
			&contextPtr->fastLambda,
			&contextPtr->fullLambda,
			&contextPtr->fastChromaLambda,
			&contextPtr->fullChromaLambda,
			&contextPtr->fullChromaLambdaSao,
			contextPtr->qp,
			contextPtr->chromaQp);
	}
	else{
		(*lambdaAssignmentFunctionTable[sequenceControlSetPtr->staticConfig.predStructure])(
            pictureControlSetPtr->ParentPcsPtr,
			&contextPtr->fastLambda,
			&contextPtr->fullLambda,
			&contextPtr->fastChromaLambda,
			&contextPtr->fullChromaLambda,
			&contextPtr->fullChromaLambdaSao,
			contextPtr->qp,
			contextPtr->chromaQp);
	}
	// Configure the number of candidate buffers to search at each depth

    // 64x64 CU
    contextPtr->bufferDepthIndexStart[0] = 0;
	contextPtr->bufferDepthIndexWidth[0] = 5; // 4 NFL + 1 for temporary data

    // 32x32 CU
    contextPtr->bufferDepthIndexStart[1] = contextPtr->bufferDepthIndexStart[0] + contextPtr->bufferDepthIndexWidth[0];
	contextPtr->bufferDepthIndexWidth[1] = 8; // 4 NFL + 3 MPM + 1 for temporary data

    // 16x16 CU
    contextPtr->bufferDepthIndexStart[2] = contextPtr->bufferDepthIndexStart[1] + contextPtr->bufferDepthIndexWidth[1];
	contextPtr->bufferDepthIndexWidth[2] = 8; // 4 NFL + 3 MPM + 1 for temporary data

    // 8x8 CU
    contextPtr->bufferDepthIndexStart[3] = contextPtr->bufferDepthIndexStart[2] + contextPtr->bufferDepthIndexWidth[2];
	contextPtr->bufferDepthIndexWidth[3] = 8; // 4 NFL + 3 MPM + 1 for temporary data

    // 4x4 CU
    contextPtr->bufferDepthIndexStart[4] = contextPtr->bufferDepthIndexStart[3] + contextPtr->bufferDepthIndexWidth[3];
	contextPtr->bufferDepthIndexWidth[4] = 5; // 4 NFL + 1 for temporary data

	// Slice Type
	sliceType =
		(pictureControlSetPtr->ParentPcsPtr->idrFlag == EB_TRUE) ? EB_I_PICTURE :
		pictureControlSetPtr->sliceType;

	// Increment the MD Rate Estimation array pointer to point to the right address based on the QP and slice type

	/* Note(CHKN) : Rate estimation will use FrameQP even when Qp modulation is ON */

	mdRateEstimationArray = (MdRateEstimationContext_t*)sequenceControlSetPtr->encodeContextPtr->mdRateEstimationArray;
	mdRateEstimationArray += sliceType * TOTAL_NUMBER_OF_QP_VALUES + contextPtr->qp;

	// Reset MD rate Estimation table to initial values by copying from mdRateEstimationArray
	contextPtr->mdRateEstimationPtr = mdRateEstimationArray;

	EB_U32  candidateIndex;
	for (candidateIndex = 0; candidateIndex < MODE_DECISION_CANDIDATE_MAX_COUNT; ++candidateIndex) {
		contextPtr->fastCandidatePtrArray[candidateIndex]->mdRateEstimationPtr = mdRateEstimationArray;
	}

	// TMVP Map Writer Pointer
	if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
		contextPtr->referenceObjectWritePtr = (EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr;
	else
		contextPtr->referenceObjectWritePtr = (EbReferenceObject_t*)EB_NULL;

	// Reset CABAC Contexts
	contextPtr->coeffEstEntropyCoderPtr = pictureControlSetPtr->coeffEstEntropyCoderPtr;

	return;
}

void ConfigureChroma(
    PictureControlSet_t			   *pictureControlSetPtr,
    ModeDecisionContext_t		   *contextPtr,
    LargestCodingUnit_t            *lcuPtr) {

    EB_U32     lcuAddr = lcuPtr->index;
    EB_U32     lcuEdgeNum = pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum;
    LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuAddr]);

    lcuPtr->chromaEncodeMode = CHROMA_MODE_FULL;

    EB_BOOL chromaCond0 = lcuStatPtr->stationaryEdgeOverTimeFlag;
    EB_BOOL chromaCond1 = pictureControlSetPtr->ParentPcsPtr->lcuHomogeneousAreaArray[lcuAddr] && lcuStatPtr->cuStatArray[0].highChroma;
    EB_BOOL chromaCond2 = !lcuStatPtr->cuStatArray[0].highLuma;
    EB_BOOL chromaCond3 = ((pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture > 60) || (lcuPtr->auraStatus == AURA_STATUS_1) || (pictureControlSetPtr->ParentPcsPtr->isPan));

    // 0: Full Search Chroma for All 
    // 1: Best Search Chroma for All LCUs; Chroma OFF if I_SLICE, Chroma for only MV_Merge if P/B_SLICE
    // 2: Full vs. Best Swicth Method 0: chromaCond0 || chromaCond1 || chromaCond2
    // 3: Full vs. Best Swicth Method 1: chromaCond0 || chromaCond1
    // 4: Full vs. Best Swicth Method 2: chromaCond2 || chromaCond3
    // 5: Full vs. Best Swicth Method 3: chromaCond0
    // If INTRA Close Loop, then the switch modes (2,3,4,5) are not supported as reference samples for Chroma compensation will be a mix of source samples and reconstructed samples

    if (contextPtr->chromaLevel == 0) {
        lcuPtr->chromaEncodeMode = CHROMA_MODE_FULL;
    }
    else if (contextPtr->chromaLevel == 1) {
        lcuPtr->chromaEncodeMode = CHROMA_MODE_BEST;
    }
    else if (contextPtr->chromaLevel == 2) {
        lcuPtr->chromaEncodeMode = (chromaCond0 || chromaCond1 || chromaCond2) ?
            (EB_U8)CHROMA_MODE_FULL :
            (EB_U8)CHROMA_MODE_BEST;
    }
    else if (contextPtr->chromaLevel == 3) {
        lcuPtr->chromaEncodeMode = (chromaCond0 || chromaCond1) ?
            (EB_U8)CHROMA_MODE_FULL :
            (EB_U8)CHROMA_MODE_BEST;
    }
    else if (contextPtr->chromaLevel == 4) {
        lcuPtr->chromaEncodeMode = chromaCond2 || chromaCond3 ?
            (EB_U8)CHROMA_MODE_FULL :
            (EB_U8)CHROMA_MODE_BEST;
    }
    else {
        lcuPtr->chromaEncodeMode = chromaCond0 ?
            (EB_U8)CHROMA_MODE_FULL :
            (EB_U8)CHROMA_MODE_BEST;
    }

    // hack disable chroma search for P422/444 for known error
    if (lcuPtr->chromaEncodeMode == CHROMA_MODE_FULL && pictureControlSetPtr->colorFormat >= EB_YUV422) {
        lcuPtr->chromaEncodeMode = CHROMA_MODE_BEST;
    }

    contextPtr->useChromaInformationInFastLoop = (lcuEdgeNum > 0 || lcuPtr->chromaEncodeMode == CHROMA_MODE_FULL) ? 1 : 0;

    contextPtr->useChromaInformationInFullLoop = (lcuPtr->chromaEncodeMode == CHROMA_MODE_FULL) ? 1 : 0;

    if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST) {
        contextPtr->useChromaInformationInFastLoop = 0;
        contextPtr->useChromaInformationInFullLoop = 0;
    }

    contextPtr->use4x4ChromaInformationInFullLoop = contextPtr->useChromaInformationInFullLoop && lcuPtr->intra4x4SearchMethod == INTRA4x4_INLINE_SEARCH  && contextPtr->intraMdOpenLoopFlag == EB_FALSE ? EB_TRUE : EB_FALSE;
}


void DeriveIntraInterBiasFlag(
    SequenceControlSet_t           *sequenceControlSetPtr,
    PictureControlSet_t			   *pictureControlSetPtr,
    ModeDecisionContext_t		   *contextPtr,
    LargestCodingUnit_t            *lcuPtr) {


    EB_U32     lcuAddr = lcuPtr->index;
    LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuAddr]);

    contextPtr->useIntraInterBias = EB_FALSE;

    if (sequenceControlSetPtr->staticConfig.improveSharpness) {

        contextPtr->useIntraInterBias = EB_TRUE;

        if (lcuPtr->pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_2) {
            if (lcuPtr->pictureControlSetPtr->ParentPcsPtr->lcuCmplxContrastArray[lcuAddr]) {
                contextPtr->useIntraInterBias = EB_FALSE;
            }
        }

        if (lcuPtr->pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_1) {
            if (lcuPtr->pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum) {
                contextPtr->useIntraInterBias = EB_FALSE;
            }
        }

        if (lcuStatPtr->stationaryEdgeOverTimeFlag)
            contextPtr->useIntraInterBias = EB_FALSE;

        if (pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_3_1) {

            contextPtr->useIntraInterBias = EB_FALSE;
        }

        if (pictureControlSetPtr->ParentPcsPtr->yMean[lcuAddr][RASTER_SCAN_CU_INDEX_64x64] < TH_BIAS_DARK_LCU) {
            contextPtr->useIntraInterBias = EB_FALSE;
        }

    }

    if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE) {
        contextPtr->useIntraInterBias = EB_FALSE;
    }
}

void ProductConfigurePicLcuMdDetectors(
    PictureControlSet_t		*pictureControlSetPtr,
    ModeDecisionContext_t	*contextPtr,
    LargestCodingUnit_t     *lcuPtr)

{

    LcuBasedDetectors_t           *mdPicLcuDetect = contextPtr->mdPicLcuDetect;
    EB_U8                          index;
    EB_U16                         lcuAddr = lcuPtr->index;
    LcuStat_t                     *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuAddr]);
    PictureParentControlSet_t     *ppcsPtr = pictureControlSetPtr->ParentPcsPtr;


    mdPicLcuDetect->intraInterCond1 = (ppcsPtr->grassPercentageInPicture > 60 && lcuPtr->auraStatus == AURA_STATUS_1) || ppcsPtr->failingMotionLcuFlag[lcuAddr] ? EB_FALSE : EB_TRUE;
    mdPicLcuDetect->intraInterCond2 = (ppcsPtr->lcuFlatNoiseArray[lcuAddr] &&
        ppcsPtr->similarColocatedLcuArrayAllLayers[lcuAddr] == EB_TRUE &&
        (ppcsPtr->failingMotionLcuFlag[lcuAddr] || ppcsPtr->similarColocatedLcuArray[lcuAddr] || ppcsPtr->temporalLayerIndex > 0)) ? EB_TRUE : EB_FALSE;


    mdPicLcuDetect->intraInterCond3 = EB_FALSE;

    EB_U8 cuUseRefSrc = (pictureControlSetPtr->ParentPcsPtr->useSrcRef);

    if (pictureControlSetPtr->temporalLayerIndex == 0 || cuUseRefSrc) {
        if (pictureControlSetPtr->ParentPcsPtr->isLcuHomogeneousOverTime[lcuAddr] &&
            pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum == 0
            && (!lcuStatPtr->cuStatArray[0].skinArea) && (!ppcsPtr->failingMotionLcuFlag[lcuAddr]) && (ppcsPtr->lcuFlatNoiseArray[lcuAddr])) {

            mdPicLcuDetect->intraInterCond3 = EB_TRUE;
        }
    }


    if (ppcsPtr->lcuHomogeneousAreaArray[lcuAddr] || (&(lcuStatPtr->cuStatArray[0]))->grassArea)

        mdPicLcuDetect->biPredCond1 = EB_TRUE;
    else
        mdPicLcuDetect->biPredCond1 = EB_FALSE;

    // AntiContouring Threasholds
    for (index = 0; index < 4; index++) {
        mdPicLcuDetect->enableContouringQCUpdateFlag32x32[index] =
            (ppcsPtr->lcuYSrcEnergyCuArray[lcuPtr->index][index + 1] == 0 ||
                ppcsPtr->lcuYSrcMeanCuArray[lcuPtr->index][index + 1] > ANTI_CONTOURING_LUMA_T2 ||
                ppcsPtr->lcuYSrcMeanCuArray[lcuPtr->index][index + 1] < ANTI_CONTOURING_LUMA_T1) ? 0 :
                (ppcsPtr->lcuYSrcEnergyCuArray[lcuPtr->index][index + 1] < ((32 * 32)*C1_TRSHLF_N / C1_TRSHLF_D)) ? 1 :
            (ppcsPtr->lcuYSrcEnergyCuArray[lcuPtr->index][index + 1] < ((32 * 32)*C2_TRSHLF_N / C2_TRSHLF_D)) ? 2 : 0;
    }
}


void ConfigureMpm(
    ModeDecisionContext_t	*contextPtr)
{
    if (contextPtr->restrictIntraGlobalMotion) {
        contextPtr->mpmSearch = EB_FALSE;
    }
    else if (contextPtr->mpmLevel == 0) {
        contextPtr->mpmSearch = EB_TRUE;
        contextPtr->mpmSearchCandidate = MAX_MPM_CANDIDATES;
    }
    else if (contextPtr->mpmLevel == 1) {
        contextPtr->mpmSearch = EB_TRUE;
        contextPtr->mpmSearchCandidate = 1;
    }
    else  {
        contextPtr->mpmSearch = EB_FALSE;
    }
}

/******************************************************
* Derive the INTRA4x4 Search Method:
  Input   : INTRA4x4 Level, Partitioning Method
  Output  : INTRA4x4 Search: in-line or as refinement or off
************************************************/
void DeriveIntra4x4SearchMethod(
    PictureControlSet_t    *pictureControlSetPtr,
    ModeDecisionContext_t  *contextPtr,
    LargestCodingUnit_t    *lcuPtr) {

    if (pictureControlSetPtr->ParentPcsPtr->lcuFlatNoiseArray[lcuPtr->index] == EB_FALSE) {

        // Set INTRA4x4 Search Level
        // Level    Settings 
        // 0        INLINE if not BDP, refinment otherwise 
        // 1        REFINMENT   
        // 2        OFF
        if (contextPtr->intra4x4Level == 0) {
            if ((pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL85_DEPTH_MODE ||
                pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL84_DEPTH_MODE ||
                (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuPtr->index] == LCU_FULL85_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuPtr->index] == LCU_FULL84_DEPTH_MODE)))) {
                lcuPtr->intra4x4SearchMethod = INTRA4x4_INLINE_SEARCH;
            }
            else {
                lcuPtr->intra4x4SearchMethod = INTRA4x4_REFINEMENT_SEARCH;
            }
        }
        else if (contextPtr->intra4x4Level == 1) {
            lcuPtr->intra4x4SearchMethod = INTRA4x4_REFINEMENT_SEARCH;
        }
        else {
            lcuPtr->intra4x4SearchMethod = INTRA4x4_OFF;
        }

    }
    else {
        lcuPtr->intra4x4SearchMethod = INTRA4x4_OFF;
    }
}

/******************************************************
* Derive the Depth Refinment level used in MD and BDP
Output  : depth Refinment
******************************************************/
void DeriveDepthRefinment(
    PictureControlSet_t      *pictureControlSetPtr,
    ModeDecisionContext_t    *contextPtr,
    LargestCodingUnit_t      *lcuPtr) {

    EB_U32      lcuAddr = lcuPtr->index;
    LcuStat_t  *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuAddr]);
             
    EB_U8       stationaryEdgeOverTimeFlag = lcuStatPtr->stationaryEdgeOverTimeFlag;

    contextPtr->depthRefinment = 0;

    // S-LOGO                                       
    if (stationaryEdgeOverTimeFlag > 0) {
        if (lcuStatPtr->lowDistLogo)
            contextPtr->depthRefinment = 1;
        else
            contextPtr->depthRefinment = 2;
    }


    if (pictureControlSetPtr->ParentPcsPtr->highDarkLowLightAreaDensityFlag && pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex > 0 && pictureControlSetPtr->ParentPcsPtr->sharpEdgeLcuFlag[lcuAddr] && !pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArrayAllLayers[lcuAddr]) {
        contextPtr->depthRefinment = 2;
    }

    if ((pictureControlSetPtr->ParentPcsPtr->sliceType != EB_I_PICTURE && (pictureControlSetPtr->ParentPcsPtr->logoPicFlag) &&
        pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuAddr].edgeBlockNum)) {
        contextPtr->depthRefinment = 2;
    }
}

/******************************************************
 * Mode Decision Configure LCU
 ******************************************************/
void ModeDecisionConfigureLcu(
    ModeDecisionContext_t   *contextPtr,
    LargestCodingUnit_t     *lcuPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr,
    EB_U8                    pictureQp,
    EB_U8                    lcuQp)

{
    // Load INTRA4x4 Settings (should be before CHROMA)
    DeriveIntra4x4SearchMethod(
        pictureControlSetPtr,
        contextPtr,
        lcuPtr);

    // Load Chroma Settings
    ConfigureChroma(
        pictureControlSetPtr,
        contextPtr,
        lcuPtr);
   
    // Load MPM Settings
    ConfigureMpm(
        contextPtr);

    // Derive Depth Refinment Fla
    DeriveDepthRefinment(
        pictureControlSetPtr,
        contextPtr,
        lcuPtr);

    ProductConfigurePicLcuMdDetectors(
        pictureControlSetPtr,
        contextPtr,
        lcuPtr);

    DeriveIntraInterBiasFlag(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        contextPtr,
        lcuPtr);

    if (sequenceControlSetPtr->staticConfig.rateControlMode == 0 && sequenceControlSetPtr->staticConfig.improveSharpness == 0) { 
		contextPtr->qp = (EB_U8)pictureQp;
        lcuPtr->qp = (EB_U8)contextPtr->qp;
    }
    //RC is on
    else {
		contextPtr->qp = (EB_U8) lcuQp;
    }

	// Asuming cb and cr offset to be the same for chroma QP in both slice and pps for lambda computation

	EB_U8	qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
	contextPtr->chromaQp = MapChromaQp(qpScaled);

    /* Note(CHKN) : when Qp modulation varies QP on a sub-LCU(CU) basis,  Lamda has to change based on Cu->QP , and then this code has to move inside the CU loop in MD */

    // Lambda Assignement
    if(pictureControlSetPtr->sliceType == EB_I_PICTURE && pictureControlSetPtr->temporalId == 0){

        (*lambdaAssignmentFunctionTable[3])(
            pictureControlSetPtr->ParentPcsPtr,
            &contextPtr->fastLambda,
            &contextPtr->fullLambda,
            &contextPtr->fastChromaLambda,
            &contextPtr->fullChromaLambda,
            &contextPtr->fullChromaLambdaSao,
            contextPtr->qp,
            contextPtr->chromaQp);
    }
    else{

		// Change lambda QP for 4K 5L and 6L
		EB_U8 lambdaQp = contextPtr->qp;

		(*lambdaAssignmentFunctionTable[sequenceControlSetPtr->staticConfig.predStructure])(
            pictureControlSetPtr->ParentPcsPtr,
            &contextPtr->fastLambda,
            &contextPtr->fullLambda,
            &contextPtr->fastChromaLambda,
            &contextPtr->fullChromaLambda,
            &contextPtr->fullChromaLambdaSao,
			lambdaQp,
            contextPtr->chromaQp);
    }

    return;
}
