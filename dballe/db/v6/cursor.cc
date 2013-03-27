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
#include "db.h"
#include "dballe/db/internals.h"
#include "dballe/db/v6/repinfo.h"

#include <wreport/var.h>
#include <dballe/core/defs.h>
#include <dballe/core/aliases.h>
#include <dballe/core/record.h>
#include <dballe/db/querybuf.h>
#include "lev_tr.h"

#include <sql.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <regex.h>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

namespace {

/**
 * Constants used to define what is needed from the FROM part of the query
 */
/** Add pseudoana to the FROM part of the query */
#define DBA_DB_FROM_PA          (1 << 0)
/** Add lev_tr to the FROM part of the query */
#define DBA_DB_FROM_LTR         (1 << 1)
/** Add data to the FROM part of the query */
#define DBA_DB_FROM_D           (1 << 2)
/** Add repinfo to the FROM part of the query */
#define DBA_DB_FROM_RI          (1 << 3)
/** Add the the block variables as 'dblo' to the FROM part of the query */
#define DBA_DB_FROM_DBLO        (1 << 5)
/** Add the the station variables as 'dsta' to the FROM part of the query */
#define DBA_DB_FROM_DSTA        (1 << 6)
/** Add the the pseudoana variables as 'dana' to the FROM part of the query */
#define DBA_DB_FROM_DANA        (1 << 7)
/** Add an extra data table as 'ddf' to the FROM part of the query, to restrict
 * the query on variable values */
#define DBA_DB_FROM_DDF         (1 << 8)
/** Add an extra attr table as 'adf' to the FROM part of the query, to restrict
 * the query on variable attributes */
#define DBA_DB_FROM_ADF         (1 << 9)

struct QueryBuilder
{
    /** Database to operate on */
    DB& db;

    /** Statement to build variables to */
    Statement& stm;

    /** Cursor with the output variables */
    Cursor& cur;

    /** Dynamically generated SQL query */
    Querybuf sql_query;

    /** WHERE subquery */
    Querybuf sql_where;

    /** What values are wanted from the query */
    unsigned int wanted;

    /** Modifier flags to enable special query behaviours */
    unsigned int modifiers;

    /** What is needed from the SELECT part of the query */
    unsigned int select_wanted;

    /** What is needed from the FROM part of the query */
    unsigned int from_wanted;

    /** Sequence number to use to bind ODBC input parameters */
    unsigned int input_seq;

    /** Sequence number to use to bind ODBC output parameters */
    unsigned int output_seq;

    /** True if we also accept results from the anagraphical context */
    bool query_station_vars;

    /// true if we have already appended the "ORDER BY" clause to the query
    bool has_orderby;

    /** Selection parameters (input) for the query
     * @{
     */
    SQL_TIMESTAMP_STRUCT    sel_dtmin;
    SQL_TIMESTAMP_STRUCT    sel_dtmax;
    char    sel_ident[64];
    /** @} */

    QueryBuilder(DB& db, Statement& stm, Cursor& cur, int wanted, int modifiers)
        : db(db), stm(stm), cur(cur), sql_query(2048), sql_where(1024),
          wanted(wanted), modifiers(modifiers),
          select_wanted(0), from_wanted(0), input_seq(1), output_seq(1),
          query_station_vars(false), has_orderby(false) {}

    /**
     * Add one or more fields to the ORDER BY part of sql_query.
     */
    void add_to_orderby(const char* fields);

    /**
     * Add extra JOIN clauses to sql_query according to what is wanted.
     *
     * @param base
     *   The first table mentioned in the query, to which the other tables are
     *   joined
     */
    void add_other_froms(unsigned int base, const Record& rec);

    /**
     * Add an extra data table to the join, set to act as a filter on the
     * station level
     */
    void add_station_filter(Querybuf& q, const char* name, const char* extra_query=NULL);

    /// Resolve table/field dependencies adding the missing bits to from_wanted
    void resolve_dependencies();

    /// Prepare SELECT Part and see what needs to be available in the FROM part
    void make_select();

    /**
     * Prepare the extra bits of the SELECT part that need information after
     * dependency computation
     */
    void make_extra_select();

    /// Build the FROM and WHERE parts of the query
    void make_from(const Record& rec);

    /// Add an int field to the WHERE part of the query, binding it as an input parameter
    void add_int(const Record& rec, dba_keyword key, const char* sql, int needed_from);

    /// Build the WHERE part of the query, and bind the input parameters
    void make_where(const Record& rec);

    /// Add repinfo-related WHERE clauses on column \a colname to \a buf from \a query
    void add_repinfo_where(Querybuf& buf, const Record& query, const char* colname);

    /// Build the big data query
    void build_query(const Record& rec);

    /// Build the query with just SELECT COUNT(*)
    void build_count_query(const Record& rec);

    /// Build the query with just a select for date extremes
    void build_date_extremes_query(const Record& rec);
};

} // anonymous namespace


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

