/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#include <stdint.h>
#ifdef _WIN32
#include <stdlib.h>
//#if  (WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
#include <time.h>
#include <windows.h>
//#endif

#elif __linux__
#include <stdio.h>
#include <stdlib.h>
//#if   (LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
#include <sys/time.h>
#include <time.h>
//#endif

#else
#error OS/Platform not supported.
#endif

void EbStartTime(
    uint64_t *Startseconds,
    uint64_t *Startuseconds)
{

#if __linux__ //(LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
    struct timeval start;
    gettimeofday(&start, NULL);
    *Startseconds=start.tv_sec;
    *Startuseconds=start.tv_usec;
#elif _WIN32 //(WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
    *Startseconds = (uint64_t) clock();
    (void) (*Startuseconds);
#else
    (void) (*Startuseconds);
    (void) (*Startseconds);
#endif

}

void EbFinishTime(
    uint64_t *Finishseconds,
    uint64_t *Finishuseconds)
{

#if __linux__ //(LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
    struct timeval finish;
    gettimeofday(&finish, NULL);
    *Finishseconds=finish.tv_sec;
    *Finishuseconds=finish.tv_usec;
#elif _WIN32 //(WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
    *Finishseconds= (uint64_t)clock();
    (void) (*Finishuseconds);
#else
    (void) (*Finishuseconds);
    (void) (*Finishseconds);
#endif

}
void EbComputeOverallElapsedTime(
    uint64_t Startseconds,
    uint64_t Startuseconds,
    uint64_t Finishseconds,
    uint64_t Finishuseconds,
    double *duration)
{
#if __linux__ //(LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
    long   mtime, seconds, useconds;
    seconds  = Finishseconds - Startseconds;
    useconds = Finishuseconds - Startuseconds;
    mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
    *duration = (double)mtime/1000;
    //printf("\nElapsed time: %3.3ld seconds\n", mtime/1000);
#elif _WIN32 //(WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
    //double  duration;
    *duration = (double)(Finishseconds - Startseconds) / CLOCKS_PER_SEC;
    //printf("\nElapsed time: %3.3f seconds\n", *duration);
    (void) (Startuseconds);
    (void) (Finishuseconds);
#else
    (void) (Startuseconds);
    (void) (Startseconds);
    (void) (Finishuseconds);
    (void) (Finishseconds);

#endif

}

void EbSleep(
    uint64_t milliSeconds)
{

    if(milliSeconds) {
#if __linux__
        struct timespec req,rem;
        req.tv_sec=(int32_t)(milliSeconds/1000);
        milliSeconds -= req.tv_sec * 1000;
        req.tv_nsec = milliSeconds * 1000000UL;
        nanosleep(&req,&rem);
#elif _WIN32
        Sleep((DWORD) milliSeconds);
#else
#error OS Not supported
#endif
    }
}

void EbInjector(
    uint64_t processedFrameCount,
    uint32_t injectorFrameRate)
{

#if __linux__
    uint64_t                  currentTimesSeconds = 0;
    uint64_t                  currentTimesuSeconds = 0;
    static uint64_t           startTimesSeconds;
    static uint64_t           startTimesuSeconds;
#elif _WIN32
    static LARGE_INTEGER    startCount;               // this is the start time
    static LARGE_INTEGER    counterFreq;              // performance counter frequency
    LARGE_INTEGER           nowCount;                 // this is the current time
#else
#error OS Not supported
#endif

    double                 injectorInterval  = (double)(1<<16)/(double)injectorFrameRate;     // 1.0 / injector frame rate (in this case, 1.0/encodRate)
    double                  elapsedTime;
    double                  predictedTime;
    int32_t                     bufferFrames = 1;         // How far ahead of time should we let it get
    int32_t                     milliSecAhead;
    static int32_t              firstTime = 0;

    if (firstTime == 0)
    {
        firstTime = 1;

#if __linux__
        EbStartTime((uint64_t*)&startTimesSeconds, (uint64_t*)&startTimesuSeconds);
#elif _WIN32
        QueryPerformanceFrequency(&counterFreq);
        QueryPerformanceCounter(&startCount);
#endif
    }
    else
    {

#if __linux__
        EbFinishTime((uint64_t*)&currentTimesSeconds, (uint64_t*)&currentTimesuSeconds);
        EbComputeOverallElapsedTime(startTimesSeconds, startTimesuSeconds, currentTimesSeconds, currentTimesuSeconds, &elapsedTime);
#elif _WIN32
        QueryPerformanceCounter(&nowCount);
        elapsedTime = (double)(nowCount.QuadPart - startCount.QuadPart) / (double)counterFreq.QuadPart;
#endif

        predictedTime = (processedFrameCount - bufferFrames) * injectorInterval;
        milliSecAhead = (int32_t)(1000 * (predictedTime - elapsedTime ));
        if (milliSecAhead>0)
        {
            //  timeBeginPeriod(1);
            EbSleep(milliSecAhead);
            //  timeEndPeriod (1);
        }
    }
}
