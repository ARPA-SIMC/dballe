#include "traced.h"
#include "dbapi.h"
#include "msgapi.h"
#include <wreport/error.h>
#include <wreport/utils/string.h>
#include <cstdlib>
#include <cstdio>
#include <sstream>

using namespace wreport;

namespace dballe {
namespace fortran {

std::unique_ptr<API> Tracer::begin(int dbahandle, int handle, const char* url, const char* anaflag, const char* dataflag, const char* attrflag)
{
    log_begin(dbahandle, handle, anaflag, dataflag, attrflag);
    return wrap_api(handle, fortran::DbAPI::fortran_connect(url, anaflag, dataflag, attrflag));
}

std::unique_ptr<API> Tracer::begin_messages(int handle, const char* filename, const char* mode, const char* type)
{
    log_begin_messages(handle, filename, mode, type);
    return wrap_api(handle, std::unique_ptr<API>(new fortran::MsgAPI(filename, mode, type)));
}

namespace {

struct NullTracer : public Tracer
{
    std::unique_ptr<API> wrap_api(int handle, std::unique_ptr<API> api) { return std::move(api); }
    void log_connect_url(int handle, const char* chosen_dsn) override {}
    void log_begin(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag) override {}
    void log_begin_messages(int handle, const char* filename, const char* mode, const char* type) override {}
    void log_disconnect(int handle) override {}
};

}

struct FileTracer : public Tracer
{
    std::string fname;
    FILE* trace_file = nullptr;

    FileTracer(const std::string& fname)
        : fname(fname)
    {
        trace_file = fopen(fname.c_str(), "at");
        if (!trace_file)
            error_system::throwf("Cannot open %s", fname.c_str());
        setvbuf(trace_file, nullptr, _IOLBF, 0);
        fprintf(trace_file, "// ** Execution begins **\n");
    }

    std::unique_ptr<API> wrap_api(int handle, std::unique_ptr<API> api) override
    {
        char trace_tag[16];
        if (dynamic_cast<fortran::DbAPI*>(api.get()))
            snprintf(trace_tag, 15, "dbapi%d", handle);
        else if (dynamic_cast<fortran::MsgAPI*>(api.get()))
            snprintf(trace_tag, 15, "msgapi%d", handle);
        else
            snprintf(trace_tag, 15, "api%d", handle);

        return std::unique_ptr<API>(new TracedAPI(*this, trace_tag, std::move(api)));
    }

    void log_connect_url(int handle, const char* chosen_dsn) override
    {
        std::string arg1 = str::encode_cstring(chosen_dsn);
        fprintf(trace_file, "auto db%d(DB::connect_from_url(\"%s\"));\n", handle, arg1.c_str());
    }

    void log_begin(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag) override
    {
        fprintf(trace_file, "DbAPI dbapi%d(*db%d, \"%s\", \"%s\", \"%s\");\n",
                handle, dbahandle, anaflag, dataflag, attrflag);
    }

    void log_begin_messages(int handle, const char* filename, const char* mode, const char* type) override
    {
        std::string arg1(str::encode_cstring(filename));
        fprintf(trace_file, "MsgAPI msgapi%d(\"%s\", \"%s\", \"%s\");\n",
                handle, arg1.c_str(), mode, type);
    }

    void log_disconnect(int handle) override
    {
        fprintf(trace_file, "// db%d not used anymore\n", handle);
    }

    template<typename... Args>
    void log_void(Args&&... args)
    {
        fprintf(trace_file, std::forward<Args>(args)...);
        fputs(";\n", trace_file);
    }
};

namespace {

struct FuncLogger
{
    TracedAPI& tracer;
    const char* name;
    std::stringstream args;
    bool has_result = false;

    FuncLogger(TracedAPI& tracer, const char* name) : tracer(tracer), name(name) {}
    ~FuncLogger()
    {
        if (!has_result) // Assume successful void function
            fprintf(tracer.tracer.trace_file, "wassert(%s.%s(%s));\n", tracer.name.c_str(), name, args.str().c_str());
    }

    void _log_args() {}

    template<typename T>
    void _log_args(const T& arg)
    {
        wreport::error_unimplemented::throwf("cannot log an argument of unspecified type");
    }

    void _log_args(bool arg)
    {
        args << (arg ? "true" : "false");
    }

