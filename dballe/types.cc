#include "types.h"
#include "core/csv.h"
#include <wreport/error.h>
#include <ostream>
#include <iomanip>
#include <cmath>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {

/*
 * Date
 */

Date::Date()
    : year(0xffff), month(0xff), day(0xff)
{
}

Date::Date(int ye, int mo, int da)
{
    if (ye == MISSING_INT)
    {
        year = 0xffff;
        month = day = 0xff;
    } else {
        Date::validate(ye, mo, da);
        year = ye;
        month = mo;
        day = da;
    }
}

void Date::validate(int ye, int mo, int da)
{
    if (mo == MISSING_INT || mo < 1 || mo > 12)
        error_consistency::throwf("month %d is not between 1 and 12", mo);
    if (da == MISSING_INT || da < 1 || da > days_in_month(ye, mo))
        error_consistency::throwf("day %d is not between 1 and %d", da, days_in_month(ye, mo));
}

bool Date::is_missing() const { return year == 0xffff; }

int Date::compare(const Date& o) const
{
    if (int res = year - o.year) return res;
    if (int res = month - o.month) return res;
    return day - o.day;
}

int Date::to_julian() const
{
    return calendar_to_julian(year, month, day);
}

Date Date::from_julian(int jday)
{
    Date res;
    julian_to_calendar(jday, res.year, res.month, res.day);
    return res;
}

int Date::calendar_to_julian(int year, int month, int day)
{
    // From http://libpqtypes.esilo.com/browse_source.html?file=datetime.c
    if (month > 2)
    {
        month += 1;
        year += 4800;
    }
    else
    {
        month += 13;
        year += 4799;
    }

    int century = year / 100;
    int julian = year * 365 - 32167;
    julian += year / 4 - century + century / 4;
    julian += 7834 * month / 256 + day;

    return julian;
}

void Date::julian_to_calendar(int jday, unsigned short& year, unsigned char& month, unsigned char& day)
{
    // From http://libpqtypes.esilo.com/browse_source.html?file=datetime.c
    unsigned julian = jday + 32044;
    unsigned quad = julian / 146097;
    unsigned extra = (julian - quad * 146097) * 4 + 3;
    julian += 60 + quad * 3 + extra / 146097;
    quad = julian / 1461;
    julian -= quad * 1461;
    int y = julian * 4 / 1461;
    julian = ((y != 0) ? ((julian + 305) % 365) : ((julian + 306) % 366))
        + 123;
    y += quad * 4;
    year = y - 4800;
    quad = julian * 2141 / 65536;
    day = julian - 7834 * quad / 256;
    month = (quad + 10) % 12 + 1;
}

bool Date::operator==(const Date& dt) const
{
    return year == dt.year && month == dt.month && day == dt.day;
}

bool Date::operator!=(const Date& dt) const
{
    return year != dt.year || month != dt.month || day != dt.day;
}

bool Date::operator<(const Date& dt) const
{
    if (year < dt.year) return true;
    if (year > dt.year) return false;
    if (month < dt.month) return true;
    if (month > dt.month) return false;
    return day < dt.day;
}

bool Date::operator>(const Date& dt) const
{
    if (year < dt.year) return false;
    if (year > dt.year) return true;
    if (month < dt.month) return false;
    if (month > dt.month) return true;
    return day > dt.day;
}

int Date::days_in_month(int year, int month)
{
    switch (month)
    {
        case  1: return 31;
        case  2:
            if (year % 400 == 0 || (year % 4 == 0 && ! (year % 100 == 0)))
                return 29;
            return 28;
        case  3: return 31;
        case  4: return 30;
        case  5: return 31;
        case  6: return 30;
        case  7: return 31;
        case  8: return 31;
        case  9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default:
            error_consistency::throwf("Month %d is not between 1 and 12", month);
    }
}

void Date::to_stream_iso8601(std::ostream& out) const
{
    out <<        setw(4) << setfill('0') << year
        << '-' << setw(2) << setfill('0') << (unsigned)month
        << '-' << setw(2) << setfill('0') << (unsigned)day;
}

void Date::to_csv_iso8601(CSVWriter& out) const
{
    if (is_missing())
        out.add_value_empty();
    else
    {
        char buf[16];
        snprintf(buf, 16, "%04hu-%02hhu-%02hhu", year, month, day);
        out.add_value(buf);
    }
}

std::ostream& operator<<(std::ostream& out, const Date& dt)
{
    dt.to_stream_iso8601(out);
    return out;
}


/*
 * Time
 */

Time::Time()
    : hour(0xff), minute(0xff), second(0xff)
{
}

Time::Time(int ho, int mi, int se)
{
    Time::validate(ho, mi, se);
    hour = ho;
    minute = mi;
    second = se;
}

void Time::validate(int ho, int mi, int se)
{
    if (ho == MISSING_INT || ho < 0 || ho > 23)
        error_consistency::throwf("hour %d is not between 1 and 23", ho);
    if (mi == MISSING_INT || mi < 0 || mi > 59)
        error_consistency::throwf("minute %d is not between 1 and 59", mi);
    if (ho == 23 && mi == 59)
    {
        if (se == MISSING_INT || se < 0 || se > 60)
            error_consistency::throwf("second %d is not between 1 and 60", se);
    } else {
        if (se == MISSING_INT || se < 0 || se > 59)
            error_consistency::throwf("second %d is not between 1 and 59", se);
    }
}

bool Time::is_missing() const { return hour == 0xff; }

int Time::compare(const Time& o) const
{
    if (int res = hour - o.hour) return res;
    if (int res = minute - o.minute) return res;
    return second - o.second;
}

bool Time::operator==(const Time& dt) const
{
    return hour == dt.hour && minute == dt.minute && second == dt.second;
}

bool Time::operator!=(const Time& dt) const
{
    return hour != dt.hour || minute != dt.minute || second != dt.second;
}

bool Time::operator<(const Time& dt) const
{
    if (hour < dt.hour) return true;
    if (hour > dt.hour) return false;
    if (minute < dt.minute) return true;
    if (minute > dt.minute) return false;
    return second < dt.second;
}

bool Time::operator>(const Time& dt) const
{
    if (hour < dt.hour) return false;
    if (hour > dt.hour) return true;
    if (minute < dt.minute) return false;
    if (minute > dt.minute) return true;
    return second > dt.second;
}

