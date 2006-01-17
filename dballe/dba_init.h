#ifndef DBA_INIT
#define DBA_INIT

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * Library initialisation and shutdown
 */

#include <dballe/err/dba_error.h>
	
/**
 * Initialize the library.
 *
 * This function needs to be called just once at the beginning of the work.
 */
dba_err dba_init();

/**
 * Shutdown the library.
 *
 * This function needs to be called just once at the end of the work.
 */
void dba_shutdown();


#ifdef  __cplusplus
}
#endif

#endif
