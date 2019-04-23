/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbCodingUnit_h
#define EbCodingUnit_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"
#include "EbMotionEstimationLcuResults.h"
#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbPredictionUnit.h"
#include "EbTransformUnit.h"
#include "EbCabacContextModel.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
Requirements:
-Must have enough CodingUnits for every single CU pattern
-Easy to expand/insert CU
-Easy to collapse a CU
-Easy to replace CUs
-Statically Allocated
-Contains the leaf count

*/

// Macros for deblocking filter
#define MAX_LCU_SIZE_IN_4X4BLK                                  (MAX_LCU_SIZE >> 2)
#define VERTICAL_EDGE_BS_ARRAY_SIZE                             (MAX_LCU_SIZE_IN_4X4BLK * MAX_LCU_SIZE_IN_4X4BLK)
#define HORIZONTAL_EDGE_BS_ARRAY_SIZE                           (MAX_LCU_SIZE_IN_4X4BLK * MAX_LCU_SIZE_IN_4X4BLK)

#define MAX_NUMBER_OF_BS_EDGES_PER_TREEBLOCK    128
#define MAX_NUMBER_OF_LEAFS_PER_TREEBLOCK       64
#define MAX_NUMBER_OF_4x4_TUs_IN_8x8_LEAF       4

#define SAO_BO_INTERVALS                                32
#define SAO_BO_LEN                                      4
#define SAO_EO_TYPES                                    4
#define SAO_EO_CATEGORIES                               4
#define MIN_SAO_OFFSET_VALUE                          -31// -7
#define MAX_SAO_OFFSET_VALUE                           31// 7
#define NUMBER_SAO_OFFSETS                              4
#define SAO_COMPONENT_LUMA                              0
#define SAO_COMPONENT_CHROMA                            1
#define SAO_COMPONENT_CHROMA_CB                         2
#define SAO_COMPONENT_CHROMA_CR                         3

struct PictureControlSet_s;

#define MAX_CU_COST (0xFFFFFFFFFFFFFFFFull >> 1)
#define INVALID_FAST_CANDIDATE_INDEX	~0

#define MAX_OIS_0   7 // when I Slice 
#define MAX_OIS_1   9 // when P/B Slice and oisKernelLevel = 0 
#define MAX_OIS_2  18 // when P/B Slice and oisKernelLevel = 1 

typedef struct CodingUnit_s 
{
    TransformUnit_t             transformUnitArray[TRANSFORM_UNIT_MAX_COUNT]; // 2-bytes * 21 = 42-bytes
    PredictionUnit_t            predictionUnitArray[MAX_NUM_OF_PU_PER_CU];    // 35-bytes * 4 = 140 bytes

    unsigned                    skipFlagContext             : 2;
    unsigned                    predictionModeFlag          : 2;
    unsigned                    rootCbf                     : 1;
    unsigned                    splitFlagContext            : 2;
    unsigned                    qp                          : 6;
    unsigned                    refQp                       : 6;

	signed 						deltaQp						: 8; // can be signed 8bits
	signed 						orgDeltaQp					: 8;



        // Coded Tree
	struct {
		unsigned                   leafIndex : 8;
		unsigned                   splitFlag : 1;
		unsigned                   skipFlag  : 1;

	};

} CodingUnit_t;


typedef struct OisCandidate_s {
    union {
        struct {
            unsigned distortion         :   20;
            unsigned validDistortion    :   1;
            unsigned                    :   3;
            unsigned intraMode          :   8;
        };
        EB_U32 oisResults;
    };
} OisCandidate_t;

typedef struct OisLcuResults_s 
{
    EB_U8           totalIntraLumaMode[CU_MAX_COUNT];
    OisCandidate_t  sortedOisCandidate[CU_MAX_COUNT][MAX_OIS_2];
} OisLcuResults_t;



typedef struct OisCu32Cu16Results_s
{
	EB_U8            totalIntraLumaMode[21]; 
	OisCandidate_t*  sortedOisCandidate[21];   
	
} OisCu32Cu16Results_t;

typedef struct OisCu8Results_s
{
	EB_U8            totalIntraLumaMode[64];
	OisCandidate_t*  sortedOisCandidate[64];

} OisCu8Results_t;


typedef struct SaoStats_s {

   	EB_S32                        **boDiff;
    EB_U16                        **boCount;
	EB_S32                          eoDiff[3][SAO_EO_TYPES][SAO_EO_CATEGORIES+1];
    EB_U16                          eoCount[3][SAO_EO_TYPES][SAO_EO_CATEGORIES+1];
    EB_S32                         *eoDiff1D;
    EB_U32                         *eoCount1D;

} SaoStats_t;

typedef struct SaoParameters_s {

    // SAO
    EB_BOOL                         saoMergeLeftFlag;
    EB_BOOL                         saoMergeUpFlag;
    EB_U32                          saoTypeIndex[2]; 
    EB_S32                          saoOffset[3][4];
    EB_U32                          saoBandPosition[3];

} SaoParameters_t;

typedef struct QpmLcuResults_s {
    EB_U8  cuQP;
	EB_U8  cuIntraQP;
    EB_U8  cuInterQP;
    EB_S8  deltaQp;
	EB_S8  innerLcuCudeltaQp;

} QpmLcuResults_t;


typedef struct EdgeLcuResults_s {
    EB_U8  edgeBlockNum;
	EB_U8  isolatedHighIntensityLcu;

} EdgeLcuResults_t;

typedef struct LargestCodingUnit_s {
    struct PictureControlSet_s     *pictureControlSetPtr;
    CodingUnit_t                  **codedLeafArrayPtr; 
    
    // Coding Units   
    EB_AURA_STATUS                  auraStatus; 

    unsigned     qp                                 : 8;
    unsigned     size                               : 8;
    unsigned     sizeLog2                           : 3;
    unsigned     pictureLeftEdgeFlag                : 1;
    unsigned     pictureTopEdgeFlag                 : 1;
    unsigned     pictureRightEdgeFlag               : 1;
    unsigned     pred64                             : 2;

    unsigned     index                              : 14; // supports up to 8k resolution
    unsigned     originX                            : 13; // supports up to 8k resolution 8191
    unsigned     originY                            : 13; // supports up to 8k resolution 8191

    // SAO
    SaoParameters_t                 saoParams;

    //Bits only used for quantized coeffs
    EB_U32                          quantizedCoeffsBits;
    EB_U32                          totalBits;

    // Quantized Coefficients
    EbPictureBufferDesc_t          *quantizedCoeff;
    EB_U8                           intra4x4Mode[256];
    EB_U8                           preMdcRefinementLevel;

	EB_U8							chromaEncodeMode;

    EB_INTRA4x4_SEARCH_METHOD       intra4x4SearchMethod;
#if TILES
    // Tiles
    EB_BOOL                         tileLeftEdgeFlag;
    EB_BOOL                         tileTopEdgeFlag;
    EB_BOOL                         tileRightEdgeFlag;
    EB_U32                          tileOriginX;
    EB_U32                          tileOriginY;
#endif

} LargestCodingUnit_t;







extern EB_ERRORTYPE LargestCodingUnitCtor(
    LargestCodingUnit_t        **largetCodingUnitDblPtr,
    EB_U8                        lcuSize,
    EB_U32                       pictureWidth,
    EB_U32                       pictureHeight,
    EB_U16                       lcuOriginX,
    EB_U16                       lcuOriginY,
    EB_U16                       lcuIndex,
    struct PictureControlSet_s  *pictureControlSet);

#ifdef __cplusplus
}
#endif
#endif // EbCodingUnit_h
