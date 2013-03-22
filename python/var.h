/*
 * python/var - DB-All.e Var python bindings
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
#include <wreport/var.h>

extern "C" {

typedef struct {
    PyObject_HEAD
    wreport::Var var;
} dpy_Var;

PyAPI_DATA(PyTypeObject) dpy_Var_Type;

#define dpy_Var_Check(ob) \
    (Py_TYPE(ob) == &dpy_Var_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_Var_Type))
}

namespace dballe {
namespace python {

PyObject* var_value_to_python(const wreport::Var& v);

dpy_Var* var_create(const wreport::Varinfo& v);
dpy_Var* var_create(const wreport::Varinfo& v, int val);
dpy_Var* var_create(const wreport::Varinfo& v, double val);
dpy_Var* var_create(const wreport::Varinfo& v, const char* val);
dpy_Var* var_create(const wreport::Var& v);

void register_var(PyObject* m);

}
}
