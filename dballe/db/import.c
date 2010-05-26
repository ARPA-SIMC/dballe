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

#include "config.h"

#include "import.h"
#include "internals.h"
#include "pseudoana.h"
#include "context.h"
#include "data.h"
#include "attr.h"

#include "dballe/core/conv.h"
#include "dballe/msg/context.h"

#include <sql.h>
#include <sqlext.h>

#include <string.h>
#include <stdlib.h>


dba_err dba_import_msgs(dba_db db, dba_msgs msgs, const char* repmemo, int flags)
{
	int i;
	for (i = 0; i < msgs->len; ++i)
		DBA_RUN_OR_RETURN(dba_import_msg(db, msgs->msgs[i], repmemo, flags));
	return dba_error_ok();
}

dba_err dba_import_msg(dba_db db, dba_msg msg, const char* repmemo, int flags)
{
	dba_err err = DBA_OK;
	dba_msg_context l_ana = dba_msg_find_context(msg, 257, 0,  0, 0,  0, 0, 0);
	dba_var var;
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
	if ((var = dba_msg_context_find_by_id(l_ana, DBA_MSG_LATITUDE)) != NULL)
	{
		int lat;
		DBA_RUN_OR_GOTO(fail, dba_var_enqi(var, &lat));
		da->lat = lat;
	}

	/* Longitude */
	if ((var = dba_msg_context_find_by_id(l_ana, DBA_MSG_LONGITUDE)) != NULL)
	{
		int lon;
		DBA_RUN_OR_GOTO(fail, dba_var_enqi(var, &lon));
		da->lon = lon;
	}

	/* Station identifier */
	if (mobile)
	{
		if ((var = dba_msg_context_find_by_id(l_ana, DBA_MSG_IDENT)) != NULL)
			dba_db_pseudoana_set_ident(da, dba_var_value(var));
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
	{
		int res;
		if (repmemo != NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_db_rep_cod_from_memo(db, repmemo, &res));
		} else {
			// TODO: check if B01194 first
			dba_var var = dba_msg_get_rep_memo_var(msg);
			if (var)
			{
				DBA_RUN_OR_GOTO(fail, dba_db_rep_cod_from_memo(db, dba_var_value(var), &res));
			} else {
				DBA_RUN_OR_GOTO(fail, dba_db_rep_cod_from_memo(db, dba_msg_repmemo_from_type(msg->type), &res));
			}
		}
		dc->id_report = res;
	}

	if ((flags & DBA_IMPORT_FULL_PSEUDOANA) || inserted_pseudoana)
	{
		DBA_RUN_OR_GOTO(fail, dba_db_context_obtain_ana(dc, &val));
		dd->id_context = val;

		/* Insert the rest of the pseudoana information */
		for (i = 0; i < l_ana->data_count; i++)
		{
			int inserted;
			dba_var_attr_iterator iter;
			dba_varcode code = dba_var_code(l_ana->data[i]);
			/* Do not import datetime in the pseudoana layer */
			if (code >= DBA_VAR(0, 4, 1) && code <= DBA_VAR(0, 4, 6))
				continue;

			dba_db_data_set(dd, l_ana->data[i]);

			if ((flags & DBA_IMPORT_OVERWRITE) == 0)
			{
				/* Insert only if it is missing */
				DBA_RUN_OR_GOTO(fail, dba_db_data_insert_or_ignore(dd, &inserted));
			} else {
				DBA_RUN_OR_GOTO(fail, dba_db_data_insert_or_overwrite(dd));
				inserted = 1;
			}

			dq->id_context = dd->id_context;
			dq->id_var = dba_var_code(l_ana->data[i]);

			/* Insert the attributes */
			if (inserted && (flags & DBA_IMPORT_ATTRS))
				for (iter = dba_var_attr_iterate(l_ana->data[i]); iter != NULL; 
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
			(var = dba_msg_context_find_by_id(l_ana, DBA_MSG_YEAR)) == NULL ? NULL : dba_var_value(var);
		const char* month = 
			(var = dba_msg_context_find_by_id(l_ana, DBA_MSG_MONTH)) == NULL ? NULL : dba_var_value(var);
		const char* day = 
			(var = dba_msg_context_find_by_id(l_ana, DBA_MSG_DAY)) == NULL ? NULL : dba_var_value(var);
		const char* hour = 
			(var = dba_msg_context_find_by_id(l_ana, DBA_MSG_HOUR)) == NULL ? NULL : dba_var_value(var);
		const char* min = 
			(var = dba_msg_context_find_by_id(l_ana, DBA_MSG_MINUTE)) == NULL ? NULL : dba_var_value(var);

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
		dba_msg_context ctx = msg->data[i];
		int is_ana_level = (ctx->ltype1 == 257 && ctx->l1 == 0
				 && ctx->ltype2 == 0 && ctx->l2 == 0
				 && ctx->pind == 0 && ctx->p1 == 0 && ctx->p2 == 0);

		/* Skip the anagraphical level */
		if (is_ana_level && !(flags & DBA_IMPORT_DATETIME_ATTRS))
			continue;

		/* Insert the new context */
		dc->ltype1 = ctx->ltype1;
		dc->l1 = ctx->l1;
		dc->ltype2 = ctx->ltype2;
		dc->l2 = ctx->l2;
		dc->pind = ctx->pind;
		dc->p1 = ctx->p1;
		dc->p2 = ctx->p2;
		DBA_RUN_OR_GOTO(fail, dba_db_context_get_id(dc, &val));
		if (val == -1)
			DBA_RUN_OR_GOTO(fail, dba_db_context_insert(dc, &val));

		/* Get the database ID of the context */
		dd->id_context = val;

		for (j = 0; j < ctx->data_count; j++)
		{
			dba_var var = ctx->data[j];
			dba_var_attr_iterator iter;

			// Only import dates from ana level, and only if requested
			if (is_ana_level)
			{
				dba_varcode code = dba_var_code(var);
				if (!(flags & DBA_IMPORT_DATETIME_ATTRS)
					|| DBA_VAR_X(code) != 4 || DBA_VAR_Y(code) < 1 || DBA_VAR_Y(code) > 6)
					continue;
			}

			/* Insert the variable */
			dba_db_data_set(dd, var);
			if (flags & DBA_IMPORT_OVERWRITE)
				DBA_RUN_OR_GOTO(fail, dba_db_data_insert_or_overwrite(dd));
			else
				DBA_RUN_OR_GOTO(fail, dba_db_data_insert_or_fail(dd));

			/* Insert the attributes */
			if (flags & DBA_IMPORT_ATTRS)
			{
				dq->id_context = dd->id_context;
				dq->id_var = dba_var_code(var);

				for (iter = dba_var_attr_iterate(var); iter != NULL; 
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
