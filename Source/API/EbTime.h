/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#include <stdint.h>
#ifndef EbTime_h
#define EbTime_h

void EbStartTime(uint64_t *Startseconds, uint64_t *Startuseconds);
void EbFinishTime(uint64_t *Finishseconds, uint64_t *Finishuseconds);
void EbComputeOverallElapsedTime(uint64_t Startseconds, uint64_t Startuseconds,uint64_t Finishseconds, uint64_t Finishuseconds, double *duration);
void EbAppStartTime(uint64_t *Startseconds, uint64_t *Startuseconds);
void EbAppFinishTime(uint64_t *Finishseconds, uint64_t *Finishuseconds);
void EbAppComputeOverallElapsedTime(uint64_t Startseconds, uint64_t Startuseconds,uint64_t Finishseconds, uint64_t Finishuseconds, double *duration);
void EbComputeOverallElapsedTimeMs(uint64_t Startseconds, uint64_t Startuseconds, uint64_t Finishseconds, uint64_t Finishuseconds, double *duration);
void EbSleep(uint64_t milliSeconds);
void EbInjector(uint64_t processedFrameCount, uint32_t injectorFrameRate);

#endif // EbTime_h
/* File EOF */
