#ifndef DBALLE_CORE_VALUES_H
#define DBALLE_CORE_VALUES_H

/** @file
 * Structures used as input to database insert functions.
 */

#include <dballe/core/defs.h>
#include <dballe/core/var.h>
#include <dballe/record.h>
#include <wreport/varinfo.h>
#include <map>

namespace dballe {

/**
 * Information about a station
 */
struct Station
{
    /// rep_memo for this station
    std::string report;

    /**
     * Database ID of the station.
     *
     * It will be filled when the Station is inserted on the database.
     */
    int ana_id = MISSING_INT;

    /// Station coordinates
    Coords coords;

    /// Mobile station identifier
    Ident ident;

    Station() = default;
    Station(const dballe::Record& rec) { set_from_record(rec); }

    /// Reset the database ID
    void clear_ids() { ana_id = MISSING_INT; }

    /// Fill this Station with values from a dballe::Record
    void set_from_record(const Record& rec);

    bool operator==(const Station& o) const
    {
        return report == o.report && ana_id == o.ana_id && coords == o.coords && ident == o.ident;
    }

    /**
     * Print the Station to a FILE*.
     *
     * @param out  The output stream
     * @param end  String to print after the Station
     */
    void print(FILE* out, const char* end="\n") const;
};

/**
 * Information about a physical variable
 */
struct Sampling : public Station
{
    /// Date and time at which the value was measured or forecast
    Datetime datetime;

    /// Vertical level or layer
    Level level;

    /// Time range
    Trange trange;

    Sampling() = default;
    Sampling(const dballe::Record& rec) { set_from_record(rec); }
    void set_from_record(const Record& rec);
    Sampling& operator=(const Sampling&) = default;
    Sampling& operator=(const Station& st) {
        Station::operator=(st);
        return *this;
    }

    bool operator==(const Sampling& o) const
    {
        return Station::operator==(o) && datetime == o.datetime && level == o.level && trange == o.trange;
    }

    /**
     * Print the Sampling contents to a FILE*.
     *
     * @param out  The output stream
     * @param end  String to print after the Station
     */
    void print(FILE* out, const char* end="\n") const;
};

namespace values {

/**
 * A station or measured value
 */
struct Value
{
    /// Database ID of the value
    int data_id = MISSING_INT;

    /// wreport::Var representing the value
    wreport::Var* var = nullptr;

    Value(const Value& o) : data_id(o.data_id), var(o.var ? new wreport::Var(*o.var) : nullptr) {}
    Value(Value&& o) : data_id(o.data_id), var(o.var) { o.var = nullptr; }

    /// Construct from a wreport::Var
    Value(const wreport::Var& var) : var(new wreport::Var(var)) {}

    /// Construct from a wreport::Var, taking ownership of it
    Value(std::unique_ptr<wreport::Var>&& var) : var(var.release()) {}

    ~Value() { delete var; }

    Value& operator=(const Value& o)
    {
        if (this == &o) return *this;
        data_id = o.data_id;
        delete var;
        var = o.var ? new wreport::Var(*o.var) : nullptr;
        return *this;
    }
    Value& operator=(Value&& o)
    {
        if (this == &o) return *this;
        data_id = o.data_id;
        delete var;
        var = o.var;
        o.var = nullptr;
        return *this;
    }
    bool operator==(const Value& o) const
    {
        if (data_id != o.data_id) return false;
        if (var == o.var) return true;
        if (!var || !o.var) return false;
        return *var == *o.var;
    }

    /// Reset the database ID
    void clear_ids() { data_id = MISSING_INT; }

    /// Fill from a wreport::Var
    void set(const wreport::Var& v)
    {
        delete var;
        var = new wreport::Var(v);
    }

    /// Fill from a wreport::Var, taking ownership of it
    void set(std::unique_ptr<wreport::Var>&& v)
    {
        delete var;
        var = v.release();
    }

    /// Print the contents of this Value
    void print(FILE* out) const;
};
}

/**
 * Collection of Value objects, indexed by wreport::Varcode
 */
struct Values : protected std::map<wreport::Varcode, values::Value>
{
    Values() = default;
    Values(const dballe::Record& rec) { set_from_record(rec); }

    typedef std::map<wreport::Varcode, values::Value>::const_iterator const_iterator;
    typedef std::map<wreport::Varcode, values::Value>::iterator iterator;
    const_iterator begin() const { return std::map<wreport::Varcode, values::Value>::begin(); }
    const_iterator end() const { return std::map<wreport::Varcode, values::Value>::end(); }
    iterator begin() { return std::map<wreport::Varcode, values::Value>::begin(); }
    iterator end() { return std::map<wreport::Varcode, values::Value>::end(); }
    size_t size() const { return std::map<wreport::Varcode, values::Value>::size(); }
    bool empty() const { return std::map<wreport::Varcode, values::Value>::empty(); }
    void clear() { return std::map<wreport::Varcode, values::Value>::clear(); }
    bool operator==(const Values& o) const;

    const values::Value& operator[](wreport::Varcode code) const;
    const values::Value& operator[](const char* code) const { return operator[](resolve_varcode(code)); }
    const values::Value& operator[](const std::string& code) const { return operator[](resolve_varcode(code)); }
    const values::Value* get(wreport::Varcode code) const;
    const values::Value* get(const char* code) const { return get(resolve_varcode(code)); }
    const values::Value* get(const std::string& code) const { return get(resolve_varcode(code)); }

    /// Set from a wreport::Var
    void set(const wreport::Var&);

    /// Set from a wreport::Var, taking ownership of it
    void set(std::unique_ptr<wreport::Var>&&);

    /// Set from a variable created by dballe::newvar()
    template<typename C, typename T> void set(C code, const T& val) { this->set(newvar(code, val)); }

    /// Set the database ID for the Value with this wreport::Varcode
    void add_data_id(wreport::Varcode code, int data_id);

    /// Set from the contents of a dballe::Record
    void set_from_record(const Record& rec);

    /// Reset all the database IDs
    void clear_ids()
    {
        for (auto& i : *this)
            i.second.clear_ids();
    }

    /// Print the contents of this Values
    void print(FILE* out) const;
};

/**
 * A set of station values.
 */
struct StationValues
{
    Station info;
    Values values;

    StationValues() = default;
    StationValues(const dballe::Record& rec) : info(rec), values(rec) {}

    /// Set from the contents of a dballe::Record
    void set_from_record(const Record& rec);

    bool operator==(const StationValues& o) const
    {
        return info == o.info && values == o.values;
    }

    /// Reset all the database IDs
    void clear_ids()
    {
        info.clear_ids();
        values.clear_ids();
    }

    /// Print the contents of this StationValues
    void print(FILE* out) const;
};

/**
 * A set of measured values
 */
struct DataValues
{
    Sampling info;
    Values values;

    DataValues() = default;
    DataValues(const dballe::Record& rec) : info(rec), values(rec) {}

    /// Set from the contents of a dballe::Record
    void set_from_record(const Record& rec);

    bool operator==(const DataValues& o) const
    {
        return info == o.info && values == o.values;
    }

    /// Reset all the database IDs
    void clear_ids()
    {
        info.clear_ids();
        values.clear_ids();
    }

    /// Print the contents of this StationValues
    void print(FILE* out) const;
};

}

#endif
