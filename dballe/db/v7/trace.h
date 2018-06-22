#ifndef DBALLE_DB_V7_TRACE_H
#define DBALLE_DB_V7_TRACE_H

#include <dballe/fwd.h>
#include <dballe/core/json.h>
#include <sstream>
#include <string>
#include <vector>

namespace dballe {
namespace db {
namespace v7 {

class TraceOp;

class Trace
{
protected:
    // Command line used to start the current process
    std::vector<std::string> argv;

    // Process ID of the current process (cached getpid() result)
    pid_t pid;

    // Database connection URL
    std::string db_url;

    // JSON output buffer, holding one JSON record
    std::stringstream json_buf;

    // JSON serializer
    core::JSONWriter writer;

    // Output file name
    std::string out_fname;

    // Output file
    FILE* out = 0;

    // Populate argv
    void read_argv();

    // Cancel the current output, resetting json_buf
    void output_abort();

    // Flush the current output, then reset json_buf
    void output_flush();


public:
    typedef std::unique_ptr<TraceOp> Tracer;

    Trace();
    ~Trace();

    Tracer trace_connect(const std::string& url);
    Tracer trace_reset(const char* repinfo_file=0);
    Tracer trace_remove_station_data(const Query& query);
    Tracer trace_remove(const Query& query);
    Tracer trace_remove_all();
    Tracer trace_vacuum();
    Tracer trace_query_stations(const Query& query);
    Tracer trace_query_station_data(const Query& query);
    Tracer trace_query_data(const Query& query);
    Tracer trace_query_summary(const Query& query);
    Tracer trace_export_msgs(const Query& query);

    static bool in_test_suite();
    static void set_in_test_suite();

    friend class TraceOp;
};

class TraceOp
{
protected:
    Trace* trace = 0;
    clock_t start;

public:
    TraceOp();
    TraceOp(Trace& trace, const char* operation);
    ~TraceOp();

    void done();

    template<typename T>
    void add_list(const char* key, const T& val)
    {
        trace->writer.add(key);
        trace->writer.add_list(val);
    }

    void add_null(const char* key)
    {
        trace->writer.add(key);
        trace->writer.add_null();
     }

    template<typename T>
    void add(const char* key, const T& val)
    {
        trace->writer.add(key);
        trace->writer.add(val);
    }

    void add_query(const Query& query);
};



struct SQLTrace
{
    virtual ~SQLTrace() {}

    virtual void trace_select(const std::string& query, unsigned rows=0) = 0;
    virtual void trace_insert(const std::string& query, unsigned rows=0) = 0;
    virtual void trace_update(const std::string& query, unsigned rows=0) = 0;
    virtual void trace_delete(const std::string& query) = 0;
    virtual void trace_select_row(unsigned count=1) = 0;
    virtual void trace_insert_row(unsigned count=1) = 0;
    virtual void trace_update_row(unsigned count=1) = 0;

    virtual void print(FILE* out) = 0;

    virtual void reset() = 0;
};

struct ProfileTrace : public SQLTrace
{
    unsigned profile_count_select = 0;
    unsigned profile_count_insert = 0;
    unsigned profile_count_update = 0;
    unsigned profile_count_delete = 0;
    unsigned profile_count_select_rows = 0;
    unsigned profile_count_insert_rows = 0;
    unsigned profile_count_update_rows = 0;

    void trace_select(const std::string& query, unsigned rows=0) override { ++profile_count_select; profile_count_select_rows += rows; }
    void trace_insert(const std::string& query, unsigned rows=0) override { ++profile_count_insert; profile_count_insert_rows += rows; }
    void trace_update(const std::string& query, unsigned rows=0) override { ++profile_count_update; profile_count_update_rows += rows; }
    void trace_delete(const std::string& query) override { ++profile_count_delete; }
    void trace_select_row(unsigned count=1) override { profile_count_select_rows += count; }
    void trace_insert_row(unsigned count=1) override { profile_count_insert_rows += count; }
    void trace_update_row(unsigned count=1) override { profile_count_update_rows += count; }

    void print(FILE* out) override;
    void reset() override;
};

struct QuietProfileTrace : public ProfileTrace
{
    void print(FILE* out) override;
};

}
}
}

#endif
