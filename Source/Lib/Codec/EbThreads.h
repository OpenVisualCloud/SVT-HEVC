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

extern    EbMemoryMapEntry        *memoryMap;               // library Memory table
extern    EB_U32                  *memoryMapIndex;          // library memory index
extern    EB_U64                  *totalLibMemory;          // library Memory malloc'd

#ifdef _WIN32
extern    GROUP_AFFINITY           groupAffinity;
extern    EB_U8                    numGroups;
extern    EB_BOOL                  alternateGroups;
#define EB_CREATETHREAD(type, pointer, nElements, pointerClass, threadFunction, threadContext) \
    pointer = EbCreateThread(threadFunction, threadContext); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
        if (nElements % 8 == 0) { \
            *totalLibMemory += (nElements); \
        } \
        else { \
            *totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
        } \
        if(numGroups == 1) {\
            SetThreadAffinityMask(pointer, groupAffinity.Mask);\
        }\
        else if (numGroups == 2 && alternateGroups){ \
            groupAffinity.Group = 1 - groupAffinity.Group; \
            SetThreadGroupAffinity(pointer,&groupAffinity,NULL); \
        } \
        else if (numGroups == 2 && !alternateGroups){ \
            SetThreadGroupAffinity(pointer,&groupAffinity,NULL); \
        } \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libThreadCount++;
#elif defined(__linux__)
#define __USE_GNU
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
extern    cpu_set_t                   groupAffinity;
#define EB_CREATETHREAD(type, pointer, nElements, pointerClass, threadFunction, threadContext) \
    pointer = EbCreateThread(threadFunction, threadContext); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        pthread_setaffinity_np(*((pthread_t*)pointer),sizeof(cpu_set_t),&groupAffinity); \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
		} \
		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libThreadCount++;
#else
#define EB_CREATETHREAD(type, pointer, nElements, pointerClass, threadFunction, threadContext) \
    pointer = EbCreateThread(threadFunction, threadContext); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
		} \
		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libThreadCount++;
#endif


#ifdef __cplusplus
}
#endif
#endif // EbThreads_h
