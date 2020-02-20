/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPackUnPack_h
#define EbPackUnPack_h
#ifdef __cplusplus
extern "C" {
#endif

#include "EbPackUnPack_C.h"
#include "EbPackUnPack_SSE2.h"
#include "EbPackUnPack_Intrinsic_AVX2.h"
#include "EbPictureOperators.h"

typedef void(*EB_ENC_Pack2D_TYPE)(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U16    *out16BitBuffer,
    EB_U32     innStride,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height);

EB_ENC_Pack2D_TYPE Pack2D_funcPtrArray_16Bit_SRC[2][EB_ASM_TYPE_TOTAL] =
{
    {
        // C_DEFAULT
        EB_ENC_msbPack2D,
        // AVX2
        EB_ENC_msbPack2D,
    },
    {
        // C_DEFAULT
        EB_ENC_msbPack2D,
        // AVX2
        EB_ENC_msbPack2D_AVX2_INTRIN_AL,//EB_ENC_msbPack2D_AVX2
    }
};


EB_ENC_Pack2D_TYPE CompressedPack_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
    // C_DEFAULT
    CompressedPackmsb,
    // AVX2
    CompressedPackmsb_AVX2_INTRIN,
};

typedef void(*COMPPack_TYPE)(
    const EB_U8     *innBitBuffer,
    EB_U32     innStride,
    EB_U8     *inCompnBitBuffer,
    EB_U32     outStride,
    EB_U8    *localCache,
    EB_U32     width,
    EB_U32     height);

COMPPack_TYPE  Convert_Unpack_CPack_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
    // C_DEFAULT
    CPack_C,
    // AVX2
    CPack_AVX2_INTRIN,

};

typedef void(*EB_ENC_UnPack2D_TYPE)(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U8       *outnBitBuffer,
    EB_U32       out8Stride,
    EB_U32       outnStride,
    EB_U32       width,
    EB_U32       height);

EB_ENC_UnPack2D_TYPE UnPack2D_funcPtrArray_16Bit[2][EB_ASM_TYPE_TOTAL] =
{
    {
        // C_DEFAULT
        EB_ENC_msbUnPack2D,
        // AVX2
        EB_ENC_msbUnPack2D,
    },
    {
        // C_DEFAULT
        EB_ENC_msbUnPack2D,
        // AVX512
#ifndef NON_AVX512_SUPPORT
        EB_ENC_msbUnPack2D_AVX512_INTRIN,
#else
        EB_ENC_msbUnPack2D_AVX2_INTRIN,
#endif

    }
};

typedef void(*EB_ENC_UnpackAvg_TYPE)(
    EB_U16 *ref16L0,
    EB_U32  refL0Stride,
    EB_U16 *ref16L1,
    EB_U32  refL1Stride,
    EB_U8  *dstPtr,
    EB_U32  dstStride,
    EB_U32  width,
    EB_U32  height);
EB_ENC_UnpackAvg_TYPE UnPackAvg_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
    // C_DEFAULT
    UnpackAvg,
    // AVX2
    UnpackAvg_AVX2_INTRIN,//UnpackAvg_SSE2_INTRIN,

};
typedef void(*EB_ENC_UnpackAvgSub_TYPE)(
    EB_U16 *ref16L0,
    EB_U32  refL0Stride,
    EB_U16 *ref16L1,
    EB_U32  refL1Stride,
    EB_U8  *dstPtr,
    EB_U32  dstStride,
    EB_U32  width,
    EB_U32  height);
EB_ENC_UnpackAvgSub_TYPE UnPackAvgSafeSub_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
    // C_DEFAULT
    UnpackAvgSafeSub,
    // AVX2  SafeSub
    UnpackAvgSafeSub_AVX2_INTRIN

};

typedef void(*EB_ENC_UnPack8BitData_TYPE)(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height);
EB_ENC_UnPack8BitData_TYPE UnPack8BIT_funcPtrArray_16Bit[2][EB_ASM_TYPE_TOTAL] =
{
    {
       UnPack8BitData,
       UnPack8BitData,
    },
    {
        // C_DEFAULT
        UnPack8BitData,
        // AVX2
        EB_ENC_UnPack8BitData_SSE2_INTRIN,
    }
};
typedef void(*EB_ENC_UnPack8BitDataSUB_TYPE)(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height
    );
EB_ENC_UnPack8BitDataSUB_TYPE UnPack8BITSafeSub_funcPtrArray_16Bit[EB_ASM_TYPE_TOTAL] =
{
    // C_DEFAULT
    UnPack8BitDataSafeSub,
    // SSE2
    EB_ENC_UnPack8BitDataSafeSub_SSE2_INTRIN
};

#ifdef __cplusplus
}
#endif
#endif // EbPackUnPack_h
