#include <dballe++/record.h>

using namespace std;

namespace dballe {

void Record::dumpToStderr()
{
	dba_record_print(m_rec, stderr);
}

}

// vim:set ts=4 sw=4:
