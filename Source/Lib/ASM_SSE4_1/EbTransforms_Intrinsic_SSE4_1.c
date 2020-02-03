/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include <smmintrin.h>
#include "EbTransforms_SSE4_1.h"

#define MACRO_UNPACK(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13)\
    ARG10 = _mm_unpackhi_epi##ARG1(ARG2, ARG3);\
    ARG2  = _mm_unpacklo_epi##ARG1(ARG2, ARG3);\
    ARG11 = _mm_unpackhi_epi##ARG1(ARG4, ARG5);\
    ARG4  = _mm_unpacklo_epi##ARG1(ARG4, ARG5);\
    ARG12 = _mm_unpackhi_epi##ARG1(ARG6, ARG7);\
    ARG6  = _mm_unpacklo_epi##ARG1(ARG6, ARG7);\
    ARG13 = _mm_unpackhi_epi##ARG1(ARG8, ARG9);\
    ARG8  = _mm_unpacklo_epi##ARG1(ARG8, ARG9);

void Transform8x8_SSE4_1_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define OFFSET_2        0
#define OFFSET_4        (8+OFFSET_2)
#define OFFSET_8        (8+OFFSET_4)
#define OFFSET_9        (8+OFFSET_8)
#define OFFSET_13       (8+OFFSET_9)
#define OFFSET_16       (8+OFFSET_13)
#define OFFSET_18       (8+OFFSET_16)
#define OFFSET_22       (8+OFFSET_18)
#define OFFSET_25       (8+OFFSET_22)
#define OFFSET_31       (8+OFFSET_25)
#define OFFSET_36       (8+OFFSET_31)
#define OFFSET_38       (8+OFFSET_36)
#define OFFSET_43       (8+OFFSET_38)
#define OFFSET_46       (8+OFFSET_43)
#define OFFSET_50       (8+OFFSET_46)
#define OFFSET_54       (8+OFFSET_50)
#define OFFSET_57       (8+OFFSET_54)
#define OFFSET_61       (8+OFFSET_57)
#define OFFSET_67       (8+OFFSET_61)
#define OFFSET_70       (8+OFFSET_67)
#define OFFSET_73       (8+OFFSET_70)
#define OFFSET_75       (8+OFFSET_73)
#define OFFSET_78       (8+OFFSET_75)
#define OFFSET_80       (8+OFFSET_78)
#define OFFSET_82       (8+OFFSET_80)
#define OFFSET_83       (8+OFFSET_82)
#define OFFSET_85       (8+OFFSET_83)
#define OFFSET_87       (8+OFFSET_85)
#define OFFSET_88       (8+OFFSET_87)
#define OFFSET_89       (8+OFFSET_88)
#define OFFSET_90       (8+OFFSET_89)
#define OFFSET_256      (8+OFFSET_90)
#define OFFSET_512      (8+OFFSET_256)
#define OFFSET_1024     (8+OFFSET_512)
#define OFFSET_83_36    (8+OFFSET_1024)
#define OFFSET_36_N83   (8+OFFSET_83_36)
#define OFFSET_89_75    (8+OFFSET_36_N83)
#define OFFSET_50_18    (8+OFFSET_89_75)
#define OFFSET_75_N18   (8+OFFSET_50_18)
#define OFFSET_N89_N50  (8+OFFSET_75_N18)
#define OFFSET_50_N89   (8+OFFSET_N89_N50)
#define OFFSET_18_75    (8+OFFSET_50_N89)
#define OFFSET_18_N50   (8+OFFSET_18_75)
#define OFFSET_75_N89   (8+OFFSET_18_N50)

#define MACRO_CALC_EVEN_ODD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8)\
    xmm_even0 = _mm_add_epi16(XMM1, XMM8);\
    xmm_even1 = _mm_add_epi16(XMM2, XMM7);\
    xmm_even2 = _mm_add_epi16(XMM3, XMM6);\
    xmm_even3 = _mm_add_epi16(XMM4, XMM5);\
    xmm_odd0 = _mm_sub_epi16(XMM1, XMM8);\
    xmm_odd1 = _mm_sub_epi16(XMM2, XMM7);\
    xmm_odd2 = _mm_sub_epi16(XMM3, XMM6);\
    xmm_odd3 = _mm_sub_epi16(XMM4, XMM5);

#define MACRO_TRANS_4MAC_NO_SAVE(XMM1, XMM2, XMM3, XMM4, XMM_RET, XMM_OFFSET, MEM, OFFSET1, OFFSET2, SHIFT)\
    XMM_RET = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM1, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM3, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT),\
                              _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM2, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM4, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT));

