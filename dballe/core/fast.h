#ifndef DBA_CORE_FAST_H
#define DBA_CORE_FAST_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Implement inline optimized commonly used routines
 */

#include <stdlib.h>

/* Adaptation from the _itoa_word function in glibc */
static inline char *itoa(long value, int len)
{
	static const char *digits = "0123456789";
	static char buf[20] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	char* buflim = buf+19;
	int negative = value < 0;
	//  if (negative) value = -value;
	do
		*--buflim = digits[abs(value % 10)];
	while ((value /= 10) != 0 && --len != 0);
	if (negative && len)
		*--buflim = '-';
	return buflim;
}

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
