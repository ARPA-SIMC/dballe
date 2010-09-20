/*
 * db/repinfo - repinfo table management
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

// #include <dballe/db/internals.h>

namespace dballe {
namespace db {

struct Repinfo
{

	/**
	 * Invalidate the repinfo cache.  To be called if the repinfo table is modified
	 * externally; for example, when the table is recreated on database reset.
	 */
	void invalidate_cache();

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
	void update(const char* deffile, int* added, int* deleted, int* updated);
};

#if 0
struct _dba_db;

/** repinfo cache entry */
struct _dba_db_repinfo_cache {
	/** Report code */
	DBALLE_SQL_C_UINT_TYPE id;

	/** Report name */
	char* memo;
	/** Report description */
	char* desc;
	/** Report priority */
	DBALLE_SQL_C_UINT_TYPE prio;
	/** Report descriptor (currently unused) */
	char* descriptor;
	/** Report A table value (currently unused) */
	DBALLE_SQL_C_UINT_TYPE tablea;

	/** New report name used when updating the repinfo table */
	char* new_memo;
	/** New report description used when updating the repinfo table */
	char* new_desc;
	/** New report priority used when updating the repinfo table */
	DBALLE_SQL_C_UINT_TYPE new_prio;
	/** New report descriptor used when updating the repinfo table */
	char* new_descriptor;
	/** New report A table value used when updating the repinfo table */
	DBALLE_SQL_C_UINT_TYPE new_tablea;
};
/** @copydoc _dba_db_repinfo_cache */
typedef struct _dba_db_repinfo_cache* dba_db_repinfo_cache;

/** reverse rep_memo -> rep_cod cache entry */
struct _dba_db_repinfo_memoidx {
	/** Report name */
	char memo[20];
	/** Report code */
	int id;
};
/** @copydoc _dba_db_repinfo_memoidx */
typedef struct _dba_db_repinfo_memoidx* dba_db_repinfo_memoidx;

/**
 * Fast cached access to the repinfo table
 */
struct _dba_db_repinfo
{
	/** dba_db this dba_db_repinfo is part of */
	struct _dba_db* db;

	/** Cache of dba_db_repinfo entries */
	dba_db_repinfo_cache cache;
	/** Number of items present in the cache */
	int cache_size;
	/** Cache allocation size as number of items */
	int cache_alloc_size;

	/** rep_memo -> rep_cod reverse index */
	dba_db_repinfo_memoidx memo_idx;
};
/** @copydoc _dba_db_repinfo */
typedef struct _dba_db_repinfo* dba_db_repinfo;

/**
 * Create a new dba_db_repinfo
 *
 * @param db
 *   The database accessed by this dba_db_repinfo.
 * @retval ins
 *   The resulting dba_db_repinfo structure.
 * @return
 *   The error indicator for the function (See @ref error.h)
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
 *   The error indicator for the function (See @ref error.h)
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
 *   The error indicator for the function (See @ref error.h)
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

#if 0
void dba_db_repinfo_set_ident(dba_db_repinfo ins, const char* ident);
dba_err dba_db_repinfo_get_id(dba_db_repinfo ins, int *id);
dba_err dba_db_repinfo_insert(dba_db_repinfo ins, int *id);
dba_err dba_db_repinfo_update(dba_db_repinfo ins);
#endif

#endif

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