#define MACRO_TRANS_4MAC_PMULLD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, INSTR1, INSTR2, INSTR3, SHIFT, OFST5)\
    xmm_temp1 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM1, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM2, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp3 = _mm_##INSTR2(_mm_mullo_epi32(XMM3, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM4, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp1 = _mm_##INSTR3(xmm_temp1, xmm_temp3);\
    xmm_temp1 = _mm_srai_epi32(xmm_temp1, SHIFT);\
    xmm_temp5 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM5, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM6, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp7 = _mm_##INSTR2(_mm_mullo_epi32(XMM7, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM8, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp5 = _mm_##INSTR3(xmm_temp5, xmm_temp7);\
    xmm_temp5 = _mm_srai_epi32(xmm_temp5, SHIFT);\
    xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_temp5);\
    _mm_storeu_si128((__m128i *)(transformCoefficients+OFST5), xmm_temp1);

    __m128i xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res04_hi, xmm_res0246_hi, xmm_res0145_hi, xmm_res_1to7_hi;
    __m128i xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res46_hi, xmm_res0123_hi, xmm_res4567_hi;
    __m128i xmm_trans0, xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans4, xmm_trans5, xmm_trans6, xmm_trans7;
    __m128i xmm_trans0_lo, xmm_trans4_lo, xmm_trans0_hi, xmm_trans4_hi;
    __m128i xmm_even0, xmm_even1, xmm_even2, xmm_even3, xmm_odd0, xmm_odd1, xmm_odd2, xmm_odd3, xmm_evenEven0, xmm_evenEven1, xmm_evenOdd0, xmm_evenOdd1;
    __m128i xmm_even0L, xmm_even1L, xmm_even2L, xmm_even3L, xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_evenEven0L, xmm_evenEven1L, xmm_evenOdd0L, xmm_evenOdd1L;
    __m128i xmm_offset, xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi;
    __m128i xmm_trans01_hi, xmm_trans23_hi, xmm_trans45_hi, xmm_trans67_hi, xmm_trans02_hi, xmm_trans01234_hi, xmm_trans46_hi, xmm_trans4567_hi;
    __m128i xmm_even0H, xmm_even1H, xmm_even2H, xmm_even3H, xmm_odd0H, xmm_odd1H, xmm_odd2H, xmm_odd3H, xmm_evenEven0H, xmm_evenEven1H, xmm_evenOdd0H, xmm_evenOdd1H;;
    __m128i xmm_res0L, xmm_res1L, xmm_res2L, xmm_res3L, xmm_res4L, xmm_res5L, xmm_res6L, xmm_res7L;
    __m128i xmm_res0H, xmm_res1H, xmm_res2H, xmm_res3H, xmm_res4H, xmm_res5H, xmm_res6H, xmm_res7H;
    __m128i xmm_temp1, xmm_temp3, xmm_temp5, xmm_temp7;

    xmm_res0 = _mm_loadu_si128((__m128i *)(residual));
    xmm_res1 = _mm_loadu_si128((__m128i *)(residual+srcStride));
    xmm_res2 = _mm_loadu_si128((__m128i *)(residual+2*srcStride));
    xmm_res3 = _mm_loadu_si128((__m128i *)(residual+3*srcStride));
    residual += 4 * srcStride;
    xmm_res4 = _mm_loadu_si128((__m128i *)(residual));
    xmm_res5 = _mm_loadu_si128((__m128i *)(residual+srcStride));
    xmm_res6 = _mm_loadu_si128((__m128i *)(residual+2*srcStride));
    xmm_res7 = _mm_loadu_si128((__m128i *)(residual+3*srcStride));

    MACRO_UNPACK(16, xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi)
    MACRO_UNPACK(32, xmm_res0, xmm_res2, xmm_res01_hi, xmm_res23_hi, xmm_res4, xmm_res6, xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res0123_hi, xmm_res46_hi, xmm_res4567_hi)
    MACRO_UNPACK(64, xmm_res0, xmm_res4, xmm_res02_hi, xmm_res46_hi, xmm_res01_hi, xmm_res45_hi, xmm_res0123_hi, xmm_res4567_hi, xmm_res04_hi, xmm_res0246_hi, xmm_res0145_hi, xmm_res_1to7_hi)

    MACRO_CALC_EVEN_ODD(xmm_res0, xmm_res04_hi, xmm_res02_hi, xmm_res0246_hi, xmm_res01_hi, xmm_res0145_hi, xmm_res0123_hi, xmm_res_1to7_hi)

    // TransformCoefficients 0, 2, 4, 6
    xmm_evenEven0 = _mm_add_epi16(xmm_even0, xmm_even3);
    xmm_evenEven1 = _mm_add_epi16(xmm_even1, xmm_even2);
    xmm_evenOdd0 = _mm_sub_epi16(xmm_even0, xmm_even3);
    xmm_evenOdd1 = _mm_sub_epi16(xmm_even1, xmm_even2);

    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_2));

    xmm_trans0 = _mm_slli_epi16(_mm_add_epi16(xmm_evenEven0, xmm_evenEven1), 4);
    xmm_trans4 = _mm_slli_epi16(_mm_sub_epi16(xmm_evenEven0, xmm_evenEven1), 4);

    xmm_trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83_36)),_mm_unpacklo_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2),
                                 _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83_36)),_mm_unpackhi_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2));

    xmm_trans6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36_N83)),_mm_unpacklo_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2),
                                 _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36_N83)),_mm_unpackhi_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2));

    // TransformCoefficients 1, 3, 5, 7
    xmm_odd01_lo = _mm_unpacklo_epi16(xmm_odd0, xmm_odd1);
    xmm_odd01_hi = _mm_unpackhi_epi16(xmm_odd0, xmm_odd1);
    xmm_odd23_lo = _mm_unpacklo_epi16(xmm_odd2, xmm_odd3);
    xmm_odd23_hi = _mm_unpackhi_epi16(xmm_odd2, xmm_odd3);

    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans1, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_89_75, OFFSET_50_18, 2)
    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans3, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_75_N18, OFFSET_N89_N50, 2)
    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans5, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_50_N89, OFFSET_18_75, 2)
    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans7, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_18_N50, OFFSET_75_N89, 2)

    MACRO_UNPACK(16, xmm_trans0, xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans4, xmm_trans5, xmm_trans6, xmm_trans7, xmm_trans01_hi, xmm_trans23_hi, xmm_trans45_hi, xmm_trans67_hi)
    MACRO_UNPACK(32, xmm_trans0, xmm_trans2, xmm_trans01_hi, xmm_trans23_hi, xmm_trans4, xmm_trans6, xmm_trans45_hi, xmm_trans67_hi, xmm_trans02_hi, xmm_trans01234_hi, xmm_trans46_hi, xmm_trans4567_hi)


    //---- Second Partial Butterfly ----

    // Calculate low and high
    xmm_res0L = _mm_unpacklo_epi16(xmm_trans0,        _mm_srai_epi16(xmm_trans0, 15));
    xmm_res1L = _mm_unpackhi_epi16(xmm_trans0,        _mm_srai_epi16(xmm_trans0, 15));
    xmm_res2L = _mm_unpacklo_epi16(xmm_trans02_hi,    _mm_srai_epi16(xmm_trans02_hi, 15));
    xmm_res3L = _mm_unpackhi_epi16(xmm_trans02_hi,    _mm_srai_epi16(xmm_trans02_hi, 15));
    xmm_res4L = _mm_unpacklo_epi16(xmm_trans01_hi,    _mm_srai_epi16(xmm_trans01_hi, 15));
    xmm_res5L = _mm_unpackhi_epi16(xmm_trans01_hi,    _mm_srai_epi16(xmm_trans01_hi, 15));
    xmm_res6L = _mm_unpacklo_epi16(xmm_trans01234_hi, _mm_srai_epi16(xmm_trans01234_hi, 15));
    xmm_res7L = _mm_unpackhi_epi16(xmm_trans01234_hi, _mm_srai_epi16(xmm_trans01234_hi, 15));

    xmm_res0H = _mm_unpacklo_epi16(xmm_trans4,       _mm_srai_epi16(xmm_trans4, 15));
    xmm_res1H = _mm_unpackhi_epi16(xmm_trans4,       _mm_srai_epi16(xmm_trans4, 15));
    xmm_res2H = _mm_unpacklo_epi16(xmm_trans46_hi,   _mm_srai_epi16(xmm_trans46_hi, 15));
    xmm_res3H = _mm_unpackhi_epi16(xmm_trans46_hi,   _mm_srai_epi16(xmm_trans46_hi, 15));
    xmm_res4H = _mm_unpacklo_epi16(xmm_trans45_hi,   _mm_srai_epi16(xmm_trans45_hi, 15));
    xmm_res5H = _mm_unpackhi_epi16(xmm_trans45_hi,   _mm_srai_epi16(xmm_trans45_hi, 15));
    xmm_res6H = _mm_unpacklo_epi16(xmm_trans4567_hi, _mm_srai_epi16(xmm_trans4567_hi, 15));
    xmm_res7H = _mm_unpackhi_epi16(xmm_trans4567_hi, _mm_srai_epi16(xmm_trans4567_hi, 15));

    xmm_even0L = _mm_add_epi32(xmm_res0L, xmm_res7L);
    xmm_odd0L  = _mm_sub_epi32(xmm_res0L, xmm_res7L);
    xmm_even1L = _mm_add_epi32(xmm_res1L, xmm_res6L);
    xmm_odd1L  = _mm_sub_epi32(xmm_res1L, xmm_res6L);
    xmm_even2L = _mm_add_epi32(xmm_res2L, xmm_res5L);
    xmm_odd2L  = _mm_sub_epi32(xmm_res2L, xmm_res5L);
    xmm_even3L = _mm_add_epi32(xmm_res3L, xmm_res4L);
    xmm_odd3L  = _mm_sub_epi32(xmm_res3L, xmm_res4L);

    xmm_even0H = _mm_add_epi32(xmm_res0H, xmm_res7H);
    xmm_odd0H  = _mm_sub_epi32(xmm_res0H, xmm_res7H);
    xmm_even1H = _mm_add_epi32(xmm_res1H, xmm_res6H);
    xmm_odd1H  = _mm_sub_epi32(xmm_res1H, xmm_res6H);
    xmm_even2H = _mm_add_epi32(xmm_res2H, xmm_res5H);
    xmm_odd2H  = _mm_sub_epi32(xmm_res2H, xmm_res5H);
    xmm_even3H = _mm_add_epi32(xmm_res3H, xmm_res4H);
    xmm_odd3H  = _mm_sub_epi32(xmm_res3H, xmm_res4H);

    // TransformCoefficients 0, 4
    xmm_evenEven0L = _mm_add_epi32(xmm_even0L, xmm_even3L);
    xmm_evenEven1L = _mm_add_epi32(xmm_even1L, xmm_even2L);
    xmm_evenOdd0L = _mm_sub_epi32(xmm_even0L,  xmm_even3L);
    xmm_evenOdd1L = _mm_sub_epi32(xmm_even1L,  xmm_even2L);

    xmm_evenEven0H = _mm_add_epi32(xmm_even0H, xmm_even3H);
    xmm_evenEven1H = _mm_add_epi32(xmm_even1H, xmm_even2H);
    xmm_evenOdd0H = _mm_sub_epi32(xmm_even0H,  xmm_even3H);
    xmm_evenOdd1H = _mm_sub_epi32(xmm_even1H,  xmm_even2H);

    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_4));

    xmm_trans0_lo = _mm_srai_epi32(_mm_add_epi32(xmm_offset, _mm_add_epi32(xmm_evenEven0L, xmm_evenEven1L)), 3);
    xmm_trans0_lo = _mm_packs_epi32(xmm_trans0_lo, xmm_trans0_lo);
    xmm_trans4_lo = _mm_srai_epi32(_mm_add_epi32(xmm_offset, _mm_sub_epi32(xmm_evenEven0L, xmm_evenEven1L)), 3);
    xmm_trans4_lo = _mm_packs_epi32(xmm_trans4_lo, xmm_trans4_lo);

    xmm_trans0_hi = _mm_srai_epi32(_mm_add_epi32(xmm_offset, _mm_add_epi32(xmm_evenEven0H, xmm_evenEven1H)), 3);
    xmm_trans0_hi = _mm_packs_epi32(xmm_trans0_hi, xmm_trans0_hi);
    xmm_trans4_hi = _mm_srai_epi32(_mm_add_epi32(xmm_offset, _mm_sub_epi32(xmm_evenEven0H, xmm_evenEven1H)), 3);
    xmm_trans4_hi = _mm_packs_epi32(xmm_trans4_hi, xmm_trans4_hi);

    _mm_storel_epi64((__m128i *)(transformCoefficients), xmm_trans0_lo);
    _mm_storel_epi64((__m128i *)(transformCoefficients + 4), xmm_trans0_hi);
    _mm_storel_epi64((__m128i *)(transformCoefficients + 4 * dstStride), xmm_trans4_lo);
    _mm_storel_epi64((__m128i *)(transformCoefficients + 4 * dstStride + 4), xmm_trans4_hi);

    // TransformCoefficients 2, 6
    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_256));

    xmm_trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83)),xmm_evenOdd0L),
                                                                            _mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36)),xmm_evenOdd1L)),
                                                              xmm_offset), 9),
                                 _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83)),xmm_evenOdd0H),
                                                                            _mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36)),xmm_evenOdd1H)),
                                                              xmm_offset), 9));

    _mm_storeu_si128((__m128i *)(transformCoefficients + 2 * dstStride), xmm_trans2);

    xmm_trans6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_sub_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36)),xmm_evenOdd0L),
                                                                            _mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83)),xmm_evenOdd1L)),
                                                              xmm_offset), 9),
                                 _mm_srai_epi32(_mm_add_epi32(_mm_sub_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36)),xmm_evenOdd0H),
                                                                            _mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83)),xmm_evenOdd1H)),
                                                              xmm_offset), 9));

    _mm_storeu_si128((__m128i *)(transformCoefficients + 6 * dstStride), xmm_trans6);

    // Transform Coefficients 1, 3, 5, 7

    MACRO_TRANS_4MAC_PMULLD(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_odd0H, xmm_odd1H, xmm_odd2H, xmm_odd3H, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_89, OFFSET_75, OFFSET_50, OFFSET_18, add_epi32, add_epi32, add_epi32, 9, dstStride)
    MACRO_TRANS_4MAC_PMULLD(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_odd0H, xmm_odd1H, xmm_odd2H, xmm_odd3H, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_75, OFFSET_18, OFFSET_89, OFFSET_50, sub_epi32, add_epi32, sub_epi32, 9, 3 * dstStride)
    transformCoefficients += 4 * dstStride;
    MACRO_TRANS_4MAC_PMULLD(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_odd0H, xmm_odd1H, xmm_odd2H, xmm_odd3H, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_50, OFFSET_89, OFFSET_18, OFFSET_75, sub_epi32, add_epi32, add_epi32, 9, dstStride)
    MACRO_TRANS_4MAC_PMULLD(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_odd0H, xmm_odd1H, xmm_odd2H, xmm_odd3H, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_18, OFFSET_50, OFFSET_75, OFFSET_89, sub_epi32, sub_epi32, add_epi32, 9, 3 * dstStride)

    (void)bitIncrement;
    (void)transformInnerArrayPtr;

}
void PfreqTransform8x8_SSE4_1_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define OFFSET_2        0
#define OFFSET_4        (8+OFFSET_2)
#define OFFSET_8        (8+OFFSET_4)
#define OFFSET_9        (8+OFFSET_8)
#define OFFSET_13       (8+OFFSET_9)
#define OFFSET_16       (8+OFFSET_13)
#define OFFSET_18       (8+OFFSET_16)
#define OFFSET_22       (8+OFFSET_18)
#define OFFSET_25       (8+OFFSET_22)
#define OFFSET_31       (8+OFFSET_25)
#define OFFSET_36       (8+OFFSET_31)
#define OFFSET_38       (8+OFFSET_36)
#define OFFSET_43       (8+OFFSET_38)
#define OFFSET_46       (8+OFFSET_43)
#define OFFSET_50       (8+OFFSET_46)
#define OFFSET_54       (8+OFFSET_50)
#define OFFSET_57       (8+OFFSET_54)
#define OFFSET_61       (8+OFFSET_57)
#define OFFSET_67       (8+OFFSET_61)
#define OFFSET_70       (8+OFFSET_67)
#define OFFSET_73       (8+OFFSET_70)
#define OFFSET_75       (8+OFFSET_73)
#define OFFSET_78       (8+OFFSET_75)
#define OFFSET_80       (8+OFFSET_78)
#define OFFSET_82       (8+OFFSET_80)
#define OFFSET_83       (8+OFFSET_82)
#define OFFSET_85       (8+OFFSET_83)
#define OFFSET_87       (8+OFFSET_85)
#define OFFSET_88       (8+OFFSET_87)
#define OFFSET_89       (8+OFFSET_88)
#define OFFSET_90       (8+OFFSET_89)
#define OFFSET_256      (8+OFFSET_90)
#define OFFSET_512      (8+OFFSET_256)
#define OFFSET_1024     (8+OFFSET_512)
#define OFFSET_83_36    (8+OFFSET_1024)
#define OFFSET_36_N83   (8+OFFSET_83_36)
#define OFFSET_89_75    (8+OFFSET_36_N83)
#define OFFSET_50_18    (8+OFFSET_89_75)
#define OFFSET_75_N18   (8+OFFSET_50_18)
#define OFFSET_N89_N50  (8+OFFSET_75_N18)
#define OFFSET_50_N89   (8+OFFSET_N89_N50)
#define OFFSET_18_75    (8+OFFSET_50_N89)
#define OFFSET_18_N50   (8+OFFSET_18_75)
#define OFFSET_75_N89   (8+OFFSET_18_N50)

