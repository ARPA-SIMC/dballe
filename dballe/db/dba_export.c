#include <dballe/db/dba_export.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/db/dballe.h>
#include <dballe/db/querybuf.h>
/* #define TRACE_DB */
#include <dballe/db/internals.h>

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

static dba_err set_key(dba_msg dst, int id, dba_record src, dba_keyword key)
{
	dba_var var = dba_record_key_peek(src, key);

	if (var != NULL)
		return dba_msg_set_by_id(dst, var, id);

	return dba_error_ok();
}

static dba_err copy_ana(dba_msg_type export_type, dba_msg msg, dba_record rec)
{
	int rep_cod;

	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_REP_COD, &rep_cod));

	switch (export_type)
	{
		case MSG_SYNOP: msg->type = MSG_SYNOP; break;
		case MSG_PILOT: msg->type = MSG_PILOT; break;
		case MSG_TEMP:
		case MSG_TEMP_SHIP:
			switch (rep_cod)
			{
				case 3: msg->type = MSG_TEMP; break;
				case 11: msg->type = MSG_TEMP_SHIP; break;
				default: msg->type = MSG_GENERIC; break;
			}
			break;
		case MSG_AIREP:
		case MSG_AMDAR:
		case MSG_ACARS:
			switch (rep_cod)
			{
				case 12: msg->type = MSG_AIREP; break;
				case 13: msg->type = MSG_AMDAR; break;
				case 14: msg->type = MSG_ACARS; break;
				default: msg->type = MSG_GENERIC; break;
			}
			break;
		case MSG_SHIP:
		case MSG_BUOY:
			switch (rep_cod)
			{
				case 9: msg->type = MSG_BUOY; break;
				case 10: msg->type = MSG_SHIP; break;
				default: msg->type = MSG_GENERIC; break;
			}
			break;
		case MSG_GENERIC: msg->type = MSG_GENERIC; break;
	}

	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_BLOCK,		rec, DBA_KEY_BLOCK));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_STATION, 	rec, DBA_KEY_STATION));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_IDENT,		rec, DBA_KEY_IDENT));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_YEAR,		rec, DBA_KEY_YEAR));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_MONTH,		rec, DBA_KEY_MONTH));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_DAY,			rec, DBA_KEY_DAY));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_HOUR,		rec, DBA_KEY_HOUR));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_MINUTE,		rec, DBA_KEY_MIN));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_LATITUDE,	rec, DBA_KEY_LAT));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_LONGITUDE,	rec, DBA_KEY_LON));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_HEIGHT,		rec, DBA_KEY_HEIGHT));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_HEIGHT_BARO,	rec, DBA_KEY_HEIGHTBARO));

	return dba_error_ok();
}

static dba_err copy_variable(dba_db db, dba_msg msg, dba_varcode varcode, dba_record rec)
{
	dba_err err = DBA_OK;
	dba_var var = NULL;
	int ltype, l1, l2, pind, p1, p2;
	int id, count;
	dba_record qc = NULL;
	dba_record_cursor item;
	
	DBA_RUN_OR_GOTO(cleanup, dba_record_create(&qc));

	DBA_RUN_OR_GOTO(cleanup, dba_record_var_enq(rec, varcode, &var));

	/* Now read the attributes */
	DBA_RUN_OR_GOTO(cleanup, dba_record_var_enqid(rec, varcode, &id));
	DBA_RUN_OR_GOTO(cleanup, dba_db_qc_query(db, id, NULL, 0, qc, &count));
	for (item = dba_record_iterate_first(qc); item != NULL;
			item = dba_record_iterate_next(qc, item))
	{
		dba_var attr = dba_record_cursor_variable(item);
		DBA_RUN_OR_GOTO(cleanup, dba_var_seta(var, attr));

	}
	
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE, &ltype));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_L1, &l1));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_L2, &l2));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_PINDICATOR, &pind));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_P1, &p1));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_P2, &p2));

	DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	var = NULL;

