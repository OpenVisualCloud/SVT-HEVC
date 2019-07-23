/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// Summary:
// EbThreads contains wrappers functions that hide
// platform specific objects such as threads, semaphores,
// and mutexs.  The goal is to eliminiate platform #define
// in the code.

/****************************************
 * Universal Includes
 ****************************************/
#include <stdlib.h>
#include "EbDefinitions.h"
#include "EbThreads.h"

/****************************************
 * Win32 Includes
 ****************************************/
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#else
#error OS/Platform not supported.
#endif // _WIN32
#if PRINTF_TIME
#ifdef _WIN32
#include <time.h>
void printfTime(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
    SVT_LOG("  [%i ms]\t", ((int)clock()));
	vprintf(fmt, args);
	va_end(args);
}
#endif
#endif
/****************************************
 * EbCreateThread
 ****************************************/
EB_HANDLE EbCreateThread(
    void *threadFunction(void *),
    void *threadContext)
{
    EB_HANDLE threadHandle = NULL;

#ifdef _WIN32

    threadHandle = (EB_HANDLE) CreateThread(
                       NULL,                           // default security attributes
                       0,                              // default stack size
                       (LPTHREAD_START_ROUTINE) threadFunction, // function to be tied to the new thread
                       threadContext,                  // context to be tied to the new thread
                       0,                              // thread active when created
                       NULL);                          // new thread ID

#elif __linux__
    
    pthread_attr_t attr;  
    struct sched_param param = {
        .sched_priority = 99
    };
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);

    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    threadHandle = (pthread_t*) malloc(sizeof(pthread_t));
    if (threadHandle != NULL) {
        int ret = pthread_create(
            (pthread_t*)threadHandle,      // Thread handle
            &attr,                       // attributes
            threadFunction,                 // function to be run by new thread
            threadContext);

        if (ret != 0) {
            if (ret == EPERM) {

                pthread_cancel(*((pthread_t*)threadHandle));
                free(threadHandle);

                threadHandle = (pthread_t*)malloc(sizeof(pthread_t));
                if (threadHandle != NULL) {
                    pthread_create(
                        (pthread_t*)threadHandle,      // Thread handle
                        (const pthread_attr_t*)EB_NULL,                        // attributes
                        threadFunction,                 // function to be run by new thread
                        threadContext);
                }
            }
        }
    }
    pthread_attr_destroy(&attr);
#endif // _WIN32

    return threadHandle;
}
/****************************************
 * EbDestroyThread
 ****************************************/
EB_ERRORTYPE EbDestroyThread(
    EB_HANDLE threadHandle)
{
    EB_ERRORTYPE error_return = EB_ErrorNone;

#ifdef _WIN32
    //error_return = TerminateThread((HANDLE) threadHandle, 0) ? EB_ErrorDestroyThreadFailed : EB_ErrorNone;
    error_return = WaitForSingleObject(threadHandle, INFINITE);
#elif __linux__
    //error_return = pthread_cancel(*((pthread_t*) threadHandle)) ? EB_ErrorDestroyThreadFailed : EB_ErrorNone;
    pthread_join(*((pthread_t*) threadHandle), NULL);
    free(threadHandle);
#endif // _WIN32

    return error_return;
}

/***************************************
 * EbCreateSemaphore
 ***************************************/
EB_HANDLE EbCreateSemaphore(
    EB_U32 initialCount,
    EB_U32 maxCount)
{
    EB_HANDLE semaphoreHandle = NULL;
    (void) maxCount;

#ifdef _WIN32
    semaphoreHandle = (EB_HANDLE) CreateSemaphore(
                          NULL,                           // default security attributes
                          initialCount,                   // initial semaphore count
                          maxCount,                       // maximum semaphore count
                          NULL);                          // semaphore is not named
#elif __linux__
    semaphoreHandle = (sem_t*) malloc(sizeof(sem_t));
    sem_init(
        (sem_t*) semaphoreHandle,       // semaphore handle
        0,                              // shared semaphore (not local)
        initialCount);                  // initial count
#endif // _WIN32    

    return semaphoreHandle;
}

/***************************************
 * EbPostSemaphore
 ***************************************/
EB_ERRORTYPE EbPostSemaphore(
    EB_HANDLE semaphoreHandle)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    return_error = ReleaseSemaphore(
                       semaphoreHandle,    // semaphore handle
                       1,                  // amount to increment the semaphore
                       NULL)               // pointer to previous count (optional)
                   ? EB_ErrorSemaphoreUnresponsive : EB_ErrorNone;
