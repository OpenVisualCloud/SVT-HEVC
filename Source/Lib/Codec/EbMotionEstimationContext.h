/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationContext_h
#define EbMotionEstimationContext_h

#include "EbDefinitions.h"
#include "EbMdRateEstimation.h"
#include "EbCodingUnit.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif

// Max Search Area
#define MAX_SEARCH_AREA_WIDTH   1350 // This should be a function for the MAX HME L0 * the multiplications per layers and per Hierarchichal structures
#define MAX_SEARCH_AREA_HEIGHT  675 // This should be a function for the MAX HME L0 * the multiplications per layers and per Hierarchichal structures

// 1-D interpolation shift value
#define IFShift 6


#define VARIANCE_PRECISION      16

#define MEAN_PRECISION      (VARIANCE_PRECISION >> 1)

#define HME_RECTANGULAR    0
#define HME_SPARSE         1

// Quater pel refinement methods
typedef enum EB_QUARTER_PEL_REFINEMENT_METHOD {
    EB_QUARTER_IN_FULL,
    EB_QUARTER_IN_HALF_HORIZONTAL,
    EB_QUARTER_IN_HALF_VERTICAL,
    EB_QUARTER_IN_HALF_DIAGONAL
} EB_QUARTER_PEL_INTERPOLATION_METHOD;

// HME parameters

#define HME_DECIM_FILTER_TAP     9

typedef struct MePredictionUnit_s {
    EB_U64  distortion;
    EB_S16  xMv;
    EB_S16  yMv;
    EB_U32  subPelDirection;
} MePredictionUnit_t;

