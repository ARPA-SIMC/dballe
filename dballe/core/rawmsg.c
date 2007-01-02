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

#include "rawmsg.h"

#include <stdlib.h> /* malloc */

dba_err dba_rawmsg_create(dba_rawmsg* msg)
{
	if ((*msg = (dba_rawmsg)calloc(1, sizeof(struct _dba_rawmsg))) == NULL)
		return dba_error_alloc("allocating a new dba_rawmsg");
	return dba_error_ok();
}

void dba_rawmsg_delete(dba_rawmsg msg)
{
	if (msg->buf)
	{
		free(msg->buf);
		msg->buf = NULL;
		msg->alloclen = 0;
	}
}

void dba_rawmsg_reset(dba_rawmsg msg)
{
	/* Preserve buf and alloclen so that allocated memory can be reused */
	msg->offset = 0;
	msg->index = 0;
	msg->len = 0;
}

dba_err dba_rawmsg_acquire_buf(dba_rawmsg msg, unsigned char* buf, int size)
{
	if (msg->buf)
	{
		free(msg->buf);
		msg->buf = NULL;
		msg->alloclen = 0;
	}

	msg->buf = buf;
	msg->alloclen = size;
	msg->len = size;
	return dba_error_ok();
}

dba_err dba_rawmsg_get_raw(dba_rawmsg msg, const unsigned char** buf, int* size)
{
	*buf = msg->buf;
	*size = msg->len;
	return dba_error_ok();
}

dba_err dba_rawmsg_expand_buffer(dba_rawmsg msg)
{
	if (msg->buf == 0)
	{
		msg->alloclen = 2048;
		if ((msg->buf = (unsigned char*)malloc(msg->alloclen)) == NULL)
			return dba_error_alloc("allocating memory for message data");
	} else {
		unsigned char* newbuf;

		/* If we are below 16Kb, double the size, else grow by 2Kb */
		if (msg->alloclen < 16*1024)
			msg->alloclen <<= 1;
		else
			msg->alloclen += 2048;

		if ((newbuf = (unsigned char*)realloc(msg->buf, msg->alloclen)) == NULL)
			return dba_error_alloc("allocating more memory for message data");
		msg->buf = newbuf;
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
