#ifndef CPLUSPLUS_DBALLE_ERROR_H
#define CPLUSPLUS_DBALLE_ERROR_H

#include <dballe/err/dba_error.h>

#include <string>

class DBAException
{
public:
	const char* what() { return dba_error_get_message(); }

	std::string message() { return dba_error_get_message(); }
	std::string context() { return dba_error_get_context() ?: ""; }
	std::string details() { return dba_error_get_details() ?: ""; }

	std::string fulldesc() {
		std::string res = message() + " while " + context();
		const char* details = dba_error_get_details();
		if (details != NULL)
			res += std::string(".  Details:\n%s") + details;
		return res;
	}
};

#define DBA_RUN_OR_THROW(...) do { \
		if ((__VA_ARGS__) != DBA_OK) \
			throw DBAException(); \
	} while (0)

#endif
