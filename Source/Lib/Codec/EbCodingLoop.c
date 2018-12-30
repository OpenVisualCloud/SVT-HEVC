/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbTransformUnit.h"
#include "EbRateDistortionCost.h"
#include "EbDeblockingFilter.h"
#include "EbSampleAdaptiveOffset.h"
#include "EbPictureOperators.h"

#include "EbModeDecisionProcess.h"
#include "EbEncDecProcess.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"
#include "EbComputeSAD.h"
#include "EbTransforms.h"
#include "EbModeDecisionConfiguration.h"
#include "emmintrin.h"

/*******************************************
* set Penalize Skip Flag
*
* Summary: Set the PenalizeSkipFlag to true
* When there is luminance/chrominance change
* or in noisy clip with low motion at meduim
* varince area
*
*******************************************/
typedef void (*EB_ENCODE_LOOP_FUNC_PTR)(
	EncDecContext_t				*contextPtr,
	LargestCodingUnit_t			*lcuPtr,
	EB_U32						 originX,            
	EB_U32						 originY,           
	EB_U32						 cbQp,              
	EbPictureBufferDesc_t		*predSamples,         // no basis/offset
	EbPictureBufferDesc_t		*coeffSamplesTB,      // lcu based
	EbPictureBufferDesc_t		*residual16bit,       // no basis/offset 
	EbPictureBufferDesc_t		*transform16bit,      // no basis/offset
	EB_S16						*transformScratchBuffer,
	EB_U32						*countNonZeroCoeffs,
	EB_U32						 useDeltaQp,
	CabacEncodeContext_t        *cabacEncodeCtxPtr,
	EB_U32						 intraLumaMode,
	CabacCost_t                 *CabacCost,
	EB_U32						 dZoffset) ;

typedef void (*EB_GENERATE_RECON_FUNC_PTR)(
    EncDecContext_t       *contextPtr,                                 
	EB_U32                 originX,                                    
	EB_U32                 originY,                                    
	EbPictureBufferDesc_t *predSamples,     // no basis/offset         
	EbPictureBufferDesc_t *residual16bit,    // no basis/offset        
	EB_S16                *transformScratchBuffer);                                   
             
typedef void (*EB_ENCODE_LOOP_INTRA_4x4_FUNC_PTR)(
	EncDecContext_t				*contextPtr,
	LargestCodingUnit_t			*lcuPtr,
	EB_U32						 originX,            
	EB_U32						 originY,           
	EB_U32						 cbQp,              
	EbPictureBufferDesc_t		*predSamples,         // no basis/offset
	EbPictureBufferDesc_t		*coeffSamplesTB,      // lcu based
	EbPictureBufferDesc_t		*residual16bit,       // no basis/offset 
	EbPictureBufferDesc_t		*transform16bit,      // no basis/offset
	EB_S16						*transformScratchBuffer,
	EB_U32						*countNonZeroCoeffs,
	EB_U32						 componentMask,
	EB_U32						 useDeltaQp, 
	CabacEncodeContext_t        *cabacEncodeCtxPtr,
	EB_U32                        intraLumaMode,
	CabacCost_t                 *CabacCost,
	EB_U32						  dZoffset) ;

typedef void (*EB_GENERATE_RECON_INTRA_4x4_FUNC_PTR)(
    EncDecContext_t       *contextPtr,                                 
	EB_U32                 originX,                                    
	EB_U32                 originY,                                    
	EbPictureBufferDesc_t *predSamples,     // no basis/offset         
	EbPictureBufferDesc_t *residual16bit,    // no basis/offset        
	EB_S16                *transformScratchBuffer,                     
	EB_U32                 componentMask);       

typedef EB_ERRORTYPE(*EB_GENERATE_INTRA_SAMPLES_FUNC_PTR)(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);
typedef EB_ERRORTYPE(*EB_ENC_PASS_INTRA_FUNC_PTR)( 
    void                       *referenceSamples,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      puSize,
    EbPictureBufferDesc_t      *predictionPtr,
    EB_U32                      lumaMode);
typedef EB_ERRORTYPE (*EB_LCU_INTERNAL_DLF_FUNC_PTR)(
	EbPictureBufferDesc_t *reconpicture,
    EB_U32                 lcuPosx,
    EB_U32                 lcuPosy,
    EB_U32                 lcuWidth,
    EB_U32                 lcuHeight,
    EB_U8                 *verticalEdgeBSArray,
    EB_U8                 *horizontalEdgeBSArray,
    PictureControlSet_t   *reconPictureControlSet);
typedef void (*EB_LCU_BOUNDARY_DLF_FUNC_PTR)(
	EbPictureBufferDesc_t *reconpicture,
    EB_U32                 lcuPos_x,                       
    EB_U32                 lcuPos_y,                       
    EB_U32                 lcuWidth,                       
    EB_U32                 lcuHeight,                      
    EB_U8                 *lcuVerticalEdgeBSArray,         
    EB_U8                 *lcuHorizontalEdgeBSArray,       
    EB_U8                 *topLcuVerticalEdgeBSArray,      
    EB_U8                 *leftLcuHorizontalEdgeBSArray,   
    PictureControlSet_t   *pictureControlSetPtr);
typedef void (*EB_LCU_PIC_EDGE_DLF_FUNC_PTR)(
    EbPictureBufferDesc_t *reconPic,                       
    EB_U32                 lcuIdx,
    EB_U32                 lcuPos_x,                       
    EB_U32                 lcuPos_y,                       
    EB_U32                 lcuWidth,                       
    EB_U32                 lcuHeight,                      
    PictureControlSet_t   *pictureControlSetPtr);  

void AddChromaEncDec(
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    CodingUnit_t            *cuPtr,
    ModeDecisionContext_t   *contextPtr,
    EncDecContext_t         *contextPtrED,
    EbPictureBufferDesc_t   *inputPicturePtr,
    EB_U32                   inputCbOriginIndex,
    EB_U32					 cuChromaOriginIndex,
    EB_U32                   candIdxInput);
