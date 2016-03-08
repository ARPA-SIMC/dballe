#include "query.h"
#include "var.h"
#include "json.h"
#include "record.h"
#include <sstream>
#include <cmath>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace core {

std::unique_ptr<dballe::Query> Query::clone() const
{
    return unique_ptr<Query>(new Query(*this));
}

const Query& Query::downcast(const dballe::Query& query)
{
    const Query* ptr = dynamic_cast<const Query*>(&query);
    if (!ptr)
        throw error_consistency("query given is not a core::Query");
    return *ptr;
}

Query& Query::downcast(dballe::Query& query)
{
    Query* ptr = dynamic_cast<Query*>(&query);
    if (!ptr)
        throw error_consistency("query given is not a core::Query");
    return *ptr;
}

unsigned Query::get_modifiers() const
{
    return parse_modifiers(query.c_str());
}

void Query::clear()
{
    ana_id = MISSING_INT;
    prio_min = MISSING_INT;
    prio_max = MISSING_INT;
    rep_memo.clear();
    mobile = MISSING_INT;
    ident.clear();
    latrange = LatRange();
    lonrange = LonRange();
    datetime = DatetimeRange();
    level = Level();
    trange = Trange();
    varcodes.clear();
    query.clear();
    ana_filter.clear();
    data_filter.clear();
    attr_filter.clear();
    limit = MISSING_INT;
    block = MISSING_INT;
    station = MISSING_INT;
}

void Query::set_from_record(const dballe::Record& rec)
{
    const auto& r = core::Record::downcast(rec);

    want_missing = 0;
    // Ana ID
    ana_id = rec.enq("ana_id", MISSING_INT);
    // Priority
    int i = rec.enq("priority", MISSING_INT);
    if (i != MISSING_INT)
        prio_min = prio_max = i;
    else
    {
        prio_min = rec.enq("priomin", MISSING_INT);
        prio_max = rec.enq("priomax", MISSING_INT);
    }
    // Network
    rep_memo = rec.enq("rep_memo", "");
    // Mobile
    mobile = rec.enq("mobile", MISSING_INT);
    // Ident
    if (const Var* var = rec.get("ident"))
        ident = var->enqc();
    else
        ident.clear();
    // Latitude
    i = rec.enq("lat", MISSING_INT);
    if (i != MISSING_INT)
        latrange.set(i, i);
    else
    {
        int imin = rec.enq("latmin", MISSING_INT);
        int imax = rec.enq("latmax", MISSING_INT);
        latrange.set(
                imin == MISSING_INT ? LatRange::IMIN : imin,
                imax == MISSING_INT ? LatRange::IMAX : imax);
    }
    // Longitude
    i = rec.enq("lon", MISSING_INT);
    if (i != MISSING_INT)
        lonrange.set(i, i);
    else
        lonrange.set(rec.enq("lonmin", MISSING_INT), rec.enq("lonmax", MISSING_INT));
    // Datetime
    // fetch all values involved in the computation
    int ye = rec.enq("year", MISSING_INT);
    int mo = rec.enq("month", MISSING_INT);
    int da = rec.enq("day", MISSING_INT);
    int ho = rec.enq("hour", MISSING_INT);
    int mi = rec.enq("min", MISSING_INT);
    int se = rec.enq("sec", MISSING_INT);
    int yemin = rec.enq("yearmin", MISSING_INT);
    int momin = rec.enq("monthmin", MISSING_INT);
    int damin = rec.enq("daymin", MISSING_INT);
    int homin = rec.enq("hourmin", MISSING_INT);
    int mimin = rec.enq("minumin", MISSING_INT);
    int semin = rec.enq("secmin", MISSING_INT);
    int yemax = rec.enq("yearmax", MISSING_INT);
    int momax = rec.enq("monthmax", MISSING_INT);
    int damax = rec.enq("daymax", MISSING_INT);
    int homax = rec.enq("hourmax", MISSING_INT);
    int mimax = rec.enq("minumax", MISSING_INT);
    int semax = rec.enq("secmax", MISSING_INT);
    // give absolute values priority over ranges
    if (ye != MISSING_INT) yemin = yemax = ye;
    if (mo != MISSING_INT) momin = momax = mo;
    if (da != MISSING_INT) damin = damax = da;
    if (ho != MISSING_INT) homin = homax = ho;
    if (mi != MISSING_INT) mimin = mimax = mi;
    if (se != MISSING_INT) semin = semax = se;
    datetime = DatetimeRange(yemin, momin, damin, homin, mimin, semin, yemax, momax, damax, homax, mimax, semax);
    // Level
    level = r.get_level();
    // Trange
    trange = r.get_trange();
    // Varcodes
    varcodes.clear();
    if (const Var* var = rec.get("var"))
        varcodes.insert(resolve_varcode(var->enq("")));
    else if (const Var* var = rec.get("varlist"))
        resolve_varlist(var->enq(""), varcodes);
    // Query
    query = rec.enq("query", "");
    // Filters
    ana_filter = rec.enq("ana_filter", "");
    data_filter = rec.enq("data_filter", "");
    attr_filter = rec.enq("attr_filter", "");
    // Limit
    limit = rec.enq("limit", MISSING_INT);
    // WMO block/station
    block = rec.enq("block", MISSING_INT);
    station = rec.enq("station", MISSING_INT);
}

