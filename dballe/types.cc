#include "types.h"
#include <wreport/error.h>
#include <ostream>
#include <iomanip>

using namespace wreport;
using namespace std;

namespace dballe {

/*
 * Date
 */

bool Date::is_missing() const { return year == 0xffff; }

void Date::from_array(const int* vals)
{
    year = vals[0];
    month = vals[1];
    day = vals[2];
}

void Date::to_array(int* vals) const
{
    vals[0] = (unsigned)year;
    vals[1] = (unsigned)month;
    vals[2] = (unsigned)day;
}

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

void Date::from_julian(int jday)
{
    julian_to_calendar(jday, year, month, day);
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


/*
 * Time
 */

bool Time::is_missing() const { return hour == 0xff; }

void Time::from_array(const int* vals)
{
    hour = vals[0];
    minute = vals[1];
    second = vals[2];
}

void Time::to_array(int* vals) const
{
    vals[0] = (unsigned)hour;
    vals[1] = (unsigned)minute;
    vals[2] = (unsigned)second;
}

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


/*
 * Datetime
 */

Date Datetime::date() const { return Date(year, month, day); }

Time Datetime::time() const { return Time(hour, minute, second); }

bool Datetime::is_missing() const { return year == 0xffff; }

int Datetime::to_julian() const
{
    return Date::calendar_to_julian(year, month, day);
}

void Datetime::from_julian(int jday)
{
    Date::julian_to_calendar(jday, year, month, day);
}

void Datetime::from_array(const int* vals)
{
    year = vals[0];
    month = vals[1];
    day = vals[2];
    hour = vals[3];
    minute = vals[4];
    second = vals[5];
}

void Datetime::to_array(int* vals) const
{
    vals[0] = (unsigned)year;
    vals[1] = (unsigned)month;
    vals[2] = (unsigned)day;
    vals[3] = (unsigned)hour;
    vals[4] = (unsigned)minute;
    vals[5] = (unsigned)second;
}

void Datetime::to_iso8601(std::ostream& out, char sep, const char* tz) const
{
    out <<        setw(4) << setfill('0') << year
        << '-' << setw(2) << setfill('0') << (unsigned)month
        << '-' << setw(2) << setfill('0') << (unsigned)day
        << sep << setw(2) << setfill('0') << (unsigned)hour
        << ':' << setw(2) << setfill('0') << (unsigned)minute
        << ':' << setw(2) << setfill('0') << (unsigned)second
        << tz;
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

bool Datetime::operator==(const Datetime& dt) const
{
    return year == dt.year && month == dt.month && day == dt.day
        && hour == dt.hour && minute == dt.minute && second == dt.second;
}

bool Datetime::operator!=(const Datetime& dt) const
{
    return year != dt.year || month != dt.month || day != dt.day
        || hour != dt.hour || minute != dt.minute || second != dt.second;
}

bool Datetime::operator<(const Datetime& dt) const
{
    if (year < dt.year) return true;
    if (year > dt.year) return false;
    if (month < dt.month) return true;
    if (month > dt.month) return false;
    if (day < dt.day) return true;
    if (day > dt.day) return false;
    if (hour < dt.hour) return true;
    if (hour > dt.hour) return false;
    if (minute < dt.minute) return true;
    if (minute > dt.minute) return false;
    return second < dt.second;
}

bool Datetime::operator>(const Datetime& dt) const
{
    if (year < dt.year) return false;
    if (year > dt.year) return true;
    if (month < dt.month) return false;
    if (month > dt.month) return true;
    if (day < dt.day) return false;
    if (day > dt.day) return true;
    if (hour < dt.hour) return false;
    if (hour > dt.hour) return true;
    if (minute < dt.minute) return false;
    if (minute > dt.minute) return true;
    return second > dt.second;
}

Datetime Datetime::from_iso8601(const char* str)
{
    int ye, mo, da, ho, mi, se;
    char sep;
    if (sscanf(str, "%04d-%02d-%02d%c%02d:%02d:%02d", &ye, &mo, &da, &sep, &ho, &mi, &se) != 7)
        error_consistency::throwf("cannot parse date/time string \"%s\"", str);
    if (sep != 'T' && sep != ' ')
        error_consistency::throwf("invalid iso8601 separator '%c' in datetime string \"%s\"", sep, str);
    return Datetime(ye, mo, da, ho, mi, se);
}

bool Datetime::range_equals(
        const Datetime& begin1, const Datetime& until1,
        const Datetime& begin2, const Datetime& until2)
{
    return begin1 == begin2 && until1 == until2;
}

bool Datetime::range_contains(
        const Datetime& begin1, const Datetime& until1,
        const Datetime& begin2, const Datetime& until2)
{
    if (!begin1.is_missing() && (begin2.is_missing() || begin2 < begin1))
        return false;

    if (!until1.is_missing() && (until2.is_missing() || until2 > until1))
        return false;

    return true;
}

bool Datetime::range_disjoint(
        const Datetime& begin1, const Datetime& until1,
        const Datetime& begin2, const Datetime& until2)
{
    if (!until1.is_missing() && !begin2.is_missing() && until1 < until2)
        return true;

    if (!until2.is_missing() && !begin1.is_missing() && until2 < until1)
        return true;

    return false;
}

Datetime Datetime::lower_bound() const
{
    Datetime res;

    if (is_missing()) return res;

    res.year = year;
    res.month = month == 0xff ? 1 : month;
    res.day = day == 0xff ? 1 : day;
    res.hour = hour == 0xff ? 0 : hour;
    res.minute = minute == 0xff ? 0 : minute;
    res.second = second == 0xff ? 0 : second;

    return res;
}

Datetime Datetime::upper_bound() const
{
    Datetime res;

    if (is_missing()) return res;

    res.year = year;
    res.month = month == 0xff ? 12 : month;
    res.day = day == 0xff ? Date::days_in_month(res.year, res.month) : day;
    res.hour = hour == 0xff ? 23 : hour;
    res.minute = minute == 0xff ? 59 : minute;
    res.second = second == 0xff ? 59 : second;

    return res;
}



}