/***************************************************
* Update Coding Unit Neighbor Arrays
***************************************************/
static void EncodePassUpdateLeafDepthNeighborArrays(
	NeighborArrayUnit_t     *leafDepthNeighborArray,
    EB_U8                    depth,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size)
{
	// Mode Type Update
	NeighborArrayUnitModeWrite(
		leafDepthNeighborArray,
        &depth,
		originX,
		originY,
		size,
		size,
		NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

	return;
}

/***************************************************
* Update Intra Mode Neighbor Arrays
***************************************************/
static void EncodePassUpdateIntraModeNeighborArrays(
	NeighborArrayUnit_t     *modeTypeNeighborArray,
	NeighborArrayUnit_t     *intraLumaModeNeighborArray,
    EB_U8                    lumaMode,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size)
{
	EB_U8 modeType = INTRA_MODE;

	// Mode Type Update
	NeighborArrayUnitModeWrite(
		modeTypeNeighborArray,
		&modeType,
		originX,
		originY,
		size,
		size,
		NEIGHBOR_ARRAY_UNIT_FULL_MASK);

	// Intra Luma Mode Update
	NeighborArrayUnitModeWrite(
		intraLumaModeNeighborArray,
        &lumaMode,
		originX,
		originY,
		size,
		size,
		NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

	return;
}

/***************************************************
* Update Inter Mode Neighbor Arrays
***************************************************/
static void EncodePassUpdateInterModeNeighborArrays(
    NeighborArrayUnit_t     *modeTypeNeighborArray,
    NeighborArrayUnit_t     *mvNeighborArray,
    NeighborArrayUnit_t     *skipNeighborArray,
    MvUnit_t                *mvUnit,
    EB_U8                   *skipFlag,
    EB_U32                   originX,
    EB_U32                   originY,
    EB_U32                   size)
{
    EB_U8 modeType = INTER_MODE;

    // Mode Type Update
    NeighborArrayUnitModeWrite(
        modeTypeNeighborArray,
        &modeType,
        originX,
        originY,
        size,
        size,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    // Motion Vector Unit
    NeighborArrayUnitModeWrite(
        mvNeighborArray,
        (EB_U8*)mvUnit,
        originX,
        originY,
        size,
        size,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    // Skip Flag
    NeighborArrayUnitModeWrite(
        skipNeighborArray,
        skipFlag,
        originX,
        originY,
        size,
        size,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    return;
}

/***************************************************
* Update Recon Samples Neighbor Arrays
***************************************************/
static void EncodePassUpdateReconSampleNeighborArrays(
    NeighborArrayUnit_t     *lumaReconSampleNeighborArray,
    NeighborArrayUnit_t     *cbReconSampleNeighborArray,
    NeighborArrayUnit_t     *crReconSampleNeighborArray,
    EbPictureBufferDesc_t   *reconBuffer,
    EB_U32                   originX,
    EB_U32                   originY,
    EB_U32                   size,
    EB_BOOL                  is16bit)
{
    if (is16bit == EB_TRUE){

        {
            // Recon Samples - Luma
            NeighborArrayUnit16bitSampleWrite(
                lumaReconSampleNeighborArray,
                (EB_U16*)(reconBuffer->bufferY),
                reconBuffer->strideY,
                reconBuffer->originX + originX,
                reconBuffer->originY + originY,
                originX,
                originY,
                size,
                size,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        }

        {
            // Recon Samples - Cb
            NeighborArrayUnit16bitSampleWrite(
                cbReconSampleNeighborArray,
                (EB_U16*)(reconBuffer->bufferCb),
                reconBuffer->strideCb,
                (reconBuffer->originX + originX) >> 1,
                (reconBuffer->originY + originY) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            // Recon Samples - Cr
            NeighborArrayUnit16bitSampleWrite(
                crReconSampleNeighborArray,
                (EB_U16*)(reconBuffer->bufferCr),
                reconBuffer->strideCr,
                (reconBuffer->originX + originX) >> 1,
                (reconBuffer->originY + originY) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        }

    }
    else {

        {
            // Recon Samples - Luma
            NeighborArrayUnitSampleWrite(
                lumaReconSampleNeighborArray,
                reconBuffer->bufferY,
                reconBuffer->strideY,
                reconBuffer->originX + originX,
                reconBuffer->originY + originY,
                originX,
                originY,
                size,
                size,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        }

        {
            // Recon Samples - Cb
            NeighborArrayUnitSampleWrite(
                cbReconSampleNeighborArray,
                reconBuffer->bufferCb,
                reconBuffer->strideCb,
                (reconBuffer->originX + originX) >> 1,
                (reconBuffer->originY + originY) >> 1,
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
                (reconBuffer->originX + originX) >> 1,
                (reconBuffer->originY + originY) >> 1,
                originX >> 1,
                originY >> 1,
                size >> 1,
                size >> 1,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        }
    }

    return;
}




/************************************************************
* Update Intra Luma Neighbor Modes
************************************************************/
void GeneratePuIntraLumaNeighborModes(
	CodingUnit_t            *cuPtr,
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

	(&cuPtr->predictionUnitArray[0])->intraLumaLeftMode = (EB_U32)(
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		(EB_U32)intraLumaNeighborArray->leftArray[intraLumaModeLeftNeighborIndex]);

	(&cuPtr->predictionUnitArray[0])->intraLumaTopMode = (EB_U32)(
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] != INTRA_MODE) ? EB_INTRA_DC :
		((puOriginY & (lcuSize - 1)) == 0) ? EB_INTRA_DC :                                         // If we are at the top of the LCU boundary, then
		(EB_U32)intraLumaNeighborArray->topArray[intraLumaModeTopNeighborIndex]);       //   use DC. This seems like we could use a LCU-width


	return;
}

/**********************************************************
* Encode Pass - Update Sao Parameter Neighbor Array
**********************************************************/
static void EncodePassUpdateSaoNeighborArrays(
	NeighborArrayUnit_t     *saoParamNeighborArray,
	SaoParameters_t         *saoParams,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size)
{
	NeighborArrayUnitModeWrite(
		saoParamNeighborArray,
		(EB_U8*)saoParams,
		originX,
		originY,
		size,
		size,
		NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

	return;
}

/**********************************************************
* Encode Loop
*
* Summary: Performs a H.265 conformant
*   Transform, Quantization  and Inverse Quantization of a TU.
*
* Inputs:
*   originX
*   originY
*   tuSize
*   lcuSize
*   input - input samples (position sensitive)
*   pred - prediction samples (position independent)
*
* Outputs:
*   Inverse quantized coeff - quantization indices (position sensitive)
*
**********************************************************/

static void EncodeLoop(
	EncDecContext_t       *contextPtr,
	LargestCodingUnit_t   *lcuPtr,
	EB_U32                 originX,          
	EB_U32                 originY,          
	EB_U32                 cbQp,
	EbPictureBufferDesc_t *predSamples,             // no basis/offset   
	EbPictureBufferDesc_t *coeffSamplesTB,          // lcu based
	EbPictureBufferDesc_t *residual16bit,           // no basis/offset
	EbPictureBufferDesc_t *transform16bit,          // no basis/offset
	EB_S16                *transformScratchBuffer,
	EB_U32				  *countNonZeroCoeffs,
	EB_U32				   useDeltaQp,
	CabacEncodeContext_t  *cabacEncodeCtxPtr,
	EB_U32                 intraLumaMode,			    
	CabacCost_t           *CabacCost,
	EB_U32                 dZoffset)  


{
    
    EB_U32                 chromaQp           = cbQp; 
    CodingUnit_t		  *cuPtr              = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr              = &cuPtr->transformUnitArray[contextPtr->tuItr]; 
    EB_SLICE               sliceType          = lcuPtr->pictureControlSetPtr->sliceType;
    EB_U32                 temporalLayerIndex = lcuPtr->pictureControlSetPtr->temporalLayerIndex;
    EB_U32                 qp                 = cuPtr->qp;
    EB_U32                 tuSize             = (contextPtr->cuStats->size == 64) ? 32 : contextPtr->cuStats->size;
    EbPictureBufferDesc_t  *inputSamples      = contextPtr->inputSamples;


	const EB_U32 inputLumaOffset    = ((originY + inputSamples->originY)          * inputSamples->strideY) + (originX + inputSamples->originX);
	const EB_U32 inputCbOffset      = (((originY + inputSamples->originY) >> 1)    * inputSamples->strideCb) + ((originX + inputSamples->originX) >> 1);
	const EB_U32 inputCrOffset      = (((originY + inputSamples->originY) >> 1)    * inputSamples->strideCr) + ((originX + inputSamples->originX) >> 1);
    const EB_U32 predLumaOffset     = (( predSamples->originY+originY)        * predSamples->strideY ) + ( predSamples->originX+originX);
	const EB_U32 predCbOffset       = (((predSamples->originY+originY) >> 1)  * predSamples->strideCb) + ((predSamples->originX+originX) >> 1);
	const EB_U32 predCrOffset       = (((predSamples->originY+originY) >> 1)  * predSamples->strideCr) + ((predSamples->originX+originX) >> 1);
    const EB_U32 scratchLumaOffset  = ((originY & (64 - 1))        * 64) + (originX  & (64 - 1));
	const EB_U32 scratchCbOffset    = (((originY & (64 - 1)) >> 1)   * 32) + ((originX & (64 - 1)) >> 1);
	const EB_U32 scratchCrOffset    = (((originY & (64 - 1)) >> 1)   * 32) + ((originX & (64 - 1)) >> 1);

    EB_U8 enableContouringQCUpdateFlag;

    enableContouringQCUpdateFlag = DeriveContouringClass(
        lcuPtr->pictureControlSetPtr->ParentPcsPtr,
        lcuPtr->index,
        cuPtr->leafIndex) && (cuPtr->qp < lcuPtr->pictureControlSetPtr->pictureQp);

	//**********************************
	// Luma
	//**********************************	
	{
        PictureResidual(
            inputSamples->bufferY + inputLumaOffset,
            inputSamples->strideY,
            predSamples->bufferY + predLumaOffset,
            predSamples->strideY,
            ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
            64,
            tuSize,
            tuSize);

        EstimateTransform(
            ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
            64,
            ((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
            64,
            tuSize,
            transformScratchBuffer,
            BIT_INCREMENT_8BIT,
            (EB_BOOL)(tuSize == MIN_PU_SIZE),
            contextPtr->transCoeffShapeLuma);



		UnifiedQuantizeInvQuantize(

			contextPtr,

			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
			64,
			((EB_S16*)coeffSamplesTB->bufferY) + scratchLumaOffset,
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			qp,
			inputSamples->bitDepth,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[0]),
			contextPtr->transCoeffShapeLuma,

			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_LUMA,
			temporalLayerIndex,
			dZoffset,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);
	
		tuPtr->lumaCbf = countNonZeroCoeffs[0] ? EB_TRUE : EB_FALSE;

		tuPtr->isOnlyDc[0] = (countNonZeroCoeffs[0] == 1 && (((EB_S16*)residual16bit->bufferY) + scratchLumaOffset)[0] != 0 && tuSize != 32) ?
			EB_TRUE :
			EB_FALSE;

		if (contextPtr->transCoeffShapeLuma && tuPtr->lumaCbf && tuPtr->isOnlyDc[0] == EB_FALSE) {
            if (contextPtr->transCoeffShapeLuma == N2_SHAPE || contextPtr->transCoeffShapeLuma == N4_SHAPE) {
                PfZeroOutUselessQuadrants(
                    ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                    64,
                    (tuSize >> 1));
            }

            if (contextPtr->transCoeffShapeLuma == N4_SHAPE) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
					64,
					(tuSize >> 2));
			}
		}
	}

	{
		//**********************************
		// Cb
		//********************************** 
		PictureResidual(
			inputSamples->bufferCb + inputCbOffset,
			inputSamples->strideCb,
			predSamples->bufferCb + predCbOffset,
			predSamples->strideCb,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize >> 1,
			tuSize >> 1);
        // For the case that DC path chosen for chroma, we check the DC values and determine to use DC or N2Shape for chroma. Since there is only one flag for ChromaShaping, we do the prediction of Cr and Cb and decide on the chroma shaping
        if (contextPtr->transCoeffShapeChroma == ONLY_DC_SHAPE) {
            EB_S64 sumResidual = SumResidual_funcPtrArray[(ASM_TYPES & AVX2_MASK) && 1](

                ((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
                tuSize >> 1,
                32);
            sumResidual = (ABS(sumResidual) / (tuSize / 2) / (tuSize / 2)); // Normalized based on the size. For chroma, tusize/2 +Tusize/2
            if (sumResidual > 0) {
                contextPtr->transCoeffShapeChroma = N2_SHAPE;
            }
        }
        PictureResidual(
            inputSamples->bufferCr + inputCrOffset,
            inputSamples->strideCr,
            predSamples->bufferCr + predCrOffset,
            predSamples->strideCr,
            ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
            32,
            tuSize >> 1,
            tuSize >> 1);
        if (contextPtr->transCoeffShapeChroma == ONLY_DC_SHAPE) {
			EB_S64 sumResidual = SumResidual_funcPtrArray[(ASM_TYPES & AVX2_MASK) && 1](
                ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
                tuSize >> 1,
                32);
            sumResidual = (ABS(sumResidual) / (tuSize / 2) / (tuSize / 2)); // Normalized based on the size. For chroma, tusize/2 +Tusize/2
            if (sumResidual > 0) {
                contextPtr->transCoeffShapeChroma = N2_SHAPE;
            }
        }

		EstimateTransform(
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize >> 1,
			transformScratchBuffer,
			BIT_INCREMENT_8BIT,
			EB_FALSE,
            contextPtr->transCoeffShapeChroma);

		UnifiedQuantizeInvQuantize(
			contextPtr,
            lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCb) + scratchCbOffset,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			chromaQp,
			inputSamples->bitDepth,
			tuSize >> 1,
			sliceType,
			&(countNonZeroCoeffs[1]),
            contextPtr->transCoeffShapeChroma,
            contextPtr->cleanSparseCeoffPfEncDec,
            contextPtr->pmpMaskingLevelEncDec,
            cuPtr->predictionModeFlag,
			useDeltaQp == EB_TRUE ? contextPtr->forceCbfFlag : 0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost),

		tuPtr->cbCbf = countNonZeroCoeffs[1] ? EB_TRUE : EB_FALSE;

		tuPtr->isOnlyDc[1] = (countNonZeroCoeffs[1] == 1 && (((EB_S16*)residual16bit->bufferCb) + scratchCbOffset)[0] != 0) ?
			EB_TRUE :
			EB_FALSE;
		if (contextPtr->transCoeffShapeChroma && tuPtr->cbCbf && tuPtr->isOnlyDc[1] == EB_FALSE) {
            if (contextPtr->transCoeffShapeChroma == PF_N2 || contextPtr->transCoeffShapeChroma == PF_N4) {
                PfZeroOutUselessQuadrants(
                    ((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
                    residual16bit->strideCb,
                    (tuSize >> 2));
            }

            if (contextPtr->transCoeffShapeChroma == PF_N4) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
					residual16bit->strideCb,
					(tuSize >> 3));
			}
		}


		//**********************************
		// Cr
		//********************************** 
		EstimateTransform(
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			tuSize >> 1,
			transformScratchBuffer,
			BIT_INCREMENT_8BIT,
			EB_FALSE,
            contextPtr->transCoeffShapeChroma);


		 {
			 UnifiedQuantizeInvQuantize(

				 contextPtr,

                 lcuPtr->pictureControlSetPtr,
				 ((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
				 32,
				 ((EB_S16*)coeffSamplesTB->bufferCr) + scratchCrOffset,
				 ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
				 chromaQp,
				 inputSamples->bitDepth,
				 tuSize >> 1,
				 sliceType,
				 &(countNonZeroCoeffs[2]),
                 contextPtr->transCoeffShapeChroma,
                 contextPtr->cleanSparseCeoffPfEncDec,
                 contextPtr->pmpMaskingLevelEncDec,
                 cuPtr->predictionModeFlag,
				 0,
				 enableContouringQCUpdateFlag,
				 COMPONENT_CHROMA,
				 temporalLayerIndex,
			     0,
				 cabacEncodeCtxPtr,
				 contextPtr->fullLambda,
				 intraLumaMode,
				 EB_INTRA_CHROMA_DM,
				 CabacCost);

		}

		 tuPtr->crCbf = countNonZeroCoeffs[2] ? EB_TRUE : EB_FALSE;

         tuPtr->isOnlyDc[2] = (countNonZeroCoeffs[2] == 1 && (((EB_S16*)residual16bit->bufferCr) + scratchCbOffset)[0] != 0) ?
                EB_TRUE :
                EB_FALSE;
         if (contextPtr->transCoeffShapeChroma && tuPtr->crCbf && tuPtr->isOnlyDc[2] == EB_FALSE) {

             if (contextPtr->transCoeffShapeChroma == PF_N2 || contextPtr->transCoeffShapeChroma == PF_N4) {
                 PfZeroOutUselessQuadrants(
                     ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
                     residual16bit->strideCr,
                     (tuSize >> 2));
             }

             if (contextPtr->transCoeffShapeChroma == PF_N4) {
                 PfZeroOutUselessQuadrants(
                     ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
                     residual16bit->strideCr,
                     (tuSize >> 3));
             }
         }
	}
	tuPtr->transCoeffShapeLuma   = contextPtr->transCoeffShapeLuma;
	tuPtr->transCoeffShapeChroma = contextPtr->transCoeffShapeChroma;
    tuPtr->nzCoefCount[0] = (EB_U16)countNonZeroCoeffs[0]; 
    tuPtr->nzCoefCount[1] = (EB_U16)countNonZeroCoeffs[1]; 
    tuPtr->nzCoefCount[2] = (EB_U16)countNonZeroCoeffs[2]; 
	return;
}

/**********************************************************
* Encode Generate Recon
*
* Summary: Performs a H.265 conformant
*   Inverse Transform and generate
*   the reconstructed samples of a TU.
*
* Inputs:
*   originX
*   originY
*   tuSize
*   lcuSize
*   input - Inverse Qunatized Coeff (position sensitive)
*   pred - prediction samples (position independent)
*
* Outputs:
*   Recon  (position independent)
*
**********************************************************/
static void EncodeGenerateRecon(
	EncDecContext_t       *contextPtr,
	EB_U32                 originX,
	EB_U32                 originY,
	EbPictureBufferDesc_t *predSamples,     // no basis/offset
	EbPictureBufferDesc_t *residual16bit,    // no basis/offset
	EB_S16                *transformScratchBuffer)
{
	EB_U32 predLumaOffset;
	EB_U32 predChromaOffset;
	EB_U32 scratchLumaOffset;
	EB_U32 scratchChromaOffset;
	EB_U32 reconLumaOffset;
	EB_U32 reconChromaOffset;

    CodingUnit_t		  *cuPtr              = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr              = &cuPtr->transformUnitArray[contextPtr->tuItr]; 
    EB_U32                 tuSize             = (contextPtr->cuStats->size == 64) ? 32 : contextPtr->cuStats->size;
    EbPictureBufferDesc_t *reconSamples       = predSamples;
	// *Note - The prediction is built in-place in the Recon buffer. It is overwritten with Reconstructed
	//   samples if the CBF==1 && SKIP==False

	//**********************************
	// Luma
	//********************************** 

	{
        predLumaOffset =    (predSamples->originY+originY)             * predSamples->strideY    + (predSamples->originX+originX);
		scratchLumaOffset = ((originY & (63))  * 64) + (originX & (63));	
        reconLumaOffset =   (reconSamples->originY+originY)            * reconSamples->strideY   + (reconSamples->originX+originX);
		if (tuPtr->lumaCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

			EncodeInvTransform(
				tuPtr->transCoeffShapeLuma == ONLY_DC_SHAPE || tuPtr->isOnlyDc[0],
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				tuSize,
				transformScratchBuffer,
				BIT_INCREMENT_8BIT,
				(EB_BOOL)(tuSize == MIN_PU_SIZE));

			AdditionKernel_funcPtrArray[(ASM_TYPES & PREAVX2_MASK) && 1][tuSize >> 3](
				predSamples->bufferY + predLumaOffset,
				predSamples->strideY,
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				reconSamples->bufferY + reconLumaOffset,
				reconSamples->strideY,
				tuSize,
				tuSize);
		}
	}

	//**********************************
	// Chroma
	//********************************** 

	{
        predChromaOffset    = (((predSamples->originY+originY) >> 1)           * predSamples->strideCb)   + ((predSamples->originX+originX) >> 1);
		scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
		reconChromaOffset   = (((reconSamples->originY+originY) >> 1)          * reconSamples->strideCb)  + ((reconSamples->originX+originX) >> 1);
		//**********************************
		// Cb
		//********************************** 
		if (tuPtr->cbCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

			EncodeInvTransform(
				tuPtr->transCoeffShapeChroma == ONLY_DC_SHAPE || tuPtr->isOnlyDc[1],
				((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
				32,
				((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
				32,
				tuSize>>1,
				transformScratchBuffer,
				BIT_INCREMENT_8BIT,
				EB_FALSE);

			AdditionKernel_funcPtrArray[(ASM_TYPES & PREAVX2_MASK) && 1][tuSize >> 4](
				predSamples->bufferCb + predChromaOffset,
				predSamples->strideCb,
				((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
				32,
				reconSamples->bufferCb + reconChromaOffset,
				reconSamples->strideCb,
				tuSize>>1,
				tuSize>>1);
		}

		//**********************************
		// Cr
		//********************************** 
        predChromaOffset    = (((predSamples->originY+originY) >> 1)           * predSamples->strideCr)   + ((predSamples->originX+originX) >> 1);
		scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
		reconChromaOffset   = (((reconSamples->originY+originY) >> 1)          * reconSamples->strideCr)  + ((reconSamples->originX+originX) >> 1);
		if (tuPtr->crCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

			EncodeInvTransform(
				tuPtr->transCoeffShapeChroma == ONLY_DC_SHAPE || tuPtr->isOnlyDc[2],
				((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
				32,
				((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
				32,
				tuSize>>1,
				transformScratchBuffer,
				BIT_INCREMENT_8BIT,
				EB_FALSE);

			AdditionKernel_funcPtrArray[(ASM_TYPES & PREAVX2_MASK) && 1][tuSize >> 4](
				predSamples->bufferCr + predChromaOffset,
				predSamples->strideCr,
				((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
				32,
				reconSamples->bufferCr + reconChromaOffset,
				reconSamples->strideCr,
				tuSize>>1,
				tuSize>>1);
		}
	}

	return;
}

/**********************************************************
* Encode Loop
*
* Summary: Performs a H.265 conformant
*   Transform, Quantization  and Inverse Quantization of a TU.
*
* Inputs:
*   originX
*   originY
*   tuSize
*   lcuSize
*   input - input samples (position sensitive)
*   pred - prediction samples (position independent)
*
* Outputs:
*   Inverse quantized coeff - quantization indices (position sensitive)
*
**********************************************************/
static void EncodeLoop16bit(
	EncDecContext_t				*contextPtr,
	LargestCodingUnit_t			*lcuPtr,
	EB_U32						 originX,            
	EB_U32						 originY,           
	EB_U32						 cbQp,              
	EbPictureBufferDesc_t		*predSamples,         // no basis/offset
	EbPictureBufferDesc_t		*coeffSamplesTB,      // lcu based
	EbPictureBufferDesc_t		*residual16bit,       // no basis/offset 
	EbPictureBufferDesc_t		*transform16bit,      // no basis/offset
	EB_S16						*transformScratchBuffer,
	EB_U32						*countNonZeroCoeffs,
	EB_U32						 useDeltaQp,
	CabacEncodeContext_t		*cabacEncodeCtxPtr,
	EB_U32						 intraLumaMode,
	CabacCost_t					*CabacCost,
	EB_U32						 dZoffset) 

{
    

    EB_U32                 chromaQp           = cbQp; 

    CodingUnit_t		  *cuPtr              = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr              = &cuPtr->transformUnitArray[contextPtr->tuItr]; 
    EB_SLICE               sliceType          = lcuPtr->pictureControlSetPtr->sliceType;
    EB_U32                 temporalLayerIndex = lcuPtr->pictureControlSetPtr->temporalLayerIndex;
    EB_U32                 qp                 = cuPtr->qp;
    EB_U32                 tuSize             = (contextPtr->cuStats->size == 64) ? 32 : contextPtr->cuStats->size;
    EbPictureBufferDesc_t *inputSamples16bit  = contextPtr->inputSample16bitBuffer;
    EbPictureBufferDesc_t *predSamples16bit   = predSamples;

	const EB_U32           inputLumaOffset    = ((originY   & (63))          * inputSamples16bit->strideY ) + (originX  & (63));
	const EB_U32           inputCbOffset      = (((originY  & (63)) >> 1)    * inputSamples16bit->strideCb) + ((originX & (63)) >> 1);
	const EB_U32           inputCrOffset      = (((originY  & (63)) >> 1)    * inputSamples16bit->strideCr) + ((originX & (63)) >> 1);

    const EB_U32           predLumaOffset     = (( predSamples16bit->originY+originY)        * predSamples16bit->strideY ) + ( predSamples16bit->originX+originX);
	const EB_U32           predCbOffset       = (((predSamples16bit->originY+originY) >> 1)  * predSamples16bit->strideCb) + ((predSamples16bit->originX+originX) >> 1);
	const EB_U32           predCrOffset       = (((predSamples16bit->originY+originY) >> 1)  * predSamples16bit->strideCr) + ((predSamples16bit->originX+originX) >> 1);
    const EB_U32           scratchLumaOffset  = ((originY   & (63))        * 64) + (originX  & (63));
	const EB_U32           scratchCbOffset    = (((originY  & (63)) >> 1)  * 32) + ((originX & (63)) >> 1);
	const EB_U32           scratchCrOffset    = (((originY  & (63)) >> 1)  * 32) + ((originX & (63)) >> 1);

    EB_U8 enableContouringQCUpdateFlag;

    enableContouringQCUpdateFlag = DeriveContouringClass(
        lcuPtr->pictureControlSetPtr->ParentPcsPtr,
        lcuPtr->index,
        cuPtr->leafIndex) && (cuPtr->qp < lcuPtr->pictureControlSetPtr->pictureQp);

	//Update QP for Quant
	qp += QP_BD_OFFSET;

	chromaQp += QP_BD_OFFSET;
	
	{

        //**********************************
		// Luma
		//**********************************
		PictureResidual16bit(
			((EB_U16*)inputSamples16bit->bufferY) + inputLumaOffset,
			inputSamples16bit->strideY,
			((EB_U16*)predSamples16bit->bufferY) + predLumaOffset,
			predSamples16bit->strideY,
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			64,
			tuSize,
			tuSize);

		EncodeTransform(
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			64,
			((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
			64,
			tuSize,
			transformScratchBuffer,
			BIT_INCREMENT_10BIT,
			(EB_BOOL)(tuSize == MIN_PU_SIZE),
            contextPtr->transCoeffShapeLuma);

		UnifiedQuantizeInvQuantize(

			contextPtr,

            lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
			64,
			((EB_S16*)coeffSamplesTB->bufferY) + scratchLumaOffset,
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			qp,
			EB_10BIT,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[0]),
            contextPtr->transCoeffShapeLuma,
            contextPtr->cleanSparseCeoffPfEncDec,
            contextPtr->pmpMaskingLevelEncDec,
            cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_LUMA,
			temporalLayerIndex,
			dZoffset,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->lumaCbf = countNonZeroCoeffs[0] ? EB_TRUE : EB_FALSE;

		tuPtr->isOnlyDc[0] = (countNonZeroCoeffs[0] == 1 && (((EB_S16*)residual16bit->bufferY) + scratchLumaOffset)[0] != 0 && tuSize != 32) ?
			EB_TRUE :
			EB_FALSE;
		if (contextPtr->transCoeffShapeLuma && tuPtr->lumaCbf && tuPtr->isOnlyDc[0] == EB_FALSE) {
            if (contextPtr->transCoeffShapeLuma == N2_SHAPE || contextPtr->transCoeffShapeLuma == N4_SHAPE) {
                PfZeroOutUselessQuadrants(
                    ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                    64,
                    (tuSize >> 1));
            }

            if (contextPtr->transCoeffShapeLuma == N4_SHAPE) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
					64,
					(tuSize >> 2));
			}
		}
	}

	{
		//**********************************
		// Cb
		//********************************** 
		PictureResidual16bit(
			((EB_U16*)inputSamples16bit->bufferCb) + inputCbOffset,
			inputSamples16bit->strideCb,
			((EB_U16*)predSamples16bit->bufferCb) + predCbOffset,
			predSamples16bit->strideCb,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize >> 1,
			tuSize >> 1);

        // For the case that DC path chosen for chroma, we check the DC values and determine to use DC or N2Shape for chroma. Since there is only one flag for ChromaShaping, we do the prediction of Cr and Cb and decide on the chroma shaping
        if (contextPtr->transCoeffShapeChroma == ONLY_DC_SHAPE) {
			EB_S64 sumResidual = SumResidual_funcPtrArray[(ASM_TYPES & AVX2_MASK) && 1](
                ((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
                tuSize >> 1,
                32);

            sumResidual = (ABS(sumResidual) / (tuSize / 2) / (tuSize / 2)); // Normalized based on the size. For chroma, tusize/2 +Tusize/2
            if (sumResidual > (1 << BIT_INCREMENT_10BIT)) {
                contextPtr->transCoeffShapeChroma = N2_SHAPE;
            }
        }

        PictureResidual16bit(
            ((EB_U16*)inputSamples16bit->bufferCr) + inputCrOffset,
            inputSamples16bit->strideCr,
            ((EB_U16*)predSamples16bit->bufferCr) + predCrOffset,
            predSamples16bit->strideCr,
            ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
            32,
            tuSize >> 1,
            tuSize >> 1);

        if (contextPtr->transCoeffShapeChroma == ONLY_DC_SHAPE) {

			EB_S64 sumResidual = SumResidual_funcPtrArray[(ASM_TYPES & AVX2_MASK) && 1](
                ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
                tuSize >> 1,
                32);
            sumResidual = (ABS(sumResidual) / (tuSize / 2) / (tuSize / 2)); // Normalized based on the size. For chroma, tusize/2 +Tusize/2
            if (sumResidual > (1<<BIT_INCREMENT_10BIT)) {
                contextPtr->transCoeffShapeChroma = N2_SHAPE;
            }
        }
        

		EncodeTransform(
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize >> 1,
			transformScratchBuffer,
			BIT_INCREMENT_10BIT,
			EB_FALSE,
            contextPtr->transCoeffShapeChroma);

		UnifiedQuantizeInvQuantize(
			contextPtr,
			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCb) + scratchCbOffset,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			chromaQp,
			EB_10BIT,
			tuSize >> 1,
			sliceType,
			&(countNonZeroCoeffs[1]),
			contextPtr->transCoeffShapeChroma,
			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->cbCbf = countNonZeroCoeffs[1] ? EB_TRUE : EB_FALSE;

		tuPtr->isOnlyDc[1] = (countNonZeroCoeffs[1] == 1 && (((EB_S16*)residual16bit->bufferCb) + scratchCbOffset)[0] != 0) ?
			EB_TRUE :
			EB_FALSE;
		if (contextPtr->transCoeffShapeChroma && tuPtr->cbCbf && tuPtr->isOnlyDc[1] == EB_FALSE) {
            if (contextPtr->transCoeffShapeChroma == PF_N2 || contextPtr->transCoeffShapeChroma == PF_N4) {
                PfZeroOutUselessQuadrants(
                    ((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
                    residual16bit->strideCb,
                    (tuSize >> 2));
            }

            if (contextPtr->transCoeffShapeChroma == PF_N4) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
					residual16bit->strideCb,
					(tuSize >> 3));
			}
		}


		//**********************************
		// Cr
		//********************************** 
		EncodeTransform(
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			tuSize >> 1,
			transformScratchBuffer,
			BIT_INCREMENT_10BIT,
            EB_FALSE,
            contextPtr->transCoeffShapeChroma);

		UnifiedQuantizeInvQuantize(

			contextPtr,

			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCr) + scratchCrOffset,
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			chromaQp,
			EB_10BIT,
			tuSize >> 1,
			sliceType,
			&(countNonZeroCoeffs[2]),
			contextPtr->transCoeffShapeChroma,

			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			useDeltaQp == EB_TRUE ? contextPtr->forceCbfFlag : 0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);


		tuPtr->crCbf = countNonZeroCoeffs[2] ? EB_TRUE : EB_FALSE;

		tuPtr->isOnlyDc[2] = (countNonZeroCoeffs[2] == 1 && (((EB_S16*)residual16bit->bufferCr) + scratchCbOffset)[0] != 0) ?
			EB_TRUE :
			EB_FALSE;
		if (contextPtr->transCoeffShapeChroma && tuPtr->crCbf && tuPtr->isOnlyDc[2] == EB_FALSE) {
             if (contextPtr->transCoeffShapeChroma == PF_N2 || contextPtr->transCoeffShapeChroma == PF_N4) {
                 PfZeroOutUselessQuadrants(
                     ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
                     residual16bit->strideCr,
                     (tuSize >> 2));
             }

             if (contextPtr->transCoeffShapeChroma == PF_N4) {
                 PfZeroOutUselessQuadrants(
                     ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
                     residual16bit->strideCr,
                     (tuSize >> 3));
             }
         }
	}
	tuPtr->transCoeffShapeLuma   = contextPtr->transCoeffShapeLuma;
	tuPtr->transCoeffShapeChroma = contextPtr->transCoeffShapeChroma;
    tuPtr->nzCoefCount[0] = (EB_U16)countNonZeroCoeffs[0]; 
    tuPtr->nzCoefCount[1] = (EB_U16)countNonZeroCoeffs[1]; 
    tuPtr->nzCoefCount[2] = (EB_U16)countNonZeroCoeffs[2]; 
    return;
}

/**********************************************************
* Encode Generate Recon
*
* Summary: Performs a H.265 conformant
*   Inverse Transform and generate
*   the reconstructed samples of a TU.
*
* Inputs:
*   originX
*   originY
*   tuSize
*   lcuSize
*   input - Inverse Qunatized Coeff (position sensitive)
*   pred - prediction samples (position independent)
*
* Outputs:
*   Recon  (position independent)
*
**********************************************************/
static void EncodeGenerateRecon16bit(
    EncDecContext_t       *contextPtr,
    EB_U32                 originX,
    EB_U32                 originY,
    EbPictureBufferDesc_t *predSamples,     // no basis/offset  
    EbPictureBufferDesc_t *residual16bit,    // no basis/offset 
    EB_S16                *transformScratchBuffer)
{

    EB_U32 predLumaOffset;
    EB_U32 predChromaOffset;
    EB_U32 scratchLumaOffset;
    EB_U32 scratchChromaOffset;
    EB_U32 reconLumaOffset;
    EB_U32 reconChromaOffset;

    CodingUnit_t		  *cuPtr = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr = &cuPtr->transformUnitArray[contextPtr->tuItr];
    EB_U32                 tuSize = (contextPtr->cuStats->size == 64) ? 32 : contextPtr->cuStats->size;

    //**********************************
    // Luma
    //********************************** 

	{
        predLumaOffset = (predSamples->originY + originY)* predSamples->strideY + (predSamples->originX + originX);
        scratchLumaOffset = ((originY & (63)) * 64) + (originX & (63));
        reconLumaOffset = (predSamples->originY + originY)* predSamples->strideY + (predSamples->originX + originX);
        if (tuPtr->lumaCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

            EncodeInvTransform(
				tuPtr->transCoeffShapeLuma == ONLY_DC_SHAPE || tuPtr->isOnlyDc[0],
                ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                64,
                ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                64,
                tuSize,
                transformScratchBuffer,
                BIT_INCREMENT_10BIT,
                (EB_BOOL)(tuSize == MIN_PU_SIZE));

            AdditionKernel_funcPtrArray16bit[(ASM_TYPES & PREAVX2_MASK) && 1](
                (EB_U16*)predSamples->bufferY + predLumaOffset,
                predSamples->strideY,
                ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                64,
                (EB_U16*)predSamples->bufferY + reconLumaOffset,
                predSamples->strideY,
                tuSize,
                tuSize);

        }

    }

    //**********************************
    // Chroma
    //********************************** 

	{

        //**********************************
        // Cb
        //********************************** 
        predChromaOffset = (((predSamples->originY + originY) >> 1)  * predSamples->strideCb) + ((predSamples->originX + originX) >> 1);
        scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
        reconChromaOffset = (((predSamples->originY + originY) >> 1) * predSamples->strideCb) + ((predSamples->originX + originX) >> 1);
        if (tuPtr->cbCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

            EncodeInvTransform(
				tuPtr->transCoeffShapeChroma == ONLY_DC_SHAPE || tuPtr->isOnlyDc[1],
                ((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
                32,
                ((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
                32,
                tuSize >> 1,
                transformScratchBuffer,
                BIT_INCREMENT_10BIT,
                EB_FALSE);

            AdditionKernel_funcPtrArray16bit[(ASM_TYPES & PREAVX2_MASK) && 1](
                (EB_U16*)predSamples->bufferCb + predChromaOffset,
                predSamples->strideCb,
                ((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
                32,
                (EB_U16*)predSamples->bufferCb + reconChromaOffset,
                predSamples->strideCb,
                tuSize >> 1,
                tuSize >> 1);

        }

        //**********************************
        // Cr
        //********************************** 
        predChromaOffset = (((predSamples->originY + originY) >> 1)  * predSamples->strideCr) + ((predSamples->originX + originX) >> 1);
        scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
        reconChromaOffset = (((predSamples->originY + originY) >> 1) * predSamples->strideCr) + ((predSamples->originX + originX) >> 1);
        if (tuPtr->crCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

            EncodeInvTransform(
				tuPtr->transCoeffShapeChroma == ONLY_DC_SHAPE || tuPtr->isOnlyDc[2],
                ((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
                32,
                ((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
                32,
                tuSize >> 1,
                transformScratchBuffer,
                BIT_INCREMENT_10BIT,
                EB_FALSE);
            
            AdditionKernel_funcPtrArray16bit[(ASM_TYPES & PREAVX2_MASK) && 1](
                (EB_U16*)predSamples->bufferCr + predChromaOffset,
                predSamples->strideCr,
                ((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
                32,
                (EB_U16*)predSamples->bufferCr + reconChromaOffset,
                predSamples->strideCr,
                tuSize >> 1,
                tuSize >> 1);
        }
    }

    return;
}

static EB_ENCODE_LOOP_FUNC_PTR   EncodeLoopFunctionTable[2] =
{
    EncodeLoop,
    EncodeLoop16bit
};

EB_GENERATE_RECON_FUNC_PTR   EncodeGenerateReconFunctionPtr[2] =
{
    EncodeGenerateRecon,
    EncodeGenerateRecon16bit
};

static void EncodeLoopIntra4x4(
	EncDecContext_t       *contextPtr,
	LargestCodingUnit_t   *lcuPtr,
	EB_U32                 originX,          
	EB_U32                 originY,          
	EB_U32                 cbQp,
	EbPictureBufferDesc_t *predSamples,             // no basis/offset   
	EbPictureBufferDesc_t *coeffSamplesTB,          // lcu based
	EbPictureBufferDesc_t *residual16bit,           // no basis/offset
	EbPictureBufferDesc_t *transform16bit,          // no basis/offset
	EB_S16                *transformScratchBuffer,
	EB_U32				  *countNonZeroCoeffs,
	EB_U32                 componentMask,
	EB_U32				   useDeltaQp,
	CabacEncodeContext_t  *cabacEncodeCtxPtr,
	EB_U32                 intraLumaMode,		  
	CabacCost_t           *CabacCost,
	EB_U32                 dZoffset)  

{
    
    EB_U32                 chromaQp           = cbQp; 
    CodingUnit_t		  *cuPtr              = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr              = &cuPtr->transformUnitArray[contextPtr->tuItr]; 
    EB_SLICE               sliceType          = lcuPtr->pictureControlSetPtr->sliceType;
    EB_U32                 temporalLayerIndex = lcuPtr->pictureControlSetPtr->temporalLayerIndex;
    EB_U32                 qp                 = cuPtr->qp;
    EB_U32                 tuSize             = MIN_PU_SIZE; 
    EbPictureBufferDesc_t  *inputSamples      = contextPtr->inputSamples;


	const EB_U32 inputLumaOffset    = ((originY + inputSamples->originY)          * inputSamples->strideY) + (originX + inputSamples->originX);
	const EB_U32 inputCbOffset      = (((originY + inputSamples->originY) >> 1)    * inputSamples->strideCb) + ((originX + inputSamples->originX) >> 1);
	const EB_U32 inputCrOffset      = (((originY + inputSamples->originY) >> 1)    * inputSamples->strideCr) + ((originX + inputSamples->originX) >> 1);
    const EB_U32 predLumaOffset     = (( predSamples->originY+originY)        * predSamples->strideY ) + ( predSamples->originX+originX);
	const EB_U32 predCbOffset       = (((predSamples->originY+originY) >> 1)  * predSamples->strideCb) + ((predSamples->originX+originX) >> 1);
	const EB_U32 predCrOffset       = (((predSamples->originY+originY) >> 1)  * predSamples->strideCr) + ((predSamples->originX+originX) >> 1);
    const EB_U32 scratchLumaOffset  = ((originY & (64 - 1))        * 64) + (originX  & (64 - 1));
	const EB_U32 scratchCbOffset    = (((originY & (64 - 1)) >> 1)   * 32) + ((originX & (64 - 1)) >> 1);
	const EB_U32 scratchCrOffset    = (((originY & (64 - 1)) >> 1)   * 32) + ((originX & (64 - 1)) >> 1);

    EB_U8 enableContouringQCUpdateFlag;

    enableContouringQCUpdateFlag = DeriveContouringClass(
        lcuPtr->pictureControlSetPtr->ParentPcsPtr,
        lcuPtr->index,
        cuPtr->leafIndex) && (cuPtr->qp < lcuPtr->pictureControlSetPtr->pictureQp);

	//**********************************
	// Luma
	//**********************************
     if (componentMask == PICTURE_BUFFER_DESC_FULL_MASK || componentMask == PICTURE_BUFFER_DESC_LUMA_MASK) {
	
        PictureResidual(
            inputSamples->bufferY + inputLumaOffset,
            inputSamples->strideY,
            predSamples->bufferY + predLumaOffset,
            predSamples->strideY,
            ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
            64,
            tuSize,
            tuSize);

        EstimateTransform(
            ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
            64,
            ((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
            64,
            tuSize,
            transformScratchBuffer,
            BIT_INCREMENT_8BIT,
            (EB_BOOL)(tuSize == MIN_PU_SIZE),
			contextPtr->transCoeffShapeLuma);


		UnifiedQuantizeInvQuantize(

			contextPtr,

			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
			64,
			((EB_S16*)coeffSamplesTB->bufferY) + scratchLumaOffset,
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			qp,
			inputSamples->bitDepth,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[0]),
			contextPtr->transCoeffShapeLuma,

			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_LUMA,
			temporalLayerIndex,
			dZoffset,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->lumaCbf = countNonZeroCoeffs[0] ? EB_TRUE : EB_FALSE;

		if (contextPtr->transCoeffShapeLuma && tuPtr->lumaCbf) {

			PfZeroOutUselessQuadrants(
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				(tuSize >> 1));

			if (contextPtr->transCoeffShapeLuma == N4_SHAPE) {

				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
					64,
					(tuSize >> 2));
			}
		}

	}

	if (componentMask == PICTURE_BUFFER_DESC_FULL_MASK || componentMask == PICTURE_BUFFER_DESC_CHROMA_MASK){

		//**********************************
		// Cb
		//********************************** 
		PictureResidual(
			inputSamples->bufferCb + inputCbOffset,
			inputSamples->strideCb,
			predSamples->bufferCb + predCbOffset,
			predSamples->strideCb,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize,
			tuSize);

		EstimateTransform(
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize,
			transformScratchBuffer,
			BIT_INCREMENT_8BIT,
			EB_FALSE,
			contextPtr->transCoeffShapeChroma);


		UnifiedQuantizeInvQuantize(

			contextPtr,

			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCb) + scratchCbOffset,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			chromaQp,
			inputSamples->bitDepth,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[1]),
			contextPtr->transCoeffShapeChroma,

			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			useDeltaQp == EB_TRUE ? contextPtr->forceCbfFlag : 0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->cbCbf = countNonZeroCoeffs[1] ? EB_TRUE : EB_FALSE;

		if (contextPtr->transCoeffShapeChroma && tuPtr->cbCbf) {

			PfZeroOutUselessQuadrants(
				((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
				residual16bit->strideCb,
				(tuSize >> 1));

            if (contextPtr->transCoeffShapeChroma == PF_N4) {

				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
					residual16bit->strideCb,
					(tuSize >> 2));
			}
		}
		//**********************************
		// Cr
		//********************************** 

		PictureResidual(
			inputSamples->bufferCr + inputCrOffset,
			inputSamples->strideCr,
			predSamples->bufferCr + predCrOffset,
			predSamples->strideCr,
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			32,
			tuSize,
			tuSize);

		EstimateTransform(
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			tuSize,
			transformScratchBuffer,
			BIT_INCREMENT_8BIT,
			EB_FALSE,
            contextPtr->transCoeffShapeChroma);


		UnifiedQuantizeInvQuantize(

			contextPtr,

			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCr) + scratchCrOffset,
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			chromaQp,
			inputSamples->bitDepth,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[2]),
			contextPtr->transCoeffShapeChroma,

			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		 tuPtr->crCbf = countNonZeroCoeffs[2] ? EB_TRUE : EB_FALSE;
		 if (contextPtr->transCoeffShapeChroma && tuPtr->crCbf) {

			 PfZeroOutUselessQuadrants(
				 ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
				 32,
				 (tuSize >> 1));

             if (contextPtr->transCoeffShapeChroma == PF_N4) {

				 PfZeroOutUselessQuadrants(
					 ((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
					 32,
					 (tuSize >> 2));
			 }
		 }
	}

	tuPtr->transCoeffShapeLuma   = contextPtr->transCoeffShapeLuma;
	tuPtr->transCoeffShapeChroma = contextPtr->transCoeffShapeChroma;
    tuPtr->nzCoefCount[0] = (EB_U16)countNonZeroCoeffs[0]; 
    tuPtr->nzCoefCount[1] = (EB_U16)countNonZeroCoeffs[1]; 
    tuPtr->nzCoefCount[2] = (EB_U16)countNonZeroCoeffs[2]; 

	return;
}

static void EncodeGenerateReconIntra4x4(
	EncDecContext_t       *contextPtr,
	EB_U32                 originX,
	EB_U32                 originY,
	EbPictureBufferDesc_t *predSamples,     // no basis/offset
	EbPictureBufferDesc_t *residual16bit,    // no basis/offset
	EB_S16                *transformScratchBuffer,
	EB_U32                 componentMask)
{
	EB_U32 predLumaOffset;
	EB_U32 predChromaOffset;
	EB_U32 scratchLumaOffset;
	EB_U32 scratchChromaOffset;
	EB_U32 reconLumaOffset;
	EB_U32 reconChromaOffset;

    CodingUnit_t		  *cuPtr              = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr              = &cuPtr->transformUnitArray[contextPtr->tuItr]; 
    EB_U32                 tuSize             = MIN_PU_SIZE;
    EbPictureBufferDesc_t *reconSamples       = predSamples;
	// *Note - The prediction is built in-place in the Recon buffer. It is overwritten with Reconstructed
	//   samples if the CBF==1 && SKIP==False

	//**********************************
	// Luma
	//********************************** 

	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

        predLumaOffset =    (predSamples->originY+originY)             * predSamples->strideY    + (predSamples->originX+originX);
		scratchLumaOffset = ((originY & (63))  * 64) + (originX & (63));	
        reconLumaOffset =   (reconSamples->originY+originY)            * reconSamples->strideY   + (reconSamples->originX+originX);
		if (tuPtr->lumaCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

			EncodeInvTransform(
                // DC_ONLY Sahpe not supported for DST
				DEFAULT_SHAPE,
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				tuSize,
				transformScratchBuffer,
				BIT_INCREMENT_8BIT,
				(EB_BOOL)(tuSize == MIN_PU_SIZE));

			AdditionKernel_funcPtrArray[(ASM_TYPES & PREAVX2_MASK) && 1][tuSize >> 3](
				predSamples->bufferY + predLumaOffset,
				predSamples->strideY,
				((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
				64,
				reconSamples->bufferY + reconLumaOffset,
				reconSamples->strideY,
				tuSize,
				tuSize);
		}
	}

	//**********************************
	// Chroma
	//********************************** 

	if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

        predChromaOffset    = (((predSamples->originY+originY) >> 1)           * predSamples->strideCb)   + ((predSamples->originX+originX) >> 1);
		scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
		reconChromaOffset   = (((reconSamples->originY+originY) >> 1)          * reconSamples->strideCb)  + ((reconSamples->originX+originX) >> 1);
		//**********************************
		// Cb
		//********************************** 
		if (tuPtr->cbCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

			EncodeInvTransform(
                // DC_ONLY Sahpe not supported for DST
				DEFAULT_SHAPE,
				((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
				32,
				((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
				32,
				tuSize,
				transformScratchBuffer,
				BIT_INCREMENT_8BIT,
				EB_FALSE);

			AdditionKernel_funcPtrArray[(ASM_TYPES & PREAVX2_MASK) && 1][tuSize >> 3](
				predSamples->bufferCb + predChromaOffset,
				predSamples->strideCb,
				((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
				32,
				reconSamples->bufferCb + reconChromaOffset,
				reconSamples->strideCb,
				tuSize,
				tuSize);
		}

		//**********************************
		// Cr
		//********************************** 
        predChromaOffset    = (((predSamples->originY+originY) >> 1)           * predSamples->strideCr)   + ((predSamples->originX+originX) >> 1);
		scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
		reconChromaOffset   = (((reconSamples->originY+originY) >> 1)          * reconSamples->strideCr)  + ((reconSamples->originX+originX) >> 1);
		if (tuPtr->crCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

			EncodeInvTransform(
                // DC_ONLY Sahpe not supported for DST
				DEFAULT_SHAPE,
				((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
				32,
				((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
				32,
				tuSize,
				transformScratchBuffer,
				BIT_INCREMENT_8BIT,
				EB_FALSE);

			AdditionKernel_funcPtrArray[(ASM_TYPES & PREAVX2_MASK) && 1][tuSize >> 3](
				predSamples->bufferCr + predChromaOffset,
				predSamples->strideCr,
				((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
				32,
				reconSamples->bufferCr + reconChromaOffset,
				reconSamples->strideCr,
				tuSize,
				tuSize);
		}
	}

	return;
}

static void EncodeLoopIntra4x416bit(
	EncDecContext_t				*contextPtr,
	LargestCodingUnit_t			*lcuPtr,
	EB_U32						 originX,            
	EB_U32						 originY,           
	EB_U32						 cbQp,              
	EbPictureBufferDesc_t		*predSamples,         // no basis/offset
	EbPictureBufferDesc_t		*coeffSamplesTB,      // lcu based
	EbPictureBufferDesc_t		*residual16bit,       // no basis/offset 
	EbPictureBufferDesc_t		*transform16bit,      // no basis/offset
	EB_S16						*transformScratchBuffer,
	EB_U32						*countNonZeroCoeffs,
	EB_U32						 componentMask,
	EB_U32						 useDeltaQp, 
	CabacEncodeContext_t		*cabacEncodeCtxPtr,
	EB_U32						 intraLumaMode,
	CabacCost_t					*CabacCost,
	EB_U32						 dZoffset) 

{
    
    EB_U32                 chromaQp           = cbQp; 

    CodingUnit_t		  *cuPtr              = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr              = &cuPtr->transformUnitArray[contextPtr->tuItr]; 
    EB_SLICE               sliceType          = lcuPtr->pictureControlSetPtr->sliceType;
    EB_U32                 temporalLayerIndex = lcuPtr->pictureControlSetPtr->temporalLayerIndex;
    EB_U32                 qp                 = cuPtr->qp;
    EB_U32                 tuSize             = MIN_PU_SIZE;
    EbPictureBufferDesc_t *inputSamples16bit  = contextPtr->inputSample16bitBuffer;
    EbPictureBufferDesc_t *predSamples16bit   = predSamples;

    const EB_U32           inputLumaOffset    = ((originY   & (63))          * inputSamples16bit->strideY ) + (originX  & (63));
	const EB_U32           inputCbOffset      = (((originY  & (63)) >> 1)    * inputSamples16bit->strideCb) + ((originX & (63)) >> 1);
	const EB_U32           inputCrOffset      = (((originY  & (63)) >> 1)    * inputSamples16bit->strideCr) + ((originX & (63)) >> 1);

    const EB_U32           predLumaOffset     = (( predSamples16bit->originY+originY)        * predSamples16bit->strideY ) + ( predSamples16bit->originX+originX);
	const EB_U32           predCbOffset       = (((predSamples16bit->originY+originY) >> 1)  * predSamples16bit->strideCb) + ((predSamples16bit->originX+originX) >> 1);
	const EB_U32           predCrOffset       = (((predSamples16bit->originY+originY) >> 1)  * predSamples16bit->strideCr) + ((predSamples16bit->originX+originX) >> 1);
    const EB_U32           scratchLumaOffset  = ((originY   & (63))        * 64) + (originX  & (63));
	const EB_U32           scratchCbOffset    = (((originY  & (63)) >> 1)  * 32) + ((originX & (63)) >> 1);
	const EB_U32           scratchCrOffset    = (((originY  & (63)) >> 1)  * 32) + ((originX & (63)) >> 1);

    EB_U8 enableContouringQCUpdateFlag;

    enableContouringQCUpdateFlag = DeriveContouringClass(
        lcuPtr->pictureControlSetPtr->ParentPcsPtr,
        lcuPtr->index,
        cuPtr->leafIndex) && (cuPtr->qp < lcuPtr->pictureControlSetPtr->pictureQp);

    //Update QP for Quant
	qp += QP_BD_OFFSET;

	chromaQp += QP_BD_OFFSET;

     if ( componentMask == PICTURE_BUFFER_DESC_FULL_MASK || componentMask == PICTURE_BUFFER_DESC_LUMA_MASK){

        //**********************************
		// Luma
		//**********************************
		PictureResidual16bit(
			((EB_U16*)inputSamples16bit->bufferY) + inputLumaOffset,
			inputSamples16bit->strideY,
			((EB_U16*)predSamples16bit->bufferY) + predLumaOffset,
			predSamples16bit->strideY,
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			64,
			tuSize,
			tuSize);

		EncodeTransform(
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			64,
			((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
			64,
			tuSize,
			transformScratchBuffer,
			BIT_INCREMENT_10BIT,
			(EB_BOOL)(tuSize == MIN_PU_SIZE),
			contextPtr->transCoeffShapeLuma);

		UnifiedQuantizeInvQuantize(

			contextPtr,

			lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferY) + scratchLumaOffset,
			64,
			((EB_S16*)coeffSamplesTB->bufferY) + scratchLumaOffset,
			((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
			qp,
			EB_10BIT,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[0]),
			contextPtr->transCoeffShapeLuma,

			contextPtr->cleanSparseCeoffPfEncDec,
			contextPtr->pmpMaskingLevelEncDec,
			cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_LUMA,
			temporalLayerIndex,
			dZoffset,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->lumaCbf = countNonZeroCoeffs[0] ? EB_TRUE : EB_FALSE;

		if (contextPtr->transCoeffShapeLuma && tuPtr->lumaCbf) {

			if (contextPtr->transCoeffShapeLuma == N2_SHAPE || contextPtr->transCoeffShapeLuma == N4_SHAPE) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
					64,
					(tuSize >> 1));
			}

			if (contextPtr->transCoeffShapeLuma == N4_SHAPE) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
					64,
					(tuSize >> 2));
			}
		}



	}

	if (componentMask == PICTURE_BUFFER_DESC_FULL_MASK || componentMask == PICTURE_BUFFER_DESC_CHROMA_MASK){

		//**********************************
		// Cb
		//********************************** 
		PictureResidual16bit(
			((EB_U16*)inputSamples16bit->bufferCb) + inputCbOffset,
			inputSamples16bit->strideCb,
			((EB_U16*)predSamples16bit->bufferCb) + predCbOffset,
			predSamples16bit->strideCb,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize,
			tuSize);

		EncodeTransform(
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			tuSize,
			transformScratchBuffer,
			BIT_INCREMENT_10BIT,
			EB_FALSE,
            contextPtr->transCoeffShapeChroma);

		UnifiedQuantizeInvQuantize(

			contextPtr,

            lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCb) + scratchCbOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCb) + scratchCbOffset,
			((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
			chromaQp,
			EB_10BIT,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[1]),
            contextPtr->transCoeffShapeChroma,

            contextPtr->cleanSparseCeoffPfEncDec,
            contextPtr->pmpMaskingLevelEncDec,
            cuPtr->predictionModeFlag,
			0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->cbCbf = countNonZeroCoeffs[1] ? EB_TRUE : EB_FALSE;

		if (contextPtr->transCoeffShapeChroma && tuPtr->cbCbf) {

			if (contextPtr->transCoeffShapeChroma == PF_N2 || contextPtr->transCoeffShapeChroma == PF_N4) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
					residual16bit->strideCb,
					(tuSize >> 2));
			}

			if (contextPtr->transCoeffShapeChroma == PF_N4) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCb) + scratchCbOffset,
					residual16bit->strideCb,
					(tuSize >> 3));
			}
		}


		//**********************************
		// Cr
		//********************************** 
		PictureResidual16bit(
			((EB_U16*)inputSamples16bit->bufferCr) + inputCrOffset,
			inputSamples16bit->strideCr,
			((EB_U16*)predSamples16bit->bufferCr) + predCrOffset,
			predSamples16bit->strideCr,
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			32,
			tuSize,
			tuSize);

		EncodeTransform(
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			tuSize,
			transformScratchBuffer,
			BIT_INCREMENT_10BIT,
			EB_FALSE,
            contextPtr->transCoeffShapeChroma);

		UnifiedQuantizeInvQuantize(

			contextPtr,

            lcuPtr->pictureControlSetPtr,
			((EB_S16*)transform16bit->bufferCr) + scratchCrOffset,
			32,
			((EB_S16*)coeffSamplesTB->bufferCr) + scratchCrOffset,
			((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
			chromaQp,
			EB_10BIT,
			tuSize,
			sliceType,
			&(countNonZeroCoeffs[2]),
            contextPtr->transCoeffShapeChroma,

            contextPtr->cleanSparseCeoffPfEncDec,
            contextPtr->pmpMaskingLevelEncDec,
            cuPtr->predictionModeFlag,
			useDeltaQp == EB_TRUE ? contextPtr->forceCbfFlag : 0,
			enableContouringQCUpdateFlag,
			COMPONENT_CHROMA,
			temporalLayerIndex,
			0,
			cabacEncodeCtxPtr,
			contextPtr->fullLambda,
			intraLumaMode,
			EB_INTRA_CHROMA_DM,
			CabacCost);

		tuPtr->crCbf = countNonZeroCoeffs[2] ? EB_TRUE : EB_FALSE;

		if (contextPtr->transCoeffShapeChroma && tuPtr->crCbf) {

			if (contextPtr->transCoeffShapeChroma == PF_N2 || contextPtr->transCoeffShapeChroma == PF_N4) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
					residual16bit->strideCr,
					(tuSize >> 2));
			}

			if (contextPtr->transCoeffShapeChroma == PF_N4) {
				PfZeroOutUselessQuadrants(
					((EB_S16*)residual16bit->bufferCr) + scratchCrOffset,
					residual16bit->strideCr,
					(tuSize >> 3));
			}
		}


	}

	tuPtr->transCoeffShapeLuma   = contextPtr->transCoeffShapeLuma;
	tuPtr->transCoeffShapeChroma = contextPtr->transCoeffShapeChroma;
    tuPtr->nzCoefCount[0] = (EB_U16)countNonZeroCoeffs[0]; 
    tuPtr->nzCoefCount[1] = (EB_U16)countNonZeroCoeffs[1]; 
    tuPtr->nzCoefCount[2] = (EB_U16)countNonZeroCoeffs[2]; 
    return;
}

static void EncodeGenerateReconIntra4x416bit(
    EncDecContext_t       *contextPtr,
    EB_U32                 originX,
    EB_U32                 originY,
    EbPictureBufferDesc_t *predSamples,     // no basis/offset  
    EbPictureBufferDesc_t *residual16bit,    // no basis/offset 
    EB_S16                *transformScratchBuffer,
	EB_U32                 componentMask)
{

    EB_U32 predLumaOffset;
    EB_U32 predChromaOffset;
    EB_U32 scratchLumaOffset;
    EB_U32 scratchChromaOffset;
    EB_U32 reconLumaOffset;
    EB_U32 reconChromaOffset;

    CodingUnit_t		  *cuPtr = contextPtr->cuPtr;
    TransformUnit_t       *tuPtr = &cuPtr->transformUnitArray[contextPtr->tuItr];
    EB_U32                 tuSize = MIN_PU_SIZE;

    //**********************************
    // Luma
    //********************************** 

	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

        predLumaOffset = (predSamples->originY + originY)* predSamples->strideY + (predSamples->originX + originX);
        scratchLumaOffset = ((originY & (63)) * 64) + (originX & (63));
        reconLumaOffset = (predSamples->originY + originY)* predSamples->strideY + (predSamples->originX + originX);
        if (tuPtr->lumaCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

            EncodeInvTransform(
                // DC_ONLY Sahpe not supported for DST
				DEFAULT_SHAPE,
                ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                64,
                ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                64,
                tuSize,
                transformScratchBuffer,
                BIT_INCREMENT_10BIT,
                (EB_BOOL)(tuSize == MIN_PU_SIZE));

			AdditionKernel_funcPtrArray16bit[(ASM_TYPES & PREAVX2_MASK) && 1](
                (EB_U16*)predSamples->bufferY + predLumaOffset,
                predSamples->strideY,
                ((EB_S16*)residual16bit->bufferY) + scratchLumaOffset,
                64,
                (EB_U16*)predSamples->bufferY + reconLumaOffset,
                predSamples->strideY,
                tuSize,
                tuSize);


        }

    }

    //**********************************
    // Chroma
    //********************************** 
	if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

        //**********************************
        // Cb
        //********************************** 
        predChromaOffset = (((predSamples->originY + originY) >> 1)  * predSamples->strideCb) + ((predSamples->originX + originX) >> 1);
        scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
        reconChromaOffset = (((predSamples->originY + originY) >> 1) * predSamples->strideCb) + ((predSamples->originX + originX) >> 1);
        if (tuPtr->cbCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

            EncodeInvTransform(
                // DC_ONLY Sahpe not supported for DST
				DEFAULT_SHAPE,
                ((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
                32,
                ((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
                32,
                tuSize,
                transformScratchBuffer,
                BIT_INCREMENT_10BIT,
                EB_FALSE);

			AdditionKernel_funcPtrArray16bit[(ASM_TYPES & PREAVX2_MASK) && 1](
                (EB_U16*)predSamples->bufferCb + predChromaOffset,
                predSamples->strideCb,
                ((EB_S16*)residual16bit->bufferCb) + scratchChromaOffset,
                32,
                (EB_U16*)predSamples->bufferCb + reconChromaOffset,
                predSamples->strideCb,
                tuSize,
                tuSize);

        }

        //**********************************
        // Cr
        //********************************** 
        predChromaOffset = (((predSamples->originY + originY) >> 1)  * predSamples->strideCr) + ((predSamples->originX + originX) >> 1);
        scratchChromaOffset = (((originY & (63)) >> 1) * 32) + ((originX & (63)) >> 1);
        reconChromaOffset = (((predSamples->originY + originY) >> 1) * predSamples->strideCr) + ((predSamples->originX + originX) >> 1);
        if (tuPtr->crCbf == EB_TRUE && cuPtr->skipFlag == EB_FALSE) {

            EncodeInvTransform(
                // DC_ONLY Sahpe not supported for DST
				DEFAULT_SHAPE,
                ((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
                32,
                ((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
                32,
                tuSize,
                transformScratchBuffer,
                BIT_INCREMENT_10BIT,
                EB_FALSE);

			AdditionKernel_funcPtrArray16bit[(ASM_TYPES & PREAVX2_MASK) && 1](
                (EB_U16*)predSamples->bufferCr + predChromaOffset,
                predSamples->strideCr,
                ((EB_S16*)residual16bit->bufferCr) + scratchChromaOffset,
                32,
                (EB_U16*)predSamples->bufferCr + reconChromaOffset,
                predSamples->strideCr,
                tuSize,
                tuSize);

        }
    }

    return;
}

static EB_ENCODE_LOOP_INTRA_4x4_FUNC_PTR   EncodeLoopIntra4x4FunctionTable[2] =
{
    EncodeLoopIntra4x4,
    EncodeLoopIntra4x416bit
};

EB_GENERATE_RECON_INTRA_4x4_FUNC_PTR   EncodeGenerateReconIntra4x4FunctionPtr[2] =
{
    EncodeGenerateReconIntra4x4,
    EncodeGenerateReconIntra4x416bit
};

EB_GENERATE_INTRA_SAMPLES_FUNC_PTR GenerateIntraReferenceSamplesFuncTable[2] = 
{
    GenerateIntraReferenceSamplesEncodePass,
    GenerateIntraReference16bitSamplesEncodePass
};

EB_ENC_PASS_INTRA_FUNC_PTR EncodePassIntraPredictionFuncTable[2] =
{
    EncodePassIntraPrediction,
    EncodePassIntraPrediction16bit
};

EB_LCU_INTERNAL_DLF_FUNC_PTR LcuInternalAreaDLFCoreFuncTable[2] =
{
    LCUInternalAreaDLFCore,
    LCUInternalAreaDLFCore16bit
};

EB_LCU_BOUNDARY_DLF_FUNC_PTR LcuBoundaryDLFCoreFuncTable[2] =
{
    LCUBoundaryDLFCore,
    LCUBoundaryDLFCore16bit
};

EB_LCU_PIC_EDGE_DLF_FUNC_PTR LcuPicEdgeDLFCoreFuncTable[2] =
{
    LCUPictureEdgeDLFCore,
    LCUPictureEdgeDLFCore16bit
};



/*************************************************
* Encode Pass Motion Vector Prediction
*************************************************/
static void EncodePassMvPrediction(
    SequenceControlSet_t    *sequenceControlSetPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    EB_U32                   lcuIndex,
    EncDecContext_t         *contextPtr)
{
    // AMVP Signaled, or we failed to find a Merge MV match
    if (contextPtr->cuPtr->predictionUnitArray->mergeFlag == EB_FALSE)
    {
        EB_U64 mvdBitsIdx0;
        EB_U64 mvdBitsIdx1;
        EB_S32 xMvdIdx0;
        EB_S32 yMvdIdx0;
        EB_S32 xMvdIdx1;
        EB_S32 yMvdIdx1;

        contextPtr->cuPtr->predictionUnitArray->mergeFlag = EB_FALSE;
        contextPtr->cuPtr->skipFlag = EB_FALSE;

        // Generate AMVP List
        if (contextPtr->cuPtr->predictionUnitArray->interPredDirectionIndex == UNI_PRED_LIST_0 ||
            contextPtr->cuPtr->predictionUnitArray->interPredDirectionIndex == BI_PRED)
        {
            FillAMVPCandidates(
                pictureControlSetPtr->epMvNeighborArray,
                pictureControlSetPtr->epModeTypeNeighborArray,
                contextPtr->cuOriginX,
                contextPtr->cuOriginY,
                contextPtr->cuStats->size,
                contextPtr->cuStats->size,
                contextPtr->cuStats->size,
                contextPtr->cuStats->depth,
                sequenceControlSetPtr->lcuSize,
                pictureControlSetPtr,
                pictureControlSetPtr->ParentPcsPtr->disableTmvpFlag ? EB_FALSE : EB_TRUE,
                lcuIndex,
                REF_LIST_0,
                contextPtr->xMvAmvpCandidateArrayList0,
                contextPtr->yMvAmvpCandidateArrayList0,
                &contextPtr->amvpCandidateCountRefList0);

            xMvdIdx0 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_0].x - contextPtr->xMvAmvpCandidateArrayList0[0]);
            yMvdIdx0 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_0].y - contextPtr->yMvAmvpCandidateArrayList0[0]);
            GetMvdFractionBits(xMvdIdx0, yMvdIdx0, contextPtr->mdRateEstimationPtr, &mvdBitsIdx0);

            if (contextPtr->amvpCandidateCountRefList0 > 1) {
                xMvdIdx1 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_0].x - contextPtr->xMvAmvpCandidateArrayList0[1]);
                yMvdIdx1 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_0].y - contextPtr->yMvAmvpCandidateArrayList0[1]);
                GetMvdFractionBits(xMvdIdx1, yMvdIdx1, contextPtr->mdRateEstimationPtr, &mvdBitsIdx1);

                // Assign the AMVP predictor index 
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].predIdx = (mvdBitsIdx1 < mvdBitsIdx0);

                // Assign the MV Predictor
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].mvdX = contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].predIdx ? xMvdIdx1 : xMvdIdx0;
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].mvdY = contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].predIdx ? yMvdIdx1 : yMvdIdx0;
            }
            else {
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].predIdx = 0;

                // Assign the MV Predictor
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].mvdX = xMvdIdx0;
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_0].mvdY = yMvdIdx0;
            }
        }

        // Generate AMVP List
        if (contextPtr->cuPtr->predictionUnitArray->interPredDirectionIndex == UNI_PRED_LIST_1 ||
            contextPtr->cuPtr->predictionUnitArray->interPredDirectionIndex == BI_PRED)
        {
            FillAMVPCandidates(
                pictureControlSetPtr->epMvNeighborArray,
                pictureControlSetPtr->epModeTypeNeighborArray,
                contextPtr->cuOriginX,
                contextPtr->cuOriginY,
                contextPtr->cuStats->size,
                contextPtr->cuStats->size,
                contextPtr->cuStats->size,
                contextPtr->cuStats->depth,
                sequenceControlSetPtr->lcuSize,
                pictureControlSetPtr,
                pictureControlSetPtr->ParentPcsPtr->disableTmvpFlag ? EB_FALSE : EB_TRUE,
                lcuIndex,
                REF_LIST_1,
                contextPtr->xMvAmvpCandidateArrayList1,
                contextPtr->yMvAmvpCandidateArrayList1,
                &contextPtr->amvpCandidateCountRefList1);

            // Assign the MV Predictor
            xMvdIdx0 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_1].x - contextPtr->xMvAmvpCandidateArrayList1[0]);
            yMvdIdx0 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_1].y - contextPtr->yMvAmvpCandidateArrayList1[0]);
            GetMvdFractionBits(xMvdIdx0, yMvdIdx0, contextPtr->mdRateEstimationPtr, &mvdBitsIdx0);

            if (contextPtr->amvpCandidateCountRefList1 > 1) {
                xMvdIdx1 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_1].x - contextPtr->xMvAmvpCandidateArrayList1[1]);
                yMvdIdx1 = (contextPtr->cuPtr->predictionUnitArray->mv[REF_LIST_1].y - contextPtr->yMvAmvpCandidateArrayList1[1]);
                GetMvdFractionBits(xMvdIdx1, yMvdIdx1, contextPtr->mdRateEstimationPtr, &mvdBitsIdx1);

                // Assign the AMVP predictor index 
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].predIdx = (mvdBitsIdx1 < mvdBitsIdx0);
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].mvdX = contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].predIdx ? xMvdIdx1 : xMvdIdx0;
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].mvdY = contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].predIdx ? yMvdIdx1 : yMvdIdx0;
            }
            else {
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].predIdx = 0;
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].mvdX = xMvdIdx0;
                contextPtr->cuPtr->predictionUnitArray->mvd[REF_LIST_1].mvdY = yMvdIdx0;
            }

            // Assign the MV Predictor

        }


    }

    return;
}

/*******************************************
* Encode Pass - Assign Delta Qp
*******************************************/
static void EncodePassUpdateQp(
    PictureControlSet_t     *pictureControlSetPtr,
    EncDecContext_t         *contextPtr,
    EB_BOOL                  availableCoeff,
    EB_BOOL                  isDeltaQpEnable,
    EB_BOOL                 *isDeltaQpNotCoded,
    EB_U32                   difCuDeltaQpDepth,
    EB_U8                   *prevCodedQp,
    EB_U8                   *prevQuantGroupCodedQp,
    EB_U32                   lcuQp
    )
{

    EB_U32 refQp;
    EB_U8 qp;

    EB_U32  log2MinCuQpDeltaSize = LOG2F_MAX_LCU_SIZE - difCuDeltaQpDepth;
    EB_S32  qpTopNeighbor = 0;
    EB_S32  qpLeftNeighbor = 0;
    EB_BOOL newQuantGroup;
    EB_U32  quantGroupX = contextPtr->cuOriginX - (contextPtr->cuOriginX & ((1 << log2MinCuQpDeltaSize) - 1));
    EB_U32  quantGroupY = contextPtr->cuOriginY - (contextPtr->cuOriginY & ((1 << log2MinCuQpDeltaSize) - 1));
    EB_BOOL sameLcuCheckTop = (((quantGroupY - 1)  >> LOG2F_MAX_LCU_SIZE) == ((quantGroupY)  >> LOG2F_MAX_LCU_SIZE)) ? EB_TRUE : EB_FALSE;
    EB_BOOL sameLcuCheckLeft = (((quantGroupX - 1) >> LOG2F_MAX_LCU_SIZE) == ((quantGroupX)  >> LOG2F_MAX_LCU_SIZE)) ? EB_TRUE : EB_FALSE;
    // Neighbor Array
    EB_U32 qpLeftNeighborIndex = 0;
    EB_U32 qpTopNeighborIndex = 0;

    // CU larger than the quantization group
    if (Log2f(contextPtr->cuStats->size) >= log2MinCuQpDeltaSize){
        *isDeltaQpNotCoded = EB_TRUE;
    }

    // At the beginning of a new quantization group
    if (((contextPtr->cuOriginX & ((1 << log2MinCuQpDeltaSize) - 1)) == 0) &&
        ((contextPtr->cuOriginY & ((1 << log2MinCuQpDeltaSize) - 1)) == 0))
    {
        *isDeltaQpNotCoded = EB_TRUE;
        newQuantGroup = EB_TRUE;
    }
    else {
        newQuantGroup = EB_FALSE;
    }

    // setting the previous Quantization Group QP
    if (newQuantGroup == EB_TRUE) {
        *prevCodedQp = *prevQuantGroupCodedQp;
    }

    if (sameLcuCheckTop)  {
        qpTopNeighborIndex =
            LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            quantGroupX,
            quantGroupY - 1,
            pictureControlSetPtr->qpArrayStride);
        qpTopNeighbor = pictureControlSetPtr->qpArray[qpTopNeighborIndex];
    }
    else {
        qpTopNeighbor = *prevCodedQp;
    }

    if (sameLcuCheckLeft)  {
        qpLeftNeighborIndex =
            LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            quantGroupX - 1,
            quantGroupY,
            pictureControlSetPtr->qpArrayStride);

        qpLeftNeighbor = pictureControlSetPtr->qpArray[qpLeftNeighborIndex];
    }
    else {
        qpLeftNeighbor = *prevCodedQp;
    }

    refQp = (qpLeftNeighbor + qpTopNeighbor + 1) >> 1;

    qp = (EB_U8)contextPtr->cuPtr->qp;
    // Update the State info
    if (isDeltaQpEnable) {
        if (*isDeltaQpNotCoded) {
            if (availableCoeff){
                qp = (EB_U8)contextPtr->cuPtr->qp;
                *prevCodedQp = qp;
                *prevQuantGroupCodedQp = qp;
                *isDeltaQpNotCoded = EB_FALSE;
            }
            else{
                qp = (EB_U8)refQp;
                *prevQuantGroupCodedQp = qp;
            }
        }
    }
    else{
        qp = (EB_U8)lcuQp;
    }
    contextPtr->cuPtr->qp = qp;
    return;
}

void SetPmEncDecMode(
    PictureControlSet_t     *pictureControlSetPtr,
    EncDecContext_t		    *contextPtr,
    EB_U32                   lcuIndex,
    EB_U8                    stationaryEdgeOverTimeFlag,
    EB_U8                    pmStationaryEdgeOverTimeFlag){



    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->ParentPcsPtr->sequenceControlSetWrapperPtr->objectPtr;



    contextPtr->cleanSparseCeoffPfEncDec    = 0;

    contextPtr->pmpMaskingLevelEncDec       = 0;

    EB_BOOL pmSensitiveUncoveredBackground = EB_FALSE;
    // Derived for REF P & B & kept false otherwise (for temporal distance equal to 1 uncovered area are easier to handle)
    if (pictureControlSetPtr->sliceType != EB_I_SLICE) {
        if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            EbReferenceObject_t  * refObjL0;
            refObjL0 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
            pmSensitiveUncoveredBackground = ((pictureControlSetPtr->ParentPcsPtr->failingMotionLcuFlag[lcuIndex] || contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) && (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex] < PM_NON_MOVING_INDEX_TH || refObjL0->nonMovingIndexArray[lcuIndex] < PM_NON_MOVING_INDEX_TH));
        }
    }

    EB_BOOL pmSensitiveComplexArea = EB_FALSE;
    // Derived for all frames
    pmSensitiveComplexArea = pictureControlSetPtr->highIntraSlection && pictureControlSetPtr->ParentPcsPtr->complexLcuArray[lcuIndex] == LCU_COMPLEXITY_STATUS_1;


    EB_BOOL pmSensitiveSkinArea = EB_FALSE;
    LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]);
    if (pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_1) {
        if (lcuStatPtr->cuStatArray[0].skinArea) {
            pmSensitiveSkinArea = EB_TRUE;
        }
    }

    EB_BOOL pmSensitiveCmplxContrastArea = EB_FALSE;
    if (pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_2) {
        if (pictureControlSetPtr->ParentPcsPtr->lcuCmplxContrastArray[lcuIndex]) {
            pmSensitiveCmplxContrastArea = EB_TRUE;
        }
    }

	if (sequenceControlSetPtr->staticConfig.bitRateReduction == EB_TRUE && !contextPtr->forceCbfFlag && !((pictureControlSetPtr->sliceType == EB_I_SLICE && contextPtr->cuStats->size == 8) || stationaryEdgeOverTimeFlag || pmSensitiveSkinArea || pmSensitiveCmplxContrastArea)) {

		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {

            if (pictureControlSetPtr->sliceType != EB_I_SLICE) {

                if (stationaryEdgeOverTimeFlag || pictureControlSetPtr->ParentPcsPtr->logoPicFlag || pmSensitiveUncoveredBackground || pmSensitiveComplexArea) {

                    contextPtr->pmpMaskingLevelEncDec = 1;
                }
                else if (pmStationaryEdgeOverTimeFlag){
                    contextPtr->pmpMaskingLevelEncDec = 2;
                }
                else
                {
                    if (pictureControlSetPtr->temporalLayerIndex == 0) {

                        if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) {
                            contextPtr->pmpMaskingLevelEncDec = 2;
                        }
                        else{
                            contextPtr->pmpMaskingLevelEncDec = 3;
                        }
                    }
                    else {
                        contextPtr->cleanSparseCeoffPfEncDec = 1;
                        if (pictureControlSetPtr->ParentPcsPtr->highDarkLowLightAreaDensityFlag && pictureControlSetPtr->ParentPcsPtr->sharpEdgeLcuFlag[lcuIndex] && !pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArrayAllLayers[lcuIndex]){
                            contextPtr->pmpMaskingLevelEncDec = 2;
                        }
                        else
                        {

                            if (pictureControlSetPtr->temporalLayerIndex == 3) {
                                if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) {
                                    contextPtr->pmpMaskingLevelEncDec = 6;
                                }
                                else{
                                    contextPtr->pmpMaskingLevelEncDec = 7;
                                }
                            }
                            else{
                                if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) {
                                    contextPtr->pmpMaskingLevelEncDec = 4;
                                }
                                else{
                                    contextPtr->pmpMaskingLevelEncDec = 5;
                                }
                            }
                        }
                    }
                }

            }
            else{
                if (stationaryEdgeOverTimeFlag == 0 && !pictureControlSetPtr->ParentPcsPtr->logoPicFlag)
                {
                    contextPtr->pmpMaskingLevelEncDec = 1;
                }
            }


            if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == 32 && contextPtr->cuPtr->predictionUnitArray->intraLumaMode != EB_INTRA_DC && contextPtr->cuPtr->predictionUnitArray->intraLumaMode != EB_INTRA_PLANAR) {
                contextPtr->pmpMaskingLevelEncDec = 0;
            }



            if (pictureControlSetPtr->sliceType == EB_P_SLICE) {
                contextPtr->pmpMaskingLevelEncDec = 1;
            }


        }
        else{

            if (stationaryEdgeOverTimeFlag == 0 && !pictureControlSetPtr->ParentPcsPtr->logoPicFlag)
            {

                if (pictureControlSetPtr->temporalLayerIndex > 0 && !pmSensitiveUncoveredBackground && !pmSensitiveComplexArea) {
                    contextPtr->cleanSparseCeoffPfEncDec = 1;
                }
                if (pictureControlSetPtr->sliceType != EB_I_SLICE) {
                    {
                        if (stationaryEdgeOverTimeFlag || pictureControlSetPtr->ParentPcsPtr->logoPicFlag){
                            contextPtr->pmpMaskingLevelEncDec = 0;
                        }
                        if (pmSensitiveUncoveredBackground || pmSensitiveComplexArea) {
                            contextPtr->pmpMaskingLevelEncDec = 1;
                        }
                        else
                        {

                            if (pictureControlSetPtr->temporalLayerIndex == 0) {

                                if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) {
                                    contextPtr->pmpMaskingLevelEncDec = 2;
                                }
                                else{
                                    contextPtr->pmpMaskingLevelEncDec = 3;
                                }
                            }
                            else {

                                if (pictureControlSetPtr->temporalLayerIndex == 3) {
                                    if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) {
                                        contextPtr->pmpMaskingLevelEncDec = 6;
                                    }
                                    else{
                                        contextPtr->pmpMaskingLevelEncDec = 7;
                                    }
                                }
                                else{
                                    if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE) {
                                        contextPtr->pmpMaskingLevelEncDec = 4;
                                    }
                                    else{
                                        contextPtr->pmpMaskingLevelEncDec = 5;
                                    }
                                }
                            }

			

							if (contextPtr->cuPtr->predictionModeFlag == INTRA_MODE && contextPtr->cuStats->size == 32 && contextPtr->cuPtr->predictionUnitArray->intraLumaMode != EB_INTRA_DC && contextPtr->cuPtr->predictionUnitArray->intraLumaMode != EB_INTRA_PLANAR) {
								contextPtr->pmpMaskingLevelEncDec = 0;
							}

							if (pictureControlSetPtr->sliceType == EB_P_SLICE) {
								contextPtr->pmpMaskingLevelEncDec = 1;
							}


                        }
                    }
                }
                else{

					contextPtr->pmpMaskingLevelEncDec = 0;

                }
            }

        }
    }
}


void Pack2DBlock(
    EncDecContext_t        *contextPtr,
    EbPictureBufferDesc_t  *inputPicture,
    EB_U32					originX,
    EB_U32					originY,
    EB_U32					width,
    EB_U32					height) {

    const EB_U32 inputLumaOffset = ((originY + inputPicture->originY)      * inputPicture->strideY) + (originX + inputPicture->originX);
    const EB_U32 inputBitIncLumaOffset = ((originY + inputPicture->originY)      * inputPicture->strideBitIncY) + (originX + inputPicture->originX);
    const EB_U32 inputCbOffset = (((originY + inputPicture->originY) >> 1) * inputPicture->strideCb) + ((originX + inputPicture->originX) >> 1);
    const EB_U32 inputBitIncCbOffset = (((originY + inputPicture->originY) >> 1) * inputPicture->strideBitIncCb) + ((originX + inputPicture->originX) >> 1);
    const EB_U32 inputCrOffset = (((originY + inputPicture->originY) >> 1) * inputPicture->strideCr) + ((originX + inputPicture->originX) >> 1);
    const EB_U32 inputBitIncCrOffset = (((originY + inputPicture->originY) >> 1) * inputPicture->strideBitIncCr) + ((originX + inputPicture->originX) >> 1);


    const EB_U32 blockLumaOffset = ((originY % 64) * contextPtr->inputSample16bitBuffer->strideY) + (originX % 64);
    const EB_U32 blockCbOffset = (((originY % 64) >> 1) * contextPtr->inputSample16bitBuffer->strideCb) + ((originX % 64) >> 1);
    const EB_U32 blockCrOffset = (((originY % 64) >> 1) * contextPtr->inputSample16bitBuffer->strideCr) + ((originX % 64) >> 1);

    {
        Pack2D_SRC(
            inputPicture->bufferY + inputLumaOffset,
            inputPicture->strideY,
            inputPicture->bufferBitIncY + inputBitIncLumaOffset,
            inputPicture->strideBitIncY,
            ((EB_U16 *)(contextPtr->inputSample16bitBuffer->bufferY)) + blockLumaOffset,
            contextPtr->inputSample16bitBuffer->strideY,
            width,
            height);  //this should be depending on a configuration param

        Pack2D_SRC(
            inputPicture->bufferCb + inputCbOffset,
            inputPicture->strideCr,
            inputPicture->bufferBitIncCb + inputBitIncCbOffset,
            inputPicture->strideBitIncCr,
            ((EB_U16 *)(contextPtr->inputSample16bitBuffer->bufferCb)) + blockCbOffset,
            contextPtr->inputSample16bitBuffer->strideCb,
            width >> 1,
            height >> 1);  //this should be depending on a configuration param

        Pack2D_SRC(
            inputPicture->bufferCr + inputCrOffset,
            inputPicture->strideCr,
            inputPicture->bufferBitIncCr + inputBitIncCrOffset,
            inputPicture->strideBitIncCr,
            ((EB_U16 *)(contextPtr->inputSample16bitBuffer->bufferCr)) + blockCrOffset,
            contextPtr->inputSample16bitBuffer->strideCr,
            width >> 1,
            height >> 1); //this should be depending on a configuration param

    }

}

EB_ERRORTYPE QpmDeriveBeaAndSkipQpmFlagLcu(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    LargestCodingUnit_t                    *lcuPtr,
    EB_U32                                 lcuIndex,
    EncDecContext_t                        *contextPtr)
{

    EB_ERRORTYPE                    return_error = EB_ErrorNone;
    EB_U8                           pictureQp = pictureControlSetPtr->pictureQp;
    EB_U8                           minQpAllowed = (EB_U8)sequenceControlSetPtr->staticConfig.minQpAllowed;
    EB_U8                           maxQpAllowed = (EB_U8)sequenceControlSetPtr->staticConfig.maxQpAllowed;


	contextPtr->qpmQp = pictureQp;

    LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]);


    contextPtr->nonMovingDeltaQp = 0;

    contextPtr->grassEnhancementFlag = ((pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_1) && (lcuStatPtr->cuStatArray[0].grassArea)
        && (lcuPtr->pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex].edgeBlockNum > 0))
        ? EB_TRUE : EB_FALSE;

    contextPtr->backgorundEnhancement = EB_FALSE;


	contextPtr->backgorundEnhancement = EB_FALSE;

	contextPtr->skipQpmFlag = (sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction) ? EB_FALSE : EB_TRUE;

	if ((pictureControlSetPtr->ParentPcsPtr->logoPicFlag == EB_FALSE) && ((pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_3_1) || (pictureControlSetPtr->ParentPcsPtr->highDarkLowLightAreaDensityFlag) || (pictureControlSetPtr->ParentPcsPtr->intraCodedBlockProbability > 90))){
		contextPtr->skipQpmFlag = EB_TRUE;
	}

	if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE){
		contextPtr->skipQpmFlag = EB_TRUE;
	}

    if (contextPtr->skipQpmFlag == EB_FALSE) {
        if (pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage > 30 && pictureControlSetPtr->sliceType != EB_I_SLICE){
			contextPtr->qpmQp = CLIP3(minQpAllowed, maxQpAllowed, pictureQp + 1);
        }
    }

	return return_error;
}

EB_ERRORTYPE EncQpmDeriveDeltaQPForEachLeafLcu(
	SequenceControlSet_t                   *sequenceControlSetPtr,
	PictureControlSet_t                    *pictureControlSetPtr,
	LargestCodingUnit_t                    *lcuPtr,
	EB_U32                                  lcuIndex,
	CodingUnit_t                           *cuPtr,
	EB_U32                                  cuDepth,
	EB_U32                                  cuIndex,
	EB_U32                                  cuSize,
	EB_U8                                   type,
	EB_U8                                   parent32x32Index,
	EncDecContext_t                        *contextPtr)
{
	EB_ERRORTYPE                    return_error = EB_ErrorNone;


	//LcuParams_t				        lcuParams;
	EB_S64                          complexityDistance;
	EB_S8                           deltaQp = 0;
	EB_U8                           qpmQp = contextPtr->qpmQp;
	EB_U8                           minQpAllowed = (EB_U8)sequenceControlSetPtr->staticConfig.minQpAllowed;
	EB_U8                           maxQpAllowed = (EB_U8)sequenceControlSetPtr->staticConfig.maxQpAllowed;
	EB_U8                           cuQP;

    EB_BOOL  skipOis8x8  = (pictureControlSetPtr->ParentPcsPtr->skipOis8x8 && cuSize == 8);

	EB_U32 usedDepth = cuDepth;
	if (skipOis8x8)
		usedDepth = 2;

	EB_U32 cuIndexInRaterScan = MD_SCAN_TO_RASTER_SCAN[cuIndex];

	EB_BOOL acEnergyBasedAntiContouring = pictureControlSetPtr->sliceType == EB_I_SLICE ? EB_TRUE : EB_FALSE;
	EB_U8   lowerQPClass;
	
	EB_S8	nonMovingDeltaQp = contextPtr->nonMovingDeltaQp;

	EB_S8	bea64x64DeltaQp;

	cuQP = qpmQp;
	cuPtr->qp = qpmQp;

	EB_U32  distortion = 0;

	if (!contextPtr->skipQpmFlag){

		// INTRA MODE
		if (type == INTRA_MODE){
			
				OisCu32Cu16Results_t  *oisCu32Cu16ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuIndex];
				OisCu8Results_t   	  *oisCu8ResultsPtr      = pictureControlSetPtr->ParentPcsPtr->oisCu8Results[lcuIndex];

				if (cuSize > 32){
					distortion =
						oisCu32Cu16ResultsPtr->sortedOisCandidate[1][0].distortion +
						oisCu32Cu16ResultsPtr->sortedOisCandidate[2][0].distortion +
						oisCu32Cu16ResultsPtr->sortedOisCandidate[3][0].distortion +
						oisCu32Cu16ResultsPtr->sortedOisCandidate[4][0].distortion;
				}
				else if (cuSize == 32) {
					const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth + me2Nx2NOffset[contextPtr->cuStats->depth];
					distortion = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
				}
				else{
					if (cuSize > 8){
						const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth + me2Nx2NOffset[contextPtr->cuStats->depth];
						distortion = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
					}
					else{
						if (skipOis8x8){

							const CodedUnitStats_t  *cuStats = GetCodedUnitStats(ParentBlockIndex[cuIndex]);
							const EB_U32 me2Nx2NTableOffset = cuStats->cuNumInDepth + me2Nx2NOffset[cuStats->depth];

							distortion = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
						}
						else {								

							const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth;

							if (oisCu8ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].validDistortion){
								distortion = oisCu8ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
							}
							else{

								const CodedUnitStats_t  *cuStats = GetCodedUnitStats(ParentBlockIndex[cuIndex]);
								const EB_U32 me2Nx2NTableOffset = cuStats->cuNumInDepth + me2Nx2NOffset[cuStats->depth];

								if (oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].validDistortion){
									distortion = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
								}
								else {
									distortion = 0;
								}
							}

						}
					}
				}
				
				
				distortion = (EB_U32)CLIP3(pictureControlSetPtr->ParentPcsPtr->intraComplexityMin[usedDepth], pictureControlSetPtr->ParentPcsPtr->intraComplexityMax[usedDepth], distortion);
				complexityDistance = ((EB_S32)distortion - (EB_S32)pictureControlSetPtr->ParentPcsPtr->intraComplexityAvg[usedDepth]);

				if (complexityDistance < 0){

					deltaQp = (pictureControlSetPtr->ParentPcsPtr->intraMinDistance[usedDepth] != 0) ? (EB_S8)((contextPtr->minDeltaQpWeight * contextPtr->minDeltaQp[usedDepth] * complexityDistance) / (100 * pictureControlSetPtr->ParentPcsPtr->intraMinDistance[usedDepth])) : 0;
				}
				else{

					deltaQp = (pictureControlSetPtr->ParentPcsPtr->intraMaxDistance[usedDepth] != 0) ? (EB_S8)((contextPtr->maxDeltaQpWeight * contextPtr->maxDeltaQp[usedDepth] * complexityDistance) / (100 * pictureControlSetPtr->ParentPcsPtr->intraMaxDistance[usedDepth])) : 0;
				}
				// QPM action
				if (lcuPtr->pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_2) {
					if (lcuPtr->pictureControlSetPtr->ParentPcsPtr->lcuCmplxContrastArray[lcuIndex] && deltaQp > 0) {
						deltaQp = 0;
					}
				}
			}
			// INTER MODE
			else{
				distortion = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex][cuIndexInRaterScan].distortionDirection[0].distortion;
				if (skipOis8x8){
					EB_U32 cuIndexRScan = MD_SCAN_TO_RASTER_SCAN[ParentBlockIndex[cuIndex]];

					distortion = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex][cuIndexRScan].distortionDirection[0].distortion;

				}
				distortion = (EB_U32)CLIP3(pictureControlSetPtr->ParentPcsPtr->interComplexityMin[usedDepth], pictureControlSetPtr->ParentPcsPtr->interComplexityMax[usedDepth], distortion);
				complexityDistance = ((EB_S32)distortion - (EB_S32)pictureControlSetPtr->ParentPcsPtr->interComplexityAvg[usedDepth]);

				if (complexityDistance < 0){

					deltaQp = (pictureControlSetPtr->ParentPcsPtr->interMinDistance[usedDepth] != 0) ? (EB_S8)((contextPtr->minDeltaQpWeight * contextPtr->minDeltaQp[usedDepth] * complexityDistance) / (100 * pictureControlSetPtr->ParentPcsPtr->interMinDistance[usedDepth])) : 0;
				}
				else{

					deltaQp = (pictureControlSetPtr->ParentPcsPtr->interMaxDistance[usedDepth] != 0) ? (EB_S8)((contextPtr->maxDeltaQpWeight * contextPtr->maxDeltaQp[usedDepth] * complexityDistance) / (100 * pictureControlSetPtr->ParentPcsPtr->interMaxDistance[usedDepth])) : 0;
				}
			}

		if (contextPtr->backgorundEnhancement){
			// Use the 8x8 background enhancement only for the Intra slice, otherwise, use the existing LCU based BEA results 
			bea64x64DeltaQp = nonMovingDeltaQp;

			if (((cuIndex > 0) && ((pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][parent32x32Index]) > ANTI_CONTOURING_LUMA_T2 || (pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][parent32x32Index]) < ANTI_CONTOURING_LUMA_T1)) ||
				((cuIndex == 0) && ((pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][0]) > ANTI_CONTOURING_LUMA_T2 || (pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][0]) < ANTI_CONTOURING_LUMA_T1))) {

				if (bea64x64DeltaQp < 0){
					bea64x64DeltaQp = 0;
				}

			}

            deltaQp += bea64x64DeltaQp;
		}

        if ((pictureControlSetPtr->ParentPcsPtr->logoPicFlag)){
			deltaQp = (deltaQp < contextPtr->minDeltaQp[0]) ? deltaQp : contextPtr->minDeltaQp[0];
		}

        LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]);
        if (lcuStatPtr->stationaryEdgeOverTimeFlag && deltaQp > 0){
            deltaQp = 0;
        }            
		// QPM action
        if (lcuPtr->pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_2) {
            if (lcuPtr->pictureControlSetPtr->ParentPcsPtr->lcuCmplxContrastArray[lcuIndex] && deltaQp > 0) {
                deltaQp = 0;
            }
        }

        if (acEnergyBasedAntiContouring) {

            lowerQPClass = DeriveContouringClass(
                lcuPtr->pictureControlSetPtr->ParentPcsPtr,
                lcuPtr->index,
                (EB_U8) cuIndex);

		    if (lowerQPClass){
                if (lowerQPClass == 3) 
                    deltaQp = ANTI_CONTOURING_DELTA_QP_0;
                else if (lowerQPClass == 2) 
                    deltaQp = ANTI_CONTOURING_DELTA_QP_1;
                 else if (lowerQPClass == 1) 
                     deltaQp = ANTI_CONTOURING_DELTA_QP_2;
		    }
        }


        deltaQp -= contextPtr->grassEnhancementFlag ? 3 : 0;

		deltaQp = ((deltaQp < 0 && sequenceControlSetPtr->staticConfig.bitRateReduction && !sequenceControlSetPtr->staticConfig.improveSharpness) ||
			(deltaQp > 0 && sequenceControlSetPtr->staticConfig.improveSharpness && !sequenceControlSetPtr->staticConfig.bitRateReduction)) ? 0 : deltaQp;

		if (sequenceControlSetPtr->staticConfig.rateControlMode == 1 || sequenceControlSetPtr->staticConfig.rateControlMode == 2){

			if (qpmQp > RC_QPMOD_MAXQP){
				deltaQp = MIN(0, deltaQp);
			}

			cuQP = (EB_U32)(qpmQp + deltaQp);


			if ((qpmQp <= RC_QPMOD_MAXQP)){
				cuQP = (EB_U8)CLIP3(
					minQpAllowed,
					RC_QPMOD_MAXQP,
					cuQP);
			}
		}
		else{
            cuQP = (EB_U8)(qpmQp + deltaQp);
		}

		cuQP = (EB_U8)CLIP3(
			minQpAllowed,
			maxQpAllowed,
			cuQP);


	}

    cuPtr->qp = cuQP ;

	lcuPtr->qp = (cuSize == 64) ? (EB_U8)cuPtr->qp : lcuPtr->qp;


	cuPtr->deltaQp = (EB_S16)cuPtr->qp - (EB_S16)qpmQp;

	cuPtr->orgDeltaQp = cuPtr->deltaQp;



	return return_error;
}


