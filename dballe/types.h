#ifndef DBALLE_TYPES_H
#define DBALLE_TYPES_H

/** @file
 * Common base types used by most of DB-All.e code.
 */

#include <iosfwd>
#include <limits.h>

namespace dballe {

/**
 * Value to use for missing parts of level and time range values
 */
static const int MISSING_INT = INT_MAX;

/**
 * Calendar date.
 *
 * If year is 0xffff, then all the date is considered missing. Else, all fields
 * must be set.
 */
struct Date
{
    unsigned short year;
    unsigned char month;
    unsigned char day;

    /// Construct a missing date
    Date();

    /**
     * Construct from broken down values.
     *
     * Missing values will be replaced with lower bounds.
     *
     * Invalid values will be constrained within the nearest valid boundary.
     */
    Date(int ye, int mo=1, int da=1);

    /// Copy constructor
    Date(const Date& d) = default;

    /// Create a date from a Julian day
    static Date from_julian(int jday);

    /// Check if this date is the missing value
    bool is_missing() const;

    /// Convert the date to Julian day
    int to_julian() const;

    /**
     * Write the date to an output stream in ISO8601 date format.
     */
    void to_stream_iso8601(std::ostream& out) const;

    /**
     * Generic comparison
     *
     * Returns a negative number if *this < other
     * Returns zero if *this == other
     * Returns a positive number if *this > other
     */
    int compare(const Date& other) const;

    bool operator<(const Date& dt) const;
    bool operator>(const Date& dt) const;
    bool operator==(const Date& dt) const;
    bool operator!=(const Date& dt) const;

    static int days_in_month(int year, int month);
    static int calendar_to_julian(int year, int month, int day);
    static void julian_to_calendar(int jday, unsigned short& year, unsigned char& month, unsigned char& day);
};


/**
 * Time of the day.
 *
 * If hour is 0xff, then all the time is considered missing. Else, all fields
 * must be set.
 */
struct Time
{
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    /// Construct a missing time
    Time();

    /**
     * Construct from broken down values.
     *
     * Missing values will be replaced with lower bounds.
     *
     * Invalid values will be constrained within the nearest valid boundary.
     */
    Time(int ho, int mi=0, int se=0);

    Time(const Time& t) = default;

    /// Check if this time is the missing value
    bool is_missing() const;

    /**
     * Write the time to an output stream in ISO8601 extended format
     * (hh:mm:ss).
     */
    void to_stream_iso8601(std::ostream& out) const;

    /**
     * Generic comparison
     *
     * Returns a negative number if *this < other
     * Returns zero if *this == other
     * Returns a positive number if *this > other
     */
    int compare(const Time& other) const;

    bool operator<(const Time& dt) const;
    bool operator>(const Time& dt) const;
    bool operator==(const Time& dt) const;
    bool operator!=(const Time& dt) const;
};


/**
 * Date and time
 *
 * If year is 0xffff, then all the datetime is considered missing. Else, all
 * fields must be set.
 */
struct Datetime
{
    unsigned short year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    /// Construct a missing datetime
    Datetime();
    Datetime(const Date& date, const Time& time);

    /**
     * Construct from broken down values.
     *
     * Missing values will be replaced with lower bounds.
     *
     * Invalid values will be constrained within the nearest valid boundary.
     */
    Datetime(int ye, int mo=1, int da=1, int ho=0, int mi=0, int se=0);

    /// Set the date from a Julian day
    static Datetime from_julian(int jday, int ho=0, int mi=0, int se=0);

    /**
     * Return a Datetime filling the parts set to MISSING_INT with their lowest
     * possible values
     */
    static Datetime lower_bound(int ye, int mo, int da, int ho, int mi, int se);

    /**
     * Return a Datetime filling the parts set to MISSING_INT with their
     * highest possible values
     */
    static Datetime upper_bound(int ye, int mo, int da, int ho, int mi, int se);

    /// Return a Date with this date
    Date date() const;

    /// Return a Time with this time
    Time time() const;

    /// Check if this datetime is the missing value
    bool is_missing() const;

    /// Convert the date to Julian day
    int to_julian() const;

    /**
     * Write the datetime to an output stream in ISO8601 combined format.
     *
     * @param sep the separator character between date and time
     * @param tz the timezone string to use at the end of the datetime
     */
    void to_stream_iso8601(std::ostream& out, char sep='T', const char* tz="") const;

