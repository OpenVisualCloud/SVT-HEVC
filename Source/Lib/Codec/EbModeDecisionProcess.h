/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbModeDecisionProcess_h
#define EbModeDecisionProcess_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"

#include "EbSystemResourceManager.h"
#include "EbPictureBufferDesc.h"
#include "EbModeDecision.h"
#include "EbInterPrediction.h"
#include "EbEntropyCoding.h"
#include "EbTransQuantBuffers.h"
#include "EbReferenceObject.h"
#include "EbNeighborArrays.h"


//#include "EbEncDecProcess.h"

#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Defines
 **************************************/
#define MODE_DECISION_CANDIDATE_MAX_COUNT               84

#define MODE_DECISION_CANDIDATE_BUFFER_MAX_COUNT        34

#define MAX_FULL_LOOP_CANIDATES_PER_DEPTH                7 // 4 NFL + 3 MPM

#define DEPTH_ONE_STEP   21
#define DEPTH_TWO_STEP    5
#define DEPTH_THREE_STEP  1

/**************************************
 * Macros
 **************************************/

#define GROUP_OF_4_8x8_BLOCKS(originX,originY) (((originX >> 3) & 0x1) && ((originY >> 3) & 0x1) ? EB_TRUE : EB_FALSE)
#define GROUP_OF_4_16x16_BLOCKS(originX,originY) (((((originX >> 3) & 0x2) == 0x2) && (((originY >> 3) & 0x2) == 0x2)) ? EB_TRUE : EB_FALSE)
#define GROUP_OF_4_32x32_BLOCKS(originX,originY) (((((originX >> 3) & 0x4) == 0x4) && (((originY >> 3) & 0x4) == 0x4)) ? EB_TRUE : EB_FALSE)

/**************************************
 * Coding Loop Context
 **************************************/
typedef struct MDEncPassCuData_s
{
    EB_U64       skipCost;
    EB_U64       mergeCost;
    EB_U64       chromaDistortion;
    EB_U64       yFullDistortion[DIST_CALC_TOTAL];
    EB_U64       yCoeffBits;
    EB_U32       yCbf;
    EB_U64       fastLumaRate;
    EB_S16        yDc[4];// Store the ABS of DC values per TU. If one TU, stored in 0, otherwise 4 tus stored in 0 to 3
    EB_U16        yCountNonZeroCoeffs[4];// Store nonzero CoeffNum, per TU. If one TU, stored in 0, otherwise 4 tus stored in 0 to 3

} MDEncPassCuData_t;    
typedef struct LcuBasedDetectors_s
{
    unsigned intraInterCond1                      : 1; // intra / inter bias
    unsigned intraInterCond2                      : 1; // intra / inter bias
	unsigned intraInterCond3                      : 1; // intra / inter bias
    unsigned intraInterCond4                      : 1;
    unsigned biPredCond1                          : 1; // FastLoop

    EB_BOOL  enableContouringQCUpdateFlag32x32[4];  // Anticontouring Condition for 32x32 in LCU

}LcuBasedDetectors_t;

typedef struct MdCodingUnit_s
{
	unsigned					testedCuFlag : 1;   //tells whether this CU is tested in MD.
	unsigned                    mdcArrayIndex : 7;
	unsigned                    countNonZeroCoeffs : 11;
	unsigned                    topNeighborDepth : 2;
	unsigned                    leftNeighborDepth : 2;
	unsigned                    topNeighborMode : 2;
	unsigned                    leftNeighborMode : 2;
	unsigned                    fullDistortion : 32;
	unsigned                    chromaDistortion : 32;
	unsigned                    chromaDistortionInterDepth : 32;
	EB_U64						cost;
	EB_U64						costLuma;

}MdCodingUnit_t;

