/*
 * db/v6/qbuilder - build SQL queries for V6 databases
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "qbuilder.h"
#include "dballe/core/defs.h"
#include "dballe/core/aliases.h"
#include "dballe/core/query.h"
#include "dballe/db/sql/repinfo.h"
#include <wreport/var.h>
#include <regex.h>
#include <cstring>
#include <cstdlib>
#include "config.h"
#ifdef HAVE_LIBPQ
#include "dballe/db/postgresql/internals.h"
#endif
#ifdef HAVE_MYSQL
#include "dballe/db/mysql/internals.h"
#endif

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

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

static Varinfo decode_data_filter(const std::string& filter, const char** op, const char** val, const char** val1)
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

    int res = regexec(re_normal, filter.c_str(), 4, matches, 0);
    if (res != 0 && res != REG_NOMATCH)
        error_regexp::throwf(res, re_normal, "Trying to parse '%s' as a 'normal' filter", filter.c_str());
    if (res == 0)
    {
        int len;
        /* We have a normal filter */

        /* Parse the varcode */
        code = parse_varcode(filter.c_str(), matches[1]);
        /* Query informations for the varcode */
        Varinfo info = varinfo(code);

        /* Parse the operator */
        len = matches[2].rm_eo - matches[2].rm_so;
        if (len > 4)
            error_consistency::throwf("operator %.*s is not valid", len, filter.c_str() + matches[2].rm_so);
        memcpy(oper, filter.c_str() + matches[2].rm_so, len);
        oper[len] = 0;
        if (strcmp(oper, "!=") == 0)
            *op = "<>";
        else if (strcmp(oper, "==") == 0)
            *op = "=";
        else
            *op = oper;

        /* Parse the value */
        parse_value(filter.c_str(), matches[3], info, value);
        *val = value;
        *val1 = NULL;
        return info;
    }
    else
    {
        res = regexec(re_between, filter.c_str(), 4, matches, 0);
        if (res == REG_NOMATCH)
            error_consistency::throwf("%s is not a valid filter", filter.c_str());
        if (res != 0)
            error_regexp::throwf(res, re_normal, "Trying to parse '%s' as a 'between' filter", filter.c_str());

        /* We have a between filter */

        /* Parse the varcode */
        code = parse_varcode(filter.c_str(), matches[2]);
        /* Query informations for the varcode */
        Varinfo info = varinfo(code);
        /* No need to parse the operator */
        oper[0] = 0;
        *op = oper;
        /* Parse the values */
        parse_value(filter.c_str(), matches[1], info, value);
        parse_value(filter.c_str(), matches[3], info, value1);
        *val = value;
        *val1 = value1;
        return info;
    }
}


struct Constraints
{
    const Query& query;
    const char* tbl;
    Querybuf& q;
    bool found;

    Constraints(const Query& query, const char* tbl, Querybuf& q)
        : query(query), tbl(tbl), q(q), found(false) {}

#if 0
    void add_int(dba_keyword key, const char* sql)
    {
        const Var* var = rec.key_peek(key);
        if (!var || !var->isset()) return;
        //TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
        q.append_listf(sql, tbl, var->enqi());
        found = true;
    }
#endif

    void add_lat()
    {
        if (query.coords_min.lat == query.coords_max.lat)
        {
            if (query.coords_min.lat == MISSING_INT)
                return;
            q.append_listf("%s.lat=%d", tbl, query.coords_min.lat);
            found = true;
        } else {
            if (query.coords_min.lat != MISSING_INT)
            {
                q.append_listf("%s.lat>=%d", tbl, query.coords_min.lat);
                found = true;
            }
            if (query.coords_max.lat != MISSING_INT)
            {
                q.append_listf("%s.lat<=%d", tbl, query.coords_max.lat);
                found = true;
            }
        }
    }

    void add_lon()
    {
        if (query.coords_min.lon == query.coords_max.lon)
        {
            if (query.coords_min.lon == MISSING_INT)
                return;
            q.append_listf("%s.lon=%d", tbl, query.coords_min.lon);
            found = true;
        } else {
            if (query.coords_min.lon == MISSING_INT)
                throw error_consistency("'lonmin' query parameter was specified without 'lonmax'");
            if (query.coords_max.lon == MISSING_INT)
                throw error_consistency("'lonmax' query parameter was specified without 'lonmin'");

            if (query.coords_min.lon < query.coords_max.lon)
            {
                q.append_listf("%s.lon>=%d AND %s.lon<=%d", tbl, query.coords_min.lon, tbl, query.coords_max.lon);
                found = true;
            } else {
                q.append_listf("((%s.lon>=%d AND %s.lon<=18000000) OR (%s.lon>=-18000000 AND %s.lon<=%d))",
                        tbl, query.coords_min.lon, tbl, tbl, tbl, query.coords_max.lon);
                found = true;
            }
        }
    }

