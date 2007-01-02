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

#ifndef DBALLE_DB_ATTR_H
#define DBALLE_DB_ATTR_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/db/internals.h>
#include <dballe/core/var.h>

struct _dba_db;
	
/**
 * Precompiled queries to manipulate the attr table
 */
struct _dba_db_attr
{
	/** dba_db this dba_db_attr is part of */
	struct _dba_db* db;
	/** Precompiled select statement */
	SQLHSTMT sstm;
	/** Precompiled insert statement */
	SQLHSTMT istm;
	/** Precompiled replace statement */
	SQLHSTMT rstm;

	/** context id SQL parameter */
	int id_context;
	/** variable id SQL parameter */
	dba_varcode id_var;
	/** attribute id SQL parameter */
	dba_varcode type;
	/** attribute value SQL parameter */
	char value[255];
	/** attribute value indicator */
	SQLLEN value_ind;
};
/** @copydoc _dba_db_attr */
typedef struct _dba_db_attr* dba_db_attr;

/**
 * Create a new dba_db_attr
 * 
 * @param db
 *   The ::dba_db this ::dba_db_attr will access
 * @retval ins
 *   The newly created ::dba_db_attr (it will need to be deallocated wth dba_db_attr_delete())
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_attr_create(dba_db db, dba_db_attr* ins);

/**
 * Deletes a dba_db_attr
 *
 * @param ins
 *   The ::dba_db_attr to delete
 */
void dba_db_attr_delete(dba_db_attr ins);

/**
 * Set the input fields of a ::dba_db_attr using the values in a ::dba_var
 *
 * @param ins
 *   The ::dba_db_attr to fill in
 * @param var
 *   The ::dba_var with the data to copy into ins
 */
void dba_db_attr_set(dba_db_attr ins, dba_var var);

/**
 * Set the value input field of a ::dba_db_attr from a string
 *
 * @param ins
 *   The ::dba_db_attr to fill in
 * @param value
 *   The value to copy into ins
 */
void dba_db_attr_set_value(dba_db_attr ins, const char* value);

/**
 * Insert an entry into the attr table
 *
 * @param ins
 *   The ::dba_db_attr with the fields filled in with the data to insert.
 * @param replace
 *   If set to true, an existing attribute with the same context and
 *   ::dba_varcode will be overwritten; else, trying to replace an existing
 *   attribute will result in an error.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_attr_insert(dba_db_attr ins, int replace);

/**
 * Load from the database all the attributes for var
 *
 * @param ins
 *   ::dba_db_attr to use for the query, with the context ID filled in
 * @param var
 *   ::dba_var to which the resulting attributes will be added
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_attr_load(dba_db_attr ins, dba_var var);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
