#include "qbuilder.h"
#include "config.h"
#include "dballe/core/aliases.h"
#include "dballe/core/defs.h"
#include "dballe/core/query.h"
#include "dballe/core/varmatch.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/sql/sql.h"
#include "dballe/var.h"
#include "transaction.h"
#include <cstdlib>
#include <cstring>
#include <regex.h>
#include <wreport/var.h>
#ifdef HAVE_LIBPQ
#include "dballe/sql/postgresql.h"
#endif
#ifdef HAVE_MYSQL
#include "dballe/sql/mysql.h"
#endif

using namespace std;
using namespace wreport;
using dballe::sql::Querybuf;
using dballe::sql::ServerType;

namespace dballe {
namespace db {
namespace v7 {

static Varcode parse_varcode(const char* str, regmatch_t pos)
{
    Varcode res;
    /* Parse the varcode */
    if (str[pos.rm_so] == 'B')
        res = WR_STRING_TO_VAR(str + pos.rm_so + 1);
    else
        res = varcode_alias_resolve_substring(str + pos.rm_so,
                                              pos.rm_eo - pos.rm_so);

    if (res == 0)
        error_consistency::throwf(
            "cannot resolve the variable code or alias in \"%.*s\"",
            pos.rm_eo - pos.rm_so, str + pos.rm_so);

    return res;
}

static void parse_value(const char* str, regmatch_t pos, Varinfo info,
                        char* value)
{
    /* Parse the value */
    const char* s = str + pos.rm_so;
    int len       = pos.rm_eo - pos.rm_so;
    switch (info->type)
    {
        case Vartype::String: {
            // Copy the string, escaping quotes
            int i = 0, j = 0;

            value[j++] = '\'';
            for (; i < len && j < 253; ++i, ++j)
            {
                if (s[i] == '\'')
                    value[j++] = '\\';
                value[j] = s[i];
            }
            value[j++] = '\'';
            value[j]   = 0;
            break;
        }
        case Vartype::Binary:
            throw error_consistency(
                "cannot use a *_filter on a binary variable");
        case Vartype::Integer:
        case Vartype::Decimal: {
            double dval;
            if (sscanf(s, "%lf", &dval) != 1)
                error_consistency::throwf("value in \"%.*s\" must be a number",
                                          len, s);
            sprintf(value, "%d", info->encode_decimal(dval));
            break;
        }
    }
}

static Varinfo decode_data_filter(const std::string& filter, const char** op,
                                  const char** val, const char** val1)
{
    static regex_t* re_normal  = NULL;
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
        if (int res = regcomp(re_normal, "^([^<=>]+)([<=>]+)([^<=>]+)$",
                              REG_EXTENDED))
            throw error_regexp(
                res, re_normal,
                "compiling regular expression to match normal filters");
    }
    if (re_between == NULL)
    {
        re_between = new regex_t;
        if (int res = regcomp(re_between, "^([^<=>]+)<=([^<=>]+)<=([^<=>]+)$",
                              REG_EXTENDED))
            throw error_regexp(
                res, re_between,
                "compiling regular expression to match 'between' filters");
    }

