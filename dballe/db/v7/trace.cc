#include "trace.h"
#include "dballe/core/query.h"
#include <wreport/error.h>
#include <unistd.h>

using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {

namespace {

std::string query_to_string(const Query& query)
{
    std::stringstream json_buf;
    core::JSONWriter writer(json_buf);
    core::Query::downcast(query).serialize(writer);
    return json_buf.str();
}

}

namespace trace {

Step::Step(const std::string& name)
    : name(name), start(clock())
{
}

Step::Step(const std::string& name, const std::string& detail)
    : name(name), detail(detail), start(clock())
{
}

Step::~Step()
{
    delete child;
    delete sibling;
}

void Step::done()
{
    end = clock();
}

unsigned Step::elapsed_usec() const
{
    return (end - start) * 1000000 / CLOCKS_PER_SEC;
}


Tracer<> Transaction::trace_query_stations(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("query_stations", query_to_string(query))));
}

Tracer<> Transaction::trace_query_station_data(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("query_station_data", query_to_string(query))));
}

Tracer<> Transaction::trace_query_data(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("query_data", query_to_string(query))));
}

Tracer<> Transaction::trace_query_summary(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("query_summary", query_to_string(query))));
}

Tracer<> Transaction::trace_import(unsigned count)
{
    return Tracer<>(add_child(new trace::Step("import", std::to_string(count))));
}

Tracer<> Transaction::trace_export_msgs(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("export_msgs", query_to_string(query))));
}

Tracer<> Transaction::trace_insert_station_data()
{
    return Tracer<>(add_child(new trace::Step("insert_station_data")));
}

Tracer<> Transaction::trace_insert_data()
{
    return Tracer<>(add_child(new trace::Step("insert_data")));
}

Tracer<> Transaction::trace_add_station_vars()
{
    return Tracer<>(add_child(new trace::Step("insert_data")));
}

}

void ProfileTrace::print(FILE* out)
{
    fprintf(stderr, "Transaction end: %u queries\n", profile_count_select + profile_count_insert + profile_count_update + profile_count_delete);
    fprintf(stderr, "   %u selects, %u rows\n", profile_count_select, profile_count_select_rows);
    fprintf(stderr, "   %u inserts, %u rows\n", profile_count_insert, profile_count_insert_rows);
    fprintf(stderr, "   %u updates, %u rows\n", profile_count_update, profile_count_update_rows);
    fprintf(stderr, "   %u deletes\n", profile_count_delete);
}

void ProfileTrace::reset()
{
    profile_count_select = 0;
    profile_count_insert = 0;
    profile_count_update = 0;
    profile_count_delete = 0;
    profile_count_select_rows = 0;
    profile_count_insert_rows = 0;
    profile_count_update_rows = 0;
}

void QuietProfileTrace::print(FILE* out)
{
}

#if 0
TraceOp::TraceOp()
{
}

TraceOp::TraceOp(Trace& trace, const char* operation)
    : trace(&trace), start(clock())
{
    trace.writer.start_mapping();
    add("op", operation);
    add_list("cmdline", trace.argv);
    add("pid", trace.pid);
    add("url", trace.db_url);
}

TraceOp::~TraceOp()
{
    if (!trace) return;
    trace->output_abort();
}

void TraceOp::done()
{
    if (!trace) return;
    int elapsed = (clock() - start) * 1000 / CLOCKS_PER_SEC;
    add("elapsed", elapsed);
    trace->writer.end_mapping();
    trace->output_flush();
}

void TraceOp::add_query(const Query& query)
{
    if (!trace) return;
    trace->writer.add("query");
    core::Query::downcast(query).serialize(trace->writer);
}

Trace::Trace()
    : writer(json_buf)
{
    const char* outdir = getenv("DBA_LOGDIR");
    if (outdir)
    {
        read_argv();
        pid = getpid();

        time_t t = time(NULL);
        struct tm tmp;
        localtime_r(&t, &tmp);
        char buf[20];
        strftime(buf, 20, "%Y%m%d-%H%M%S-", &tmp);
        out_fname = outdir;
        out_fname += "/";
        out_fname += buf;
        out_fname += std::to_string(pid);
        out_fname += ".log";

        out = fopen(out_fname.c_str(), "at");
        if (!out)
            error_system::throwf("cannot open file %s", out_fname.c_str());
    }
}
#endif

CollectTrace::~CollectTrace()
{
    for (auto& i: steps)
        delete i;
//    if (out) fclose(out);
}

#if 0
void Trace::read_argv()
{
    FILE* in = fopen("/proc/self/cmdline", "rb");
    if (!in) throw error_system("cannot open /proc/self/cmdline");

    std::string cur;
    char c;
    while ((c = getc(in)) != EOF)
    {
        if (c == 0)
        {
            argv.push_back(cur);
            cur.clear();
        } else
            cur += c;
    }

    if (ferror(in))
    {
        int e = errno;
        fclose(in);
        throw error_system("cannot read from /proc/self/cmdline", e);
    }
}

void Trace::output_abort()
{
    writer.reset();
    json_buf.str("");
}

void Trace::output_flush()
{
    fputs(json_buf.str().c_str(), out);
    putc('\n', out);
    writer.reset();
    json_buf.str("");
}
#endif

Tracer<> CollectTrace::trace_connect(const std::string& url)
{
    steps.push_back(new trace::Step("connect", url));
    return Tracer<>(steps.back());
}

Tracer<> CollectTrace::trace_reset(const char* repinfo_file)
{
    steps.push_back(new trace::Step("reset", repinfo_file ? repinfo_file : ""));
    return steps.back();
}

Tracer<trace::Transaction> CollectTrace::trace_transaction()
{
    trace::Transaction* res = new trace::Transaction;
    steps.push_back(res);
    return res;
}

Tracer<> CollectTrace::trace_remove_station_data(const Query& query)
{
    steps.push_back(new trace::Step("remove_station_data", query_to_string(query)));
    return Tracer<>(steps.back());
}

Tracer<> CollectTrace::trace_remove(const Query& query)
{
    steps.push_back(new trace::Step("remove", query_to_string(query)));
    return Tracer<>(steps.back());
}

Tracer<> CollectTrace::trace_remove_all()
{
    steps.push_back(new trace::Step("remove_all"));
    return Tracer<>(steps.back());
}

Tracer<> CollectTrace::trace_vacuum()
{
    steps.push_back(new trace::Step("vacuum"));
    return Tracer<>(steps.back());
}

static bool _in_test_suite = false;

bool Trace::in_test_suite() { return _in_test_suite; }
void Trace::set_in_test_suite() { _in_test_suite = true; }


}
}
}
