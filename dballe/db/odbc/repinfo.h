/*
 * db/v5/repinfo - repinfo table management
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_ODBC_V5_REPINFO_H
#define DBALLE_DB_ODBC_V5_REPINFO_H

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

#include <dballe/db/v5/repinfo.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
struct ODBCConnection;

namespace v5 {

namespace repinfo {

/** repinfo cache entry */
struct Cache
{
    /// Report code
    unsigned id;

    /** Report name */
    std::string memo;
    /** Report description */
    std::string desc;
    /// Report priority
    int prio;
    /** Report descriptor (currently unused) */
    std::string descriptor;
    /// Report A table value (currently unused)
    unsigned tablea;

    /** New report name used when updating the repinfo table */
    std::string new_memo;
    /** New report description used when updating the repinfo table */
    std::string new_desc;
    /// New report priority used when updating the repinfo table
    int new_prio;
    /** New report descriptor used when updating the repinfo table */
    std::string new_descriptor;
    /// New report A table value used when updating the repinfo table
    unsigned new_tablea;

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
struct ODBCRepinfo : public Repinfo
{
    /** Cache of table entries */
    std::vector<repinfo::Cache> cache;

    /** rep_memo -> rep_cod reverse index */
    mutable std::vector<repinfo::Memoidx> memo_idx;


    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    ODBCConnection& conn;

    ODBCRepinfo(ODBCConnection& conn);
    ODBCRepinfo(const ODBCRepinfo&) = delete;
    ODBCRepinfo(const ODBCRepinfo&&) = delete;
    virtual ~ODBCRepinfo();
    ODBCRepinfo& operator=(const ODBCRepinfo&) = delete;

    void to_record(int id, Record& rec) override;
    const char* get_rep_memo(int id) override;
    int get_id(const char* rep_memo) override;
    int get_priority(int id) override;
    void update(const char* deffile, int* added, int* deleted, int* updated) override;
    std::map<std::string, int> get_priorities() override;
    int obtain_id(const char* memo) override;
    void dump(FILE* out) override;
    std::vector<int> ids_by_prio(const Record& rec) override;

    /**
     * Invalidate the repinfo cache.  To be called if the repinfo table is modified
     * externally; for example, when the table is recreated on database reset.
     */
    void invalidate_cache();

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
};

}

namespace v6 {

struct ODBCRepinfo : public v5::ODBCRepinfo
{
    ODBCRepinfo(ODBCConnection& conn);

protected:
    /// Return how many time this ID is used in the database
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
#endif

