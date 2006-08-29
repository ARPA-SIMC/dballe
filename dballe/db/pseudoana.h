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

#ifndef DBALLE_DB_PSEUDOANA_H
#define DBALLE_DB_PSEUDOANA_H

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
 * Precompiled query to insert a value in pseudoana
 */
struct _dba_db_pseudoana
{
	struct _dba_db* db;
	SQLHSTMT sfstm;
	SQLHSTMT smstm;
	SQLHSTMT istm;
	SQLHSTMT ustm;
	SQLHSTMT dstm;

	int id;
	int lat;
	int lon;
	char ident[64];
	SQLINTEGER ident_ind;
};
typedef struct _dba_db_pseudoana* dba_db_pseudoana;


dba_err dba_db_pseudoana_create(dba_db db, dba_db_pseudoana* ins);
void dba_db_pseudoana_delete(dba_db_pseudoana ins);
void dba_db_pseudoana_set_ident(dba_db_pseudoana ins, const char* ident);
dba_err dba_db_pseudoana_get_id(dba_db_pseudoana ins, int *id);
dba_err dba_db_pseudoana_insert(dba_db_pseudoana ins, int *id);
dba_err dba_db_pseudoana_update(dba_db_pseudoana ins);

/**
 * Remove a pseudoana record
 *
 * @param ins
 *   The dba_db_pseudoana structure, with id filled with the id of the pseudoana to
 *   remove.
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_pseudoana_remove(dba_db_pseudoana ins);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
