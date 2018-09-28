/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/*------------------------------------------------------------------
* strncpy_s.c / strcpy_s.c / strnlen_s.c
*
* October 2008, Bo Berry
*
* Copyright (c) 2008-2011 by Cisco Systems, Inc
* All rights reserved.

* safe_str_constraint.c
*
* October 2008, Bo Berry
* 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
*
* Copyright (c) 2008, 2009, 2012 Cisco Systems
* All rights reserved.

* ignore_handler_s.c
*
* 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
*
* Copyright (c) 2012 Cisco Systems
* All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use,
* copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following
* conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*------------------------------------------------------------------
*/


#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#include <semaphore.h>
#else
#error OS/Platform not supported
#endif // _WIN32

#include <immintrin.h>

#include "EbAppFifo.h"
#include "EbTypes.h"

/***************************************
 * App Command Fifo Constructor
 ***************************************/
void AppCommandFifoCtor(
    AppCommandFifo_t *fifoPtr,
    EB_U32 totalBuffersSize)
{
    fifoPtr->head = 0;
    fifoPtr->tail = 0;
    fifoPtr->size = 0;
    fifoPtr->totalBuffersSize   = totalBuffersSize /*+ APP_TOTALBUFFERS*/;

    fifoPtr->buffer = (AppCommandItem_t *) malloc(sizeof(AppCommandItem_t)*fifoPtr->totalBuffersSize);

    // Initialize Mutex
#ifdef _WIN32
    fifoPtr->mutexHandle = CreateMutex(
                               NULL,                               // default security attributes
                               FALSE,                              // FALSE := not initially owned
                               NULL);                              // the mutex's unique name (optional)

#elif __linux__
    fifoPtr->mutexHandle = (void *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(
        (pthread_mutex_t*) fifoPtr->mutexHandle,
        NULL);                              // default attributes

#endif // _WIN32

    // Initialize Semaphore
#ifdef _WIN32
    // Create Command Fifo Semaphore
    fifoPtr->semaphoreHandle = CreateSemaphore( // Semaphore handle
                                   NULL,                                   // default security attributes
                                   0,                                      // Initial Count
                                   fifoPtr->totalBuffersSize,              // Max Count
                                   NULL);                                  // the semaphore's unique name (optional)

#elif __linux__
    // Create Command Fifo Semaphore
    fifoPtr->semaphoreHandle = (void *) malloc(sizeof(sem_t));
    sem_init(
        (sem_t*) (fifoPtr->semaphoreHandle),    // semaphore handle
        0,                                      // shared semaphore (not local)
        0);                                     // initial count

#endif // _WIN32        

}

/***************************************
 * App Command Fifo Destructor
 ***************************************/
void AppCommandFifoDtor(
    AppCommandFifo_t *fifoPtr)
{
    // Release Mutex & Semaphore
#ifdef _WIN32
    CloseHandle(fifoPtr->mutexHandle);
    CloseHandle(fifoPtr->semaphoreHandle);
    free(fifoPtr->buffer);
#elif __linux__
    pthread_mutex_destroy((pthread_mutex_t*) fifoPtr->mutexHandle);
    free(fifoPtr->mutexHandle);
    sem_destroy((sem_t*) fifoPtr->semaphoreHandle);
    free(fifoPtr->semaphoreHandle);
    free(fifoPtr->buffer);
#endif // _WIN32      
}


/***************************************
 * App Command Fifo Pop
 ***************************************/
void AppCommandFifoPop(
    AppCommandFifo_t *fifoPtr,
    AppCommandItem_t *dstPtr)
{

    // Get the Fifo mutex so changes can safely be made
#ifdef _WIN32
    WaitForSingleObject((HANDLE) fifoPtr->mutexHandle, INFINITE);
#elif __linux__
    pthread_mutex_lock((pthread_mutex_t*) fifoPtr->mutexHandle);
#endif // _WIN32

    if (fifoPtr->size > 0) {
		EB_MEMCPY(dstPtr, &fifoPtr->buffer[fifoPtr->tail++], sizeof(AppCommandItem_t));
        fifoPtr->tail = (fifoPtr->tail == fifoPtr->totalBuffersSize) ? 0 : fifoPtr->tail; // Rollover head index if its at the end of the array
        --fifoPtr->size;
    }

    // Release the mutex
#ifdef _WIN32
    ReleaseMutex((HANDLE) fifoPtr->mutexHandle);
#elif __linux__
    pthread_mutex_unlock((pthread_mutex_t*) fifoPtr->mutexHandle);
#endif // _WIN32    

    return;
}

/***************************************
 * App Command Fifo Push
 ***************************************/
void AppCommandFifoPush(
    AppCommandFifo_t *fifoPtr,
    AppCommandItem_t *srcPtr)
{
    // Get the Fifo mutex so changes can safely be made
#ifdef _WIN32
    WaitForSingleObject((HANDLE) fifoPtr->mutexHandle, INFINITE);
#elif __linux__
    pthread_mutex_lock((pthread_mutex_t*) fifoPtr->mutexHandle);
#endif // _WIN32

    if (fifoPtr->size < fifoPtr->totalBuffersSize) {
		EB_MEMCPY(&fifoPtr->buffer[fifoPtr->head++], srcPtr, sizeof(AppCommandItem_t));
        fifoPtr->head = (fifoPtr->head == fifoPtr->totalBuffersSize) ? 0 : fifoPtr->head; // Rollover head index if its at the end of the array
        ++fifoPtr->size;
    }

    // Release the mutex
#ifdef _WIN32
    ReleaseMutex((HANDLE) fifoPtr->mutexHandle);
#elif __linux__
    pthread_mutex_unlock((pthread_mutex_t*) fifoPtr->mutexHandle);
#endif // _WIN32 

    // Post the Semaphore
#ifdef _WIN32
    ReleaseSemaphore(
        fifoPtr->semaphoreHandle,   // semaphore handle
        1,                          // amount to increment the semaphore
        NULL);                      // pointer to previous count (optional)
#elif __linux__
    sem_post((sem_t*) fifoPtr->semaphoreHandle);
#endif // _WIN32                

    return;
}
void WaitForCommandPost(
    AppCommandFifo_t *fifoPtr) 
{
    // Wait for the Semaphore to be posted
#ifdef _WIN32
    WaitForSingleObject((HANDLE)(fifoPtr->semaphoreHandle), INFINITE);
#elif __linux__
    sem_wait((sem_t*)fifoPtr->semaphoreHandle);
#endif // _WIN32
    return;
}

/* SAFE STRING LIBRARY */

#ifndef EOK
#define EOK             ( 0 )
#endif

#ifndef ESZEROL
#define ESZEROL         ( 401 )       /* length is zero              */
#endif

#ifndef ESLEMIN
#define ESLEMIN         ( 402 )       /* length is below min         */
#endif

#ifndef ESLEMAX
#define ESLEMAX         ( 403 )       /* length exceeds max          */
#endif

#ifndef ESNULLP
#define ESNULLP         ( 400 )       /* null ptr                    */
#endif

#ifndef ESOVRLP
#define ESOVRLP         ( 404 )       /* overlap undefined           */
#endif

#ifndef ESEMPTY
#define ESEMPTY         ( 405 )       /* empty string                */
#endif

#ifndef ESNOSPC
#define ESNOSPC         ( 406 )       /* not enough space for s2     */
#endif

#ifndef ESUNTERM
#define ESUNTERM        ( 407 )       /* unterminated string         */
#endif

#ifndef ESNODIFF
#define ESNODIFF        ( 408 )       /* no difference               */
#endif

#ifndef ESNOTFND
#define ESNOTFND        ( 409 )       /* not found                   */
#endif

#define RSIZE_MAX_MEM      ( 256UL << 20 )     /* 256MB */

#define RCNEGATE(x)  (x)
#define RSIZE_MAX_STR      ( 4UL << 10 )      /* 4KB */
#define sl_default_handler ignore_handler_s
#define EXPORT_SYMBOL(sym)

#ifndef sldebug_printf
#define sldebug_printf(...)
#endif

#ifndef _RSIZE_T_DEFINED
typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif  /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif  /* _ERRNO_T_DEFINED */

/*
* Function used by the libraries to invoke the registered
* runtime-constraint handler. Always needed.
*/

typedef void(*constraint_handler_t) (const char * /* msg */,
    void *       /* ptr */,
    errno_t      /* error */);
extern void ignore_handler_s(const char *msg, void *ptr, errno_t error);

/*
* Function used by the libraries to invoke the registered
* runtime-constraint handler. Always needed.
*/
extern void invoke_safe_str_constraint_handler(
    const char *msg,
    void *ptr,
    errno_t error);


static inline void handle_error(char *orig_dest, rsize_t orig_dmax,
    char *err_msg, errno_t err_code)
{
    (void)orig_dmax;
    *orig_dest = '\0';

    invoke_safe_str_constraint_handler(err_msg, NULL, err_code);
    return;
}
static constraint_handler_t str_handler = NULL;

void
invoke_safe_str_constraint_handler(const char *msg,
void *ptr,
errno_t error)
{
	if (NULL != str_handler) {
		str_handler(msg, ptr, error);
	}
	else {
		sl_default_handler(msg, ptr, error);
	}
}

void ignore_handler_s(const char *msg, void *ptr, errno_t error)
{
	(void)msg;
	(void)ptr;
	(void)error;
	sldebug_printf("IGNORE CONSTRAINT HANDLER: (%u) %s\n", error,
		(msg) ? msg : "Null message");
	return;
}
EXPORT_SYMBOL(ignore_handler_s)

errno_t
strncpy_ss(char *dest, rsize_t dmax, const char *src, rsize_t slen)
{
	rsize_t orig_dmax;
	char *orig_dest;
	const char *overlap_bumper;

	if (dest == NULL) {
		invoke_safe_str_constraint_handler("strncpy_ss: dest is null",
			NULL, ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (dmax == 0) {
		invoke_safe_str_constraint_handler("strncpy_ss: dmax is 0",
			NULL, ESZEROL);
		return RCNEGATE(ESZEROL);
	}

	if (dmax > RSIZE_MAX_STR) {
		invoke_safe_str_constraint_handler("strncpy_ss: dmax exceeds max",
			NULL, ESLEMAX);
		return RCNEGATE(ESLEMAX);
	}

	/* hold base in case src was not copied */
	orig_dmax = dmax;
	orig_dest = dest;

	if (src == NULL) {
		handle_error(orig_dest, orig_dmax, (char*) ("strncpy_ss: "
			"src is null"),
			ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (slen == 0) {
		handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
			"slen is zero"),
			ESZEROL);
		return RCNEGATE(ESZEROL);
	}

	if (slen > RSIZE_MAX_STR) {
		handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
			"slen exceeds max"),
			ESLEMAX);
		return RCNEGATE(ESLEMAX);
	}


	if (dest < src) {
		overlap_bumper = src;

		while (dmax > 0) {
			if (dest == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			if (slen == 0) {
				/*
				* Copying truncated to slen chars.  Note that the TR says to
				* copy slen chars plus the null char.  We null the slack.
				*/
				*dest = '\0';
				return RCNEGATE(EOK);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			slen--;
			dest++;
			src++;
		}

	}
	else {
		overlap_bumper = dest;

		while (dmax > 0) {
			if (src == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)( "strncpy_s: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			if (slen == 0) {
				/*
				* Copying truncated to slen chars.  Note that the TR says to
				* copy slen chars plus the null char.  We null the slack.
				*/
				*dest = '\0';
				return RCNEGATE(EOK);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			slen--;
			dest++;
			src++;
		}
	}

	/*
	* the entire src was not copied, so zero the string
	*/
	handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: not enough "
		"space for src"),
		ESNOSPC);
	return RCNEGATE(ESNOSPC);
}
EXPORT_SYMBOL(strncpy_ss)

errno_t
strcpy_ss(char *dest, rsize_t dmax, const char *src)
{
	rsize_t orig_dmax;
	char *orig_dest;
	const char *overlap_bumper;

	if (dest == NULL) {
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: dest is null"),
			NULL, ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (dmax == 0) {
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: dmax is 0"),
			NULL, ESZEROL);
		return RCNEGATE(ESZEROL);
	}

	if (dmax > RSIZE_MAX_STR) {
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: dmax exceeds max"),
			NULL, ESLEMAX);
		return RCNEGATE(ESLEMAX);
	}

	if (src == NULL) {
		*dest = '\0';
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: src is null"),
			NULL, ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (dest == src) {
		return RCNEGATE(EOK);
	}

	/* hold base of dest in case src was not copied */
	orig_dmax = dmax;
	orig_dest = dest;

	if (dest < src) {
		overlap_bumper = src;

		while (dmax > 0) {
			if (dest == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strcpy_ss: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			dest++;
			src++;
		}

	}
	else {
		overlap_bumper = dest;

		while (dmax > 0) {
			if (src == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strcpy_ss: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			dest++;
			src++;
		}
	}

	/*
	* the entire src must have been copied, if not reset dest
	* to null the string.
	*/
	handle_error(orig_dest, orig_dmax, (char*)("strcpy_ss: not "
		"enough space for src"),
		ESNOSPC);
	return RCNEGATE(ESNOSPC);
}
EXPORT_SYMBOL(strcpy_ss)

rsize_t
strnlen_ss(const char *dest, rsize_t dmax)
{
	rsize_t count;

	if (dest == NULL) {
		return RCNEGATE(0);
	}

	if (dmax == 0) {
		invoke_safe_str_constraint_handler((char*)("strnlen_ss: dmax is 0"),
			NULL, ESZEROL);
		return RCNEGATE(0);
	}

	if (dmax > RSIZE_MAX_STR) {
		invoke_safe_str_constraint_handler((char*)("strnlen_ss: dmax exceeds max"),
			NULL, ESLEMAX);
		return RCNEGATE(0);
	}

	count = 0;
	while (*dest && dmax) {
		count++;
		dmax--;
		dest++;
	}

	return RCNEGATE(count);
}
EXPORT_SYMBOL(strnlen_ss)

/* SAFE STRING LIBRARY */