void Time::to_stream_iso8601(std::ostream& out) const
{
    out <<        setw(2) << setfill('0') << (unsigned)hour
        << ':' << setw(2) << setfill('0') << (unsigned)minute
        << ':' << setw(2) << setfill('0') << (unsigned)second;
}

void Time::to_csv_iso8601(CSVWriter& out) const
{
    if (is_missing())
        out.add_value_empty();
    else
    {
        char buf[16];
        snprintf(buf, 16, "%02hhu:%02hhu:%02hhu", hour, minute, second);
        out.add_value(buf);
    }
}

std::ostream& operator<<(std::ostream& out, const Time& dt)
{
    dt.to_stream_iso8601(out);
    return out;
}


/*
 * Datetime
 */

Datetime::Datetime()
    : year(0xffff), month(0xff), day(0xff), hour(0xff), minute(0xff), second(0xff)
{
}

Datetime::Datetime(const Date& date, const Time& time)
        : year(date.year), month(date.month), day(date.day),
          hour(time.hour), minute(time.minute), second(time.second) {}

Datetime::Datetime(int ye, int mo, int da, int ho, int mi, int se)
{
    if (ye == MISSING_INT)
    {
        year = 0xffff;
        month = day = hour = minute = second = 0xff;
    } else {
        Datetime::validate(ye, mo, da, ho, mi, se);
        year = ye;
        month = mo;
        day = da;
        hour = ho;
        minute = mi;
        second = se;
    }
}

void Datetime::validate(int ye, int mo, int da, int ho, int mi, int se)
{
    Date::validate(ye, mo, da);
    Time::validate(ho, mi, se);
}

void Datetime::normalise_h24(int& ye, int& mo, int& da, int& ho, int& mi, int& se)
{
    if (ho != 24 || mi != 0 || se != 0) return;
    ho = 0;
    ++da;
    if (da > Date::days_in_month(ye, mo))
    {
        da = 1;
        ++mo;
        if (mo > 12)
        {
            mo = 1;
            ++ye;
        }
    }
}

Datetime Datetime::from_julian(int jday, int ho, int mi, int se)
{
    return Datetime(Date::from_julian(jday), Time(ho, mi, se));
}

static void check_partial_consistency(const Datetime& dt)
{
    if (dt.year == 0xffff)
    {
        if (dt.month  != 0xff) error_consistency::throwf("month %d given with no year", dt.month);
        if (dt.day    != 0xff) error_consistency::throwf("day %d given with no year", dt.day);
        if (dt.hour   != 0xff) error_consistency::throwf("hour %d given with no year", dt.hour);
        if (dt.minute != 0xff) error_consistency::throwf("minute %d given with no year", dt.minute);
        if (dt.second != 0xff) error_consistency::throwf("second %d given with no year", dt.second);
    }
    if (dt.month == 0xff) {
        if (dt.day != 0xff) error_consistency::throwf("day %d given with no month", dt.day);
    }
    if (dt.hour == 0xff) {
        if (dt.minute != 0xff) error_consistency::throwf("minute %d given with no hour", dt.minute);
        if (dt.second != 0xff) error_consistency::throwf("second %d given with no hour", dt.second);
    }
    if (dt.minute == 0xff) {
        if (dt.second != 0xff) error_consistency::throwf("second %d given with no minute", dt.second);
    }
}

static void check_partial_consistency(int ye, int mo, int da, int ho, int mi, int se)
{
    if (ye == MISSING_INT)
    {
        if (mo != MISSING_INT) error_consistency::throwf("month %d given with no year", mo);
        if (da != MISSING_INT) error_consistency::throwf("day %d given with no year", da);
        if (ho != MISSING_INT) error_consistency::throwf("hour %d given with no year", ho);
        if (mi != MISSING_INT) error_consistency::throwf("minute %d given with no year", mi);
        if (se != MISSING_INT) error_consistency::throwf("second %d given with no year", se);
    }
    if (mo == MISSING_INT) {
        if (da != MISSING_INT) error_consistency::throwf("day %d given with no month", da);
    }
    if (ho == MISSING_INT) {
        if (mi != MISSING_INT) error_consistency::throwf("minute %d given with no hour", mi);
        if (se != MISSING_INT) error_consistency::throwf("second %d given with no hour", se);
    }
    if (mi == MISSING_INT) {
        if (se != MISSING_INT) error_consistency::throwf("second %d given with no minute", se);
    }
}

Datetime Datetime::lower_bound(int ye, int mo, int da, int ho, int mi, int se)
{
    check_partial_consistency(ye, mo, da, ho, mi, se);
    if (ye == MISSING_INT) return Datetime();
    if (mo == MISSING_INT) mo = 1;
    if (da == MISSING_INT) da = 1;
    if (ho == MISSING_INT) ho = 0;
    if (mi == MISSING_INT) mi = 0;
    if (se == MISSING_INT) se = 0;
    return Datetime(ye, mo, da, ho, mi, se);
}

Datetime Datetime::upper_bound(int ye, int mo, int da, int ho, int mi, int se)
{
    check_partial_consistency(ye, mo, da, ho, mi, se);
    if (ye == MISSING_INT) return Datetime();
    if (mo == MISSING_INT) mo = 12;
    if (da == MISSING_INT) da = Date::days_in_month(ye, mo);
    if (ho == MISSING_INT) ho = 23;
    if (mi == MISSING_INT) mi = 59;
    if (se == MISSING_INT) se = 59;
    return Datetime(ye, mo, da, ho, mi, se);
}

void Datetime::set_lower_bound()
{
    check_partial_consistency(*this);
    if (year   == 0xffff) { month = day = hour = minute = second = 0xff; return; }
    if (month  == 0xff) month = 1;
    if (day    == 0xff) day = 1;
    if (hour   == 0xff) hour = 0;
    if (minute == 0xff) minute = 0;
    if (second == 0xff) second = 0;
}

void Datetime::set_upper_bound()
{
    check_partial_consistency(*this);
    if (year   == 0xffff) { month = day = hour = minute = second = 0xff; return; }
    if (month  == 0xff) month = 12;
    if (day    == 0xff) day = Date::days_in_month(year, month);
    if (hour   == 0xff) hour = 23;
    if (minute == 0xff) minute = 59;
    if (second == 0xff) second = 59;
}

