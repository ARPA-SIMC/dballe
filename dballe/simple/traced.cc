#include "traced.h"
#include <wreport/error.h>
#include <cstdlib>
#include <cstdio>

namespace dballe {
namespace fortran {

namespace {

struct NullTracer : public Tracer
{
};

struct FileTracer : public Tracer
{
    std::string fname;
    FILE* trace_file = nullptr;

    FileTracer(const std::string& fname)
        : fname(fname)
    {
        trace_file = fopen(fname.c_str(), "at");
        if (!trace_file)
            wreport::error_system::throwf("Cannot open %s", fname.c_str());
        setvbuf(trace_file, nullptr, _IOLBF, 0);
        fprintf(trace_file, "// ** Execution begins **\n");
    }
};

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


TracedAPI::TracedAPI(const std::string& name, std::unique_ptr<API> api)
    : name(name), api(std::move(api))
{
}

void TracedAPI::scopa(const char* repinfofile)
{
    api->scopa(repinfofile);
}

void TracedAPI::remove_all()
{
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
    api->seti(param, value);
}

void TracedAPI::setb(const char* param, signed char value)
{
    api->setb(param, value);
}

void TracedAPI::setr(const char* param, float value)
{
    api->setr(param, value);
}

void TracedAPI::setd(const char* param, double value)
{
    api->setd(param, value);
}

void TracedAPI::setc(const char* param, const char* value)
{
    api->setc(param, value);
}

void TracedAPI::setcontextana()
{
    api->setcontextana();
}

void TracedAPI::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
    api->enqlevel(ltype1, l1, ltype2, l2);
}

void TracedAPI::setlevel(int ltype1, int l1, int ltype2, int l2)
{
    api->setlevel(ltype1, l1, ltype2, l2);
}

void TracedAPI::enqtimerange(int& ptype, int& p1, int& p2)
{
    api->enqtimerange(ptype, p1, p2);
}

void TracedAPI::settimerange(int ptype, int p1, int p2)
{
    api->settimerange(ptype, p1, p2);
}

void TracedAPI::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    api->enqdate(year, month, day, hour, min, sec);
}

void TracedAPI::setdate(int year, int month, int day, int hour, int min, int sec)
{
    api->setdate(year, month, day, hour, min, sec);
}

void TracedAPI::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
    api->setdatemin(year, month, day, hour, min, sec);
}

void TracedAPI::setdatemax(int year, int month, int day, int hour, int min, int sec)
{
    api->setdatemax(year, month, day, hour, min, sec);
}

void TracedAPI::unset(const char* param)
{
    api->unset(param);
}

void TracedAPI::unsetall()
{
    api->unsetall();
}

void TracedAPI::unsetb()
{
    api->unsetb();
}

int TracedAPI::quantesono()
{
    return api->quantesono();
}

void TracedAPI::elencamele()
{
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
    api->prendilo();
}

void TracedAPI::dimenticami()
{
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
    api->critica();
}

void TracedAPI::scusa()
{
    api->scusa();
}

void TracedAPI::messages_open_input(const char* filename, const char* mode, Encoding format, bool simplified)
{
    api->messages_open_input(filename, mode, format, simplified);
}

void TracedAPI::messages_open_output(const char* filename, const char* mode, Encoding format)
{
    api->messages_open_output(filename, mode, format);
}

bool TracedAPI::messages_read_next()
{
    return api->messages_read_next();
}

void TracedAPI::messages_write_next(const char* template_name)
{
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
    api->fatto();
}

}
}
