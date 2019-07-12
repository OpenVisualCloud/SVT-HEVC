/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_h
#define EbTransforms_h

#include "EbTransforms_C.h"
#include "EbTransforms_SSE2.h"
#include "EbTransforms_SSSE3.h"
#include "EbTransforms_SSE4_1.h"
#include "EbTransforms_AVX2.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"


#include "EbEncDecProcess.h"

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif


static const EB_U32 QFunc[] = {26214,23302,20560,18396,16384,14564};    
static const EB_U32 FFunc[] = {40,45,51,57,64,72};

//QP in [0..63]
static const EB_U8 QpModSix[]=
{
  0, 1, 2, 3, 4, 5,   
  0, 1, 2, 3, 4, 5,   
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3, 4, 5,
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3, 4, 5, 
  0, 1, 2, 3
};

static const EB_U8 QpDivSix[]=
{
  0, 0, 0, 0, 0, 0, 
  1, 1, 1, 1, 1, 1, 
  2, 2, 2, 2, 2, 2, 
  3, 3, 3, 3, 3, 3, 
  4, 4, 4, 4, 4, 4, 
  5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 
  7, 7, 7, 7, 7, 7, 
  8, 8, 8, 8, 8, 8,
  9, 9, 9, 9, 9, 9,
  10, 10, 10,10

};

static void  QiQVoidFunc(){}

/*****************************
 * DEBUG MACROS
 *****************************/
#define ZERO_COEFF 0
#define ZERO_COEFF_CHROMA 0

extern EB_ERRORTYPE Quantize(
    EB_S16            *coeff,
    const EB_U32       coeffStride,
    EB_S16            *quantCoeff,
    const EB_U32       quantCoeffStride,
    const EB_BITDEPTH  bitDepth,
    const EB_U32       transformSize,
    const EB_U32       qp,
    const EB_PICTURE     sliceType);

extern EB_ERRORTYPE InvQuantize(
    EB_S16            *quantCoeff,
    const EB_U32       quantCoeffStride,
    EB_S16            *reconCoeff,
    const EB_U32       reconCoeffStride,
    const EB_BITDEPTH  bitDepth,
    const EB_U32       transformSize,
    const EB_U32       qp);

extern void UnifiedQuantizeInvQuantize(

	EncDecContext_t       *contextPtr,

	PictureControlSet_t *pictureControlSetPtr,
	EB_S16              *coeff,
	const EB_U32         coeffStride,
	EB_S16              *quantCoeff,
	EB_S16              *reconCoeff,
	EB_U32               qp,
	EB_U32               bitDepth,
	EB_U32               areaSize,
	EB_PICTURE             sliceType,
	EB_U32			    *yCountNonZeroCoeffs,
	EB_TRANS_COEFF_SHAPE transCoeffShape,

	EB_U8		         cleanSparseCeoffPfEncDec,
	EB_U8			     pmpMaskingLevelEncDec,
	EB_MODETYPE		     type,
	EB_U32               enableCbflag,
	EB_U8                enableContouringQCUpdateFlag,
	EB_U32               componentType,
	EB_U32               temporalLayerIndex,
	EB_U32               dZoffset,

	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U64                        lambda,
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	CabacCost_t                  *CabacCost);


extern EB_ERRORTYPE EncodeTransform(
    EB_S16              *residualBuffer,
    EB_U32               residualStride,
    EB_S16              *coeffBuffer,
    EB_U32               coeffStride,
    EB_U32               transformSize,
    EB_S16              *transformInnerArrayPtr,
    EB_U32               bitIncrement,
    EB_BOOL              dstTransformFlag,
    EB_TRANS_COEFF_SHAPE transCoeffShape);

extern EB_ERRORTYPE EstimateTransform(
	EB_S16              *residualBuffer,
	EB_U32               residualStride,
	EB_S16              *coeffBuffer,
	EB_U32               coeffStride,
	EB_U32               transformSize,
	EB_S16              *transformInnerArrayPtr,
	EB_U32               bitIncrement,
	EB_BOOL              dstTansformFlag,
    EB_TRANS_COEFF_SHAPE transCoeffShape);


