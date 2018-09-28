/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBSEI_h
#define EBSEI_h

#include "EbTypes.h"
#include "EbApiSei.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
extern EB_ERRORTYPE EbVideoUsabilityInfoCtor(
    AppVideoUsabilityInfo_t *vuiPtr);



extern void EbVideoUsabilityInfoCopy(
    AppVideoUsabilityInfo_t *dstVuiPtr,
    AppVideoUsabilityInfo_t *srcVuiPtr);

extern void EbPictureTimeingSeiCtor(
    AppPictureTimingSei_t   *picTimingPtr);

extern void EbBufferingPeriodSeiCtor(
    AppBufferingPeriodSei_t   *bufferingPeriodPtr);

extern void EbRecoveryPointSeiCtor(
    AppRecoveryPoint_t   *recoveryPointSeiPtr);

extern EB_U32 GetPictureTimingSEILength(
    AppPictureTimingSei_t      *picTimingSeiPtr,
    AppVideoUsabilityInfo_t    *vuiPtr);

extern EB_U32 GetBufPeriodSEILength(
    AppBufferingPeriodSei_t    *bufferingPeriodPtr,
    AppVideoUsabilityInfo_t    *vuiPtr);

extern EB_U32 GetRecoveryPointSEILength(
    AppRecoveryPoint_t    *recoveryPointSeiPtr);
#ifdef __cplusplus
}
#endif
#endif // EBSEI_h
