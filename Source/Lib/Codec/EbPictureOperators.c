/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/*********************************
 * Includes
 *********************************/

#include "EbPictureOperators.h"
#define VARIANCE_PRECISION      16
#define MEAN_PRECISION      (VARIANCE_PRECISION >> 1)

#include "EbComputeSAD.h"
#include "EbPackUnPack.h"

/*********************************
 * x86 implememtation of Picture Addition
 *********************************/
void PictureAddition(
    EB_U8  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U8  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height)
{

	AdditionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][width >> 3](
        predPtr,
        predStride,
        residualPtr,
        residualStride,
        reconPtr,
        reconStride,
        width,
        height
    );

    return;
}

/*********************************
 * Picture Copy 8bit Elements
 *********************************/
EB_ERRORTYPE PictureCopy8Bit(
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
    EB_U32                   componentMask)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Execute the Kernels
    if (componentMask & PICTURE_BUFFER_DESC_Y_FLAG) {

        PicCopyKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][areaWidth>>3](
            &(src->bufferY[srcLumaOriginIndex]),
            src->strideY,
            &(dst->bufferY[dstLumaOriginIndex]),
            dst->strideY,
            areaWidth,
            areaHeight);
    }

    if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {

		PicCopyKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][chromaAreaWidth >> 3](
            &(src->bufferCb[srcChromaOriginIndex]),
            src->strideCb,
            &(dst->bufferCb[dstChromaOriginIndex]),
            dst->strideCb,
            chromaAreaWidth,
            chromaAreaHeight);
    }

    if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {

		PicCopyKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][chromaAreaWidth >> 3](
            &(src->bufferCr[srcChromaOriginIndex]),
            src->strideCr,
            &(dst->bufferCr[dstChromaOriginIndex]),
            dst->strideCr,
            chromaAreaWidth,
            chromaAreaHeight);
    }

    return return_error;
}


/*******************************************
* Picture Residue : subsampled version
  Computes the residual data 
*******************************************/
void PictureSubSampledResidual(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight,
    EB_U8    lastLine)    //the last line has correct prediction data, so no duplication to be done.
{

    ResidualKernelSubSampled_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][areaWidth>>3](
        input,
        inputStride,
        pred,
        predStride,
        residual,
        residualStride,
        areaWidth,
        areaHeight,
        lastLine);

    return;
}
/*******************************************
* Pciture Residue
  Computes the residual data
*******************************************/
void PictureResidual(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{

    ResidualKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][areaWidth>>3](
        input,
        inputStride,
        pred,
        predStride,
        residual,
        residualStride,
        areaWidth,
        areaHeight);

    return;
}

/*******************************************
 * Pciture Residue 16bit input
   Computes the residual data
 *******************************************/
void PictureResidual16bit(
    EB_U16   *input,
    EB_U32   inputStride,
    EB_U16   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{

    ResidualKernel_funcPtrArray16Bit[!!(ASM_TYPES & PREAVX2_MASK)](
        input,
        inputStride,
        pred,
        predStride,
        residual,
        residualStride,
        areaWidth,
        areaHeight);

    return;
}

EB_U64 ComputeNxMSatd8x8Units_U8(
	EB_U8  *src,      //EB_S16 *diff,       // input parameter, diff samples Ptr
	EB_U32  srcStride, //EB_U32  diffStride, // input parameter, source stride
	EB_U32  width,      // input parameter, block width (N)
	EB_U32  height,     // input parameter, block height (M)
	EB_U64 *dcValue)
{
	EB_U64 satd = 0;
	EB_U32 blockIndexInWidth;
	EB_U32 blockIndexInHeight;
	EB_SATD_U8_TYPE Compute8x8SatdFunction = Compute8x8Satd_U8_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)];

	for (blockIndexInHeight = 0; blockIndexInHeight < height >> 3; ++blockIndexInHeight) {
		for (blockIndexInWidth = 0; blockIndexInWidth < width >> 3; ++blockIndexInWidth) {
			satd += Compute8x8SatdFunction(&(src[(blockIndexInWidth << 3) + (blockIndexInHeight << 3) * srcStride]), dcValue, srcStride);
		}
	}

	return satd;
}


