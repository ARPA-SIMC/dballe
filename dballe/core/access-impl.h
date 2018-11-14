// Helper function for keyword access implementations.

#include <dballe/var.h>
#include <dballe/types.h>
#include <dballe/value.h>
#include <dballe/core/var.h>
#include <dballe/msg/context.h>

namespace {

template<typename T>
struct MaybeBase
{
    T& _val;
    bool _found = false;
    bool _missing = true;

    MaybeBase(T& val) : _val(val)
    {
    }

    bool missing() const { return _missing; }

    void found()
    {
        _found = true;
    }

    void notfound()
    {
        _found = false;
    }

    void set(const T& val)
    {
        _found = true;
        _missing = false;
        _val = val;
    }

    void maybe_search_value(const char* key, const wreport::Var* var)
    {
        if (_found)
            return;
        if (!var)
            return;
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (code != var->code())
            throw_if_notfound(key);
        _found = true;
        if (var && var->isset())
        {
            _missing = false;
            _val = var->enq<T>();
        }
    }

    void maybe_search_value(const char* key, const dballe::Value& value)
    {
        if (_found)
            return;
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (code != value.code())
            throw_if_notfound(key);
        const wreport::Var* var = value.get();
        _found = true;
        if (var && var->isset())
        {
            _missing = false;
            _val = var->enq<T>();
        }
    }

    template<typename Values>
    void maybe_search_values(const char* key, const Values& values)
    {
        if (_found)
            return;
        wreport::Varcode code = dballe::resolve_varcode(key);
        const wreport::Var* var = values.maybe_var(code);
        _found = true;
        if (var && var->isset())
        {
            _missing = false;
            _val = var->enq<T>();
        }
    }

    void maybe_search_values(const char* key, const dballe::msg::Context* ctx)
    {
        if (_found)
            return;
        if (!ctx)
            return;
        wreport::Varcode code = dballe::resolve_varcode(key);
        const wreport::Var* var = ctx->find(code);
        _found = true;
        if (var && var->isset())
        {
            _missing = false;
            _val = var->enq<T>();
        }
    }

    [[noreturn]] void throw_if_notfound(const char* key)
    {
        wreport::error_notfound::throwf("key %s not found on this query result", key);
    }
};

struct Int : public MaybeBase<int>
{
    using MaybeBase::MaybeBase;
    using MaybeBase::set;
};

struct Double : public MaybeBase<double>
{
    using MaybeBase::MaybeBase;
    using MaybeBase::set;

    void set(const int& val)
    {
        _found = true;
        _missing = false;
        _val = val;
    }
};

struct String : public MaybeBase<std::string>
{
    using MaybeBase::MaybeBase;
    using MaybeBase::set;

    void set(const int& val)
    {
        _found = true;
        _missing = false;
        _val = std::to_string(val);
    }

    void set_ident(const dballe::Ident& ident)
    {
        _found = true;
        if (ident.is_missing())
            _missing = true;
        else
        {
            _missing = false;
            _val = ident.get();
        }
    }

    void set_int(int val)
    {
        _found = true;
        if (val != dballe::MISSING_INT)
        {
            _missing = false;
            _val = std::to_string(val);
        }
    }

    void maybe_set(const char* val)
    {
        _found = true;
        if (!val)
        {
            _missing = true;
        } else {
            _missing = false;
            _val = val;
        }
    }

    void maybe_set(const std::string& val)
    {
        _found = true;
        if (val.empty())
        {
            _missing = true;
        } else {
            _missing = false;
            _val = val;
        }
    }

    void set_latlon(double val)
    {
        char buf[15];
        snprintf(buf, 14, "%.5f", val);
        set(buf);
    }
};

template<typename Base>
struct Maybe : public Base
{
    using Base::Base;
    using Base::set;

    void maybe_set_int(int val, int missing=dballe::MISSING_INT)
    {
        if (val != missing)
            this->set(val);
        else
            this->found();
    }

    void set_varcode(wreport::Varcode code)
    {
        char buf[7];
        dballe::format_bcode(code, buf);
        this->set(std::string(buf));
    }
};

}