    void add_mobile()
    {
        if (query.mobile != MISSING_INT)
        {
            if (query.mobile == 0)
            {
                q.append_listf("%s.ident IS NULL", tbl);
                TRACE("found fixed/mobile: adding AND %s.ident IS NULL.\n", tbl);
            } else {
                q.append_listf("NOT (%s.ident IS NULL)", tbl);
                TRACE("found fixed/mobile: adding AND NOT (%s.ident IS NULL)\n", tbl);
            }
            found = true;
        }
    }
};

QueryBuilder::QueryBuilder(DB& db, const Query& query, unsigned int modifiers)
    : conn(*db.conn), db(db), query(query), sql_query(2048), sql_from(1024), sql_where(1024),
      modifiers(modifiers), query_station_vars(false)
{
    query_station_vars = query.query_station_vars;
}

DataQueryBuilder::DataQueryBuilder(DB& db, const Query& query, unsigned int modifiers)
    : QueryBuilder(db, query, modifiers)
{
    query_data_id = query.data_id;
}


void QueryBuilder::build()
{
    build_select();

    sql_where.start_list(" AND ");
    bool has_where = build_where();
    if (query.limit != MISSING_INT && conn.server_type == ServerType::ORACLE)
    {
        sql_where.append_listf("rownum <= %d", query.limit);
        has_where = true;
    }

    // Finalise the query
    sql_query.append(sql_from);
    if (has_where)
    {
        sql_query.append(" WHERE ");
        sql_query.append(sql_where);
    }

    // Append ORDER BY as needed
    if (!(modifiers & DBA_DB_MODIFIER_UNSORTED))
    {
        if (query.limit != MISSING_INT && conn.server_type == ServerType::ORACLE)
            throw error_unimplemented("sorted queries with result limit are not implemented for Oracle");

        build_order_by();
    }

    // Append LIMIT if requested
    if (query.limit != MISSING_INT && conn.server_type != ServerType::ORACLE)
        sql_query.appendf(" LIMIT %d", query.limit);
}

void StationQueryBuilder::build_select()
{
    sql_query.append("SELECT s.id, s.lat, s.lon, s.ident");
    sql_from.append(
            " FROM station s"
    );
    select_station = true;
}

bool StationQueryBuilder::build_where()
{
    bool has_where = false;

    // Add pseudoana-specific where parts
    has_where |= add_pa_where("s");

    switch (query.varcodes.size())
    {
        case 0: break;
        case 1:
            sql_where.append_listf("EXISTS(SELECT id FROM data s_stvar"
                                   " WHERE s_stvar.id_station=s.id AND s_stvar.id_lev_tr != -1"
                                   "   AND s_stvar.id_var=%d)", *query.varcodes.begin());
            has_where = true;
            break;
        default:
            sql_where.append_listf("EXISTS(SELECT id FROM data s_stvar"
                                   " WHERE s_stvar.id_station=s.id AND s_stvar.id_lev_tr != -1"
                                   "   AND s_stvar.id_var IN (");
            sql_where.append_varlist(query.varcodes);
            sql_where.append("))");
            has_where = true;
            break;
    }

    return has_where;
}

void StationQueryBuilder::build_order_by()
{
    sql_query.append(" ORDER BY s.id");
}

void DataQueryBuilder::build_select()
{
    sql_query.append("SELECT s.id, s.lat, s.lon, s.ident, d.id_report, d.id_lev_tr, d.id_var, d.id, d.datetime, d.value");
    select_station = true;
    select_varinfo = true;
    select_data_id = true;
    select_data = true;
    sql_from.append(
            " FROM station s"
            " JOIN data d ON s.id=d.id_station"
    );
    if (query_data_id != MISSING_INT)
        sql_from.append(" LEFT OUTER JOIN lev_tr ltr ON ltr.id=d.id_lev_tr");
    else if (!query_station_vars)
        sql_from.append(" JOIN lev_tr ltr ON ltr.id=d.id_lev_tr");
}

