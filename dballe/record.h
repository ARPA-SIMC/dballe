#ifndef DBALLE_RECORD_H
#define DBALLE_RECORD_H

#include <wreport/var.h>
#include <dballe/types.h>
#include <memory>

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

    /// Remove/unset a key from the record
    virtual void unset(const char* key) = 0;

    /// Get a value, if set, or nullptr if not
    virtual const wreport::Var* get(const char* key) const = 0;

    /// Check if a value is set
    virtual bool isset(const char* key) const;

    /// Check if two records are the same
    virtual bool equals(const Record& rec) const = 0;

    /// Check if two records are the same
    bool operator==(const Record& rec) const;

    /// Check if two records differ
    bool operator!=(const Record& rec) const;

    /// Get a value, if set, or throw an exception if not
    const wreport::Var& operator[](const char* key) const;

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

    /**
     * Generate a sequence of key names and const Var& for all the
     * contents of the record
     */
    void foreach_key(std::function<void(const char*, const wreport::Var&)> dest) const { foreach_key_ref(dest); }

    /**
     * Generate a sequence of key names and unique_ptr<Var> for all the
     * contents of the record
     */
    void foreach_key(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const { foreach_key_copy(dest); }

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
    virtual void foreach_key_ref(std::function<void(const char*, const wreport::Var&)> dest) const = 0;
    virtual void foreach_key_copy(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const = 0;
};

}
#endif
