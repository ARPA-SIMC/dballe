#include "config.h"

#include <dballe/db/import.h>
#include <dballe/db/internals.h>
#include <dballe/db/pseudoana.h>
#include <dballe/db/context.h>
#include <dballe/db/data.h>
#include <dballe/db/attr.h>
#include <dballe/db/dba_db.h>

#include <dballe/conv/dba_conv.h>
#include <dballe/msg/dba_msg.h>

#include <string.h>
#include <stdlib.h>

#if 0
static dba_err dba_db_insert_rec(dba_db db, dba_record rec, int lt, int l1, int l2, int pi, int p1, int p2, int overwrite)
{
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE, lt));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L1, l1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L2, l2));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_PINDICATOR, pi));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P1, p1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P2, p2));
	DBA_RUN_OR_RETURN(dba_db_insert_or_replace(db, rec, overwrite, overwrite, NULL));
	return dba_error_ok();
}

static dba_err dba_db_insert_attrs(dba_db db, dba_record rec, int overwrite)
{
	dba_err err = DBA_OK;
	dba_record qc = NULL;
	dba_record_cursor item;
	
	DBA_RUN_OR_RETURN(dba_record_create(&qc));
	for (item = dba_record_iterate_first(rec); item != NULL;
			item = dba_record_iterate_next(rec, item))
	{
		int id = dba_record_cursor_id(item);
		dba_var var = dba_record_cursor_variable(item);
		dba_var_attr_iterator ai;
		for (ai = dba_var_attr_iterate(var); ai != NULL; ai = dba_var_attr_iterator_next(ai))
		{
			dba_var attr = dba_var_attr_iterator_attr(ai);
			DBA_RUN_OR_RETURN(dba_record_var_set_direct(qc, attr));
		}
		DBA_RUN_OR_GOTO(cleanup, dba_db_qc_insert_or_replace(db, id, qc, overwrite));
		dba_record_clear_vars(qc);
	}

cleanup:
	if (qc != NULL)
		dba_record_delete(qc);
	return err = DBA_OK ? dba_error_ok() : err;
}

static dba_err dba_db_import_get_key(dba_record dst, dba_keyword key, dba_msg src, int id)
{
	dba_msg_datum d = dba_msg_find_by_id(src, id);
	if (d == NULL)
		return dba_record_key_unset(dst, key);
	else
		return dba_record_key_set(dst, key, d->var);
}

static dba_err dba_db_import_common(dba_db db, dba_record rec, dba_msg msg, int overwrite)
{
	int i;
	int oltype = -1, ol1 = -1, ol2 = -1, opind = -1, op1 = -1, op2 = -1;
	 
	/* Fill in common anagraphical context fields */
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_YEAR, msg, DBA_MSG_YEAR));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_MONTH, msg, DBA_MSG_MONTH));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_DAY, msg, DBA_MSG_DAY));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_HOUR, msg, DBA_MSG_HOUR));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_MIN, msg, DBA_MSG_MINUTE));
	//DBA_RUN_OR_GOTO(cleanup, dba_setvar(rec, "sec", msg->var_second));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_LAT, msg, DBA_MSG_LATITUDE));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_LON, msg, DBA_MSG_LONGITUDE));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_HEIGHT, msg, DBA_MSG_HEIGHT));
	DBA_RUN_OR_RETURN(dba_db_import_get_key(rec, DBA_KEY_HEIGHTBARO, msg, DBA_MSG_HEIGHT_BARO));

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level l = msg->data[i];
		int j;
		for (j = 0; j < l->data_count; j++)
		{
			dba_msg_datum d = l->data[j];
			if (l->ltype != oltype || l->l1 != ol1 || l->l2 != ol2 ||
				d->pind != opind || d->p1 != op1 || d->p2 != op2)
			{
				if (oltype != -1)
				{
					DBA_RUN_OR_RETURN(dba_db_insert_rec(db, rec,  oltype, ol1, ol2,  opind, op1, op2, overwrite));
					DBA_RUN_OR_RETURN(dba_db_insert_attrs(db, rec, overwrite));
					dba_record_clear_vars(rec);
				}
				oltype = l->ltype;
				ol1 = l->l1;
				ol2 = l->l2;
				opind = d->pind;
				op1 = d->p1;
				op2 = d->p2;
			}
			DBA_RUN_OR_RETURN(dba_record_var_set_direct(rec, d->var));
		}
	}
	if (oltype != -1)
	{
		DBA_RUN_OR_RETURN(dba_db_insert_rec(db, rec,  oltype, ol1, ol2,  opind, op1, op2, overwrite));
		DBA_RUN_OR_RETURN(dba_db_insert_attrs(db, rec, overwrite));
	}

	return dba_error_ok();
}

