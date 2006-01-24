#ifndef DBA_EXPORT_H
#define DBA_EXPORT_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 * Export functions from the DB-ALLe database to ::dba_msg
 */

#include <dballe/db/dballe.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/core/dba_record.h>

/**
 * Perform the query in `query', and return the results as a NULL-terminated
 * array of dba_msg.
 *
 * @param db
 *   The database to use for the query
 * @param type
 *   The type of message to export
 * @retval msgs
 *   The returned array of results
 * @param query
 *   The query to perform
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_export(dba db, dba_msg_type type, dba_msg** msgs, dba_record query);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
