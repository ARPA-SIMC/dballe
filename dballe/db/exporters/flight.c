#include <dballe/msg/dba_msg.h>
#include <dballe/db/dballe.h>

#include <stdlib.h>
#include <string.h>

#include "common.inc"

static dba_err copy_ana_in_flight(dba_msg msg, dba_record rec)
{
	int rep_cod;

	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_REP_COD, &rep_cod));
	switch (rep_cod)
	{
		case 12: msg->type = MSG_AIREP; break;
		case 13: msg->type = MSG_AMDAR; break;
		case 14: msg->type = MSG_ACARS; break;
		default: msg->type = MSG_GENERIC; break;
	}
	
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_IDENT,		rec, DBA_KEY_IDENT));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_YEAR,		rec, DBA_KEY_YEAR));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_MONTH,		rec, DBA_KEY_MONTH));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_DAY,			rec, DBA_KEY_DAY));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_HOUR,		rec, DBA_KEY_HOUR));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_MINUTE,		rec, DBA_KEY_MIN));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_LATITUDE,	rec, DBA_KEY_LAT));
	DBA_RUN_OR_RETURN(set_key(msg, DBA_MSG_LONGITUDE,	rec, DBA_KEY_LON));

	return dba_error_ok();
}

dba_err dba_db_export_flight(dba db, dba_msg** msgs, dba_record query)
{
	/* Read the values from the database */
	dba_err err = DBA_OK;
	dba_cursor cur = NULL;
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
	DBA_RUN_OR_GOTO(cleanup, dba_query(db, query, &cur, &count));
	/*fprintf(stderr, "Count: %d\n", count);*/
	if (count == 0)
		goto cleanup;

	mlist = (dba_msg*)calloc(1, (count + 1) * sizeof(dba_msg));
	if (mlist == NULL)
	{
		err = dba_error_alloc("allocating space for an array of flights");
		goto cleanup;
	}
	i = 0;

	DBA_RUN_OR_GOTO(cleanup, dba_record_create(&res));

	do {
		dba_varcode varcode;
		double lat;
		double lon;
		const char* datetime;

		DBA_RUN_OR_GOTO(cleanup, dba_cursor_next(cur, res, &varcode, &is_last));

		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqc(res, DBA_KEY_DATETIME, &datetime));
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqd(res, DBA_KEY_LAT, &lat));
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqd(res, DBA_KEY_LON, &lon));

		if (lat != last_lat || lon != last_lon || strcmp(datetime, last_datetime) != 0)
		{
			if (last_datetime[0] != 0)
			{
//				dba_unset(msg->context, "leveltype"); dba_unset(msg->context, "l1"); dba_unset(msg->context, "l2");
//				dba_unset(msg->context, "pindicator"); dba_unset(msg->context, "p1"); dba_unset(msg->context, "p2");
				mlist[i++] = msg;
				msg = NULL;
			}

			DBA_RUN_OR_GOTO(cleanup, dba_msg_create(&msg));
		
			/* Copy the new context informations */
			DBA_RUN_OR_GOTO(cleanup, copy_ana_in_flight(msg, res));

			strncpy(last_datetime, datetime, 20);
			last_lat = lat;
			last_lon = lon;
		}

		/* Insert data about varcode in msg */
		DBA_RUN_OR_GOTO(cleanup, copy_variable(db, msg, varcode, res));
	} while (!is_last);

	if (msg != NULL)
	{
		/*
		dba_unset(msg->context, "leveltype"); dba_unset(msg->context, "l1"); dba_unset(msg->context, "l2");
		dba_unset(msg->context, "pindicator"); dba_unset(msg->context, "p1"); dba_unset(msg->context, "p2");
		*/
		mlist[i++] = msg;
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
		dba_cursor_delete(cur);
	if (res != NULL)
		dba_record_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}


/* vim:set ts=4 sw=4: */
