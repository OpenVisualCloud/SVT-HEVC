/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbTransforms_SSE2.h"
#include "EbIntrinMacros16bit_SSE2.h"
#include <emmintrin.h>

#define SHIFT_INV_2ND 12
#define OFFSET_2        0
#define OFFSET_4        (8+OFFSET_2)
#define OFFSET_8        (8+OFFSET_4)
#define OFFSET_9        (8+OFFSET_8)
#define OFFSET_64       (8+OFFSET_9)
#define OFFSET_256      (8+OFFSET_64)
#define OFFSET_512      (8+OFFSET_256)
#define OFFSET_1024     (8+OFFSET_512)
#define OFFSET_2048     (8+OFFSET_1024)
#define OFFSET_SHIFT_7  (8+OFFSET_2048)
#define OFFSET_SHIFT_12 (8+OFFSET_SHIFT_7)
#define OFFSET_64_64    (8+OFFSET_SHIFT_12)
#define OFFSET_90_57    (8+OFFSET_64_64)
#define OFFSET_89_50    (8+OFFSET_90_57)
#define OFFSET_87_43    (8+OFFSET_89_50)
#define OFFSET_83_36    (8+OFFSET_87_43)
#define OFFSET_80_25    (8+OFFSET_83_36)
#define OFFSET_75_18    (8+OFFSET_80_25)
#define OFFSET_70_9     (8+OFFSET_75_18)
#define OFFSET_64_N64   (8+OFFSET_70_9)
#define OFFSET_87_N80   (8+OFFSET_64_N64)
#define OFFSET_75_N89   (8+OFFSET_87_N80)
#define OFFSET_57_N90   (8+OFFSET_75_N89)
#define OFFSET_36_N83   (8+OFFSET_57_N90)
#define OFFSET_9_N70    (8+OFFSET_36_N83)
#define OFFSET_N18_N50  (8+OFFSET_9_N70)
#define OFFSET_N43_N25  (8+OFFSET_N18_N50)
#define OFFSET_80_N25   (8+OFFSET_N43_N25)
#define OFFSET_50_18    (8+OFFSET_80_N25)
#define OFFSET_9_57     (8+OFFSET_50_18)
#define OFFSET_N36_83   (8+OFFSET_9_57)
#define OFFSET_N70_90   (8+OFFSET_N36_83)
#define OFFSET_N89_75   (8+OFFSET_N70_90)
#define OFFSET_N87_43   (8+OFFSET_N89_75)
#define OFFSET_70_90    (8+OFFSET_N87_43)
#define OFFSET_18_75    (8+OFFSET_70_90)
#define OFFSET_N43_25   (8+OFFSET_18_75)
#define OFFSET_N83_N36  (8+OFFSET_N43_25)
#define OFFSET_N87_N80  (8+OFFSET_N83_N36)
#define OFFSET_N50_N89  (8+OFFSET_N87_N80)
#define OFFSET_9_N57    (8+OFFSET_N50_N89)
#define OFFSET_57_N9    (8+OFFSET_9_N57)
#define OFFSET_N18_N75  (8+OFFSET_57_N9)
#define OFFSET_N80_N87  (8+OFFSET_N18_N75)
#define OFFSET_N25_43   (8+OFFSET_N80_N87)
#define OFFSET_50_89    (8+OFFSET_N25_43)
#define OFFSET_90_70    (8+OFFSET_50_89)
#define OFFSET_43_N87   (8+OFFSET_90_70)
#define OFFSET_N50_N18  (8+OFFSET_43_N87)
#define OFFSET_N90_70   (8+OFFSET_N50_N18)
#define OFFSET_57_9     (8+OFFSET_N90_70)
#define OFFSET_89_N75   (8+OFFSET_57_9)
#define OFFSET_25_N80   (8+OFFSET_89_N75)
#define OFFSET_25_43    (8+OFFSET_25_N80)
#define OFFSET_N75_89   (8+OFFSET_25_43)
#define OFFSET_N70_9    (8+OFFSET_N75_89)
#define OFFSET_90_N57   (8+OFFSET_N70_9)
#define OFFSET_18_50    (8+OFFSET_90_N57)
#define OFFSET_N80_87   (8+OFFSET_18_50)
#define OFFSET_9_70     (8+OFFSET_N80_87)
#define OFFSET_N89_N50  (8+OFFSET_9_70)
#define OFFSET_N25_N80  (8+OFFSET_N89_N50)
#define OFFSET_43_87    (8+OFFSET_N25_N80)
#define OFFSET_N75_N18  (8+OFFSET_43_87)
#define OFFSET_N57_N90  (8+OFFSET_N75_N18)
#define OFFSET_N9_N70   (8+OFFSET_N57_N90)
#define OFFSET_25_80    (8+OFFSET_N9_N70)
#define OFFSET_N43_N87  (8+OFFSET_25_80)
#define OFFSET_57_90    (8+OFFSET_N43_N87)
#define OFFSET_N25_N43  (8+OFFSET_57_90)
#define OFFSET_70_N9    (8+OFFSET_N25_N43)
#define OFFSET_N90_57   (8+OFFSET_70_N9)
#define OFFSET_80_N87   (8+OFFSET_N90_57)
#define OFFSET_N43_87   (8+OFFSET_80_N87)
#define OFFSET_90_N70   (8+OFFSET_N43_87)
#define OFFSET_N57_N9   (8+OFFSET_90_N70)
#define OFFSET_N25_80   (8+OFFSET_N57_N9)
#define OFFSET_N57_9    (8+OFFSET_N25_80)
#define OFFSET_80_87    (8+OFFSET_N57_9)
#define OFFSET_25_N43   (8+OFFSET_80_87)
#define OFFSET_N90_N70  (8+OFFSET_25_N43)
#define OFFSET_N70_N90  (8+OFFSET_N90_N70)
#define OFFSET_43_N25   (8+OFFSET_N70_N90)
#define OFFSET_87_80    (8+OFFSET_43_N25)
#define OFFSET_N9_57    (8+OFFSET_87_80)
#define OFFSET_N80_25   (8+OFFSET_N9_57)
#define OFFSET_N9_N57   (8+OFFSET_N80_25)
#define OFFSET_70_N90   (8+OFFSET_N9_N57)
#define OFFSET_87_N43   (8+OFFSET_70_N90)
#define OFFSET_N87_80   (8+OFFSET_87_N43)
#define OFFSET_N57_90   (8+OFFSET_N87_80)
#define OFFSET_N9_70    (8+OFFSET_N57_90)
#define OFFSET_43_25    (8+OFFSET_N9_70)
#define OFFSET_N90_N57  (8+OFFSET_43_25)
#define OFFSET_N87_N43  (8+OFFSET_N90_N57)
#define OFFSET_N80_N25  (8+OFFSET_N87_N43)
#define OFFSET_N70_N9   (8+OFFSET_N80_N25)
#define OFFSET_90_61    (8+OFFSET_N70_N9)
#define OFFSET_90_54    (8+OFFSET_90_61)
#define OFFSET_88_46    (8+OFFSET_90_54)
#define OFFSET_85_38    (8+OFFSET_88_46)
#define OFFSET_82_31    (8+OFFSET_85_38)
#define OFFSET_78_22    (8+OFFSET_82_31)
#define OFFSET_73_13    (8+OFFSET_78_22)
#define OFFSET_67_4     (8+OFFSET_73_13)
#define OFFSET_90_N73   (8+OFFSET_67_4)
#define OFFSET_82_N85   (8+OFFSET_90_N73)
#define OFFSET_67_N90   (8+OFFSET_82_N85)
#define OFFSET_46_N88   (8+OFFSET_67_N90)
#define OFFSET_22_N78   (8+OFFSET_46_N88)
#define OFFSET_N4_N61   (8+OFFSET_22_N78)
#define OFFSET_N31_N38  (8+OFFSET_N4_N61)
#define OFFSET_N54_N13  (8+OFFSET_N31_N38)
#define OFFSET_88_N46   (8+OFFSET_N54_N13)
#define OFFSET_67_N4    (8+OFFSET_88_N46)
#define OFFSET_31_38    (8+OFFSET_67_N4)
#define OFFSET_N13_73   (8+OFFSET_31_38)
#define OFFSET_N54_90   (8+OFFSET_N13_73)
#define OFFSET_N82_85   (8+OFFSET_N54_90)
#define OFFSET_N90_61   (8+OFFSET_N82_85)
#define OFFSET_N78_22   (8+OFFSET_N90_61)
#define OFFSET_85_82    (8+OFFSET_N78_22)
#define OFFSET_46_88    (8+OFFSET_85_82)
#define OFFSET_N13_54   (8+OFFSET_46_88)
#define OFFSET_N67_N4   (8+OFFSET_N13_54)
#define OFFSET_N90_N61  (8+OFFSET_N67_N4)
#define OFFSET_N73_N90  (8+OFFSET_N90_N61)
#define OFFSET_N22_N78  (8+OFFSET_N73_N90)
#define OFFSET_38_N31   (8+OFFSET_N22_N78)
#define OFFSET_22_N46   (8+OFFSET_38_N31)
#define OFFSET_N54_N90  (8+OFFSET_22_N46)
#define OFFSET_N90_N67  (8+OFFSET_N54_N90)
#define OFFSET_N61_4    (8+OFFSET_N90_N67)
#define OFFSET_13_73    (8+OFFSET_N61_4)
#define OFFSET_78_88    (8+OFFSET_13_73)
#define OFFSET_78_N88   (8+OFFSET_78_88)
#define OFFSET_N82_31   (8+OFFSET_78_N88)
#define OFFSET_N73_90   (8+OFFSET_N82_31)
#define OFFSET_13_54    (8+OFFSET_N73_90)
#define OFFSET_85_N38   (8+OFFSET_13_54)
#define OFFSET_N22_N46  (8+OFFSET_85_N38)
#define OFFSET_73_N13   (8+OFFSET_N22_N46)
#define OFFSET_N31_82   (8+OFFSET_73_N13)
#define OFFSET_N38_85   (8+OFFSET_N31_82)
#define OFFSET_N90_54   (8+OFFSET_N38_85)
#define OFFSET_67_90    (8+OFFSET_N90_54)
#define OFFSET_N54_13   (8+OFFSET_67_90)
#define OFFSET_N78_N88  (8+OFFSET_N54_13)
#define OFFSET_N22_46   (8+OFFSET_N78_N88)
#define OFFSET_N90_N73  (8+OFFSET_N22_46)
#define OFFSET_4_N61    (8+OFFSET_N90_N73)
#define OFFSET_61_N4    (8+OFFSET_4_N61)
#define OFFSET_N46_22   (8+OFFSET_61_N4)
#define OFFSET_82_85    (8+OFFSET_N46_22)
#define OFFSET_31_N38   (8+OFFSET_82_85)
#define OFFSET_N88_N78  (8+OFFSET_31_N38)
#define OFFSET_90_67    (8+OFFSET_N88_N78)
#define OFFSET_54_N90   (8+OFFSET_90_67)
#define OFFSET_N85_38   (8+OFFSET_54_N90)
#define OFFSET_N4_67    (8+OFFSET_N85_38)
#define OFFSET_88_N78   (8+OFFSET_N4_67)
#define OFFSET_N46_N22  (8+OFFSET_88_N78)
#define OFFSET_N61_90   (8+OFFSET_N46_N22)
#define OFFSET_82_N31   (8+OFFSET_N61_90)
#define OFFSET_13_N73   (8+OFFSET_82_N31)
#define OFFSET_46_22    (8+OFFSET_13_N73)
#define OFFSET_N90_67   (8+OFFSET_46_22)
#define OFFSET_38_N85   (8+OFFSET_N90_67)
#define OFFSET_54_13    (8+OFFSET_38_N85)
#define OFFSET_N90_73   (8+OFFSET_54_13)
#define OFFSET_31_N82   (8+OFFSET_N90_73)
#define OFFSET_61_4     (8+OFFSET_31_N82)
#define OFFSET_N88_78   (8+OFFSET_61_4)
#define OFFSET_38_85    (8+OFFSET_N88_78)
#define OFFSET_N4_61    (8+OFFSET_38_85)
#define OFFSET_N67_N90  (8+OFFSET_N4_61)
#define OFFSET_N31_N82  (8+OFFSET_N67_N90)
#define OFFSET_N78_N22  (8+OFFSET_N31_N82)
#define OFFSET_90_73    (8+OFFSET_N78_N22)
#define OFFSET_N61_N90  (8+OFFSET_90_73)
#define OFFSET_4_67     (8+OFFSET_N61_N90)
#define OFFSET_54_N13   (8+OFFSET_4_67)
#define OFFSET_N88_N46  (8+OFFSET_54_N13)
#define OFFSET_85_N82   (8+OFFSET_N88_N46)
#define OFFSET_N38_N31  (8+OFFSET_85_N82)
#define OFFSET_N13_N73  (8+OFFSET_N38_N31)
#define OFFSET_22_78    (8+OFFSET_N13_N73)
#define OFFSET_N46_N88  (8+OFFSET_22_78)
#define OFFSET_54_90    (8+OFFSET_N46_N88)

