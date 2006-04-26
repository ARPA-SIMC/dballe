#include <dballe/db/export.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/db/dba_db.h>
#include <dballe/db/querybuf.h>
/* #define TRACE_DB */
#include <dballe/db/internals.h>

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

dba_err dba_db_export(dba_db db, dba_record rec, dba_msg_consumer cons, void* data)
{
	const char* query =
		"SELECT pa.id, pa.lat, pa.lon, pa.ident,"
		"       c.ltype, c.l1, c.l2,"
		"       c.ptype, c.p1, c.p2,"
		"       d.id_var, c.datetime, d.value, ri.id, ri.memo, ri.prio, c.id, a.type, a.value"
		"  FROM pseudoana AS pa, context AS c, repinfo AS ri, data AS d"
		"  LEFT JOIN attr AS a ON a.id_context = d.id_context AND a.id_var = d.id_var"
		" WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	/* Bound variables */
	int out_lat;
	int out_lon;
	char out_ident[64];			SQLINTEGER out_ident_ind;
	int out_leveltype;
	int out_l1;
	int out_l2;
	int out_pindicator;
	int out_p1;
	int out_p2;
	int out_varcode;
	char out_datetime[25];
	char out_value[255];
	int out_rep_cod;
	char out_rep_memo[20];
	int out_priority;
	int out_ana_id;
	int out_context_id;
	int out_attr_varcode;		SQLINTEGER out_attr_varcode_ind;
	char out_attr_value[255];	SQLINTEGER out_attr_value_ind;
	int last_lat = -1;
	int last_lon = -1;
	int last_context_id = -1;
	dba_varcode last_varcode = 0;
	char last_datetime[25];
	int last_ltype = -1;
	int last_l1 = -1;
	int last_l2 = -1;
	int last_pind = -1;
	int last_p1 = -1;
	int last_p2 = -1;
	dba_msg msg = NULL;
	dba_var var = NULL;
	dba_var attr = NULL;
	int pseq = 1;
	int res;

	assert(db);

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	/* Write the SQL query */

	/* Initial query */
	dba_querybuf_reset(db->querybuf);
	DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, query));

	/* Bind output fields */
