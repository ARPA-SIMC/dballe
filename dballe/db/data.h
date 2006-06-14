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
 * Pseudoana table management used by the db module, but not
 * exported as official API.
 */

#include <dballe/db/internals.h>

struct _dba_db;
	
/**
 * Precompiled query to insert a value in data
 */
struct _dba_db_data
{
	struct _dba_db* db;
	SQLHSTMT istm;
	SQLHSTMT ustm;

	int id_context;
	dba_varcode id_var;
	char value[255];
	SQLINTEGER value_ind;
};
typedef struct _dba_db_data* dba_db_data;


dba_err dba_db_data_create(dba_db db, dba_db_data* ins);
void dba_db_data_delete(dba_db_data ins);
void dba_db_data_set(dba_db_data ins, dba_var var);
void dba_db_data_set_value(dba_db_data ins, const char* value);
dba_err dba_db_data_insert(dba_db_data ins, int rewrite);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
