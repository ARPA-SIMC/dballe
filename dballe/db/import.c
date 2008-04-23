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

#include "config.h"

#include "import.h"
#include "internals.h"
#include "pseudoana.h"
#include "context.h"
#include "data.h"
#include "attr.h"

#include "dballe/core/conv.h"

#include <sql.h>
#include <sqlext.h>

#include <string.h>
#include <stdlib.h>


dba_err dba_import_msgs(dba_db db, dba_msgs msgs, int repcod, int flags)
{
	int i;
	for (i = 0; i < msgs->len; ++i)
		DBA_RUN_OR_RETURN(dba_import_msg(db, msgs->msgs[i], repcod, flags));
	return dba_error_ok();
}

dba_err dba_import_msg(dba_db db, dba_msg msg, int repcod, int flags)
{
	dba_err err = DBA_OK;
	dba_msg_level l_ana = dba_msg_find_level(msg, 257, 0, 0, 0);
	dba_msg_datum d;
	dba_db_pseudoana da;
	dba_db_context dc;
	dba_db_data dd;
	dba_db_attr dq;
	int i, j;
	int mobile, val;
	int inserted_pseudoana = 0;

	if (l_ana == NULL)
		return dba_error_consistency("cannot import into the database a message without pseudoana information");
	
	/* Quick access to the various database components */
	DBA_RUN_OR_RETURN(dba_db_need_pseudoana(db));
	DBA_RUN_OR_RETURN(dba_db_need_context(db));
	DBA_RUN_OR_RETURN(dba_db_need_data(db));
	DBA_RUN_OR_RETURN(dba_db_need_attr(db));
	da = db->pseudoana;
	dc = db->context;
	dd = db->data;
	dq = db->attr;

	/* Check if the station is mobile */
	mobile = dba_msg_get_ident_var(msg) == NULL ? 0 : 1;

	/* Begin transaction */
	/*if (!(flags & DBA_IMPORT_NO_TRANSACTIONS))*/
		DBA_RUN_OR_RETURN(dba_db_begin(db));

	/* Fill up the pseudoana informations needed to fetch an existing ID */

	/* Latitude */
	if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_LATITUDE)) != NULL)
	{
		int lat;
		DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &lat));
		da->lat = lat;
	}

	/* Longitude */
	if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_LONGITUDE)) != NULL)
	{
		int lon;
		DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &lon));
		da->lon = lon;
	}

	/* Station identifier */
	if (mobile)
	{
		if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_IDENT)) != NULL)
			dba_db_pseudoana_set_ident(da, dba_var_value(d->var));
		else {
			err = dba_error_notfound("looking for ident in message to insert");
			goto fail;
		}
	} else {
		da->ident[0] = 0;
		da->ident_ind = SQL_NULL_DATA;
	}

	/* Check if we can reuse a pseudoana row */
	DBA_RUN_OR_GOTO(fail, dba_db_pseudoana_get_id(da, &val));
	if (val == -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_db_pseudoana_insert(da, &val));
		inserted_pseudoana = 1;
	}
	dc->id_ana = val;

	/* Report code */
	dc->id_report = repcod != -1 ? repcod : dba_msg_repcod_from_type(msg->type);

	if (flags & DBA_IMPORT_FULL_PSEUDOANA || inserted_pseudoana)
	{
		DBA_RUN_OR_GOTO(fail, dba_db_context_obtain_ana(dc, &val));
		dd->id_context = val;

		/* Insert the rest of the pseudoana information */
		if ((flags & DBA_IMPORT_OVERWRITE) || inserted_pseudoana)
			for (i = 0; i < l_ana->data_count; i++)
			{
				dba_var_attr_iterator iter;
				dba_varcode code = dba_var_code(l_ana->data[i]->var);
				/* Do not import datetime in the pseudoana layer */
				if (code >= DBA_VAR(0, 4, 1) && code <= DBA_VAR(0, 4, 6))
					continue;

				dba_db_data_set(dd, l_ana->data[i]->var);
				DBA_RUN_OR_GOTO(fail, dba_db_data_insert(dd, (flags & DBA_IMPORT_OVERWRITE)));

				dq->id_context = dd->id_context;
				dq->id_var = dba_var_code(l_ana->data[i]->var);

				/* Insert the attributes */
				if (flags & DBA_IMPORT_ATTRS)
					for (iter = dba_var_attr_iterate(l_ana->data[i]->var); iter != NULL; 
							iter = dba_var_attr_iterator_next(iter))
					{
						dba_var attr = dba_var_attr_iterator_attr(iter);
						if (dba_var_value(attr) != NULL)
						{
							dba_db_attr_set(dq, attr);
							DBA_RUN_OR_GOTO(fail, dba_db_attr_insert(dq, (flags & DBA_IMPORT_OVERWRITE)));
						}
					}
			}
	}

	/* Fill up the common contexts information for the rest of the data */

	/* Date and time */
	{
		const char* year = 
			(d = dba_msg_level_find_by_id(l_ana, DBA_MSG_YEAR)) == NULL ? NULL : dba_var_value(d->var);
		const char* month = 
			(d = dba_msg_level_find_by_id(l_ana, DBA_MSG_MONTH)) == NULL ? NULL : dba_var_value(d->var);
		const char* day = 
			(d = dba_msg_level_find_by_id(l_ana, DBA_MSG_DAY)) == NULL ? NULL : dba_var_value(d->var);
		const char* hour = 
			(d = dba_msg_level_find_by_id(l_ana, DBA_MSG_HOUR)) == NULL ? NULL : dba_var_value(d->var);
		const char* min = 
			(d = dba_msg_level_find_by_id(l_ana, DBA_MSG_MINUTE)) == NULL ? NULL : dba_var_value(d->var);

		if (year == NULL || month == NULL || day == NULL || hour == NULL || min == NULL)
		{
			err = dba_error_notfound("looking for datetime informations in message to insert");
			goto fail;
		}

		dc->date.year = strtol(year, 0, 10);
		dc->date.month = strtol(month, 0, 10);
		dc->date.day = strtol(day, 0, 10);
		dc->date.hour = strtol(hour, 0, 10);
		dc->date.minute = strtol(min, 0, 10);
		dc->date.second = 0;
	}

	/* Insert the rest of the data */
	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		int old_pind = -1;
		int old_p1 = -1;
		int old_p2 = -1;

		/* Fill in the context */
		dc->ltype1 = lev->ltype1;
		dc->l1 = lev->l1;
		dc->ltype2 = lev->ltype2;
		dc->l2 = lev->l2;

		for (j = 0; j < lev->data_count; j++)
		{
			dba_msg_datum dat = lev->data[j];
			dba_var_attr_iterator iter;

			/* Skip the anagraphical level */
			if (lev->ltype1 == 257 && lev->l1 == 0 && lev->ltype2 == 0 && lev->l2 == 0
				&& dat->pind == 0 && dat->p1 == 0 && dat->p2 == 0)
			{
				dba_varcode code = dba_var_code(dat->var);
				if (!(flags & DBA_IMPORT_DATETIME_ATTRS)
					|| DBA_VAR_X(code) != 4 || DBA_VAR_Y(code) < 1 || DBA_VAR_Y(code) > 6)
					continue;
			}


			if (dat->pind != old_pind || dat->p1 != old_p1 || dat->p2 != old_p2)
			{
				/* Insert the new context when the datum coordinates change */
				dc->pind = dat->pind;
				dc->p1 = dat->p1;
				dc->p2 = dat->p2;

				DBA_RUN_OR_GOTO(fail, dba_db_context_get_id(dc, &val));
				if (val == -1)
					DBA_RUN_OR_GOTO(fail, dba_db_context_insert(dc, &val));
				dd->id_context = val;

				old_pind = dat->pind;
				old_p1 = dat->p1;
				old_p2 = dat->p2;
			}

			/* Insert the variable */
			dba_db_data_set(dd, dat->var);
			DBA_RUN_OR_GOTO(fail, dba_db_data_insert(dd, (flags & DBA_IMPORT_OVERWRITE)));

			/* Insert the attributes */
			if (flags & DBA_IMPORT_ATTRS)
			{
				dq->id_context = dd->id_context;
				dq->id_var = dba_var_code(dat->var);

				for (iter = dba_var_attr_iterate(dat->var); iter != NULL; 
						iter = dba_var_attr_iterator_next(iter))
				{
					dba_var attr = dba_var_attr_iterator_attr(iter);
					if (dba_var_value(attr) != NULL)
					{
						dba_db_attr_set(dq, attr);
						DBA_RUN_OR_GOTO(fail, dba_db_attr_insert(dq, (flags & DBA_IMPORT_OVERWRITE)));
					}
				}
			}
		}
	}

	/*if (!(flags & DBA_IMPORT_NO_TRANSACTIONS))*/
		DBA_RUN_OR_GOTO(fail, dba_db_commit(db));
    return dba_error_ok();

fail:
	if (!(flags & DBA_IMPORT_NO_TRANSACTIONS))
		dba_db_rollback(db);
	return err;
}

/* vim:set ts=4 sw=4: */
