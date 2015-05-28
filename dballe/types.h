#ifndef DBALLE_TYPES_H
#define DBALLE_TYPES_H

/** @file
 * Common base types used by most of DB-All.e code.
 */

#include <iosfwd>

namespace dballe {

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
    void to_iso8601(std::ostream& out, char sep='T', const char* tz="") const;

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


}

#endif
