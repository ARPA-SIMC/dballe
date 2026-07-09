#include "data.h"
#include <cstring>
#include <sstream>

using namespace wreport;

namespace dballe {
namespace core {

Data::~Data() {}

const Data& Data::downcast(const dballe::Data& data)
{
    const Data* ptr = dynamic_cast<const Data*>(&data);
    if (!ptr)
        throw error_consistency("data given is not a core::Data");
    return *ptr;
}

Data& Data::downcast(dballe::Data& data)
{
    Data* ptr = dynamic_cast<Data*>(&data);
    if (!ptr)
        throw error_consistency("data given is not a core::Data");
    return *ptr;
}

void Data::validate() { datetime.set_lower_bound(); }

#if 0
void Record::set_datetime(const Datetime& dt)
{
    if (dt.is_missing())
    {
        datetime = DatetimeRange();
    } else {
        datetime.min = datetime.max = dt;
    }
}

void Record::set_datetimerange(const DatetimeRange& range)
{
    datetime = range;
}

void Record::set_level(const Level& lev)
{
    level = lev;
}

void Record::set_trange(const Trange& tr)
{
    trange = tr;
}

void Record::set_var(const wreport::Var& var)
{
    if (var.isset())
        obtain(var.code()).setval(var);
    else
        unset_var(var.code());
}

void Record::set_var_acquire(std::unique_ptr<wreport::Var>&& var)
{
    int pos = find_item(var->code());
    if (pos == -1)
    {
        // Insertion sort the new variable

        // Enlarge the buffer
        m_vars.resize(m_vars.size() + 1);

        /* Insertionsort.  Crude, but our datasets should be too small for an
         * RB-Tree to be worth it */
        for (pos = m_vars.size() - 1; pos > 0; --pos)
            if (m_vars[pos - 1]->code() > var->code())
                m_vars[pos] = m_vars[pos - 1];
            else
                break;
    } else
        delete m_vars[pos];
    m_vars[pos] = var.release();
}

void Record::set_latrange(const LatRange& lr)
{
    latrange = lr;
}

void Record::set_lonrange(const LonRange& lr)
{
    lonrange = lr;
}
#endif

bool Data::operator==(const dballe::Data& other) const
{
    const auto& o = downcast(other);
    return std::tie(station, datetime, level, trange, values) ==
           std::tie(o.station, o.datetime, o.level, o.trange, o.values);
}

bool Data::operator!=(const dballe::Data& other) const
{
    const auto& o = downcast(other);
    return std::tie(station, datetime, level, trange, values) !=
           std::tie(o.station, o.datetime, o.level, o.trange, o.values);
}

void Data::clear_ids()
{
    station.id = MISSING_INT;
    values.clear_ids();
}

void Data::clear_vars() { values.clear(); }

void Data::clear()
{
    station  = DBStation();
    datetime = Datetime();
    level    = Level();
    trange   = Trange();
    values.clear();
}

#if 0
const std::vector<wreport::Var*>& Record::vars() const
{
    return m_vars;
}

Coords Record::get_coords() const
{
    return station.coords;
}

Ident Record::get_ident() const
{
    return station.ident;
}

Level Record::get_level() const
{
    return level;
}

Trange Record::get_trange() const
{
    return trange;
}

Datetime Record::get_datetime() const
{
    if (datetime.min != datetime.max)
        return Datetime();

    Datetime res = datetime.min;
    res.set_lower_bound();
    return res;
}

DatetimeRange Record::get_datetimerange() const
{
    DatetimeRange res = datetime;
    res.min.set_lower_bound();
    res.max.set_upper_bound();
    return res;
}

Station Record::get_station() const
{
    return station;
}

DBStation Record::get_dbstation() const
{
    return station;
}

const wreport::Var* Record::get_var(wreport::Varcode code) const
{
    int pos = find_item(code);
    if (pos == -1) return NULL;
    return m_vars[pos];
}

void Record::set_coords(const Coords& c)
{
    station.coords = c;
}

void Record::set_station(const Station& s)
{
    station.report = s.report;
    station.coords = s.coords;
    station.ident = s.ident;
    mobile = station.ident.is_missing() ? 0 : 1;
}

void Record::set_dbstation(const DBStation& s)
{
    station = s;
    mobile = station.ident.is_missing() ? 0 : 1;
}
#endif

void Data::set_from_string(const char* str)
{
    // Split the input as name=val
    const char* s = strchr(str, '=');

    if (!s)
        error_consistency::throwf(
            "there should be an = between the name and the value in '%s'", str);

    std::string key(str, s - str);
    setf(key.data(), key.size(), s + 1);
}

void Data::set_from_test_string(const std::string& s)
{
    if (s.empty())
        return;
    size_t cur = 0;
    while (true)
    {
        size_t next = s.find(", ", cur);
        if (next == std::string::npos)
        {
            set_from_string(s.substr(cur).c_str());
            break;
        }
        else
        {
            set_from_string(s.substr(cur, next - cur).c_str());
            cur = next + 2;
        }
    }
    validate();
}

namespace {

struct BufferPrinter
{
    std::stringstream s;
    bool first = true;

    template <typename KEY, typename VAL>
    void print(const KEY& key, const VAL& val)
    {
        if (first)
            first = false;
        else
            s << ",";
        s << key << "=" << val;
    }
};

struct FilePrinter
{
    FILE* out;

    template <typename VAL> void print(const char* key, const VAL& val)
    {
        fprintf(out, "%s=", key);
        val.print(out);
    }

    void print(const char* key, int val) { fprintf(out, "%s=%d\n", key, val); }

    void print(const char* key, const std::string& val)
    {
        fprintf(out, "%s=%s\n", key, val.c_str());
    }
};

} // namespace

std::string Data::to_string() const
{
    BufferPrinter printer;

    if (!station.is_missing())
        printer.print("station", station);
    if (!datetime.is_missing())
        printer.print("datetime", datetime);
    if (!level.is_missing())
        printer.print("level", level);
    if (!trange.is_missing())
        printer.print("trange", trange);
    for (const auto& val : values)
        printer.print(varcode_format(val.code()), val);

    return printer.s.str();
}

void Data::print(FILE* out) const
{
    FilePrinter printer;
    printer.out = out;

    if (!station.is_missing())
        printer.print("station", station);
    if (!datetime.is_missing())
        printer.print("datetime", datetime);
    if (!level.is_missing())
        printer.print("level", level);
    if (!trange.is_missing())
        printer.print("trange", trange);
    for (const auto& var : values)
        var.print(out);
}

} // namespace core
} // namespace dballe
