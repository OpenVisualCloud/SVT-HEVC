/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"

#include "EbResourceCoordinationResults.h"
#include "EbPictureAnalysisProcess.h"
#include "EbPictureAnalysisResults.h"
#include "EbMcp.h"
#include "EbMotionEstimation.h"
#include "EbReferenceObject.h"

#include "EbComputeMean.h"
#include "EbMeSadCalculation.h"
#include "EbPictureOperators.h"
#include "EbComputeMean_SSE2.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"

#define VARIANCE_PRECISION		16
#define  LCU_LOW_VAR_TH                5
#define  PIC_LOW_VAR_PERCENTAGE_TH    60
#define	FLAT_MAX_VAR			50
#define FLAT_MAX_VAR_DECIM		(50-00)
#define	NOISE_MIN_LEVEL_0		 70000//120000
#define NOISE_MIN_LEVEL_DECIM_0 (70000+000000)//(120000+000000)
#define	NOISE_MIN_LEVEL_1        120000
#define NOISE_MIN_LEVEL_DECIM_1	(120000+000000)
#define DENOISER_QP_TH			29
#define DENOISER_BITRATE_TH		14000000
#define SAMPLE_THRESHOLD_PRECENT_BORDER_LINE      15
#define SAMPLE_THRESHOLD_PRECENT_TWO_BORDER_LINES 10

/************************************************
* Picture Analysis Context Constructor
************************************************/
EB_ERRORTYPE PictureAnalysisContextCtor(
	EbPictureBufferDescInitData_t * inputPictureBufferDescInitData,
	EB_BOOL                         denoiseFlag,
    PictureAnalysisContext_t      **contextDblPtr,
    EbFifo_t                       *resourceCoordinationResultsInputFifoPtr,
    EbFifo_t                       *pictureAnalysisResultsOutputFifoPtr,
    EB_U16						    lcuTotalCount)
{
	PictureAnalysisContext_t *contextPtr;
	EB_MALLOC(PictureAnalysisContext_t*, contextPtr, sizeof(PictureAnalysisContext_t), EB_N_PTR);
	*contextDblPtr = contextPtr;

	contextPtr->resourceCoordinationResultsInputFifoPtr = resourceCoordinationResultsInputFifoPtr;
	contextPtr->pictureAnalysisResultsOutputFifoPtr = pictureAnalysisResultsOutputFifoPtr;

	EB_ERRORTYPE return_error = EB_ErrorNone;

	if (denoiseFlag == EB_TRUE){

		//denoised
        // If 420/422, re-use luma for chroma
        // If 444, re-use luma for Cr
        if (inputPictureBufferDescInitData->colorFormat != EB_YUV444) {
		    inputPictureBufferDescInitData->bufferEnableMask = PICTURE_BUFFER_DESC_Y_FLAG;
        } else {
		    inputPictureBufferDescInitData->bufferEnableMask = PICTURE_BUFFER_DESC_Y_FLAG | PICTURE_BUFFER_DESC_Cb_FLAG;
        }

		return_error = EbPictureBufferDescCtor(
			(EB_PTR*)&(contextPtr->denoisedPicturePtr),
			(EB_PTR)inputPictureBufferDescInitData);

		if (return_error == EB_ErrorInsufficientResources){
			return EB_ErrorInsufficientResources;
		}

        if (inputPictureBufferDescInitData->colorFormat != EB_YUV444) {
		contextPtr->denoisedPicturePtr->bufferCb = contextPtr->denoisedPicturePtr->bufferY;
		contextPtr->denoisedPicturePtr->bufferCr = contextPtr->denoisedPicturePtr->bufferY + contextPtr->denoisedPicturePtr->chromaSize;
        } else {
		    contextPtr->denoisedPicturePtr->bufferCr = contextPtr->denoisedPicturePtr->bufferY;
        }

		// noise
		inputPictureBufferDescInitData->maxHeight = MAX_LCU_SIZE;
		inputPictureBufferDescInitData->bufferEnableMask = PICTURE_BUFFER_DESC_Y_FLAG;

		return_error = EbPictureBufferDescCtor(
			(EB_PTR*)&(contextPtr->noisePicturePtr),
			(EB_PTR)inputPictureBufferDescInitData);

		if (return_error == EB_ErrorInsufficientResources){
			return EB_ErrorInsufficientResources;
		}
	}

    EB_MALLOC(EB_U16**, contextPtr->grad, sizeof(EB_U16*) * lcuTotalCount, EB_N_PTR);
    for (EB_U16 lcuIndex = 0; lcuIndex < lcuTotalCount; ++lcuIndex) {
        EB_MALLOC(EB_U16*, contextPtr->grad[lcuIndex], sizeof(EB_U16) * CU_MAX_COUNT, EB_N_PTR);
    }


	return EB_ErrorNone;
}

static void DownSampleChroma(EbPictureBufferDesc_t* inputPicturePtr, EbPictureBufferDesc_t* outputPicturePtr)
{
	EB_U32 inputColorFormat = inputPicturePtr->colorFormat;
	EB_U16 inputSubWidthCMinus1 = (inputColorFormat == EB_YUV444 ? 1 : 2) - 1;
	EB_U16 inputSubHeightCMinus1 = (inputColorFormat >= EB_YUV422 ? 1 : 2) - 1;

	EB_U32 outputColorFormat = outputPicturePtr->colorFormat;
	EB_U16 outputSubWidthCMinus1 = (outputColorFormat == EB_YUV444 ? 1 : 2) - 1;
	EB_U16 outputSubHeightCMinus1 = (outputColorFormat >= EB_YUV422 ? 1 : 2) - 1;

	EB_U32 strideIn, strideOut;
	EB_U32 inputOriginIndex, outputOriginIndex;

	EB_U8 *ptrIn;
	EB_U8 *ptrOut;

	EB_U32 ii, jj;

	//Cb
	{
		strideIn = inputPicturePtr->strideCb;
		inputOriginIndex = (inputPicturePtr->originX >> inputSubWidthCMinus1) +
            (inputPicturePtr->originY >> inputSubHeightCMinus1)  * inputPicturePtr->strideCb;
		ptrIn = &(inputPicturePtr->bufferCb[inputOriginIndex]);

		strideOut = outputPicturePtr->strideCb;
		outputOriginIndex = (outputPicturePtr->originX >> outputSubWidthCMinus1) +
            (outputPicturePtr->originY >> outputSubHeightCMinus1)  * outputPicturePtr->strideCb;
		ptrOut = &(outputPicturePtr->bufferCb[outputOriginIndex]);

		for (jj = 0; jj < (EB_U32)(outputPicturePtr->height >> outputSubHeightCMinus1); jj++) {
			for (ii = 0; ii < (EB_U32)(outputPicturePtr->width >> outputSubWidthCMinus1); ii++) {
				ptrOut[ii + jj * strideOut] =
                    ptrIn[(ii << (1 - inputSubWidthCMinus1)) +
                    (jj << (1 - inputSubHeightCMinus1)) * strideIn];
			}
		}

	}

	//Cr
	{
		strideIn = inputPicturePtr->strideCr;
		inputOriginIndex = (inputPicturePtr->originX >> inputSubWidthCMinus1) + (inputPicturePtr->originY >> inputSubHeightCMinus1)  * inputPicturePtr->strideCr;
		ptrIn = &(inputPicturePtr->bufferCr[inputOriginIndex]);

		strideOut = outputPicturePtr->strideCr;
		outputOriginIndex = (outputPicturePtr->originX >> outputSubWidthCMinus1) + (outputPicturePtr->originY >> outputSubHeightCMinus1)  * outputPicturePtr->strideCr;
		ptrOut = &(outputPicturePtr->bufferCr[outputOriginIndex]);

		for (jj = 0; jj < (EB_U32)(outputPicturePtr->height >> outputSubHeightCMinus1); jj++) {
			for (ii = 0; ii < (EB_U32)(outputPicturePtr->width >> outputSubWidthCMinus1); ii++) {
				ptrOut[ii + jj * strideOut] =
                    ptrIn[(ii << (1 - inputSubWidthCMinus1)) +
                    (jj << (1 - inputSubHeightCMinus1)) * strideIn];
			}
		}
	}
}

/************************************************
 * Picture Analysis Context Destructor
 ************************************************/

/********************************************
 * Decimation2D
 *      decimates the input
 ********************************************/
void Decimation2D(
	EB_U8 *  inputSamples,      // input parameter, input samples Ptr
	EB_U32   inputStride,       // input parameter, input stride
	EB_U32   inputAreaWidth,    // input parameter, input area width
	EB_U32   inputAreaHeight,   // input parameter, input area height    
	EB_U8 *  decimSamples,      // output parameter, decimated samples Ptr
	EB_U32   decimStride,       // input parameter, output stride
	EB_U32   decimStep)        // input parameter, area height  
{

	EB_U32 horizontalIndex;
	EB_U32 verticalIndex;


	for (verticalIndex = 0; verticalIndex < inputAreaHeight; verticalIndex += decimStep) {
		for (horizontalIndex = 0; horizontalIndex < inputAreaWidth; horizontalIndex += decimStep) {

			decimSamples[(horizontalIndex >> (decimStep >> 1))] = inputSamples[horizontalIndex];

		}
		inputSamples += (inputStride << (decimStep >> 1));
		decimSamples += decimStride;
	}

	return;
}

/********************************************
* CalculateHistogram
*      creates n-bins histogram for the input
********************************************/
static void CalculateHistogram(
	EB_U8 *  inputSamples,      // input parameter, input samples Ptr
	EB_U32   inputAreaWidth,    // input parameter, input area width
	EB_U32   inputAreaHeight,   // input parameter, input area height  
	EB_U32   stride,            // input parameter, input stride
    EB_U8    decimStep,         // input parameter, area height  
	EB_U32  *histogram,			// output parameter, output histogram
	EB_U64  *sum)

{

	EB_U32 horizontalIndex;
	EB_U32 verticalIndex;
	*sum = 0;

	for (verticalIndex = 0; verticalIndex < inputAreaHeight; verticalIndex += decimStep) {
		for (horizontalIndex = 0; horizontalIndex < inputAreaWidth; horizontalIndex += decimStep) {
			++(histogram[inputSamples[horizontalIndex]]);
			*sum += inputSamples[horizontalIndex];
		}
        inputSamples += (stride << (decimStep >> 1));
	}

	return;
}


static EB_U64 ComputeVariance32x32(
	EbPictureBufferDesc_t       *inputPaddedPicturePtr,         // input parameter, Input Padded Picture  
	EB_U32                       inputLumaOriginIndex,          // input parameter, LCU index, used to point to source/reference samples
	EB_U64						*variance8x8)
{

	EB_U32 blockIndex;

	EB_U64 meanOf8x8Blocks[16];
	EB_U64 meanOf8x8SquaredValuesBlocks[16];

	EB_U64 meanOf16x16Blocks[4];
	EB_U64 meanOf16x16SquaredValuesBlocks[4];

	EB_U64 meanOf32x32Blocks;
	EB_U64 meanOf32x32SquaredValuesBlocks;
	/////////////////////////////////////////////
	// (0,0)
	blockIndex = inputLumaOriginIndex;

	meanOf8x8Blocks[0] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[0] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (0,1)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[1] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[1] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (0,2)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[2] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[2] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (0,3)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[3] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[3] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	

	// (1,0)
	blockIndex = inputLumaOriginIndex + (inputPaddedPicturePtr->strideY << 3);
	meanOf8x8Blocks[4] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[4] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (1,1)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[5] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[5] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (1,2)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[6] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[6] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (1,3)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[7] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[7] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	

	// (2,0)
	blockIndex = inputLumaOriginIndex + (inputPaddedPicturePtr->strideY << 4);
	meanOf8x8Blocks[8] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[8] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (2,1)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[9] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[9] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (2,2)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[10] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[10] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (2,3)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[11] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[11] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	

	// (3,0)
	blockIndex = inputLumaOriginIndex + (inputPaddedPicturePtr->strideY << 3) + (inputPaddedPicturePtr->strideY << 4);
	meanOf8x8Blocks[12] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[12] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (3,1)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[13] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[13] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (3,2)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[14] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[14] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (3,3)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[15] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[15] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);


	/////////////////////////////////////////////

	variance8x8[0] = meanOf8x8SquaredValuesBlocks[0] - (meanOf8x8Blocks[0] * meanOf8x8Blocks[0]);
	variance8x8[1] = meanOf8x8SquaredValuesBlocks[1] - (meanOf8x8Blocks[1] * meanOf8x8Blocks[1]);
	variance8x8[2] = meanOf8x8SquaredValuesBlocks[2] - (meanOf8x8Blocks[2] * meanOf8x8Blocks[2]);
	variance8x8[3] = meanOf8x8SquaredValuesBlocks[3] - (meanOf8x8Blocks[3] * meanOf8x8Blocks[3]);
	variance8x8[4] = meanOf8x8SquaredValuesBlocks[4] - (meanOf8x8Blocks[4] * meanOf8x8Blocks[4]);
	variance8x8[5] = meanOf8x8SquaredValuesBlocks[5] - (meanOf8x8Blocks[5] * meanOf8x8Blocks[5]);
	variance8x8[6] = meanOf8x8SquaredValuesBlocks[6] - (meanOf8x8Blocks[6] * meanOf8x8Blocks[6]);
	variance8x8[7] = meanOf8x8SquaredValuesBlocks[7] - (meanOf8x8Blocks[7] * meanOf8x8Blocks[7]);
	variance8x8[8] = meanOf8x8SquaredValuesBlocks[8] - (meanOf8x8Blocks[8] * meanOf8x8Blocks[8]);
	variance8x8[9] = meanOf8x8SquaredValuesBlocks[9] - (meanOf8x8Blocks[9] * meanOf8x8Blocks[9]);
	variance8x8[10] = meanOf8x8SquaredValuesBlocks[10] - (meanOf8x8Blocks[10] * meanOf8x8Blocks[10]);
	variance8x8[11] = meanOf8x8SquaredValuesBlocks[11] - (meanOf8x8Blocks[11] * meanOf8x8Blocks[11]);
	variance8x8[12] = meanOf8x8SquaredValuesBlocks[12] - (meanOf8x8Blocks[12] * meanOf8x8Blocks[12]);
	variance8x8[13] = meanOf8x8SquaredValuesBlocks[13] - (meanOf8x8Blocks[13] * meanOf8x8Blocks[13]);
	variance8x8[14] = meanOf8x8SquaredValuesBlocks[14] - (meanOf8x8Blocks[14] * meanOf8x8Blocks[14]);
	variance8x8[15] = meanOf8x8SquaredValuesBlocks[15] - (meanOf8x8Blocks[15] * meanOf8x8Blocks[15]);

	// 16x16
	meanOf16x16Blocks[0] = (meanOf8x8Blocks[0] + meanOf8x8Blocks[1] + meanOf8x8Blocks[8] + meanOf8x8Blocks[9]) >> 2;
	meanOf16x16Blocks[1] = (meanOf8x8Blocks[2] + meanOf8x8Blocks[3] + meanOf8x8Blocks[10] + meanOf8x8Blocks[11]) >> 2;
	meanOf16x16Blocks[2] = (meanOf8x8Blocks[4] + meanOf8x8Blocks[5] + meanOf8x8Blocks[12] + meanOf8x8Blocks[13]) >> 2;
	meanOf16x16Blocks[3] = (meanOf8x8Blocks[6] + meanOf8x8Blocks[7] + meanOf8x8Blocks[14] + meanOf8x8Blocks[15]) >> 2;

	
	meanOf16x16SquaredValuesBlocks[0] = (meanOf8x8SquaredValuesBlocks[0] + meanOf8x8SquaredValuesBlocks[1] + meanOf8x8SquaredValuesBlocks[8] + meanOf8x8SquaredValuesBlocks[9]) >> 2;
	meanOf16x16SquaredValuesBlocks[1] = (meanOf8x8SquaredValuesBlocks[2] + meanOf8x8SquaredValuesBlocks[3] + meanOf8x8SquaredValuesBlocks[10] + meanOf8x8SquaredValuesBlocks[11]) >> 2;
	meanOf16x16SquaredValuesBlocks[2] = (meanOf8x8SquaredValuesBlocks[4] + meanOf8x8SquaredValuesBlocks[5] + meanOf8x8SquaredValuesBlocks[12] + meanOf8x8SquaredValuesBlocks[13]) >> 2;
	meanOf16x16SquaredValuesBlocks[3] = (meanOf8x8SquaredValuesBlocks[6] + meanOf8x8SquaredValuesBlocks[7] + meanOf8x8SquaredValuesBlocks[14] + meanOf8x8SquaredValuesBlocks[15]) >> 2;

	// 32x32
	meanOf32x32Blocks = (meanOf16x16Blocks[0] + meanOf16x16Blocks[1] + meanOf16x16Blocks[2] + meanOf16x16Blocks[3]) >> 2;
	

	meanOf32x32SquaredValuesBlocks = (meanOf16x16SquaredValuesBlocks[0] + meanOf16x16SquaredValuesBlocks[1] + meanOf16x16SquaredValuesBlocks[2] + meanOf16x16SquaredValuesBlocks[3]) >> 2;
		

	return (meanOf32x32SquaredValuesBlocks - (meanOf32x32Blocks * meanOf32x32Blocks));
}

static EB_U64 ComputeVariance16x16(
	EbPictureBufferDesc_t       *inputPaddedPicturePtr,         // input parameter, Input Padded Picture  
	EB_U32                       inputLumaOriginIndex,          // input parameter, LCU index, used to point to source/reference samples
	EB_U64						*variance8x8)
{

	EB_U32 blockIndex;

	EB_U64 meanOf8x8Blocks[4];
	EB_U64 meanOf8x8SquaredValuesBlocks[4];

	EB_U64 meanOf16x16Blocks;
	EB_U64 meanOf16x16SquaredValuesBlocks;

	// (0,0)
	blockIndex = inputLumaOriginIndex;

	meanOf8x8Blocks[0] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[0] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (0,1)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[1] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[1] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (1,0)
	blockIndex = inputLumaOriginIndex + (inputPaddedPicturePtr->strideY << 3);
	meanOf8x8Blocks[2] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[2] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	// (1,1)
	blockIndex = blockIndex + 8;
	meanOf8x8Blocks[3] = ComputeMeanFunc[0][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);
	meanOf8x8SquaredValuesBlocks[3] = ComputeMeanFunc[1][!!(ASM_TYPES & AVX2_MASK)](&(inputPaddedPicturePtr->bufferY[blockIndex]), inputPaddedPicturePtr->strideY, 8, 8);

	variance8x8[0] = meanOf8x8SquaredValuesBlocks[0] - (meanOf8x8Blocks[0] * meanOf8x8Blocks[0]);
	variance8x8[1] = meanOf8x8SquaredValuesBlocks[1] - (meanOf8x8Blocks[1] * meanOf8x8Blocks[1]);
	variance8x8[2] = meanOf8x8SquaredValuesBlocks[2] - (meanOf8x8Blocks[2] * meanOf8x8Blocks[2]);
	variance8x8[3] = meanOf8x8SquaredValuesBlocks[3] - (meanOf8x8Blocks[3] * meanOf8x8Blocks[3]);

	// 16x16
	meanOf16x16Blocks = (meanOf8x8Blocks[0] + meanOf8x8Blocks[1] + meanOf8x8Blocks[2] + meanOf8x8Blocks[3]) >> 2;
	meanOf16x16SquaredValuesBlocks = (meanOf8x8SquaredValuesBlocks[0] + meanOf8x8SquaredValuesBlocks[1] + meanOf8x8SquaredValuesBlocks[2] + meanOf8x8SquaredValuesBlocks[3]) >> 2;

	return (meanOf16x16SquaredValuesBlocks - (meanOf16x16Blocks * meanOf16x16Blocks));
}

