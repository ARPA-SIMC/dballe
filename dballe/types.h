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

/// Calendar date
struct Date
{
    unsigned short year = 0xffff;
    unsigned char month = 0xff;
    unsigned char day = 0xff;

    Date() = default;
    Date(unsigned short year, unsigned char month=1, unsigned char day=1)
        : year(year), month(month), day(day) {}
    Date(const Date& d) = default;

    /// Check if this date is the missing value
    bool is_missing() const;

    /// Convert the date to Julian day
    int to_julian() const;

    /// Set the date from a Julian day
    void from_julian(int jday);

    /// Set the date from an array of 3 integers
    void from_array(const int* vals);

    /// Copy the date to an array of 3 integers
    void to_array(int* vals) const;

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


/// Time of the day
struct Time
{
    unsigned char hour = 0xff;
    unsigned char minute = 0xff;
    unsigned char second = 0xff;

    Time() = default;
    Time(unsigned char hour, unsigned char minute=0, unsigned char second=0)
        : hour(hour), minute(minute), second(second) {}
    Time(const Time& d) = default;

    /// Check if this time is the missing value
    bool is_missing() const;

    /// Set the time from an array of 3 integers
    void from_array(const int* vals);

    /// Copy the time to an array of 3 integers
    void to_array(int* vals) const;

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


/// Date and time
struct Datetime
{
    unsigned short year = 0xffff;
    unsigned char month = 0xff;
    unsigned char day = 0xff;
    unsigned char hour = 0xff;
    unsigned char minute = 0xff;
    unsigned char second = 0xff;

    Datetime() = default;
    Datetime(const Date& date, const Time& time)
        : year(date.year), month(date.month), day(date.day),
          hour(time.hour), minute(time.minute), second(time.second) {}
    Datetime(unsigned short year, unsigned char month=1, unsigned char day=1,
             unsigned char hour=0, unsigned char minute=0, unsigned char second=0)
        : year(year), month(month), day(day),
          hour(hour), minute(minute), second(second) {}

    /// Return a Date with this date
    Date date() const;

    /// Return a Time with this time
    Time time() const;

    /**
     * Return a new datetime, same as this one but with missing fields filled
     * with the lowest valid values. If the datetime is missing, it returns a
     * missing datetime.
     */
    Datetime lower_bound() const;

    /**
     * Return a new datetime, same as this one but with missing fields filled
     * with the highest valid values. If the datetime is missing, it returns a
     * missing datetime.
     */
    Datetime upper_bound() const;

    /// Check if this datetime is the missing value
    bool is_missing() const;

    /// Convert the date to Julian day
    int to_julian() const;

    /// Set the date from a Julian day
    void from_julian(int jday);

    /// Set the time from an array of 6 integers
    void from_array(const int* vals);

    /// Copy the time to an array of 6 integers
    void to_array(int* vals) const;

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

    /// Check if the two ranges are the same
    static bool range_equals(
            const Datetime& begin1, const Datetime& until1,
            const Datetime& begin2, const Datetime& until2);

    /**
     * Checks if the interval [begin1, end1] contains [begin2, end2] or if the
     * two intervals are the same
     */
    static bool range_contains(
            const Datetime& begin1, const Datetime& until1,
            const Datetime& begin2, const Datetime& until2);

    /// Check if the two ranges are completely disjoint
    static bool range_disjoint(
            const Datetime& begin1, const Datetime& until1,
            const Datetime& begin2, const Datetime& until2);

    /**
     * Parse an ISO8601 datetime string.
     *
     * Both 'T' and ' ' are allowed as separators.
     */
    static Datetime from_iso8601(const char* str);
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
