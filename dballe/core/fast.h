/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

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

/**
 * Format a value into its decimal representation.
 *
 * @warning This function is not thread safe.
 *
 * @param value
 *   Number to convert.
 * @param len
 *   Maximum length of the resulting string.
 *   The most significant digits are truncated if the len is too short to fit
 *   the value.
 * @return
 *   The string representation in a statically allocated string.
 */
static inline char *itoa(long value, int len)
{
	/* Adaptation from the _itoa_word function in glibc */
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