typedef enum EB_ME_TIER_ZERO_PU {

    // 2Nx2N [85 partitions]
    ME_TIER_ZERO_PU_64x64       =    0,
    ME_TIER_ZERO_PU_32x32_0     =    1,
    ME_TIER_ZERO_PU_32x32_1     =    2,
    ME_TIER_ZERO_PU_32x32_2     =    3,
    ME_TIER_ZERO_PU_32x32_3     =    4,
    ME_TIER_ZERO_PU_16x16_0     =    5,
    ME_TIER_ZERO_PU_16x16_1     =    6,
    ME_TIER_ZERO_PU_16x16_2     =    7,
    ME_TIER_ZERO_PU_16x16_3     =    8,
    ME_TIER_ZERO_PU_16x16_4     =    9,
    ME_TIER_ZERO_PU_16x16_5     =    10,
    ME_TIER_ZERO_PU_16x16_6     =    11,
    ME_TIER_ZERO_PU_16x16_7     =    12,
    ME_TIER_ZERO_PU_16x16_8     =    13,
    ME_TIER_ZERO_PU_16x16_9     =    14,
    ME_TIER_ZERO_PU_16x16_10    =    15,
    ME_TIER_ZERO_PU_16x16_11    =    16,
    ME_TIER_ZERO_PU_16x16_12    =    17,
    ME_TIER_ZERO_PU_16x16_13    =    18,
    ME_TIER_ZERO_PU_16x16_14    =    19,
    ME_TIER_ZERO_PU_16x16_15    =    20,
    ME_TIER_ZERO_PU_8x8_0       =    21,
    ME_TIER_ZERO_PU_8x8_1       =    22,
    ME_TIER_ZERO_PU_8x8_2       =    23,
    ME_TIER_ZERO_PU_8x8_3       =    24,
    ME_TIER_ZERO_PU_8x8_4       =    25,
    ME_TIER_ZERO_PU_8x8_5       =    26,
    ME_TIER_ZERO_PU_8x8_6       =    27,
    ME_TIER_ZERO_PU_8x8_7       =    28,
    ME_TIER_ZERO_PU_8x8_8       =    29,
    ME_TIER_ZERO_PU_8x8_9       =    30,
    ME_TIER_ZERO_PU_8x8_10      =    31,
    ME_TIER_ZERO_PU_8x8_11      =    32,
    ME_TIER_ZERO_PU_8x8_12      =    33,
    ME_TIER_ZERO_PU_8x8_13      =    34,
    ME_TIER_ZERO_PU_8x8_14      =    35,
    ME_TIER_ZERO_PU_8x8_15      =    36,
    ME_TIER_ZERO_PU_8x8_16      =    37,
    ME_TIER_ZERO_PU_8x8_17      =    38,
    ME_TIER_ZERO_PU_8x8_18      =    39,
    ME_TIER_ZERO_PU_8x8_19      =    40,
    ME_TIER_ZERO_PU_8x8_20      =    41,
    ME_TIER_ZERO_PU_8x8_21      =    42,
    ME_TIER_ZERO_PU_8x8_22      =    43,
    ME_TIER_ZERO_PU_8x8_23      =    44,
    ME_TIER_ZERO_PU_8x8_24      =    45,
    ME_TIER_ZERO_PU_8x8_25      =    46,
    ME_TIER_ZERO_PU_8x8_26      =    47,
    ME_TIER_ZERO_PU_8x8_27      =    48,
    ME_TIER_ZERO_PU_8x8_28      =    49,
    ME_TIER_ZERO_PU_8x8_29      =    50,
    ME_TIER_ZERO_PU_8x8_30      =    51,
    ME_TIER_ZERO_PU_8x8_31      =    52,
    ME_TIER_ZERO_PU_8x8_32      =    53,
    ME_TIER_ZERO_PU_8x8_33      =    54,
    ME_TIER_ZERO_PU_8x8_34      =    55,
    ME_TIER_ZERO_PU_8x8_35      =    56,
    ME_TIER_ZERO_PU_8x8_36      =    57,
    ME_TIER_ZERO_PU_8x8_37      =    58,
    ME_TIER_ZERO_PU_8x8_38      =    59,
    ME_TIER_ZERO_PU_8x8_39      =    60,
    ME_TIER_ZERO_PU_8x8_40      =    61,
    ME_TIER_ZERO_PU_8x8_41      =    62,
    ME_TIER_ZERO_PU_8x8_42      =    63,
    ME_TIER_ZERO_PU_8x8_43      =    64,
    ME_TIER_ZERO_PU_8x8_44      =    65,
    ME_TIER_ZERO_PU_8x8_45      =    66,
    ME_TIER_ZERO_PU_8x8_46      =    67,
    ME_TIER_ZERO_PU_8x8_47      =    68,
    ME_TIER_ZERO_PU_8x8_48      =    69,
    ME_TIER_ZERO_PU_8x8_49      =    70,
    ME_TIER_ZERO_PU_8x8_50      =    71,
    ME_TIER_ZERO_PU_8x8_51      =    72,
    ME_TIER_ZERO_PU_8x8_52      =    73,
    ME_TIER_ZERO_PU_8x8_53      =    74,
    ME_TIER_ZERO_PU_8x8_54      =    75,
    ME_TIER_ZERO_PU_8x8_55      =    76,
    ME_TIER_ZERO_PU_8x8_56      =    77,
    ME_TIER_ZERO_PU_8x8_57      =    78,
    ME_TIER_ZERO_PU_8x8_58      =    79,
    ME_TIER_ZERO_PU_8x8_59      =    80,
    ME_TIER_ZERO_PU_8x8_60      =    81,
    ME_TIER_ZERO_PU_8x8_61      =    82,
    ME_TIER_ZERO_PU_8x8_62      =    83,
    ME_TIER_ZERO_PU_8x8_63      =    84,
    // 2NxN  [42 partitions]
    ME_TIER_ZERO_PU_64x32_0     =    85,
    ME_TIER_ZERO_PU_64x32_1     =    86,
    ME_TIER_ZERO_PU_32x16_0     =    87,
    ME_TIER_ZERO_PU_32x16_1     =    88,
    ME_TIER_ZERO_PU_32x16_2     =    89,
    ME_TIER_ZERO_PU_32x16_3     =    90,
    ME_TIER_ZERO_PU_32x16_4     =    91,
    ME_TIER_ZERO_PU_32x16_5     =    92,
    ME_TIER_ZERO_PU_32x16_6     =    93,
    ME_TIER_ZERO_PU_32x16_7     =    94,
    ME_TIER_ZERO_PU_16x8_0      =    95,
    ME_TIER_ZERO_PU_16x8_1      =    96,
    ME_TIER_ZERO_PU_16x8_2      =    97,
    ME_TIER_ZERO_PU_16x8_3      =    98,
    ME_TIER_ZERO_PU_16x8_4      =    99,
    ME_TIER_ZERO_PU_16x8_5      =    100,
    ME_TIER_ZERO_PU_16x8_6      =    101,
    ME_TIER_ZERO_PU_16x8_7      =    102,
    ME_TIER_ZERO_PU_16x8_8      =    103,
    ME_TIER_ZERO_PU_16x8_9      =    104,
    ME_TIER_ZERO_PU_16x8_10     =    105,
    ME_TIER_ZERO_PU_16x8_11     =    106,
    ME_TIER_ZERO_PU_16x8_12     =    107,
    ME_TIER_ZERO_PU_16x8_13     =    108,
    ME_TIER_ZERO_PU_16x8_14     =    109,
    ME_TIER_ZERO_PU_16x8_15     =    110,
    ME_TIER_ZERO_PU_16x8_16     =    111,
    ME_TIER_ZERO_PU_16x8_17     =    112,
    ME_TIER_ZERO_PU_16x8_18     =    113,
    ME_TIER_ZERO_PU_16x8_19     =    114,
    ME_TIER_ZERO_PU_16x8_20     =    115,
    ME_TIER_ZERO_PU_16x8_21     =    116,
    ME_TIER_ZERO_PU_16x8_22     =    117,
    ME_TIER_ZERO_PU_16x8_23     =    118,
    ME_TIER_ZERO_PU_16x8_24     =    119,
    ME_TIER_ZERO_PU_16x8_25     =    120,
    ME_TIER_ZERO_PU_16x8_26     =    121,
    ME_TIER_ZERO_PU_16x8_27     =    122,
    ME_TIER_ZERO_PU_16x8_28     =    123,
    ME_TIER_ZERO_PU_16x8_29     =    124,
    ME_TIER_ZERO_PU_16x8_30     =    125,
    ME_TIER_ZERO_PU_16x8_31     =    126,
    // Nx2N  [42 partitions]
    ME_TIER_ZERO_PU_32x64_0     =    127,
    ME_TIER_ZERO_PU_32x64_1     =    128,
    ME_TIER_ZERO_PU_16x32_0     =    129,
    ME_TIER_ZERO_PU_16x32_1     =    130,
    ME_TIER_ZERO_PU_16x32_2     =    131,
    ME_TIER_ZERO_PU_16x32_3     =    132,
    ME_TIER_ZERO_PU_16x32_4     =    133,
    ME_TIER_ZERO_PU_16x32_5     =    134,
    ME_TIER_ZERO_PU_16x32_6     =    135,
    ME_TIER_ZERO_PU_16x32_7     =    136,
    ME_TIER_ZERO_PU_8x16_0      =    137,
    ME_TIER_ZERO_PU_8x16_1      =    138,
    ME_TIER_ZERO_PU_8x16_2      =    139,
    ME_TIER_ZERO_PU_8x16_3      =    140,
    ME_TIER_ZERO_PU_8x16_4      =    141,
    ME_TIER_ZERO_PU_8x16_5      =    142,
    ME_TIER_ZERO_PU_8x16_6      =    143,
    ME_TIER_ZERO_PU_8x16_7      =    144,
    ME_TIER_ZERO_PU_8x16_8      =    145,
    ME_TIER_ZERO_PU_8x16_9      =    146,
    ME_TIER_ZERO_PU_8x16_10     =    147,
    ME_TIER_ZERO_PU_8x16_11     =    148,
    ME_TIER_ZERO_PU_8x16_12     =    149,
    ME_TIER_ZERO_PU_8x16_13     =    150,
    ME_TIER_ZERO_PU_8x16_14     =    151,
    ME_TIER_ZERO_PU_8x16_15     =    152,
    ME_TIER_ZERO_PU_8x16_16     =    153,
    ME_TIER_ZERO_PU_8x16_17     =    154,
    ME_TIER_ZERO_PU_8x16_18     =    155,
    ME_TIER_ZERO_PU_8x16_19     =    156,
    ME_TIER_ZERO_PU_8x16_20     =    157,
    ME_TIER_ZERO_PU_8x16_21     =    158,
    ME_TIER_ZERO_PU_8x16_22     =    159,
    ME_TIER_ZERO_PU_8x16_23     =    160,
    ME_TIER_ZERO_PU_8x16_24     =    161,
    ME_TIER_ZERO_PU_8x16_25     =    162,
    ME_TIER_ZERO_PU_8x16_26     =    163,
    ME_TIER_ZERO_PU_8x16_27     =    164,
    ME_TIER_ZERO_PU_8x16_28     =    165,
    ME_TIER_ZERO_PU_8x16_29     =    166,
    ME_TIER_ZERO_PU_8x16_30     =    167,
    ME_TIER_ZERO_PU_8x16_31     =    168,

    // 2NxnU [10 partitions]
    ME_TIER_ZERO_PU_64x16_0     =    169,
    ME_TIER_ZERO_PU_64x16_1     =    170,
    ME_TIER_ZERO_PU_32x8_0      =    171,
    ME_TIER_ZERO_PU_32x8_1      =    172,
    ME_TIER_ZERO_PU_32x8_2      =    173,
    ME_TIER_ZERO_PU_32x8_3      =    174,
    ME_TIER_ZERO_PU_32x8_4      =    175,
    ME_TIER_ZERO_PU_32x8_5      =    176,
    ME_TIER_ZERO_PU_32x8_6      =    177,
    ME_TIER_ZERO_PU_32x8_7      =    178,
    // 2NxnD [10 partitions]
    ME_TIER_ZERO_PU_64x48_0     =    179,
    ME_TIER_ZERO_PU_64x48_1     =    180,
    ME_TIER_ZERO_PU_32x24_0     =    181,
    ME_TIER_ZERO_PU_32x24_1     =    182,
    ME_TIER_ZERO_PU_32x24_2     =    183,
    ME_TIER_ZERO_PU_32x24_3     =    184,
    ME_TIER_ZERO_PU_32x24_4     =    185,
    ME_TIER_ZERO_PU_32x24_5     =    186,
    ME_TIER_ZERO_PU_32x24_6     =    187,
    ME_TIER_ZERO_PU_32x24_7     =    188,
    // nLx2N [10 partitions]
    ME_TIER_ZERO_PU_16x64_0     =    189,
    ME_TIER_ZERO_PU_16x64_1     =    190,
    ME_TIER_ZERO_PU_8x32_0      =    191,
    ME_TIER_ZERO_PU_8x32_1      =    192,
    ME_TIER_ZERO_PU_8x32_2      =    193,
    ME_TIER_ZERO_PU_8x32_3      =    194,
    ME_TIER_ZERO_PU_8x32_4      =    195,
    ME_TIER_ZERO_PU_8x32_5      =    196,
    ME_TIER_ZERO_PU_8x32_6      =    197,
    ME_TIER_ZERO_PU_8x32_7      =    198,
    // nRx2N [10 partitions]
    ME_TIER_ZERO_PU_48x64_0     =    199,
    ME_TIER_ZERO_PU_48x64_1     =    200,
    ME_TIER_ZERO_PU_24x32_0     =    201,
    ME_TIER_ZERO_PU_24x32_1     =    202,
    ME_TIER_ZERO_PU_24x32_2     =    203,
    ME_TIER_ZERO_PU_24x32_3     =    204,
    ME_TIER_ZERO_PU_24x32_4     =    205,
    ME_TIER_ZERO_PU_24x32_5     =    206,
    ME_TIER_ZERO_PU_24x32_6     =    207,
    ME_TIER_ZERO_PU_24x32_7     =    208
} EB_ME_TIER_ZERO_PU;

