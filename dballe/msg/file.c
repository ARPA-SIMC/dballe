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

#include "file.h"
#include "aof_codec.h"
#include "bufrex_codec.h"
#include "marshal.h"

#include <stdlib.h>

dba_err dba_file_read_msgs(dba_file file, dba_msgs* msgs, int* found)
{
	dba_err err = DBA_OK;
	dba_rawmsg rm = NULL;
	
	DBA_RUN_OR_RETURN(dba_rawmsg_create(&rm));
	DBA_RUN_OR_GOTO(cleanup, dba_file_read(file, rm, found));
	if (*found)
		/* Parse the message */
		DBA_RUN_OR_GOTO(cleanup, dba_marshal_decode(rm, msgs));
	else
		*msgs = NULL;

cleanup:
	if (rm != NULL)
		dba_rawmsg_delete(rm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_file_write_msgs(dba_file file, dba_msgs msgs, int cat, int subcat)
{
	dba_err err = DBA_OK;
	dba_rawmsg raw = NULL;

	switch (dba_file_type(file))
	{
		case BUFR:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_bufr(msgs, cat, subcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write(file, raw));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_crex(msgs, cat, subcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write(file, raw));
			break;
		case AOF: 
			err = dba_error_unimplemented("export to AOF format");
			goto cleanup;
	}

cleanup:
	if (raw != NULL)
		dba_rawmsg_delete(raw);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
