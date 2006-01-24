#include <dballe/db/dba_export.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/db/dballe.h>

#include <stdlib.h>
#include <string.h>

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

/* vim:set ts=4 sw=4 syntax=c: */

dba_err dba_db_export(dba_db db, dba_msg_type type, dba_msg** msgs, dba_record query)
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
	dba_msg* mlist = NULL;
	dba_msg msg = NULL;

	last_datetime[0] = 0;

	/* Query the values */
	DBA_RUN_OR_GOTO(cleanup, dba_db_query(db, query, &cur, &count));
	/*fprintf(stderr, "Count: %d\n", count);*/
	if (count == 0)
		goto cleanup;

	mlist = (dba_msg*)calloc(1, (count + 1) * sizeof(dba_msg));
	if (mlist == NULL)
	{
		err = dba_error_alloc("allocating space for an array of messages");
		goto cleanup;
	}
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
				if (type == MSG_TEMP || type == MSG_TEMP_SHIP)
				{
					dba_msg copy;
					DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_pack_levels(msg, &copy));
					/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
					mlist[i++] = copy;
					dba_msg_delete(msg);
				} else {
					mlist[i++] = msg;
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
		if (type == MSG_TEMP || type == MSG_TEMP_SHIP)
		{
			dba_msg copy;
			DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_pack_levels(msg, &copy));
			/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
			mlist[i++] = copy;
			dba_msg_delete(msg);
		} else {
			mlist[i++] = msg;
		}
		msg = NULL;
	}

	*msgs = mlist;

cleanup:
	if (err != DBA_OK && mlist != NULL)
	{
		for (i = 0; mlist[i] != NULL; i++)
			dba_msg_delete(mlist[i]);
		free(mlist);
	}
	if (cur != NULL)
		dba_db_cursor_delete(cur);
	if (res != NULL)
		dba_record_delete(res);
	if (msg != NULL)
		dba_msg_delete(msg);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