dba_err dba_import_msg_old(dba_db db, dba_msg msg, int overwrite)
{
	dba_err err = DBA_OK;
	dba_record rec = NULL;

	DBA_RUN_OR_RETURN(dba_record_create(&rec));

	switch (msg->type)
	{
		case MSG_SYNOP:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_BLOCK, msg, DBA_MSG_BLOCK));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_STATION, msg, DBA_MSG_STATION));
			break;
		case MSG_SHIP:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 10));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_IDENT, msg, DBA_MSG_IDENT));
			break;
		case MSG_BUOY:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 9));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_IDENT, msg, DBA_MSG_IDENT));
			break;
		case MSG_AIREP:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 12));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_IDENT, msg, DBA_MSG_IDENT));
			break;
		case MSG_AMDAR:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 13));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_IDENT, msg, DBA_MSG_IDENT));
			break;
		case MSG_ACARS:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 14));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_IDENT, msg, DBA_MSG_IDENT));
			break;
		case MSG_PILOT:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 4));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_BLOCK, msg, DBA_MSG_BLOCK));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_STATION, msg, DBA_MSG_STATION));
			break;
		case MSG_TEMP:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 3));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_BLOCK, msg, DBA_MSG_BLOCK));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_STATION, msg, DBA_MSG_STATION));
			break;
		case MSG_TEMP_SHIP:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 11));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_IDENT, msg, DBA_MSG_IDENT));
			break;
		case MSG_GENERIC:
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, 255));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_BLOCK, msg, DBA_MSG_BLOCK));
			DBA_RUN_OR_GOTO(cleanup, dba_db_import_get_key(rec, DBA_KEY_STATION, msg, DBA_MSG_STATION));
			break;
	}

	DBA_RUN_OR_GOTO(cleanup, dba_db_import_common(db, rec, msg, overwrite));

cleanup:
	if (rec != NULL)
		dba_record_delete(rec);
	return err = DBA_OK ? dba_error_ok() : err;
}
#endif