#define MACRO_CALC_EVEN_ODD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8)\
    xmm_even0 = _mm_add_epi16(XMM1, XMM8);\
    xmm_even1 = _mm_add_epi16(XMM2, XMM7);\
    xmm_even2 = _mm_add_epi16(XMM3, XMM6);\
    xmm_even3 = _mm_add_epi16(XMM4, XMM5);\
    xmm_odd0 = _mm_sub_epi16(XMM1, XMM8);\
    xmm_odd1 = _mm_sub_epi16(XMM2, XMM7);\
    xmm_odd2 = _mm_sub_epi16(XMM3, XMM6);\
    xmm_odd3 = _mm_sub_epi16(XMM4, XMM5);

#define MACRO_TRANS_4MAC_NO_SAVE(XMM1, XMM2, XMM3, XMM4, XMM_RET, XMM_OFFSET, MEM, OFFSET1, OFFSET2, SHIFT)\
    XMM_RET = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM1, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM3, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT),\
                              _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM2, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM4, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT));

#define MACRO_TRANS_4MAC_PMULLD_PF(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, INSTR1, INSTR2, INSTR3, SHIFT, OFST5)\
    xmm_temp1 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM1, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM2, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp3 = _mm_##INSTR2(_mm_mullo_epi32(XMM3, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM4, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp1 = _mm_##INSTR3(xmm_temp1, xmm_temp3);\
    xmm_temp1 = _mm_srai_epi32(xmm_temp1, SHIFT);\
    /*xmm_temp5 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM5, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM6, _mm_load_si128((__m128i *)(MEM + OFST2))));*/\
    /*xmm_temp7 = _mm_##INSTR2(_mm_mullo_epi32(XMM7, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM8, _mm_load_si128((__m128i *)(MEM + OFST4))));*/\
    /*xmm_temp5 = _mm_##INSTR3(xmm_temp5, xmm_temp7);*/\
    /*xmm_temp5 = _mm_srai_epi32(xmm_temp5, SHIFT);*/\
    /*xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_temp5);*/\
    xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_junk);\
    _mm_storeu_si128((__m128i *)(transformCoefficients+OFST5), xmm_temp1);

