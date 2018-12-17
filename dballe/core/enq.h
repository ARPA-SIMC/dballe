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

struct Enq
{
    const char* key;
    unsigned len;
    bool missing = true;

    Enq(const char* key, unsigned len)
        : key(key), len(len)
    {
    }

};

struct Enqi : public Enq
{
    using Enq::Enq;
    int res;

    void set_bool(bool val)
    {
        res = val ? 1 : 0;
        missing = false;
    }

    void set_int(int val)
    {
        res = val;
        missing = false;
    }

    void set_dballe_int(int val)
    {
        if (val == MISSING_INT)
            return;
        res = val;
        missing = false;
    }

    void set_string(const std::string& val)
    {
        wreport::error_consistency::throwf("cannot enqi `%s`", key);
    }

    void set_ident(const Ident& ident)
    {
        wreport::error_consistency::throwf("cannot enqi `%s`", key);
    }

    void set_varcode(wreport::Varcode val)
    {
        wreport::error_consistency::throwf("cannot enqi `%s`", key);
    }

    void set_var(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqi `%s`", key);
    }

    void set_attrs(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqi `%s`", key);
    }

    void set_lat(int lat)
    {
        if (lat == MISSING_INT)
            return;
        res = lat;
        missing = false;
    }

    void set_lon(int lon)
    {
        if (lon == MISSING_INT)
            return;
        res = lon;
        missing = false;
    }

    template<typename Station>
    void set_coords(const Station& s) { wreport::error_consistency::throwf("cannot enqi `%s`", key); }

    template<typename Station>
    void set_station(const Station& s) { wreport::error_consistency::throwf("cannot enqi `%s`", key); }

    void set_datetime(const Datetime& dt) { wreport::error_consistency::throwf("cannot enqi `%s`", key); }
    void set_level(const Level& dt) { wreport::error_consistency::throwf("cannot enqi `%s`", key); }
    void set_trange(const Trange& dt) { wreport::error_consistency::throwf("cannot enqi `%s`", key); }

    template<typename Values>
    bool search_b_values(const Values& values)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        const wreport::Var* var = values.maybe_var(code);
        if (var && var->isset())
        {
            missing = false;
            res = var->enqi();
        }
        return true;
    }

    bool search_b_value(const dballe::Value& value)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        if (code != value.code())
            wreport::error_notfound::throwf("key %s not found on this query result", key);

        const wreport::Var* var = value.get();
        if (var && var->isset())
        {
            missing = false;
            res = var->enqi();
        }
        return true;
    }

    template<typename Values>
    void search_alias_values(const Values& values)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        const wreport::Var* var = values.maybe_var(code);
        if (var && var->isset())
        {
            missing = false;
            res = var->enqi();
        }
    }

    void search_alias_value(const dballe::Value& value)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (code != value.code())
            wreport::error_notfound::throwf("key %s not found on this query result", key);
        const wreport::Var* var = value.get();
        if (var && var->isset())
        {
            missing = false;
            res = var->enqi();
        }
    }
};

struct Enqd : public Enq
{
    using Enq::Enq;
    double res;

    void set_bool(bool val)
    {
        res = val ? 1 : 0;
        missing = false;
    }

    void set_int(int val)
    {
        res = val;
        missing = false;
    }

    void set_dballe_int(int val)
    {
        if (val == MISSING_INT)
            return;
        res = val;
        missing = false;
    }

    void set_string(const std::string& val)
    {
        wreport::error_consistency::throwf("cannot enqd `%s`", key);
    }

    void set_ident(const Ident& ident)
    {
        wreport::error_consistency::throwf("cannot enqd `%s`", key);
    }

    void set_varcode(wreport::Varcode val)
    {
        wreport::error_consistency::throwf("cannot enqd `%s`", key);
    }

    void set_var(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqd `%s`", key);
    }

    void set_attrs(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqd `%s`", key);
    }

    void set_lat(int lat)
    {
        if (lat == MISSING_INT)
            return;
        res = Coords::lat_from_int(lat);
        missing = false;
    }

    void set_lon(int lon)
    {
        if (lon == MISSING_INT)
            return;
        res = Coords::lon_from_int(lon);
        missing = false;
    }

    template<typename Station>
    void set_coords(const Station& s) { wreport::error_consistency::throwf("cannot enqd `%s`", key); }

    template<typename Station>
    void set_station(const Station& s) { wreport::error_consistency::throwf("cannot enqd `%s`", key); }

    void set_datetime(const Datetime& dt) { wreport::error_consistency::throwf("cannot enqd `%s`", key); }
    void set_level(const Level& dt) { wreport::error_consistency::throwf("cannot enqd `%s`", key); }
    void set_trange(const Trange& dt) { wreport::error_consistency::throwf("cannot enqd `%s`", key); }

    template<typename Values>
    bool search_b_values(const Values& values)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        const wreport::Var* var = values.maybe_var(code);
        if (var && var->isset())
        {
            missing = false;
            res = var->enqd();
        }
        return true;
    }

    bool search_b_value(const dballe::Value& value)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        if (code != value.code())
            wreport::error_notfound::throwf("key %s not found on this query result", key);

        const wreport::Var* var = value.get();
        if (var && var->isset())
        {
            missing = false;
            res = var->enqd();
        }
        return true;
    }

    template<typename Values>
    void search_alias_values(const Values& values)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        const wreport::Var* var = values.maybe_var(code);
        if (var && var->isset())
        {
            missing = false;
            res = var->enqd();
        }
    }

    void search_alias_value(const dballe::Value& value)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (code != value.code())
            wreport::error_notfound::throwf("key %s not found on this query result", key);
        const wreport::Var* var = value.get();
        if (var && var->isset())
        {
            missing = false;
            res = var->enqd();
        }
    }
};

}
}

#endif
