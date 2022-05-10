/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#include <stdio.h>
#include <inttypes.h>


#include "EbMalloc.h"
#include "EbThreads.h"

#ifdef DEBUG_MEMORY_USAGE

static EB_HANDLE gMallocMutex;

#ifdef _WIN32

#include <windows.h>

static INIT_ONCE gMallocOnce = INIT_ONCE_STATIC_INIT;

BOOL CALLBACK CreateMallocMutex (
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *lpContext)
{
    (void)InitOnce;
    (void)Parameter;
    (void)lpContext;
    gMallocMutex = EbCreateMutex();
    return TRUE;
}

static EB_HANDLE GetMallocMutex()
{
    InitOnceExecuteOnce(&gMallocOnce, CreateMallocMutex, NULL, NULL);
    return gMallocMutex;
}
#else
#include <pthread.h>
static void CreateMallocMutex()
{
    gMallocMutex = EbCreateMutex();
}

static pthread_once_t gMallocOnce = PTHREAD_ONCE_INIT;

static EB_HANDLE GetMallocMutex()
{
    pthread_once(&gMallocOnce, CreateMallocMutex);
    return gMallocMutex;
}
#endif // _WIN32

//hash function to speedup etnry search
uint32_t hash(void* p)
{
#define MASK32 ((((uint64_t)1)<<32)-1)

    uint64_t v = (uint64_t)p;
    uint64_t low32 = v & MASK32;
    return (uint32_t)((v >> 32) + low32);
}

typedef struct MemoryEntry{
    void* ptr;
    EB_PTRType type;
    size_t count;
    const char* file;
    uint32_t line;
} MemoryEntry;

//+1 to get a better hash result
#define MEM_ENTRY_SIZE (16 * 1024 * 1024 + 1)

MemoryEntry gMemEntry[MEM_ENTRY_SIZE];

#define TO_INDEX(v) ((v) % MEM_ENTRY_SIZE)
static EB_BOOL gAddMemEntryWarning = EB_TRUE;
static EB_BOOL gRemoveMemEntryWarning = EB_TRUE;


/*********************************************************************************
*
* @brief
*  compare and update current memory entry.
*
* @param[in] e
*  current memory entry.
*
* @param[in] param
*  param you set to ForEachMemEntry
*
*
* @returns  return EB_TRUE if you want get early exit in ForEachMemEntry
*
s*
********************************************************************************/

typedef EB_BOOL (*Predicate)(MemoryEntry* e, void* param);

/*********************************************************************************
*
* @brief
*  Loop through mem entries.
*
* @param[in] bucket
*  the hash bucket
*
* @param[in] start
*  loop start position
*
* @param[in] pred
*  return EB_TRUE if you want early exit
*
* @param[out] param
*  param send to pred.
*
* @returns  return EB_TRUE if we got early exit.
*
*
********************************************************************************/
static EB_BOOL ForEachHashEntry(MemoryEntry* bucket, uint32_t start, Predicate pred, void* param)
{

    uint32_t s = TO_INDEX(start);
    uint32_t i = s;

    do {
        MemoryEntry* e = bucket + i;
        if (pred(e, param)) {
            return EB_TRUE;
        }
        i++;
        i = TO_INDEX(i);
    } while (i != s);
     return EB_FALSE;
}

static EB_BOOL ForEachMemEntry(uint32_t start, Predicate pred, void* param)
{
    EB_BOOL ret;
    EB_HANDLE m = GetMallocMutex();
    EbBlockOnMutex(m);
    ret = ForEachHashEntry(gMemEntry, start, pred, param);
    EbReleaseMutex(m);
    return ret;
}

static const char* ResourceTypeName(EB_PTRType type)
{
    static const char *name[EB_PTR_TYPE_TOTAL] = {"malloced memory", "calloced memory", "aligned memory", "mutex", "semaphore", "thread"};
    return name[type];
}

static EB_BOOL AddMemEntry(MemoryEntry* e, void* param)
{
    MemoryEntry* newItem = (MemoryEntry*)param;
    if (!e->ptr) {
        EB_MEMCPY(e, newItem, sizeof(MemoryEntry));
        return EB_TRUE;
    }
    return EB_FALSE;
}