#define MACRO_TRANS_4MAC_PMULLD_OLD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, INSTR1, INSTR2, INSTR3, SHIFT, OFST5)\
    xmm_temp1 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM1, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM2, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp3 = _mm_##INSTR2(_mm_mullo_epi32(XMM3, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM4, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp1 = _mm_##INSTR3(xmm_temp1, xmm_temp3);\
    xmm_temp1 = _mm_srai_epi32(xmm_temp1, SHIFT);\
    xmm_temp5 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM5, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM6, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp7 = _mm_##INSTR2(_mm_mullo_epi32(XMM7, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM8, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp5 = _mm_##INSTR3(xmm_temp5, xmm_temp7);\
    xmm_temp5 = _mm_srai_epi32(xmm_temp5, SHIFT);\
    xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_temp5);\
    _mm_storeu_si128((__m128i *)(transformCoefficients+OFST5), xmm_temp1);

    __m128i xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res04_hi, xmm_res0246_hi, xmm_res0145_hi, xmm_res_1to7_hi;
    __m128i xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res46_hi, xmm_res0123_hi, xmm_res4567_hi;
    __m128i xmm_trans0, xmm_trans1, xmm_trans2, xmm_trans3;
    __m128i xmm_trans0_lo/*, xmm_trans4_lo, xmm_trans0_hi, xmm_trans4_hi*/;
    __m128i xmm_even0, xmm_even1, xmm_even2, xmm_even3, xmm_odd0, xmm_odd1, xmm_odd2, xmm_odd3, xmm_evenEven0, xmm_evenEven1, xmm_evenOdd0, xmm_evenOdd1;
    __m128i xmm_even0L, xmm_even1L, xmm_even2L, xmm_even3L, xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_evenEven0L, xmm_evenEven1L, xmm_evenOdd0L, xmm_evenOdd1L;
    __m128i xmm_offset, xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi;
    __m128i xmm_trans01_hi, xmm_trans23_hi, /*xmm_trans45_hi,*/ /*xmm_trans67_hi, */xmm_trans02_hi, xmm_trans01234_hi/*, xmm_trans46_hi, xmm_trans4567_hi*/;
  //  __m128i /*xmm_even0H, */xmm_even1H/*, xmm_even2H,*/ /*xmm_even3H,*/ /*xmm_odd0H,*/ /*xmm_odd1H,*/ /*xmm_odd2H, xmm_odd3H,*/ /*xmm_evenEven0H, xmm_evenEven1H,*/ /*xmm_evenOdd0H, xmm_evenOdd1H*/;;
    __m128i xmm_res0L, xmm_res1L, xmm_res2L, xmm_res3L, xmm_res4L, xmm_res5L, xmm_res6L, xmm_res7L;
   // __m128i xmm_res0H, xmm_res1H, /*xmm_res2H, */xmm_res3H, xmm_res4H, xmm_res5H, /*xmm_res6H, xmm_res7H*/;
    __m128i xmm_temp1, xmm_temp3/*, xmm_temp5, xmm_temp7*/;

     __m128i xmm_junk = _mm_setzero_si128(); //set to zero to avoid valgrind issue

    xmm_res0 = _mm_loadu_si128((__m128i *)(residual));
    xmm_res1 = _mm_loadu_si128((__m128i *)(residual+srcStride));
    xmm_res2 = _mm_loadu_si128((__m128i *)(residual+2*srcStride));
    xmm_res3 = _mm_loadu_si128((__m128i *)(residual+3*srcStride));
    residual += 4 * srcStride;
    xmm_res4 = _mm_loadu_si128((__m128i *)(residual));
    xmm_res5 = _mm_loadu_si128((__m128i *)(residual+srcStride));
    xmm_res6 = _mm_loadu_si128((__m128i *)(residual+2*srcStride));
    xmm_res7 = _mm_loadu_si128((__m128i *)(residual+3*srcStride));

    MACRO_UNPACK(16, xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi)
    MACRO_UNPACK(32, xmm_res0, xmm_res2, xmm_res01_hi, xmm_res23_hi, xmm_res4, xmm_res6, xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res0123_hi, xmm_res46_hi, xmm_res4567_hi)
    MACRO_UNPACK(64, xmm_res0, xmm_res4, xmm_res02_hi, xmm_res46_hi, xmm_res01_hi, xmm_res45_hi, xmm_res0123_hi, xmm_res4567_hi, xmm_res04_hi, xmm_res0246_hi, xmm_res0145_hi, xmm_res_1to7_hi)

    MACRO_CALC_EVEN_ODD(xmm_res0, xmm_res04_hi, xmm_res02_hi, xmm_res0246_hi, xmm_res01_hi, xmm_res0145_hi, xmm_res0123_hi, xmm_res_1to7_hi)

    // TransformCoefficients 0, 2, 4, 6
    xmm_evenEven0 = _mm_add_epi16(xmm_even0, xmm_even3);
    xmm_evenEven1 = _mm_add_epi16(xmm_even1, xmm_even2);
    xmm_evenOdd0 = _mm_sub_epi16(xmm_even0, xmm_even3);
    xmm_evenOdd1 = _mm_sub_epi16(xmm_even1, xmm_even2);

    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_2));

    xmm_trans0 = _mm_slli_epi16(_mm_add_epi16(xmm_evenEven0, xmm_evenEven1), 4);
    //xmm_trans4 = _mm_slli_epi16(_mm_sub_epi16(xmm_evenEven0, xmm_evenEven1), 4);

    xmm_trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83_36)),_mm_unpacklo_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2),
                                 _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83_36)),_mm_unpackhi_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2));


    // TransformCoefficients 1, 3, 5, 7
    xmm_odd01_lo = _mm_unpacklo_epi16(xmm_odd0, xmm_odd1);
    xmm_odd01_hi = _mm_unpackhi_epi16(xmm_odd0, xmm_odd1);
    xmm_odd23_lo = _mm_unpacklo_epi16(xmm_odd2, xmm_odd3);
    xmm_odd23_hi = _mm_unpackhi_epi16(xmm_odd2, xmm_odd3);

    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans1, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_89_75, OFFSET_50_18, 2)
    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans3, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_75_N18, OFFSET_N89_N50, 2)