    /**
     * Generic comparison
     *
     * Returns a negative number if *this < other
     * Returns zero if *this == other
     * Returns a positive number if *this > other
     */
    int compare(const Datetime& other) const;

    bool operator==(const Datetime& dt) const;
    bool operator!=(const Datetime& dt) const;
    bool operator<(const Datetime& dt) const;
    bool operator>(const Datetime& dt) const;
    bool operator<=(const Datetime& dt) const;
    bool operator>=(const Datetime& dt) const;

    /**
     * Parse an ISO8601 datetime string.
     *
     * Both 'T' and ' ' are allowed as separators.
     */
    static Datetime from_iso8601(const char* str);
};


/**
 * Range of datetimes.
 *
 * The range includes the extremes. A missing extreme in the range means an
 * open ended range.
 */
struct DatetimeRange
{
    /// Lower bound of the range
    Datetime min;
    /// Upper bound of the range
    Datetime max;

    DatetimeRange() = default;
    DatetimeRange(const Datetime& dt) : min(dt), max(dt) {}
    DatetimeRange(const Datetime& min, const Datetime& max) : min(min), max(max) {}
    DatetimeRange(
            int yemin, int momin, int damin, int homin, int mimin, int semin,
            int yemax, int momax, int damax, int homax, int mimax, int semax);

    /// Check if this range is open on both sides
    bool is_missing() const;

    bool operator==(const DatetimeRange& dtr) const;
    bool operator!=(const DatetimeRange& dtr) const;

    /// Set the extremes
    void set(const Datetime& min, const Datetime& max);

    /**
     * Set the extremes from broken down components.
     *
     * If yemin or yemax are MISSING_INT, they are taken as an open ended range
     * boundary.
     *
     * If any other *min values are MISSING_INT, they are filled with the
     * lowest possible valid value they can have.
     *
     * If any other *max values are MISSING_INT, they are filled with the
     * highest possible valid value they can have.
     */
    void set(int yemin, int momin, int damin, int homin, int mimin, int semin,
             int yemax, int momax, int damax, int homax, int mimax, int semax);

    /**
     * Merge \a range into this one, resulting in the smallest range that
     * contains both.
     */
    void merge(const DatetimeRange& range);

    /// Check if a Datetime is inside this range
    bool contains(const Datetime& dt) const;

    /// Check if a range is inside this range (extremes included)
    bool contains(const DatetimeRange& dtr) const;

    /// Check if the two ranges are completely disjoint
    bool is_disjoint(const DatetimeRange& dtr) const;
};


/**
 * Range of latitudes.
 *
 * When given as an integer, a latitude value is intended in 1/100000 of a
 * degree, which is the maximum resolution supported by DB-All.e.
 *
 * When given as a double a latitude value is intended to be in degrees.
 *
 * Values are matched between imin and imax, both extremes are considered part
 * of the range.
 *
 * Invariant: imin <= imax.
 */
struct LatRange
{
    /// Minimum possible integer value
    static const int IMIN = -9000000;
    /// Maximum possible integer value
    static const int IMAX = 9000000;
    /// Minimum possible double value
    static constexpr double DMIN = -90.0;
    /// Maximum possible double value
    static constexpr double DMAX = 90.0;

    /// Minimum latitude
    int imin = IMIN;
    /// Maximum latitude
    int imax = IMAX;

    /// Construct a LatRange matching any latitude
    LatRange() = default;
    /// Construct a LatRange given integer extremes
    LatRange(int min, int max);
    /// Construct a LatRange given extremes in degrees
    LatRange(double min, double max);

    bool operator==(const LatRange& lr) const;
    bool operator!=(const LatRange& lr) const;

    /// Return true if the LatRange matches any latitude
    bool is_missing() const;

    /// Get the extremes as double
    void get(double& min, double& max) const;

    /// Set the extremes as integers
    void set(int min, int max);

    /// Set the extremes in degrees
    void set(double min, double max);

    /// Check if a point is inside this range (extremes included)
    bool contains(int lat) const;

    /// Check if a point is inside this range (extremes included)
    bool contains(double lat) const;

