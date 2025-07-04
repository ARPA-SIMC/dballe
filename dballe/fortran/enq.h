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

    const char* name() const override { return "enqc"; }

    void set_bool(bool val) override
    {
        API::to_fortran(val ? "1" : "0", res, res_len);
        missing = false;
    }

    void set_int(int val) override
    {
        API::to_fortran(val, res, res_len);
        missing = false;
    }

    void set_dballe_int(int val) override
    {
        if (val == MISSING_INT)
            return;
        API::to_fortran(val, res, res_len);
        missing = false;
    }

    void set_string(const std::string& val) override
    {
        API::to_fortran(val, res, res_len);
        missing = false;
    }

    void set_ident(const Ident& ident) override
    {
        if (ident.is_missing())
            return;
        API::to_fortran(ident.get(), res, res_len);
        missing = false;
    }

    void set_varcode(wreport::Varcode val) override
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        API::to_fortran(buf, res, res_len);
        missing = false;
    }

    void set_lat(int lat) override
    {
        if (lat == MISSING_INT)
            return;
        API::to_fortran(lat, res, res_len);
        missing = false;
    }

    void set_lon(int lon) override
    {
        if (lon == MISSING_INT)
            return;
        API::to_fortran(lon, res, res_len);
        missing = false;
    }

    void set_var_value(const wreport::Var& var) override
    {
        missing = false;
        API::to_fortran(var.enqc(), res, res_len);
    }
};

} // namespace fortran
} // namespace dballe

#endif
