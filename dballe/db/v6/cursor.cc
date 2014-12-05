/*
 * db/v6/cursor - manage select queries
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

#include "cursor.h"
#include "qbuilder.h"
#include "db.h"
#include "dballe/db/odbc/internals.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/v6/internals.h"

#include <wreport/var.h>
#include <dballe/core/defs.h>
#include <dballe/core/record.h>

#include <sql.h>
#include <cstdio>
#include <cstring>

namespace {

// Consts used for to_record_todo
const unsigned int TOREC_PSEUDOANA   = 1 << 0;
const unsigned int TOREC_BASECONTEXT = 1 << 1;
const unsigned int TOREC_DATACONTEXT = 1 << 2;
const unsigned int TOREC_DATA        = 1 << 3;

}

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

bool Cursor::SQLRecord::querybest_fields_are_the_same(const SQLRecord& r)
{
    if (out_ana_id != r.out_ana_id) return false;
    if (out_id_ltr != r.out_id_ltr) return false;
    if (memcmp(&out_datetime, &r.out_datetime, sizeof(SQL_TIMESTAMP_STRUCT)) != 0) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}

Cursor::Cursor(v6::DB& db, unsigned int modifiers)
    : db(db), conn(*dynamic_cast<ODBCConnection*>(db.conn)), modifiers(modifiers)
{
}

Cursor::~Cursor()
{
}

dballe::DB& Cursor::get_db() const { return db; }

int Cursor::attr_reference_id() const
{
    if (sqlrec.out_id_ltr == -1)
        return MISSING_INT;
    return sqlrec.out_id_data;
}

int Cursor::remaining() const
{
    return count;
}

int Cursor::get_station_id() const { return sqlrec.out_ana_id; }
double Cursor::get_lat() const { return (double)sqlrec.out_lat / 100000.0; }
double Cursor::get_lon() const { return (double)sqlrec.out_lon / 100000.0; }
const char* Cursor::get_ident(const char* def) const
{
    if (sqlrec.out_ident_ind == SQL_NULL_DATA || sqlrec.out_ident[0] == 0)
        return def;
    return sqlrec.out_ident;
}
const char* Cursor::get_rep_memo() const
{
    return db.repinfo().get_rep_memo(sqlrec.out_rep_cod);
}
Level Cursor::get_level() const
{
    if (sqlrec.out_id_ltr == -1)
        return Level();
    return db.lev_tr_cache().to_level(sqlrec.out_id_ltr);
}
Trange Cursor::get_trange() const
{
    if (sqlrec.out_id_ltr == -1)
        return Trange();
    return db.lev_tr_cache().to_trange(sqlrec.out_id_ltr);
}
void Cursor::get_datetime(int (&dt)[6]) const
{
    dt[0] = sqlrec.out_datetime.year;
    dt[1] = sqlrec.out_datetime.month;
    dt[2] = sqlrec.out_datetime.day;
    dt[3] = sqlrec.out_datetime.hour;
    dt[4] = sqlrec.out_datetime.minute;
    dt[5] = sqlrec.out_datetime.second;
}
wreport::Varcode Cursor::get_varcode() const { return (wreport::Varcode)sqlrec.out_varcode; }
wreport::Var Cursor::get_var() const
{
    return Var(varinfo(sqlrec.out_varcode), sqlrec.out_value);
}

void Cursor::to_record_pseudoana(Record& rec)
{
    rec.key(DBA_KEY_ANA_ID).seti(sqlrec.out_ana_id);
    rec.key(DBA_KEY_LAT).seti(sqlrec.out_lat);
    rec.key(DBA_KEY_LON).seti(sqlrec.out_lon);
    if (sqlrec.out_ident_ind != SQL_NULL_DATA && sqlrec.out_ident[0] != 0)
    {
        rec.key(DBA_KEY_IDENT).setc(sqlrec.out_ident);
        rec.key(DBA_KEY_MOBILE).seti(1);
    } else {
        rec.key_unset(DBA_KEY_IDENT);
        rec.key(DBA_KEY_MOBILE).seti(0);
    }
}

void Cursor::to_record_repinfo(Record& rec)
{
    db.repinfo().to_record(sqlrec.out_rep_cod, rec);
}

void Cursor::to_record_ltr(Record& rec)
{
    if (sqlrec.out_id_ltr != -1)
        db.lev_tr_cache().to_rec(sqlrec.out_id_ltr, rec);
    else
    {
        rec.key(DBA_KEY_LEVELTYPE1).unset();
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

void Cursor::query_attrs(function<void(unique_ptr<Var>)> dest)
{
    db.query_attrs(sqlrec.out_id_data, sqlrec.out_varcode, dest);
}

void Cursor::attr_insert(const dballe::Record& attrs)
{
    db.attr_insert(sqlrec.out_id_data, sqlrec.out_varcode, attrs);
}

void Cursor::attr_remove(const AttrList& qcs)
{
    db.attr_remove(sqlrec.out_id_data, sqlrec.out_varcode, qcs);
}

void Cursor::add_station_info(Record& rec)
{
    /* Extra variables to add:
     *
     * HEIGHT,      B07001  1793
     * HEIGHT_BARO, B07031  1823
     * ST_NAME,     B01019   275
     * BLOCK,       B01001   257
     * STATION,     B01002   258
    */
    const char* query;
    switch (conn.server_type)
    {
        case ServerType::MYSQL:
            query =
                "SELECT d.id_var, d.value, ri.prio"
                "  FROM data d, repinfo ri"
                " WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = ?"
                " GROUP BY d.id_var,ri.id "
                "HAVING ri.prio=MAX(ri.prio)";
            break;
        default:
            query =
                "SELECT d.id_var, d.value"
                "  FROM data d, repinfo ri"
                " WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = ?"
                " AND ri.prio=("
                "  SELECT MAX(sri.prio) FROM repinfo sri"
                "    JOIN data sd ON sri.id=sd.id_report"
                "  WHERE sd.id_station=d.id_station AND sd.id_lev_tr = -1"
                "    AND sd.id_var=d.id_var)";
            break;
    }

    unsigned short st_out_code;
    char st_out_val[256];
    SQLLEN st_out_val_ind;

    /* Allocate statement handle */
    auto stm = conn.odbcstatement();

    /* Bind input fields */
    stm->bind_in(1, sqlrec.out_ana_id);

    /* Bind output fields */
    stm->bind_out(1, st_out_code);
    stm->bind_out(2, st_out_val, sizeof(st_out_val), st_out_val_ind);

    /* Perform the query */
    stm->exec_direct(query);

    /* Get the results and save them in the record */
    while (stm->fetch())
        rec.var(st_out_code).setc(st_out_val);
}

