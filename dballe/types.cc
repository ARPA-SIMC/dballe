#include "types.h"
#include <wreport/error.h>
#include <ostream>
#include <iomanip>
#include <cstdarg>

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

void Date::to_stream_iso8601(std::ostream& out) const
{
    out <<        setw(4) << setfill('0') << year
        << '-' << setw(2) << setfill('0') << (unsigned)month
        << '-' << setw(2) << setfill('0') << (unsigned)day;
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

void Time::to_stream_iso8601(std::ostream& out) const
{
    out <<        setw(2) << setfill('0') << (unsigned)hour
        << ':' << setw(2) << setfill('0') << (unsigned)minute
        << ':' << setw(2) << setfill('0') << (unsigned)second;
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

/*
 * Level
 */

bool Level::is_missing() const { return ltype1 == MISSING_INT && l1 == MISSING_INT && ltype2 == MISSING_INT && l2 == MISSING_INT; }

int Level::compare(const Level& l) const
{
    int res;
    if ((res = ltype1 - l.ltype1)) return res;
    if ((res = l1 - l.l1)) return res;
    if ((res = ltype2 - l.ltype2)) return res;
    return l2 - l.l2;
}

bool Level::operator==(const Level& l) const
{
    return ltype1 == l.ltype1 && l1 == l.l1
        && ltype2 == l.ltype2 && l2 == l.l2;
}

bool Level::operator!=(const Level& l) const
{
    return ltype1 != l.ltype1 || l1 != l.l1
        || ltype2 != l.ltype2 || l2 != l.l2;
}

bool Level::operator<(const Level& l) const { return compare(l) < 0; }
bool Level::operator>(const Level& l) const { return compare(l) > 0; }

namespace {
std::string fmtf( const char* f, ... )
{
    char *c;
    va_list ap;
    va_start( ap, f );
    vasprintf( &c, f, ap );
    std::string ret( c );
    free( c );
    return ret;
}
}

static std::string describe_level(int ltype, int val)
{
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
                  return fmtf("Nominal top of atmosphere, channel %d", val);
        case 9:   return "Sea bottom";
        case 20:  return fmtf("Isothermal level, %.1fK", (double)val/10);
        case 100: return fmtf("Isobaric surface, %.2fhPa", (double)val/100);
        case 101: return "Mean sea level";
        case 102: return fmtf("%.3fm above mean sea level", (double)val/1000);
        case 103: return fmtf("%.3fm above ground", (double)val/1000);
        case 104: return fmtf("Sigma level %.5f", (double)val/10000);
        case 105: return fmtf("Hybrid level %d", val);
        case 106: return fmtf("%.3fm below land surface", (double)val/1000);
        case 107: return fmtf("Isentropic (theta) level, potential temperature %.1fK", (double)val/10);
        case 108: return fmtf("Pressure difference %.2fhPa from ground to level", (double)val/100);
        case 109: return fmtf("Potential vorticity surface %.3f 10-6 K m2 kg-1 s-1", (double)val/1000);
        case 111: return fmtf("ETA* level %.5f", (double)val/10000);
        case 117: return fmtf("Mixed layer depth %.3fm", (double)val/1000);
        case 160: return fmtf("%.3fm below sea level", (double)val/1000);
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
        case 235: return fmtf("Ocean Isotherm Level, %.1fK", (double)val/10); break;
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
                default: return fmtf("%d %d", ltype, val);
            }
            break;
        case 259: return fmtf("Cloud group %d", val);
        case 260: return fmtf("Cloud drift group %d", val);
        case 261: return fmtf("Cloud elevation group %d", val);
        case 262: return "Direction and elevation of clouds";
        case MISSING_INT: return "Information about the station that generated the data";
        default:    return fmtf("%d %d", ltype, val); break;
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

Level Level::cloud(int ltype2, int l2) { return Level(256, MISSING_INT, ltype2, l2); }


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

bool Trange::operator==(const Trange& tr) const
{
    return pind == tr.pind && p1 == tr.p1 && p2 == tr.p2;
}

bool Trange::operator!=(const Trange& tr) const
{
    return pind != tr.pind || p1 != tr.p1 || p2 != tr.p2;
}

bool Trange::operator<(const Trange& t) const { return compare(t) < 0; }
bool Trange::operator>(const Trange& t) const { return compare(t) > 0; }

void Trange::to_stream(std::ostream& out, const char* undef) const
{
    if (pind == MISSING_INT) out << undef; else out << pind;
    out << ",";
    if (p1 == MISSING_INT) out << undef; else out << p1;
    out << ",";
    if (p2 == MISSING_INT) out << undef; else out << p2;
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
        default:  return fmtf("%d %d %d", pind, p1, p2);
    }
}



}