bool DataQueryBuilder::build_where()
{
    if (query_data_id != MISSING_INT)
    {
        // Skip arbitrary limits on id_lev_tr if data_id is queried, since we
        // must allow to select either a station or a data value

        //TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
        sql_where.append_listf("d.id=%d", query_data_id);
    } else {
        if (query_station_vars)
            sql_where.append_list("d.id_lev_tr = -1");
        else
            sql_where.append_list("d.id_lev_tr != -1");
    }

    // Add pseudoana-specific where parts
    add_pa_where("s");
    add_dt_where("d");
    add_ltr_where("ltr");
    add_varcode_where("d");
    add_repinfo_where("d");
    add_datafilter_where("d");
    add_attrfilter_where("d");

    return true;
}

void DataQueryBuilder::build_order_by()
{
    sql_query.append(" ORDER BY d.id_station");
    if (modifiers & DBA_DB_MODIFIER_SORT_FOR_EXPORT)
        sql_query.append(", d.id_report");
    sql_query.append(", d.datetime");
    if (!query_station_vars)
        sql_query.append(", ltr.ltype1, ltr.l1, ltr.ltype2, ltr.l2, ltr.ptype, ltr.p1, ltr.p2");
    if (!(modifiers & DBA_DB_MODIFIER_SORT_FOR_EXPORT))
        sql_query.append(", d.id_report");
    sql_query.append(", d.id_var");
}


void IdQueryBuilder::build_select()
{
    sql_query.append("SELECT d.id");
    select_data_id = true;
    sql_from.append(
            " FROM station s"
            " JOIN data d ON s.id = d.id_station"
    );
    if (!query_station_vars)
        sql_from.append(" JOIN lev_tr ltr ON ltr.id = d.id_lev_tr");
}

void IdQueryBuilder::build_order_by()
{
    // No ordering required
}


void SummaryQueryBuilder::build_select()
{
    if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
    {
        sql_query.append(R"(
            SELECT s.id, s.lat, s.lon, s.ident, d.id_report, d.id_lev_tr, d.id_var,
                   COUNT(1), MIN(d.datetime), MAX(d.datetime)
        )");
        select_summary_details = true;
    } else {
        sql_query.append("SELECT DISTINCT s.id, s.lat, s.lon, s.ident, d.id_report, d.id_lev_tr, d.id_var");
    }

    select_station = true;
    select_varinfo = true;
    /*
    // Abuse id_data and datetime for count and min(datetime)
    stm.bind_out(output_seq++, cur.sqlrec.out_id_data);
    stm.bind_out(output_seq++, cur.sqlrec.out_datetime);
    stm.bind_out(output_seq++, cur_s.out_datetime_max);
    */
    sql_from.append(
            " FROM station s"
            " JOIN data d ON s.id = d.id_station"
    );
    if (query_data_id != MISSING_INT)
        sql_from.append(" LEFT OUTER JOIN lev_tr ltr ON ltr.id=d.id_lev_tr");
    else if (!query_station_vars)
        sql_from.append(" JOIN lev_tr ltr ON ltr.id=d.id_lev_tr");
}

void SummaryQueryBuilder::build_order_by()
{
    // No ordering required, but we may add a GROUP BY
    if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
        sql_query.append(" GROUP BY s.id, d.id_report, d.id_lev_tr, d.id_var");
}