typedef struct ModeDecisionContext_s
{
	EbFifo_t                       *modeDecisionConfigurationInputFifoPtr;
	EbFifo_t                       *modeDecisionOutputFifoPtr;

	EB_S16                         *transformInnerArrayPtr;

	ModeDecisionCandidate_t       **fastCandidatePtrArray;
	ModeDecisionCandidate_t        *fastCandidateArray;

    // TMVP
	EbReferenceObject_t            *referenceObjectWritePtr;

    // Coding Unit Workspace---------------------------
	EbPictureBufferDesc_t          *predictionBuffer;

    // Intra Reference Samples
	IntraReferenceSamples_t        *intraRefPtr;

    ModeDecisionCandidateBuffer_t **candidateBufferPtrArray;

	InterPredictionContext_t       *interPredictionContext;

	MdRateEstimationContext_t      *mdRateEstimationPtr;

	SaoStats_t                     *saoStats;

    // Transform and Quantization Buffers
	EbTransQuantBuffers_t			*transQuantBuffersPtr;

    // MCP Context
	MotionCompensationPredictionContext_t *mcpContext;

    struct EncDecContext_s         *encDecContextPtr;

	EB_U64                         *fastCostArray;
	EB_U64                         *fullCostArray;
	EB_U64                         *fullCostSkipPtr;
	EB_U64                         *fullCostMergePtr;

	

	// Fast loop buffers
	EB_U8                           bufferDepthIndexStart[MAX_LEVEL_COUNT];
	EB_U8                           bufferDepthIndexWidth[MAX_LEVEL_COUNT];

	// Lambda
	EB_U8                           qp;
	EB_U8                           chromaQp;
	EB_U32                          fastLambda;
	EB_U32                          fullLambda;
	EB_U32                          fastChromaLambda;
	EB_U32                          fullChromaLambda;
	EB_U32                          fullChromaLambdaSao;
	
	//  Context Variables---------------------------------

    LargestCodingUnit_t            *lcuPtr;
    TransformUnit_t                *tuPtr;
	const TransformUnitStats_t     *tuStats;
    CodingUnit_t                   *cuPtr;
	const CodedUnitStats_t         *cuStats;
    PredictionUnit_t               *puPtr;
	const PredictionUnitStats_t    *puStats;

    MvUnit_t                        mvUnit;
	MvMergeCandidate_t              mergeCandidateArray[MAX_NUM_OF_MV_MERGE_CANDIDATE];

    // Inter depth decision
    EB_U8                           mpmSearchCandidate;
	EB_U8                           groupOf8x8BlocksCount;
	EB_U8                           groupOf16x16BlocksCount;
    EB_U8                           puItr;
    EB_U8                           cuSizeLog2;
    EB_U8                           amvpCandidateCountRefList0;
	EB_U8                           amvpCandidateCountRefList1;

	EB_U8                           bestCandidateIndexArray[MAX_FULL_LOOP_CANIDATES_PER_DEPTH];

	EB_U16                          cuOriginX;
	EB_U16                          cuOriginY;
	EB_S16                          xMvAmvpCandidateArrayList0[MAX_NUM_OF_AMVP_CANDIDATES];
	EB_S16                          yMvAmvpCandidateArrayList0[MAX_NUM_OF_AMVP_CANDIDATES];
	EB_S16                          xMvAmvpCandidateArrayList1[MAX_NUM_OF_AMVP_CANDIDATES];
	EB_S16                          yMvAmvpCandidateArrayList1[MAX_NUM_OF_AMVP_CANDIDATES];
	
    EB_U32	                        mostProbableModeArray[MAX_MPM_CANDIDATES];
	EB_U32                          useChromaInformationInFastLoop;                                

    EB_BOOL                         useChromaInformationInFullLoop;
    EB_U8                           cuDepth;
	EB_U8                           cuSize;
	EB_U8                           lcuSize;
	EB_U8                           lcuChromaSize;
	EB_U16                          cuChromaOriginX;
	EB_U16                          cuChromaOriginY;
    EB_U16                          puOriginX;
	EB_U16                          puOriginY;
    EB_U16                          puWidth;
	EB_U16                          puHeight;

	EB_PF_MODE                      pfMdMode;

    EB_BOOL                         mpmSearch;
    EB_BOOL                         useIntraInterBias;
    EB_BOOL                         lumaIntraRefSamplesGenDone;
    EB_BOOL                         chromaIntraRefSamplesGenDone;
    EB_BOOL                         roundMvToInteger;

    EB_U8                           intraLumaLeftModeArray[4];
    EB_U8                           intraLumaTopModeArray[4];

    // Entropy Coder
	EntropyCoder_t                 *coeffEstEntropyCoderPtr;
	CabacCost_t                    *CabacCost;
    SyntaxContextModelEncContext_t  syntaxCabacCtxModelArray;

    MDEncPassCuData_t               mdEpPipeLcu[CU_MAX_COUNT];    
    LcuBasedDetectors_t            *mdPicLcuDetect;

    EbPictureBufferDesc_t          *pillarReconBuffer;
    EbPictureBufferDesc_t          *mdReconBuffer;

    EB_U32                          fullReconSearchCount;
    EB_U32                          mvMergeSkipModeCount;

    NeighborArrayUnit_t            *intraLumaModeNeighborArray;
    NeighborArrayUnit_t            *mvNeighborArray;
    NeighborArrayUnit_t            *skipFlagNeighborArray;
    NeighborArrayUnit_t            *modeTypeNeighborArray;
    NeighborArrayUnit_t            *leafDepthNeighborArray;
	NeighborArrayUnit_t            *lumaReconNeighborArray;
    NeighborArrayUnit_t            *cbReconNeighborArray;
    NeighborArrayUnit_t            *crReconNeighborArray;
 
    BdpCuData_t                     pillarCuArray;
    EB_BOOL                         cu8x8RefinementOnFlag;
    EB_BOOL                         depthRefinment ;
    EB_BOOL                         conformantMvMergeTable;
 
	EB_BOOL                         edgeBlockNumFlag;
	MdCodingUnit_t					mdLocalCuUnit[CU_MAX_COUNT];
	EB_BOOL                         cuUseRefSrcFlag; 
    EB_BOOL                         restrictIntraGlobalMotion;    
    EB_U8                           interpolationMethod;

    EB_BOOL                         coeffCabacUpdate;
    CoeffCtxtMdl_t                  latestValidCoeffCtxModel; //tracks the best CU partition so far
    CoeffCtxtMdl_t                  i4x4CoeffCtxModel;

	EB_BOOL                         use4x4ChromaInformationInFullLoop;

    // Multi-modes signal(s) 
    EB_U8                           intraInjectionMethod;
    EB_BOOL                         spatialSseFullLoop;
    EB_BOOL                         intra8x8RestrictionInterSlice;
    EB_BOOL                         generateAmvpTableMd;
    EB_BOOL	                        fullLoopEscape;
    EB_BOOL                         singleFastLoopFlag;
    EB_BOOL                         amvpInjection;
    EB_BOOL                         unipred3x3Injection;
    EB_BOOL                         bipred3x3Injection;
    EB_RDOQ_PMCORE_TYPE             rdoqPmCoreMethod;
    EB_BOOL                         enableExitPartitioning;
    EB_U8                           chromaLevel;
    EB_BOOL                         intraMdOpenLoopFlag;
    EB_BOOL                         limitIntra;
    EB_U8                           mpmLevel;
    EB_U8                           pfMdLevel;
    EB_U8                           intra4x4Level;
    EB_U8                           intra4x4Nfl;
    EB_U8                           intra4x4IntraInjection;
    EB_U8                           nmmLevelMd;
    EB_U8                           nmmLevelBdp;
    EB_U8                           nflLevelMd;
    EB_U8                           nflLevelPillar8x8ref;
    EB_U8                           nflLevelMvMerge64x64ref;

    EB_U16                          tileIndex;
} ModeDecisionContext_t;

