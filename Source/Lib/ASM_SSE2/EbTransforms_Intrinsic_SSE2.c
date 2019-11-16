/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbTransforms_SSE2.h"
#include "EbIntrinMacros16bit_SSE2.h"
#include <emmintrin.h>

/*****************************
* Defines
*****************************/

#define MACRO_TRANS_2MAC_NO_SAVE(XMM_1, XMM_2, XMM_3, XMM_4, XMM_OFFSET, OFFSET1, OFFSET2, SHIFT)\
    XMM_3 = _mm_load_si128((__m128i *)(TransformAsmConst + OFFSET1));\
    XMM_4 = _mm_load_si128((__m128i *)(TransformAsmConst + OFFSET2));\
    XMM_3 = _mm_madd_epi16(XMM_3, XMM_1);\
    XMM_4 = _mm_madd_epi16(XMM_4, XMM_2);\
    XMM_3 = _mm_srai_epi32(_mm_add_epi32(XMM_4, _mm_add_epi32(XMM_3, XMM_OFFSET)), SHIFT);\
    XMM_3 = _mm_packs_epi32(XMM_3, XMM_3);

#define MACRO_TRANS_2MAC(XMM_1, XMM_2, XMM_3, XMM_4, XMM_OFFSET, OFFSET1, OFFSET2, SHIFT, OFFSET3)\
    MACRO_TRANS_2MAC_NO_SAVE(XMM_1, XMM_2, XMM_3, XMM_4, XMM_OFFSET, OFFSET1, OFFSET2, SHIFT)\
    _mm_storel_epi64((__m128i *)(transformCoefficients+OFFSET3), XMM_3);

#define TRANS8x8_OFFSET_83_36    0
#define TRANS8x8_OFFSET_36_N83  (8 + TRANS8x8_OFFSET_83_36)
#define TRANS8x8_OFFSET_89_75   (8 + TRANS8x8_OFFSET_36_N83)
#define TRANS8x8_OFFSET_50_18   (8 + TRANS8x8_OFFSET_89_75)
#define TRANS8x8_OFFSET_75_N18  (8 + TRANS8x8_OFFSET_50_18)
#define TRANS8x8_OFFSET_N89_N50 (8 + TRANS8x8_OFFSET_75_N18)
#define TRANS8x8_OFFSET_50_N89  (8 + TRANS8x8_OFFSET_N89_N50)
#define TRANS8x8_OFFSET_18_75   (8 + TRANS8x8_OFFSET_50_N89)
#define TRANS8x8_OFFSET_18_N50  (8 + TRANS8x8_OFFSET_18_75)
#define TRANS8x8_OFFSET_75_N89  (8 + TRANS8x8_OFFSET_18_N50)
#define TRANS8x8_OFFSET_256     (8 + TRANS8x8_OFFSET_75_N89)
#define TRANS8x8_OFFSET_64_64   (8 + TRANS8x8_OFFSET_256)
#define TRANS8x8_OFFSET_N18_N50 (8 + TRANS8x8_OFFSET_64_64)
#define TRANS8x8_OFFSET_N75_N89 (8 + TRANS8x8_OFFSET_N18_N50)
#define TRANS8x8_OFFSET_N36_N83 (8 + TRANS8x8_OFFSET_N75_N89)
#define TRANS8x8_OFFSET_N83_N36 (8 + TRANS8x8_OFFSET_N36_N83)
#define TRANS8x8_OFFSET_36_83   (8 + TRANS8x8_OFFSET_N83_N36)
#define TRANS8x8_OFFSET_50_89   (8 + TRANS8x8_OFFSET_36_83)
#define TRANS8x8_OFFSET_18_N75  (8 + TRANS8x8_OFFSET_50_89)
#define TRANS8x8_OFFSET_N64_64  (8 + TRANS8x8_OFFSET_18_N75)
#define TRANS8x8_OFFSET_64_N64  (8 + TRANS8x8_OFFSET_N64_64)
#define TRANS8x8_OFFSET_N75_N18 (8 + TRANS8x8_OFFSET_64_N64)
#define TRANS8x8_OFFSET_89_N50  (8 + TRANS8x8_OFFSET_N75_N18)
#define TRANS8x8_OFFSET_83_N36  (8 + TRANS8x8_OFFSET_89_N50)
#define TRANS8x8_OFFSET_N36_83  (8 + TRANS8x8_OFFSET_83_N36)
#define TRANS8x8_OFFSET_N83_36  (8 + TRANS8x8_OFFSET_N36_83)
#define TRANS8x8_OFFSET_89_N75  (8 + TRANS8x8_OFFSET_N83_36)
#define TRANS8x8_OFFSET_50_N18  (8 + TRANS8x8_OFFSET_89_N75)

#define MACRO_CALC_EVEN_ODD(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8)\
    even0 = _mm_add_epi16(XMM1, XMM8);\
    even1 = _mm_add_epi16(XMM2, XMM7);\
    even2 = _mm_add_epi16(XMM3, XMM6);\
    even3 = _mm_add_epi16(XMM4, XMM5);\
    odd0 = _mm_sub_epi16(XMM1, XMM8);\
    odd1 = _mm_sub_epi16(XMM2, XMM7);\
    odd2 = _mm_sub_epi16(XMM3, XMM6);\
    odd3 = _mm_sub_epi16(XMM4, XMM5);

#define MACRO_TRANS_4MAC_NO_SAVE(XMM1, XMM2, XMM3, XMM4, XMM_RET, XMM_OFFSET, MEM, OFFSET1, OFFSET2, SHIFT)\
    XMM_RET = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM1, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM3, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT),\
                              _mm_srai_epi32(_mm_add_epi32(_mm_add_epi32(_mm_madd_epi16(XMM2, _mm_load_si128((__m128i*)(MEM+OFFSET1))),\
                                                                         _mm_madd_epi16(XMM4, _mm_load_si128((__m128i*)(MEM+OFFSET2)))), XMM_OFFSET), SHIFT));

#define MACRO_TRANS_8MAC(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, SHIFT, INSTR, DST, OFST5)\
    sum1 = _mm_add_epi32(_mm_madd_epi16(XMM1, _mm_loadu_si128((__m128i *)(MEM + OFST1))), _mm_madd_epi16(XMM2, _mm_loadu_si128((__m128i *)(MEM + OFST2))));\
    sum2 = _mm_add_epi32(_mm_madd_epi16(XMM3, _mm_loadu_si128((__m128i *)(MEM + OFST3))), _mm_madd_epi16(XMM4, _mm_loadu_si128((__m128i *)(MEM + OFST4))));\
    sum1 = _mm_srai_epi32(_mm_add_epi32(XMM_OFST, _mm_add_epi32(sum1, sum2)), SHIFT);\
    sum3 = _mm_add_epi32(_mm_madd_epi16(XMM5, _mm_loadu_si128((__m128i *)(MEM + OFST1))), _mm_madd_epi16(XMM6, _mm_loadu_si128((__m128i *)(MEM + OFST2))));\
    sum4 = _mm_add_epi32(_mm_madd_epi16(XMM7, _mm_loadu_si128((__m128i *)(MEM + OFST3))), _mm_madd_epi16(XMM8, _mm_loadu_si128((__m128i *)(MEM + OFST4))));\
    sum3 = _mm_srai_epi32(_mm_add_epi32(XMM_OFST, _mm_add_epi32(sum3, sum4)), SHIFT);\
    sum = _mm_packs_epi32(sum1, sum3);\
    INSTR((__m128i *)(DST + OFST5), sum);

#define MACRO_TRANS_8MAC_PF_N2(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, SHIFT, INSTR, DST, OFST5)\
    sum1 = _mm_add_epi32(_mm_madd_epi16(XMM1, _mm_loadu_si128((__m128i *)(MEM + OFST1))), _mm_madd_epi16(XMM2, _mm_loadu_si128((__m128i *)(MEM + OFST2))));\
    sum2 = _mm_add_epi32(_mm_madd_epi16(XMM3, _mm_loadu_si128((__m128i *)(MEM + OFST3))), _mm_madd_epi16(XMM4, _mm_loadu_si128((__m128i *)(MEM + OFST4))));\
    sum1 = _mm_srai_epi32(_mm_add_epi32(XMM_OFST, _mm_add_epi32(sum1, sum2)), SHIFT);\
    /*sum3 = _mm_add_epi32(_mm_madd_epi16(XMM5, _mm_loadu_si128((__m128i *)(MEM + OFST1))), _mm_madd_epi16(XMM6, _mm_loadu_si128((__m128i *)(MEM + OFST2))));*/\
    /*sum4 = _mm_add_epi32(_mm_madd_epi16(XMM7, _mm_loadu_si128((__m128i *)(MEM + OFST3))), _mm_madd_epi16(XMM8, _mm_loadu_si128((__m128i *)(MEM + OFST4))));*/\
    /*sum3 = _mm_srai_epi32(_mm_add_epi32(XMM_OFST, _mm_add_epi32(sum3, sum4)), SHIFT);*/\
    /*sum = _mm_packs_epi32(sum1, sum3);*/\
	sum = _mm_packs_epi32(sum1, sum1);\
    INSTR((__m128i *)(DST + OFST5), sum);
