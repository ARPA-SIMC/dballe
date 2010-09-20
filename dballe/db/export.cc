/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/* #define TRACE_DB */

#include "export.h"
#include "querybuf.h"
#include "internals.h"
#include "attr.h"

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

static dba_err fill_ana_layer(dba_db db, dba_msg msg, int id_ana, int id_report)
{
	static const char query[] =
		"SELECT d.id_var, d.value, a.type, a.value"
		"  FROM context c, data d"
		"  LEFT JOIN attr a ON a.id_context = d.id_context AND a.id_var = d.id_var"
		" WHERE d.id_context = c.id AND c.id_ana = ? AND c.id_report = ?"
		"   AND c.datetime = {ts '1000-01-01 00:00:00.0'} AND c.ltype1 = 257 AND c.l1 = 0"
		"   AND c.ltype2 = 0 AND c.l2 = 0 AND c.ptype = 0 AND c.p1 = 0 AND c.p2 = 0"
		" ORDER BY d.id_var, a.type";
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	/* Bound variables */
	DBALLE_SQL_C_SINT_TYPE in_id_ana = id_ana;
	DBALLE_SQL_C_SINT_TYPE in_id_report = id_report;
	DBALLE_SQL_C_SINT_TYPE out_varcode;
	char out_value[255];
	DBALLE_SQL_C_SINT_TYPE out_attr_varcode;		SQLLEN out_attr_varcode_ind;
	char out_attr_value[255];	SQLLEN out_attr_value_ind;
	dba_varcode last_varcode = 0;
	dba_var var = NULL;
	dba_var attr = NULL;
	int res;

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	/* Bind input fields */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &in_id_ana, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &in_id_report, 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, DBALLE_SQL_C_SINT, &out_varcode, sizeof(out_varcode), NULL);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_value, sizeof(out_value), NULL);
	SQLBindCol(stm, 3, DBALLE_SQL_C_SINT, &out_attr_varcode, sizeof(out_attr_varcode), &out_attr_varcode_ind);
	SQLBindCol(stm, 4, SQL_C_CHAR, &out_attr_value, sizeof(out_attr_value), &out_attr_value_ind);

	TRACE("FAL Performing query: %s\n", query);

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, sizeof(query));
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", query);
		goto cleanup;
	}

	/* Retrieve results */
	last_varcode = -1;
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		TRACE("FAL Got B%02ld%03ld %s\n", DBA_VAR_X(out_varcode), DBA_VAR_Y(out_varcode), out_value);

		/* First process the variable, possibly inserting the old one in the message */
		if (last_varcode != out_varcode)
		{
			TRACE("FAL New var\n");
			if (var != NULL)
			{
				TRACE("FAL Inserting old var B%02d%03d\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var, 257, 0, 0, 0, 0, 0, 0));
				var = NULL;
			}
			DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(out_varcode, &var));
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(var, out_value));

			last_varcode = out_varcode;
		}

		if (out_attr_varcode_ind != -1)
		{
			TRACE("FAL New attribute\n");
			DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(out_attr_varcode, &attr));
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(attr, out_attr_value));
			DBA_RUN_OR_GOTO(cleanup, dba_var_seta(var, attr));
			attr = NULL;			
		}
	}

	if (var != NULL)
	{
		TRACE("FAL Inserting leftover old var B%02d%03d\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
		DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var, 257, 0, 0, 0, 0, 0, 0));
		var = NULL;
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	if (attr != NULL)
		dba_var_delete(attr);
	if (var != NULL)
		dba_var_delete(var);
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err consume_one(dba_msg msg, dba_msg_consumer cons, void* data)
{
	dba_err err = DBA_OK;
	dba_msgs msgs = NULL;
//	fprintf(stderr, "CONSUME ONE %p\n", msg);
	DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&msgs));
	DBA_RUN_OR_GOTO(cleanup, dba_msgs_append_acquire(msgs, msg));
	DBA_RUN_OR_GOTO(cleanup, cons(msgs, data));
	msgs = NULL;

cleanup:
	if (err != DBA_OK)
	{
		/* If we report a problem we don't consume msg, so we tell msgs not to
		 * deallocate it */
		msgs->msgs[0] = NULL;
	}
	if (msgs != NULL)
		dba_msgs_delete(msgs);
	return err == DBA_OK ? dba_error_ok() : err;
}

static inline int sqltimecmp(const SQL_TIMESTAMP_STRUCT* a, const SQL_TIMESTAMP_STRUCT* b)
{
	return memcmp(a, b, sizeof(SQL_TIMESTAMP_STRUCT));
}