    int res = regexec(re_normal, filter.c_str(), 4, matches, 0);
    if (res != 0 && res != REG_NOMATCH)
        error_regexp::throwf(res, re_normal,
                             "Trying to parse '%s' as a 'normal' filter",
                             filter.c_str());
    if (res == 0)
    {
        int len;
        /* We have a normal filter */

        /* Parse the varcode */
        code         = parse_varcode(filter.c_str(), matches[1]);
        /* Query informations for the varcode */
        Varinfo info = varinfo(code);

        /* Parse the operator */
        len = matches[2].rm_eo - matches[2].rm_so;
        if (len > 4)
            error_consistency::throwf("operator %.*s is not valid", len,
                                      filter.c_str() + matches[2].rm_so);
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
        *val  = value;
        *val1 = NULL;
        return info;
    }
    else
    {
        res = regexec(re_between, filter.c_str(), 4, matches, 0);
        if (res == REG_NOMATCH)
            error_consistency::throwf("%s is not a valid filter",
                                      filter.c_str());
        if (res != 0)
            error_regexp::throwf(res, re_normal,
                                 "Trying to parse '%s' as a 'between' filter",
                                 filter.c_str());

        /* We have a between filter */

        /* Parse the varcode */
        code         = parse_varcode(filter.c_str(), matches[2]);
        /* Query informations for the varcode */
        Varinfo info = varinfo(code);
        /* No need to parse the operator */
        oper[0]      = 0;
        *op          = oper;
        /* Parse the values */
        parse_value(filter.c_str(), matches[1], info, value);
        parse_value(filter.c_str(), matches[3], info, value1);
        *val  = value;
        *val1 = value1;
        return info;
    }
}

struct Constraints
{
    const core::Query& query;
    const char* tbl;
    Querybuf& q;
    bool found;

    Constraints(const core::Query& query, const char* tbl, Querybuf& q)
        : query(query), tbl(tbl), q(q), found(false)
    {
    }

    void add_lat()
    {
        if (query.latrange.is_missing())
            return;
        if (query.latrange.imin == query.latrange.imax)
            q.append_listf("%s.lat=%d", tbl, query.latrange.imin);
        else
        {
            if (query.latrange.imin != LatRange::IMIN)
                q.append_listf("%s.lat>=%d", tbl, query.latrange.imin);
            if (query.latrange.imax != LatRange::IMAX)
                q.append_listf("%s.lat<=%d", tbl, query.latrange.imax);
        }
        found = true;
    }

    void add_lon()
    {
        if (query.lonrange.is_missing())
            return;

        if (query.lonrange.imin == query.lonrange.imax)
            q.append_listf("%s.lon=%d", tbl, query.lonrange.imin);
        else if (query.lonrange.imin < query.lonrange.imax)
            q.append_listf("%s.lon>=%d AND %s.lon<=%d", tbl,
                           query.lonrange.imin, tbl, query.lonrange.imax);
        else
            q.append_listf("((%s.lon>=%d AND %s.lon<=18000000) OR "
                           "(%s.lon>=-18000000 AND %s.lon<=%d))",
                           tbl, query.lonrange.imin, tbl, tbl, tbl,
                           query.lonrange.imax);
        found = true;
    }

    void add_mobile()
    {
        if (query.mobile != MISSING_INT)
        {
            if (query.mobile == 0)
            {
                q.append_listf("%s.ident IS NULL", tbl);
                TRACE("found fixed/mobile: adding AND %s.ident IS NULL.\n",
                      tbl);
            }
            else
            {
                q.append_listf("NOT (%s.ident IS NULL)", tbl);
                TRACE("found fixed/mobile: adding AND NOT (%s.ident IS NULL)\n",
                      tbl);
            }
            found = true;
        }
    }
};

QueryBuilder::QueryBuilder(std::shared_ptr<v7::Transaction> tr,
                           const core::Query& query, unsigned int modifiers,
                           bool query_station_vars)
    : conn(*tr->db->conn), tr(tr), query(query), sql_query(2048),
      sql_from(1024), sql_where(1024), modifiers(modifiers),
      query_station_vars(query_station_vars)
{
}

DataQueryBuilder::DataQueryBuilder(std::shared_ptr<v7::Transaction> tr,
                                   const core::Query& query,
                                   unsigned int modifiers,
                                   bool query_station_vars)
    : QueryBuilder(tr, query, modifiers, query_station_vars),
      query_attrs(modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES)
{
}

DataQueryBuilder::~DataQueryBuilder() { delete attr_filter; }

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
        if (query.limit != MISSING_INT &&
            conn.server_type == ServerType::ORACLE)
            throw error_unimplemented("sorted queries with result limit are "
                                      "not implemented for Oracle");

        build_order_by();
    }

