/*
 * db/mem/cursor - iterate results of queries on mem databases
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_MEM_CURSOR_H
#define DBA_DB_MEM_CURSOR_H

#include <dballe/memdb/memdb.h>
#include <dballe/memdb/results.h>
#include <dballe/db/db.h>
#include <iosfwd>

namespace dballe {
struct DB;
struct Record;

namespace memdb {
template<typename T> class ValueStorage;
}

namespace db {

namespace mem {
struct DB;

/**
 * Simple typedef to make typing easier, and also to help some versions of swig
 * match this complex type
 */
typedef std::vector<wreport::Varcode> AttrList;

/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
class Cursor : public db::Cursor
{
protected:
    /// Database to operate on
    mem::DB& db;

    /// Modifier flags to enable special query behaviours
    const unsigned int modifiers;

    /// Number of results still to be fetched
    size_t count;

    const memdb::Station* cur_station;
    const memdb::Value* cur_value;
    const wreport::Var* cur_var;

public:
    virtual ~Cursor();

    dballe::DB& get_db() const override;

    /// Get the number of rows still to be fetched
    int remaining() const override;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    bool next() override = 0;

    /// Discard the results that have not been read yet
    void discard_rest() override = 0;

    /**
     * Query attributes for the current variable
     */
    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest) override;

    int get_station_id() const override;
    double get_lat() const override;
    double get_lon() const override;
    const char* get_ident(const char* def=0) const override;
    const char* get_rep_memo() const override;
    Level get_level() const override;
    Trange get_trange() const override;
    Datetime get_datetime() const override;
    wreport::Varcode get_varcode() const override;
    wreport::Var get_var() const override;

    int attr_reference_id() const override;

#if 0
    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
#endif
    static std::unique_ptr<db::Cursor> createStations(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Station>& res);
    static std::unique_ptr<db::Cursor> createStationData(mem::DB& db, unsigned modifiers, memdb::Results<memdb::StationValue>& res);
    static std::unique_ptr<db::Cursor> createData(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Value>& res);
    static std::unique_ptr<db::Cursor> createDataBest(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Value>& res);
    static std::unique_ptr<db::Cursor> createSummary(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Value>& res);

protected:
    /**
     * Create a query cursor
     *
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     */
    Cursor(mem::DB& db, unsigned modifiers);

    void to_record_station(Record& rec);
    void to_record_levtr(Record& rec);
    void to_record_varcode(Record& rec);
    void to_record_value(Record& rec);

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec);
};

namespace cursor {

/**
 * Wrapper around a Value index that compares so that all values from which the
 * best report should be selected appear to be the same
 */
struct DataBestKey
{
    const memdb::ValueStorage<memdb::Value>& values;
    size_t idx;

    DataBestKey(const memdb::ValueStorage<memdb::Value>& values, size_t idx)
        : values(values), idx(idx) {}

    const memdb::Value& value() const;

    bool operator<(const DataBestKey& o) const;
};

std::ostream& operator<<(std::ostream& out, const DataBestKey& k);

}

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
