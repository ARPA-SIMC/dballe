#ifndef DBALLE_PYTHON_COMMON_H
#define DBALLE_PYTHON_COMMON_H

#include "utils/core.h"
#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/python.h>
#include <wreport/error.h>
#include <wreport/varinfo.h>

namespace dballe {
namespace python {

struct Wreport;

/// Wreport python API
extern Wreport wreport_api;

/// Given a wreport exception, set the Python error indicator appropriately.
void set_wreport_exception(const wreport::error& e);

/// Given a generic exception, set the Python error indicator appropriately.
void set_std_exception(const std::exception& e);

/**
 * Check result of a function that returns a FILE*, raising a python exception
 * if it is null.
 *
 * Returns f unchanged.
 */
FILE* check_file_result(FILE* f, const char* filename=nullptr);

#define DBALLE_CATCH_RETURN_PYO \
      catch (PythonException&) { \
        return nullptr; \
    } catch (wreport::error& e) { \
        set_wreport_exception(e); return nullptr; \
    } catch (std::exception& se) { \
        set_std_exception(se); return nullptr; \
    }

#define DBALLE_CATCH_RETURN_INT \
      catch (PythonException&) { \
        return -1; \
    } catch (wreport::error& e) { \
        set_wreport_exception(e); return -1; \
    } catch (std::exception& se) { \
        set_std_exception(se); return -1; \
    }

#define DBALLE_CATCH_RETHROW_PYTHON \
      catch (PythonException&) { \
        throw; \
    } catch (wreport::error& e) { \
        set_wreport_exception(e); throw PythonException(); \
    } catch (std::exception& se) { \
        set_std_exception(se); throw PythonException(); \
    }

/// If val is MISSING_INT, returns None, else return it as a PyLong
PyObject* dballe_int_to_python(int val);

/// Convert a Python object to an integer, returning MISSING_INT if it is None
int dballe_int_from_python(PyObject* o);

/// Call repr() on \a o, and return the result as a string
std::string object_repr(PyObject* o);

/**
 * Initialize the python bits to use used by the common functions.
 *
 * This can be called multiple times and will execute only once.
 */
void common_init();

}
}
#endif
