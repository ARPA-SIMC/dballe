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

#include "cursor.h"
#include "db.h"
#include "dballe/core/record.h"
#if 0
#include "dballe/db/internals.h"
#include "dballe/db/v6/repinfo.h"

#include <wreport/var.h>
#include <dballe/core/defs.h>
#include "lev_tr.h"

#include <sql.h>
#include <cstdio>
#include <cstring>
#endif

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {
namespace db {
namespace mem {

#if 0
bool Cursor::SQLRecord::querybest_fields_are_the_same(const SQLRecord& r)
{
    if (out_ana_id != r.out_ana_id) return false;
    if (out_id_ltr != r.out_id_ltr) return false;
    if (memcmp(&out_datetime, &r.out_datetime, sizeof(SQL_TIMESTAMP_STRUCT)) != 0) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}
#endif

Cursor::Cursor(mem::DB& db, unsigned modifiers, size_t count)
    : db(db), modifiers(modifiers), count(count),
      cur_station_id(0), cur_station(0), cur_value(0)
{
}

Cursor::~Cursor()
{
}

dballe::DB& Cursor::get_db() const { return db; }

int Cursor::remaining() const
{
    return count;
}

int Cursor::attr_reference_id() const
{
#warning no idea what to use here yet
    //return sqlrec.out_id_data;
    return 0;
}

int Cursor::get_station_id() const { return cur_station_id; }
double Cursor::get_lat() const { return cur_station->coords.dlat(); }
double Cursor::get_lon() const { return cur_station->coords.dlon(); }
const char* Cursor::get_ident(const char* def) const
{
    if (cur_station->mobile)
        return cur_station->ident.c_str();
    else
        return def;
}
const char* Cursor::get_rep_memo(const char* def) const { return cur_station->report.c_str(); }

Level Cursor::get_level() const
{
    if (cur_value)
        return cur_value->levtr.level;
    else
        return Level();
}
Trange Cursor::get_trange() const
{
    if (cur_value)
        return cur_value->levtr.trange;
    else
        return Trange();
}
void Cursor::get_datetime(int (&dt)[6]) const
{
    if (cur_value)
    {
        dt[0] = cur_value->datetime.year;
        dt[1] = cur_value->datetime.month;
        dt[2] = cur_value->datetime.day;
        dt[3] = cur_value->datetime.hour;
        dt[4] = cur_value->datetime.minute;
        dt[5] = cur_value->datetime.second;
    } else {
        dt[0] = 1000;
        dt[1] = 1;
        dt[2] = 1;
        dt[3] = 0;
        dt[4] = 0;
        dt[5] = 0;
    }
}
wreport::Varcode Cursor::get_varcode() const { return (wreport::Varcode)0; }
wreport::Var Cursor::get_var() const
{
    throw error_consistency("get_var not supported on this query");
}

void Cursor::to_record_station(Record& rec)
{
    rec.set(DBA_KEY_ANA_ID, (int)cur_station_id);
    rec.set(cur_station->coords);
    if (cur_station->mobile)
    {
        rec.set(DBA_KEY_IDENT, cur_station->ident);
        rec.key(DBA_KEY_MOBILE).seti(1);
    } else {
        rec.key_unset(DBA_KEY_IDENT);
        rec.key(DBA_KEY_MOBILE).seti(0);
    }
    rec.set(DBA_KEY_REP_MEMO, cur_station->report);
    rec.set(DBA_KEY_PRIORITY, db.repinfo.get_prio(cur_station->report));
}

void Cursor::to_record_stationvar(const StationValue& value, Record& rec)
{
    rec.set(*value.var);
}

#if 0
void Cursor::to_record_ltr(Record& rec)
{
    if (sqlrec.out_id_ltr != -1)
        db.lev_tr_cache().to_rec(sqlrec.out_id_ltr, rec);
    else
    {
        rec.key(DBA_KEY_LEVELTYPE1).seti(257);
        rec.key(DBA_KEY_L1).unset();
        rec.key(DBA_KEY_LEVELTYPE2).unset();
        rec.key(DBA_KEY_L2).unset();
        rec.key(DBA_KEY_PINDICATOR).unset();
        rec.key(DBA_KEY_P1).unset();
        rec.key(DBA_KEY_P2).unset();
    }
}

void Cursor::to_record_datetime(Record& rec)
{
    rec.key(DBA_KEY_YEAR).seti(sqlrec.out_datetime.year);
    rec.key(DBA_KEY_MONTH).seti(sqlrec.out_datetime.month);
    rec.key(DBA_KEY_DAY).seti(sqlrec.out_datetime.day);
    rec.key(DBA_KEY_HOUR).seti(sqlrec.out_datetime.hour);
    rec.key(DBA_KEY_MIN).seti(sqlrec.out_datetime.minute);
    rec.key(DBA_KEY_SEC).seti(sqlrec.out_datetime.second);
}

void Cursor::to_record_varcode(Record& rec)
{
    char bname[7];
    snprintf(bname, 7, "B%02d%03d",
            WR_VAR_X(sqlrec.out_varcode),
            WR_VAR_Y(sqlrec.out_varcode));
    rec.key(DBA_KEY_VAR).setc(bname);
}
#endif

unsigned Cursor::query_attrs(const std::vector<wreport::Varcode>& qcs, Record& attrs)
{
#warning to be implemented
    return 0;
    //return db.query_attrs(sqlrec.out_id_data, sqlrec.out_varcode, qcs, attrs);
}

void Cursor::add_station_info(const Station& station, Record& rec)
{
    Results<StationValue> res(db.memdb.stationvalues);
    //db.memdb.stationvalues.query(station, res);
    if (res.is_select_all())
    {
        for (Results<StationValue>::all_const_iterator i = res.all_begin();
                i != res.all_end(); ++i)
            to_record_stationvar(*i, rec);
    } else {
        for (Results<StationValue>::selected_const_iterator i = res.selected_begin();
                i != res.selected_end(); ++i)
            to_record_stationvar(*i, rec);
    }
}

template<typename T, typename ITER>
bool CursorLinear<T, ITER>::next()
{
    if (iter_cur == iter_end)
        return false;

    ++iter_cur;
    --count;
    return iter_cur != iter_end;
}

template<typename T, typename ITER>
void CursorLinear<T, ITER>::discard_rest()
{
    iter_cur = iter_end;
    count = 0;
}

template<typename ITER>
bool CursorStations<ITER>::next()
{
    bool res = CursorLinear<memdb::Station, ITER>::next();
    if (res)
    {
        this->cur_station_id = this->iter_cur.index();
        this->cur_station = &(*this->iter_cur);
    }
    return res;
}

template<typename ITER>
void CursorStations<ITER>::to_record(Record& rec)
{
    this->to_record_station(rec);
    this->add_station_info(*this->iter_cur, rec);
}

#if 0
unsigned CursorStations::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
            to_record(r);
            fprintf(dump, "%02d %02.4f %02.4f %-10s\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_LAT, 0.0),
                    r.get(DBA_KEY_LON, 0.0),
                    r.get(DBA_KEY_IDENT, ""));
        }
    }
    return count;
}

