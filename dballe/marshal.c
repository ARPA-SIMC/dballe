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

#include <dballe/marshal.h>

#include <dballe/aof/decoder.h>
#include <dballe/bufrex/bufrex.h>

dba_err dba_marshal_decode(dba_rawmsg rmsg, dba_msg *msg)
{
	switch (rmsg->encoding)
	{
		case BUFR: DBA_RUN_OR_RETURN(bufrex_decode_bufr(rmsg, msg)); break;
		case CREX: DBA_RUN_OR_RETURN(bufrex_decode_crex(rmsg, msg)); break;
		case AOF: DBA_RUN_OR_RETURN(aof_decoder_decode(rmsg, msg)); break;
	}
	return dba_error_ok();
}

dba_err dba_marshal_encode(dba_msg msg, dba_encoding type, dba_rawmsg *rmsg)
{
	switch (type)
	{
		case BUFR: DBA_RUN_OR_RETURN(bufrex_encode_bufr(msg, 0, 0, rmsg)); break;
		case CREX: DBA_RUN_OR_RETURN(bufrex_encode_crex(msg, 0, 0, rmsg)); break;
		case AOF: return dba_error_unimplemented("exporting to AOF"); break;
	}
	return dba_error_ok();
}


/* vim:set ts=4 sw=4: */
