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
#include <wreport/varinfo.h>

#ifndef FDBA_TRACE_H
#define FDBA_TRACE_H

#define IF_TRACING(code) do { if (dballe::fortran::do_trace) { code; } } while(0)

namespace dballe {
namespace fortran {

extern bool do_trace;

void trace_init();

void log_error(wreport::error& e);
void log_result(wreport::Varcode res);
void log_result(int res);
void log_result(const char* res);

struct SessionTracer
{
    char trace_tag[10];

    void log_quantesono();
    void log_voglioquesto();
    void log_voglioancora();
    void log_dammelo();
    void log_ancora();
    void log_messages_read_next();
};



}
}

#endif

