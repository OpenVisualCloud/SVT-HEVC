/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbAvcStyleMcp_C.h"
#include "EbPictureOperators_C.h"
#include "EbTypes.h"
#include "EbUtility.h"
#include "EbDefinitions.h"


static const EB_S8 AvcStyleLumaIFCoeff[4][4] = {
        { 0, 0, 0, 0 },
        { -1, 25, 9, -1 },
        { -2, 18, 18, -2 },
        { -1, 9, 25, -1 },
};

void AvcStyleCopyNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos)
{
    (void)tempBuf;
    (void)fracPos;
    PictureCopyKernel(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void AvcStyleLumaInterpolationFilterHorizontal(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos)
{
    const EB_S8  *IFCoeff = AvcStyleLumaIFCoeff[fracPos];
    const EB_S32  IFInitPosOffset = -1;
    const EB_U8   IFShift = 5;
    const EB_S16  IFOffset = POW2(IFShift - 1);
    EB_U32        x, y;
    (void)tempBuf;

    refPic += IFInitPosOffset;
    for (y = 0; y < puHeight; y++) {
        for (x = 0; x < puWidth; x++) {
            dst[x] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE,
                (refPic[x] * IFCoeff[0] + refPic[x + 1] * IFCoeff[1] + refPic[x + 2] * IFCoeff[2] + refPic[x + 3] * IFCoeff[3] + IFOffset) >> IFShift);
        }
        refPic += srcStride;
        dst += dstStride;
    }
}

void AvcStyleLumaInterpolationFilterVertical(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos)
{
    const EB_S8  *IFCoeff = AvcStyleLumaIFCoeff[fracPos];
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -(EB_S32)srcStride;
    const EB_U8   IFShift = 5;
    const EB_S32  IFOffset = POW2(IFShift - 1);
    EB_U32        x, y;
    (void)tempBuf;

    refPic += IFInitPosOffset;
    for (y = 0; y < puHeight; y++) {
        for (x = 0; x < puWidth; x++) {
            dst[x] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE,
                (refPic[x] * IFCoeff[0] + refPic[x + IFStride] * IFCoeff[1] + refPic[x + 2 * IFStride] * IFCoeff[2] + refPic[x + 3 * IFStride] * IFCoeff[3] + IFOffset) >> IFShift);
        }
        refPic += srcStride;
        dst += dstStride;
    }
}


// Weighted Prediction

void WpAvcStyleCopy(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{

    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_BYTE  tempDst = dst;

    EB_U8    IFShift = wpDenominator;
    EB_S32   IFOffset = (wpDenominator == 0) ? 0 : 1 << (wpDenominator - 1);


    (void)tempBuf;
    (void)fracPos;
    PictureCopyKernel(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);


    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx) {

            *(tempDst + columnIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (*(tempDst + columnIdx))) + IFOffset) >> IFShift) + wpOffset);
        }

        tempDst += dstStride;
    }
}

void WpAvcStyleLumaInterpolationFilterHorizontal(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S8  *IFCoeff = AvcStyleLumaIFCoeff[fracPos];
    const EB_S32  IFInitPosOffset = -1;
    const EB_U8   IFShift = 5 + wpDenominator;
    const EB_S16  IFOffset = POW2(IFShift - 1);

    EB_U32        x, y;
    EB_S32        sum;

    (void)tempBuf;

    refPic += IFInitPosOffset;
    for (y = 0; y < puHeight; y++) {
        for (x = 0; x < puWidth; x++) {

            sum = refPic[x] * IFCoeff[0] + refPic[x + 1] * IFCoeff[1] + refPic[x + 2] * IFCoeff[2] + refPic[x + 3] * IFCoeff[3];
            dst[x] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * sum) + IFOffset) >> IFShift) + wpOffset);
        }

        refPic += srcStride;
        dst += dstStride;
    }
}

void WpAvcStyleLumaInterpolationFilterVertical(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S8  *IFCoeff = AvcStyleLumaIFCoeff[fracPos];
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -(EB_S32)srcStride;
    const EB_U8   IFShift = 5 + wpDenominator;
    const EB_S32  IFOffset = POW2(IFShift - 1);
    EB_U32        x, y;
    EB_S32        sum;

    (void)tempBuf;

    refPic += IFInitPosOffset;
    for (y = 0; y < puHeight; y++) {
        for (x = 0; x < puWidth; x++) {

            sum = refPic[x] * IFCoeff[0] + refPic[x + IFStride] * IFCoeff[1] + refPic[x + 2 * IFStride] * IFCoeff[2] + refPic[x + 3 * IFStride] * IFCoeff[3];
            dst[x] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * sum) + IFOffset) >> IFShift) + wpOffset);

        }
        refPic += srcStride;
        dst += dstStride;
    }
}

void WpAvcStyleLumaInterpolationFilterPose(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic, srcStride, tempBuf, puWidth, puWidth, puHeight, 0,  2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterVertical(refPic, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosf(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterPosj(refPic, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, tempBuf + 2 * tempBufSize, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosg(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterVertical(refPic + 1, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosi(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterVertical(refPic, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterPosj(refPic, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, tempBuf + 2 * tempBufSize, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosj(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic - srcStride, srcStride, tempBuf, puWidth, puWidth, (puHeight + 4), 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterVertical(tempBuf + puWidth, puWidth, dst, dstStride, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
}

void WpAvcStyleLumaInterpolationFilterPosk(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterVertical(refPic + 1, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterPosj(refPic, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, tempBuf + 2 * tempBufSize, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosp(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterVertical(refPic, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic + srcStride, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosq(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic + srcStride, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterPosj(refPic, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, tempBuf + 2 * tempBufSize, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

void WpAvcStyleLumaInterpolationFilterPosr(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32 tempBufSize = puWidth*puHeight;
    (void)fracPos;
    WpAvcStyleLumaInterpolationFilterVertical(refPic + 1, srcStride, tempBuf, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    WpAvcStyleLumaInterpolationFilterHorizontal(refPic + srcStride, srcStride, tempBuf + tempBufSize, puWidth, puWidth, puHeight, 0, 2, wpWeight, wpOffset, wpDenominator);
    PictureAverageKernel(tempBuf, puWidth, tempBuf + tempBufSize, puWidth, dst, dstStride, puWidth, puHeight);
}

