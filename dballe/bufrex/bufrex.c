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

#include "bufrex_msg.h"
#include "exporters/exporters.h"
#include <dballe/core/verbose.h>


#include <stdio.h>
#include <stdlib.h>	/* malloc */

dba_err bufrex_decode_bufr(dba_rawmsg raw, dba_msgs* msgs)
{
	dba_err err = DBA_OK;
	bufrex_msg rmsg = NULL;
	dba_msgs res = NULL;

	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&rmsg, BUFREX_BUFR));
	DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&res));
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_decode(rmsg, raw));
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "Decoded BUFR data:\n");
		bufrex_msg_print(rmsg, DBA_VERBOSE_STREAM);
	}
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_to_dba_msgs(rmsg, res));
	*msgs = res;
	res = NULL;

cleanup:
	if (rmsg != NULL)
		bufrex_msg_delete(rmsg);
	if (res != NULL)
		dba_msgs_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_decode_crex(dba_rawmsg raw, dba_msgs* msgs)
{
	dba_err err = DBA_OK;
	bufrex_msg rmsg = NULL;
	dba_msgs res = NULL;

	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&rmsg, BUFREX_CREX));
	DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&res));
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_decode(rmsg, raw));
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "Decoded CREX data:\n");
		bufrex_msg_print(rmsg, DBA_VERBOSE_STREAM);
	}
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_to_dba_msgs(rmsg, res));
	*msgs = res;
	res = NULL;

cleanup:
	if (rmsg != NULL)
		bufrex_msg_delete(rmsg);
	if (res != NULL)
		dba_msgs_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_encode_bufr(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_msg braw = NULL;
	*raw = NULL;

	if (msgs->len == 0) return dba_error_consistency("tried to encode an empty dba_msgs");

	/* Create and setup the bufrex_msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&braw, BUFREX_BUFR));

	/* Compute the right type and subtype if missing */
	if (type == 0 || subtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msgs->msgs[0], &(braw->type), &(braw->subtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
	}

	/* Setup encoding parameters */
	if (msgs->msgs[0]->type == MSG_GENERIC)
	{
		braw->opt.bufr.origin = 200;
		braw->opt.bufr.master_table = 12;
		braw->opt.bufr.local_table = 0;
	} else {
		braw->opt.bufr.origin = 98;
		braw->opt.bufr.master_table = 6;
		braw->opt.bufr.local_table = 1;
	}

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_from_dba_msgs(braw, msgs));

	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "BUFR data before encoding:\n");
		bufrex_msg_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_msg_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_encode_crex(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_msg braw = NULL;
	*raw = NULL;

	if (msgs->len == 0) return dba_error_consistency("tried to encode an empty dba_msgs");

	/* Create and setup the bufrex_msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&braw, BUFREX_CREX));

	/* Compute the right type and subtype if missing */
	if (type == 0 || subtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msgs->msgs[0], &(braw->type), &(braw->subtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
	}

	/* Setup encoding parameters */
	if (msgs->msgs[0]->type == MSG_GENERIC)
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
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_from_dba_msgs(braw, msgs));
	
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "CREX data before encoding:\n");
		bufrex_msg_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_msg_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
