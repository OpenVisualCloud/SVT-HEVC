/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSourceBasedOperations_h
#define EbSourceBasedOperations_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbNoiseExtractAVX2.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CuComplexityInfo_s
{
	EB_U32 spatioTemporalWeight;
	EB_U32 currentTemporalComplexity;
	EB_U32 currentSpatialComplexity;
	EB_U64 spatioTemporalComplexity;
	EB_U32 currentMotionComplexity;
	EB_U32 currentspatioTemporalComplexity;
	EB_U32 spatialComplexity;
	EB_U32 temporalComplexity;
	EB_U8  complextityDecreaseFactor;
} CuComplexityInfo_t;


/**************************************
 * Context
 **************************************/

typedef struct SourceBasedOperationsContext_s
{
	EbFifo_t *initialrateControlResultsInputFifoPtr;
	EbFifo_t *pictureDemuxResultsOutputFifoPtr;

	// Delta QP Map
	EB_S8     minDeltaQP;
	EB_U8     maxDeltaQP;

	EB_S16    minDeltaQpWeight[3][4];
	EB_S16    maxDeltaQpWeight[3][4];



	// Skin
	EB_U8    grassPercentageInPicture;
	EB_U8    picturePercentageFaceSkinLcu;

	 // local zz cost array
	EB_U32  pictureNumGrassLcu;

	EB_U32  highContrastNum;

	EB_BOOL highDist;
	EB_U32  countOfMovingLcus;
	EB_U32  countOfNonMovingLcus;
	EB_U64  yNonMovingMean;
	EB_U64  yMovingMean;
	EB_U32  toBeIntraCodedProbability;
	EB_U32  depth1BlockNum;
    EB_U8   *yMeanPtr;
	EB_U8   *crMeanPtr;
	EB_U8   *cbMeanPtr;

} SourceBasedOperationsContext_t;

/***************************************
 * Extern Function Declaration
 ***************************************/

extern EB_ERRORTYPE SourceBasedOperationsContextCtor(
    SourceBasedOperationsContext_t **contextDblPtr,
    EbFifo_t						*initialrateControlResultsInputFifoPtr,
    EbFifo_t					    *pictureDemuxResultsOutputFifoPtr);

extern void* SourceBasedOperationsKernel(void *inputPtr);

#ifdef __cplusplus
}
#endif
#endif // EbSourceBasedOperations_h