void InvTransform4x4_SSE2_INTRIN(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define MACRO_4x4(SHIFT_INSTR, SHIFT_ARG)\
    xmm_even1 = _mm_madd_epi16(xmm_trans_1_3, _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2+OFFSET_64_N64)));\
    xmm_odd1  = _mm_madd_epi16(xmm_trans_2_4, _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2+OFFSET_36_N83)));\
    xmm_even0 = _mm_madd_epi16(xmm_trans_1_3, _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2+OFFSET_64_64)));\
    xmm_odd0  = _mm_madd_epi16(xmm_trans_2_4, _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2+OFFSET_83_36)));\
    xmm_even0 = _mm_add_epi32(xmm_even0, xmm_offset);\
    xmm_even1 = _mm_add_epi32(xmm_even1, xmm_offset);\
    xmm_res1 = _mm_##SHIFT_INSTR(_mm_add_epi32(xmm_even0, xmm_odd0), SHIFT_ARG);\
    xmm_res2 = _mm_##SHIFT_INSTR(_mm_add_epi32(xmm_even1, xmm_odd1), SHIFT_ARG);\
    xmm_res3 = _mm_##SHIFT_INSTR(_mm_sub_epi32(xmm_even1, xmm_odd1), SHIFT_ARG);\
    xmm_res4 = _mm_##SHIFT_INSTR(_mm_sub_epi32(xmm_even0, xmm_odd0), SHIFT_ARG);\
    xmm_res_1_3 = _mm_packs_epi32(xmm_res1, xmm_res3);\
    xmm_res_2_4 = _mm_packs_epi32(xmm_res2, xmm_res4);

    __m128i xmm0, xmm2, xmm5, xmm_shift, xmm_offset;
    __m128i xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res_1_3, xmm_res_2_4, xmm_even0, xmm_even1, xmm_odd0, xmm_odd1;
    __m128i xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans4, xmm_trans_1_3, xmm_trans_2_4;
    __m128i xmm_res_3_4, xmm_res_1_2, xmm_res_1_2_3_4_lo, xmm_res_1_2_3_4_hi;

    xmm_trans1 = _mm_loadl_epi64((__m128i *)(transformCoefficients));
    xmm_trans2 = _mm_loadl_epi64((__m128i *)(transformCoefficients + srcStride));
    xmm_trans3 = _mm_loadl_epi64((__m128i *)(transformCoefficients + srcStride * 2));
    xmm_trans4 = _mm_loadl_epi64((__m128i *)(transformCoefficients + srcStride * 3));

    xmm_shift = _mm_cvtsi32_si128(SHIFT_INV_2ND - bitIncrement);

    xmm_trans_1_3 = _mm_unpacklo_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_2_4 = _mm_unpacklo_epi16(xmm_trans2, xmm_trans4);

    xmm_offset = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_64));
    MACRO_4x4(srai_epi32, 7)

    xmm5 = _mm_unpackhi_epi16(xmm_res_1_3, xmm_res_2_4);
    xmm0 = _mm_unpacklo_epi16(xmm_res_1_3, xmm_res_2_4);
    xmm2 = _mm_unpackhi_epi32(xmm0, xmm5);
    xmm0 = _mm_unpacklo_epi32(xmm0, xmm5);

    xmm_trans_1_3 = _mm_unpacklo_epi16(xmm0, xmm2);
    xmm_trans_2_4 = _mm_unpackhi_epi16(xmm0, xmm2);

    xmm_offset = _mm_srli_epi32(_mm_set1_epi32(0x0800), bitIncrement);

    MACRO_4x4(sra_epi32, xmm_shift)

    xmm_res_3_4 = _mm_unpackhi_epi16(xmm_res_1_3, xmm_res_2_4);
    xmm_res_1_2 = _mm_unpacklo_epi16(xmm_res_1_3, xmm_res_2_4);
    xmm_res_1_2_3_4_hi = _mm_unpackhi_epi32(xmm_res_1_2, xmm_res_3_4);
    xmm_res_1_2_3_4_lo = _mm_unpacklo_epi32(xmm_res_1_2, xmm_res_3_4);

    _mm_storel_epi64((__m128i *)(residual), xmm_res_1_2_3_4_lo);
    xmm_res_1_2_3_4_lo = _mm_srli_si128(xmm_res_1_2_3_4_lo, 8);
    _mm_storel_epi64((__m128i *)(residual + dstStride), xmm_res_1_2_3_4_lo);
    _mm_storel_epi64((__m128i *)(residual + 2 * dstStride), xmm_res_1_2_3_4_hi);
    xmm_res_1_2_3_4_hi = _mm_srli_si128(xmm_res_1_2_3_4_hi, 8);
    _mm_storel_epi64((__m128i *)(residual + 3 * dstStride), xmm_res_1_2_3_4_hi);

    (void)transformInnerArrayPtr;
}


