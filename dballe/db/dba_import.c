#include "config.h"

#include <dballe/db/dba_import.h>
#include <dballe/db/dballe.h>

#include <dballe/conv/dba_conv.h>
#include <dballe/msg/dba_msg.h>

static dba_err dba_db_insert_rec(dba db, dba_record rec, int lt, int l1, int l2, int pi, int p1, int p2)
{
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE, lt));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L1, l1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L2, l2));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_PINDICATOR, pi));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P1, p1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P2, p2));
	DBA_RUN_OR_RETURN(dba_insert(db, rec));
	return dba_error_ok();
}

static dba_err dba_db_import_get_key(dba_record dst, dba_keyword key, dba_msg src, int id)
{
	dba_msg_datum d = dba_msg_find_by_id(src, id);
	if (d == NULL)
		return dba_record_key_unset(dst, key);
	else
		return dba_record_key_set(dst, key, d->var);
}

static dba_err dba_db_import_common(dba db, dba_record rec, dba_msg msg)
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
					DBA_RUN_OR_RETURN(dba_db_insert_rec(db, rec,  oltype, ol1, ol2,  opind, op1, op2));
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
		DBA_RUN_OR_RETURN(dba_db_insert_rec(db, rec,  oltype, ol1, ol2,  opind, op1, op2));

	return dba_error_ok();
}

dba_err dba_import_msg(dba db, dba_msg msg)
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

	DBA_RUN_OR_GOTO(cleanup, dba_db_import_common(db, rec, msg));

cleanup:
	if (rec != NULL)
		dba_record_delete(rec);
	return err = DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