typedef void(*EB_LAMBDA_ASSIGN_FUNC)(
    PictureParentControlSet_t *pictureControlSetPtr,
    EB_U32                    *fastLambda,
    EB_U32                    *fullLambda,
    EB_U32                    *fastChromaLambda,
    EB_U32                    *fullChromaLambda,
    EB_U32                    *fullChromaLambdaSao,
    EB_U8                      qp,
    EB_U8                      chromaQp);



/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE ModeDecisionContextCtor(
    ModeDecisionContext_t  **contextDblPtr,
    EbFifo_t                *modeDecisionConfigurationInputFifoPtr,
    EbFifo_t                *modeDecisionOutputFifoPtr,
    EB_BOOL                  is16bit);


//extern void ResetModeDecisionNeighborArrays(
//        PictureControlSet_t *pictureControlSetPtr,
//        EB_U32 tileIndex);

extern void  ApplyIntraInterModeBias(
    EB_BOOL                        intraInterCond1,
    EB_BOOL                        intraInterCond2,
	EB_BOOL                        intraInterCond3,
	EB_U64                         *fullCostPtr);

extern const EB_LAMBDA_ASSIGN_FUNC lambdaAssignmentFunctionTable[4];

extern void ProductResetModeDecision(
    ModeDecisionContext_t   *contextPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr);

extern void ModeDecisionConfigureLcu(
    ModeDecisionContext_t   *contextPtr,
    LargestCodingUnit_t     *lcuPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr,
    EB_U8                    pictureQp,
    EB_U8                    lcuQp);


#ifdef __cplusplus
}
#endif
#endif // EbModeDecisionProcess_h
