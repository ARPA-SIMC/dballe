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

#include "cursor.h"
#include "qbuilder.h"
#include "db.h"
#include "dballe/db/internals.h"
#include "dballe/db/v6/repinfo.h"

#include <wreport/var.h>
#include <dballe/core/defs.h>
#include <dballe/core/record.h>
#include "lev_tr.h"

#include <sql.h>
#include <cstdio>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

bool Cursor::SQLRecord::querybest_fields_are_the_same(const SQLRecord& r)
{
    if (out_ana_id != r.out_ana_id) return false;
    if (out_id_ltr_ind != r.out_id_ltr_ind) return false;
    if (out_id_ltr_ind != SQL_NULL_DATA and out_id_ltr != r.out_id_ltr) return false;
    if (memcmp(&out_datetime, &r.out_datetime, sizeof(SQL_TIMESTAMP_STRUCT)) != 0) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}

Cursor::Cursor(v6::DB& db, unsigned int wanted, unsigned int modifiers)
    : db(db), wanted(wanted), modifiers(modifiers)
{
}

Cursor::~Cursor()
{
}

int Cursor::attr_reference_id() const
{
    return sqlrec.out_id_data;
}

int Cursor::query_count(db::Statement& stm, const Record& rec)
{
    // select <count(*)> from...
    return 0;
}

int Cursor::query_stations(db::Statement& stm, const Record& rec)
{
    // select <station info> from...
    return 0;
}

int Cursor::query_data(db::Statement& stm, const Record& rec)
{
    // select <context and data> from...
    return 0;
}

void Cursor::query_summary(db::Statement& stm, const Record& rec)
{
    // select <context, count(data), min(reftime), max(reftime)> from...
    // ...group by context
}

int Cursor::raw_query(db::Statement& stm, const Record& rec)
{
    int count;

    if (db.conn->server_type == ORACLE && !(modifiers & DBA_DB_MODIFIER_STREAM))
    {
        /* FIXME: this is a temporary solution giving an approximate row count only:
         * insert/delete/update queries run between the count and the select will
         * change the size of the result set */
        count = getcount(rec);
    }

    QueryBuilder qb(db, stm, *this, wanted, modifiers);

    qb.build_query(rec);

    from_wanted = qb.from_wanted;

    TRACE("Performing query: %s\n", qb.sql_query.c_str());
    //if (qb.modifiers & DBA_DB_MODIFIER_BEST)
    //    fprintf(stderr, "Q %s\n", qb.sql_query.c_str());

    if (modifiers & DBA_DB_MODIFIER_STREAM && db.conn->server_type != ORACLE)
        stm.set_cursor_forward_only();

#if 0
    //DBA_RUN_OR_RETURN(setstmtattr(cur->stm, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_STATIC, SQL_IS_INTEGER, "Setting SQL_CURSOR_STATIC"));
    DBA_RUN_OR_RETURN(setstmtattr(cur->stm, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER)SQL_SCROLLABLE, SQL_IS_INTEGER, "Setting SQL_SCROLLABLE"));
    DBA_RUN_OR_RETURN(setstmtattr(cur->stm, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, SQL_IS_INTEGER, "Setting SQL_CURSOR_DYNAMIC"));

#endif
    //fprintf(stderr, "********************** 0 ************\n");
    //fprintf(stderr, "** Q %s\n", dba_querybuf_get(sql_query));

    /* Perform the query */
    stm.exec_direct(qb.sql_query.data(), qb.sql_query.size());

    /* Get the number of affected rows */
    if (db.conn->server_type != ORACLE)
        count = stm.rowcount();

    /* Retrieve results will happen in dba_db_cursor_next() */
    return count;
}

int Cursor::getcount(const Record& rec)
{
    db::Statement stm(*db.conn);

    /* Scan query modifiers */
    QueryBuilder qb(db, stm, *this, wanted, modifiers);

    qb.build_count_query(rec);

    TRACE("Performing query: %s\n", qb.sql_query.c_str());
    /* fprintf(stderr, "Performing query: %s\n", dba_querybuf_get(sql_query)); */

    /* Perform the query */
    stm.exec_direct(qb.sql_query.data(), qb.sql_query.size());

    if (!stm.fetch())
        throw error_consistency("no results when trying to get the row count");

    stm.close_cursor();
    return count;
}

int Cursor::remaining() const
{
    return count;
}

wreport::Varcode Cursor::varcode() const
{
    return sqlrec.out_varcode;
}