#define MACRO_TRANS_8MAC_PF_N4(XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM_OFST, MEM, OFST1, OFST2, OFST3, OFST4, SHIFT, INSTR, DST, OFST5)\
    sum1 = _mm_add_epi32(_mm_madd_epi16(XMM1, _mm_loadu_si128((__m128i *)(MEM + OFST1))), _mm_madd_epi16(XMM2, _mm_loadu_si128((__m128i *)(MEM + OFST2))));\
    sum2 = _mm_add_epi32(_mm_madd_epi16(XMM3, _mm_loadu_si128((__m128i *)(MEM + OFST3))), _mm_madd_epi16(XMM4, _mm_loadu_si128((__m128i *)(MEM + OFST4))));\
    sum1 = _mm_srai_epi32(_mm_add_epi32(XMM_OFST, _mm_add_epi32(sum1, sum2)), SHIFT);\
    /*sum3 = _mm_add_epi32(_mm_madd_epi16(XMM5, _mm_loadu_si128((__m128i *)(MEM + OFST1))), _mm_madd_epi16(XMM6, _mm_loadu_si128((__m128i *)(MEM + OFST2))));*/\
    /*sum4 = _mm_add_epi32(_mm_madd_epi16(XMM7, _mm_loadu_si128((__m128i *)(MEM + OFST3))), _mm_madd_epi16(XMM8, _mm_loadu_si128((__m128i *)(MEM + OFST4))));*/\
    /*sum3 = _mm_srai_epi32(_mm_add_epi32(XMM_OFST, _mm_add_epi32(sum3, sum4)), SHIFT);*/\
    /*sum = _mm_packs_epi32(sum1, sum3);*/\
	sum = _mm_packs_epi32(sum1, sum1);\
    INSTR((__m128i *)(DST + OFST5), sum);

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
EB_ALIGN(16) const EB_S16 DstTransformAsmConst_SSE2[] = {
    1, 0, 1, 0, 1, 0, 1, 0,
    29, 55, 29, 55, 29, 55, 29, 55,
    74, 84, 74, 84, 74, 84, 74, 84,
    84, -29, 84, -29, 84, -29, 84, -29,
    -74, 55, -74, 55, -74, 55, -74, 55,
    55, -84, 55, -84, 55, -84, 55, -84,
    74, -29, 74, -29, 74, -29, 74, -29,
    37, 37, 37, 37, 37, 37, 37, 37,
    74, 74, 74, 74, 74, 74, 74, 74,
    0, -37, 0, -37, 0, -37, 0, -37,
    0, -74, 0, -74, 0, -74, 0, -74,
    //74,    0,   74,    0,   74,    0,   74,    0,
    //55,  -29,   55,  -29,   55,  -29,   55,  -29,
};

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
EB_ALIGN(16) const EB_S16 InvTransformAsmConst_SSE2[] = {
    2, 0, 2, 0, 2, 0, 2, 0,
    4, 0, 4, 0, 4, 0, 4, 0,
    8, 0, 8, 0, 8, 0, 8, 0,
    9, 0, 9, 0, 9, 0, 9, 0,
    64, 0, 64, 0, 64, 0, 64, 0,
    256, 0, 256, 0, 256, 0, 256, 0,
    512, 0, 512, 0, 512, 0, 512, 0,
    1024, 0, 1024, 0, 1024, 0, 1024, 0,
    2048, 0, 2048, 0, 2048, 0, 2048, 0,
    7, 0, 0, 0, 0, 0, 0, 0,
    12, 0, 0, 0, 0, 0, 0, 0,
    64, 64, 64, 64, 64, 64, 64, 64,
    90, 57, 90, 57, 90, 57, 90, 57,
    89, 50, 89, 50, 89, 50, 89, 50,
    87, 43, 87, 43, 87, 43, 87, 43,
    83, 36, 83, 36, 83, 36, 83, 36,
    80, 25, 80, 25, 80, 25, 80, 25,
    75, 18, 75, 18, 75, 18, 75, 18,
    70, 9, 70, 9, 70, 9, 70, 9,
    64, -64, 64, -64, 64, -64, 64, -64,
    87, -80, 87, -80, 87, -80, 87, -80,
    75, -89, 75, -89, 75, -89, 75, -89,
    57, -90, 57, -90, 57, -90, 57, -90,
    36, -83, 36, -83, 36, -83, 36, -83,
    9, -70, 9, -70, 9, -70, 9, -70,
    -18, -50, -18, -50, -18, -50, -18, -50,
    -43, -25, -43, -25, -43, -25, -43, -25,
    80, -25, 80, -25, 80, -25, 80, -25,
    50, 18, 50, 18, 50, 18, 50, 18,
    9, 57, 9, 57, 9, 57, 9, 57,
    -36, 83, -36, 83, -36, 83, -36, 83,
    -70, 90, -70, 90, -70, 90, -70, 90,
    -89, 75, -89, 75, -89, 75, -89, 75,
    -87, 43, -87, 43, -87, 43, -87, 43,
    70, 90, 70, 90, 70, 90, 70, 90,
    18, 75, 18, 75, 18, 75, 18, 75,
    -43, 25, -43, 25, -43, 25, -43, 25,
    -83, -36, -83, -36, -83, -36, -83, -36,
    -87, -80, -87, -80, -87, -80, -87, -80,
    -50, -89, -50, -89, -50, -89, -50, -89,
    9, -57, 9, -57, 9, -57, 9, -57,
    57, -9, 57, -9, 57, -9, 57, -9,
    -18, -75, -18, -75, -18, -75, -18, -75,
    -80, -87, -80, -87, -80, -87, -80, -87,
    -25, 43, -25, 43, -25, 43, -25, 43,
    50, 89, 50, 89, 50, 89, 50, 89,
    90, 70, 90, 70, 90, 70, 90, 70,
    43, -87, 43, -87, 43, -87, 43, -87,
    -50, -18, -50, -18, -50, -18, -50, -18,
    -90, 70, -90, 70, -90, 70, -90, 70,
    57, 9, 57, 9, 57, 9, 57, 9,
    89, -75, 89, -75, 89, -75, 89, -75,
    25, -80, 25, -80, 25, -80, 25, -80,
    25, 43, 25, 43, 25, 43, 25, 43,
    -75, 89, -75, 89, -75, 89, -75, 89,
    -70, 9, -70, 9, -70, 9, -70, 9,
    90, -57, 90, -57, 90, -57, 90, -57,
    18, 50, 18, 50, 18, 50, 18, 50,
    -80, 87, -80, 87, -80, 87, -80, 87,
    9, 70, 9, 70, 9, 70, 9, 70,
    -89, -50, -89, -50, -89, -50, -89, -50,
    -25, -80, -25, -80, -25, -80, -25, -80,
    43, 87, 43, 87, 43, 87, 43, 87,
    -75, -18, -75, -18, -75, -18, -75, -18,
    -57, -90, -57, -90, -57, -90, -57, -90,
    -9, -70, -9, -70, -9, -70, -9, -70,
    25, 80, 25, 80, 25, 80, 25, 80,
    -43, -87, -43, -87, -43, -87, -43, -87,
    57, 90, 57, 90, 57, 90, 57, 90,
    -25, -43, -25, -43, -25, -43, -25, -43,
    70, -9, 70, -9, 70, -9, 70, -9,
    -90, 57, -90, 57, -90, 57, -90, 57,
    80, -87, 80, -87, 80, -87, 80, -87,
    -43, 87, -43, 87, -43, 87, -43, 87,
    90, -70, 90, -70, 90, -70, 90, -70,
    -57, -9, -57, -9, -57, -9, -57, -9,
    -25, 80, -25, 80, -25, 80, -25, 80,
    -57, 9, -57, 9, -57, 9, -57, 9,
    80, 87, 80, 87, 80, 87, 80, 87,
    25, -43, 25, -43, 25, -43, 25, -43,
    -90, -70, -90, -70, -90, -70, -90, -70,
    -70, -90, -70, -90, -70, -90, -70, -90,
    43, -25, 43, -25, 43, -25, 43, -25,
    87, 80, 87, 80, 87, 80, 87, 80,
    -9, 57, -9, 57, -9, 57, -9, 57,
    -80, 25, -80, 25, -80, 25, -80, 25,
    -9, -57, -9, -57, -9, -57, -9, -57,
    70, -90, 70, -90, 70, -90, 70, -90,
    87, -43, 87, -43, 87, -43, 87, -43,
    -87, 80, -87, 80, -87, 80, -87, 80,
    -57, 90, -57, 90, -57, 90, -57, 90,
    -9, 70, -9, 70, -9, 70, -9, 70,
    43, 25, 43, 25, 43, 25, 43, 25,
    -90, -57, -90, -57, -90, -57, -90, -57,
    -87, -43, -87, -43, -87, -43, -87, -43,
    -80, -25, -80, -25, -80, -25, -80, -25,
    -70, -9, -70, -9, -70, -9, -70, -9,
    90, 61, 90, 61, 90, 61, 90, 61,
    90, 54, 90, 54, 90, 54, 90, 54,
    88, 46, 88, 46, 88, 46, 88, 46,
    85, 38, 85, 38, 85, 38, 85, 38,
    82, 31, 82, 31, 82, 31, 82, 31,
    78, 22, 78, 22, 78, 22, 78, 22,
    73, 13, 73, 13, 73, 13, 73, 13,
    67, 4, 67, 4, 67, 4, 67, 4,
    90, -73, 90, -73, 90, -73, 90, -73,
    82, -85, 82, -85, 82, -85, 82, -85,
    67, -90, 67, -90, 67, -90, 67, -90,
    46, -88, 46, -88, 46, -88, 46, -88,
    22, -78, 22, -78, 22, -78, 22, -78,
    -4, -61, -4, -61, -4, -61, -4, -61,
    -31, -38, -31, -38, -31, -38, -31, -38,
    -54, -13, -54, -13, -54, -13, -54, -13,
    88, -46, 88, -46, 88, -46, 88, -46,
    67, -4, 67, -4, 67, -4, 67, -4,
    31, 38, 31, 38, 31, 38, 31, 38,
    -13, 73, -13, 73, -13, 73, -13, 73,
    -54, 90, -54, 90, -54, 90, -54, 90,
    -82, 85, -82, 85, -82, 85, -82, 85,
    -90, 61, -90, 61, -90, 61, -90, 61,
    -78, 22, -78, 22, -78, 22, -78, 22,
    85, 82, 85, 82, 85, 82, 85, 82,
    46, 88, 46, 88, 46, 88, 46, 88,
    -13, 54, -13, 54, -13, 54, -13, 54,
    -67, -4, -67, -4, -67, -4, -67, -4,
    -90, -61, -90, -61, -90, -61, -90, -61,
    -73, -90, -73, -90, -73, -90, -73, -90,
    -22, -78, -22, -78, -22, -78, -22, -78,
    38, -31, 38, -31, 38, -31, 38, -31,
    22, -46, 22, -46, 22, -46, 22, -46,
    -54, -90, -54, -90, -54, -90, -54, -90,
    -90, -67, -90, -67, -90, -67, -90, -67,
    -61, 4, -61, 4, -61, 4, -61, 4,
    13, 73, 13, 73, 13, 73, 13, 73,
    78, 88, 78, 88, 78, 88, 78, 88,
    78, -88, 78, -88, 78, -88, 78, -88,
    -82, 31, -82, 31, -82, 31, -82, 31,
    -73, 90, -73, 90, -73, 90, -73, 90,
    13, 54, 13, 54, 13, 54, 13, 54,
    85, -38, 85, -38, 85, -38, 85, -38,
    -22, -46, -22, -46, -22, -46, -22, -46,
    73, -13, 73, -13, 73, -13, 73, -13,
    -31, 82, -31, 82, -31, 82, -31, 82,
    -38, 85, -38, 85, -38, 85, -38, 85,
    -90, 54, -90, 54, -90, 54, -90, 54,
    67, 90, 67, 90, 67, 90, 67, 90,
    -54, 13, -54, 13, -54, 13, -54, 13,
    -78, -88, -78, -88, -78, -88, -78, -88,
    -22, 46, -22, 46, -22, 46, -22, 46,
    -90, -73, -90, -73, -90, -73, -90, -73,
    4, -61, 4, -61, 4, -61, 4, -61,
    61, -4, 61, -4, 61, -4, 61, -4,
    -46, 22, -46, 22, -46, 22, -46, 22,
    82, 85, 82, 85, 82, 85, 82, 85,
    31, -38, 31, -38, 31, -38, 31, -38,
    -88, -78, -88, -78, -88, -78, -88, -78,
    90, 67, 90, 67, 90, 67, 90, 67,
    54, -90, 54, -90, 54, -90, 54, -90,
    -85, 38, -85, 38, -85, 38, -85, 38,
    -4, 67, -4, 67, -4, 67, -4, 67,
    88, -78, 88, -78, 88, -78, 88, -78,
    -46, -22, -46, -22, -46, -22, -46, -22,
    -61, 90, -61, 90, -61, 90, -61, 90,
    82, -31, 82, -31, 82, -31, 82, -31,
    13, -73, 13, -73, 13, -73, 13, -73,
    46, 22, 46, 22, 46, 22, 46, 22,
    -90, 67, -90, 67, -90, 67, -90, 67,
    38, -85, 38, -85, 38, -85, 38, -85,
    54, 13, 54, 13, 54, 13, 54, 13,
    -90, 73, -90, 73, -90, 73, -90, 73,
    31, -82, 31, -82, 31, -82, 31, -82,
    61, 4, 61, 4, 61, 4, 61, 4,
    -88, 78, -88, 78, -88, 78, -88, 78,
    38, 85, 38, 85, 38, 85, 38, 85,
    -4, 61, -4, 61, -4, 61, -4, 61,
    -67, -90, -67, -90, -67, -90, -67, -90,
    -31, -82, -31, -82, -31, -82, -31, -82,
    -78, -22, -78, -22, -78, -22, -78, -22,
    90, 73, 90, 73, 90, 73, 90, 73,
    -61, -90, -61, -90, -61, -90, -61, -90,
    4, 67, 4, 67, 4, 67, 4, 67,
    54, -13, 54, -13, 54, -13, 54, -13,
    -88, -46, -88, -46, -88, -46, -88, -46,
    85, -82, 85, -82, 85, -82, 85, -82,
    -38, -31, -38, -31, -38, -31, -38, -31,
    -13, -73, -13, -73, -13, -73, -13, -73,
    22, 78, 22, 78, 22, 78, 22, 78,
    -46, -88, -46, -88, -46, -88, -46, -88,
    54, 90, 54, 90, 54, 90, 54, 90
};

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
EB_ALIGN(16) const EB_S16 InvDstTransformAsmConst_SSE2[] = {
    64, 0, 64, 0, 64, 0, 64, 0,
    29, 84, 29, 84, 29, 84, 29, 84,
    74, 55, 74, 55, 74, 55, 74, 55,
    55, -29, 55, -29, 55, -29, 55, -29,
    74, -84, 74, -84, 74, -84, 74, -84,
    74, -74, 74, -74, 74, -74, 74, -74,
    0, 74, 0, 74, 0, 74, 0, 74,
    84, 55, 84, 55, 84, 55, 84, 55,
    -74, -29, -74, -29, -74, -29, -74, -29,
};


// Coefficients for inverse 32-point transform
EB_EXTERN const EB_S16 coeff_tbl2[48 * 8] =
{
    64, 89, 64, 75, 64, 50, 64, 18, 64, -18, 64, -50, 64, -75, 64, -89,
    83, 75, 36, -18, -36, -89, -83, -50, -83, 50, -36, 89, 36, 18, 83, -75,
    64, 50, -64, -89, -64, 18, 64, 75, 64, -75, -64, -18, -64, 89, 64, -50,
    36, 18, -83, -50, 83, 75, -36, -89, -36, 89, 83, -75, -83, 50, 36, -18,
    90, 87, 87, 57, 80, 9, 70, -43, 57, -80, 43, -90, 25, -70, 9, -25,
    80, 70, 9, -43, -70, -87, -87, 9, -25, 90, 57, 25, 90, -80, 43, -57,
    57, 43, -80, -90, -25, 57, 90, 25, -9, -87, -87, 70, 43, 9, 70, -80,
    25, 9, -70, -25, 90, 43, -80, -57, 43, 70, 9, -80, -57, 87, 87, -90,
    90, 90, 90, 82, 88, 67, 85, 46, 82, 22, 78, -4, 73, -31, 67, -54,
    61, -73, 54, -85, 46, -90, 38, -88, 31, -78, 22, -61, 13, -38, 4, -13,
    88, 85, 67, 46, 31, -13, -13, -67, -54, -90, -82, -73, -90, -22, -78, 38,
    -46, 82, -4, 88, 38, 54, 73, -4, 90, -61, 85, -90, 61, -78, 22, -31,
    82, 78, 22, -4, -54, -82, -90, -73, -61, 13, 13, 85, 78, 67, 85, -22,
    31, -88, -46, -61, -90, 31, -67, 90, 4, 54, 73, -38, 88, -90, 38, -46,
    73, 67, -31, -54, -90, -78, -22, 38, 78, 85, 67, -22, -38, -90, -90, 4,
    -13, 90, 82, 13, 61, -88, -46, -31, -88, 82, -4, 46, 85, -73, 54, -61,
    61, 54, -73, -85, -46, -4, 82, 88, 31, -46, -88, -61, -13, 82, 90, 13,
    -4, -90, -90, 38, 22, 67, 85, -78, -38, -22, -78, 90, 54, -31, 67, -73,
    46, 38, -90, -88, 38, 73, 54, -4, -90, -67, 31, 90, 61, -46, -88, -31,
    22, 85, 67, -78, -85, 13, 13, 61, 73, -90, -82, 54, 4, 22, 78, -82,
    31, 22, -78, -61, 90, 85, -61, -90, 4, 73, 54, -38, -88, -4, 82, 46,
    -38, -78, -22, 90, 73, -82, -90, 54, 67, -13, -13, -31, -46, 67, 85, -88,
    13, 4, -38, -13, 61, 22, -78, -31, 88, 38, -90, -46, 85, 54, -73, -61,
    54, 67, -31, -73, 4, 78, 22, -82, -46, 85, 67, -88, -82, 90, 90, -90
};

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
EB_EXTERN const EB_S16 coeff_tbl[48 * 8] =
{
    64, 64, 89, 75, 83, 36, 75, -18, 64, -64, 50, -89, 36, -83, 18, -50,
    64, 64, 50, 18, -36, -83, -89, -50, -64, 64, 18, 75, 83, -36, 75, -89,
    64, 64, -18, -50, -83, -36, 50, 89, 64, -64, -75, -18, -36, 83, 89, -75,
    64, 64, -75, -89, 36, 83, 18, -75, -64, 64, 89, -50, -83, 36, 50, -18,
    90, 87, 87, 57, 80, 9, 70, -43, 57, -80, 43, -90, 25, -70, 9, -25,
    80, 70, 9, -43, -70, -87, -87, 9, -25, 90, 57, 25, 90, -80, 43, -57,
    57, 43, -80, -90, -25, 57, 90, 25, -9, -87, -87, 70, 43, 9, 70, -80,
    25, 9, -70, -25, 90, 43, -80, -57, 43, 70, 9, -80, -57, 87, 87, -90,
    90, 90, 90, 82, 88, 67, 85, 46, 82, 22, 78, -4, 73, -31, 67, -54,
    61, -73, 54, -85, 46, -90, 38, -88, 31, -78, 22, -61, 13, -38, 4, -13,
    88, 85, 67, 46, 31, -13, -13, -67, -54, -90, -82, -73, -90, -22, -78, 38,
    -46, 82, -4, 88, 38, 54, 73, -4, 90, -61, 85, -90, 61, -78, 22, -31,
    82, 78, 22, -4, -54, -82, -90, -73, -61, 13, 13, 85, 78, 67, 85, -22,
    31, -88, -46, -61, -90, 31, -67, 90, 4, 54, 73, -38, 88, -90, 38, -46,
    73, 67, -31, -54, -90, -78, -22, 38, 78, 85, 67, -22, -38, -90, -90, 4,
    -13, 90, 82, 13, 61, -88, -46, -31, -88, 82, -4, 46, 85, -73, 54, -61,
    61, 54, -73, -85, -46, -4, 82, 88, 31, -46, -88, -61, -13, 82, 90, 13,
    -4, -90, -90, 38, 22, 67, 85, -78, -38, -22, -78, 90, 54, -31, 67, -73,
    46, 38, -90, -88, 38, 73, 54, -4, -90, -67, 31, 90, 61, -46, -88, -31,
    22, 85, 67, -78, -85, 13, 13, 61, 73, -90, -82, 54, 4, 22, 78, -82,
    31, 22, -78, -61, 90, 85, -61, -90, 4, 73, 54, -38, -88, -4, 82, 46,
    -38, -78, -22, 90, 73, -82, -90, 54, 67, -13, -13, -31, -46, 67, 85, -88,
    13, 4, -38, -13, 61, 22, -78, -31, 88, 38, -90, -46, 85, 54, -73, -61,
    54, 67, -31, -73, 4, 78, 22, -82, -46, 85, 67, -88, -82, 90, 90, -90
};

