#ifndef DBALLE_SIMPLE_TRACED_H
#define DBALLE_SIMPLE_TRACED_H

#include <dballe/simple/simple.h>
#include <memory>

namespace dballe {
namespace fortran {

struct Tracer
{
    virtual ~Tracer() {}

    virtual std::unique_ptr<API> wrap_api(int handle, std::unique_ptr<API> api) = 0;

    std::unique_ptr<API> begin(int dbahandle, int handle, const char* url, const char* anaflag, const char* dataflag, const char* attrflag);
    std::unique_ptr<API> begin_messages(int handle, const char* filename, const char* mode, const char* type);

    virtual void log_connect_url(int handle, const char* chosen_dsn) = 0;
    virtual void log_disconnect(int handle) = 0;
    virtual void log_begin(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag) = 0;
    virtual void log_begin_messages(int handle, const char* filename, const char* mode, const char* type) = 0;

    static std::unique_ptr<Tracer> create();
};

struct FileTracer;

struct TracedAPI : public API
{
    FileTracer& tracer;
    std::string name;
    std::unique_ptr<API> api;

    TracedAPI(FileTracer& tracer, const std::string& name, std::unique_ptr<API> api);

    void scopa(const char* repinfofile=0) override;
    void remove_all() override;
    int enqi(const char* param) override;
    signed char enqb(const char* param) override;
    float enqr(const char* param) override;
    double enqd(const char* param) override;
    bool enqc(const char* param, std::string& res) override;
    void seti(const char* param, int value) override;
    void setb(const char* param, signed char value) override;
    void setr(const char* param, float value) override;
    void setd(const char* param, double value) override;
    void setc(const char* param, const char* value) override;
    void setcontextana() override;
    void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) override;
    void setlevel(int ltype1, int l1, int ltype2, int l2) override;
    void enqtimerange(int& pind, int& p1, int& p2) override;
    void settimerange(int pind, int p1, int p2) override;
    void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) override;
    void setdate(int year, int month, int day, int hour, int min, int sec) override;
    void setdatemin(int year, int month, int day, int hour, int min, int sec) override;
    void setdatemax(int year, int month, int day, int hour, int min, int sec) override;
    void unset(const char* param) override;
    void unsetall() override;
    void unsetb() override;
    int quantesono() override;
    void elencamele() override;
    int voglioquesto() override;
    wreport::Varcode dammelo() override;
    void prendilo() override;
    void dimenticami() override;
    int voglioancora() override;
    const char* ancora() override;
    void critica() override;
    void scusa() override;
    void messages_open_input(const char* filename, const char* mode, Encoding format, bool simplified=true) override;
    void messages_open_output(const char* filename, const char* mode, Encoding format) override;
    bool messages_read_next() override;
    void messages_write_next(const char* template_name=0) override;
    const char* spiegal(int ltype1, int l1, int ltype2, int l2) override;
    const char* spiegat(int ptype, int p1, int p2) override;
    const char* spiegab(const char* varcode, const char* value) override;
    void commit() override;
};

}
}

#endif
