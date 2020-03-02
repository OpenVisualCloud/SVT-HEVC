/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbModeDecision_h
#define EbModeDecision_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbSyntaxElements.h"
#include "EbPictureBufferDesc.h"
//#include "EbMdRateEstimation.h"
#include "EbAdaptiveMotionVectorPrediction.h"
#include "EbPictureOperators.h"
#include "EbNeighborArrays.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
#define ENABLE_AMVP_MV_FOR_RC_PU   0

#define MAX_MPM_CANDIDATES		   3

	static const EB_U32 tuIndexList[3][16] =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4 },
		{ 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 }
	};

#define MERGE_PENALTY                          10

	// Create incomplete struct definition for the following function pointer typedefs
	struct ModeDecisionCandidateBuffer_s;
	struct ModeDecisionContext_s;

	/**************************************
	* Function Ptrs Definitions
	**************************************/
typedef EB_ERRORTYPE(*EB_PREDICTION_FUNC)(
	    struct ModeDecisionContext_s           *contextPtr,
		EB_U32                                  componentMask,
		PictureControlSet_t                    *pictureControlSetPtr,
	    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr);

	typedef EB_ERRORTYPE(*EB_FAST_COST_FUNC)(
        struct ModeDecisionContext_s           *contextPtr,
		CodingUnit_t                           *cuPtr,
	struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
		EB_U32                                  qp,
		EB_U64                                  lumaDistortion,
		EB_U64                                  chromaDistortion,
		EB_U64                                  lambda,
		PictureControlSet_t                    *pictureControlSetPtr);

	typedef EB_ERRORTYPE(*EB_FULL_COST_FUNC)(
		LargestCodingUnit_t					   *lcuPtr,
		CodingUnit_t                           *cuPtr,
		EB_U32                                  cuSize,
		EB_U32                                  cuSizeLog2,
	struct ModeDecisionCandidateBuffer_s       *candidateBufferPtr,
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

	typedef EB_ERRORTYPE(*EB_FULL_LUMA_COST_FUNC)(
		CodingUnit_t                           *cuPtr,
		EB_U32                                  cuSize,
		EB_U32                                  cuSizeLog2,
	struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
		EB_U64                                 *yDistortion,
		EB_U64                                  lambda,
		EB_U64                                 *yCoeffBits,
		EB_U32                                  transformSize);

	/**************************************
	* Mode Decision Candidate
	**************************************/
	typedef struct ModeDecisionCandidate_s
	{
		// *Warning - this struct has been organized to be cache efficient when being
		//    constructured in the function GenerateAmvpMergeInterIntraMdCandidatesCU.
		//    Changing the ordering could affect performance

		union {
			struct {
				unsigned                        meDistortion    : 20;
				unsigned                        distortionReady : 1;
				unsigned                                        : 3;
				unsigned                        intraLumaMode   : 8;
			};
			EB_U32 oisResults;
		};
        union {
            struct{
                union {

                    struct{
                        EB_S16              motionVector_x_L0;  //Note: Do not change the order of these fields
                        EB_S16              motionVector_y_L0;
                    };
                    EB_U32 MVsL0;
                };
                union {

                    struct{
                        EB_S16              motionVector_x_L1;  //Note: Do not change the order of these fields
                        EB_S16              motionVector_y_L1;
                    };
                    EB_U32 MVsL1;
                };
            };
            EB_U64 MVs;
        };

		EB_U8                                   skipFlag;
		EB_BOOL                                 mergeFlag;
		EB_U8                                   mergeIndex;
        EB_U16                                  countNonZeroCoeffs;
		EB_BOOL                                 predictionIsReady;
		EB_BOOL                                 predictionIsReadyLuma;
		EB_U8                                   type;
        EB_BOOL									mpmFlag;

		// MD Rate Estimation Ptr
		MdRateEstimationContext_t              *mdRateEstimationPtr; // 64 bits
		EB_U64                                  fastLumaRate;
		EB_U64                                  fastChromaRate;
		EB_U64                                  chromaDistortion;
		EB_U64                                  chromaDistortionInterDepth;
		EB_U32                                  lumaDistortion;
		EB_U32                                  fullDistortion;
        EB_U32									fastLoopLumaDistortion;

		// Function Pointers
		EB_PTR                                  predictionContextPtr;
		EB_FAST_COST_FUNC                       fastCostFuncPtr;
		EB_FULL_COST_FUNC                       fullCostFuncPtr;
		EB_FULL_LUMA_COST_FUNC                  fullLumaCostFuncPtr;
		EB_PREDDIRECTION                        predictionDirection[MAX_NUM_OF_PU_PER_CU]; // 2 bits


		EB_S16                                  motionVectorPred_x[MAX_NUM_OF_REF_PIC_LIST]; // 16 bits
		EB_S16                                  motionVectorPred_y[MAX_NUM_OF_REF_PIC_LIST]; // 16 bits
		EB_U8                                   motionVectorPredIdx[MAX_NUM_OF_REF_PIC_LIST]; // 2 bits
		EB_U8                                   transformSize;       // 8 bit
		EB_U8                                   transformChromaSize; // 8 bit
		EB_U8                                   rootCbf;             // ?? bit - determine empirically
		EB_U8                                   cbCbf;               // ?? bit
		EB_U8                                   crCbf;               // ?? bit
        EB_U32                                  yCbf;                // Issue, should be less than 32


	} ModeDecisionCandidate_t;

	/**************************************
	* Mode Decision Candidate Buffer
	**************************************/
	typedef struct IntraChromaCandidateBuffer_s {
		EB_U32                                  mode;
		EB_U64                                  cost;
		EB_U64                                  distortion;

		EbPictureBufferDesc_t                  *predictionPtr;
		EbPictureBufferDesc_t                  *residualPtr;

	} IntraChromaCandidateBuffer_t;

	/**************************************
	* Mode Decision Candidate Buffer
	**************************************/
	typedef struct ModeDecisionCandidateBuffer_s {
        EbDctor                                 dctor;
		// Candidate Ptr
		ModeDecisionCandidate_t                *candidatePtr;

		// Video Buffers
		EbPictureBufferDesc_t                  *predictionPtr;
		EbPictureBufferDesc_t                  *residualQuantCoeffPtr;// One buffer for residual and quantized coefficient
		EbPictureBufferDesc_t                  *reconCoeffPtr;

		// *Note - We should be able to combine the reconCoeffPtr & reconPtr pictures (they aren't needed at the same time)
		EbPictureBufferDesc_t                  *reconPtr;

		// Distortion (SAD)
		EB_U64                                  residualLumaSad;

		EB_U64									fullLambdaRate;
        EB_U64                                  fullCostLuma;

		// Costs
		EB_U64                                 *fastCostPtr;
		EB_U64                                 *fullCostPtr;
		EB_U64                                 fullCostPlusCuBoundDist;	// fullCostPlusCuBoundDist= fullCost + CU boundaries distortion if derived otherwise fullCostPlusCuBoundDist = fullCost

		EB_U64                                 *fullCostSkipPtr;
		EB_U64                                 *fullCostMergePtr;
		//
		EB_U64                                 cbCoeffBits;
		EB_U64                                 cbDistortion[2];
		EB_U64                                 crCoeffBits;
		EB_U64                                 crDistortion[2];
		EB_U64                                 fullCostNoAc;
		CoeffCtxtMdl_t                         candBuffCoeffCtxModel;
        EB_BOOL                                weightChromaDistortion;
        EB_U64                                 yFullDistortion[DIST_CALC_TOTAL];
        EB_U64                                 yCoeffBits;
		EB_S16                                 yDc[4];// Store the ABS of DC values per TU. If one TU, stored in 0, otherwise 4 tus stored in 0 to 3
        EB_U16                                 yCountNonZeroCoeffs[4];// Store nonzero CoeffNum, per TU. If one TU, stored in 0, otherwise 4 tus stored in 0 to 3

	} ModeDecisionCandidateBuffer_t;

	/**************************************
	* Extern Function Declarations
	**************************************/
extern EB_ERRORTYPE ModeDecisionCandidateBufferCtor(
		ModeDecisionCandidateBuffer_t  *bufferPtr,
		EB_U16                          lcuMaxSize,
		EB_BITDEPTH                     maxBitdepth,
		EB_U64                         *fastCostPtr,
		EB_U64                         *fullCostPtr,
		EB_U64                         *fullCostSkipPtr,
		EB_U64                         *fullCostMergePtr);


    EB_ERRORTYPE ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
		LargestCodingUnit_t            *lcuPtr,
                struct ModeDecisionContext_s   *contextPtr,
		const EB_U32                    leafIndex,
		const EB_U32                    lcuAddr,
		EB_U32                         *bufferTotalCount,
		EB_U32                         *fastCandidateTotalCount,
		EB_PTR                          interPredContextPtr,
		PictureControlSet_t            *pictureControlSetPtr,
		EB_BOOL							mpmSearch,
		EB_U8	                        mpmSearchCandidate,
		EB_U32                         *mostProbableModeArray);



	EB_U8 ProductFullModeDecision(
        struct ModeDecisionContext_s   *contextPtr,
		CodingUnit_t                   *cuPtr,
		EB_U8                           cuSize,
		ModeDecisionCandidateBuffer_t **bufferPtrArray,
		EB_U32                          candidateTotalCount,
		EB_U8                          *bestCandidateIndexArray,
		EB_U32                         *bestIntraMode);

	EB_ERRORTYPE PreModeDecision(
		CodingUnit_t                   *cuPtr,
		EB_U32                          bufferTotalCount,
		ModeDecisionCandidateBuffer_t **bufferPtrArray,
		EB_U32                         *fullCandidateTotalCountPtr,
		EB_U8                          *bestCandidateIndexArray,
		EB_U8                          *disableMergeIndex,
        EB_BOOL                         sameFastFullCandidate);

	typedef EB_ERRORTYPE(*EB_INTRA_4x4_FAST_LUMA_COST_FUNC)(
		struct ModeDecisionContext_s           *contextPtr,
		EB_U32                                  puIndex,
	    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
		EB_U64                                  lumaDistortion,
		EB_U64                                  lambda);

	typedef EB_ERRORTYPE(*EB_INTRA_4x4_FULL_LUMA_COST_FUNC)(
	    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
		EB_U64                                 *yDistortion,
		EB_U64                                  lambda,
		EB_U64                                 *yCoeffBits,
		EB_U32                                  transformSize);

	typedef EB_ERRORTYPE(*EB_INTRA_NxN_FAST_COST_FUNC)(
		CodingUnit_t                           *cuPtr,
	    struct ModeDecisionCandidateBuffer_s   *candidateBufferPtr,
		EB_U32                                  qp,
		EB_U64                                  lumaDistortion,
		EB_U64                                  chromaDistortion,
		EB_U64                                  lambda,
		PictureControlSet_t                    *pictureControlSetPtr);

typedef EB_ERRORTYPE(*EB_INTRA_NxN_FULL_COST_FUNC)(
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


	typedef EB_ERRORTYPE(*EB_FULL_NXN_COST_FUNC)(
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

	struct CodingLoopContext_s;
#ifdef __cplusplus
}
#endif
#endif // EbModeDecision_h
