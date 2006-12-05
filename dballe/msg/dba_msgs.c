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

#include "dba_msgs.h"

#include <stdlib.h>

dba_err dba_msgs_create(dba_msgs* msgs)
{
	*msgs = (dba_msgs)calloc(1, sizeof(struct _dba_msgs));
	if (*msgs == NULL)
		return dba_error_alloc("allocating new dba_msgs");
	return dba_error_ok();
}

void dba_msgs_delete(dba_msgs msgs)
{
	int i;
	for (i = 0; i < msgs->len; ++i)
		if (msgs->msgs[i] != NULL)
			dba_msg_delete(msgs->msgs[i]);
	free(msgs);
}

dba_err dba_msgs_append_acquire(dba_msgs msgs, dba_msg msg)
{
	if (msgs->len == msgs->alloclen)
	{
		if (msgs->alloclen == 0)
		{
			msgs->alloclen = 4;
			if ((msgs->msgs = (dba_msg*)malloc(msgs->alloclen * sizeof(dba_msg))) == NULL)
				return dba_error_alloc("allocating memory for data in dba_msgs");
		} else {
			dba_msg* newarr;
			/* Double the size of the msgs buffer */
			msgs->alloclen <<= 1;
			if ((newarr = (dba_msg*)realloc(msgs->msgs, msgs->alloclen * sizeof(dba_msg))) == NULL)
				return dba_error_alloc("allocating memory for expanding data in dba_msgs");
			msgs->msgs = newarr;
		}
	}
	msgs->msgs[msgs->len++] = msg;
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