void Cursor::to_record(Record& rec)
{
    /* Empty the record from old data */
    /* See if it works without: in theory if the caller does a record_clear
     * before the query, all the values coming out of dba_db_cursor_next should
     * just overwrite the previous ones, as the range of output parameters does
     * not change */
    /* dba_record_clear(rec); */
    db::v6::Repinfo& ri = db.repinfo();

    if (from_wanted & DBA_DB_FROM_PA)
    {
        rec.key(DBA_KEY_ANA_ID).seti(sqlrec.out_ana_id);
        if (wanted & DBA_DB_WANT_COORDS)
        {
            rec.key(DBA_KEY_LAT).seti(sqlrec.out_lat);
            rec.key(DBA_KEY_LON).seti(sqlrec.out_lon);
        }
        if (wanted & DBA_DB_WANT_IDENT)
        {
            if (sqlrec.out_ident_ind != SQL_NULL_DATA && sqlrec.out_ident[0] != 0)
            {
                rec.key(DBA_KEY_IDENT).setc(sqlrec.out_ident);
                rec.key(DBA_KEY_MOBILE).seti(1);
            } else {
                rec.key_unset(DBA_KEY_IDENT);
                rec.key(DBA_KEY_MOBILE).seti(0);
            }
        }
    }
    if (from_wanted & DBA_DB_FROM_LTR)
    {
        if (sqlrec.out_id_ltr_ind != SQL_NULL_DATA)
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
    if (from_wanted & DBA_DB_FROM_D)
    {
        if (wanted & DBA_DB_WANT_CONTEXT_ID)
            rec.key(DBA_KEY_CONTEXT_ID).seti(sqlrec.out_id_data);
        if (wanted & DBA_DB_WANT_REPCOD)
            rec.key(DBA_KEY_REP_COD).seti(sqlrec.out_rep_cod);

        /* If PA was not wanted, we can still get the ana_id */
        if (wanted & DBA_DB_WANT_ANA_ID)
            rec.key(DBA_KEY_ANA_ID).seti(sqlrec.out_ana_id);

        if (wanted & DBA_DB_WANT_DATETIME)
        {
            /*fprintf(stderr, "SETTING %s to %d\n", #var,  _db_cursor[cur].out_##var); */
            /*
            int year, mon, day, hour, min, sec;
            if (sscanf(out_datetime,
                        "%04d-%02d-%02d %02d:%02d:%02d", &year, &mon, &day, &hour, &min, &sec) != 6)
                return dba_error_consistency("parsing datetime string \"%s\"", out_datetime);
            */
            //DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_DATETIME, out_datetime));
            rec.key(DBA_KEY_YEAR).seti(sqlrec.out_datetime.year);
            rec.key(DBA_KEY_MONTH).seti(sqlrec.out_datetime.month);
            rec.key(DBA_KEY_DAY).seti(sqlrec.out_datetime.day);
            rec.key(DBA_KEY_HOUR).seti(sqlrec.out_datetime.hour);
            rec.key(DBA_KEY_MIN).seti(sqlrec.out_datetime.minute);
            rec.key(DBA_KEY_SEC).seti(sqlrec.out_datetime.second);
        }

        if (wanted & DBA_DB_WANT_VAR_NAME || wanted & DBA_DB_WANT_VAR_VALUE)
        {
            char bname[7];
            snprintf(bname, 7, "B%02d%03d",
                    WR_VAR_X(sqlrec.out_varcode),
                    WR_VAR_Y(sqlrec.out_varcode));
            rec.key(DBA_KEY_VAR).setc(bname);

            if (wanted & DBA_DB_WANT_VAR_VALUE)
            {
                rec.clear_vars();
                rec.var(sqlrec.out_varcode).setc(sqlrec.out_value);
            }
        }
    }

    if (from_wanted & (DBA_DB_FROM_RI | DBA_DB_FROM_D))
    {
        if (!(from_wanted & DBA_DB_FROM_D))
            rec.key(DBA_KEY_REP_COD).seti(sqlrec.out_rep_cod);

        if (wanted & DBA_DB_WANT_REPCOD)
        {
            const v5::repinfo::Cache* c = ri.get_by_id(sqlrec.out_rep_cod);
            if (c != NULL)
            {
                rec.key(DBA_KEY_REP_MEMO).setc(c->memo.c_str());
                rec.key(DBA_KEY_PRIORITY).seti(c->prio);
            }
        }
    }

    if (modifiers & DBA_DB_MODIFIER_ANAEXTRA)
        add_station_info(rec);
}

unsigned Cursor::query_attrs(const std::vector<wreport::Varcode>& qcs, Record& attrs)
{
    return db.query_attrs(sqlrec.out_id_data, sqlrec.out_varcode, qcs, attrs);
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
#define BASE_QUERY \
        "SELECT d.id_var, d.value, ri.id, ri.prio" \
        "  FROM data d, repinfo ri" \
        " WHERE d.id_lev_tr IS NULL AND ri.id = d.id_report AND d.id_station = ?"

    const char* query;
    switch (db.conn->server_type)
    {
        case MYSQL:
            query = BASE_QUERY
                " GROUP BY d.id_var,ri.id "
                "HAVING ri.prio=MAX(ri.prio)";
            break;
        default:
            query = BASE_QUERY
                " AND ri.prio=("
                "  SELECT MAX(sri.prio) FROM repinfo sri"
                "    JOIN data sd ON sri.id=sd.id_report"
                "  WHERE sd.id_station=d.id_station AND sd.id_lev_tr IS NULL"
                "    AND sd.id_var=d.id_var)";
            break;
    }
#undef BASE_QUERY

    unsigned short st_out_code;
    char st_out_val[256];
    SQLLEN st_out_val_ind;
    DBALLE_SQL_C_SINT_TYPE st_out_rep_cod;

    /* Allocate statement handle */
    db::Statement stm(*db.conn);

    /* Bind input fields */
    stm.bind_in(1, sqlrec.out_ana_id);

    /* Bind output fields */
    stm.bind_out(1, st_out_code);
    stm.bind_out(2, st_out_val, sizeof(st_out_val), st_out_val_ind);
    stm.bind_out(3, st_out_rep_cod);

    /* Perform the query */
    stm.exec_direct(query);

    /* Get the results and save them in the record */
    while (stm.fetch())
    {
        rec.var(st_out_code).setc(st_out_val);
        rec.key(DBA_KEY_REP_COD).seti(st_out_rep_cod);
    }
}

CursorLinear::CursorLinear(DB& db, unsigned int wanted, unsigned int modifiers)
    : Cursor(db, wanted, modifiers), stm(0)
{
    stm = new db::Statement(*db.conn);
}

CursorLinear::~CursorLinear()
{
    if (stm) delete stm;
}

int CursorLinear::query(const Record& rec)
{
    count = raw_query(*stm, rec);
    return count;
}

void CursorLinear::query_datetime_extremes(const Record& query, Record& result)
{
    QueryBuilder qb(db, *stm, *this, 0, 0);
    qb.from_wanted |= DBA_DB_FROM_D;

    SQL_TIMESTAMP_STRUCT dmin;
    SQL_TIMESTAMP_STRUCT dmax;
    SQLLEN dmin_ind;
    SQLLEN dmax_ind;
    qb.build_date_extremes_query(query);
    qb.stm.bind_out(qb.output_seq++, dmin, dmin_ind);
    qb.stm.bind_out(qb.output_seq++, dmax, dmax_ind);

    from_wanted = qb.from_wanted;

    TRACE("Performing query: %s\n", qb.sql_query.c_str());

    stm->set_cursor_forward_only();

    /* Perform the query */
    stm->exec_direct(qb.sql_query.data(), qb.sql_query.size());

    // Fetch result row
    bool res = stm->fetch();
    if (!res)
    {
        stm->close_cursor();
        throw error_consistency("datetime extremes query returned no results");
    }

    if (dmin_ind == SQL_NULL_DATA)
    {
        result.unset(DBA_KEY_YEARMIN);
        result.unset(DBA_KEY_MONTHMIN);
        result.unset(DBA_KEY_DAYMIN);
        result.unset(DBA_KEY_HOURMIN);
        result.unset(DBA_KEY_MINUMIN);
        result.unset(DBA_KEY_SECMIN);

        result.unset(DBA_KEY_YEARMAX);
        result.unset(DBA_KEY_MONTHMAX);
        result.unset(DBA_KEY_DAYMAX);
        result.unset(DBA_KEY_HOURMAX);
        result.unset(DBA_KEY_MINUMAX);
        result.unset(DBA_KEY_SECMAX);
    } else {
        result.key(DBA_KEY_YEARMIN).seti(dmin.year);
        result.key(DBA_KEY_MONTHMIN).seti(dmin.month);
        result.key(DBA_KEY_DAYMIN).seti(dmin.day);
        result.key(DBA_KEY_HOURMIN).seti(dmin.hour);
        result.key(DBA_KEY_MINUMIN).seti(dmin.minute);
        result.key(DBA_KEY_SECMIN).seti(dmin.second);

        result.key(DBA_KEY_YEARMAX).seti(dmax.year);
        result.key(DBA_KEY_MONTHMAX).seti(dmax.month);
        result.key(DBA_KEY_DAYMAX).seti(dmax.day);
        result.key(DBA_KEY_HOURMAX).seti(dmax.hour);
        result.key(DBA_KEY_MINUMAX).seti(dmax.minute);
        result.key(DBA_KEY_SECMAX).seti(dmax.second);
    }
    stm->close_cursor();
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

CursorBest::CursorBest(DB& db, unsigned int wanted, unsigned int modifiers)
    : Cursor(db, wanted, modifiers), results(0) {}

CursorBest::~CursorBest()
{
    if (results)
        fclose(results);
}

int CursorBest::query(const Record& rec)
{
    db::Statement stm(*db.conn);

    // No need to buffer server-side: we do it here
    stm.set_cursor_forward_only();

    // Make the query, we ignore the result count from the server
    raw_query(stm, rec);

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
        // Filter results keeping only those with the best priority
        if (first)
        {
            // The first record initialises 'best'
            best = sqlrec;
            first = false;
        } else if (sqlrec.querybest_fields_are_the_same(best)) {
            // If they match, keep the record with the highest priority
            if (sqlrec.out_priority > best.out_priority)
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

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