/*******************************************
ComputeVariance64x64
this function is exactly same as
PictureAnalysisComputeVarianceLcu excpet it
does not store data for every block,
just returns the 64x64 data point
*******************************************/
static EB_U64 ComputeVariance64x64(
    EbPictureBufferDesc_t       *inputPaddedPicturePtr,         // input parameter, Input Padded Picture  
	EB_U32                       inputLumaOriginIndex,          // input parameter, LCU index, used to point to source/reference samples
	EB_U64						*variance32x32)
{


	EB_U32 blockIndex;

	EB_U64 meanOf8x8Blocks[64];
	EB_U64 meanOf8x8SquaredValuesBlocks[64];

	EB_U64 meanOf16x16Blocks[16];
	EB_U64 meanOf16x16SquaredValuesBlocks[16];

	EB_U64 meanOf32x32Blocks[4];
	EB_U64 meanOf32x32SquaredValuesBlocks[4];

	EB_U64 meanOf64x64Blocks;
	EB_U64 meanOf64x64SquaredValuesBlocks;

	// (0,0)
	blockIndex = inputLumaOriginIndex;
	const EB_U16 strideY = inputPaddedPicturePtr->strideY;

    if (!!(ASM_TYPES & AVX2_MASK)) {

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[0], &meanOf8x8SquaredValuesBlocks[0]);

        // (0,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[4], &meanOf8x8SquaredValuesBlocks[4]);
        // (0,5)
        blockIndex = blockIndex + 24;

        // (1,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[8], &meanOf8x8SquaredValuesBlocks[8]);

        // (1,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[12], &meanOf8x8SquaredValuesBlocks[12]);

        // (1,5)
        blockIndex = blockIndex + 24;

        // (2,0)
        blockIndex = inputLumaOriginIndex + (strideY << 4);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[16], &meanOf8x8SquaredValuesBlocks[16]);

        // (2,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[20], &meanOf8x8SquaredValuesBlocks[20]);

        // (2,5)
        blockIndex = blockIndex + 24;

        // (3,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[24], &meanOf8x8SquaredValuesBlocks[24]);

        // (3,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[28], &meanOf8x8SquaredValuesBlocks[28]);

        // (3,5)
        blockIndex = blockIndex + 24;

        // (4,0)
        blockIndex = inputLumaOriginIndex + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[32], &meanOf8x8SquaredValuesBlocks[32]);

        // (4,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[36], &meanOf8x8SquaredValuesBlocks[36]);

        // (4,5)
        blockIndex = blockIndex + 24;

        // (5,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[40], &meanOf8x8SquaredValuesBlocks[40]);

        // (5,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[44], &meanOf8x8SquaredValuesBlocks[44]);

        // (5,5)
        blockIndex = blockIndex + 24;

        // (6,0)
        blockIndex = inputLumaOriginIndex + (strideY << 4) + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[48], &meanOf8x8SquaredValuesBlocks[48]);

        // (6,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[52], &meanOf8x8SquaredValuesBlocks[52]);

        // (6,5)
        blockIndex = blockIndex + 24;

        // (7,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4) + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[56], &meanOf8x8SquaredValuesBlocks[56]);

        // (7,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[60], &meanOf8x8SquaredValuesBlocks[60]);


    }
    else{
        meanOf8x8Blocks[0] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[0] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[1] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[1] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[2] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[2] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[3] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[3] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[4] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[4] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[5] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[5] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[6] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[6] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (0,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[7] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[7] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (1,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 3);
	    meanOf8x8Blocks[8] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[8] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (1,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[9] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[9] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

	    // (1,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[10] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
	    meanOf8x8SquaredValuesBlocks[10] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (1,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[11] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[11] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (1,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[12] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[12] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (1,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[13] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[13] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (1,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[14] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[14] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (1,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[15] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[15] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 4);
	    meanOf8x8Blocks[16] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[16] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[17] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[17] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[18] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[18] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[19] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[19] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    /// (2,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[20] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[20] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[21] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[21] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[22] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[22] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (2,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[23] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[23] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4);
	    meanOf8x8Blocks[24] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[24] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[25] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[25] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[26] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[26] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[27] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[27] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[28] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[28] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[29] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[29] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[30] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[30] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (3,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[31] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[31] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 5);
	    meanOf8x8Blocks[32] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[32] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[33] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[33] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[34] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[34] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[35] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[35] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[36] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[36] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[37] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[37] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[38] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[38] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (4,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[39] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[39] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 5);
	    meanOf8x8Blocks[40] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[40] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[41] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[41] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[42] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[42] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[43] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[43] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[44] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[44] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[45] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[45] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[46] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[46] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (5,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[47] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[47] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 4) + (strideY << 5);
	    meanOf8x8Blocks[48] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[48] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[49] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[49] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[50] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[50] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[51] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[51] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[52] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[52] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[53] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[53] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[54] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[54] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (6,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[55] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[55] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,0)
	    blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4) + (strideY << 5);
	    meanOf8x8Blocks[56] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[56] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,1)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[57] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[57] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,2)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[58] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[58] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,3)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[59] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[59] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,4)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[60] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[60] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,5)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[61] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[61] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,6)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[62] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[62] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);

	    // (7,7)
	    blockIndex = blockIndex + 8;
	    meanOf8x8Blocks[63] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
	    meanOf8x8SquaredValuesBlocks[63] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]),  strideY);
    
    
    }


	// 16x16
	meanOf16x16Blocks[0] = (meanOf8x8Blocks[0] + meanOf8x8Blocks[1] + meanOf8x8Blocks[8] + meanOf8x8Blocks[9]) >> 2;
	meanOf16x16Blocks[1] = (meanOf8x8Blocks[2] + meanOf8x8Blocks[3] + meanOf8x8Blocks[10] + meanOf8x8Blocks[11]) >> 2;
	meanOf16x16Blocks[2] = (meanOf8x8Blocks[4] + meanOf8x8Blocks[5] + meanOf8x8Blocks[12] + meanOf8x8Blocks[13]) >> 2;
	meanOf16x16Blocks[3] = (meanOf8x8Blocks[6] + meanOf8x8Blocks[7] + meanOf8x8Blocks[14] + meanOf8x8Blocks[15]) >> 2;

	meanOf16x16Blocks[4] = (meanOf8x8Blocks[16] + meanOf8x8Blocks[17] + meanOf8x8Blocks[24] + meanOf8x8Blocks[25]) >> 2;
	meanOf16x16Blocks[5] = (meanOf8x8Blocks[18] + meanOf8x8Blocks[19] + meanOf8x8Blocks[26] + meanOf8x8Blocks[27]) >> 2;
	meanOf16x16Blocks[6] = (meanOf8x8Blocks[20] + meanOf8x8Blocks[21] + meanOf8x8Blocks[28] + meanOf8x8Blocks[29]) >> 2;
	meanOf16x16Blocks[7] = (meanOf8x8Blocks[22] + meanOf8x8Blocks[23] + meanOf8x8Blocks[30] + meanOf8x8Blocks[31]) >> 2;

	meanOf16x16Blocks[8] = (meanOf8x8Blocks[32] + meanOf8x8Blocks[33] + meanOf8x8Blocks[40] + meanOf8x8Blocks[41]) >> 2;
	meanOf16x16Blocks[9] = (meanOf8x8Blocks[34] + meanOf8x8Blocks[35] + meanOf8x8Blocks[42] + meanOf8x8Blocks[43]) >> 2;
	meanOf16x16Blocks[10] = (meanOf8x8Blocks[36] + meanOf8x8Blocks[37] + meanOf8x8Blocks[44] + meanOf8x8Blocks[45]) >> 2;
	meanOf16x16Blocks[11] = (meanOf8x8Blocks[38] + meanOf8x8Blocks[39] + meanOf8x8Blocks[46] + meanOf8x8Blocks[47]) >> 2;

	meanOf16x16Blocks[12] = (meanOf8x8Blocks[48] + meanOf8x8Blocks[49] + meanOf8x8Blocks[56] + meanOf8x8Blocks[57]) >> 2;
	meanOf16x16Blocks[13] = (meanOf8x8Blocks[50] + meanOf8x8Blocks[51] + meanOf8x8Blocks[58] + meanOf8x8Blocks[59]) >> 2;
	meanOf16x16Blocks[14] = (meanOf8x8Blocks[52] + meanOf8x8Blocks[53] + meanOf8x8Blocks[60] + meanOf8x8Blocks[61]) >> 2;
	meanOf16x16Blocks[15] = (meanOf8x8Blocks[54] + meanOf8x8Blocks[55] + meanOf8x8Blocks[62] + meanOf8x8Blocks[63]) >> 2;

	meanOf16x16SquaredValuesBlocks[0] = (meanOf8x8SquaredValuesBlocks[0] + meanOf8x8SquaredValuesBlocks[1] + meanOf8x8SquaredValuesBlocks[8] + meanOf8x8SquaredValuesBlocks[9]) >> 2;
	meanOf16x16SquaredValuesBlocks[1] = (meanOf8x8SquaredValuesBlocks[2] + meanOf8x8SquaredValuesBlocks[3] + meanOf8x8SquaredValuesBlocks[10] + meanOf8x8SquaredValuesBlocks[11]) >> 2;
	meanOf16x16SquaredValuesBlocks[2] = (meanOf8x8SquaredValuesBlocks[4] + meanOf8x8SquaredValuesBlocks[5] + meanOf8x8SquaredValuesBlocks[12] + meanOf8x8SquaredValuesBlocks[13]) >> 2;
	meanOf16x16SquaredValuesBlocks[3] = (meanOf8x8SquaredValuesBlocks[6] + meanOf8x8SquaredValuesBlocks[7] + meanOf8x8SquaredValuesBlocks[14] + meanOf8x8SquaredValuesBlocks[15]) >> 2;

	meanOf16x16SquaredValuesBlocks[4] = (meanOf8x8SquaredValuesBlocks[16] + meanOf8x8SquaredValuesBlocks[17] + meanOf8x8SquaredValuesBlocks[24] + meanOf8x8SquaredValuesBlocks[25]) >> 2;
	meanOf16x16SquaredValuesBlocks[5] = (meanOf8x8SquaredValuesBlocks[18] + meanOf8x8SquaredValuesBlocks[19] + meanOf8x8SquaredValuesBlocks[26] + meanOf8x8SquaredValuesBlocks[27]) >> 2;
	meanOf16x16SquaredValuesBlocks[6] = (meanOf8x8SquaredValuesBlocks[20] + meanOf8x8SquaredValuesBlocks[21] + meanOf8x8SquaredValuesBlocks[28] + meanOf8x8SquaredValuesBlocks[29]) >> 2;
	meanOf16x16SquaredValuesBlocks[7] = (meanOf8x8SquaredValuesBlocks[22] + meanOf8x8SquaredValuesBlocks[23] + meanOf8x8SquaredValuesBlocks[30] + meanOf8x8SquaredValuesBlocks[31]) >> 2;

	meanOf16x16SquaredValuesBlocks[8] = (meanOf8x8SquaredValuesBlocks[32] + meanOf8x8SquaredValuesBlocks[33] + meanOf8x8SquaredValuesBlocks[40] + meanOf8x8SquaredValuesBlocks[41]) >> 2;
	meanOf16x16SquaredValuesBlocks[9] = (meanOf8x8SquaredValuesBlocks[34] + meanOf8x8SquaredValuesBlocks[35] + meanOf8x8SquaredValuesBlocks[42] + meanOf8x8SquaredValuesBlocks[43]) >> 2;
	meanOf16x16SquaredValuesBlocks[10] = (meanOf8x8SquaredValuesBlocks[36] + meanOf8x8SquaredValuesBlocks[37] + meanOf8x8SquaredValuesBlocks[44] + meanOf8x8SquaredValuesBlocks[45]) >> 2;
	meanOf16x16SquaredValuesBlocks[11] = (meanOf8x8SquaredValuesBlocks[38] + meanOf8x8SquaredValuesBlocks[39] + meanOf8x8SquaredValuesBlocks[46] + meanOf8x8SquaredValuesBlocks[47]) >> 2;

	meanOf16x16SquaredValuesBlocks[12] = (meanOf8x8SquaredValuesBlocks[48] + meanOf8x8SquaredValuesBlocks[49] + meanOf8x8SquaredValuesBlocks[56] + meanOf8x8SquaredValuesBlocks[57]) >> 2;
	meanOf16x16SquaredValuesBlocks[13] = (meanOf8x8SquaredValuesBlocks[50] + meanOf8x8SquaredValuesBlocks[51] + meanOf8x8SquaredValuesBlocks[58] + meanOf8x8SquaredValuesBlocks[59]) >> 2;
	meanOf16x16SquaredValuesBlocks[14] = (meanOf8x8SquaredValuesBlocks[52] + meanOf8x8SquaredValuesBlocks[53] + meanOf8x8SquaredValuesBlocks[60] + meanOf8x8SquaredValuesBlocks[61]) >> 2;
	meanOf16x16SquaredValuesBlocks[15] = (meanOf8x8SquaredValuesBlocks[54] + meanOf8x8SquaredValuesBlocks[55] + meanOf8x8SquaredValuesBlocks[62] + meanOf8x8SquaredValuesBlocks[63]) >> 2;

	// 32x32
	meanOf32x32Blocks[0] = (meanOf16x16Blocks[0] + meanOf16x16Blocks[1] + meanOf16x16Blocks[4] + meanOf16x16Blocks[5]) >> 2;
	meanOf32x32Blocks[1] = (meanOf16x16Blocks[2] + meanOf16x16Blocks[3] + meanOf16x16Blocks[6] + meanOf16x16Blocks[7]) >> 2;
	meanOf32x32Blocks[2] = (meanOf16x16Blocks[8] + meanOf16x16Blocks[9] + meanOf16x16Blocks[12] + meanOf16x16Blocks[13]) >> 2;
	meanOf32x32Blocks[3] = (meanOf16x16Blocks[10] + meanOf16x16Blocks[11] + meanOf16x16Blocks[14] + meanOf16x16Blocks[15]) >> 2;

	meanOf32x32SquaredValuesBlocks[0] = (meanOf16x16SquaredValuesBlocks[0] + meanOf16x16SquaredValuesBlocks[1] + meanOf16x16SquaredValuesBlocks[4] + meanOf16x16SquaredValuesBlocks[5]) >> 2;
	meanOf32x32SquaredValuesBlocks[1] = (meanOf16x16SquaredValuesBlocks[2] + meanOf16x16SquaredValuesBlocks[3] + meanOf16x16SquaredValuesBlocks[6] + meanOf16x16SquaredValuesBlocks[7]) >> 2;
	meanOf32x32SquaredValuesBlocks[2] = (meanOf16x16SquaredValuesBlocks[8] + meanOf16x16SquaredValuesBlocks[9] + meanOf16x16SquaredValuesBlocks[12] + meanOf16x16SquaredValuesBlocks[13]) >> 2;
	meanOf32x32SquaredValuesBlocks[3] = (meanOf16x16SquaredValuesBlocks[10] + meanOf16x16SquaredValuesBlocks[11] + meanOf16x16SquaredValuesBlocks[14] + meanOf16x16SquaredValuesBlocks[15]) >> 2;


	variance32x32[0] = meanOf32x32SquaredValuesBlocks[0] - (meanOf32x32Blocks[0] * meanOf32x32Blocks[0]);
	variance32x32[1] = meanOf32x32SquaredValuesBlocks[1] - (meanOf32x32Blocks[1] * meanOf32x32Blocks[1]);
	variance32x32[2] = meanOf32x32SquaredValuesBlocks[2] - (meanOf32x32Blocks[2] * meanOf32x32Blocks[2]);
	variance32x32[3] = meanOf32x32SquaredValuesBlocks[3] - (meanOf32x32Blocks[3] * meanOf32x32Blocks[3]);


	// 64x64
	meanOf64x64Blocks = (meanOf32x32Blocks[0] + meanOf32x32Blocks[1] + meanOf32x32Blocks[2] + meanOf32x32Blocks[3]) >> 2;
	meanOf64x64SquaredValuesBlocks = (meanOf32x32SquaredValuesBlocks[0] + meanOf32x32SquaredValuesBlocks[1] + meanOf32x32SquaredValuesBlocks[2] + meanOf32x32SquaredValuesBlocks[3]) >> 2;

	return (meanOf64x64SquaredValuesBlocks - (meanOf64x64Blocks * meanOf64x64Blocks));
}



static EB_U8  getFilteredTypes(EB_U8  *ptr,
	EB_U32  stride,
	EB_U8   filterType)
{
	EB_U8 *p = ptr - 1 - stride;

	EB_U32 a = 0;

	if (filterType == 0){

		//Luma
		a = (p[1] +
			p[0 + stride] + 4 * p[1 + stride] + p[2 + stride] +
			p[1 + 2 * stride]) / 8;

	}
	else if (filterType == 1){
        a = (                    2 * p[1] +
			 2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
			                     2 * p[1 + 2 * stride]  );  
        
        a =  (( (EB_U32)((a *2730) >> 14) + 1) >> 1) & 0xFFFF;

        //fixed point version of a=a/12 to mimic x86 instruction _mm256_mulhrs_epi16;
        //a= (a*2730)>>15;
	}
	else if (filterType == 2){


		a = (4 * p[1] +
			4 * p[0 + stride] + 4 * p[1 + stride] + 4 * p[2 + stride] +
			4 * p[1 + 2 * stride]) / 20;
	}
	else if (filterType == 3){

		a = (1 * p[0] + 1 * p[1] + 1 * p[2] +
			1 * p[0 + stride] + 4 * p[1 + stride] + 1 * p[2 + stride] +
			1 * p[0 + 2 * stride] + 1 * p[1 + 2 * stride] + 1 * p[2 + 2 * stride]) / 12;


	}
	else if (filterType == 4){

		//gaussian matrix(Chroma)
		a = (1 * p[0] + 2 * p[1] + 1 * p[2] +
			2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
			1 * p[0 + 2 * stride] + 2 * p[1 + 2 * stride] + 1 * p[2 + 2 * stride]) / 16;

	}
	else if (filterType == 5){

		a = (2 * p[0] + 2 * p[1] + 2 * p[2] +
			2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
			2 * p[0 + 2 * stride] + 2 * p[1 + 2 * stride] + 2 * p[2 + 2 * stride]) / 20;

	}
	else if (filterType == 6){

		a = (4 * p[0] + 4 * p[1] + 4 * p[2] +
			4 * p[0 + stride] + 4 * p[1 + stride] + 4 * p[2 + stride] +
			4 * p[0 + 2 * stride] + 4 * p[1 + 2 * stride] + 4 * p[2 + 2 * stride]) / 36;

	}

	return  (EB_U8)CLIP3EQ(0, 255, a);
}


/*******************************************
* noiseExtractLumaStrong
*  strong filter Luma.
*******************************************/
void noiseExtractLumaStrong(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32                       lcuOriginY
	, EB_U32                       lcuOriginX
	)
{
	EB_U32  ii, jj;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised;

	EB_U32 strideOut;
	EB_U32 idx = (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width) ? lcuOriginX : 0;

	//Luma
	{
		picHeight = inputPicturePtr->height;
		picWidth = inputPicturePtr->width;
		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);

		strideIn = inputPicturePtr->strideY;
		inputOriginIndex = inputPicturePtr->originX + (inputPicturePtr->originY + lcuOriginY)* inputPicturePtr->strideY;
		ptrIn = &(inputPicturePtr->bufferY[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;
		strideOut = denoisedPicturePtr->strideY;
		ptrDenoised = &(denoisedPicturePtr->bufferY[inputOriginIndexPad]);

		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < picWidth; ii++){

				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || lcuOriginY + lcuHeight < picHeight) && ii>0 && ii < picWidth - 1){

					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 4);

				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];

				}

			}
		}
	}

}

/*******************************************
* noiseExtractChromaStrong
*  strong filter chroma.
*******************************************/
void noiseExtractChromaStrong(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32                       lcuOriginY
	, EB_U32                       lcuOriginX
	)
{
	EB_U32  ii, jj;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised;

	EB_U32 strideOut;
	EB_U32 idx = (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width) ? lcuOriginX : 0;

    EB_U32 colorFormat      = inputPicturePtr->colorFormat;
    EB_U16 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;


	//Cb
	{
		picHeight = inputPicturePtr->height >> subHeightCMinus1;
		picWidth = inputPicturePtr->width >> subWidthCMinus1;
		lcuHeight = MIN(MAX_LCU_SIZE >> subHeightCMinus1, picHeight - lcuOriginY);

		strideIn = inputPicturePtr->strideCb;
		inputOriginIndex = (inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)  * inputPicturePtr->strideCb;
		ptrIn = &(inputPicturePtr->bufferCb[inputOriginIndex]);

		inputOriginIndexPad = (denoisedPicturePtr->originX >> subWidthCMinus1) + ((denoisedPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)  * denoisedPicturePtr->strideCb;
		strideOut = denoisedPicturePtr->strideCb;
		ptrDenoised = &(denoisedPicturePtr->bufferCb[inputOriginIndexPad]);


		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < picWidth; ii++){


				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || (lcuOriginY + lcuHeight) < picHeight) && ii > 0 && ii < picWidth - 1){
					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 6);
				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
				}

			}
		}
	}

	//Cr
	{
		picHeight = inputPicturePtr->height >> subHeightCMinus1;
		picWidth = inputPicturePtr->width >> subWidthCMinus1;
		lcuHeight = MIN(MAX_LCU_SIZE >> subHeightCMinus1, picHeight - lcuOriginY);

		strideIn = inputPicturePtr->strideCr;
		inputOriginIndex = (inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)  * inputPicturePtr->strideCr;
		ptrIn = &(inputPicturePtr->bufferCr[inputOriginIndex]);

		inputOriginIndexPad = (denoisedPicturePtr->originX >> subWidthCMinus1) + ((denoisedPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)  * denoisedPicturePtr->strideCr;
		strideOut = denoisedPicturePtr->strideCr;
		ptrDenoised = &(denoisedPicturePtr->bufferCr[inputOriginIndexPad]);


		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < picWidth; ii++){

				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || (lcuOriginY + lcuHeight) < picHeight) && ii > 0 && ii < picWidth - 1){
					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 6);
				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
				}

			}
		}
	}
}

/*******************************************
* noiseExtractChromaWeak
*  weak filter chroma.
*******************************************/
void noiseExtractChromaWeak(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32                       lcuOriginY
	, EB_U32                       lcuOriginX
	)
{
	EB_U32  ii, jj;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised;

	EB_U32 strideOut;

	EB_U32 idx = (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width) ? lcuOriginX : 0;

    EB_U32 colorFormat      = inputPicturePtr->colorFormat;
    EB_U16 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;


	//Cb
	{
		picHeight = inputPicturePtr->height >> subHeightCMinus1;
		picWidth = inputPicturePtr->width >> subWidthCMinus1;

		lcuHeight = MIN(MAX_LCU_SIZE >> subHeightCMinus1, picHeight - lcuOriginY);

		strideIn = inputPicturePtr->strideCb;
		inputOriginIndex = (inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)* inputPicturePtr->strideCb;
		ptrIn = &(inputPicturePtr->bufferCb[inputOriginIndex]);

		inputOriginIndexPad = (denoisedPicturePtr->originX >> subWidthCMinus1) + ((denoisedPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)* denoisedPicturePtr->strideCb;
		strideOut = denoisedPicturePtr->strideCb;
		ptrDenoised = &(denoisedPicturePtr->bufferCb[inputOriginIndexPad]);


		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < picWidth; ii++){


				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || (lcuOriginY + lcuHeight) < picHeight) && ii > 0 && ii < picWidth - 1){
					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 4);
				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
				}

			}
		}
	}

	//Cr
	{
		picHeight = inputPicturePtr->height >> subHeightCMinus1;
		picWidth = inputPicturePtr->width >> subWidthCMinus1;
		lcuHeight = MIN(MAX_LCU_SIZE >> subHeightCMinus1, picHeight - lcuOriginY);

		strideIn = inputPicturePtr->strideCr;
		inputOriginIndex = (inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)* inputPicturePtr->strideCr;
		ptrIn = &(inputPicturePtr->bufferCr[inputOriginIndex]);

		inputOriginIndexPad = (denoisedPicturePtr->originX >> subWidthCMinus1) + ((denoisedPicturePtr->originY >> subHeightCMinus1) + lcuOriginY)* denoisedPicturePtr->strideCr;
		strideOut = denoisedPicturePtr->strideCr;
		ptrDenoised = &(denoisedPicturePtr->bufferCr[inputOriginIndexPad]);


		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < picWidth; ii++){

				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || (lcuOriginY + lcuHeight) < picHeight) && ii > 0 && ii < picWidth - 1){
					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 4);
				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
				}

			}
		}
	}

}

