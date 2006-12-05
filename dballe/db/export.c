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

/* #define TRACE_DB */

#include <dballe/db/export.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/db/dba_db.h>
#include <dballe/db/querybuf.h>
#include <dballe/db/internals.h>
#include <dballe/db/attr.h>

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
		"  FROM context AS c, data AS d"
		"  LEFT JOIN attr AS a ON a.id_context = d.id_context AND a.id_var = d.id_var"
		" WHERE d.id_context = c.id AND c.id_ana = ? AND c.id_report = ?"
		"   AND c.datetime = '1000-01-01 00:00:00' AND c.ltype = 257 AND c.l1 = 0"
		"   AND c.l2 = 0 AND c.ptype = 0 AND c.p1 = 0 AND c.p2 = 0"
		" ORDER BY d.id_var, a.type";
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	/* Bound variables */
	int out_varcode;
	char out_value[255];
	int out_attr_varcode;		SQLINTEGER out_attr_varcode_ind;
	char out_attr_value[255];	SQLINTEGER out_attr_value_ind;
	dba_varcode last_varcode = 0;
	dba_var var = NULL;
	dba_var attr = NULL;
	int res;

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	/* Bind input fields */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_ana, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_report, 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, SQL_C_SLONG, &out_varcode, sizeof(out_varcode), NULL);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_value, sizeof(out_value), NULL);
	SQLBindCol(stm, 3, SQL_C_SLONG, &out_attr_varcode, sizeof(out_attr_varcode), &out_attr_varcode_ind);
	SQLBindCol(stm, 4, SQL_C_CHAR, &out_attr_value, sizeof(out_attr_value), &out_attr_value_ind);

	TRACE("Performing query: %s\n", query);

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
		TRACE("Got B%02d%03d %s\n", DBA_VAR_X(out_varcode), DBA_VAR_Y(out_varcode), out_value);

		/* First process the variable, possibly inserting the old one in the message */
		if (last_varcode != out_varcode)
		{
			TRACE("New var\n");
			if (var != NULL)
			{
				TRACE("Inserting old var B%02d%03d\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var, 257, 0, 0, 0, 0, 0));
				var = NULL;
			}
			DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(out_varcode, &var));
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(var, out_value));

			last_varcode = out_varcode;
		}

		if (out_attr_varcode_ind != -1)
		{
			TRACE("New attribute\n");
			DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(out_attr_varcode, &attr));
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(attr, out_attr_value));
			DBA_RUN_OR_GOTO(cleanup, dba_var_seta(var, attr));
			attr = NULL;			
		}
	}

	if (var != NULL)
	{
		TRACE("Inserting leftover old var B%02d%03d\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
		DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var, 257, 0, 0, 0, 0, 0));
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

dba_err dba_db_export(dba_db db, dba_record rec, dba_msg_consumer cons, void* data)
{
	dba_err err = DBA_OK;
	dba_db_cursor cur = NULL;
	dba_var var = NULL;
	dba_msg msg = NULL;
	dba_msg copy = NULL;
	int last_lat = -1;
	int last_lon = -1;
	char last_datetime[25];

	DBA_RUN_OR_RETURN(dba_db_need_attr(db));

	DBA_RUN_OR_RETURN(dba_db_cursor_create(db, &cur));

	DBA_RUN_OR_RETURN(dba_db_cursor_query(cur, rec,
				DBA_DB_WANT_ANA_ID | DBA_DB_WANT_CONTEXT_ID |
				DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT | DBA_DB_WANT_LEVEL |
                DBA_DB_WANT_TIMERANGE | DBA_DB_WANT_DATETIME |
                DBA_DB_WANT_VAR_NAME | DBA_DB_WANT_VAR_VALUE |
                DBA_DB_WANT_REPCOD,
				DBA_DB_MODIFIER_STREAM));

	/* Retrieve results */
	last_datetime[0] = 0;
	while (1)
	{
		int has_data;
		DBA_RUN_OR_RETURN(dba_db_cursor_next(cur, &has_data));
		if (!has_data)
			break;

		TRACE("Got B%02d%03d %d,%d,%d %d,%d,%d %s\n",
				DBA_VAR_X(cur->out_idvar), DBA_VAR_Y(cur->out_idvar),
				cur->out_ltype, cur->out_l1, cur->out_l2, cur->out_pind, cur->out_p1, cur->out_p2,
				cur->out_value);

		/* Create the variable that we got from here */
		DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(cur->out_idvar, &var));
		DBA_RUN_OR_GOTO(cleanup, dba_var_setc(var, cur->out_value));

		/* Load the attributes from the database */
		db->attr->id_context = cur->out_context_id;
		DBA_RUN_OR_GOTO(cleanup, dba_db_attr_load(db->attr, var));

		/* See if we have the start of a new message */
		if (cur->out_lat != last_lat || cur->out_lon != last_lon || strcmp(cur->out_datetime, last_datetime) != 0)
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
		
			msg->type = dba_msg_type_from_repcod(cur->out_rep_cod);

			/* Fill in the basic pseudoana values */
			DBA_RUN_OR_GOTO(cleanup, dba_msg_seti(msg, DBA_VAR(0, 5, 1), cur->out_lat, -1, 257, 0, 0, 0, 0, 0));
			DBA_RUN_OR_GOTO(cleanup, dba_msg_seti(msg, DBA_VAR(0, 6, 1), cur->out_lon, -1, 257, 0, 0, 0, 0, 0));
			if (cur->out_ident_ind != SQL_NULL_DATA)
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_ident(msg, cur->out_ident, -1));

			DBA_RUN_OR_GOTO(cleanup, fill_ana_layer(db, msg, cur->out_ana_id, 254));

			strncpy(last_datetime, cur->out_datetime, 20);
			last_lat = cur->out_lat;
			last_lon = cur->out_lon;
		}

		TRACE("Inserting var B%02d%03d\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
		DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var,
					cur->out_ltype, cur->out_l1, cur->out_l2,
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
			DBA_RUN_OR_GOTO(cleanup, consume_one(copy, cons, data));
		}
		msg = NULL;
	}

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
