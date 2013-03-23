/*
 * python/cursor - DB-All.e DB cursor python bindings
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
#include <dballe/db/db.h>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::db::Cursor* cur;
} dpy_Cursor;

PyAPI_DATA(PyTypeObject) dpy_Cursor_Type;

#define dpy_DB_Check(ob) \
    (Py_TYPE(ob) == &dpy_Cursor_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_Cursor_Type))
}

namespace dballe {
namespace python {

void register_cursor(PyObject* m);

}
}
