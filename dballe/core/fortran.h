#ifndef DBALLE_CORE_FORTRAN_H
#define DBALLE_CORE_FORTRAN_H

#include <string>
#include <dballe/fwd.h>
#include <dballe/types.h>
#include <dballe/values.h>
#include <dballe/core/var.h>

namespace dballe {
namespace impl {

void string_to_fortran(const char* str, char* buf, unsigned buf_len);
void string_to_fortran(const std::string& str, char* buf, unsigned buf_len);

// TODO: move to dballe/fortran
struct Enqi
{
    const char* key;
    unsigned len;
    int res;
    bool missing = true;

    Enqi(const char* key, unsigned len)
        : key(key), len(len)
    {
    }

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

    void set_var(wreport::Varcode val)
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

struct Enqd
{
    const char* key;
    unsigned len;
    double res;
    bool missing = true;

    Enqd(const char* key, unsigned len)
        : key(key), len(len)
    {
    }

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

    void set_var(wreport::Varcode val)
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

struct Enqs
{
    const char* key;
    unsigned len;
    std::string res;
    bool missing = true;

    Enqs(const char* key, unsigned len)
        : key(key), len(len)
    {
    }

    void set_bool(bool val)
    {
        res = val ? "1" : "0";
        missing = false;
    }

    void set_int(int val)
    {
        res = std::to_string(val);
        missing = false;
    }

    void set_dballe_int(int val)
    {
        if (val == MISSING_INT)
            return;
        res = std::to_string(val);
        missing = false;
    }

    void set_string(const std::string& val)
    {
        res = val;
        missing = false;
    }

    void set_ident(const Ident& ident)
    {
        if (ident.is_missing())
            return;
        res = ident.get();
        missing = false;
    }

    void set_var(wreport::Varcode val)
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = buf;
        missing = false;
    }

    void set_lat(int lat)
    {
        if (lat == MISSING_INT)
            return;
        res = std::to_string(lat);
        missing = false;
    }

    void set_lon(int lon)
    {
        if (lon == MISSING_INT)
            return;
        res = std::to_string(lon);
        missing = false;
    }

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
            res = var->enqs();
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
            res = var->enqs();
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
            res = var->enqs();
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
            res = var->enqs();
        }
    }
};

struct Enqf
{
    const char* key;
    unsigned len;
    std::string res;
    bool missing = true;

    Enqf(const char* key, unsigned len)
        : key(key), len(len)
    {
    }

    void set_bool(bool val)
    {
        res = val ? "1" : "0";
        missing = false;
    }

    void set_int(int val)
    {
        res = std::to_string(val);
        missing = false;
    }

    void set_dballe_int(int val)
    {
        if (val == MISSING_INT)
            return;
        res = std::to_string(val);
        missing = false;
    }

    void set_string(const std::string& val)
    {
        res = val;
        missing = false;
    }

    void set_ident(const Ident& ident)
    {
        if (ident.is_missing())
            return;
        res = ident.get();
        missing = false;
    }

    void set_var(wreport::Varcode val)
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = buf;
        missing = false;
    }

    void set_lat(int lat)
    {
        if (lat == MISSING_INT)
            return;
        char buf[15];
        snprintf(buf, 14, "%.5f", Coords::lat_from_int(lat));
        res = buf;
        missing = false;
    }

    void set_lon(int lon)
    {
        if (lon == MISSING_INT)
            return;
        char buf[15];
        snprintf(buf, 14, "%.5f", Coords::lon_from_int(lon));
        res = buf;
        missing = false;
    }

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
            res = var->format();
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
            res = var->format();
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
            res = var->format();
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
            res = var->format();
        }
    }
};


}
}

#endif
