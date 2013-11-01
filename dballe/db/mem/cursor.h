/*
 * db/v6/cursor - manage select queries
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

/** @file
 * @ingroup db
 *
 * Functions used to manage a general DB-ALLe query
 */

#ifndef DBA_DB_V6_CURSOR_H
#define DBA_DB_V6_CURSOR_H

#include <dballe/db/odbcworkarounds.h>
#include <dballe/db/db.h>
#include <wreport/varinfo.h>
#include <sqltypes.h>
#include <cstddef>
#include <vector>

namespace dballe {
struct DB;
struct Record;

namespace db {
struct Statement;

namespace v6 {
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
    /// Query results from SQL output
    struct SQLRecord
    {
        DBALLE_SQL_C_SINT_TYPE  out_lat;
        DBALLE_SQL_C_SINT_TYPE  out_lon;
        char    out_ident[64];      SQLLEN out_ident_ind;
        wreport::Varcode        out_varcode;
        SQL_TIMESTAMP_STRUCT    out_datetime;
        char    out_value[255];
        DBALLE_SQL_C_SINT_TYPE  out_rep_cod;
        DBALLE_SQL_C_SINT_TYPE  out_ana_id;
        DBALLE_SQL_C_SINT_TYPE  out_id_ltr;
        DBALLE_SQL_C_SINT_TYPE  out_id_data;
        int priority;

        /**
         * Checks true if ana_id, id_ltr, datetime and varcode are the same in
         * both records
         *
         * @returns true if they match, false if they are different
         */
        bool querybest_fields_are_the_same(const SQLRecord& r);
    };

    /** Database to operate on */
    v6::DB& db;

    /** Modifier flags to enable special query behaviours */
    const unsigned int modifiers;

    /** Number of results still to be fetched */
    DBALLE_SQL_C_SINT_TYPE count;

    /// Results written by fetch
    SQLRecord sqlrec;


    virtual ~Cursor();

    virtual dballe::DB& get_db() const;

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    int remaining() const;

    /// Perform the query
    virtual void query(const Record& query) = 0;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    virtual bool next() = 0;

    /// Discard the results that have not been read yet
    virtual void discard_rest() = 0;

    /// Get the varcode of the current entry
    wreport::Varcode varcode() const;

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

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;

protected:
    /**
     * Create a query cursor
     *
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     */
    Cursor(v6::DB& db, unsigned int modifiers);

    void to_record_pseudoana(Record& rec);
    void to_record_repinfo(Record& rec);
    void to_record_ltr(Record& rec);
    void to_record_datetime(Record& rec);
    void to_record_varcode(Record& rec);

    int query_stations(db::Statement& stm, const Record& rec);
    int query_data(db::Statement& stm, const Record& rec);

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec);

#if 0
    /**
     * Perform the raw sql query
     *
     * @returns
     *   the number of results
     */
    int raw_query(db::Statement& stm, const Record& rec);
#endif

    /**
     * Return the number of results for a query.
     *
     * This is the same as Cursor::query, but it does a SELECT COUNT(*) only.
     *
     * @warning: do not use it except to get an approximate row count:
     * insert/delete/update queries run between the count and the select will
     * change the size of the result set.
     */
    //int getcount(const Record& query);
};

class CursorLinear : public Cursor
{
public:
    virtual ~CursorLinear();

protected:
    /** ODBC statement to use for the query */
    db::Statement* stm;

    CursorLinear(DB& db, unsigned int modifiers);

    virtual void discard_rest();
    virtual bool next();

    friend class dballe::db::v6::DB;
};

struct CursorStations : public CursorLinear
{
    /// Query station info
    virtual void query(const Record& rec);
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorStations(DB& db, unsigned int modifiers)
        : CursorLinear(db, modifiers) {}

    friend class dballe::db::v6::DB;
};

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

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