#define DBA_QUERY_BIND_NONNULL(num, type, name) \
	SQLBindCol(stm, num, type, &out_##name, sizeof(out_##name), NULL);
#define DBA_QUERY_BIND(num, type, name) \
	SQLBindCol(stm, num, type, &out_##name, sizeof(out_##name), &out_##name##_ind);
	DBA_QUERY_BIND_NONNULL( 1, SQL_C_SLONG, ana_id);
	DBA_QUERY_BIND_NONNULL( 2, SQL_C_SLONG, lat);
	DBA_QUERY_BIND_NONNULL( 3, SQL_C_SLONG, lon);
	DBA_QUERY_BIND( 4, SQL_C_CHAR, ident);
	DBA_QUERY_BIND_NONNULL( 5, SQL_C_SLONG, leveltype);
	DBA_QUERY_BIND_NONNULL( 6, SQL_C_SLONG, l1);
	DBA_QUERY_BIND_NONNULL( 7, SQL_C_SLONG, l2);
	DBA_QUERY_BIND_NONNULL( 8, SQL_C_SLONG, pindicator);
	DBA_QUERY_BIND_NONNULL( 9, SQL_C_SLONG, p1);
	DBA_QUERY_BIND_NONNULL(10, SQL_C_SLONG, p2);
	DBA_QUERY_BIND_NONNULL(11, SQL_C_SLONG, varcode);
	DBA_QUERY_BIND_NONNULL(12, SQL_C_CHAR, datetime);
	DBA_QUERY_BIND_NONNULL(13, SQL_C_CHAR, value);
	DBA_QUERY_BIND_NONNULL(14, SQL_C_SLONG, rep_cod);
	DBA_QUERY_BIND_NONNULL(15, SQL_C_CHAR, rep_memo);
	DBA_QUERY_BIND_NONNULL(16, SQL_C_SLONG, priority);
	DBA_QUERY_BIND_NONNULL(17, SQL_C_SLONG, context_id);
	DBA_QUERY_BIND(18, SQL_C_SLONG, attr_varcode);
	DBA_QUERY_BIND(19, SQL_C_CHAR, attr_value);
#undef DBA_QUERY_BIND
#undef DBA_QUERY_BIND_NONNULL
	
	/* Add the select part */
	DBA_RUN_OR_GOTO(cleanup, dba_db_prepare_select(db, rec, stm, &pseq));

	DBA_RUN_OR_GOTO(cleanup, dba_querybuf_append(db->querybuf,
		" ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2"));

	TRACE("Performing query: %s\n", dba_querybuf_get(db->querybuf));

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)dba_querybuf_get(db->querybuf), dba_querybuf_size(db->querybuf));
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", dba_querybuf_get(db->querybuf));
		goto cleanup;
	}

	/* Retrieve results */
	last_datetime[0] = 0;
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		TRACE("Got B%02d%03d %d,%d,%d %d,%d,%d %s\n",
				DBA_VAR_X(out_varcode), DBA_VAR_Y(out_varcode),
				out_leveltype, out_l1, out_l2, out_pindicator, out_p1, out_p2,
				out_value);

		/* First process the variable, possibly inserting the old one in the message */
		if (last_context_id != out_context_id || last_varcode != out_varcode)
		{
			TRACE("New var\n");
			if (var != NULL)
			{
				TRACE("Inserting old var B%02d%03d\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var,
															last_ltype, last_l1, last_l2,
															last_pind, last_p1, last_p2));
				var = NULL;
			}
			DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(out_varcode, &var));
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(var, out_value));

			last_context_id = out_context_id;
			last_varcode = out_varcode;
			last_ltype = out_leveltype;
			last_l1 = out_l1;
			last_l2 = out_l2;
			last_pind = out_pindicator;
			last_p1 = out_p1;
			last_p2 = out_p2;
		}

		if (out_attr_varcode_ind != -1)
		{
			TRACE("New attribute\n");
			DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(out_attr_varcode, &attr));
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(attr, out_attr_value));
			DBA_RUN_OR_GOTO(cleanup, dba_var_seta(var, attr));
			attr = NULL;			
		}

		if (out_lat != last_lat || out_lon != last_lon || strcmp(out_datetime, last_datetime) != 0)
		{
			TRACE("New message\n");
			if (msg != NULL)
			{
				TRACE("Sending old message to consumer\n");
				if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
				{
					dba_msg copy;
					DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_pack_levels(msg, &copy));
					/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
					DBA_RUN_OR_GOTO(cleanup, cons(copy, data));
					dba_msg_delete(msg);
				} else {
					DBA_RUN_OR_GOTO(cleanup, cons(msg, data));
				}
				msg = NULL;
			}

			DBA_RUN_OR_GOTO(cleanup, dba_msg_create(&msg));
		
			msg->type = dba_msg_type_from_repcod(out_rep_cod);

			DBA_RUN_OR_GOTO(cleanup, fill_ana_layer(db, msg, out_ana_id, 254));

			strncpy(last_datetime, out_datetime, 20);
			last_lat = out_lat;
			last_lon = out_lon;
		}
	}

	if (var != NULL)
	{
		TRACE("Inserting leftover old var B%02d%03d at l(%d,%d,%d) p(%d,%d,%d)\n", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)), last_ltype, last_l1, last_l2, last_pind, last_p1, last_p2);
		DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var,
													last_ltype, last_l1, last_l2,
													last_pind, last_p1, last_p2));
		var = NULL;
	}

	if (msg != NULL)
	{
		TRACE("Inserting leftover old message\n");
		if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
		{
			dba_msg copy;
			DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_pack_levels(msg, &copy));
			/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
			DBA_RUN_OR_GOTO(cleanup, cons(copy, data));
			dba_msg_delete(msg);
		} else {
			DBA_RUN_OR_GOTO(cleanup, cons(msg, data));
		}
		msg = NULL;
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	if (attr != NULL)
		dba_var_delete(attr);
	if (var != NULL)
		dba_var_delete(var);
	if (msg != NULL)
		dba_msg_delete(msg);
	return err == DBA_OK ? dba_error_ok() : err;
}
/* vim:set ts=4 sw=4: */
