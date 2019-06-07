#ifndef DBALLE_CORE_ENQ_H
#define DBALLE_CORE_ENQ_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <dballe/values.h>
#include <dballe/core/var.h>
#include <wreport/error.h>
#include <string>

namespace dballe {
namespace impl {

/**
 * Class passed to key-value accessors to set values in an invoker-defined way
 */
struct Enq
{
    const char* key;
    unsigned len;
    bool missing = true;

    Enq(const char* key, unsigned len)
        : key(key), len(len)
    {
    }
    virtual ~Enq() {}

    [[noreturn]] void throw_notfound()
    {
        wreport::error_notfound::throwf("key %s not found on this query result", key);
    }

    // Return the name of this access operation
    virtual const char* name() const = 0;

    // Set a boolean value
    virtual void set_bool(bool val) = 0;

    // Set an always defined int value
    virtual void set_int(int val) = 0;

    // Set an int value that is undefined if it is MISSING_INT
    virtual void set_dballe_int(int val) = 0;

    // Set a string
    virtual void set_string(const std::string& val)
    {
        wreport::error_consistency::throwf("cannot %s `%s`", name(), key);
    }

    // Set station identifier
    virtual void set_ident(const Ident& ident)
    {
        wreport::error_consistency::throwf("cannot %s `%s`", name(), key);
    }

    // Set variable code
    virtual void set_varcode(wreport::Varcode val)
    {
        wreport::error_consistency::throwf("cannot %s `%s`", name(), key);
    }

    // Set variable value
    virtual void set_var(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot %s `%s`", name(), key);
    }

    /**
     * Set the value using the value of the given variable.
     *
     * var can be assumed to always be set, that is, the caller has already
     * checked that it has a value.
     */
    virtual void set_var_value(const wreport::Var& var) = 0;

    // Set variable attributes
    virtual void set_attrs(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot %s `%s`", name(), key);
    }

    // Set latitude
    virtual void set_lat(int lat) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set longitude
    virtual void set_lon(int lon) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set coordinates
    virtual void set_coords(const Coords& c) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set station
    virtual void set_station(const Station& s) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set station with DB info
    virtual void set_dbstation(const DBStation& s) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set datetime
    virtual void set_datetime(const Datetime& dt) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set level
    virtual void set_level(const Level& dt) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    // Set timerange
    virtual void set_trange(const Trange& dt) { wreport::error_consistency::throwf("cannot %s `%s`", name(), key); }

    template<typename Values>
    bool search_b_values(const Values& values)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        const wreport::Var* var = values.maybe_var(code);
        if (var && var->isset())
            set_var_value(*var);
        return true;
    }

    bool search_b_value(const dballe::Value& value)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        if (code != value.code())
            throw_notfound();

        const wreport::Var* var = value.get();
        if (var && var->isset())
            set_var_value(*var);
        return true;
    }

    template<typename Values>
    void search_alias_values(const Values& values)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        const wreport::Var* var = values.maybe_var(code);
        if (var && var->isset())
            set_var_value(*var);
    }

    void search_alias_value(const dballe::Value& value)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (code != value.code())
            throw_notfound();
        const wreport::Var* var = value.get();
        if (var && var->isset())
            set_var_value(*var);
    }
};

struct Enqi : public Enq
{
    using Enq::Enq;
    int res;

    const char* name() const override { return "enqi"; }

    void set_bool(bool val) override
    {
        res = val ? 1 : 0;
        missing = false;
    }

    void set_int(int val) override
    {
        res = val;
        missing = false;
    }

    void set_dballe_int(int val) override
    {
        if (val == MISSING_INT)
            return;
        res = val;
        missing = false;
    }

    void set_lat(int lat) override
    {
        if (lat == MISSING_INT)
            return;
        res = lat;
        missing = false;
    }

    void set_lon(int lon) override
    {
        if (lon == MISSING_INT)
            return;
        res = lon;
        missing = false;
    }

    void set_var_value(const wreport::Var& var) override
    {
        missing = false;
        res = var.enqi();
    }
};

struct Enqd : public Enq
{
    using Enq::Enq;
    double res;

    const char* name() const override { return "enqd"; }

    void set_bool(bool val) override
    {
        res = val ? 1 : 0;
        missing = false;
    }

    void set_int(int val) override
    {
        res = val;
        missing = false;
    }

    void set_dballe_int(int val) override
    {
        if (val == MISSING_INT)
            return;
        res = val;
        missing = false;
    }

    void set_lat(int lat) override
    {
        if (lat == MISSING_INT)
            return;
        res = Coords::lat_from_int(lat);
        missing = false;
    }

    void set_lon(int lon) override
    {
        if (lon == MISSING_INT)
            return;
        res = Coords::lon_from_int(lon);
        missing = false;
    }

    void set_var_value(const wreport::Var& var) override
    {
        missing = false;
        res = var.enqd();
    }
};

}
}

#endif
