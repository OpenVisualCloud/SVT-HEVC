/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_h
#define EbPictureOperators_h

#include "EbPictureOperators_C.h"
#include "EbPictureOperators_SSE2.h"
#include "EbPictureOperators_SSE4_1.h"
#include "EbPictureOperators_AVX2.h"
#include "EbHmCode.h"
#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#ifdef __cplusplus
extern "C" {
#endif

extern void PictureAddition(
    EB_U8  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U8  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height);

void PictureAddition16bit_TEST(
    EB_U16  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U16  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height);

extern EB_ERRORTYPE PictureCopy8Bit(
    EbPictureBufferDesc_t   *src,
    EB_U32                   srcLumaOriginIndex,
    EB_U32                   srcChromaOriginIndex,
    EbPictureBufferDesc_t   *dst,
    EB_U32                   dstLumaOriginIndex,
    EB_U32                   dstChromaOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight,
    EB_U32                   chromaAreaWidth,
    EB_U32                   chromaAreaHeight,
    EB_U32                   componentMask);

extern EB_ERRORTYPE PictureFastDistortion(
    EbPictureBufferDesc_t   *input,
    EB_U32                   inputLumaOriginIndex,
    EB_U32                   inputChromaOriginIndex,
    EbPictureBufferDesc_t   *pred,
    EB_U32                   predLumaOriginIndex,
    EB_U32                   predChromaOriginIndex,
    EB_U32                   size,
    EB_U32                   componentMask,
    EB_U64                   lumaDistortion[DIST_CALC_TOTAL],
    EB_U64                   chromaDistortion[DIST_CALC_TOTAL]);

EB_ERRORTYPE PictureFullDistortion_R(
    EbPictureBufferDesc_t   *coeff,
    EB_U32                   coeffLumaOriginIndex,
    EB_U32                   coeffChromaOriginIndex,
    EbPictureBufferDesc_t   *reconCoeff,
    EB_U32                   areaSize,
    EB_U32                   chromaAreaSize,
    EB_U32                   componentMask,
    EB_U64                   lumaDistortion[DIST_CALC_TOTAL],
    EB_U64                   cbDistortion[DIST_CALC_TOTAL],
    EB_U64                   crDistortion[DIST_CALC_TOTAL],
	EB_U32                   *countNonZeroCoeffs,
	EB_MODETYPE				 mode);

extern EB_ERRORTYPE PictureFullDistortionLuma(
    EbPictureBufferDesc_t   *coeff,
    EB_U32                   coeffLumaOriginIndex,
    EbPictureBufferDesc_t   *reconCoeff,
    EB_U32                   reconCoeffLumaOriginIndex,
    EB_U32                   areaSize,
    EB_U64                   lumaDistortion[DIST_CALC_TOTAL],
	EB_U32                   countNonZeroCoeffsY,
	EB_MODETYPE				 mode);

extern EB_ERRORTYPE PictureFullDistortionChroma(
    EbPictureBufferDesc_t   *coeff,
    EB_U32                   coeffCbOriginIndex,
	EB_U32                   coeffCrOriginIndex,
    EbPictureBufferDesc_t   *reconCoeff,
    EB_U32                   reconCoeffCbOriginIndex,
	EB_U32                   reconCoeffCrOriginIndex,
    EB_U32                   areaSize,
    EB_U64                   cbDistortion[DIST_CALC_TOTAL],
	EB_U64                   crDistortion[DIST_CALC_TOTAL],
	EB_U32                   countNonZeroCoeffsCb,
	EB_U32                   countNonZeroCoeffsCr,
	EB_MODETYPE				 mode);

extern EB_U64 ComputeNxMSatdSadLCU(
    EB_U8  *src,        // input parameter, source samples Ptr
    EB_U32  srcStride,  // input parameter, source stride
    EB_U32  width,      // input parameter, block width (N)
    EB_U32  height);    // input parameter, block height (M)

//Residual Data
extern void PictureResidual(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);
extern void PictureSubSampledResidual(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight,
    EB_U8    lastLine);
extern void PictureResidual16bit(
    EB_U16   *input,
    EB_U32   inputStride,
    EB_U16   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void CompressedPackBlk(
	EB_U8     *in8BitBuffer,
	EB_U32     in8Stride,
	EB_U8     *innBitBuffer,
	EB_U32     innStride,
	EB_U16    *out16BitBuffer,
	EB_U32     outStride,
	EB_U32     width,
	EB_U32     height
	);
void Conv2bToCPackLcu(
	const EB_U8     *innBitBuffer,
	EB_U32     innStride,
	EB_U8     *inCompnBitBuffer,
	EB_U32     outStride,
	EB_U8    *localCache,
	EB_U32     width,
	EB_U32     height);

void CompressedPackLcu(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U32     innStride,
    EB_U16    *out16BitBuffer,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height);

void Pack2D_SRC( 
   EB_U8     *in8BitBuffer,
   EB_U32     in8Stride, 
   EB_U8     *innBitBuffer, 
   EB_U32     innStride, 
   EB_U16    *out16BitBuffer, 
   EB_U32     outStride, 
   EB_U32     width,
   EB_U32     height);

void UnPack2D( 
   EB_U16      *in16BitBuffer,
   EB_U32       inStride, 
   EB_U8       *out8BitBuffer,
   EB_U32       out8Stride, 
   EB_U8       *outnBitBuffer,  
   EB_U32       outnStride, 
   EB_U32       width,
   EB_U32       height);
void extract8Bitdata(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,   
    EB_U32       width,
    EB_U32       height
    );
void UnpackL0L1Avg(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,      
        EB_U32  width,
        EB_U32  height);
void Extract8BitdataSafeSub(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,   
    EB_U32       width,
    EB_U32       height
    );
void UnpackL0L1AvgSafeSub(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,      
        EB_U32  width,
        EB_U32  height);

void memcpy16bit(
    EB_U16                     * outPtr,
    EB_U16                     * inPtr,   
    EB_U64                       numOfElements );
void memset16bit(
    EB_U16                     * inPtr,
    EB_U16                       value,
    EB_U64                       numOfElements );

static void PictureAdditionVoidFunc(){}
static void PicCopyVoidFunc(){}
static void PicResdVoidFunc(){}
static void PicZeroOutCoefVoidFunc(){}
static void FullDistortionVoidFunc(){}

EB_S32  sumResidual( EB_S16 * inPtr,
                     EB_U32   size,
                     EB_U32   strideIn );


typedef EB_S32(*EB_SUM_RES)(
                     EB_S16 * inPtr,
                     EB_U32   size,
                     EB_U32   strideIn );

static EB_SUM_RES FUNC_TABLE SumResidual_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
	sumResidual,
	// AVX2
	sumResidual8bit_AVX2_INTRIN,
};

void memset16bitBlock (
                    EB_S16 * inPtr,
                    EB_U32   strideIn,
                    EB_U32   size,
                    EB_S16   value
    );

typedef void(*EB_MEMSET16bitBLK)(
                    EB_S16 * inPtr,
                    EB_U32   strideIn,
                    EB_U32   size,
                    EB_S16   value );

static EB_MEMSET16bitBLK FUNC_TABLE memset16bitBlock_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
	memset16bitBlock,
	// AVX2
	memset16bitBlock_AVX2_INTRIN,
};