void CursorData::query(const Record& rec)
{
#if 0
    if (db.conn->server_type == ORACLE && !(modifiers & DBA_DB_MODIFIER_STREAM))
    {
        /* FIXME: this is a temporary solution giving an approximate row count only:
         * insert/delete/update queries run between the count and the select will
         * change the size of the result set */
        count = getcount(rec);
    }
#endif
    DataQueryBuilder qb(db, *stm, *this, rec, modifiers);
    qb.build();
    //fprintf(stderr, "Query: %s\n", qb.sql_query.c_str());

    if (modifiers & DBA_DB_MODIFIER_STREAM && db.conn->server_type != ORACLE)
        stm->set_cursor_forward_only();
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    // Get the number of affected rows
    if (db.conn->server_type != ORACLE)
        count = stm->select_rowcount();
}

void CursorData::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    rec.key(DBA_KEY_CONTEXT_ID).seti(sqlrec.out_id_data);
    to_record_varcode(rec);
    to_record_ltr(rec);
    to_record_datetime(rec);

    rec.clear_vars();
    rec.var(sqlrec.out_varcode).setc(sqlrec.out_value);

    if (modifiers & DBA_DB_MODIFIER_ANAEXTRA)
        add_station_info(rec);
}

unsigned CursorData::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
/*
            to_record(r);
            fprintf(dump, "%02d %06d %06d %-10s\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_LAT, 0.0),
                    r.get(DBA_KEY_LON, 0.0),
                    r.get(DBA_KEY_IDENT, ""));
                    */
        }
    }
    return count;
}