void Query::set_from_test_string(const std::string& s)
{
    if (s.empty())
        clear();
    else
    {
        core::Record rec;
        rec.set_from_test_string(s);
        set_from_record(rec);
    }
}

namespace {

bool removed_or_changed(int val, int other)
{
    // if (val == other) return false; // Not changed
    // if (val != MISSING_INT && other == MISSING_INT) return false; // Added filter
    // if (val == MISSING_INT && other != MISSING_INT) return true; // Removed filter
    // if (val != other) return true; // Changed
    return !(other == val || other == MISSING_INT);
}

bool removed_or_changed(const Ident& val, const Ident& other)
{
    return !(other == val || other.is_missing());
}

bool removed_or_changed(const std::string& val, const std::string& other)
{
    return !(other == val || other.empty());
}

// Return true if sub is a subset of sup, or the same as sup
template<typename T>
bool is_subset(const std::set<T>& sub, const std::set<T>& sup)
{
    auto isub = sub.begin();
    auto isup = sup.begin();
    while (isub != sub.end() && isup != sup.end())
    {
        // A common item: ok. Skip it and continue.
        if (*isub == *isup)
        {
            ++isub;
            ++isup;
            continue;
        }
        // sub contains an item not in sup: not a subset.
        if (*isub < *isup)
            return false;
        // sup contains an item not in sub: ok. Skip it and continue.
        ++isup;
    }
    // If we have seen all of sub so far, then it is a subset.
    return isub == sub.end();
}

// Return true if sub1--sub2 is contained in sup1--sup2, or is the same
bool is_subrange(int sub1, int sub2, int sup1, int sup2)
{
    // If sup is the whole domain, sub is contained in it
    if (sup1 == MISSING_INT && sup2 == MISSING_INT) return true;
    // If sup is left-open, only check the right bounds
    if (sup1 == MISSING_INT) return sub2 != MISSING_INT && sub2 <= sup2;
    // If sup is right-open, only check the left bounds
    if (sup2 == MISSING_INT) return sub1 != MISSING_INT && sub1 >= sup1;
    // sup is bounded both ways
    if (sub1 == MISSING_INT || sub1 < sup1) return false;
    if (sub2 == MISSING_INT || sub2 > sup2) return false;
    return true;
}

}

