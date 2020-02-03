/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbMcp_C.h"
#include "EbPictureOperators_C.h"
#include "EbDefinitions.h"
#include "EbUtility.h"

static const EB_S16 chromaIFCoeff[7][MaxChromaFilterTag] = {
        { -2, 58, 10, -2 },
        { -4, 54, 16, -2 },
        { -6, 46, 28, -4 },
        { -4, 36, 36, -4 },
        { -4, 28, 46, -6 },
        { -2, 16, 54, -4 },
        { -2, 10, 58, -2 },
};

void BiPredClipping(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_BYTE    dst,
    EB_U32     dstStride,
    EB_S32     offset)
{
    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_U32   tempBufIdx0, tempBufIdx1, tempDstIdx;
    EB_BYTE  tempDstY;

    tempBufIdx0 = 0;
    tempBufIdx1 = 0;
    tempDstIdx = 0;
    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        tempDstY = dst + tempDstIdx;
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx)
        {
            *(tempDstY + columnIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, ((list0Src[tempBufIdx0 + columnIdx] + list1Src[tempBufIdx1 + columnIdx] + offset) >> Shift5));
        }
        tempBufIdx0 += puWidth;
        tempBufIdx1 += puWidth;
        tempDstIdx += dstStride;
    }
}

void LumaInterpolationFilterPosdInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = Offset4;
    EB_U8         IFShift = Shift4;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPoshInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = Offset4;
    EB_U8         IFShift = Shift4;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPosnInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = Offset4;
    EB_U8         IFShift = Shift4;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += 2 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPosnInRawOutRaw(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = 0;
    EB_U8         IFShift = Shift2;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += 2 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}



/** uniPredCopy()
simply copies the data from the reference picture array to the destination buffer.
*/
void LumaInterpolationCopy(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    (void)firstPassIFDst;
    PictureCopyKernel(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosaNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };             // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void LumaInterpolationFilterPosbNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void LumaInterpolationFilterPoscNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                     // to be modified
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void LumaInterpolationFilterPosdNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void LumaInterpolationFilterPoseNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosfNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosgNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}
void LumaInterpolationFilterPoshNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                            // to be modified
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void LumaInterpolationFilterPosiNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosjNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}


void LumaInterpolationFilterPoskNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}
void LumaInterpolationFilterPosnNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void LumaInterpolationFilterPospNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPospOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosqNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosrNew(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosbOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified
    EB_S32        IFOffset = -1 * MinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif
}

void LumaInterpolationFilterPoscOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                     // to be modified
    EB_S32        IFOffset = -1 * MinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif
}


void LumaInterpolationFilterPosdOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = -1 * MinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}


void LumaInterpolationFilterPosdInRawOutRaw(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = 0;
    EB_U8         IFShift = Shift2;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPoseOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosfOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}


void LumaInterpolationFilterPosgOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPoshOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                            // to be modified
    EB_S32        IFOffset = -1 * MinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}


void LumaInterpolationFilterPoshInRawOutRaw(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    const EB_S16  vericalIFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = 0;
    EB_U8         IFShift = Shift2;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPosiOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosjOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}



void LumaInterpolationFilterPoskOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosnOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = -1 * MinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void LumaInterpolationFilterPosqOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosrOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight);
}

void WpLumaInterpolationFilterPosa16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void WpLumaInterpolationFilterPosb16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16*       tempRefPic;
    EB_U16*       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[7];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void WpLumaInterpolationFilterPosc16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16*       tempRefPic;
    EB_U16*       tempDst;


    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[6];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void WpLumaInterpolationFilterPosd16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16*       tempRefPic;
    EB_U16*       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void WpLumaInterpolationFilterPosdInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6];

            sum = ((sum >> WP_SHIFT2D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void WpLumaInterpolationFilterPose16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosdInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosf16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosdInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosg16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosdInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosh16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[7];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void WpLumaInterpolationFilterPoshInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[7];

            sum = ((sum >> WP_SHIFT2D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpLumaInterpolationFilterPosi16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPoshInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosj16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPoshInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosk16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPoshInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosn16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[6];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void WpLumaInterpolationFilterPosnInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += 2 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[6];

            sum = ((sum >> WP_SHIFT2D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpLumaInterpolationFilterPosp16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosnInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosq16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosnInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}
void WpLumaInterpolationFilterPosr16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosnInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpChromaInterpolationCopy16bit(
    EB_U16*               refPic,
    EB_U32                srcStride,
    EB_U16*               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32   verticalIdx = puHeight;
    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_U16  *tempDst = dst;

    EB_U8    WpIFShift = wpDenominator;
    EB_S32   WpIFOffset = (wpDenominator == 0) ? 0 : 1 << (wpDenominator - 1);

    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;

    do
    {
        EB_MEMCPY(dst, refPic, puWidth*sizeof(EB_U16));
        refPic += srcStride;
        dst += dstStride;
    } while (--verticalIdx);

    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx) {

            *(tempDst + columnIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * (*(tempDst + columnIdx))) + WpIFOffset) >> WpIFShift) + wpOffset);
        }

        tempDst += dstStride;
    }

    return;
}

