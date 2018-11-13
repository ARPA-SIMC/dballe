#ifndef DBALLE_PYTHON_RECORD_H
#define DBALLE_PYTHON_RECORD_H

#include <Python.h>
#include <dballe/fwd.h>
#include <dballe/core/fwd.h>

namespace wreport {
struct Var;
}

namespace dballe {
namespace python {

void read_query(PyObject* from_python, dballe::core::Query& query);
void read_data(PyObject* from_python, dballe::core::Data& data);
void read_values(PyObject* from_python, dballe::core::Values& values);
void set_var(PyObject* dict, const wreport::Var& var);

}
}
#endif
