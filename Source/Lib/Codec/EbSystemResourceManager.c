/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbSystemResourceManager.h"

static void EbFifoDctor(EB_PTR p)
{
    EbFifo_t *obj = (EbFifo_t*)p;
    EB_DESTROY_SEMAPHORE(obj->countingSemaphore);
    EB_DESTROY_MUTEX(obj->lockoutMutex);
}

/**************************************
 * EbFifoCtor
 **************************************/
static EB_ERRORTYPE EbFifoCtor(
    EbFifo_t           *fifoPtr,
    EB_U32              initialCount,
    EB_U32              maxCount,
    EbObjectWrapper_t  *firstWrapperPtr,
    EbObjectWrapper_t  *lastWrapperPtr,
    EbMuxingQueue_t    *queuePtr)
{
    fifoPtr->dctor = EbFifoDctor;
    // Create Counting Semaphore
    EB_CREATE_SEMAPHORE(fifoPtr->countingSemaphore, initialCount, maxCount);

    // Create Buffer Pool Mutex
    EB_CREATE_MUTEX(fifoPtr->lockoutMutex);

    // Initialize Fifo First & Last ptrs
    fifoPtr->firstPtr           = firstWrapperPtr;
    fifoPtr->lastPtr            = lastWrapperPtr;

    // Copy the Muxing Queue ptr this Fifo belongs to
    fifoPtr->queuePtr           = queuePtr;

    return EB_ErrorNone;
}


/**************************************
 * EbFifoPushBack
 **************************************/
static EB_ERRORTYPE EbFifoPushBack(
    EbFifo_t            *fifoPtr,
    EbObjectWrapper_t   *wrapperPtr)
{
    // If FIFO is empty
    if(fifoPtr->firstPtr == (EbObjectWrapper_t*) EB_NULL) {
        fifoPtr->firstPtr = wrapperPtr;
        fifoPtr->lastPtr  = wrapperPtr;
    } else {
        fifoPtr->lastPtr->nextPtr = wrapperPtr;
        fifoPtr->lastPtr = wrapperPtr;
    }

    fifoPtr->lastPtr->nextPtr = (EbObjectWrapper_t*) EB_NULL;

    return EB_ErrorNone;
}


/**************************************
 * EbFifoPopFront
 **************************************/
static EB_ERRORTYPE EbFifoPopFront(
    EbFifo_t            *fifoPtr,
    EbObjectWrapper_t  **wrapperPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Set wrapperPtr to head of BufferPool
    *wrapperPtr = fifoPtr->firstPtr;

    // Update tail of BufferPool if the BufferPool is now empty
    fifoPtr->lastPtr = (fifoPtr->firstPtr == fifoPtr->lastPtr) ? (EbObjectWrapper_t*) EB_NULL: fifoPtr->lastPtr;

    // Update head of BufferPool
    fifoPtr->firstPtr = fifoPtr->firstPtr->nextPtr;

    return return_error;
}

/**************************************
* EbFifoPopFront
**************************************/
static EB_BOOL EbFifoPeakFront(
    EbFifo_t            *fifoPtr)
{

    // Set wrapperPtr to head of BufferPool
    if (fifoPtr->firstPtr == (EbObjectWrapper_t*)EB_NULL) {
        return EB_TRUE;
    }
    else {
        return EB_FALSE; 
    }
}

static void EbCircularBufferDctor(EB_PTR p)
{
    EbCircularBuffer_t* obj = (EbCircularBuffer_t*)p;
    EB_FREE(obj->arrayPtr);
}

/**************************************
 * EbCircularBufferCtor
 **************************************/
static EB_ERRORTYPE EbCircularBufferCtor(
    EbCircularBuffer_t   *bufferPtr,
    EB_U32                bufferTotalCount)
{
    bufferPtr->dctor = EbCircularBufferDctor;
    bufferPtr->bufferTotalCount = bufferTotalCount;

    EB_CALLOC(bufferPtr->arrayPtr, bufferPtr->bufferTotalCount, sizeof(EB_PTR));

    return EB_ErrorNone;
}

/**************************************
 * EbCircularBufferEmptyCheck
 **************************************/