bool Query::is_subquery(const dballe::Query& other_gen) const
{
    const Query& other = downcast(other_gen);

    if (removed_or_changed(ana_id, other.ana_id)) return false;
    if (!is_subrange(prio_min, prio_max, other.prio_min, other.prio_max)) return false;
    if (removed_or_changed(rep_memo, other.rep_memo)) return false;
    if (removed_or_changed(mobile, other.mobile)) return false;
    if (removed_or_changed(ident, other.ident)) return false;
    if (!other.latrange.contains(latrange)) return false;
    if (!other.lonrange.contains(lonrange)) return false;
    if (!other.datetime.contains(datetime)) return false;
    if (removed_or_changed(level.ltype1, other.level.ltype1)) return false;
    if (removed_or_changed(level.l1, other.level.l1)) return false;
    if (removed_or_changed(level.ltype2, other.level.ltype2)) return false;
    if (removed_or_changed(level.l2, other.level.l2)) return false;
    if (removed_or_changed(trange.pind, other.trange.pind)) return false;
    if (removed_or_changed(trange.p1, other.trange.p1)) return false;
    if (removed_or_changed(trange.p2, other.trange.p2)) return false;
    // If other.varcodes is a subset, of varcodes, then we just added new
    // varcodes to the filter
    if (!is_subset(other.varcodes, varcodes)) return false;
    // Parse query and check its components
    unsigned mods = parse_modifiers(query.c_str());
    unsigned omods = parse_modifiers(other.query.c_str());
    if (mods != omods)
    {
        // The only relevant bits is query=best, all the rest we can safely ignore
        if ((mods & DBA_DB_MODIFIER_BEST) != (omods & DBA_DB_MODIFIER_BEST)) return false;
    }
    if (removed_or_changed(ana_filter, other.ana_filter)) return false;
    if (removed_or_changed(data_filter, other.data_filter)) return false;
    if (removed_or_changed(attr_filter, other.attr_filter)) return false;
    // We tolerate limit being changed as long as it has only been reduced
    if (other.limit != MISSING_INT && (limit == MISSING_INT || limit > other.limit)) return false;
    if (removed_or_changed(block, other.block)) return false;
    if (removed_or_changed(station, other.station)) return false;
    return true;
}

namespace {

struct VarGen
{
    std::function<void(const char*, Var&&)>& dest;

    VarGen(std::function<void(const char*, Var&&)>& dest)
        : dest(dest) {}

    template<typename T>
    void gen(dba_keyword key, const T& val)
    {
        Varinfo info = Record::keyword_info(key);
        dest(Record::keyword_name(key), Var(info, val));
    };

    void gen(dba_keyword key, const std::string& val)
    {
        Varinfo info = Record::keyword_info(key);
        dest(Record::keyword_name(key), Var(info, val.c_str()));
    };

    void gen(dba_keyword key, const Ident& val)
    {
        Varinfo info = Record::keyword_info(key);
        dest(Record::keyword_name(key), Var(info, val.get()));
    };
};

}

