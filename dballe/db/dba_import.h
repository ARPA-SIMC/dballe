#ifndef DBA_IMPORT_H
#define DBA_IMPORT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/db/dballe.h>
#include <dballe/msg/dba_msg.h>

/**
 * Import a dba_msg message into the Dballe database
 *
 * @param db
 *   The DBALLE database to write the data into
 * @param msg
 *   The dba_msg containing the data to import
 * @return
 *   The error indicator for the function
 */
dba_err dba_import_msg(dba db, dba_msg msg);

#ifdef  __cplusplus
}
#endif

#endif
