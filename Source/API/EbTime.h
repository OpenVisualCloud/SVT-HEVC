/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTime_h
#define EbTime_h

void EbStartTime(unsigned long long *Startseconds, unsigned long long *Startuseconds);
void EbFinishTime(unsigned long long *Finishseconds, unsigned long long *Finishuseconds);
void EbComputeOverallElapsedTime(unsigned long long Startseconds, unsigned long long Startuseconds,unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration);
void EbComputeOverallElapsedTimeMs(unsigned long long Startseconds, unsigned long long Startuseconds, unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration);
void EbSleep(unsigned long long milliSeconds);
void EbInjector(unsigned long long processedFrameCount, unsigned int injectorFrameRate);

#endif // EbTime_h
/* File EOF */