cleanup:
	if (var != NULL)
		dba_var_delete(var);
	if (qc != NULL)
		dba_record_delete(qc);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_export(dba_db db, dba_msg_type type, dba_record query, dba_msg_consumer cons, void* data)
{
	/* Read the values from the database */
	dba_err err = DBA_OK;
	dba_db_cursor cur = NULL;
	dba_record res = NULL;
	int count;
	int i;
	int is_last;
	double last_lat = 0;
	double last_lon = 0;
	char last_datetime[20];
	dba_msg msg = NULL;

	last_datetime[0] = 0;

	/* Query the values */
	DBA_RUN_OR_GOTO(cleanup, dba_db_query(db, query, &cur, &count));
	/*fprintf(stderr, "Count: %d\n", count);*/
	if (count == 0)
		goto cleanup;

	i = 0;

	DBA_RUN_OR_GOTO(cleanup, dba_record_create(&res));

	do {
		dba_varcode varcode;
		double lat;
		double lon;
		const char* datetime;

		DBA_RUN_OR_GOTO(cleanup, dba_db_cursor_next(cur, res, &varcode, &is_last));

		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqc(res, DBA_KEY_DATETIME, &datetime));
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqd(res, DBA_KEY_LAT, &lat));
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqd(res, DBA_KEY_LON, &lon));

		if (lat != last_lat || lon != last_lon || strcmp(datetime, last_datetime) != 0)
		{
			if (last_datetime[0] != 0)
			{
				if (type == MSG_PILOT || type == MSG_TEMP || type == MSG_TEMP_SHIP)
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
		
			/* Copy the new context informations */
			DBA_RUN_OR_GOTO(cleanup, copy_ana(type, msg, res));

			strncpy(last_datetime, datetime, 20);
			last_lat = lat;
			last_lon = lon;
		}

		/* Insert data about varcode in msg */
		DBA_RUN_OR_GOTO(cleanup, copy_variable(db, msg, varcode, res));
	} while (!is_last);

	if (msg != NULL)
	{
		if (type == MSG_PILOT || type == MSG_TEMP || type == MSG_TEMP_SHIP)
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
	if (cur != NULL)
		dba_db_cursor_delete(cur);
	if (res != NULL)
		dba_record_delete(res);
	if (msg != NULL)
		dba_msg_delete(msg);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_query_msgs(dba_db db, dba_msg_type export_type, dba_record rec, dba_msg_consumer cons, void* data)
{
	const char* query =
		"SELECT pa.id, pa.lat, pa.lon, pa.ident, pa.height, pa.heightbaro,"
		"       pa.block, pa.station, pa.name,"
		"       c.ltype, c.l1, c.l2,"
		"       c.ptype, c.p1, c.p2,"
		"       d.id_var, c.datetime, d.value, ri.id, ri.memo, ri.prio, d.id, a.type, a.value"
		"  FROM pseudoana AS pa, context AS c, repinfo AS ri, data AS d"
		"  LEFT JOIN attr AS a ON a.id_data = d.id"
		" WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	/* Bound variables */
	int out_lat;
	int out_lon;
	char out_ident[64];			SQLINTEGER out_ident_ind;
	int out_height;				SQLINTEGER out_height_ind;
	int out_heightbaro;			SQLINTEGER out_heightbaro_ind;
	int out_block;				SQLINTEGER out_block_ind;
	int out_station;			SQLINTEGER out_station_ind;
	char out_name[255];			SQLINTEGER out_name_ind;
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
	int out_data_id;
	int out_attr_varcode;		SQLINTEGER out_attr_varcode_ind;
	char out_attr_value[255];	SQLINTEGER out_attr_value_ind;
	int last_lat = -1;
	int last_lon = -1;
	int last_data_id = -1;
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
	int rep_cod;
	int pseq = 1;
	int res;

	assert(db);

	/* Get the rep_cod from the query */
	DBA_RUN_OR_RETURN(dba_db_get_rep_cod(db, rec, &rep_cod));

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
	DBA_QUERY_BIND( 5, SQL_C_SLONG, height);
	DBA_QUERY_BIND( 6, SQL_C_SLONG, heightbaro);
	DBA_QUERY_BIND( 7, SQL_C_SLONG, block);
	DBA_QUERY_BIND( 8, SQL_C_SLONG, station);
	DBA_QUERY_BIND( 9, SQL_C_CHAR, name);
	DBA_QUERY_BIND_NONNULL(10, SQL_C_SLONG, leveltype);
	DBA_QUERY_BIND_NONNULL(11, SQL_C_SLONG, l1);
	DBA_QUERY_BIND_NONNULL(12, SQL_C_SLONG, l2);
	DBA_QUERY_BIND_NONNULL(13, SQL_C_SLONG, pindicator);
	DBA_QUERY_BIND_NONNULL(14, SQL_C_SLONG, p1);
	DBA_QUERY_BIND_NONNULL(15, SQL_C_SLONG, p2);
	DBA_QUERY_BIND_NONNULL(16, SQL_C_SLONG, varcode);
	DBA_QUERY_BIND_NONNULL(17, SQL_C_CHAR, datetime);
	DBA_QUERY_BIND_NONNULL(18, SQL_C_CHAR, value);
	DBA_QUERY_BIND_NONNULL(19, SQL_C_SLONG, rep_cod);
	DBA_QUERY_BIND_NONNULL(20, SQL_C_CHAR, rep_memo);
	DBA_QUERY_BIND_NONNULL(21, SQL_C_SLONG, priority);
	DBA_QUERY_BIND_NONNULL(22, SQL_C_SLONG, data_id);
	DBA_QUERY_BIND(23, SQL_C_SLONG, attr_varcode);
	DBA_QUERY_BIND(24, SQL_C_CHAR, attr_value);
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
		if (last_data_id != out_data_id)
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

			last_data_id = out_data_id;
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
		
			switch (export_type)
			{
				case MSG_SYNOP: msg->type = MSG_SYNOP; break;
				case MSG_PILOT: msg->type = MSG_PILOT; break;
				case MSG_TEMP:
				case MSG_TEMP_SHIP:
					switch (rep_cod)
					{
						case 3: msg->type = MSG_TEMP; break;
						case 11: msg->type = MSG_TEMP_SHIP; break;
						default: msg->type = MSG_GENERIC; break;
					}
					break;
				case MSG_AIREP:
				case MSG_AMDAR:
				case MSG_ACARS:
					switch (rep_cod)
					{
						case 12: msg->type = MSG_AIREP; break;
						case 13: msg->type = MSG_AMDAR; break;
						case 14: msg->type = MSG_ACARS; break;
						default: msg->type = MSG_GENERIC; break;
					}
					break;
				case MSG_SHIP:
				case MSG_BUOY:
					switch (rep_cod)
					{
						case 9: msg->type = MSG_BUOY; break;
						case 10: msg->type = MSG_SHIP; break;
						default: msg->type = MSG_GENERIC; break;
					}
					break;
				case MSG_GENERIC: msg->type = MSG_GENERIC; break;
			}

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
