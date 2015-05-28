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

/**
 * Value to use for missing levels and time range components
 */
static const int MISSING_INT = INT_MAX;

struct Level
{
    /** Type of the first level.  See @ref level_table. */
    int ltype1;
    /** L1 value of the level.  See @ref level_table. */
    int l1;
    /** Type of the second level.  See @ref level_table. */
    int ltype2;
    /** L2 value of the level.  See @ref level_table. */
    int l2;

    Level(int ltype1=MISSING_INT, int l1=MISSING_INT, int ltype2=MISSING_INT, int l2=MISSING_INT)
        : ltype1(ltype1), l1(l1), ltype2(ltype2), l2(l2) {}
    Level(const char* ltype1, const char* l1=NULL, const char* ltype2=NULL, const char* l2=NULL);

    bool is_missing() const { return ltype1 == MISSING_INT && l1 == MISSING_INT && ltype2 == MISSING_INT && l2 == MISSING_INT; }

    bool operator==(const Level& l) const
    {
        return ltype1 == l.ltype1 && l1 == l.l1
            && ltype2 == l.ltype2 && l2 == l.l2;
    }

    bool operator!=(const Level& l) const
    {
        return ltype1 != l.ltype1 || l1 != l.l1
            || ltype2 != l.ltype2 || l2 != l.l2;
    }

    bool operator<(const Level& l) const { return compare(l) < 0; }
    bool operator>(const Level& l) const { return compare(l) > 0; }

    /**
     * Compare two Level strutures, for use in sorting.
     *
     * @return
     *   -1 if *this < l, 0 if *this == l, 1 if *this > l
     */
    int compare(const Level& l) const
    {
        int res;
        if ((res = ltype1 - l.ltype1)) return res;
        if ((res = l1 - l.l1)) return res;
        if ((res = ltype2 - l.ltype2)) return res;
        return l2 - l.l2;
    }

    /**
     * Return a string description of this level
     */
    std::string describe() const;

    void format(std::ostream& out, const char* undef="-") const;

    static inline Level cloud(int ltype2=MISSING_INT, int l2=MISSING_INT) { return Level(256, MISSING_INT, ltype2, l2); }
    static inline Level waves(int ltype2=MISSING_INT, int l2=MISSING_INT) { return Level(264, MISSING_INT, ltype2, l2); }
    static inline Level ana() { return Level(); }
};

std::ostream& operator<<(std::ostream& out, const Level& l);

struct Trange
{
    /** Time range type indicator.  See @ref trange_table. */
    int pind;
    /** Time range P1 indicator.  See @ref trange_table. */
    int p1;
    /** Time range P2 indicator.  See @ref trange_table. */
    int p2;

    Trange(int pind=MISSING_INT, int p1=MISSING_INT, int p2=MISSING_INT)
        : pind(pind), p1(p1), p2(p2) {}
    Trange(const char* pind, const char* p1=NULL, const char* p2=NULL);

    bool is_missing() const { return pind == MISSING_INT && p1 == MISSING_INT && p2 == MISSING_INT; }

    bool operator==(const Trange& tr) const
    {
        return pind == tr.pind && p1 == tr.p1 && p2 == tr.p2;
    }

    bool operator!=(const Trange& tr) const
    {
        return pind != tr.pind || p1 != tr.p1 || p2 != tr.p2;
    }

    bool operator<(const Trange& t) const { return compare(t) < 0; }
    bool operator>(const Trange& t) const { return compare(t) > 0; }

    /**
     * Compare two Trange strutures, for use in sorting.
     *
     * @return
     *   -1 if *this < t, 0 if *this == t, 1 if *this > t
     */
    int compare(const Trange& t) const
    {
        int res;
        if ((res = pind - t.pind)) return res;
        if ((res = p1 - t.p1)) return res;
        return p2 - t.p2;
    }

    /**
     * Return a string description of this time range
     */
    std::string describe() const;

    void format(std::ostream& out, const char* undef="-") const;

    static inline Trange instant() { return Trange(254, 0, 0); }
    static inline Trange ana() { return Trange(); }
};

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
