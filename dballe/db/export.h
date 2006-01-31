#ifndef DBA_EXPORT_H
#define DBA_EXPORT_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 * Export functions from the DB-ALLe database to ::dba_msg
 */

#include <dballe/db/dba_db.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/core/dba_record.h>

/**
 * Callback function used to collect the messages produced by export functions
 */
typedef dba_err (*dba_msg_consumer)(dba_msg msg, void* data);

/**
 * Perform the query in `query', and return the results as a NULL-terminated
 * array of dba_msg.
 *
 * @param db
 *   The database to use for the query
 * @param type
 *   The type of message to export
 * @param query
 *   The query to perform
 * @param cons
 *   The ::dba_msg_consumer function that will handle the resulting messages
 * @param data
 *   Arbitrary extra value to be passed as-is to the ::dba_msg_consumer
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_export(dba_db db, dba_msg_type type, dba_record query, dba_msg_consumer cons, void* data);

#if 0
dba_err dba_db_export_old(dba_db db, dba_msg_type export_type, dba_record query, dba_msg_consumer cons, void* data);
#endif

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
