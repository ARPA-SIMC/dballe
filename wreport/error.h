/*
 * wreport/error - wreport exceptions
 *
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

#ifndef WREPORT_ERROR_H
#define WREPORT_ERROR_H

#include <stdexcept>
#include <string>

/** @file
@ingroup core
wreport exceptions

All wreport exceptions are derived from wreport::error, which is in turn
derived from std::exception.

All wreport exceptions also have an exception specific error code, which makes
it easy to turn a caught exception into an errno-style error code, when
providing C or Fortran bindings.
*/

namespace wreport {

enum ErrorCode {
	/** No error */
	WR_ERR_NONE			=  0,
	/** Item not found */
	WR_ERR_NOTFOUND		=  1,
	/** Wrong variable type */
	WR_ERR_TYPE			=  2,
	/** Cannot allocate memory */
	WR_ERR_ALLOC			=  3,
	/** ODBC error */
	WR_ERR_ODBC			=  4,
	/** Handle management error */
	WR_ERR_HANDLES			=  5,
	/** Buffer is too short to fit data */
	WR_ERR_TOOLONG			=  6,
	/** Error reported by the system */
	WR_ERR_SYSTEM			=  7,
	/** Consistency check failed */
	WR_ERR_CONSISTENCY		=  8,
	/** Parse error */
	WR_ERR_PARSE			=  9,
	/** Write error */
	WR_ERR_WRITE			= 10,
	/** Regular expression error */
	WR_ERR_REGEX			= 11,
	/** Feature not implemented */
	WR_ERR_UNIMPLEMENTED		= 12,
	/** Value outside acceptable domain */
	WR_ERR_DOMAIN			= 13
};


#define WREPORT_THROWF_ATTRS(a, b) __attribute__ ((noreturn, format(printf, a, b)))

/// Base class for DB-All.e exceptions
struct error : public std::exception
{
	virtual ErrorCode code() const throw () = 0;

	static const char* strerror(ErrorCode code);
};

/// Reports that a search-like function could not find what was requested.
struct error_notfound : public error
{
	std::string msg;

	error_notfound(const std::string& msg) : msg(msg) {}
	~error_notfound() throw () {}

	ErrorCode code() const throw () { return WR_ERR_NOTFOUND; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/**
 * For functions handling data with multiple types, reports a mismatch
 * between the type requested and the type found.
 */
struct error_type : public error
{
	std::string msg;

	error_type(const std::string& msg) : msg(msg) {}
	~error_type() throw () {}

	ErrorCode code() const throw () { return WR_ERR_TYPE; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/// Reports that memory allocation has failed.
struct error_alloc : public error
{
	const char* msg;

	error_alloc(const char* msg) : msg(msg) {}
	~error_alloc() throw () {}

	ErrorCode code() const throw () { return WR_ERR_ALLOC; }

	virtual const char* what() const throw () { return msg; }
};

/**
 * For functions working with handles, reports a problem with handling handles,
 * such as impossibility to allocate a new one, or an invalid handle being
 * passed to the function.
 */
struct error_handles : public error
{
	std::string msg;

	error_handles(const std::string& msg) : msg(msg) {}
	~error_handles() throw () {}

	ErrorCode code() const throw () { return WR_ERR_HANDLES; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/// Report an error with a buffer being to short for the data it needs to fit.
struct error_toolong : public error
{
	std::string msg;

	error_toolong(const std::string& msg) : msg(msg) {}
	~error_toolong() throw () {}

	ErrorCode code() const throw () { return WR_ERR_TOOLONG; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/**
 * Report a system error message.  The message description will be looked up
 * using the current value of errno.
 */
struct error_system : public error
{
	std::string msg;

	error_system(const std::string& msg);
	error_system(const std::string& msg, int errno_val);
	~error_system() throw () {}

	ErrorCode code() const throw () { return WR_ERR_SYSTEM; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/// Report an error when a consistency check failed.
struct error_consistency : public error
{
	std::string msg;

	error_consistency(const std::string& msg) : msg(msg) {};
	~error_consistency() throw () {}

	ErrorCode code() const throw () { return WR_ERR_CONSISTENCY; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/// Report an error when parsing informations.
struct error_parse : public error
{
	std::string msg;

	/**
	 * @param file
	 *   The file that is being parsed
	 * @param line
	 *   The line of the file where the problem has been found
	 */
	error_parse(const std::string& msg) : msg(msg) {}
	error_parse(const char* file, int line, const std::string& msg);
	~error_parse() throw () {}

	ErrorCode code() const throw () { return WR_ERR_PARSE; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* file, int line, const char* fmt, ...) WREPORT_THROWF_ATTRS(3, 4);
};

/// Report an error while handling regular expressions
struct error_regexp : public error
{
	std::string msg;

	/**
	 * @param code
	 *   The error code returned by the regular expression functions.
	 * @param re
	 *   The pointer to the regex_t structure that was being used when the error
	 *   occurred.
	 */
	error_regexp(int code, void* re, const std::string& msg);
	~error_regexp() throw () {}

	ErrorCode code() const throw () { return WR_ERR_REGEX; }

	virtual const char* what() const throw () { return msg.c_str(); }
};

/// Reports that a feature is still not implemented.
struct error_unimplemented : public error
{
	std::string msg;

	error_unimplemented(const std::string& msg) : msg(msg) {};
	~error_unimplemented() throw () {}

	ErrorCode code() const throw () { return WR_ERR_UNIMPLEMENTED; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

/// Report an error with a buffer being to short for the data it needs to fit.
struct error_domain : public error
{
	std::string msg;

	error_domain(const std::string& msg) : msg(msg) {}
	~error_domain() throw () {}

	ErrorCode code() const throw () { return WR_ERR_DOMAIN; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};

}

/* vim:set ts=4 sw=4: */
#endif