typedef struct MeTierZero_s {

    MePredictionUnit_t  pu[MAX_ME_PU_COUNT];

} MeTierZero_t;

typedef struct IntraReferenceSamplesOpenLoop_s {
    EbDctor                 dctor;
    EB_U8                  *yIntraReferenceArray;
    EB_U8                  *yIntraReferenceArrayReverse;

    // Scratch buffers used in the interpolaiton process
    EB_U8                   ReferenceAboveLineY[MAX_INTRA_REFERENCE_SAMPLES];
    EB_U8                   ReferenceLeftLineY[MAX_INTRA_REFERENCE_SAMPLES];
    EB_BOOL                 AboveReadyFlagY;
    EB_BOOL                 LeftReadyFlagY;
} IntraReferenceSamplesOpenLoop_t;

typedef EB_U64 (*EB_ME_DISTORTION_FUNC)(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref,
    EB_U32  refStride,
    EB_U32  width,
    EB_U32  height);

typedef struct MePredUnit_s {
    EB_U32  distortion;
	EB_PREDDIRECTION  predictionDirection;
    EB_U32  Mv[MAX_NUM_OF_REF_PIC_LIST];

} MePredUnit_t;

typedef struct MotionEstimationTierZero_s {
    MePredUnit_t  pu[MAX_ME_PU_COUNT];
} MotionEstimationTierZero_t;

