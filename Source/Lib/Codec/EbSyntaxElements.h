/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSyntaxElements_h
#define EbSyntaxElements_h
#ifdef __cplusplus
extern "C" {
#endif
typedef enum NalRefIdc {
    NAL_REF_IDC_PRIORITY_LOWEST = 0,
    NAL_REF_IDC_PRIORITY_LOW,
    NAL_REF_IDC_PRIORITY_HIGH,
    NAL_REF_IDC_PRIORITY_HIGHEST
} NalRefIdc;


#define EB_INTRA_PLANAR     0
#define EB_INTRA_DC         1
#define EB_INTRA_MODE_2     2
#define EB_INTRA_MODE_3     3
#define EB_INTRA_MODE_4     4
#define EB_INTRA_MODE_5     5
#define EB_INTRA_MODE_6     6
#define EB_INTRA_MODE_7     7
#define EB_INTRA_MODE_8     8
#define EB_INTRA_MODE_9     9
#define EB_INTRA_HORIZONTAL 10
#define EB_INTRA_MODE_11    11
#define EB_INTRA_MODE_12    12
#define EB_INTRA_MODE_13    13
#define EB_INTRA_MODE_14    14
#define EB_INTRA_MODE_15    15
#define EB_INTRA_MODE_16    16
#define EB_INTRA_MODE_17    17
#define EB_INTRA_MODE_18    18
#define EB_INTRA_MODE_19    19
#define EB_INTRA_MODE_20    20
#define EB_INTRA_MODE_21    21
#define EB_INTRA_MODE_22    22
#define EB_INTRA_MODE_23    23
#define EB_INTRA_MODE_24    24
#define EB_INTRA_MODE_25    25
#define EB_INTRA_VERTICAL   26
#define EB_INTRA_MODE_27    27
#define EB_INTRA_MODE_28    28
#define EB_INTRA_MODE_29    29
#define EB_INTRA_MODE_30    30
#define EB_INTRA_MODE_31    31
#define EB_INTRA_MODE_32    32
#define EB_INTRA_MODE_33    33
#define EB_INTRA_MODE_34    34
#define EB_INTRA_MODE_35    35 
#define EB_INTRA_MODE_4x4   36 
#define EB_INTRA_MODE_INVALID ~0u

#define EB_INTRA_CHROMA_PLANAR      0
#define EB_INTRA_CHROMA_VERTICAL    1
#define EB_INTRA_CHROMA_HORIZONTAL  2
#define EB_INTRA_CHROMA_DC          3
#define EB_INTRA_CHROMA_DM          4
#define EB_INTRA_CHROMA_TOTAL_COUNT 5
#define EB_INTRA_CHROMA_INVALID     ~0u

// SAO
#define SE_SAO_LUMA_OFFSET_COUNT        4
#define SE_SAO_CHROMA_OFFSET_COUNT      4
#define CTB_SPLIT_FLAG_MAX_COUNT        4

#ifdef __cplusplus
}
#endif
#endif // EbSyntaxElements_h