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
 * @param fast
 *   If true, perform the import outside of the transaction.  This will make
 *   the function faster but not atomic: if interrupted, for example in case of
 *   error, then the data inserted before the interruption will stay in the
 *   database.
 * @return
 *   The error indicator for the function
 */
dba_err dba_import_msg(dba_db db, dba_msg msg, int repcod, int overwrite, int fast);

#ifdef  __cplusplus
}
#endif

#endif