void Query::foreach_key(std::function<void(const char*, Var&&)> dest) const
{
    VarGen vargen(dest);

    if (ana_id != MISSING_INT) vargen.gen(DBA_KEY_ANA_ID, ana_id);
    if (prio_min != prio_max)
    {
        if (prio_min != MISSING_INT)
            vargen.gen(DBA_KEY_PRIORITY, prio_min);
    } else {
        if (prio_min != MISSING_INT)
            vargen.gen(DBA_KEY_PRIOMIN, prio_min);
        if (prio_max != MISSING_INT)
            vargen.gen(DBA_KEY_PRIOMAX, prio_max);
    }
    if (!rep_memo.empty())
        vargen.gen(DBA_KEY_REP_MEMO, rep_memo);
    if (mobile != MISSING_INT)
        vargen.gen(DBA_KEY_MOBILE, mobile);
    if (!ident.is_missing())
        vargen.gen(DBA_KEY_IDENT, ident);
    if (!latrange.is_missing())
    {
        if (latrange.imin == latrange.imax)
            vargen.gen(DBA_KEY_LAT, latrange.imin);
        else
        {
            if (latrange.imin != LatRange::IMIN) vargen.gen(DBA_KEY_LATMIN, latrange.imin);
            if (latrange.imax != LatRange::IMAX) vargen.gen(DBA_KEY_LATMAX, latrange.imax);
        }
    }
    if (!lonrange.is_missing())
    {
        if (lonrange.imin == lonrange.imax)
            vargen.gen(DBA_KEY_LON, lonrange.imin);
        else
        {
            vargen.gen(DBA_KEY_LONMIN, lonrange.imin);
            vargen.gen(DBA_KEY_LONMAX, lonrange.imax);
        }
    }
    if (datetime.min == datetime.max)
    {
        if (!datetime.min.is_missing())
        {
            vargen.gen(DBA_KEY_YEAR, datetime.min.year);
            vargen.gen(DBA_KEY_MONTH, datetime.min.month);
            vargen.gen(DBA_KEY_DAY, datetime.min.day);
            vargen.gen(DBA_KEY_HOUR, datetime.min.hour);
            vargen.gen(DBA_KEY_MIN, datetime.min.minute);
            vargen.gen(DBA_KEY_SEC, datetime.min.second);
        }
    } else {
        if (!datetime.min.is_missing())
        {
            vargen.gen(DBA_KEY_YEARMIN, datetime.min.year);
            vargen.gen(DBA_KEY_MONTHMIN, datetime.min.month);
            vargen.gen(DBA_KEY_DAYMIN, datetime.min.day);
            vargen.gen(DBA_KEY_HOURMIN, datetime.min.hour);
            vargen.gen(DBA_KEY_MINUMIN, datetime.min.minute);
            vargen.gen(DBA_KEY_SECMIN, datetime.min.second);
        }
        if (!datetime.max.is_missing())
        {
            vargen.gen(DBA_KEY_YEARMAX, datetime.max.year);
            vargen.gen(DBA_KEY_MONTHMAX, datetime.max.month);
            vargen.gen(DBA_KEY_DAYMAX, datetime.max.day);
            vargen.gen(DBA_KEY_HOURMAX, datetime.max.hour);
            vargen.gen(DBA_KEY_MINUMAX, datetime.max.minute);
            vargen.gen(DBA_KEY_SECMAX, datetime.max.second);
        }
    }
    if (level.ltype1 != MISSING_INT) vargen.gen(DBA_KEY_LEVELTYPE1, level.ltype1);
    if (level.l1 != MISSING_INT) vargen.gen(DBA_KEY_L1, level.l1);
    if (level.ltype2 != MISSING_INT) vargen.gen(DBA_KEY_LEVELTYPE2, level.ltype2);
    if (level.l2 != MISSING_INT) vargen.gen(DBA_KEY_L2, level.l2);
    if (trange.pind != MISSING_INT) vargen.gen(DBA_KEY_PINDICATOR, trange.pind);
    if (trange.p1 != MISSING_INT) vargen.gen(DBA_KEY_P1, trange.p1);
    if (trange.p2 != MISSING_INT) vargen.gen(DBA_KEY_P2, trange.p2);
    switch (varcodes.size())
    {
         case 0:
             break;
         case 1:
             vargen.gen(DBA_KEY_VAR, varcode_format(*varcodes.begin()));
             break;
         default: {
             string codes;
             for (const auto& code: varcodes)
             {
                 if (codes.empty())
                     codes = varcode_format(code);
                 else
                 {
                     codes += ",";
                     codes += varcode_format(code);
                 }
             }
             vargen.gen(DBA_KEY_VARLIST, codes);
             break;
         }
    }
    if (!query.empty()) vargen.gen(DBA_KEY_QUERY, query);
    if (!ana_filter.empty()) vargen.gen(DBA_KEY_ANA_FILTER, ana_filter);
    if (!data_filter.empty()) vargen.gen(DBA_KEY_DATA_FILTER, data_filter);
    if (!attr_filter.empty()) vargen.gen(DBA_KEY_ATTR_FILTER, attr_filter);
    if (limit != MISSING_INT) vargen.gen(DBA_KEY_LIMIT, limit);
    if (block != MISSING_INT) dest("block", Var(varinfo(WR_VAR(0, 1, 1)), block));
    if (station != MISSING_INT) dest("station", Var(varinfo(WR_VAR(0, 1, 2)), station));
}

namespace {

struct Printer
{
    const Query& q;
    FILE* out;
    bool first = true;