#define MACRO_UNPACK_PF(ARG1, ARG2, ARG3, ARG4, ARG5, ARG10, ARG11)\
    ARG10 = _mm_unpackhi_epi##ARG1(ARG2, ARG3);\
    ARG2  = _mm_unpacklo_epi##ARG1(ARG2, ARG3);\
    ARG11 = _mm_unpackhi_epi##ARG1(ARG4, ARG5);\
    ARG4  = _mm_unpacklo_epi##ARG1(ARG4, ARG5);

    MACRO_UNPACK_PF(16, xmm_trans0, xmm_trans1, xmm_trans2,     xmm_trans3,     xmm_trans01_hi, xmm_trans23_hi)
    MACRO_UNPACK_PF(32, xmm_trans0, xmm_trans2, xmm_trans01_hi, xmm_trans23_hi, xmm_trans02_hi, xmm_trans01234_hi)



    //---- Second Partial Butterfly ----

    // Calculate low and high
    xmm_res0L = _mm_unpacklo_epi16(xmm_trans0,        _mm_srai_epi16(xmm_trans0, 15));
    xmm_res1L = _mm_unpackhi_epi16(xmm_trans0,        _mm_srai_epi16(xmm_trans0, 15));
    xmm_res2L = _mm_unpacklo_epi16(xmm_trans02_hi,    _mm_srai_epi16(xmm_trans02_hi, 15));
    xmm_res3L = _mm_unpackhi_epi16(xmm_trans02_hi,    _mm_srai_epi16(xmm_trans02_hi, 15));
    xmm_res4L = _mm_unpacklo_epi16(xmm_trans01_hi,    _mm_srai_epi16(xmm_trans01_hi, 15));
    xmm_res5L = _mm_unpackhi_epi16(xmm_trans01_hi,    _mm_srai_epi16(xmm_trans01_hi, 15));
    xmm_res6L = _mm_unpacklo_epi16(xmm_trans01234_hi, _mm_srai_epi16(xmm_trans01234_hi, 15));
    xmm_res7L = _mm_unpackhi_epi16(xmm_trans01234_hi, _mm_srai_epi16(xmm_trans01234_hi, 15));


    xmm_even0L = _mm_add_epi32(xmm_res0L, xmm_res7L);
    xmm_odd0L  = _mm_sub_epi32(xmm_res0L, xmm_res7L);
    xmm_even1L = _mm_add_epi32(xmm_res1L, xmm_res6L);
    xmm_odd1L  = _mm_sub_epi32(xmm_res1L, xmm_res6L);
    xmm_even2L = _mm_add_epi32(xmm_res2L, xmm_res5L);
    xmm_odd2L  = _mm_sub_epi32(xmm_res2L, xmm_res5L);
    xmm_even3L = _mm_add_epi32(xmm_res3L, xmm_res4L);
    xmm_odd3L  = _mm_sub_epi32(xmm_res3L, xmm_res4L);

    // TransformCoefficients 0, 4
    xmm_evenEven0L = _mm_add_epi32(xmm_even0L, xmm_even3L);
    xmm_evenEven1L = _mm_add_epi32(xmm_even1L, xmm_even2L);
    xmm_evenOdd0L = _mm_sub_epi32(xmm_even0L,  xmm_even3L);
    xmm_evenOdd1L = _mm_sub_epi32(xmm_even1L,  xmm_even2L);

    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_4));

    xmm_trans0_lo = _mm_srai_epi32(_mm_add_epi32(xmm_offset, _mm_add_epi32(xmm_evenEven0L, xmm_evenEven1L)), 3);
    xmm_trans0_lo = _mm_packs_epi32(xmm_trans0_lo, xmm_trans0_lo);

    _mm_storel_epi64((__m128i *)(transformCoefficients), xmm_trans0_lo);

    // TransformCoefficients 2, 6
    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_256));

    xmm_trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83)),xmm_evenOdd0L),
                                                                            _mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36)),xmm_evenOdd1L)),
                                                              xmm_offset), 9), xmm_junk);


    _mm_storeu_si128((__m128i *)(transformCoefficients + 2 * dstStride), xmm_trans2);

    MACRO_TRANS_4MAC_PMULLD_PF(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_junk, xmm_junk, xmm_junk, xmm_junk, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_89, OFFSET_75, OFFSET_50, OFFSET_18, add_epi32, add_epi32, add_epi32, 9, dstStride)
    MACRO_TRANS_4MAC_PMULLD_PF(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_junk, xmm_junk, xmm_junk, xmm_junk, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_75, OFFSET_18, OFFSET_89, OFFSET_50, sub_epi32, add_epi32, sub_epi32, 9, 3 * dstStride)

    (void)bitIncrement;
    (void)transformInnerArrayPtr;

}