bool QueryBuilder::add_pa_where(const char* tbl)
{
    Constraints c(query, tbl, sql_where);
    if (query.ana_id != MISSING_INT)
    {
        sql_where.append_listf("%s.id=%d", tbl, query.ana_id);
        c.found = true;
    }
    c.add_lat();
    c.add_lon();
    c.add_mobile();
    if (query.has_ident)
    {
        if (false) {
            // This is only here to move the other optional bits into else ifs
            // that can be compiled out
            ;
#if HAVE_LIBPQ
        } else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn)) {
            sql_where.append_listf("%s.ident=$1::text", tbl);
            bind_in_ident = query.ident.c_str();
            TRACE("found ident: adding AND %s.ident=$1::text.  val is %s\n", tbl, query.ident());
#endif
#if HAVE_MYSQL
        } else if (MySQLConnection* c = dynamic_cast<MySQLConnection*>(&conn)) {
            string escaped = c->escape(query.ident);
            sql_where.append_listf("%s.ident='%s'", tbl, escaped.c_str());
            TRACE("found ident: adding AND %s.ident='%s'.  val is %s\n", tbl, escape.c_str(), query.ident());
#endif
        } else {
            sql_where.append_listf("%s.ident=?", tbl);
            bind_in_ident = query.ident.c_str();
            TRACE("found ident: adding AND %s.ident = ?.  val is %s\n", tbl, query.ident());
        }
        c.found = true;
    }
    if (query.block != MISSING_INT)
    {
        // No need to escape since the variable is integer
        sql_where.append_listf("EXISTS(SELECT id FROM data %s_blo WHERE %s_blo.id_station=%s.id"
                               " AND %s_blo.id_var=257 AND %s_blo.id_lev_tr = -1 AND %s_blo.value='%d')",
                tbl, tbl, tbl, tbl, tbl, tbl, query.block);
        c.found = true;
    }
    if (query.station != MISSING_INT)
    {
        sql_where.append_listf("EXISTS(SELECT id FROM data %s_sta WHERE %s_sta.id_station=%s.id"
                               " AND %s_sta.id_var=258 AND %s_sta.id_lev_tr = -1 AND %s_sta.value='%d')",
                tbl, tbl, tbl, tbl, tbl, tbl, query.station);
        c.found = true;
    }
    if (!query.ana_filter.empty())
    {
        const char *op, *value, *value1;
        Varinfo info = decode_data_filter(query.ana_filter, &op, &value, &value1);

        sql_where.append_listf("EXISTS(SELECT id FROM data %s_af WHERE %s_af.id_station=%s.id"
                               " AND %s_af.id_lev_tr = -1"
                               " AND %s_af.id_var=%d", tbl, tbl, tbl, tbl, tbl, info->var);

        if (value[0] == '\'')
            if (value1 == NULL)
                sql_where.appendf(" AND %s_af.value%s%s)", tbl, op, value);
            else
                sql_where.appendf(" AND %s_af.value BETWEEN %s AND %s)", tbl, value, value1);
        else
        {
            const char* type = (conn.server_type == ServerType::MYSQL) ? "SIGNED" : "INT";
            if (value1 == NULL)
                sql_where.appendf(" AND CAST(%s_af.value AS %s)%s%s)", tbl, type, op, value);
            else
                sql_where.appendf(" AND CAST(%s_af.value AS %s) BETWEEN %s AND %s)", tbl, type, value, value1);
        }

        c.found = true;
    }

    return c.found;
}

