#ifndef DBALLE_CPP_ERROR_H
#define DBALLE_CPP_ERROR_H

#include <dballe/core/error.h>

#include <string>
#include <exception>

namespace dballe {
namespace exception {

/**
 * Wrap the DB-All.e error module using exceptions
 */
class Exception : public std::exception
{
	/// Error code
	dba_err_code code;
	/// Error message
	std::string message;
	/// Error context
	std::string context;
	/// Error details
	std::string details;
	/// Full formatted error information
	std::string fullInfo;

public:
	Exception();
	virtual ~Exception() throw () {}

	/// Return a full description of what happened
	virtual const char* what() const throw ()
	{
		return fullInfo.c_str();
	}
};

/**
 * Exceptions corresponding to the various DB-All.e error codes
 * @{
 */
struct NotFound : public Exception {};
struct Type : public Exception {};
struct Alloc : public Exception {};
struct ODBC : public Exception {};
struct Handles : public Exception {};
struct TooLong : public Exception {};
struct System : public Exception {};
struct Consistency : public Exception {};
struct Parse : public Exception {};
struct Write : public Exception {};
struct Regex : public Exception {};
struct Unimplemented : public Exception {};
/** @} */

void throwAppropriateException();

}

static inline void checked(dba_err err)
{
	if (err != DBA_OK)
		dballe::exception::throwAppropriateException();
}

}

#endif