static __m128i reverse_epi16(__m128i x)
{
    x = _mm_shuffle_epi32(x, 0x1b); // 00011011
    x = _mm_shufflelo_epi16(x, 0xb1); // 10110001
    x = _mm_shufflehi_epi16(x, 0xb1);
    return x;
}

// 16-point forward transform (16 rows)
static void Transform16(short *src, int src_stride, short *dst, int dst_stride, int shift)
{
    int i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl;

    for (i = 0; i < 16; i++)
    {
        __m128i x0, x1;
        __m128i y0, y1;
        __m128i a0, a1, a2, a3;
        __m128i b0, b1, b2, b3;

        y0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
        y1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));


        // 16-point butterfly
        y1 = reverse_epi16(y1);

        x0 = _mm_add_epi16(y0, y1);
        x1 = _mm_sub_epi16(y0, y1);

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

        b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
        b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
        b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
        b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);

        x0 = _mm_packs_epi32(b0, b1);
        x1 = _mm_packs_epi32(b2, b3);

        y0 = _mm_unpacklo_epi16(x0, x1);
        y1 = _mm_unpackhi_epi16(x0, x1);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), y0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), y1);
    }
}

// 16-point inverse transform (16 rows)
static void InvTransform16(
    EB_S16  *src,
    EB_U32   src_stride,
    EB_S16  *dst,
    EB_U32   dst_stride,
    EB_U32   shift)
{
    int i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl2;

    for (i = 0; i < 16; i++)
    {
        __m128i x0, x1;
        __m128i y0, y1;
        __m128i a0, a1, a2, a3;
        __m128i b0, b1, b2, b3;
        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00)); // 00 01 02 03 04 05 06 07
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08)); // 08 09 0a 0b 0c 0d 0e 0f

        y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
        y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f

        x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
        x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f

        y0 = _mm_unpacklo_epi16(x0, x1); // 00 02 04 06 08 0a 0c 0e
        y1 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f

        x0 = y0;
        x1 = y1;

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));


        a0 = _mm_add_epi32(a0, o0);
        a1 = _mm_add_epi32(a1, o0);

        b0 = _mm_add_epi32(a0, a2);
        b1 = _mm_add_epi32(a1, a3);
        b2 = _mm_sub_epi32(a0, a2);
        b3 = _mm_sub_epi32(a1, a3);

        a0 = b0;
        a1 = b1;
        a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
        a3 = _mm_shuffle_epi32(b2, 0x1b);

        a0 = _mm_sra_epi32(a0, s0);
        a1 = _mm_sra_epi32(a1, s0);
        a2 = _mm_sra_epi32(a2, s0);
        a3 = _mm_sra_epi32(a3, s0);

        x0 = _mm_packs_epi32(a0, a1);
        x1 = _mm_packs_epi32(a2, a3);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
    }
}

// transpose 16x16 block of data
static void Transpose16(
    EB_S16     *src,
    EB_U32      src_stride,
    EB_S16     *dst,
    EB_U32      dst_stride)
{
    int i, j;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;

            a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
            a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
            a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
            a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
            a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
            a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
            a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
            a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

            b0 = _mm_unpacklo_epi16(a0, a4);
            b1 = _mm_unpacklo_epi16(a1, a5);
            b2 = _mm_unpacklo_epi16(a2, a6);
            b3 = _mm_unpacklo_epi16(a3, a7);
            b4 = _mm_unpackhi_epi16(a0, a4);
            b5 = _mm_unpackhi_epi16(a1, a5);
            b6 = _mm_unpackhi_epi16(a2, a6);
            b7 = _mm_unpackhi_epi16(a3, a7);

            a0 = _mm_unpacklo_epi16(b0, b2);
            a1 = _mm_unpacklo_epi16(b1, b3);
            a2 = _mm_unpackhi_epi16(b0, b2);
            a3 = _mm_unpackhi_epi16(b1, b3);
            a4 = _mm_unpacklo_epi16(b4, b6);
            a5 = _mm_unpacklo_epi16(b5, b7);
            a6 = _mm_unpackhi_epi16(b4, b6);
            a7 = _mm_unpackhi_epi16(b5, b7);

            b0 = _mm_unpacklo_epi16(a0, a1);
            b1 = _mm_unpackhi_epi16(a0, a1);
            b2 = _mm_unpacklo_epi16(a2, a3);
            b3 = _mm_unpackhi_epi16(a2, a3);
            b4 = _mm_unpacklo_epi16(a4, a5);
            b5 = _mm_unpackhi_epi16(a4, a5);
            b6 = _mm_unpacklo_epi16(a6, a7);
            b7 = _mm_unpackhi_epi16(a6, a7);

            _mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
        }
    }
}

static void PfreqTranspose32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride)
{
    EB_U32 i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 2; j++)
        {
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;

            a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
            a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
            a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
            a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
            a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
            a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
            a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
            a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

            b0 = _mm_unpacklo_epi16(a0, a4);
            b1 = _mm_unpacklo_epi16(a1, a5);
            b2 = _mm_unpacklo_epi16(a2, a6);
            b3 = _mm_unpacklo_epi16(a3, a7);
            b4 = _mm_unpackhi_epi16(a0, a4);
            b5 = _mm_unpackhi_epi16(a1, a5);
            b6 = _mm_unpackhi_epi16(a2, a6);
            b7 = _mm_unpackhi_epi16(a3, a7);

            a0 = _mm_unpacklo_epi16(b0, b2);
            a1 = _mm_unpacklo_epi16(b1, b3);
            a2 = _mm_unpackhi_epi16(b0, b2);
            a3 = _mm_unpackhi_epi16(b1, b3);
            a4 = _mm_unpacklo_epi16(b4, b6);
            a5 = _mm_unpacklo_epi16(b5, b7);
            a6 = _mm_unpackhi_epi16(b4, b6);
            a7 = _mm_unpackhi_epi16(b5, b7);

            b0 = _mm_unpacklo_epi16(a0, a1);
            b1 = _mm_unpackhi_epi16(a0, a1);
            b2 = _mm_unpacklo_epi16(a2, a3);
            b3 = _mm_unpackhi_epi16(a2, a3);
            b4 = _mm_unpacklo_epi16(a4, a5);
            b5 = _mm_unpackhi_epi16(a4, a5);
            b6 = _mm_unpacklo_epi16(a6, a7);
            b7 = _mm_unpackhi_epi16(a6, a7);

            _mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
        }
    }
}

static void PfreqN4SecTranspose32_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride)
{
	EB_U32 i, j;

	i = j = 0;
	{
		{
			__m128i a0, a1, a2, a3, a4, a5, a6, a7;
			__m128i b0, b1, b2, b3, b4, b5, b6, b7;

			a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
			a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
			a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
			a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
			a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
			a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
			a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
			a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

			b0 = _mm_unpacklo_epi16(a0, a4);
			b1 = _mm_unpacklo_epi16(a1, a5);
			b2 = _mm_unpacklo_epi16(a2, a6);
			b3 = _mm_unpacklo_epi16(a3, a7);
			b4 = _mm_unpackhi_epi16(a0, a4);
			b5 = _mm_unpackhi_epi16(a1, a5);
			b6 = _mm_unpackhi_epi16(a2, a6);
			b7 = _mm_unpackhi_epi16(a3, a7);

			a0 = _mm_unpacklo_epi16(b0, b2);
			a1 = _mm_unpacklo_epi16(b1, b3);
			a2 = _mm_unpackhi_epi16(b0, b2);
			a3 = _mm_unpackhi_epi16(b1, b3);
			a4 = _mm_unpacklo_epi16(b4, b6);
			a5 = _mm_unpacklo_epi16(b5, b7);
			a6 = _mm_unpackhi_epi16(b4, b6);
			a7 = _mm_unpackhi_epi16(b5, b7);

			b0 = _mm_unpacklo_epi16(a0, a1);
			b1 = _mm_unpackhi_epi16(a0, a1);
			b2 = _mm_unpacklo_epi16(a2, a3);
			b3 = _mm_unpackhi_epi16(a2, a3);
			b4 = _mm_unpacklo_epi16(a4, a5);
			b5 = _mm_unpackhi_epi16(a4, a5);
			b6 = _mm_unpacklo_epi16(a6, a7);
			b7 = _mm_unpackhi_epi16(a6, a7);

			_mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);

		}
	}
		}
static void PfreqN4FirstTranspose32_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride)
{
	EB_U32 i, j;

	for (i = 0; i < 4; i++)
	{
		//for (j = 0; j < 2; j++)
		j = 0;
		{

			__m128i a0, a1, a2, a3, a4, a5, a6, a7;
			__m128i b0, b1, b2, b3, b4, b5, b6, b7;

			a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
			a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
			a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
			a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
			a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
			a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
			a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
			a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

			b0 = _mm_unpacklo_epi16(a0, a4);
			b1 = _mm_unpacklo_epi16(a1, a5);
			b2 = _mm_unpacklo_epi16(a2, a6);
			b3 = _mm_unpacklo_epi16(a3, a7);
			b4 = _mm_unpackhi_epi16(a0, a4);
			b5 = _mm_unpackhi_epi16(a1, a5);
			b6 = _mm_unpackhi_epi16(a2, a6);
			b7 = _mm_unpackhi_epi16(a3, a7);

			a0 = _mm_unpacklo_epi16(b0, b2);
			a1 = _mm_unpacklo_epi16(b1, b3);
			a2 = _mm_unpackhi_epi16(b0, b2);
			a3 = _mm_unpackhi_epi16(b1, b3);
			a4 = _mm_unpacklo_epi16(b4, b6);
			a5 = _mm_unpacklo_epi16(b5, b7);
			a6 = _mm_unpackhi_epi16(b4, b6);
			a7 = _mm_unpackhi_epi16(b5, b7);

			b0 = _mm_unpacklo_epi16(a0, a1);
			b1 = _mm_unpackhi_epi16(a0, a1);
			b2 = _mm_unpacklo_epi16(a2, a3);
			b3 = _mm_unpackhi_epi16(a2, a3);
			b4 = _mm_unpacklo_epi16(a4, a5);
			b5 = _mm_unpackhi_epi16(a4, a5);
			b6 = _mm_unpacklo_epi16(a6, a7);
			b7 = _mm_unpackhi_epi16(a6, a7);

			_mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);

		}
	}
		}

void PfreqTranspose32Type1_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride)
{
    EB_U32 i, j;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;

            a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
            a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
            a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
            a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
            a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
            a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
            a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
            a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

            b0 = _mm_unpacklo_epi16(a0, a4);
            b1 = _mm_unpacklo_epi16(a1, a5);
            b2 = _mm_unpacklo_epi16(a2, a6);
            b3 = _mm_unpacklo_epi16(a3, a7);
            b4 = _mm_unpackhi_epi16(a0, a4);
            b5 = _mm_unpackhi_epi16(a1, a5);
            b6 = _mm_unpackhi_epi16(a2, a6);
            b7 = _mm_unpackhi_epi16(a3, a7);

            a0 = _mm_unpacklo_epi16(b0, b2);
            a1 = _mm_unpacklo_epi16(b1, b3);
            a2 = _mm_unpackhi_epi16(b0, b2);
            a3 = _mm_unpackhi_epi16(b1, b3);
            a4 = _mm_unpacklo_epi16(b4, b6);
            a5 = _mm_unpacklo_epi16(b5, b7);
            a6 = _mm_unpackhi_epi16(b4, b6);
            a7 = _mm_unpackhi_epi16(b5, b7);

            b0 = _mm_unpacklo_epi16(a0, a1);
            b1 = _mm_unpackhi_epi16(a0, a1);
            b2 = _mm_unpacklo_epi16(a2, a3);
            b3 = _mm_unpackhi_epi16(a2, a3);
            b4 = _mm_unpacklo_epi16(a4, a5);
            b5 = _mm_unpackhi_epi16(a4, a5);
            b6 = _mm_unpacklo_epi16(a6, a7);
            b7 = _mm_unpackhi_epi16(a6, a7);

            _mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
        }
    }
}
void PfreqTranspose32Type2_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride)
{
    EB_U32 i, j;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 4; j++)
        {
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;

            a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
            a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
            a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
            a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
            a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
            a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
            a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
            a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

            b0 = _mm_unpacklo_epi16(a0, a4);
            b1 = _mm_unpacklo_epi16(a1, a5);
            b2 = _mm_unpacklo_epi16(a2, a6);
            b3 = _mm_unpacklo_epi16(a3, a7);
            b4 = _mm_unpackhi_epi16(a0, a4);
            b5 = _mm_unpackhi_epi16(a1, a5);
            b6 = _mm_unpackhi_epi16(a2, a6);
            b7 = _mm_unpackhi_epi16(a3, a7);

            a0 = _mm_unpacklo_epi16(b0, b2);
            a1 = _mm_unpacklo_epi16(b1, b3);
            a2 = _mm_unpackhi_epi16(b0, b2);
            a3 = _mm_unpackhi_epi16(b1, b3);
            a4 = _mm_unpacklo_epi16(b4, b6);
            a5 = _mm_unpacklo_epi16(b5, b7);
            a6 = _mm_unpackhi_epi16(b4, b6);
            a7 = _mm_unpackhi_epi16(b5, b7);

            b0 = _mm_unpacklo_epi16(a0, a1);
            b1 = _mm_unpackhi_epi16(a0, a1);
            b2 = _mm_unpacklo_epi16(a2, a3);
            b3 = _mm_unpackhi_epi16(a2, a3);
            b4 = _mm_unpacklo_epi16(a4, a5);
            b5 = _mm_unpackhi_epi16(a4, a5);
            b6 = _mm_unpacklo_epi16(a6, a7);
            b7 = _mm_unpackhi_epi16(a6, a7);

            _mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
        }
    }
}

