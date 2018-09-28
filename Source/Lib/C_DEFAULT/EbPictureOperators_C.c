/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPictureOperators_C.h"
#include "EbUtility.h"

/*********************************
* Picture Average
*********************************/
void PictureAverageKernel(
    EB_BYTE                  src0,
    EB_U32                   src0Stride,
    EB_BYTE                  src1,
    EB_U32                   src1Stride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight)
{
    EB_U32 x, y;

    for (y = 0; y < areaHeight; y++) {
        for (x = 0; x < areaWidth; x++) {
            dst[x] = (src0[x] + src1[x] + 1) >> 1;
        }
        src0 += src0Stride;
        src1 += src1Stride;
        dst += dstStride;
    }
}

void PictureAverageKernel1Line_C(
    EB_BYTE                  src0,
    EB_BYTE                  src1,
    EB_BYTE                  dst,
    EB_U32                   areaWidth)
{
    EB_U32 i;
    for (i = 0; i < areaWidth; i++)
        dst[i] = (src0[i] + src1[i] + 1) / 2;

}

/*********************************
* Picture Copy Kernel
*********************************/
void PictureCopyKernel(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight,
    EB_U32                   bytesPerSample)  //=1 always)
{
    EB_U32 sampleCount = 0;
    const EB_U32 sampleTotalCount = areaWidth*areaHeight;
    const EB_U32 copyLength = areaWidth * bytesPerSample;

    srcStride *= bytesPerSample;
    dstStride *= bytesPerSample;

    while (sampleCount < sampleTotalCount) {
        EB_MEMCPY(dst, src, copyLength);
        src += srcStride;
        dst += dstStride;
        sampleCount += areaWidth;
    }

    return;
}

/*********************************
* Picture Single Channel Kernel
*********************************/
void PictureAdditionKernel(
    EB_U8  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U8  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height)
{
    EB_U32          columnIndex;
    EB_U32          rowIndex = 0;
    const EB_S32    maxValue = 0xFF;

    while (rowIndex < height) {

        columnIndex = 0;
        while (columnIndex < width) {
            reconPtr[columnIndex] = (EB_U8)CLIP3(0, maxValue, ((EB_S32)residualPtr[columnIndex]) + ((EB_S32)predPtr[columnIndex]));
            ++columnIndex;
        }

        residualPtr += residualStride;
        predPtr += predStride;
        reconPtr += reconStride;
        ++rowIndex;
    }

    return;
}

/*********************************
* Picture addtion 16bit Kernel
*********************************/
void PictureAdditionKernel16bit(
    EB_U16  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U16  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height)
{
    EB_U32          columnIndex;
    EB_U32          rowIndex = 0;
    const EB_S32    maxValue = 0x3FF;//0xFF;//CHKN

    while (rowIndex < height) {

        columnIndex = 0;
        while (columnIndex < width) {
            reconPtr[columnIndex] = (EB_U16)CLIP3(0, maxValue, ((EB_S32)residualPtr[columnIndex]) + ((EB_S32)predPtr[columnIndex]));
            ++columnIndex;
        }

        residualPtr += residualStride;
        predPtr += predStride;
        reconPtr += reconStride;
        ++rowIndex;
    }

    return;
}

/*********************************
* Copy Kernel 8 bits
*********************************/
void CopyKernel8Bit(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight)
{

    EB_U32                   bytesPerSample = 1;

    EB_U32 sampleCount = 0;
    const EB_U32 sampleTotalCount = areaWidth*areaHeight;
    const EB_U32 copyLength = areaWidth * bytesPerSample;

    srcStride *= bytesPerSample;
    dstStride *= bytesPerSample;

    while (sampleCount < sampleTotalCount) {
        EB_MEMCPY(dst, src, copyLength);
        src += srcStride;
        dst += dstStride;
        sampleCount += areaWidth;
    }

    return;
}
/*********************************
* Copy Kernel 16 bits
*********************************/
void CopyKernel16Bit(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight)
{

    EB_U32                   bytesPerSample = 2;

    EB_U32 sampleCount = 0;
    const EB_U32 sampleTotalCount = areaWidth*areaHeight;
    const EB_U32 copyLength = areaWidth * bytesPerSample;

    srcStride *= bytesPerSample;
    dstStride *= bytesPerSample;

    while (sampleCount < sampleTotalCount) {
        EB_MEMCPY(dst, src, copyLength);
        src += srcStride;
        dst += dstStride;
        sampleCount += areaWidth;
    }

    return;
}