/*******************************************
* noiseExtractLumaWeak
*  weak filter Luma and store noise.
*******************************************/
void noiseExtractLumaWeak(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EB_U32                       lcuOriginY
	, EB_U32						 lcuOriginX
	)
{
	EB_U32  ii, jj;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;
	EB_U32  noiseOriginIndex;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised;

	EB_U8 *ptrNoise;
	EB_U32 strideOut;

	EB_U32 idx = (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width) ? lcuOriginX : 0;

	//Luma
	{
		picHeight = inputPicturePtr->height;
		picWidth = inputPicturePtr->width;
		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);

		strideIn = inputPicturePtr->strideY;
		inputOriginIndex = inputPicturePtr->originX + (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY;
		ptrIn = &(inputPicturePtr->bufferY[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;
		strideOut = denoisedPicturePtr->strideY;
		ptrDenoised = &(denoisedPicturePtr->bufferY[inputOriginIndexPad]);

		noiseOriginIndex = noisePicturePtr->originX + noisePicturePtr->originY * noisePicturePtr->strideY;
		ptrNoise = &(noisePicturePtr->bufferY[noiseOriginIndex]);


		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < picWidth; ii++){

				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || lcuOriginY + lcuHeight < picHeight) && ii>0 && ii < picWidth - 1){

					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 0);
					ptrNoise[ii + jj*strideOut] = CLIP3EQ(0, 255, ptrIn[ii + jj*strideIn] - ptrDenoised[ii + jj*strideOut]);

				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
					ptrNoise[ii + jj*strideOut] = 0;
				}

			}
		}
	}

}

void noiseExtractLumaWeakLcu(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EB_U32                       lcuOriginY
	, EB_U32						 lcuOriginX
	)
{
	EB_U32  ii, jj;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth, lcuWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;
	EB_U32  noiseOriginIndex;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised;

	EB_U8 *ptrNoise;
	EB_U32 strideOut;

	EB_U32 idx = (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width) ? lcuOriginX : 0;

	//Luma
	{
		picHeight = inputPicturePtr->height;
		picWidth = inputPicturePtr->width;
		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);
		lcuWidth = MIN(MAX_LCU_SIZE, picWidth - lcuOriginX);

		strideIn = inputPicturePtr->strideY;
		inputOriginIndex = inputPicturePtr->originX + lcuOriginX + (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY;
		ptrIn = &(inputPicturePtr->bufferY[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX + lcuOriginX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;
		strideOut = denoisedPicturePtr->strideY;
		ptrDenoised = &(denoisedPicturePtr->bufferY[inputOriginIndexPad]);

		noiseOriginIndex = noisePicturePtr->originX + lcuOriginX + noisePicturePtr->originY * noisePicturePtr->strideY;
		ptrNoise = &(noisePicturePtr->bufferY[noiseOriginIndex]);


		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = idx; ii < lcuWidth; ii++){

				if ((jj>0 || lcuOriginY > 0) && (jj < lcuHeight - 1 || lcuOriginY + lcuHeight < picHeight) && (ii>0 || lcuOriginX>0) && (ii + lcuOriginX) < picWidth - 1/* & ii < lcuWidth - 1*/){

					ptrDenoised[ii + jj*strideOut] = getFilteredTypes(&ptrIn[ii + jj*strideIn], strideIn, 0);
					ptrNoise[ii + jj*strideOut] = CLIP3EQ(0, 255, ptrIn[ii + jj*strideIn] - ptrDenoised[ii + jj*strideOut]);

				}
				else{
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
					ptrNoise[ii + jj*strideOut] = 0;
				}

			}
		}
	}

}

static EB_ERRORTYPE ZeroOutChromaBlockMean(
	PictureParentControlSet_t   *pictureControlSetPtr,          // input parameter, Picture Control Set Ptr
	EB_U32                       lcuCodingOrder                // input parameter, LCU address
	)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;
	// 16x16 mean	  
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_0] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_1] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_2] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_3] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_4] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_5] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_6] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_7] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_8] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_9] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_10] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_11] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_12] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_13] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_14] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_15] = 0;

	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_0] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_1] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_2] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_3] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_4] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_5] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_6] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_7] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_8] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_9] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_10] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_11] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_12] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_13] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_14] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_15] = 0;

	// 32x32 mean
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_0] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_1] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_2] = 0;
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_3] = 0;

	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_0] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_1] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_2] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_3] = 0;

	// 64x64 mean
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_64x64] = 0;
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_64x64] = 0;

	return return_error;

}

/*******************************************
* ComputeChromaBlockMean
*   computes the chroma block mean for 64x64, 32x32 and 16x16 CUs inside the tree block
*******************************************/
static EB_ERRORTYPE ComputeChromaBlockMean(
	PictureParentControlSet_t   *pictureControlSetPtr,          // input parameter, Picture Control Set Ptr
	EbPictureBufferDesc_t       *inputPaddedPicturePtr,         // input parameter, Input Padded Picture
	EB_U32                       lcuCodingOrder,                // input parameter, LCU address
	EB_U32                       inputCbOriginIndex,            // input parameter, LCU index, used to point to source/reference samples
	EB_U32                       inputCrOriginIndex)            // input parameter, LCU index, used to point to source/reference samples
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 cbBlockIndex, crBlockIndex;

	EB_U64 cbMeanOf16x16Blocks[16];
	EB_U64 crMeanOf16x16Blocks[16];

	EB_U64 cbMeanOf32x32Blocks[4];
	EB_U64 crMeanOf32x32Blocks[4];

	EB_U64 cbMeanOf64x64Blocks;
	EB_U64 crMeanOf64x64Blocks;


	// (0,0) 16x16 block
	cbBlockIndex = inputCbOriginIndex;
	crBlockIndex = inputCrOriginIndex;

	const EB_U16 strideCb = inputPaddedPicturePtr->strideCb;
	const EB_U16 strideCr = inputPaddedPicturePtr->strideCr;

	cbMeanOf16x16Blocks[0] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[0] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (0,1)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[1] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[1] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (0,2)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[2] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[2] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (0,3)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[3] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[3] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (1,0)
	cbBlockIndex = inputCbOriginIndex + (strideCb << 3);
	crBlockIndex = inputCrOriginIndex + (strideCr << 3);
	cbMeanOf16x16Blocks[4] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[4] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (1,1)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[5] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[5] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (1,2)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[6] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[6] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (1,3)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[7] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[7] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (2,0)
	cbBlockIndex = inputCbOriginIndex + (strideCb << 4);
	crBlockIndex = inputCrOriginIndex + (strideCr << 4);
	cbMeanOf16x16Blocks[8] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[8] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (2,1)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[9] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[9] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (2,2)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[10] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[10] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (2,3)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[11] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[11] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (3,0)
	cbBlockIndex = inputCbOriginIndex + (strideCb * 24);
	crBlockIndex = inputCrOriginIndex + (strideCr * 24);
	cbMeanOf16x16Blocks[12] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[12] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (3,1)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[13] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[13] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (3,2)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[14] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[14] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);

	// (3,3)
	cbBlockIndex = cbBlockIndex + 8;
	crBlockIndex = crBlockIndex + 8;
	cbMeanOf16x16Blocks[15] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCb[cbBlockIndex]), strideCb);
	crMeanOf16x16Blocks[15] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferCr[crBlockIndex]), strideCr);


	// 32x32
	cbMeanOf32x32Blocks[0] = (cbMeanOf16x16Blocks[0] + cbMeanOf16x16Blocks[1] + cbMeanOf16x16Blocks[4] + cbMeanOf16x16Blocks[5]) >> 2;
	crMeanOf32x32Blocks[0] = (crMeanOf16x16Blocks[0] + crMeanOf16x16Blocks[1] + crMeanOf16x16Blocks[4] + crMeanOf16x16Blocks[5]) >> 2;

	cbMeanOf32x32Blocks[1] = (cbMeanOf16x16Blocks[2] + cbMeanOf16x16Blocks[3] + cbMeanOf16x16Blocks[6] + cbMeanOf16x16Blocks[7]) >> 2;
	crMeanOf32x32Blocks[1] = (crMeanOf16x16Blocks[2] + crMeanOf16x16Blocks[3] + crMeanOf16x16Blocks[6] + crMeanOf16x16Blocks[7]) >> 2;


	cbMeanOf32x32Blocks[2] = (cbMeanOf16x16Blocks[8] + cbMeanOf16x16Blocks[9] + cbMeanOf16x16Blocks[12] + cbMeanOf16x16Blocks[13]) >> 2;
	crMeanOf32x32Blocks[2] = (crMeanOf16x16Blocks[8] + crMeanOf16x16Blocks[9] + crMeanOf16x16Blocks[12] + crMeanOf16x16Blocks[13]) >> 2;

	cbMeanOf32x32Blocks[3] = (cbMeanOf16x16Blocks[10] + cbMeanOf16x16Blocks[11] + cbMeanOf16x16Blocks[14] + cbMeanOf16x16Blocks[15]) >> 2;
	crMeanOf32x32Blocks[3] = (crMeanOf16x16Blocks[10] + crMeanOf16x16Blocks[11] + crMeanOf16x16Blocks[14] + crMeanOf16x16Blocks[15]) >> 2;

	// 64x64
	cbMeanOf64x64Blocks = (cbMeanOf32x32Blocks[0] + cbMeanOf32x32Blocks[1] + cbMeanOf32x32Blocks[3] + cbMeanOf32x32Blocks[3]) >> 2;
	crMeanOf64x64Blocks = (crMeanOf32x32Blocks[0] + crMeanOf32x32Blocks[1] + crMeanOf32x32Blocks[3] + crMeanOf32x32Blocks[3]) >> 2;
	// 16x16 mean	  
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_0] = (EB_U8) (cbMeanOf16x16Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_1] = (EB_U8) (cbMeanOf16x16Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_2] = (EB_U8) (cbMeanOf16x16Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_3] = (EB_U8) (cbMeanOf16x16Blocks[3] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_4] = (EB_U8) (cbMeanOf16x16Blocks[4] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_5] = (EB_U8) (cbMeanOf16x16Blocks[5] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_6] = (EB_U8) (cbMeanOf16x16Blocks[6] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_7] = (EB_U8) (cbMeanOf16x16Blocks[7] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_8] = (EB_U8) (cbMeanOf16x16Blocks[8] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_9] = (EB_U8) (cbMeanOf16x16Blocks[9] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_10] = (EB_U8) (cbMeanOf16x16Blocks[10] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_11] = (EB_U8) (cbMeanOf16x16Blocks[11] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_12] = (EB_U8) (cbMeanOf16x16Blocks[12] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_13] = (EB_U8) (cbMeanOf16x16Blocks[13] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_14] = (EB_U8) (cbMeanOf16x16Blocks[14] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_15] = (EB_U8) (cbMeanOf16x16Blocks[15] >> MEAN_PRECISION);

	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_0] = (EB_U8) (crMeanOf16x16Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_1] = (EB_U8) (crMeanOf16x16Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_2] = (EB_U8) (crMeanOf16x16Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_3] = (EB_U8) (crMeanOf16x16Blocks[3] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_4] = (EB_U8) (crMeanOf16x16Blocks[4] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_5] = (EB_U8) (crMeanOf16x16Blocks[5] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_6] = (EB_U8) (crMeanOf16x16Blocks[6] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_7] = (EB_U8) (crMeanOf16x16Blocks[7] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_8] = (EB_U8) (crMeanOf16x16Blocks[8] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_9] = (EB_U8) (crMeanOf16x16Blocks[9] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_10] = (EB_U8) (crMeanOf16x16Blocks[10] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_11] = (EB_U8) (crMeanOf16x16Blocks[11] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_12] = (EB_U8) (crMeanOf16x16Blocks[12] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_13] = (EB_U8) (crMeanOf16x16Blocks[13] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_14] = (EB_U8) (crMeanOf16x16Blocks[14] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_16x16_15] = (EB_U8) (crMeanOf16x16Blocks[15] >> MEAN_PRECISION);

	// 32x32 mean
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_0] = (EB_U8) (cbMeanOf32x32Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_1] = (EB_U8) (cbMeanOf32x32Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_2] = (EB_U8) (cbMeanOf32x32Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_3] = (EB_U8) (cbMeanOf32x32Blocks[3] >> MEAN_PRECISION);

	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_0] = (EB_U8)(crMeanOf32x32Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_1] = (EB_U8)(crMeanOf32x32Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_2] = (EB_U8)(crMeanOf32x32Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_32x32_3] = (EB_U8)(crMeanOf32x32Blocks[3] >> MEAN_PRECISION);

	// 64x64 mean
	pictureControlSetPtr->cbMean[lcuCodingOrder][ME_TIER_ZERO_PU_64x64] = (EB_U8) (cbMeanOf64x64Blocks >> MEAN_PRECISION);
	pictureControlSetPtr->crMean[lcuCodingOrder][ME_TIER_ZERO_PU_64x64] = (EB_U8) (crMeanOf64x64Blocks >> MEAN_PRECISION);

	return return_error;
}