/************************************
this function checks whether any intra
CU is present in the current LCU
*************************************/
EB_BOOL isIntraPresent(
	LargestCodingUnit_t                 *lcuPtr)
{
	EB_U8 leafIndex = 0;
	while (leafIndex < CU_MAX_COUNT) {

		CodingUnit_t * const cuPtr = lcuPtr->codedLeafArrayPtr[leafIndex];

		if (cuPtr->splitFlag == EB_FALSE) {

			const CodedUnitStats_t *cuStatsPtr = GetCodedUnitStats(leafIndex);
			if (cuPtr->predictionModeFlag == INTRA_MODE)
				return EB_TRUE;


			leafIndex += DepthOffset[cuStatsPtr->depth];
		}
		else {
			leafIndex++;
		}
	}

	return EB_FALSE;

}


void EncodePassPreFetchRef(
    PictureControlSet_t     *pictureControlSetPtr,
    EncDecContext_t         *contextPtr,
    CodingUnit_t            *cuPtr, 
    const CodedUnitStats_t  *cuStats,
    PredictionUnit_t        *puPtr,
    EB_BOOL                  is16bit)
{

    if (cuPtr->predictionModeFlag == INTER_MODE){

        if (is16bit)
        {
            puPtr = cuPtr->predictionUnitArray;
            contextPtr->mvUnit.predDirection = (EB_U8)puPtr->interPredDirectionIndex;
            contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
            contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;

            if ((contextPtr->mvUnit.predDirection == UNI_PRED_LIST_0) || (contextPtr->mvUnit.predDirection == BI_PRED))
            {

                EbPictureBufferDesc_t  *refPicList0 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U16                  refList0PosX = 0;
                EB_U16                  refList0PosY = 0;
                EB_U8					counter;
                EB_U16				   *src0Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
                refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

                refList0PosX = (EB_U32)CLIP3(
                    (EB_S32)((refPicList0->originX - 71) << 2),
                    (EB_S32)((refPicList0->width + refPicList0->originX + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginX + refPicList0->originX) << 2) + contextPtr->mvUnit.mv[REF_LIST_0].x);

                refList0PosY = (EB_U32)CLIP3(
                    (EB_S32)((refPicList0->originY - 71) << 2),
                    (EB_S32)((refPicList0->height + refPicList0->originY + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginY + refPicList0->originY) << 2) + contextPtr->mvUnit.mv[REF_LIST_0].y);

                EB_U32  lumaOffSet = ((refList0PosX >> 2) - 4) * 2 + ((refList0PosY >> 2) - 4) * 2 * refPicList0->strideY;
                EB_U32  cbOffset = ((refList0PosX >> 3) - 2) * 2 + ((refList0PosY >> 3) - 2) * 2 * refPicList0->strideCb;
                EB_U32  crOffset = ((refList0PosX >> 3) - 2) * 2 + ((refList0PosY >> 3) - 2) * 2 * refPicList0->strideCr;


                contextPtr->mcpContext->localReferenceBlockL0->bufferY = refPicList0->bufferY + lumaOffSet;
                contextPtr->mcpContext->localReferenceBlockL0->bufferCb = refPicList0->bufferCb + cbOffset;
                contextPtr->mcpContext->localReferenceBlockL0->bufferCr = refPicList0->bufferCr + crOffset;
                contextPtr->mcpContext->localReferenceBlockL0->strideY = refPicList0->strideY;
                contextPtr->mcpContext->localReferenceBlockL0->strideCb = refPicList0->strideCb;
                contextPtr->mcpContext->localReferenceBlockL0->strideCr = refPicList0->strideCr;


                src0Ptr = (EB_U16 *)contextPtr->mcpContext->localReferenceBlockL0->bufferY + 4 + 4 * contextPtr->mcpContext->localReferenceBlockL0->strideY;

                for (counter = 0; counter < cuStats->size; counter++)
                {
                    char const* p0 = (char const*)(src0Ptr + counter*contextPtr->mcpContext->localReferenceBlockL0->strideY);
                    _mm_prefetch(p0, _MM_HINT_T2);
                    char const* p1 = (char const*)(src0Ptr + counter*contextPtr->mcpContext->localReferenceBlockL0->strideY + (cuStats->size >> 1));
                    _mm_prefetch(p1, _MM_HINT_T2);
                }

            }

            if ((contextPtr->mvUnit.predDirection == UNI_PRED_LIST_1) || (contextPtr->mvUnit.predDirection == BI_PRED))
            {
                // Setup List 0
                EbPictureBufferDesc_t  *refPicList1 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U16                  refList1PosX = 0;
                EB_U16                  refList1PosY = 0;
                EB_U8					counter;
                EB_U16				   *src1Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
                refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

                refList1PosX = (EB_U32)CLIP3(
                    (EB_S32)((refPicList1->originX - 71) << 2),
                    (EB_S32)((refPicList1->width + refPicList1->originX + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginX + refPicList1->originX) << 2) + contextPtr->mvUnit.mv[REF_LIST_1].x);

                refList1PosY = (EB_U32)CLIP3(
                    (EB_S32)((refPicList1->originY - 71) << 2),
                    (EB_S32)((refPicList1->height + refPicList1->originY + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginY + refPicList1->originY) << 2) + contextPtr->mvUnit.mv[REF_LIST_1].y);

                EB_U32  lumaOffSet = ((refList1PosX >> 2) - 4) * 2 + ((refList1PosY >> 2) - 4) * 2 * refPicList1->strideY; //refPicList0->originX + refPicList0->originY*refPicList0->strideY; //
                EB_U32  cbOffset = ((refList1PosX >> 3) - 2) * 2 + ((refList1PosY >> 3) - 2) * 2 * refPicList1->strideCb;
                EB_U32  crOffset = ((refList1PosX >> 3) - 2) * 2 + ((refList1PosY >> 3) - 2) * 2 * refPicList1->strideCr;


                contextPtr->mcpContext->localReferenceBlockL1->bufferY = refPicList1->bufferY + lumaOffSet;
                contextPtr->mcpContext->localReferenceBlockL1->bufferCb = refPicList1->bufferCb + cbOffset;
                contextPtr->mcpContext->localReferenceBlockL1->bufferCr = refPicList1->bufferCr + crOffset;
                contextPtr->mcpContext->localReferenceBlockL1->strideY = refPicList1->strideY;
                contextPtr->mcpContext->localReferenceBlockL1->strideCb = refPicList1->strideCb;
                contextPtr->mcpContext->localReferenceBlockL1->strideCr = refPicList1->strideCr;


                src1Ptr = (EB_U16 *)contextPtr->mcpContext->localReferenceBlockL1->bufferY + 4 + 4 * contextPtr->mcpContext->localReferenceBlockL1->strideY;

                for (counter = 0; counter < cuStats->size; counter++)
                {
                    char const* p0 = (char const*)(src1Ptr + counter*contextPtr->mcpContext->localReferenceBlockL1->strideY);
                    _mm_prefetch(p0, _MM_HINT_T2);
                    char const* p1 = (char const*)(src1Ptr + counter*contextPtr->mcpContext->localReferenceBlockL1->strideY + (cuStats->size >> 1));
                    _mm_prefetch(p1, _MM_HINT_T2);
                }

            }
        }
        else
        {
            puPtr = cuPtr->predictionUnitArray;
            contextPtr->mvUnit.predDirection = (EB_U8)puPtr->interPredDirectionIndex;
            contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
            contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;

            if ((contextPtr->mvUnit.predDirection == UNI_PRED_LIST_0) || (contextPtr->mvUnit.predDirection == BI_PRED))
            {
                // Setup List 0
                EbPictureBufferDesc_t  *refPicList0 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U16                  refList0PosX = 0;
                EB_U16                  refList0PosY = 0;
                EB_U32					integPosL0x;
                EB_U32					integPosL0y;
                EB_U8					counter;
                EB_U8				   *src0Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
                refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

                refList0PosX = (EB_U32)CLIP3(
                    (EB_S32)((refPicList0->originX - 71) << 2),
                    (EB_S32)((refPicList0->width + refPicList0->originX + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginX + refPicList0->originX) << 2) + contextPtr->mvUnit.mv[REF_LIST_0].x);

                refList0PosY = (EB_U32)CLIP3(
                    (EB_S32)((refPicList0->originY - 71) << 2),
                    (EB_S32)((refPicList0->height + refPicList0->originY + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginY + refPicList0->originY) << 2) + contextPtr->mvUnit.mv[REF_LIST_0].y);


                //compute the luma fractional position
                integPosL0x = (refList0PosX >> 2);
                integPosL0y = (refList0PosY >> 2);


                src0Ptr = refPicList0->bufferY + integPosL0x + integPosL0y*refPicList0->strideY;
                for (counter = 0; counter < cuStats->size; counter++)
                {
                    char const* p0 = (char const*)(src0Ptr + counter*refPicList0->strideY);
                    _mm_prefetch(p0, _MM_HINT_T2);
                }

            }

            if ((contextPtr->mvUnit.predDirection == UNI_PRED_LIST_1) || (contextPtr->mvUnit.predDirection == BI_PRED))
            {
                // Setup List 0
                EbPictureBufferDesc_t  *refPicList1 = 0;
                EbReferenceObject_t    *referenceObject;
                EB_U16                  refList1PosX = 0;
                EB_U16                  refList1PosY = 0;
                EB_U32					integPosL1x;
                EB_U32					integPosL1y;
                EB_U8					counter;
                EB_U8				   *src1Ptr;

                referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
                refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

                refList1PosX = (EB_U32)CLIP3(
                    (EB_S32)((refPicList1->originX - 71) << 2),
                    (EB_S32)((refPicList1->width + refPicList1->originX + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginX + refPicList1->originX) << 2) + contextPtr->mvUnit.mv[REF_LIST_1].x);

                refList1PosY = (EB_U32)CLIP3(
                    (EB_S32)((refPicList1->originY - 71) << 2),
                    (EB_S32)((refPicList1->height + refPicList1->originY + 7) << 2),
                    (EB_S32)((contextPtr->cuOriginY + refPicList1->originY) << 2) + contextPtr->mvUnit.mv[REF_LIST_1].y);


                //uni-prediction List1 luma
                integPosL1x = (refList1PosX >> 2);
                integPosL1y = (refList1PosY >> 2);


                src1Ptr = refPicList1->bufferY + integPosL1x + integPosL1y*refPicList1->strideY;
                for (counter = 0; counter < cuStats->size; counter++)
                {
                    char const* p1 = (char const*)(src1Ptr + counter*refPicList1->strideY);
                    _mm_prefetch(p1, _MM_HINT_T2);
                }

            }
        }
    }
}


