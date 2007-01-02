#ifndef DBALLE_CPP_BINDHELP_H
#define DBALLE_CPP_BINDHELP_H

#include <dballe++/error.h>

namespace dballe {

static inline void checked(dba_err err)
{
	if (err != DBA_OK)
		throw dballe::Exception();
}

}

#endif