    Printer(const Query& q, FILE* out) : q(q), out(out) {}

    void print_int(const char* name, int val)
    {
        if (val == MISSING_INT) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=%d", name, val);
        first = false;
    }

    void print_str(const char* name, bool is_present, const std::string& val)
    {
        if (!is_present) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=%s", name, val.c_str());
        first = false;
    }

    void print_ident(const Ident& val)
    {
        if (val.is_missing()) return;
        if (!first) fputs(", ", out);
        fprintf(out, "ident=%s", val.get());
        first = false;
    }

    void print_latrange(const LatRange& latrange)
    {
        if (latrange.is_missing()) return;
        double dmin, dmax;
        latrange.get(dmin, dmax);
        if (dmin != LatRange::DMIN)
        {
            if (!first) fputs(", ", out);
            fprintf(out, "latmin=%.5f", dmin);
            first = false;
        }
        if (dmax != LatRange::DMAX)
        {
            if (!first) fputs(", ", out);
            fprintf(out, "latmax=%.5f", dmax);
            first = false;
        }
    }

    void print_lonrange(const LonRange& lonrange)
    {
        if (lonrange.is_missing()) return;
        double dmin, dmax;
        lonrange.get(dmin, dmax);
        if (!first) fputs(", ", out);
        fprintf(out, "lonmin=%.5f, lonmax=%.5f", dmin, dmax);
        first = false;
    }

    void print_datetimerange(const DatetimeRange& dtr)
    {
        if (dtr.is_missing()) return;
        if (!first) fputs(", ", out);
        if (dtr.min == dtr.max)
            fprintf(out, "datetime=%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                    dtr.max.year, dtr.max.month, dtr.max.day,
                    dtr.max.hour, dtr.max.minute, dtr.max.second);
        else if (dtr.min.is_missing())
            fprintf(out, "datetime<=%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                    dtr.max.year, dtr.max.month, dtr.max.day,
                    dtr.max.hour, dtr.max.minute, dtr.max.second);
        else if (dtr.max.is_missing())
            fprintf(out, "datetime>=%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                    dtr.min.year, dtr.min.month, dtr.min.day,
                    dtr.min.hour, dtr.min.minute, dtr.min.second);
        else
            fprintf(out, "datetime=%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu to %04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                    dtr.min.year, dtr.min.month, dtr.min.day,
                    dtr.min.hour, dtr.min.minute, dtr.min.second,
                    dtr.max.year, dtr.max.month, dtr.max.day,
                    dtr.max.hour, dtr.max.minute, dtr.max.second);
        first = false;
    }

    void print_level(const char* name, const Level& l)
    {
        if (l == Level()) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=", name);
        l.print(out, "-", "");
        first = false;
    }

    void print_trange(const char* name, const Trange& t)
    {
        if (t == Trange()) return;
        if (!first) fputs(", ", out);
        fprintf(out, "%s=", name);
        t.print(out, "-", "");
        first = false;
    }

    void print_varcodes(const char* name, const set<Varcode>& varcodes)
    {
        if (varcodes.empty()) return;
        if (!first) fputs(", ", out);
        fputs(name, out);
        putc('=', out);
        bool lfirst = true;
        for (const auto& v : varcodes)
        {
            if (!lfirst) fputs(",", out);
            fprintf(out, "%01d%02d%03d", WR_VAR_F(v), WR_VAR_X(v), WR_VAR_Y(v));
            lfirst = false;
        }
        first = false;
    }

    void print()
    {
        print_int("ana_id", q.ana_id);
        print_int("prio_min", q.prio_min);
        print_int("prio_max", q.prio_max);
        print_str("rep_memo", !q.rep_memo.empty(), q.rep_memo);
        print_int("mobile", q.mobile);
        print_ident(q.ident);
        print_latrange(q.latrange);
        print_lonrange(q.lonrange);
        print_datetimerange(q.datetime);
        print_level("level", q.level);
        print_trange("trange", q.trange);
        print_varcodes("varcodes", q.varcodes);
        print_str("query", !q.query.empty(), q.query);
        print_str("ana_filter", !q.ana_filter.empty(), q.ana_filter);
        print_str("data_filter", !q.data_filter.empty(), q.data_filter);
        print_str("attr_filter", !q.attr_filter.empty(), q.attr_filter);
        print_int("limit", q.limit);
        print_int("block", q.block);
        print_int("station", q.station);
        putc('\n', out);
    }
};

}

