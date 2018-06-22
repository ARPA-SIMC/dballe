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
#ifndef DBALLE_DB_TRACE_H
#define DBALLE_DB_TRACE_H

#include <dballe/core/json.h>
#include <wreport/varinfo.h>
#include <sys/types.h>
#include <vector>
#include <sstream>
#include <memory>
#include <ctime>

namespace dballe {
struct Query;

namespace db {

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


}
}

#endif