    void _log_args(double arg)
    {
        if (arg == API::missing_double)
            args << "API::missing_double";
        else
            args << arg;
    }

    void _log_args(float arg)
    {
        if (arg == API::missing_float)
            args << "API::missing_float";
        else
            args << arg;
    }

    void _log_args(int arg)
    {
        if (arg == API::missing_int)
            args << "API::missing_int";
        else
            args << arg;
    }

    void _log_args(signed char arg)
    {
        if (arg == API::missing_byte)
            args << "API::missing_byte";
        else
            args << (int)arg;
    }

    void _log_args(Encoding encoding)
    {
        args << File::encoding_name(encoding);
    }

    void _log_args(const char* arg)
    {
        if (arg)
            args << '"' << str::encode_cstring(arg) << '"';
        else
            args << "nullptr";
    }

    template<typename T, typename... Args>
    void _log_args(const T& arg, Args&&... args)
    {
        _log_args(arg);
        this->args << ", ";
        _log_args(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_args(Args&&... args)
    {
        _log_args(std::forward<Args>(args)...);
    }

    int log_result(bool res)
    {
        fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == %s);\n", tracer.name.c_str(), name, args.str().c_str(), res ? "true" : "false");
        has_result = true;
        return res;
    }

    int log_result(int res)
    {
        if (res == API::missing_int)
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == API::missing_int);\n", tracer.name.c_str(), name, args.str().c_str());
        else
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == %d);\n", tracer.name.c_str(), name, args.str().c_str(), res);
        has_result = true;
        return res;
    }

    signed char log_result(signed char res)
    {
        if (res == API::missing_byte)
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == API::missing_byte);\n", tracer.name.c_str(), name, args.str().c_str());
        else
            fprintf(tracer.tracer.trace_file, "wassert(actual((int)%s.%s(%s)) == %d);\n", tracer.name.c_str(), name, args.str().c_str(), (int)res);
        has_result = true;
        return res;
    }

    float log_result(float res)
    {
        if (res == API::missing_float)
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == API::missing_float);\n", tracer.name.c_str(), name, args.str().c_str());
        else
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == %f);\n", tracer.name.c_str(), name, args.str().c_str(), (double)res);
        has_result = true;
        return res;
    }

    double log_result(double res)
    {
        if (res == API::missing_double)
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == API::missing_double);\n", tracer.name.c_str(), name, args.str().c_str());
        else
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == %f);\n", tracer.name.c_str(), name, args.str().c_str(), res);
        has_result = true;
        return res;
    }

    const char* log_result(const char* res)
    {
        if (res)
        {
            std::string fmt = str::encode_cstring(res);
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == \"%s\");\n", tracer.name.c_str(), name, args.str().c_str(), fmt.c_str());
        } else {
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == nullptr);\n", tracer.name.c_str(), name, args.str().c_str());
        }
        has_result = true;
        return res;
    }

    wreport::Varcode log_result(wreport::Varcode res)
    {
        if (res == 0)
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == 0);\n", tracer.name.c_str(), name, args.str().c_str());
        else
            fprintf(tracer.tracer.trace_file, "wassert(actual(%s.%s(%s)) == WR_VAR(%d, %d, %d));\n", tracer.name.c_str(), name, args.str().c_str(), WR_VAR_FXY(res));
        has_result = true;
        return res;
    }

    void log_exception(std::exception& e)
    {
        fprintf(tracer.tracer.trace_file, "wassert_throws(std::exception, %s.%s(%s)); // %s\n", tracer.name.c_str(), name, args.str().c_str(), e.what());
        has_result = true;
    }
};

template<typename... Margs, typename... Args>
void run_and_log(TracedAPI& tracer, const char* name, void (API::*meth)(Margs...), Args&&... args)
{
    FuncLogger logger(tracer, name);
    logger.log_args(std::forward<Args>(args)...);
    try {
        ((tracer.api.get())->*(meth))(std::forward<Args>(args)...);
    } catch (std::exception& e) {
        logger.log_exception(e);
        throw;
    }
}

template<typename R, typename... Margs, typename... Args>
R run_and_log(TracedAPI& tracer, const char* name, R (API::*meth)(Margs...), Args&&... args)
{
    FuncLogger logger(tracer, name);
    logger.log_args(std::forward<Args>(args)...);
    try {
        return logger.log_result(((tracer.api.get())->*(meth))(std::forward<Args>(args)...));
    } catch (std::exception& e) {
        logger.log_exception(e);
        throw;
    }
}

}

