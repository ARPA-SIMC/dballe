#ifndef DBALLE_CORE_VALUES_H
#define DBALLE_CORE_VALUES_H

#include <dballe/core/defs.h>
#include <dballe/core/var.h>
#include <dballe/record.h>
#include <wreport/varinfo.h>
#include <map>

namespace dballe {

struct Station
{
    std::string report;
    int ana_id = MISSING_INT;
    Coords coords;
    Ident ident;

    Station() = default;
    Station(const dballe::Record& rec) { from_record(rec); }
    void clear_ids() { ana_id = MISSING_INT; }
    void from_record(const Record& rec);
    bool operator==(const Station& o) const
    {
        return report == o.report && ana_id == o.ana_id && coords == o.coords && ident == o.ident;
    }
};

struct Sampling : public Station
{
    Datetime datetime;
    Level level;
    Trange trange;

    Sampling() = default;
    Sampling(const dballe::Record& rec) { from_record(rec); }
    void from_record(const Record& rec);
    Sampling& operator=(const Sampling&) = default;
    Sampling& operator=(const Station& st) {
        Station::operator=(st);
        return *this;
    }

    bool operator==(const Sampling& o) const
    {
        return Station::operator==(o) && datetime == o.datetime && level == o.level && trange == o.trange;
    }
};

namespace values {
struct Value
{
    int data_id = MISSING_INT;
    wreport::Var* var = nullptr;

    Value(const Value& o) : data_id(o.data_id), var(o.var ? new wreport::Var(*o.var) : nullptr) {}
    Value(Value&& o) : data_id(o.data_id), var(o.var) { o.var = nullptr; }
    Value(const wreport::Var& var) : var(new wreport::Var(var)) {}
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
    void clear_ids() { data_id = MISSING_INT; }
    void set(const wreport::Var& v)
    {
        delete var;
        var = new wreport::Var(v);
    }
    void set(std::unique_ptr<wreport::Var>&& v)
    {
        delete var;
        var = v.release();
    }

};
}

// FIXME: map, or hashmap, or vector enforced to be unique
struct Values : protected std::map<wreport::Varcode, values::Value>
{
    Values() = default;
    Values(const dballe::Record& rec) { from_record(rec); }

    typedef std::map<wreport::Varcode, values::Value>::const_iterator const_iterator;
    typedef std::map<wreport::Varcode, values::Value>::iterator iterator;
    const_iterator begin() const { return std::map<wreport::Varcode, values::Value>::begin(); }
    const_iterator end() const { return std::map<wreport::Varcode, values::Value>::end(); }
    iterator begin() { return std::map<wreport::Varcode, values::Value>::begin(); }
    iterator end() { return std::map<wreport::Varcode, values::Value>::end(); }
    size_t size() const { return std::map<wreport::Varcode, values::Value>::size(); }
    bool empty() const { return std::map<wreport::Varcode, values::Value>::empty(); }
    bool operator==(const Values& o) const;

    const values::Value& operator[](wreport::Varcode code) const;
    const values::Value& operator[](const char* code) const { return operator[](resolve_varcode_safe(code)); }
    const values::Value& operator[](const std::string& code) const { return operator[](resolve_varcode_safe(code)); }
    const values::Value* get(wreport::Varcode code) const;
    const values::Value* get(const char* code) const { return get(resolve_varcode_safe(code)); }
    const values::Value* get(const std::string& code) const { return get(resolve_varcode_safe(code)); }
    void set(const wreport::Var&);
    void set(std::unique_ptr<wreport::Var>&&);
    template<typename C, typename T> void set(C code, const T& val) { this->set(newvar(code, val)); }
    void add_data_id(wreport::Varcode code, int data_id);
    void from_record(const Record& rec);
    void clear_ids()
    {
        for (auto& i : *this)
            i.second.clear_ids();
    }
};

struct StationValues
{
    Station info;
    Values values;

    StationValues() = default;
    StationValues(const dballe::Record& rec) : info(rec), values(rec) {}
    void from_record(const Record& rec);
    bool operator==(const StationValues& o) const
    {
        return info == o.info && values == o.values;
    }
    void clear_ids()
    {
        info.clear_ids();
        values.clear_ids();
    }
};

struct DataValues
{
    Sampling info;
    Values values;

    DataValues() = default;
    DataValues(const dballe::Record& rec) : info(rec), values(rec) {}
    void from_record(const Record& rec);
    bool operator==(const DataValues& o) const
    {
        return info == o.info && values == o.values;
    }
    void clear_ids()
    {
        info.clear_ids();
        values.clear_ids();
    }
};

}

#endif
