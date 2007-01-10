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

#include "dballe/db/querybuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

struct _dba_querybuf
{
	int maxsize;
	int size;
	int list_first;
	char list_sep[10];
	char buf[];
};

dba_err dba_querybuf_create(int maxsize, dba_querybuf* buf)
{
	if (maxsize < 1)
		return dba_error_consistency("checking that the querybuf max size is more than 0");
	
	if (((*buf) = (dba_querybuf)malloc(sizeof(struct _dba_querybuf) + maxsize)) == NULL)
		return dba_error_alloc("Allocating a new dba_querybuf");

	(*buf)->maxsize = maxsize;
	dba_querybuf_reset(*buf);

	return dba_error_ok();
}

void dba_querybuf_delete(dba_querybuf buf)
{
	free(buf);
}

void dba_querybuf_reset(dba_querybuf buf)
{
	buf->buf[0] = 0;
	buf->size = 0;
	buf->list_first = 1;
	buf->list_sep[0] = 0;
}

const char* dba_querybuf_get(dba_querybuf buf)
{
	return buf->buf;
}

int dba_querybuf_size(dba_querybuf buf)
{
	return buf->size;
}

dba_err dba_querybuf_start_list(dba_querybuf buf, const char* sep)
{
	buf->list_first = 1;
	strncpy(buf->list_sep, sep, 10);
	buf->list_sep[9] = 0;
	return dba_error_ok();
}

dba_err dba_querybuf_append(dba_querybuf buf, const char* str)
{
	const char* s;
	for (s = str; *s; s++)
	{
		if (buf->size >= buf->maxsize - 1)
		{
			/* Leave the string properly null-terminated also in case of error */
			buf->buf[buf->maxsize - 1] = 0;
			return dba_error_consistency("checking that the string to append fits in the querybuf");
		}
		buf->buf[buf->size++] = *s;
	}
	buf->buf[buf->size] = 0;
	return dba_error_ok();
}

dba_err dba_querybuf_appendf(dba_querybuf buf, const char* fmt, ...)
{
	int size;
	va_list ap;
	va_start(ap, fmt);
	size = vsnprintf(buf->buf + buf->size, buf->maxsize - buf->size, fmt, ap);
	if (size >= buf->maxsize - buf->size)
	{
		buf->buf[buf->maxsize - 1] = 0;
		return dba_error_consistency("checking that the formatted string to append fits in the querybuf");
	}
	buf->size += size;
	va_end(ap);

	return dba_error_ok();
}

dba_err dba_querybuf_append_list(dba_querybuf buf, const char* str)
{
	if (buf->list_first)
		buf->list_first = 0;
	else
		DBA_RUN_OR_RETURN(dba_querybuf_append(buf, buf->list_sep));
	return dba_querybuf_append(buf, str);
}


/* vim:set ts=4 sw=4: */