dba_err dba_import_msg(dba_db db, dba_msg msg, int overwrite)
{
	dba_err err = DBA_OK;
	dba_msg_level l_ana = dba_msg_find_level(msg, 257, 0, 0);
	dba_msg_datum d;
	dba_db_pseudoana da;
	dba_db_context dc;
	dba_db_data dd;
	dba_db_attr dq;
	int i, j;
	int mobile;

	da = db->pseudoana;
	dc = db->context;
	dd = db->data;
	dq = db->attr;
	
	switch (msg->type)
	{
		case MSG_SYNOP:		mobile = 0; dc->id_report = 1;	break;
		case MSG_SHIP:		mobile = 1; dc->id_report = 10;	break;
		case MSG_BUOY:		mobile = 1; dc->id_report = 9;	break;
		case MSG_AIREP:		mobile = 1; dc->id_report = 12;	break;
		case MSG_AMDAR:		mobile = 1; dc->id_report = 13;	break;
		case MSG_ACARS:		mobile = 1; dc->id_report = 14;	break;
		case MSG_PILOT:		mobile = 0; dc->id_report = 4;	break;
		case MSG_TEMP:		mobile = 0; dc->id_report = 3;	break;
		case MSG_TEMP_SHIP:	mobile = 1; dc->id_report = 11;	break;
		case MSG_GENERIC:
		default:			mobile = 0; dc->id_report = 255;	break;
	}

	DBA_RUN_OR_RETURN(dba_db_begin(db));

	// Fill up the pseudoana informations needed to fetch an existing ID

	/* Latitude */
	if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_LATITUDE)) != NULL)
		DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &(da->lat)));

	/* Longitude */
	if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_LONGITUDE)) != NULL)
		DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &(da->lon)));

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

	// Check if we can reuse a pseudoana row
	DBA_RUN_OR_GOTO(fail, dba_db_pseudoana_get_id(da, &(dc->id_ana)));

	if (dc->id_ana == -1 || overwrite)
	{
		// Fill up the rest of pseudoana informations

		if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_BLOCK)) != NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &(da->block)));
			da->block_ind = 0;
		}
		else
			da->block_ind = SQL_NULL_DATA;

		if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_STATION)) != NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &(da->station)));
			da->station_ind = 0;
		}
		else
			da->station_ind = SQL_NULL_DATA;

		if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_HEIGHT)) != NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &(da->height)));
			da->height_ind = 0;
		}
		else
			da->height_ind = SQL_NULL_DATA;

		if ((d = dba_msg_level_find_by_id(l_ana, DBA_MSG_HEIGHT_BARO)) != NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &(da->heightbaro)));
			da->heightbaro_ind = 0;
		}
		else
			da->heightbaro_ind = SQL_NULL_DATA;

		// TODO: char name[255]; SQLINTEGER name_ind;
		da->name_ind = SQL_NULL_DATA;

		if (dc->id_ana == -1)
			DBA_RUN_OR_GOTO(fail, dba_db_pseudoana_insert(da, &(dc->id_ana)));
		else
			DBA_RUN_OR_GOTO(fail, dba_db_pseudoana_update(da));
	}

	// Fill up the date in context
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

		dc->date_ind = snprintf(dc->date, 25,
				"%04ld-%02ld-%02ld %02ld:%02ld:00",
					strtol(year, 0, 10),
					strtol(month, 0, 10),
					strtol(day, 0, 10),
					strtol(hour, 0, 10),
					strtol(min, 0, 10));
	}

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		int old_pind = -1;
		int old_p1 = -1;
		int old_p2 = -1;

		// Fill in the context
		dc->ltype = lev->ltype;
		dc->l1 = lev->l1;
		dc->l2 = lev->l2;

		for (j = 0; j < lev->data_count; j++)
		{
			dba_msg_datum dat = lev->data[j];
			dba_var_attr_iterator iter;

			if (0)
			{
				dba_varcode code = dba_var_code(dat->var);
				/* Don't insert anagraphical informations that are already
				 * memorised in pseudoana and context */
				if (lev->ltype == 257 && lev->l1 == 0 && lev->l1 == 0 &&
					dat->pind == 0 && dat->p1 == 0 && dat->p2 == 0)
					if ((code >= DBA_VAR(0, 4, 1) && code <= DBA_VAR(0, 4, 5)) ||
						code == DBA_VAR(0, 5, 1) || code == DBA_VAR(0, 6, 1) ||
						code == DBA_VAR(0, 1, 11))
						continue;
			}

			if (dat->pind != old_pind || dat->p1 != old_p1 || dat->p2 != old_p2)
			{
				// Insert the new context when the datum coordinates change
				dc->pind = dat->pind;
				dc->p1 = dat->p1;
				dc->p2 = dat->p2;

				DBA_RUN_OR_GOTO(fail, dba_db_context_get_id(dc, &(dd->id_context)));
				if (dd->id_context == -1)
					DBA_RUN_OR_GOTO(fail, dba_db_context_insert(dc, &(dd->id_context)));

				old_pind = dat->pind;
				old_p1 = dat->p1;
				old_p2 = dat->p2;
			}

			// Insert the variable
			dba_db_data_set(dd, dat->var);
			DBA_RUN_OR_GOTO(fail, dba_db_data_insert(dd, overwrite, &(dq->id_data)));

			// Insert the attributes
			for (iter = dba_var_attr_iterate(dat->var); iter != NULL; 
					iter = dba_var_attr_iterator_next(iter))
			{
				dba_var attr = dba_var_attr_iterator_attr(iter);
				if (dba_var_value(attr) != NULL)
				{
					dba_db_attr_set(dq, attr);
					DBA_RUN_OR_GOTO(fail, dba_db_attr_insert(dq, overwrite));
				}
			}
		}
	}

    DBA_RUN_OR_GOTO(fail, dba_db_commit(db));
    return dba_error_ok();

fail:
	dba_db_rollback(db);
	return err;
}

#if 0
/*
 * If can_replace, then existing data can be rewritten, else it can only add new data
 *
 * If update_pseudoana, then the pseudoana informations are overwritten using
 * information from `rec'; else data from `rec' is written into pseudoana only
 * if there is no suitable anagraphical data for it.
 */