void QueryBuilder::build_query(const Record& rec)
{
    int limit = -1;
    if (const Var* var = rec.key_peek(DBA_KEY_LIMIT))
        limit = var->enqi();

    if (limit != -1 && db.conn->server_type == ORACLE && (modifiers & DBA_DB_MODIFIER_BEST))
        throw error_unimplemented("best-value queries with result limit are not implemented for Oracle");

    sql_query.append("SELECT ");
    if (modifiers & DBA_DB_MODIFIER_DISTINCT)
        sql_query.append("DISTINCT ");
    if (modifiers & DBA_DB_MODIFIER_BIGANA && db.conn->server_type == MYSQL)
        sql_query.append("straight_join ");

    /* Prepare WHERE part and see what needs to be available in the FROM part */
    make_where(rec);

    /* Prepare SELECT Part and see what needs to be available in the FROM part.
     * We do this after creating the WHERE part, so that we can add
     * more opportunistic extra values (see the end of make_select) */
    make_select();

    /* Solve dependencies among the various parts of the query */
    resolve_dependencies();

    // Extra bits of select after dependency resolution
    make_extra_select();

    /* Append the FROM part of the query */
    make_from(rec);

    /* Append the WHERE part that we prepared previously */
    if (!sql_where.empty())
    {
        sql_query.append("WHERE ");
        sql_query.append(sql_where);
    }

    /* Append ORDER BY as needed */
    if (!(modifiers & DBA_DB_MODIFIER_UNSORTED))
    {
        if (limit != -1 && db.conn->server_type == ORACLE)
            throw error_unimplemented("sorted queries with result limit are not implemented for Oracle");

        if (select_wanted & DBA_DB_FROM_D) {
            if (wanted & DBA_DB_WANT_ANA_ID)
                add_to_orderby("d.id_station");
            if (modifiers & DBA_DB_MODIFIER_SORT_FOR_EXPORT)
                add_to_orderby("d.id_report");
            if (wanted & DBA_DB_WANT_DATETIME)
                add_to_orderby("d.datetime");
            if (!query_station_vars)
            {
                if (wanted & DBA_DB_WANT_LEVEL)
                    add_to_orderby("ltr.ltype1, ltr.l1, ltr.ltype2, ltr.l2");
                if (wanted & DBA_DB_WANT_TIMERANGE)
                    add_to_orderby("ltr.ptype, ltr.p1, ltr.p2");
            }
            if (!(modifiers & DBA_DB_MODIFIER_SORT_FOR_EXPORT) && (wanted & DBA_DB_WANT_REPCOD))
            {
                if (select_wanted & DBA_DB_FROM_RI)
                    add_to_orderby("ri.prio");
                else 
                    add_to_orderby("d.id_report");
            }
        } else if (select_wanted & DBA_DB_FROM_PA) {
            if (wanted & DBA_DB_WANT_ANA_ID)
                add_to_orderby("pa.id");
            if (wanted & DBA_DB_WANT_IDENT)
                add_to_orderby("pa.ident");
        }
    }

    /* Append LIMIT if requested */
    if (limit != -1)
    {
        if (db.conn->server_type == ORACLE)
        {
            sql_query.appendf(" AND rownum <= %d", limit);
        } else {
            sql_query.appendf(" LIMIT %d", limit);
        }
    }
}

void QueryBuilder::build_count_query(const Record& rec)
{
    sql_query.append("SELECT ");

#if 0
    if (cur->modifiers & DBA_DB_MODIFIER_DISTINCT)
        DBA_RUN_OR_RETURN(dba_querybuf_append(sql_query, "COUNT(DISTINCT *) "));
    else
#endif
        sql_query.append("COUNT(*) ");
    stm.bind_out(output_seq++, cur.count);

    /* Prepare WHERE part and see what needs to be available in the FROM part */
    make_where(rec);

    /* Solve dependencies among the various parts of the query */
    resolve_dependencies();

    /* Append the FROM part of the query */
    make_from(rec);

    /* Append the WHERE part that we prepared previously */
    if (!sql_where.empty())
    {
        sql_query.append("WHERE ");
        sql_query.append(sql_where);
    }
}

void QueryBuilder::build_date_extremes_query(const Record& rec)
{
    sql_query.append("SELECT MIN(d.datetime), MAX(d.datetime) ");

    /* Prepare WHERE part and see what needs to be available in the FROM part */
    make_where(rec);

    /* Solve dependencies among the various parts of the query */
    resolve_dependencies();

    /* Append the FROM part of the query */
    make_from(rec);

    /* Append the WHERE part that we prepared previously */
    if (!sql_where.empty())
    {
        sql_query.append("WHERE ");
        sql_query.append(sql_where);
    }
}

