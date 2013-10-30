/*
 * db/v5/repinfo - repinfo table management
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_V5_REPINFO_H
#define DBALLE_DB_V5_REPINFO_H

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
struct Connection;

namespace v5 {

namespace repinfo {

/** repinfo cache entry */
struct Cache
{
	/** Report code */
	DBALLE_SQL_C_UINT_TYPE id;

	/** Report name */
	std::string memo;
	/** Report description */
	std::string desc;
	/** Report priority */
	DBALLE_SQL_C_SINT_TYPE prio;
	/** Report descriptor (currently unused) */
	std::string descriptor;
	/** Report A table value (currently unused) */
	DBALLE_SQL_C_UINT_TYPE tablea;

	/** New report name used when updating the repinfo table */
	std::string new_memo;
	/** New report description used when updating the repinfo table */
	std::string new_desc;
	/** New report priority used when updating the repinfo table */
	DBALLE_SQL_C_SINT_TYPE new_prio;
	/** New report descriptor used when updating the repinfo table */
	std::string new_descriptor;
	/** New report A table value used when updating the repinfo table */
	DBALLE_SQL_C_UINT_TYPE new_tablea;

	Cache(int id, const std::string& memo, const std::string& desc, int prio, const std::string& descriptor, int tablea);
	void make_new();
};

/** reverse rep_memo -> rep_cod cache entry */
struct Memoidx
{
	/** Report name */
	std::string memo;
	/** Report code */
	int id;

	bool operator<(const Memoidx& memo) const;
};

}

/**
 * Fast cached access to the repinfo table
 */
struct Repinfo
{
	/** Cache of table entries */
	std::vector<repinfo::Cache> cache;

	/** rep_memo -> rep_cod reverse index */
	mutable std::vector<repinfo::Memoidx> memo_idx;


	/**
	 * DB connection. The pointer is assumed always valid during the
	 * lifetime of the object
	 */
	Connection* conn;

    Repinfo(Connection* conn);
    virtual ~Repinfo();

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

	/**
	 * Get the id of a repinfo entry given its name
	 *
	 * @param memo
	 *   The name to query
	 * @return
	 *   The resulting id.  It will always be a valid one, because the functions
	 *   fails if memo is not found.
	 */
	int get_id(const char* memo) const;

    /**
     * Get the id of a repinfo entry given its name.
     *
     * It creates a new entry if the memo is missing from the database.
     *
     * @param memo
     *   The name to query
     * @return
     *   The resulting id.
     */
    int obtain_id(const char* memo);

	/**
	 * Check if the database contains the given rep_cod id
	 *
	 * @param id
	 *   id to check
	 * @return
	 *   true if id exists, else false.
	 */
	bool has_id(unsigned id) const;

	/**
	 * Get a repinfo cache entry by id.
	 *
	 * @param id
	 *   id to query
	 * @return
	 *   The Cache structure found, or NULL if none was found.
	 */
	const repinfo::Cache* get_by_id(unsigned id) const;

	/**
	 * Get a repinfo cache entry by name.
	 *
	 * @param memo
	 *   name to query
	 * @return
	 *   The Cache structure found, or NULL if none was found.
	 */
	const repinfo::Cache* get_by_memo(const char* memo) const;

    /**
     * Return a vector of IDs matching the priority constraints in the given record.
     */
    std::vector<int> ids_by_prio(const Record& rec) const;

    /**
     * Get a mapping between rep_memo and their priorities
     */
    std::map<std::string, int> get_priorities() const;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out);

protected:
    /// Return how many time this ID is used in the database
    virtual int id_use_count(unsigned id, const char* name);

	void read_cache();
	void cache_append(unsigned id, const char* memo, const char* desc, int prio, const char* descriptor, int tablea);
	void rebuild_memo_idx() const;
	int cache_find_by_memo(const char* memo) const;
	int cache_find_by_id(unsigned id) const;
	std::vector<repinfo::Cache> read_repinfo_file(const char* deffile);
    /// Create an automatic entry for a missing memo, and insert it in the database
    void insert_auto_entry(const char* memo);

private:
	// disallow copy
	Repinfo(const Repinfo&);
	Repinfo& operator=(const Repinfo&);
};

} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
