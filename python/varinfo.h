/*
 * python/varinfo - DB-All.e Varinfo python bindings
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <Python.h>
#include <wreport/varinfo.h>

extern "C" {

typedef struct {
    PyObject_HEAD
    wreport::Varinfo info;
} dpy_Varinfo;

}

namespace dballe {
namespace python {

dpy_Varinfo* varinfo_create(const wreport::Varinfo& v);

void register_varinfo(PyObject* m);

}
}