void EncodePassPackLcu(
    SequenceControlSet_t   *sequenceControlSetPtr,
    EbPictureBufferDesc_t  *inputPicture,
    EncDecContext_t        *contextPtr,
    EB_U32                  lcuOriginX,
    EB_U32                  lcuOriginY,
    EB_U32                  lcuWidth,
    EB_U32                  lcuHeight) {

    if ((sequenceControlSetPtr->staticConfig.compressedTenBitFormat == 1))
    {

        const EB_U32 inputLumaOffset = ((lcuOriginY + inputPicture->originY)       * inputPicture->strideY) + (lcuOriginX + inputPicture->originX);
        const EB_U32 inputCbOffset = (((lcuOriginY + inputPicture->originY) >> 1)  * inputPicture->strideCb) + ((lcuOriginX + inputPicture->originX) >> 1);
        const EB_U32 inputCrOffset = (((lcuOriginY + inputPicture->originY) >> 1)  * inputPicture->strideCr) + ((lcuOriginX + inputPicture->originX) >> 1);
        const EB_U16 luma2BitWidth = inputPicture->width / 4;
        const EB_U16 chroma2BitWidth = inputPicture->width / 8;


        CompressedPackLcu(
            inputPicture->bufferY + inputLumaOffset,
            inputPicture->strideY,
            inputPicture->bufferBitIncY + lcuOriginY*luma2BitWidth + (lcuOriginX / 4)*lcuHeight,
            lcuWidth / 4,
            (EB_U16 *)contextPtr->inputSample16bitBuffer->bufferY,
            MAX_LCU_SIZE,
            lcuWidth,
            lcuHeight);

        CompressedPackLcu(
            inputPicture->bufferCb + inputCbOffset,
            inputPicture->strideCb,
            inputPicture->bufferBitIncCb + lcuOriginY / 2 * chroma2BitWidth + (lcuOriginX / 8)*(lcuHeight / 2),
            lcuWidth / 8,
            (EB_U16 *)contextPtr->inputSample16bitBuffer->bufferCb,
            MAX_LCU_SIZE_CHROMA,
            lcuWidth >> 1,
            lcuHeight >> 1);

        CompressedPackLcu(
            inputPicture->bufferCr + inputCrOffset,
            inputPicture->strideCr,
            inputPicture->bufferBitIncCr + lcuOriginY / 2 * chroma2BitWidth + (lcuOriginX / 8)*(lcuHeight / 2),
            lcuWidth / 8,
            (EB_U16 *)contextPtr->inputSample16bitBuffer->bufferCr,
            MAX_LCU_SIZE_CHROMA,
            lcuWidth >> 1,
            lcuHeight >> 1);

    }
    else {

        const EB_U32 inputLumaOffset = ((lcuOriginY + inputPicture->originY)         * inputPicture->strideY) + (lcuOriginX + inputPicture->originX);
        const EB_U32 inputBitIncLumaOffset = ((lcuOriginY + inputPicture->originY)         * inputPicture->strideBitIncY) + (lcuOriginX + inputPicture->originX);
        const EB_U32 inputCbOffset = (((lcuOriginY + inputPicture->originY) >> 1)  * inputPicture->strideCb) + ((lcuOriginX + inputPicture->originX) >> 1);
        const EB_U32 inputBitIncCbOffset = (((lcuOriginY + inputPicture->originY) >> 1)  * inputPicture->strideBitIncCb) + ((lcuOriginX + inputPicture->originX) >> 1);
        const EB_U32 inputCrOffset = (((lcuOriginY + inputPicture->originY) >> 1)  * inputPicture->strideCr) + ((lcuOriginX + inputPicture->originX) >> 1);
        const EB_U32 inputBitIncCrOffset = (((lcuOriginY + inputPicture->originY) >> 1)  * inputPicture->strideBitIncCr) + ((lcuOriginX + inputPicture->originX) >> 1);

        Pack2D_SRC(
            inputPicture->bufferY + inputLumaOffset,
            inputPicture->strideY,
            inputPicture->bufferBitIncY + inputBitIncLumaOffset,
            inputPicture->strideBitIncY,
            (EB_U16 *)contextPtr->inputSample16bitBuffer->bufferY,
            MAX_LCU_SIZE,
            lcuWidth,
            lcuHeight);


        Pack2D_SRC(
            inputPicture->bufferCb + inputCbOffset,
            inputPicture->strideCr,
            inputPicture->bufferBitIncCb + inputBitIncCbOffset,
            inputPicture->strideBitIncCr,
            (EB_U16 *)contextPtr->inputSample16bitBuffer->bufferCb,
            MAX_LCU_SIZE_CHROMA,
            lcuWidth >> 1,
            lcuHeight >> 1);


        Pack2D_SRC(
            inputPicture->bufferCr + inputCrOffset,
            inputPicture->strideCr,
            inputPicture->bufferBitIncCr + inputBitIncCrOffset,
            inputPicture->strideBitIncCr,
            (EB_U16 *)contextPtr->inputSample16bitBuffer->bufferCr,
            MAX_LCU_SIZE_CHROMA,
            lcuWidth >> 1,
            lcuHeight >> 1);
    }
}


