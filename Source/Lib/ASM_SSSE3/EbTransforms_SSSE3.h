/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_SSSE3_h
#define EbTransforms_SSSE3_h

#include "EbTypes.h"
#ifdef __cplusplus
extern "C" {
#endif
    
void QuantizeInvQuantize4x4_SSE3(
    EB_S16          *coeff,
    const EB_U32     coeffStride,
    EB_S16          *quantCoeff,
    EB_S16          *reconCoeff,
    const EB_U32     qFunc,
    const EB_U32     q_offset,
    const EB_S32     shiftedQBits,
    const EB_S32     shiftedFFunc,
    const EB_S32     iq_offset,
    const EB_S32     shiftNum,
    const EB_U32     areaSize,
    EB_U32          *nonzerocoeff);


void PFinvTransform32x32_SSSE3(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 *intermediate, EB_U32 addshift);
void PFinvTransform16x16_SSSE3(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 *intermediate, EB_U32 addshift);

#ifdef __cplusplus
}
#endif
#endif // EbTransforms_SSSE3_h

