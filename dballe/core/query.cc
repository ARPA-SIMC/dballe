#include "query.h"
#include "var.h"
#include "json.h"
#include <sstream>
#include <cmath>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace core {

void Query::validate()
{
    lonrange.set(lonrange);
    dtrange.min.set_lower_bound();
    dtrange.max.set_upper_bound();
}

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
    priomin = MISSING_INT;
    priomax = MISSING_INT;
    report.clear();
    mobile = MISSING_INT;
    ident.clear();
    latrange = LatRange();
    lonrange = LonRange();
    dtrange = DatetimeRange();
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

bool Query::empty() const
{
    return (
        ana_id == MISSING_INT
        && priomin == MISSING_INT
        && priomax == MISSING_INT
        && report.empty()
        && mobile == MISSING_INT
        && ident.is_missing()
        && latrange.is_missing()
        && lonrange.is_missing()
        && dtrange.is_missing()
        && level.is_missing()
        && trange.is_missing()
        && varcodes.empty()
        && query.empty()
        && ana_filter.empty()
        && data_filter.empty()
        && attr_filter.empty()
        && limit == MISSING_INT
        && block == MISSING_INT
        && station == MISSING_INT
    );
}

void Query::set_from_string(const char* str)
{
    // Split the input as name=val
    const char* s = strchr(str, '=');

    if (!s) error_consistency::throwf("there should be an = between the name and the value in '%s'", str);

    string key(str, s - str);
    const char* val = s + 1;
    if (strcmp(val, "-") == 0)
        unset(key.data(), key.size());
    else
        setf(key.data(), key.size(), val);
}

void Query::set_from_test_string(const std::string& s)
{
    clear();
    if (s.empty())
        return;

    size_t cur = 0;
    while (true)
    {
        size_t next = s.find(", ", cur);
        if (next == string::npos)
        {
            set_from_string(s.substr(cur).c_str());
            break;
        } else {
            set_from_string(s.substr(cur, next - cur).c_str());
            cur = next + 2;
        }
    }

    validate();
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
    if (!is_subrange(priomin, priomax, other.priomin, other.priomax)) return false;
    if (removed_or_changed(report, other.report)) return false;
    if (removed_or_changed(mobile, other.mobile)) return false;
    if (removed_or_changed(ident, other.ident)) return false;
    if (!other.latrange.contains(latrange)) return false;
    if (!other.lonrange.contains(lonrange)) return false;
    if (!other.dtrange.contains(dtrange)) return false;
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
        if (!(mods & DBA_DB_MODIFIER_BEST) && (omods & DBA_DB_MODIFIER_BEST)) return false;
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
        print_int("priomin", q.priomin);
        print_int("priomax", q.priomax);
        print_str("report", !q.report.empty(), q.report);
        print_int("mobile", q.mobile);
        print_ident(q.ident);
        print_latrange(q.latrange);
        print_lonrange(q.lonrange);
        print_datetimerange(q.dtrange);
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
    if (priomin != MISSING_INT) out.add("prio_min", priomin);
    if (priomax != MISSING_INT) out.add("prio_max", priomax);
    if (!report.empty()) out.add("rep_memo", report);
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
    if (dtrange.min == dtrange.max)
    {
        if (!dtrange.min.is_missing()) out.add("datetime", dtrange.min);
    } else {
        if (!dtrange.min.is_missing()) out.add("datetime_min", dtrange.min);
        if (!dtrange.max.is_missing()) out.add("datetime_max", dtrange.max);
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
            case 5:
                if (strncmp(s, "attrs", 4) == 0)
                    modifiers |= DBA_DB_MODIFIER_WITH_ATTRIBUTES;
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

Query Query::from_json(core::json::Stream& in)
{
    Query res;
    in.parse_object([&](const std::string& key) {
        if (key == "ana_id")
            res.ana_id = in.parse_signed<int>();
        else if (key == "prio_min")
            res.priomin = in.parse_signed<int>();
        else if (key == "prio_max")
            res.priomax = in.parse_signed<int>();
        else if (key == "rep_memo")
            res.report = in.parse_string();
        else if (key == "mobile")
            res.mobile = in.parse_signed<int>();
        else if (key == "ident")
            res.ident = in.parse_string();
        else if (key == "latmin")
            res.latrange.imin = in.parse_signed<int>();
        else if (key == "latmax")
            res.latrange.imax = in.parse_signed<int>();
        else if (key == "lonmin")
            res.lonrange.imin = in.parse_signed<int>();
        else if (key == "lonmax")
            res.lonrange.imax = in.parse_signed<int>();
        else if (key == "datetime")
            res.dtrange.max = res.dtrange.min = in.parse_datetime();
        else if (key == "datetime_min")
            res.dtrange.min = in.parse_datetime();
        else if (key == "datetime_max")
            res.dtrange.max = in.parse_datetime();
        else if (key == "level")
            res.level = in.parse_level();
        else if (key == "trange")
            res.trange = in.parse_trange();
        else if (key == "varcodes")
        {
            in.parse_array([&]{
                res.varcodes.insert(in.parse_unsigned<wreport::Varcode>());
            });
        }
        else if (key == "query")
            res.query = in.parse_string();
        else if (key == "ana_filter")
            res.ana_filter = in.parse_string();
        else if (key == "data_filter")
            res.data_filter = in.parse_string();
        else if (key == "attr_filter")
            res.attr_filter = in.parse_string();
        else if (key == "limit")
            res.limit = in.parse_signed<int>();
        else if (key == "block")
            res.block = in.parse_signed<int>();
        else if (key == "station")
            res.station = in.parse_signed<int>();
    });
    return res;
}

}
}