std::unique_ptr<Tracer> Tracer::create()
{
    // Init API tracing if requested
    const char* tracefile = getenv("DBALLE_TRACE_FORTRAN");
    if (!tracefile) tracefile = getenv("DBA_FORTRAN_TRACE");
    if (tracefile)
        return std::unique_ptr<Tracer>(new FileTracer(tracefile));
    else
        return std::unique_ptr<Tracer>(new NullTracer);
}


TracedAPI::TracedAPI(FileTracer& tracer, const std::string& name, std::unique_ptr<API> api)
    : tracer(tracer), name(name), api(std::move(api))
{
}

#define RUN(name, ...) run_and_log(*this, #name, &API::name, ## __VA_ARGS__);

void TracedAPI::scopa(const char* repinfofile)
{
    RUN(scopa, repinfofile);
}

void TracedAPI::remove_all()
{
    RUN(remove_all);
}

int TracedAPI::enqi(const char* param)
{
    return RUN(enqi, param);
}

signed char TracedAPI::enqb(const char* param)
{
    return RUN(enqb, param);
}

float TracedAPI::enqr(const char* param)
{
    return RUN(enqr, param);
}

double TracedAPI::enqd(const char* param)
{
    return RUN(enqd, param);
}

bool TracedAPI::enqc(const char* param, std::string& res)
{
    FILE*& out = tracer.trace_file;
    std::string arg_param = str::encode_cstring(param);
    bool ret;
    fputs("{\n", out);
    fputs("    std::string res;\n", out);
    try {
        ret = api->enqc(param, res);
    } catch (std::exception& e) {
        fprintf(out, "    wassert_throws(std::exception, %s.enqc(\"%s\", res)); // %s\n", name.c_str(), arg_param.c_str(), e.what());
        fputs("}\n", out);
        throw;
    }
    if (ret)
    {
        std::string arg_res = str::encode_cstring(res);
        fprintf(out, "    wassert_true(%s.enqc(\"%s\", res));\n", name.c_str(), arg_param.c_str());
        fprintf(out, "    wassert(actual(res) == \"%s\");\n", arg_res.c_str());
    }
    else
        fprintf(out, "    wassert_false(%s.enqc(\"%s\", res));\n", name.c_str(), arg_param.c_str());
    fputs("}\n", out);
    return ret;
}

void TracedAPI::seti(const char* param, int value)
{
    RUN(seti, param, value);
}

void TracedAPI::setb(const char* param, signed char value)
{
    RUN(setb, param, value);
}

void TracedAPI::setr(const char* param, float value)
{
    RUN(setr, param, value);
}

void TracedAPI::setd(const char* param, double value)
{
    RUN(setd, param, value);
}

void TracedAPI::setc(const char* param, const char* value)
{
    RUN(setc, param, value);
}

void TracedAPI::setcontextana()
{
    RUN(setcontextana);
}

namespace {

void print_compare_int(FILE* out, const char* name, int val)
{
    if (val == API::missing_int)
        fprintf(out, "    wassert(actual(%s) == API::missing_int);\n", name);
    else
        fprintf(out, "    wassert(actual(%s) == %d);\n", name, val);
}

}

void TracedAPI::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
    FILE*& out = tracer.trace_file;
    fputs("{\n", out);
    fputs("    int ltype1, l1, ltype2, l2;\n", out);
    try {
        api->enqlevel(ltype1, l1, ltype2, l2);
    } catch (std::exception& e) {
        fprintf(out, "    wassert_throws(std::exception, %s.enqlevel(ltype1, l1, ltype2, l2)); // %s\n", name.c_str(), e.what());
        fputs("}\n", out);
        throw;
    }
    fprintf(out, "    wassert(%s.enqlevel(ltype1, l1, ltype2, l2));\n", name.c_str());
    print_compare_int(out, "ltype1", ltype1);
    print_compare_int(out, "l1", l1);
    print_compare_int(out, "ltype2", ltype2);
    print_compare_int(out, "l2", l2);
    fputs("}\n", out);
}

void TracedAPI::setlevel(int ltype1, int l1, int ltype2, int l2)
{
    RUN(setlevel, ltype1, l1, ltype2, l2);
}

