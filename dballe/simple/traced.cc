#include "traced.h"
#include "dbapi.h"
#include "msgapi.h"
#include <wreport/error.h>
#include <wreport/utils/string.h>
#include <cstdlib>
#include <cstdio>

using namespace wreport;

namespace dballe {
namespace fortran {

std::unique_ptr<API> Tracer::preparati(int dbahandle, int handle, const char* url, const char* anaflag, const char* dataflag, const char* attrflag)
{
    log_preparati(dbahandle, handle, anaflag, dataflag, attrflag);
    return wrap_api(handle, fortran::DbAPI::fortran_connect(url, anaflag, dataflag, attrflag));
}

std::unique_ptr<API> Tracer::messaggi(int handle, const char* filename, const char* mode, const char* type)
{
    log_messaggi(handle, filename, mode, type);
    return wrap_api(handle, std::unique_ptr<API>(new fortran::MsgAPI(filename, mode, type)));
}

namespace {

struct NullTracer : public Tracer
{
    std::unique_ptr<API> wrap_api(int handle, std::unique_ptr<API> api) { return std::move(api); }
    void log_presentati_url(int handle, const char* chosen_dsn) override {}
    void log_preparati(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag) override {}
    void log_messaggi(int handle, const char* filename, const char* mode, const char* type) override {}
    void log_arrivederci(int handle) override {}
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

    void log_presentati_url(int handle, const char* chosen_dsn) override
    {
        std::string arg1 = str::encode_cstring(chosen_dsn);
        fprintf(trace_file, "auto db%d(DB::connect_from_url(\"%s\"));\n", handle, arg1.c_str());
    }

    void log_preparati(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag) override
    {
        fprintf(trace_file, "DbAPI dbapi%d(*db%d, \"%s\", \"%s\", \"%s\");\n",
                handle, dbahandle, anaflag, dataflag, attrflag);
    }

    void log_messaggi(int handle, const char* filename, const char* mode, const char* type) override
    {
        std::string arg1(str::encode_cstring(filename));
        fprintf(trace_file, "MsgAPI msgapi%d(\"%s\", \"%s\", \"%s\");\n",
                handle, arg1.c_str(), mode, type);
    }

    void log_arrivederci(int handle) override
    {
        fprintf(trace_file, "// db%d not used anymore\n", handle);
    }
};


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

void TracedAPI::scopa(const char* repinfofile)
{
    if (repinfofile)
    {
        std::string arg = str::encode_cstring(repinfofile);
        fprintf(tracer.trace_file, "%s.scopa(\"%s\");\n", name.c_str(), arg.c_str());
    }
    else
        fprintf(tracer.trace_file, "%s.scopa();\n", name.c_str());
    api->scopa(repinfofile);
}

void TracedAPI::remove_all()
{
    fprintf(tracer.trace_file, "%s.remove_all();\n", name.c_str());
    api->remove_all();
}

int TracedAPI::enqi(const char* param)
{
    return api->enqi(param);
}

signed char TracedAPI::enqb(const char* param)
{
    return api->enqb(param);
}

float TracedAPI::enqr(const char* param)
{
    return api->enqr(param);
}

double TracedAPI::enqd(const char* param)
{
    return api->enqd(param);
}

bool TracedAPI::enqc(const char* param, std::string& res)
{
    return api->enqc(param, res);
}

void TracedAPI::seti(const char* param, int value)
{
    std::string arg = str::encode_cstring(param);
    fprintf(tracer.trace_file, "%s.seti(\"%s\", %d);\n", name.c_str(), arg.c_str(), value);
    api->seti(param, value);
}

void TracedAPI::setb(const char* param, signed char value)
{
    std::string arg = str::encode_cstring(param);
    fprintf(tracer.trace_file, "%s.setb(\"%s\", %hhd;\n", name.c_str(), arg.c_str(), value);
    api->setb(param, value);
}

void TracedAPI::setr(const char* param, float value)
{
    std::string arg = str::encode_cstring(param);
    fprintf(tracer.trace_file, "%s.seti(\"%s\", %f);\n", name.c_str(), arg.c_str(), (double)value);
    api->setr(param, value);
}

void TracedAPI::setd(const char* param, double value)
{
    std::string arg = str::encode_cstring(param);
    fprintf(tracer.trace_file, "%s.setd(\"%s\", %f);\n", name.c_str(), arg.c_str(), value);
    api->setd(param, value);
}

void TracedAPI::setc(const char* param, const char* value)
{
    std::string arg1 = str::encode_cstring(param);
    std::string arg2 = str::encode_cstring(value);
    fprintf(tracer.trace_file, "%s.setc(\"%s\", \"%s\");\n", name.c_str(), arg1.c_str(), arg2.c_str());
    api->setc(param, value);
}

void TracedAPI::setcontextana()
{
    fprintf(tracer.trace_file, "%s.setcontextana();\n", name.c_str());
    api->setcontextana();
}

void TracedAPI::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
    api->enqlevel(ltype1, l1, ltype2, l2);
}

void TracedAPI::setlevel(int ltype1, int l1, int ltype2, int l2)
{
    fprintf(tracer.trace_file, "%s.setlevel(%d, %d, %d, %d);\n", name.c_str(), ltype1, l1, ltype2, l2);
    api->setlevel(ltype1, l1, ltype2, l2);
}

void TracedAPI::enqtimerange(int& pind, int& p1, int& p2)
{
    api->enqtimerange(pind, p1, p2);
}

void TracedAPI::settimerange(int pind, int p1, int p2)
{
    fprintf(tracer.trace_file, "%s.settimerange(%d, %d, %d);\n", name.c_str(), pind, p1, p2);
    api->settimerange(pind, p1, p2);
}

void TracedAPI::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    api->enqdate(year, month, day, hour, min, sec);
}

void TracedAPI::setdate(int year, int month, int day, int hour, int min, int sec)
{
    fprintf(tracer.trace_file, "%s.setdate(%d, %d, %d, %d, %d, %d);\n", name.c_str(), year, month, day, hour, min, sec);
    api->setdate(year, month, day, hour, min, sec);
}

void TracedAPI::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
    fprintf(tracer.trace_file, "%s.setdatemin(%d, %d, %d, %d, %d, %d);\n", name.c_str(), year, month, day, hour, min, sec);
    api->setdatemin(year, month, day, hour, min, sec);
}

