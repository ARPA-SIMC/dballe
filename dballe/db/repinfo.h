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

#ifndef DBALLE_DB_REPINFO_H
#define DBALLE_DB_REPINFO_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module, but not
 * exported as official API.
 */

#include <dballe/db/internals.h>

struct _dba_db;

struct _dba_db_repinfo_cache {
	int id;

	char* memo;
	char* desc;
	int	  prio;
	char* descriptor;
	int   tablea;

	char* new_memo;
	char* new_desc;
	int	  new_prio;
	char* new_descriptor;
	int	  new_tablea;
};
typedef struct _dba_db_repinfo_cache* dba_db_repinfo_cache;

struct _dba_db_repinfo_memoidx {
	char memo[30];
	int id;
};
typedef struct _dba_db_repinfo_memoidx* dba_db_repinfo_memoidx;

/**
 * Precompiled query to insert a value in repinfo
 */
struct _dba_db_repinfo
{
	struct _dba_db* db;

	dba_db_repinfo_cache cache;
	int cache_size;
	int cache_alloc_size;

	dba_db_repinfo_memoidx memo_idx;
};
typedef struct _dba_db_repinfo* dba_db_repinfo;

/**
 * Create a new dba_db_repinfo
 *
 * @param db
 *   The database accessed by this dba_db_repinfo.
 * @retval ins
 *   The resulting dba_db_repinfo structure.
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_repinfo_create(dba_db db, dba_db_repinfo* ins);

/**
 * Delete a dba_db_repinfo
 *
 * @param ins
 *   The dba_db_repinfo to delete.
 */
void dba_db_repinfo_delete(dba_db_repinfo ins);

/**
 * Get the id of a repinfo entry given its name
 *
 * @param ri
 *   dba_db_repinfo used for the query
 * @param memo
 *   The name to query
 * @retval id
 *   The resulting id.  It will always be a valid one, because the functions
 *   fails if memo is not found.
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_repinfo_get_id(dba_db_repinfo ri, const char* memo, int* id);

/**
 * Check if the database contains the given rep_cod id
 *
 * @param ri
 *   dba_db_repinfo used for the query
 * @param id
 *   id to check
 * @retval exists
 *   Set to true if id exists, else false.
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_repinfo_has_id(dba_db_repinfo ri, int id, int* exists);

/**
 * Get a repinfo cache entry by id.
 *
 * @param ri
 *   dba_db_repinfo used for the query
 * @param id
 *   id to query
 * @return
 *   The dba_db_repinfo_cache structure found, or NULL if none was found.
 */
dba_db_repinfo_cache dba_db_repinfo_get_by_id(dba_db_repinfo ri, int id);

/**
 * Get a repinfo cache entry by name.
 *
 * @param ri
 *   dba_db_repinfo used for the query
 * @param memo
 *   name to query
 * @return
 *   The dba_db_repinfo_cache structure found, or NULL if none was found.
 */
dba_db_repinfo_cache dba_db_repinfo_get_by_memo(dba_db_repinfo ri, const char* memo);

/**
 * Update the report type information in the database using the data from the
 * given file.
 *
 * @param ri
 *   dba_db_repinfo used to update the database
 * @param deffile
 *   Pathname of the file to use for the update.  The NULL value is accepted
 *   and means to use the default configure repinfo.csv file.
 * @retval added
 *   Number of entries that have been added during the update.
 * @retval deleted
 *   Number of entries that have been deleted during the update.
 * @retval updated
 *   Number of entries that have been updated during the update.
 */
dba_err dba_db_repinfo_update(dba_db_repinfo ri, const char* deffile, int* added, int* deleted, int* updated);

#if 0
void dba_db_repinfo_set_ident(dba_db_repinfo ins, const char* ident);
dba_err dba_db_repinfo_get_id(dba_db_repinfo ins, int *id);
dba_err dba_db_repinfo_insert(dba_db_repinfo ins, int *id);
dba_err dba_db_repinfo_update(dba_db_repinfo ins);
#endif


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