/***************************************
* Function Types
***************************************/
typedef void(*EB_ADDDKERNEL_TYPE)(
    EB_U8  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U8  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height);

typedef void(*EB_ADDDKERNEL_TYPE_16BIT)(
    EB_U16  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U16  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height);

typedef void(*EB_PICCOPY_TYPE)(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);



typedef void(*EB_RESDKERNEL_TYPE)(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

typedef void(*EB_RESDKERNEL_TYPE_16BIT)(
    EB_U16   *input,
    EB_U32   inputStride,
    EB_U16   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

typedef void(*EB_ZEROCOEFF_TYPE)(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);

typedef void(*EB_FULLDIST_TYPE)(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[DIST_CALC_TOTAL],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

typedef EB_U64(*EB_SATD_TYPE)(
    EB_S16 *diff);

typedef EB_U64(*EB_SATD_U8_TYPE)(
	EB_U8 *diff,
	EB_U64 *dcValue,
	EB_U32  srcStride);

/***************************************
* Function Tables
***************************************/
static EB_ADDDKERNEL_TYPE FUNC_TABLE AdditionKernel_funcPtrArray[EB_ASM_TYPE_TOTAL][9] = {
        // C_DEFAULT
        {
            /*0 4x4   */    PictureAdditionKernel,
            /*1 8x8   */    PictureAdditionKernel,
            /*2 16x16 */    PictureAdditionKernel,
            /*3       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
            /*4 32x32 */    PictureAdditionKernel,
            /*5       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
            /*6       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
            /*7       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
            /*8 64x64 */    PictureAdditionKernel
        },
        // AVX2
        {
			/*0 4x4   */    PictureAdditionKernel4x4_SSE_INTRIN,
			/*1 8x8   */    PictureAdditionKernel8x8_SSE2_INTRIN,
			/*2 16x16 */    PictureAdditionKernel16x16_SSE2_INTRIN,
            /*3       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
			/*4 32x32 */    PictureAdditionKernel32x32_SSE2_INTRIN,
            /*5       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
            /*6       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
            /*7       */    (EB_ADDDKERNEL_TYPE)PictureAdditionVoidFunc,
			/*8 64x64 */    PictureAdditionKernel64x64_SSE2_INTRIN,
        },
};

static EB_ADDDKERNEL_TYPE_16BIT FUNC_TABLE AdditionKernel_funcPtrArray16bit[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
    PictureAdditionKernel16bit,
	// AVX2
	PictureAdditionKernel16bit_SSE2_INTRIN,
};

static EB_PICCOPY_TYPE FUNC_TABLE PicCopyKernel_funcPtrArray[EB_ASM_TYPE_TOTAL][9] = {
        // C_DEFAULT
        {
            /*0 4x4   */     CopyKernel8Bit,
            /*1 8x8   */     CopyKernel8Bit,
            /*2 16x16 */     CopyKernel8Bit,
            /*3       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
            /*4 32x32 */     CopyKernel8Bit,
            /*5       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
            /*6       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
            /*7       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
            /*8 64x64 */     CopyKernel8Bit
        },
        // AVX2
        {
			/*0 4x4   */     PictureCopyKernel4x4_SSE_INTRIN,
			/*1 8x8   */     PictureCopyKernel8x8_SSE2_INTRIN,
			/*2 16x16 */     PictureCopyKernel16x16_SSE2_INTRIN,
            /*3       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
			/*4 32x32 */     PictureCopyKernel32x32_SSE2_INTRIN,
            /*5       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
            /*6       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
            /*7       */     (EB_PICCOPY_TYPE)PicCopyVoidFunc,
			/*8 64x64 */     PictureCopyKernel64x64_SSE2_INTRIN,
        },
};


typedef void(*EB_RESDKERNELSUBSAMPLED_TYPE)(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight , 
    EB_U8    lastLine  
    );
static EB_RESDKERNELSUBSAMPLED_TYPE FUNC_TABLE ResidualKernelSubSampled_funcPtrArray[EB_ASM_TYPE_TOTAL][9] = {
	// C_DEFAULT
	{
		/*0 4x4  */     ResidualKernelSubSampled,
		/*1 8x8  */     ResidualKernelSubSampled,
		/*2 16x16 */    ResidualKernelSubSampled,
		/*3  */         (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*4 32x32 */    ResidualKernelSubSampled,
		/*5      */     (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*6  */         (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*7      */     (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*8 64x64 */    ResidualKernelSubSampled
	},
	// AVX2
	{
		/*0 4x4  */     ResidualKernelSubSampled4x4_SSE_INTRIN,
		/*1 8x8  */     ResidualKernelSubSampled8x8_SSE2_INTRIN,
		/*2 16x16 */    ResidualKernelSubSampled16x16_SSE2_INTRIN,
		/*3  */         (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*4 32x32 */    ResidualKernelSubSampled32x32_SSE2_INTRIN,
		/*5      */     (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*6  */         (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*7      */     (EB_RESDKERNELSUBSAMPLED_TYPE)PicResdVoidFunc,
		/*8 64x64 */    ResidualKernelSubSampled64x64_SSE2_INTRIN,
	},
};

static EB_RESDKERNEL_TYPE FUNC_TABLE ResidualKernel_funcPtrArray[EB_ASM_TYPE_TOTAL][9] = {
	// C_DEFAULT
	{
        /*0 4x4  */     ResidualKernel,
        /*1 8x8  */     ResidualKernel,
        /*2 16x16 */    ResidualKernel,
        /*3  */         (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
        /*4 32x32 */    ResidualKernel,
        /*5      */     (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
        /*6  */         (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
        /*7      */     (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
        /*8 64x64 */    ResidualKernel
	},
	// AVX2
	{
		/*0 4x4  */     ResidualKernel4x4_SSE_INTRIN,
		/*1 8x8  */     ResidualKernel8x8_SSE2_INTRIN,
		/*2 16x16 */    ResidualKernel16x16_SSE2_INTRIN,
		/*3  */         (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
		/*4 32x32 */    ResidualKernel32x32_SSE2_INTRIN,
		/*5      */     (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
		/*6  */         (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
		/*7      */     (EB_RESDKERNEL_TYPE)PicResdVoidFunc,
		/*8 64x64 */    ResidualKernel64x64_SSE2_INTRIN,
	},
};


static EB_RESDKERNEL_TYPE_16BIT FUNC_TABLE ResidualKernel_funcPtrArray16Bit[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT     
    ResidualKernel16bit,
    // AVX2     
    ResidualKernel16bit_SSE2_INTRIN
};

static EB_ZEROCOEFF_TYPE FUNC_TABLE PicZeroOutCoef_funcPtrArray[EB_ASM_TYPE_TOTAL][5] = {
        // C_DEFAULT
        {
            /*0 4x4   */     ZeroOutCoeffKernel,
            /*1 8x8   */     ZeroOutCoeffKernel,
            /*2 16x16 */     ZeroOutCoeffKernel,
            /*3       */     (EB_ZEROCOEFF_TYPE)PicZeroOutCoefVoidFunc,
            /*4 32x32 */     ZeroOutCoeffKernel
        },
        // AVX2
        {
            /*0 4x4   */     ZeroOutCoeff4x4_SSE,
            /*1 8x8   */     ZeroOutCoeff8x8_SSE2,
            /*2 16x16 */     ZeroOutCoeff16x16_SSE2,
            /*3       */     (EB_ZEROCOEFF_TYPE)PicZeroOutCoefVoidFunc,
            /*4 32x32 */     ZeroOutCoeff32x32_SSE2
        },
};

static EB_FULLDIST_TYPE FUNC_TABLE FullDistortionIntrinsic_funcPtrArray[EB_ASM_TYPE_TOTAL][2][2][9] = {     
    // C_DEFAULT
    // It was found that the SSE2 intrinsic code is much faster (~2x) than the SSE4.1 code
    {
        {
            {
                /*0 4x4   */    FullDistortionKernelCbfZero_32bit,
                /*1 8x8   */    FullDistortionKernelCbfZero_32bit,
                /*2 16x16 */    FullDistortionKernelCbfZero_32bit,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernelCbfZero_32bit,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernelCbfZero_32bit,
            },
            {
                /*0 4x4   */    FullDistortionKernelCbfZero_32bit,
                /*1 8x8   */    FullDistortionKernelCbfZero_32bit,
                /*2 16x16 */    FullDistortionKernelCbfZero_32bit,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernelCbfZero_32bit,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernelCbfZero_32bit,
            }
        },
        {
            {
                /*0 4x4   */    FullDistortionKernel_32bit,
                /*1 8x8   */    FullDistortionKernel_32bit,
                /*2 16x16 */    FullDistortionKernel_32bit,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernel_32bit,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernel_32bit,
            },
            {
                /*0 4x4   */    FullDistortionKernelIntra_32bit,
                /*1 8x8   */    FullDistortionKernelIntra_32bit,
                /*2 16x16 */    FullDistortionKernelIntra_32bit,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernelIntra_32bit,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernelIntra_32bit,
            }

        }
    },    
    // AVX2
    // It was found that the SSE2 intrinsic code is much faster (~2x) than the SSE4.1 code
    {
        {
            {
                /*0 4x4   */    FullDistortionKernelCbfZero4x4_32bit_BT_SSE2,
                /*1 8x8   */    FullDistortionKernelCbfZero8x8_32bit_BT_SSE2,
                /*2 16x16 */    FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2,
            },
            {
                /*0 4x4   */    FullDistortionKernelCbfZero4x4_32bit_BT_SSE2,
                /*1 8x8   */    FullDistortionKernelCbfZero8x8_32bit_BT_SSE2,
                /*2 16x16 */    FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2,
            }
        },
        {
            {
                /*0 4x4   */    FullDistortionKernel4x4_32bit_BT_SSE2,
                /*1 8x8   */    FullDistortionKernel8x8_32bit_BT_SSE2,
                /*2 16x16 */    FullDistortionKernel16MxN_32bit_BT_SSE2,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernel16MxN_32bit_BT_SSE2,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernel16MxN_32bit_BT_SSE2,
            },
            {
                /*0 4x4   */    FullDistortionKernelIntra4x4_32bit_BT_SSE2,
                /*1 8x8   */    FullDistortionKernelIntra8x8_32bit_BT_SSE2,
                /*2 16x16 */    FullDistortionKernelIntra16MxN_32bit_BT_SSE2,
                /*3       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*4 32x32 */    FullDistortionKernelIntra16MxN_32bit_BT_SSE2,
                /*5       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*6       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*7       */    (EB_FULLDIST_TYPE)FullDistortionVoidFunc,
                /*8 64x64 */    FullDistortionKernelIntra16MxN_32bit_BT_SSE2,
            }
        }
    },   
};

static EB_SATD_TYPE FUNC_TABLE Compute8x8Satd_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    Compute8x8Satd,
    // ASM_AVX2
    Compute8x8Satd_SSE4
};

static EB_SATD_U8_TYPE FUNC_TABLE Compute8x8Satd_U8_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
    Compute8x8Satd_U8,
	// ASM_AVX2
	Compute8x8Satd_U8_SSE4
};

typedef EB_U64(*EB_SPATIALFULLDIST_TYPE)(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *recon,
    EB_U32   reconStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

static EB_SPATIALFULLDIST_TYPE FUNC_TABLE SpatialFullDistortionKernel_funcPtrArray[EB_ASM_TYPE_TOTAL][5] = {
    // C_DEFAULT
    {
        // 4x4
        SpatialFullDistortionKernel,
        // 8x8
        SpatialFullDistortionKernel,
        // 16x16
        SpatialFullDistortionKernel,
        // 32x32
        SpatialFullDistortionKernel,
        // 64x64
        SpatialFullDistortionKernel
    },
    // ASM_AVX2
    {
        // 4x4
        SpatialFullDistortionKernel4x4_SSSE3_INTRIN,
        // 8x8
        SpatialFullDistortionKernel8x8_SSSE3_INTRIN,
        // 16x16
        SpatialFullDistortionKernel16MxN_SSSE3_INTRIN,
        // 32x32
        SpatialFullDistortionKernel16MxN_SSSE3_INTRIN,
        // 64x64
        SpatialFullDistortionKernel16MxN_SSSE3_INTRIN
    },
};




#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_h
