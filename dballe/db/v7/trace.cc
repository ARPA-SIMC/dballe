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

std::vector<std::string> read_argv()
{
    std::vector<std::string> argv;
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
    return argv;
}

std::string format_time(time_t time)
{
    struct tm tmp;
    localtime_r(&time, &tmp);
    char buf[20];
    strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", &tmp);
    return buf;
}

std::string format_time_fname(time_t time)
{
    struct tm tmp;
    localtime_r(&time, &tmp);
    char buf[20];
    strftime(buf, 20, "%Y%m%d-%H%M%S", &tmp);
    return buf;
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

void Step::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("name", name);
    writer.add("detail", detail);
    writer.add("rows", (int)rows);
    writer.add("usecs", (int)elapsed_usec());
    if (child)
    {
        writer.add("ops");
        writer.start_list();
        for (Step* s = child; s; s = s->sibling)
            s->to_json(writer);
        writer.end_list();
    }
    writer.end_mapping();
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

Tracer<> Transaction::trace_func(const std::string& name)
{
    return Tracer<>(add_child(new trace::Step(name)));
}

Tracer<> Transaction::trace_remove_station_data(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("remove_station_data", query_to_string(query))));
}

Tracer<> Transaction::trace_remove_data(const Query& query)
{
    return Tracer<>(add_child(new trace::Step("remove_data", query_to_string(query))));
}

}


QuietCollectTrace::~QuietCollectTrace()
{
    for (auto& i: steps)
        delete i;
}

Tracer<> QuietCollectTrace::trace_connect(const std::string& url)
{
    steps.push_back(new trace::Step("connect", url));
    return Tracer<>(steps.back());
}

Tracer<> QuietCollectTrace::trace_reset(const char* repinfo_file)
{
    steps.push_back(new trace::Step("reset", repinfo_file ? repinfo_file : ""));
    return steps.back();
}

Tracer<trace::Transaction> QuietCollectTrace::trace_transaction()
{
    trace::Transaction* res = new trace::Transaction;
    steps.push_back(res);
    return res;
}

Tracer<> QuietCollectTrace::trace_remove_all()
{
    steps.push_back(new trace::Step("remove_all"));
    return Tracer<>(steps.back());
}

Tracer<> QuietCollectTrace::trace_vacuum()
{
    steps.push_back(new trace::Step("vacuum"));
    return Tracer<>(steps.back());
}


CollectTrace::CollectTrace(const std::string& logdir)
    : logdir(logdir), start(time(nullptr))
{
}

void CollectTrace::save()
{
    pid_t pid = getpid();
    std::stringstream json_buf;
    core::JSONWriter writer(json_buf);
    writer.start_mapping();

    writer.add("cmdline");
    writer.add_list(read_argv());
    writer.add("pid", (int)pid);
    writer.add("start", format_time(start));
    writer.add("end", format_time(time(nullptr)));

    writer.add("ops");
    writer.start_list();
    for (const auto& s: steps)
        s->to_json(writer);
    writer.end_list();

    writer.end_mapping();

    std::string fname = logdir;
    fname += "/";
    fname += format_time_fname(start);
    fname += "-";
    fname += std::to_string(pid);
    fname += ".json";

    FILE* out = fopen(fname.c_str(), "wt");
    fwrite(json_buf.str().data(), json_buf.str().size(), 1, out);
    putc('\n', out);
    fclose(out);
}


static bool _in_test_suite = false;

bool Trace::in_test_suite() { return _in_test_suite; }
void Trace::set_in_test_suite() { _in_test_suite = true; }


}
}
}