void WpChromaInterpolationFilterOneD16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];

            sum = (((sum + WP_OFFSET1D_10BIT) >> WP_SHIFT1D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpChromaInterpolationFilterTwoDInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_S32        firstPassIFDstStride = puWidth;

    const EB_S16 *vericalIFCoeff = *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -1 * ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;

    EB_S32        WpIFShift = WP_SHIFT_10BIT + wpDenominator;
    EB_S32        WpIFOffset = (1 << (WP_SHIFT_10BIT + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3];

            sum = ((sum >> WP_SHIFT2D_10BIT) + WP_IF_OFFSET_10BIT);
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * sum) + WpIFOffset) >> WpIFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpChromaInterpolationFilterTwoD16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw16bit(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    WpChromaInterpolationFilterTwoDInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy, wpWeight, wpOffset, wpDenominator);
}

void WpBiPredClipping16bit(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_U16    *dst,
    EB_U32     dstStride,
    EB_S16     wpWeightList0,
    EB_S16     wpWeightList1,
    EB_U8      wpDenominator,
    EB_S16     wpOffset)
{
    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_U32   tempBufIdx0, tempBufIdx1, tempDstIdx;
    EB_U16  *tempDstY;

    EB_U8         IFShift = WP_BI_SHIFT_10BIT + wpDenominator;
    EB_S32        IFOffset = (1 << (WP_BI_SHIFT_10BIT + wpDenominator - 1));

    tempBufIdx0 = 0;
    tempBufIdx1 = 0;
    tempDstIdx = 0;
    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        tempDstY = dst + tempDstIdx;
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx)
        {
            *(tempDstY + columnIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeightList0 * (list0Src[tempBufIdx0 + columnIdx] + WP_IF_OFFSET_10BIT)) + (wpWeightList1 * (list1Src[tempBufIdx1 + columnIdx] + WP_IF_OFFSET_10BIT)) + IFOffset + (wpOffset << (IFShift - 1))) >> IFShift));
        }
        tempBufIdx0 += puWidth;
        tempBufIdx1 += puWidth;
        tempDstIdx += dstStride;
    }
}

// Weighted Prediction - EncoderBitDepth : 8
void WpLumaInterpolationCopy(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{

    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_BYTE  tempDst = dst;

    EB_U8    IFShift = wpDenominator;
    EB_S32   IFOffset = (wpDenominator == 0) ? 0 : 1 << (wpDenominator - 1);

    (void)firstPassIFDst;
    PictureCopyKernel(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);

    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx) {

            *(tempDst + columnIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (*(tempDst + columnIdx))) + IFOffset) >> IFShift) + wpOffset);
        }

        tempDst += dstStride;
    }
}