EB_U64 ComputeNxMSatd4x4Units_U8(
	EB_U8  *src,       //EB_S16 *diff,       // input parameter, diff samples Ptr
	EB_U32  srcStride, //EB_U32  diffStride, // input parameter, source stride
	EB_U32  width,      // input parameter, block width (N)
	EB_U32  height,     // input parameter, block height (M)
	EB_U64 *dcValue)
{

	EB_U64 satd = 0;
	EB_U32 blockIndexInWidth;
	EB_U32 blockIndexInHeight;

	for (blockIndexInHeight = 0; blockIndexInHeight < height >> 2; ++blockIndexInHeight) {
		for (blockIndexInWidth = 0; blockIndexInWidth < width >> 2; ++blockIndexInWidth) {
			satd += Compute4x4Satd_U8(&(src[(blockIndexInWidth << 2) + (blockIndexInHeight << 2) * srcStride]), dcValue, srcStride);

		}
	}

	return satd;
}
/*******************************************
 *   returns NxM Sum of Absolute Transformed Differences using Compute4x4Satd
 *******************************************/
EB_U64 ComputeNxMSatdSadLCU(
    EB_U8  *src,        // input parameter, source samples Ptr
    EB_U32  srcStride,  // input parameter, source stride
    EB_U32  width,      // input parameter, block width (N)
    EB_U32  height)     // input parameter, block height (M) 
{
    EB_U64 satd = 0;
    EB_U64  dcValue = 0;
    EB_U64  acValue = 0;

    if (width >= 8 && height >= 8){
        satd = ComputeNxMSatd8x8Units_U8(
            src,
            srcStride,
            width,
            height,
            &dcValue);
    }
    else{
        satd =
            ComputeNxMSatd4x4Units_U8(
            src,
            srcStride,
            width,
            height,
            &dcValue);

    }

    acValue = satd - (dcValue >> 2);

    return acValue;
}

/*******************************************
 * Picture Fast Distortion
 *  Used in the Fast Mode Decision Loop
 *******************************************/
EB_ERRORTYPE PictureFastDistortion(
    EbPictureBufferDesc_t   *input,
    EB_U32                   inputLumaOriginIndex,
    EB_U32                   inputChromaOriginIndex,
    EbPictureBufferDesc_t   *pred,
    EB_U32                   predLumaOriginIndex,
    EB_U32                   predChromaOriginIndex,
    EB_U32                   size,
    EB_U32                   componentMask,
    EB_U64                   lumaDistortion[DIST_CALC_TOTAL],
    EB_U64                   chromaDistortion[DIST_CALC_TOTAL])
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    const EB_U32 chromaSize = size == 4 ? 4 : size >> 1;

    // Y
    if (componentMask & PICTURE_BUFFER_DESC_Y_FLAG) {

        lumaDistortion[DIST_CALC_RESIDUAL] += NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][size >> 3](
            &(input->bufferY[inputLumaOriginIndex]),
            input->strideY,
            &(pred->bufferY[predLumaOriginIndex]),
            64,
            size,
            size);
    }

    // Cb
    if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {

        chromaDistortion[DIST_CALC_RESIDUAL] += NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][chromaSize >> 3]( // Always SAD without weighting
            &(input->bufferCb[inputChromaOriginIndex]),
            input->strideCb,
            &(pred->bufferCb[predChromaOriginIndex]),
            32,
            chromaSize,
            chromaSize);
    }

    // Cr
    if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {

        chromaDistortion[DIST_CALC_RESIDUAL] += NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][chromaSize >> 3]( // Always SAD without weighting
            &(input->bufferCr[inputChromaOriginIndex]),
            input->strideCr,
            &(pred->bufferCr[predChromaOriginIndex]),
            32,
            chromaSize,
            chromaSize);
    }

    return return_error;
}

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
	EB_MODETYPE				 mode)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    //TODO due to a change in full kernel distortion , ASM has to be updated to not accumulate the input distortion by the output
    lumaDistortion[0]   = 0;
    lumaDistortion[1]   = 0;
    cbDistortion[0]     = 0;
    cbDistortion[1]     = 0;
    crDistortion[0]     = 0;
    crDistortion[1]     = 0;

    // Y
    if (componentMask & PICTURE_BUFFER_DESC_Y_FLAG) {
			
		FullDistortionIntrinsic_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][countNonZeroCoeffs[0] != 0][mode == INTRA_MODE][areaSize>>3](
            &(((EB_S16*) coeff->bufferY)[coeffLumaOriginIndex]),
            coeff->strideY,
            &(((EB_S16*) reconCoeff->bufferY)[coeffLumaOriginIndex]),
            reconCoeff->strideY,           
            lumaDistortion,
            areaSize,	
            areaSize);
    }

    // Cb
    if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
        
		FullDistortionIntrinsic_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][countNonZeroCoeffs[1] != 0][mode == INTRA_MODE][chromaAreaSize >> 3](
            &(((EB_S16*) coeff->bufferCb)[coeffChromaOriginIndex]),
            coeff->strideCb,
            &(((EB_S16*) reconCoeff->bufferCb)[coeffChromaOriginIndex]),
            reconCoeff->strideCb,           
            cbDistortion,
            chromaAreaSize,
            chromaAreaSize);
    }

    // Cr
    if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {

		FullDistortionIntrinsic_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][countNonZeroCoeffs[2] != 0][mode == INTRA_MODE][chromaAreaSize >> 3](
            &(((EB_S16*) coeff->bufferCr)[coeffChromaOriginIndex]),
            coeff->strideCr,
            &(((EB_S16*) reconCoeff->bufferCr)[coeffChromaOriginIndex]),
            reconCoeff->strideCr,         
            crDistortion,
            chromaAreaSize,
            chromaAreaSize);
    }

    return return_error;
}