static void Transpose32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride)
{
    EB_U32 i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;

            a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
            a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
            a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
            a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
            a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
            a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
            a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
            a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

            b0 = _mm_unpacklo_epi16(a0, a4);
            b1 = _mm_unpacklo_epi16(a1, a5);
            b2 = _mm_unpacklo_epi16(a2, a6);
            b3 = _mm_unpacklo_epi16(a3, a7);
            b4 = _mm_unpackhi_epi16(a0, a4);
            b5 = _mm_unpackhi_epi16(a1, a5);
            b6 = _mm_unpackhi_epi16(a2, a6);
            b7 = _mm_unpackhi_epi16(a3, a7);

            a0 = _mm_unpacklo_epi16(b0, b2);
            a1 = _mm_unpacklo_epi16(b1, b3);
            a2 = _mm_unpackhi_epi16(b0, b2);
            a3 = _mm_unpackhi_epi16(b1, b3);
            a4 = _mm_unpacklo_epi16(b4, b6);
            a5 = _mm_unpacklo_epi16(b5, b7);
            a6 = _mm_unpackhi_epi16(b4, b6);
            a7 = _mm_unpackhi_epi16(b5, b7);

            b0 = _mm_unpacklo_epi16(a0, a1);
            b1 = _mm_unpackhi_epi16(a0, a1);
            b2 = _mm_unpacklo_epi16(a2, a3);
            b3 = _mm_unpackhi_epi16(a2, a3);
            b4 = _mm_unpacklo_epi16(a4, a5);
            b5 = _mm_unpackhi_epi16(a4, a5);
            b6 = _mm_unpacklo_epi16(a6, a7);
            b7 = _mm_unpackhi_epi16(a6, a7);

            _mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
        }
    }
}

void Pfreq2DInvTransform32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl2;

    for (i = 0; i < 32; i++)
    {
        __m128i x0, x1, x2, x3;
        __m128i y0, y1, y2;
        __m128i a0, a1, a2, a3, a4, a5, a6, a7;
        __m128i b0, b1, b2, b3, b4, b5, b6, b7;
        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00)); // 00 01 02 03 04 05 06 07
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08)); // 08 09 0a 0b 0c 0d 0e 0f

        y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
        y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f

        x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
        x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f

        y0 = _mm_unpacklo_epi64(x0, x0); // 00 04 08 0c 10 14 18 1c     y0=part of it zero
        y1 = _mm_unpacklo_epi64(x1, x1); // 02 06 0a 0e 12 16 1a 1e     y1=part of it zero
        y2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f

        x0 = y0;   //part of it zero
        x1 = y1;   //part of it zero
        x2 = y2;

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));

        a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));

        a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));

        a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));

        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));

        a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));

        a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));

        a0 = _mm_add_epi32(a0, o0);
        a1 = _mm_add_epi32(a1, o0);

        b0 = _mm_add_epi32(a0, a2);
        b1 = _mm_add_epi32(a1, a3);
        b2 = _mm_sub_epi32(a0, a2);
        b3 = _mm_sub_epi32(a1, a3);

        a0 = b0;
        a1 = b1;
        a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
        a3 = _mm_shuffle_epi32(b2, 0x1b);

        b0 = _mm_add_epi32(a0, a4);
        b1 = _mm_add_epi32(a1, a5);
        b2 = _mm_add_epi32(a2, a6);
        b3 = _mm_add_epi32(a3, a7);
        b4 = _mm_sub_epi32(a0, a4);
        b5 = _mm_sub_epi32(a1, a5);
        b6 = _mm_sub_epi32(a2, a6);
        b7 = _mm_sub_epi32(a3, a7);

        a0 = _mm_sra_epi32(b0, s0);
        a1 = _mm_sra_epi32(b1, s0);
        a2 = _mm_sra_epi32(b2, s0);
        a3 = _mm_sra_epi32(b3, s0);
        a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
        a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
        a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
        a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

        x0 = _mm_packs_epi32(a0, a1);
        x1 = _mm_packs_epi32(a2, a3);
        x2 = _mm_packs_epi32(a4, a5);
        x3 = _mm_packs_epi32(a6, a7);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x10), x2);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x18), x3);
    }
}
void Pfreq1DInvTransform32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl2;

    for (i = 0; i < 16; i++)
    {
        __m128i x0, x1, x2, x3;
        __m128i y0, y1, y2;
        __m128i a0, a1, a2, a3, a4, a5, a6, a7;
        __m128i b0, b1, b2, b3, b4, b5, b6, b7;
        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00)); // 00 01 02 03 04 05 06 07
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08)); // 08 09 0a 0b 0c 0d 0e 0f

        y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
        y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f

        x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
        x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f

        y0 = _mm_unpacklo_epi64(x0, x0); // 00 04 08 0c 10 14 18 1c     y0=part of it zero
        y1 = _mm_unpacklo_epi64(x1, x1); // 02 06 0a 0e 12 16 1a 1e     y1=part of it zero
        y2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f

        x0 = y0;   //part of it zero
        x1 = y1;   //part of it zero
        x2 = y2;

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));

        a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));

        a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));

        a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));

        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));

        a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));

        a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));

        a0 = _mm_add_epi32(a0, o0);
        a1 = _mm_add_epi32(a1, o0);

        b0 = _mm_add_epi32(a0, a2);
        b1 = _mm_add_epi32(a1, a3);
        b2 = _mm_sub_epi32(a0, a2);
        b3 = _mm_sub_epi32(a1, a3);

        a0 = b0;
        a1 = b1;
        a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
        a3 = _mm_shuffle_epi32(b2, 0x1b);

        b0 = _mm_add_epi32(a0, a4);
        b1 = _mm_add_epi32(a1, a5);
        b2 = _mm_add_epi32(a2, a6);
        b3 = _mm_add_epi32(a3, a7);
        b4 = _mm_sub_epi32(a0, a4);
        b5 = _mm_sub_epi32(a1, a5);
        b6 = _mm_sub_epi32(a2, a6);
        b7 = _mm_sub_epi32(a3, a7);

        a0 = _mm_sra_epi32(b0, s0);
        a1 = _mm_sra_epi32(b1, s0);
        a2 = _mm_sra_epi32(b2, s0);
        a3 = _mm_sra_epi32(b3, s0);
        a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
        a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
        a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
        a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

        x0 = _mm_packs_epi32(a0, a1);
        x1 = _mm_packs_epi32(a2, a3);
        x2 = _mm_packs_epi32(a4, a5);
        x3 = _mm_packs_epi32(a6, a7);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x10), x2);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x18), x3);
    }
}

void PfreqEstimateInvTransform32x32_SSE2(
    EB_S16 *src,
    const EB_U32  src_stride,
    EB_S16 *dst,
    const EB_U32  dst_stride,
    EB_S16 *intermediate,
    EB_U32  addshift)
{
    PfreqTranspose32Type1_SSE2(src, src_stride, intermediate, 32);
    Pfreq1DInvTransform32_SSE2(intermediate, 32, dst, dst_stride, 7);
    PfreqTranspose32Type2_SSE2(dst, dst_stride, intermediate, 32);
    Pfreq2DInvTransform32_SSE2(intermediate, 32, dst, dst_stride, 12 - addshift);
}

// 32-point inverse transform (32 rows)
static void InvTransform32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl2;

    for (i = 0; i < 32; i++)
    {
        __m128i x0, x1, x2, x3;
        __m128i y0, y1, y2, y3;
        __m128i a0, a1, a2, a3, a4, a5, a6, a7;
        __m128i b0, b1, b2, b3, b4, b5, b6, b7;
        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00)); // 00 01 02 03 04 05 06 07
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08)); // 08 09 0a 0b 0c 0d 0e 0f
        x2 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x10)); // 10 11 12 13 14 15 16 17
        x3 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x18)); // 18 19 1a 1b 1c 1d 1e 1f

        y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
        y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f
        y2 = _mm_unpacklo_epi16(x2, x3); // 10 18
        y3 = _mm_unpackhi_epi16(x2, x3); // 24 2c

        x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
        x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f
        x2 = _mm_unpacklo_epi16(y2, y3); // 10 14
        x3 = _mm_unpackhi_epi16(y2, y3); // 12 16

        y0 = _mm_unpacklo_epi64(x0, x2); // 00 04 08 0c 10 14 18 1c
        y1 = _mm_unpacklo_epi64(x1, x3); // 02 06 0a 0e 12 16 1a 1e
        y2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f
        y3 = _mm_unpackhi_epi16(x2, x3); // 11 13 15 17 19 1b 1d 1f

        x0 = y0;
        x1 = y1;
        x2 = y2;
        x3 = y3;

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

        a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

        a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));

        a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

        a0 = _mm_add_epi32(a0, o0);
        a1 = _mm_add_epi32(a1, o0);

        b0 = _mm_add_epi32(a0, a2);
        b1 = _mm_add_epi32(a1, a3);
        b2 = _mm_sub_epi32(a0, a2);
        b3 = _mm_sub_epi32(a1, a3);

        a0 = b0;
        a1 = b1;
        a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
        a3 = _mm_shuffle_epi32(b2, 0x1b);

        b0 = _mm_add_epi32(a0, a4);
        b1 = _mm_add_epi32(a1, a5);
        b2 = _mm_add_epi32(a2, a6);
        b3 = _mm_add_epi32(a3, a7);
        b4 = _mm_sub_epi32(a0, a4);
        b5 = _mm_sub_epi32(a1, a5);
        b6 = _mm_sub_epi32(a2, a6);
        b7 = _mm_sub_epi32(a3, a7);

        a0 = _mm_sra_epi32(b0, s0);
        a1 = _mm_sra_epi32(b1, s0);
        a2 = _mm_sra_epi32(b2, s0);
        a3 = _mm_sra_epi32(b3, s0);
        a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
        a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
        a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
        a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

        x0 = _mm_packs_epi32(a0, a1);
        x1 = _mm_packs_epi32(a2, a3);
        x2 = _mm_packs_epi32(a4, a5);
        x3 = _mm_packs_epi32(a6, a7);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x10), x2);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x18), x3);
    }
}

void EstimateInvTransform32x32_SSE2(
    EB_S16 *src,
    const EB_U32  src_stride,
    EB_S16 *dst,
    const EB_U32  dst_stride,
    EB_S16 *intermediate,
    EB_U32  addshift)
{
    Transpose32_SSE2(src, src_stride, intermediate, 32);
    InvTransform32_SSE2(intermediate, 32, dst, dst_stride, 7);
    Transpose32_SSE2(dst, dst_stride, intermediate, 32);
    InvTransform32_SSE2(intermediate, 32, dst, dst_stride, 12 - addshift);
}



// forward 16x16 transform
void Transform16x16_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32  addshift)
{
    Transform16(src, src_stride, intermediate, 16, 4 + addshift);
    Transpose16(intermediate, 16, dst, dst_stride);
    Transform16(dst, dst_stride, intermediate, 16, 9);
    Transpose16(intermediate, 16, dst, dst_stride);
}


// inverse 16x16 transform
void EstimateInvTransform16x16_SSE2(
    EB_S16  *src,
    EB_U32   src_stride,
    EB_S16  *dst,
    EB_U32   dst_stride,
    EB_S16  *intermediate,
    EB_U32   addshift)
{
    Transpose16(src, src_stride, intermediate, 16);
    InvTransform16(intermediate, 16, dst, dst_stride, 7);
    Transpose16(dst, dst_stride, intermediate, 16);
    InvTransform16(intermediate, 16, dst, dst_stride, 12 - addshift);
}


static void Transform32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl;

    for (i = 0; i < 32; i++)
    {
        __m128i x0, x1, x2, x3;
        __m128i y0, y1, y2, y3;
        __m128i a0, a1, a2, a3, a4, a5, a6, a7;
        __m128i b0, b1, b2, b3, b4, b5, b6, b7;

        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));
        x2 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x10));
        x3 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x18));

        // 32-point butterfly
        x2 = reverse_epi16(x2);
        x3 = reverse_epi16(x3);

        y0 = _mm_add_epi16(x0, x3);
        y1 = _mm_add_epi16(x1, x2);

        y2 = _mm_sub_epi16(x0, x3);
        y3 = _mm_sub_epi16(x1, x2);

        // 16-point butterfly
        y1 = reverse_epi16(y1);

        x0 = _mm_add_epi16(y0, y1);
        x1 = _mm_sub_epi16(y0, y1);

        x2 = y2;
        x3 = y3;

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
        a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
        a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

        a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

        a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
        a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));

        a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
        a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

        b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
        b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
        b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
        b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);
        b4 = _mm_sra_epi32(_mm_add_epi32(a4, o0), s0);
        b5 = _mm_sra_epi32(_mm_add_epi32(a5, o0), s0);
        b6 = _mm_sra_epi32(_mm_add_epi32(a6, o0), s0);
        b7 = _mm_sra_epi32(_mm_add_epi32(a7, o0), s0);

        x0 = _mm_packs_epi32(b0, b1);
        x1 = _mm_packs_epi32(b2, b3);
        x2 = _mm_packs_epi32(b4, b5);
        x3 = _mm_packs_epi32(b6, b7);

        y0 = _mm_unpacklo_epi16(x0, x1);
        y1 = _mm_unpackhi_epi16(x0, x1);
        y2 = x2;
        y3 = x3;
        x0 = _mm_unpacklo_epi16(y0, y2);
        x1 = _mm_unpackhi_epi16(y0, y2);
        x2 = _mm_unpacklo_epi16(y1, y3);
        x3 = _mm_unpackhi_epi16(y1, y3);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x10), x2);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x18), x3);
    }
}

