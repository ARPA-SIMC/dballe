#ifndef DBALLE_DATA_H
#define DBALLE_DATA_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/var.h>
#include <memory>

namespace dballe {

/**
 * Key/value store where keys are strings and values are wreport variables.
 *
 * Keys are defined from a known vocabulary, where each key has an associated
 * wreport::Varinfo structure.
 */
struct Data
{
    virtual ~Data() {}

#if 0
    /// Return a copy of this record
    virtual std::unique_ptr<Data> clone() const = 0;
#endif

    /// Create a new Record
    static std::unique_ptr<Data> create();

    /// Remove all contents from the record
    virtual void clear() = 0;

    /// Remove all Bxxyyy keys from the record, leaving the rest intact
    virtual void clear_vars() = 0;

#if 0
    /// Set year, month, day, hour, min, sec
    virtual void set_datetime(const Datetime& dt) = 0;
    /// Set lat, lon
    virtual void set_coords(const Coords& c) = 0;
    /// Set latmin, latmax
    virtual void set_latrange(const LatRange& lr) = 0;
    /// Set lonmin, lonmax
    virtual void set_lonrange(const LonRange& lr) = 0;
    /// Set datetime-min and datetime-max values
    virtual void set_datetimerange(const DatetimeRange& lr) = 0;
    /// Set leveltype1, l1, leveltype2, l2
    virtual void set_level(const Level& lev) = 0;
    /// Set pindicator, p1, p2
    virtual void set_trange(const Trange& tr) = 0;
    /// Set var.code() == var.value()
    virtual void set_var(const wreport::Var& var) = 0;
    /// Set var.code() == var
    virtual void set_var_acquire(std::unique_ptr<wreport::Var>&& var) = 0;
    /// Set rep_memo, lat, lon, ident
    virtual void set_station(const Station& s) = 0;
    /// Set rep_memo, lat, lon, ident, ana_id
    virtual void set_dbstation(const DBStation& s) = 0;
#endif

    // Uniform set interface
#if 0
    void set(const Datetime& dt) { set_datetime(dt); }
    void set(const DatetimeRange& dt) { set_datetimerange(dt); }
    void set(const Coords& c) { set_coords(c); }
    void set(const LatRange& lr) { set_latrange(lr); }
    void set(const LonRange& lr) { set_lonrange(lr); }
    void set(const Level& lev) { set_level(lev); }
    void set(const Trange& tr) { set_trange(tr); }
    void set(const wreport::Var& var) { set_var(var); }
    void set(std::unique_ptr<wreport::Var>&& var) { set_var_acquire(std::move(var)); }
    void set(const Station& s) { set_station(s); }
    void set(const DBStation& s) { set_dbstation(s); }
#endif
    /// Check if two records are the same
    virtual bool operator==(const Data& rec) const = 0;

    /// Check if two records differ
    virtual bool operator!=(const Data& rec) const = 0;

#if 0
    /// Compose a Coords out of lat, lon values
    virtual Coords get_coords() const = 0;

    /// Compose an Ident out of the ident value
    virtual Ident get_ident() const = 0;

    /// Compose a Level out of the leveltype1...l2 values
    virtual Level get_level() const = 0;

    /// Compose a Trange out of the pindicator...p2 values
    virtual Trange get_trange() const = 0;

    /// Compose a Datetime out of the year...sec values
    virtual Datetime get_datetime() const = 0;

    /// Compose a DatetimeRange out of the yearmin...secmax values
    virtual DatetimeRange get_datetimerange() const = 0;

    /// Compose a Station out of rep_memo, lat, lon, ident
    virtual Station get_station() const = 0;

    /// Compose a DBStation out of rep_memo, lat, lon, ident, ana_id
    virtual DBStation get_dbstation() const = 0;

    /// Generic templatized get
    template<typename T>
    inline T get() const { throw wreport::error_unimplemented(); }

    /// Return a variable if present, or nullptr if not
    virtual const wreport::Var* get_var(wreport::Varcode code) const = 0;
#endif

#if 0
    /**
     * Copy all data from the record source into dest.  At the end of the function,
     * dest will contain its previous values, plus the values in source.  If a
     * value is present both in source and in dest, the one in dest will be
     * overwritten.
     *
     * @param source
     *   The record to copy data from.
     */
    virtual void add(const Record& source) = 0;
#endif

#if 0
    /**
     * Return true if all elements of \a subset are present in this record,
     * with the same value
     */
    virtual bool contains(const Record& subset) const = 0;
#endif

    /// Print the contents of this record to the given stream
    virtual void print(FILE* out) const = 0;
};

#if 0
template<> inline Coords Record::get() const { return get_coords(); }
template<> inline Station Record::get() const { return get_station(); }
template<> inline DBStation Record::get() const { return get_dbstation(); }
template<> inline Ident Record::get() const { return get_ident(); }
template<> inline Level Record::get() const { return get_level(); }
template<> inline Trange Record::get() const { return get_trange(); }
template<> inline Datetime Record::get() const { return get_datetime(); }
template<> inline DatetimeRange Record::get() const { return get_datetimerange(); }
#endif

}
#endif