static EB_BOOL EbCircularBufferEmptyCheck(
    EbCircularBuffer_t   *bufferPtr)
{
    return ((bufferPtr->headIndex == bufferPtr->tailIndex) && (bufferPtr->arrayPtr[bufferPtr->headIndex] == EB_NULL)) ? EB_TRUE : EB_FALSE;
}

/**************************************
 * EbCircularBufferPopFront
 **************************************/
static EB_ERRORTYPE EbCircularBufferPopFront(
    EbCircularBuffer_t   *bufferPtr,
    EB_PTR               *objectPtr)
{
    // Copy the head of the buffer into the objectPtr
    *objectPtr = bufferPtr->arrayPtr[bufferPtr->headIndex];
    bufferPtr->arrayPtr[bufferPtr->headIndex] = EB_NULL;

    // Increment the head & check for rollover
    bufferPtr->headIndex = (bufferPtr->headIndex == bufferPtr->bufferTotalCount - 1) ? 0 : bufferPtr->headIndex + 1;

    // Decrement the Current Count
    --bufferPtr->currentCount;

    return EB_ErrorNone;
}

/**************************************
 * EbCircularBufferPushBack
 **************************************/
static EB_ERRORTYPE EbCircularBufferPushBack(
    EbCircularBuffer_t   *bufferPtr,
    EB_PTR                objectPtr)
{
    // Copy the pointer into the array
    bufferPtr->arrayPtr[bufferPtr->tailIndex] = objectPtr;

    // Increment the tail & check for rollover
    bufferPtr->tailIndex = (bufferPtr->tailIndex == bufferPtr->bufferTotalCount - 1) ? 0 : bufferPtr->tailIndex + 1;

    // Increment the Current Count
    ++bufferPtr->currentCount;

    return EB_ErrorNone;
}

/**************************************
 * EbCircularBufferPushFront
 **************************************/
static EB_ERRORTYPE EbCircularBufferPushFront(
    EbCircularBuffer_t   *bufferPtr,
    EB_PTR                objectPtr)
{
    // Decrement the headIndex
    bufferPtr->headIndex = (bufferPtr->headIndex == 0) ? bufferPtr->bufferTotalCount - 1 : bufferPtr->headIndex - 1;

    // Copy the pointer into the array
    bufferPtr->arrayPtr[bufferPtr->headIndex] = objectPtr;

    // Increment the Current Count
    ++bufferPtr->currentCount;
    
    return EB_ErrorNone;
}

static void EbMuxingQueueDctor(EB_PTR p)
{
    EbMuxingQueue_t* obj = (EbMuxingQueue_t*)p;
    EB_DELETE_PTR_ARRAY(obj->processFifoPtrArray, obj->processTotalCount);
    EB_DELETE(obj->objectQueue);
    EB_DELETE(obj->processQueue);
    EB_DESTROY_MUTEX(obj->lockoutMutex);
}


/**************************************
 * EbMuxingQueueCtor
 **************************************/
static EB_ERRORTYPE EbMuxingQueueCtor(
    EbMuxingQueue_t    *queuePtr,
    EB_U32              objectTotalCount,
    EB_U32              processTotalCount,
    EbFifo_t         ***processFifoPtrArrayPtr)
{
    queuePtr->dctor = EbMuxingQueueDctor;
    queuePtr->processTotalCount = processTotalCount;

    // Lockout Mutex
    EB_CREATE_MUTEX(queuePtr->lockoutMutex);

    // Construct Object Circular Buffer
    EB_NEW(
        queuePtr->objectQueue,
        EbCircularBufferCtor,
        objectTotalCount);

    // Construct Process Circular Buffer
    EB_NEW(
        queuePtr->processQueue,
        EbCircularBufferCtor,
        queuePtr->processTotalCount);

    // Construct the Process Fifos
    EB_ALLOC_PTR_ARRAY(queuePtr->processFifoPtrArray, queuePtr->processTotalCount);

    for(EB_U32 processIndex=0; processIndex < queuePtr->processTotalCount; ++processIndex) {
        EB_NEW(
            queuePtr->processFifoPtrArray[processIndex],
            EbFifoCtor,
            0,
            objectTotalCount,
            (EbObjectWrapper_t *)EB_NULL,
            (EbObjectWrapper_t *)EB_NULL,
            queuePtr);
    }

    *processFifoPtrArrayPtr = queuePtr->processFifoPtrArray;

    return EB_ErrorNone;
}