/*******************************************
* ComputeBlockMeanComputeVariance
*   computes the variance and the block mean of all CUs inside the tree block
*******************************************/
static EB_ERRORTYPE ComputeBlockMeanComputeVariance(
    PictureParentControlSet_t   *pictureControlSetPtr,          // input parameter, Picture Control Set Ptr
    EbPictureBufferDesc_t       *inputPaddedPicturePtr,         // input parameter, Input Padded Picture
    EB_U32                       lcuIndex,                      // input parameter, LCU address
    EB_U32                       inputLumaOriginIndex)          // input parameter, LCU index, used to point to source/reference samples
{

    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U32 blockIndex;

    EB_U64 meanOf8x8Blocks[64];
    EB_U64 meanOf8x8SquaredValuesBlocks[64];

    EB_U64 meanOf16x16Blocks[16];
    EB_U64 meanOf16x16SquaredValuesBlocks[16];

    EB_U64 meanOf32x32Blocks[4];
    EB_U64 meanOf32x32SquaredValuesBlocks[4];

    EB_U64 meanOf64x64Blocks;
    EB_U64 meanOf64x64SquaredValuesBlocks;

	if (pictureControlSetPtr->disableVarianceFlag) {
		memset16bit(pictureControlSetPtr->variance[lcuIndex], 125, MAX_ME_PU_COUNT);
		EB_MEMSET(pictureControlSetPtr->yMean[lcuIndex], 125, sizeof(EB_U8) * MAX_ME_PU_COUNT);

	}
	else {

		// (0,0)
		blockIndex = inputLumaOriginIndex;

    const EB_U16 strideY = inputPaddedPicturePtr->strideY;

    if (!!(ASM_TYPES & AVX2_MASK)){

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[0], &meanOf8x8SquaredValuesBlocks[0]);

        // (0,1)
        blockIndex = blockIndex + 32;


        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[4], &meanOf8x8SquaredValuesBlocks[4]);

        // (0,5)
        blockIndex = blockIndex + 24;

        // (1,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[8], &meanOf8x8SquaredValuesBlocks[8]);

        // (1,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[12], &meanOf8x8SquaredValuesBlocks[12]);

        // (1,5)
        blockIndex = blockIndex + 24;

        // (2,0)
        blockIndex = inputLumaOriginIndex + (strideY << 4);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[16], &meanOf8x8SquaredValuesBlocks[16]);

        // (2,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[20], &meanOf8x8SquaredValuesBlocks[20]);

        // (2,5)
        blockIndex = blockIndex + 24;

        // (3,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4);


        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[24], &meanOf8x8SquaredValuesBlocks[24]);

        // (3,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[28], &meanOf8x8SquaredValuesBlocks[28]);

        // (3,5)
        blockIndex = blockIndex + 24;

        // (4,0)
        blockIndex = inputLumaOriginIndex + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[32], &meanOf8x8SquaredValuesBlocks[32]);

        // (4,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[36], &meanOf8x8SquaredValuesBlocks[36]);

        // (4,5)
        blockIndex = blockIndex + 24;

        // (5,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[40], &meanOf8x8SquaredValuesBlocks[40]);

        // (5,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[44], &meanOf8x8SquaredValuesBlocks[44]);

        // (5,5)
        blockIndex = blockIndex + 24;

        // (6,0)
        blockIndex = inputLumaOriginIndex + (strideY << 4) + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[48], &meanOf8x8SquaredValuesBlocks[48]);

        // (6,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[52], &meanOf8x8SquaredValuesBlocks[52]);

        // (6,5)
        blockIndex = blockIndex + 24;


        // (7,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4) + (strideY << 5);

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[56], &meanOf8x8SquaredValuesBlocks[56]);


        // (7,1)
        blockIndex = blockIndex + 32;

        ComputeIntermVarFour8x8_AVX2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY, &meanOf8x8Blocks[60], &meanOf8x8SquaredValuesBlocks[60]);

       
    }
    else{
        meanOf8x8Blocks[0] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[0] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[1] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[1] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[2] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[2] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[3] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[3] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[4] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[4] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[5] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[5] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[6] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[6] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (0,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[7] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[7] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3);
        meanOf8x8Blocks[8] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[8] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[9] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[9] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[10] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[10] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[11] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[11] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[12] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[12] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[13] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[13] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[14] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[14] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (1,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[15] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[15] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,0)
        blockIndex = inputLumaOriginIndex + (strideY << 4);
        meanOf8x8Blocks[16] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[16] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[17] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[17] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[18] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[18] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[19] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[19] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        /// (2,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[20] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[20] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[21] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[21] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[22] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[22] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (2,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[23] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[23] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4);
        meanOf8x8Blocks[24] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[24] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[25] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[25] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[26] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[26] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[27] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[27] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[28] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[28] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[29] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[29] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[30] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[30] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (3,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[31] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[31] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,0)
        blockIndex = inputLumaOriginIndex + (strideY << 5);
        meanOf8x8Blocks[32] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[32] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[33] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[33] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[34] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[34] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[35] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[35] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[36] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[36] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[37] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[37] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[38] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[38] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (4,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[39] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[39] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 5);
        meanOf8x8Blocks[40] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[40] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[41] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[41] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[42] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[42] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[43] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[43] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[44] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[44] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[45] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[45] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[46] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[46] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (5,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[47] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[47] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,0)
        blockIndex = inputLumaOriginIndex + (strideY << 4) + (strideY << 5);
        meanOf8x8Blocks[48] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[48] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[49] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[49] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[50] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[50] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[51] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[51] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[52] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[52] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[53] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[53] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[54] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[54] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (6,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[55] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[55] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,0)
        blockIndex = inputLumaOriginIndex + (strideY << 3) + (strideY << 4) + (strideY << 5);
        meanOf8x8Blocks[56] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[56] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,1)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[57] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[57] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,2)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[58] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[58] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,3)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[59] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[59] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,4)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[60] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[60] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,5)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[61] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[61] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,6)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[62] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[62] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);

        // (7,7)
        blockIndex = blockIndex + 8;
        meanOf8x8Blocks[63] = ComputeSubMean8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
        meanOf8x8SquaredValuesBlocks[63] = ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(&(inputPaddedPicturePtr->bufferY[blockIndex]), strideY);
    }


	// 16x16
	meanOf16x16Blocks[0] = (meanOf8x8Blocks[0] + meanOf8x8Blocks[1] + meanOf8x8Blocks[8] + meanOf8x8Blocks[9]) >> 2;
	meanOf16x16Blocks[1] = (meanOf8x8Blocks[2] + meanOf8x8Blocks[3] + meanOf8x8Blocks[10] + meanOf8x8Blocks[11]) >> 2;
	meanOf16x16Blocks[2] = (meanOf8x8Blocks[4] + meanOf8x8Blocks[5] + meanOf8x8Blocks[12] + meanOf8x8Blocks[13]) >> 2;
	meanOf16x16Blocks[3] = (meanOf8x8Blocks[6] + meanOf8x8Blocks[7] + meanOf8x8Blocks[14] + meanOf8x8Blocks[15]) >> 2;

	meanOf16x16Blocks[4] = (meanOf8x8Blocks[16] + meanOf8x8Blocks[17] + meanOf8x8Blocks[24] + meanOf8x8Blocks[25]) >> 2;
	meanOf16x16Blocks[5] = (meanOf8x8Blocks[18] + meanOf8x8Blocks[19] + meanOf8x8Blocks[26] + meanOf8x8Blocks[27]) >> 2;
	meanOf16x16Blocks[6] = (meanOf8x8Blocks[20] + meanOf8x8Blocks[21] + meanOf8x8Blocks[28] + meanOf8x8Blocks[29]) >> 2;
	meanOf16x16Blocks[7] = (meanOf8x8Blocks[22] + meanOf8x8Blocks[23] + meanOf8x8Blocks[30] + meanOf8x8Blocks[31]) >> 2;

	meanOf16x16Blocks[8] = (meanOf8x8Blocks[32] + meanOf8x8Blocks[33] + meanOf8x8Blocks[40] + meanOf8x8Blocks[41]) >> 2;
	meanOf16x16Blocks[9] = (meanOf8x8Blocks[34] + meanOf8x8Blocks[35] + meanOf8x8Blocks[42] + meanOf8x8Blocks[43]) >> 2;
	meanOf16x16Blocks[10] = (meanOf8x8Blocks[36] + meanOf8x8Blocks[37] + meanOf8x8Blocks[44] + meanOf8x8Blocks[45]) >> 2;
	meanOf16x16Blocks[11] = (meanOf8x8Blocks[38] + meanOf8x8Blocks[39] + meanOf8x8Blocks[46] + meanOf8x8Blocks[47]) >> 2;

	meanOf16x16Blocks[12] = (meanOf8x8Blocks[48] + meanOf8x8Blocks[49] + meanOf8x8Blocks[56] + meanOf8x8Blocks[57]) >> 2;
	meanOf16x16Blocks[13] = (meanOf8x8Blocks[50] + meanOf8x8Blocks[51] + meanOf8x8Blocks[58] + meanOf8x8Blocks[59]) >> 2;
	meanOf16x16Blocks[14] = (meanOf8x8Blocks[52] + meanOf8x8Blocks[53] + meanOf8x8Blocks[60] + meanOf8x8Blocks[61]) >> 2;
	meanOf16x16Blocks[15] = (meanOf8x8Blocks[54] + meanOf8x8Blocks[55] + meanOf8x8Blocks[62] + meanOf8x8Blocks[63]) >> 2;

	meanOf16x16SquaredValuesBlocks[0] = (meanOf8x8SquaredValuesBlocks[0] + meanOf8x8SquaredValuesBlocks[1] + meanOf8x8SquaredValuesBlocks[8] + meanOf8x8SquaredValuesBlocks[9]) >> 2;
	meanOf16x16SquaredValuesBlocks[1] = (meanOf8x8SquaredValuesBlocks[2] + meanOf8x8SquaredValuesBlocks[3] + meanOf8x8SquaredValuesBlocks[10] + meanOf8x8SquaredValuesBlocks[11]) >> 2;
	meanOf16x16SquaredValuesBlocks[2] = (meanOf8x8SquaredValuesBlocks[4] + meanOf8x8SquaredValuesBlocks[5] + meanOf8x8SquaredValuesBlocks[12] + meanOf8x8SquaredValuesBlocks[13]) >> 2;
	meanOf16x16SquaredValuesBlocks[3] = (meanOf8x8SquaredValuesBlocks[6] + meanOf8x8SquaredValuesBlocks[7] + meanOf8x8SquaredValuesBlocks[14] + meanOf8x8SquaredValuesBlocks[15]) >> 2;

	meanOf16x16SquaredValuesBlocks[4] = (meanOf8x8SquaredValuesBlocks[16] + meanOf8x8SquaredValuesBlocks[17] + meanOf8x8SquaredValuesBlocks[24] + meanOf8x8SquaredValuesBlocks[25]) >> 2;
	meanOf16x16SquaredValuesBlocks[5] = (meanOf8x8SquaredValuesBlocks[18] + meanOf8x8SquaredValuesBlocks[19] + meanOf8x8SquaredValuesBlocks[26] + meanOf8x8SquaredValuesBlocks[27]) >> 2;
	meanOf16x16SquaredValuesBlocks[6] = (meanOf8x8SquaredValuesBlocks[20] + meanOf8x8SquaredValuesBlocks[21] + meanOf8x8SquaredValuesBlocks[28] + meanOf8x8SquaredValuesBlocks[29]) >> 2;
	meanOf16x16SquaredValuesBlocks[7] = (meanOf8x8SquaredValuesBlocks[22] + meanOf8x8SquaredValuesBlocks[23] + meanOf8x8SquaredValuesBlocks[30] + meanOf8x8SquaredValuesBlocks[31]) >> 2;

	meanOf16x16SquaredValuesBlocks[8] = (meanOf8x8SquaredValuesBlocks[32] + meanOf8x8SquaredValuesBlocks[33] + meanOf8x8SquaredValuesBlocks[40] + meanOf8x8SquaredValuesBlocks[41]) >> 2;
	meanOf16x16SquaredValuesBlocks[9] = (meanOf8x8SquaredValuesBlocks[34] + meanOf8x8SquaredValuesBlocks[35] + meanOf8x8SquaredValuesBlocks[42] + meanOf8x8SquaredValuesBlocks[43]) >> 2;
	meanOf16x16SquaredValuesBlocks[10] = (meanOf8x8SquaredValuesBlocks[36] + meanOf8x8SquaredValuesBlocks[37] + meanOf8x8SquaredValuesBlocks[44] + meanOf8x8SquaredValuesBlocks[45]) >> 2;
	meanOf16x16SquaredValuesBlocks[11] = (meanOf8x8SquaredValuesBlocks[38] + meanOf8x8SquaredValuesBlocks[39] + meanOf8x8SquaredValuesBlocks[46] + meanOf8x8SquaredValuesBlocks[47]) >> 2;

	meanOf16x16SquaredValuesBlocks[12] = (meanOf8x8SquaredValuesBlocks[48] + meanOf8x8SquaredValuesBlocks[49] + meanOf8x8SquaredValuesBlocks[56] + meanOf8x8SquaredValuesBlocks[57]) >> 2;
	meanOf16x16SquaredValuesBlocks[13] = (meanOf8x8SquaredValuesBlocks[50] + meanOf8x8SquaredValuesBlocks[51] + meanOf8x8SquaredValuesBlocks[58] + meanOf8x8SquaredValuesBlocks[59]) >> 2;
	meanOf16x16SquaredValuesBlocks[14] = (meanOf8x8SquaredValuesBlocks[52] + meanOf8x8SquaredValuesBlocks[53] + meanOf8x8SquaredValuesBlocks[60] + meanOf8x8SquaredValuesBlocks[61]) >> 2;
	meanOf16x16SquaredValuesBlocks[15] = (meanOf8x8SquaredValuesBlocks[54] + meanOf8x8SquaredValuesBlocks[55] + meanOf8x8SquaredValuesBlocks[62] + meanOf8x8SquaredValuesBlocks[63]) >> 2;

	// 32x32
	meanOf32x32Blocks[0] = (meanOf16x16Blocks[0] + meanOf16x16Blocks[1] + meanOf16x16Blocks[4] + meanOf16x16Blocks[5]) >> 2;
	meanOf32x32Blocks[1] = (meanOf16x16Blocks[2] + meanOf16x16Blocks[3] + meanOf16x16Blocks[6] + meanOf16x16Blocks[7]) >> 2;
	meanOf32x32Blocks[2] = (meanOf16x16Blocks[8] + meanOf16x16Blocks[9] + meanOf16x16Blocks[12] + meanOf16x16Blocks[13]) >> 2;
	meanOf32x32Blocks[3] = (meanOf16x16Blocks[10] + meanOf16x16Blocks[11] + meanOf16x16Blocks[14] + meanOf16x16Blocks[15]) >> 2;

	meanOf32x32SquaredValuesBlocks[0] = (meanOf16x16SquaredValuesBlocks[0] + meanOf16x16SquaredValuesBlocks[1] + meanOf16x16SquaredValuesBlocks[4] + meanOf16x16SquaredValuesBlocks[5]) >> 2;
	meanOf32x32SquaredValuesBlocks[1] = (meanOf16x16SquaredValuesBlocks[2] + meanOf16x16SquaredValuesBlocks[3] + meanOf16x16SquaredValuesBlocks[6] + meanOf16x16SquaredValuesBlocks[7]) >> 2;
	meanOf32x32SquaredValuesBlocks[2] = (meanOf16x16SquaredValuesBlocks[8] + meanOf16x16SquaredValuesBlocks[9] + meanOf16x16SquaredValuesBlocks[12] + meanOf16x16SquaredValuesBlocks[13]) >> 2;
	meanOf32x32SquaredValuesBlocks[3] = (meanOf16x16SquaredValuesBlocks[10] + meanOf16x16SquaredValuesBlocks[11] + meanOf16x16SquaredValuesBlocks[14] + meanOf16x16SquaredValuesBlocks[15]) >> 2;

	// 64x64
	meanOf64x64Blocks = (meanOf32x32Blocks[0] + meanOf32x32Blocks[1] + meanOf32x32Blocks[2] + meanOf32x32Blocks[3]) >> 2;
	meanOf64x64SquaredValuesBlocks = (meanOf32x32SquaredValuesBlocks[0] + meanOf32x32SquaredValuesBlocks[1] + meanOf32x32SquaredValuesBlocks[2] + meanOf32x32SquaredValuesBlocks[3]) >> 2;

	// 8x8 means
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_0] = (EB_U8)(meanOf8x8Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_1] = (EB_U8)(meanOf8x8Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_2] = (EB_U8)(meanOf8x8Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_3] = (EB_U8)(meanOf8x8Blocks[3] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_4] = (EB_U8)(meanOf8x8Blocks[4] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_5] = (EB_U8)(meanOf8x8Blocks[5] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_6] = (EB_U8)(meanOf8x8Blocks[6] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_7] = (EB_U8)(meanOf8x8Blocks[7] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_8] = (EB_U8)(meanOf8x8Blocks[8] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_9] = (EB_U8)(meanOf8x8Blocks[9] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_10] = (EB_U8)(meanOf8x8Blocks[10] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_11] = (EB_U8)(meanOf8x8Blocks[11] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_12] = (EB_U8)(meanOf8x8Blocks[12] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_13] = (EB_U8)(meanOf8x8Blocks[13] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_14] = (EB_U8)(meanOf8x8Blocks[14] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_15] = (EB_U8)(meanOf8x8Blocks[15] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_16] = (EB_U8)(meanOf8x8Blocks[16] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_17] = (EB_U8)(meanOf8x8Blocks[17] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_18] = (EB_U8)(meanOf8x8Blocks[18] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_19] = (EB_U8)(meanOf8x8Blocks[19] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_20] = (EB_U8)(meanOf8x8Blocks[20] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_21] = (EB_U8)(meanOf8x8Blocks[21] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_22] = (EB_U8)(meanOf8x8Blocks[22] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_23] = (EB_U8)(meanOf8x8Blocks[23] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_24] = (EB_U8)(meanOf8x8Blocks[24] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_25] = (EB_U8)(meanOf8x8Blocks[25] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_26] = (EB_U8)(meanOf8x8Blocks[26] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_27] = (EB_U8)(meanOf8x8Blocks[27] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_28] = (EB_U8)(meanOf8x8Blocks[28] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_29] = (EB_U8)(meanOf8x8Blocks[29] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_30] = (EB_U8)(meanOf8x8Blocks[30] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_31] = (EB_U8)(meanOf8x8Blocks[31] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_32] = (EB_U8)(meanOf8x8Blocks[32] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_33] = (EB_U8)(meanOf8x8Blocks[33] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_34] = (EB_U8)(meanOf8x8Blocks[34] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_35] = (EB_U8)(meanOf8x8Blocks[35] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_36] = (EB_U8)(meanOf8x8Blocks[36] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_37] = (EB_U8)(meanOf8x8Blocks[37] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_38] = (EB_U8)(meanOf8x8Blocks[38] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_39] = (EB_U8)(meanOf8x8Blocks[39] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_40] = (EB_U8)(meanOf8x8Blocks[40] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_41] = (EB_U8)(meanOf8x8Blocks[41] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_42] = (EB_U8)(meanOf8x8Blocks[42] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_43] = (EB_U8)(meanOf8x8Blocks[43] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_44] = (EB_U8)(meanOf8x8Blocks[44] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_45] = (EB_U8)(meanOf8x8Blocks[45] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_46] = (EB_U8)(meanOf8x8Blocks[46] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_47] = (EB_U8)(meanOf8x8Blocks[47] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_48] = (EB_U8)(meanOf8x8Blocks[48] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_49] = (EB_U8)(meanOf8x8Blocks[49] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_50] = (EB_U8)(meanOf8x8Blocks[50] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_51] = (EB_U8)(meanOf8x8Blocks[51] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_52] = (EB_U8)(meanOf8x8Blocks[52] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_53] = (EB_U8)(meanOf8x8Blocks[53] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_54] = (EB_U8)(meanOf8x8Blocks[54] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_55] = (EB_U8)(meanOf8x8Blocks[55] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_56] = (EB_U8)(meanOf8x8Blocks[56] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_57] = (EB_U8)(meanOf8x8Blocks[57] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_58] = (EB_U8)(meanOf8x8Blocks[58] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_59] = (EB_U8)(meanOf8x8Blocks[59] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_60] = (EB_U8)(meanOf8x8Blocks[60] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_61] = (EB_U8)(meanOf8x8Blocks[61] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_62] = (EB_U8)(meanOf8x8Blocks[62] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_8x8_63] = (EB_U8)(meanOf8x8Blocks[63] >> MEAN_PRECISION);

	// 16x16 mean	  
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_0] = (EB_U8)(meanOf16x16Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_1] = (EB_U8)(meanOf16x16Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_2] = (EB_U8)(meanOf16x16Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_3] = (EB_U8)(meanOf16x16Blocks[3] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_4] = (EB_U8)(meanOf16x16Blocks[4] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_5] = (EB_U8)(meanOf16x16Blocks[5] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_6] = (EB_U8)(meanOf16x16Blocks[6] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_7] = (EB_U8)(meanOf16x16Blocks[7] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_8] = (EB_U8)(meanOf16x16Blocks[8] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_9] = (EB_U8)(meanOf16x16Blocks[9] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_10] = (EB_U8)(meanOf16x16Blocks[10] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_11] = (EB_U8)(meanOf16x16Blocks[11] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_12] = (EB_U8)(meanOf16x16Blocks[12] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_13] = (EB_U8)(meanOf16x16Blocks[13] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_14] = (EB_U8)(meanOf16x16Blocks[14] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_16x16_15] = (EB_U8)(meanOf16x16Blocks[15] >> MEAN_PRECISION);

	// 32x32 mean
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_32x32_0] = (EB_U8)(meanOf32x32Blocks[0] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_32x32_1] = (EB_U8)(meanOf32x32Blocks[1] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_32x32_2] = (EB_U8)(meanOf32x32Blocks[2] >> MEAN_PRECISION);
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_32x32_3] = (EB_U8)(meanOf32x32Blocks[3] >> MEAN_PRECISION);

	// 64x64 mean
	pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_64x64] = (EB_U8)(meanOf64x64Blocks >> MEAN_PRECISION);

	// 8x8 variances
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_0] = (EB_U16)((meanOf8x8SquaredValuesBlocks[0] - (meanOf8x8Blocks[0] * meanOf8x8Blocks[0])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_1] = (EB_U16)((meanOf8x8SquaredValuesBlocks[1] - (meanOf8x8Blocks[1] * meanOf8x8Blocks[1])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_2] = (EB_U16)((meanOf8x8SquaredValuesBlocks[2] - (meanOf8x8Blocks[2] * meanOf8x8Blocks[2])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_3] = (EB_U16)((meanOf8x8SquaredValuesBlocks[3] - (meanOf8x8Blocks[3] * meanOf8x8Blocks[3])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_4] = (EB_U16)((meanOf8x8SquaredValuesBlocks[4] - (meanOf8x8Blocks[4] * meanOf8x8Blocks[4])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_5] = (EB_U16)((meanOf8x8SquaredValuesBlocks[5] - (meanOf8x8Blocks[5] * meanOf8x8Blocks[5])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_6] = (EB_U16)((meanOf8x8SquaredValuesBlocks[6] - (meanOf8x8Blocks[6] * meanOf8x8Blocks[6])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_7] = (EB_U16)((meanOf8x8SquaredValuesBlocks[7] - (meanOf8x8Blocks[7] * meanOf8x8Blocks[7])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_8] = (EB_U16)((meanOf8x8SquaredValuesBlocks[8] - (meanOf8x8Blocks[8] * meanOf8x8Blocks[8])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_9] = (EB_U16)((meanOf8x8SquaredValuesBlocks[9] - (meanOf8x8Blocks[9] * meanOf8x8Blocks[9])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_10] = (EB_U16)((meanOf8x8SquaredValuesBlocks[10] - (meanOf8x8Blocks[10] * meanOf8x8Blocks[10])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_11] = (EB_U16)((meanOf8x8SquaredValuesBlocks[11] - (meanOf8x8Blocks[11] * meanOf8x8Blocks[11])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_12] = (EB_U16)((meanOf8x8SquaredValuesBlocks[12] - (meanOf8x8Blocks[12] * meanOf8x8Blocks[12])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_13] = (EB_U16)((meanOf8x8SquaredValuesBlocks[13] - (meanOf8x8Blocks[13] * meanOf8x8Blocks[13])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_14] = (EB_U16)((meanOf8x8SquaredValuesBlocks[14] - (meanOf8x8Blocks[14] * meanOf8x8Blocks[14])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_15] = (EB_U16)((meanOf8x8SquaredValuesBlocks[15] - (meanOf8x8Blocks[15] * meanOf8x8Blocks[15])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_16] = (EB_U16)((meanOf8x8SquaredValuesBlocks[16] - (meanOf8x8Blocks[16] * meanOf8x8Blocks[16])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_17] = (EB_U16)((meanOf8x8SquaredValuesBlocks[17] - (meanOf8x8Blocks[17] * meanOf8x8Blocks[17])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_18] = (EB_U16)((meanOf8x8SquaredValuesBlocks[18] - (meanOf8x8Blocks[18] * meanOf8x8Blocks[18])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_19] = (EB_U16)((meanOf8x8SquaredValuesBlocks[19] - (meanOf8x8Blocks[19] * meanOf8x8Blocks[19])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_20] = (EB_U16)((meanOf8x8SquaredValuesBlocks[20] - (meanOf8x8Blocks[20] * meanOf8x8Blocks[20])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_21] = (EB_U16)((meanOf8x8SquaredValuesBlocks[21] - (meanOf8x8Blocks[21] * meanOf8x8Blocks[21])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_22] = (EB_U16)((meanOf8x8SquaredValuesBlocks[22] - (meanOf8x8Blocks[22] * meanOf8x8Blocks[22])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_23] = (EB_U16)((meanOf8x8SquaredValuesBlocks[23] - (meanOf8x8Blocks[23] * meanOf8x8Blocks[23])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_24] = (EB_U16)((meanOf8x8SquaredValuesBlocks[24] - (meanOf8x8Blocks[24] * meanOf8x8Blocks[24])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_25] = (EB_U16)((meanOf8x8SquaredValuesBlocks[25] - (meanOf8x8Blocks[25] * meanOf8x8Blocks[25])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_26] = (EB_U16)((meanOf8x8SquaredValuesBlocks[26] - (meanOf8x8Blocks[26] * meanOf8x8Blocks[26])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_27] = (EB_U16)((meanOf8x8SquaredValuesBlocks[27] - (meanOf8x8Blocks[27] * meanOf8x8Blocks[27])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_28] = (EB_U16)((meanOf8x8SquaredValuesBlocks[28] - (meanOf8x8Blocks[28] * meanOf8x8Blocks[28])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_29] = (EB_U16)((meanOf8x8SquaredValuesBlocks[29] - (meanOf8x8Blocks[29] * meanOf8x8Blocks[29])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_30] = (EB_U16)((meanOf8x8SquaredValuesBlocks[30] - (meanOf8x8Blocks[30] * meanOf8x8Blocks[30])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_31] = (EB_U16)((meanOf8x8SquaredValuesBlocks[31] - (meanOf8x8Blocks[31] * meanOf8x8Blocks[31])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_32] = (EB_U16)((meanOf8x8SquaredValuesBlocks[32] - (meanOf8x8Blocks[32] * meanOf8x8Blocks[32])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_33] = (EB_U16)((meanOf8x8SquaredValuesBlocks[33] - (meanOf8x8Blocks[33] * meanOf8x8Blocks[33])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_34] = (EB_U16)((meanOf8x8SquaredValuesBlocks[34] - (meanOf8x8Blocks[34] * meanOf8x8Blocks[34])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_35] = (EB_U16)((meanOf8x8SquaredValuesBlocks[35] - (meanOf8x8Blocks[35] * meanOf8x8Blocks[35])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_36] = (EB_U16)((meanOf8x8SquaredValuesBlocks[36] - (meanOf8x8Blocks[36] * meanOf8x8Blocks[36])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_37] = (EB_U16)((meanOf8x8SquaredValuesBlocks[37] - (meanOf8x8Blocks[37] * meanOf8x8Blocks[37])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_38] = (EB_U16)((meanOf8x8SquaredValuesBlocks[38] - (meanOf8x8Blocks[38] * meanOf8x8Blocks[38])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_39] = (EB_U16)((meanOf8x8SquaredValuesBlocks[39] - (meanOf8x8Blocks[39] * meanOf8x8Blocks[39])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_40] = (EB_U16)((meanOf8x8SquaredValuesBlocks[40] - (meanOf8x8Blocks[40] * meanOf8x8Blocks[40])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_41] = (EB_U16)((meanOf8x8SquaredValuesBlocks[41] - (meanOf8x8Blocks[41] * meanOf8x8Blocks[41])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_42] = (EB_U16)((meanOf8x8SquaredValuesBlocks[42] - (meanOf8x8Blocks[42] * meanOf8x8Blocks[42])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_43] = (EB_U16)((meanOf8x8SquaredValuesBlocks[43] - (meanOf8x8Blocks[43] * meanOf8x8Blocks[43])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_44] = (EB_U16)((meanOf8x8SquaredValuesBlocks[44] - (meanOf8x8Blocks[44] * meanOf8x8Blocks[44])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_45] = (EB_U16)((meanOf8x8SquaredValuesBlocks[45] - (meanOf8x8Blocks[45] * meanOf8x8Blocks[45])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_46] = (EB_U16)((meanOf8x8SquaredValuesBlocks[46] - (meanOf8x8Blocks[46] * meanOf8x8Blocks[46])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_47] = (EB_U16)((meanOf8x8SquaredValuesBlocks[47] - (meanOf8x8Blocks[47] * meanOf8x8Blocks[47])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_48] = (EB_U16)((meanOf8x8SquaredValuesBlocks[48] - (meanOf8x8Blocks[48] * meanOf8x8Blocks[48])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_49] = (EB_U16)((meanOf8x8SquaredValuesBlocks[49] - (meanOf8x8Blocks[49] * meanOf8x8Blocks[49])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_50] = (EB_U16)((meanOf8x8SquaredValuesBlocks[50] - (meanOf8x8Blocks[50] * meanOf8x8Blocks[50])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_51] = (EB_U16)((meanOf8x8SquaredValuesBlocks[51] - (meanOf8x8Blocks[51] * meanOf8x8Blocks[51])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_52] = (EB_U16)((meanOf8x8SquaredValuesBlocks[52] - (meanOf8x8Blocks[52] * meanOf8x8Blocks[52])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_53] = (EB_U16)((meanOf8x8SquaredValuesBlocks[53] - (meanOf8x8Blocks[53] * meanOf8x8Blocks[53])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_54] = (EB_U16)((meanOf8x8SquaredValuesBlocks[54] - (meanOf8x8Blocks[54] * meanOf8x8Blocks[54])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_55] = (EB_U16)((meanOf8x8SquaredValuesBlocks[55] - (meanOf8x8Blocks[55] * meanOf8x8Blocks[55])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_56] = (EB_U16)((meanOf8x8SquaredValuesBlocks[56] - (meanOf8x8Blocks[56] * meanOf8x8Blocks[56])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_57] = (EB_U16)((meanOf8x8SquaredValuesBlocks[57] - (meanOf8x8Blocks[57] * meanOf8x8Blocks[57])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_58] = (EB_U16)((meanOf8x8SquaredValuesBlocks[58] - (meanOf8x8Blocks[58] * meanOf8x8Blocks[58])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_59] = (EB_U16)((meanOf8x8SquaredValuesBlocks[59] - (meanOf8x8Blocks[59] * meanOf8x8Blocks[59])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_60] = (EB_U16)((meanOf8x8SquaredValuesBlocks[60] - (meanOf8x8Blocks[60] * meanOf8x8Blocks[60])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_61] = (EB_U16)((meanOf8x8SquaredValuesBlocks[61] - (meanOf8x8Blocks[61] * meanOf8x8Blocks[61])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_62] = (EB_U16)((meanOf8x8SquaredValuesBlocks[62] - (meanOf8x8Blocks[62] * meanOf8x8Blocks[62])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_8x8_63] = (EB_U16)((meanOf8x8SquaredValuesBlocks[63] - (meanOf8x8Blocks[63] * meanOf8x8Blocks[63])) >> VARIANCE_PRECISION);

	// 16x16 variances
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_0] = (EB_U16)((meanOf16x16SquaredValuesBlocks[0] - (meanOf16x16Blocks[0] * meanOf16x16Blocks[0])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_1] = (EB_U16)((meanOf16x16SquaredValuesBlocks[1] - (meanOf16x16Blocks[1] * meanOf16x16Blocks[1])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_2] = (EB_U16)((meanOf16x16SquaredValuesBlocks[2] - (meanOf16x16Blocks[2] * meanOf16x16Blocks[2])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_3] = (EB_U16)((meanOf16x16SquaredValuesBlocks[3] - (meanOf16x16Blocks[3] * meanOf16x16Blocks[3])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_4] = (EB_U16)((meanOf16x16SquaredValuesBlocks[4] - (meanOf16x16Blocks[4] * meanOf16x16Blocks[4])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_5] = (EB_U16)((meanOf16x16SquaredValuesBlocks[5] - (meanOf16x16Blocks[5] * meanOf16x16Blocks[5])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_6] = (EB_U16)((meanOf16x16SquaredValuesBlocks[6] - (meanOf16x16Blocks[6] * meanOf16x16Blocks[6])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_7] = (EB_U16)((meanOf16x16SquaredValuesBlocks[7] - (meanOf16x16Blocks[7] * meanOf16x16Blocks[7])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_8] = (EB_U16)((meanOf16x16SquaredValuesBlocks[8] - (meanOf16x16Blocks[8] * meanOf16x16Blocks[8])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_9] = (EB_U16)((meanOf16x16SquaredValuesBlocks[9] - (meanOf16x16Blocks[9] * meanOf16x16Blocks[9])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_10] = (EB_U16)((meanOf16x16SquaredValuesBlocks[10] - (meanOf16x16Blocks[10] * meanOf16x16Blocks[10])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_11] = (EB_U16)((meanOf16x16SquaredValuesBlocks[11] - (meanOf16x16Blocks[11] * meanOf16x16Blocks[11])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_12] = (EB_U16)((meanOf16x16SquaredValuesBlocks[12] - (meanOf16x16Blocks[12] * meanOf16x16Blocks[12])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_13] = (EB_U16)((meanOf16x16SquaredValuesBlocks[13] - (meanOf16x16Blocks[13] * meanOf16x16Blocks[13])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_14] = (EB_U16)((meanOf16x16SquaredValuesBlocks[14] - (meanOf16x16Blocks[14] * meanOf16x16Blocks[14])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_16x16_15] = (EB_U16)((meanOf16x16SquaredValuesBlocks[15] - (meanOf16x16Blocks[15] * meanOf16x16Blocks[15])) >> VARIANCE_PRECISION);

	// 32x32 variances
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_32x32_0] = (EB_U16)((meanOf32x32SquaredValuesBlocks[0] - (meanOf32x32Blocks[0] * meanOf32x32Blocks[0])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_32x32_1] = (EB_U16)((meanOf32x32SquaredValuesBlocks[1] - (meanOf32x32Blocks[1] * meanOf32x32Blocks[1])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_32x32_2] = (EB_U16)((meanOf32x32SquaredValuesBlocks[2] - (meanOf32x32Blocks[2] * meanOf32x32Blocks[2])) >> VARIANCE_PRECISION);
	pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_32x32_3] = (EB_U16)((meanOf32x32SquaredValuesBlocks[3] - (meanOf32x32Blocks[3] * meanOf32x32Blocks[3])) >> VARIANCE_PRECISION);

		// 64x64 variance
		pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_64x64] = (EB_U16)((meanOf64x64SquaredValuesBlocks - (meanOf64x64Blocks * meanOf64x64Blocks)) >> VARIANCE_PRECISION);
}
	return return_error;
}

static EB_ERRORTYPE DenoiseInputPicture(
	PictureAnalysisContext_t	*contextPtr,
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32		 lcuIndex;
	EB_U32       lcuOriginX;
	EB_U32       lcuOriginY;
	EB_U16       verticalIdx;
    EB_U32 		 colorFormat      = inputPicturePtr->colorFormat;
    EB_U16 		 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 		 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
	//use denoised input if the source is extremly noisy 
	if (pictureControlSetPtr->picNoiseClass >= PIC_NOISE_CLASS_4){

		EB_U32 inLumaOffSet = inputPicturePtr->originX + inputPicturePtr->originY      * inputPicturePtr->strideY;
        EB_U32 inChromaOffSet = (inputPicturePtr->originX >> subWidthCMinus1) + (inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCb;
		EB_U32 denLumaOffSet = denoisedPicturePtr->originX + denoisedPicturePtr->originY   * denoisedPicturePtr->strideY;
        EB_U32 denChromaOffSet = (denoisedPicturePtr->originX >> subWidthCMinus1) + (denoisedPicturePtr->originY >> subHeightCMinus1) * denoisedPicturePtr->strideCb;

		//filter Luma
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;


			if (lcuOriginX == 0)
				StrongLumaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				inputPicturePtr,
				denoisedPicturePtr,
				lcuOriginY,
				lcuOriginX);

			if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
			{
				noiseExtractLumaStrong(
					inputPicturePtr,
					denoisedPicturePtr,
					lcuOriginY,
					lcuOriginX);
			}

		}

		//copy Luma
		for (verticalIdx = 0; verticalIdx < inputPicturePtr->height; ++verticalIdx) {
			EB_MEMCPY(inputPicturePtr->bufferY + inLumaOffSet + verticalIdx * inputPicturePtr->strideY,
				denoisedPicturePtr->bufferY + denLumaOffSet + verticalIdx * denoisedPicturePtr->strideY,
				sizeof(EB_U8) * inputPicturePtr->width);
		}

		//copy chroma
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;

			if (lcuOriginX == 0)
				StrongChromaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				inputPicturePtr,
				denoisedPicturePtr,
				lcuOriginY >> subHeightCMinus1,
				lcuOriginX >> subWidthCMinus1);

			if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
			{
				noiseExtractChromaStrong(
					inputPicturePtr,
					denoisedPicturePtr,
					lcuOriginY >> subHeightCMinus1,
					lcuOriginX >> subWidthCMinus1);
			}

		}

		//copy chroma
		for (verticalIdx = 0; verticalIdx < inputPicturePtr->height >> subHeightCMinus1; ++verticalIdx) {

			EB_MEMCPY(inputPicturePtr->bufferCb + inChromaOffSet + verticalIdx * inputPicturePtr->strideCb,
				denoisedPicturePtr->bufferCb + denChromaOffSet + verticalIdx * denoisedPicturePtr->strideCb,
				sizeof(EB_U8) * inputPicturePtr->width >> subWidthCMinus1);

			EB_MEMCPY(inputPicturePtr->bufferCr + inChromaOffSet + verticalIdx * inputPicturePtr->strideCr,
				denoisedPicturePtr->bufferCr + denChromaOffSet + verticalIdx * denoisedPicturePtr->strideCr,
				sizeof(EB_U8) * inputPicturePtr->width >> subWidthCMinus1);
		}

	}
	else if (pictureControlSetPtr->picNoiseClass >= PIC_NOISE_CLASS_3_1){

		EB_U32 inLumaOffSet = inputPicturePtr->originX + inputPicturePtr->originY      * inputPicturePtr->strideY;
        EB_U32 inChromaOffSet = (inputPicturePtr->originX >> subWidthCMinus1) + (inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCb;
		EB_U32 denLumaOffSet = denoisedPicturePtr->originX + denoisedPicturePtr->originY   * denoisedPicturePtr->strideY;
        EB_U32 denChromaOffSet = (denoisedPicturePtr->originX >> subWidthCMinus1) + (denoisedPicturePtr->originY >> subHeightCMinus1) * denoisedPicturePtr->strideCb;


		for (verticalIdx = 0; verticalIdx < inputPicturePtr->height; ++verticalIdx) {
			EB_MEMCPY(inputPicturePtr->bufferY + inLumaOffSet + verticalIdx * inputPicturePtr->strideY,
				denoisedPicturePtr->bufferY + denLumaOffSet + verticalIdx * denoisedPicturePtr->strideY,
				sizeof(EB_U8) * inputPicturePtr->width);
		}

		//copy chroma
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;

			if (lcuOriginX == 0)
				WeakChromaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				inputPicturePtr,
				denoisedPicturePtr,
				lcuOriginY >> subHeightCMinus1,
				lcuOriginX >> subWidthCMinus1);

			if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
			{
				noiseExtractChromaWeak(
					inputPicturePtr,
					denoisedPicturePtr,
					lcuOriginY >> subHeightCMinus1,
					lcuOriginX >> subWidthCMinus1);
			}

		}



		for (verticalIdx = 0; verticalIdx < inputPicturePtr->height >> subHeightCMinus1; ++verticalIdx) {

			EB_MEMCPY(inputPicturePtr->bufferCb + inChromaOffSet + verticalIdx * inputPicturePtr->strideCb,
				denoisedPicturePtr->bufferCb + denChromaOffSet + verticalIdx * denoisedPicturePtr->strideCb,
				sizeof(EB_U8) * inputPicturePtr->width >> subWidthCMinus1);

			EB_MEMCPY(inputPicturePtr->bufferCr + inChromaOffSet + verticalIdx * inputPicturePtr->strideCr,
				denoisedPicturePtr->bufferCr + denChromaOffSet + verticalIdx * denoisedPicturePtr->strideCr,
				sizeof(EB_U8) * inputPicturePtr->width >> subWidthCMinus1);
		}

	}

    else if (contextPtr->picNoiseVarianceFloat >= 1.0  && sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {

		//Luma : use filtered only for flatNoise LCUs
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;

			EB_U32  lcuHeight = MIN(MAX_LCU_SIZE, inputPicturePtr->height - lcuOriginY);
			EB_U32  lcuWidth = MIN(MAX_LCU_SIZE, inputPicturePtr->width - lcuOriginX);

			EB_U32 inLumaOffSet = inputPicturePtr->originX + lcuOriginX + (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY;
			EB_U32 denLumaOffSet = denoisedPicturePtr->originX + lcuOriginX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;


			if (pictureControlSetPtr->lcuFlatNoiseArray[lcuIndex] == 1){


				for (verticalIdx = 0; verticalIdx < lcuHeight; ++verticalIdx) {

					EB_MEMCPY(inputPicturePtr->bufferY + inLumaOffSet + verticalIdx * inputPicturePtr->strideY,
						denoisedPicturePtr->bufferY + denLumaOffSet + verticalIdx * denoisedPicturePtr->strideY,
						sizeof(EB_U8) * lcuWidth);

				}
			}
		}
	}

	return return_error;
}

static EB_ERRORTYPE DetectInputPictureNoise(
	PictureAnalysisContext_t	*contextPtr,
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32					 lcuIndex;

	EB_U64                   picNoiseVariance;

	EB_U32			         totLcuCount, noiseTh;

	EB_U32                   lcuOriginX;
	EB_U32                   lcuOriginY;
	EB_U32				     inputLumaOriginIndex;

	picNoiseVariance = 0;
	totLcuCount = 0;

	//Variance calc for noise picture
    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

        lcuOriginX = lcuParams->originX;
        lcuOriginY = lcuParams->originY;
		inputLumaOriginIndex = (noisePicturePtr->originY + lcuOriginY) * noisePicturePtr->strideY +
			noisePicturePtr->originX + lcuOriginX;


		EB_U32  noiseOriginIndex = noisePicturePtr->originX + lcuOriginX + noisePicturePtr->originY * noisePicturePtr->strideY;

		if (lcuOriginX == 0)
			WeakLumaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
			inputPicturePtr,
			denoisedPicturePtr,
			noisePicturePtr,
			lcuOriginY,
			lcuOriginX);

		if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
		{
			noiseExtractLumaWeak(
				inputPicturePtr,
				denoisedPicturePtr,
				noisePicturePtr,
				lcuOriginY,
				lcuOriginX);
		}

		//do it only for complete 64x64 blocks
        if (lcuParams->isCompleteLcu) 
		{

			EB_U64 noiseBlkVar32x32[4], denoiseBlkVar32x32[4];

			EB_U64 noiseBlkVar = ComputeVariance64x64(           
                noisePicturePtr,
				noiseOriginIndex,
				noiseBlkVar32x32);

            EB_U64 noiseBlkVarTh ;
            EB_U64 denBlkVarTh = FLAT_MAX_VAR;

			if (pictureControlSetPtr->noiseDetectionTh == 1)
				noiseBlkVarTh = NOISE_MIN_LEVEL_0;
			else
				noiseBlkVarTh = NOISE_MIN_LEVEL_1;

			picNoiseVariance += (noiseBlkVar >> 16);

			EB_U64 denBlkVar = ComputeVariance64x64(                          
                denoisedPicturePtr,
				inputLumaOriginIndex,
				denoiseBlkVar32x32) >> 16;

			if (denBlkVar < denBlkVarTh && noiseBlkVar > noiseBlkVarTh) {
				pictureControlSetPtr->lcuFlatNoiseArray[lcuIndex] = 1;
			}

			totLcuCount++;
		}

	}

    if (totLcuCount > 0) {
        contextPtr->picNoiseVarianceFloat = (double)picNoiseVariance / (double)totLcuCount;

        picNoiseVariance = picNoiseVariance / totLcuCount;
    }

	//the variance of a 64x64 noise area tends to be bigger for small resolutions.
	if (sequenceControlSetPtr->lumaHeight <= 720)
		noiseTh = 25;
	else
		noiseTh = 0;

	if (picNoiseVariance >= 80 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_10;
	else if (picNoiseVariance >= 70 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_9;
	else if (picNoiseVariance >= 60 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_8;
	else if (picNoiseVariance >= 50 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_7;
	else if (picNoiseVariance >= 40 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_6;
	else if (picNoiseVariance >= 30 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_5;
	else if (picNoiseVariance >= 20 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_4;
	else if (picNoiseVariance >= 17 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3_1;
	else if (picNoiseVariance >= 10 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3;
	else if (picNoiseVariance >= 5 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_2;
	else
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_1;

	if (pictureControlSetPtr->picNoiseClass >= PIC_NOISE_CLASS_4)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3_1;

	return return_error;

}

static EB_ERRORTYPE FullSampleDenoise(
	PictureAnalysisContext_t	*contextPtr,
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EB_U32                       lcuTotalCount,
	EB_BOOL                      denoiseFlag)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32					 lcuCodingOrder;
	EbPictureBufferDesc_t	*inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;
	EbPictureBufferDesc_t	*denoisedPicturePtr = contextPtr->denoisedPicturePtr;
	EbPictureBufferDesc_t	*noisePicturePtr = contextPtr->noisePicturePtr;

	//Reset the flat noise flag array to False for both RealTime/HighComplexity Modes
	for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {
		pictureControlSetPtr->lcuFlatNoiseArray[lcuCodingOrder] = 0;
    }

    pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_INV; //this init is for both REAL-TIME and BEST-QUALITY

    DetectInputPictureNoise(
        contextPtr,
        sequenceControlSetPtr,
        pictureControlSetPtr,
        inputPicturePtr,
        noisePicturePtr,
        denoisedPicturePtr);

    if (denoiseFlag == EB_TRUE)
    {
        DenoiseInputPicture(
            contextPtr,
            sequenceControlSetPtr,
            pictureControlSetPtr,
            inputPicturePtr,
            denoisedPicturePtr);
    }

    return return_error;

}

static EB_ERRORTYPE SubSampleFilterNoise(
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32		 lcuIndex;
	EB_U32       lcuOriginX;
	EB_U32       lcuOriginY;
	EB_U16       verticalIdx;
    EB_U32       colorFormat = inputPicturePtr->colorFormat;
    EB_U16       subWidthCMinus1 = (colorFormat  == EB_YUV444 ? 1 : 2) - 1;
    EB_U16       subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

	if (pictureControlSetPtr->picNoiseClass == PIC_NOISE_CLASS_3_1) {

		EB_U32 inLumaOffSet = inputPicturePtr->originX + inputPicturePtr->originY      * inputPicturePtr->strideY;
        EB_U32 inChromaOffSet = (inputPicturePtr->originX >> subWidthCMinus1) + (inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCb;
		EB_U32 denLumaOffSet = denoisedPicturePtr->originX + denoisedPicturePtr->originY   * denoisedPicturePtr->strideY;
        EB_U32 denChromaOffSet = (denoisedPicturePtr->originX >> subWidthCMinus1) + (denoisedPicturePtr->originY >> subHeightCMinus1) * denoisedPicturePtr->strideCb;


		//filter Luma
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;

			if (lcuOriginX == 0)
				WeakLumaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				inputPicturePtr,
				denoisedPicturePtr,
				noisePicturePtr,
				lcuOriginY,
				lcuOriginX);

			if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
			{
				noiseExtractLumaWeak(
					inputPicturePtr,
					denoisedPicturePtr,
					noisePicturePtr,
					lcuOriginY,
					lcuOriginX);
			}
		}

		//copy luma
		for (verticalIdx = 0; verticalIdx < inputPicturePtr->height; ++verticalIdx) {
			EB_MEMCPY(inputPicturePtr->bufferY + inLumaOffSet + verticalIdx * inputPicturePtr->strideY,
				denoisedPicturePtr->bufferY + denLumaOffSet + verticalIdx * denoisedPicturePtr->strideY,
				sizeof(EB_U8) * inputPicturePtr->width);
		}

		//filter chroma
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;

			if (lcuOriginX == 0)
				WeakChromaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				inputPicturePtr,
				denoisedPicturePtr,
				lcuOriginY >> subHeightCMinus1,
				lcuOriginX >> subWidthCMinus1);

			if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
			{
				noiseExtractChromaWeak(
					inputPicturePtr,
					denoisedPicturePtr,
					lcuOriginY >> subHeightCMinus1,
					lcuOriginX >> subWidthCMinus1);
			}

		}

		//copy chroma
		for (verticalIdx = 0; verticalIdx < inputPicturePtr->height >> subHeightCMinus1; ++verticalIdx) {

			EB_MEMCPY(inputPicturePtr->bufferCb + inChromaOffSet + verticalIdx * inputPicturePtr->strideCb,
				denoisedPicturePtr->bufferCb + denChromaOffSet + verticalIdx * denoisedPicturePtr->strideCb,
				sizeof(EB_U8) * inputPicturePtr->width >> subWidthCMinus1);

			EB_MEMCPY(inputPicturePtr->bufferCr + inChromaOffSet + verticalIdx * inputPicturePtr->strideCr,
				denoisedPicturePtr->bufferCr + denChromaOffSet + verticalIdx * denoisedPicturePtr->strideCr,
				sizeof(EB_U8) * inputPicturePtr->width >> subWidthCMinus1);
		}

	} else if (pictureControlSetPtr->picNoiseClass == PIC_NOISE_CLASS_2){

		EB_U32 newTotFN = 0;

		//for each LCU ,re check the FN information for only the FNdecim ones 
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;
			EB_U32  inputLumaOriginIndex = noisePicturePtr->originX + lcuOriginX + (noisePicturePtr->originY + lcuOriginY) * noisePicturePtr->strideY;
			EB_U32  noiseOriginIndex = noisePicturePtr->originX + lcuOriginX + (noisePicturePtr->originY * noisePicturePtr->strideY);

			if (lcuParams->isCompleteLcu && pictureControlSetPtr->lcuFlatNoiseArray[lcuIndex] == 1)
			{

				WeakLumaFilterLcu_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
					inputPicturePtr,
					denoisedPicturePtr,
					noisePicturePtr,
					lcuOriginY,
					lcuOriginX);

				if (lcuOriginX + MAX_LCU_SIZE > inputPicturePtr->width)
				{
					noiseExtractLumaWeakLcu(
						inputPicturePtr,
						denoisedPicturePtr,
						noisePicturePtr,
						lcuOriginY,
						lcuOriginX);
				}

				EB_U64 noiseBlkVar32x32[4], denoiseBlkVar32x32[4];
				EB_U64 noiseBlkVar = ComputeVariance64x64(
                    noisePicturePtr, noiseOriginIndex, noiseBlkVar32x32);
				EB_U64 denBlkVar = ComputeVariance64x64(                    
                    denoisedPicturePtr, inputLumaOriginIndex, denoiseBlkVar32x32) >> 16;

                EB_U64 noiseBlkVarTh ;
                EB_U64 denBlkVarTh = FLAT_MAX_VAR;

			    if (pictureControlSetPtr->noiseDetectionTh == 1)
				    noiseBlkVarTh = NOISE_MIN_LEVEL_0;
			    else
				    noiseBlkVarTh = NOISE_MIN_LEVEL_1;

				if (denBlkVar<denBlkVarTh && noiseBlkVar> noiseBlkVarTh) {
					pictureControlSetPtr->lcuFlatNoiseArray[lcuIndex] = 1;
					//SVT_LOG("POC %i (%i,%i) denBlkVar: %i  noiseBlkVar :%i\n", pictureControlSetPtr->pictureNumber,lcuOriginX,lcuOriginY, denBlkVar, noiseBlkVar);
					newTotFN++;

				}
				else{
					pictureControlSetPtr->lcuFlatNoiseArray[lcuIndex] = 0;
				}
			}
		}

        //filter Luma
        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

            lcuOriginX = lcuParams->originX;
            lcuOriginY = lcuParams->originY;

			if (lcuOriginX + 64 <= inputPicturePtr->width && lcuOriginY + 64 <= inputPicturePtr->height)
			{


				//use the denoised for FN LCUs
				if (pictureControlSetPtr->lcuFlatNoiseArray[lcuIndex] == 1){

					EB_U32  lcuHeight = MIN(MAX_LCU_SIZE, inputPicturePtr->height - lcuOriginY);
					EB_U32  lcuWidth = MIN(MAX_LCU_SIZE, inputPicturePtr->width - lcuOriginX);

					EB_U32 inLumaOffSet = inputPicturePtr->originX + lcuOriginX + (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY;
					EB_U32 denLumaOffSet = denoisedPicturePtr->originX + lcuOriginX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;

					for (verticalIdx = 0; verticalIdx < lcuHeight; ++verticalIdx) {

						EB_MEMCPY(inputPicturePtr->bufferY + inLumaOffSet + verticalIdx * inputPicturePtr->strideY,
							denoisedPicturePtr->bufferY + denLumaOffSet + verticalIdx * denoisedPicturePtr->strideY,
							sizeof(EB_U8) * lcuWidth);

					}
				}

			}

		}

	}
	return return_error;
}

static EB_ERRORTYPE QuarterSampleDetectNoise(
	PictureAnalysisContext_t	*contextPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t       *quarterDecimatedPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32						 pictureWidthInLcu)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U64                   picNoiseVariance;

	EB_U32			         totLcuCount, noiseTh;

	EB_U32				     blockIndex;

	picNoiseVariance = 0;
	totLcuCount = 0;


	EB_U16 vert64x64Index;
	EB_U16 horz64x64Index;
	EB_U32 block64x64X;
	EB_U32 block64x64Y;
	EB_U32 vert32x32Index;
	EB_U32 horz32x32Index;
	EB_U32 block32x32X;
	EB_U32 block32x32Y;
	EB_U32 noiseOriginIndex;
	EB_U32 lcuCodingOrder;

	// Loop over 64x64 blocks on the downsampled domain (each block would contain 16 LCUs on the full sampled domain)
	for (vert64x64Index = 0; vert64x64Index < (quarterDecimatedPicturePtr->height / 64); vert64x64Index++){
		for (horz64x64Index = 0; horz64x64Index < (quarterDecimatedPicturePtr->width / 64); horz64x64Index++){

			block64x64X = horz64x64Index * 64;
			block64x64Y = vert64x64Index * 64;

			if (block64x64X == 0)
				WeakLumaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				quarterDecimatedPicturePtr,
				denoisedPicturePtr,
				noisePicturePtr,
				block64x64Y,
				block64x64X);

			if (block64x64Y + MAX_LCU_SIZE > quarterDecimatedPicturePtr->width)
			{
				noiseExtractLumaWeak(
					quarterDecimatedPicturePtr,
					denoisedPicturePtr,
					noisePicturePtr,
					block64x64Y,
					block64x64X);
			}


			// Loop over 32x32 blocks (i.e, 64x64 blocks in full resolution)
			for (vert32x32Index = 0; vert32x32Index < 2; vert32x32Index++){
				for (horz32x32Index = 0; horz32x32Index < 2; horz32x32Index++){

					block32x32X = block64x64X + horz32x32Index * 32;
					block32x32Y = block64x64Y + vert32x32Index * 32;

					//do it only for complete 32x32 blocks (i.e, complete 64x64 blocks in full resolution)
					if ((block32x32X + 32 <= quarterDecimatedPicturePtr->width) && (block32x32Y + 32 <= quarterDecimatedPicturePtr->height))
					{

						lcuCodingOrder = ((vert64x64Index * 2) + vert32x32Index) * pictureWidthInLcu + ((horz64x64Index * 2) + horz32x32Index);


						EB_U64 noiseBlkVar8x8[16], denoiseBlkVar8x8[16];

						noiseOriginIndex = noisePicturePtr->originX + block32x32X + noisePicturePtr->originY * noisePicturePtr->strideY;

						EB_U64 noiseBlkVar = ComputeVariance32x32(
							noisePicturePtr,
							noiseOriginIndex,
							noiseBlkVar8x8);


						picNoiseVariance += (noiseBlkVar >> 16);

						blockIndex = (noisePicturePtr->originY + block32x32Y) * noisePicturePtr->strideY + noisePicturePtr->originX + block32x32X;

						EB_U64 denBlkVar = ComputeVariance32x32(
							denoisedPicturePtr,
							blockIndex,
							denoiseBlkVar8x8) >> 16;

                        EB_U64 denBlkVarDecTh;

                        if (pictureControlSetPtr->noiseDetectionTh == 0){
                            denBlkVarDecTh = NOISE_MIN_LEVEL_DECIM_1;
                        }
                        else{
                            denBlkVarDecTh = NOISE_MIN_LEVEL_DECIM_0;
                        }

						if (denBlkVar < FLAT_MAX_VAR_DECIM && noiseBlkVar> denBlkVarDecTh) {
							pictureControlSetPtr->lcuFlatNoiseArray[lcuCodingOrder] = 1;
						}

						totLcuCount++;
					}
				}
			}
		}
	}

    if (totLcuCount > 0) {
        contextPtr->picNoiseVarianceFloat = (double)picNoiseVariance / (double)totLcuCount;

        picNoiseVariance = picNoiseVariance / totLcuCount;
    }

	//the variance of a 64x64 noise area tends to be bigger for small resolutions.
	//if (sequenceControlSetPtr->lumaHeight <= 720)
	//	noiseTh = 25;
	//else if (sequenceControlSetPtr->lumaHeight <= 1080)
	//	noiseTh = 10;
	//else
	noiseTh = 0;

	//look for extreme noise or big enough flat noisy area to be denoised.   
	if (picNoiseVariance > 60)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3_1; //Noise+Edge information is too big, so may be this is all noise (action: frame based denoising) 
	else if (picNoiseVariance >= 10 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3;   //Noise+Edge information is big enough, so there is no big enough flat noisy area (action : no denoising)
	else if (picNoiseVariance >= 5 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_2;   //Noise+Edge information is relatively small, so there might be a big enough flat noisy area(action : denoising only for FN blocks)
	else
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_1;   //Noise+Edge information is very small, so no noise nor edge area (action : no denoising)


	
	return return_error;

}



static EB_ERRORTYPE SubSampleDetectNoise(
	PictureAnalysisContext_t	*contextPtr,
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t       *sixteenthDecimatedPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32						 pictureWidthInLcu)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U64                   picNoiseVariance;

	EB_U32			         totLcuCount, noiseTh;

	EB_U32				     blockIndex;

	picNoiseVariance = 0;
	totLcuCount = 0;


	EB_U16 vert64x64Index;
	EB_U16 horz64x64Index;
	EB_U32 block64x64X;
	EB_U32 block64x64Y;
	EB_U32 vert16x16Index;
	EB_U32 horz16x16Index;
	EB_U32 block16x16X;
	EB_U32 block16x16Y;
	EB_U32 noiseOriginIndex;
	EB_U32 lcuCodingOrder;

	// Loop over 64x64 blocks on the downsampled domain (each block would contain 16 LCUs on the full sampled domain)
	for (vert64x64Index = 0; vert64x64Index < (sixteenthDecimatedPicturePtr->height / 64); vert64x64Index++){
		for (horz64x64Index = 0; horz64x64Index < (sixteenthDecimatedPicturePtr->width / 64); horz64x64Index++){

			block64x64X = horz64x64Index * 64;
			block64x64Y = vert64x64Index * 64;

			if (block64x64X == 0)
				WeakLumaFilter_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				sixteenthDecimatedPicturePtr,
				denoisedPicturePtr,
				noisePicturePtr,
				block64x64Y,
				block64x64X);

			if (block64x64Y + MAX_LCU_SIZE > sixteenthDecimatedPicturePtr->width)
			{
				noiseExtractLumaWeak(
					sixteenthDecimatedPicturePtr,
					denoisedPicturePtr,
					noisePicturePtr,
					block64x64Y,
					block64x64X);
			}


			// Loop over 16x16 blocks (i.e, 64x64 blocks in full resolution)
			for (vert16x16Index = 0; vert16x16Index < 4; vert16x16Index++){
				for (horz16x16Index = 0; horz16x16Index < 4; horz16x16Index++){

					block16x16X = block64x64X + horz16x16Index * 16;
					block16x16Y = block64x64Y + vert16x16Index * 16;

					//do it only for complete 16x16 blocks (i.e, complete 64x64 blocks in full resolution)
					if (block16x16X + 16 <= sixteenthDecimatedPicturePtr->width && block16x16Y + 16 <= sixteenthDecimatedPicturePtr->height)
					{

						lcuCodingOrder = ((vert64x64Index * 4) + vert16x16Index) * pictureWidthInLcu + ((horz64x64Index * 4) + horz16x16Index);


						EB_U64 noiseBlkVar8x8[4], denoiseBlkVar8x8[4];

						noiseOriginIndex = noisePicturePtr->originX + block16x16X + noisePicturePtr->originY * noisePicturePtr->strideY;

						EB_U64 noiseBlkVar = ComputeVariance16x16(
							noisePicturePtr,
							noiseOriginIndex,
							noiseBlkVar8x8);


						picNoiseVariance += (noiseBlkVar >> 16);

						blockIndex = (noisePicturePtr->originY + block16x16Y) * noisePicturePtr->strideY + noisePicturePtr->originX + block16x16X;

						EB_U64 denBlkVar = ComputeVariance16x16(
							denoisedPicturePtr,
							blockIndex,
							denoiseBlkVar8x8) >> 16;

                        EB_U64  noiseBlkVarDecTh ;
                        EB_U64 denBlkVarDecTh = FLAT_MAX_VAR_DECIM;

						if (pictureControlSetPtr->noiseDetectionTh == 1) {
							noiseBlkVarDecTh = NOISE_MIN_LEVEL_DECIM_0;
						}
						else {
							noiseBlkVarDecTh = NOISE_MIN_LEVEL_DECIM_1;
						}

						if (denBlkVar < denBlkVarDecTh && noiseBlkVar> noiseBlkVarDecTh) {
							pictureControlSetPtr->lcuFlatNoiseArray[lcuCodingOrder] = 1;
						}
						totLcuCount++;
					}
				}
			}
		}
	}

    if (totLcuCount > 0) {
        contextPtr->picNoiseVarianceFloat = (double)picNoiseVariance / (double)totLcuCount;

        picNoiseVariance = picNoiseVariance / totLcuCount;
    }

	//the variance of a 64x64 noise area tends to be bigger for small resolutions.
	if (sequenceControlSetPtr->lumaHeight <= 720)
		noiseTh = 25;
	else if (sequenceControlSetPtr->lumaHeight <= 1080)
		noiseTh = 10;
	else
		noiseTh = 0;

	//look for extreme noise or big enough flat noisy area to be denoised.    
	if (picNoiseVariance >= 55 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3_1; //Noise+Edge information is too big, so may be this is all noise (action: frame based denoising) 
	else if (picNoiseVariance >= 10 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_3;   //Noise+Edge information is big enough, so there is no big enough flat noisy area (action : no denoising)
	else if (picNoiseVariance >= 5 + noiseTh)
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_2;   //Noise+Edge information is relatively small, so there might be a big enough flat noisy area(action : denoising only for FN blocks)
	else
		pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_1;   //Noise+Edge information is very small, so no noise nor edge area (action : no denoising)

	return return_error;

}

static EB_ERRORTYPE QuarterSampleDenoise(
	PictureAnalysisContext_t	*contextPtr,
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t		*quarterDecimatedPicturePtr,
	EB_U32                       lcuTotalCount,
	EB_BOOL                      denoiseFlag,
	EB_U32						 pictureWidthInLcu)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32					 lcuCodingOrder;
	EbPictureBufferDesc_t	*inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;
	EbPictureBufferDesc_t	*denoisedPicturePtr = contextPtr->denoisedPicturePtr;
	EbPictureBufferDesc_t	*noisePicturePtr = contextPtr->noisePicturePtr;

	//Reset the flat noise flag array to False for both RealTime/HighComplexity Modes
	for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {
		pictureControlSetPtr->lcuFlatNoiseArray[lcuCodingOrder] = 0;
	}

	pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_INV; //this init is for both REAL-TIME and BEST-QUALITY

    Decimation2D(
        &inputPicturePtr->bufferY[inputPicturePtr->originX + inputPicturePtr->originY * inputPicturePtr->strideY],
        inputPicturePtr->strideY,
        inputPicturePtr->width,
        inputPicturePtr->height,
        &quarterDecimatedPicturePtr->bufferY[quarterDecimatedPicturePtr->originX + (quarterDecimatedPicturePtr->originY * quarterDecimatedPicturePtr->strideY)],
        quarterDecimatedPicturePtr->strideY,
        2);


	QuarterSampleDetectNoise(
		contextPtr,
		pictureControlSetPtr,
		quarterDecimatedPicturePtr,
		noisePicturePtr,
		denoisedPicturePtr,
		pictureWidthInLcu);

	if (denoiseFlag == EB_TRUE) {
        
        // Turn OFF the de-noiser for Class 2 at QP=29 and lower (for Fixed_QP) and at the target rate of 14Mbps and higher (for RC=ON)
		if ((pictureControlSetPtr->picNoiseClass == PIC_NOISE_CLASS_3_1) || ((pictureControlSetPtr->picNoiseClass == PIC_NOISE_CLASS_2) && ((sequenceControlSetPtr->staticConfig.rateControlMode == 0 && sequenceControlSetPtr->qp > DENOISER_QP_TH) || (sequenceControlSetPtr->staticConfig.rateControlMode != 0 && sequenceControlSetPtr->staticConfig.targetBitRate < DENOISER_BITRATE_TH)))) {

			SubSampleFilterNoise(
				sequenceControlSetPtr,
				pictureControlSetPtr,
				inputPicturePtr,
				noisePicturePtr,
				denoisedPicturePtr);
		}
	}

	return return_error;

}


static EB_ERRORTYPE HalfSampleDenoise(
	PictureAnalysisContext_t	*contextPtr,
	SequenceControlSet_t		*sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t		*sixteenthDecimatedPicturePtr,
	EB_U32                       lcuTotalCount,
	EB_BOOL                      denoiseFlag,
	EB_U32						 pictureWidthInLcu)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32					 lcuCodingOrder;
	EbPictureBufferDesc_t	*inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;
	EbPictureBufferDesc_t	*denoisedPicturePtr = contextPtr->denoisedPicturePtr;
	EbPictureBufferDesc_t	*noisePicturePtr = contextPtr->noisePicturePtr;

	//Reset the flat noise flag array to False for both RealTime/HighComplexity Modes
	for (lcuCodingOrder = 0; lcuCodingOrder < lcuTotalCount; ++lcuCodingOrder) {
		pictureControlSetPtr->lcuFlatNoiseArray[lcuCodingOrder] = 0;
	}

	pictureControlSetPtr->picNoiseClass = PIC_NOISE_CLASS_INV; //this init is for both REAL-TIME and BEST-QUALITY
    
    Decimation2D(
        &inputPicturePtr->bufferY[inputPicturePtr->originX + inputPicturePtr->originY * inputPicturePtr->strideY],
        inputPicturePtr->strideY,
        inputPicturePtr->width,
        inputPicturePtr->height,
        &sixteenthDecimatedPicturePtr->bufferY[sixteenthDecimatedPicturePtr->originX + (sixteenthDecimatedPicturePtr->originY * sixteenthDecimatedPicturePtr->strideY)],
        sixteenthDecimatedPicturePtr->strideY,
        4);

	SubSampleDetectNoise(
		contextPtr,
		sequenceControlSetPtr,
		pictureControlSetPtr,
		sixteenthDecimatedPicturePtr,
		noisePicturePtr,
		denoisedPicturePtr,
		pictureWidthInLcu);

	if (denoiseFlag == EB_TRUE) {

		// Turn OFF the de-noiser for Class 2 at QP=29 and lower (for Fixed_QP) and at the target rate of 14Mbps and higher (for RC=ON)
		if ((pictureControlSetPtr->picNoiseClass == PIC_NOISE_CLASS_3_1) || ((pictureControlSetPtr->picNoiseClass == PIC_NOISE_CLASS_2) && ((sequenceControlSetPtr->staticConfig.rateControlMode == 0 && sequenceControlSetPtr->qp > DENOISER_QP_TH) || (sequenceControlSetPtr->staticConfig.rateControlMode != 0 && sequenceControlSetPtr->staticConfig.targetBitRate < DENOISER_BITRATE_TH)))) {

			SubSampleFilterNoise(
				sequenceControlSetPtr,
				pictureControlSetPtr,
				inputPicturePtr,
				noisePicturePtr,
				denoisedPicturePtr);
		}
	}

	return return_error;

}


/************************************************
 * Set Picture Parameters based on input configuration
 ** Setting Number of regions per resolution
 ** Setting width and height for subpicture and when picture scan type is 1
 ************************************************/
static void SetPictureParametersForStatisticsGathering(
	SequenceControlSet_t            *sequenceControlSetPtr
	)
{
	sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth = HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_WIDTH;
	sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight = HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_HEIGHT;
	sequenceControlSetPtr->pictureActivityRegionTh = HIGHER_THAN_CLASS_1_PICTURE_ACTIVITY_REGIONS_TH;

	return;
}

/************************************************
 * Picture Pre Processing Operations *
 *** A function that groups all of the Pre proceesing
 * operations performed on the input picture
 *** Operations included at this point:
 ***** Borders preprocessing
 ***** Denoising
 ************************************************/
static void PicturePreProcessingOperations(
	PictureParentControlSet_t       *pictureControlSetPtr,
	PictureAnalysisContext_t        *contextPtr,
	SequenceControlSet_t            *sequenceControlSetPtr,
	EbPictureBufferDesc_t           *quarterDecimatedPicturePtr,
	EbPictureBufferDesc_t           *sixteenthDecimatedPicturePtr,
	EB_U32                           lcuTotalCount,
	EB_U32                           pictureWidthInLcu)
{
	if (pictureControlSetPtr->noiseDetectionMethod == NOISE_DETECT_HALF_PRECISION) {

		HalfSampleDenoise(
			contextPtr,
			sequenceControlSetPtr,
			pictureControlSetPtr,
			sixteenthDecimatedPicturePtr,
			lcuTotalCount,
			pictureControlSetPtr->enableDenoiseSrcFlag,
			pictureWidthInLcu);
	}
    else if (pictureControlSetPtr->noiseDetectionMethod == NOISE_DETECT_QUARTER_PRECISION) {
		QuarterSampleDenoise(
			contextPtr,
			sequenceControlSetPtr,
			pictureControlSetPtr,
			quarterDecimatedPicturePtr,
			lcuTotalCount,
			pictureControlSetPtr->enableDenoiseSrcFlag,
			pictureWidthInLcu);
	} else {	
		FullSampleDenoise(
			contextPtr,
			sequenceControlSetPtr,
			pictureControlSetPtr,
			lcuTotalCount,
			pictureControlSetPtr->enableDenoiseSrcFlag
		);		
	}
	return;

}

/**************************************************************
* Generate picture histogram bins for YUV pixel intensity *
* Calculation is done on a region based (Set previously, resolution dependent)
**************************************************************/
static void SubSampleLumaGeneratePixelIntensityHistogramBins(
	SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	EbPictureBufferDesc_t           *inputPicturePtr,
    EB_U64                          *sumAverageIntensityTotalRegionsLuma){

	EB_U32                          regionWidth;
	EB_U32                          regionHeight;
	EB_U32                          regionWidthOffset;
	EB_U32                          regionHeightOffset;
	EB_U32                          regionInPictureWidthIndex;
	EB_U32                          regionInPictureHeightIndex;
	EB_U32							histogramBin;
	EB_U64                          sum;

	regionWidth = inputPicturePtr->width / sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth;
	regionHeight = inputPicturePtr->height / sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight;

	// Loop over regions inside the picture
	for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth; regionInPictureWidthIndex++){  // loop over horizontal regions
		for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight; regionInPictureHeightIndex++){ // loop over vertical regions


			// Initialize bins to 1
			InitializeBuffer_32bits_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0], 64, 0, 1);

			regionWidthOffset = (regionInPictureWidthIndex == sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth - 1) ?
				inputPicturePtr->width - (sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth * regionWidth) :
				0;

			regionHeightOffset = (regionInPictureHeightIndex == sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight - 1) ?
				inputPicturePtr->height - (sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight * regionHeight) :
				0;

			// Y Histogram 
			CalculateHistogram(
				&inputPicturePtr->bufferY[(inputPicturePtr->originX + regionInPictureWidthIndex * regionWidth) + ((inputPicturePtr->originY + regionInPictureHeightIndex * regionHeight) * inputPicturePtr->strideY)],
				regionWidth + regionWidthOffset,
				regionHeight + regionHeightOffset,
				inputPicturePtr->strideY,
                1,
                pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0],
				&sum);

			pictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0] = (EB_U8)((sum + (((regionWidth + regionWidthOffset)*(regionHeight + regionHeightOffset)) >> 1)) / ((regionWidth + regionWidthOffset)*(regionHeight + regionHeightOffset)));
            (*sumAverageIntensityTotalRegionsLuma) += (sum << 4);
            for (histogramBin = 0; histogramBin < HISTOGRAM_NUMBER_OF_BINS; histogramBin++){ // Loop over the histogram bins
				pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][histogramBin] =
					pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][histogramBin] << 4;
			}
		}
	}

	return;
}

static void SubSampleChromaGeneratePixelIntensityHistogramBins(
    SequenceControlSet_t            *sequenceControlSetPtr,
    PictureParentControlSet_t       *pictureControlSetPtr,
    EbPictureBufferDesc_t           *inputPicturePtr,
    EB_U64                          *sumAverageIntensityTotalRegionsCb,
    EB_U64                          *sumAverageIntensityTotalRegionsCr){

    EB_U64                          sum;
    EB_U32                          regionWidth;
    EB_U32                          regionHeight;
    EB_U32                          regionWidthOffset;
    EB_U32                          regionHeightOffset;
    EB_U32                          regionInPictureWidthIndex;
    EB_U32                          regionInPictureHeightIndex;

    EB_U16                          histogramBin;
    EB_U8                           decimStep = 4;

    regionWidth  = inputPicturePtr->width / sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth;
    regionHeight = inputPicturePtr->height / sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight;

    // Loop over regions inside the picture
    for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth; regionInPictureWidthIndex++){  // loop over horizontal regions
        for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight; regionInPictureHeightIndex++){ // loop over vertical regions


            // Initialize bins to 1
			InitializeBuffer_32bits_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][1], 64, 0, 1);
			InitializeBuffer_32bits_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][2], 64, 0, 1);

            regionWidthOffset = (regionInPictureWidthIndex == sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth - 1) ?
                inputPicturePtr->width - (sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth * regionWidth) :
                0;

            regionHeightOffset = (regionInPictureHeightIndex == sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight - 1) ?
                inputPicturePtr->height - (sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight * regionHeight) :
                0;


            // U Histogram
            CalculateHistogram(
                &inputPicturePtr->bufferCb[((inputPicturePtr->originX + regionInPictureWidthIndex * regionWidth) >> 1) + (((inputPicturePtr->originY + regionInPictureHeightIndex * regionHeight) >> 1) * inputPicturePtr->strideCb)],
                (regionWidth + regionWidthOffset) >> 1,
                (regionHeight + regionHeightOffset) >> 1,
                inputPicturePtr->strideCb,
                decimStep,
                pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][1],
                &sum);

            sum = (sum << decimStep);
            *sumAverageIntensityTotalRegionsCb += sum;
            pictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][1] = (EB_U8)((sum + (((regionWidth + regionWidthOffset)*(regionHeight + regionHeightOffset)) >> 3)) / (((regionWidth + regionWidthOffset)*(regionHeight + regionHeightOffset)) >> 2));

            for (histogramBin = 0; histogramBin < HISTOGRAM_NUMBER_OF_BINS; histogramBin++){ // Loop over the histogram bins
                pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][1][histogramBin] =
                    pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][1][histogramBin] << decimStep;
            }

            // V Histogram
            CalculateHistogram(
                &inputPicturePtr->bufferCr[((inputPicturePtr->originX + regionInPictureWidthIndex * regionWidth) >> 1) + (((inputPicturePtr->originY + regionInPictureHeightIndex * regionHeight) >> 1) * inputPicturePtr->strideCr)],
                (regionWidth + regionWidthOffset) >> 1,
                (regionHeight + regionHeightOffset) >> 1,
                inputPicturePtr->strideCr,
                decimStep,
                pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][2],
                &sum);

            sum = (sum << decimStep);
            *sumAverageIntensityTotalRegionsCr += sum;
            pictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][2] = (EB_U8)((sum + (((regionWidth + regionWidthOffset)*(regionHeight + regionHeightOffset)) >> 3)) / (((regionWidth + regionWidthOffset)*(regionHeight + regionHeightOffset)) >> 2));

            for (histogramBin = 0; histogramBin < HISTOGRAM_NUMBER_OF_BINS; histogramBin++){ // Loop over the histogram bins
                pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][2][histogramBin] =
                    pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][2][histogramBin] << decimStep;
            }
        }
    }
    return;

}

static void EdgeDetectionMeanLumaChroma16x16(
	SequenceControlSet_t        *sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
    PictureAnalysisContext_t    *contextPtr,
	EB_U32                       totalLcuCount)
{

	EB_U32               lcuIndex;


	EB_U32 maxGrad = 1;

	// The values are calculated for every 4th frame
	if ((pictureControlSetPtr->pictureNumber & 3) == 0){
		for (lcuIndex = 0; lcuIndex < totalLcuCount; lcuIndex++) {

			LcuStat_t *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

			EB_MEMSET(lcuStatPtr, 0, sizeof(LcuStat_t));
			LcuParams_t     *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
			if (lcuParams->potentialLogoLcu &&lcuParams->isCompleteLcu)

			{
				EB_U8 *yMeanPtr = pictureControlSetPtr->yMean[lcuIndex];
				EB_U8 *crMeanPtr = pictureControlSetPtr->crMean[lcuIndex];
				EB_U8 *cbMeanPtr = pictureControlSetPtr->cbMean[lcuIndex];

				EB_U8 rasterScanCuIndex;

				for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
					EB_U8 cuIndex = rasterScanCuIndex - 5;
					EB_U8 x = cuIndex & 3;
					EB_U8 y = (cuIndex >> 2);
					EB_S32 gradx = 0;
					EB_S32 grady = 0;
					EB_S32 nbcompx = 0;
					EB_S32 nbcompy = 0;
					if (x != 0)
					{
						gradx += ABS((EB_S32)(yMeanPtr[rasterScanCuIndex]) - (EB_S32)(yMeanPtr[rasterScanCuIndex - 1]));
						gradx += ABS((EB_S32)(crMeanPtr[rasterScanCuIndex]) - (EB_S32)(crMeanPtr[rasterScanCuIndex - 1]));
						gradx += ABS((EB_S32)(cbMeanPtr[rasterScanCuIndex]) - (EB_S32)(cbMeanPtr[rasterScanCuIndex - 1]));
						nbcompx++;
					}
					if (x != 3)
					{
						gradx += ABS((EB_S32)(yMeanPtr[rasterScanCuIndex + 1]) - (EB_S32)(yMeanPtr[rasterScanCuIndex]));
						gradx += ABS((EB_S32)(crMeanPtr[rasterScanCuIndex + 1]) - (EB_S32)(crMeanPtr[rasterScanCuIndex]));
						gradx += ABS((EB_S32)(cbMeanPtr[rasterScanCuIndex + 1]) - (EB_S32)(cbMeanPtr[rasterScanCuIndex]));
						nbcompx++;
					}
					gradx = gradx / nbcompx;


					if (y != 0)
					{
						grady += ABS((EB_S32)(yMeanPtr[rasterScanCuIndex]) - (EB_S32)(yMeanPtr[rasterScanCuIndex - 4]));
						grady += ABS((EB_S32)(crMeanPtr[rasterScanCuIndex]) - (EB_S32)(crMeanPtr[rasterScanCuIndex - 4]));
						grady += ABS((EB_S32)(cbMeanPtr[rasterScanCuIndex]) - (EB_S32)(cbMeanPtr[rasterScanCuIndex - 4]));
						nbcompy++;
					}
					if (y != 3)
					{
						grady += ABS((EB_S32)(yMeanPtr[rasterScanCuIndex + 4]) - (EB_S32)(yMeanPtr[rasterScanCuIndex]));
						grady += ABS((EB_S32)(crMeanPtr[rasterScanCuIndex + 4]) - (EB_S32)(crMeanPtr[rasterScanCuIndex]));
						grady += ABS((EB_S32)(cbMeanPtr[rasterScanCuIndex + 4]) - (EB_S32)(cbMeanPtr[rasterScanCuIndex]));

						nbcompy++;
					}

					grady = grady / nbcompy;
                    
                    contextPtr->grad[lcuIndex][rasterScanCuIndex] = (EB_U16) (ABS(gradx) + ABS(grady));
					if (contextPtr->grad[lcuIndex][rasterScanCuIndex] > maxGrad){
						maxGrad = contextPtr->grad[lcuIndex][rasterScanCuIndex];
					}
				}
			}
		}

		for (lcuIndex = 0; lcuIndex < totalLcuCount; lcuIndex++) {
			LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
			if (lcuParams->potentialLogoLcu &&lcuParams->isCompleteLcu){
				LcuStat_t *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

				EB_U32 rasterScanCuIndex;
				for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
					lcuStatPtr->cuStatArray[rasterScanCuIndex].edgeCu = (EB_U16)MIN(((contextPtr->grad[lcuIndex][rasterScanCuIndex] * (255*3)) / maxGrad), 255) < 30 ? 0 : 1;
				}
			}
		}
	}
	else{
		for (lcuIndex = 0; lcuIndex < totalLcuCount; lcuIndex++) {

			LcuStat_t *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

			EB_MEMSET(lcuStatPtr, 0, sizeof(LcuStat_t));
		}
	}
}

/******************************************************
* Edge map derivation
******************************************************/
static void EdgeDetection(
	SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr)
{

	EB_U16  *variancePtr;
	EB_U64 thrsldLevel0 = (pictureControlSetPtr->picAvgVariance * 70) / 100;
	EB_U8  *meanPtr;
	EB_U32 pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
	EB_U32 pictureHeightInLcu = (sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
	EB_U32 neighbourLcuIndex = 0;
	EB_U64 similarityCount = 0;
	EB_U64 similarityCount0 = 0;
	EB_U64 similarityCount1 = 0;
	EB_U64 similarityCount2 = 0;
	EB_U64 similarityCount3 = 0;
	EB_U32 lcu_X = 0;
	EB_U32 lcu_Y = 0;
	EB_U32 lcuIndex;
	EB_BOOL highVarianceLucFlag;
    	
	EB_U32 rasterScanCuIndex = 0;
	EB_U32 numberOfEdgeLcu = 0;
	EB_BOOL highIntensityLcuFlag;

	EB_U64 neighbourLcuMean;
	EB_S32 i, j;

	EB_U8 highIntensityTh = 180;
	EB_U8 lowIntensityTh  = 120;
    EB_U8 highIntensityTh1   = 200;
    EB_U8 veryLowIntensityTh =  20;

    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

        lcu_X = lcuParams->horizontalIndex;
        lcu_Y = lcuParams->verticalIndex;

        EdgeLcuResults_t *edgeResultsPtr = pictureControlSetPtr->edgeResultsPtr;
        pictureControlSetPtr->edgeResultsPtr[lcuIndex].edgeBlockNum = 0;
        pictureControlSetPtr->edgeResultsPtr[lcuIndex].isolatedHighIntensityLcu = 0; 
        pictureControlSetPtr->sharpEdgeLcuFlag[lcuIndex] = 0;

		if (lcu_X >  0 && lcu_X < (EB_U32)(pictureWidthInLcu - 1) && lcu_Y >  0 && lcu_Y < (EB_U32)(pictureHeightInLcu - 1)){

			variancePtr = pictureControlSetPtr->variance[lcuIndex];
		    meanPtr = pictureControlSetPtr->yMean[lcuIndex];


			similarityCount = 0;

			highVarianceLucFlag =
				(variancePtr[RASTER_SCAN_CU_INDEX_64x64] > thrsldLevel0) ? EB_TRUE : EB_FALSE;
            edgeResultsPtr[lcuIndex].edgeBlockNum = highVarianceLucFlag;
            if (variancePtr[0] > highIntensityTh1){
                EB_U8 sharpEdge = 0;
                for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
                    sharpEdge = (variancePtr[rasterScanCuIndex] < veryLowIntensityTh) ? sharpEdge + 1 : sharpEdge;
                    
                }
                if (sharpEdge > 4)
                {
                    pictureControlSetPtr->sharpEdgeLcuFlag[lcuIndex] = 1;
                }
            }


			if (lcu_X > 3 && lcu_X < (EB_U32)(pictureWidthInLcu - 4) && lcu_Y >  3 && lcu_Y < (EB_U32)(pictureHeightInLcu - 4)){

				highIntensityLcuFlag =
					(meanPtr[RASTER_SCAN_CU_INDEX_64x64] > highIntensityTh) ? EB_TRUE : EB_FALSE;

				if (highIntensityLcuFlag){

					neighbourLcuIndex = lcuIndex - 1;
					neighbourLcuMean = pictureControlSetPtr->yMean[neighbourLcuIndex][RASTER_SCAN_CU_INDEX_64x64];

					similarityCount0 = (neighbourLcuMean < lowIntensityTh) ? 1 : 0;

					neighbourLcuIndex = lcuIndex + 1;

					neighbourLcuMean = pictureControlSetPtr->yMean[neighbourLcuIndex][RASTER_SCAN_CU_INDEX_64x64];
					similarityCount1 = (neighbourLcuMean < lowIntensityTh) ? 1 : 0;

					neighbourLcuIndex = lcuIndex - pictureWidthInLcu;
					neighbourLcuMean = pictureControlSetPtr->yMean[neighbourLcuIndex][RASTER_SCAN_CU_INDEX_64x64];
					similarityCount2 = (neighbourLcuMean < lowIntensityTh) ? 1 : 0;

					neighbourLcuIndex = lcuIndex + pictureWidthInLcu;
					neighbourLcuMean = pictureControlSetPtr->yMean[neighbourLcuIndex][RASTER_SCAN_CU_INDEX_64x64];
					similarityCount3 = (neighbourLcuMean < lowIntensityTh) ? 1 : 0;

					similarityCount = similarityCount0 + similarityCount1 + similarityCount2 + similarityCount3;
									
					if (similarityCount > 0){


						for (i = -4; i < 5; i++){
							for (j = -4; j < 5; j++){
								neighbourLcuIndex = lcuIndex + (i * pictureWidthInLcu) + j;
                                pictureControlSetPtr->edgeResultsPtr[neighbourLcuIndex].isolatedHighIntensityLcu = 1;
							}
						}
					}
				}
			}


            if (highVarianceLucFlag){  
                numberOfEdgeLcu += edgeResultsPtr[lcuIndex].edgeBlockNum;
			}
		}
	}

	pictureControlSetPtr->lcuBlockPercentage = (EB_U8)((numberOfEdgeLcu * 100) / pictureControlSetPtr->lcuTotalCount);

	return;
}

/******************************************************
* Calculate the variance of variance to determine Homogeneous regions. Note: Variance calculation should be on.
******************************************************/
static inline void DetermineHomogeneousRegionInPicture(
    SequenceControlSet_t            *sequenceControlSetPtr,
    PictureParentControlSet_t       *pictureControlSetPtr)
{

    EB_U16  *variancePtr;
    EB_U32 lcuIndex;

    EB_U32 cuNum, cuSize, cuIndexOffset, cuH, cuW;
    EB_U64 nullVarCnt = 0;
    EB_U64 veryLowVarCnt = 0;
    EB_U64 varLcuCnt = 0;
    EB_U32 lcuTotalCount = pictureControlSetPtr->lcuTotalCount;

    for (lcuIndex = 0; lcuIndex < lcuTotalCount; ++lcuIndex) {
        EB_U64 meanSqrVariance32x32Based[4] = { 0 }, meanVariance32x32Based[4] = { 0 };

        EB_U64 meanSqrVariance64x64Based = 0, meanVariance64x64Based = 0;
        EB_U64 varOfVar64x64Based = 0;

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

        // Initialize
        pictureControlSetPtr->lcuHomogeneousAreaArray[lcuIndex] = EB_TRUE;

        variancePtr = pictureControlSetPtr->variance[lcuIndex];

        if (lcuParams->isCompleteLcu){

            nullVarCnt += (variancePtr[ME_TIER_ZERO_PU_64x64] == 0) ? 1 : 0;
		
            varLcuCnt++;

            veryLowVarCnt += ((variancePtr[ME_TIER_ZERO_PU_64x64]) < LCU_LOW_VAR_TH) ? 1 : 0;
            cuSize = 8;
            cuIndexOffset = ME_TIER_ZERO_PU_8x8_0;
            cuNum = 64 / cuSize;

            //Variance of 8x8 blocks in a 32x32
            for (cuH = 0; cuH < (cuNum / 2); cuH++){
                for (cuW = 0; cuW < (cuNum / 2); cuW++){

                    meanSqrVariance32x32Based[0] += (variancePtr[cuIndexOffset + cuH*cuNum + cuW])*(variancePtr[cuIndexOffset + cuH*cuNum + cuW]);
                    meanVariance32x32Based[0] += (variancePtr[cuIndexOffset + cuH*cuNum + cuW]);

                    meanSqrVariance32x32Based[1] += (variancePtr[cuIndexOffset + cuH*cuNum + cuW + 4])*(variancePtr[cuIndexOffset + cuH*cuNum + cuW + 4]);
                    meanVariance32x32Based[1] += (variancePtr[cuIndexOffset + cuH*cuNum + cuW + 4]);

                    meanSqrVariance32x32Based[2] += (variancePtr[cuIndexOffset + (cuH + 4)*cuNum + cuW])*(variancePtr[cuIndexOffset + (cuH + 4)*cuNum + cuW]);
                    meanVariance32x32Based[2] += (variancePtr[cuIndexOffset + (cuH + 4)*cuNum + cuW]);

                    meanSqrVariance32x32Based[3] += (variancePtr[cuIndexOffset + (cuH + 4)*cuNum + cuW + 4])*(variancePtr[cuIndexOffset + (cuH + 4)*cuNum + cuW + 4]);
                    meanVariance32x32Based[3] += (variancePtr[cuIndexOffset + (cuH + 4)*cuNum + cuW + 4]);

                }
            }

            meanSqrVariance32x32Based[0] = meanSqrVariance32x32Based[0] >> 4;
            meanVariance32x32Based[0] = meanVariance32x32Based[0] >> 4;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][0] = meanSqrVariance32x32Based[0] - meanVariance32x32Based[0] * meanVariance32x32Based[0];

            meanSqrVariance32x32Based[1] = meanSqrVariance32x32Based[1] >> 4;
            meanVariance32x32Based[1] = meanVariance32x32Based[1] >> 4;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][1] = meanSqrVariance32x32Based[1] - meanVariance32x32Based[1] * meanVariance32x32Based[1];

            meanSqrVariance32x32Based[2] = meanSqrVariance32x32Based[2] >> 4;
            meanVariance32x32Based[2] = meanVariance32x32Based[2] >> 4;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][2] = meanSqrVariance32x32Based[2] - meanVariance32x32Based[2] * meanVariance32x32Based[2];

            meanSqrVariance32x32Based[3] = meanSqrVariance32x32Based[3] >> 4;
            meanVariance32x32Based[3] = meanVariance32x32Based[3] >> 4;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][3] = meanSqrVariance32x32Based[3] - meanVariance32x32Based[3] * meanVariance32x32Based[3];

            // Compute the 64x64 based variance of variance
            {
                EB_U32 varIndex;
                // Loop over all 8x8s in a 64x64
                for (varIndex = ME_TIER_ZERO_PU_8x8_0; varIndex <= ME_TIER_ZERO_PU_8x8_63; varIndex++) {
                    meanSqrVariance64x64Based += variancePtr[varIndex] * variancePtr[varIndex];
                    meanVariance64x64Based += variancePtr[varIndex];
                }

                meanSqrVariance64x64Based = meanSqrVariance64x64Based >> 6;
                meanVariance64x64Based = meanVariance64x64Based >> 6;

                // Compute variance
                varOfVar64x64Based = meanSqrVariance64x64Based - meanVariance64x64Based * meanVariance64x64Based;

                // Turn off detail preservation if the varOfVar is greater than a threshold
                if (varOfVar64x64Based > VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)
                {
                    pictureControlSetPtr->lcuHomogeneousAreaArray[lcuIndex] = EB_FALSE;
                }
            }

        }
        else{

            // Should be re-calculated and scaled properly
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][0] = 0xFFFFFFFFFFFFFFFF;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][1] = 0xFFFFFFFFFFFFFFFF;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][2] = 0xFFFFFFFFFFFFFFFF;
            pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][3] = 0xFFFFFFFFFFFFFFFF;
        }
    }
    pictureControlSetPtr->veryLowVarPicFlag = EB_FALSE;
    if (varLcuCnt > 0) {
        if (((veryLowVarCnt * 100) / varLcuCnt) > PIC_LOW_VAR_PERCENTAGE_TH) {
            pictureControlSetPtr->veryLowVarPicFlag = EB_TRUE;
        }
    }

    pictureControlSetPtr->logoPicFlag = EB_FALSE;
    if (varLcuCnt > 0) {
        if (((veryLowVarCnt * 100) / varLcuCnt) > 80) {
            pictureControlSetPtr->logoPicFlag = EB_TRUE;
        }
    }

    return;
}

/************************************************
 * ComputePictureSpatialStatistics
 ** Compute Block Variance
 ** Compute Picture Variance
 ** Compute Block Mean for all blocks in the picture
 ************************************************/
static void ComputePictureSpatialStatistics(
	SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
    PictureAnalysisContext_t        *contextPtr,
	EbPictureBufferDesc_t           *inputPicturePtr,
	EbPictureBufferDesc_t           *inputPaddedPicturePtr,
	EB_U32                           lcuTotalCount)
{
	EB_U32 lcuIndex;
	EB_U32 lcuOriginX;        // to avoid using child PCS
	EB_U32 lcuOriginY;
	EB_U32 inputLumaOriginIndex;
	EB_U32 inputCbOriginIndex;
	EB_U32 inputCrOriginIndex;
	EB_U64 picTotVariance;

	// Variance
	picTotVariance = 0;

	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {
        LcuParams_t   *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

		lcuOriginX = lcuParams->originX;
		lcuOriginY = lcuParams->originY;
		inputLumaOriginIndex = (inputPaddedPicturePtr->originY + lcuOriginY) * inputPaddedPicturePtr->strideY +
			inputPaddedPicturePtr->originX + lcuOriginX;

		inputCbOriginIndex = ((inputPicturePtr->originY + lcuOriginY) >> 1) * inputPicturePtr->strideCb + ((inputPicturePtr->originX + lcuOriginX) >> 1);
		inputCrOriginIndex = ((inputPicturePtr->originY + lcuOriginY) >> 1) * inputPicturePtr->strideCr + ((inputPicturePtr->originX + lcuOriginX) >> 1);

		ComputeBlockMeanComputeVariance(
			pictureControlSetPtr,
			inputPaddedPicturePtr,
			lcuIndex,
			inputLumaOriginIndex);

        if (lcuParams->isCompleteLcu){

			ComputeChromaBlockMean(
				pictureControlSetPtr,
				inputPicturePtr,
				lcuIndex,
				inputCbOriginIndex,
				inputCrOriginIndex);
		}
		else{
			ZeroOutChromaBlockMean(
				pictureControlSetPtr,
				lcuIndex);
		}

		picTotVariance += (pictureControlSetPtr->variance[lcuIndex][RASTER_SCAN_CU_INDEX_64x64]);
	}

	pictureControlSetPtr->picAvgVariance = (EB_U16) (picTotVariance / lcuTotalCount);
    // Calculate the variance of variance to determine Homogeneous regions. Note: Variance calculation should be on.
    DetermineHomogeneousRegionInPicture(
        sequenceControlSetPtr,
        pictureControlSetPtr);

    EdgeDetectionMeanLumaChroma16x16(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        contextPtr,
        sequenceControlSetPtr->lcuTotalCount);

	EdgeDetection(
		sequenceControlSetPtr,
		pictureControlSetPtr);


	return;
}

static void CalculateInputAverageIntensity(
    SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	EbPictureBufferDesc_t           *inputPicturePtr,
	EB_U64                           sumAverageIntensityTotalRegionsLuma,
	EB_U64                           sumAverageIntensityTotalRegionsCb,
	EB_U64                           sumAverageIntensityTotalRegionsCr)
{

    if (sequenceControlSetPtr->scdMode == SCD_MODE_0){
        EB_U16 blockIndexInWidth;
        EB_U16 blockIndexInHeight;
        EB_U64 mean = 0;

        const EB_U16 strideY = inputPicturePtr->strideY;

        // Loop over 8x8 blocks and calculates the mean value
        for (blockIndexInHeight = 0; blockIndexInHeight < inputPicturePtr->height >> 3; ++blockIndexInHeight) {
            for (blockIndexInWidth = 0; blockIndexInWidth < inputPicturePtr->width >> 3; ++blockIndexInWidth) {
                mean += ComputeSubMean8x8_SSE2_INTRIN(&(inputPicturePtr->bufferY[(blockIndexInWidth << 3) + (blockIndexInHeight << 3) * strideY]), strideY);
            }
        }

        mean = ((mean + ((inputPicturePtr->height* inputPicturePtr->width) >> 7)) / ((inputPicturePtr->height* inputPicturePtr->width) >> 6));
        mean = (mean + (1 << (MEAN_PRECISION - 1))) >> MEAN_PRECISION;
        pictureControlSetPtr->averageIntensity[0] = (EB_U8)mean;
    }

    else{
        pictureControlSetPtr->averageIntensity[0] = (EB_U8)((sumAverageIntensityTotalRegionsLuma + ((inputPicturePtr->width*inputPicturePtr->height) >> 1)) / (inputPicturePtr->width*inputPicturePtr->height));
        pictureControlSetPtr->averageIntensity[1] = (EB_U8)((sumAverageIntensityTotalRegionsCb + ((inputPicturePtr->width*inputPicturePtr->height) >> 3)) / ((inputPicturePtr->width*inputPicturePtr->height) >> 2));
        pictureControlSetPtr->averageIntensity[2] = (EB_U8)((sumAverageIntensityTotalRegionsCr + ((inputPicturePtr->width*inputPicturePtr->height) >> 3)) / ((inputPicturePtr->width*inputPicturePtr->height) >> 2));
    }
    
    return;
}

/************************************************
 * Gathering statistics per picture
 ** Calculating the pixel intensity histogram bins per picture needed for SCD
 ** Computing Picture Variance
 ************************************************/
static void GatheringPictureStatistics(
	SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
    PictureAnalysisContext_t        *contextPtr,
	EbPictureBufferDesc_t           *inputPicturePtr,
	EbPictureBufferDesc_t           *inputPaddedPicturePtr,
	EbPictureBufferDesc_t			*sixteenthDecimatedPicturePtr,
	EB_U32                           lcuTotalCount)
{

	EB_U64                          sumAverageIntensityTotalRegionsLuma = 0;
	EB_U64                          sumAverageIntensityTotalRegionsCb = 0;
	EB_U64                          sumAverageIntensityTotalRegionsCr = 0;

	// Histogram bins
   // Use 1/16 Luma for Histogram generation
   // 1/16 input ready 
   SubSampleLumaGeneratePixelIntensityHistogramBins(
       sequenceControlSetPtr,
       pictureControlSetPtr,
       sixteenthDecimatedPicturePtr,
       &sumAverageIntensityTotalRegionsLuma);

   // Use 1/4 Chroma for Histogram generation
   // 1/4 input not ready => perform operation on the fly 
   SubSampleChromaGeneratePixelIntensityHistogramBins(
       sequenceControlSetPtr,
       pictureControlSetPtr,
       inputPicturePtr,
       &sumAverageIntensityTotalRegionsCb,
       &sumAverageIntensityTotalRegionsCr);
    
	// Calculate the LUMA average intensity
    CalculateInputAverageIntensity(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        inputPicturePtr,
        sumAverageIntensityTotalRegionsLuma,
        sumAverageIntensityTotalRegionsCb,
        sumAverageIntensityTotalRegionsCr);

	ComputePictureSpatialStatistics(
		sequenceControlSetPtr,
		pictureControlSetPtr,
        contextPtr,
		inputPicturePtr,
		inputPaddedPicturePtr,
		lcuTotalCount);

	return;
}

/************************************************
 * Pad Picture at the right and bottom sides
 ** To match a multiple of min CU size in width and height
 ************************************************/
static void PadPictureToMultipleOfMinCuSizeDimensions(
	SequenceControlSet_t            *sequenceControlSetPtr,
	EbPictureBufferDesc_t           *inputPicturePtr)
{
    EB_BOOL is16BitInput = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);
    EB_U32 colorFormat = inputPicturePtr->colorFormat;
    EB_U16 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

	// Input Picture Padding
	PadInputPicture(
		&inputPicturePtr->bufferY[inputPicturePtr->originX + (inputPicturePtr->originY * inputPicturePtr->strideY)],
		inputPicturePtr->strideY,
		(inputPicturePtr->width - sequenceControlSetPtr->padRight),
		(inputPicturePtr->height - sequenceControlSetPtr->padBottom),
		sequenceControlSetPtr->padRight,
		sequenceControlSetPtr->padBottom);

	PadInputPicture(
		&inputPicturePtr->bufferCb[(inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCb)],
		inputPicturePtr->strideCb,
        (inputPicturePtr->width - sequenceControlSetPtr->padRight) >> subWidthCMinus1,
        (inputPicturePtr->height - sequenceControlSetPtr->padBottom) >> subHeightCMinus1,
        sequenceControlSetPtr->padRight >> subWidthCMinus1,
        sequenceControlSetPtr->padBottom >> subHeightCMinus1);

	PadInputPicture(
		&inputPicturePtr->bufferCr[(inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCr)],
		inputPicturePtr->strideCr,
        (inputPicturePtr->width - sequenceControlSetPtr->padRight) >> subWidthCMinus1,
        (inputPicturePtr->height - sequenceControlSetPtr->padBottom) >> subHeightCMinus1,
        sequenceControlSetPtr->padRight >> subWidthCMinus1,
        sequenceControlSetPtr->padBottom >> subHeightCMinus1);

    if (is16BitInput) {
        PadInputPicture(
            &inputPicturePtr->bufferBitIncY[inputPicturePtr->originX + (inputPicturePtr->originY * inputPicturePtr->strideBitIncY)],
            inputPicturePtr->strideBitIncY,
            (inputPicturePtr->width - sequenceControlSetPtr->padRight),
            (inputPicturePtr->height - sequenceControlSetPtr->padBottom),
            sequenceControlSetPtr->padRight,
            sequenceControlSetPtr->padBottom);

        PadInputPicture(
			&inputPicturePtr->bufferBitIncCb[(inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideBitIncCb)],
            inputPicturePtr->strideBitIncCb,
            (inputPicturePtr->width - sequenceControlSetPtr->padRight) >> subWidthCMinus1,
            (inputPicturePtr->height - sequenceControlSetPtr->padBottom) >> subHeightCMinus1,
            sequenceControlSetPtr->padRight >> subWidthCMinus1,
            sequenceControlSetPtr->padBottom >> subHeightCMinus1);

        PadInputPicture(
			&inputPicturePtr->bufferBitIncCr[(inputPicturePtr->originX >> subWidthCMinus1) + ((inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideBitIncCr)],
            inputPicturePtr->strideBitIncCr,
            (inputPicturePtr->width - sequenceControlSetPtr->padRight) >> subWidthCMinus1,
            (inputPicturePtr->height - sequenceControlSetPtr->padBottom) >> subHeightCMinus1,
            sequenceControlSetPtr->padRight >> subWidthCMinus1,
            sequenceControlSetPtr->padBottom >> subHeightCMinus1);

    }

	return;
}

/************************************************
 * Pad Picture at the right and bottom sides
 ** To complete border LCU smaller than LCU size
 ************************************************/
static void PadPictureToMultipleOfLcuDimensions(
	EbPictureBufferDesc_t           *inputPaddedPicturePtr
        )
{

	// Generate Padding
	GeneratePadding(
		&inputPaddedPicturePtr->bufferY[0],
		inputPaddedPicturePtr->strideY,
		inputPaddedPicturePtr->width,
		inputPaddedPicturePtr->height,
		inputPaddedPicturePtr->originX,
		inputPaddedPicturePtr->originY);

	return;
}

/************************************************
* 1/4 & 1/16 input picture decimation
************************************************/
static void DecimateInputPicture(
    SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	EbPictureBufferDesc_t           *inputPaddedPicturePtr,
	EbPictureBufferDesc_t           *quarterDecimatedPicturePtr,
	EbPictureBufferDesc_t           *sixteenthDecimatedPicturePtr) {

    // Decimate input picture for HME L1
    EB_BOOL  preformQuarterPellDecimationFlag;
    if (sequenceControlSetPtr->staticConfig.speedControlFlag){    
        preformQuarterPellDecimationFlag = EB_TRUE;
    }
    else{
        if (pictureControlSetPtr->enableHmeLevel1Flag == 1){
            preformQuarterPellDecimationFlag = EB_TRUE;
        }
        else{
            preformQuarterPellDecimationFlag = EB_FALSE;
        }
    }

    if (preformQuarterPellDecimationFlag) {
        Decimation2D(
		        &inputPaddedPicturePtr->bufferY[inputPaddedPicturePtr->originX + inputPaddedPicturePtr->originY * inputPaddedPicturePtr->strideY],
		        inputPaddedPicturePtr->strideY,
		        inputPaddedPicturePtr->width ,
		        inputPaddedPicturePtr->height,
		        &quarterDecimatedPicturePtr->bufferY[quarterDecimatedPicturePtr->originX+quarterDecimatedPicturePtr->originY*quarterDecimatedPicturePtr->strideY],
		        quarterDecimatedPicturePtr->strideY,
		        2);

            GeneratePadding(
		        &quarterDecimatedPicturePtr->bufferY[0],
		        quarterDecimatedPicturePtr->strideY,
		        quarterDecimatedPicturePtr->width,
		        quarterDecimatedPicturePtr->height,
		        quarterDecimatedPicturePtr->originX,
		        quarterDecimatedPicturePtr->originY);

	}

    // Decimate input picture for HME L0
	// Sixteenth Input Picture Decimation 
    Decimation2D(
		&inputPaddedPicturePtr->bufferY[inputPaddedPicturePtr->originX + inputPaddedPicturePtr->originY * inputPaddedPicturePtr->strideY],
		inputPaddedPicturePtr->strideY,
		inputPaddedPicturePtr->width ,
		inputPaddedPicturePtr->height ,
		&sixteenthDecimatedPicturePtr->bufferY[sixteenthDecimatedPicturePtr->originX+sixteenthDecimatedPicturePtr->originY*sixteenthDecimatedPicturePtr->strideY],
		sixteenthDecimatedPicturePtr->strideY,
		4);

    GeneratePadding(
		&sixteenthDecimatedPicturePtr->bufferY[0],
		sixteenthDecimatedPicturePtr->strideY,
		sixteenthDecimatedPicturePtr->width,
		sixteenthDecimatedPicturePtr->height,
		sixteenthDecimatedPicturePtr->originX,
		sixteenthDecimatedPicturePtr->originY);
}

/************************************************
 * Picture Analysis Kernel
 * The Picture Analysis Process pads & decimates the input pictures.
 * The Picture Analysis also includes creating an n-bin Histogram,
 * gathering picture 1st and 2nd moment statistics for each 8x8 block,
 * which are used to compute variance.
 * The Picture Analysis process is multithreaded, so pictures can be
 * processed out of order as long as all inputs are available.
 ************************************************/
void* PictureAnalysisKernel(void *inputPtr)
{
	PictureAnalysisContext_t        *contextPtr = (PictureAnalysisContext_t*)inputPtr;
	PictureParentControlSet_t       *pictureControlSetPtr;
	SequenceControlSet_t            *sequenceControlSetPtr;

	EbObjectWrapper_t               *inputResultsWrapperPtr;
	ResourceCoordinationResults_t   *inputResultsPtr;
	EbObjectWrapper_t               *outputResultsWrapperPtr;
	PictureAnalysisResults_t        *outputResultsPtr;
	EbPaReferenceObject_t           *paReferenceObject;

	EbPictureBufferDesc_t           *inputPaddedPicturePtr;
	EbPictureBufferDesc_t           *quarterDecimatedPicturePtr;
	EbPictureBufferDesc_t           *sixteenthDecimatedPicturePtr;
	EbPictureBufferDesc_t           *inputPicturePtr;

	// Variance    
	EB_U32                          pictureWidthInLcu;
	EB_U32                          pictureHeighInLcu;
	EB_U32                          lcuTotalCount;

	for (;;) {

		// Get Input Full Object
		EbGetFullObject(
			contextPtr->resourceCoordinationResultsInputFifoPtr,
			&inputResultsWrapperPtr);
        EB_CHECK_END_OBJ(inputResultsWrapperPtr);

		inputResultsPtr = (ResourceCoordinationResults_t*)inputResultsWrapperPtr->objectPtr;
		pictureControlSetPtr = (PictureParentControlSet_t*)inputResultsPtr->pictureControlSetWrapperPtr->objectPtr;
		sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
		inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld PA IN \n", pictureControlSetPtr->pictureNumber);
#endif
		paReferenceObject = (EbPaReferenceObject_t*)pictureControlSetPtr->paReferencePictureWrapperPtr->objectPtr;
		inputPaddedPicturePtr = (EbPictureBufferDesc_t*)paReferenceObject->inputPaddedPicturePtr;
		quarterDecimatedPicturePtr = (EbPictureBufferDesc_t*)paReferenceObject->quarterDecimatedPicturePtr;
		sixteenthDecimatedPicturePtr = (EbPictureBufferDesc_t*)paReferenceObject->sixteenthDecimatedPicturePtr;

		// Variance  
		pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
		pictureHeighInLcu = (sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
		lcuTotalCount = pictureWidthInLcu * pictureHeighInLcu;

        // Set picture parameters to account for subpicture, picture scantype, and set regions by resolutions
		SetPictureParametersForStatisticsGathering(
			sequenceControlSetPtr);

		// Pad pictures to multiple min cu size
		PadPictureToMultipleOfMinCuSizeDimensions(
			sequenceControlSetPtr,
			inputPicturePtr);

		// Pre processing operations performed on the input picture 
        PicturePreProcessingOperations(
            pictureControlSetPtr,
            contextPtr,
            sequenceControlSetPtr,
            quarterDecimatedPicturePtr,
            sixteenthDecimatedPicturePtr,
            lcuTotalCount,
            pictureWidthInLcu);
	
        if (inputPicturePtr->colorFormat >= EB_YUV422) {
            // Jing: Do the conversion of 422/444=>420 here since it's multi-threaded kernel
            //       Reuse the Y, only add cb/cr in the newly created buffer desc
            //       NOTE: since denoise may change the src, so this part is after PicturePreProcessingOperations()
            pictureControlSetPtr->chromaDownSamplePicturePtr->bufferY = inputPicturePtr->bufferY;
            DownSampleChroma(inputPicturePtr, pictureControlSetPtr->chromaDownSamplePicturePtr);
        } else {
            pictureControlSetPtr->chromaDownSamplePicturePtr = inputPicturePtr;
        }

		// Pad input picture to complete border LCUs
		PadPictureToMultipleOfLcuDimensions(
			inputPaddedPicturePtr
        );
        
		// 1/4 & 1/16 input picture decimation 
		DecimateInputPicture(
            sequenceControlSetPtr,
			pictureControlSetPtr,
			inputPaddedPicturePtr,
			quarterDecimatedPicturePtr,
			sixteenthDecimatedPicturePtr);

		// Gathering statistics of input picture, including Variance Calculation, Histogram Bins
		GatheringPictureStatistics(
			sequenceControlSetPtr,
			pictureControlSetPtr,
            contextPtr,
			pictureControlSetPtr->chromaDownSamplePicturePtr, //420 inputPicturePtr
			inputPaddedPicturePtr,
			sixteenthDecimatedPicturePtr,
			lcuTotalCount);


		// Hold the 64x64 variance and mean in the reference frame 
		EB_U32 lcuIndex;
		for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex){
			paReferenceObject->variance[lcuIndex] = pictureControlSetPtr->variance[lcuIndex][ME_TIER_ZERO_PU_64x64];
			paReferenceObject->yMean[lcuIndex] = pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_64x64];

		}

		// Get Empty Results Object
		EbGetEmptyObject(
			contextPtr->pictureAnalysisResultsOutputFifoPtr,
			&outputResultsWrapperPtr);

		outputResultsPtr = (PictureAnalysisResults_t*)outputResultsWrapperPtr->objectPtr;
		outputResultsPtr->pictureControlSetWrapperPtr = inputResultsPtr->pictureControlSetWrapperPtr;

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld PA OUT \n", pictureControlSetPtr->pictureNumber);
#endif

		// Release the Input Results
		EbReleaseObject(inputResultsWrapperPtr);

#if LATENCY_PROFILE
        double latency = 0.0;
        EB_U64 finishTimeSeconds = 0;
        EB_U64 finishTimeuSeconds = 0;
        EbFinishTime((uint64_t*)&finishTimeSeconds, (uint64_t*)&finishTimeuSeconds);

        EbComputeOverallElapsedTimeMs(
                pictureControlSetPtr->startTimeSeconds,
                pictureControlSetPtr->startTimeuSeconds,
                finishTimeSeconds,
                finishTimeuSeconds,
                &latency);

        SVT_LOG("POC %lld PA OUT, decoder order %d, latency %3.3f \n",
                pictureControlSetPtr->pictureNumber,
                pictureControlSetPtr->decodeOrder,
                latency);
#endif
		// Post the Full Results Object
		EbPostFullObject(outputResultsWrapperPtr);

	}
	return EB_NULL;
}

void UnusedVariablevoidFunc_PA()
{
	(void)SadCalculation_8x8_16x16_funcPtrArray;
	(void)SadCalculation_32x32_64x64_funcPtrArray;
}