void WpLumaInterpolationFilterPosa(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6];

            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * sum) + IFOffset) >> IFShift) + wpOffset);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void WpLumaInterpolationFilterPosb(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, ((wpWeight * sum + IFOffset) >> IFShift) + wpOffset);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void WpLumaInterpolationFilterPosc(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, ((wpWeight * sum + IFOffset) >> IFShift) + wpOffset);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void WpLumaInterpolationFilterPosd(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, ((wpWeight * sum + IFOffset) >> IFShift) + wpOffset);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void WpLumaInterpolationFilterPosdInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6];

            sum = (sum >> Shift3);
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (sum + MinusOffset6) + IFOffset) >> IFShift) + wpOffset));

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpLumaInterpolationFilterPose(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosdInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosf(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosdInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosg(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosdInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosh(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, ((wpWeight * sum + IFOffset) >> IFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void WpLumaInterpolationFilterPoshInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[7];

            sum = (sum >> Shift3);
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (sum + MinusOffset6) + IFOffset) >> IFShift) + wpOffset));

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpLumaInterpolationFilterPosi(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPoshInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosj(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPoshInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosk(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPoshInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosn(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {

            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, ((wpWeight * sum + IFOffset) >> IFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void WpLumaInterpolationFilterPosnInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += 2 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[6];

            sum = (sum >> Shift3);
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (sum + MinusOffset6) + IFOffset) >> IFShift) + wpOffset));

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpLumaInterpolationFilterPosp(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosnInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosq(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosnInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}

void WpLumaInterpolationFilterPosr(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    WpLumaInterpolationFilterPosnInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, wpWeight, wpOffset, wpDenominator);
}


void WpChromaInterpolationCopy(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{

    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_BYTE  tempDst = dst;

    EB_U8    IFShift = wpDenominator;
    EB_S32   IFOffset = (wpDenominator == 0) ? 0 : 1 << (wpDenominator - 1);

    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;
    PictureCopyKernel(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);

    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx) {

            *(tempDst + columnIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (*(tempDst + columnIdx))) + IFOffset) >> IFShift) + wpOffset);
        }

        tempDst += dstStride;
    }
}

void WpChromaInterpolationFilterOneD(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];

            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, ((wpWeight * sum + IFOffset) >> IFShift) + wpOffset);

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpChromaInterpolationFilterOneDOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    EB_U32        dstStride = puWidth;
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);
    EB_S32        IFOffset = -MinusOffset6;
    EB_U8         IFShift = 0;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpChromaInterpolationFilterTwoDInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_S32        firstPassIFDstStride = puWidth;

    const EB_S16 *vericalIFCoeff = *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -1 * ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;

    EB_U8         IFShift = Shift3 + wpDenominator;
    EB_S32        IFOffset = (1 << (Shift3 + wpDenominator - 1));

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3];

            sum = (sum >> Shift3);
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * (sum + MinusOffset6) + IFOffset) >> IFShift) + wpOffset));

        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void WpChromaInterpolationFilterTwoD(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    WpChromaInterpolationFilterOneDOutRaw(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    WpChromaInterpolationFilterTwoDInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy, wpWeight, wpOffset, wpDenominator);
}

void WpBiPredClipping(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_BYTE    dst,
    EB_U32     dstStride,
    EB_S32     offset,
    EB_S16     wpWeightList0,
    EB_S16     wpWeightList1,
    EB_U8      wpDenominator,
    EB_S16     wpOffset)
{
    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_U32   tempBufIdx0, tempBufIdx1, tempDstIdx;
    EB_BYTE  tempDstY;

    EB_U8         IFShift = Shift5 + wpDenominator;
    EB_S32        IFOffset = 1 << (IFShift - 1);

    tempBufIdx0 = 0;
    tempBufIdx1 = 0;
    tempDstIdx = 0;
    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        tempDstY = dst + tempDstIdx;
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx)
        {
            *(tempDstY + columnIdx) = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeightList0 * (list0Src[tempBufIdx0 + columnIdx] + offset)) +
                (wpWeightList1 * (list1Src[tempBufIdx1 + columnIdx] + offset)) +
                IFOffset + (wpOffset << (IFShift - 1))) >> IFShift));
        }
        tempBufIdx0 += puWidth;
        tempBufIdx1 += puWidth;
        tempDstIdx += dstStride;
    }
}

// Weighted Prediction - EncoderBitDepth : 16
void WpLumaInterpolationCopy16bit(
    EB_U16*               refPic,
    EB_U32                srcStride,
    EB_U16*               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_S16                wpWeight,
    EB_S16                wpOffset,
    EB_U8                 wpDenominator)
{
    EB_U32   verticalIdx = puHeight;
    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_U16  *tempDst = dst;

    EB_U8    WpIFShift = wpDenominator;
    EB_S32   WpIFOffset = (wpDenominator == 0) ? 0 : 1 << (wpDenominator - 1);

    (void)firstPassIFDst;
    do
    {
        EB_MEMCPY(dst, refPic, puWidth*sizeof(EB_U16));
        refPic += srcStride;
        dst += dstStride;
    } while (--verticalIdx);

    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx) {

            *(tempDst + columnIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (((wpWeight * (*(tempDst + columnIdx))) + WpIFOffset) >> WpIFShift) + wpOffset);
        }

        tempDst += dstStride;
    }
}


