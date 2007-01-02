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

#ifndef DBALLE_DB_DATA_H
#define DBALLE_DB_DATA_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/internals.h>
#include <dballe/core/var.h>

struct _dba_db;
	
/**
 * Precompiled query to manipulate the data table
 */
struct _dba_db_data
{
	/** dba_db this dba_db_data is part of */
	struct _dba_db* db;
	/** Precompiled insert statement */
	SQLHSTMT istm;
	/** Precompiled update statement */
	SQLHSTMT ustm;

	/** Context ID SQL parameter */
	int id_context;
	/** Variable type SQL parameter */
	dba_varcode id_var;
	/** Variable value SQL parameter */
	char value[255];
	/** Variable value indicator */
	SQLLEN value_ind;
};
/** @copydoc _dba_db_data */
typedef struct _dba_db_data* dba_db_data;

/**
 * Create a new dba_db_data
 * 
 * @param db
 *   The ::dba_db this ::dba_db_data will access
 * @retval ins
 *   The newly created ::dba_db_data (it will need to be deallocated wth dba_db_data_delete())
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_data_create(dba_db db, dba_db_data* ins);

/**
 * Deletes a dba_db_data
 *
 * @param ins
 *   The ::dba_db_data to delete
 */
void dba_db_data_delete(dba_db_data ins);

/**
 * Set the input fields of a ::dba_db_data using the values in a ::dba_var
 *
 * @param ins
 *   The ::dba_db_data to fill in
 * @param var
 *   The ::dba_var with the data to copy into ins
 */
void dba_db_data_set(dba_db_data ins, dba_var var);

/**
 * Set the value input field of a ::dba_db_data from a string
 *
 * @param ins
 *   The ::dba_db_data to fill in
 * @param value
 *   The value to copy into ins
 */
void dba_db_data_set_value(dba_db_data ins, const char* value);

/**
 * Insert an entry into the data table
 *
 * @param ins
 *   The ::dba_db_data with the fields filled in with the data to insert.
 * @param rewrite
 *   If set to true, an existing data with the same context and ::dba_varcode
 *   will be overwritten; else, trying to replace an existing value will result
 *   in an error.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_data_insert(dba_db_data ins, int rewrite);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
