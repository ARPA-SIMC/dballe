#include <dballe/dba_init.h>
#include <dballe/core/verbose.h>
#include <dballe/db/dballe.h>

dba_err dba_init()
{
	dba_verbose_init();
	DBA_RUN_OR_RETURN(dba_db_init());

	return dba_error_ok();
}

void dba_shutdown()
{
	dba_db_shutdown();
}