Date Datetime::date() const { return is_missing() ? Date() : Date(year, month, day); }

Time Datetime::time() const { return is_missing() ? Time() : Time(hour, minute, second); }

bool Datetime::is_missing() const { return year == 0xffff; }

int Datetime::to_julian() const
{
    return Date::calendar_to_julian(year, month, day);
}

int Datetime::compare(const Datetime& o) const
{
    if (int res = year - o.year) return res;
    if (int res = month - o.month) return res;
    if (int res = day - o.day) return res;
    if (int res = hour - o.hour) return res;
    if (int res = minute - o.minute) return res;
    return second - o.second;
}

bool Datetime::operator==(const Datetime& o) const
{
    return std::tie(year, month, day, hour, minute, second) == std::tie(o.year, o.month, o.day, o.hour, o.minute, o.second);
}

bool Datetime::operator!=(const Datetime& o) const
{
    return std::tie(year, month, day, hour, minute, second) != std::tie(o.year, o.month, o.day, o.hour, o.minute, o.second);
}

bool Datetime::operator<(const Datetime& o) const
{
    return std::tie(year, month, day, hour, minute, second) < std::tie(o.year, o.month, o.day, o.hour, o.minute, o.second);
}

bool Datetime::operator>(const Datetime& o) const
{
    return std::tie(year, month, day, hour, minute, second) > std::tie(o.year, o.month, o.day, o.hour, o.minute, o.second);
}

bool Datetime::operator<=(const Datetime& o) const
{
    return std::tie(year, month, day, hour, minute, second) <= std::tie(o.year, o.month, o.day, o.hour, o.minute, o.second);
}

bool Datetime::operator>=(const Datetime& o) const
{
    return std::tie(year, month, day, hour, minute, second) >= std::tie(o.year, o.month, o.day, o.hour, o.minute, o.second);
}

Datetime Datetime::from_iso8601(const char* str)
{
    // Handle empty strings as missing values
    if (!*str)
        return Datetime();

    int ye, mo, da, ho, mi, se;
    char sep;
    if (sscanf(str, "%04d-%02d-%02d%c%02d:%02d:%02d", &ye, &mo, &da, &sep, &ho, &mi, &se) != 7)
        error_consistency::throwf("cannot parse date/time string \"%s\"", str);
    if (sep != 'T' && sep != ' ')
        error_consistency::throwf("invalid iso8601 separator '%c' in datetime string \"%s\"", sep, str);
    return Datetime(ye, mo, da, ho, mi, se);
}

void Datetime::to_stream_iso8601(std::ostream& out, char sep, const char* tz) const
{
    out <<        setw(4) << setfill('0') << year
        << '-' << setw(2) << setfill('0') << (unsigned)month
        << '-' << setw(2) << setfill('0') << (unsigned)day
        << sep << setw(2) << setfill('0') << (unsigned)hour
        << ':' << setw(2) << setfill('0') << (unsigned)minute
        << ':' << setw(2) << setfill('0') << (unsigned)second
        << tz;
}

int Datetime::print_iso8601(FILE* out, char sep, const char* end) const
{
    return fprintf(out, "%04hu-%02hhu-%02hhu%c%02hhu:%02hhu:%02hhu%s",
            year, month, day, sep, hour, minute, second, end);
}

int Datetime::print(FILE* out, const char* end) const
{
    return print_iso8601(out, 'T', end);
}

void Datetime::to_csv_iso8601(CSVWriter& out, char sep, const char* tz) const
{
    if (is_missing())
        out.add_value_empty();
    else
    {
        char buf[32];
        snprintf(buf, 32, "%04hu-%02hhu-%02hhu%c%02hhu:%02hhu:%02hhu%s",
                year, month, day, sep, hour, minute, second, tz);
        out.add_value(buf);
    }
}

std::string Datetime::to_string(char sep, const char* tz) const
{
    if (is_missing())
        return std::string();
    char buf[32];
    int len = snprintf(buf, 32, "%04hu-%02hhu-%02hhu%c%02hhu:%02hhu:%02hhu%s",
            year, month, day, sep, hour, minute, second, tz);
    return std::string(buf, len);
}

std::ostream& operator<<(std::ostream& out, const Datetime& dt)
{
    dt.to_stream_iso8601(out);
    return out;
}


/*
 * DatetimeRange
 */

DatetimeRange::DatetimeRange(
            int yemin, int momin, int damin, int homin, int mimin, int semin,
            int yemax, int momax, int damax, int homax, int mimax, int semax)
{
    set(yemin, momin, damin, homin, mimin, semin, yemax, momax, damax, homax, mimax, semax);
}

bool DatetimeRange::is_missing() const
{
    return min.is_missing() && max.is_missing();
}

bool DatetimeRange::operator==(const DatetimeRange& o) const { return std::tie(min, max) == std::tie(o.min, o.max); }
bool DatetimeRange::operator!=(const DatetimeRange& o) const { return std::tie(min, max) != std::tie(o.min, o.max); }
bool DatetimeRange::operator<(const DatetimeRange& o) const { return std::tie(min, max) < std::tie(o.min, o.max); }
bool DatetimeRange::operator<=(const DatetimeRange& o) const { return std::tie(min, max) <= std::tie(o.min, o.max); }
bool DatetimeRange::operator>(const DatetimeRange& o) const { return std::tie(min, max) > std::tie(o.min, o.max); }
bool DatetimeRange::operator>=(const DatetimeRange& o) const { return std::tie(min, max) >= std::tie(o.min, o.max); }

void DatetimeRange::merge(const DatetimeRange& range)
{
    if (!min.is_missing() && (range.min.is_missing() || range.min < min))
        min = range.min;

    if (!max.is_missing() && (range.max.is_missing() || range.max > max))
        max = range.max;
}

void DatetimeRange::set(const Datetime& min, const Datetime& max)
{
    this->min = min;
    this->max = max;
}

void DatetimeRange::set(
        int yemin, int momin, int damin, int homin, int mimin, int semin,
        int yemax, int momax, int damax, int homax, int mimax, int semax)
{
    min = Datetime::lower_bound(yemin, momin, damin, homin, mimin, semin);
    max = Datetime::upper_bound(yemax, momax, damax, homax, mimax, semax);
}