void ChromaUniPredCopy16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

void ChromaInterpolationFilterOneD16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);


void ChromaInterpolationFilterTwoD16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);


void ChromaInterpolationCopyOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

void ChromaInterpolationFilterOneDOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);
void ChromaInterpolationFilterTwoDOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy);

/** The luma interpolation phase is defined as follows:
A  a  b  c
d  e  f  g
h  i  j  k
n  p  q  r
*/

/** The chroma interpolation phase is defined as follows:
B   ab  ac  ad  ae  af  ag  ah
ba  bb  bc  bd  be  bf  bg  bh
ca  cb  cc  cd  ce  cf  cg  ch
da  db  dc  dd  de  df  dg  dh
ea  eb  ec  ed  ee  ef  eg  eh
fa  fb  fc  fd  fe  ff  fg  fh
ga  gb  gc  gd  ge  gf  gg  gh
ha  hb  hc  hd  he  hf  hg  hh
*/


void LumaInterpolationCopyOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32   dstStride = puWidth;
    EB_U32   horizontalIdx;
    EB_U32   verticalIdx = puHeight;
    EB_U16  *tempRefPic = refPic - 1 + (puHeight - 1)*srcStride;
    EB_S16  *tempDst = dst - 1 + (puHeight - 1)*dstStride;

    (void)firstPassIFDst;

    do
    {
        horizontalIdx = puWidth;
        do
        {
            *(tempDst + horizontalIdx) = (EB_S16)((*(tempRefPic + horizontalIdx)) << BI_SHIFT_10BIT) - BI_OFFSET_10BIT;
        } while (--horizontalIdx);
        tempRefPic -= srcStride;
        tempDst -= dstStride;
    } while (--verticalIdx);
}
void ChromaInterpolationCopyOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    EB_U32   dstStride = puWidth;
    EB_U32   horizontalIdx;
    EB_U32   verticalIdx = puHeight;
    EB_U16  *tempRefPic = refPic - 1 + (puHeight - 1)*srcStride;
    EB_S16  *tempDst = dst - 1 + (puHeight - 1)*dstStride;

    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;


    do
    {
        horizontalIdx = puWidth;
        do
        {
            *(tempDst + horizontalIdx) = (EB_S16)((*(tempRefPic + horizontalIdx)) << BI_SHIFT_10BIT) - BI_OFFSET_10BIT;
        } while (--horizontalIdx);
        tempRefPic -= srcStride;
        tempDst -= dstStride;
    } while (--verticalIdx);
}


void BiPredClipping16bit(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_U16    *dst,
    EB_U32     dstStride)
{
    EB_U32   rowIdx;
    EB_U32   columnIdx;
    EB_U32   tempBufIdx0, tempBufIdx1, tempDstIdx;
    EB_U16  *tempDstY;

    tempBufIdx0 = 0;
    tempBufIdx1 = 0;
    tempDstIdx = 0;
    for (rowIdx = 0; rowIdx < puHeight; ++rowIdx)
    {
        tempDstY = dst + tempDstIdx;
        for (columnIdx = 0; columnIdx < puWidth; ++columnIdx)
        {
            *(tempDstY + columnIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, ((list0Src[tempBufIdx0 + columnIdx] + list1Src[tempBufIdx1 + columnIdx] + BI_AVG_OFFSET_10BIT) >> BI_AVG_SHIFT_10BIT));
        }
        tempBufIdx0 += puWidth;
        tempBufIdx1 += puWidth;
        tempDstIdx += dstStride;
    }
}

void ChromaInterpolationFilterTwoDInRawOutRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy)
{
    EB_S32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    //vertical filtering parameter setting
    const EB_S16 *vericalIFCoeff = *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -1 * ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    EB_S32        IFOffset = BI_OFFSET2D2_10BIT;
    EB_U8         IFShift = BI_SHIFT2D2_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void ChromaInterpolationFilterTwoDOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw16bit(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight, fracPosy);
}

void LumaInterpolationFilterPosdOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                          // to be modified
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void LumaInterpolationFilterPoshOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void LumaInterpolationFilterPosnOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void LumaInterpolationFilterPosdInRawOutRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = BI_OFFSET2D2_10BIT;
    EB_U8         IFShift = BI_SHIFT2D2_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void LumaInterpolationFilterPoseOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPosfOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPosgOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPoshInRawOutRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    const EB_S16  vericalIFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = BI_OFFSET2D2_10BIT;
    EB_U8         IFShift = BI_SHIFT2D2_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void LumaInterpolationFilterPosiOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPosjOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPoskOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPosnInRawOutRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = BI_OFFSET2D2_10BIT;
    EB_U8         IFShift = BI_SHIFT2D2_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += 2 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void LumaInterpolationFilterPospOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPosqOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}
void LumaInterpolationFilterPosrOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRawOutRaw16bit(firstPassIFDst, dst, puWidth, puHeight);
}

void ChromaInterpolationCopy(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;
    PictureCopyKernel(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationCopy16bit(
    EB_U16*               refPic,
    EB_U32                srcStride,
    EB_U16*               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32   verticalIdx = puHeight;
    (void)firstPassIFDst;
    do
    {
        EB_MEMCPY(dst, refPic, puWidth*sizeof(EB_U16));    // EB_U8 to be modified
        refPic += srcStride;
        dst += dstStride;
    } while (--verticalIdx);
    //(void)firstPassIFDst;
    //PictureCopyKernel16bit(refPic, srcStride/**sizeof(EB_U16)*/, dst, dstStride*sizeof(EB_U16), puWidth*sizeof(EB_U16), puHeight);
}

void uniPredCopy16bit(
    EB_U16*               refPic,
    EB_U32                srcStride,
    EB_U16*               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                *firstPassIFDst)
{
    EB_U32   verticalIdx = puHeight;
    (void)firstPassIFDst;
    do
    {
        EB_MEMCPY(dst, refPic, puWidth*sizeof(EB_U16));    // EB_U8 to be modified
        refPic += srcStride;
        dst += dstStride;
    } while (--verticalIdx);

    return;
}

void ChromaInterpolationCopy16bit(
    EB_U16*               refPic,
    EB_U32                srcStride,
    EB_U16*               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    EB_U32   verticalIdx = puHeight;
    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;

    do
    {
        EB_MEMCPY(dst, refPic, puWidth*sizeof(EB_U16));    // EB_U8 to be modified
        refPic += srcStride;
        dst += dstStride;
    } while (--verticalIdx);

    return;
}

/** biPredCopy()
fetches the data from the reference picture array, then
(X + offset6)<<shift6 as the input to the weighted sample prediction.
*/
void LumaInterpolationCopyOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    EB_U32   dstStride = puWidth;
    EB_U32   horizontalIdx;
    EB_U32   verticalIdx = puHeight;
    EB_BYTE  tempRefPic = refPic - 1 + (puHeight - 1)*srcStride;
    EB_S16  *tempDst = dst - 1 + (puHeight - 1)*dstStride;

    (void)firstPassIFDst;

    do
    {
        horizontalIdx = puWidth;
        do
        {
            *(tempDst + horizontalIdx) = (EB_S16)((*(tempRefPic + horizontalIdx)) << Shift6) - MinusOffset6;
        } while (--horizontalIdx);
        tempRefPic -= srcStride;
        tempDst -= dstStride;
    } while (--verticalIdx);
}

void ChromaInterpolationCopyOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    EB_U32   dstStride = puWidth;
    EB_U32   horizontalIdx;
    EB_U32   verticalIdx = puHeight;
    EB_BYTE  tempRefPic = refPic - 1 + (puHeight - 1)*srcStride;
    EB_S16  *tempDst = dst - 1 + (puHeight - 1)*dstStride;

    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;

    do
    {
        horizontalIdx = puWidth;
        do
        {
            *(tempDst + horizontalIdx) = (EB_S16)((*(tempRefPic + horizontalIdx)) << Shift6) - ChromaMinusOffset6;
        } while (--horizontalIdx);
        tempRefPic -= srcStride;
        tempDst -= dstStride;
    } while (--verticalIdx);
}

void LumaInterpolationFilterPosaOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };             // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified
    EB_S32        IFOffset = -1 * MinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}

void LumaInterpolationFilterPosaNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };             // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified
    EB_S32        IFOffset = OFFSET1D_10BIT;//Offset3;
    EB_U8         IFShift = SHIFT1D_10BIT;//Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void LumaInterpolationFilterPosbNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified

    EB_S32        IFOffset = OFFSET1D_10BIT;
    EB_U8         IFShift = SHIFT1D_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16*       tempRefPic;
    EB_U16*       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void LumaInterpolationFilterPoscNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                     // to be modified
    EB_S32        IFOffset = OFFSET1D_10BIT;
    EB_U8         IFShift = SHIFT1D_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16*       tempRefPic;
    EB_U16*       tempDst;


    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void LumaInterpolationFilterPosdNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = OFFSET1D_10BIT;
    EB_U8         IFShift = SHIFT1D_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16*       tempRefPic;
    EB_U16*       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void LumaInterpolationFilterPosaOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };             // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                      // to be modified
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16 *       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif

    return;
}
void LumaInterpolationFilterPosdInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { -1, 4, -10, 58, 17, -5, 1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = OFFSET2D2_10BIT;
    EB_U8         IFShift = SHIFT2D2_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void LumaInterpolationFilterPoseNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosbOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                     // to be modified
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif
}
void LumaInterpolationFilterPosfNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPoscOutRaw16bit(
    EB_U16                *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
#if USE_PRE_COMPUTE

#else
    EB_U32        dstStride = puWidth;
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = 1;
    const EB_S32  IFInitPosOffset = -3;                                     // to be modified
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + 2 * IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + 3 * IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + 4 * IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + 5 * IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + 6 * IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + 7 * IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
#endif
}

void LumaInterpolationFilterPosgNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}


void LumaInterpolationFilterPoshNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    const EB_S16  IFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                            // to be modified
    EB_S32        IFOffset = OFFSET1D_10BIT;
    EB_U8         IFShift = SHIFT1D_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[6] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}

void LumaInterpolationFilterPoshInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[8] = { -1, 4, -11, 40, 40, -11, 4, -1 };        // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = OFFSET2D2_10BIT;
    EB_U8         IFShift = SHIFT2D2_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += 3 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[6] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[7];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPosiNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosjNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPoskNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 3 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosnNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    const EB_S16  IFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    const EB_S32  IFStride = srcStride;
    const EB_S32  IFInitPosOffset = -3 * srcStride;                           // to be modified
    EB_S32        IFOffset = OFFSET1D_10BIT;
    EB_U8         IFShift = SHIFT1D_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            // to be modified
            sum = *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 2)) * IFCoeff[3] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + IFStride) * IFCoeff[4] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * IFCoeff[5] +
                *(tempRefPic + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * IFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);

    return;
}
void LumaInterpolationFilterPosnInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
    EB_U32                firstPassIFDstStride = puWidth;
    const EB_S16  vericalIFCoeff[7] = { 1, -5, 17, 58, -10, 4, -1 };            // to be modified
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -3 * firstPassIFDstStride;                // to be modified
    EB_S32        IFOffset = OFFSET2D2_10BIT;
    EB_U8         IFShift = SHIFT2D2_10BIT;

    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += 2 * firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2)) * vericalIFCoeff[3] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + IFStride) * vericalIFCoeff[4] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1)) * vericalIFCoeff[5] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 2) + (IFStride << 1) + IFStride) * vericalIFCoeff[6];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void LumaInterpolationFilterPospNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosqNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}
void LumaInterpolationFilterPosrNew16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit(refPic - 2 * srcStride, srcStride, firstPassIFDst, puWidth, puHeight + 6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosnInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void ChromaInterpolationFilterOneD16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);
    EB_S32        IFOffset = OFFSET1D_10BIT;
    EB_U8         IFShift = SHIFT1D_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_U16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void ChromaInterpolationFilterTwoDInRaw16bit(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy)
{
    EB_S32        firstPassIFDstStride = puWidth;
    //vertical filtering parameter setting
    const EB_S16 *vericalIFCoeff = *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -1 * ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    EB_S32        IFOffset = OFFSET2D2_10BIT;
    EB_U8         IFShift = SHIFT2D2_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_U16       *tempDst;

    firstPassIFDst += ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void ChromaInterpolationFilterOneDOutRaw16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    EB_U32        dstStride = puWidth;
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);
    EB_S32        IFOffset = OFFSET2D1_10BIT;
    EB_U8         IFShift = SHIFT2D1_10BIT;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_U16       *tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}
