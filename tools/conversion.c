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

#include "conversion.h"
#include "processor.h"

#include <dballe/bufrex/bufrex.h>
/* #include <dballe/aof/aof_encoder.h> */
#include <dballe/dba_file.h>

static dba_err process_dba_msg(dba_msgs msgs, dba_file file, int type, int subtype)
{
	dba_encoding ftype = dba_file_get_type(file);

	switch (ftype)
	{
		case BUFR:
		{
			dba_rawmsg raw;
			DBA_RUN_OR_RETURN(bufrex_encode_bufr(msgs, type, subtype, &raw));
			DBA_RUN_OR_RETURN(dba_file_write_raw(file, raw));
			dba_rawmsg_delete(raw);
			break;
		}
		case CREX:
		{
			dba_rawmsg raw;
			DBA_RUN_OR_RETURN(bufrex_encode_crex(msgs, type, subtype, &raw));
			DBA_RUN_OR_RETURN(dba_file_write_raw(file, raw));
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
	dba_file file = (dba_file)data;

	if (decoded == NULL)
		return dba_error_ok();

	if (braw != NULL)
		DBA_RUN_OR_RETURN(process_dba_msg(decoded, file, braw->subtype == 0 ? 0 : braw->type, braw->subtype));
	else
		DBA_RUN_OR_RETURN(process_dba_msg(decoded, file, 0, 0));

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