static void Pfreq1DTransform32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl;

    for (i = 0; i < 32; i++)
    {
        __m128i x0, x1, x2, x3;
        __m128i y0, y1, y2, y3;
        __m128i a0, a2, a4, a5;
        __m128i b0, b1, b2, b3, b4, b5;


        b1 = s0;
        b3 = s0;

        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));
        x2 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x10));
        x3 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x18));


        // 32-point butterfly
        x2 = reverse_epi16(x2);
        x3 = reverse_epi16(x3);

        y0 = _mm_add_epi16(x0, x3);
        y1 = _mm_add_epi16(x1, x2);

        y2 = _mm_sub_epi16(x0, x3);
        y3 = _mm_sub_epi16(x1, x2);

        // 16-point butterfly
        y1 = reverse_epi16(y1);

        x0 = _mm_add_epi16(y0, y1);
        x1 = _mm_sub_epi16(y0, y1);


        x2 = y2;
        x3 = y3;



        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        //a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        //a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
        //a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
        //a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        //a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        //a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
        //a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
        //a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

        a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

        //a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));
        //
        //a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

        b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
        //b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
        b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
        //b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);
        b4 = _mm_sra_epi32(_mm_add_epi32(a4, o0), s0);
        b5 = _mm_sra_epi32(_mm_add_epi32(a5, o0), s0);
        //b6 = _mm_sra_epi32(_mm_add_epi32(a6, o0), s0);
        //b7 = _mm_sra_epi32(_mm_add_epi32(a7, o0), s0);

        x0 = _mm_packs_epi32(b0, b1);
        x1 = _mm_packs_epi32(b2, b3);
        x2 = _mm_packs_epi32(b4, b5);
        //x3 = _mm_packs_epi32(b6, b7);

        y0 = _mm_unpacklo_epi16(x0, x1);
        //y1 = _mm_unpackhi_epi16(x0, x1);
        y2 = x2;
        //y3 = x3;
        x0 = _mm_unpacklo_epi16(y0, y2);
        x1 = _mm_unpackhi_epi16(y0, y2);
        //x2 = _mm_unpacklo_epi16(y1, y3);
        //x3 = _mm_unpackhi_epi16(y1, y3);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
        //_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x10), x2);
        //_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x18), x3);
    }
}
static void Pfreq2DTransform32_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl;

    for (i = 0; i < 16; i++)
    {
        __m128i x0, x1, x2, x3;
        __m128i y0, y1, y2, y3;
        __m128i a0, a2, a4, a5;
        __m128i b0, b1, b2, b3, b4, b5;

        b1 = s0;
        b3 = s0;

        x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
        x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));
        x2 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x10));
        x3 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x18));


        // 32-point butterfly
        x2 = reverse_epi16(x2);
        x3 = reverse_epi16(x3);

        y0 = _mm_add_epi16(x0, x3);
        y1 = _mm_add_epi16(x1, x2);

        y2 = _mm_sub_epi16(x0, x3);
        y3 = _mm_sub_epi16(x1, x2);

        // 16-point butterfly
        y1 = reverse_epi16(y1);

        x0 = _mm_add_epi16(y0, y1);
        x1 = _mm_sub_epi16(y0, y1);


        x2 = y2;
        x3 = y3;



        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        //a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
        //a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
        //a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
        //a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        //a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
        //a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
        //a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
        //a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

        a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
        a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

        //a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
        //a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));
        //
        //a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
        //a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

        b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
        //b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
        b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
        //b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);
        b4 = _mm_sra_epi32(_mm_add_epi32(a4, o0), s0);
        b5 = _mm_sra_epi32(_mm_add_epi32(a5, o0), s0);
        //b6 = _mm_sra_epi32(_mm_add_epi32(a6, o0), s0);
        //b7 = _mm_sra_epi32(_mm_add_epi32(a7, o0), s0);

        x0 = _mm_packs_epi32(b0, b1);
        x1 = _mm_packs_epi32(b2, b3);
        x2 = _mm_packs_epi32(b4, b5);
        //x3 = _mm_packs_epi32(b6, b7);

        y0 = _mm_unpacklo_epi16(x0, x1);
        //y1 = _mm_unpackhi_epi16(x0, x1);
        y2 = x2;
        //y3 = x3;
        x0 = _mm_unpacklo_epi16(y0, y2);
        x1 = _mm_unpackhi_epi16(y0, y2);
        //x2 = _mm_unpacklo_epi16(y1, y3);
        //x3 = _mm_unpackhi_epi16(y1, y3);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
        //_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x10), x2);
        //_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x18), x3);
    }
}

static void PfreqN41DTransform32_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride,
	EB_U32  shift)
{
	EB_U32 i;
	__m128i s0 = _mm_cvtsi32_si128(shift);
	__m128i o0 = _mm_set1_epi32(1 << (shift - 1));
	const __m128i *coeff32 = (const __m128i *)coeff_tbl;

	for (i = 0; i < 32; i++)
	{
		__m128i x0, x1, x2, x3;
		__m128i y0, y1, y2, y3;
		__m128i a0, a2, a4/*, a5*/;
		__m128i b0, b1, b2, b3, b4/*, b5*/;


		b1 = s0;
		b3 = s0;

		x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
		x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));
		x2 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x10));
		x3 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x18));


		// 32-point butterfly
		x2 = reverse_epi16(x2);
		x3 = reverse_epi16(x3);

		y0 = _mm_add_epi16(x0, x3);
		y1 = _mm_add_epi16(x1, x2);

		y2 = _mm_sub_epi16(x0, x3);
		y3 = _mm_sub_epi16(x1, x2);

		// 16-point butterfly
		y1 = reverse_epi16(y1);

		x0 = _mm_add_epi16(y0, y1);
		x1 = _mm_sub_epi16(y0, y1);


		x2 = y2;
		x3 = y3;



		a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

		//a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
		//a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
		//a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
		//a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

		a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

		//a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
		//a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
		//a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
		//a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

		a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

		/**///        a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
		/**///        a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

		//a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));
		//
		//a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

		b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
		//b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
		b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
		//b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);
		b4 = _mm_sra_epi32(_mm_add_epi32(a4, o0), s0);
		/**///         b5 = _mm_sra_epi32(_mm_add_epi32(a5, o0), s0);
		//b6 = _mm_sra_epi32(_mm_add_epi32(a6, o0), s0);
		//b7 = _mm_sra_epi32(_mm_add_epi32(a7, o0), s0);

		x0 = _mm_packs_epi32(b0, b1);
		x1 = _mm_packs_epi32(b2, b3);
		x2 = _mm_packs_epi32(b4, b4);

		//x3 = _mm_packs_epi32(b6, b7);

		y0 = _mm_unpacklo_epi16(x0, x1);
		//y1 = _mm_unpackhi_epi16(x0, x1);
		y2 = x2;
		//y3 = x3;
		x0 = _mm_unpacklo_epi16(y0, y2);
		/**///        x1 = _mm_unpackhi_epi16(y0, y2);
		//x2 = _mm_unpacklo_epi16(y1, y3);
		//x3 = _mm_unpackhi_epi16(y1, y3);

		_mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
		/**///       _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);
		//_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x10), x2);
		//_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x18), x3);
	}
}
static void PfreqN42DTransform32_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride,
	EB_U32  shift)
{
	EB_U32 i;
	__m128i s0 = _mm_cvtsi32_si128(shift);
	__m128i o0 = _mm_set1_epi32(1 << (shift - 1));
	const __m128i *coeff32 = (const __m128i *)coeff_tbl;

	for (i = 0; i < 8; i++)

	{
		__m128i x0, x1, x2, x3;
		__m128i y0, y1, y2, y3;
		__m128i a0, a2, a4/*, a5*/;
		__m128i b0, b1, b2, b3, b4/*, b5*/;

		b1 = s0;
		b3 = s0;

		x0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
		x1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));
		x2 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x10));
		x3 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x18));


		// 32-point butterfly
		x2 = reverse_epi16(x2);
		x3 = reverse_epi16(x3);

		y0 = _mm_add_epi16(x0, x3);
		y1 = _mm_add_epi16(x1, x2);

		y2 = _mm_sub_epi16(x0, x3);
		y3 = _mm_sub_epi16(x1, x2);

		// 16-point butterfly
		y1 = reverse_epi16(y1);

		x0 = _mm_add_epi16(y0, y1);
		x1 = _mm_sub_epi16(y0, y1);


		x2 = y2;
		x3 = y3;



		a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

		//a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
		//a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
		//a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
		//a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

		a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

		//a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
		//a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
		//a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
		//a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

		a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
		a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

		/**///         a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
		/**///         a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

		//a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
		//a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));
		//
		//a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
		//a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

		b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
		//b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
		b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
		//b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);
		b4 = _mm_sra_epi32(_mm_add_epi32(a4, o0), s0);
		/**///  b5 = _mm_sra_epi32(_mm_add_epi32(a5, o0), s0);
		//b6 = _mm_sra_epi32(_mm_add_epi32(a6, o0), s0);
		//b7 = _mm_sra_epi32(_mm_add_epi32(a7, o0), s0);

		x0 = _mm_packs_epi32(b0, b1);
		x1 = _mm_packs_epi32(b2, b3);
		x2 = _mm_packs_epi32(b4, b4);//do not use b5

		//x3 = _mm_packs_epi32(b6, b7);

		y0 = _mm_unpacklo_epi16(x0, x1);
		//y1 = _mm_unpackhi_epi16(x0, x1);
		y2 = x2;
		//y3 = x3;
		x0 = _mm_unpacklo_epi16(y0, y2);
		/**///   x1 = _mm_unpackhi_epi16(y0, y2);
		//x2 = _mm_unpacklo_epi16(y1, y3);
		//x3 = _mm_unpackhi_epi16(y1, y3);

		_mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), x0);
		/**///    _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x08), x1);

		//_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x10), x2);
		//_mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x18), x3);
	}
}

void Transform32x32_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift)
{
    Transform32_SSE2(src, src_stride, intermediate, 32, 6 + addshift);
    Transpose32_SSE2(intermediate, 32, dst, dst_stride);
    Transform32_SSE2(dst, dst_stride, intermediate, 32, 9);
    Transpose32_SSE2(intermediate, 32, dst, dst_stride);

    return;
}

void PfreqN4Transform32x32_SSE2(
	EB_S16 *src,
	const EB_U32 src_stride,
	EB_S16 *dst,
	const EB_U32 dst_stride,
	EB_S16 *intermediate,
	EB_U32 addshift)
{
	PfreqN41DTransform32_SSE2(src, src_stride, intermediate, 32, 6 + addshift);
	PfreqN4FirstTranspose32_SSE2(intermediate, 32, dst, dst_stride);
	PfreqN42DTransform32_SSE2(dst, dst_stride, intermediate, 32, 9);
	PfreqN4SecTranspose32_SSE2(intermediate, 32, dst, dst_stride);

	return;
}

static void Pfreq1DTransform16_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl;

    for (i = 0; i < 16; i++)
    {
        __m128i x0, x1;
        __m128i y0, y1;
        __m128i a0, a2;
        __m128i b0, b1, b2, b3;

        b1 = s0;
        b3 = s0;

        y0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
        y1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));


        // 16-point butterfly
        y1 = reverse_epi16(y1);

        x0 = _mm_add_epi16(y0, y1);
        x1 = _mm_sub_epi16(y0, y1);

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

        b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
        b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);

        x0 = _mm_packs_epi32(b0, b1);
        x1 = _mm_packs_epi32(b2, b3);

        y0 = _mm_unpacklo_epi16(x0, x1);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), y0);
    }
}

static void Pfreq2DTransform16_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride,
    EB_U32  shift)
{
    EB_U32 i;
    __m128i s0 = _mm_cvtsi32_si128(shift);
    __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
    const __m128i *coeff32 = (const __m128i *)coeff_tbl;

    for (i = 0; i < 8; i++)
    {
        __m128i x0, x1;
        __m128i y0, y1;
        __m128i a0, a2;
        __m128i b0, b1, b2, b3;

        b1 = s0;
        b3 = s0;

        y0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
        y1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));


        // 16-point butterfly
        y1 = reverse_epi16(y1);

        x0 = _mm_add_epi16(y0, y1);
        x1 = _mm_sub_epi16(y0, y1);

        a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
        a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));


        a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
        a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));


        b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
        b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);

        x0 = _mm_packs_epi32(b0, b1);
        x1 = _mm_packs_epi32(b2, b3);

        y0 = _mm_unpacklo_epi16(x0, x1);

        _mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), y0);
    }
}
static void PfreqTranspose16_SSE2(
    EB_S16 *src,
    EB_U32  src_stride,
    EB_S16 *dst,
    EB_U32  dst_stride)
{
    EB_U32 i, j;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 1; j++)
        {
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;

            a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
            a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
            a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
            a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
            a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
            a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
            a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
            a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

            b0 = _mm_unpacklo_epi16(a0, a4);
            b1 = _mm_unpacklo_epi16(a1, a5);
            b2 = _mm_unpacklo_epi16(a2, a6);
            b3 = _mm_unpacklo_epi16(a3, a7);
            b4 = _mm_unpackhi_epi16(a0, a4);
            b5 = _mm_unpackhi_epi16(a1, a5);
            b6 = _mm_unpackhi_epi16(a2, a6);
            b7 = _mm_unpackhi_epi16(a3, a7);

            a0 = _mm_unpacklo_epi16(b0, b2);
            a1 = _mm_unpacklo_epi16(b1, b3);
            a2 = _mm_unpackhi_epi16(b0, b2);
            a3 = _mm_unpackhi_epi16(b1, b3);
            a4 = _mm_unpacklo_epi16(b4, b6);
            a5 = _mm_unpacklo_epi16(b5, b7);
            a6 = _mm_unpackhi_epi16(b4, b6);
            a7 = _mm_unpackhi_epi16(b5, b7);

            b0 = _mm_unpacklo_epi16(a0, a1);
            b1 = _mm_unpackhi_epi16(a0, a1);
            b2 = _mm_unpacklo_epi16(a2, a3);
            b3 = _mm_unpackhi_epi16(a2, a3);
            b4 = _mm_unpacklo_epi16(a4, a5);
            b5 = _mm_unpackhi_epi16(a4, a5);
            b6 = _mm_unpacklo_epi16(a6, a7);
            b7 = _mm_unpackhi_epi16(a6, a7);

            _mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
            _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
        }
    }
}
void PfreqTransform16x16_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift)
{
    Pfreq1DTransform16_SSE2(src, src_stride, intermediate, 16, 4 + addshift);
    PfreqTranspose16_SSE2(intermediate, 16, dst, dst_stride);
    Pfreq2DTransform16_SSE2(dst, dst_stride, intermediate, 16, 9);
    PfreqTranspose16_SSE2(intermediate, 16, dst, dst_stride);

    return;
}

