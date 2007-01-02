#ifndef DBALLE_CPP_ERROR_H
#define DBALLE_CPP_ERROR_H

#include <dballe/core/error.h>

#include <string>
#include <exception>

namespace dballe {

class Exception : public std::exception
{
	dba_err_code code;
	std::string message;
	std::string context;
	std::string details;
	std::string fullInfo;

public:
	Exception();
	virtual ~Exception() throw () {}

	virtual const char* what() const throw ()
	{
		return fullInfo.c_str();
	}
};

}

#endif