/**************************************
 * EbMuxingQueueAssignation
 **************************************/
static EB_ERRORTYPE EbMuxingQueueAssignation(
    EbMuxingQueue_t *queuePtr)
{
    EbFifo_t *processFifoPtr;
    EbObjectWrapper_t *wrapperPtr;

    // while loop
    while((EbCircularBufferEmptyCheck(queuePtr->objectQueue)  == EB_FALSE) &&
            (EbCircularBufferEmptyCheck(queuePtr->processQueue) == EB_FALSE)) {
        // Get the next process
        EbCircularBufferPopFront(
            queuePtr->processQueue,
            (void **) &processFifoPtr);

        // Get the next object
        EbCircularBufferPopFront(
            queuePtr->objectQueue,
            (void **) &wrapperPtr);

        // Block on the Process Fifo's Mutex
        EbBlockOnMutex(processFifoPtr->lockoutMutex);

        // Put the object on the fifo
        EbFifoPushBack(
            processFifoPtr,
            wrapperPtr);

        // Release the Process Fifo's Mutex
        EbReleaseMutex(processFifoPtr->lockoutMutex);

        // Post the semaphore
        EbPostSemaphore(processFifoPtr->countingSemaphore);
    }

    return EB_ErrorNone;
}

/**************************************
 * EbMuxingQueueObjectPushBack
 **************************************/
static EB_ERRORTYPE EbMuxingQueueObjectPushBack(
    EbMuxingQueue_t    *queuePtr,
    EbObjectWrapper_t  *objectPtr)
{
    EbCircularBufferPushBack(
        queuePtr->objectQueue,
        objectPtr);

    EbMuxingQueueAssignation(queuePtr);

    return EB_ErrorNone;
}

/**************************************
* EbMuxingQueueObjectPushFront
**************************************/
static EB_ERRORTYPE EbMuxingQueueObjectPushFront(
	EbMuxingQueue_t    *queuePtr,
	EbObjectWrapper_t  *objectPtr)
{
	EbCircularBufferPushFront(
		queuePtr->objectQueue,
		objectPtr);

	EbMuxingQueueAssignation(queuePtr);

	return EB_ErrorNone;
}

/*********************************************************************
 * EbObjectReleaseEnable
 *   Enables the releaseEnable member of EbObjectWrapper.  Used by
 *   certain objects (e.g. SequenceControlSet) to control whether
 *   EbObjectWrappers are allowed to be released or not.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that manages the EbObjectWrapper.
 *      The emptyFifo's lockoutMutex is used to write protect the
 *      modification of the EbObjectWrapper.
 *
 *   wrapperPtr
 *      Pointer to the EbObjectWrapper to be modified.
 *********************************************************************/