void PfreqN4Transform8x8_SSE4_1_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define OFFSET_2        0
#define OFFSET_4        (8+OFFSET_2)
#define OFFSET_8        (8+OFFSET_4)
#define OFFSET_9        (8+OFFSET_8)
#define OFFSET_13       (8+OFFSET_9)
#define OFFSET_16       (8+OFFSET_13)
#define OFFSET_18       (8+OFFSET_16)
#define OFFSET_22       (8+OFFSET_18)
#define OFFSET_25       (8+OFFSET_22)
#define OFFSET_31       (8+OFFSET_25)
#define OFFSET_36       (8+OFFSET_31)
#define OFFSET_38       (8+OFFSET_36)
#define OFFSET_43       (8+OFFSET_38)
#define OFFSET_46       (8+OFFSET_43)
#define OFFSET_50       (8+OFFSET_46)
#define OFFSET_54       (8+OFFSET_50)
#define OFFSET_57       (8+OFFSET_54)
#define OFFSET_61       (8+OFFSET_57)
#define OFFSET_67       (8+OFFSET_61)
#define OFFSET_70       (8+OFFSET_67)
#define OFFSET_73       (8+OFFSET_70)
#define OFFSET_75       (8+OFFSET_73)
#define OFFSET_78       (8+OFFSET_75)
#define OFFSET_80       (8+OFFSET_78)
#define OFFSET_82       (8+OFFSET_80)
#define OFFSET_83       (8+OFFSET_82)
#define OFFSET_85       (8+OFFSET_83)
#define OFFSET_87       (8+OFFSET_85)
#define OFFSET_88       (8+OFFSET_87)
#define OFFSET_89       (8+OFFSET_88)
#define OFFSET_90       (8+OFFSET_89)
#define OFFSET_256      (8+OFFSET_90)
#define OFFSET_512      (8+OFFSET_256)
#define OFFSET_1024     (8+OFFSET_512)
#define OFFSET_83_36    (8+OFFSET_1024)
#define OFFSET_36_N83   (8+OFFSET_83_36)
#define OFFSET_89_75    (8+OFFSET_36_N83)
#define OFFSET_50_18    (8+OFFSET_89_75)
#define OFFSET_75_N18   (8+OFFSET_50_18)
#define OFFSET_N89_N50  (8+OFFSET_75_N18)
#define OFFSET_50_N89   (8+OFFSET_N89_N50)
#define OFFSET_18_75    (8+OFFSET_50_N89)
#define OFFSET_18_N50   (8+OFFSET_18_75)
#define OFFSET_75_N89   (8+OFFSET_18_N50)

#define MACRO_CALC_EVEN_ODD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8)\
    xmm_even0 = _mm_add_epi16(XMM1, XMM8);\
    xmm_even1 = _mm_add_epi16(XMM2, XMM7);\
    xmm_even2 = _mm_add_epi16(XMM3, XMM6);\
    xmm_even3 = _mm_add_epi16(XMM4, XMM5);\
    xmm_odd0 = _mm_sub_epi16(XMM1, XMM8);\
    xmm_odd1 = _mm_sub_epi16(XMM2, XMM7);\
    xmm_odd2 = _mm_sub_epi16(XMM3, XMM6);\
    xmm_odd3 = _mm_sub_epi16(XMM4, XMM5);