typedef struct MeContext_s {
    EbDctor         dctor;
    // MV offset (search center)
    EB_BOOL         updateHmeSearchCenter;
    EB_S16          xMvOffset;
    EB_S16          yMvOffset;

    // Search region stride
	EB_U32  interpolatedStride;
	EB_U32  interpolatedFullStride[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];

    MotionEstimationTierZero_t    meCandidate[MAX_ME_CANDIDATE_PER_PU];

    EB_U8   *hmeBuffer;
    EB_U32   hmeBufferStride;

    // Intermediate LCU-sized buffer to retain the input samples
    EB_U8 *lcuBuffer;
    EB_U8 *lcuBufferPtr;
    EB_U32 lcuBufferStride;
    EB_U8 *hmeLcuBuffer;
    EB_U32 hmeLcuBufferStride;
	EB_U8 *lcuSrcPtr;
	EB_U32 lcuSrcStride;
    EB_U8 *quarterLcuBuffer;
    EB_U32 quarterLcuBufferStride;
    EB_U8 *sixteenthLcuBuffer;
    EB_U32 sixteenthLcuBufferStride;

    EB_U8 *integerBuffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
	EB_U8 *integerBufferPtr[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    EB_U8 *posbBuffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    EB_U8 *poshBuffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    EB_U8 *posjBuffer[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];

    EB_U8  *oneDIntermediateResultsBuf0;
    EB_U8  *oneDIntermediateResultsBuf1;

    EB_S16 xSearchAreaOrigin[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    EB_S16 ySearchAreaOrigin[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];

    EB_U8 *avctempBuffer;

    EB_U32  *pBestSad8x8;
    EB_U32  *pBestSad16x16;
    EB_U32  *pBestSad32x32;
    EB_U32  *pBestSad64x64;

    EB_U32  *pBestMV8x8;
    EB_U32  *pBestMV16x16;
    EB_U32  *pBestMV32x32;
    EB_U32  *pBestMV64x64;

    EB_U32  *pBestSad64x32;
    EB_U32  *pBestSad32x16;
    EB_U32  *pBestSad16x8;
    EB_U32  *pBestSad32x64;
    EB_U32  *pBestSad16x32;
    EB_U32  *pBestSad8x16;
    EB_U32  *pBestSad64x16;
    EB_U32  *pBestSad32x8;
    EB_U32  *pBestSad64x48;
    EB_U32  *pBestSad32x24;
    EB_U32  *pBestSad16x64;
    EB_U32  *pBestSad8x32;
    EB_U32  *pBestSad48x64;
    EB_U32  *pBestSad24x32;

    EB_U32  *pBestMV64x32;
    EB_U32  *pBestMV32x16;
    EB_U32  *pBestMV16x8;
    EB_U32  *pBestMV32x64;
    EB_U32  *pBestMV16x32;
    EB_U32  *pBestMV8x16;
    EB_U32  *pBestMV64x16;
    EB_U32  *pBestMV32x8;
    EB_U32  *pBestMV64x48;
    EB_U32  *pBestMV32x24;
    EB_U32  *pBestMV16x64;
    EB_U32  *pBestMV8x32;
    EB_U32  *pBestMV48x64;
    EB_U32  *pBestMV24x32;

    EB_U32  pSad32x32[4];
    EB_U32  pSad16x16[16];
    EB_U32  pSad8x8[64];

    EB_U8   psubPelDirection64x64;

    EB_U8   psubPelDirection32x32[4];
    EB_U8   psubPelDirection16x16[16];
    EB_U8   psubPelDirection8x8[64];

    EB_U8   psubPelDirection64x32[2];
    EB_U8   psubPelDirection32x16[8];
    EB_U8   psubPelDirection16x8[32];

    EB_U8   psubPelDirection32x64[2];
    EB_U8   psubPelDirection16x32[8];
    EB_U8   psubPelDirection8x16[32];

    EB_U8   psubPelDirection64x16[2];
    EB_U8   psubPelDirection32x8[8];

    EB_U8   psubPelDirection64x48[2];
    EB_U8   psubPelDirection32x24[8];

    EB_U8   psubPelDirection16x64[2];
    EB_U8   psubPelDirection8x32[8];

    EB_U8   psubPelDirection48x64[2];
    EB_U8   psubPelDirection24x32[8];

    EB_U32  pLcuBestSad[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    EB_U32  pLcuBestMV[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    EB_U32  pLcuBipredSad[MAX_ME_PU_COUNT];//needs to be upgraded to 209 pus
    EB_U32  pBestSadMap[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    EB_U32  pBestMvMap[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];


    EB_U32  pLcuBestSsd[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX][MAX_ME_PU_COUNT];
    EB_U32  *pBestSsd8x8;
    EB_U32  *pBestSsd16x16;
    EB_U32  *pBestSsd32x32;
    EB_U32  *pBestSsd64x64;

    EB_U16  *pEightPosSad16x16;

	EB_U64   lambda;
    EB_U8    hmeSearchType;

    // Multi-Mode signal(s)
    EB_U8    fractionalSearchMethod;
    EB_U8    fractionalSearchModel;
    EB_BOOL  fractionalSearch64x64;
    EB_BOOL  oneQuadrantHME;

    // ME
    EB_U8    searchAreaWidth;
    EB_U8    searchAreaHeight;
    // HME
    EB_U16   numberHmeSearchRegionInWidth;
    EB_U16   numberHmeSearchRegionInHeight;
    EB_U16   hmeLevel0TotalSearchAreaWidth;
    EB_U16   hmeLevel0TotalSearchAreaHeight;
    EB_U16   hmeLevel0SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U16   hmeLevel0SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    EB_U16   hmeLevel1SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U16   hmeLevel1SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    EB_U16   hmeLevel2SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U16   hmeLevel2SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];


	EB_BitFraction *mvdBitsArray;

} MeContext_t;

extern EB_ERRORTYPE MeContextCtor(
    MeContext_t     *objectPtr);

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationContext_h