void QueryBuilder::make_from(const Record& rec)
{
    /* Ignore anagraphical context unless explicitly requested */
    if (from_wanted & DBA_DB_FROM_D && query_station_vars)
    {
        sql_where.append_list("d.id_lev_tr IS NULL ");
        TRACE("ignoring station info context as it has not been explicitly requested: adding AND d.id_lev_tr IS NULL\n");
    }

    // Make sure we do not include station vars by mistake
    if (from_wanted & DBA_DB_FROM_D && !(from_wanted & DBA_DB_FROM_LTR) && !query_station_vars)
        sql_where.append_list("d.id_lev_tr IS NOT NULL ");

    /* Create the FROM part with everything that is needed */
    if (from_wanted & DBA_DB_FROM_D) {
        sql_query.append(" FROM data d ");
        add_other_froms(DBA_DB_FROM_D, rec);
    } else if (from_wanted & DBA_DB_FROM_PA) {
        sql_query.append(" FROM station pa ");
        add_other_froms(DBA_DB_FROM_PA, rec);
    } else if (from_wanted & DBA_DB_FROM_LTR && !query_station_vars) {
        sql_query.append(" FROM lev_tr ltr ");
        add_other_froms(DBA_DB_FROM_LTR, rec);
    } else if (from_wanted & DBA_DB_FROM_RI) {
        sql_query.append(" FROM repinfo ri ");
        add_other_froms(DBA_DB_FROM_RI, rec);
    }
}

void QueryBuilder::add_to_orderby(const char* fields)
{
    if (!has_orderby)
    {
        sql_query.append(" ORDER BY ");
        has_orderby = true;
    } else
        sql_query.append(", ");
    sql_query.append(fields);
}

void QueryBuilder::add_other_froms(unsigned int base, const Record& rec)
{
    /* Remove the base table from the things to add */
    unsigned int wanted = from_wanted & ~base;

    if (wanted & DBA_DB_FROM_PA)
        switch (base)
        {
            case DBA_DB_FROM_D: sql_query.append("JOIN station pa ON d.id_station = pa.id "); break;
            default: error_consistency::throwf("requested to add a JOIN on station on the unsupported base %d", base);
        }

    if (wanted & DBA_DB_FROM_D)
        switch (base)
        {
            case DBA_DB_FROM_PA: sql_query.append("JOIN data d ON d.id_station=pa.id "); break;
            case DBA_DB_FROM_LTR: sql_query.append("JOIN data d ON ltr.id=d.id_lev_tr "); break;
            case DBA_DB_FROM_RI: sql_query.append("JOIN data d ON d.id_report=ri.id "); break;
            default: error_consistency::throwf("requested to add a JOIN on data on the unsupported base %d", base);
        }

    if ((wanted & DBA_DB_FROM_LTR) && !query_station_vars)
        sql_query.append("JOIN lev_tr ltr ON d.id_lev_tr=ltr.id ");

    if (wanted & DBA_DB_FROM_RI)
        sql_query.append("JOIN repinfo ri ON ri.id=d.id_report ");

    if (wanted & DBA_DB_FROM_DBLO)
        add_station_filter(sql_query, "dblo", "dblo.id_var=257");

    if (wanted & DBA_DB_FROM_DSTA)
        add_station_filter(sql_query, "dsta", "dsta.id_var=258");

    if (wanted & DBA_DB_FROM_DANA)
        add_station_filter(sql_query, "dana");

    if (wanted & DBA_DB_FROM_DDF)
        sql_query.append("JOIN data ddf ON ddf.id_station=d.id_station AND ddf.id_report=d.id_report AND ddf.id_lev_tr=d.id_lev_tr AND ddf.datetime=d.datetime ");

    if (wanted & DBA_DB_FROM_ADF)
        sql_query.append("JOIN attr adf ON adf.id_data=d.id ");
}

void QueryBuilder::add_station_filter(Querybuf& q, const char* name, const char* extra_query)
{
    q.appendf("JOIN data %s ON ", name);
    q.start_list(" AND ");

    if (wanted & DBA_DB_FROM_D)
        q.append_listf("%s.id_station=d.id_station", name);
    else if (wanted & DBA_DB_FROM_PA)
        q.append_listf("%s.id_station=pa.id", name);

    if (wanted & DBA_DB_FROM_D)
        q.append_listf("%s.id_report=d.id_report", name);
    else if (wanted & DBA_DB_FROM_RI)
        q.append_listf("%s.id_report=ri.id", name);

    q.append_listf("%s.id_lev_tr IS NULL", name);
    if (extra_query)
        sql_query.append_list(extra_query);
    q.append(" ");
}