bool DatetimeRange::contains(const Datetime& dt) const
{
    if (min.is_missing())
        if (max.is_missing())
            return true;
        else
            return dt <= max;
    else
        if (max.is_missing())
            return dt >= min;
        else
            return min <= dt && dt <= max;
}

bool DatetimeRange::contains(const DatetimeRange& dtr) const
{
    if (!min.is_missing() && (dtr.min.is_missing() || dtr.min < min))
        return false;

    if (!max.is_missing() && (dtr.max.is_missing() || dtr.max > max))
        return false;

    return true;
}

bool DatetimeRange::is_disjoint(const DatetimeRange& dtr) const
{
    if (!max.is_missing() && !dtr.min.is_missing() && max < dtr.min)
        return true;

    if (!dtr.max.is_missing() && !min.is_missing() && dtr.max < min)
        return true;

    return false;
}

int DatetimeRange::print(FILE* out, const char* end) const
{
    int res = min.print(out, " to ");
    res += max.print(out, end);
    return res;
}

std::ostream& operator<<(std::ostream& out, const DatetimeRange& dtr)
{
    if (dtr.min == dtr.max)
        dtr.min.to_stream_iso8601(out);
    else
    {
        out << "(";
        dtr.min.to_stream_iso8601(out);
        out << " to ";
        dtr.max.to_stream_iso8601(out);
        out << ")";
    }
    return out;
}


/*
 * Coordinate utilities
 */

namespace {

inline int ll_to_int(double ll) { return lround(ll * 100000.0); }
inline double ll_from_int(int ll) { return (double)ll / 100000.0; }
inline int normalon(int lon)
{
    return ((lon + 18000000) % 36000000) - 18000000;
}

}


/*
 * Coords
 */

Coords::Coords(int lat, int lon)
{
    set(lat, lon);
}

Coords::Coords(double lat, double lon)
    : lat(ll_to_int(lat)),
      lon(normalon(ll_to_int(lon)))
{
}

bool Coords::is_missing() const
{
    return lat == MISSING_INT && lon == MISSING_INT;
}

void Coords::set_lat(double lat) { this->lat = ll_to_int(lat); }

void Coords::set_lon(double lon) { this->lon = normalon(ll_to_int(lon)); }

void Coords::set_lat(int lat)
{
    if (lat == MISSING_INT)
        this->lat = this->lon = MISSING_INT;
    else
        this->lat = lat;
}

void Coords::set_lon(int lon)
{
    if (lon == MISSING_INT)
        this->lat = this->lon = MISSING_INT;
    else
        this->lon = normalon(lon);
}

void Coords::set(int lat, int lon)
{
    if (lat == MISSING_INT || lon == MISSING_INT)
        this->lat = this->lon = MISSING_INT;
    else
    {
        this->lat = lat;
        this->lon = normalon(lon);
    }
}

void Coords::set(double lat, double lon)
{
    this->lat = ll_to_int(lat);
    this->lon = normalon(ll_to_int(lon));
}

double Coords::dlat() const { return ll_from_int(lat); }
double Coords::dlon() const { return ll_from_int(lon); }

int Coords::compare(const Coords& o) const
{
    if (int res = lat - o.lat) return res;
    return lon - o.lon;
}

bool Coords::operator==(const Coords& o) const { return std::tie(lat, lon) == std::tie(o.lat, o.lon); }
bool Coords::operator!=(const Coords& o) const { return std::tie(lat, lon) != std::tie(o.lat, o.lon); }
bool Coords::operator<(const Coords& o) const { return std::tie(lat, lon) < std::tie(o.lat, o.lon); }
bool Coords::operator>(const Coords& o) const { return std::tie(lat, lon) > std::tie(o.lat, o.lon); }
bool Coords::operator<=(const Coords& o) const { return std::tie(lat, lon) <= std::tie(o.lat, o.lon); }
bool Coords::operator>=(const Coords& o) const { return std::tie(lat, lon) >= std::tie(o.lat, o.lon); }

int Coords::print(FILE* out, const char* end) const
{
    return fprintf(out, "%.5f,%.5f%s", dlat(), dlon(), end);
}

std::string Coords::to_string(const char* undef) const
{
    string res;
    if (lat == MISSING_INT)
        res += undef;
    else
        res += std::to_string(dlat());
    res += ",";
    if (lon == MISSING_INT)
        res += undef;
    else
        res += std::to_string(dlon());
    return res;
}

std::ostream& operator<<(std::ostream& out, const Coords& c)
{
    if (c.is_missing())
        return out << "(-,-)";
    else
        return out << fixed
                   << "(" << setprecision(5) << c.dlat()
                   << "," << setprecision(5) << c.dlon()
                   << ")"
    //             << resetiosflags(ios_base::floatfield);
    //             << defaultfloat;
            ;
}

int Coords::lat_to_int(double lat)
{
    return ll_to_int(lat);
}

int Coords::lon_to_int(double lon)
{
    return normalon(ll_to_int(lon));
}

double Coords::lat_from_int(int lat)
{
    return ll_from_int(lat);
}

double Coords::lon_from_int(int lon)
{
    return ll_from_int(lon);
}


/*
 * LatRange
 */

constexpr int LatRange::IMIN;
constexpr int LatRange::IMAX;
constexpr double LatRange::DMIN;
constexpr double LatRange::DMAX;

LatRange::LatRange(int min, int max)
    : imin(min == MISSING_INT ? LatRange::IMIN : min),
      imax(max == MISSING_INT ? LatRange::IMAX : max) {}

LatRange::LatRange(double min, double max)
    : imin(ll_to_int(min)),
      imax(ll_to_int(max))
{
}

bool LatRange::operator==(const LatRange& lr) const
{
    return imin == lr.imin && imax == lr.imax;
}

bool LatRange::operator!=(const LatRange& lr) const
{
    return imin != lr.imin || imax != lr.imax;
}

bool LatRange::is_missing() const { return imin == IMIN && imax == IMAX; }

double LatRange::dmin() const { return ll_from_int(imin); }
double LatRange::dmax() const { return ll_from_int(imax); }

void LatRange::get(double& min, double& max) const
{
    min = ll_from_int(imin);
    max = ll_from_int(imax);
}

