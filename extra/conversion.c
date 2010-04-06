/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "conversion.h"
#include "processor.h"

#include <dballe/msg/bufrex_codec.h>
/* #include <dballe/aof/aof_encoder.h> */
#include <dballe/msg/file.h>

static dba_err process_bufrex_msg(bufrex_msg msg, dba_file file)
{
	dba_rawmsg raw;
	DBA_RUN_OR_RETURN(bufrex_msg_encode(msg, &raw));
	DBA_RUN_OR_RETURN(dba_file_write(file, raw));
	dba_rawmsg_delete(raw);
	return dba_error_ok();
}

static dba_err process_dba_msg(dba_msgs msgs, dba_file file, int type, int subtype, int localsubtype)
{
	dba_encoding ftype = dba_file_type(file);

	switch (ftype)
	{
		case BUFR:
		{
			dba_rawmsg raw;
			DBA_RUN_OR_RETURN(bufrex_encode_bufr(msgs, type, subtype, localsubtype, &raw));
			DBA_RUN_OR_RETURN(dba_file_write(file, raw));
			dba_rawmsg_delete(raw);
			break;
		}
		case CREX:
		{
			dba_rawmsg raw;
			DBA_RUN_OR_RETURN(bufrex_encode_crex(msgs, type, localsubtype, &raw));
			DBA_RUN_OR_RETURN(dba_file_write(file, raw));
			dba_rawmsg_delete(raw);
			break;
		}
		case AOF:
		{
			return dba_error_unimplemented("encoding of AOF messages");
			/*
			dba_rawmsg rmsg;
			DBA_RUN_OR_RETURN(dba_rawmsg_create(&rmsg));
			DBA_RUN_OR_RETURN(aof_encoder_encode(m, rmsg));
			DBA_RUN_OR_RETURN(((bufrex_action)data->outAction)(rmsg, m, data->outActionData));
			dba_rawmsg_delete(rmsg);
			break;
			*/
		}
	}
	return dba_error_ok();
}

dba_err convert_message(dba_rawmsg msg, bufrex_msg braw, dba_msgs decoded, void* data)
{
	struct conversion_info* info = (struct conversion_info*)data;

	if (decoded == NULL || decoded->len == 0)
	{
		fprintf(stderr, "No interpreted information available: is a recoding enough?\n");
		// See if we can just recode the raw data

		// We want bufrex raw data
		if (braw == NULL)
		{
			fprintf(stderr, "No BUFREX raw data to attempt low-level bufrex recoding\n");
			return dba_error_ok();
		}

		// No report override
		if (info->dest_rep_memo != NULL)
		{
			fprintf(stderr, "report override not allowed for low-level bufrex recoding\n");
			return dba_error_ok();
		}

		// Same encoding
		if ((dba_file_type(info->file) == BUFR && braw->encoding_type == BUFREX_CREX)
		     || (dba_file_type(info->file) == CREX && braw->encoding_type == BUFREX_BUFR))
		{
			fprintf(stderr, "encoding change not yet supported for low-level bufrex recoding\n");
			return dba_error_ok();
		}

		// No template change
		if ((info->dest_type != 0 || info->dest_subtype != 0 || info->dest_localsubtype != 0)
		        || (info->dest_type == braw->type && info->dest_subtype == braw->subtype && info->dest_localsubtype == braw->localsubtype))
		{
			fprintf(stderr, "template change not supported for low-level bufrex recoding\n");
			return dba_error_ok();
		}

		// We can just recode the raw braw
		fprintf(stderr, "we can do a low-level bufrex recoding\n");
		return process_bufrex_msg(braw, info->file);
	}

	if (info->dest_rep_memo != NULL)
	{
		// Force message type (will also influence choice of template later)
		dba_msg_type type = dba_msg_type_from_repmemo(info->dest_rep_memo);
		size_t i;
		for (i = 0; i < decoded->len; ++i)
			decoded->msgs[i]->type = type;
	}

	if (info->dest_type != 0 || info->dest_subtype != 0 || info->dest_localsubtype != 0)
		// Force template
		DBA_RUN_OR_RETURN(process_dba_msg(decoded, info->file, info->dest_type, info->dest_subtype, info->dest_localsubtype));
	else if (braw != NULL && info->dest_rep_memo == NULL)
		// Preserve template
		DBA_RUN_OR_RETURN(process_dba_msg(decoded, info->file, braw->subtype == 0 ? 0 : braw->type, braw->subtype, braw->localsubtype));
	else
		// Choose appropriate template
		DBA_RUN_OR_RETURN(process_dba_msg(decoded, info->file, 0, 0, 0));

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
