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

#include "exporters.h"

static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type);

struct _bufrex_exporter bufrex_exporter_generic = {
	/* Category */
	255,
	/* Subcategory */
	255,
	/* Local subcategory */
	0,
	/* dba_msg type it can convert from */
	MSG_GENERIC,
	/* Data descriptor section */
	(dba_varcode[]){ 0 },
	/* Datadesc function */
	bufrex_standard_datadesc_func,
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type)
{
	dba_err err = DBA_OK;
	int i, j;
	int ltype1 = -1, l1 = -1, ltype2 = -1, l2 = -1, pind = -1, p1 = -1, p2 = -1;
	dba_var repcod = dba_msg_get_rep_cod_var(src);

	if (repcod)
		DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_var(dst, dba_var_code(repcod), repcod));
	else if (src->type != MSG_GENERIC)
		DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 193), dba_msg_repcod_from_type(src->type)));

	for (i = 0; i < src->data_count; i++)
	{
		dba_msg_level lev = src->data[i];

		for (j = 0; j < lev->data_count; j++)
		{
			dba_var_attr_iterator iter;
			dba_msg_datum d = lev->data[j];
			if (dba_var_value(d->var) == NULL)
				continue;
			// Don't add rep_cod again
			if (d->var == repcod)
				continue;

			/* Update the context in the message, if needed */
			if (ltype1 != lev->ltype1)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 7, 192), lev->ltype1));
				ltype1 = lev->ltype1;
			}
			if (l1 != lev->l1)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 7, 193), lev->l1));
				l1 = lev->l1;
			}
			if (ltype2 != lev->ltype2)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 7, 195), lev->ltype2));
				ltype2 = lev->ltype2;
			}
			if (l2 != lev->l2)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 7, 194), lev->l2));
				l2 = lev->l2;
			}
			if (pind != d->pind)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 4, 192), d->pind));
				pind = d->pind;
			}
			if (p1 != d->p1)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 4, 193), d->p1));
				p1 = d->p1;
			}
			if (p2 != d->p2)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_i(dst, DBA_VAR(0, 4, 194), d->p2));
				p2 = d->p2;
			}

			/* Store the variable */
			DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_var(dst, dba_var_code(d->var), d->var));

			/* Store the attributes */
			for (iter = dba_var_attr_iterate(d->var);
					iter != NULL;
					iter = dba_var_attr_iterator_next(iter))
			{
				dba_var attr = dba_var_attr_iterator_attr(iter);
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable_var(dst, dba_var_code(attr), attr));
			}
		}
	}

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
