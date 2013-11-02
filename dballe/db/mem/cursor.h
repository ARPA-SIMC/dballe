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
#include <dballe/memdb/query.h>
#include <dballe/db/db.h>
#if 0
#include <wreport/varinfo.h>
#include <cstddef>
#include <vector>
#endif

namespace dballe {
struct DB;
struct Record;

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
struct Cursor : public db::Cursor
{
    /// Database to operate on
    mem::DB& db;

    /// Modifier flags to enable special query behaviours
    const unsigned int modifiers;

    /// Number of results still to be fetched
    size_t count;

    size_t cur_station_id;
    const memdb::Station* cur_station;
    const memdb::Value* cur_value;

    virtual ~Cursor();

    virtual dballe::DB& get_db() const;

    /// Get the number of rows still to be fetched
    int remaining() const;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    virtual bool next() = 0;

    /// Discard the results that have not been read yet
    virtual void discard_rest() = 0;

    /**
     * Query attributes for the current variable
     */
    unsigned query_attrs(const AttrList& qcs, Record& attrs);

    virtual int get_station_id() const;
    virtual double get_lat() const;
    virtual double get_lon() const;
    virtual const char* get_ident(const char* def=0) const;
    virtual const char* get_rep_memo(const char* def=0) const;
    virtual Level get_level() const;
    virtual Trange get_trange() const;
    virtual void get_datetime(int (&dt)[6]) const;
    virtual wreport::Varcode get_varcode() const;
    virtual wreport::Var get_var() const;

    virtual int attr_reference_id() const;

#if 0
    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
#endif

protected:
    /**
     * Create a query cursor
     *
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     */
    Cursor(mem::DB& db, unsigned modifiers, size_t count=0);

    void to_record_station(Record& rec);
    void to_record_stationvar(const memdb::StationValue& value, Record& rec);
#if 0
    void to_record_ltr(Record& rec);
    void to_record_datetime(Record& rec);
    void to_record_varcode(Record& rec);
#endif

    /// Query extra station info and add it to \a rec
    void add_station_info(const memdb::Station& station, Record& rec);
};

template<typename T>
class CursorLinear : public Cursor
{
public:
    virtual ~CursorLinear() {};

protected:
    memdb::Results<T> res;
    typename memdb::Results<T>::const_iterator iter_cur;
    typename memdb::Results<T>::const_iterator iter_end;

    CursorLinear(DB& db, unsigned int modifiers, memdb::Results<T>& res)
        : Cursor(db, modifiers, res.size()), res(res), iter_cur(res.begin()), iter_end(res.end()) {}

    virtual void discard_rest();
    virtual bool next();

    friend class mem::DB;
};

struct CursorStations : public CursorLinear<memdb::Station>
{
protected:
    virtual void to_record(Record& rec);
    virtual bool next();
#if 0
    virtual unsigned test_iterate(FILE* dump=0);
#endif

    CursorStations(DB& db, unsigned int modifiers, memdb::Results<memdb::Station>& res)
        : CursorLinear<memdb::Station>(db, modifiers, res) {}

    friend class mem::DB;
};

#if 0
struct CursorData : public CursorLinear
{
    /// Query data
    virtual void query(const Record& rec);
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

    /// Query count of items (only for stations and data)
    //int query_count(const Record& rec);

protected:
    CursorData(DB& db, unsigned int modifiers)
        : CursorLinear(db, modifiers) {}

    friend class dballe::db::v6::DB;
};

class CursorSummary : public CursorLinear
{
public:
    SQL_TIMESTAMP_STRUCT    out_datetime_max;

    /// Query stats about all possible context combinations
    virtual void query(const Record& rec);
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorSummary(DB& db, unsigned int modifiers)
        : CursorLinear(db, modifiers) {}

    friend class dballe::db::v6::DB;
};

struct CursorDataIDs : public CursorLinear
{
    /// Query the data IDs only, to use to delete things
    virtual void query(const Record& rec);
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorDataIDs(DB& db, unsigned int modifiers)
        : CursorLinear(db, modifiers) {}

    friend class dballe::db::v6::DB;
};

class CursorBest : public Cursor
{
public:
    virtual ~CursorBest();

    virtual void query(const Record& rec);
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    FILE* results;

    CursorBest(DB& db, unsigned int modifiers);

    // Save all cursor results to a temp file, filtered to keep the best values
    // only
    int buffer_results(db::Statement& stm);

    virtual void discard_rest();
    virtual bool next();

    friend class dballe::db::v6::DB;
};
#endif

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
