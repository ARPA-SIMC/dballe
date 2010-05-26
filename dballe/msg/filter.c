/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "context.h"

/*
 * 1: the message matches
 * 0: the message does not match
 */
static int match_message(dba_msg m, dba_record filter)
{
#warning to be implemented
	return 1;
};

/*
 * 1: the context matches
 * 0: the context does not match
 */
static int match_context(dba_msg m, dba_msg_context l, dba_record filter)
{
#warning to be implemented
	return 1;
};

/*
 * 1: the variable matches
 * 0: the variable does not match
 */
static int match_var(dba_msg m, dba_msg_context l, dba_var var, dba_record filter)
{
#warning to be implemented
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
		dba_msg_context l = src->data[li];
		if (!match_context(src, l, filter))
			continue;
		for (di = 0; di < l->data_count; ++di)
		{
			dba_var var = l->data[di];
			if (!match_var(src, l, var, filter))
				continue;
			
			DBA_RUN_OR_GOTO(cleanup, dba_msg_set(res, var, dba_var_code(var),
					l->ltype1, l->l1, l->ltype2, l->l2,
					l->pind, l->p1, l->p2));
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
