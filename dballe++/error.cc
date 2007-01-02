#include <dballe++/error.h>
#include <sstream>

using namespace std;

namespace dballe {

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

}