    // Append LIMIT if requested
    if (query.limit != MISSING_INT && conn.server_type != ServerType::ORACLE)
        sql_query.appendf(" LIMIT %d", query.limit);
}

void StationQueryBuilder::build_select()
{
    sql_query.append("SELECT s.id, s.rep, s.lat, s.lon, s.ident");
    sql_from.append(" FROM station s");
    select_station = true;
}

bool StationQueryBuilder::build_where()
{
    bool has_where = false;

    // Add pseudoana-specific where parts
    has_where |= add_pa_where("s");

    /*
     * Querying var= or varlist= on a station query means querying stations
     * that measure that variable or those variables.
     */
    switch (query.varcodes.size())
    {
        case 0: break;
        case 1:
            sql_where.append_listf("EXISTS(SELECT id FROM data s_stvar"
                                   " WHERE s_stvar.id_station=s.id"
                                   "   AND s_stvar.code=%d)",
                                   *query.varcodes.begin());
            has_where = true;
            break;
        default:
            sql_where.append_listf("EXISTS(SELECT id FROM data s_stvar"
                                   " WHERE s_stvar.id_station=s.id"
                                   "   AND s_stvar.code IN (");
            sql_where.append_varlist(query.varcodes);
            sql_where.append("))");
            has_where = true;
            break;
    }

    if (!query.report.empty())
    {
        int src_val = tr->repinfo().get_id(query.report.c_str());
        if (src_val == -1)
        {
            sql_where.append_listf("1=0");
            TRACE("rep_memo %s not found: adding AND 1=0\n",
                  query.report.c_str());
        }
        else
        {
            sql_where.append_listf("s.rep=%d", src_val);
            TRACE("found rep_memo %s: adding AND s.rep=%d\n",
                  query.report.c_str(), src_val);
        }
        has_where = true;
    }

    return has_where;
}

void StationQueryBuilder::build_order_by()
{
    // https://github.com/ARPA-SIMC/dballe/issues/18
    // confirmed that nothing relies on the ordering of stations in station
    // queries

    // sql_query.append(" ORDER BY s.id");
}

void DataQueryBuilder::build_select()
{
    if (query_station_vars)
        sql_query.append(
            "SELECT s.id, s.rep, s.lat, s.lon, s.ident, d.code, d.id, d.value");
    else
        sql_query.append("SELECT s.id, s.rep, s.lat, s.lon, s.ident, "
                         "d.id_levtr, d.code, d.id, d.datetime, d.value");
    if (query_attrs || !query.attr_filter.empty())
    {
        sql_query.append(", d.attrs");
        select_attrs = true;
        if (!query.attr_filter.empty())
        {
            delete attr_filter;
            attr_filter = Varmatch::parse(query.attr_filter).release();
        }
    }
    select_station = true;
    select_varinfo = true;
    select_data_id = true;
    select_data    = true;
    sql_from.append(" FROM station s");

    if (query_station_vars)
    {
        sql_from.append(" JOIN station_data d ON s.id=d.id_station");
    }
    else
    {
        sql_from.append(" JOIN data d ON s.id=d.id_station");
        sql_from.append(" JOIN levtr ltr ON ltr.id=d.id_levtr");
    }
}

bool DataQueryBuilder::build_where()
{
    bool has_where = false;

    // Add pseudoana-specific where parts
    has_where = add_pa_where("s") || has_where;
    if (!query_station_vars)
    {
        has_where = add_dt_where("d") || has_where;
        has_where = add_ltr_where("ltr") || has_where;
    }
    has_where = add_varcode_where("d") || has_where;
    has_where = add_repinfo_where("s") || has_where;
    has_where = add_datafilter_where("d") || has_where;
    // has_where = add_attrfilter_where("d") || has_where;

    return has_where;
}

bool DataQueryBuilder::match_attrs(const Var& var) const
{
    for (const Var* a = var.next_attr(); a != NULL; a = a->next_attr())
        if ((*attr_filter)(*a))
            return true;
    return false;
}