void InvTransform8x8_SSE2_INTRIN(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define MACRO_8X8_ODD(XMM_TRANS_1_3_LO, XMM_TRANS_1_3_HI, XMM_TRANS_2_4_LO, XMM_TRANS_2_4_HI, XMM_DCT1, XMM_DCT2, OFFSET)\
    xmm_odd_lo = _mm_add_epi32(_mm_madd_epi16(XMM_TRANS_1_3_LO, XMM_DCT1), _mm_madd_epi16(XMM_TRANS_2_4_LO, XMM_DCT2));\
    _mm_store_si128((__m128i *)(transformInnerArrayPtrAligned + OFFSET), xmm_odd_lo);\
    xmm_odd_hi = _mm_add_epi32(_mm_madd_epi16(XMM_TRANS_1_3_HI, XMM_DCT1), _mm_madd_epi16(XMM_TRANS_2_4_HI, XMM_DCT2));\
    _mm_store_si128((__m128i *)(transformInnerArrayPtrAligned + OFFSET + 8), xmm_odd_hi);

#define MACRO_8X8_FINAL(XMM_EVEN_LO, XMM_EVEN_HI, XMM_RES1, XMM_RES2, OFFSET, SHIFT_INSTR, SHIFT)\
    xmm_odd_lo = _mm_load_si128((__m128i *)(transformInnerArrayPtrAligned + OFFSET));\
    xmm_odd_hi = _mm_load_si128((__m128i *)(transformInnerArrayPtrAligned + OFFSET + 8));\
    XMM_RES1 = _mm_packs_epi32(_mm_##SHIFT_INSTR(_mm_add_epi32(XMM_EVEN_LO, xmm_odd_lo), SHIFT), _mm_##SHIFT_INSTR(_mm_add_epi32(XMM_EVEN_HI, xmm_odd_hi), SHIFT));\
    XMM_RES2 = _mm_packs_epi32(_mm_##SHIFT_INSTR(_mm_sub_epi32(XMM_EVEN_LO, xmm_odd_lo), SHIFT), _mm_##SHIFT_INSTR(_mm_sub_epi32(XMM_EVEN_HI, xmm_odd_hi), SHIFT));

    EB_ALIGN(16) EB_S16 * transformInnerArrayPtrAligned = transformInnerArrayPtr;

    __m128i xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans4, xmm_trans_1_3_hi, xmm_trans_1_3_lo, xmm_trans_2_4_hi, xmm_trans_2_4_lo;
    __m128i xmm_DCT1, xmm_DCT2, xmm_odd_hi, xmm_odd_lo, xmm_offset, xmm_shift;
    __m128i xmm_even0L, xmm_even0H, xmm_even1L, xmm_even1H, xmm_even2L, xmm_even2H, xmm_even3L, xmm_even3H, xmm_evenOdd0L, xmm_evenOdd0H, xmm_evenOdd1L, xmm_evenOdd1H;
    __m128i xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi;
    __m128i xmm_res02_hi, xmm_res13_hi, xmm_res46_hi, xmm_res57_hi, xmm_res04_hi, xmm_res26_hi, xmm_res15_hi, xmm_res37_hi;
    __m128i xmm_evenEven0L, xmm_evenEven0H, xmm_evenEven1L, xmm_evenEven1H;

    // Odd transform coefficients (1,3,5,7)
    transformCoefficients += srcStride;
    xmm_trans1 = _mm_loadu_si128((__m128i *)(transformCoefficients));
    xmm_trans2 = _mm_loadu_si128((__m128i *)(transformCoefficients + 2 * srcStride));
    xmm_trans3 = _mm_loadu_si128((__m128i *)(transformCoefficients + 4 * srcStride));
    xmm_trans4 = _mm_loadu_si128((__m128i *)(transformCoefficients + 6 * srcStride));

    xmm_trans_1_3_hi = _mm_unpackhi_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_1_3_lo = _mm_unpacklo_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_2_4_hi = _mm_unpackhi_epi16(xmm_trans2, xmm_trans4);
    xmm_trans_2_4_lo = _mm_unpacklo_epi16(xmm_trans2, xmm_trans4);

    //Odd0
    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_89_50));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_75_18));
    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0)

    //Odd1
    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_75_N89));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_N18_N50));
    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0x10)

    //Odd2
    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_50_18));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_N89_75));
    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0x20)

    //Odd3
    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_18_75));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_N50_N89));
    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0x30)

    // Even transform coefficients (0,2,4,6)
    transformCoefficients -= srcStride;
    xmm_trans1 = _mm_loadu_si128((__m128i *)(transformCoefficients));
    xmm_trans2 = _mm_loadu_si128((__m128i *)(transformCoefficients + 2 * srcStride));
    xmm_trans3 = _mm_loadu_si128((__m128i *)(transformCoefficients + 4 * srcStride));
    xmm_trans4 = _mm_loadu_si128((__m128i *)(transformCoefficients + 6 * srcStride));

    xmm_trans_1_3_hi = _mm_unpackhi_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_1_3_lo = _mm_unpacklo_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_2_4_hi = _mm_unpackhi_epi16(xmm_trans2, xmm_trans4);
    xmm_trans_2_4_lo = _mm_unpacklo_epi16(xmm_trans2, xmm_trans4);

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_64_64));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_64_N64));
    xmm_offset = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_64));

    xmm_evenEven0L = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_lo, xmm_DCT1), xmm_offset);
    xmm_evenEven0H = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_hi, xmm_DCT1), xmm_offset);

    xmm_evenEven1L = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_lo, xmm_DCT2), xmm_offset);
    xmm_evenEven1H = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_hi, xmm_DCT2), xmm_offset);

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_83_36));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_36_N83));

    xmm_evenOdd0L = _mm_madd_epi16(xmm_trans_2_4_lo, xmm_DCT1);
    xmm_evenOdd0H = _mm_madd_epi16(xmm_trans_2_4_hi, xmm_DCT1);
    xmm_evenOdd1L = _mm_madd_epi16(xmm_trans_2_4_lo, xmm_DCT2);
    xmm_evenOdd1H = _mm_madd_epi16(xmm_trans_2_4_hi, xmm_DCT2);

    xmm_even0L = _mm_add_epi32(xmm_evenEven0L, xmm_evenOdd0L);
    xmm_even0H = _mm_add_epi32(xmm_evenEven0H, xmm_evenOdd0H);
    xmm_even3L = _mm_sub_epi32(xmm_evenEven0L, xmm_evenOdd0L);
    xmm_even3H = _mm_sub_epi32(xmm_evenEven0H, xmm_evenOdd0H);

    xmm_even1L = _mm_add_epi32(xmm_evenEven1L, xmm_evenOdd1L);
    xmm_even1H = _mm_add_epi32(xmm_evenEven1H, xmm_evenOdd1H);
    xmm_even2L = _mm_sub_epi32(xmm_evenEven1L, xmm_evenOdd1L);
    xmm_even2H = _mm_sub_epi32(xmm_evenEven1H, xmm_evenOdd1H);

    MACRO_8X8_FINAL(xmm_even0L, xmm_even0H, xmm_res0, xmm_res7, 0,    srai_epi32, 7)
    MACRO_8X8_FINAL(xmm_even1L, xmm_even1H, xmm_res1, xmm_res6, 0x10, srai_epi32, 7)
    MACRO_8X8_FINAL(xmm_even2L, xmm_even2H, xmm_res2, xmm_res5, 0x20, srai_epi32, 7)
    MACRO_8X8_FINAL(xmm_even3L, xmm_even3H, xmm_res3, xmm_res4, 0x30, srai_epi32, 7)

    MACRO_UNPACK(16, xmm_res0, xmm_res1, xmm_res2,     xmm_res3,     xmm_res4,     xmm_res5,     xmm_res6,     xmm_res7,     xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi)
    MACRO_UNPACK(32, xmm_res0, xmm_res2, xmm_res01_hi, xmm_res23_hi, xmm_res4,     xmm_res6,     xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res13_hi, xmm_res46_hi, xmm_res57_hi)
    MACRO_UNPACK(64, xmm_res0, xmm_res4, xmm_res02_hi, xmm_res46_hi, xmm_res01_hi, xmm_res45_hi, xmm_res13_hi, xmm_res57_hi, xmm_res04_hi, xmm_res26_hi, xmm_res15_hi, xmm_res37_hi)

    // Store even trasnform coefficient results (0,2,4,6) into transformArrayPtr
    _mm_store_si128((__m128i *)(transformInnerArrayPtrAligned + 0x40), xmm_res0);
    _mm_store_si128((__m128i *)(transformInnerArrayPtrAligned + 0x48), xmm_res02_hi);
    _mm_store_si128((__m128i *)(transformInnerArrayPtrAligned + 0x50), xmm_res01_hi);
    _mm_store_si128((__m128i *)(transformInnerArrayPtrAligned + 0x58), xmm_res13_hi);

    // Odd transform coefficients from transformArrayPtr (high results from MACRO_UNPACK)
    xmm_trans_1_3_lo = _mm_unpacklo_epi16(xmm_res04_hi, xmm_res15_hi);
    xmm_trans_1_3_hi = _mm_unpackhi_epi16(xmm_res04_hi, xmm_res15_hi);
    xmm_trans_2_4_lo = _mm_unpacklo_epi16(xmm_res26_hi, xmm_res37_hi);
    xmm_trans_2_4_hi = _mm_unpackhi_epi16(xmm_res26_hi, xmm_res37_hi);

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_89_50));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_75_18));

    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0)

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_75_N89));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_N18_N50));

    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0x10)

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_50_18));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_N89_75));

    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0x20)

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_18_75));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_N50_N89));

    MACRO_8X8_ODD(xmm_trans_1_3_lo, xmm_trans_1_3_hi, xmm_trans_2_4_lo, xmm_trans_2_4_hi, xmm_DCT1, xmm_DCT2, 0x30)

    // Load even transform coefficients from transformArrayPtr
    xmm_trans1 = _mm_load_si128((__m128i *)(transformInnerArrayPtrAligned + 0x40));
    xmm_trans2 = _mm_load_si128((__m128i *)(transformInnerArrayPtrAligned + 0x48));
    xmm_trans3 = _mm_load_si128((__m128i *)(transformInnerArrayPtrAligned + 0x50));
    xmm_trans4 = _mm_load_si128((__m128i *)(transformInnerArrayPtrAligned + 0x58));

    xmm_trans_1_3_hi = _mm_unpackhi_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_1_3_lo = _mm_unpacklo_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_2_4_hi = _mm_unpackhi_epi16(xmm_trans2, xmm_trans4);
    xmm_trans_2_4_lo = _mm_unpacklo_epi16(xmm_trans2, xmm_trans4);

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_64_64));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_64_N64));
    xmm_offset = _mm_srli_epi32(_mm_set1_epi32(0x0800), bitIncrement);

    xmm_evenEven0L = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_lo, xmm_DCT1), xmm_offset);
    xmm_evenEven0H = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_hi, xmm_DCT1), xmm_offset);

    xmm_evenEven1L = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_lo, xmm_DCT2), xmm_offset);
    xmm_evenEven1H = _mm_add_epi32(_mm_madd_epi16(xmm_trans_1_3_hi, xmm_DCT2), xmm_offset);

    xmm_DCT1 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_83_36));
    xmm_DCT2 = _mm_load_si128((__m128i *)(InvTransformAsmConst_SSE2 + OFFSET_36_N83));

    xmm_evenOdd0L = _mm_madd_epi16(xmm_trans_2_4_lo, xmm_DCT1);
    xmm_evenOdd0H = _mm_madd_epi16(xmm_trans_2_4_hi, xmm_DCT1);
    xmm_evenOdd1L = _mm_madd_epi16(xmm_trans_2_4_lo, xmm_DCT2);
    xmm_evenOdd1H = _mm_madd_epi16(xmm_trans_2_4_hi, xmm_DCT2);

    xmm_even0L = _mm_add_epi32(xmm_evenEven0L, xmm_evenOdd0L);
    xmm_even0H = _mm_add_epi32(xmm_evenEven0H, xmm_evenOdd0H);
    xmm_even3L = _mm_sub_epi32(xmm_evenEven0L, xmm_evenOdd0L);
    xmm_even3H = _mm_sub_epi32(xmm_evenEven0H, xmm_evenOdd0H);

    xmm_even1L = _mm_add_epi32(xmm_evenEven1L, xmm_evenOdd1L);
    xmm_even1H = _mm_add_epi32(xmm_evenEven1H, xmm_evenOdd1H);
    xmm_even2L = _mm_sub_epi32(xmm_evenEven1L, xmm_evenOdd1L);
    xmm_even2H = _mm_sub_epi32(xmm_evenEven1H, xmm_evenOdd1H);
    xmm_shift = _mm_cvtsi32_si128(SHIFT_INV_2ND - bitIncrement); //shift2nd

    MACRO_8X8_FINAL(xmm_even0L, xmm_even0H, xmm_res0, xmm_res7, 0, sra_epi32, xmm_shift)
    MACRO_8X8_FINAL(xmm_even1L, xmm_even1H, xmm_res1, xmm_res6, 0x10, sra_epi32, xmm_shift)
    MACRO_8X8_FINAL(xmm_even2L, xmm_even2H, xmm_res2, xmm_res5, 0x20, sra_epi32, xmm_shift)
    MACRO_8X8_FINAL(xmm_even3L, xmm_even3H, xmm_res3, xmm_res4, 0x30, sra_epi32, xmm_shift)

    MACRO_UNPACK(16, xmm_res0, xmm_res1, xmm_res2,     xmm_res3,     xmm_res4,     xmm_res5,     xmm_res6,     xmm_res7,     xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi)
    MACRO_UNPACK(32, xmm_res0, xmm_res2, xmm_res01_hi, xmm_res23_hi, xmm_res4,     xmm_res6,     xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res13_hi, xmm_res46_hi, xmm_res57_hi)
    MACRO_UNPACK(64, xmm_res0, xmm_res4, xmm_res02_hi, xmm_res46_hi, xmm_res01_hi, xmm_res45_hi, xmm_res13_hi, xmm_res57_hi, xmm_res04_hi, xmm_res26_hi, xmm_res15_hi, xmm_res37_hi)

    _mm_storeu_si128((__m128i *)(residual), xmm_res0);
    _mm_storeu_si128((__m128i *)(residual + dstStride), xmm_res04_hi);
    _mm_storeu_si128((__m128i *)(residual + 2 * dstStride), xmm_res02_hi);
    _mm_storeu_si128((__m128i *)(residual + 3 * dstStride), xmm_res26_hi);
    residual += 4 * dstStride;
    _mm_storeu_si128((__m128i *)(residual), xmm_res01_hi);
    _mm_storeu_si128((__m128i *)(residual + dstStride), xmm_res15_hi);
    _mm_storeu_si128((__m128i *)(residual + 2 * dstStride), xmm_res13_hi);
    _mm_storeu_si128((__m128i *)(residual + 3 * dstStride), xmm_res37_hi);
}



