/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

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

void EbStartTime(unsigned long long *Startseconds, unsigned long long *Startuseconds) {

#if __linux__ //(LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
    struct timeval start;
    gettimeofday(&start, NULL);
    *Startseconds=start.tv_sec;
    *Startuseconds=start.tv_usec;
#elif _WIN32 //(WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
    *Startseconds = (unsigned long long) clock();
    (void) (*Startuseconds);
#else
    (void) (*Startuseconds);
    (void) (*Startseconds);
#endif

}

void EbFinishTime(unsigned long long *Finishseconds, unsigned long long *Finishuseconds) {

#if __linux__ //(LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
    struct timeval finish;
    gettimeofday(&finish, NULL);
    *Finishseconds=finish.tv_sec;
    *Finishuseconds=finish.tv_usec;
#elif _WIN32 //(WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
    *Finishseconds= (unsigned long long)clock();
    (void) (*Finishuseconds);
#else
    (void) (*Finishuseconds);
    (void) (*Finishseconds);
#endif

}
void EbComputeOverallElapsedTime(unsigned long long Startseconds, unsigned long long Startuseconds,unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration)
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

void EbSleep(unsigned long long milliSeconds) {

    if(milliSeconds) {
#if __linux__     
        struct timespec req,rem;
        req.tv_sec=(int)(milliSeconds/1000);
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

void EbInjector(unsigned long long processedFrameCount, unsigned int injectorFrameRate){

#if __linux__ 
    unsigned long long                  currentTimesSeconds = 0;
    unsigned long long                  currentTimesuSeconds = 0;
    static unsigned long long           startTimesSeconds;
    static unsigned long long           startTimesuSeconds;
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
    int                     bufferFrames = 1;         // How far ahead of time should we let it get
    int                     milliSecAhead;
    static int              firstTime = 0;

    if (firstTime == 0) 
    {  
        firstTime = 1;

#if __linux__
        EbStartTime((unsigned long long*)&startTimesSeconds, (unsigned long long*)&startTimesuSeconds);
#elif _WIN32
        QueryPerformanceFrequency(&counterFreq);
        QueryPerformanceCounter(&startCount);
#endif
    }
    else
    {

#if __linux__
        EbFinishTime((unsigned long long*)&currentTimesSeconds, (unsigned long long*)&currentTimesuSeconds);
        EbComputeOverallElapsedTime(startTimesSeconds, startTimesuSeconds, currentTimesSeconds, currentTimesuSeconds, &elapsedTime);
#elif _WIN32
        QueryPerformanceCounter(&nowCount);
        elapsedTime = (double)(nowCount.QuadPart - startCount.QuadPart) / (double)counterFreq.QuadPart;
#endif

        predictedTime = (processedFrameCount - bufferFrames) * injectorInterval;
        milliSecAhead = (int)(1000 * (predictedTime - elapsedTime ));
        if (milliSecAhead>0)
        {
            //  timeBeginPeriod(1);
            EbSleep(milliSecAhead);
            //  timeEndPeriod (1);
        }
    }
}