#if 0
bool DataQueryBuilder::add_attrfilter_where(const char* tbl)
{
    if (query.attr_filter.empty()) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(query.attr_filter, &op, &value, &value1);

    const char* atbl = query_station_vars ? "station_attr" : "attr";

    sql_from.appendf(" JOIN %s %s_atf ON %s.id=%s_atf.id_data AND %s_atf.code=%d", atbl, tbl, tbl, tbl, tbl, info->code);
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
#endif

void DataQueryBuilder::build_order_by()
{
    if (modifiers & DBA_DB_MODIFIER_BEST)
        sql_query.append(" ORDER BY s.lat, s.lon, s.ident");
    else
        sql_query.append(" ORDER BY d.id_station");

    if (!query_station_vars)
    {
        // Query=last falls here, since it is only useable to query querying
        // non-station values
        sql_query.append(", d.datetime");
        sql_query.append(", ltr.ltype1, ltr.l1, ltr.ltype2, ltr.l2, ltr.pind, "
                         "ltr.p1, ltr.p2");
    }
    if (modifiers & (DBA_DB_MODIFIER_BEST | DBA_DB_MODIFIER_LAST))
        sql_query.append(", s.rep");
    sql_query.append(", d.code");
}

void IdQueryBuilder::build_select()
{
    sql_query.append("SELECT d.id");
    if (!query.attr_filter.empty())
    {
        sql_query.append(", d.attrs");
        select_attrs = true;
    }
    select_data_id = true;
    sql_from.append(" FROM station s");
    if (query_station_vars)
        sql_from.append(" JOIN station_data d ON s.id = d.id_station");
    else
    {
        sql_from.append(" JOIN data d ON s.id = d.id_station");
        sql_from.append(" JOIN levtr ltr ON ltr.id = d.id_levtr");
    }
}

void IdQueryBuilder::build_order_by()
{
    // No ordering required
}

void SummaryQueryBuilder::build_select()
{
    if (!query.attr_filter.empty())
        throw error_consistency(
            "attr_filter is not supported on summary queries");

    if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
    {
        if (query_station_vars)
            sql_query.append(
                "SELECT s.id, s.rep, s.lat, s.lon, s.ident, d.code, COUNT(1)");
        else
            sql_query.append(R"(
                SELECT s.id, s.rep, s.lat, s.lon, s.ident, d.id_levtr, d.code,
                       COUNT(1), MIN(d.datetime), MAX(d.datetime)
            )");
        select_summary_details = true;
    }
    else
    {
        if (query_station_vars)
            sql_query.append(
                "SELECT DISTINCT s.id, s.rep, s.lat, s.lon, s.ident, d.code");
        else
            sql_query.append("SELECT DISTINCT s.id, s.rep, s.lat, s.lon, "
                             "s.ident, d.id_levtr, d.code");
    }

    select_station = true;
    select_varinfo = true;
    /*
    // Abuse id_data and datetime for count and min(datetime)
    stm.bind_out(output_seq++, cur.sqlrec.out_id_data);
    stm.bind_out(output_seq++, cur.sqlrec.out_datetime);
    stm.bind_out(output_seq++, cur_s.out_datetime_max);
    */
    sql_from.append(" FROM station s");
    if (query_station_vars)
        sql_from.append(" JOIN station_data d ON s.id = d.id_station");
    else
    {
        sql_from.append(" JOIN data d ON s.id = d.id_station");
        sql_from.append(" JOIN levtr ltr ON ltr.id=d.id_levtr");
    }
}

void SummaryQueryBuilder::build_order_by()
{
    // No ordering required, but we may add a GROUP BY
    if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
    {
        if (query_station_vars)
            sql_query.append(" GROUP BY s.id, d.code");
        else
            sql_query.append(" GROUP BY s.id, d.id_levtr, d.code");
    }
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
    if (!query.ident.is_missing())
    {
        if (false)
        {
            // This is only here to move the other optional bits into else ifs
            // that can be compiled out
            ;
#ifdef HAVE_LIBPQ
        }
        else if (dynamic_cast<dballe::sql::PostgreSQLConnection*>(&conn))
        {
            sql_where.append_listf("%s.ident=$1::text", tbl);
            bind_in_ident = query.ident.get();
            TRACE("found ident: adding AND %s.ident=$1::text.  val is %s\n",
                  tbl, query.ident.get());
#endif
#ifdef HAVE_MYSQL
        }
        else if (dballe::sql::MySQLConnection* c =
                     dynamic_cast<dballe::sql::MySQLConnection*>(&conn))
        {
            string escaped = c->escape(query.ident.get());
            sql_where.append_listf("%s.ident='%s'", tbl, escaped.c_str());
            TRACE("found ident: adding AND %s.ident='%s'.  val is %s\n", tbl,
                  escaped.c_str(), query.ident.get());
#endif
        }
        else
        {
            sql_where.append_listf("%s.ident=?", tbl);
            bind_in_ident = query.ident.get();
            TRACE("found ident: adding AND %s.ident = ?.  val is %s\n", tbl,
                  query.ident.get());
        }
        c.found = true;
    }
    if (query.block != MISSING_INT)
    {
        // No need to escape since the variable is integer
        sql_where.append_listf("EXISTS(SELECT id FROM station_data %s_blo "
                               "WHERE %s_blo.id_station=%s.id"
                               " AND %s_blo.code=257 AND %s_blo.value='%d')",
                               tbl, tbl, tbl, tbl, tbl, query.block);
        c.found = true;
    }
    if (query.station != MISSING_INT)
    {
        sql_where.append_listf("EXISTS(SELECT id FROM station_data %s_sta "
                               "WHERE %s_sta.id_station=%s.id"
                               " AND %s_sta.code=258 AND %s_sta.value='%d')",
                               tbl, tbl, tbl, tbl, tbl, query.station);
        c.found = true;
    }
    if (!query.ana_filter.empty())
    {
        const char *op, *value, *value1;
        Varinfo info =
            decode_data_filter(query.ana_filter, &op, &value, &value1);

        sql_where.append_listf("EXISTS(SELECT id FROM station_data %s_af WHERE "
                               "%s_af.id_station=%s.id"
                               " AND %s_af.code=%d",
                               tbl, tbl, tbl, tbl, info->code);

        if (value[0] == '\'')
            if (value1 == NULL)
                sql_where.appendf(" AND %s_af.value%s%s)", tbl, op, value);
            else
                sql_where.appendf(" AND %s_af.value BETWEEN %s AND %s)", tbl,
                                  value, value1);
        else
        {
            const char* type =
                (conn.server_type == ServerType::MYSQL) ? "SIGNED" : "INT";
            if (value1 == NULL)
                sql_where.appendf(" AND CAST(%s_af.value AS %s)%s%s)", tbl,
                                  type, op, value);
            else
                sql_where.appendf(
                    " AND CAST(%s_af.value AS %s) BETWEEN %s AND %s)", tbl,
                    type, value, value1);
        }

        c.found = true;
    }

    return c.found;
}

bool QueryBuilder::add_dt_where(const char* tbl)
{
    if (query_station_vars)
        return false;

    bool found = false;
    if (!query.dtrange.is_missing())
    {
        Datetime dtmin = query.dtrange.min;
        Datetime dtmax = query.dtrange.max;
        if (dtmin == dtmax)
        {
            // Add constraint on the exact date interval
            sql_where.append_listf("%s.datetime=", tbl);
            conn.add_datetime(sql_where, dtmin);
            TRACE("found exact time: adding AND "
                  "%s.datetime=%04hu-%02hhu-%02hhu%c%02hhu:%02hhu:%02hhu\n",
                  tbl, dtmin.year, dtmin.month, dtmin.day, dtmin.hour,
                  dtmin.minute, dtmin.second);
            found = true;
        }
        else
        {
            if (!dtmin.is_missing())
            {
                // Add constraint on the minimum date interval
                sql_where.append_listf("%s.datetime>=", tbl);
                conn.add_datetime(sql_where, dtmin);
                TRACE(
                    "found min time: adding AND "
                    "%s.datetime>=%04hu-%02hhu-%02hhu%c%02hhu:%02hhu:%02hhu\n",
                    tbl, dtmin.year, dtmin.month, dtmin.day, dtmin.hour,
                    dtmin.minute, dtmin.second);
                found = true;
            }
            if (!dtmax.is_missing())
            {
                sql_where.append_listf("%s.datetime<=", tbl);
                conn.add_datetime(sql_where, dtmax);
                TRACE(
                    "found max time: adding AND "
                    "%s.datetime<=%04hu-%02hhu-%02hhu%c%02hhu:%02hhu:%02hhu\n",
                    tbl, dtmax.year, dtmax.month, dtmax.day, dtmax.hour,
                    dtmax.minute, dtmax.second);
                found = true;
            }
        }
    }

    return found;
}

bool QueryBuilder::add_ltr_where(const char* tbl)
{
    if (query_station_vars)
        return false;

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
        sql_where.append_listf("%s.pind=%d", tbl, query.trange.pind);
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
            sql_where.append_listf("%s.code=%d", tbl,
                                   (int)*query.varcodes.begin());
            TRACE("found b: adding AND %s.code=%d\n", tbl,
                  (int)*query.varcodes.begin());
            found = true;
            break;
        default:
            sql_where.append_listf("%s.code IN (", tbl);
            sql_where.append_varlist(query.varcodes);
            sql_where.append(")");
            TRACE("found blist: adding AND %s.code IN (...%zd items...)\n", tbl,
                  query.varcodes.size());
            found = true;
            break;
    }

    return found;
}

