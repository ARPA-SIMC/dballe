/*
 * Copyright (C) 2013--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <string>
#include <cstdlib>
#include <cstdio>

using namespace std;

namespace {

std::string c_escape(const std::string& str)
{
    string res;
    for (string::const_iterator i = str.begin(); i != str.end(); ++i)
        if (*i == '\n')
            res += "\\n";
        else if (*i == '\t')
            res += "\\t";
        else if (*i == 0 || iscntrl(*i))
        {
            char buf[5];
            snprintf(buf, 5, "\\x%02x", (unsigned int)*i);
            res += buf;
        }
        else if (*i == '"' || *i == '\\')
        {
            res += "\\";
            res += *i;
        }
        else
            res += *i;
    return res;
}

}


namespace dballe {
namespace fortran {

bool do_trace = false;
FILE* trace_file = 0;

void trace_init()
{
    // Init API tracing if requested
    const char* tracefile = getenv("DBALLE_TRACE_FORTRAN");
    if (!tracefile) tracefile = getenv("DBA_FORTRAN_TRACE");
    if (tracefile)
    {
        trace_file = fopen(tracefile, "at");
        setvbuf(trace_file, nullptr, _IOLBF, 0);
        do_trace = true;
        fprintf(trace_file, "// ** Execution begins **\n");
    }
    else
    {
        trace_file = 0;
        do_trace = false;
    }
}

void log_error(wreport::error& e)
{
    fprintf(trace_file, "// error: %s\n", e.what());
}

void log_result(wreport::Varcode code)
{
    fprintf(trace_file, "wassert(actual(code) == WR_VAR(0, %d %d);\n", WR_VAR_X(code), WR_VAR_Y(code));
}

void log_result(int res)
{
    fprintf(trace_file, "wassert(actual(ires) == %d);\n", res);
}

void log_result(const char* res)
{
    if (res)
    {
        string arg = c_escape(res);
        fprintf(trace_file, "wassert(actual(sres) == \"%s\");\n", arg.c_str());
    } else
        fprintf(trace_file, "wassert(actual(sres) == null);\n");
}

void SessionTracer::log_quantesono()
{
    fprintf(trace_file, "ires = %s.quantesono();\n", trace_tag);
}

void SessionTracer::log_voglioquesto()
{
    fprintf(trace_file, "ires = %s.voglioquesto();\n", trace_tag);
}

void SessionTracer::log_voglioancora()
{
    fprintf(trace_file, "ires = %s.voglioancora();\n", trace_tag);
}

void SessionTracer::log_dammelo()
{
    fprintf(trace_file, "code = %s.dammelo();\n", trace_tag);
}

void SessionTracer::log_ancora()
{
    fprintf(trace_file, "sres = %s.ancora();\n", trace_tag);
}

void SessionTracer::log_messages_read_next()
{
    fprintf(trace_file, "ires = %s.messages_read_next();\n", trace_tag);
}

}
}

void dballe_fortran_debug_flush_trace_log()
{
    if (dballe::fortran::trace_file) fflush(dballe::fortran::trace_file);
}