static void PfreqN42DTransform16_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride,
	EB_U32  shift)
{
	EB_U32 i;
	__m128i s0 = _mm_cvtsi32_si128(shift);
	__m128i o0 = _mm_set1_epi32(1 << (shift - 1));
	const __m128i *coeff32 = (const __m128i *)coeff_tbl;

	for (i = 0; i < 4; i++)

	{
		__m128i x0, x1;
		__m128i y0, y1;
		__m128i a0, a2;
		__m128i b0, b1, b2, b3;

		b1 = s0;
		b3 = s0;

		y0 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x00));
		y1 = _mm_loadu_si128((const __m128i *)(src + i*src_stride + 0x08));


		// 16-point butterfly
		y1 = reverse_epi16(y1);

		x0 = _mm_add_epi16(y0, y1);
		x1 = _mm_sub_epi16(y0, y1);

		a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
		a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));


		a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
		a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));


		b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
		b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);

		x0 = _mm_packs_epi32(b0, b1);
		x1 = _mm_packs_epi32(b2, b3);

		y0 = _mm_unpacklo_epi16(x0, x1);

		_mm_storeu_si128((__m128i *)(dst + i*dst_stride + 0x00), y0);//TODO change to 64bit
	}
}
static void PfreqN4FirstTranspose16_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride)
{
	EB_U32 i, j;
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 1; j++)
		{
			__m128i a0, a1, a2, a3, a4, a5, a6, a7;
			__m128i b0, b1, b2, b3/*, b4, b5, b6, b7*/;

			a0 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 0)*src_stride + 8 * j));
			a1 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 1)*src_stride + 8 * j));
			a2 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 2)*src_stride + 8 * j));
			a3 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 3)*src_stride + 8 * j));
			a4 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 4)*src_stride + 8 * j));
			a5 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 5)*src_stride + 8 * j));
			a6 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 6)*src_stride + 8 * j));
			a7 = _mm_loadu_si128((const __m128i *)(src + (8 * i + 7)*src_stride + 8 * j));

			b0 = _mm_unpacklo_epi16(a0, a4);
			b1 = _mm_unpacklo_epi16(a1, a5);
			b2 = _mm_unpacklo_epi16(a2, a6);
			b3 = _mm_unpacklo_epi16(a3, a7);
			// b4 = _mm_unpackhi_epi16(a0, a4);
			// b5 = _mm_unpackhi_epi16(a1, a5);
			// b6 = _mm_unpackhi_epi16(a2, a6);
			// b7 = _mm_unpackhi_epi16(a3, a7);

			a0 = _mm_unpacklo_epi16(b0, b2);
			a1 = _mm_unpacklo_epi16(b1, b3);
			a2 = _mm_unpackhi_epi16(b0, b2);
			a3 = _mm_unpackhi_epi16(b1, b3);
			// a4 = _mm_unpacklo_epi16(b4, b6);
			// a5 = _mm_unpacklo_epi16(b5, b7);
			// a6 = _mm_unpackhi_epi16(b4, b6);
			// a7 = _mm_unpackhi_epi16(b5, b7);

			b0 = _mm_unpacklo_epi16(a0, a1);
			b1 = _mm_unpackhi_epi16(a0, a1);
			b2 = _mm_unpacklo_epi16(a2, a3);
			b3 = _mm_unpackhi_epi16(a2, a3);
			// b4 = _mm_unpacklo_epi16(a4, a5);
			// b5 = _mm_unpackhi_epi16(a4, a5);
			// b6 = _mm_unpacklo_epi16(a6, a7);
			// b7 = _mm_unpackhi_epi16(a6, a7);

			_mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
			// _mm_storeu_si128((__m128i *)(dst + (8 * j + 4)*dst_stride + 8 * i), b4);
			// _mm_storeu_si128((__m128i *)(dst + (8 * j + 5)*dst_stride + 8 * i), b5);
			// _mm_storeu_si128((__m128i *)(dst + (8 * j + 6)*dst_stride + 8 * i), b6);
			// _mm_storeu_si128((__m128i *)(dst + (8 * j + 7)*dst_stride + 8 * i), b7);
		}
	}
}
static void PfreqN4SecondTranspose16_SSE2(
	EB_S16 *src,
	EB_U32  src_stride,
	EB_S16 *dst,
	EB_U32  dst_stride)
{
	EB_U32 i, j;

	i = j = 0;
	{
		{
			__m128i a0, a1, a2, a3/*, a4, a5, a6, a7*/;
			__m128i b0, b1, b2, b3/*, b4, b5, b6, b7*/;

			a0 = _mm_loadu_si128((const __m128i *)(src + (0)*src_stride));  //TODO load only 64bit
			a1 = _mm_loadu_si128((const __m128i *)(src + (1)*src_stride));
			a2 = _mm_loadu_si128((const __m128i *)(src + (2)*src_stride));
			a3 = _mm_loadu_si128((const __m128i *)(src + (3)*src_stride));

			b0 = _mm_unpacklo_epi16(a0, a0/*a4*/);
			b1 = _mm_unpacklo_epi16(a1, a1/*a5*/);
			b2 = _mm_unpacklo_epi16(a2, a2/*a6*/);
			b3 = _mm_unpacklo_epi16(a3, a3/*a7*/);

			a0 = _mm_unpacklo_epi16(b0, b2);
			a1 = _mm_unpacklo_epi16(b1, b3);
			a2 = _mm_unpackhi_epi16(b0, b2);
			a3 = _mm_unpackhi_epi16(b1, b3);

			b0 = _mm_unpacklo_epi16(a0, a1);
			b1 = _mm_unpackhi_epi16(a0, a1);
			b2 = _mm_unpacklo_epi16(a2, a3);
			b3 = _mm_unpackhi_epi16(a2, a3);

			_mm_storeu_si128((__m128i *)(dst + (8 * j + 0)*dst_stride + 8 * i), b0);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 1)*dst_stride + 8 * i), b1);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 2)*dst_stride + 8 * i), b2);
			_mm_storeu_si128((__m128i *)(dst + (8 * j + 3)*dst_stride + 8 * i), b3);
		}
	}
}

void PfreqTransform32x32_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift)
{
    Pfreq1DTransform32_SSE2(src, src_stride, intermediate, 32, 6 + addshift);
    PfreqTranspose32_SSE2(intermediate, 32, dst, dst_stride);
    Pfreq2DTransform32_SSE2(dst, dst_stride, intermediate, 32, 9);
    PfreqTranspose32_SSE2(intermediate, 32, dst, dst_stride);

    return;
}

void PfreqN4Transform16x16_SSE2(
	EB_S16 *src,
	const EB_U32 src_stride,
	EB_S16 *dst,
	const EB_U32 dst_stride,
	EB_S16 *intermediate,
	EB_U32 addshift)
{
	Pfreq1DTransform16_SSE2(src, src_stride, intermediate, 16, 4 + addshift);
	PfreqN4FirstTranspose16_SSE2(intermediate, 16, dst, dst_stride);
	PfreqN42DTransform16_SSE2(dst, dst_stride, intermediate, 16, 9);
	PfreqN4SecondTranspose16_SSE2(intermediate, 16, dst, dst_stride);

	return;
}


void Transform4x4_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define OFFSET_128 0
#define OFFSET_64_64 8
#define OFFSET_83_36 16
#define OFFSET_N36_N83 24
#define OFFSET_64_N64 32
#define OFFSET_N64_64 40
#define OFFSET_36_N83 48
#define OFFSET_83_N36 56


    EB_ALIGN(16) EB_S16 transformIntrinConst_SSE2[] = {
         128, 0,   128, 0,   128, 0,   128, 0,
         64,  64,  64,  64,  64,  64,  64,  64,
         83,  36,  83,  36,  83,  36,  83,  36,
        -36, -83, -36, -83, -36, -83, -36, -83,
         64, -64,  64, -64,  64, -64,  64, -64,
        -64,  64, -64,  64, -64,  64, -64,  64,
         36, -83,  36, -83,  36, -83,  36, -83,
         83, -36,  83, -36,  83, -36,  83, -36
    };
     EB_ALIGN(16) const EB_S16 * TransformAsmConst = transformIntrinConst_SSE2;
    __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm_offset, xmm_shift;

    xmm_shift = _mm_cvtsi32_si128(5 - bitIncrement);
    xmm0 = _mm_loadl_epi64((__m128i*)(residual));
    xmm1 = _mm_loadl_epi64((__m128i*)(residual + srcStride));
    xmm2 = _mm_loadl_epi64((__m128i *)(residual + 2 * srcStride));
    xmm3 = _mm_loadl_epi64((__m128i *)(residual + 3 * srcStride));
    xmm0 = _mm_unpacklo_epi16(xmm0, xmm1);
    xmm2 = _mm_unpacklo_epi16(xmm2, xmm3);

    xmm1 = _mm_unpackhi_epi32(xmm0, xmm2);
    xmm0 = _mm_unpacklo_epi32(xmm0, xmm2);
    xmm1 = _mm_unpacklo_epi64(_mm_srli_si128(xmm1, 8), xmm1);
    xmm3 = _mm_sub_epi16(xmm0, xmm1);
    xmm0 = _mm_add_epi16(xmm0, xmm1);

    xmm4 = xmm2 = xmm0;
    xmm0 = _mm_srli_si128(xmm0, 8);
    xmm2 = _mm_sll_epi16(_mm_add_epi16(xmm2, xmm0), xmm_shift);
    xmm4 = _mm_sll_epi16(_mm_sub_epi16(xmm4, xmm0), xmm_shift);

    xmm_offset = _mm_slli_epi16(_mm_set1_epi32(1), bitIncrement);
    xmm_shift = _mm_cvtsi32_si128(bitIncrement + 1);

    xmm1 = _mm_unpacklo_epi16(xmm3, _mm_srli_si128(xmm3, 8));

    xmm3 = _mm_madd_epi16(xmm1, _mm_load_si128((__m128i *)(transformIntrinConst_SSE2 + OFFSET_36_N83)));
    xmm1 = _mm_madd_epi16(xmm1, _mm_load_si128((__m128i *)(transformIntrinConst_SSE2 + OFFSET_83_36)));
    xmm1 = _mm_add_epi32(xmm1, xmm_offset);
    xmm3 = _mm_add_epi32(xmm3, xmm_offset);
    xmm1 = _mm_sra_epi32(xmm1, xmm_shift);
    xmm3 = _mm_sra_epi32(xmm3, xmm_shift);
    xmm1 = _mm_packs_epi32(xmm1, xmm3);

    xmm2 = _mm_unpacklo_epi32(xmm2, xmm1);
    xmm1 = _mm_srli_si128(xmm1, 8);
    xmm4 = _mm_unpacklo_epi32(xmm4, xmm1);

    xmm3 = _mm_unpackhi_epi64(xmm2, xmm4);
    xmm2 = _mm_unpacklo_epi64(xmm2, xmm4);

    xmm_offset = _mm_load_si128((__m128i *)(transformIntrinConst_SSE2 + OFFSET_128));

    MACRO_TRANS_2MAC(xmm2, xmm3, xmm0, xmm1, xmm_offset, OFFSET_64_64, OFFSET_64_64, 8, 0)
    MACRO_TRANS_2MAC(xmm2, xmm3, xmm4, xmm5, xmm_offset, OFFSET_83_36, OFFSET_N36_N83, 8, dstStride)
    MACRO_TRANS_2MAC(xmm2, xmm3, xmm6, xmm0, xmm_offset, OFFSET_64_N64, OFFSET_N64_64, 8, 2 * dstStride)
    MACRO_TRANS_2MAC(xmm2, xmm3, xmm1, xmm4, xmm_offset, OFFSET_36_N83, OFFSET_83_N36, 8, 3 * dstStride)

    (void)transformCoefficients;
    (void)transformInnerArrayPtr;

#undef OFFSET_128
#undef OFFSET_64_64
#undef OFFSET_83_36
#undef OFFSET_N36_N83
#undef OFFSET_64_N64
#undef OFFSET_N64_64
#undef OFFSET_36_N83
#undef OFFSET_83_N36
}

