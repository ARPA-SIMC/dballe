/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006,2007  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "filter.h"

/*
 * 1: the message matches
 * 0: the message does not match
 */
static int match_message(dba_msg m, dba_record filter)
{
	return 1;
};

/*
 * 1: the level matches
 * 0: the level does not match
 */
static int match_level(dba_msg m, dba_msg_level l, dba_record filter)
{
	return 1;
};

/*
 * 1: the datum matches
 * 0: the datum does not match
 */
static int match_datum(dba_msg m, dba_msg_level l, dba_msg_datum d, dba_record filter)
{
	return 1;
}

dba_err dba_msg_filter_copy(dba_msg src, dba_msg* dst, dba_record filter)
{
	dba_err err = DBA_OK;
	dba_msg res = NULL;
	int li, di;
	int copied = 0;

	if (!match_message(src, filter))
	{
		*dst = NULL;
		return dba_error_ok();
	}

	DBA_RUN_OR_RETURN(dba_msg_create(&res));

	for (li = 0; li < src->data_count; ++li)
	{
		dba_msg_level l = src->data[li];
		if (!match_level(src, l, filter))
			continue;
		for (di = 0; di < l->data_count; ++di)
		{
			dba_msg_datum d = l->data[di];
			if (!match_datum(src, l, d, filter))
				continue;
			
			DBA_RUN_OR_GOTO(cleanup, dba_msg_set(res, d->var, dba_var_code(d->var),
					l->ltype, l->l1, l->l2,
					d->pind, d->p1, d->p2));
			++copied;
		}
	}

	if (!copied)
	{
		dba_msg_delete(res);
		res = NULL;
	}

	// Hand over the new message to the caller
	*dst = res;
	res = NULL;

cleanup:
	if (res)
		dba_msg_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
