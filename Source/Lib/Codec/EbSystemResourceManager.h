/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSystemResource_h
#define EbSystemResource_h

#include "EbThreads.h"
#include "EbDefinitions.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/*********************************
 * Defines
 *********************************/
#define EB_ObjectWrapperReleasedValue   ~0u

/*********************************************************************
 * Object Wrapper
 *   Provides state information for each type of object in the
 *   encoder system (i.e. SequenceControlSet, PictureBufferDesc,
 *   ProcessResults, and GracefulDegradation)
 *********************************************************************/
typedef struct EbObjectWrapper_s {
    EbDctor                   dctor;
    EbDctor                   objectDestroyer;
    // objectPtr - pointer to the object being managed.
    void                     *objectPtr;

    // liveCount - a count of the number of pictures actively being
    //   encoded in the pipeline at any given time.  Modification
    //   of this value by any process must be protected by a mutex.
    EB_U32                    liveCount;

    // releaseEnable - a flag that enables the release of
    //   EbObjectWrapper for reuse in the encoding of subsequent
    //   pictures in the encoder pipeline.
    EB_BOOL                   releaseEnable;

    // quitSignal - a flag that main thread sets to break out from kernels
    EB_BOOL                   quitSignal;

    // systemResourcePtr - a pointer to the SystemResourceManager
    //   that the object belongs to.
    struct EbSystemResource_s *systemResourcePtr;

    // nextPtr - a pointer to a different EbObjectWrapper.  Used
    //   only in the implemenation of a single-linked Fifo.
    struct EbObjectWrapper_s *nextPtr;

} EbObjectWrapper_t;

/*********************************************************************
 * Fifo
 *   Defines a static (i.e. no dynamic memory allocation) single
 *   linked-list, constant time fifo implmentation. The fifo uses
 *   the EbObjectWrapper member nextPtr to create the linked-list.
 *   The Fifo also contains a countingSemaphore for OS thread-blocking
 *   and dynamic EbObjectWrapper counting.
 *********************************************************************/
typedef struct EbFifo_s {
    EbDctor dctor;
    // countingSemaphore - used for OS thread-blocking & dynamically
    //   counting the number of EbObjectWrappers currently in the
    //   EbFifo.
    EB_HANDLE countingSemaphore;

    // lockoutMutex - used to prevent more than one thread from
    //   modifying EbFifo simultaneously.
    EB_HANDLE lockoutMutex;

    // firstPtr - pointer the the head of the Fifo
    EbObjectWrapper_t *firstPtr;

    // lastPtr - pointer to the tail of the Fifo
    EbObjectWrapper_t *lastPtr;

    // queuePtr - pointer to MuxingQueue that the EbFifo is
    //   associated with.
    struct EbMuxingQueue_s *queuePtr;

} EbFifo_t;

/*********************************************************************
 * CircularBuffer
 *********************************************************************/
typedef struct EbCircularBuffer_s {
    EbDctor dctor;
    EB_PTR *arrayPtr;
    EB_U32  headIndex;
    EB_U32  tailIndex;
    EB_U32  bufferTotalCount;
    EB_U32  currentCount;

} EbCircularBuffer_t;

/*********************************************************************
 * MuxingQueue
 *********************************************************************/
typedef struct EbMuxingQueue_s {
    EbDctor             dctor;
    EB_HANDLE           lockoutMutex;
    EbCircularBuffer_t *objectQueue;
    EbCircularBuffer_t *processQueue;
    EB_U32              processTotalCount;
    EbFifo_t          **processFifoPtrArray;

} EbMuxingQueue_t;

/*********************************************************************
 * SystemResource
 *   Defines a complete solution for managing objects in the encoder
 *   system (i.e. SequenceControlSet, PictureBufferDesc, ProcessResults, and
 *   GracefulDegradation).  The objectTotalCount and wrapperPtrPool are
 *   only used to construct and destruct the SystemResource.  The
 *   fullFifo provides downstream pipeline data flow control.  The
 *   emptyFifo provides upstream pipeline backpressure flow control.
 *********************************************************************/
typedef struct EbSystemResource_s {
    EbDctor             dctor;
    // objectTotalCount - A count of the number of objects contained in the
    //   System Resoruce.
    EB_U32              objectTotalCount;

    // wrapperPtrPool - An array of pointers to the EbObjectWrappers used
    //   to construct and destruct the SystemResource.
    EbObjectWrapper_t **wrapperPtrPool;

    // The empty FIFO contains a queue of empty buffers
    //EbFifo_t           *emptyFifo;
    EbMuxingQueue_t     *emptyQueue;

    // The full FIFO contains a queue of completed buffers
    //EbFifo_t           *fullFifo;
    EbMuxingQueue_t     *fullQueue;

} EbSystemResource_t;

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
extern EB_ERRORTYPE EbObjectReleaseEnable(
    EbObjectWrapper_t   *wrapperPtr);

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
extern EB_ERRORTYPE EbObjectReleaseDisable(
    EbObjectWrapper_t   *wrapperPtr);

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
 *
 *   incrementNumber
 *      The number to increment the live count by.
 *********************************************************************/
extern EB_ERRORTYPE EbObjectIncLiveCount(
    EbObjectWrapper_t   *wrapperPtr,
    EB_U32               incrementNumber);

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
extern EB_ERRORTYPE EbSystemResourceCtor(
    EbSystemResource_t  *resourcePtr,
    EB_U32               objectTotalCount,
    EB_U32               producerProcessTotalCount,
    EB_U32               consumerProcessTotalCount,
    EbFifo_t          ***producerFifoPtrArrayPtr,
    EbFifo_t          ***consumerFifoPtrArrayPtr,
    EB_BOOL              fullFifoEnabled,
    EB_CREATOR           objectCreator,
    EB_PTR               objectInitDataPtr,
    EbDctor              objectDestroyer);

/*********************************************************************
 * EbSystemResourceGetEmptyObject
 *   Dequeues an empty EbObjectWrapper from the SystemResource.  The
 *   new EbObjectWrapper will be populated with the contents of the
 *   wrapperCopyPtr if wrapperCopyPtr is not NULL. This function blocks
 *   on the SystemResource emptyFifo countingSemaphore. This function
 *   is write protected by the SystemResource emptyFifo lockoutMutex.
 *
 *   resourcePtr
 *      Pointer to the SystemResource that provides the empty
 *      EbObjectWrapper.
 *
 *   wrapperDblPtr
 *      Double pointer used to pass the pointer to the empty
 *      EbObjectWrapper pointer.
 *********************************************************************/
extern EB_ERRORTYPE EbGetEmptyObject(
    EbFifo_t   *emptyFifoPtr,
    EbObjectWrapper_t **wrapperDblPtr);

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
extern EB_ERRORTYPE EbPostFullObject(
    EbObjectWrapper_t   *objectPtr);

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
extern EB_ERRORTYPE EbGetFullObject(
    EbFifo_t   *fullFifoPtr,
    EbObjectWrapper_t **wrapperDblPtr);

/*********************************************************************
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
*********************************************************************/
extern EB_ERRORTYPE EbGetFullObjectNonBlocking(
    EbFifo_t   *fullFifoPtr,
    EbObjectWrapper_t **wrapperDblPtr);

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
extern EB_ERRORTYPE EbReleaseObject(
    EbObjectWrapper_t   *objectPtr);
#ifdef __cplusplus
}
#endif
#endif //EbSystemResource_h
