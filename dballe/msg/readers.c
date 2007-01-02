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

#include "readers.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <assert.h>


dba_err dba_file_reader_read(dba_file_reader reader, dba_rawmsg msg, int* found)
{
	return reader->read_fun(reader, msg, found);
}

void dba_file_reader_delete(dba_file_reader reader)
{
	reader->delete_fun(reader);
}


/* ** ** **  Reader for BUFR  ** ** ** */

struct _bufr_reader
{
	struct _dba_file_reader parent;
};
typedef struct _bufr_reader* bufr_reader;

static void bufr_reader_delete(bufr_reader reader);
static dba_err bufr_reader_read(bufr_reader reader, dba_rawmsg msg, int* found);

dba_err dba_file_reader_create_bufr(dba_file_reader* reader, dba_rawfile file)
{
	bufr_reader res = (bufr_reader)calloc(1, sizeof(struct _bufr_reader));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR reader");
	res->parent.delete_fun = (dba_file_reader_delete_fun)bufr_reader_delete;
	res->parent.read_fun = (dba_file_reader_read_fun)bufr_reader_read;
	res->parent.file = file;

	*reader = (dba_file_reader)res;
	return dba_error_ok();
}

static void bufr_reader_delete(bufr_reader reader)
{
	free(reader);
}





/* ** ** **  Reader for CREX  ** ** ** */

struct _crex_reader
{
	struct _dba_file_reader parent;
};
typedef struct _crex_reader* crex_reader;

static void crex_reader_delete(crex_reader reader);
static dba_err crex_reader_read(crex_reader reader, dba_rawmsg msg, int* found);

dba_err dba_file_reader_create_crex(dba_file_reader* reader, dba_rawfile file)
{
	crex_reader res = (crex_reader)calloc(1, sizeof(struct _crex_reader));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR reader");
	res->parent.delete_fun = (dba_file_reader_delete_fun)crex_reader_delete;
	res->parent.read_fun = (dba_file_reader_read_fun)crex_reader_read;
	res->parent.file = file;

	*reader = (dba_file_reader)res;
	return dba_error_ok();
}

static void crex_reader_delete(crex_reader reader)
{
	free(reader);
}



/* vim:set ts=4 sw=4: */