//computes a subsampled residual and then duplicate it
void ResidualKernelSubSampled(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight,
    EB_U8    lastLine)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;

    EB_U8   *input_O = input;
    EB_U8   *pred_O = pred;
    EB_S16  *residual_O = residual;


    //hard code subampling dimensions, keep residualStride
    areaHeight >>= 1;
    inputStride <<= 1;
    predStride <<= 1;


    while (rowIndex < areaHeight) {
        columnIndex = 0;
        while (columnIndex < areaWidth) {
            residual[columnIndex] = ((EB_S16)input[columnIndex]) - ((EB_S16)pred[columnIndex]);
            residual[columnIndex + residualStride] = ((EB_S16)input[columnIndex]) - ((EB_S16)pred[columnIndex]);
            ++columnIndex;
        }

        input += inputStride;
        pred += predStride;
        residual += (residualStride << 1);
        ++rowIndex;
    }

    //do the last line:
    if (lastLine) {
        inputStride = inputStride / 2;
        predStride = predStride / 2;
        areaHeight = areaHeight * 2;
        columnIndex = 0;
        while (columnIndex < areaWidth) {
            residual_O[(areaHeight - 1)*residualStride + columnIndex] = ((EB_S16)input_O[(areaHeight - 1)*inputStride + columnIndex]) - ((EB_S16)pred_O[(areaHeight - 1)*predStride + columnIndex]);
            ++columnIndex;
        }

    }
    return;
}

/*******************************************
* Residual Kernel
Computes the residual data
*******************************************/
void ResidualKernel(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;

    while (rowIndex < areaHeight) {
        columnIndex = 0;
        while (columnIndex < areaWidth) {
            residual[columnIndex] = ((EB_S16)input[columnIndex]) - ((EB_S16)pred[columnIndex]);
            ++columnIndex;
        }

        input += inputStride;
        pred += predStride;
        residual += residualStride;
        ++rowIndex;
    }

    return;
}

/*******************************************
* Residual Kernel 16bit
Computes the residual data
*******************************************/
void ResidualKernel16bit(
    EB_U16   *input,
    EB_U32   inputStride,
    EB_U16   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;

    while (rowIndex < areaHeight) {
        columnIndex = 0;
        while (columnIndex < areaWidth) {
            residual[columnIndex] = ((EB_S16)input[columnIndex]) - ((EB_S16)pred[columnIndex]);
            ++columnIndex;
        }

        input += inputStride;
        pred += predStride;
        residual += residualStride;
        ++rowIndex;
    }

    return;
}

/*********************************
* Zero Out Coeff Kernel
*********************************/
void ZeroOutCoeffKernel(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight)
{
    EB_U32 i, j;

    for (j = 0; j < areaHeight; ++j) {
        for (i = 0; i < areaWidth; ++i) {
            coeffbuffer[j*coeffStride + i + coeffOriginIndex] = 0;
        }
    }

}

// C equivalents
static EB_S32 SQR16to32(EB_S16 x)
{
    return x * x;
}

EB_EXTERN void FullDistortionKernelIntra_32bit(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;
    EB_U32  residualDistortion = 0;

    while (rowIndex < areaHeight) {

        columnIndex = 0;
        while (columnIndex < areaWidth) {
            residualDistortion += SQR16to32(coeff[columnIndex] - reconCoeff[columnIndex]);
            ++columnIndex;
        }

        coeff += coeffStride;
        reconCoeff += reconCoeffStride;
        ++rowIndex;
    }

    distortionResult[0] = residualDistortion;
    distortionResult[1] = residualDistortion;
}

EB_EXTERN void FullDistortionKernel_32bit(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;
    EB_U32  residualDistortion = 0;
    EB_U32  predictionDistortion = 0;

    while (rowIndex < areaHeight) {

        columnIndex = 0;
        while (columnIndex < areaWidth) {
            residualDistortion += SQR16to32(coeff[columnIndex] - reconCoeff[columnIndex]);
            predictionDistortion += SQR16to32(coeff[columnIndex]);
            ++columnIndex;
        }

        coeff += coeffStride;
        reconCoeff += reconCoeffStride;
        ++rowIndex;
    }

    distortionResult[0] = residualDistortion;
    distortionResult[1] = predictionDistortion;
}

EB_EXTERN void FullDistortionKernelCbfZero_32bit(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;
    EB_U32  predictionDistortion = 0;

    while (rowIndex < areaHeight) {

        columnIndex = 0;
        while (columnIndex < areaWidth) {
            predictionDistortion += SQR16to32(coeff[columnIndex]);
            ++columnIndex;
        }

        coeff += coeffStride;
        reconCoeff += reconCoeffStride;
        ++rowIndex;
    }

    distortionResult[0] = predictionDistortion;
    distortionResult[1] = predictionDistortion;
}

EB_U64 SpatialFullDistortionKernel(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *recon,
    EB_U32   reconStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight)
{
    EB_U32  columnIndex;
    EB_U32  rowIndex = 0;

    EB_U64  spatialDistortion = 0;

    while (rowIndex < areaHeight) {

        columnIndex = 0;
        while (columnIndex < areaWidth) {
            spatialDistortion += (EB_S64)SQR((EB_S64)(input[columnIndex]) - (recon[columnIndex]));
            ++columnIndex;
        }

        input += inputStride;
        recon += reconStride;
        ++rowIndex;
    }
    return spatialDistortion;
}