void LatRange::set(int min, int max)
{
    imin = min == MISSING_INT ? LatRange::IMIN : min;
    imax = max == MISSING_INT ? LatRange::IMAX : max;
}

void LatRange::set(double min, double max)
{
    imin = ll_to_int(min);
    imax = ll_to_int(max);
}

bool LatRange::contains(int lat) const
{
    return lat >= imin && lat <= imax;
}

bool LatRange::contains(double lat) const
{
    int ilat = ll_to_int(lat);
    return ilat >= imin && ilat <= imax;
}

bool LatRange::contains(const LatRange& lr) const
{
    return imin <= lr.imin && lr.imax <= imax;
}

int LatRange::print(FILE* out, const char* end) const
{
    double dmin, dmax;
    get(dmin, dmax);
    return fprintf(out, "(%.5f to %.5f)%s", dmin, dmax, end);
}

std::ostream& operator<<(std::ostream& out, const LatRange& lr)
{
    double dmin, dmax;
    lr.get(dmin, dmax);
    out << fixed
        << "(" << setprecision(5) << dmin
        << " to " << setprecision(5) << dmax
        << ")"
        << resetiosflags(ios_base::floatfield);
    //    << defaultfloat;
    return out;
}


/*
 * LonRange
 */

LonRange::LonRange(int min, int max)
{
    set(min, max);
}

LonRange::LonRange(double min, double max)
    : imin(normalon(ll_to_int(min))), imax(normalon(ll_to_int(max)))
{
    if (min != max && imin == imax)
        imin = imax = MISSING_INT;
}

bool LonRange::operator==(const LonRange& lr) const
{
    if ((imin == MISSING_INT || imax == MISSING_INT) && (lr.imin == MISSING_INT || lr.imax == MISSING_INT))
        return true;
    return imin == lr.imin && imax == lr.imax;
}

bool LonRange::operator!=(const LonRange& lr) const
{
    if ((imin == MISSING_INT || imax == MISSING_INT) && (lr.imin == MISSING_INT || lr.imax == MISSING_INT))
        return false;
    return imin != lr.imin || imax != lr.imax;
}

bool LonRange::is_missing() const
{
    return imin == MISSING_INT || imax == MISSING_INT;
}

double LonRange::dmin() const { return imin == MISSING_INT ? -180.0 : ll_from_int(imin); }
double LonRange::dmax() const { return imax == MISSING_INT ?  180.0 : ll_from_int(imax); }

void LonRange::get(double& min, double& max) const
{
    if (is_missing())
    {
        min = -180.0;
        max = 180.0;
    } else {
        min = ll_from_int(imin);
        max = ll_from_int(imax);
    }
}

void LonRange::set(int min, int max)
{
    if ((min != MISSING_INT || max != MISSING_INT) && (min == MISSING_INT || max == MISSING_INT))
        error_consistency::throwf("cannot set longitude range to an open ended range");
    imin = min == MISSING_INT ? MISSING_INT : normalon(min);
    imax = max == MISSING_INT ? MISSING_INT : normalon(max);
    // Catch cases like min=0 max=360, that would match anything, and set them
    // to missing range match
    if (min != max && imin == imax)
        imin = imax = MISSING_INT;
}

void LonRange::set(double min, double max)
{
    set(ll_to_int(min), ll_to_int(max));
}

void LonRange::set(const LonRange& lr)
{
    set(lr.imin, lr.imax);
}

bool LonRange::contains(int lon) const
{
    if (imin == imax)
    {
        if (imin == MISSING_INT)
            return true;
        return lon == imin;
    } else if (imin < imax) {
        return lon >= imin && lon <= imax;
    } else {
        return ((lon >= imin and lon <= 18000000)
             or (lon >= -18000000 and lon <= imax));
    }
}

bool LonRange::contains(double lon) const
{
    return contains(ll_to_int(lon));
}

bool LonRange::contains(const LonRange& lr) const
{
    if (is_missing()) return true;
    if (lr.is_missing()) return false;

    // Longitude ranges can match outside or inside the interval
    if (imin < imax)
    {
        // we match inside the interval
        if (lr.imin < lr.imax)
        {
            // lr matches inside the interval
            return imin <= lr.imin && lr.imax <= imax;
        } else {
            // lr matches outside the interval
            return false;
        }
    } else {
        // we match outside the interval
        if (lr.imin < lr.imax)
        {
            // lr matches inside the interval
            return lr.imax <= imin || lr.imin >= imax;
        } else {
            // lr matches outside the interval
            return lr.imin <= imin || lr.imax >= imax;
        }
    }
}

int LonRange::print(FILE* out, const char* end) const
{
    double dmin, dmax;
    get(dmin, dmax);
    return fprintf(out, "(%.5f to %.5f)%s", dmin, dmax, end);
}

std::ostream& operator<<(std::ostream& out, const LonRange& lr)
{
    double dmin, dmax;
    lr.get(dmin, dmax);
    out << fixed
        << "(" << setprecision(5) << dmin
        << " to " << setprecision(5) << dmax
        << ")"
        << resetiosflags(ios_base::floatfield);
    //    << defaultfloat;
    return out;
}


/*
 * Level
 */

bool Level::is_missing() const { return ltype1 == MISSING_INT && l1 == MISSING_INT && ltype2 == MISSING_INT && l2 == MISSING_INT; }

bool Level::operator==(const Level& o) const
{
    return std::tie(ltype1, l1, ltype2, l2) == std::tie(o.ltype1, o.l1, o.ltype2, o.l2);
}

bool Level::operator!=(const Level& o) const
{
    return std::tie(ltype1, l1, ltype2, l2) != std::tie(o.ltype1, o.l1, o.ltype2, o.l2);
}

bool Level::operator<(const Level& o) const
{
    return std::tie(ltype1, l1, ltype2, l2) < std::tie(o.ltype1, o.l1, o.ltype2, o.l2);
}

bool Level::operator>(const Level& o) const
{
    return std::tie(ltype1, l1, ltype2, l2) > std::tie(o.ltype1, o.l1, o.ltype2, o.l2);
}

bool Level::operator<=(const Level& o) const
{
    return std::tie(ltype1, l1, ltype2, l2) <= std::tie(o.ltype1, o.l1, o.ltype2, o.l2);
}

bool Level::operator>=(const Level& o) const
{
    return std::tie(ltype1, l1, ltype2, l2) >= std::tie(o.ltype1, o.l1, o.ltype2, o.l2);
}