void CursorDataIDs::query(const Record& rec)
{
    IdQueryBuilder qb(db, *stm, *this, rec, modifiers);
    qb.build();
    //fprintf(stderr, "Query: %s\n", qb.sql_query.c_str());

    if (modifiers & DBA_DB_MODIFIER_STREAM && db.conn->server_type != ORACLE)
        stm->set_cursor_forward_only();
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    count = 0;
}

void CursorDataIDs::to_record(Record& rec)
{
}

unsigned CursorDataIDs::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
        if (dump)
            fprintf(dump, "%03d\n", (int)sqlrec.out_id_data);
    return count;
}

#if 0
int CursorLinear::query_count(const Record& rec)
{
    to_record_todo = 0;
    // select <count(*)> from...
    return 0;
}
#endif

void CursorSummary::query(const Record& rec)
{
    SummaryQueryBuilder qb(db, *stm, *this, rec, modifiers);
    qb.build();
    //fprintf(stderr, "Query: %s\n", qb.sql_query.c_str());

    stm->set_cursor_forward_only();
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    count = 0;
}

void CursorSummary::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    //rec.key(DBA_KEY_CONTEXT_ID).seti(sqlrec.out_id_data);
    to_record_varcode(rec);
    to_record_ltr(rec);

    /*
    // Min datetime
    rec.key(DBA_KEY_YEARMIN).seti(sqlrec.out_datetime.year);
    rec.key(DBA_KEY_MONTHMIN).seti(sqlrec.out_datetime.month);
    rec.key(DBA_KEY_DAYMIN).seti(sqlrec.out_datetime.day);
    rec.key(DBA_KEY_HOURMIN).seti(sqlrec.out_datetime.hour);
    rec.key(DBA_KEY_MINUMIN).seti(sqlrec.out_datetime.minute);
    rec.key(DBA_KEY_SECMIN).seti(sqlrec.out_datetime.second);

    // Max datetime
    rec.key(DBA_KEY_YEARMAX).seti(out_datetime_max.year);
    rec.key(DBA_KEY_MONTHMAX).seti(out_datetime_max.month);
    rec.key(DBA_KEY_DAYMAX).seti(out_datetime_max.day);
    rec.key(DBA_KEY_HOURMAX).seti(out_datetime_max.hour);
    rec.key(DBA_KEY_MINUMAX).seti(out_datetime_max.minute);
    rec.key(DBA_KEY_SECMAX).seti(out_datetime_max.second);

    // Abuse id_data and datetime for count and min(datetime)
    rec.key(DBA_KEY_LIMIT).seti(sqlrec.out_id_data);
    */
}