void EbAddMemEntry(void* ptr,  EB_PTRType type, size_t count, const char* file, uint32_t line)
{
    MemoryEntry item;
    item.ptr = ptr;
    item.type = type;
    item.count = count;
    item.file = file;
    item.line = line;
    if (ForEachMemEntry(hash(ptr), AddMemEntry, &item))
        return;
    if (gAddMemEntryWarning) {
        fprintf(stderr, "SVT: can't add memory entry.\r\n");
        fprintf(stderr, "SVT: You have memory leak or you need increase MEM_ENTRY_SIZE\r\n");
        gAddMemEntryWarning = EB_FALSE;
    }
}

static EB_BOOL RemoveMemEntry(MemoryEntry* e, void* param)
{
    MemoryEntry* item = (MemoryEntry*)param;
    if (e->ptr == item->ptr) {
        if (e->type == item->type) {
            e->ptr = NULL;
            return EB_TRUE;
        } else if (e->type == EB_C_PTR && item->type == EB_N_PTR) {
            //speical case, we use EB_FREE to free calloced memory
            e->ptr = NULL;
            return EB_TRUE;
        }
    }
    return EB_FALSE;
}

void EbRemoveMemEntry(void* ptr, EB_PTRType type)
{
    if (!ptr)
        return;
    MemoryEntry item;
    item.ptr = ptr;
    item.type = type;
    if (ForEachMemEntry(hash(ptr), RemoveMemEntry, &item))
        return;
    if (gRemoveMemEntryWarning) {
        fprintf(stderr, "SVT: something wrong. you freed a unallocated resource %p, type = %s\r\n", ptr, ResourceTypeName(type));
        gRemoveMemEntryWarning = EB_FALSE;
    }
}

typedef struct MemSummary {
    uint64_t amount[EB_PTR_TYPE_TOTAL];
    uint32_t occupied;
} MemSummary;

static EB_BOOL CountMemEntry(MemoryEntry* e, void* param)
{
    MemSummary* sum = (MemSummary*)param;
    if (e->ptr) {
        sum->amount[e->type] += e->count;
        sum->occupied++;
    }
    return EB_FALSE;
}

static void GetMemoryUsageAndScale(uint64_t amount, double* usage, char* scale)
{
    char scales[] = { ' ', 'K', 'M', 'G' };
    size_t i;
    uint64_t v;
    for (i = 1; i < sizeof(scales); i++) {
        v = (uint64_t)1 << (i * 10);
        if (amount < v)
            break;
    }
    i--;
    v = (uint64_t)1 << (i * 10);
    *usage = (double)amount / v;
    *scale = scales[i];
}

//this need more memory and cpu
#define PROFILE_MEMORY_USAGE
#ifdef PROFILE_MEMORY_USAGE

//if we use a static array here, this size + sizeof(gMemEntry) will exceed max size allowed on windows.
static MemoryEntry* gProfileEntry;

uint32_t GetHashLocation(FILE* f, int line) {
#define MASK32 ((((uint64_t)1)<<32)-1)

    uint64_t v = (uint64_t)f;
    uint64_t low32 = v & MASK32;
    return (uint32_t)((v >> 32) + low32 + line);
}

static EB_BOOL AddLocation(MemoryEntry* e, void* param) {
    MemoryEntry* newItem = (MemoryEntry*)param;
    if (!e->ptr) {
        *e = *newItem;
        return EB_TRUE;
    } else if (e->file == newItem->file && e->line == newItem->line) {
        e->count += newItem->count;
        return EB_TRUE;
    }
    //to next position.
    return EB_FALSE;
}

static EB_BOOL CollectMem(MemoryEntry* e, void* param) {
    EB_PTRType type = *(EB_PTRType*)param;
    if (e->ptr && e->type == type) {
        ForEachHashEntry(gProfileEntry, 0, AddLocation, e);
    }
    //Loop entire bucket.
    return EB_FALSE;
}

static int CompareCount(const void* a,const void* b)
{
    const MemoryEntry* pa = (const MemoryEntry*)a;
    const MemoryEntry* pb = (const MemoryEntry*)b;
    if (pb->count < pa->count) return -1;
    if (pb->count == pa->count) return 0;
    return 1;
}

