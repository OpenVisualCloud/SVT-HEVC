/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAvailability_h
#define EbAvailability_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
extern EB_BOOL isBottomLeftAvailable(
    EB_U32                depth,
    EB_U32                partIndex);

extern EB_BOOL isUpperRightAvailable(
    EB_U32                depth,
    EB_U32                partIndex);
#ifdef __cplusplus
}
#endif
#endif // EbAvailability_h