bool QueryBuilder::add_repinfo_where(const char* tbl)
{
    bool found = false;

    if (query.priomin != MISSING_INT || query.priomax != MISSING_INT)
    {
        // Filter the repinfo cache and build a IN query
        std::vector<int> ids = tr->repinfo().ids_by_prio(query);
        if (ids.empty())
        {
            // No repinfo matches, so we just introduce a false value
            sql_where.append_list("1=0");
        }
        else
        {
            sql_where.append_listf("%s.rep IN (", tbl);
            for (std::vector<int>::const_iterator i = ids.begin();
                 i != ids.end(); ++i)
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

    if (!query.report.empty())
    {
        int src_val = tr->repinfo().get_id(query.report.c_str());
        if (src_val == -1)
        {
            sql_where.append_listf("1=0");
            TRACE("rep_memo %s not found: adding AND 1=0\n",
                  query.report.c_str());
        }
        else
        {
            sql_where.append_listf("%s.rep=%d", tbl, src_val);
            TRACE("found rep_memo %s: adding AND %s.rep=%d\n",
                  query.report.c_str(), tbl, (int)src_val);
        }
        found = true;
    }

    return found;
}

bool QueryBuilder::add_datafilter_where(const char* tbl)
{
    if (query.data_filter.empty())
        return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(query.data_filter, &op, &value, &value1);

    sql_where.append_listf("%s.code=%d", tbl, (int)info->code);

    if (value[0] == '\'')
        if (value1 == NULL)
            sql_where.append_listf("%s.value%s%s", tbl, op, value);
        else
            sql_where.append_listf("%s.value BETWEEN %s AND %s", tbl, value,
                                   value1);
    else
    {
        const char* type =
            (conn.server_type == ServerType::MYSQL) ? "SIGNED" : "INT";
        if (value1 == NULL)
            sql_where.append_listf("CAST(%s.value AS %s)%s%s", tbl, type, op,
                                   value);
        else
            sql_where.append_listf("CAST(%s.value AS %s) BETWEEN %s AND %s",
                                   tbl, type, value, value1);
    }

    return true;
}

} // namespace v7
} // namespace db
} // namespace dballe