void DstTransform4x4_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
#define OFFSET_DST_1        0
#define OFFSET_DST_29_55    (8+OFFSET_DST_1)
#define OFFSET_DST_74_84    (8+OFFSET_DST_29_55)
#define OFFSET_DST_84_N29   (8+OFFSET_DST_74_84)
#define OFFSET_DST_N74_55   (8+OFFSET_DST_84_N29)
#define OFFSET_DST_55_N84   (8+OFFSET_DST_N74_55)
#define OFFSET_DST_74_N29   (8+OFFSET_DST_55_N84)
#define OFFSET_DST_37_37    (8+OFFSET_DST_74_N29)
#define OFFSET_DST_74_74    (8+OFFSET_DST_37_37)
#define OFFSET_DST_0_N37    (8+OFFSET_DST_74_74)
#define OFFSET_DST_0_N74    (8+OFFSET_DST_0_N37)

    __m128i xmm_res0, xmm_res1, xmm_res2, xmm_res3, xmm_res0_1, xmm_res2_3, xmm_res_lo, xmm_res_hi, xmm_offset;
    __m128i xmm_trans0, xmm_trans1, xmm_trans2, xmm_trans3, xmm_trans0_1, xmm_trans2_3, xmm_trans_lo, xmm_trans_hi;
    __m128i xmm_temp;

    EB_U32 shift = bitIncrement + 1;
    EB_ALIGN(16) const EB_S16 * TransformAsmConst = DstTransformAsmConst_SSE2;

    xmm_res0 = _mm_loadl_epi64((__m128i *)(residual));
    xmm_res1 = _mm_loadl_epi64((__m128i *)(residual + srcStride));
    xmm_res2 = _mm_loadl_epi64((__m128i *)(residual + 2 * srcStride));
    xmm_res3 = _mm_loadl_epi64((__m128i *)(residual + 3 * srcStride));
    xmm_offset = _mm_srli_epi32(_mm_slli_epi32(_mm_load_si128((__m128i *)(DstTransformAsmConst_SSE2 + OFFSET_DST_1)), shift), 1);

    xmm_res0_1 = _mm_unpacklo_epi32(xmm_res0, xmm_res1); // |res01    |res-S1-01|res23    |res-S1-23|
    xmm_res2_3 = _mm_unpacklo_epi32(xmm_res2, xmm_res3); // |res-S2-01|res-S3-01|res-S2-23|res-S3-23|
    xmm_res_hi = _mm_unpackhi_epi64(xmm_res0_1, xmm_res2_3); // |res23    |res-S1-23|res-S2-23|res-S3-23|
    xmm_res_lo = _mm_unpacklo_epi64(xmm_res0_1, xmm_res2_3); // |res01    |res-S1-01|res-S2-01|res-S3-01|

    MACRO_TRANS_2MAC_NO_SAVE(xmm_res_lo, xmm_res_hi, xmm_trans0, xmm_temp, xmm_offset, OFFSET_DST_29_55, OFFSET_DST_74_84, shift)
    MACRO_TRANS_2MAC_NO_SAVE(xmm_res_lo, xmm_res_hi, xmm_trans1, xmm_temp, xmm_offset, OFFSET_DST_74_74, OFFSET_DST_0_N74, shift)
    MACRO_TRANS_2MAC_NO_SAVE(xmm_res_lo, xmm_res_hi, xmm_trans2, xmm_temp, xmm_offset, OFFSET_DST_84_N29, OFFSET_DST_N74_55, shift)
    MACRO_TRANS_2MAC_NO_SAVE(xmm_res_lo, xmm_res_hi, xmm_trans3, xmm_temp, xmm_offset, OFFSET_DST_55_N84, OFFSET_DST_74_N29, shift)

    // Second Partial Bufferfly
    xmm_offset = _mm_set1_epi32(0x00000080); // 128
    xmm_trans0_1 = _mm_unpacklo_epi32(xmm_trans0, xmm_trans1);
    xmm_trans2_3 = _mm_unpacklo_epi32(xmm_trans2, xmm_trans3);
    xmm_trans_hi = _mm_unpackhi_epi64(xmm_trans0_1, xmm_trans2_3);
    xmm_trans_lo = _mm_unpacklo_epi64(xmm_trans0_1, xmm_trans2_3);

    MACRO_TRANS_2MAC(xmm_trans_lo, xmm_trans_hi, xmm_trans0, xmm_temp, xmm_offset, OFFSET_DST_29_55, OFFSET_DST_74_84, 8, 0)
    MACRO_TRANS_2MAC(xmm_trans_lo, xmm_trans_hi, xmm_trans1, xmm_temp, xmm_offset, OFFSET_DST_74_74, OFFSET_DST_0_N74, 8, dstStride)
    MACRO_TRANS_2MAC(xmm_trans_lo, xmm_trans_hi, xmm_trans2, xmm_temp, xmm_offset, OFFSET_DST_84_N29, OFFSET_DST_N74_55, 8, (2 * dstStride))
    MACRO_TRANS_2MAC(xmm_trans_lo, xmm_trans_hi, xmm_trans3, xmm_temp, xmm_offset, OFFSET_DST_55_N84, OFFSET_DST_74_N29, 8, (3 * dstStride))

    (void)transformInnerArrayPtr;
}

void Transform8x8_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    // Transform8x8 has its own table because the larger table's offset macros exceed 256 (which is maximum macro expansion depth
    // Use a smaller table with values just for Transform8x8.

    EB_ALIGN(16) EB_S16 transformIntrinConst_8x8[] = {
        83,  36,  83,  36,  83,  36,  83,  36,
        36, -83,  36, -83,  36, -83,  36, -83,
        89,  75,  89,  75,  89,  75,  89,  75,
        50,  18,  50,  18,  50,  18,  50,  18,
        75, -18,  75, -18,  75, -18,  75, -18,
       -89, -50, -89, -50, -89, -50, -89, -50,
        50, -89,  50, -89,  50, -89,  50, -89,
        18,  75,  18,  75,  18,  75,  18,  75,
        18, -50,  18, -50,  18, -50,  18, -50,
        75, -89,  75, -89,  75, -89,  75, -89,
        256, 0,   256, 0,   256, 0,   256, 0,
        64,  64,  64,  64,  64,  64,  64,  64,
       -18, -50, -18, -50, -18, -50, -18, -50,
       -75, -89, -75, -89, -75, -89, -75, -89,
       -36, -83, -36, -83, -36, -83, -36, -83,
       -83, -36, -83, -36, -83, -36, -83, -36,
        36,  83,  36,  83,  36,  83,  36,  83,
        50,  89,  50,  89,  50,  89,  50,  89,
        18, -75,  18, -75,  18, -75,  18, -75,
       -64,  64, -64,  64, -64,  64, -64,  64,
        64, -64,  64, -64,  64, -64,  64, -64,
       -75, -18, -75, -18, -75, -18, -75, -18,
        89, -50,  89, -50,  89, -50,  89, -50,
        83, -36,  83, -36,  83, -36,  83, -36,
       -36,  83, -36,  83, -36,  83, -36,  83,
       -83,  36, -83,  36, -83,  36, -83,  36,
        89, -75,  89, -75,  89, -75,  89, -75,
        50, -18,  50, -18,  50, -18,  50, -18,
    };
    __m128i sum, sum1, sum2, sum3, sum4;
    __m128i res0, res1, res2, res3, res4, res5, res6, res7;
    __m128i res01, res23, res45, res67, res02, res0123, res46, res4567, res04, res0246, res0145, res0_to_7;
    __m128i even0, even1, even2, even3, odd0, odd1, odd2, odd3, odd01_lo, odd01_hi, odd23_lo, odd23_hi;
    __m128i evenEven0, evenEven1, evenOdd0, evenOdd1;
    __m128i trans0, trans1, trans2, trans3, trans4, trans5, trans6, trans7, trans01, trans23, trans45, trans67;
    __m128i trans02, trans0123, trans46, trans4567;
    __m128i xmm_offset;
    EB_ALIGN(16) EB_S16 * TransformIntrinConst = transformIntrinConst_8x8;
    EB_U32 shift;

    res0 = _mm_loadu_si128((__m128i *)residual);
    res1 = _mm_loadu_si128((__m128i *)(residual + srcStride));
    res2 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
    res3 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));
    residual += (srcStride << 2);
    res4 = _mm_loadu_si128((__m128i *)residual);
    res5 = _mm_loadu_si128((__m128i *)(residual + srcStride));
    res6 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
    res7 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));

    MACRO_UNPACK(16, res0, res1, res2, res3, res4, res5, res6, res7, res01, res23, res45, res67)
    MACRO_UNPACK(32, res0, res2, res01, res23, res4, res6, res45, res67, res02, res0123, res46, res4567)
    MACRO_UNPACK(64, res0, res4, res02, res46, res01, res45, res0123, res4567, res04, res0246, res0145, res0_to_7)
    MACRO_CALC_EVEN_ODD(res0, res04, res02, res0246, res01, res0145, res0123, res0_to_7)

    evenEven0 = _mm_add_epi16(even0, even3);
    evenEven1 = _mm_add_epi16(even1, even2);
    evenOdd0 = _mm_sub_epi16(even0, even3);
    evenOdd1 = _mm_sub_epi16(even1, even2);

    shift = 4 - bitIncrement;
    trans0 = _mm_slli_epi16(_mm_add_epi16(evenEven0, evenEven1), shift);
    trans4 = _mm_slli_epi16(_mm_sub_epi16(evenEven0, evenEven1), shift);

    xmm_offset = _mm_slli_epi32(_mm_set1_epi32(0x00000002), bitIncrement);
    shift = bitIncrement + 2;

    trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_83_36)),_mm_unpacklo_epi16(evenOdd0, evenOdd1)), xmm_offset), shift),
                             _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_83_36)),_mm_unpackhi_epi16(evenOdd0, evenOdd1)), xmm_offset), shift));

    trans6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_36_N83)),_mm_unpacklo_epi16(evenOdd0, evenOdd1)), xmm_offset), shift),
                             _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_36_N83)),_mm_unpackhi_epi16(evenOdd0, evenOdd1)), xmm_offset), shift));

    // TransformCoefficients 1, 3, 5, 7
    odd01_lo = _mm_unpacklo_epi16(odd0, odd1);
    odd01_hi = _mm_unpackhi_epi16(odd0, odd1);
    odd23_lo = _mm_unpacklo_epi16(odd2, odd3);
    odd23_hi = _mm_unpackhi_epi16(odd2, odd3);

    MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans1, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_89_75,  TRANS8x8_OFFSET_50_18, shift)
    MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans3, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_75_N18, TRANS8x8_OFFSET_N89_N50, shift)
    MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans5, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_50_N89, TRANS8x8_OFFSET_18_75, shift)
    MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans7, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_18_N50, TRANS8x8_OFFSET_75_N89, shift)

    MACRO_UNPACK(32, trans0, trans1, trans2, trans3, trans4, trans5, trans6, trans7, trans01, trans23, trans45, trans67)
    MACRO_UNPACK(64, trans0, trans2, trans01, trans23, trans4, trans6, trans45, trans67, trans02, trans0123, trans46, trans4567)

    xmm_offset = _mm_loadu_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_256));

    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64,    TRANS8x8_OFFSET_64_64,   TRANS8x8_OFFSET_64_64, 9, _mm_storeu_si128, transformCoefficients, 0)
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_89_75, TRANS8x8_OFFSET_50_18,    TRANS8x8_OFFSET_N18_N50, TRANS8x8_OFFSET_N75_N89, 9, _mm_storeu_si128, transformCoefficients, (dstStride))
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_83_36, TRANS8x8_OFFSET_N36_N83,  TRANS8x8_OFFSET_N83_N36, TRANS8x8_OFFSET_36_83, 9, _mm_storeu_si128, transformCoefficients, (2 * dstStride))
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_75_N18, TRANS8x8_OFFSET_N89_N50, TRANS8x8_OFFSET_50_89,   TRANS8x8_OFFSET_18_N75, 9, _mm_storeu_si128, transformCoefficients, (3 * dstStride))
    transformCoefficients += 4 * dstStride;
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_64_N64, TRANS8x8_OFFSET_N64_64, TRANS8x8_OFFSET_64_N64, TRANS8x8_OFFSET_N64_64, 9, _mm_storeu_si128, transformCoefficients, 0)
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_50_N89, TRANS8x8_OFFSET_18_75,  TRANS8x8_OFFSET_N75_N18, TRANS8x8_OFFSET_89_N50, 9, _mm_storeu_si128, transformCoefficients, (dstStride))
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_36_N83, TRANS8x8_OFFSET_83_N36, TRANS8x8_OFFSET_N36_83, TRANS8x8_OFFSET_N83_36, 9, _mm_storeu_si128, transformCoefficients, (2 * dstStride))
    MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_18_N50, TRANS8x8_OFFSET_75_N89, TRANS8x8_OFFSET_89_N75, TRANS8x8_OFFSET_50_N18, 9, _mm_storeu_si128, transformCoefficients, (3 * dstStride))

    (void)transformInnerArrayPtr;
}


