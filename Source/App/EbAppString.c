
#include "EbAppString.h"

/* SAFE STRING LIBRARY */

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

/* SAFE STRING LIBRARY */
