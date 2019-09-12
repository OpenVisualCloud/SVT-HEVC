/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifdef __cplusplus
extern "C" {
#endif
#define MACRO_VERTICAL_LUMA_8(ARG1, ARG2, ARG3)\
    _mm_storeu_si128((__m128i *)predictionPtr, _mm_or_si128(_mm_and_si128(ARG1, ARG2), ARG3));\
    ARG1 = _mm_srli_si128(ARG1, 2);\
    _mm_storeu_si128((__m128i *)(predictionPtr + pStride), _mm_or_si128(_mm_and_si128(ARG1, ARG2), ARG3));\
    ARG1 = _mm_srli_si128(ARG1, 2);\
    _mm_storeu_si128((__m128i *)(predictionPtr + 2 * pStride), _mm_or_si128(_mm_and_si128(ARG1, ARG2), ARG3));\
    ARG1 = _mm_srli_si128(ARG1, 2);\
    _mm_storeu_si128((__m128i *)(predictionPtr + 3 * pStride), _mm_or_si128(_mm_and_si128(ARG1, ARG2), ARG3));\
    ARG1 = _mm_srli_si128(ARG1, 2);

#define MACRO_UNPACK(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13)\
    ARG10 = _mm_unpackhi_epi##ARG1(ARG2, ARG3);\
    ARG2  = _mm_unpacklo_epi##ARG1(ARG2, ARG3);\
    ARG11 = _mm_unpackhi_epi##ARG1(ARG4, ARG5);\
    ARG4  = _mm_unpacklo_epi##ARG1(ARG4, ARG5);\
    ARG12 = _mm_unpackhi_epi##ARG1(ARG6, ARG7);\
    ARG6  = _mm_unpacklo_epi##ARG1(ARG6, ARG7);\
    ARG13 = _mm_unpackhi_epi##ARG1(ARG8, ARG9);\
    ARG8  = _mm_unpacklo_epi##ARG1(ARG8, ARG9);

#define MACRO_UNPACK_V2(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11)\
    ARG10 = _mm_unpackhi_epi##ARG1(ARG2, ARG3);\
    ARG2  = _mm_unpacklo_epi##ARG1(ARG2, ARG3);\
    ARG11 = _mm_unpackhi_epi##ARG1(ARG4, ARG5);\
    ARG4  = _mm_unpacklo_epi##ARG1(ARG4, ARG5);\
    ARG6  = _mm_unpacklo_epi##ARG1(ARG6, ARG7);\
    ARG8  = _mm_unpacklo_epi##ARG1(ARG8, ARG9);

#ifdef __cplusplus
}
#endif