CursorLinear::CursorLinear(DB& db, unsigned int modifiers)
    : Cursor(db, modifiers), stm(0)
{
    stm = conn.odbcstatement().release();
}

CursorLinear::~CursorLinear()
{
    if (stm) delete stm;
}

bool CursorLinear::next()
{
    /* Fetch new data */
    bool res = stm->fetch();
    if (count != -1)
        --count;
    if (!res)
        stm->close_cursor();
    return res;
}

void CursorLinear::discard_rest()
{
    stm->close_cursor();
}

void CursorStations::query(const Record& rec)
{
#if 0
    if (conn.server_type == ServerType::ORACLE && !(modifiers & DBA_DB_MODIFIER_STREAM))
    {
        /* FIXME: this is a temporary solution giving an approximate row count only:
         * insert/delete/update queries run between the count and the select will
         * change the size of the result set */
        count = getcount(rec);
    }
#endif

    StationQueryBuilder qb(db, *stm, *this, rec, modifiers);
    qb.build();

    if (modifiers & DBA_DB_MODIFIER_STREAM && conn.server_type != ServerType::ORACLE)
    {
        stm->set_cursor_forward_only();
    //} else {
    //    stm->set_cursor_static();
    }
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    //fprintf(stderr, "Query: %s\n", qb.sql_query.c_str());

    // Get the number of affected rows
    if (conn.server_type != ServerType::ORACLE)
        count = stm->select_rowcount();
}

void CursorStations::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    add_station_info(rec);
}

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
    if (conn.server_type == ServerType::ORACLE && !(modifiers & DBA_DB_MODIFIER_STREAM))
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

    if (modifiers & DBA_DB_MODIFIER_STREAM && conn.server_type != ServerType::ORACLE)
        stm->set_cursor_forward_only();
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    // Get the number of affected rows
    if (conn.server_type != ServerType::ORACLE)
        count = stm->select_rowcount();
}

void CursorData::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    if (sqlrec.out_id_ltr == -1)
        rec.unset(DBA_KEY_CONTEXT_ID);
    else
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

    if (modifiers & DBA_DB_MODIFIER_STREAM && conn.server_type != ServerType::ORACLE)
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
            fprintf(dump, "%02d %s %03d %s %04d-%02d-%02d %02d:%02d:%02d  %04d-%02d-%02d %02d:%02d:%02d  %d\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_REP_MEMO, -1),
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
    : Cursor(db, modifiers) {}

CursorBest::~CursorBest() {}

void CursorBest::query(const Record& rec)
{
    auto stm = conn.odbcstatement();

    DataQueryBuilder qb(db, *stm, *this, rec, modifiers);
    qb.build();
    // fprintf(stderr, "Query: %s\n", qb.sql_query.c_str());

    stm->set_cursor_forward_only();
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    buffer_results(*stm);
}

void CursorBest::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    if (sqlrec.out_id_ltr == -1)
        rec.unset(DBA_KEY_CONTEXT_ID);
    else
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

int CursorBest::buffer_results(ODBCStatement& stm)
{
    db::v5::Repinfo& ri = db.repinfo();

    bool first = true;
    SQLRecord best;

    // Copy results to temporary file
    while (stm.fetch())
    {
        // Fill priority
        sqlrec.priority = ri.get_priority(sqlrec.out_rep_cod);

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
            results.append(best);
            // And restart with a new candidate best record for the next batch
            best = sqlrec;
        }
    }

    // Write out the last best value
    if (!first)
        results.append(best);

    count = results.size();

    // We are done adding, prepare the structbuf for reading
    results.ready_to_read();

    return count;
}

bool CursorBest::next()
{
    if (count <= 0) return false;

    sqlrec = results[results.size() - count];
    --count;

    return true;
}

void CursorBest::discard_rest()
{
    count = 0;
}

}
}
}
