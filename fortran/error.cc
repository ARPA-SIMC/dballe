/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/** @file
 * @ingroup fortranfull
 * Error inspection functions for Dballe.
 *
 * These funtions closely wrap the Dballe functions in dba_error.h
 */

#include "handles.h"
#include <wreport/error.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>

extern "C" {
#include <f77.h>
}

#define MAX_CALLBACKS 50

using namespace wreport;

namespace {
typedef void (*fdba_error_callback)(INTEGER(data));

struct HErrcb : public dballe::fortran::HBase
{
	ErrorCode error;
	fdba_error_callback cb;
	int data;

	void start() {}
	void stop() {}

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

void error_init()
{
	herr.init("Error Handling", "MAX_CALLBACKS");
}

int error(wreport::error& e)
{
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

extern "C" {

// FDBA_HANDLE_BODY(errcb, MAX_CALLBACKS, "Error handling callbacks")


/**
 * Return the error code for the last error that happened
 *
 * See @ref dba_error_code()
 *
 * @return
 *   The error code.  Please see the documentation of ::dba_err_code for the
 *   possible values.
 */
F77_INTEGER_FUNCTION(idba_error_code)()
{
	return last_err_code;
}

/**
 * Return the error message for the last error that happened.
 *
 * The error message is just a description of the error code.  To see more
 * details of the specific condition that caused the error, use
 * fdba_error_context() and fdba_error_details()
 *
 * See @ref dba_error_message()
 *
 * @param message
 *   The string holding the error messag.  If the string is not long enough, it
 *   will be truncated.
 */
F77_SUBROUTINE(idba_error_message)(CHARACTER(message) TRAIL(message))
{
	GENPTR_CHARACTER(message)
	cnfExprt(last_err_msg, message, message_length);
}

/**
 * Return a string describing the context in which the error happened.
 *
 * This string describes what the code that failed was trying to do.
 *
 * See @ref dba_error_context()
 *
 * @param message
 *   The string holding the error context.  If the string is not long enough,
 *   it will be truncated.
 */
F77_SUBROUTINE(idba_error_context)(CHARACTER(message) TRAIL(message))
{
	GENPTR_CHARACTER(message)
	cnfExprt("", message, message_length);
}

/**
 * Return a string with additional details about the error.
 *
 * This string contains additional details about the error in case the code was
 * able to get extra informations about it, for example by querying the error
 * functions of an underlying module.
 *
 * See @ref dba_error_details()
 *
 * @param message
 *   The string holding the error details.  If the string is not long enough,
 *   it will be truncated.
 */
F77_SUBROUTINE(idba_error_details)(CHARACTER(message) TRAIL(message))
{
	GENPTR_CHARACTER(message)
	cnfExprt("", message, message_length);
}

/**
 * Set a callback to be invoked when an error of a specific kind happens.
 *
 * @param code
 *   The error code (See @ref ::dba_err_code) of the error that triggers this
 *   callback.  If DBA_ERR_NONE is used, then the callback is invoked on all
 *   errors.
 * @param func
 *   The function to be called.
 * @param data
 *   An arbitrary integer data that is passed verbatim to the callback function
 *   when invoked.
 * @retval handle
 *   A handle that can be used to remove the callback
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_error_set_callback)(
		INTEGER(code),
		SUBROUTINE(func),
		INTEGER(data),
		INTEGER(handle))
{
	GENPTR_INTEGER(code)
	GENPTR_SUBROUTINE(func)
	GENPTR_INTEGER(data)
	GENPTR_INTEGER(handle)

	*handle = herr.request();
	HErrcb& h = herr.get(*handle);
	h.error = (ErrorCode)*code;
	h.cb = (fdba_error_callback)func;
	h.data = *data;
	return fortran::success();
}

/**
 * Remove a callback set previously.
 *
 * @param handle
 *   The handle previously returned by idba_error_set_callback
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_error_remove_callback)(INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	
	herr.release(*handle);
	return fortran::success();
}

/**
 * Default callback that prints a message and exits.
 *
 * The message is printed only if a non-zero value is supplied as user data
 */
F77_INTEGER_FUNCTION(idba_default_error_handler)(INTEGER(debug))
{
	GENPTR_INTEGER(debug)
	if (*debug)
		fprintf(stderr, "DB-All.e error %d: %s\n", last_err_code, last_err_msg);
	exit(1);
}
F77_INTEGER_FUNCTION(idba_default_error_handle)(INTEGER(debug))
{
	GENPTR_INTEGER(debug)
	if (*debug)
		fprintf(stderr, "DB-All.e error %d: %s\n", last_err_code, last_err_msg);
	exit(1);
}
/**
 * Default callback that prints a message and exists, except in case of overflows.
 *
 * In case of overflows it prints a warning and continues execution
 */
F77_INTEGER_FUNCTION(idba_error_handle_tolerating_overflows)(INTEGER(debug))
{
	GENPTR_INTEGER(debug)
	if (last_err_code != WR_ERR_NOTFOUND);
	{
		if (*debug)
			fprintf(stderr, "DB-All.e error %d: %s\n", last_err_code, last_err_msg);
		exit(1);
	}
	return 0;
}

}
