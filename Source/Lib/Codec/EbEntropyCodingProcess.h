/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingProcess_h
#define EbEntropyCodingProcess_h

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
#include "EbCodingUnit.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Enc Dec Context
 **************************************/
typedef struct EntropyCodingContext_s 
{
    EbFifo_t                       *encDecInputFifoPtr;
    EbFifo_t                       *entropyCodingOutputFifoPtr;  // to packetization
    EbFifo_t                       *rateControlOutputFifoPtr; // feedback to rate control

    EB_U32                          lcuTotalCount; 
	// Lambda
	EB_U8                           qp;
	EB_U8                           chromaQp;
    // Coding Unit Workspace---------------------------
    EbPictureBufferDesc_t          *coeffBufferTB;                              //Used to hold quantized coeff for one TB in EncPass.

    //  Context Variables---------------------------------
    EB_U32                          cuItr;
    CodingUnit_t                   *cuPtr;
    const CodedUnitStats_t         *cuStats;
    EB_U32                          cuIndex;
    EB_U8                           cuDepth; 
    EB_U32                          cuSize; 
    EB_U32                          lcuSize;      
    EB_U32                          lcuChromaSize;      
    EB_U32                          cuSizeLog2;
    EB_U32                          cuOriginX;      
    EB_U32                          cuOriginY;      
    EB_U32                          cuChromaOriginX;
    EB_U32                          cuChromaOriginY;

    EB_U32                          puItr;
    PredictionUnit_t               *puPtr;
    const PredictionUnitStats_t    *puStats;
    EB_U32                          puOriginX;
    EB_U32                          puOriginY;
    EB_U32                          puWidth;
    EB_U32                          puHeight;
    MvUnit_t                        mvUnit;
    EB_U32                          mergeCandidateCount;
    MvMergeCandidate_t              mergeCandidateArray[MAX_NUM_OF_MV_MERGE_CANDIDATE];
    EB_U32                          amvpCandidateCountRefList0;
    EB_S32                          xMvAmvpCandidateArrayList0[MAX_NUM_OF_AMVP_CANDIDATES];
    EB_S32                          yMvAmvpCandidateArrayList0[MAX_NUM_OF_AMVP_CANDIDATES];
    EB_U32                          amvpCandidateCountRefList1;
    EB_S32                          xMvAmvpCandidateArrayList1[MAX_NUM_OF_AMVP_CANDIDATES];
    EB_S32                          yMvAmvpCandidateArrayList1[MAX_NUM_OF_AMVP_CANDIDATES];
    
    EB_U32                          tuItr;
    EB_U32                          tuArea;
    EB_U32                          tuTotalArea;
    TransformUnit_t                *tuPtr;
    const TransformUnitStats_t     *tuStats;
    EB_U32                          tuOriginX;      
    EB_U32                          tuOriginY;     
    EB_U32                          tuSize;
    EB_U32                          tuSizeChroma;
    
    // MCP Context
    EB_BOOL                         is16bit; //enable 10 bit encode in CL

} EntropyCodingContext_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EntropyCodingContextCtor(
    EntropyCodingContext_t **contextDblPtr,
    EbFifo_t                *encDecInputFifoPtr,
    EbFifo_t                *packetizationOutputFifoPtr,
    EbFifo_t                *rateControlOutputFifoPtr,
    EB_BOOL                  is16bit);
    
extern void* EntropyCodingKernel(void *inputPtr);

#ifdef __cplusplus
}
#endif
#endif // EbEntropyCodingProcess_h
