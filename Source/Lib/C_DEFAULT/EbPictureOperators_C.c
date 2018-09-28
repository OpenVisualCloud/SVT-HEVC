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


/*******************************************
* Compute8x8Satd
*   returns 8x8 Sum of Absolute Transformed Differences
*******************************************/
EB_U64 Compute8x8Satd(
    EB_S16 *diff)       // input parameter, diff samples Ptr
{
    EB_U64 satdBlock8x8 = 0;
    EB_S16 m1[8][8], m2[8][8], m3[8][8];
    EB_U32 i, j, jj;

    // Horizontal
    for (j = 0; j < 8; j++) {
        jj = j << 3;
        m2[j][0] = diff[jj] + diff[jj + 4];
        m2[j][1] = diff[jj + 1] + diff[jj + 5];
        m2[j][2] = diff[jj + 2] + diff[jj + 6];
        m2[j][3] = diff[jj + 3] + diff[jj + 7];
        m2[j][4] = diff[jj] - diff[jj + 4];
        m2[j][5] = diff[jj + 1] - diff[jj + 5];
        m2[j][6] = diff[jj + 2] - diff[jj + 6];
        m2[j][7] = diff[jj + 3] - diff[jj + 7];

        m1[j][0] = m2[j][0] + m2[j][2];
        m1[j][1] = m2[j][1] + m2[j][3];
        m1[j][2] = m2[j][0] - m2[j][2];
        m1[j][3] = m2[j][1] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][6];
        m1[j][5] = m2[j][5] + m2[j][7];
        m1[j][6] = m2[j][4] - m2[j][6];
        m1[j][7] = m2[j][5] - m2[j][7];

        m2[j][0] = m1[j][0] + m1[j][1];
        m2[j][1] = m1[j][0] - m1[j][1];
        m2[j][2] = m1[j][2] + m1[j][3];
        m2[j][3] = m1[j][2] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][5];
        m2[j][5] = m1[j][4] - m1[j][5];
        m2[j][6] = m1[j][6] + m1[j][7];
        m2[j][7] = m1[j][6] - m1[j][7];
    }

    // Vertical
    for (i = 0; i < 8; i++) {
        m3[0][i] = m2[0][i] + m2[4][i];
        m3[1][i] = m2[1][i] + m2[5][i];
        m3[2][i] = m2[2][i] + m2[6][i];
        m3[3][i] = m2[3][i] + m2[7][i];
        m3[4][i] = m2[0][i] - m2[4][i];
        m3[5][i] = m2[1][i] - m2[5][i];
        m3[6][i] = m2[2][i] - m2[6][i];
        m3[7][i] = m2[3][i] - m2[7][i];

        m1[0][i] = m3[0][i] + m3[2][i];
        m1[1][i] = m3[1][i] + m3[3][i];
        m1[2][i] = m3[0][i] - m3[2][i];
        m1[3][i] = m3[1][i] - m3[3][i];
        m1[4][i] = m3[4][i] + m3[6][i];
        m1[5][i] = m3[5][i] + m3[7][i];
        m1[6][i] = m3[4][i] - m3[6][i];
        m1[7][i] = m3[5][i] - m3[7][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
    }

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            satdBlock8x8 += (EB_U64)ABS(m2[i][j]);
        }
    }

    satdBlock8x8 = ((satdBlock8x8 + 2) >> 2);

    return satdBlock8x8;
}



EB_U64 Compute8x8Satd_U8(
    EB_U8  *src,       // input parameter, diff samples Ptr
    EB_U64 *dcValue,
    EB_U32  srcStride)
{
    EB_U64 satdBlock8x8 = 0;
    EB_S16 m1[8][8], m2[8][8], m3[8][8];
    EB_U32 i, j;

    // Horizontal
    for (j = 0; j < 8; j++) {
        m2[j][0] = src[j*srcStride] + src[j*srcStride + 4];
        m2[j][1] = src[j*srcStride + 1] + src[j*srcStride + 5];
        m2[j][2] = src[j*srcStride + 2] + src[j*srcStride + 6];
        m2[j][3] = src[j*srcStride + 3] + src[j*srcStride + 7];
        m2[j][4] = src[j*srcStride] - src[j*srcStride + 4];
        m2[j][5] = src[j*srcStride + 1] - src[j*srcStride + 5];
        m2[j][6] = src[j*srcStride + 2] - src[j*srcStride + 6];
        m2[j][7] = src[j*srcStride + 3] - src[j*srcStride + 7];

        m1[j][0] = m2[j][0] + m2[j][2];
        m1[j][1] = m2[j][1] + m2[j][3];
        m1[j][2] = m2[j][0] - m2[j][2];
        m1[j][3] = m2[j][1] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][6];
        m1[j][5] = m2[j][5] + m2[j][7];
        m1[j][6] = m2[j][4] - m2[j][6];
        m1[j][7] = m2[j][5] - m2[j][7];

        m2[j][0] = m1[j][0] + m1[j][1];
        m2[j][1] = m1[j][0] - m1[j][1];
        m2[j][2] = m1[j][2] + m1[j][3];
        m2[j][3] = m1[j][2] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][5];
        m2[j][5] = m1[j][4] - m1[j][5];
        m2[j][6] = m1[j][6] + m1[j][7];
        m2[j][7] = m1[j][6] - m1[j][7];
    }

    // Vertical
    for (i = 0; i < 8; i++) {
        m3[0][i] = m2[0][i] + m2[4][i];
        m3[1][i] = m2[1][i] + m2[5][i];
        m3[2][i] = m2[2][i] + m2[6][i];
        m3[3][i] = m2[3][i] + m2[7][i];
        m3[4][i] = m2[0][i] - m2[4][i];
        m3[5][i] = m2[1][i] - m2[5][i];
        m3[6][i] = m2[2][i] - m2[6][i];
        m3[7][i] = m2[3][i] - m2[7][i];

        m1[0][i] = m3[0][i] + m3[2][i];
        m1[1][i] = m3[1][i] + m3[3][i];
        m1[2][i] = m3[0][i] - m3[2][i];
        m1[3][i] = m3[1][i] - m3[3][i];
        m1[4][i] = m3[4][i] + m3[6][i];
        m1[5][i] = m3[5][i] + m3[7][i];
        m1[6][i] = m3[4][i] - m3[6][i];
        m1[7][i] = m3[5][i] - m3[7][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
    }

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            satdBlock8x8 += (EB_U64)ABS(m2[i][j]);
        }
    }

    satdBlock8x8 = ((satdBlock8x8 + 2) >> 2);
    *dcValue += m2[0][0];
    return satdBlock8x8;
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