void QueryBuilder::resolve_dependencies()
{
    if (wanted & DBA_DB_WANT_COORDS)
        from_wanted |= DBA_DB_FROM_PA;
    if (wanted & DBA_DB_WANT_IDENT)
        from_wanted |= DBA_DB_FROM_PA;
    if (wanted & DBA_DB_WANT_LEVEL)
        from_wanted |= DBA_DB_FROM_LTR;
    if (wanted & DBA_DB_WANT_TIMERANGE)
        from_wanted |= DBA_DB_FROM_LTR;
    if (wanted & DBA_DB_WANT_DATETIME)
        from_wanted |= DBA_DB_FROM_D;
    if (wanted & DBA_DB_WANT_REPCOD)
        from_wanted |= DBA_DB_FROM_D;
    if (wanted & DBA_DB_WANT_VAR_NAME || wanted & DBA_DB_WANT_VAR_VALUE)
        from_wanted |= DBA_DB_FROM_D;

    /* If querybest is used, then we need ri.prio here so that GROUP BY can use it */
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        from_wanted |= DBA_DB_FROM_RI;
        from_wanted |= DBA_DB_FROM_D;
    }

    /* For these parameters we can try to be opportunistic and avoid extra joins */
    if (wanted & DBA_DB_WANT_ANA_ID)
    {
        if (!(from_wanted & DBA_DB_FROM_PA) && from_wanted & DBA_DB_FROM_D) {
        } else {
            from_wanted |= DBA_DB_FROM_PA;
        }
    }

    if (wanted & DBA_DB_WANT_CONTEXT_ID)
        from_wanted |= DBA_DB_FROM_D;

    /* Enforce join dependencies */
    if (from_wanted & (DBA_DB_FROM_DDF))
        from_wanted |= DBA_DB_FROM_D;
    if (from_wanted & (DBA_DB_FROM_ADF))
        from_wanted |= DBA_DB_FROM_D;
    if (from_wanted & DBA_DB_FROM_PA && from_wanted & DBA_DB_FROM_D)
        from_wanted |= DBA_DB_FROM_D;

    /* Always join with context if we need to weed out the extra ana data */
    if (modifiers & DBA_DB_MODIFIER_NOANAEXTRA)
        from_wanted |= DBA_DB_FROM_D;
}

void QueryBuilder::make_select()
{
    sql_query.start_list(", ");
    if (wanted & DBA_DB_WANT_COORDS)
    {
        from_wanted |= DBA_DB_FROM_PA;
        select_wanted |= DBA_DB_FROM_PA;
        sql_query.append_list("pa.lat");
        stm.bind_out(output_seq++, cur.sqlrec.out_lat);
        sql_query.append_list("pa.lon");
        stm.bind_out(output_seq++, cur.sqlrec.out_lon);
    }
    if (wanted & DBA_DB_WANT_IDENT)
    {
        from_wanted |= DBA_DB_FROM_PA;
        select_wanted |= DBA_DB_FROM_PA;
        sql_query.append_list("pa.ident");
        stm.bind_out(output_seq++, cur.sqlrec.out_ident, sizeof(cur.sqlrec.out_ident), cur.sqlrec.out_ident_ind);
    }
    if (wanted & DBA_DB_WANT_DATETIME)
    {
        from_wanted |= DBA_DB_FROM_D;
        select_wanted |= DBA_DB_FROM_D;
        sql_query.append_list("d.datetime");
        stm.bind_out(output_seq++, cur.sqlrec.out_datetime);
    }
    if (wanted & DBA_DB_WANT_REPCOD)
    {
        from_wanted |= DBA_DB_FROM_D;
        select_wanted |= DBA_DB_FROM_D;
        sql_query.append_list("d.id_report");
        stm.bind_out(output_seq++, cur.sqlrec.out_rep_cod);
    }
    if (wanted & DBA_DB_WANT_VAR_NAME || wanted & DBA_DB_WANT_VAR_VALUE)
    {
        from_wanted |= DBA_DB_FROM_D;
        select_wanted |= DBA_DB_FROM_D;
        sql_query.append_list("d.id_var");
        stm.bind_out(output_seq++, cur.sqlrec.out_varcode);

        if (wanted & DBA_DB_WANT_VAR_VALUE)
        {
            from_wanted |= DBA_DB_FROM_D;
            select_wanted |= DBA_DB_FROM_D;
            sql_query.append_list("d.value");
            stm.bind_out(output_seq++, cur.sqlrec.out_value, sizeof(cur.sqlrec.out_value));
        }
    }

    /* If querybest is used, then we need ri.prio here so that GROUP BY can use it */
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        from_wanted |= DBA_DB_FROM_RI;
        select_wanted |= DBA_DB_FROM_RI;
        sql_query.append_list("ri.prio");
        stm.bind_out(output_seq++, cur.sqlrec.out_priority);
    }

    /* For these parameters we can try to be opportunistic and avoid extra joins */
    if (wanted & DBA_DB_WANT_ANA_ID)
    {
        if (select_wanted & DBA_DB_FROM_PA)
        {
            /* Try pa first */
            sql_query.append_list("pa.id");
            stm.bind_out(output_seq++, cur.sqlrec.out_ana_id);
        } else if (select_wanted & DBA_DB_FROM_D) {
            /* Then c */
            sql_query.append_list("d.id_station");
            stm.bind_out(output_seq++, cur.sqlrec.out_ana_id);
        } else {
            /* If we don't have anything to reuse, get it from pa */
            sql_query.append_list("pa.id");
            stm.bind_out(output_seq++, cur.sqlrec.out_ana_id);
            from_wanted |= DBA_DB_FROM_PA;
            select_wanted |= DBA_DB_FROM_PA;
        }
    }

    if (wanted & DBA_DB_WANT_CONTEXT_ID)
    {
        sql_query.append_list("d.id");
        stm.bind_out(output_seq++, cur.sqlrec.out_id_data);
        from_wanted |= DBA_DB_FROM_D;
        select_wanted |= DBA_DB_FROM_D;
    }
}

