/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTime_h
#define EbTime_h

void StartTime(unsigned long long *Startseconds, unsigned long long *Startuseconds);
void FinishTime(unsigned long long *Finishseconds, unsigned long long *Finishuseconds);
void ComputeOverallElapsedTime(unsigned long long Startseconds, unsigned long long Startuseconds,unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration);
void ComputeOverallElapsedTimeMs(unsigned long long Startseconds, unsigned long long Startuseconds, unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration);
void EbSleep(unsigned long long milliSeconds);
void EbInjector(unsigned long long processedFrameCount, unsigned int injectorFrameRate);

#endif // EbTime_h
/* File EOF */
