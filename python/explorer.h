#ifndef DBALLE_PYTHON_EXPLORER_H
#define DBALLE_PYTHON_EXPLORER_H

#include "utils/core.h"
#include <dballe/db/explorer.h>
#include <memory>

extern "C" {

typedef struct
{
    PyObject_HEAD dballe::db::DBExplorer* explorer;
} dpy_DBExplorer;

extern PyTypeObject* dpy_DBExplorer_Type;

#define dpy_DBExplorer_Check(ob)                                               \
    (Py_TYPE(ob) == dpy_DBExplorer_Type ||                                     \
     PyType_IsSubtype(Py_TYPE(ob), dpy_DBExplorer_Type))

typedef struct
{
    PyObject_HEAD dballe::db::DBExplorer::Update update;
} dpy_DBExplorerUpdate;

extern PyTypeObject* dpy_DBExplorerUpdate_Type;

#define dpy_DBExplorerUpdate_Check(ob)                                         \
    (Py_TYPE(ob) == dpy_DBExplorerUpdate_Type ||                               \
     PyType_IsSubtype(Py_TYPE(ob), dpy_DBExplorerUpdate_Type))

typedef struct
{
    PyObject_HEAD dballe::db::Explorer* explorer;
} dpy_Explorer;

extern PyTypeObject* dpy_Explorer_Type;

#define dpy_Explorer_Check(ob)                                                 \
    (Py_TYPE(ob) == dpy_Explorer_Type ||                                       \
     PyType_IsSubtype(Py_TYPE(ob), dpy_Explorer_Type))

typedef struct
{
    PyObject_HEAD dballe::db::Explorer::Update update;
} dpy_ExplorerUpdate;

extern PyTypeObject* dpy_ExplorerUpdate_Type;

#define dpy_ExplorerUpdate_Check(ob)                                           \
    (Py_TYPE(ob) == dpy_ExplorerUpdate_Type ||                                 \
     PyType_IsSubtype(Py_TYPE(ob), dpy_ExplorerUpdate_Type))
}

namespace dballe {
namespace python {

/**
 * Create a new dpy_Explorer
 */
dpy_Explorer* explorer_create();

/**
 * Create a new dpy_Explorer
 */
dpy_Explorer* explorer_create(const std::string& pathname);

/**
 * Create a new dpy_Explorer, taking over memory management
 */
dpy_Explorer* explorer_create(std::unique_ptr<dballe::db::Explorer> explorer);

/**
 * Create a new dpy_Explorer
 */
dpy_DBExplorer* dbexplorer_create();

/**
 * Create a new dpy_Explorer
 */
dpy_DBExplorer* dbexplorer_create(const std::string& pathname);

/**
 * Create a new dpy_Explorer, taking over memory management
 */
dpy_DBExplorer*
dbexplorer_create(std::unique_ptr<dballe::db::DBExplorer> explorer);

void register_explorer(PyObject* m);

} // namespace python
} // namespace dballe

#endif
