/*
 * dballe/db/trace - Trace and measure DB queries
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include "trace.h"
#include "dballe/core/query.h"
#include <wreport/error.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

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
        out_fname += to_string(pid);
        out_fname += ".log";

        out = fopen(out_fname.c_str(), "at");
        if (!out)
            error_system::throwf("cannot open file %s", out_fname.c_str());
    }
}

Trace::~Trace()
{
    if (out) fclose(out);
}

void Trace::read_argv()
{
    FILE* in = fopen("/proc/self/cmdline", "rb");
    if (!in) throw error_system("cannot open /proc/self/cmdline");

    string cur;
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
    json_buf.clear();
}

void Trace::output_flush()
{
    fputs(json_buf.c_str(), out);
    putc('\n', out);
    writer.reset();
    json_buf.clear();
}

Trace::Tracer Trace::trace_connect(const std::string& url)
{
    if (!out) return Tracer(new TraceOp());
    db_url = url;
    return Tracer(new TraceOp(*this, "connect"));
}

Trace::Tracer Trace::trace_reset(const char* repinfo_file)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "reset"));
    if (repinfo_file)
        res->add("repinfo", repinfo_file);
    else
        res->add_null("repinfo");
    return res;
}

Trace::Tracer Trace::trace_remove_station_data(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "remove_station_data"));
    res->add_query(query);
    return res;
}

Trace::Tracer Trace::trace_remove(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "remove"));
    res->add_query(query);
    return res;
}

Trace::Tracer Trace::trace_remove_all()
{
    if (!out) return Tracer(new TraceOp());
    return Tracer(new TraceOp(*this, "remove_all"));
}

Trace::Tracer Trace::trace_vacuum()
{
    if (!out) return Tracer(new TraceOp());
    return Tracer(new TraceOp(*this, "vacuum"));
}

Trace::Tracer Trace::trace_query_stations(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "query_stations"));
    res->add_query(query);
    return res;
}

Trace::Tracer Trace::trace_query_station_data(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "query_station_data"));
    res->add_query(query);
    return res;
}

Trace::Tracer Trace::trace_query_data(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "query_data"));
    res->add_query(query);
    return res;
}

Trace::Tracer Trace::trace_query_summary(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "query_summary"));
    res->add_query(query);
    return res;
}

Trace::Tracer Trace::trace_export_msgs(const Query& query)
{
    if (!out) return Tracer(new TraceOp());
    Tracer res(new TraceOp(*this, "export_msgs"));
    res->add_query(query);
    return res;
}


}
}
