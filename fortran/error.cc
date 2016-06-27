/** @file
 * @ingroup fortran
 * Error inspection functions for Dballe.
 */

#include "handles.h"
#include "trace.h"
#include "common.h"
#include <wreport/error.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

#define MAX_CALLBACKS 50

using namespace wreport;

namespace {
typedef void (*fdba_error_callback)(int* data);

struct HErrcb : public dballe::fortran::HBase
{
	ErrorCode error;
	fdba_error_callback cb;
	int data;

	void start() { dballe::fortran::HBase::start(); }
	void stop() { dballe::fortran::HBase::stop(); }

	// Check if this callback should be triggered by this error code
	// If it should, invoke the callback
	void check_invoke(ErrorCode code)
	{
		if (error == 0 || error == code)
			cb(&data);
	}
};

struct dballe::fortran::Handler<HErrcb, MAX_CALLBACKS> herr;

static ErrorCode last_err_code = WR_ERR_NONE;
static char last_err_msg[1024];

}

namespace dballe {
namespace fortran {

static int usage_refcount = 0;
void error_init()
{
	if (usage_refcount > 0)
		return;
	herr.init("Error Handling", "MAX_CALLBACKS");
	++usage_refcount;
}

int error(wreport::error& e)
{
    IF_TRACING(log_error(e));

	last_err_code = e.code();
	strncpy(last_err_msg, e.what(), 1024);
	size_t todo = herr.in_use;
	for (int i = 0; todo && i < MAX_CALLBACKS; ++i)
		if (herr.records[i].used)
		{
			herr.records[i].check_invoke(last_err_code);
			--todo;
		}
	return 1;
}

int success()
{
	last_err_code = WR_ERR_NONE;
	last_err_msg[0] = 0;
	return 0;
}

}
}

using namespace dballe;

/**@name Error management routines
 *
 * @{
 */

extern "C" {

/**
 * Return the error code for the last function that was called.
 *
 * This is a list of known codes:
 *
 * @li 0: No error
 * @li 1: Item not found
 * @li 2: Wrong variable type
 * @li 3: Cannot allocate memory
 * @li 4: Database error
 * @li 5: Handle management error
 * @li 6: Buffer is too short to fit data
 * @li 7: Error reported by the system
 * @li 8: Consistency check failed
 * @li 9: Parse error
 * @li 10: Write error
 * @li 11: Regular expression error
 * @li 12: Feature not implemented
 * @li 13: Value outside acceptable domain
 *
 * @return
 *   The error code.
 */
int idba_error_code()
{
    return last_err_code;
}

/**
 * Return the error message for the last function that was called.
 *
 * The error message is just a description of the error code.  To see more
 * details of the specific condition that caused the error, use
 * idba_error_context() and idba_error_details()
 *
 * @param message
 *   The string holding the error message.  If the string is not long enough, it
 *   will be truncated.
 */
void idba_error_message(char* message, unsigned message_len)
{
    fortran::cstring_to_fortran(last_err_msg, message, message_len);
}

/**
 * Return a string describing the error context description for the last
 * function that was called.
 *
 * This string describes what the code that failed was trying to do.
 *
 * @param message
 *   The string holding the error context.  If the string is not long enough,
 *   it will be truncated.
 */
void idba_error_context(char* message, unsigned message_len)
{
    fortran::cstring_to_fortran("", message, message_len);
}

/**
 * Return a string with additional details about the error for the last
 * function that was called.
 *
 * This string contains additional details about the error in case the code was
 * able to get extra informations about it, for example by querying the error
 * functions of an underlying module.
 *
 * @param message
 *   The string holding the error details.  If the string is not long enough,
 *   it will be truncated.
 */
void idba_error_details(char* message, unsigned message_len)
{
    fortran::cstring_to_fortran("", message, message_len);
}

/**
 * Set a callback to be invoked when an error of a specific kind happens.
 *
 * @param code
 *   The error code of the error that triggers this callback.  If DBA_ERR_NONE
 *   is used, then the callback is invoked on all errors.
 * @param[in] func
 *   The function to be called.
 * @param data
 *   An arbitrary integer data that is passed verbatim to the callback function
 *   when invoked.
 * @retval handle
 *   A handle that can be used to remove the callback
 * @return
 *   The error indicator for the function
 */
int idba_error_set_callback(int code, fdba_error_callback func, int data, int* handle)
{
    // Initialise the error library in case it has not been done yet
    fortran::error_init();

    *handle = herr.request();
    HErrcb& h = herr.get(*handle);
    h.error = (ErrorCode)code;
    h.cb = func;
    h.data = data;
    return fortran::success();
}

/**
 * Remove a previously set callback.
 *
 * @param handle
 *   The handle previously returned by idba_error_set_callback
 * @return
 *   The error indicator for the function
 */
void idba_error_remove_callback(int* handle)
{
    herr.release(*handle);
}

/**
 * Predefined error callback that prints a message and exits.
 *
 * The message is printed only if a non-zero value is supplied as user data
 */
void idba_default_error_handler(int* debug)
{
    if (*debug)
        fprintf(stderr, "DB-All.e error %d: %s\n", last_err_code, last_err_msg);
    exit(1);
}

/**
 * Predefined error callback that prints a message and exists, except in case
 * of overflow errors.
 *
 * In case of overflows it prints a warning and continues execution
 */
void idba_error_handle_tolerating_overflows(int* debug)
{
    if (last_err_code != WR_ERR_NOTFOUND)
    {
        if (*debug)
            fprintf(stderr, "DB-All.e error %d: %s\n", last_err_code, last_err_msg);
        exit(1);
    }
}

/// @}

}
