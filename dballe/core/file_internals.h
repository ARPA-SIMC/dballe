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

/** Type of function that is used to create a kind of ::dba_file instance */
typedef dba_err (*dba_file_create_fun)(dba_encoding type, FILE* fd, const char* mode, dba_file* file);
/** Type of function that is used to delete a kind of ::dba_file instance */
typedef void (*dba_file_delete_fun)(dba_file);
/** Type of function that is used to read from a kind of ::dba_file instance */
typedef dba_err (*dba_file_read_fun)(dba_file file, dba_rawmsg msg, int* found);
/** Type of function that is used to write to a kind of ::dba_file instance */
typedef dba_err (*dba_file_write_fun)(dba_file file, dba_rawmsg msg);

/**
 * Base contents of a ::dba_file.
 * 
 * Specialised ::dba_file implementations can just use a struct _dba_file or a
 * larger structure with a struct _dba_file as the first element.
 *
 * On creation of a ::dba_file instance, the fun_* methods need to be set with
 * the functions used to delete, read and write.
 */
struct _dba_file
{
	/** Name of the file */
	char* name;
	/** FILE structure used to read or write to the file */
	FILE* fd;
	/** Set to true if fd should be closed when dba_file_delete() is called */
	int close_on_exit;
	/** Index of the last message read from the file */
	int idx;
	/** File encoding */
	dba_encoding type;

	/** Function to use to delete this dba_file */
	dba_file_delete_fun fun_delete;
	/** Function to use to read a message */
	dba_file_read_fun fun_read;
	/** Function to use to write an encoded message */
	dba_file_write_fun fun_write;
};

/**
 * Function used to create a ::dba_file for AOF.
 *
 * This is originally initialised to NULL.  The AOF implementation needs to set
 * this to the function used to create a ::dba_file for AOF: they can do so
 * using a library constructor, i.e. a function defined with
 * __attribute__((constructor)).
 */
extern dba_file_create_fun dba_file_aof_create;

/**
 * Function used to create a ::dba_file for BUFR.
 *
 * This is originally initialised to NULL.  The BUFR implementation needs to set
 * this to the function used to create a ::dba_file for BUFR: they can do so
 * using a library constructor, i.e. a function defined with
 * __attribute__((constructor)).
 */
extern dba_file_create_fun dba_file_bufr_create;

/**
 * Function used to create a ::dba_file for CREX.
 *
 * This is originally initialised to NULL.  The CREX implementation needs to set
 * this to the function used to create a ::dba_file for CREX: they can do so
 * using a library constructor, i.e. a function defined with
 * __attribute__((constructor)).
 */
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