void QueryBuilder::make_extra_select()
{
    if ((wanted & DBA_DB_WANT_LEVEL) || (wanted & DBA_DB_WANT_TIMERANGE))
    {
        if (from_wanted & DBA_DB_FROM_D)
        {
            sql_query.append_list("d.id_lev_tr");
            stm.bind_out(output_seq++, cur.sqlrec.out_id_ltr, cur.sqlrec.out_id_ltr_ind);
        } else if (from_wanted & DBA_DB_FROM_LTR) {
            sql_query.append_list("ltr.id");
            stm.bind_out(output_seq++, cur.sqlrec.out_id_ltr, cur.sqlrec.out_id_ltr_ind);
        } else
            cur.sqlrec.out_id_ltr_ind = SQL_NULL_DATA;
    }
}

void QueryBuilder::add_int(const Record& rec, dba_keyword key, const char* sql, int needed_from)
{
    const Var* var = rec.key_peek(key);
    if (!var || !var->isset()) return;
    //TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
    sql_where.append_listf(sql, var->enqi());
    from_wanted |= needed_from;
}

// Normalise longitude values to the [-180..180[ interval
static inline int normalon(int lon)
{
    return ((lon + 18000000) % 36000000) - 18000000;
}

static Varcode parse_varcode(const char* str, regmatch_t pos)
{
    Varcode res;
    /* Parse the varcode */
    if (str[pos.rm_so] == 'B')
        res = WR_STRING_TO_VAR(str + pos.rm_so + 1);
    else
        res = varcode_alias_resolve_substring(str + pos.rm_so, pos.rm_eo - pos.rm_so);

    if (res == 0)
        error_consistency::throwf("cannot resolve the variable code or alias in \"%.*s\"", pos.rm_eo - pos.rm_so, str + pos.rm_so);

    return res;
}

static void parse_value(const char* str, regmatch_t pos, Varinfo info, char* value)
{
    /* Parse the value */
    const char* s = str + pos.rm_so;
    int len = pos.rm_eo - pos.rm_so;
    if (info->is_string())
    {
        /* Copy the string, escaping quotes */
        int i = 0, j = 0;

        value[j++] = '\'';
        for (; i < len && j < 253; ++i, ++j)
        {
            if (s[i] == '\'')
                value[j++] = '\\';
            value[j] = s[i];
        }
        value[j++] = '\'';
        value[j] = 0;
    }
    else
    {
        double dval;
        if (sscanf(s, "%lf", &dval) != 1)
            error_consistency::throwf("value in \"%.*s\" must be a number", len, s);
        Var tmpvar(info, dval);
        strncpy(value, tmpvar.value(), 255);
        value[254] = 0;
    }
}


Varinfo decode_data_filter(const char* filter, const char** op, const char** val, const char** val1)
{
    static regex_t* re_normal = NULL;
    static regex_t* re_between = NULL;
    regmatch_t matches[4];

    static char oper[5];
    static char value[255];
    static char value1[255];
#if 0
    size_t len = strcspn(filter, "<=>");
    const char* s = filter + len;
#endif
    Varcode code;

    /* Compile the regular expression if it has not yet been done */
    if (re_normal == NULL)
    {
        re_normal = new regex_t;
        if (int res = regcomp(re_normal, "^([^<=>]+)([<=>]+)([^<=>]+)$", REG_EXTENDED))
            throw error_regexp(res, re_normal, "compiling regular expression to match normal filters");
    }
    if (re_between == NULL)
    {
        re_between = new regex_t;
        if (int res = regcomp(re_between, "^([^<=>]+)<=([^<=>]+)<=([^<=>]+)$", REG_EXTENDED))
            throw error_regexp(res, re_between, "compiling regular expression to match 'between' filters");
    }

    int res = regexec(re_normal, filter, 4, matches, 0);
    if (res != 0 && res != REG_NOMATCH)
        error_regexp::throwf(res, re_normal, "Trying to parse '%s' as a 'normal' filter", filter);
    if (res == 0)
    {
        int len;
        /* We have a normal filter */

        /* Parse the varcode */
        code = parse_varcode(filter, matches[1]);
        /* Query informations for the varcode */
        Varinfo info = varinfo(code);

        /* Parse the operator */
        len = matches[2].rm_eo - matches[2].rm_so;
        if (len > 4)
            error_consistency::throwf("operator %.*s is not valid", len, filter + matches[2].rm_so);
        memcpy(oper, filter + matches[2].rm_so, len);
        oper[len] = 0;
        if (strcmp(oper, "!=") == 0)
            *op = "<>";
        else if (strcmp(oper, "==") == 0)
            *op = "=";
        else
            *op = oper;

        /* Parse the value */
        parse_value(filter, matches[3], info, value);
        *val = value;
        *val1 = NULL;
        return info;
    }
    else
    {
        res = regexec(re_between, filter, 4, matches, 0);
        if (res == REG_NOMATCH)
            error_consistency::throwf("%s is not a valid filter", filter);
        if (res != 0)
            error_regexp::throwf(res, re_normal, "Trying to parse '%s' as a 'between' filter", filter);

        /* We have a between filter */

        /* Parse the varcode */
        code = parse_varcode(filter, matches[2]);
        /* Query informations for the varcode */
        Varinfo info = varinfo(code);
        /* No need to parse the operator */
        oper[0] = 0;
        *op = oper;
        /* Parse the values */
        parse_value(filter, matches[1], info, value);
        parse_value(filter, matches[3], info, value1);
        *val = value;
        *val1 = value1;
        return info;
    }
}

