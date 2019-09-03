/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncDecProcess_h
#define EbEncDecProcess_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"

#include "EbModeDecisionProcess.h"

#include "EbSystemResourceManager.h"
#include "EbPictureBufferDesc.h"
#include "EbModeDecision.h"
#include "EbInterPrediction.h"
#include "EbEntropyCoding.h"
#include "EbTransQuantBuffers.h"
#include "EbReferenceObject.h"
#include "EbNeighborArrays.h"
#include "EbCodingUnit.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PM_STRIDE   4
typedef struct EbPMCand_s
{
	
	EB_S16       trCoeff[4 * 4];
	EB_S16       quCoeff[4 * 4];
	EB_S16       iqCoeff[4 * 4];
	EB_U8        maskingLevel;
	EB_U64       cost;
	EB_U32       nzCoeff;

} EbPMCand_t;



/**************************************
 * Enc Dec Context
 **************************************/
typedef struct EncDecContext_s
{
    EbFifo_t                       *modeDecisionInputFifoPtr;
	EbFifo_t                       *encDecOutputFifoPtr;
	EbFifo_t                       *encDecFeedbackFifoPtr;
	EbFifo_t                       *pictureDemuxOutputFifoPtr;   // to picture-manager

	EB_S16                         *transformInnerArrayPtr;
	MdRateEstimationContext_t      *mdRateEstimationPtr;
	SaoStats_t                     *saoStats;

    ModeDecisionContext_t          *mdContext;

	// Lambda
	EB_U8                           qp;
	EB_U8                           chromaQp;
	EB_U32                          fastLambda;
	EB_U32                          fullLambda;
	EB_U32                          fastChromaLambda;
	EB_U32                          fullChromaLambda;
	EB_U32                          fullChromaLambdaSao;

	// TMVP
	EbReferenceObject_t            *referenceObjectWritePtr;

    // MCP Context
	MotionCompensationPredictionContext_t *mcpContext;

    // Intra Reference Samples
	IntraReferenceSamples_t        *intraRefPtr;
	IntraReference16bitSamples_t   *intraRefPtr16;  //We need a different buffer for ENC pass then the MD one.
        

	// Coding Unit Workspace---------------------------
	EbPictureBufferDesc_t          *residualBuffer;
	EbPictureBufferDesc_t          *transformBuffer;
    EbPictureBufferDesc_t          *inputSamples;
	EbPictureBufferDesc_t          *inputSample16bitBuffer;

	//  Context Variables---------------------------------
	CodingUnit_t                   *cuPtr;
	const CodedUnitStats_t         *cuStats;
    EB_U16                          cuOriginX; // within the picture
	EB_U16                          cuOriginY; // within the picture

	EB_U8                           lcuSize;


    EB_U32                          lcuIndex;


	EB_U16                          cuChromaOriginX;
	EB_U16                          cuChromaOriginY;

	MvUnit_t                        mvUnit;
	EB_U8                           amvpCandidateCountRefList0;
	EB_S16                          xMvAmvpCandidateArrayList0[MAX_NUM_OF_AMVP_CANDIDATES];
	EB_S16                          yMvAmvpCandidateArrayList0[MAX_NUM_OF_AMVP_CANDIDATES];
	EB_U8                           amvpCandidateCountRefList1;
	EB_S16                          xMvAmvpCandidateArrayList1[MAX_NUM_OF_AMVP_CANDIDATES];
	EB_S16                          yMvAmvpCandidateArrayList1[MAX_NUM_OF_AMVP_CANDIDATES];

	EB_U8                           tuItr;
	EB_BOOL                         is16bit; //enable 10 bit encode in CL
    EB_COLOR_FORMAT                 colorFormat;
	// SAO application 
	EB_U8                          *saoUpBuffer[2];
	EB_U8                          *saoLeftBuffer[2];
	EB_U16                         *saoUpBuffer16[2];
	EB_U16                         *saoLeftBuffer16[2];

	EB_U64                          totIntraCodedArea;
    EB_U32                          codedLcuCount;
	EB_U8                           intraCodedAreaLCU[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];//percentage of intra coded area 0-100%
    EB_U8					        cleanSparseCeoffPfEncDec;
    EB_U8					        pmpMaskingLevelEncDec;
	EB_U8 						    forceCbfFlag;

	EB_BOOL							skipQpmFlag;
	
    EB_S16							minDeltaQpWeight;
	EB_S16							maxDeltaQpWeight;
	EB_S8							minDeltaQp[4];
	EB_S8							maxDeltaQp[4];
	EB_S8							nonMovingDeltaQp;

	EB_BOOL							grassEnhancementFlag;
	EB_BOOL							backgorundEnhancement;
	EB_U8                           qpmQp;


    EB_TRANS_COEFF_SHAPE            transCoeffShapeLuma;
    EB_TRANS_COEFF_SHAPE            transCoeffShapeChroma;
	

	EbPMCand_t                      pmCandBuffer[5];

    // Multi-modes signal(s)
    EB_BOOL                         allowEncDecMismatch;
    EB_BOOL                         fastEl;
	EB_U32                          yBitsThsld;
    EB_U8                           saoMode;
    EB_PM_MODE  				    pmMode;     // agressive vs. conservative
    EB_BOOL                         pmMethod;   // 1-stgae   vs. 2-stage 

    EB_U16                          encDecTileIndex;
    ////
} EncDecContext_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EncDecContextCtor(
    EncDecContext_t        **contextDblPtr,
    EbFifo_t                *modeDecisionConfigurationInputFifoPtr,
    EbFifo_t                *packetizationOutputFifoPtr,
    EbFifo_t                *feedbackFifoPtr,
    EbFifo_t                *pictureDemuxFifoPtr,
    EB_BOOL                  is16bit,
    EB_COLOR_FORMAT          colorFormat);


    
extern void* EncDecKernel(void *inputPtr);
#ifdef __cplusplus
}
#endif  
#endif // EbEncDecProcess_h
