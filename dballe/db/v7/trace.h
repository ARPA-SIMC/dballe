#ifndef DBALLE_DB_V7_TRACE_H
#define DBALLE_DB_V7_TRACE_H

#include <dballe/fwd.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/core/json.h>
#include <sstream>
#include <string>
#include <vector>

namespace dballe {
namespace db {
namespace v7 {

namespace trace {

/**
 * One operation being traced
 */
class Step
{
protected:
    /// Parent operation in the operation stack
    Step* parent = nullptr;
    /// First child operation in the operation stack
    Step* child = nullptr;
    /// Next sibling operation in the operation stack
    Step* sibling = nullptr;
    /// Operation name
    std::string name;
    /// Optional details about the operation
    std::string detail;
    /// Number of database rows affected
    unsigned rows = 0;
    /// Timing start
    clock_t start = 0;
    /// Timing end
    clock_t end = 0;

    template<typename T>
    void add_sibling(T* step)
    {
        if (!sibling)
        {
            sibling = step;
            step->parent = parent;
        }
        else
            sibling->add_sibling(step);
    }

public:
    Step(const std::string& name);
    Step(const std::string& name, const std::string& detail);
    ~Step();

    void done();
    unsigned elapsed_usec() const;

    template<typename T>
    void add_child(T* step)
    {
        if (!child)
        {
            child = step;
            step->parent = this;
        }
        else
            child->add_sibling(step);
    }
};

}

/**
 * Smart pointer for trace::Step objects, which calls done() when going out of
 * scope
 */
template<typename Step>
class Tracer
{
protected:
    Step* step;

public:
    Tracer(Step* step) : step(step) {}
    Tracer(const Tracer&) = delete;
    Tracer(Tracer&& o)
        : step(o.step)
    {
        o.step = nullptr;
    }
    Tracer& operator=(const Tracer&) = delete;
    Tracer& operator=(Tracer&&) = delete;
    ~Tracer()
    {
        if (step) step->done();
    }

    Step* operator->() { return step; }
    operator bool() const { return step; }
};

struct Trace
{
    virtual ~Trace() {}

    virtual Tracer<> trace_connect(const std::string& url) = 0;
    virtual Tracer<> trace_reset(const char* repinfo_file=0) = 0;
    virtual Tracer<> trace_remove_station_data(const Query& query) = 0;
    virtual Tracer<> trace_remove(const Query& query) = 0;
    virtual Tracer<> trace_remove_all() = 0;
    virtual Tracer<> trace_vacuum() = 0;
    virtual Tracer<> trace_query_stations(const Query& query) = 0;
    virtual Tracer<> trace_query_station_data(const Query& query) = 0;
    virtual Tracer<> trace_query_data(const Query& query) = 0;
    virtual Tracer<> trace_query_summary(const Query& query) = 0;
    virtual Tracer<> trace_export_msgs(const Query& query) = 0;

    static bool in_test_suite();
    static void set_in_test_suite();
};

struct NullTrace : public Trace
{
    Tracer<> trace_connect(const std::string& url) override { return Tracer<>(nullptr); }
    Tracer<> trace_reset(const char* repinfo_file=0) override { return Tracer<>(nullptr); }
    Tracer<> trace_remove_station_data(const Query& query) override { return Tracer<>(nullptr); }
    Tracer<> trace_remove(const Query& query) override { return Tracer<>(nullptr); }
    Tracer<> trace_remove_all() override { return Tracer<>(nullptr); }
    Tracer<> trace_vacuum() override { return Tracer<>(nullptr); }
    Tracer<> trace_query_stations(const Query& query) override { return Tracer<>(nullptr); }
    Tracer<> trace_query_station_data(const Query& query) override { return Tracer<>(nullptr); }
    Tracer<> trace_query_data(const Query& query) override { return Tracer<>(nullptr); }
    Tracer<> trace_query_summary(const Query& query) override { return Tracer<>(nullptr); }
    Tracer<> trace_export_msgs(const Query& query) override { return Tracer<>(nullptr); }
};

class CollectTrace : public Trace
{
protected:
    std::vector<trace::Step*> steps;
#if 0
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
#endif

public:
    CollectTrace() = default;
    CollectTrace(const CollectTrace&) = delete;
    CollectTrace(CollectTrace&&) = delete;
    CollectTrace& operator=(const CollectTrace&) = delete;
    CollectTrace& operator=(CollectTrace&&) = delete;
    ~CollectTrace();

    Tracer<> trace_connect(const std::string& url) override;
    Tracer<> trace_reset(const char* repinfo_file=0) override;
    Tracer<> trace_remove_station_data(const Query& query) override;
    Tracer<> trace_remove(const Query& query) override;
    Tracer<> trace_remove_all() override;
    Tracer<> trace_vacuum() override;
    Tracer<> trace_query_stations(const Query& query) override;
    Tracer<> trace_query_station_data(const Query& query) override;
    Tracer<> trace_query_data(const Query& query) override;
    Tracer<> trace_query_summary(const Query& query) override;
    Tracer<> trace_export_msgs(const Query& query) override;
};

struct QuietCollectTrace : public CollectTrace
{
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