int Level::compare(const Level& l) const
{
    int res;
    if ((res = ltype1 - l.ltype1)) return res;
    if ((res = l1 - l.l1)) return res;
    if ((res = ltype2 - l.ltype2)) return res;
    return l2 - l.l2;
}

static std::string describe_level(int ltype, int val)
{
    char buf[256];

    switch (ltype)
    {
        case 1:   return "Ground or water surface";
        case 2:   return "Cloud base level";
        case 3:   return "Level of cloud tops";
        case 4:   return "Level of 0°C isotherm";
        case 5:   return "Level of adiabatic condensation lifted from the surface";
        case 6:   return "Maximum wind level";
        case 7:   return "Tropopause";
        case 8:   if (val == 0)
                      return "Nominal top of atmosphere";
                  else
                      return snprintf(buf, 256, "Nominal top of atmosphere, channel %d", val), buf;
        case 9:   return "Sea bottom";
        case 20:  return snprintf(buf, 256, "Isothermal level, %.1fK", (double)val/10), buf;
        case 100: return snprintf(buf, 256, "Isobaric surface, %.2fhPa", (double)val/100), buf;
        case 101: return "Mean sea level";
        case 102: return snprintf(buf, 256, "%.3fm above mean sea level", (double)val/1000), buf;
        case 103: return snprintf(buf, 256, "%.3fm above ground", (double)val/1000), buf;
        case 104: return snprintf(buf, 256, "Sigma level %.5f", (double)val/10000), buf;
        case 105: return snprintf(buf, 256, "Hybrid level %d", val), buf;
        case 106: return snprintf(buf, 256, "%.3fm below land surface", (double)val/1000), buf;
        case 107: return snprintf(buf, 256, "Isentropic (theta) level, potential temperature %.1fK", (double)val/10), buf;
        case 108: return snprintf(buf, 256, "Pressure difference %.2fhPa from ground to level", (double)val/100), buf;
        case 109: return snprintf(buf, 256, "Potential vorticity surface %.3f 10-6 K m2 kg-1 s-1", (double)val/1000), buf;
        case 111: return snprintf(buf, 256, "ETA* level %.5f", (double)val/10000), buf;
        case 117: return snprintf(buf, 256, "Mixed layer depth %.3fm", (double)val/1000), buf;
        case 160: return snprintf(buf, 256, "%.3fm below sea level", (double)val/1000), buf;
        case 200: return "Entire atmosphere (considered as a single layer)";
        case 201: return "Entire ocean (considered as a single layer)";
        case 204: return "Highest tropospheric freezing level";
        case 206: return "Grid scale cloud bottom level";
        case 207: return "Grid scale cloud top level";
        case 209: return "Boundary layer cloud bottom level";
        case 210: return "Boundary layer cloud top level";
        case 211: return "Boundary layer cloud layer";
        case 212: return "Low cloud bottom level";
        case 213: return "Low cloud top level";
        case 214: return "Low cloud layer";
        case 215: return "Cloud ceiling";
        case 220: return "Planetary Boundary Layer";
        case 222: return "Middle cloud bottom level";
        case 223: return "Middle cloud top level";
        case 224: return "Middle cloud layer";
        case 232: return "High cloud bottom level";
        case 233: return "High cloud top level";
        case 234: return "High cloud layer";
        case 235: return snprintf(buf, 256, "Ocean Isotherm Level, %.1fK", (double)val/10), buf;
        case 240: return "Ocean Mixed Layer";
        case 242: return "Convective cloud bottom level";
        case 243: return "Convective cloud top level";
        case 244: return "Convective cloud layer";
        case 245: return "Lowest level of the wet bulb zero";
        case 246: return "Maximum equivalent potential temperature level";
        case 247: return "Equilibrium level";
        case 248: return "Shallow convective cloud bottom level";
        case 249: return "Shallow convective cloud top level";
        case 251: return "Deep convective cloud bottom level";
        case 252: return "Deep convective cloud top level";
        case 253: return "Lowest bottom level of supercooled liquid water layer";
        case 254: return "Highest top level of supercooled liquid water layer";
        case 255: return "Missing";
        case 256: return "Clouds";
        case 258:
            switch (val) {
                case 0: return "General cloud group";
                case 1: return "CL";
                case 2: return "CM";
                case 3: return "CH";
                default: return snprintf(buf, 256, "%d %d", ltype, val), buf;
            }
            break;
        case 259: return snprintf(buf, 256, "Cloud group %d", val), buf;
        case 260: return snprintf(buf, 256, "Cloud drift group %d", val), buf;
        case 261: return snprintf(buf, 256, "Cloud elevation group %d", val), buf;
        case 262: return "Direction and elevation of clouds";
        case 265: return snprintf(buf, 256, "Non-physical data level #%d", val), buf;
        case MISSING_INT: return "Information about the station that generated the data";
        default:    return snprintf(buf, 256, "%d %d", ltype, val), buf; break;
    }
}

std::string Level::describe() const
{
    if (ltype2 == MISSING_INT)
        return describe_level(ltype1, l1);

    if (ltype1 == 256 || ltype1 == 264)
    {
        string lev1 = describe_level(ltype1, l1);
        string lev2 = describe_level(ltype2, l2);
        return lev1 + ", " + lev2;
    } else {
        string lev1 = describe_level(ltype1, l1);
        string lev2 = describe_level(ltype2, l2);
        return "Layer from [" + lev1 + "] to [" + lev2 + "]";
    }
}

void Level::to_stream(std::ostream& out, const char* undef) const
{
    if (ltype1 == MISSING_INT) out << undef; else out << ltype1;
    out << ",";
    if (l1 == MISSING_INT) out << undef; else out << l1;
    out << ",";
    if (ltype2 == MISSING_INT) out << undef; else out << ltype2;
    out << ",";
    if (l2 == MISSING_INT) out << undef; else out << l2;
}

std::string Level::to_string(const char* undef) const
{
    string res;
    if (ltype1 == MISSING_INT) res += undef; else res += std::to_string(ltype1);
    res += ",";
    if (l1 == MISSING_INT) res += undef; else res += std::to_string(l1);
    res += ",";
    if (ltype2 == MISSING_INT) res += undef; else res += std::to_string(ltype2);
    res += ",";
    if (l2 == MISSING_INT) res += undef; else res += std::to_string(l2);
    return res;
}