void InvDstTransform4x4_SSE2_INTRIN(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define OFFSET_DST_64      0
#define OFFSET_DST_29_84   (8+OFFSET_DST_64)
#define OFFSET_DST_74_55   (8+OFFSET_DST_29_84)
#define OFFSET_DST_55_N29  (8+OFFSET_DST_74_55)
#define OFFSET_DST_74_N84  (8+OFFSET_DST_55_N29)
#define OFFSET_DST_74_N74  (8+OFFSET_DST_74_N84)
#define OFFSET_DST_0_74    (8+OFFSET_DST_74_N74)
#define OFFSET_DST_84_55   (8+OFFSET_DST_0_74)
#define OFFSET_DST_N74_N29 (8+OFFSET_DST_84_55)

#define MACRO_DST_4X4(SHIFT_INSTR, SHIFT)\
    xmm_res1 = _mm_##SHIFT_INSTR(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_29_84)), xmm_trans_1_3),\
                                                             _mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_74_55)), xmm_trans_2_4)),\
                                               xmm_offset), SHIFT);\
    xmm_res2 = _mm_##SHIFT_INSTR(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_55_N29)), xmm_trans_1_3),\
                                                             _mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_74_N84)), xmm_trans_2_4)),\
                                               xmm_offset), SHIFT);\
    xmm_res3 = _mm_##SHIFT_INSTR(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_74_N74)), xmm_trans_1_3),\
                                                             _mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_0_74)), xmm_trans_2_4)),\
                                               xmm_offset), SHIFT);\
    xmm_res4 = _mm_##SHIFT_INSTR(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_84_55)), xmm_trans_1_3),\
                                                             _mm_madd_epi16(_mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_N74_N29)), xmm_trans_2_4)),\
                                               xmm_offset), SHIFT);\
    xmm_res_1_3 = _mm_packs_epi32(xmm_res1, xmm_res3);\
    xmm_res_2_4 = _mm_packs_epi32(xmm_res2, xmm_res4);\
    xmm_res_lo = _mm_unpacklo_epi16(xmm_res_1_3, xmm_res_2_4);\
    xmm_res_hi = _mm_unpackhi_epi16(xmm_res_1_3, xmm_res_2_4);\
    xmm_res_lo_32 = _mm_unpacklo_epi32(xmm_res_lo, xmm_res_hi);\
    xmm_res_hi_32 = _mm_unpackhi_epi32(xmm_res_lo, xmm_res_hi);


    __m128i xmm_shift, xmm_offset, xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans4, xmm_trans_1_3, xmm_trans_2_4;
    __m128i xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res_1_3, xmm_res_2_4, xmm_res_lo_32, xmm_res_hi_32, xmm_res_lo, xmm_res_hi;

    xmm_shift = _mm_cvtsi32_si128(12 - bitIncrement );
    xmm_trans1 = _mm_loadl_epi64((__m128i *)(transformCoefficients));
    xmm_trans2 = _mm_loadl_epi64((__m128i *)(transformCoefficients + srcStride));
    xmm_trans3 = _mm_loadl_epi64((__m128i *)(transformCoefficients + srcStride * 2));
    xmm_trans4 = _mm_loadl_epi64((__m128i *)(transformCoefficients + srcStride * 3));
    xmm_trans_1_3 = _mm_unpacklo_epi16(xmm_trans1, xmm_trans3);
    xmm_trans_2_4 = _mm_unpacklo_epi16(xmm_trans2, xmm_trans4);

    xmm_offset = _mm_load_si128((__m128i *)(InvDstTransformAsmConst_SSE2 + OFFSET_DST_64));
    MACRO_DST_4X4(srai_epi32, 7)

    xmm_trans_1_3 = _mm_unpacklo_epi16(xmm_res_lo_32, xmm_res_hi_32);
    xmm_trans_2_4 = _mm_unpackhi_epi16(xmm_res_lo_32, xmm_res_hi_32);

    xmm_offset = _mm_srli_epi32(_mm_sll_epi32(xmm_offset, xmm_shift), 7);
    MACRO_DST_4X4(sra_epi32, xmm_shift)

    _mm_storel_epi64((__m128i *)(residual), xmm_res_lo_32);
    xmm_res_lo_32 = _mm_srli_si128(xmm_res_lo_32, 8);
    _mm_storel_epi64((__m128i *)(residual + dstStride), xmm_res_lo_32);
    _mm_storel_epi64((__m128i *)(residual + 2 * dstStride), xmm_res_hi_32);
    xmm_res_hi_32 = _mm_srli_si128(xmm_res_hi_32, 8);
    _mm_storel_epi64((__m128i *)(residual + 3 * dstStride), xmm_res_hi_32);

    (void)transformInnerArrayPtr;
}