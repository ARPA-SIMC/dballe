/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBA_EXPORT_H
#define DBA_EXPORT_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 * Export functions from the DB-ALLe database to ::dba_msg
 */

#include <dballe/db/db.h>
#include <dballe/msg/msgs.h>
#include <dballe/core/record.h>

/**
 * Callback function used to collect the messages produced by export functions
 */
typedef dba_err (*dba_msg_consumer)(dba_msgs msgs, void* data);

/**
 * Perform the query in `query', and return the results as a NULL-terminated
 * array of dba_msg.
 *
 * @param db
 *   The database to use for the query
 * @param query
 *   The query to perform
 * @param cons
 *   The ::dba_msg_consumer function that will handle the resulting messages
 * @param data
 *   Arbitrary extra value to be passed as-is to the ::dba_msg_consumer
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_export(dba_db db, dba_record query, dba_msg_consumer cons, void* data);

#if 0
dba_err dba_db_export_old(dba_db db, dba_msg_type export_type, dba_record query, dba_msg_consumer cons, void* data);
#endif

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
