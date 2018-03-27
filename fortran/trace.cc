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

void log_presentati_url(int handle, const char* chosen_dsn)
{
    string arg1 = c_escape(chosen_dsn);
    fprintf(trace_file, "auto db%d(DB::connect_from_url(\"%s\"));\n", handle, arg1.c_str());
}

void log_presentati_dsn(int handle, const char* dsn, const char* user, const char* pwd)
{
    string arg1 = c_escape(dsn);
    string arg2 = c_escape(user);
    fprintf(trace_file, "auto db%d(DB::connect(\"%s\", \"%s\", \"xxx\"));\n",
            handle, arg1.c_str(), arg2.c_str());
}

void log_arrivederci(int handle)
{
    fprintf(trace_file, "// db%d not used anymore\n", handle);
}

void log_error(wreport::error& e)
{
    fprintf(trace_file, "// error: %s\n", e.what());
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

void SessionTracer::log_preparati(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag)
{
    snprintf(trace_tag, 10, "dbapi%d", handle);
    fprintf(trace_file, "DbAPI %s(*db%d, \"%s\", \"%s\", \"%s\");\n",
            trace_tag, dbahandle, anaflag, dataflag, attrflag);
}

void SessionTracer::log_messaggi(int handle, const char* filename, const char* mode, const char* type)
{
    snprintf(trace_tag, 10, "msgapi%d", handle);
    string arg1(c_escape(filename));
    fprintf(trace_file, "MsgAPI %s(\"%s\", \"%s\", %s);\n",
            trace_tag, arg1.c_str(), mode, type);
}

void SessionTracer::log_func(const char* call)
{
    fprintf(trace_file, "%s.%s();\n", trace_tag, call);
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
    fprintf(trace_file, "sres = %s.dammelo();\n", trace_tag);
}

void SessionTracer::log_ancora()
{
    fprintf(trace_file, "sres = %s.ancora();\n", trace_tag);
}

void SessionTracer::log_set(const char* parm, int val)
{
    string arg = c_escape(parm);
    fprintf(trace_file, "%s.seti(\"%s\", %d);\n", trace_tag, arg.c_str(), val);
}

void SessionTracer::log_set(const char* parm, double val)
{
    string arg = c_escape(parm);
    fprintf(trace_file, "%s.setd(\"%s\", %f);\n", trace_tag, arg.c_str(), val);
}

void SessionTracer::log_set(const char* parm, const char* val)
{
    string arg1 = c_escape(parm);
    string arg2 = c_escape(val);
    fprintf(trace_file, "%s.setc(\"%s\", \"%s\");\n", trace_tag, arg1.c_str(), arg2.c_str());
}

void SessionTracer::log_setlevel(int ltype1, int l1, int ltype2, int l2)
{
    fprintf(trace_file, "%s.setlevel(%d, %d, %d, %d);\n", trace_tag, ltype1, l1, ltype2, l2);
}

void SessionTracer::log_settimerange(int pind, int p1, int p2)
{
    fprintf(trace_file, "%s.settimerange(%d, %d, %d);\n", trace_tag, pind, p1, p2);
}

void SessionTracer::log_setdate(int y, int m, int d, int ho, int mi, int se, const char* what)
{
    fprintf(trace_file, "%s.setdate%s(%d, %d, %d, %d, %d, %d);\n", trace_tag, what, y, m, d, ho, mi, se);
}

void SessionTracer::log_unset(const char* parm)
{
    string arg = c_escape(parm);
    fprintf(trace_file, "%s.unset(\"%s\");\n", trace_tag, arg.c_str());
}

void SessionTracer::log_scopa(const char* fname)
{
    if (fname)
    {
        string arg = c_escape(fname);
        fprintf(trace_file, "%s.scopa(\"%s\");\n", trace_tag, arg.c_str());
    }
    else
        fprintf(trace_file, "%s.scopa();\n", trace_tag);
}

void SessionTracer::log_fatto()
{
    fprintf(trace_file, "// %s not used anymore\n", trace_tag);
}

void SessionTracer::log_messages_open_input(const char* fname, const char* mode, const char* format, bool simplified)
{
    string arg1 = c_escape(fname);
    string arg2 = c_escape(mode);
    fprintf(trace_file, "%s.messages_open_input(\"%s\", \"%s\", %s, %s);\n",
            trace_tag, arg1.c_str(), arg2.c_str(), format, simplified ? "true" : "false");
}

void SessionTracer::log_messages_open_output(const char* fname, const char* mode, const char* format)
{
    string arg1 = c_escape(fname);
    string arg2 = c_escape(mode);
    fprintf(trace_file, "%s.messages_open_output(\"%s\", \"%s\", %s);\n",
            trace_tag, arg1.c_str(), arg2.c_str(), format);
}

void SessionTracer::log_messages_read_next()
{
    fprintf(trace_file, "ires = %s.messages_read_next();\n", trace_tag);
}

void SessionTracer::log_messages_write_next(const char* template_name)
{
    string arg = c_escape(template_name);
    fprintf(trace_file, "%s.messages_write_next(\"%s\");\n", trace_tag, arg.c_str());
}

}
}

void dballe_fortran_debug_flush_trace_log()
{
    if (dballe::fortran::trace_file) fflush(dballe::fortran::trace_file);
}
