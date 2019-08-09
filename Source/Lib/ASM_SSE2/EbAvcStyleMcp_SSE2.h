/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBAVCSTYLEMCP_SSE2_H
#define EBAVCSTYLEMCP_SSE2_H

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void AvcStyleCopy_SSE2(EB_BYTE refPic, EB_U32 srcStride, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_BYTE tempBuf,  EB_U32 fracPos);

#ifdef __cplusplus
}
#endif
#endif //EBAVCSTYLEMCP_H