void TracedAPI::setdatemax(int year, int month, int day, int hour, int min, int sec)
{
    fprintf(tracer.trace_file, "%s.setdatemax(%d, %d, %d, %d, %d, %d);\n", name.c_str(), year, month, day, hour, min, sec);
    api->setdatemax(year, month, day, hour, min, sec);
}

void TracedAPI::unset(const char* param)
{
    std::string arg = str::encode_cstring(param);
    fprintf(tracer.trace_file, "%s.unset(\"%s\");\n", name.c_str(), arg.c_str());
    api->unset(param);
}

void TracedAPI::unsetall()
{
    fprintf(tracer.trace_file, "%s.unsetall();\n", name.c_str());
    api->unsetall();
}

void TracedAPI::unsetb()
{
    fprintf(tracer.trace_file, "%s.unsetb();\n", name.c_str());
    api->unsetb();
}

int TracedAPI::quantesono()
{
    return api->quantesono();
}

void TracedAPI::elencamele()
{
    fprintf(tracer.trace_file, "%s.elencamele();\n", name.c_str());
    api->elencamele();
}

int TracedAPI::voglioquesto()
{
    return api->voglioquesto();
}

wreport::Varcode TracedAPI::dammelo()
{
    return api->dammelo();
}

void TracedAPI::prendilo()
{
    fprintf(tracer.trace_file, "%s.prendilo();\n", name.c_str());
    api->prendilo();
}

void TracedAPI::dimenticami()
{
    fprintf(tracer.trace_file, "%s.dimenticami();\n", name.c_str());
    api->dimenticami();
}

int TracedAPI::voglioancora()
{
    return api->voglioancora();
}

const char* TracedAPI::ancora()
{
    return api->ancora();
}

void TracedAPI::critica()
{
    fprintf(tracer.trace_file, "%s.critica();\n", name.c_str());
    api->critica();
}

void TracedAPI::scusa()
{
    fprintf(tracer.trace_file, "%s.scusa();\n", name.c_str());
    api->scusa();
}

void TracedAPI::messages_open_input(const char* filename, const char* mode, Encoding format, bool simplified)
{
    std::string arg1 = str::encode_cstring(filename);
    std::string arg2 = str::encode_cstring(mode);
    std::string enc = File::encoding_name(format);
    fprintf(tracer.trace_file, "%s.messages_open_input(\"%s\", \"%s\", Encoding::%s, %s);\n",
            name.c_str(), arg1.c_str(), arg2.c_str(), enc.c_str(), simplified ? "true" : "false");
    api->messages_open_input(filename, mode, format, simplified);
}

void TracedAPI::messages_open_output(const char* filename, const char* mode, Encoding format)
{
    std::string arg1 = str::encode_cstring(filename);
    std::string arg2 = str::encode_cstring(mode);
    std::string enc = File::encoding_name(format);
    fprintf(tracer.trace_file, "%s.messages_open_output(\"%s\", \"%s\", Encoding::%s);\n",
            name.c_str(), arg1.c_str(), arg2.c_str(), enc.c_str());
    api->messages_open_output(filename, mode, format);
}

bool TracedAPI::messages_read_next()
{
    return api->messages_read_next();
}

void TracedAPI::messages_write_next(const char* template_name)
{
    std::string arg = str::encode_cstring(template_name);
    fprintf(tracer.trace_file, "%s.messages_write_next(\"%s\");\n", name.c_str(), arg.c_str());
    api->messages_write_next(template_name);
}

const char* TracedAPI::spiegal(int ltype1, int l1, int ltype2, int l2)
{
    return api->spiegal(ltype1, l1, ltype2, l2);
}

const char* TracedAPI::spiegat(int ptype, int p1, int p2)
{
    return api->spiegat(ptype, p1, p2);
}

const char* TracedAPI::spiegab(const char* varcode, const char* value)
{
    return api->spiegab(varcode, value);
}

void TracedAPI::fatto()
{
    fprintf(tracer.trace_file, "// %s not used anymore\n", name.c_str());
    api->fatto();
}

}
}