#define MACRO_TRANS_4MAC_NO_SAVE(XMM1, XMM2, XMM3, XMM4, XMM_RET, XMM_OFFSET, MEM, OFFSET1, OFFSET2, SHIFT)\
    XMM_RET = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM1, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM3, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT),\
                              _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM2, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM4, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT));

#define MACRO_TRANS_4MAC_PMULLD_PF(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, INSTR1, INSTR2, INSTR3, SHIFT, OFST5)\
    xmm_temp1 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM1, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM2, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp3 = _mm_##INSTR2(_mm_mullo_epi32(XMM3, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM4, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp1 = _mm_##INSTR3(xmm_temp1, xmm_temp3);\
    xmm_temp1 = _mm_srai_epi32(xmm_temp1, SHIFT);\
    /*xmm_temp5 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM5, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM6, _mm_load_si128((__m128i *)(MEM + OFST2))));*/\
    /*xmm_temp7 = _mm_##INSTR2(_mm_mullo_epi32(XMM7, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM8, _mm_load_si128((__m128i *)(MEM + OFST4))));*/\
    /*xmm_temp5 = _mm_##INSTR3(xmm_temp5, xmm_temp7);*/\
    /*xmm_temp5 = _mm_srai_epi32(xmm_temp5, SHIFT);*/\
    /*xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_temp5);*/\
    xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_junk);\
    _mm_storeu_si128((__m128i *)(transformCoefficients+OFST5), xmm_temp1);

#define MACRO_TRANS_4MAC_PMULLD_OLD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, INSTR1, INSTR2, INSTR3, SHIFT, OFST5)\
    xmm_temp1 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM1, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM2, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp3 = _mm_##INSTR2(_mm_mullo_epi32(XMM3, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM4, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp1 = _mm_##INSTR3(xmm_temp1, xmm_temp3);\
    xmm_temp1 = _mm_srai_epi32(xmm_temp1, SHIFT);\
    xmm_temp5 = _mm_##INSTR1(_mm_add_epi32(XMM_OFST, _mm_mullo_epi32(XMM5, _mm_load_si128((__m128i *)(MEM + OFST1)))), _mm_mullo_epi32(XMM6, _mm_load_si128((__m128i *)(MEM + OFST2))));\
    xmm_temp7 = _mm_##INSTR2(_mm_mullo_epi32(XMM7, _mm_load_si128((__m128i *)(MEM + OFST3))), _mm_mullo_epi32(XMM8, _mm_load_si128((__m128i *)(MEM + OFST4))));\
    xmm_temp5 = _mm_##INSTR3(xmm_temp5, xmm_temp7);\
    xmm_temp5 = _mm_srai_epi32(xmm_temp5, SHIFT);\
    xmm_temp1 = _mm_packs_epi32(xmm_temp1, xmm_temp5);\
    _mm_storeu_si128((__m128i *)(transformCoefficients+OFST5), xmm_temp1);

    __m128i xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res04_hi, xmm_res0246_hi, xmm_res0145_hi, xmm_res_1to7_hi;
    __m128i xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res46_hi, xmm_res0123_hi, xmm_res4567_hi;
    __m128i xmm_trans0, xmm_trans1, xmm_trans2, xmm_trans3;
    __m128i xmm_trans0_lo/*, xmm_trans4_lo, xmm_trans0_hi, xmm_trans4_hi*/;
    __m128i xmm_even0, xmm_even1, xmm_even2, xmm_even3, xmm_odd0, xmm_odd1, xmm_odd2, xmm_odd3, xmm_evenEven0, xmm_evenEven1, xmm_evenOdd0, xmm_evenOdd1;
    __m128i xmm_even0L, xmm_even1L, xmm_even2L, xmm_even3L, xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_evenEven0L, xmm_evenEven1L, xmm_evenOdd0L, xmm_evenOdd1L;
    __m128i xmm_offset, xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi;
    __m128i xmm_trans01_hi, xmm_trans23_hi, /*xmm_trans45_hi,*/ /*xmm_trans67_hi, */xmm_trans02_hi, xmm_trans01234_hi/*, xmm_trans46_hi, xmm_trans4567_hi*/;
    __m128i xmm_res0L, xmm_res1L, xmm_res2L, xmm_res3L, xmm_res4L, xmm_res5L, xmm_res6L, xmm_res7L;
    __m128i xmm_temp1, xmm_temp3/*, xmm_temp5, xmm_temp7*/;

    __m128i xmm_junk = _mm_setzero_si128(); //set to zero to avoid valgrind issue

    xmm_res0 = _mm_loadu_si128((__m128i *)(residual));
    xmm_res1 = _mm_loadu_si128((__m128i *)(residual + srcStride));
    xmm_res2 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
    xmm_res3 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));
    residual += 4 * srcStride;
    xmm_res4 = _mm_loadu_si128((__m128i *)(residual));
    xmm_res5 = _mm_loadu_si128((__m128i *)(residual + srcStride));
    xmm_res6 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
    xmm_res7 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));

    MACRO_UNPACK(16, xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res4, xmm_res5, xmm_res6, xmm_res7, xmm_res01_hi, xmm_res23_hi, xmm_res45_hi, xmm_res67_hi)
        MACRO_UNPACK(32, xmm_res0, xmm_res2, xmm_res01_hi, xmm_res23_hi, xmm_res4, xmm_res6, xmm_res45_hi, xmm_res67_hi, xmm_res02_hi, xmm_res0123_hi, xmm_res46_hi, xmm_res4567_hi)
        MACRO_UNPACK(64, xmm_res0, xmm_res4, xmm_res02_hi, xmm_res46_hi, xmm_res01_hi, xmm_res45_hi, xmm_res0123_hi, xmm_res4567_hi, xmm_res04_hi, xmm_res0246_hi, xmm_res0145_hi, xmm_res_1to7_hi)

        MACRO_CALC_EVEN_ODD(xmm_res0, xmm_res04_hi, xmm_res02_hi, xmm_res0246_hi, xmm_res01_hi, xmm_res0145_hi, xmm_res0123_hi, xmm_res_1to7_hi)

        // TransformCoefficients 0, 2, 4, 6
        xmm_evenEven0 = _mm_add_epi16(xmm_even0, xmm_even3);
    xmm_evenEven1 = _mm_add_epi16(xmm_even1, xmm_even2);
    xmm_evenOdd0 = _mm_sub_epi16(xmm_even0, xmm_even3);
    xmm_evenOdd1 = _mm_sub_epi16(xmm_even1, xmm_even2);

    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_2));

    xmm_trans0 = _mm_slli_epi16(_mm_add_epi16(xmm_evenEven0, xmm_evenEven1), 4);
    //xmm_trans4 = _mm_slli_epi16(_mm_sub_epi16(xmm_evenEven0, xmm_evenEven1), 4);

    xmm_trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83_36)), _mm_unpacklo_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2),
        _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83_36)), _mm_unpackhi_epi16(xmm_evenOdd0, xmm_evenOdd1)), xmm_offset), 2));

    // TransformCoefficients 1, 3, 5, 7
    xmm_odd01_lo = _mm_unpacklo_epi16(xmm_odd0, xmm_odd1);
    xmm_odd01_hi = _mm_unpackhi_epi16(xmm_odd0, xmm_odd1);
    xmm_odd23_lo = _mm_unpacklo_epi16(xmm_odd2, xmm_odd3);
    xmm_odd23_hi = _mm_unpackhi_epi16(xmm_odd2, xmm_odd3);

    MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans1, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_89_75, OFFSET_50_18, 2)
        MACRO_TRANS_4MAC_NO_SAVE(xmm_odd01_lo, xmm_odd01_hi, xmm_odd23_lo, xmm_odd23_hi, xmm_trans3, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_75_N18, OFFSET_N89_N50, 2)


