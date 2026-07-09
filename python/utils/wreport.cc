#include "wreport.h"

namespace dballe {
namespace python {

Wreport::Wreport()
{
    // We can't throw here, because we should support the object being
    // instantiated at module level
}

Wreport::~Wreport() {}

void Wreport::import()
{
    // Make sure we are idempotent on double import
    if (m_api)
        return;

    pyo_unique_ptr module(throw_ifnull(PyImport_ImportModule("wreport")));

    m_api = (wrpy_c_api*)PyCapsule_Import("_wreport._C_API", 0);
    if (!m_api)
        throw PythonException();

#ifdef WREPORT_API1_MIN_VERSION
    if (m_api->version_major != 1)
    {
        PyErr_Format(PyExc_RuntimeError,
                     "wreport C API version is %d.%d but only 1.x is supported",
                     m_api->version_major, m_api->version_minor);
        throw PythonException();
    }

    if (m_api->version_minor < WREPORT_API1_MIN_VERSION)
    {
        PyErr_Format(PyExc_RuntimeError,
                     "wreport C API version is %d.%d but only 1.x is "
                     "supported, with x > %d",
                     m_api->version_major, m_api->version_minor,
                     WREPORT_API1_MIN_VERSION);
        throw PythonException();
    }
#endif
}

void Wreport::require_imported() const
{
    if (!m_api)
    {
        PyErr_SetString(
            PyExc_RuntimeError,
            "attempted to use the wreport C API without importing it");
        throw PythonException();
    }
}

} // namespace python
} // namespace dballe