void Query::print(FILE* out) const
{
    Printer printer(*this, out);
    printer.print();
}

void Query::serialize(JSONWriter& out) const
{
    if (ana_id != MISSING_INT) out.add("ana_id", ana_id);
    if (prio_min != MISSING_INT) out.add("prio_min", prio_min);
    if (prio_max != MISSING_INT) out.add("prio_max", prio_max);
    if (!rep_memo.empty()) out.add("rep_memo", rep_memo);
    if (mobile != MISSING_INT) out.add("mobile", mobile);
    if (!ident.is_missing()) out.add("ident", ident.get());
    if (!latrange.is_missing())
    {
        if (latrange.imin != LatRange::IMIN) out.add("latmin", latrange.imin);
        if (latrange.imax != LatRange::IMAX) out.add("latmax", latrange.imax);
    }
    if (!lonrange.is_missing())
    {
        out.add("lonmin", lonrange.imin);
        out.add("lonmax", lonrange.imax);
    }
    if (datetime.min == datetime.max)
    {
        if (!datetime.min.is_missing()) out.add("datetime", datetime.min);
    } else {
        if (!datetime.min.is_missing()) out.add("datetime_min", datetime.min);
        if (!datetime.max.is_missing()) out.add("datetime_max", datetime.max);
    }
    if (!level.is_missing()) out.add("level", level);
    if (!trange.is_missing()) out.add("trange", trange);
    if (!varcodes.empty())
    {
        out.add("varcodes");
        out.add_list(varcodes);
    }
    if (!query.empty()) out.add("query", query);
    if (!ana_filter.empty()) out.add("ana_filter", ana_filter);
    if (!data_filter.empty()) out.add("data_filter", data_filter);
    if (!attr_filter.empty()) out.add("attr_filter", attr_filter);
    if (limit != MISSING_INT) out.add("limit", limit);
    if (block != MISSING_INT) out.add("block", block);
    if (station != MISSING_INT) out.add("station", station);
}

unsigned Query::parse_modifiers(const dballe::Record& rec)
{
    /* Decode query modifiers */
    const Var* var = rec.get("query");
    if (!var) return 0;
    if (!var->isset()) return 0;
    return parse_modifiers(var->enqc());
}

unsigned Query::parse_modifiers(const char* s)
{
    unsigned modifiers = 0;
    while (*s)
    {
        size_t len = strcspn(s, ",");
        int got = 1;
        switch (len)
        {
            case 0:
                /* If it's an empty token, skip it */
                break;
            case 4:
                /* "best": if more values exist in a point, get only the
                   best one */
                if (strncmp(s, "best", 4) == 0)
                    modifiers |= DBA_DB_MODIFIER_BEST;
                else
                    got = 0;
                break;
            case 6:
                /* "bigana": optimize with date first */
                if (strncmp(s, "bigana", 6) == 0)
                    ; // Not used anymore
                else if (strncmp(s, "nosort", 6) == 0)
                    modifiers |= DBA_DB_MODIFIER_UNSORTED;
                else if (strncmp(s, "stream", 6) == 0)
                    ; // Not used anymore
                else
                    got = 0;
                break;
            case 7:
                if (strncmp(s, "details", 7) == 0)
                    modifiers |= DBA_DB_MODIFIER_SUMMARY_DETAILS;
                else
                    got = 0;
                break;
            default:
                got = 0;
                break;
        }

        /* Check that we parsed it correctly */
        if (!got)
            error_consistency::throwf("Query modifier \"%.*s\" is not recognized", (int)len, s);

        /* Move to the next token */
        s += len;
        if (*s == ',')
            ++s;
    }

    return modifiers;
}

}
}