unsigned CursorSummary::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
            to_record(r);
            fprintf(dump, "%02d %03d %03d %s %04d-%02d-%02d %02d:%02d:%02d  %04d-%02d-%02d %02d:%02d:%02d  %d\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_REP_COD, -1),
                    (int)sqlrec.out_id_ltr,
                    r.get(DBA_KEY_VAR, ""),
                    r.get(DBA_KEY_YEARMIN, 0), r.get(DBA_KEY_MONTHMIN, 0), r.get(DBA_KEY_DAYMIN, 0),
                    r.get(DBA_KEY_HOURMIN, 0), r.get(DBA_KEY_MINUMIN, 0), r.get(DBA_KEY_SECMIN, 0),
                    r.get(DBA_KEY_YEARMAX, 0), r.get(DBA_KEY_MONTHMAX, 0), r.get(DBA_KEY_DAYMAX, 0),
                    r.get(DBA_KEY_HOURMAX, 0), r.get(DBA_KEY_MINUMAX, 0), r.get(DBA_KEY_SECMAX, 0),
                    r.get(DBA_KEY_LIMIT, -1));
        }
    }
    return count;
}


CursorBest::CursorBest(DB& db, unsigned int modifiers)
    : Cursor(db, modifiers), results(0) {}

CursorBest::~CursorBest()
{
    if (results)
        fclose(results);
}

void CursorBest::query(const Record& rec)
{
    db::Statement stm(*db.conn);

    DataQueryBuilder qb(db, stm, *this, rec, modifiers);
    qb.build();
    // fprintf(stderr, "Query: %s\n", qb.sql_query.c_str());

    stm.set_cursor_forward_only();
    stm.exec_direct(qb.sql_query.data(), qb.sql_query.size());

    buffer_results(stm);
}

void CursorBest::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    rec.key(DBA_KEY_CONTEXT_ID).seti(sqlrec.out_id_data);
    to_record_varcode(rec);
    to_record_ltr(rec);
    to_record_datetime(rec);

    rec.clear_vars();
    rec.var(sqlrec.out_varcode).setc(sqlrec.out_value);

    if (modifiers & DBA_DB_MODIFIER_ANAEXTRA)
        add_station_info(rec);
}

unsigned CursorBest::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        /*
        if (dump)
            fprintf(dump, "%03d\n", (int)sqlrec.out_id_data);
            */
    }
    return count;
}

int CursorBest::buffer_results(db::Statement& stm)
{
    db::v6::Repinfo& ri = db.repinfo();

    // Do the counting here, so we don't count the records we skip
    count = 0;

    // Open temporary file
    results = tmpfile();
    if (!results)
        throw error_system("cannot create temporary file for query=best results");

    bool first = true;
    SQLRecord best;

    // Copy results to temporary file
    while (stm.fetch())
    {
        // Fill priority
        const db::v5::repinfo::Cache* ri_entry = ri.get_by_id(sqlrec.out_rep_cod);
        sqlrec.priority = ri_entry ? ri_entry->prio : INT_MAX;

        // Filter results keeping only those with the best priority
        if (first)
        {
            // The first record initialises 'best'
            best = sqlrec;
            first = false;
        } else if (sqlrec.querybest_fields_are_the_same(best)) {
            // If they match, keep the record with the highest priority
            if (sqlrec.priority > best.priority)
                best = sqlrec;
        } else {
            // If they don't match, write out the previous best value
            if (fwrite(&best, sizeof(best), 1, results) != 1)
                throw error_system("cannot write query=best result record to temporary file");
            ++count;
            // And restart with a new candidate best record for the next batch
            best = sqlrec;
        }
    }

    if (!first)
    {
        // Write out the last best value
        if (fwrite(&best, sizeof(best), 1, results) != 1)
            throw error_system("cannot write query=best result record to temporary file");
        ++count;
    }

    // Go back to the beginning of the file so that next() can read results
    rewind(results);

    return count;
}

bool CursorBest::next()
{
    if (count <= 0) return false;

    if (fread(&sqlrec, sizeof(sqlrec), 1, results) != 1)
        throw error_system("cannot read query=best result record from temporary file");
    --count;

    return true;
}

void CursorBest::discard_rest()
{
    if (results)
    {
        fclose(results);
        results = 0;
    }
}
#endif

template class CursorStations<memdb::Results<memdb::Station>::all_const_iterator>;
template class CursorStations<memdb::Results<memdb::Station>::selected_const_iterator>;

}
}
}
#include "dballe/memdb/query.tcc"
