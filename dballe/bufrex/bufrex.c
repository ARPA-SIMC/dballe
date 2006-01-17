#include "bufrex_raw.h"
#include "exporters/exporters.h"
#include <dballe/core/verbose.h>


#include <stdio.h>
#include <stdlib.h>	/* malloc */

dba_err bufrex_decode_bufr(dba_rawmsg raw, dba_msg* msg)
{
	dba_err err = DBA_OK;
	bufrex_raw rmsg = NULL;

	DBA_RUN_OR_RETURN(bufrex_raw_create(&rmsg, BUFREX_BUFR));
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_decode(rmsg, raw));
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_RAW))
	{
		dba_verbose(DBA_VERB_BUFREX_RAW, "Decoded BUFR data:\n");
		bufrex_raw_print(rmsg, DBA_VERBOSE_STREAM);
	}
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_to_msg(rmsg, msg));

cleanup:
	if (rmsg != NULL)
		bufrex_raw_delete(rmsg);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_decode_crex(dba_rawmsg raw, dba_msg* msg)
{
	dba_err err = DBA_OK;
	bufrex_raw rmsg = NULL;

	DBA_RUN_OR_RETURN(bufrex_raw_create(&rmsg, BUFREX_CREX));
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_decode(rmsg, raw));
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_RAW))
	{
		dba_verbose(DBA_VERB_BUFREX_RAW, "Decoded CREX data:\n");
		bufrex_raw_print(rmsg, DBA_VERBOSE_STREAM);
	}
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_to_msg(rmsg, msg));

cleanup:
	if (rmsg != NULL)
		bufrex_raw_delete(rmsg);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_encode_bufr(dba_msg msg, int type, int subtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_raw braw = NULL;
	*raw = NULL;

	/* Create and setup the bufrex_raw */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_create(&braw, BUFREX_BUFR));

	/* Compute the right type and subtype if missing */
	if (type == 0 || subtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msg, &(braw->type), &(braw->subtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
	}

	/* Setup encoding parameters */
	if (msg->type == MSG_GENERIC)
	{
		braw->opt.bufr.origin = 255;
		braw->opt.bufr.master_table = 12;
		braw->opt.bufr.local_table = 0;
	} else {
		braw->opt.bufr.origin = 98;
		braw->opt.bufr.master_table = 6;
		braw->opt.bufr.local_table = 1;
	}

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_from_msg(braw, msg));

	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_RAW))
	{
		dba_verbose(DBA_VERB_BUFREX_RAW, "BUFR data before encoding:\n");
		bufrex_raw_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_raw_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_encode_crex(dba_msg msg, int type, int subtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_raw braw = NULL;
	*raw = NULL;

	/* Create and setup the bufrex_raw */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_create(&braw, BUFREX_CREX));

	/* Compute the right type and subtype if missing */
	if (type == 0 || subtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msg, &(braw->type), &(braw->subtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
	}

	/* Setup encoding parameters */
	if (msg->type == MSG_GENERIC)
	{
		braw->opt.crex.master_table = 99;
		braw->edition = 2;
		braw->opt.crex.table = 3;
	} else {
		braw->opt.crex.master_table = 0;
		braw->edition = 1;
		braw->opt.crex.table = 3;
	}

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_from_msg(braw, msg));
	
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_RAW))
	{
		dba_verbose(DBA_VERB_BUFREX_RAW, "CREX data before encoding:\n");
		bufrex_raw_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_raw_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
