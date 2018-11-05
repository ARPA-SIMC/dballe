#ifndef DBALLE_RECORD_H
#define DBALLE_RECORD_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/var.h>
#include <memory>
#include <functional>

namespace dballe {

/**
 * Key/value store where keys are strings and values are wreport variables.
 *
 * Keys are defined from a known vocabulary, where each key has an associated
 * wreport::Varinfo structure.
 */
struct Record
{
    virtual ~Record() {}

    /// Return a copy of this record
    virtual std::unique_ptr<Record> clone() const = 0;

    /// Create a new Record
    static std::unique_ptr<Record> create();

    /// Remove all contents from the record
    virtual void clear() = 0;

    /// Remove all Bxxyyy keys from the record, leaving the rest intact
    virtual void clear_vars() = 0;

    /**
     * Set a key to an integer value.
     *
     * If the key that is being set has a decimal component (like lat and lon),
     * the integer value represents the units of maximum precision of the
     * field. For example, using seti to set lat to 4500000 is the same as
     * setting it to 45.0.
     */
    virtual void seti(const char* key, int val) = 0;

    /**
     * Set a key to a double value.
     */
    virtual void setd(const char* key, double val) = 0;

    /**
     * Set a key to a string value.
     *
     * If the key that is being set has a decimal component (like lat and lon),
     * the string is converted to an integer value representing the units of
     * maximum precision of the field. For example, using seti to set lat to
     * "4500000" is the same as setting it to 45.0.
     */
    virtual void setc(const char* key, const char* val) = 0;

    /**
     * Set a key to a string value.
     *
     * If the key that is being set has a decimal component (like lat and lon),
     * the string is converted to an integer value representing the units of
     * maximum precision of the field. For example, using seti to set lat to
     * "4500000" is the same as setting it to 45.0.
     */
    virtual void sets(const char* key, const std::string& val) = 0;

    /**
     * Set a key to a string value.
     *
     * Contrarily to setc, the string is parsed according to the natural
     * representation for the given key. For example, if lat is set to "45",
     * then it gets the value 45.0.
     *
     * Also, if a Decimal or Integer value is assigned "-", it is unset
     * instead.
     */
    virtual void setf(const char* key, const char* val) = 0;

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

    void set(const char* key, int val) { seti(key, val); }
    void set(const char* key, double val) { setd(key, val); }
    void set(const char* key, const char* val) { setc(key, val); }
    void set(const char* key, const std::string& val) { sets(key, val); }
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

    /// Remove/unset a key from the record
    virtual void unset(const char* key) = 0;

    /// Check if a value is set
    virtual bool isset(const char* key) const;

    /// Check if two records are the same
    virtual bool equals(const Record& rec) const = 0;

    /// Check if two records are the same
    bool operator==(const Record& rec) const;

    /// Check if two records differ
    bool operator!=(const Record& rec) const;

    const char* enq(const char* key, const char* def) const
    {
        if (const wreport::Var* var = get(key))
            return var->enq(def);
        return def;
    }

    template<typename T>
    T enq(const char* key, const T& def) const
    {
        if (const wreport::Var* var = get(key))
            return var->enq(def);
        return def;
    }

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

    /**
     * Return true if all elements of \a subset are present in this record,
     * with the same value
     */
    virtual bool contains(const Record& subset) const = 0;

    /// Print the contents of this record to the given stream
    virtual void print(FILE* out) const = 0;

    /**
     * Return informations about a key
     *
     * @return
     *   The wreport::Varinfo structure corresponding to the key
     */
    static wreport::Varinfo key_info(const char* key);

    /**
     * Return informations about a key
     *
     * @return
     *   The wreport::Varinfo structure corresponding to the key
     */
    static wreport::Varinfo key_info(const std::string& key);

protected:
    /// Get a value, if set, or throw an exception if not
    const wreport::Var& operator[](const char* key) const;

    /// Get a value, if set, or nullptr if not
    virtual const wreport::Var* get(const char* key) const = 0;
};

template<> inline Coords Record::get() const { return get_coords(); }
template<> inline Station Record::get() const { return get_station(); }
template<> inline DBStation Record::get() const { return get_dbstation(); }
template<> inline Ident Record::get() const { return get_ident(); }
template<> inline Level Record::get() const { return get_level(); }
template<> inline Trange Record::get() const { return get_trange(); }
template<> inline Datetime Record::get() const { return get_datetime(); }
template<> inline DatetimeRange Record::get() const { return get_datetimerange(); }

}
#endif
