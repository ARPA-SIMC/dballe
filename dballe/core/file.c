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
#include "file_internals.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

dba_file_create_fun dba_file_aof_create = NULL;
dba_file_create_fun dba_file_bufr_create = NULL;
dba_file_create_fun dba_file_crex_create = NULL;

dba_err dba_file_create(dba_encoding type, const char* name, const char* mode, dba_file* file)
{
	dba_err err = DBA_OK;
	FILE* in = NULL;
	int close_on_exit;
	dba_file res = NULL;

	/* Open the file */
	if (strcmp(name, "(stdin)") == 0)
	{
		in = stdin;
		close_on_exit = 0;
	}
	else if (strcmp(name, "(stdout)") == 0)
	{
		in = stdout;
		close_on_exit = 0;
	}
	else if (strcmp(name, "(stderr)") == 0)
	{
		in = stderr;
		close_on_exit = 0;
	}
	else
	{
		in = fopen(name, mode);
		if (in == NULL)
		{
			err = dba_error_system("opening %s with mode '%s'", name, mode);
			goto cleanup;
		}
		close_on_exit = 1;
	}

	/* Attempt auto-detect if needed */
	if (type == -1)
	{
		int c = getc(in);
		if (c == EOF)
		{
			err = dba_error_system("reading the first byte of %s to detect its encoding", name);
			goto cleanup;
		}
		if (ungetc(c, in) == EOF)
		{
			err = dba_error_system("putting the first byte of %s back into the input stream", name);
			goto cleanup;
		}
		
		switch (c)
		{
			case 'B': type = BUFR; break;
			case 'C': type = CREX; break;
			case 0: type = AOF; break;
			case 0x38: type = AOF; break;
			default:
				err = dba_error_notfound("could not detect the encoding of %s", name);
				goto cleanup;
		}
	}

	/* Call the appropriate constructor */
	switch (type)
	{
		case BUFR:
			if (dba_file_bufr_create == NULL)
			{
				err = dba_error_unimplemented("BUFR support is not available");
				goto cleanup;
			}
			DBA_RUN_OR_GOTO(cleanup, dba_file_bufr_create(type, in, mode, &res));
			break;
		case CREX:
			if (dba_file_crex_create == NULL)
			{
				err = dba_error_unimplemented("CREX support is not available");
				goto cleanup;
			}
			DBA_RUN_OR_GOTO(cleanup, dba_file_crex_create(type, in, mode, &res));
			break;
		case AOF:
			if (dba_file_aof_create == NULL)
			{
				err = dba_error_unimplemented("AOF support is not available");
				goto cleanup;
			}
			DBA_RUN_OR_GOTO(cleanup, dba_file_aof_create(type, in, mode, &res));
			break;
		default:
			err = dba_error_consistency("Requested opening a file of unknown type %d", (int)type);
			goto cleanup;
	}
	
	res->name = strdup(name);
	res->fd = in;
	res->close_on_exit = close_on_exit;
	res->idx = 0;
	res->type = type;
	in = NULL;

	/* Further initialisation or res that could generate errors goes here */

	*file = res;
	res = NULL;

cleanup:
	if (in != NULL)
		fclose(in);
	if (res != NULL)
		dba_file_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}



#if 0


	*file = (dba_rawfile)calloc(1, sizeof(struct _dba_rawfile));
	if (*file == NULL)
		return dba_error_alloc("allocating new _dba_rawfile");
	(*file)->name = strdup(name);
	if ((*file)->name == NULL)
	{
		free(*file);
		*file = NULL;
		return dba_error_alloc("allocating space for a file name");
	}

	return dba_error_ok();
}









	dba_err err;
	dba_file res = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (res == NULL)
		return dba_error_alloc("Allocating a new file reader");
	res->type = type;

	DBA_RUN_OR_GOTO(fail, dba_rawfile_create(&(res->rawfile), name, mode));

	if (type == -1)
		/* Peek at the first byte to guess the encoding */
		DBA_RUN_OR_GOTO(fail, dba_rawfile_guess_encoding(res->rawfile, &type));


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

#endif

void dba_file_delete(dba_file file)
{
	if (file->name)
	{
		free(file->name);
		file->name = 0;
	}
	if (file->fd && file->close_on_exit)
	{
		fclose(file->fd);
		file->fd = 0;
	}
	file->fun_delete(file);
}

dba_encoding dba_file_type(dba_file file)
{
	return file->type;
}

const char* dba_file_name(dba_file file)
{
	return file->name;
}


dba_err dba_file_read(dba_file file, dba_rawmsg msg, int* found)
{
	if (file->fun_read == NULL)
		return dba_error_unimplemented("reading %s files is not implemented", dba_encoding_name(file->type));
	else
	{
		DBA_RUN_OR_RETURN(file->fun_read(file, msg, found));
		++file->idx;
		return dba_error_ok();
	}
}

dba_err dba_file_write(dba_file file, dba_rawmsg msg)
{
	if (file->fun_write == NULL)
		return dba_error_unimplemented("writing %s files is not implemented", dba_encoding_name(file->type));
	else
	{
		DBA_RUN_OR_RETURN(file->fun_write(file, msg));
		++file->idx;
		return dba_error_ok();
	}
}


/*
 * Default implementations
 */

dba_err dba_file_default_write_impl(dba_file file, dba_rawmsg msg)
{
	if (fwrite(msg->buf, msg->len, 1, file->fd) != 1)
		return dba_error_system("writing message data (%d bytes) on output", msg->len);
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
