/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#ifndef EbMalloc_h
#define EbMalloc_h
#include <stdlib.h>
#include <stdio.h>

#ifndef NDEBUG
#define DEBUG_MEMORY_USAGE
#endif

typedef enum EB_PTRType {
    EB_N_PTR = 0,         // malloc'd pointer
    EB_C_PTR = 1,         // calloc'd pointer
    EB_A_PTR = 2,         // malloc'd pointer aligned
    EB_MUTEX = 3,         // mutex
    EB_SEMAPHORE = 4,     // semaphore
    EB_THREAD = 5,        // thread handle
    EB_PTR_TYPE_TOTAL,
} EB_PTRType;

#ifdef DEBUG_MEMORY_USAGE
void EbAddMemEntry(void* ptr,  EB_PTRType type, size_t count, const char* file, uint32_t line);
void EbRemoveMemEntry(void* ptr, EB_PTRType type);

#define EB_ADD_MEM_ENTRY(p, type, count) \
    EbAddMemEntry(p, type, count, __FILE__, __LINE__)

#define EB_REMOVE_MEM_ENTRY(p, type) \
    EbRemoveMemEntry(p, type);

#else

#define EB_ADD_MEM_ENTRY(p, type, count)
#define EB_REMOVE_MEM_ENTRY(p, type)

#endif //DEBUG_MEMORY_USAGE

#define EB_NO_THROW_ADD_MEM(p, size, type) \
    do { \
        if (!p) { \
            fprintf(stderr,"allocate memory failed, at %s, L%d\n", __FILE__, __LINE__); \
        } else { \
            EB_ADD_MEM_ENTRY(p, type, size); \
        } \
    } while (0)

#define EB_CHECK_MEM(p) \
    do { \
        if (!p) {\
            return EB_ErrorInsufficientResources; \
        } \
    } while (0)

#define EB_ADD_MEM(p, size, type) \
    do { \
        EB_NO_THROW_ADD_MEM(p, size, type); \
        EB_CHECK_MEM(p); \
    } while (0)

#define EB_NO_THROW_MALLOC(pointer, size) \
    do { \
        void* p = malloc(size); \
        EB_NO_THROW_ADD_MEM(p, size, EB_N_PTR); \
        *(void**)&(pointer) = p; \
    } while (0)

#define EB_MALLOC(pointer, size) \
    do { \
        EB_NO_THROW_MALLOC(pointer, size); \
        EB_CHECK_MEM(pointer); \
    } while (0)

#define EB_NO_THROW_CALLOC(pointer, count, size) \
    do { \
        void* p = calloc(count, size); \
        EB_NO_THROW_ADD_MEM(p, count * size, EB_C_PTR); \
        *(void**)&(pointer) = p; \
    } while (0)

#define EB_CALLOC(pointer, count, size) \
    do { \
        EB_NO_THROW_CALLOC(pointer, count, size); \
        EB_CHECK_MEM(pointer); \
    } while (0)

#define EB_FREE(pointer) \
    do {\
        free(pointer); \
        EB_REMOVE_MEM_ENTRY(pointer, EB_N_PTR); \
        pointer = NULL; \
    } while (0)


#define EB_MALLOC_ARRAY(pa, count) \
    do {\
        size_t size = sizeof(*(pa)); \
        EB_MALLOC(pa, (count)*size); \
    } while (0)

#define EB_CALLOC_ARRAY(pa, count) \
    do {\
        size_t size = sizeof(*(pa)); \
        EB_CALLOC(pa, count, size); \
    } while (0)

#define EB_FREE_ARRAY(pa) \
    EB_FREE(pa);


#define EB_ALLOC_PTR_ARRAY(pa, count) \
    do {\
        size_t size = sizeof(*(pa)); \
        EB_CALLOC(pa, count, size); \
    } while (0)

#define EB_FREE_PTR_ARRAY(pa, count) \
    do {\
        if (pa) { \
            uint32_t i; \
            for (i = 0; i < count; i++) { \
                EB_FREE(pa[i]); \
            } \
            EB_FREE(pa); \
        } \
    } while (0)


#define EB_MALLOC_2D(p2d, width, height) \
    do {\
        EB_MALLOC_ARRAY(p2d, width); \
        EB_MALLOC_ARRAY(p2d[0], width * height); \
        for (uint32_t w = 1; w < width; w++) { \
            p2d[w] = p2d[0] + w * height; \
        } \
    } while (0)

#define EB_CALLOC_2D(p2d, width, height) \
    do {\
        EB_MALLOC_ARRAY(p2d, width); \
        EB_CALLOC_ARRAY(p2d[0], width * height); \
        for (uint32_t w = 1; w < width; w++) { \
             p2d[w] = p2d[0] + w * height; \
        } \
    } while (0)

#define EB_FREE_2D(p2d) \
    do { \
        if (p2d) EB_FREE_ARRAY(p2d[0]); \
        EB_FREE_ARRAY(p2d); \
    } while (0)


#ifdef _WIN32
#define EB_MALLOC_ALIGNED(pointer, size) \
    do {\
        void* p = _aligned_malloc(size,ALVALUE); \
        EB_ADD_MEM(p, size, EB_A_PTR); \
        *(void**)&(pointer) = p; \
    } while (0)

#define EB_FREE_ALIGNED(pointer) \
    do { \
        _aligned_free(pointer); \
        EB_REMOVE_MEM_ENTRY(pointer, EB_A_PTR); \
        pointer = NULL; \
    } while (0)
#else
#define EB_MALLOC_ALIGNED(pointer, size) \
    do {\
        if (posix_memalign((void**)(&(pointer)), ALVALUE, size) != 0) \
            return EB_ErrorInsufficientResources; \
        EB_ADD_MEM(pointer, size, EB_A_PTR); \
    } while (0)

#define EB_FREE_ALIGNED(pointer) \
    do { \
        free(pointer); \
        EB_REMOVE_MEM_ENTRY(pointer, EB_A_PTR); \
        pointer = NULL; \
    } while (0)
#endif


#define EB_MALLOC_ALIGNED_ARRAY(pa, count) \
    EB_MALLOC_ALIGNED(pa, sizeof(*(pa))*(count))

#define EB_CALLOC_ALIGNED_ARRAY(pa, count) \
    do { \
        size_t size = sizeof(*(pa))*(count); \
        EB_MALLOC_ALIGNED(pa, size); \
        memset(pa, 0, size); \
    } while (0)

#define EB_FREE_ALIGNED_ARRAY(pa) \
    EB_FREE_ALIGNED(pa)


void EbPrintMemoryUsage();
void EbIncreaseComponentCount();
void EbDecreaseComponentCount();


#endif //EbMalloc_h