extern EB_ERRORTYPE EstimateInvTransform(
    EB_S16      *coeffBuffer,
    EB_U32       coeffStride,
    EB_S16      *reconBuffer,
    EB_U32       reconStride,
    EB_U32       transformSize,
    EB_S16      *transformInnerArrayPtr,
    EB_U32       bitIncrement,
    EB_BOOL      dstTransformFlag,
    EB_U32       partialFrequencyN2Flag);

extern EB_ERRORTYPE EncodeInvTransform(
	EB_BOOL      isOnlyDc,
    EB_S16      *coeffBuffer,
    EB_U32       coeffStride,
    EB_S16      *reconBuffer,
    EB_U32       reconStride,
    EB_U32       transformSize,
    EB_S16      *transformInnerArrayPtr,
    EB_U32       bitIncrement,
    EB_BOOL      dstTransformFlag);

extern EB_ERRORTYPE CalculateCbf(
    EB_S16      *buffer,
    EB_U32       stride,
    EB_U32       cuSize,
    EB_U32       transformSize,
    EB_U32      *cbfBuffer);
extern EB_U8 MapChromaQp(
	EB_U8 qp
	);



extern void DecoupledQuantizeInvQuantizeLoops(
    EB_S16                        *coeff,
	const EB_U32                  coeffStride,
	EB_S16                        *quantCoeff,
	EB_S16                        *reconCoeff,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U64                        lambda,
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_U32                        componentType,
	EB_U8                         temporalLayerIndex,
	EB_BOOL                       isUsedAsReferenceFlag,
	EB_U8                         chromaLambda,
    EB_U16                        qp,
    EB_U32                        bitDepth,
	CabacCost_t                  *CabacCost,
	const EB_U32                  qFunc,
	const EB_U32                  q_offset,
	const EB_S32                  shiftedQBits,
	const EB_S32                  shiftedFFunc,
	const EB_S32                  iq_offset,
	const EB_S32                  shiftNum,
	const EB_U32                  areaSize,
	EB_U32                        *nonzerocoeff,
    EB_RDOQ_PMCORE_TYPE            rdoType);


extern void PfZeroOutUselessQuadrants(
    EB_S16* transformCoeffBuffer,
    EB_U32  transformCoeffStride,
    EB_U32  quadrantSize);

/*****************************
* Function Pointer Typedef
*****************************/
typedef void(*EB_QIQ_TYPE)(
    EB_S16           *coeff,
    const EB_U32     coeffStride,
    EB_S16           *quantCoeff,
    EB_S16           *reconCoeff,
    const EB_U32     qFunc,
    const EB_U32     q_offset,
    const EB_S32     shiftedQBits,
    const EB_S32     shiftedFFunc,
    const EB_S32     iq_offset,
    const EB_S32     shiftNum,
    const EB_U32     areaSize,
    EB_U32			 *nonzerocoeff);

typedef void(*EB_MAT_MUL_TYPE)(
    EB_S16           *coeff,
    const EB_U32     coeffStride,
    const EB_U16     *maskingMatrix,
    const EB_U32     maskingMatrixStride,
    const EB_U32     computeSize,
    const EB_S32     offset,
    const EB_S32     shiftNum,
    EB_U32			 *nonzerocoeff);

extern void MatMult(
    EB_S16           *coeff,
    const EB_U32     coeffStride,
    const EB_U16     *maskingMatrix,
    const EB_U32     maskingMatrixStride,
    const EB_U32     computeSize,
    const EB_S32     offset,
    const EB_S32     shiftNum,
    EB_U32			 *nonzerocoeff);