void Level::to_csv(CSVWriter& out) const
{
    if (ltype1 == MISSING_INT) out.add_value_empty(); else out.add_value(ltype1);
    if (l1 == MISSING_INT) out.add_value_empty(); else out.add_value(l1);
    if (ltype2 == MISSING_INT) out.add_value_empty(); else out.add_value(ltype2);
    if (l2 == MISSING_INT) out.add_value_empty(); else out.add_value(l2);
}

Level Level::cloud(int ltype2, int l2) { return Level(256, MISSING_INT, ltype2, l2); }

int Level::print(FILE* out, const char* undef, const char* end) const
{
    int res = 0;
    if (ltype1 == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d,", ltype1);
    if (l1 == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d,", l1);
    if (ltype2 == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d,", ltype2);
    if (l2 == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d", l2);
    res += fprintf(out, "%s", end);
    return res;
}

std::ostream& operator<<(std::ostream& out, const Level& l)
{
    l.to_stream(out);
    return out;
}


/*
 * Trange
 */

bool Trange::is_missing() const { return pind == MISSING_INT && p1 == MISSING_INT && p2 == MISSING_INT; }

int Trange::compare(const Trange& t) const
{
    int res;
    if ((res = pind - t.pind)) return res;
    if ((res = p1 - t.p1)) return res;
    return p2 - t.p2;
}

bool Trange::operator==(const Trange& o) const { return std::tie(pind, p1, p2) == std::tie(o.pind, o.p1, o.p2); }
bool Trange::operator!=(const Trange& o) const { return std::tie(pind, p1, p2) != std::tie(o.pind, o.p1, o.p2); }
bool Trange::operator<(const Trange& o) const { return std::tie(pind, p1, p2) < std::tie(o.pind, o.p1, o.p2); }
bool Trange::operator>(const Trange& o) const { return std::tie(pind, p1, p2) > std::tie(o.pind, o.p1, o.p2); }
bool Trange::operator<=(const Trange& o) const { return std::tie(pind, p1, p2) <= std::tie(o.pind, o.p1, o.p2); }
bool Trange::operator>=(const Trange& o) const { return std::tie(pind, p1, p2) >= std::tie(o.pind, o.p1, o.p2); }

void Trange::to_stream(std::ostream& out, const char* undef) const
{
    if (pind == MISSING_INT) out << undef; else out << pind;
    out << ",";
    if (p1 == MISSING_INT) out << undef; else out << p1;
    out << ",";
    if (p2 == MISSING_INT) out << undef; else out << p2;
}

std::string Trange::to_string(const char* undef) const
{
    string res;
    if (pind == MISSING_INT) res += undef; else res += std::to_string(pind);
    res += ",";
    if (p1 == MISSING_INT) res += undef; else res += std::to_string(p1);
    res += ",";
    if (p2 == MISSING_INT) res += undef; else res += std::to_string(p2);
    return res;
}

void Trange::to_csv(CSVWriter& out) const
{
    if (pind == MISSING_INT) out.add_value_empty(); else out.add_value(pind);
    if (p1 == MISSING_INT) out.add_value_empty(); else out.add_value(p1);
    if (p2 == MISSING_INT) out.add_value_empty(); else out.add_value(p2);
}

Trange Trange::instant() { return Trange(254, 0, 0); }

static std::string format_seconds(int val)
{
    static const int bufsize = 128;
    char buf[bufsize];

    if (val == MISSING_INT) return "-";

    int i = 0;
    if (val / (3600*24) != 0)
    {
        i += snprintf(buf+i, bufsize-i, "%dd ", val / (3600*24));
        val = abs(val) % (3600*24);
    }
    if (val / 3600 != 0)
    {
        i += snprintf(buf+i, bufsize-i, "%dh ", val / 3600);
        val = abs(val) % 3600;
    }
    if (val / 60 != 0)
    {
        i += snprintf(buf+i, bufsize-i, "%dm ", val / 60);
        val = abs(val) % 60;
    }
    if (val)
        i += snprintf(buf+i, bufsize-i, "%ds ", val);
    if (i > 0)
        --i;
    else
    {
        buf[0] = '0';
        i = 1;
    }
    buf[i] = 0;

    return buf;
}

static std::string mkdesc(const std::string& root, int p1, int p2)
{
    if (p1 == MISSING_INT && p2 == MISSING_INT)
        return root;

    string res = root;
    if (p2 != MISSING_INT)
        res += " over " + format_seconds(p2);

    if (p1 == MISSING_INT)
        return res;
    if (p1 < 0)
        return res + format_seconds(-p1) + " before reference time";
    return res + " at forecast time " + format_seconds(p1);
}

std::string Trange::describe() const
{
    char buf[256];

    switch (pind)
    {
        case 0:   return mkdesc("Average", p1, p2);
        case 1:   return mkdesc("Accumulation", p1, p2);
        case 2:   return mkdesc("Maximum", p1, p2);
        case 3:   return mkdesc("Minimum", p1, p2);
        case 4:   return mkdesc("Difference (end minus beginning)", p1, p2);
        case 5:   return mkdesc("Root Mean Square", p1, p2);
        case 6:   return mkdesc("Standard Deviation", p1, p2);
        case 7:   return mkdesc("Covariance (temporal variance)", p1, p2);
        case 8:   return mkdesc("Difference (beginning minus end)", p1, p2);
        case 9:   return mkdesc("Ratio", p1, p2);
        case 51:  return mkdesc("Climatological Mean Value", p1, p2);
        case 200: return mkdesc("Vectorial mean", p1, p2);
        case 201: return mkdesc("Mode", p1, p2);
        case 202: return mkdesc("Standard deviation vectorial mean", p1, p2);
        case 203: return mkdesc("Vectorial maximum", p1, p2);
        case 204: return mkdesc("Vectorial minimum", p1, p2);
        case 205: return mkdesc("Product with a valid time ranging", p1, p2);
        case 254:
              if (p1 == 0 && p2 == 0)
                  return "Analysis or observation, istantaneous value";
              else
                  return "Forecast at t+" + format_seconds(p1) + ", instantaneous value";
        default:  return snprintf(buf, 256, "%d %d %d", pind, p1, p2), buf;
    }
}

int Trange::print(FILE* out, const char* undef, const char* end) const
{
    int res = 0;
    if (pind == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d,", pind);
    if (p1 == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d,", p1);
    if (p2 == MISSING_INT)
        res += fprintf(out, "%s,", undef);
    else
        res += fprintf(out, "%d", p2);
    res += fprintf(out, "%s", end);
    return res;
}

std::ostream& operator<<(std::ostream& out, const Trange& l)
{
    l.to_stream(out);
    return out;
}


/*
 * Ident
 */

Ident::Ident(const char* value) : value(value ? strdup(value) : nullptr) {}
Ident::Ident(const std::string& value) : value(strndup(value.data(), value.size())) {}
Ident::Ident(const Ident& o) : value(o.value ? strdup(o.value) : nullptr) {}
Ident::Ident(Ident&& o) : value(o.value) { o.value = nullptr; }
Ident::~Ident() { free(value); }
Ident& Ident::operator=(const Ident& o)
{
    if (value == o.value) return *this;
    free(value);
    value = o.value ? strdup(o.value) : nullptr;
    return *this;
}
Ident& Ident::operator=(Ident&& o)
{
    if (value == o.value) return *this;
    free(value);
    if (o.value)
    {
        value = strdup(o.value);
        o.value = nullptr;
    } else
        value = nullptr;
    return *this;
}
Ident& Ident::operator=(const char* o)
{
    if (value) free(value);
    value = o ? strdup(o) : nullptr;
    return *this;

}
Ident& Ident::operator=(const std::string& o)
{
    if (value) free(value);
    value = strndup(o.c_str(), o.size());
    return *this;
}
void Ident::clear()
{
    free(value);
    value = 0;
}
int Ident::compare(const Ident& o) const
{
    if (!value && !o.value) return 0;
    if (!value && o.value) return -1;
    if (value && !o.value) return 1;
    return strcmp(value, o.value);
}
int Ident::compare(const char* o) const
{
    if (!value && !o) return 0;
    if (!value && o) return -1;
    if (value && !o) return 1;
    return strcmp(value, o);
}
int Ident::compare(const std::string& o) const
{
    if (!value) return -1;
    return strcmp(value, o.c_str());
}

bool Ident::is_missing() const { return value == nullptr; }

Ident::operator std::string() const
{
    if (!value) throw error_consistency("ident is not set");
    return std::string(value);
}

std::ostream& operator<<(std::ostream& out, const Ident& i)
{
    if (i.is_missing())
        return out;
    else
        return out << (const char*)i;
}


/*
 * Station
 */

bool Station::is_missing() const
{
    return report.empty() && coords.is_missing() && ident.is_missing();
}

int Station::print(FILE* out, const char* end) const
{
    int res = 0;
    if (coords.is_missing())
    {
        fputs("(-,-) ", out);
        res += 6;
    }
    else
        res += coords.print(out, " ");

    if (ident.is_missing())
    {
        putc('-', out);
        res += 1;
    }
    else
    {
        fputs(ident.get(), out);
        res += strlen(ident.get());
    }

    res += fprintf(out, " %s%s", report.c_str(), end);
    return res;
}

std::string Station::to_string(const char* undef) const
{
    string res = report;
    res += ",";
    res += coords.to_string(undef);
    res += ",";
    if (ident.is_missing())
        res += undef;
    else
        res += ident.get();
    return res;
}

std::ostream& operator<<(std::ostream& out, const Station& st)
{
    return out << st.coords << "," << st.ident << "," << st.report;
}


/*
 * DBStation
 */

bool DBStation::is_missing() const
{
    return Station::is_missing() && id == MISSING_INT;
}

int DBStation::print(FILE* out, const char* end) const
{
    int res = 0;
    if (id == MISSING_INT)
    {
        fputs("- ", out);
        res += 2;
    }
    else
        res += fprintf(out, "%d,", id);

    res += Station::print(out, end);
    return res;
}

std::string DBStation::to_string(const char* undef) const
{
    string res = report;
    res += ",";
    if (id == MISSING_INT)
        res += undef;
    else
        res += std::to_string(id);
    res += ",";
    res += coords.to_string(undef);
    res += ",";
    if (ident.is_missing())
        res += undef;
    else
        res += ident.get();
    return res;
}

std::ostream& operator<<(std::ostream& out, const DBStation& st)
{
    if (st.id == MISSING_INT)
        out << "-,";
    else
        out << st.id << ",";

    return out << (const Station&)st;
}

}

namespace std {

size_t hash<dballe::Level>::operator()(dballe::Level const& o) const noexcept
{
    using dballe::MISSING_INT;
    size_t res = 0;
    if (o.ltype1 != MISSING_INT) res += o.ltype1;
    if (o.l1 != MISSING_INT) res += o.l1;
    if (o.ltype2 != MISSING_INT) res += o.ltype2 << 8;
    if (o.l2 != MISSING_INT) res += o.l2;
    return res;
}

size_t hash<dballe::Trange>::operator()(dballe::Trange const& o) const noexcept
{
    using dballe::MISSING_INT;
    size_t res = 0;
    if (o.pind != MISSING_INT) res += o.pind;
    if (o.p1 != MISSING_INT) res += o.p1;
    if (o.p2 != MISSING_INT) res += o.p2;
    return res;
}

size_t hash<dballe::Coords>::operator()(dballe::Coords const& o) const noexcept
{
    return o.lat xor o.lon;
}

size_t hash<dballe::Ident>::operator()(dballe::Ident const& o) const noexcept
{
    if (o.is_missing())
        return 0;
    return std::hash<std::string>{}(o.get());
}

size_t hash<dballe::Station>::operator()(dballe::Station const& o) const noexcept
{
    size_t res = std::hash<std::string>{}(o.report);
    res += std::hash<dballe::Coords>{}(o.coords);
    res += std::hash<dballe::Ident>{}(o.ident);
    return res;
}

size_t hash<dballe::DBStation>::operator()(dballe::DBStation const& o) const noexcept
{
    size_t res = std::hash<std::string>{}(o.report);
    res += o.id;
    res += std::hash<dballe::Coords>{}(o.coords);
    res += std::hash<dballe::Ident>{}(o.ident);
    return res;
}

}