dba_err dba_db_export(dba_db db, dba_record rec, dba_msg_consumer cons, void* data)
{
	dba_err err = DBA_OK;
	dba_db_cursor cur = NULL;
	dba_var var = NULL;
	dba_msg msg = NULL;
	dba_msg copy = NULL;
	int last_lat = -1;
	int last_lon = -1;
	SQL_TIMESTAMP_STRUCT last_datetime;
	char last_ident[70];
	int last_rep_cod = -1;

	DBA_RUN_OR_RETURN(dba_db_need_attr(db));

	DBA_RUN_OR_RETURN(dba_db_cursor_create(db, &cur));

	DBA_RUN_OR_RETURN(dba_db_cursor_query(cur, rec,
				DBA_DB_WANT_ANA_ID | DBA_DB_WANT_CONTEXT_ID |
				DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT | DBA_DB_WANT_LEVEL |
                DBA_DB_WANT_TIMERANGE | DBA_DB_WANT_DATETIME |
                DBA_DB_WANT_VAR_NAME | DBA_DB_WANT_VAR_VALUE |
                DBA_DB_WANT_REPCOD,
				DBA_DB_MODIFIER_SORT_FOR_EXPORT));
			/*	DBA_DB_MODIFIER_STREAM)); */

	/* Retrieve results */
	last_datetime.year = 0;
	last_ident[0] = 0;
	while (1)
	{
		int has_data;
		int ident_differs;
		DBA_RUN_OR_RETURN(dba_db_cursor_next(cur, &has_data));
		if (!has_data)
			break;

		if (cur->out_ident_ind != SQL_NULL_DATA)
			ident_differs = strncmp(last_ident, cur->out_ident, cur->out_ident_ind) != 0;
		else
			ident_differs = last_ident[0] != 0;

		TRACE("Got B%02ld%03ld %ld,%ld, %ld,%ld %ld,%ld,%ld %s\n",
				DBA_VAR_X(cur->out_idvar), DBA_VAR_Y(cur->out_idvar),
				cur->out_ltype1, cur->out_l1, cur->out_ltype2, cur->out_l2, cur->out_pind, cur->out_p1, cur->out_p2,
				cur->out_value);

		/* Create the variable that we got from here */
		DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(cur->out_idvar, &var));
		DBA_RUN_OR_GOTO(cleanup, dba_var_setc(var, cur->out_value));

		/* Load the attributes from the database */
		db->attr->id_context = cur->out_context_id;
		DBA_RUN_OR_GOTO(cleanup, dba_db_attr_load(db->attr, var));

		/* See if we have the start of a new message */
		if (cur->out_lat != last_lat || cur->out_lon != last_lon || sqltimecmp(&(cur->out_datetime), &last_datetime) != 0 || ident_differs || cur->out_rep_cod != last_rep_cod)
		{
			TRACE("New message\n");
			if (msg != NULL)
			{
				TRACE("Sending old message to consumer\n");
				if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
				{
					DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_pack_levels(msg, &copy));
					/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
					DBA_RUN_OR_GOTO(cleanup, consume_one(copy, cons, data));
					dba_msg_delete(msg);
					copy = NULL;
				} else {
					DBA_RUN_OR_GOTO(cleanup, consume_one(msg, cons, data));
				}
				msg = NULL;
			}

			DBA_RUN_OR_GOTO(cleanup, dba_msg_create(&msg));
		
			{
				const char* memo;
				DBA_RUN_OR_GOTO(cleanup, dba_db_rep_memo_from_cod(db, cur->out_rep_cod, &memo));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_rep_memo(msg, memo, -1));
				msg->type = dba_msg_type_from_repmemo(memo);
			}

			/* Fill in the basic pseudoana values */
			DBA_RUN_OR_GOTO(cleanup, dba_msg_seti(msg, DBA_VAR(0, 5, 1), cur->out_lat, -1, 257, 0, 0, 0, 0, 0, 0));
			DBA_RUN_OR_GOTO(cleanup, dba_msg_seti(msg, DBA_VAR(0, 6, 1), cur->out_lon, -1, 257, 0, 0, 0, 0, 0, 0));
			if (cur->out_ident_ind != SQL_NULL_DATA)
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_ident(msg, cur->out_ident, -1));

			/* Fill in datetime */
			{
				/*
				int year, mon, day, hour, min, sec;
				if (sscanf(cur->out_datetime,
							"%04d-%02d-%02d %02d:%02d:%02d", &year, &mon, &day, &hour, &min, &sec) != 6)
				{
					err = dba_error_consistency("parsing datetime string \"%s\"", cur->out_datetime);
					goto cleanup;
				}
				*/

				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_year(msg, cur->out_datetime.year, -1));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_month(msg, cur->out_datetime.month, -1));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_day(msg, cur->out_datetime.day, -1));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_hour(msg, cur->out_datetime.hour, -1));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_minute(msg, cur->out_datetime.minute, -1));
				/*DBA_RUN_OR_GOTO(cleanup, dba_msg_set_second(msg, sec, -1));*/
			}

			DBA_RUN_OR_GOTO(cleanup, fill_ana_layer(db, msg, cur->out_ana_id, cur->out_rep_cod));

			last_datetime = cur->out_datetime;
			last_lat = cur->out_lat;
			last_lon = cur->out_lon;
			if (cur->out_ident_ind != SQL_NULL_DATA)
				strncpy(last_ident, cur->out_ident, cur->out_ident_ind);
			else
				last_ident[0] = 0;
			last_rep_cod = cur->out_rep_cod;
		}

		TRACE("Inserting var B%02d%03d (%s)\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)), dba_var_value(var));
		DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var,
					cur->out_ltype1, cur->out_l1, cur->out_ltype2, cur->out_l2,
					cur->out_pind, cur->out_p1, cur->out_p2));
		var = NULL;
	}

	if (msg != NULL)
	{
		TRACE("Inserting leftover old message\n");
		if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
		{
			DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_pack_levels(msg, &copy));
			/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
			DBA_RUN_OR_GOTO(cleanup, consume_one(copy, cons, data));
			dba_msg_delete(msg);
			copy = NULL;
		} else {
			DBA_RUN_OR_GOTO(cleanup, consume_one(msg, cons, data));
		}
		msg = NULL;
	}

	/* Useful for Oracle to end the session */
    DBA_RUN_OR_GOTO(cleanup, dba_db_commit(db));

cleanup:
	if (var != NULL)
		dba_var_delete(var);
	if (msg != NULL)
		dba_msg_delete(msg);
	if (copy != NULL)
		dba_msg_delete(copy);
	return err == DBA_OK ? dba_error_ok() : err;
}
/* vim:set ts=4 sw=4: */
