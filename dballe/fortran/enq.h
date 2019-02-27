#ifndef DBALLE_FORTRAN_ENQ_H
#define DBALLE_FORTRAN_ENQ_H

#include <dballe/core/enq.h>
#include <dballe/fortran/api.h>

namespace dballe {
namespace fortran {

struct Enqc : public impl::Enq
{
    using Enq::Enq;
    char* res;
    unsigned res_len;

    Enqc(const char* key, unsigned len, char* res, unsigned res_len)
        : Enq(key, len), res(res), res_len(res_len)
    {
    }

    void set_bool(bool val)
    {
        API::to_fortran(val ? "1" : "0", res, res_len);
        missing = false;
    }

    void set_int(int val)
    {
        API::to_fortran(val, res, res_len);
        missing = false;
    }

    void set_dballe_int(int val)
    {
        if (val == MISSING_INT)
            return;
        API::to_fortran(val, res, res_len);
        missing = false;
    }

    void set_string(const std::string& val)
    {
        API::to_fortran(val, res, res_len);
        missing = false;
    }

    void set_ident(const Ident& ident)
    {
        if (ident.is_missing())
            return;
        API::to_fortran(ident.get(), res, res_len);
        missing = false;
    }

    void set_varcode(wreport::Varcode val)
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        API::to_fortran(buf, res, res_len);
        missing = false;
    }

    void set_var(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqc `%s`", key);
    }

    void set_attrs(const wreport::Var* val)
    {
        wreport::error_consistency::throwf("cannot enqc `%s`", key);
    }

    void set_lat(int lat)
    {
        if (lat == MISSING_INT)
            return;
        API::to_fortran(lat, res, res_len);
        missing = false;
    }

    void set_lon(int lon)
    {
        if (lon == MISSING_INT)
            return;
        API::to_fortran(lon, res, res_len);
        missing = false;
    }

    template<typename Station>
    void set_coords(const Station& s) { wreport::error_consistency::throwf("cannot enqc `%s`", key); }
    template<typename Station>
    void set_station(const Station& s) { wreport::error_consistency::throwf("cannot enqc `%s`", key); }
    void set_datetime(const Datetime& dt) { wreport::error_consistency::throwf("cannot enqc `%s`", key); }
    void set_level(const Level& dt) { wreport::error_consistency::throwf("cannot enqc `%s`", key); }
    void set_trange(const Trange& dt) { wreport::error_consistency::throwf("cannot enqc `%s`", key); }

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
            API::to_fortran(var->enqc(), res, res_len);
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
            API::to_fortran(var->enqc(), res, res_len);
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
            API::to_fortran(var->enqc(), res, res_len);
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
            API::to_fortran(var->enqc(), res, res_len);
        }
    }
};

}
}

#endif