/*******************************************
 * Picture Full Distortion
 *  Used in the Full Mode Decision Loop for the only case of a MVP-SKIP candidate
 *******************************************/

EB_ERRORTYPE PictureFullDistortionLuma(
    EbPictureBufferDesc_t   *coeff,
    EB_U32                   coeffLumaOriginIndex,
    EbPictureBufferDesc_t   *reconCoeff,
    EB_U32                   reconCoeffLumaOriginIndex,
    EB_U32                   areaSize,
    EB_U64                   lumaDistortion[DIST_CALC_TOTAL],
	EB_U32                   countNonZeroCoeffsY,
	EB_MODETYPE				 mode)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    
    //TODO due to a change in full kernel distortion , ASM has to be updated to not accumulate the input distortion by the output
    lumaDistortion[0]   = 0;
    lumaDistortion[1]   = 0;
	
    // Y
	FullDistortionIntrinsic_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][countNonZeroCoeffsY != 0][mode == INTRA_MODE][areaSize >> 3](
        &(((EB_S16*) coeff->bufferY)[coeffLumaOriginIndex]),
        coeff->strideY,
        &(((EB_S16*) reconCoeff->bufferY)[reconCoeffLumaOriginIndex]),
        reconCoeff->strideY,           
        lumaDistortion,
        areaSize,
        areaSize);


    return return_error;
}


void extract8Bitdata(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,   
    EB_U32       width,
    EB_U32       height
    )
{
    
    UnPack8BIT_funcPtrArray_16Bit[((width & 3) == 0) && ((height & 1)== 0)][!!(ASM_TYPES & PREAVX2_MASK)](
        in16BitBuffer,
        inStride,
        out8BitBuffer,    
        out8Stride,   
        width,
        height);
}
void UnpackL0L1Avg(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,      
        EB_U32  width,
        EB_U32  height)
 {
 
     UnPackAvg_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
        ref16L0,
        refL0Stride,
        ref16L1,
        refL1Stride,
        dstPtr,
        dstStride,      
        width,
        height);
     
 
 }
void Extract8BitdataSafeSub(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,   
    EB_U32       width,
    EB_U32       height
    )
{

    UnPack8BITSafeSub_funcPtrArray_16Bit[!!(ASM_TYPES & AVX2_MASK)](
        in16BitBuffer,
        inStride,
        out8BitBuffer,    
        out8Stride,   
        width,
        height
        );
}
void UnpackL0L1AvgSafeSub(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,      
        EB_U32  width,
        EB_U32  height)
 {
     //fix C

     UnPackAvgSafeSub_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
        ref16L0,
        refL0Stride,
        ref16L1,
        refL1Stride,
        dstPtr,
        dstStride,   
        width,
        height);
     
 
 }