bool QueryBuilder::add_dt_where(const char* tbl)
{
    if (query.query_station_vars) return false;

    bool found = false;
    Datetime dtmin = query.datetime_min.lower_bound();
    Datetime dtmax = query.datetime_max.upper_bound();
    if (!dtmin.is_missing() || !dtmax.is_missing())
    {
        if (dtmin == dtmax)
        {
            // Add constraint on the exact date interval
            sql_where.append_listf("%s.datetime=", tbl);
            conn.add_datetime(sql_where, dtmin);
            TRACE("found exact time: adding AND %s.datetime={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                    tbl, minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
            found = true;
        }
        else
        {
            if (!dtmin.is_missing())
            {
                // Add constraint on the minimum date interval
                sql_where.append_listf("%s.datetime>=", tbl);
                conn.add_datetime(sql_where, dtmin);
                TRACE("found min time: adding AND %s.datetime>={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                    tbl, minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
                found = true;
            }
            if (!dtmax.is_missing())
            {
                sql_where.append_listf("%s.datetime<=", tbl);
                conn.add_datetime(sql_where, dtmax);
                TRACE("found max time: adding AND %s.datetime<={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                        tbl, maxvalues[0], maxvalues[1], maxvalues[2], maxvalues[3], maxvalues[4], maxvalues[5]);
                found = true;
            }
        }
    }

    return found;
}

bool QueryBuilder::add_ltr_where(const char* tbl)
{
    if (query_station_vars) return false;

    bool found = false;
    if (query.level.ltype1 != MISSING_INT)
    {
        sql_where.append_listf("%s.ltype1=%d", tbl, query.level.ltype1);
        found = true;
    }
    if (query.level.l1 != MISSING_INT)
    {
        sql_where.append_listf("%s.l1=%d", tbl, query.level.l1);
        found = true;
    }
    if (query.level.ltype2 != MISSING_INT)
    {
        sql_where.append_listf("%s.ltype2=%d", tbl, query.level.ltype2);
        found = true;
    }
    if (query.level.l2 != MISSING_INT)
    {
        sql_where.append_listf("%s.l2=%d", tbl, query.level.l2);
        found = true;
    }
    if (query.trange.pind != MISSING_INT)
    {
        sql_where.append_listf("%s.ptype=%d", tbl, query.trange.pind);
        found = true;
    }
    if (query.trange.p1 != MISSING_INT)
    {
        sql_where.append_listf("%s.p1=%d", tbl, query.trange.p1);
        found = true;
    }
    if (query.trange.p2 != MISSING_INT)
    {
        sql_where.append_listf("%s.p2=%d", tbl, query.trange.p2);
        found = true;
    }
    return found;
}

bool QueryBuilder::add_varcode_where(const char* tbl)
{
    bool found = false;

    switch (query.varcodes.size())
    {
        case 0: break;
        case 1:
            sql_where.append_listf("%s.id_var=%d", tbl, (int)*query.varcodes.begin());
            TRACE("found b: adding AND %s.id_var=%d\n", tbl, (int)*query.varcodes.begin());
            found = true;
            break;
        case 2:
            sql_where.append_listf("%s.id_var IN (", tbl);
            sql_where.append_varlist(query.varcodes);
            sql_where.append(")");
            TRACE("found blist: adding AND %s.id_var IN (%s)\n", tbl, val);
            found = true;
            break;
    }

    return found;
}

bool QueryBuilder::add_repinfo_where(const char* tbl)
{
    bool found = false;
 
    if (query.prio_min != MISSING_INT || query.prio_max != MISSING_INT)
    {
        // Filter the repinfo cache and build a IN query
        std::vector<int> ids = db.repinfo().ids_by_prio(query);
        if (ids.empty())
        {
            // No repinfo matches, so we just introduce a false value
            sql_where.append_list("1=0");
        } else {
            sql_where.append_listf("%s.id_report IN (", tbl);
            for (std::vector<int>::const_iterator i = ids.begin(); i != ids.end(); ++i)
            {
                if (i == ids.begin())
                    sql_where.appendf("%d", *i);
                else
                    sql_where.appendf(",%d", *i);
            }
            sql_where.append(")");
        }
        found = true;
    }

    if (!query.rep_memo.empty())
    {
        int src_val = db.repinfo().get_id(query.rep_memo.c_str());
        sql_where.append_listf("%s.id_report=%d", tbl, src_val);
        TRACE("found rep_memo %s: adding AND %s.id_report=%d\n", val, tbl, (int)src_val);
        found = true;
    }

    return found;
}

bool QueryBuilder::add_datafilter_where(const char* tbl)
{
    if (query.data_filter.empty()) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(query.data_filter, &op, &value, &value1);

    sql_where.append_listf("%s.id_var=%d", tbl, (int)info->var);

    if (value[0] == '\'')
        if (value1 == NULL)
            sql_where.append_listf("%s.value%s%s", tbl, op, value);
        else
            sql_where.append_listf("%s.value BETWEEN %s AND %s", tbl, value, value1);
    else
    {
        const char* type = (conn.server_type == ServerType::MYSQL) ? "SIGNED" : "INT";
        if (value1 == NULL)
            sql_where.append_listf("CAST(%s.value AS %s)%s%s", tbl, type, op, value);
        else
            sql_where.append_listf("CAST(%s.value AS %s) BETWEEN %s AND %s", tbl, type, value, value1);
    }

    return true;
}

bool QueryBuilder::add_attrfilter_where(const char* tbl)
{
    if (query.attr_filter.empty()) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(query.attr_filter, &op, &value, &value1);

    sql_from.appendf(" JOIN attr %s_atf ON %s.id=%s_atf.id_data AND %s_atf.type=%d", tbl, tbl, tbl, tbl, info->var);
    if (value[0] == '\'')
        if (value1 == NULL)
            sql_where.append_listf("%s_atf.value%s%s", tbl, op, value);
        else
            sql_where.append_listf("%s_atf.value BETWEEN %s AND %s", tbl, value, value1);
    else
    {
        const char* type = (conn.server_type == ServerType::MYSQL) ? "SIGNED" : "INT";
        if (value1 == NULL)
            sql_where.append_listf("CAST(%s_atf.value AS %s)%s%s", tbl, type, op, value);
        else
            sql_where.append_listf("CAST(%s_atf.value AS %s) BETWEEN %s AND %s", tbl, type, value, value1);
    }
    return true;
}

}
}
}
