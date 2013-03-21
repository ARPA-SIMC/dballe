/*
 * python/record - DB-All.e Record python bindings
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
#include "record.h"

extern "C" {

typedef struct {
    PyObject_HEAD
} dballe_RecordObject;

static PyTypeObject dballe_RecordType = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.Record",              // tp_name
    sizeof(dballe_RecordObject),  // tp_basicsize
    0,                         // tp_itemsize
    0,                         // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    0,                         // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    0,                         // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e Record",       // tp_doc
};

}

namespace dballe {
namespace python {

void register_record(PyObject* m)
{
    dballe_RecordType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dballe_RecordType) < 0)
        return;

    Py_INCREF(&dballe_RecordType);
    PyModule_AddObject(m, "Record", (PyObject*)&dballe_RecordType);
}

}
}
