/*------------------------------------------------------------------
* strncpy_s.c / strcpy_s.c / strnlen_s.c
*
* October 2008, Bo Berry
*
* Copyright � 2008-2011 by Cisco Systems, Inc
* All rights reserved.

* safe_str_constraint.c
*
* October 2008, Bo Berry
* 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
*
* Copyright � 2008, 2009, 2012 Cisco Systems
* All rights reserved.

* ignore_handler_s.c
*
* 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
*
* Copyright � 2012 Cisco Systems
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
#ifndef EbAppString_h
#define EbAppString_h

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef _RSIZE_T_DEFINED
typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif  /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int32_t errno_t;
#endif  /* _ERRNO_T_DEFINED */

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

#define RSIZE_MAX_STR      ( 4UL << 10 )      /* 4KB */
#define RCNEGATE(x)  (x)

#ifndef sldebug_printf
#define sldebug_printf(...)
#endif

typedef void(*constraint_handler_t) (const char * /* msg */,
    void *       /* ptr */,
    errno_t      /* error */);

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

#define sl_default_handler ignore_handler_s
extern void ignore_handler_s(const char *msg, void *ptr, errno_t error);

/* string copy */
errno_t strcpy_ss(
    char *dest, rsize_t dmax, const char *src);

/* fitted string copy */
errno_t strncpy_ss(
    char *dest, rsize_t dmax, const char *src, rsize_t slen);

/* string length */
rsize_t strnlen_ss(
    const char *s, rsize_t smax);

#define EB_STRNCPY(dst, dst_size, src, count) \
    strncpy_ss(dst, dst_size, src, count)

#define EB_STRCPY(dst, size, src) \
    strcpy_ss(dst, size, src)

#define EB_STRCMP(target,token) \
    strcmp(target,token)

#define EB_STRLEN(target, max_size) \
    strnlen_ss(target, max_size)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbAppString_h
/* File EOF */