/*
 * Create the WHERE part of the query
 */
void QueryBuilder::make_where(const Record& rec)
{
    sql_where.start_list(" AND ");

//  fprintf(stderr, "A1 '%s'\n", dba_querybuf_get(sql_where));

    add_int(rec, DBA_KEY_ANA_ID, "pa.id=%d", DBA_DB_FROM_PA);
    add_int(rec, DBA_KEY_LAT, "pa.lat=%d", DBA_DB_FROM_PA);
    add_int(rec, DBA_KEY_LATMIN, "pa.lat>=%d", DBA_DB_FROM_PA);
    add_int(rec, DBA_KEY_LATMAX, "pa.lat<=%d", DBA_DB_FROM_PA);
    //add_int(rec, cur->sel_lonmin, DBA_KEY_LON, "pa.lon=?", DBA_DB_FROM_PA);
    if (const char* val = rec.key_peek_value(DBA_KEY_LON))
    {
        sql_where.append_listf("pa.lon=%d", normalon(strtol(val, 0, 10)));
        from_wanted |= DBA_DB_FROM_PA;
    }
    if (rec.key_peek_value(DBA_KEY_LONMIN) && rec.key_peek_value(DBA_KEY_LONMAX))
    {
        int lonmin = normalon(rec.key(DBA_KEY_LONMIN).enqi());
        int lonmax = normalon(rec.key(DBA_KEY_LONMAX).enqi());
        if (lonmin < lonmax)
        {
            sql_where.append_listf("pa.lon>=%d AND pa.lon<=%d", lonmin, lonmax);
        } else {
            sql_where.append_listf("((pa.lon>=%d AND pa.lon<=18000000) OR (pa.lon>=-18000000 AND pa.lon<=%d))", lonmin, lonmax);
        }
        from_wanted |= DBA_DB_FROM_PA;
    } else if (rec.key_peek_value(DBA_KEY_LONMIN) != NULL) {
        throw error_consistency("'lonmin' query parameter was specified without 'lonmax'");
    } else if (rec.key_peek_value(DBA_KEY_LONMAX) != NULL) {
        throw error_consistency("'lonmax' query parameter was specified without 'lonmin'");
    }

//  fprintf(stderr, "A2 '%s'\n", dba_querybuf_get(sql_where));

    if (const char* val = rec.key_peek_value(DBA_KEY_MOBILE))
    {
        if (val[0] == '0')
        {
            sql_where.append_list("pa.ident IS NULL");
            TRACE("found fixed/mobile: adding AND pa.ident IS NULL.\n");
        } else {
            sql_where.append_list("NOT (pa.ident IS NULL)");
            TRACE("found fixed/mobile: adding AND NOT (pa.ident IS NULL)\n");
        }
        from_wanted |= DBA_DB_FROM_PA;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_IDENT))
    {
        strncpy(sel_ident, val, 64);
        sel_ident[63] = 0;
        sql_where.append_list("pa.ident=?");
        TRACE("found ident: adding AND pa.ident = ?.  val is %s\n", sel_ident);
        stm.bind_in(input_seq++, sel_ident);
        from_wanted |= DBA_DB_FROM_PA;
    }

    /* Set the time extremes */
    {
        int minvalues[6], maxvalues[6];
        rec.parse_date_extremes(minvalues, maxvalues);

        if (minvalues[0] != -1 || maxvalues[0] != -1)
        {
            if (memcmp(minvalues, maxvalues, 6 * sizeof(int)) == 0)
            {
                /* Add constraint on the exact date interval */
                sel_dtmin.year = minvalues[0];
                sel_dtmin.month = minvalues[1];
                sel_dtmin.day = minvalues[2];
                sel_dtmin.hour = minvalues[3];
                sel_dtmin.minute = minvalues[4];
                sel_dtmin.second = minvalues[5];
                sel_dtmin.fraction = 0;
                sql_where.append_listf("d.datetime=?");
                TRACE("found exact time: adding AND d.datetime={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                        minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
                stm.bind_in(input_seq++, sel_dtmin);
                from_wanted |= DBA_DB_FROM_D;

            }
            else
            {
                if (minvalues[0] != -1)
                {
                    /* Add constraint on the minimum date interval */
                    sel_dtmin.year = minvalues[0];
                    sel_dtmin.month = minvalues[1];
                    sel_dtmin.day = minvalues[2];
                    sel_dtmin.hour = minvalues[3];
                    sel_dtmin.minute = minvalues[4];
                    sel_dtmin.second = minvalues[5];
                    sel_dtmin.fraction = 0;
                    sql_where.append_listf("d.datetime>=?");
                    TRACE("found min time: adding AND d.datetime>={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                        minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
                    stm.bind_in(input_seq++, sel_dtmin);
                    from_wanted |= DBA_DB_FROM_D;
                }
                if (maxvalues[0] != -1)
                {
                    sel_dtmax.year = maxvalues[0];
                    sel_dtmax.month = maxvalues[1];
                    sel_dtmax.day = maxvalues[2];
                    sel_dtmax.hour = maxvalues[3];
                    sel_dtmax.minute = maxvalues[4];
                    sel_dtmax.second = maxvalues[5];
                    sel_dtmax.fraction = 0;
                    sql_where.append_listf("d.datetime<=?");
                    TRACE("found max time: adding AND d.datetime<={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                            maxvalues[0], maxvalues[1], maxvalues[2], maxvalues[3], maxvalues[4], maxvalues[5]);
                    stm.bind_in(input_seq++, sel_dtmax);
                    from_wanted |= DBA_DB_FROM_D;
                }
            }
        }

        if (rec.key_peek_value(DBA_KEY_CONTEXT_ID) != NULL ||
            minvalues[0] == 1000 || maxvalues[0] == 1000)
            query_station_vars = true;

        if (modifiers & DBA_DB_MODIFIER_NOANAEXTRA)
            query_station_vars = false;
    }

//  fprintf(stderr, "A3 '%s'\n", dba_querybuf_get(sql_where));

    if (!query_station_vars)
    {
        add_int(rec, DBA_KEY_LEVELTYPE1, "ltr.ltype1=%d", DBA_DB_FROM_LTR);
        add_int(rec, DBA_KEY_L1, "ltr.l1=%d", DBA_DB_FROM_LTR);
        add_int(rec, DBA_KEY_LEVELTYPE2, "ltr.ltype2=%d", DBA_DB_FROM_LTR);
        add_int(rec, DBA_KEY_L2, "ltr.l2=%d", DBA_DB_FROM_LTR);
        add_int(rec, DBA_KEY_PINDICATOR, "ltr.ptype=%d", DBA_DB_FROM_LTR);
        add_int(rec, DBA_KEY_P1, "ltr.p1=%d", DBA_DB_FROM_LTR);
        add_int(rec, DBA_KEY_P2, "ltr.p2=%d", DBA_DB_FROM_LTR);
    }

    add_int(rec, DBA_KEY_CONTEXT_ID, "d.id=%d", DBA_DB_FROM_D);

    /* rep_memo has priority over rep_cod */
    if (const char* val = rec.key_peek_value(DBA_KEY_REP_MEMO))
    {
        int src_val = db.repinfo().get_id(val);
        sql_where.append_listf("d.id_report=%d", src_val);
        TRACE("found rep_memo %s: adding AND d.id_report=%d\n", val, (int)src_val);
        from_wanted |= DBA_DB_FROM_D;
    } else
        add_int(rec, DBA_KEY_REP_COD, "d.id_report=%d", DBA_DB_FROM_D);


    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        sql_where.append_listf("d.id_var=%d", descriptor_code(val));
        TRACE("found b: adding AND d.id_var=%d [from %s]\n", (int)descriptor_code(val), val);
        from_wanted |= DBA_DB_FROM_D;
    }
    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        size_t pos;
        size_t len;
        sql_where.append_list("d.id_var IN (");
        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
        {
            Varcode code = WR_STRING_TO_VAR(val + pos + 1);
            if (pos == 0)
                sql_where.appendf("%d", code);
            else
                sql_where.appendf(",%d", code);
        }
        sql_where.append(")");
        TRACE("found blist: adding AND d.id_var IN (%s)\n", val);
        from_wanted |= DBA_DB_FROM_D;
    }

    add_repinfo_where(sql_where, rec, "ri");

    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 1)))
    {
        sql_where.append_list("dblo.value='");
        sql_where.append(val);
        sql_where.append("'");
        from_wanted |= DBA_DB_FROM_DBLO;
    }
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 2)))
    {
        sql_where.append_list("dsta.value='");
        sql_where.append(val);
        sql_where.append("'");
        from_wanted |= DBA_DB_FROM_DSTA;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_FILTER))
    {
        const char *op, *value, *value1;
        Varinfo info = decode_data_filter(val, &op, &value, &value1);
        sql_where.append_list("dana.id_var=");
        if (value[0] == '\'')
            if (value1 == NULL)
                sql_where.appendf("%d AND dana.value%s%s", info->var, op, value);
            else
                sql_where.appendf("%d AND dana.value BETWEEN %s AND %s", info->var, value, value1);
        else
        {
            const char* type = (db.conn->server_type == MYSQL) ? "SIGNED" : "INT";
            if (value1 == NULL)
                sql_where.appendf("%d AND CAST(dana.value AS %s) %s%s", info->var, type, op, value);
            else
                sql_where.appendf("%d AND CAST(dana.value AS %s) BETWEEN %s AND %s", info->var, type, value, value1);
        }
        from_wanted |= DBA_DB_FROM_DANA;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_DATA_FILTER))
    {
        const char *op, *value, *value1;
        Varinfo info = decode_data_filter(val, &op, &value, &value1);
        sql_where.append_list("ddf.id_var=");
        if (value[0] == '\'')
            if (value1 == NULL)
                sql_where.appendf("%d AND ddf.value%s%s", info->var, op, value);
            else
                sql_where.appendf("%d AND ddf.value BETWEEN %s AND %s", info->var, value, value1);
        else
        {
            const char* type = (db.conn->server_type == MYSQL) ? "SIGNED" : "INT";
            if (value1 == NULL)
                sql_where.appendf("%d AND CAST(ddf.value AS %s)%s%s", info->var, type, op, value);
            else
                sql_where.appendf("%d AND CAST(ddf.value AS %s) BETWEEN %s AND %s", info->var, type, value, value1);
        }
        from_wanted |= DBA_DB_FROM_DDF;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_ATTR_FILTER))
    {
        const char *op, *value, *value1;
        Varinfo info = decode_data_filter(val, &op, &value, &value1);
        sql_where.append_list("adf.type=");
        if (value[0] == '\'')
            if (value1 == NULL)
                sql_where.appendf("%d AND adf.value%s%s", info->var, op, value);
            else
                sql_where.appendf("%d AND adf.value BETWEEN %s AND %s", info->var, value, value1);
        else
        {
            const char* type = (db.conn->server_type == MYSQL) ? "SIGNED" : "INT";
            if (value1 == NULL)
                sql_where.appendf("%d AND CAST(adf.value AS %s)%s%s", info->var, type, op, value);
            else
                sql_where.appendf("%d AND CAST(adf.value AS %s) BETWEEN %s AND %s", info->var, type, value, value1);
        }
        from_wanted |= DBA_DB_FROM_ADF;
    }
}


