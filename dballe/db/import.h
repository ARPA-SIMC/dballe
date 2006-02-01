#ifndef DBA_IMPORT_H
#define DBA_IMPORT_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 * Import functions from a ::dba_msg to the DB-ALLe database
 */

#include <dballe/db/dba_db.h>
#include <dballe/msg/dba_msg.h>

/**
 * Import a dba_msg message into the Dballe database
 *
 * @param db
 *   The DBALLE database to write the data into
 * @param msg
 *   The dba_msg containing the data to import
 * @param overwrite
 *   If true, message data will overwrite existing values; if false, trying to
 *   insert existing data will cause an error.
 * @return
 *   The error indicator for the function
 */
dba_err dba_import_msg(dba_db db, dba_msg msg, int repcod, int overwrite);

#ifdef  __cplusplus
}
#endif

#endif
