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

#include "writers.h"

#include <netinet/in.h>
#include <byteswap.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <assert.h>

extern dba_err aof_reader_read_record(dba_rawfile file, uint32_t** rec, int* len);


dba_err dba_file_writer_write_raw(dba_file_writer writer, dba_rawmsg msg)
{
	return writer->write_raw_fun(writer, msg);
}

dba_err dba_file_writer_write(dba_file_writer writer, dba_msg msg)
{
	return writer->write_fun(writer, msg);
}

void dba_file_writer_delete(dba_file_writer writer)
{
	writer->delete_fun(writer);
}


/* ** ** **  Writer for BUFR  ** ** ** */

struct _bufr_writer
{
	struct _dba_file_writer parent;
	int out_type;
	int out_subtype;
};
typedef struct _bufr_writer* bufr_writer;

static void bufr_writer_delete(bufr_writer writer);
static dba_err bufr_writer_write(bufr_writer writer, dba_msg msg);
static dba_err bufr_writer_write_raw(bufr_writer writer, dba_rawmsg msg);

dba_err dba_file_writer_create_bufr(dba_file_writer* writer, dba_rawfile file)
{
	bufr_writer res = (bufr_writer)calloc(1, sizeof(struct _bufr_writer));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR writer");
	res->parent.delete_fun = (dba_file_writer_delete_fun)bufr_writer_delete;
	res->parent.write_fun = (dba_file_writer_write_fun)bufr_writer_write;
	res->parent.write_raw_fun = (dba_file_writer_write_raw_fun)bufr_writer_write_raw;
	res->parent.file = file;

	*writer = (dba_file_writer)res;
	return dba_error_ok();
}

static void bufr_writer_delete(bufr_writer writer)
{
	free(writer);
}

dba_err dba_file_writer_set_bufr_template(dba_file_writer writer, int type, int subtype)
{
	bufr_writer w = (bufr_writer)writer;
	w->out_type = type;
	w->out_subtype = subtype;
	return dba_error_ok();
}

static dba_err bufr_writer_write_raw(bufr_writer writer, dba_rawmsg msg)
{
	return dba_rawfile_write(writer->parent.file, msg);
}

static dba_err bufr_writer_write(bufr_writer writer, dba_msg msg)
{
	dba_err err = DBA_OK;
#if 0
	bufr_message cmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, bufr_from_msg(msg, &cmsg, writer->out_type, writer->out_subtype));
	DBA_RUN_OR_GOTO(cleanup, bufr_message_encode(cmsg));
	DBA_RUN_OR_GOTO(cleanup, bufr_writer_write_raw(writer, cmsg));

cleanup:
	if (cmsg != NULL)
		bufr_message_delete(cmsg);
#endif
	return err == DBA_OK ? dba_error_ok() : err;
}




/* ** ** **  Writer for CREX  ** ** ** */

struct _crex_writer
{
	struct _dba_file_writer parent;
	int out_type;
	int out_subtype;
};
typedef struct _crex_writer* crex_writer;

static void crex_writer_delete(crex_writer writer);
static dba_err crex_writer_write(crex_writer writer, dba_msg msg);
static dba_err crex_writer_write_raw(crex_writer writer, dba_rawmsg msg);

dba_err dba_file_writer_create_crex(dba_file_writer* writer, dba_rawfile file)
{
	crex_writer res = (crex_writer)calloc(1, sizeof(struct _crex_writer));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR writer");
	res->parent.delete_fun = (dba_file_writer_delete_fun)crex_writer_delete;
	res->parent.write_fun = (dba_file_writer_write_fun)crex_writer_write;
	res->parent.write_raw_fun = (dba_file_writer_write_raw_fun)crex_writer_write_raw;
	res->parent.file = file;

	*writer = (dba_file_writer)res;
	return dba_error_ok();
}

static void crex_writer_delete(crex_writer writer)
{
	free(writer);
}

dba_err dba_file_writer_set_crex_template(dba_file_writer writer, int type, int subtype)
{
	crex_writer w = (crex_writer)writer;
	w->out_type = type;
	w->out_subtype = subtype;
	return dba_error_ok();
}


static dba_err crex_writer_write(crex_writer writer, dba_msg msg)
{
	dba_err err = DBA_OK;
#if 0
	crex_message cmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, crex_from_msg(msg, &cmsg, writer->out_type, writer->out_subtype));
	DBA_RUN_OR_GOTO(cleanup, crex_message_encode(cmsg));
	DBA_RUN_OR_GOTO(cleanup, crex_writer_write_raw(writer, cmsg));

cleanup:
	if (cmsg != NULL)
		crex_message_delete(cmsg);
#endif
	return err == DBA_OK ? dba_error_ok() : err;
}






/* ** ** **  Writer for AOF  ** ** ** */

struct _aof_writer
{
	struct _dba_file_writer parent;

	/* First data record */
	uint32_t fdr[14];
	/* Data description record */
	uint32_t ddr[17];

	/* Start time of the observation */
	struct tm start;

	/* End time of the observation */
	struct tm end;

	/* 0 if we should write with the host endianness; 1 if we should write
	 * little endian; 2 if we should write big endian */
	enum { END_ARCH, END_LE, END_BE } endianness;
};
typedef struct _aof_writer* aof_writer;

static void aof_writer_delete(aof_writer writer);
static dba_err aof_writer_write(aof_writer writer, dba_msg msg);
static dba_err aof_writer_write_raw(aof_writer writer, dba_rawmsg msg);

dba_err dba_file_writer_create_aof(dba_file_writer* writer, dba_rawfile file)
{
	char* env_swap = getenv("DBA_AOF_ENDIANNESS");
	aof_writer res = (aof_writer)calloc(1, sizeof(struct _aof_writer));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR writer");
	res->parent.delete_fun = (dba_file_writer_delete_fun)aof_writer_delete;
	res->parent.write_fun = (dba_file_writer_write_fun)aof_writer_write;
	res->parent.write_raw_fun = (dba_file_writer_write_raw_fun)aof_writer_write_raw;
	res->parent.file = file;

	if (env_swap == NULL)
		res->endianness = END_ARCH;
	else if (strcmp(env_swap, "ARCH") == 0)
		res->endianness = END_ARCH;
	else if (strcmp(env_swap, "LE") == 0)
		res->endianness = END_LE;
	else if (strcmp(env_swap, "BE") == 0)
		res->endianness = END_BE;
	else
		res->endianness = END_ARCH;

	*writer = (dba_file_writer)res;
	return dba_error_ok();
}

static void aof_writer_delete(aof_writer writer)
{
	free(writer);
}




/* vim:set ts=4 sw=4: */