#elif __linux__
    return_error = sem_post((sem_t*) semaphoreHandle) ? EB_ErrorSemaphoreUnresponsive : EB_ErrorNone;
#endif // _WIN32

    return return_error;
}

/***************************************
 * EbBlockOnSemaphore
 ***************************************/
EB_ERRORTYPE EbBlockOnSemaphore(
    EB_HANDLE semaphoreHandle)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    return_error = WaitForSingleObject((HANDLE) semaphoreHandle, INFINITE) ? EB_ErrorSemaphoreUnresponsive : EB_ErrorNone;
#elif __linux__
    return_error = sem_wait((sem_t*) semaphoreHandle) ? EB_ErrorSemaphoreUnresponsive : EB_ErrorNone;
#endif // _WIN32

    return return_error;
}

/***************************************
 * EbDestroySemaphore
 ***************************************/
EB_ERRORTYPE EbDestroySemaphore(
    EB_HANDLE semaphoreHandle)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    return_error = CloseHandle((HANDLE) semaphoreHandle) ? EB_ErrorDestroySemaphoreFailed : EB_ErrorNone;
#elif __linux__
    return_error = sem_destroy((sem_t*) semaphoreHandle) ? EB_ErrorDestroySemaphoreFailed : EB_ErrorNone;
    free(semaphoreHandle);
#endif // _WIN32

    return return_error;
}
/***************************************
 * EbCreateMutex
 ***************************************/
EB_HANDLE EbCreateMutex(
    void)
{
    EB_HANDLE mutexHandle = NULL;

#ifdef _WIN32
    mutexHandle = (EB_HANDLE)CreateMutex(
        NULL,                   // default security attributes
        FALSE,                  // FALSE := not initially owned
        NULL);                  // mutex is not named

#elif __linux__

    mutexHandle = (EB_HANDLE)malloc(sizeof(pthread_mutex_t));
    if (mutexHandle != NULL) {
        pthread_mutex_init(
            (pthread_mutex_t*)mutexHandle,
            NULL);                  // default attributes
    }
#endif // _WIN32

    return mutexHandle;
}

/***************************************
 * EbPostMutex
 ***************************************/
EB_ERRORTYPE EbReleaseMutex(
    EB_HANDLE mutexHandle)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    return_error = ReleaseMutex((HANDLE) mutexHandle)? EB_ErrorCreateMutexFailed : EB_ErrorNone;
#elif __linux__
    return_error = pthread_mutex_unlock((pthread_mutex_t*) mutexHandle) ? EB_ErrorCreateMutexFailed : EB_ErrorNone;
#endif // _WIN32

    return return_error;
}

/***************************************
 * EbBlockOnMutex
 ***************************************/
EB_ERRORTYPE EbBlockOnMutex(
    EB_HANDLE mutexHandle)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    return_error = WaitForSingleObject((HANDLE) mutexHandle, INFINITE) ? EB_ErrorMutexUnresponsive : EB_ErrorNone;
#elif __linux__
    return_error = pthread_mutex_lock((pthread_mutex_t*) mutexHandle) ? EB_ErrorMutexUnresponsive : EB_ErrorNone;
#endif // _WIN32

    return return_error;
}

/***************************************
 * EbBlockOnMutexTimeout
 ***************************************/
EB_ERRORTYPE EbBlockOnMutexTimeout(
    EB_HANDLE mutexHandle,
    EB_U32    timeout)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    WaitForSingleObject((HANDLE) mutexHandle, timeout);
#elif __linux__
    return_error = pthread_mutex_lock((pthread_mutex_t*) mutexHandle) ? EB_ErrorMutexUnresponsive : EB_ErrorNone;
    (void) timeout;
#endif // _WIN32

    return return_error;
}

/***************************************
 * EbDestroyMutex
 ***************************************/
EB_ERRORTYPE EbDestroyMutex(
    EB_HANDLE mutexHandle)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

#ifdef _WIN32
    return_error = CloseHandle((HANDLE) mutexHandle) ? EB_ErrorDestroyMutexFailed : EB_ErrorNone;
#elif __linux__
    return_error = pthread_mutex_destroy((pthread_mutex_t*) mutexHandle) ? EB_ErrorDestroyMutexFailed : EB_ErrorNone;
    free(mutexHandle);
#endif // _WIN32

    return return_error;
}
