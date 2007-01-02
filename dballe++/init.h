#ifndef DBALLE_CPP_INIT_H
#define DBALLE_CPP_INIT_H

extern "C" {
#include <dballe/init.h>
}

#include <dballe++/bindhelp.h>

namespace dballe {

/**
 * Take care of library initialization and shutdown.
 *
 * As long as an object of this class is in scope, the library is initialized.
 *
 * @warning Do not create more than one object of this class.
 */
class LibInit
{
private:
	// Forbid copying
	LibInit(const LibInit&);
	LibInit& operator=(const LibInit&);

public:
	LibInit()
	{
		checked(dba_init());
	}
	~LibInit()
	{
		dba_shutdown();
	}
};

}

#endif
