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

#include "config.h"

#include "dba_rawfile.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

dba_err dba_rawfile_create(dba_rawfile* file, const char* name, const char* mode)
{
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

	if (strcmp(name, "(stdin)") == 0)
	{
		(*file)->fd = stdin;
		(*file)->close_on_exit = 0;
	}
	else if (strcmp(name, "(stdout)") == 0)
	{
		(*file)->fd = stdout;
		(*file)->close_on_exit = 0;
	}
	else if (strcmp(name, "(stderr)") == 0)
	{
		(*file)->fd = stderr;
		(*file)->close_on_exit = 0;
	}
	else
	{
		(*file)->fd = fopen(name, mode);
		if ((*file)->fd == NULL)
		{
			free((*file)->name);
			free(*file);
			*file = NULL;
			return dba_error_system("opening %s with mode '%s'", name, mode);
		}
		(*file)->close_on_exit = 1;
	}

	return dba_error_ok();
}

void dba_rawfile_delete(dba_rawfile file)
{
	if (file->name)
		free(file->name);
	if (file->fd && file->close_on_exit)
		fclose(file->fd);
	free(file);
}

dba_err dba_rawfile_guess_encoding(dba_rawfile file, dba_encoding* enc)
{
	int c = getc(file->fd);
	if (c == EOF)
		return dba_error_system("reading the first byte of %s to detect its encoding", file->name);
	if (ungetc(c, file->fd) == EOF)
		return dba_error_system("putting the first byte of %s back into the input stream", file->name);
	
	switch (c)
	{
		case 'B': *enc = BUFR; break;
		case 'C': *enc = CREX; break;
		case 0: *enc = AOF; break;
		case 38: *enc = AOF; break;
		default: *enc = BUFR; break;
	}

	return dba_error_ok();
}


dba_err dba_rawfile_write(dba_rawfile file, dba_rawmsg msg)
{
	const unsigned char* buf;
	int size;
	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(msg, &buf, &size));
	if (fwrite(buf, size, 1, file->fd) != 1)
		return dba_error_system("writing message data on output");
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
