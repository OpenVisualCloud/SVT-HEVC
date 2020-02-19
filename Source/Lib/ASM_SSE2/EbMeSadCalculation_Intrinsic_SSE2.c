/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbMeSadCalculation_SSE2.h"
#include <emmintrin.h>
#include "EbDefinitions.h"

void SadCalculation_8x8_16x16_SSE2_INTRIN(
    EB_U8   *src,
    EB_U32   srcStride,
    EB_U8   *ref,
    EB_U32   refStride,
    EB_U32  *pBestSad8x8,
    EB_U32  *pBestSad16x16,
    EB_U32  *pBestMV8x8,
    EB_U32  *pBestMV16x16,
    EB_U32   mv,
    EB_U32  *pSad16x16)
{
    __m128i xmm_sad16x16, xmm_sad8x8_0_1, xmm_sad8x8_2_3, xmm_sad16x16_total, sad8x8_0_3, sad8x8_less_than_bitmask, xmm_N1;
    __m128i sad8x8_greater_or_eq_bitmask, BestMV8x8, BestSad8x8, xmm_pBestSad8x8, xmm_pBestMV8x8, xmm_mv;

    srcStride <<= 1;
    refStride <<= 1;

    //sad8x8_0, sad8x8_1

    xmm_sad8x8_0_1 = _mm_add_epi32(_mm_sad_epu8(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)ref)),
                                    _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride)), _mm_loadu_si128((__m128i*)(ref+refStride))));
    xmm_sad8x8_0_1 = _mm_add_epi32(_mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (2*srcStride))), _mm_loadu_si128((__m128i*)(ref + (2*refStride)))), xmm_sad8x8_0_1);
    xmm_sad8x8_0_1 = _mm_add_epi32(_mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (3 * srcStride))), _mm_loadu_si128((__m128i*)(ref + (3 * refStride)))), xmm_sad8x8_0_1);

    src += srcStride<<2;
    ref += refStride<<2;

    //sad8x8_2, sad8x8_3

    xmm_sad8x8_2_3 = _mm_add_epi32(_mm_sad_epu8(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)ref)),
                                    _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_loadu_si128((__m128i*)(ref + refStride))));
    xmm_sad8x8_2_3 = _mm_add_epi32(_mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (2 * srcStride))), _mm_loadu_si128((__m128i*)(ref + (2 * refStride)))), xmm_sad8x8_2_3);
    xmm_sad8x8_2_3 = _mm_add_epi32(_mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (3 * srcStride))), _mm_loadu_si128((__m128i*)(ref + (3 * refStride)))), xmm_sad8x8_2_3);

    xmm_sad16x16 = _mm_add_epi32(xmm_sad8x8_0_1, xmm_sad8x8_2_3);

    xmm_sad16x16_total = _mm_slli_epi32(_mm_add_epi32(_mm_srli_si128(xmm_sad16x16, 8), xmm_sad16x16), 1);

    *pSad16x16 = _mm_cvtsi128_si32(xmm_sad16x16_total);

    sad8x8_0_3 = _mm_slli_epi32(_mm_packs_epi32(xmm_sad8x8_0_1, xmm_sad8x8_2_3), 1);

    xmm_mv = _mm_cvtsi64_si128(mv);
    xmm_mv = _mm_unpacklo_epi32(xmm_mv, xmm_mv);
    xmm_mv = _mm_unpacklo_epi64(xmm_mv, xmm_mv);

    xmm_pBestSad8x8 = _mm_loadu_si128((__m128i*)pBestSad8x8);
    xmm_pBestMV8x8 = _mm_loadu_si128((__m128i*)pBestMV8x8);

    // sad8x8_0 < pBestSad8x8[0] for 0 to 3
    sad8x8_less_than_bitmask = _mm_cmplt_epi32(sad8x8_0_3, xmm_pBestSad8x8);

    xmm_N1 = _mm_cmpeq_epi8(xmm_sad8x8_0_1, xmm_sad8x8_0_1);

    sad8x8_greater_or_eq_bitmask = _mm_sub_epi32(xmm_N1, sad8x8_less_than_bitmask);

    BestSad8x8 = _mm_or_si128(_mm_and_si128(xmm_pBestSad8x8, sad8x8_greater_or_eq_bitmask), _mm_and_si128(sad8x8_less_than_bitmask, sad8x8_0_3));
    BestMV8x8 = _mm_or_si128(_mm_and_si128(xmm_pBestMV8x8, sad8x8_greater_or_eq_bitmask), _mm_and_si128(sad8x8_less_than_bitmask, xmm_mv));

    _mm_storeu_si128((__m128i*)pBestSad8x8, BestSad8x8);
    _mm_storeu_si128((__m128i*)pBestMV8x8, BestMV8x8);

    EB_U64 sad16x16 = _mm_cvtsi128_si64(xmm_sad16x16_total);
    if (sad16x16 < pBestSad16x16[0]){
        pBestSad16x16[0] = (EB_U32)sad16x16;
        pBestMV16x16[0] = _mm_cvtsi128_si32(xmm_mv);
    }
}

 void SadCalculation_32x32_64x64_SSE2_INTRIN(
    EB_U32  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv)
 {
     __m128i xmm_N1, sad32x32_greater_than_bitmask, sad32x32_less_than_or_eq_bitmask, BestSad32x32, BestMV32x32, xmm_mv;
     __m128i Sad16x16_0_7_lo, Sad16x16_0_7_hi, Sad16x16_8_15_lo, Sad16x16_8_15_hi, xmm_sad64x64, xmm_sad64x64_total, xmm_pBestSad32x32, xmm_pBestMV32x32;

     Sad16x16_0_7_lo = _mm_unpacklo_epi32(_mm_loadu_si128((__m128i*)pSad16x16), _mm_loadu_si128((__m128i*)(pSad16x16 + 4)));
     Sad16x16_0_7_hi = _mm_unpackhi_epi32(_mm_loadu_si128((__m128i*)pSad16x16), _mm_loadu_si128((__m128i*)(pSad16x16 + 4)));
     Sad16x16_8_15_lo = _mm_unpacklo_epi32(_mm_loadu_si128((__m128i*)(pSad16x16 + 8)), _mm_loadu_si128((__m128i*)(pSad16x16 + 12)));
     Sad16x16_8_15_hi = _mm_unpackhi_epi32(_mm_loadu_si128((__m128i*)(pSad16x16 + 8)), _mm_loadu_si128((__m128i*)(pSad16x16 + 12)));

     xmm_sad64x64 = _mm_add_epi32(_mm_add_epi32(_mm_unpacklo_epi64(Sad16x16_0_7_lo, Sad16x16_8_15_lo), _mm_unpackhi_epi64(Sad16x16_0_7_lo, Sad16x16_8_15_lo)),
                                  _mm_add_epi32(_mm_unpacklo_epi64(Sad16x16_0_7_hi, Sad16x16_8_15_hi), _mm_unpackhi_epi64(Sad16x16_0_7_hi, Sad16x16_8_15_hi)));

     xmm_sad64x64_total = _mm_add_epi32(_mm_srli_si128(xmm_sad64x64, 8), xmm_sad64x64);

     xmm_sad64x64_total = _mm_add_epi32(_mm_srli_si128(xmm_sad64x64_total, 4), xmm_sad64x64_total);

     xmm_mv = _mm_cvtsi32_si128(mv);
     xmm_mv = _mm_unpacklo_epi32(xmm_mv, xmm_mv);
     xmm_mv = _mm_unpacklo_epi64(xmm_mv, xmm_mv);

     xmm_pBestSad32x32 = _mm_loadu_si128((__m128i*)pBestSad32x32);
     xmm_pBestMV32x32 = _mm_loadu_si128((__m128i*)pBestMV32x32);

     sad32x32_greater_than_bitmask = _mm_cmpgt_epi32(xmm_pBestSad32x32, xmm_sad64x64);// _mm_cmplt_epi32(xmm_pBestSad32x32, xmm_sad64x64);

     xmm_N1 = _mm_cmpeq_epi8(xmm_mv, xmm_mv); // anything compared to itself is equal (get 0xFFFFFFFF)
     sad32x32_less_than_or_eq_bitmask = _mm_sub_epi32(xmm_N1, sad32x32_greater_than_bitmask);

     BestSad32x32 = _mm_or_si128(_mm_and_si128(xmm_pBestSad32x32, sad32x32_less_than_or_eq_bitmask), _mm_and_si128(xmm_sad64x64, sad32x32_greater_than_bitmask));
     BestMV32x32 = _mm_or_si128(_mm_and_si128(xmm_pBestMV32x32, sad32x32_less_than_or_eq_bitmask), _mm_and_si128(xmm_mv, sad32x32_greater_than_bitmask));

     _mm_storeu_si128((__m128i*)pBestSad32x32, BestSad32x32);
     _mm_storeu_si128((__m128i*)pBestMV32x32, BestMV32x32);


     EB_U32 sad64x64 = _mm_cvtsi128_si32(xmm_sad64x64_total);
     if (sad64x64 < pBestSad64x64[0]){
         pBestSad64x64[0] = sad64x64;
         pBestMV64x64[0] = _mm_cvtsi128_si32(xmm_mv);
     }
}


void InitializeBuffer_32bits_SSE2_INTRIN(
    EB_U32*        Pointer,
    EB_U32        Count128,
    EB_U32        Count32,
    EB_U32        Value)
{
    __m128i xmm1, xmm2;
    EB_U32 index128;
    xmm2 = _mm_cvtsi32_si128(Value);
    xmm1 = _mm_or_si128(_mm_slli_si128(xmm2, 4), xmm2);
    xmm2 = _mm_or_si128(_mm_slli_si128(xmm1, 8), xmm1);

    for (index128 = 0; index128 < Count128; ++index128){
        _mm_storeu_si128((__m128i *)Pointer, xmm2);
        Pointer += 4;
    }
    if (Count32 == 3) { //Initialize 96 bits
        _mm_storel_epi64((__m128i *)(Pointer), xmm2);
        *(Pointer + 2) = _mm_cvtsi128_si32(xmm2);
    }
    else if (Count32 == 2) { // Initialize 64 bits
        _mm_storel_epi64((__m128i *)Pointer, xmm2);
    }
    else if (Count32 == 1){ // Initialize 32 bits
        *(Pointer) = _mm_cvtsi128_si32(xmm2);
    }
}
