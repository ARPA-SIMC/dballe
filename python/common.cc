/*
 * python/common - Common functions for DB-All.e python bindings
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
#include "common.h"
#include "dballe/core/aliases.h"

using namespace wreport;

namespace dballe {
namespace python {

wreport::Varcode resolve_varcode(const char* name)
{
    wreport::Varcode res = 0;
    if ((res = varcode_alias_resolve(name)) == 0)
        res = descriptor_code(name);
    return res;
}

PyObject* format_varcode(wreport::Varcode code)
{
    char buf[7];
    snprintf(buf, 7, "%c%02d%03d",
            WR_VAR_F(code) == 0 ? 'B' :
            WR_VAR_F(code) == 1 ? 'R' :
            WR_VAR_F(code) == 2 ? 'C' :
            WR_VAR_F(code) == 3 ? 'D' : '?',
            WR_VAR_X(code), WR_VAR_Y(code));
    return PyString_FromString(buf);
}

PyObject* raise_wreport_exception(const wreport::error& e)
{
    switch (e.code())
    {
        case WR_ERR_NONE:
            PyErr_SetString(PyExc_SystemError, e.what());
            return NULL;
        case WR_ERR_NOTFOUND:    // Item not found
            PyErr_SetString(PyExc_KeyError, e.what());
            return NULL;
        case WR_ERR_TYPE:        // Wrong variable type
            PyErr_SetString(PyExc_TypeError, e.what());
            return NULL;
        case WR_ERR_ALLOC:       // Cannot allocate memory
            PyErr_SetString(PyExc_MemoryError, e.what());
            return NULL;
        case WR_ERR_ODBC:        // ODBC error
            PyErr_SetString(PyExc_OSError, e.what());
            return NULL;
        case WR_ERR_HANDLES:     // Handle management error
            PyErr_SetString(PyExc_SystemError, e.what());
            return NULL;
        case WR_ERR_TOOLONG:     // Buffer is too short to fit data
            PyErr_SetString(PyExc_ValueError, e.what());
            return NULL;
        case WR_ERR_SYSTEM:      // Error reported by the system
            PyErr_SetString(PyExc_OSError, e.what());
            return NULL;
        case WR_ERR_CONSISTENCY: // Consistency check failed
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        case WR_ERR_PARSE:       // Parse error
            PyErr_SetString(PyExc_ValueError, e.what());
            return NULL;
        case WR_ERR_WRITE:       // Write error
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        case WR_ERR_REGEX:       // Regular expression error
            PyErr_SetString(PyExc_ValueError, e.what());
            return NULL;
        case WR_ERR_UNIMPLEMENTED: // Feature not implemented
            PyErr_SetString(PyExc_NotImplementedError, e.what());
            return NULL;
        case WR_ERR_DOMAIN:      // Value outside acceptable domain
            PyErr_SetString(PyExc_OverflowError, e.what());
            return NULL;
    }
}

PyObject* raise_std_exception(const std::exception& e)
{
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
}

}
}