void ChromaInterpolationFilterTwoD16bit(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw16bit(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRaw16bit(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy);
}


/** OneDFilterChoram() handles the 1D interpolation filtering for chroma.
Please note that EITHER U OR V componets is processed in this function
*/
void ChromaInterpolationFilterOneD(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);
    EB_S32        IFOffset = Offset3;
    EB_U8         IFShift = Shift3;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_BYTE       tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void ChromaInterpolationFilterOneDOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    EB_U32        dstStride = puWidth;
    const EB_S16 *IFCoeff = fracPosx ? *(chromaIFCoeff + fracPosx - 1) : *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = fracPosx ? 1 : srcStride;
    EB_S32        IFInitPosOffset = fracPosx ? (-1 * ((MaxChromaFilterTag - 1) >> 1)) : (-1 * ((MaxChromaFilterTag - 1) >> 1)*(EB_S32)srcStride);
    EB_S32        IFOffset = -1 * ChromaMinusOffset1;
    EB_U8         IFShift = Shift1;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_BYTE       tempRefPic;
    EB_S16       *tempDst;

    (void)firstPassIFDst;

    tempRefPic = refPic + puHeight*srcStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempRefPic -= srcStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempRefPic + horizontalIdx) * IFCoeff[0] +
                *(tempRefPic + horizontalIdx + IFStride) * IFCoeff[1] +
                *(tempRefPic + horizontalIdx + (IFStride << 1)) * IFCoeff[2] +
                *(tempRefPic + horizontalIdx + (IFStride << 1) + IFStride) * IFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void ChromaInterpolationFilterTwoDInRaw(
    EB_S16               *firstPassIFDst,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy)
{
    EB_S32        firstPassIFDstStride = puWidth;
    //vertical filtering parameter setting
    const EB_S16 *vericalIFCoeff = *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -1 * ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    EB_S32        IFOffset = ChromaOffset4;
    EB_U8         IFShift = Shift4;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_BYTE       tempDst;

    firstPassIFDst += ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_U8)CLIP3(0, MAX_Sample_Value, (sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

/** TwoDFilterChroma()
handles the 2D interpolation filtering for chroma.
Please note that EITHER U OR V componets is processed in this function.
*/
void ChromaInterpolationFilterTwoD(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRaw(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy);
}

void ChromaInterpolationFilterTwoDInRawOutRaw(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy)
{
    EB_S32        firstPassIFDstStride = puWidth;
    EB_U32        dstStride = puWidth;
    //vertical filtering parameter setting
    const EB_S16 *vericalIFCoeff = *(chromaIFCoeff + fracPosy - 1);
    EB_S32        IFStride = firstPassIFDstStride;
    EB_S32        IFInitPosOffset = -1 * ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    EB_S32        IFOffset = 0;
    EB_U8         IFShift = Shift2;
    EB_U32        verticalIdx = puHeight;
    EB_U32        horizontalIdx;
    EB_S32        sum;
    EB_S16       *tempFirstPassIFDst;
    EB_S16       *tempDst;

    firstPassIFDst += ((MaxChromaFilterTag - 1) >> 1)*firstPassIFDstStride;
    tempFirstPassIFDst = firstPassIFDst + puHeight*firstPassIFDstStride + IFInitPosOffset - 1;
    tempDst = dst + puHeight*dstStride - 1;
    do
    {
        horizontalIdx = puWidth;
        tempFirstPassIFDst -= firstPassIFDstStride;
        tempDst -= dstStride;
        do
        {
            sum = *(tempFirstPassIFDst + horizontalIdx) * vericalIFCoeff[0] +
                *(tempFirstPassIFDst + horizontalIdx + IFStride) * vericalIFCoeff[1] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1)) * vericalIFCoeff[2] +
                *(tempFirstPassIFDst + horizontalIdx + (IFStride << 1) + IFStride) * vericalIFCoeff[3];
            *(tempDst + horizontalIdx) = (EB_S16)((sum + IFOffset) >> IFShift);
        } while (--horizontalIdx);
    } while (--verticalIdx);
}

void ChromaInterpolationFilterTwoDOutRaw(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRawOutRaw(firstPassIFDst, dst, puWidth, puHeight, fracPosy);
}