EB_ERRORTYPE EbObjectReleaseEnable(
    EbObjectWrapper_t   *wrapperPtr)
{
    EbBlockOnMutex(wrapperPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    wrapperPtr->releaseEnable = EB_TRUE;

    EbReleaseMutex(wrapperPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    return EB_ErrorNone;
}

/*********************************************************************
 * EbObjectReleaseDisable
 *   Disables the releaseEnable member of EbObjectWrapper.  Used by
 *   certain objects (e.g. SequenceControlSet) to control whether
 *   EbObjectWrappers are allowed to be released or not.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that manages the EbObjectWrapper.
 *      The emptyFifo's lockoutMutex is used to write protect the
 *      modification of the EbObjectWrapper.
 *
 *   wrapperPtr
 *      Pointer to the EbObjectWrapper to be modified.
 *********************************************************************/
EB_ERRORTYPE EbObjectReleaseDisable(
    EbObjectWrapper_t   *wrapperPtr)
{
    EbBlockOnMutex(wrapperPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    wrapperPtr->releaseEnable = EB_FALSE;

    EbReleaseMutex(wrapperPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    return EB_ErrorNone;
}

/*********************************************************************
 * EbObjectIncLiveCount
 *   Increments the liveCount member of EbObjectWrapper.  Used by
 *   certain objects (e.g. SequenceControlSet) to count the number of active
 *   pointers of a EbObjectWrapper in pipeline at any point in time.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that manages the EbObjectWrapper.
 *      The emptyFifo's lockoutMutex is used to write protect the
 *      modification of the EbObjectWrapper.
 *
 *   wrapperPtr
 *      Pointer to the EbObjectWrapper to be modified.
 *********************************************************************/
EB_ERRORTYPE EbObjectIncLiveCount(
    EbObjectWrapper_t   *wrapperPtr,
    EB_U32               incrementNumber)
{
    EbBlockOnMutex(wrapperPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    wrapperPtr->liveCount += incrementNumber;

    EbReleaseMutex(wrapperPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    return EB_ErrorNone;
}

//ugly hack
typedef struct DctorAble
{
    EbDctor dctor;
} DctorAble;

void EBObjectWrapperDctor(EB_PTR p)
{
    EbObjectWrapper_t* wrapper = (EbObjectWrapper_t*)p;
    if (wrapper->objectDestroyer) {
        //customized destroyer
        if (wrapper->objectPtr)
            wrapper->objectDestroyer(wrapper->objectPtr);
    }
    else {
        //hack....
        DctorAble* obj = (DctorAble*)wrapper->objectPtr;
        EB_DELETE(obj);
    }
}

static EB_ERRORTYPE EBObjectWrapperCtor(EbObjectWrapper_t* wrapper,
    EbSystemResource_t  *resource,
    EB_CREATOR           objectCreator,
    EB_PTR               objectInitDataPtr,
    EbDctor              objectDestroyer)
{
    EB_ERRORTYPE ret;

    wrapper->dctor = EBObjectWrapperDctor;
    wrapper->releaseEnable = EB_TRUE;
    wrapper->quitSignal = EB_FALSE;
    wrapper->systemResourcePtr = resource;
    wrapper->objectDestroyer = objectDestroyer;
    ret = objectCreator(&wrapper->objectPtr, objectInitDataPtr);
    if (ret != EB_ErrorNone)
        return ret;
    return EB_ErrorNone;
}

static void EbSystemResourceDctor(EB_PTR p)
{
    EbSystemResource_t* obj = (EbSystemResource_t*)p;
    EB_DELETE(obj->fullQueue);
    EB_DELETE(obj->emptyQueue);
    EB_DELETE_PTR_ARRAY(obj->wrapperPtrPool, obj->objectTotalCount);
}

/*********************************************************************
 * EbSystemResourceCtor
 *   Constructor for EbSystemResource.  Fully constructs all members
 *   of EbSystemResource including the object with the passed
 *   ObjectCtor function.
 *
 *   resourcePtr
 *     Pointer that will contain the SystemResource to be constructed.
 *
 *   objectTotalCount
 *     Number of objects to be managed by the SystemResource.
 *
 *   fullFifoEnabled
 *     Bool that describes if the SystemResource is to have an output
 *     fifo.  An outputFifo is not used by certain objects (e.g.
 *     SequenceControlSet).
 *
 *   ObjectCtor
 *     Function pointer to the constructor of the object managed by
 *     SystemResource referenced by resourcePtr. No object level
 *     construction is performed if ObjectCtor is NULL.
 *
 *   objectInitDataPtr

 *     Pointer to data block to be used during the construction of
 *     the object. objectInitDataPtr is passed to ObjectCtor when
 *     ObjectCtor is called.
 *********************************************************************/
EB_ERRORTYPE EbSystemResourceCtor(
    EbSystemResource_t  *resourcePtr,
    EB_U32               objectTotalCount,
    EB_U32               producerProcessTotalCount,
    EB_U32               consumerProcessTotalCount,
    EbFifo_t          ***producerFifoPtrArrayPtr,
    EbFifo_t          ***consumerFifoPtrArrayPtr,
    EB_BOOL              fullFifoEnabled,
    EB_CREATOR           objectCreator,
    EB_PTR               objectInitDataPtr,
    EbDctor              objectDestroyer)
{
    EB_U32 wrapperIndex;
    resourcePtr->dctor = EbSystemResourceDctor;

    resourcePtr->objectTotalCount = objectTotalCount;

    // Allocate array for wrapper pointers
    EB_ALLOC_PTR_ARRAY(resourcePtr->wrapperPtrPool, resourcePtr->objectTotalCount);

    // Initialize each wrapper
    for (wrapperIndex=0; wrapperIndex < resourcePtr->objectTotalCount; ++wrapperIndex) {
        EB_NEW(
            resourcePtr->wrapperPtrPool[wrapperIndex],
            EBObjectWrapperCtor,
            resourcePtr,
            objectCreator,
            objectInitDataPtr,
            objectDestroyer);
    }

    // Initialize the Empty Queue
    EB_NEW(
        resourcePtr->emptyQueue,
        EbMuxingQueueCtor,
        resourcePtr->objectTotalCount,
        producerProcessTotalCount,
        producerFifoPtrArrayPtr);
    // Fill the Empty Fifo with every ObjectWrapper
    for (wrapperIndex=0; wrapperIndex < resourcePtr->objectTotalCount; ++wrapperIndex) {
        EbMuxingQueueObjectPushBack(
            resourcePtr->emptyQueue,
            resourcePtr->wrapperPtrPool[wrapperIndex]);
    }

    // Initialize the Full Queue
    if (fullFifoEnabled == EB_TRUE) {
        EB_NEW(
            resourcePtr->fullQueue,
            EbMuxingQueueCtor,
            resourcePtr->objectTotalCount,
            consumerProcessTotalCount,
            consumerFifoPtrArrayPtr);
    }

    return EB_ErrorNone;
}



/*********************************************************************
 * EbSystemResourceReleaseProcess
 *********************************************************************/
static EB_ERRORTYPE EbReleaseProcess(
    EbFifo_t   *processFifoPtr)
{
    EbBlockOnMutex(processFifoPtr->queuePtr->lockoutMutex);

    EbCircularBufferPushFront(
        processFifoPtr->queuePtr->processQueue,
        processFifoPtr);

    EbMuxingQueueAssignation(processFifoPtr->queuePtr);

    EbReleaseMutex(processFifoPtr->queuePtr->lockoutMutex);

    return EB_ErrorNone;
}

/*********************************************************************
 * EbSystemResourcePostObject
 *   Queues a full EbObjectWrapper to the SystemResource. This
 *   function posts the SystemResource fullFifo countingSemaphore.
 *   This function is write protected by the SystemResource fullFifo
 *   lockoutMutex.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that the EbObjectWrapper is
 *      posted to.
 *
 *   wrapperPtr
 *      Pointer to EbObjectWrapper to be posted.
 *********************************************************************/
EB_ERRORTYPE EbPostFullObject(
    EbObjectWrapper_t   *objectPtr)
{
    EbBlockOnMutex(objectPtr->systemResourcePtr->fullQueue->lockoutMutex);

    EbMuxingQueueObjectPushBack(
        objectPtr->systemResourcePtr->fullQueue,
        objectPtr);

    EbReleaseMutex(objectPtr->systemResourcePtr->fullQueue->lockoutMutex);

    return EB_ErrorNone;
}

/*********************************************************************
 * EbSystemResourceReleaseObject
 *   Queues an empty EbObjectWrapper to the SystemResource. This
 *   function posts the SystemResource emptyFifo countingSemaphore.
 *   This function is write protected by the SystemResource emptyFifo
 *   lockoutMutex.
 *
 *   objectPtr
 *      Pointer to EbObjectWrapper to be released.
 *********************************************************************/
EB_ERRORTYPE EbReleaseObject(
    EbObjectWrapper_t   *objectPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EbBlockOnMutex(objectPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    // Decrement liveCount
    objectPtr->liveCount = (objectPtr->liveCount == 0) ? objectPtr->liveCount : objectPtr->liveCount - 1;

    if((objectPtr->releaseEnable == EB_TRUE) && (objectPtr->liveCount == 0)) {

        // Set liveCount to EB_ObjectWrapperReleasedValue
        objectPtr->liveCount = EB_ObjectWrapperReleasedValue;

		EbMuxingQueueObjectPushFront(
			objectPtr->systemResourcePtr->emptyQueue,
			objectPtr);

    }

    EbReleaseMutex(objectPtr->systemResourcePtr->emptyQueue->lockoutMutex);

    return return_error;
}

/*********************************************************************
 * EbSystemResourceGetEmptyObject
 *   Dequeues an empty EbObjectWrapper from the SystemResource.  This
 *   function blocks on the SystemResource emptyFifo countingSemaphore.
 *   This function is write protected by the SystemResource emptyFifo
 *   lockoutMutex.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that provides the empty
 *      EbObjectWrapper.
 *
 *   wrapperDblPtr
 *      Double pointer used to pass the pointer to the empty
 *      EbObjectWrapper pointer.
 *********************************************************************/
EB_ERRORTYPE EbGetEmptyObject(
    EbFifo_t   *emptyFifoPtr,
    EbObjectWrapper_t **wrapperDblPtr)
{
    // Queue the Fifo requesting the empty fifo
    EbReleaseProcess(emptyFifoPtr);

    // Block on the counting Semaphore until an empty buffer is available
    EbBlockOnSemaphore(emptyFifoPtr->countingSemaphore);

    // Acquire lockout Mutex
    EbBlockOnMutex(emptyFifoPtr->lockoutMutex);

    // Get the empty object
    EbFifoPopFront(
        emptyFifoPtr,
        wrapperDblPtr);

    // Reset the wrapper's liveCount
    (*wrapperDblPtr)->liveCount = 0;

    // Object release enable
    (*wrapperDblPtr)->releaseEnable = EB_TRUE;

    // Release Mutex
    EbReleaseMutex(emptyFifoPtr->lockoutMutex);

    return EB_ErrorNone;
}

/*********************************************************************
 * EbSystemResourceGetFullObject
 *   Dequeues an full EbObjectWrapper from the SystemResource. This
 *   function blocks on the SystemResource fullFifo countingSemaphore.
 *   This function is write protected by the SystemResource fullFifo
 *   lockoutMutex.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that provides the full
 *      EbObjectWrapper.
 *
 *   wrapperDblPtr
 *      Double pointer used to pass the pointer to the full
 *      EbObjectWrapper pointer.
 *********************************************************************/
EB_ERRORTYPE EbGetFullObject(
    EbFifo_t   *fullFifoPtr,
    EbObjectWrapper_t **wrapperDblPtr)
{
    // Queue the process requesting the full fifo
    EbReleaseProcess(fullFifoPtr);

    // Block on the counting Semaphore until an empty buffer is available
    EbBlockOnSemaphore(fullFifoPtr->countingSemaphore);

    // Acquire lockout Mutex
    EbBlockOnMutex(fullFifoPtr->lockoutMutex);

    EbFifoPopFront(
        fullFifoPtr,
        wrapperDblPtr);

    // Release Mutex
    EbReleaseMutex(fullFifoPtr->lockoutMutex);

    return EB_ErrorNone;
}

/******************************************************************************
* EbSystemResourceGetFullObject
*   Dequeues an full EbObjectWrapper from the SystemResource. This
*   function does not block on the SystemResource fullFifo countingSemaphore.
*   This function is write protected by the SystemResource fullFifo
*   lockoutMutex.
*
*   resourcePtr
*      Pointer to the SystemResource that provides the full
*      EbObjectWrapper.
*
*   wrapperDblPtr
*      Double pointer used to pass the pointer to the full
*      EbObjectWrapper pointer.
*******************************************************************************/
EB_ERRORTYPE EbGetFullObjectNonBlocking(
    EbFifo_t   *fullFifoPtr,
    EbObjectWrapper_t **wrapperDblPtr)
{
    EB_BOOL      fifoEmpty;
    // Queue the Fifo requesting the full fifo
    EbReleaseProcess(fullFifoPtr);

    // Acquire lockout Mutex
    EbBlockOnMutex(fullFifoPtr->lockoutMutex);

    fifoEmpty = EbFifoPeakFront(
                        fullFifoPtr);

    // Release Mutex
    EbReleaseMutex(fullFifoPtr->lockoutMutex);

    if (fifoEmpty == EB_FALSE)
        EbGetFullObject(
            fullFifoPtr,
            wrapperDblPtr);
    else
        *wrapperDblPtr = (EbObjectWrapper_t*)EB_NULL;

    return EB_ErrorNone;
}