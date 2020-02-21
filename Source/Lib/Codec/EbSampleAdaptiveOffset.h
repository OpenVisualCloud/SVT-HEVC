/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSampleAdaptiveOffset_h
#define EbSampleAdaptiveOffset_h

#include "EbSampleAdaptiveOffset_C.h"
#include "EbSampleAdaptiveOffset_SSE2.h"
#include "EbSaoApplication_SSSE3.h"

#include "EbDefinitions.h"

#include "EbPictureControlSet.h"
#ifdef __cplusplus
extern "C" {
#endif
struct PictureControlSet_s;

extern EB_ERRORTYPE SaoStatsCtor(SaoStats_t *saoStatsPtr);


extern EB_ERRORTYPE SaoGenerationDecision(
	SaoStats_t                 *saoStats,
	SaoParameters_t            *saoParams,
	MdRateEstimationContext_t  *mdRateEstimationPtr,
	EB_U64                      fullLambda,
    EB_U64                      fullChromaLambdaSao,
    EB_BOOL                     mmSAO,
    struct PictureControlSet_s *pictureControlSetPtr,
	EB_U32                      tbOriginX,
	EB_U32                      tbOriginY,
	EB_U32                      lcuWidth,
	EB_U32                      lcuHeight,
	SaoParameters_t            *saoPtr,
	SaoParameters_t            *leftSaoPtr,
	SaoParameters_t            *upSaoPtr,
	EB_S64                     *saoLumaBestCost,
	EB_S64                     *saoChromaBestCost);


extern EB_ERRORTYPE SaoGenerationDecision16bit(
    EbPictureBufferDesc_t      *inputLcuPtr,
    SaoStats_t                 *saoStats,
    SaoParameters_t            *saoParams,
    MdRateEstimationContext_t  *mdRateEstimationPtr,
    EB_U64                      fullLambda,
	EB_U64                      fullChromaLambdaSao,
	EB_BOOL                     mmSAO,
    struct PictureControlSet_s *pictureControlSetPtr,
    EB_U32                      tbOriginX,
    EB_U32                      tbOriginY,
    EB_U32                      lcuWidth,
    EB_U32                      lcuHeight,
    SaoParameters_t            *saoPtr,
    SaoParameters_t            *leftSaoPtr,
    SaoParameters_t            *upSaoPtr,
    EB_S64                     *saoLumaBestCost,
    EB_S64                     *saoChromaBestCost);


EB_ERRORTYPE GatherSaoStatisticsLcu16bit_SSE2(
	EB_U16					*inputSamplePtr,        // input parameter, source Picture Ptr
	EB_U32                   inputStride,           // input parameter, source stride
	EB_U16                  *reconSamplePtr,        // input parameter, deblocked Picture Ptr
	EB_U32                   reconStride,           // input parameter, deblocked stride
	EB_U32                   lcuWidth,              // input parameter, LCU width
	EB_U32                   lcuHeight,             // input parameter, LCU height
	EB_S32                  *boDiff,                // output parameter, used to store Band Offset diff, boDiff[SAO_BO_INTERVALS]
	EB_U16                  *boCount,										// output parameter, used to store Band Offset count, boCount[SAO_BO_INTERVALS]
	EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],     // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
	EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1]);    // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]


typedef EB_ERRORTYPE(*EB_SAOGATHER_FUNC)(
    EB_U8                   *inputSamplePtr,
    EB_U32                   inputStride,
    EB_U8                   *reconSamplePtr,
    EB_U32                   reconStride,
    EB_U32                   lcuWidth,
    EB_U32                   lcuHeight,
    EB_S32                  *boDiff,
    EB_U16                  *boCount,
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1]);


typedef EB_ERRORTYPE(*EB_SAOGATHER_90_45_135_FUNC)(
    EB_U8                   *inputSamplePtr,
    EB_U32                   inputStride,
    EB_U8                   *reconSamplePtr,
    EB_U32                   reconStride,
    EB_U32                   lcuWidth,
    EB_U32                   lcuHeight,
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1]);

