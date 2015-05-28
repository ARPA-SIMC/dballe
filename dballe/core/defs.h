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

std::ostream& operator<<(std::ostream& out, const Coords& c);

std::ostream& operator<<(std::ostream& out, const Date& dt);

std::ostream& operator<<(std::ostream& out, const Time& t);

std::ostream& operator<<(std::ostream& out, const Datetime& dt);

}

// vim:set ts=4 sw=4:
#endif