void QueryBuilder::add_repinfo_where(Querybuf& buf, const Record& rec, const char* colname)
{
    const char* val;
#define ADD_INT(key, sql, needed) do { \
    if ((val = rec.key_peek_value(key)) != NULL) { \
        int ival = strtol(val, 0, 10); \
        /*TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);*/ \
        buf.append_listf(sql, colname, ival); \
        from_wanted |= needed; \
    } } while (0)
    
    ADD_INT(DBA_KEY_PRIORITY, "%s.prio=%d", DBA_DB_FROM_RI);
    ADD_INT(DBA_KEY_PRIOMIN, "%s.prio>=%d", DBA_DB_FROM_RI);
    ADD_INT(DBA_KEY_PRIOMAX, "%s.prio<=%d", DBA_DB_FROM_RI);
#undef ADD_INT
}

#if 0
static dba_err rowcount(dba_db db, const char* table, DBALLE_SQL_C_SINT_TYPE* count)
{
    dba_err err = DBA_OK;
    SQLHSTMT stm = NULL;
    char buf[100];
    int len, res;

    /* Allocate statement handle */
    DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

    /* Bind count directly in the output  */
    SQLBindCol(stm, 1, DBALLE_SQL_C_SINT, count, sizeof(*count), NULL);

    len = snprintf(buf, 100, "SELECT COUNT(*) FROM %s", table);
    res = SQLExecDirect(stm, (unsigned char*)buf, len);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
                "Counting the elements of table %s", table);
        goto cleanup;
    }

    /* Get the result */
    if (SQLFetch(stm) == SQL_NO_DATA)
    {
        err = dba_error_consistency("no results from database when querying row count of table %s", table);
        goto cleanup;
    }

cleanup:
    if (stm != NULL)
        SQLFreeHandle(SQL_HANDLE_STMT, stm);
    return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err setstmtattr(SQLHSTMT stm, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len, const char* context)
{
    int res = SQLSetStmtAttr(stm, attr, val, len);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
        return dba_db_error_odbc(SQL_HANDLE_STMT, stm, context);
    return dba_error_ok();
}
#endif

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
