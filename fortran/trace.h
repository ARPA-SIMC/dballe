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
#include <wreport/error.h>

#ifndef FDBA_TRACE_H
#define FDBA_TRACE_H

#define IF_TRACING(code) do { if (dballe::fortran::do_trace) { code; } } while(0)

namespace dballe {
namespace fortran {

extern bool do_trace;

void trace_init();

void log_presentati_url(int handle, const char* chosen_dsn);
void log_presentati_dsn(int handle, const char* dsn, const char* user, const char* pwd);
void log_arrivederci(int handle);
void log_error(wreport::error& e);
void log_result(int res);
void log_result(const char* res);

struct SessionTracer
{
    char trace_tag[10];

    void log_preparati(int dbahandle, int handle, const char* anaflag, const char* dataflag, const char* attrflag);
    void log_messaggi(int handle, const char* filename, const char* mode, const char* type);
    void log_func(const char* call);
    void log_quantesono();
    void log_voglioquesto();
    void log_voglioancora();
    void log_dammelo();
    void log_ancora();
    void log_set(const char* parm, int val);
    void log_set(const char* parm, double val);
    void log_set(const char* parm, const char* val);
    void log_setlevel(int ltype1, int l1, int ltype2, int l2);
    void log_settimerange(int pind, int p1, int p2);
    void log_setdate(int y, int m, int d, int ho, int mi, int se, const char* what="");
    void log_unset(const char* parm);
    void log_scopa(const char* fname=0);
    void log_fatto();
    void log_messages_open_input(const char* fname, const char* mode, const char* format, bool simplified=true);
    void log_messages_open_output(const char* fname, const char* mode, const char* format);
    void log_messages_read_next();
    void log_messages_write_next(const char* template_name);
};



}
}

#endif

