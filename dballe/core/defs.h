#ifndef DBA_MSG_DEFS_H
#define DBA_MSG_DEFS_H

/** @file
 * Common definitions
 */

#include <dballe/types.h>
#include <limits.h>
#include <string>
#include <iosfwd>

namespace dballe {

/**
 * Supported encodings
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2,
} Encoding;

const char* encoding_name(Encoding enc);
Encoding parse_encoding(const std::string& s);

std::ostream& operator<<(std::ostream& out, const Level& l);

std::ostream& operator<<(std::ostream& out, const Trange& l);

/// Coordinates
struct Coords
{
    /// Latitude multiplied by 100000 (5 significant digits preserved)
    int lat = MISSING_INT;
    /// Longitude normalised from -180.0 to 180.0 and multiplied by 100000 (5
    /// significant digits preserved) 
    int lon = MISSING_INT;

    Coords() {}
    Coords(int lat, int lon);
    Coords(double lat, double lon);

    bool is_missing() const { return lat == MISSING_INT && lon == MISSING_INT; }

    void set_lat(int lat);
    void set_lon(int lat);
    void set_lat(double lat);
    void set_lon(double lat);

    double dlat() const;
    double dlon() const;

    bool operator<(const Coords& o) const { return compare(o) < 0; }
    bool operator>(const Coords& o) const { return compare(o) > 0; }

    bool operator==(const Coords& c) const
    {
        return lat == c.lat && lon == c.lon;
    }

    bool operator!=(const Coords& c) const
    {
        return lat != c.lat || lon != c.lon;
    }

    /**
     * Compare two Coords strutures, for use in sorting.
     *
     * @return
     *   -1 if *this < o, 0 if *this == o, 1 if *this > o
     */
    int compare(const Coords& o) const
    {
        if (int res = lat - o.lat) return res;
        return lon - o.lon;
    }

    // Normalise longitude values to the [-180..180[ interval
    static int normalon(int lon);
    static double fnormalon(double lon);
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
    static const int IMIN = -9000000;
    static const int IMAX = 9000000;
    static constexpr double DMIN = -90.0;
    static constexpr double DMAX = 90.0;

    /// Minimum latitude
    int imin = IMIN;
    /// Maximum latitude
    int imax = IMAX;

    LatRange() = default;
    LatRange(int min, int max) : imin(min), imax(max) {}
    LatRange(double min, double max);

    bool operator==(const LatRange& lr) const
    {
        return imin == lr.imin && imax == lr.imax;
    }

    bool operator!=(const LatRange& lr) const
    {
        return imin != lr.imin || imax != lr.imax;
    }

    bool is_missing() const;

    void get(double& min, double& max) const;
    void set(int min, int max);
    void set(double min, double max);

    bool contains(int lat) const;
    bool contains(double lat) const;
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
    int imin = MISSING_INT;
    int imax = MISSING_INT;

    LonRange() = default;
    LonRange(int min, int max);
    LonRange(double min, double max);

    bool operator==(const LonRange& lr) const
    {
        return imin == lr.imin && imax == lr.imax;
    }

    bool operator!=(const LonRange& lr) const
    {
        return imin != lr.imin || imax != lr.imax;
    }

    bool is_missing() const;

    /// If is_missing, returns -180.0, 180.0
    void get(double& min, double& max) const;
    void set(int min, int max);
    void set(double min, double max);

    bool contains(int lon) const;
    bool contains(double lon) const;
};


std::ostream& operator<<(std::ostream& out, const Coords& c);
std::ostream& operator<<(std::ostream& out, const Date& dt);
std::ostream& operator<<(std::ostream& out, const Time& t);
std::ostream& operator<<(std::ostream& out, const Datetime& dt);
std::ostream& operator<<(std::ostream& out, const LatRange& lr);
std::ostream& operator<<(std::ostream& out, const LonRange& lr);

}

// vim:set ts=4 sw=4:
#endif