    /// Check if a range is inside this range (extremes included)
    bool contains(const LatRange& lr) const;
};


/**
 * Range of longitudes.
 *
 * When given as an integer, a longitude value is intended in 1/100000 of a
 * degree, which is the maximum resolution supported by DB-All.e.
 *
 * When given as a double a longitude value is intended to be in degrees.
 *
 * Longitude values are normalized between -180.0 and 180.0. The range is the
 * angle that goes from imin to imax. Both extremes are considered part of the
 * range.
 *
 * A range that matches any longitude has both imin and imax set to
 * MISSING_INT.
 *
 * Invariant: if imin == MISSING_INT, then imax == MISSING_INT. An open-ended
 * longitude range makes no sense, since longitudes move alongide a closed
 * circle.
 */
struct LonRange
{
    /// Initial point of the longitude range
    int imin = MISSING_INT;
    /// Final point of the longitude range
    int imax = MISSING_INT;

    /// Construct a range that matches any longitude
    LonRange() = default;
    /// Construct a range given integer extremes
    LonRange(int min, int max);
    /// Construct a range given extremes in degrees
    LonRange(double min, double max);

    bool operator==(const LonRange& lr) const;
    bool operator!=(const LonRange& lr) const;

    /// Return true if the LonRange matches any longitude
    bool is_missing() const;

    /**
     * Get the extremes in degrees.
     *
     * If is_missing() == true, it returns the values -180.0 and 180.0
     */
    void get(double& min, double& max) const;

    /// Set the extremes as integers
    void set(int min, int max);

    /// Set the extremes in degrees
    void set(double min, double max);

    /// Check if a point is inside this range (extremes included)
    bool contains(int lon) const;

    /// Check if a point is inside this range (extremes included)
    bool contains(double lon) const;

    /// Check if a range is inside this range (extremes included)
    bool contains(const LonRange& lr) const;
};


/// Vertical level or layer
struct Level
{
    /// Type of the level or the first layer. See @ref level_table.
    int ltype1;
    /// L1 value of the level or the first layer. See @ref level_table.
    int l1;
    /// Type of the the second layer. See @ref level_table.
    int ltype2;
    /// L2 value of the second layer. See @ref level_table.
    int l2;

    Level(int ltype1=MISSING_INT, int l1=MISSING_INT, int ltype2=MISSING_INT, int l2=MISSING_INT)
        : ltype1(ltype1), l1(l1), ltype2(ltype2), l2(l2) {}

    /// Check if this level is fully set to the missing value
    bool is_missing() const;

    bool operator==(const Level& l) const;
    bool operator!=(const Level& l) const;
    bool operator<(const Level& l) const;
    bool operator>(const Level& l) const;

    /**
     * Generic comparison
     *
     * Returns a negative number if *this < other
     * Returns zero if *this == other
     * Returns a positive number if *this > other
     */
    int compare(const Level& l) const;

    /**
     * Return a string description of this level
     */
    std::string describe() const;

    /// Format to an output stream
    void to_stream(std::ostream& out, const char* undef="-") const;

    /// Create a cloud special level. See @ref level_table.
    static Level cloud(int ltype2=MISSING_INT, int l2=MISSING_INT);
};


/**
 * Information on how a value has been sampled or computed with regards to time.
 */
struct Trange
{
    /// Time range type indicator.  See @ref trange_table.
    int pind;
    /// Time range P1 indicator.  See @ref trange_table.
    int p1;
    /// Time range P2 indicator.  See @ref trange_table.
    int p2;

    Trange(int pind=MISSING_INT, int p1=MISSING_INT, int p2=MISSING_INT)
        : pind(pind), p1(p1), p2(p2) {}

    /// Check if this level is fully set to the missing value
    bool is_missing() const;

    /**
     * Generic comparison
     *
     * Returns a negative number if *this < other
     * Returns zero if *this == other
     * Returns a positive number if *this > other
     */
    int compare(const Trange& t) const;

    bool operator==(const Trange& tr) const;
    bool operator!=(const Trange& tr) const;
    bool operator<(const Trange& t) const;
    bool operator>(const Trange& t) const;

    /**
     * Return a string description of this time range
     */
    std::string describe() const;

    /// Format to an output stream
    void to_stream(std::ostream& out, const char* undef="-") const;

    /// Time range for instant values
    static Trange instant();
};



}

#endif
