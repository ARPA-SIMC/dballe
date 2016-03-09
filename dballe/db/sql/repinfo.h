#ifndef DBALLE_DB_SQL_REPINFO_H
#define DBALLE_DB_SQL_REPINFO_H

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

#include <memory>
#include <map>
#include <string>
#include <vector>

namespace dballe {
struct Record;

namespace core {
struct Query;
}

namespace db {
struct Connection;

namespace sql {

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

    void dump(FILE* out) const;
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

/// Fast cached access to the repinfo table
struct Repinfo
{
    Connection& conn;

    Repinfo(Connection& conn);
    virtual ~Repinfo() {}

    //static std::unique_ptr<Repinfo> create(Connection& conn);

    /**
     * Fill repinfo information in a Record based on the repinfo entry with the
     * given ID
     */
    void to_record(int id, Record& rec);

    /// Get the rep_memo for a given ID; throws if id is not valud
    const char* get_rep_memo(int id);

    /// Get the ID for a given rep_memo; throws if rep_memo is not valid
    int get_id(const char* rep_memo);

    /// Get the priority for a given ID; returns INT_MAX if id is not valid
    int get_priority(int id);

    /**
     * Update the report type information in the database using the data from the
     * given file.
     *
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
     * Get a mapping between rep_memo and their priorities
     */
    std::map<std::string, int> get_priorities();

    /**
     * Return a vector of IDs matching the priority constraints in the given record.
     */
    std::vector<int> ids_by_prio(const core::Query& rec);

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

    /// Dump the entire contents of the database to an output stream
    virtual void dump(FILE* out) = 0;

protected:
    /** Cache of table entries */
    std::vector<repinfo::Cache> cache;

    /** rep_memo -> rep_cod reverse index */
    mutable std::vector<repinfo::Memoidx> memo_idx;

    /// Get a Cache entry by database ID
    const repinfo::Cache* get_by_id(unsigned id) const;

    /// Get a Cache entry by report name
    const repinfo::Cache* get_by_memo(const char* memo) const;

    /// Lookup a cache index by database ID. Returns -1 if not found
    int cache_find_by_id(unsigned id) const;

    /// Lookup a cache index by report name. Returns -1 if not found
    int cache_find_by_memo(const char* memo) const;

    /// Append an entry to the cache
    void cache_append(unsigned id, const char* memo, const char* desc, int prio, const char* descriptor, int tablea);

    /// Rebuild the memo_idx cache
    void rebuild_memo_idx() const;

    /// Read cache entries from a repinfo file on disk
    std::vector<repinfo::Cache> read_repinfo_file(const char* deffile);

    /// Return how many time this ID is used in the database
    virtual int id_use_count(unsigned id, const char* name) = 0;

    /// Delete a repinfo entry
    virtual void delete_entry(unsigned id) = 0;

    /// Update an entry using the new_* fields of \a entry
    virtual void update_entry(const repinfo::Cache& entry) = 0;

    /// Insert an entry using the new_* fields of \a entry
    virtual void insert_entry(const repinfo::Cache& entry) = 0;

    /// Reread the repinfo cache from the database
    virtual void read_cache() = 0;

    /// Create an automatic entry for a missing memo, and insert it in the database
    virtual void insert_auto_entry(const char* memo) = 0;
};

}
}
}
#endif