void UnPack2D(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U8       *outnBitBuffer,
    EB_U32       outnStride,
    EB_U32       width,
    EB_U32       height
    )
{
#ifndef NON_AVX512_SUPPORT
    UnPack2D_funcPtrArray_16Bit[((width & 3) == 0) && ((height & 1)== 0)][!!(ASM_TYPES & AVX512_MASK)](
#else
    UnPack2D_funcPtrArray_16Bit[((width & 3) == 0) && ((height & 1) == 0)][!!(ASM_TYPES & AVX2_MASK)](
#endif
        in16BitBuffer,
        inStride,
        out8BitBuffer,
        outnBitBuffer,
        out8Stride,
        outnStride,
        width,
        height);
}

void Pack2D_SRC(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U32     innStride,
    EB_U16    *out16BitBuffer,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height
   )
{
	 
    Pack2D_funcPtrArray_16Bit_SRC[((width & 3) == 0) && ((height & 1)== 0)][!!(ASM_TYPES & AVX2_MASK)](
        in8BitBuffer,
        in8Stride,
        innBitBuffer,
        out16BitBuffer,
        innStride,
        outStride,
        width,
        height);
}

void CompressedPackLcu(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U32     innStride,
    EB_U16    *out16BitBuffer,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height
)
{

    CompressedPack_funcPtrArray[((width == 64 || width == 32) ? (!!(ASM_TYPES & AVX2_MASK)) : EB_ASM_C)](
        in8BitBuffer,
        in8Stride,
        innBitBuffer,
        out16BitBuffer,
        innStride,
        outStride,
        width,
        height);
}


void CompressedPackBlk(
	EB_U8     *in8BitBuffer,
	EB_U32     in8Stride,
	EB_U8     *innBitBuffer,
	EB_U32     innStride,
	EB_U16    *out16BitBuffer,
	EB_U32     outStride,
	EB_U32     width,
	EB_U32     height
	)
{


	CompressedPack_funcPtrArray[((width == 64 || width == 32 || width == 16 || width == 8) ? (!!(ASM_TYPES & AVX2_MASK)) : EB_ASM_C)](
		in8BitBuffer,
		in8Stride,
		innBitBuffer,
		out16BitBuffer,
		innStride,
		outStride,
		width,
		height);

}

void Conv2bToCPackLcu(
	const EB_U8     *innBitBuffer,
	EB_U32     innStride,
	EB_U8     *inCompnBitBuffer,
	EB_U32     outStride,
	EB_U8    *localCache,
	EB_U32     width,
	EB_U32     height)
{

	Convert_Unpack_CPack_funcPtrArray[((width == 64 || width == 32) ? (!!(ASM_TYPES & AVX2_MASK)) : EB_ASM_C)](
		innBitBuffer,
		innStride,
		inCompnBitBuffer,
		outStride,
		localCache,
		width,
		height);

}


/*******************************************
 * memset16bit
 *******************************************/
void memset16bit(
    EB_U16                     * inPtr,
    EB_U16                       value,
    EB_U64                       numOfElements )
{
    EB_U64 i;

    for( i =0;  i<numOfElements;   i++) {
        inPtr[i]  = value;
    }
}
/*******************************************
 * memcpy16bit
 *******************************************/
void memcpy16bit(
    EB_U16                     * outPtr,
    EB_U16                     * inPtr,
    EB_U64                       numOfElements )
{
    EB_U64 i;

    for( i =0;  i<numOfElements;   i++) {
        outPtr[i]  =  inPtr[i]  ;
    }
}

 
EB_S32  sumResidual( EB_S16 * inPtr,
                     EB_U32   size,
                     EB_U32   strideIn )
{

    EB_S32 sumBlock = 0;
    EB_U32 i,j;

    for(j=0; j<size;    j++)
         for(i=0; i<size;    i++)
             sumBlock+=inPtr[j*strideIn + i];

    return sumBlock;

}

void memset16bitBlock (
                    EB_S16 * inPtr,
                    EB_U32   strideIn,
                    EB_U32   size,
                    EB_S16   value    )
{

    EB_U32 i;
    for (i = 0; i < size; i++)
       memset16bit((EB_U16*)inPtr + i*strideIn, value, size);

}  


void UnusedVariablevoidFunc_PicOper()
{
    (void)NxMSadKernel_funcPtrArray;
	(void)NxMSadLoopKernel_funcPtrArray;
    (void)NxMSadAveragingKernel_funcPtrArray;
}