/*******************************************
* Encode Pass
*
* Summary: Performs a H.265 conformant
*   reconstruction based on the LCU
*   mode decision.
*
* Inputs:
*   SourcePic
*   Coding Results
*   LCU Location
*   Sequence Control Set
*   Picture Control Set
*
* Outputs:
*   Reconstructed Samples
*   Coefficient Samples
*
*******************************************/
EB_EXTERN void EncodePass(
    SequenceControlSet_t    *sequenceControlSetPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    LargestCodingUnit_t     *lcuPtr,
    EB_U32                   tbAddr,
    EB_U32                   lcuOriginX,
    EB_U32                   lcuOriginY,
    EB_U32                   lcuQp,
    EB_BOOL                  enableSaoFlag,
    EncDecContext_t         *contextPtr)
{

    EB_BOOL                 is16bit = contextPtr->is16bit;

    EbPictureBufferDesc_t *reconBuffer = is16bit ? pictureControlSetPtr->reconPicture16bitPtr : pictureControlSetPtr->reconPicturePtr;
    EbPictureBufferDesc_t *coeffBufferTB = lcuPtr->quantizedCoeff;

    EbPictureBufferDesc_t *inputPicture;
    ModeDecisionContext_t *mdcontextPtr;

    mdcontextPtr = contextPtr->mdContext;
    inputPicture = contextPtr->inputSamples = (EbPictureBufferDesc_t*)pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

    LcuStat_t               *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[tbAddr]);

    // TMVP
    TmvpUnit_t             *tmvpMapWritePtr;
    EB_U32                  tmvpMapHorizontalStartIndex;
    EB_U32                  tmvpMapVerticalStartIndex;
    EB_U32                  tmvpMapHorizontalEndIndex;
    EB_U32                  tmvpMapVerticalEndIndex;
    EB_U32                  tmvpMapIndex;
    EB_U32                  mvCompressionUnitSizeMinus1 = (1 << LOG_MV_COMPRESS_UNIT_SIZE) - 1;

    // DLF
    EB_U32                  startIndex;
    EB_U8                   blk4x4IndexX;

    EB_U8                   blk4x4IndexY;
    EB_BOOL                 availableCoeff;

    // QP Neighbor Arrays
    EB_BOOL                 isDeltaQpNotCoded = EB_TRUE;

    // LCU Stats
    EB_U32                  lcuWidth = MIN(sequenceControlSetPtr->lcuSize, sequenceControlSetPtr->lumaWidth - lcuOriginX);
    EB_U32                  lcuHeight = MIN(sequenceControlSetPtr->lcuSize, sequenceControlSetPtr->lumaHeight - lcuOriginY);

    // SAO
    EB_S64                  saoLumaBestCost;
    EB_S64                  saoChromaBestCost;

    // MV merge mode
    EB_U32                  yCbf;
    EB_U32                  cbCbf;
    EB_U32                  crCbf;
    EB_U64                  yCoeffBits;
    EB_U64                  cbCoeffBits;
    EB_U64                  crCoeffBits;
    EB_U64                  yFullDistortion[DIST_CALC_TOTAL];
    EB_U64                  yTuFullDistortion[DIST_CALC_TOTAL];
    EB_U32                  countNonZeroCoeffs[3];
    EB_U64                  yTuCoeffBits;
    EB_U64                  cbTuCoeffBits;
    EB_U64                  crTuCoeffBits;
    EB_U32                  lumaShift;
    EB_U32                  scratchLumaOffset;
    EncodeContext_t        *encodeContextPtr;
    EB_U32                  lcuRowIndex = lcuOriginY / MAX_LCU_SIZE;

    // Dereferencing early
    NeighborArrayUnit_t    *epModeTypeNeighborArray = pictureControlSetPtr->epModeTypeNeighborArray;
    NeighborArrayUnit_t    *epIntraLumaModeNeighborArray = pictureControlSetPtr->epIntraLumaModeNeighborArray;
    NeighborArrayUnit_t    *epMvNeighborArray = pictureControlSetPtr->epMvNeighborArray;
    NeighborArrayUnit_t    *epLumaReconNeighborArray = is16bit ? pictureControlSetPtr->epLumaReconNeighborArray16bit : pictureControlSetPtr->epLumaReconNeighborArray;
    NeighborArrayUnit_t    *epCbReconNeighborArray = is16bit ? pictureControlSetPtr->epCbReconNeighborArray16bit : pictureControlSetPtr->epCbReconNeighborArray;
    NeighborArrayUnit_t    *epCrReconNeighborArray = is16bit ? pictureControlSetPtr->epCrReconNeighborArray16bit : pictureControlSetPtr->epCrReconNeighborArray;
    NeighborArrayUnit_t    *epSkipFlagNeighborArray = pictureControlSetPtr->epSkipFlagNeighborArray;
    NeighborArrayUnit_t    *epLeafDepthNeighborArray = pictureControlSetPtr->epLeafDepthNeighborArray;

    EB_BOOL                 constrainedIntraFlag = pictureControlSetPtr->constrainedIntraFlag;
    EB_BOOL                 enableStrongIntraSmoothing = sequenceControlSetPtr->enableStrongIntraSmoothing;
    CodingUnit_t          **codedLeafArrayPtr = lcuPtr->codedLeafArrayPtr;

    // This falg needs to be set true when SAO is enabled for Non reference pictures so that SAO uses filtered samples
    EB_BOOL dlfEnableFlag = (EB_BOOL)(!sequenceControlSetPtr->staticConfig.disableDlfFlag &&
        (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)) ||
        sequenceControlSetPtr->staticConfig.reconEnabled;

    dlfEnableFlag = contextPtr->allowEncDecMismatch ? EB_FALSE : dlfEnableFlag;

    const EB_BOOL isIntraLCU = contextPtr->mdContext->limitIntra ? isIntraPresent(lcuPtr) : EB_TRUE;

    EB_BOOL doRecon = (EB_BOOL)(contextPtr->mdContext->limitIntra == 0 || isIntraLCU == 1) ||
                pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag ||
                sequenceControlSetPtr->staticConfig.reconEnabled;


    CabacCost_t     *cabacCost = pictureControlSetPtr->cabacCost;
    EntropyCoder_t  *coeffEstEntropyCoderPtr = pictureControlSetPtr->coeffEstEntropyCoderPtr;
    EB_U8            cuItr;
    EB_U32           dZoffset = 0;

    if (!lcuStatPtr->stationaryEdgeOverTimeFlag && sequenceControlSetPtr->staticConfig.improveSharpness && pictureControlSetPtr->ParentPcsPtr->picNoiseClass < PIC_NOISE_CLASS_3_1) {
        EB_S16 cuDeltaQp = (EB_S16)(lcuPtr->qp - pictureControlSetPtr->ParentPcsPtr->averageQp);
        EB_U32 dzCondition = cuDeltaQp > 0 ? 0 : 1;

        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {

            if (!(pictureControlSetPtr->ParentPcsPtr->isPan ||
                (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage < 10 && lcuPtr->auraStatus == AURA_STATUS_1) ||
                (lcuStatPtr->cuStatArray[0].skinArea) ||
                (pictureControlSetPtr->ParentPcsPtr->intraCodedBlockProbability > 90) ||
                (pictureControlSetPtr->ParentPcsPtr->highDarkAreaDensityFlag))) {

                if (pictureControlSetPtr->sliceType != EB_I_SLICE &&
                    pictureControlSetPtr->temporalLayerIndex == 0 &&
                    pictureControlSetPtr->ParentPcsPtr->intraCodedBlockProbability > 60 &&
                    !pictureControlSetPtr->ParentPcsPtr->isTilt &&
                    pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage > 40)
                {
                    dZoffset = 10;
                }

                if (dzCondition) {
                    if (pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_1) {
                        if (pictureControlSetPtr->sliceType == EB_I_SLICE) {
                            dZoffset = lcuStatPtr->cuStatArray[0].grassArea ? 10 : dZoffset;
                        }
                        else if (pictureControlSetPtr->temporalLayerIndex == 0) {
                            dZoffset = lcuStatPtr->cuStatArray[0].grassArea ? 9 : dZoffset;
                        }
                        else if (pictureControlSetPtr->temporalLayerIndex == 1) {
                            dZoffset = lcuStatPtr->cuStatArray[0].grassArea ? 5 : dZoffset;
                        }
                    }

                }
            }
        }
    }

    QpmDeriveBeaAndSkipQpmFlagLcu(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        lcuPtr,
        tbAddr,
        contextPtr);

    encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE) {

        // TMVP init
        tmvpMapWritePtr = &(contextPtr->referenceObjectWritePtr->tmvpMap[tbAddr]);
        tmvpMapIndex = 0;

        //get the 16bit form of the input LCU 
        if (is16bit) {

            reconBuffer = ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture16bit;
        }

        else {
            reconBuffer = ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture;
        }
    }
    else { // non ref pictures
        reconBuffer = is16bit ? pictureControlSetPtr->reconPicture16bitPtr : pictureControlSetPtr->reconPicturePtr;
        tmvpMapWritePtr = (TmvpUnit_t*)EB_NULL;
    }


    EB_BOOL useDeltaQp = (EB_BOOL)(sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction);

    EB_BOOL singleSegment = (sequenceControlSetPtr->encDecSegmentColCountArray[pictureControlSetPtr->temporalLayerIndex] == 1) && (sequenceControlSetPtr->encDecSegmentRowCountArray[pictureControlSetPtr->temporalLayerIndex] == 1);

    EB_BOOL useDeltaQpSegments = singleSegment ? 0 : (EB_BOOL)(sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction);

    if (is16bit) {
        EncodePassPackLcu(
            sequenceControlSetPtr,
            inputPicture,
            contextPtr,
            lcuOriginX,
            lcuOriginY,
            lcuWidth,
            lcuHeight);
    }

    contextPtr->intraCodedAreaLCU[tbAddr] = 0;

    // CU Loop 
    cuItr = 0;
    while (cuItr < CU_MAX_COUNT) {

        if (codedLeafArrayPtr[cuItr]->splitFlag == EB_FALSE){
            
            // PU Stack variables
            PredictionUnit_t        *puPtr                  = (PredictionUnit_t *)EB_NULL; //  done
            EbPictureBufferDesc_t   *residualBuffer         = contextPtr->residualBuffer;
            EbPictureBufferDesc_t   *transformBuffer        = contextPtr->transformBuffer;
            EB_S16                  *transformInnerArrayPtr = contextPtr->transformInnerArrayPtr;
            const CodedUnitStats_t  *cuStats                = contextPtr->cuStats   = GetCodedUnitStats(cuItr);
            CodingUnit_t            *cuPtr                  = contextPtr->cuPtr     = lcuPtr->codedLeafArrayPtr[cuItr]; 

            _mm_prefetch((const char *)cuStats, _MM_HINT_T0);

            contextPtr->cuOriginX = (EB_U16)(lcuOriginX + cuStats->originX);
            contextPtr->cuOriginY = (EB_U16)(lcuOriginY + cuStats->originY);

            EncodePassPreFetchRef(
                pictureControlSetPtr,
                contextPtr,
                cuPtr,
                cuStats,
                puPtr,
                is16bit);

            cuPtr->deltaQp = 0;

			cuPtr->qp = (sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction) ? contextPtr->qpmQp : pictureControlSetPtr->pictureQp;
			lcuPtr->qp = (sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction) ? contextPtr->qpmQp : pictureControlSetPtr->pictureQp;
			cuPtr->orgDeltaQp = cuPtr->deltaQp;

			if (!contextPtr->skipQpmFlag && (sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction) && (contextPtr->cuStats->depth <= pictureControlSetPtr->difCuDeltaQpDepth)){

                EncQpmDeriveDeltaQPForEachLeafLcu(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    lcuPtr,
                    tbAddr,
                    cuPtr,
                    contextPtr->cuStats->depth,
                    cuItr,
                    cuStats->size,
                    cuPtr->predictionModeFlag,
                    contextPtr->cuStats->parent32x32Index,
                    contextPtr);
            }

            EB_U8    fastEl = (contextPtr->fastEl && contextPtr->cuStats->size > 8);
            EB_U64   yCoeffBitsTemp = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yCoeffBits;
            EB_S16   yDc = 0; 
            EB_U16   yCountNonZeroCoeffs = 0;
			EB_U32   yBitsThsld = (contextPtr->cuStats->size > 32) ? contextPtr->yBitsThsld : (contextPtr->cuStats->size > 16) ? (contextPtr->yBitsThsld >> 1) : (contextPtr->yBitsThsld >> 2);

            if (cuPtr->predictionModeFlag == INTRA_MODE && cuPtr->predictionUnitArray->intraLumaMode != EB_INTRA_MODE_4x4){

                contextPtr->totIntraCodedArea += cuStats->size*cuStats->size;
                if (pictureControlSetPtr->sliceType != EB_I_SLICE){
                    contextPtr->intraCodedAreaLCU[tbAddr] += cuStats->size*cuStats->size;
                }

                // *Note - Transforms are the same size as predictions 
                // Partition Loop
                contextPtr->tuItr = 0;

                {
                    // Set the PU Loop Variables
                    puPtr = cuPtr->predictionUnitArray;
                    // Generate Intra Luma Neighbor Modes
                    GeneratePuIntraLumaNeighborModes( // HT done 
                        cuPtr,
                        contextPtr->cuOriginX,
                        contextPtr->cuOriginY,
                        MAX_LCU_SIZE,
                        epIntraLumaModeNeighborArray,
                        epModeTypeNeighborArray);

                    // Transform Loop (not supported)
                    {

                        // Generate Intra Reference Samples  

                        GenerateIntraReferenceSamplesFuncTable[is16bit](
                            constrainedIntraFlag,
                            enableStrongIntraSmoothing,
                            contextPtr->cuOriginX,
                            contextPtr->cuOriginY,
                            cuStats->size,
                            cuStats->depth,
                            epModeTypeNeighborArray,
                            epLumaReconNeighborArray,
                            epCbReconNeighborArray,
                            epCrReconNeighborArray,
                            is16bit ? (void*)contextPtr->intraRefPtr16 : (void*)contextPtr->intraRefPtr,
                            (contextPtr->cuOriginX == 0),
                            (contextPtr->cuOriginY == 0),
                            (contextPtr->cuOriginX + cuStats->size) == sequenceControlSetPtr->lumaWidth ? EB_TRUE : EB_FALSE);

                        // Prediction  
                        EncodePassIntraPredictionFuncTable[is16bit](
                            is16bit ? (void*)contextPtr->intraRefPtr16 : (void*)contextPtr->intraRefPtr,
                            contextPtr->cuOriginX + reconBuffer->originX,
                            contextPtr->cuOriginY + reconBuffer->originY,
                            cuStats->size,
                            reconBuffer,
                            (EB_U32)puPtr->intraLumaMode);



                        // Encode Transform Unit -INTRA-
                        {

                            EB_U8	         qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(cuPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
                            EB_U8	         cbQp = MapChromaQp(qpScaled);

                            contextPtr->forceCbfFlag = (contextPtr->skipQpmFlag) ?
                                EB_FALSE :
                                lcuPtr->pictureLeftEdgeFlag && ((contextPtr->cuOriginX & (63)) == 0) && (contextPtr->cuOriginY == lcuOriginY);

                            SetPmEncDecMode(
                                pictureControlSetPtr,
                                contextPtr,
                                tbAddr,

                                lcuStatPtr->stationaryEdgeOverTimeFlag,
                                pictureControlSetPtr->temporalLayerIndex > 0 ? lcuStatPtr->pmStationaryEdgeOverTimeFlag : lcuStatPtr->stationaryEdgeOverTimeFlag);

    
                            // Set Fast El coef shaping method 
                            contextPtr->transCoeffShapeLuma   = DEFAULT_SHAPE;
                            contextPtr->transCoeffShapeChroma = DEFAULT_SHAPE;         
							if (fastEl && contextPtr->pmpMaskingLevelEncDec > MASK_THSHLD_1) {
                                yDc = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yDc[0];
                                yCountNonZeroCoeffs = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[0];

								if ((cuPtr->rootCbf == 0) || ((yCoeffBitsTemp <= yBitsThsld) && yDc < YDC_THSHLD_1 && yCountNonZeroCoeffs <= 1)) {
                                    // Skip pass for cuPtr->rootCbf == 0 caused some VQ issues in chroma, so DC path is used instead
                                    contextPtr->transCoeffShapeLuma     = ONLY_DC_SHAPE;
                                    contextPtr->transCoeffShapeChroma   = ONLY_DC_SHAPE;
                                }
                                else if ((yCoeffBitsTemp <= yBitsThsld * 4)) {
                                    contextPtr->transCoeffShapeLuma = N4_SHAPE;
                                    if ((cuStats->size >> 1) > 8)
                                        contextPtr->transCoeffShapeChroma = N4_SHAPE;
                                    else
                                        contextPtr->transCoeffShapeChroma = N2_SHAPE;
                                }
                                else if ((yCoeffBitsTemp <= yBitsThsld * 16)) {
                                    contextPtr->transCoeffShapeLuma     = N2_SHAPE;
                                    contextPtr->transCoeffShapeChroma   = N2_SHAPE;
                                }
                           }

                            EncodeLoopFunctionTable[is16bit](
                                contextPtr,
                                lcuPtr,
                                contextPtr->cuOriginX,
                                contextPtr->cuOriginY,
                                cbQp,
                                reconBuffer,
                                coeffBufferTB,
                                residualBuffer,
                                transformBuffer,
                                transformInnerArrayPtr,
                                countNonZeroCoeffs,
                                useDeltaQpSegments,
								(CabacEncodeContext_t*)coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
								(EB_U32)puPtr->intraLumaMode,
								pictureControlSetPtr->cabacCost,
                                cuPtr->deltaQp > 0 ? 0 : dZoffset);

                            EncodeGenerateReconFunctionPtr[is16bit](
                                contextPtr,
                                contextPtr->cuOriginX,
                                contextPtr->cuOriginY,
                                reconBuffer,
                                residualBuffer,
                                transformInnerArrayPtr);

                        }

                        // Update the Intra-specific Neighbor Arrays
                        EncodePassUpdateIntraModeNeighborArrays(
                            epModeTypeNeighborArray,
                            epIntraLumaModeNeighborArray,
                            (EB_U8)cuPtr->predictionUnitArray->intraLumaMode,
                            contextPtr->cuOriginX,
                            contextPtr->cuOriginY,
                            cuStats->size);

                        // Update Recon Samples-INTRA-                 
                        EncodePassUpdateReconSampleNeighborArrays(
                            epLumaReconNeighborArray,
                            epCbReconNeighborArray,
                            epCrReconNeighborArray,
                            reconBuffer,
                            contextPtr->cuOriginX,
                            contextPtr->cuOriginY,
                            cuStats->size,
                            is16bit);

                    } // Transform Loop

                    // set up the bS based on PU boundary for DLF
                    if (dlfEnableFlag){
                        // Update the cbf map for DLF
                        startIndex = (contextPtr->cuOriginY >> 2) * (sequenceControlSetPtr->lumaWidth >> 2) + (contextPtr->cuOriginX >> 2);
                        for (blk4x4IndexY = 0; blk4x4IndexY < (cuStats->size >> 2); ++blk4x4IndexY){
                            EB_MEMSET(&pictureControlSetPtr->cbfMapArray[startIndex], (EB_U8)(cuPtr->transformUnitArray[contextPtr->tuItr].lumaCbf), (cuStats->size >> 2));
                            startIndex += (sequenceControlSetPtr->lumaWidth >> 2);
                        }

                        SetBSArrayBasedOnPUBoundary(
                            epModeTypeNeighborArray,
                            epMvNeighborArray,
                            puPtr,
                            cuPtr,
                            cuStats,
                            lcuOriginX,
                            lcuOriginY,
                            pictureControlSetPtr,
                            pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
                            pictureControlSetPtr->verticalEdgeBSArray[tbAddr]);

                    }

                } // Partition Loop

            }

            else if (cuPtr->predictionModeFlag == INTRA_MODE) { //cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4


                //*************************
                //       INTRA  4x4
                //*************************


                contextPtr->totIntraCodedArea += cuStats->size*cuStats->size;
                if (pictureControlSetPtr->sliceType != EB_I_SLICE) {
                    contextPtr->intraCodedAreaLCU[tbAddr] += cuStats->size*cuStats->size;
                }

                // Partition Loop
                EB_U8 partitionIndex;


				for (partitionIndex = 0; partitionIndex < 4; partitionIndex++) {
					// Partition Loop
					contextPtr->tuItr = partitionIndex + 1;

                    EB_U16 partitionOriginX = contextPtr->cuOriginX + INTRA_4x4_OFFSET_X[partitionIndex];
                    EB_U16 partitionOriginY = contextPtr->cuOriginY + INTRA_4x4_OFFSET_Y[partitionIndex];

                    EB_BOOL pictureLeftBoundary = (lcuPtr->pictureLeftEdgeFlag == EB_TRUE && ((partitionOriginX & (MAX_LCU_SIZE - 1)) == 0)) ? EB_TRUE : EB_FALSE;
                    EB_BOOL pictureTopBoundary = (lcuPtr->pictureTopEdgeFlag == EB_TRUE && ((partitionOriginY & (MAX_LCU_SIZE - 1)) == 0)) ? EB_TRUE : EB_FALSE;
                    EB_BOOL pictureRightBoundary = (lcuPtr->pictureRightEdgeFlag == EB_TRUE && (((partitionOriginX + MIN_PU_SIZE) & (MAX_LCU_SIZE - 1)) == 0)) ? EB_TRUE : EB_FALSE;

                    EB_U8   intraLumaMode = lcuPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuItr] - 21) << 2) + partitionIndex];

                    // Set the PU Loop Variables
                    puPtr = cuPtr->predictionUnitArray;

                    // Generate Intra Luma Neighbor Modes
                    GeneratePuIntraLumaNeighborModes( // HT done 
                        cuPtr,
                        partitionOriginX,
                        partitionOriginY,
                        MAX_LCU_SIZE,
                        epIntraLumaModeNeighborArray,
                        epModeTypeNeighborArray);

                    // Generate Intra Reference Samples  
                    if (partitionIndex) {

                        if (is16bit){
                            GenerateLumaIntraReference16bitSamplesEncodePass(
                                constrainedIntraFlag,
                                enableStrongIntraSmoothing,
                                partitionOriginX,
                                partitionOriginY,
                                MIN_PU_SIZE,
                                MAX_LCU_SIZE,
                                cuStats->depth,
                                epModeTypeNeighborArray,
                                epLumaReconNeighborArray,
                                epCbReconNeighborArray,
                                epCrReconNeighborArray,
                                contextPtr->intraRefPtr16,
								pictureLeftBoundary,
								pictureTopBoundary,
								pictureRightBoundary);
                        }
                        else {

                            GenerateLumaIntraReferenceSamplesEncodePass(
                                constrainedIntraFlag,
                                enableStrongIntraSmoothing,
                                partitionOriginX,
                                partitionOriginY,
                                MIN_PU_SIZE,
                                MAX_LCU_SIZE,
                                cuStats->depth,
                                epModeTypeNeighborArray,
                                epLumaReconNeighborArray,
                                epCbReconNeighborArray,
                                epCrReconNeighborArray,
                                contextPtr->intraRefPtr,
								pictureLeftBoundary,
								pictureTopBoundary,
								pictureRightBoundary);
                        }

                    }
                    else {
                        if (is16bit){
                            GenerateLumaIntraReference16bitSamplesEncodePass(
                                constrainedIntraFlag,
                                enableStrongIntraSmoothing,
                                partitionOriginX,
                                partitionOriginY,
                                MIN_PU_SIZE,
                                MAX_LCU_SIZE,
                                cuStats->depth,
                                epModeTypeNeighborArray,
                                epLumaReconNeighborArray,
                                epCbReconNeighborArray,
                                epCrReconNeighborArray,
                                contextPtr->intraRefPtr16,
								pictureLeftBoundary,
								pictureTopBoundary,
								pictureRightBoundary);

                            // For the Intra4x4 case, the Chroma for the CU is coded as a single 4x4 block.
                            //   This changes how the right picture boundary is interpreted for the Luma and Chroma blocks
                            //   as there is not a one-to-one relationship between the luma/chroma blocks. This effects
                            //   only the right picture edge check and not the left or top boundary checks as the block size
                            //   has no influence on those checks.  
							pictureRightBoundary = (lcuPtr->pictureRightEdgeFlag == EB_TRUE && ((((partitionOriginX / 2) + MIN_PU_SIZE) & ((MAX_LCU_SIZE / 2) - 1)) == 0)) ? EB_TRUE : EB_FALSE;
                            GenerateChromaIntraReference16bitSamplesEncodePass(
                                constrainedIntraFlag,
                                enableStrongIntraSmoothing,
                                partitionOriginX,
                                partitionOriginY,
                                MIN_PU_SIZE << 1,
                                MAX_LCU_SIZE,
                                cuStats->depth,
                                epModeTypeNeighborArray,
                                epLumaReconNeighborArray,
                                epCbReconNeighborArray,
                                epCrReconNeighborArray,
                                contextPtr->intraRefPtr16,
								pictureLeftBoundary,
								pictureTopBoundary,
								pictureRightBoundary);
                        }
                        else {

                            GenerateLumaIntraReferenceSamplesEncodePass(
                                constrainedIntraFlag,
                                enableStrongIntraSmoothing,
                                partitionOriginX,
                                partitionOriginY,
                                MIN_PU_SIZE,
                                MAX_LCU_SIZE,
                                cuStats->depth,
                                epModeTypeNeighborArray,
                                epLumaReconNeighborArray,
                                epCbReconNeighborArray,
                                epCrReconNeighborArray,
                                contextPtr->intraRefPtr,
								pictureLeftBoundary,
								pictureTopBoundary,
								pictureRightBoundary);

                            // For the Intra4x4 case, the Chroma for the CU is coded as a single 4x4 block.
                            //   This changes how the right picture boundary is interpreted for the Luma and Chroma blocks
                            //   as there is not a one-to-one relationship between the luma/chroma blocks. This effects
                            //   only the right picture edge check and not the left or top boundary checks as the block size
                            //   has no influence on those checks.  
							pictureRightBoundary = (lcuPtr->pictureRightEdgeFlag == EB_TRUE && ((((partitionOriginX / 2) + MIN_PU_SIZE) & ((MAX_LCU_SIZE / 2) - 1)) == 0)) ? EB_TRUE : EB_FALSE;
                            GenerateChromaIntraReferenceSamplesEncodePass(
                                constrainedIntraFlag,
                                enableStrongIntraSmoothing,
                                partitionOriginX,
                                partitionOriginY,
                                MIN_PU_SIZE << 1,
                                MAX_LCU_SIZE,
                                cuStats->depth,
                                epModeTypeNeighborArray,
                                epLumaReconNeighborArray,
                                epCbReconNeighborArray,
                                epCrReconNeighborArray,
                                contextPtr->intraRefPtr,
								pictureLeftBoundary,
								pictureTopBoundary,
								pictureRightBoundary);
                        }
                    }

					// Prediction                  
					if (is16bit){
						EncodePassIntra4x4Prediction16bit(
							contextPtr->intraRefPtr16,
							partitionOriginX + reconBuffer->originX,
							partitionOriginY + reconBuffer->originY,
							MIN_PU_SIZE,
							MIN_PU_SIZE,
                            reconBuffer,
                            intraLumaMode,
                            EB_INTRA_CHROMA_DM,
							partitionIndex ? PICTURE_BUFFER_DESC_LUMA_MASK : PICTURE_BUFFER_DESC_FULL_MASK);
					}
					else{
						EncodePassIntra4x4Prediction(
							contextPtr->intraRefPtr,
							partitionOriginX + reconBuffer->originX,
							partitionOriginY + reconBuffer->originY,
							MIN_PU_SIZE,
							MIN_PU_SIZE,
                            reconBuffer,
                            intraLumaMode,
							EB_INTRA_CHROMA_DM,
							partitionIndex ? PICTURE_BUFFER_DESC_LUMA_MASK : PICTURE_BUFFER_DESC_FULL_MASK);
					}
                    
                    // Encode Transform Unit -INTRA-
                    EB_U8	         qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(cuPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
                    EB_U8	         cbQp = MapChromaQp(qpScaled);

                    contextPtr->forceCbfFlag = (contextPtr->skipQpmFlag) ?
                        EB_FALSE :
                        lcuPtr->pictureLeftEdgeFlag && ((contextPtr->cuOriginX & (63)) == 0) && (contextPtr->cuOriginY == lcuOriginY);

                    SetPmEncDecMode(
                        pictureControlSetPtr,
                        contextPtr,
                        tbAddr,

                    lcuStatPtr->stationaryEdgeOverTimeFlag,
                        pictureControlSetPtr->temporalLayerIndex > 0 ? lcuStatPtr->pmStationaryEdgeOverTimeFlag : lcuStatPtr->stationaryEdgeOverTimeFlag);

                    contextPtr->transCoeffShapeLuma   = DEFAULT_SHAPE;
                    contextPtr->transCoeffShapeChroma = DEFAULT_SHAPE;

                    EncodeLoopIntra4x4FunctionTable[is16bit](
                        contextPtr,
                        lcuPtr,
                        partitionOriginX,
                        partitionOriginY,
                        cbQp,
                        reconBuffer,
                        coeffBufferTB,
                        residualBuffer,
                        transformBuffer,
                        transformInnerArrayPtr,
                        countNonZeroCoeffs,
                        partitionIndex ? PICTURE_BUFFER_DESC_LUMA_MASK : PICTURE_BUFFER_DESC_FULL_MASK,
                        useDeltaQpSegments,
						(CabacEncodeContext_t*)coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
						intraLumaMode,
						pictureControlSetPtr->cabacCost,
                        cuPtr->deltaQp > 0 ? 0 : dZoffset);

                    EncodeGenerateReconIntra4x4FunctionPtr[is16bit](
                        contextPtr,
                        partitionOriginX,
                        partitionOriginY,
                        reconBuffer,
                        residualBuffer,
                        transformInnerArrayPtr,
                        partitionIndex ? PICTURE_BUFFER_DESC_LUMA_MASK : PICTURE_BUFFER_DESC_FULL_MASK);

                    // Update the Intra-specific Neighbor Arrays
                    EncodePassUpdateIntraModeNeighborArrays(
                        epModeTypeNeighborArray,
                        epIntraLumaModeNeighborArray,
                        intraLumaMode,
                        partitionOriginX,
                        partitionOriginY,
                        MIN_PU_SIZE);

                    // Update Recon Samples-INTRA-                 
                    EncodePassUpdateReconSampleNeighborArrays(
                        epLumaReconNeighborArray,
                        epCbReconNeighborArray,
                        epCrReconNeighborArray,
                        reconBuffer,
                        partitionOriginX,
                        partitionOriginY,
                        MIN_PU_SIZE,
                        is16bit);

                    // set up the bS based on PU boundary for DLF
                    if (dlfEnableFlag){

                        // Update the cbf map for DLF
                        startIndex = (partitionOriginY >> 2) * (sequenceControlSetPtr->lumaWidth >> 2) + (partitionOriginX >> 2);
                        for (blk4x4IndexY = 0; blk4x4IndexY < (MIN_PU_SIZE >> 2); ++blk4x4IndexY){
                            for (blk4x4IndexX = 0; blk4x4IndexX < (MIN_PU_SIZE >> 2); ++blk4x4IndexX){
                                pictureControlSetPtr->cbfMapArray[startIndex + blk4x4IndexX] = (EB_U8)cuPtr->transformUnitArray[contextPtr->tuItr].lumaCbf;
                            }
                            startIndex += (sequenceControlSetPtr->lumaWidth >> 2);
                        }

                        // Set the bS on TU boundary for DLF
                        Intra4x4SetBSArrayBasedOnTUBoundary(
                            partitionOriginX,
                            partitionOriginY,
                            MIN_PU_SIZE,
                            MIN_PU_SIZE,
                            partitionOriginY == contextPtr->cuOriginY ? EB_TRUE : EB_FALSE,
                            partitionOriginX == contextPtr->cuOriginX ? EB_TRUE : EB_FALSE,
                            contextPtr->cuStats,
                            (EB_PART_MODE)contextPtr->cuPtr->predictionModeFlag,
                            lcuOriginX,
                            lcuOriginY,
                            pictureControlSetPtr,
                            pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
                            pictureControlSetPtr->verticalEdgeBSArray[tbAddr]);


                        Intra4x4SetBSArrayBasedOnPUBoundary(
                            epModeTypeNeighborArray,
                            epMvNeighborArray,
                            puPtr,
                            cuPtr,
                            cuStats,
                            partitionOriginX & (MAX_LCU_SIZE - 1),
                            partitionOriginY & (MAX_LCU_SIZE - 1),
                            MIN_PU_SIZE,
                            MIN_PU_SIZE,
                            lcuOriginX,
                            lcuOriginY,
                            EB_FALSE,
                            EB_FALSE,
                            pictureControlSetPtr,
                            pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
                            pictureControlSetPtr->verticalEdgeBSArray[tbAddr]);

                    }

                } // Partition Loop
            }

            // Inter
            else if (cuPtr->predictionModeFlag == INTER_MODE) {
                EB_U16  tuOriginX;
                EB_U16  tuOriginY;
                EB_U8   tuSize = 0;
                EB_U8   tuSizeChroma;

                EB_BOOL isCuSkip = EB_FALSE;

                //********************************
                //        INTER
                //********************************
                EB_BOOL doMVpred = EB_TRUE;
                //if QPM and Segments are used, First Cu in LCU row should have at least one coeff. 
                 EB_BOOL isFirstCUinRow = (useDeltaQp == 1) &&
                    !singleSegment &&
                    (contextPtr->cuOriginX == 0 && contextPtr->cuOriginY == lcuOriginY) ? EB_TRUE : EB_FALSE;

                //Motion Compensation could be avoided in the case below
                EB_BOOL doMC = EB_TRUE;

                // Perform Merge/Skip Decision if the mode coming from MD is merge. for the First CU in Row merge will remain as is.

				if (cuPtr->predictionUnitArray[0].mergeFlag == EB_TRUE)
				{
					if (isFirstCUinRow == EB_FALSE)
					{
						if (lcuPtr->chromaEncodeMode == CHROMA_MODE_BEST)
						{

							EbPictureBufferDesc_t                  *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;
							const EB_U32 inputCbOriginIndex = ((contextPtr->cuOriginY >> 1) + (inputPicturePtr->originY >> 1)) * inputPicturePtr->strideCb + ((contextPtr->cuOriginX >> 1) + (inputPicturePtr->originX >> 1));
							const EB_U32 cuChromaOriginIndex = (((contextPtr->cuOriginY & 63) * 32) + (contextPtr->cuOriginX & 63)) >> 1;

							contextPtr->mdContext->cuOriginX = contextPtr->cuOriginX;
							contextPtr->mdContext->cuOriginY = contextPtr->cuOriginY;
							contextPtr->mdContext->puItr = 0;
							contextPtr->mdContext->cuSize = contextPtr->cuStats->size;
							contextPtr->mdContext->cuSizeLog2 = contextPtr->cuStats->sizeLog2;
							contextPtr->mdContext->cuStats = contextPtr->cuStats;

							AddChromaEncDec(
								pictureControlSetPtr,
								lcuPtr,
								cuPtr,
								contextPtr->mdContext,
								contextPtr,
								inputPicturePtr,
								inputCbOriginIndex,
								cuChromaOriginIndex,
								0);
						}

						if (
							pictureControlSetPtr->sliceType == EB_B_SLICE                             &&
							pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE
							)
						{
							EbReferenceObject_t  * refObjL0, *refObjL1;
							EB_U16 cuVar = (pictureControlSetPtr->ParentPcsPtr->variance[lcuPtr->index][0]);
							EB_U8 INTRA_AREA_TH[MAX_TEMPORAL_LAYERS] = { 40, 30, 30, 0, 0, 0 };
							refObjL0 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
							refObjL1 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;

							if (cuVar < 200 && (refObjL0->intraCodedArea > INTRA_AREA_TH[refObjL0->tmpLayerIdx] ||
								refObjL1->intraCodedArea > INTRA_AREA_TH[refObjL1->tmpLayerIdx]
								)
								)
								mdcontextPtr->mdEpPipeLcu[cuPtr->leafIndex].skipCost += (mdcontextPtr->mdEpPipeLcu[cuPtr->leafIndex].skipCost * 70) / 100;
						}
						isCuSkip = mdcontextPtr->mdEpPipeLcu[cuPtr->leafIndex].skipCost <= mdcontextPtr->mdEpPipeLcu[cuPtr->leafIndex].mergeCost ? 1 : 0;
					}
				}

                //MC could be avoided in some cases below
                if (isFirstCUinRow == EB_FALSE){

                    if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE && constrainedIntraFlag == EB_TRUE &&
                        cuPtr->predictionUnitArray[0].mergeFlag == EB_TRUE)
                    {
                        if (isCuSkip)
                        {
                            //here merge is decided to be skip in nonRef frame.
                            doMC = EB_FALSE;
                            doMVpred = EB_FALSE;
                        }
                    }
                    else if (contextPtr->mdContext->limitIntra && isIntraLCU == EB_FALSE)
                    {
                        if (isCuSkip)
                        {
                            doMC = EB_FALSE;
                            doMVpred = EB_FALSE;
                        }
                    }


                }


                doMC = (EB_BOOL)(doRecon | doMC);

                doMVpred = (EB_BOOL)(doRecon | doMVpred);

            {
                // 1st Partition Loop


                puPtr = cuPtr->predictionUnitArray;
                if (doMVpred)
                    EncodePassMvPrediction(
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        tbAddr,
                        contextPtr);

                // Set MvUnit
                contextPtr->mvUnit.predDirection = (EB_U8)puPtr->interPredDirectionIndex;
                contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
                contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;
                // Inter Prediction
                if (is16bit){

                    if (doMC)
                        EncodePassInterPrediction16bit(
                            &contextPtr->mvUnit,
                            contextPtr->cuOriginX,
                            contextPtr->cuOriginY,
                            cuStats->size,
                            cuStats->size,
                            pictureControlSetPtr,
                            reconBuffer,
                            contextPtr->mcpContext);
                }
                else{

                    if (doMC)
                        EncodePassInterPrediction(
                            &contextPtr->mvUnit,
                            contextPtr->cuOriginX,
                            contextPtr->cuOriginY,
                            cuStats->size,
                            cuStats->size,
                            pictureControlSetPtr,
                            reconBuffer,
                            contextPtr->mcpContext);
                }

            }

            contextPtr->tuItr = (cuStats->size < MAX_LCU_SIZE) ? 0 : 1;

            // Transform Loop
            cuPtr->transformUnitArray[0].lumaCbf = EB_FALSE;
            cuPtr->transformUnitArray[0].cbCbf = EB_FALSE;
            cuPtr->transformUnitArray[0].crCbf = EB_FALSE;

            // initialize TU Split
            yFullDistortion[DIST_CALC_RESIDUAL] = 0;
            yFullDistortion[DIST_CALC_PREDICTION] = 0;

            yCoeffBits = 0;
            cbCoeffBits = 0;
            crCoeffBits = 0;

            //SVT_LOG("sizeof %i \n",sizeof(CodingUnit_t));
            EB_U32  totTu = (cuStats->size < MAX_LCU_SIZE) ? 1 : 4;
            EB_U8   tuIt;

            EB_U8	qpScaled        = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(cuPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
            EB_U8	cbQp            = MapChromaQp(qpScaled);

            EB_U32  componentMask   = PICTURE_BUFFER_DESC_FULL_MASK;
            EB_MODETYPE predictionModeFlag = (EB_MODETYPE)cuPtr->predictionModeFlag;

            if (cuPtr->predictionUnitArray[0].mergeFlag == EB_FALSE){
            
                for (tuIt = 0; tuIt < totTu; tuIt++)
				{
					contextPtr->tuItr = (cuStats->size < MAX_LCU_SIZE) ? 0 : tuIt + 1;

                    if (cuStats->size < MAX_LCU_SIZE)
                    {
                        tuOriginX = contextPtr->cuOriginX;
                        tuOriginY = contextPtr->cuOriginY;
                        tuSize = cuStats->size;
                        tuSizeChroma = (cuStats->size >> 1);
                    }
                    else
                    {
                        tuOriginX = contextPtr->cuOriginX + ((tuIt & 1) << 5);
                        tuOriginY = contextPtr->cuOriginY + ((tuIt > 1) << 5);
                        tuSize = 32;
                        tuSizeChroma = 16;
                    }

                    //TU LOOP for MV mode + Luma CBF decision. 
                    contextPtr->forceCbfFlag = (contextPtr->skipQpmFlag) ?
                    EB_FALSE :
                             (tuOriginX == 0) && (tuOriginY == lcuOriginY);

                    SetPmEncDecMode(
                        pictureControlSetPtr,
                        contextPtr,
                        tbAddr,

                        lcuStatPtr->stationaryEdgeOverTimeFlag,
                        pictureControlSetPtr->temporalLayerIndex > 0 ? lcuStatPtr->pmStationaryEdgeOverTimeFlag : lcuStatPtr->stationaryEdgeOverTimeFlag);


                    // Set Fast El coef shaping method 
                    contextPtr->transCoeffShapeLuma     = DEFAULT_SHAPE;
                    contextPtr->transCoeffShapeChroma   = DEFAULT_SHAPE;
					if (fastEl && isFirstCUinRow == EB_FALSE && contextPtr->pmpMaskingLevelEncDec > MASK_THSHLD_1) {
                        yDc = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yDc[tuIt];
                        yCountNonZeroCoeffs = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[tuIt];


						if ((cuPtr->rootCbf == 0) || ((yCoeffBitsTemp <= yBitsThsld) && yDc < YDC_THSHLD_1 && yCountNonZeroCoeffs <= 1)) {
                            // Skip pass for cuPtr->rootCbf == 0 caused some VQ issues in chroma, so DC path is used instead
                            contextPtr->transCoeffShapeLuma = ONLY_DC_SHAPE;
                            contextPtr->transCoeffShapeChroma = ONLY_DC_SHAPE;
                        }
                        else if ((yCoeffBitsTemp <= yBitsThsld * 4)) {
                            contextPtr->transCoeffShapeLuma = N4_SHAPE;
                            if ((cuStats->size >> 1) > 8)
                                contextPtr->transCoeffShapeChroma = N4_SHAPE;
                            else
                                contextPtr->transCoeffShapeChroma = N2_SHAPE;
                        }
                        else if ((yCoeffBitsTemp <= yBitsThsld * 16)) {
                            contextPtr->transCoeffShapeLuma = N2_SHAPE;
                            contextPtr->transCoeffShapeChroma = N2_SHAPE;
                        }
                    }

                    EncodeLoopFunctionTable[is16bit](
                        contextPtr,
                        lcuPtr,
                        tuOriginX,
                        tuOriginY,
                        cbQp,
                        reconBuffer,
                        coeffBufferTB,
                        residualBuffer,
                        transformBuffer,
                        transformInnerArrayPtr,
                        countNonZeroCoeffs,
                        useDeltaQpSegments,
						(CabacEncodeContext_t*)coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
						0,
						pictureControlSetPtr->cabacCost,
                        cuPtr->deltaQp > 0 ? 0 : dZoffset);
              
                    // SKIP the CBF zero mode for DC path. There are problems with cost calculations
					if (contextPtr->transCoeffShapeLuma != ONLY_DC_SHAPE) {

						scratchLumaOffset = ((tuOriginY & (63)) * 64) + (tuOriginX & (63));
						
						// Compute Tu distortion
						PictureFullDistortionLuma(
							transformBuffer,
							scratchLumaOffset,
							residualBuffer,
							scratchLumaOffset,
							contextPtr->transCoeffShapeLuma == ONLY_DC_SHAPE || cuPtr->transformUnitArray[contextPtr->tuItr].isOnlyDc[0] == EB_TRUE ? 1 : (tuSize >> contextPtr->transCoeffShapeLuma),
							yTuFullDistortion,
							countNonZeroCoeffs[0],
							predictionModeFlag);


						lumaShift = 2 * (7 - Log2f(tuSize));

						// Note: for square Transform, the scale is 1/(2^(7-Log2(Transform size)))
						// For NSQT the scale would be 1/ (2^(7-(Log2(first Transform size)+Log2(second Transform size))/2))
						// Add Log2 of Transform size in order to calculating it multiple time in this function

						yTuFullDistortion[DIST_CALC_RESIDUAL] = (yTuFullDistortion[DIST_CALC_RESIDUAL] + (EB_U64)(1 << (lumaShift - 1))) >> lumaShift;
						yTuFullDistortion[DIST_CALC_PREDICTION] = (yTuFullDistortion[DIST_CALC_PREDICTION] + (EB_U64)(1 << (lumaShift - 1))) >> lumaShift;

						yTuCoeffBits = 0;
						cbTuCoeffBits = 0;
						crTuCoeffBits = 0;

						// Estimate Tu Coeff  bits		
                        TuEstimateCoeffBitsEncDec(
                            (tuOriginY  & (63)) * MAX_LCU_SIZE + (tuOriginX & (63)),
                            ((tuOriginY & (63)) * MAX_LCU_SIZE_CHROMA + (tuOriginX & (63))) >> 1,
                            coeffEstEntropyCoderPtr,
                            coeffBufferTB,
                            countNonZeroCoeffs,
                            &yTuCoeffBits,
                            &cbTuCoeffBits,
                            &crTuCoeffBits,
                            contextPtr->transCoeffShapeLuma == ONLY_DC_SHAPE ? 1 : (tuSize >> contextPtr->transCoeffShapeLuma),
                            contextPtr->transCoeffShapeChroma == ONLY_DC_SHAPE ? 1 : (tuSizeChroma >> contextPtr->transCoeffShapeChroma),
                            predictionModeFlag,
                            cabacCost);

						// CBF Tu decision
					    EncodeTuCalcCost(
							contextPtr,
							countNonZeroCoeffs,
							yTuFullDistortion,
							&yTuCoeffBits,
							componentMask);


						yCoeffBits += yTuCoeffBits;
						cbCoeffBits += cbTuCoeffBits;
						crCoeffBits += crTuCoeffBits;

						yFullDistortion[DIST_CALC_RESIDUAL] += yTuFullDistortion[DIST_CALC_RESIDUAL];
						yFullDistortion[DIST_CALC_PREDICTION] += yTuFullDistortion[DIST_CALC_PREDICTION];

					}


                } // Transform Loop

            } 

            //Set Final CU data flags after skip/Merge decision.
            if (isFirstCUinRow == EB_FALSE) {

                if (cuPtr->predictionUnitArray[0].mergeFlag == EB_TRUE) {

                    cuPtr->skipFlag = (isCuSkip) ? EB_TRUE : EB_FALSE;
                    cuPtr->predictionUnitArray[0].mergeFlag = (isCuSkip) ? EB_FALSE : EB_TRUE;

                }
            }

            // Initialize the Transform Loop
            contextPtr->tuItr = (cuStats->size < MAX_LCU_SIZE) ? 0 : 1;
            yCbf = 0;
            cbCbf = 0;
            crCbf = 0;

            for (tuIt = 0; tuIt < totTu; tuIt++)
			{
				contextPtr->tuItr = (cuStats->size < MAX_LCU_SIZE) ? 0 : tuIt + 1;
                if (cuStats->size < MAX_LCU_SIZE)
                {
                    tuOriginX = contextPtr->cuOriginX;
                    tuOriginY = contextPtr->cuOriginY;
                    tuSize = cuStats->size;
                    tuSizeChroma = (tuSize >> 1);
                }
                else
                {
                    tuOriginX = contextPtr->cuOriginX + ((tuIt & 1) << 5);
                    tuOriginY = contextPtr->cuOriginY + ((tuIt > 1) << 5);
                    tuSize = 32;
                    tuSizeChroma = 16;
                }

                if (cuPtr->skipFlag == EB_TRUE){
                    cuPtr->transformUnitArray[contextPtr->tuItr].lumaCbf = EB_FALSE;
                    cuPtr->transformUnitArray[contextPtr->tuItr].cbCbf = EB_FALSE;
                    cuPtr->transformUnitArray[contextPtr->tuItr].crCbf = EB_FALSE;
                }
                else if ((&cuPtr->predictionUnitArray[0])->mergeFlag == EB_TRUE) {

                    contextPtr->forceCbfFlag = (contextPtr->skipQpmFlag) ?
                        EB_FALSE :
                        (tuOriginX == 0) && (tuOriginY == lcuOriginY);

                    SetPmEncDecMode(
                        pictureControlSetPtr,
                        contextPtr,
                        tbAddr,
                        lcuStatPtr->stationaryEdgeOverTimeFlag,
                        pictureControlSetPtr->temporalLayerIndex > 0 ? lcuStatPtr->pmStationaryEdgeOverTimeFlag : lcuStatPtr->stationaryEdgeOverTimeFlag);


                    // Set Fast El coef shaping method 
                    contextPtr->transCoeffShapeLuma     = DEFAULT_SHAPE;
                    contextPtr->transCoeffShapeChroma   = DEFAULT_SHAPE;                    
                    
					if (fastEl && isFirstCUinRow == EB_FALSE && contextPtr->pmpMaskingLevelEncDec > MASK_THSHLD_1) {
                        yDc = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yDc[tuIt];
                        yCountNonZeroCoeffs = contextPtr->mdContext->mdEpPipeLcu[cuPtr->leafIndex].yCountNonZeroCoeffs[tuIt];

						if ((cuPtr->rootCbf == 0) || ((yCoeffBitsTemp <= yBitsThsld) && yDc < YDC_THSHLD_1 && yCountNonZeroCoeffs <= 1)) {
                            // Skip pass for cuPtr->rootCbf == 0 caused some VQ issues in chroma, so DC path is used instead
                            contextPtr->transCoeffShapeLuma = ONLY_DC_SHAPE;
                            contextPtr->transCoeffShapeChroma = ONLY_DC_SHAPE;
                        }
                        else if ((yCoeffBitsTemp <= yBitsThsld * 4)) {
                            contextPtr->transCoeffShapeLuma = N4_SHAPE;
                            if ((cuStats->size >> 1) > 8)
                                contextPtr->transCoeffShapeChroma = N4_SHAPE;
                            else
                                contextPtr->transCoeffShapeChroma = N2_SHAPE;
                        }
                        else if ((yCoeffBitsTemp <= yBitsThsld * 16)) {
                            contextPtr->transCoeffShapeLuma = N2_SHAPE;
                            contextPtr->transCoeffShapeChroma = N2_SHAPE;
                        }
                    }

					EncodeLoopFunctionTable[is16bit](
						contextPtr,
						lcuPtr,
						tuOriginX,
						tuOriginY,
						cbQp,
						reconBuffer,
						coeffBufferTB,
						residualBuffer,
						transformBuffer,
						transformInnerArrayPtr,
						countNonZeroCoeffs,
						useDeltaQpSegments,
						(CabacEncodeContext_t*)coeffEstEntropyCoderPtr->cabacEncodeContextPtr,
						0,
						pictureControlSetPtr->cabacCost,
                        cuPtr->deltaQp > 0 ? 0 : dZoffset);

                }
                cuPtr->rootCbf = cuPtr->rootCbf |
                cuPtr->transformUnitArray[contextPtr->tuItr].lumaCbf |
                cuPtr->transformUnitArray[contextPtr->tuItr].cbCbf |
                cuPtr->transformUnitArray[contextPtr->tuItr].crCbf;

                if (cuPtr->transformUnitArray[contextPtr->tuItr].cbCbf){
                    cuPtr->transformUnitArray[0].cbCbf = EB_TRUE;
                }

                if (cuPtr->transformUnitArray[contextPtr->tuItr].crCbf){
                    cuPtr->transformUnitArray[0].crCbf = EB_TRUE;
                }

                if (doRecon)
                    EncodeGenerateReconFunctionPtr[is16bit](
                        contextPtr,
                        tuOriginX,
                        tuOriginY,
                        reconBuffer,
                        residualBuffer,
                        transformInnerArrayPtr);


                yCbf  |= cuPtr->transformUnitArray[contextPtr->tuItr].lumaCbf;
                cbCbf |= cuPtr->transformUnitArray[contextPtr->tuItr].cbCbf;
                crCbf |= cuPtr->transformUnitArray[contextPtr->tuItr].crCbf;


                if (dlfEnableFlag){

                    EB_U32 lumaStride = (sequenceControlSetPtr->lumaWidth >> 2);
                    TransformUnit_t *tuPtr = &cuPtr->transformUnitArray[contextPtr->tuItr];

                    // Update the cbf map for DLF
                    startIndex = (tuOriginY >> 2) * lumaStride + (tuOriginX >> 2);
                    for (blk4x4IndexY = 0; blk4x4IndexY < (tuSize >> 2); ++blk4x4IndexY){
                        EB_MEMSET(&pictureControlSetPtr->cbfMapArray[startIndex], (EB_U8)tuPtr->lumaCbf, (tuSize >> 2));
                        startIndex += lumaStride;
                    }

                    if (cuStats->size == MAX_LCU_SIZE)
                        // Set the bS on TU boundary for DLF
                        SetBSArrayBasedOnTUBoundary(
                            tuOriginX,
                            tuOriginY,
                            tuSize,
                            tuSize,
                            cuStats,
                            (EB_PART_MODE)cuPtr->predictionModeFlag,
                            lcuOriginX,
                            lcuOriginY,
                            pictureControlSetPtr,
                            pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
                            pictureControlSetPtr->verticalEdgeBSArray[tbAddr]);
                }
            } // Transform Loop

            // Calculate Root CBF
            cuPtr->rootCbf = (yCbf | cbCbf | crCbf ) ? EB_TRUE  : EB_FALSE;

            // Force Skip if MergeFlag == TRUE && RootCbf == 0
            if (cuPtr->skipFlag == EB_FALSE &&
                cuPtr->predictionUnitArray[0].mergeFlag == EB_TRUE && cuPtr->rootCbf == EB_FALSE )
            {
                cuPtr->skipFlag = EB_TRUE;
            }

            {
                // Set the PU Loop Variables
                puPtr = cuPtr->predictionUnitArray;

                // Set MvUnit
                contextPtr->mvUnit.predDirection = (EB_U8)puPtr->interPredDirectionIndex;
                contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
                contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;
                // set up the bS based on PU boundary for DLF
                if (dlfEnableFlag /*&& cuStats->size < MAX_LCU_SIZE*/  ){
                    SetBSArrayBasedOnPUBoundary(
                        epModeTypeNeighborArray,
                        epMvNeighborArray,
                        puPtr,
                        cuPtr,
                        cuStats,
                        lcuOriginX,
                        lcuOriginY,
                        pictureControlSetPtr,
                        pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
                        pictureControlSetPtr->verticalEdgeBSArray[tbAddr]);
                }


                // Update Neighbor Arrays (Mode Type, MVs, SKIP)
                {
                    EB_U8 skipFlag = (EB_U8)cuPtr->skipFlag;
                    EncodePassUpdateInterModeNeighborArrays(
                        epModeTypeNeighborArray,
                        epMvNeighborArray,
                        epSkipFlagNeighborArray,
                        &contextPtr->mvUnit,
                        &skipFlag,
                        contextPtr->cuOriginX,
                        contextPtr->cuOriginY,
                        cuStats->size);

                }

            } // 2nd Partition Loop


            // Update Recon Samples Neighbor Arrays -INTER-
            
            if (doRecon)
                EncodePassUpdateReconSampleNeighborArrays(
                    epLumaReconNeighborArray,
                    epCbReconNeighborArray,
                    epCrReconNeighborArray,
                    reconBuffer,
                    contextPtr->cuOriginX,
                    contextPtr->cuOriginY,
                    cuStats->size,
                    is16bit);
            }
            else {
                CHECK_REPORT_ERROR_NC(
                    encodeContextPtr->appCallbackPtr,
                    EB_ENC_CL_ERROR2);
            }


            if (dlfEnableFlag)
            {
                // Assign the LCU-level QP  
                if (cuPtr->predictionModeFlag == INTRA_MODE && puPtr->intraLumaMode == EB_INTRA_MODE_4x4) {
					availableCoeff = (
						contextPtr->cuPtr->transformUnitArray[1].lumaCbf ||
						contextPtr->cuPtr->transformUnitArray[2].lumaCbf ||
						contextPtr->cuPtr->transformUnitArray[3].lumaCbf ||
						contextPtr->cuPtr->transformUnitArray[4].lumaCbf ||
                        contextPtr->cuPtr->transformUnitArray[1].crCbf ||
                        contextPtr->cuPtr->transformUnitArray[1].cbCbf) ? EB_TRUE : EB_FALSE;

                } else {
                    availableCoeff = (cuPtr->predictionModeFlag == INTER_MODE) ? (EB_BOOL)cuPtr->rootCbf :
                        (cuPtr->transformUnitArray[cuStats->size == MAX_LCU_SIZE ? 1 : 0].lumaCbf ||
                        cuPtr->transformUnitArray[cuStats->size == MAX_LCU_SIZE ? 1 : 0].crCbf ||
                        cuPtr->transformUnitArray[cuStats->size == MAX_LCU_SIZE ? 1 : 0].cbCbf) ? EB_TRUE : EB_FALSE;
                }


                // Assign the LCU-level QP
                EncodePassUpdateQp(
                    pictureControlSetPtr,
                    contextPtr,
                    availableCoeff,
                    useDeltaQp,
                    &isDeltaQpNotCoded,
                    pictureControlSetPtr->difCuDeltaQpDepth,
                    &(pictureControlSetPtr->encPrevCodedQp[singleSegment ? 0 : lcuRowIndex]),
                    &(pictureControlSetPtr->encPrevQuantGroupCodedQp[singleSegment ? 0 : lcuRowIndex]),
                    lcuQp);

                // Assign DLF QP
                SetQpArrayBasedOnCU(
                    pictureControlSetPtr,
                    contextPtr->cuOriginX,
                    contextPtr->cuOriginY,
                    cuStats->size / MIN_CU_SIZE,
                    cuPtr->qp);
            }

            {

                // Update Neighbor Arrays (Leaf Depth)
                EncodePassUpdateLeafDepthNeighborArrays(
                    epLeafDepthNeighborArray,
                    cuStats->depth,
                    contextPtr->cuOriginX,
                    contextPtr->cuOriginY,
                    cuStats->size);
                {
                    // Set the PU Loop Variables
                    puPtr = cuPtr->predictionUnitArray;
                    // Set MvUnit
                    contextPtr->mvUnit.predDirection = (EB_U8)puPtr->interPredDirectionIndex;
                    contextPtr->mvUnit.mv[REF_LIST_0].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
                    contextPtr->mvUnit.mv[REF_LIST_1].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;
                }


                // Update TMVP Map (create new one and compare to the old one!!!)
                if (tmvpMapWritePtr != EB_NULL){

                    puPtr = cuPtr->predictionUnitArray;
                    tmvpMapVerticalStartIndex   = (cuStats->originY + mvCompressionUnitSizeMinus1) >> LOG_MV_COMPRESS_UNIT_SIZE;         //elemPU's vertical index relative to current LCU on 16x16 basic unit
                    tmvpMapHorizontalEndIndex   = (cuStats->originX + cuStats->size + mvCompressionUnitSizeMinus1) >> LOG_MV_COMPRESS_UNIT_SIZE;
                    tmvpMapVerticalEndIndex     = (cuStats->originY + cuStats->size + mvCompressionUnitSizeMinus1) >> LOG_MV_COMPRESS_UNIT_SIZE; // the problem is at this line, in 64x48 PU, this value turns out to be 4 while it is supposed to be 3
                    tmvpMapHorizontalStartIndex = (cuStats->originX + mvCompressionUnitSizeMinus1) >> LOG_MV_COMPRESS_UNIT_SIZE;

                    while (tmvpMapVerticalStartIndex < tmvpMapVerticalEndIndex){
                        tmvpMapHorizontalStartIndex = (cuStats->originX + mvCompressionUnitSizeMinus1) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        tmvpMapIndex = (tmvpMapVerticalStartIndex * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE)) + tmvpMapHorizontalStartIndex;

                        while ((tmvpMapHorizontalStartIndex) < tmvpMapHorizontalEndIndex){
                            switch (cuPtr->predictionModeFlag){
                            case INTER_MODE:
                                switch (cuPtr->predictionUnitArray->interPredDirectionIndex){

                                case UNI_PRED_LIST_0:
                                    tmvpMapWritePtr->availabilityFlag[tmvpMapIndex] = EB_TRUE;
                                    tmvpMapWritePtr->mv[REF_LIST_0][tmvpMapIndex].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
                                    tmvpMapWritePtr->predictionDirection[tmvpMapIndex] = UNI_PRED_LIST_0;
                                    tmvpMapWritePtr->refPicPOC[REF_LIST_0][tmvpMapIndex] = ((EbReferenceObject_t*)
                                        pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr
                                        )->refPOC;
                                    break;

                                case UNI_PRED_LIST_1:
                                    tmvpMapWritePtr->availabilityFlag[tmvpMapIndex] = EB_TRUE;
                                    tmvpMapWritePtr->mv[REF_LIST_1][tmvpMapIndex].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;
                                    tmvpMapWritePtr->predictionDirection[tmvpMapIndex] = UNI_PRED_LIST_1;
                                    tmvpMapWritePtr->refPicPOC[REF_LIST_1][tmvpMapIndex] = ((EbReferenceObject_t*)
                                        pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr
                                        )->refPOC;
                                    break;

                                case BI_PRED:
                                    if (puPtr->interPredDirectionIndex == BI_PRED || puPtr->interPredDirectionIndex == UNI_PRED_LIST_0){
                                        tmvpMapWritePtr->availabilityFlag[tmvpMapIndex] = EB_TRUE;
                                        tmvpMapWritePtr->mv[REF_LIST_0][tmvpMapIndex].mvUnion = puPtr->mv[REF_LIST_0].mvUnion;
                                        tmvpMapWritePtr->predictionDirection[tmvpMapIndex] = (EB_PREDDIRECTION)puPtr->interPredDirectionIndex;
                                        tmvpMapWritePtr->refPicPOC[REF_LIST_0][tmvpMapIndex] = ((EbReferenceObject_t*)
                                            pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr
                                            )->refPOC;
                                    }

                                    if (puPtr->interPredDirectionIndex == BI_PRED || puPtr->interPredDirectionIndex == UNI_PRED_LIST_1){
                                        tmvpMapWritePtr->availabilityFlag[tmvpMapIndex] = EB_TRUE;
                                        tmvpMapWritePtr->mv[REF_LIST_1][tmvpMapIndex].mvUnion = puPtr->mv[REF_LIST_1].mvUnion;
                                        tmvpMapWritePtr->predictionDirection[tmvpMapIndex] = (EB_PREDDIRECTION)puPtr->interPredDirectionIndex;
                                        tmvpMapWritePtr->refPicPOC[REF_LIST_1][tmvpMapIndex] = ((EbReferenceObject_t*)
                                            pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr
                                            )->refPOC;
                                    }

                                    break;

                                default:
                                    CHECK_REPORT_ERROR_NC(
                                        encodeContextPtr->appCallbackPtr,
                                        EB_ENC_INTER_PRED_ERROR0);

                                }
                                break;

                            case INTRA_MODE:
                                tmvpMapWritePtr->availabilityFlag[tmvpMapIndex] = EB_FALSE;
                                break;

                            default:

                                CHECK_REPORT_ERROR_NC(
                                    encodeContextPtr->appCallbackPtr,
                                    EB_ENC_CL_ERROR2);
                                break;
                            }

                            //*Note- Filling the map for list 1 motion info will be added when B-slices are ready

                            ++tmvpMapHorizontalStartIndex;
                            ++tmvpMapIndex;
                        }
                        ++tmvpMapVerticalStartIndex;
                    }

                }
            }


            cuItr += DepthOffset[cuStats->depth];
        }
        else{
            cuItr++;
        }

    } // CU Loop

    // First Pass Deblocking
    if (dlfEnableFlag){

        EB_U32 pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + 63) >> LOG2F_MAX_LCU_SIZE;

        LcuInternalAreaDLFCoreFuncTable[is16bit](
            reconBuffer,
            lcuOriginX,
            lcuOriginY,
            lcuWidth,
            lcuHeight,
            pictureControlSetPtr->verticalEdgeBSArray[tbAddr],
            pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
            pictureControlSetPtr);

        LcuBoundaryDLFCoreFuncTable[is16bit](
            reconBuffer,
            lcuOriginX,
            lcuOriginY,
            lcuWidth,
            lcuHeight,
            pictureControlSetPtr->verticalEdgeBSArray[tbAddr],
            pictureControlSetPtr->horizontalEdgeBSArray[tbAddr],
            lcuOriginY == 0 ? (EB_U8*)EB_NULL : pictureControlSetPtr->verticalEdgeBSArray[tbAddr - pictureWidthInLcu],
            lcuOriginX == 0 ? (EB_U8*)EB_NULL : pictureControlSetPtr->horizontalEdgeBSArray[tbAddr - 1],
            pictureControlSetPtr);

        LcuPicEdgeDLFCoreFuncTable[is16bit](
            reconBuffer,
            tbAddr,
            lcuOriginX,
            lcuOriginY,
            lcuWidth,
            lcuHeight,
            pictureControlSetPtr);

    }


    // SAO Parameter Generation 
    if (enableSaoFlag) {

        EB_S16 lcuDeltaQp = (EB_S16)(lcuPtr->qp - pictureControlSetPtr->ParentPcsPtr->averageQp);

        SaoParameters_t *leftSaoPtr;
        SaoParameters_t *topSaoPtr;

        if (lcuOriginY != 0){
            EB_U32 topSaoIndex = GetNeighborArrayUnitTopIndex(
                pictureControlSetPtr->epSaoNeighborArray,
                lcuOriginX);

            topSaoPtr = ((SaoParameters_t*)pictureControlSetPtr->epSaoNeighborArray->topArray) + topSaoIndex;
        }
        else{
            topSaoPtr = (SaoParameters_t*)EB_NULL;
        }
        if (lcuOriginX != 0){
            EB_U32 leftSaoIndex = GetNeighborArrayUnitLeftIndex(
                pictureControlSetPtr->epSaoNeighborArray,
                lcuOriginY);

            leftSaoPtr = ((SaoParameters_t*)pictureControlSetPtr->epSaoNeighborArray->leftArray) + leftSaoIndex;
        }
        else{
            leftSaoPtr = (SaoParameters_t*)EB_NULL;
        }


        EB_U8   varCount32x32 = 0;
        varCount32x32 = ((pictureControlSetPtr->ParentPcsPtr->variance[tbAddr][1]) > 1000) +
            ((pictureControlSetPtr->ParentPcsPtr->variance[tbAddr][2]) > 1000) +
            ((pictureControlSetPtr->ParentPcsPtr->variance[tbAddr][3]) > 1000) +
            ((pictureControlSetPtr->ParentPcsPtr->variance[tbAddr][4]) > 1000);

        EB_BOOL shutSaoCondition0;
        EB_BOOL shutSaoCondition1;

        shutSaoCondition0 = (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE || contextPtr->saoMode) ?
            EB_FALSE :     
            ((pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[tbAddr].edgeBlockNum == 0 || (pictureControlSetPtr->sceneCaracteristicId != 0)) && (contextPtr->skipQpmFlag == EB_FALSE) && pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_1 && !lcuStatPtr->stationaryEdgeOverTimeFlag);

        shutSaoCondition1 = (contextPtr->saoMode) ?
            EB_FALSE :
            (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE) ?
            (varCount32x32 < 1 && lcuDeltaQp <= 0 && pictureControlSetPtr->sliceType != EB_I_SLICE && !lcuStatPtr->stationaryEdgeOverTimeFlag) :
            (((varCount32x32 < 1) && (lcuDeltaQp <= 0 && pictureControlSetPtr->sliceType != EB_I_SLICE) && (contextPtr->skipQpmFlag == EB_FALSE)) && pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_1 && !lcuStatPtr->stationaryEdgeOverTimeFlag);

        if (doRecon == EB_FALSE || shutSaoCondition0 || shutSaoCondition1) {

            lcuPtr->saoParams.saoTypeIndex[SAO_COMPONENT_LUMA] = 0;
            lcuPtr->saoParams.saoTypeIndex[SAO_COMPONENT_CHROMA] = 0;
            lcuPtr->saoParams.saoOffset[SAO_COMPONENT_LUMA][0] = 0;
            lcuPtr->saoParams.saoOffset[SAO_COMPONENT_LUMA][1] = 0;
            lcuPtr->saoParams.saoOffset[SAO_COMPONENT_LUMA][2] = 0;
            lcuPtr->saoParams.saoOffset[SAO_COMPONENT_LUMA][3] = 0;
            lcuPtr->saoParams.saoBandPosition[SAO_COMPONENT_LUMA] = 0;
            lcuPtr->saoParams.saoMergeLeftFlag = EB_FALSE;
            lcuPtr->saoParams.saoMergeUpFlag = EB_FALSE;

            saoLumaBestCost = 0xFFFFFFFFFFFFFFFFull;
            saoChromaBestCost = 0xFFFFFFFFFFFFFFFFull;

        }
        else {
            // Generate the SAO Parameters
            if (is16bit){
                SaoGenerationDecision16bit(
                    contextPtr->inputSample16bitBuffer,
                    contextPtr->saoStats,
                    &lcuPtr->saoParams,
                    contextPtr->mdRateEstimationPtr,
                    contextPtr->fullLambda,
		    		contextPtr->fullChromaLambdaSao,
                    contextPtr->saoMode,
                    pictureControlSetPtr,
                    lcuOriginX,
                    lcuOriginY,
                    lcuWidth,
                    lcuHeight,
                    &lcuPtr->saoParams,
                    leftSaoPtr,
                    topSaoPtr,
                    &saoLumaBestCost,
                    &saoChromaBestCost);

            }
            else{
                SaoGenerationDecision(
                    contextPtr->saoStats,
                    &lcuPtr->saoParams,
                    contextPtr->mdRateEstimationPtr,
                    contextPtr->fullLambda,
                    contextPtr->fullChromaLambdaSao,
                    contextPtr->saoMode,
                    pictureControlSetPtr,
                    lcuOriginX,
                    lcuOriginY,
                    lcuWidth,
                    lcuHeight,
                    &lcuPtr->saoParams,
                    leftSaoPtr,
                    topSaoPtr,
                    &saoLumaBestCost,
                    &saoChromaBestCost);

                if (contextPtr->skipQpmFlag == EB_FALSE){
                    if (pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_3_1 && pictureControlSetPtr->pictureQp >= 37) {
                        lcuPtr->saoParams.saoTypeIndex[SAO_COMPONENT_LUMA] = 0;
                        lcuPtr->saoParams.saoTypeIndex[SAO_COMPONENT_CHROMA] = 0;
                        lcuPtr->saoParams.saoMergeLeftFlag = EB_FALSE;
                        lcuPtr->saoParams.saoMergeUpFlag = EB_FALSE;
                    }
                }
            }           
        }

        // Update the SAO Neighbor Array
        EncodePassUpdateSaoNeighborArrays(
            pictureControlSetPtr->epSaoNeighborArray,
            &lcuPtr->saoParams,
            lcuOriginX,
            lcuOriginY,
            contextPtr->lcuSize);
    }
    return;
}


void UnusedVariablevoidFunc_CodingLoop()
{
    (void)NxMSadLoopKernel_funcPtrArray;
    (void)NxMSadAveragingKernel_funcPtrArray;
}
