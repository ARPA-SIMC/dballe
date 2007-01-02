/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/core/dba_error.h>
#include <dballe/cmdline.h>

#include <f77.h>

#include "handles.h"


/** @file
 * @ingroup fortranfull
 * Error inspection functions for Dballe.
 *
 * These funtions closely wrap the Dballe functions in dba_error.h
 */

#define MAX_CALLBACKS 50

FDBA_HANDLE_BODY(errcb, MAX_CALLBACKS, "Error handling callbacks")


/**
 * Return the error code for the last error that happened
 *
 * @see dba_error_code()
 *
 * @return
 *   The error code.  Please see the documentation of ::dba_err_code for the
 *   possible values.
 */
F77_INTEGER_FUNCTION(idba_error_code)()
{
	return dba_error_get_code();
}

/**
 * Return the error message for the last error that happened.
 *
 * The error message is just a description of the error code.  To see more
 * details of the specific condition that caused the error, use
 * fdba_error_context() and fdba_error_details()
 *
 * @see dba_error_message()
 *
 * @param message
 *   The string holding the error messag.  If the string is not long enough, it
 *   will be truncated.
 */
F77_SUBROUTINE(idba_error_message)(CHARACTER(message) TRAIL(message))
{
	GENPTR_CHARACTER(message)
	const char* msg = dba_error_get_message();

	cnfExprt(msg, message, message_length);
}

/**
 * Return a string describing the context in which the error happened.
 *
 * This string describes what the code that failed was trying to do.
 *
 * @see dba_error_context()
 *
 * @param message
 *   The string holding the error context.  If the string is not long enough,
 *   it will be truncated.
 */
F77_SUBROUTINE(idba_error_context)(CHARACTER(message) TRAIL(message))
{
	GENPTR_CHARACTER(message)
	const char* msg = dba_error_get_context();

	cnfExprt(msg, message, message_length);
}

/**
 * Return a string with additional details about the error.
 *
 * This string contains additional details about the error in case the code was
 * able to get extra informations about it, for example by querying the error
 * functions of an underlying module.
 *
 * @see dba_error_details()
 *
 * @param message
 *   The string holding the error details.  If the string is not long enough,
 *   it will be truncated.
 */
F77_SUBROUTINE(idba_error_details)(CHARACTER(message) TRAIL(message))
{
	GENPTR_CHARACTER(message)
	const char* msg = dba_error_get_details();

	if (msg != NULL)
		cnfExprt(msg, message, message_length);
	else
		cnfExprt("", message, message_length);
}

#define CBDATA (FDBA_HANDLE(errcb, *handle))

void fdba_error_callback_invoker(void* data)
{
	int ihandle = (int)data;
	int* handle = &ihandle;
	CBDATA.cb(INTEGER_ARG(&(CBDATA.data)));
}

/**
 * Set a callback to be invoked when an error of a specific kind happens.
 *
 * @param code
 *   The error code (@see ::dba_err_code) of the error that triggers this
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

	DBA_RUN_OR_RETURN(fdba_handle_alloc_errcb(handle));
	CBDATA.error = *code;
	CBDATA.cb = (fdba_error_callback)func;
	CBDATA.data = *data;

	dba_error_set_callback(*code, fdba_error_callback_invoker, (void*)*handle);
	return dba_error_ok();
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
	dba_error_remove_callback(CBDATA.error, fdba_error_callback_invoker, (void*)CBDATA.data);
	fdba_handle_release_errcb(*handle);
	return dba_error_ok();
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
		dba_cmdline_print_dba_error();
	exit(1);
}
F77_INTEGER_FUNCTION(idba_default_error_handle)(INTEGER(debug))
{
	GENPTR_INTEGER(debug)
	if (*debug)
		dba_cmdline_print_dba_error();
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
	int is_fatal = (dba_error_get_code() != DBA_ERR_NOTFOUND);
	if (*debug)
	{
		if (!is_fatal)
			fprintf(stderr, "Warning: ");
		dba_cmdline_print_dba_error();
	}
	if (is_fatal)
		exit(1);
	return 0;
}