dba_err dba_db_insert_or_replace(dba_db db, dba_record rec, int can_replace, int update_pseudoana, int* ana_id)
{
	/*
	 * FIXME: REPLACE will change the ID of the replaced rows, breaking the
	 * connection with the attributes.
	 * If on MySQL 4.1, we can use INSERT ON DUPLICATE KEY UPDATE value=?
	 * If on MySQL 4.1.1, we can use INSERT ON DUPLICATE KEY UPDATE value=VALUES(value)
	 * Else, we need to do a select first to get the ID.
	 */
	const char* insert_query =
		"INSERT INTO data (id_context, id_var, value)"
		" VALUES(?, ?, ?)";
	const char* replace_query =
		"INSERT INTO data (id_context, id_var, value)"
		" VALUES(?, ?, ?) ON DUPLICATE KEY UPDATE value=VALUES(value)";
	dba_err err;
	dba_record_cursor item;
	int id_pseudoana;
	int id_context;
	dba_varcode id_var;
	char value[255];
	SQLINTEGER value_ind;
	SQLHSTMT stm;
	int res;
	
	assert(db);

	/* Check for the existance of non-context data, otherwise it's all useless */
	if (dba_record_iterate_first(rec) == NULL)
		return dba_error_consistency("looking for data to insert");

	/* Begin the transaction */
	DBA_RUN_OR_RETURN(dba_db_begin(db));

	/* Insert the pseudoana data, and get the ID */
	DBA_RUN_OR_GOTO(failed1, dba_insert_pseudoana(db, rec, &id_pseudoana, update_pseudoana));

	/* Insert the context data, and get the ID */
	DBA_RUN_OR_GOTO(failed1, dba_insert_context(db, rec, id_pseudoana, &id_context));

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
		goto failed1;
	}

	/* Compile the SQL query */
	/* Casting to char* because ODBC is unaware of const */
	res = SQLPrepare(stm, (unsigned char*)(can_replace ? replace_query : insert_query), SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'data'");
		goto failed;
	}

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_context, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &id_var, 0, 0);
	SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, value, 0, &value_ind);

	/* Insert all found variables */
	for (item = dba_record_iterate_first(rec); item != NULL;
			item = dba_record_iterate_next(rec, item))
	{
		/* Datum to be inserted, linked to id_pseudoana and all the other IDs */
		dba_var var = dba_record_cursor_variable(item);
		const char* cur_value;

		DBA_RUN_OR_GOTO(failed, dba_var_enqc(var, &cur_value));

		/* Variable ID */
		id_var = dba_var_code(var);

		/* Variable value */
		if ((value_ind = strlen(cur_value)) > 256)
			value_ind = 255;
		strncpy(value, cur_value, value_ind);
		value[value_ind] = 0;

		/*
		fprintf(stderr, "Inserting %d %s[%d] %s[%d] %d %d %d %d\n",
				id_var, value, value_ind, datebuf, datebuf_ind, id_report, id_pseudoana, id_levellayer, id_timerange);
		*/

		res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'data'");
			goto failed;
		}

		{
			int id;
			DBA_RUN_OR_GOTO(failed, dba_db_last_insert_id(db, &id));
			dba_record_cursor_set_id(item, id);
		}
	}

	if (ana_id != NULL)
		*ana_id = id_pseudoana;
			
	SQLFreeHandle(SQL_HANDLE_STMT, stm);

	DBA_RUN_OR_GOTO(failed1, dba_db_commit(db));

	return dba_error_ok();

	/* Exits with cleanup after error */
failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
failed1:
	dba_db_rollback(db);
	return err;
}


dba_err dba_db_qc_insert_or_replace(dba_db db, int id_data, /*dba_record rec, dba_varcode var,*/ dba_record qc, int can_replace)
{
	const char* insert_query =
		"INSERT INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?)";
	const char* replace_query =
		"INSERT INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?) ON DUPLICATE KEY UPDATE value=VALUES(value)";
/*		"REPLACE INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?)"; */
	dba_err err;
	dba_record_cursor item;
#if 0
	int id_data;
#endif
	dba_varcode type;
	char value[255];
	SQLINTEGER value_ind;
	SQLHSTMT stm;
	int res;
	
	assert(db);

	/* Begin the transaction */
	DBA_RUN_OR_RETURN(dba_db_begin(db));

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		dba_db_rollback(db);
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

	/* Compile the INSERT/UPDATE SQL query */
	/* Casting to char* because ODBC is unaware of const */
	res = SQLPrepare(stm, (unsigned char*)(can_replace ? replace_query : insert_query), SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'attr'");
		goto dba_qc_insert_failed;
	}

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_data, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &type, 0, 0);
	SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, value, 0, &value_ind);
	
	/* Insert all found variables */
	for (item = dba_record_iterate_first(qc); item != NULL;
			item = dba_record_iterate_next(qc, item))
	{
		dba_var variable = dba_record_cursor_variable(item);
		const char* cur_value = dba_var_value(variable);

		/* Variable ID */
		type = dba_var_code(variable);

		/* Variable value */
		if ((value_ind = strlen(cur_value)) > 256)
			value_ind = 255;
		strncpy(value, cur_value, value_ind);
		value[value_ind] = 0;

		res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'attr'");
			goto dba_qc_insert_failed;
		}
	}
			
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	if ((err = dba_db_commit(db)))
	{
		dba_db_rollback(db);
		return err;
	}
	return dba_error_ok();

	/* Exits with cleanup after error */
dba_qc_insert_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	dba_db_rollback(db);
	return err;
}

#endif


/* vim:set ts=4 sw=4: */
