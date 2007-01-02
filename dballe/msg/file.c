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
#include "readers.h"
#include "writers.h"
#include "aof_codec.h"
#include "bufrex_codec.h"
#include "marshal.h"

#include <stdlib.h>

struct _dba_file
{
	dba_encoding type;
	dba_rawfile rawfile;
	dba_file_reader reader;
	dba_file_writer writer;
};

dba_err dba_file_create(dba_file* file, dba_encoding type, const char* name, const char* mode)
{
	dba_err err;
	dba_file res = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (res == NULL)
		return dba_error_alloc("Allocating a new file reader");
	res->type = type;

	DBA_RUN_OR_GOTO(fail, dba_rawfile_create(&(res->rawfile), name, mode));

	if (type == -1)
		/* Peek at the first byte to guess the encoding */
		DBA_RUN_OR_GOTO(fail, dba_rawfile_guess_encoding(res->rawfile, &type));

	switch (type)
	{
		case BUFR:
			DBA_RUN_OR_GOTO(fail, dba_file_reader_create_bufr(&(res->reader), res->rawfile));
			DBA_RUN_OR_GOTO(fail, dba_file_writer_create_bufr(&(res->writer), res->rawfile));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(fail, dba_file_reader_create_crex(&(res->reader), res->rawfile));
			DBA_RUN_OR_GOTO(fail, dba_file_writer_create_crex(&(res->writer), res->rawfile));
			break;
		case AOF:
			DBA_RUN_OR_GOTO(fail, dba_file_reader_create_aof(&(res->reader), res->rawfile));
			DBA_RUN_OR_GOTO(fail, dba_file_writer_create_aof(&(res->writer), res->rawfile));
			break;
	}

	*file = (dba_file)res;
	return dba_error_ok();

fail:
	if (res->rawfile != NULL)
		dba_rawfile_delete(res->rawfile);
	if (res->reader != NULL)
		dba_file_reader_delete(res->reader);
	free(res);
	*file = NULL;
	return err;
}

void dba_file_delete(dba_file file)
{
	if (file->rawfile != NULL)
		dba_rawfile_delete(file->rawfile);
	if (file->reader != NULL)
		dba_file_reader_delete(file->reader);
	free(file);
}

dba_encoding dba_file_get_type(dba_file file)
{
	return file->type;
}

dba_err dba_file_read_raw(dba_file file, dba_rawmsg msg, int* found)
{
	return dba_file_reader_read(file->reader, msg, found);
}

dba_err dba_file_read(dba_file file, dba_msgs* msgs, int* found)
{
	dba_err err = DBA_OK;
	dba_rawmsg rm = NULL;
	
	DBA_RUN_OR_RETURN(dba_rawmsg_create(&rm));
	DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, rm, found));
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

dba_err dba_file_write_raw(dba_file file, dba_rawmsg msg)
{
	return dba_file_writer_write_raw(file->writer, msg);
}

dba_err dba_file_write(dba_file file, dba_msgs msgs, int cat, int subcat)
{
	dba_err err = DBA_OK;
	dba_rawmsg raw = NULL;

	switch (file->type)
	{
		case BUFR:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_bufr(msgs, cat, subcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write_raw(file, raw));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_crex(msgs, cat, subcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write_raw(file, raw));
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
