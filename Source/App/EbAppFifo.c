
#include "EbApi.h"
#include "EbAppConfig.h"

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