static void PrintTop10Llocations() {
    EB_HANDLE m = GetMallocMutex();
    EB_PTRType type = EB_N_PTR;
    EbBlockOnMutex(m);
    gProfileEntry = (MemoryEntry*)calloc(MEM_ENTRY_SIZE, sizeof(MemoryEntry));
    if (!gProfileEntry) {
        fprintf(stderr, "not enough memory for memory profile");
        EbReleaseMutex(m);
        return;
    }

    ForEachHashEntry(gMemEntry, 0, CollectMem, &type);
    qsort(gProfileEntry, MEM_ENTRY_SIZE, sizeof(MemoryEntry), CompareCount);

    printf("top 10 %s locations:\r\n", ResourceTypeName(type));
    for (int i = 0; i < 10; i++) {
        double usage;
        char scale;
        MemoryEntry* e = gProfileEntry + i;
        GetMemoryUsageAndScale(e->count, &usage, &scale);
        printf("(%.2lf %cB): %s:%d\r\n", usage, scale, e->file, e->line);
    }
    free(gProfileEntry);
    EbReleaseMutex(m);
}
#endif //PROFILE_MEMORY_USAGE

static int gComponentCount;

#endif //DEBUG_MEMORY_USAGE

void EbPrintMemoryUsage()
{
#ifdef DEBUG_MEMORY_USAGE
    MemSummary sum;
    double fulless;
    double usage;
    char scale;
    memset(&sum, 0, sizeof(MemSummary));

    ForEachMemEntry(0, CountMemEntry, &sum);
    printf("SVT Memory Usage:\r\n");
    GetMemoryUsageAndScale(sum.amount[EB_N_PTR] + sum.amount[EB_C_PTR] + sum.amount[EB_A_PTR], &usage, &scale);
    printf("    total allocated memory:       %.2lf %cB\r\n", usage, scale);
    GetMemoryUsageAndScale(sum.amount[EB_N_PTR], &usage, &scale);
    printf("        malloced memory:          %.2lf %cB\r\n", usage, scale);
    GetMemoryUsageAndScale(sum.amount[EB_C_PTR], &usage, &scale);
    printf("        callocated memory:        %.2lf %cB\r\n", usage, scale);
    GetMemoryUsageAndScale(sum.amount[EB_A_PTR], &usage, &scale);
    printf("        allocated aligned memory: %.2lf %cB\r\n", usage, scale);

    printf("    mutex count: %d\r\n", (int)sum.amount[EB_MUTEX]);
    printf("    semaphore count: %d\r\n", (int)sum.amount[EB_SEMAPHORE]);
    printf("    thread count: %d\r\n", (int)sum.amount[EB_THREAD]);
    fulless = (double)sum.occupied / MEM_ENTRY_SIZE;
    printf("    hash table fulless: %f, hash bucket is %s\r\n", fulless, fulless < .3 ? "healthy":"too full" );
#ifdef PROFILE_MEMORY_USAGE
    PrintTop10Llocations();
#endif
#endif
}


void EbIncreaseComponentCount()
{
#ifdef DEBUG_MEMORY_USAGE
    EB_HANDLE m = GetMallocMutex();
    EbBlockOnMutex(m);
    gComponentCount++;
    EbReleaseMutex(m);
#endif
}

#ifdef DEBUG_MEMORY_USAGE
static EB_BOOL PrintLeak(MemoryEntry* e, void* param)
{
    if (e->ptr) {
        EB_BOOL* leaked = (EB_BOOL*)param;
        *leaked = EB_TRUE;
        fprintf(stderr, "SVT: %s leaked at %s:L%d\r\n", ResourceTypeName(e->type), e->file, e->line);
    }
    //loop through all items
    return EB_FALSE;
}
#endif

void EbDecreaseComponentCount()
{
#ifdef DEBUG_MEMORY_USAGE
    EB_HANDLE m = GetMallocMutex();
    EbBlockOnMutex(m);
    gComponentCount--;
    if (!gComponentCount) {
        EB_BOOL leaked = EB_FALSE;
        ForEachHashEntry(gMemEntry, 0, PrintLeak, &leaked);
        if (!leaked) {
            printf("SVT: you have no memory leak\r\n");
        }
    }
    EbReleaseMutex(m);
#endif
}
