#ifndef DBALLE_CORE_VALUES_H
#define DBALLE_CORE_VALUES_H

#include <dballe/core/defs.h>
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
};

struct Sampling : public Station
{
    Datetime datetime;
    Level level;
    Trange trange;
};

namespace values {
struct Value
{
    int data_id = MISSING_INT;
    wreport::Var* var = nullptr;

    Value(const Value&) = delete;
    Value(Value&& o) : data_id(o.data_id), var(o.var) { o.var = nullptr; }
    Value(std::unique_ptr<wreport::Var>&& var)
        : var(var.release()) {}
    ~Value() { delete var; }
    Value& operator=(const Value&) = delete;
    Value& operator=(Value&& o)
    {
        if (this == &o) return *this;
        data_id = o.data_id;
        delete var;
        var = o.var;
        o.var = nullptr;
        return *this;
    }
};
}

// FIXME: map, or hashmap, or vector enforced to be unique
struct Values : protected std::map<wreport::Varcode, values::Value>
{
    typedef std::map<wreport::Varcode, values::Value>::const_iterator const_iterator;
    typedef std::map<wreport::Varcode, values::Value>::iterator iterator;
    const_iterator begin() const { return std::map<wreport::Varcode, values::Value>::begin(); }
    const_iterator end() const { return std::map<wreport::Varcode, values::Value>::end(); }
    iterator begin() { return std::map<wreport::Varcode, values::Value>::begin(); }
    iterator end() { return std::map<wreport::Varcode, values::Value>::end(); }

    void set(const wreport::Var&);
    void set(std::unique_ptr<wreport::Var>&&);
    void add_data_id(wreport::Varcode code, int data_id);
};

struct StationValues
{
    Station info;
    Values values;
};

struct DataValues
{
    Sampling info;
    Values values;
};

}

#endif
