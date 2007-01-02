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

#ifndef DBA_CORE_FILE_INTERNALS_H
#define DBA_CORE_FILE_INTERNALS_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * dba_file internals, useful to implement dba_file extensions
 */

#include <dballe/core/file.h>
#include <stdio.h>

typedef dba_err (*dba_file_create_fun)(dba_encoding type, FILE* fd, const char* mode, dba_file* file);
typedef void (*dba_file_delete_fun)(dba_file);
typedef dba_err (*dba_file_read_fun)(dba_file file, dba_rawmsg msg, int* found);
typedef dba_err (*dba_file_write_fun)(dba_file file, dba_rawmsg msg);

struct _dba_file
{
	char* name;
	FILE* fd;
	int close_on_exit;
	int idx;
	dba_encoding type;

	/** Function to use to delete this dba_file */
	dba_file_delete_fun fun_delete;
	/** Function to use to read a message */
	dba_file_read_fun fun_read;
	/** Function to use to write an encoded message */
	dba_file_write_fun fun_write;
};

extern dba_file_create_fun dba_file_aof_create;
extern dba_file_create_fun dba_file_bufr_create;
extern dba_file_create_fun dba_file_crex_create;

/**
 * Simple default file write implementation
 */
dba_err dba_file_default_write_impl(dba_file file, dba_rawmsg msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