#define MACRO_UNPACK_PF(ARG1, ARG2, ARG3, ARG4, ARG5, ARG10, ARG11)\
    ARG10 = _mm_unpackhi_epi##ARG1(ARG2, ARG3);\
    ARG2  = _mm_unpacklo_epi##ARG1(ARG2, ARG3);\
    ARG11 = _mm_unpackhi_epi##ARG1(ARG4, ARG5);\
    ARG4  = _mm_unpacklo_epi##ARG1(ARG4, ARG5);\
    /*ARG12 = _mm_unpackhi_epi##ARG1(ARG6, ARG7);*/\
    /*ARG6  = _mm_unpacklo_epi##ARG1(ARG6, ARG7);*/\
    /*ARG13 = _mm_unpackhi_epi##ARG1(ARG8, ARG9);*/\
    /*ARG8  = _mm_unpacklo_epi##ARG1(ARG8, ARG9);*/

        MACRO_UNPACK_PF(16, xmm_trans0, xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans01_hi, xmm_trans23_hi)
        MACRO_UNPACK_PF(32, xmm_trans0, xmm_trans2, xmm_trans01_hi, xmm_trans23_hi, xmm_trans02_hi, xmm_trans01234_hi)



        //---- Second Partial Butterfly ----

        // Calculate low and high
        xmm_res0L = _mm_unpacklo_epi16(xmm_trans0, _mm_srai_epi16(xmm_trans0, 15));
    xmm_res1L = _mm_unpackhi_epi16(xmm_trans0, _mm_srai_epi16(xmm_trans0, 15));
    xmm_res2L = _mm_unpacklo_epi16(xmm_trans02_hi, _mm_srai_epi16(xmm_trans02_hi, 15));
    xmm_res3L = _mm_unpackhi_epi16(xmm_trans02_hi, _mm_srai_epi16(xmm_trans02_hi, 15));
    xmm_res4L = _mm_unpacklo_epi16(xmm_trans01_hi, _mm_srai_epi16(xmm_trans01_hi, 15));
    xmm_res5L = _mm_unpackhi_epi16(xmm_trans01_hi, _mm_srai_epi16(xmm_trans01_hi, 15));
    xmm_res6L = _mm_unpacklo_epi16(xmm_trans01234_hi, _mm_srai_epi16(xmm_trans01234_hi, 15));
    xmm_res7L = _mm_unpackhi_epi16(xmm_trans01234_hi, _mm_srai_epi16(xmm_trans01234_hi, 15));

    xmm_even0L = _mm_add_epi32(xmm_res0L, xmm_res7L);
    xmm_odd0L = _mm_sub_epi32(xmm_res0L, xmm_res7L);
    xmm_even1L = _mm_add_epi32(xmm_res1L, xmm_res6L);
    xmm_odd1L = _mm_sub_epi32(xmm_res1L, xmm_res6L);
    xmm_even2L = _mm_add_epi32(xmm_res2L, xmm_res5L);
    xmm_odd2L = _mm_sub_epi32(xmm_res2L, xmm_res5L);
    xmm_even3L = _mm_add_epi32(xmm_res3L, xmm_res4L);
    xmm_odd3L = _mm_sub_epi32(xmm_res3L, xmm_res4L);

    // TransformCoefficients 0, 4
    xmm_evenEven0L = _mm_add_epi32(xmm_even0L, xmm_even3L);
    xmm_evenEven1L = _mm_add_epi32(xmm_even1L, xmm_even2L);
    xmm_evenOdd0L = _mm_sub_epi32(xmm_even0L, xmm_even3L);
    xmm_evenOdd1L = _mm_sub_epi32(xmm_even1L, xmm_even2L);

    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_4));

    xmm_trans0_lo = _mm_srai_epi32(_mm_add_epi32(xmm_offset, _mm_add_epi32(xmm_evenEven0L, xmm_evenEven1L)), 3);
    xmm_trans0_lo = _mm_packs_epi32(xmm_trans0_lo, xmm_trans0_lo);

    _mm_storel_epi64((__m128i *)(transformCoefficients), xmm_trans0_lo);

    // TransformCoefficients 2, 6
    xmm_offset = _mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_256));

    xmm_trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_83)), xmm_evenOdd0L),
        _mm_mullo_epi32(_mm_load_si128((__m128i *)(TransformAsmConst_SSE4_1 + OFFSET_36)), xmm_evenOdd1L)),
        xmm_offset), 9),

        xmm_junk);

    _mm_storeu_si128((__m128i *)(transformCoefficients + 2 * dstStride), xmm_trans2);

    MACRO_TRANS_4MAC_PMULLD_PF(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_junk, xmm_junk, xmm_junk, xmm_junk, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_89, OFFSET_75, OFFSET_50, OFFSET_18, add_epi32, add_epi32, add_epi32, 9, dstStride)
        MACRO_TRANS_4MAC_PMULLD_PF(xmm_odd0L, xmm_odd1L, xmm_odd2L, xmm_odd3L, xmm_junk, xmm_junk, xmm_junk, xmm_junk, xmm_offset, TransformAsmConst_SSE4_1, OFFSET_75, OFFSET_18, OFFSET_89, OFFSET_50, sub_epi32, add_epi32, sub_epi32, 9, 3 * dstStride)

        (void)bitIncrement;
    (void)transformInnerArrayPtr;

}