void PfreqTransform8x8_SSE2_INTRIN(
	EB_S16                  *residual,
	const EB_U32             srcStride,
	EB_S16                  *transformCoefficients,
	const EB_U32             dstStride,
	EB_S16                  *transformInnerArrayPtr,
	EB_U32                   bitIncrement)
{
	// Transform8x8 has its own table because the larger table's offset macros exceed 256 (which is maximum macro expansion depth
	// Use a smaller table with values just for Transform8x8.

	EB_ALIGN(16) EB_S16 transformIntrinConst_8x8[] = {
		83, 36, 83, 36, 83, 36, 83, 36,
		36, -83, 36, -83, 36, -83, 36, -83,
		89, 75, 89, 75, 89, 75, 89, 75,
		50, 18, 50, 18, 50, 18, 50, 18,
		75, -18, 75, -18, 75, -18, 75, -18,
		-89, -50, -89, -50, -89, -50, -89, -50,
		50, -89, 50, -89, 50, -89, 50, -89,
		18, 75, 18, 75, 18, 75, 18, 75,
		18, -50, 18, -50, 18, -50, 18, -50,
		75, -89, 75, -89, 75, -89, 75, -89,
		256, 0, 256, 0, 256, 0, 256, 0,
		64, 64, 64, 64, 64, 64, 64, 64,
		-18, -50, -18, -50, -18, -50, -18, -50,
		-75, -89, -75, -89, -75, -89, -75, -89,
		-36, -83, -36, -83, -36, -83, -36, -83,
		-83, -36, -83, -36, -83, -36, -83, -36,
		36, 83, 36, 83, 36, 83, 36, 83,
		50, 89, 50, 89, 50, 89, 50, 89,
		18, -75, 18, -75, 18, -75, 18, -75,
		-64, 64, -64, 64, -64, 64, -64, 64,
		64, -64, 64, -64, 64, -64, 64, -64,
		-75, -18, -75, -18, -75, -18, -75, -18,
		89, -50, 89, -50, 89, -50, 89, -50,
		83, -36, 83, -36, 83, -36, 83, -36,
		-36, 83, -36, 83, -36, 83, -36, 83,
		-83, 36, -83, 36, -83, 36, -83, 36,
		89, -75, 89, -75, 89, -75, 89, -75,
		50, -18, 50, -18, 50, -18, 50, -18,
	};
	__m128i sum, sum1, sum2/*, sum3, sum4*/;
	__m128i res0, res1, res2, res3, res4, res5, res6, res7;
	__m128i res01, res23, res45, res67, res02, res0123, res46, res4567, res04, res0246, res0145, res0_to_7;
	__m128i even0, even1, even2, even3, odd0, odd1, odd2, odd3, odd01_lo, odd01_hi, odd23_lo, odd23_hi;
	__m128i evenEven0, evenEven1, evenOdd0, evenOdd1;
	__m128i trans0, trans1, trans2, trans3, trans4/*, trans5, trans6, trans7*/, trans01, trans23, trans45, trans67;
	__m128i trans02, trans0123;
	__m128i xmm_offset;
	EB_ALIGN(16) EB_S16 * TransformIntrinConst = transformIntrinConst_8x8;
	EB_U32 shift;

	res0 = _mm_loadu_si128((__m128i *)residual);
	res1 = _mm_loadu_si128((__m128i *)(residual + srcStride));
	res2 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
	res3 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));
	residual += (srcStride << 2);
	res4 = _mm_loadu_si128((__m128i *)residual);
	res5 = _mm_loadu_si128((__m128i *)(residual + srcStride));
	res6 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
	res7 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));

	MACRO_UNPACK(16, res0, res1, res2, res3, res4, res5, res6, res7, res01, res23, res45, res67)
	MACRO_UNPACK(32, res0, res2, res01, res23, res4, res6, res45, res67, res02, res0123, res46, res4567)
	MACRO_UNPACK(64, res0, res4, res02, res46, res01, res45, res0123, res4567, res04, res0246, res0145, res0_to_7)
	MACRO_CALC_EVEN_ODD(res0, res04, res02, res0246, res01, res0145, res0123, res0_to_7)

	evenEven0 = _mm_add_epi16(even0, even3);
	evenEven1 = _mm_add_epi16(even1, even2);
	evenOdd0 = _mm_sub_epi16(even0, even3);
	evenOdd1 = _mm_sub_epi16(even1, even2);

	shift = 4 - bitIncrement;
	trans0 = _mm_slli_epi16(_mm_add_epi16(evenEven0, evenEven1), shift);
	trans4 = _mm_slli_epi16(_mm_sub_epi16(evenEven0, evenEven1), shift);

	xmm_offset = _mm_slli_epi32(_mm_set1_epi32(0x00000002), bitIncrement);
	shift = bitIncrement + 2;

	trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_83_36)), _mm_unpacklo_epi16(evenOdd0, evenOdd1)), xmm_offset), shift),
	_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_83_36)), _mm_unpackhi_epi16(evenOdd0, evenOdd1)), xmm_offset), shift));

	//trans6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_36_N83)), _mm_unpacklo_epi16(evenOdd0, evenOdd1)), xmm_offset), shift),
	//	_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_36_N83)), _mm_unpackhi_epi16(evenOdd0, evenOdd1)), xmm_offset), shift));

	// TransformCoefficients 1, 3, 5, 7
	odd01_lo = _mm_unpacklo_epi16(odd0, odd1);
	odd01_hi = _mm_unpackhi_epi16(odd0, odd1);
	odd23_lo = _mm_unpacklo_epi16(odd2, odd3);
	odd23_hi = _mm_unpackhi_epi16(odd2, odd3);

	MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans1, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_89_75, TRANS8x8_OFFSET_50_18, shift)
    MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans3, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_75_N18, TRANS8x8_OFFSET_N89_N50, shift)
    //MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans5, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_50_N89, TRANS8x8_OFFSET_18_75, shift)
    //MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans7, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_18_N50, TRANS8x8_OFFSET_75_N89, shift)

	MACRO_UNPACK(32, trans0, trans1, trans2, trans3, trans4/*, trans5, trans6, trans7*/, trans1, trans1, trans1, trans01, trans23, trans45, trans67)
	MACRO_UNPACK_V2(64, trans0, trans2, trans01, trans23, trans4, trans0, /*trans6,*/ trans45, trans67, trans02, trans0123)

    xmm_offset = _mm_loadu_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_256));

	MACRO_TRANS_8MAC_PF_N2(trans0, trans02, trans01, trans0123, trans4, trans45, trans45, trans45, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64, 9, _mm_storeu_si128, transformCoefficients, 0)
	MACRO_TRANS_8MAC_PF_N2(trans0, trans02, trans01, trans0123, trans4, trans45, trans45, trans45, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_89_75, TRANS8x8_OFFSET_50_18, TRANS8x8_OFFSET_N18_N50, TRANS8x8_OFFSET_N75_N89, 9, _mm_storeu_si128, transformCoefficients, (dstStride))
	MACRO_TRANS_8MAC_PF_N2(trans0, trans02, trans01, trans0123, trans4, trans45, trans45, trans45, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_83_36, TRANS8x8_OFFSET_N36_N83, TRANS8x8_OFFSET_N83_N36, TRANS8x8_OFFSET_36_83, 9, _mm_storeu_si128, transformCoefficients, (2 * dstStride))
	MACRO_TRANS_8MAC_PF_N2(trans0, trans02, trans01, trans0123, trans4, trans45, trans45, trans45, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_75_N18, TRANS8x8_OFFSET_N89_N50, TRANS8x8_OFFSET_50_89, TRANS8x8_OFFSET_18_N75, 9, _mm_storeu_si128, transformCoefficients, (3 * dstStride))
	//transformCoefficients += 4 * dstStride;
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_64_N64, TRANS8x8_OFFSET_N64_64, TRANS8x8_OFFSET_64_N64, TRANS8x8_OFFSET_N64_64, 9, _mm_storeu_si128, transformCoefficients, 0)
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_50_N89, TRANS8x8_OFFSET_18_75, TRANS8x8_OFFSET_N75_N18, TRANS8x8_OFFSET_89_N50, 9, _mm_storeu_si128, transformCoefficients, (dstStride))
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_36_N83, TRANS8x8_OFFSET_83_N36, TRANS8x8_OFFSET_N36_83, TRANS8x8_OFFSET_N83_36, 9, _mm_storeu_si128, transformCoefficients, (2 * dstStride))
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_18_N50, TRANS8x8_OFFSET_75_N89, TRANS8x8_OFFSET_89_N75, TRANS8x8_OFFSET_50_N18, 9, _mm_storeu_si128, transformCoefficients, (3 * dstStride))

	(void)transformInnerArrayPtr;
}

void PfreqN4Transform8x8_SSE2_INTRIN(
	EB_S16                  *residual,
	const EB_U32             srcStride,
	EB_S16                  *transformCoefficients,
	const EB_U32             dstStride,
	EB_S16                  *transformInnerArrayPtr,
	EB_U32                   bitIncrement)
{
	// Transform8x8 has its own table because the larger table's offset macros exceed 256 (which is maximum macro expansion depth
	// Use a smaller table with values just for Transform8x8.

	EB_ALIGN(16) EB_S16 transformIntrinConst_8x8[] = {
		83, 36, 83, 36, 83, 36, 83, 36,
		36, -83, 36, -83, 36, -83, 36, -83,
		89, 75, 89, 75, 89, 75, 89, 75,
		50, 18, 50, 18, 50, 18, 50, 18,
		75, -18, 75, -18, 75, -18, 75, -18,
		-89, -50, -89, -50, -89, -50, -89, -50,
		50, -89, 50, -89, 50, -89, 50, -89,
		18, 75, 18, 75, 18, 75, 18, 75,
		18, -50, 18, -50, 18, -50, 18, -50,
		75, -89, 75, -89, 75, -89, 75, -89,
		256, 0, 256, 0, 256, 0, 256, 0,
		64, 64, 64, 64, 64, 64, 64, 64,
		-18, -50, -18, -50, -18, -50, -18, -50,
		-75, -89, -75, -89, -75, -89, -75, -89,
		-36, -83, -36, -83, -36, -83, -36, -83,
		-83, -36, -83, -36, -83, -36, -83, -36,
		36, 83, 36, 83, 36, 83, 36, 83,
		50, 89, 50, 89, 50, 89, 50, 89,
		18, -75, 18, -75, 18, -75, 18, -75,
		-64, 64, -64, 64, -64, 64, -64, 64,
		64, -64, 64, -64, 64, -64, 64, -64,
		-75, -18, -75, -18, -75, -18, -75, -18,
		89, -50, 89, -50, 89, -50, 89, -50,
		83, -36, 83, -36, 83, -36, 83, -36,
		-36, 83, -36, 83, -36, 83, -36, 83,
		-83, 36, -83, 36, -83, 36, -83, 36,
		89, -75, 89, -75, 89, -75, 89, -75,
		50, -18, 50, -18, 50, -18, 50, -18,
	};
	__m128i sum, sum1, sum2/*, sum3, sum4*/;
	__m128i res0, res1, res2, res3, res4, res5, res6, res7;
	__m128i res01, res23, res45, res67, res02, res0123, res46, res4567, res04, res0246, res0145, res0_to_7;
	__m128i even0, even1, even2, even3, odd0, odd1, odd2, odd3, odd01_lo, odd01_hi, odd23_lo, odd23_hi;
	__m128i evenEven0, evenEven1, evenOdd0, evenOdd1;
	__m128i trans0, trans1, trans2, trans3, trans4/*, trans5, trans6, trans7*/, trans01, trans23, trans45, trans67;
	__m128i trans02, trans0123;
	__m128i xmm_offset;
	EB_ALIGN(16) EB_S16 * TransformIntrinConst = transformIntrinConst_8x8;
	EB_U32 shift;

	res0 = _mm_loadu_si128((__m128i *)residual);
	res1 = _mm_loadu_si128((__m128i *)(residual + srcStride));
	res2 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
	res3 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));
	residual += (srcStride << 2);
	res4 = _mm_loadu_si128((__m128i *)residual);
	res5 = _mm_loadu_si128((__m128i *)(residual + srcStride));
	res6 = _mm_loadu_si128((__m128i *)(residual + 2 * srcStride));
	res7 = _mm_loadu_si128((__m128i *)(residual + 3 * srcStride));

	MACRO_UNPACK(16, res0, res1, res2, res3, res4, res5, res6, res7, res01, res23, res45, res67)
	MACRO_UNPACK(32, res0, res2, res01, res23, res4, res6, res45, res67, res02, res0123, res46, res4567)
	MACRO_UNPACK(64, res0, res4, res02, res46, res01, res45, res0123, res4567, res04, res0246, res0145, res0_to_7)
	MACRO_CALC_EVEN_ODD(res0, res04, res02, res0246, res01, res0145, res0123, res0_to_7)

	evenEven0 = _mm_add_epi16(even0, even3);
	evenEven1 = _mm_add_epi16(even1, even2);
	evenOdd0 = _mm_sub_epi16(even0, even3);
	evenOdd1 = _mm_sub_epi16(even1, even2);

	shift = 4 - bitIncrement;
	trans0 = _mm_slli_epi16(_mm_add_epi16(evenEven0, evenEven1), shift);
	trans4 = _mm_slli_epi16(_mm_sub_epi16(evenEven0, evenEven1), shift);

	xmm_offset = _mm_slli_epi32(_mm_set1_epi32(0x00000002), bitIncrement);
	shift = bitIncrement + 2;

	trans2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_83_36)), _mm_unpacklo_epi16(evenOdd0, evenOdd1)), xmm_offset), shift),
		_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_83_36)), _mm_unpackhi_epi16(evenOdd0, evenOdd1)), xmm_offset), shift));

	//trans6 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_36_N83)), _mm_unpacklo_epi16(evenOdd0, evenOdd1)), xmm_offset), shift),
	//	_mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(_mm_load_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_36_N83)), _mm_unpackhi_epi16(evenOdd0, evenOdd1)), xmm_offset), shift));

	// TransformCoefficients 1, 3, 5, 7
	odd01_lo = _mm_unpacklo_epi16(odd0, odd1);
	odd01_hi = _mm_unpackhi_epi16(odd0, odd1);
	odd23_lo = _mm_unpacklo_epi16(odd2, odd3);
	odd23_hi = _mm_unpackhi_epi16(odd2, odd3);

	MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans1, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_89_75, TRANS8x8_OFFSET_50_18, shift)
	MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans3, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_75_N18, TRANS8x8_OFFSET_N89_N50, shift)
	//MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans5, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_50_N89, TRANS8x8_OFFSET_18_75, shift)
	//MACRO_TRANS_4MAC_NO_SAVE(odd01_lo, odd01_hi, odd23_lo, odd23_hi, trans7, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_18_N50, TRANS8x8_OFFSET_75_N89, shift)

	MACRO_UNPACK(32, trans0, trans1, trans2, trans3, trans4/*, trans5, trans6, trans7*/, trans1, trans1, trans1, trans01, trans23, trans45, trans67)
	MACRO_UNPACK_V2(64, trans0, trans2, trans01, trans23, trans4, trans0, /*trans6,*/ trans45, trans67, trans02, trans0123)

	xmm_offset = _mm_loadu_si128((__m128i *)(TransformIntrinConst + TRANS8x8_OFFSET_256));

	MACRO_TRANS_8MAC_PF_N4(trans0, trans02, trans01, trans0123, trans4, trans45, trans45, trans45, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64, TRANS8x8_OFFSET_64_64, 9, _mm_storeu_si128, transformCoefficients, 0)
	MACRO_TRANS_8MAC_PF_N4(trans0, trans02, trans01, trans0123, trans4, trans45, trans45, trans45, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_89_75, TRANS8x8_OFFSET_50_18, TRANS8x8_OFFSET_N18_N50, TRANS8x8_OFFSET_N75_N89, 9, _mm_storeu_si128, transformCoefficients, (dstStride))
	//MACRO_TRANS_8MAC_PF_N2(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_83_36, TRANS8x8_OFFSET_N36_N83, TRANS8x8_OFFSET_N83_N36, TRANS8x8_OFFSET_36_83, 9, _mm_storeu_si128, transformCoefficients, (2 * dstStride))
	//MACRO_TRANS_8MAC_PF_N2(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_75_N18, TRANS8x8_OFFSET_N89_N50, TRANS8x8_OFFSET_50_89, TRANS8x8_OFFSET_18_N75, 9, _mm_storeu_si128, transformCoefficients, (3 * dstStride))
	//transformCoefficients += 4 * dstStride;
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_64_N64, TRANS8x8_OFFSET_N64_64, TRANS8x8_OFFSET_64_N64, TRANS8x8_OFFSET_N64_64, 9, _mm_storeu_si128, transformCoefficients, 0)
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_50_N89, TRANS8x8_OFFSET_18_75, TRANS8x8_OFFSET_N75_N18, TRANS8x8_OFFSET_89_N50, 9, _mm_storeu_si128, transformCoefficients, (dstStride))
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_36_N83, TRANS8x8_OFFSET_83_N36, TRANS8x8_OFFSET_N36_83, TRANS8x8_OFFSET_N83_36, 9, _mm_storeu_si128, transformCoefficients, (2 * dstStride))
	//MACRO_TRANS_8MAC(trans0, trans02, trans01, trans0123, trans4, trans46, trans45, trans4567, xmm_offset, TransformIntrinConst, TRANS8x8_OFFSET_18_N50, TRANS8x8_OFFSET_75_N89, TRANS8x8_OFFSET_89_N75, TRANS8x8_OFFSET_50_N18, 9, _mm_storeu_si128, transformCoefficients, (3 * dstStride))

	(void)transformInnerArrayPtr;
}
