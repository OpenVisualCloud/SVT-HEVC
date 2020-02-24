/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbThreads_h
#define EbThreads_h

#include "EbDefinitions.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
// Create wrapper functions that hide thread calls,
// semaphores, mutex, etc. These wrappers also hide
// platform specific implementations of these objects.

/**************************************
 * Threads
 **************************************/
extern EB_HANDLE EbCreateThread(
    void *threadFunction(void *),
    void *threadContext);
extern EB_ERRORTYPE EbDestroyThread(
    EB_HANDLE threadHandle);

/**************************************
 * Semaphores
 **************************************/
extern EB_HANDLE EbCreateSemaphore(
    EB_U32 initialCount,
    EB_U32 maxCount);
extern EB_ERRORTYPE EbPostSemaphore(
    EB_HANDLE semaphoreHandle);
extern EB_ERRORTYPE EbBlockOnSemaphore(
    EB_HANDLE semaphoreHandle);
extern EB_ERRORTYPE EbDestroySemaphore(
    EB_HANDLE semaphoreHandle);
/**************************************
 * Mutex
 **************************************/
extern EB_HANDLE EbCreateMutex(
    void);
extern EB_ERRORTYPE EbReleaseMutex(
    EB_HANDLE mutexHandle);
extern EB_ERRORTYPE EbBlockOnMutex(
    EB_HANDLE mutexHandle);
extern EB_ERRORTYPE EbBlockOnMutexTimeout(
    EB_HANDLE mutexHandle,
    EB_U32 timeout);
extern EB_ERRORTYPE EbDestroyMutex(
    EB_HANDLE mutexHandle);


#ifdef _WIN32
extern    GROUP_AFFINITY           groupAffinity;
extern    EB_U8                    numGroups;
extern    EB_BOOL                  alternateGroups;
#define EB_CREATE_THREAD(pointer, threadFunction, threadContext) \
    do { \
       pointer = EbCreateThread(threadFunction, threadContext); \
       EB_ADD_MEM(pointer, 1, EB_THREAD); \
       if(numGroups == 1) { \
            SetThreadAffinityMask(pointer, groupAffinity.Mask); \
        } \
        else if (numGroups == 2 && alternateGroups){ \
            groupAffinity.Group = 1 - groupAffinity.Group; \
            SetThreadGroupAffinity(pointer,&groupAffinity,NULL); \
        } \
        else if (numGroups == 2 && !alternateGroups){ \
            SetThreadGroupAffinity(pointer,&groupAffinity,NULL); \
        } \
    } while (0)
#elif defined(__linux__)
#ifndef __cplusplus
#define __USE_GNU
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <pthread.h>
extern    cpu_set_t                   groupAffinity;
#define EB_CREATE_THREAD(pointer, threadFunction, threadContext) \
    do { \
        pointer = EbCreateThread(threadFunction, threadContext); \
        EB_ADD_MEM(pointer, 1, EB_THREAD); \
        pthread_setaffinity_np(*((pthread_t*)pointer),sizeof(cpu_set_t),&groupAffinity); \
    } while (0)
#else
#define EB_CREATE_THREAD(pointer, threadFunction, threadContext) \
    do { \
        pointer = EbCreateThread(threadFunction, threadContext); \
        EB_ADD_MEM(pointer, 1, EB_THREAD); \
    } while (0)
#endif

#define EB_DESTROY_THREAD(pointer) \
    do { \
        if (pointer) { \
            EbDestroyThread(pointer); \
            EB_REMOVE_MEM_ENTRY(pointer, EB_THREAD); \
            pointer = NULL; \
        } \
    } while (0);

#define EB_CREATE_THREAD_ARRAY(pa, count, threadFunction, threadContext) \
    do { \
        EB_ALLOC_PTR_ARRAY(pa, count); \
        for (EB_U32 i = 0; i < count; i++) \
            EB_CREATE_THREAD(pa[i], threadFunction, threadContext[i]); \
    } while (0)

#define EB_DESTROY_THREAD_ARRAY(pa, count) \
    do { \
        if (pa) { \
            for (EB_U32 i = 0; i < count; i++) \
                EB_DESTROY_THREAD(pa[i]); \
            EB_FREE_PTR_ARRAY(pa, count); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif // EbThreads_h