void TracedAPI::enqtimerange(int& pind, int& p1, int& p2)
{
    FILE*& out = tracer.trace_file;
    fputs("{\n", out);
    fputs("    int pind, p1, p2;\n", out);
    try {
        api->enqtimerange(pind, p1, p2);
    } catch (std::exception& e) {
        fprintf(out, "    wassert_throws(std::exception, %s.enqtimerange(pind, p1, p2)); // %s\n", name.c_str(), e.what());
        fputs("}\n", out);
        throw;
    }
    fprintf(out, "    wassert(%s.enqtimerange(pind, p1, p2));\n", name.c_str());
    print_compare_int(out, "pind", pind);
    print_compare_int(out, "p1", p1);
    print_compare_int(out, "p2", p2);
    fputs("}\n", out);
}

void TracedAPI::settimerange(int pind, int p1, int p2)
{
    RUN(settimerange, pind, p1, p2);
}

void TracedAPI::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    FILE*& out = tracer.trace_file;
    fputs("{\n", out);
    fputs("    int year, month, day, hour, min, sec;\n", out);
    try {
        api->enqdate(year, month, day, hour, min, sec);
    } catch (std::exception& e) {
        fprintf(out, "    wassert_throws(std::exception, %s.enqdate(year, month, day, hour, min, sec)); // %s\n", name.c_str(), e.what());
        fputs("}\n", out);
        throw;
    }
    fprintf(out, "    wassert(%s.enqdate(year, month, day, hour, min, sec));\n", name.c_str());
    print_compare_int(out, "year",  year);
    print_compare_int(out, "month", month);
    print_compare_int(out, "day",   day);
    print_compare_int(out, "hour",  hour);
    print_compare_int(out, "min",   min);
    print_compare_int(out, "sec",   sec);
    fputs("}\n", out);
}

void TracedAPI::setdate(int year, int month, int day, int hour, int min, int sec)
{
    RUN(setdate, year, month, day, hour, min, sec);
}

void TracedAPI::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
    RUN(setdatemin, year, month, day, hour, min, sec);
}

void TracedAPI::setdatemax(int year, int month, int day, int hour, int min, int sec)
{
    RUN(setdatemax, year, month, day, hour, min, sec);
}

void TracedAPI::unset(const char* param)
{
    RUN(unset, param);
}

void TracedAPI::unsetall()
{
    RUN(unsetall);
}

void TracedAPI::unsetb()
{
    RUN(unsetb);
}

int TracedAPI::quantesono()
{
    return RUN(quantesono);
}

void TracedAPI::elencamele()
{
    RUN(elencamele);
}

int TracedAPI::voglioquesto()
{
    return RUN(voglioquesto);
}

wreport::Varcode TracedAPI::dammelo()
{
    return RUN(dammelo);
}

void TracedAPI::prendilo()
{
    RUN(prendilo);
}

void TracedAPI::dimenticami()
{
    RUN(dimenticami);
}

int TracedAPI::voglioancora()
{
    return RUN(voglioancora);
}

const char* TracedAPI::ancora()
{
    return RUN(ancora);
}

void TracedAPI::critica()
{
    RUN(critica);
}

void TracedAPI::scusa()
{
    RUN(scusa);
}

void TracedAPI::messages_open_input(const char* filename, const char* mode, Encoding format, bool simplified)
{
    RUN(messages_open_input, filename, mode, format, simplified);
}

void TracedAPI::messages_open_output(const char* filename, const char* mode, Encoding format)
{
    RUN(messages_open_output, filename, mode, format);
}

bool TracedAPI::messages_read_next()
{
    return RUN(messages_read_next);
}

void TracedAPI::messages_write_next(const char* template_name)
{
    RUN(messages_write_next, template_name);
}

const char* TracedAPI::spiegal(int ltype1, int l1, int ltype2, int l2)
{
    return RUN(spiegal, ltype1, l1, ltype2, l2);
}

const char* TracedAPI::spiegat(int ptype, int p1, int p2)
{
    return RUN(spiegat, ptype, p1, p2);
}

const char* TracedAPI::spiegab(const char* varcode, const char* value)
{
    return RUN(spiegab, varcode, value);
}

void TracedAPI::fatto()
{
    RUN(fatto);
    fprintf(tracer.trace_file, "// %s not used anymore\n", name.c_str());
}

}
}
