/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTime_h
#define EbTime_h

#include "EbTypes.h"

#define NANOSECS_PER_SEC ((EB_U32)(1000000000L))

void StartTime(EB_U64 *Startseconds, EB_U64 *Startuseconds);
void FinishTime(EB_U64 *Finishseconds, EB_U64 *Finishuseconds);
void ComputeOverallElapsedTime(EB_U64 Startseconds, EB_U64 Startuseconds,EB_U64 Finishseconds, EB_U64 Finishuseconds, double *duration);
void ComputeOverallElapsedTimeMs(EB_U64 Startseconds, EB_U64 Startuseconds, EB_U64 Finishseconds, EB_U64 Finishuseconds, double *duration);
void EbSleep(EB_U64 milliSeconds);
void EbInjector(EB_U64 processedFrameCount, EB_U32 injectorFrameRate);

#endif // EbTime_h
/* File EOF */
