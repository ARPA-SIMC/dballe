#include <dballe++/error.h>
#include <sstream>

using namespace std;

namespace dballe {
namespace exception {

Exception::Exception()
{
	stringstream str;

	code = dba_error_get_code();
	message = dba_error_get_message();
	context = dba_error_get_context();
	str << "Error " << code << " (" << message << ") while " << context << ".";

	if (const char* v = dba_error_get_details())
	{
		details = v;
		str << endl << "Details: " << endl << details << endl;
	}

	fullInfo = str.str();
}

void throwAppropriateException()
{
	switch (dba_error_get_code())
	{
		case DBA_ERR_NONE: break;
		case DBA_ERR_NOTFOUND: throw NotFound();
		case DBA_ERR_TYPE: throw Type();
		case DBA_ERR_ALLOC: throw Alloc();
		case DBA_ERR_ODBC: throw ODBC();
		case DBA_ERR_HANDLES: throw Handles();
		case DBA_ERR_TOOLONG: throw TooLong();
		case DBA_ERR_SYSTEM: throw System();
		case DBA_ERR_CONSISTENCY: throw Consistency();
		case DBA_ERR_PARSE: throw Parse();
		case DBA_ERR_WRITE: throw Write();
		case DBA_ERR_REGEX: throw Regex();
		case DBA_ERR_UNIMPLEMENTED: throw Unimplemented();
	}
}

}
}
