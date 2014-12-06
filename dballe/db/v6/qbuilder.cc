/*
 * db/v6/qbuilder - build SQL queries for V6 databases
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

#include "qbuilder.h"
#include "dballe/core/defs.h"
#include "dballe/core/aliases.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/v6/internals.h"
#include <wreport/var.h>
#include <regex.h>
#include <cstring>
#include <cstdlib>

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

static Varinfo decode_data_filter(const char* filter, const char** op, const char** val, const char** val1)
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


struct Constraints
{
    const Record& rec;
    const char* tbl;
    Querybuf& q;
    bool found;

    Constraints(const Record& rec, const char* tbl, Querybuf& q)
        : rec(rec), tbl(tbl), q(q), found(false) {}

    void add_int(dba_keyword key, const char* sql)
    {
        const Var* var = rec.key_peek(key);
        if (!var || !var->isset()) return;
        //TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
        q.append_listf(sql, tbl, var->enqi());
        found = true;
    }

    void add_lat()
    {
        add_int(DBA_KEY_LAT, "%s.lat=%d");
        int latmin = rec.get(DBA_KEY_LATMIN, -9000000);
        if (latmin > -9000000)
        {
            q.append_listf("%s.lat>=%d", tbl, latmin);
            found = true;
        }
        int latmax = rec.get(DBA_KEY_LATMAX, 9000000);
        if (latmax < 9000000)
        {
            q.append_listf("%s.lat<=%d", tbl, latmax);
            found = true;
        }
    }

    void add_lon()
    {
        //add_int(rec, cur->sel_lonmin, DBA_KEY_LON, "pa.lon=?", DBA_DB_FROM_PA);
        if (const char* val = rec.key_peek_value(DBA_KEY_LON))
        {
            q.append_listf("%s.lon=%d", tbl, normalon(strtol(val, 0, 10)));
            found = true;
        }
        if (rec.key_peek_value(DBA_KEY_LONMIN) && rec.key_peek_value(DBA_KEY_LONMAX))
        {
            int lonmin = rec.key(DBA_KEY_LONMIN).enqi();
            int lonmax = rec.key(DBA_KEY_LONMAX).enqi();
            if (lonmin == lonmax)
            {
                q.append_listf("%s.lon=%d", tbl, normalon(lonmin));
                found = true;
            } else {
                lonmin = normalon(lonmin);
                lonmax = normalon(lonmax);
                if (lonmin < lonmax)
                {
                    q.append_listf("%s.lon>=%d AND %s.lon<=%d", tbl, lonmin, tbl, lonmax);
                    found = true;
                } else if (lonmin > lonmax) {
                    q.append_listf("((%s.lon>=%d AND %s.lon<=18000000) OR (%s.lon>=-18000000 AND %s.lon<=%d))",
                            tbl, lonmin, tbl, tbl, tbl, lonmax);
                    found = true;
                }
                // If after being normalised min and max are the same, we
                // assume that one wants "any longitude", as is the case with
                // lonmin=0 lonmax=360 or lonmin=-180 lonmax=180
            }
        } else if (rec.key_peek_value(DBA_KEY_LONMIN) != NULL) {
            throw error_consistency("'lonmin' query parameter was specified without 'lonmax'");
        } else if (rec.key_peek_value(DBA_KEY_LONMAX) != NULL) {
            throw error_consistency("'lonmax' query parameter was specified without 'lonmin'");
        }
    }

    void add_mobile()
    {
        if (const char* val = rec.key_peek_value(DBA_KEY_MOBILE))
        {
            if (val[0] == '0')
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

QueryBuilder::QueryBuilder(DB& db, Statement& stm, const Record& rec, unsigned int modifiers)
    : conn(*db.conn), db(db), stm(stm), rec(rec), sql_query(2048), sql_from(1024), sql_where(1024),
      modifiers(modifiers), query_station_vars(false)
{
    query_station_vars = rec.is_ana_context();
}

DataQueryBuilder::DataQueryBuilder(DB& db, Statement& stm, const Record& rec, unsigned int modifiers)
    : QueryBuilder(db, stm, rec, modifiers)
{
    query_data_id = rec.get(DBA_KEY_CONTEXT_ID, MISSING_INT);
}


void QueryBuilder::build()
{
    int limit = rec.get(DBA_KEY_LIMIT, -1);

    build_select();

    sql_where.start_list(" AND ");
    bool has_where = build_where();
    if (limit != -1 && conn.server_type == ServerType::ORACLE)
    {
        sql_where.append_listf("rownum <= %d", limit);
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
        if (limit != -1 && conn.server_type == ServerType::ORACLE)
            throw error_unimplemented("sorted queries with result limit are not implemented for Oracle");

        build_order_by();
    }

    // Append LIMIT if requested
    if (limit != -1 && conn.server_type != ServerType::ORACLE)
        sql_query.appendf(" LIMIT %d", limit);
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

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        sql_where.append_listf("EXISTS(SELECT id FROM data s_stvar"
                               " WHERE s_stvar.id_station=s.id AND s_stvar.id_lev_tr!=-1"
                               "   AND s_stvar.id_var=%d)",
                               descriptor_code(val));
        has_where = true;
    } else if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST)) {
        sql_where.append_listf("EXISTS(SELECT id FROM data s_stvar"
                               " WHERE s_stvar.id_station=s.id AND s_stvar.id_lev_tr!=-1"
                               "   AND s_stvar.id_var IN (");
        sql_where.append_varlist(val);
        sql_where.append("))");
        has_where = true;
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
    // sql_query.append("SELECT s.id, s.lat, s.lon, s.ident, d.id_report, d.id_lev_tr, d.id_var, COUNT(*), MIN(d.datetime), MAX(d.dat

    sql_query.append("SELECT DISTINCT s.id, s.lat, s.lon, s.ident, d.id_report, d.id_lev_tr, d.id_var");
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
    // No ordering required
    // But we add a GROUP BY
    //sql_query.append(" GROUP BY s.id, d.id_report, d.id_lev_tr, d.id_var");
}


bool QueryBuilder::add_pa_where(const char* tbl)
{
    Constraints c(rec, tbl, sql_where);
    c.add_int(DBA_KEY_ANA_ID, "%s.id=%d");
    c.add_lat();
    c.add_lon();
    c.add_mobile();
    if (const char* val = rec.key_peek_value(DBA_KEY_IDENT))
    {
        sql_where.append_listf("%s.ident=?", tbl);
        TRACE("found ident: adding AND %s.ident = ?.  val is %s\n", tbl, val);
        stm.bind_in(input_seq++, val);
        c.found = true;
    }
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 1)))
    {
        // No need to escape since the variable is integer
        sql_where.append_listf("EXISTS(SELECT id FROM data %s_blo WHERE %s_blo.id_station=%s.id"
                               " AND %s_blo.id_var=257 AND %s_blo.id_lev_tr = -1 AND %s_blo.value='%s')",
                tbl, tbl, tbl, tbl, tbl, tbl, val);
        c.found = true;
    }
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 2)))
    {
        sql_where.append_listf("EXISTS(SELECT id FROM data %s_sta WHERE %s_sta.id_station=%s.id"
                               " AND %s_sta.id_var=258 AND %s_sta.id_lev_tr = -1 AND %s_sta.value='%s')",
                tbl, tbl, tbl, tbl, tbl, tbl, val);
        c.found = true;
    }
    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_FILTER))
    {
        const char *op, *value, *value1;
        Varinfo info = decode_data_filter(val, &op, &value, &value1);

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
    if (rec.is_ana_context()) return false;

    bool found = false;
    int minvalues[6], maxvalues[6];
    rec.parse_date_extremes(minvalues, maxvalues);

    if (minvalues[0] != MISSING_INT || maxvalues[0] != MISSING_INT)
    {
        if (memcmp(minvalues, maxvalues, 6 * sizeof(int)) == 0)
        {
            // Add constraint on the exact date interval
            sql_where.append_listf("%s.datetime=", tbl);
            conn.add_datetime(sql_where, minvalues);
            TRACE("found exact time: adding AND %s.datetime={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                    tbl, minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
            found = true;
        }
        else
        {
            if (minvalues[0] != MISSING_INT)
            {
                // Add constraint on the minimum date interval
                sql_where.append_listf("%s.datetime>=", tbl);
                conn.add_datetime(sql_where, minvalues);
                TRACE("found min time: adding AND %s.datetime>={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                    tbl, minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
                found = true;
            }
            if (maxvalues[0] != MISSING_INT)
            {
                sql_where.append_listf("%s.datetime<=", tbl);
                conn.add_datetime(sql_where, maxvalues);
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

    Constraints c(rec, tbl, sql_where);
    c.add_int(DBA_KEY_LEVELTYPE1, "%s.ltype1=%d");
    c.add_int(DBA_KEY_L1, "%s.l1=%d");
    c.add_int(DBA_KEY_LEVELTYPE2, "%s.ltype2=%d");
    c.add_int(DBA_KEY_L2, "%s.l2=%d");
    c.add_int(DBA_KEY_PINDICATOR, "%s.ptype=%d");
    c.add_int(DBA_KEY_P1, "%s.p1=%d");
    c.add_int(DBA_KEY_P2, "%s.p2=%d");
    return c.found;
}

bool QueryBuilder::add_varcode_where(const char* tbl)
{
    bool found = false;

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        sql_where.append_listf("%s.id_var=%d", tbl, descriptor_code(val));
        TRACE("found b: adding AND %s.id_var=%d [from %s]\n", tbl, (int)descriptor_code(val), val);
        found = true;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        sql_where.append_listf("%s.id_var IN (", tbl);
        sql_where.append_varlist(val);
        sql_where.append(")");
        TRACE("found blist: adding AND %s.id_var IN (%s)\n", tbl, val);
        found = true;
    }

    return found;
}

bool QueryBuilder::add_repinfo_where(const char* tbl)
{
    Constraints c(rec, tbl, sql_where);
 
    if (rec.key_peek(DBA_KEY_PRIORITY) || rec.key_peek(DBA_KEY_PRIOMIN) || rec.key_peek(DBA_KEY_PRIOMAX))
    {
        // Filter the repinfo cache and build a IN query
        std::vector<int> ids = db.repinfo().ids_by_prio(rec);
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
        c.found = true;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_REP_MEMO))
    {
        int src_val = db.repinfo().get_id(val);
        sql_where.append_listf("%s.id_report=%d", tbl, src_val);
        TRACE("found rep_memo %s: adding AND %s.id_report=%d\n", val, tbl, (int)src_val);
        c.found = true;
    }

    return c.found;
}

bool QueryBuilder::add_datafilter_where(const char* tbl)
{
    const char* val = rec.key_peek_value(DBA_KEY_DATA_FILTER);
    if (!val) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(val, &op, &value, &value1);

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
    const char* val = rec.key_peek_value(DBA_KEY_ATTR_FILTER);
    if (!val) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(val, &op, &value, &value1);

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
