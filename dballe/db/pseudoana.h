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
 * Pseudoana table management used by the db module.
 */

#include <dballe/db/internals.h>

struct _dba_db;
	
/**
 * Precompiled query to manipulate the pseudoana table
 */
struct _dba_db_pseudoana
{
	/** dba_db this dba_db_pseudoana is part of */
	struct _dba_db* db;
	/** Precompiled select fixed station query */
	SQLHSTMT sfstm;
	/** Precompiled select mobile station query */
	SQLHSTMT smstm;
	/** Precompiled select data by station id query */
	SQLHSTMT sstm;
	/** Precompiled insert query */
	SQLHSTMT istm;
	/** Precompiled update query */
	SQLHSTMT ustm;
	/** Precompiled delete query */
	SQLHSTMT dstm;

	/** Station ID SQL parameter */
	int id;
	/** Station latitude SQL parameter */
	int lat;
	/** Station longitude SQL parameter */
	int lon;
	/** Mobile station identifier SQL parameter */
	char ident[64];
	/** Mobile station identifier indicator */
	SQLLEN ident_ind;
};
/** @copydoc _dba_db_pseudoana */
typedef struct _dba_db_pseudoana* dba_db_pseudoana;

/**
 * Create a new dba_db_pseudoana
 * 
 * @param db
 *   The ::dba_db this ::dba_db_pseudoana will access
 * @retval ins
 *   The newly created ::dba_db_pseudoana (it will need to be deallocated wth dba_db_pseudoana_delete())
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_pseudoana_create(dba_db db, dba_db_pseudoana* ins);

/**
 * Deletes a dba_db_pseudoana
 *
 * @param ins
 *   The ::dba_db_pseudoana to delete
 */
void dba_db_pseudoana_delete(dba_db_pseudoana ins);

/**
 * Set the mobile station identifier input value for this ::dba_db_pseudoana
 *
 * @param ins
 *   ::dba_db_pseudoana structure to fill in
 * @param ident
 *   Value to copy into ins
 */
void dba_db_pseudoana_set_ident(dba_db_pseudoana ins, const char* ident);

/**
 * Get the pseudoana ID given latitude, longitude and mobile identifier
 *
 * @param ins
 *   ::dba_db_pseudoana to query
 * @retval id
 *   Resulting ID of the station
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_pseudoana_get_id(dba_db_pseudoana ins, int *id);

/**
 * Get pseudoana information given a pseudoana ID
 *
 * @param ins
 *   ::dba_db_pseudoana to query
 * @param id
 *   ID of the station to query
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_pseudoana_get_data(dba_db_pseudoana ins, int id);

/**
 * Insert a new pseudoana entry
 *
 * @param ins
 *   ::dba_db_pseudoana with all the input values filled in
 * @retval id
 *   ID of the newly inserted station
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_pseudoana_insert(dba_db_pseudoana ins, int *id);

/**
 * Update the information about a pseudoana entry
 *
 * @param ins
 *   ::dba_db_pseudoana with all the input values filled in
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_pseudoana_update(dba_db_pseudoana ins);

/**
 * Remove a pseudoana record
 *
 * @param ins
 *   The dba_db_pseudoana structure, with id filled with the id of the pseudoana to
 *   remove.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_pseudoana_remove(dba_db_pseudoana ins);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
