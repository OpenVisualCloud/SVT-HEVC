/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#ifndef EbPerFramePrediction_h
#define EbPerFramePrediction_h

#include "EbSequenceControlSet.h"
#ifdef __cplusplus
extern "C" {
#endif
EB_U64 predictBits(
    SequenceControlSet_t *sequenceControlSetPtr,
    EncodeContext_t *encodeContextPtr,
    HlRateControlHistogramEntry_t *hlRateControlHistogramPtrTemp,
    EB_U32 qp);
#ifdef __cplusplus
}
#endif        
#endif // EbPerFramePrediction_h