typedef void(*EB_MAT_OUT_MUL_TYPE)(
	EB_S16           *coeff,
	const EB_U32     coeffStride,
	EB_S16*          coeffOut,
	const EB_U32     coeffOutStride,
	const EB_U16     *maskingMatrix,
	const EB_U32     maskingMatrixStride,
	const EB_U32     computeSize,
	const EB_S32     offset,
	const EB_S32     shiftNum,
	EB_U32			 *nonzerocoeff);

void MatMultOut(
	EB_S16           *coeff,
	const EB_U32     coeffStride,
	EB_S16*          coeffOut,
	const EB_U32     coeffOutStride,
	const EB_U16     *maskingMatrix,
	const EB_U32     maskingMatrixStride,
	const EB_U32     computeSize,
	const EB_S32     offset,
	const EB_S32     shiftNum,
	EB_U32			 *nonzerocoeff);

static EB_MAT_OUT_MUL_TYPE FUNC_TABLE MatMulOut_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
	  MatMultOut,
	  MatMult4x4_OutBuff_AVX2_INTRIN,
};



typedef void(*EB_TRANSFORM_FUNC)(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

typedef void(*EB_INVTRANSFORM_FUNC)(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

/*****************************
* Function Tables
*****************************/
static EB_QIQ_TYPE FUNC_TABLE QiQ_funcPtrArray[EB_ASM_TYPE_TOTAL][5] = {
	// C_DEFAULT
	{
        /*0 4x4   */     QuantizeInvQuantize,
        /*1 8x8   */     QuantizeInvQuantize,
        /*2 16x16 */     QuantizeInvQuantize,
        /*3       */     (EB_QIQ_TYPE)QiQVoidFunc,
        /*4 32x32 */     QuantizeInvQuantize
	},
	// AVX2
	{
		/*0 4x4   */    QuantizeInvQuantize4x4_SSE3,
		/*1 8x8   */	 QuantizeInvQuantize8x8_AVX2_INTRIN,
		/*2 16x16 */	 QuantizeInvQuantizeNxN_AVX2_INTRIN,
		/*3       */    (EB_QIQ_TYPE)QiQVoidFunc,
		/*4 32x32 */	 QuantizeInvQuantizeNxN_AVX2_INTRIN,

	},
};

static EB_MAT_MUL_TYPE FUNC_TABLE MatMul_funcPtrArray[EB_ASM_TYPE_TOTAL][5] = {
    // C_DEFAULT
    {
        /*0 4x4   */     MatMult,
        /*1 8x8   */     MatMult,
        /*2 16x16 */     MatMult,
        /*3 16x16 */     MatMult,
        /*4 32x32 */     MatMult
    },
    // AVX2
    {
        /*0 4x4   */     MatMult4x4_AVX2_INTRIN,
        /*1 8x8   */     MatMult8x8_AVX2_INTRIN,
        /*2 16x16 */     MatMultNxN_AVX2_INTRIN,
        /*3 16x16 */     MatMultNxN_AVX2_INTRIN,
        /*4 32x32 */     MatMultNxN_AVX2_INTRIN
    },


};

static const EB_TRANSFORM_FUNC transformFunctionTableEstimate[EB_ASM_TYPE_TOTAL][5] = {
        // C_DEFAULT
        {
	    	Transform32x32Estimate,
		    Transform16x16Estimate,
            Transform8x8,
            Transform4x4,
            DstTransform4x4
        },
        // AVX2
        {
            lowPrecisionTransform32x32_AVX2_INTRIN,
            lowPrecisionTransform16x16_AVX2_INTRIN,
            Transform8x8_SSE4_1_INTRIN,
            Transform4x4_SSE2_INTRIN,
            DstTransform4x4_SSE2_INTRIN
        },
};

static const EB_TRANSFORM_FUNC PfreqN2TransformTable0[EB_ASM_TYPE_TOTAL][5] = {
    // NON_AVX2
    {
        Transform32x32Estimate,
        Transform16x16Estimate,
        Transform8x8,
        Transform4x4,
        DstTransform4x4

    },
    // AVX2
    {
        PfreqTransform32x32_AVX2_INTRIN,
        PfreqTransform16x16_SSE2,
        PfreqTransform8x8_SSE4_1_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    }
};

static const EB_TRANSFORM_FUNC PfreqN2TransformTable1[EB_ASM_TYPE_TOTAL][5] = {
    // NON_AVX2
    {
        PfreqTransform32x32_SSE2,
        PfreqTransform16x16_SSE2,
        PfreqTransform8x8_SSE2_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN

    },
    // AVX2
    {
        PfreqTransform32x32_AVX2_INTRIN,
        PfreqTransform16x16_SSE2,
        PfreqTransform8x8_SSE4_1_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    }
};

static const EB_TRANSFORM_FUNC PfreqN4TransformTable0[EB_ASM_TYPE_TOTAL][5] = {
    // NON_AVX2
    {
        Transform32x32Estimate,
        Transform16x16Estimate,
        Transform8x8,
        Transform4x4,
        DstTransform4x4
    },
    // AVX2
    {
        PfreqN4Transform32x32_AVX2_INTRIN,
        PfreqN4Transform16x16_SSE2,
        PfreqN4Transform8x8_SSE4_1_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    }
};

static const EB_TRANSFORM_FUNC PfreqN4TransformTable1[EB_ASM_TYPE_TOTAL][5] = {
    // NON_AVX2
    {
        PfreqN4Transform32x32_SSE2,
        PfreqN4Transform16x16_SSE2,
        PfreqN4Transform8x8_SSE2_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    },
    // AVX2
    {
        PfreqN4Transform32x32_AVX2_INTRIN,
        PfreqN4Transform16x16_SSE2,
        PfreqN4Transform8x8_SSE4_1_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    }
};

static const EB_TRANSFORM_FUNC transformFunctionTableEncode0[EB_ASM_TYPE_TOTAL][5] = {
    // NON_AVX2
    {
        Transform32x32Estimate,
        Transform16x16Estimate,
        Transform8x8,
        Transform4x4,
        DstTransform4x4
    },
    // AVX2
    {
        Transform32x32_SSE2,
        Transform16x16_SSE2,
        Transform8x8_SSE4_1_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    },
};

static const EB_TRANSFORM_FUNC transformFunctionTableEncode1[EB_ASM_TYPE_TOTAL][5] = {
    // NON_AVX2
    {
        Transform32x32_SSE2,
        Transform16x16_SSE2,
        Transform8x8_SSE2_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    },
    // AVX2
    {
        Transform32x32_SSE2,
        Transform16x16_SSE2,
        Transform8x8_SSE4_1_INTRIN,
        Transform4x4_SSE2_INTRIN,
        DstTransform4x4_SSE2_INTRIN
    },
};

static const EB_INVTRANSFORM_FUNC invTransformFunctionTableEstimate[EB_ASM_TYPE_TOTAL][5] = {
        // C_DEFAULT
        {
            InvTransform32x32,
            InvTransform16x16,
            InvTransform8x8,
            InvTransform4x4,
            InvDstTransform4x4
        },
        // AVX2
        {
            EstimateInvTransform32x32_SSE2,
            EstimateInvTransform16x16_SSE2,
            InvTransform8x8_SSE2_INTRIN,
            InvTransform4x4_SSE2_INTRIN,
            InvDstTransform4x4_SSE2_INTRIN
        },
};

static const EB_INVTRANSFORM_FUNC invTransformFunctionTableEncode[EB_ASM_TYPE_TOTAL][5] = {
        // C_DEFAULT
        {
            InvTransform32x32,
            InvTransform16x16,
            InvTransform8x8,
            InvTransform4x4,
            InvDstTransform4x4
        },
        // AVX2
        {
            PFinvTransform32x32_SSSE3,
            PFinvTransform16x16_SSSE3,
            InvTransform8x8_SSE2_INTRIN,
            InvTransform4x4_SSE2_INTRIN,
            InvDstTransform4x4_SSE2_INTRIN
        },
};

#ifdef __cplusplus
}
#endif

#endif // EbTransforms_h