typedef EB_ERRORTYPE(*EB_SAOAPPLY_EO_135_45_FUNC)(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U8                    *temporalBufferLeft,
    EB_U8                    *temporalBufferUpper,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

typedef EB_ERRORTYPE(*EB_SAOGATHER_90_45_135_16bit_SSE2_FUNC)(
    EB_U16                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U16                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],    // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1]);  // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]

typedef EB_ERRORTYPE(*EB_SAOAPPLY_BO_FUNC)(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U32                    bandPosition,
    EB_S8                    *saoOffset,
    EB_U32                    lcuWidth,
    EB_U32                    lcuHeight);

typedef EB_ERRORTYPE(*EB_SAOAPPLY_EO_0_90_FUNC)(
    EB_U8                    *reconSamplePtr,
    EB_U32                    reconStride,
    EB_U8                    *temporalBuffer,
    EB_S8                    *saoOffsetPtr,
    EB_U32                    lcuHeight,
    EB_U32                    lcuWidth);

typedef EB_ERRORTYPE(*EB_SAOAPPLY_BO_16bit_FUNC)(
    EB_U16                          *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U32                           saoBandPosition,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

typedef EB_ERRORTYPE(*EB_SAOAPPLY_EO_0_90_16bit_FUNC)(
    EB_U16                          *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                          *temporalBufferLeft,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

typedef EB_ERRORTYPE(*EB_SAOAPPLY_EO_135_45_16bit_FUNC)(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                          *temporalBufferLeft,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth);

typedef EB_ERRORTYPE(*EB_SAOGATHER_16bit_FUNC)(
	EB_U16					*inputSamplePtr,        // input parameter, source Picture Ptr
	EB_U32                   inputStride,           // input parameter, source stride
	EB_U16                  *reconSamplePtr,        // input parameter, deblocked Picture Ptr
	EB_U32                   reconStride,           // input parameter, deblocked stride
	EB_U32                   lcuWidth,              // input parameter, LCU width
	EB_U32                   lcuHeight,             // input parameter, LCU height
	EB_S32                  *boDiff,                // output parameter, used to store Band Offset diff, boDiff[SAO_BO_INTERVALS]
	EB_U16                  *boCount,											// output parameter, used to store Band Offset count, boCount[SAO_BO_INTERVALS]
	EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],		// output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
	EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1]);		// output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]

static const EB_SAOGATHER_16bit_FUNC SaoGatherFunctionTabl_16bit[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
	GatherSaoStatisticsLcu_62x62_16bit,
	// AVX2 
	GatherSaoStatisticsLcu16bit_SSE2,

};


static const EB_SAOGATHER_FUNC SaoGatherFunctionTableLossy[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    GatherSaoStatisticsLcuLossy_62x62,
    // AVX2 
    GatherSaoStatisticsLcu_BT_SSE2,
};


static const EB_SAOGATHER_90_45_135_FUNC SaoGatherFunctionTableLossy_90_45_135[EB_ASM_TYPE_TOTAL]= {
	// C_DEFAULT
    GatherSaoStatisticsLcu_OnlyEo_90_45_135_Lossy,
	// AVX2 
	GatherSaoStatisticsLcu_OnlyEo_90_45_135_BT_SSE2,
};

static const EB_SAOGATHER_90_45_135_16bit_SSE2_FUNC SaoGatherFunctionTable_90_45_135_16bit_SSE2[EB_ASM_TYPE_TOTAL][2] = {
	// C_DEFAULT
	{
        GatherSaoStatisticsLcu_62x62_OnlyEo_90_45_135_16bit,
        GatherSaoStatisticsLcu_62x62_OnlyEo_90_45_135_16bit,
	},
	// AVX2
	{
		GatherSaoStatisticsLcu_62x62_OnlyEo_90_45_135_16bit,
		GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_INTRIN,
	},
};


static const EB_SAOAPPLY_BO_FUNC SaoFunctionTableBo[EB_ASM_TYPE_TOTAL][2] = {
    // C_DEFAULT
    {
        SAOApplyBO,
        SAOApplyBO
    },
    // AVX2
    {
		SAOApplyBO,
		SAOApplyBO_BT_SSSE3
    },
};

static const EB_SAOAPPLY_EO_0_90_FUNC SaoFunctionTableEO_0_90[EB_ASM_TYPE_TOTAL][2][2] = {
	// C_DEFAULT
    {
        {
            SAOApplyEO_0,
            SAOApplyEO_0
        },
        {
            SAOApplyEO_90,
            SAOApplyEO_90
        }
    },
	// AVX2
    {
        {
            SAOApplyEO_0,
            SAOApplyEO_0_BT_SSSE3
        },
        {
            SAOApplyEO_90,
            SAOApplyEO_90_BT_SSSE3
        }
    },
};

static const EB_SAOAPPLY_EO_135_45_FUNC SaoFunctionTableEO_135_45[EB_ASM_TYPE_TOTAL][2][2][2] = {
	// C_DEFAULT
	{
        {
			{
			SAOApplyEO_135,
			SAOApplyEO_135
			},
			{
			SAOApplyEO_135,
			SAOApplyEO_135
			}
        },
        {
			{
            SAOApplyEO_45,
            SAOApplyEO_45
			},
			{
            SAOApplyEO_45,
            SAOApplyEO_45
			}
        }
    },
	// AVX2
	{
        {
			{  
				SAOApplyEO_135,
                SAOApplyEO_135_BT_SSSE3
			},
			{ 
				SAOApplyEO_135,
                SAOApplyEO_135_BT_SSSE3
			}

        },
        {
			{
                SAOApplyEO_45,
                SAOApplyEO_45_BT_SSSE3
			},
			{
                SAOApplyEO_45,
				SAOApplyEO_45_BT_SSSE3
			}
        }
    },
};

static const EB_SAOAPPLY_BO_16bit_FUNC SaoFunctionTableBo_16bit[EB_ASM_TYPE_TOTAL][2] = { 
	// C_DEFAULT
	{
        SAOApplyBO16bit,
        SAOApplyBO16bit
	},
	// AVX2
	{
		SAOApplyBO16bit,
		SAOApplyBO16bit_SSE2_INTRIN
	},
};

static const EB_SAOAPPLY_EO_0_90_16bit_FUNC SaoFunctionTableEO_0_90_16bit[EB_ASM_TYPE_TOTAL][2][2] = {
	// C_DEFAULT
    {
        {
         SAOApplyEO_0_16bit,
         SAOApplyEO_0_16bit
        },
        {
         SAOApplyEO_90_16bit,
         SAOApplyEO_90_16bit
        }
    },
	// AVX2
    {
        {
         SAOApplyEO_0_16bit,
         SAOApplyEO_0_16bit_SSE2_INTRIN
        },
        {
         SAOApplyEO_90_16bit,
         SAOApplyEO_90_16bit_SSE2_INTRIN
        }
    },
};

static const EB_SAOAPPLY_EO_135_45_16bit_FUNC SaoFunctionTableEO_135_45_16bit[EB_ASM_TYPE_TOTAL][2][2] = {
	// C_DEFAULT
    {
        {
			SAOApplyEO_135_16bit,
			SAOApplyEO_135_16bit
        },
        {
            SAOApplyEO_45_16bit,
            SAOApplyEO_45_16bit
        }
    },
	// AVX2
    {
        {
			SAOApplyEO_135_16bit,
            SAOApplyEO_135_16bit_SSE2_INTRIN
        },
        {
			SAOApplyEO_45_16bit,
			SAOApplyEO_45_16bit_SSE2_INTRIN
        }
    },
};

#ifdef __cplusplus
}
#endif
#endif
