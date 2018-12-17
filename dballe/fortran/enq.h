#ifndef DBALLE_FORTRAN_ENQ_H
#define DBALLE_FORTRAN_ENQ_H

#include "dballe/core/enq.h"

namespace dballe {
namespace fortran {

struct Enqs : public impl::Enq
{
    using Enq::Enq;
    std::string res;

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

    void set_varcode(wreport::Varcode val)
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = buf;
        missing = false;
    }

    void set_var(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqs `%s`", key);
    }

    void set_attrs(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqs `%s`", key);
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

    template<typename Station>
    void set_coords(const Station& s) { wreport::error_consistency::throwf("cannot enqs `%s`", key); }

    template<typename Station>
    void set_station(const Station& s) { wreport::error_consistency::throwf("cannot enqs `%s`", key); }

    void set_datetime(const Datetime& dt) { wreport::error_consistency::throwf("cannot enqs `%s`", key); }
    void set_level(const Level& dt) { wreport::error_consistency::throwf("cannot enqs `%s`", key); }
    void set_trange(const Trange& dt) { wreport::error_consistency::throwf("cannot enqs `%s`", key); }

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
            throw_notfound();

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
            throw_notfound();
        const wreport::Var* var = value.get();
        if (var && var->isset())
        {
            missing = false;
            res = var->enqs();
        }
    }
};

}
}

#endif
