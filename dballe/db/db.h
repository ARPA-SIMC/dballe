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

#ifndef DBA_DB_H
#define DBA_DB_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/record.h>
#include <dballe/core/var.h>
#include <dballe/db/cursor.h>

/** @file
 * @ingroup db
 *
 * Functions used to connect to Dballe and insert, query and delete data.
 */

/**
 * DB-ALLe connection
 */
#ifndef DBA_DB_DEFINED
#define DBA_DB_DEFINED
typedef struct _dba_db* dba_db;
#endif

/**
 * Start a session with DBALLE
 *
 * @param dsn
 *   The ODBC DSN of the database to use
 * @param user
 *   The user name to use to connect to the DSN
 * @param password
 *   The password to use to connect to the DSN.  To specify an empty password,
 *   pass "" or NULL
 * @param db
 *   The dba_db handle returned by the function
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_create(const char* dsn, const char* user, const char* password, dba_db* db);

/**
 * End a session with DBALLE.
 *
 * All the resources associated with db will be freed.  db should not be used
 * anymore, unless it is recreated with dba_open
 *
 * @param db
 *   The dballe session id
 */
void dba_db_delete(dba_db db);

/**
 * Reset the database, removing all existing DBALLE tables and re-creating them
 * empty.
 *
 * @param db
 *   The dballe session id
 * @param repinfo_file
 *   The name of the CSV file with the report type information data to load.
 *   The file is in CSV format with 6 columns: report code, mnemonic id,
 *   description, priority, descriptor, table A category.
 *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
 *   used.
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_reset(dba_db db, const char* repinfo_file);

/**
 * Delete all the DB-ALLe tables from the database.
 *
 * @param db
 *   The dballe session id
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_delete_tables(dba_db db);

/**
 * Update the repinfo table in the database, with the data found in the given
 * file.
 *
 * @param db
 *   The dballe session id
 * @param repinfo_file
 *   The name of the CSV file with the report type information data to load.
 *   The file is in CSV format with 6 columns: report code, mnemonic id,
 *   description, priority, descriptor, table A category.
 *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
 *   used.
 * @retval added
 *   The number of repinfo entryes that have been added
 * @retval deleted
 *   The number of repinfo entryes that have been deleted
 * @retval updated
 *   The number of repinfo entryes that have been updated
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_update_repinfo(dba_db db, const char* repinfo_file, int* added, int* deleted, int* updated);

/**
 * Get the report code from a report mnemonic
 */
dba_err dba_db_rep_cod_from_memo(dba_db db, const char* memo, int* rep_cod);

/**
 * Verify that a rep_cod is supported by the database
 *
 * @param db
 *   The dballe database
 * @param rep_cod
 *   The report code to verify
 * @retval valid
 *   Set to 1 if the report code is supported, 0 if not
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_check_rep_cod(dba_db db, int rep_cod, int* valid);

/**
 * Start a query on the anagraphic archive
 *
 * @param db
 *   The dballe session id
 * @param query
 *   The record with the query data (see technical specifications, par. 1.6.4
 *   "parameter output/input")
 * @retval cur
 *   The dba_db_cursor variable that will hold the resulting dba_db_cursor that can
 *   be used to get the result values (See @ref dba_ana_cursor_next).
 *   dba_db_ana_query will create the cursor, and it is up to the caller to
 *   delete it using dba_db_cursor_delete.
 * @param count
 *   The count of items in the anagraphic archive, returned by the function
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_ana_query(dba_db db, dba_record query, dba_db_cursor* cur, int* count);

/**
 * Insert a record into the database
 *
 * In a record with the same phisical situation already exists, the function
 * fails.
 *
 * @param db
 *   The dballe session id.
 * @param rec
 *   The record to insert.
 * @param can_replace
 *   If true, then existing data can be rewritten, else data can only be added.
 * @param pseudoana_can_add
 *   If true, then it is allowed to add new pseudoana records to the database.
 *   Otherwise, data can be added only by reusing existing ones.
 * @retval ana_id
 *   ID of the pseudoana record for the entry just inserted.  NULL can be used
 *   if the caller is not interested in this value.
 * @retval context_id
 *   ID of the context record for the entry just inserted.  NULL can be used
 *   if the caller is not interested in this value.
 * @return
 *   The error indicator for the function (See @ref dba_err).
 */
dba_err dba_db_insert(dba_db db, dba_record rec, int can_replace, int pseudoana_can_add, int* ana_id, int* context_id);

/**
 * Query the database.
 *
 * When multiple values per variable are present, the results will be presented
 * in increasing order of priority.
 *
 * @param db
 *   The dballe session id
 * @param rec
 *   The record with the query data (see technical specifications, par. 1.6.4
 *   "parameter output/input")
 * @retval cur
 *   The dba_db_cursor variable that will hold the resulting dba_db_cursor that can
 *   be used to get the result values (See @ref dba_db_cursor_next)
 *   dba_db_query will create the cursor, and it is up to the caller to delete
 *   it using dba_db_cursor_delete.
 * @retval count
 *   The number of values returned by the query
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_query(dba_db db, dba_record rec, dba_db_cursor* cur, int* count);

/**
 * Remove data from the database
 *
 * @param db
 *   The dballe session id
 * @param rec
 *   The record with the query data (see technical specifications, par. 1.6.4
 *   "parameter output/input") to select the items to be deleted
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_remove(dba_db db, dba_record rec);

/**
 * Remove orphan values from the database.
 *
 * Orphan values are currently:
 * \li context values for which no data exists
 * \li pseudoana values for which no context exists
 *
 * Depending on database size, this routine can take a few minutes to execute.
 *
 * @param db
 *   Database to operate on
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_remove_orphans(dba_db db);

/**
 * Query QC data
 *
 * @param db
 *   The dballe session id
 * @param id_context
 *   The database id of the context related to the attributes to retrieve
 * @param id_var
 *   The varcode of the variable related to the attributes to retrieve
 * @param qcs
 *   The WMO codes of the QC values requested.  If it is NULL, then all values
 *   are returned.
 * @param qcs_size
 *   Number of elements in qcs
 * @param attrs
 *   The dba_record that will hold the resulting attributes
 * @retval count
 *   Number of QC items returned in qc
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_qc_query(dba_db db, int id_context, dba_varcode id_var, dba_varcode* qcs, int qcs_size, dba_record attrs, int* count);

/**
 * Insert a new QC value into the database.
 *
 * @param db
 *   The dballe session id
 * @param id_context
 *   The database id of the context related to the attributes to insert
 * @param id_var
 *   The varcode of the variable related to the attributes to add
 * @param attrs
 *   The record with the attributes to be added
 * @param can_replace
 *   If true, then existing data can be rewritten, else data can only be added.
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_qc_insert_or_replace(dba_db db, int id_context, dba_varcode id_var, dba_record attrs, int can_replace);

/**
 * Insert a new QC value into the database.
 *
 * If the same QC value exists for the same data, it is
 * overwritten
 *
 * @param db
 *   The dballe session id
 * @param id_context
 *   The database id of the context related to the attributes to insert
 * @param id_var
 *   The varcode of the variable related to the attributes to add
 * @param attrs
 *   The record with the attributes to be added
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_qc_insert(dba_db db, int id_context, dba_varcode id_var, dba_record attrs);

/**
 * Insert a new QC value into the database.
 *
 * If the same QC value exists for the same data, the function fails.
 *
 * @param db
 *   The dballe session id
 * @param id_context
 *   The database id of the context related to the attributes to insert
 * @param id_var
 *   The varcode of the variable related to the attributes to add
 * @param attrs
 *   The record with the attributes to be added
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_qc_insert_new(dba_db db, int id_context, dba_varcode id_var, dba_record attrs);

/**
 * Delete QC data for the variable `var' in record `rec' (coming from a previous
 * dba_query)
 *
 * @param db
 *   The dballe session id
 * @param id_context
 *   The database id of the context related to the attributes to remove
 * @param id_var
 *   The varcode of the variable related to the attributes to remove
 * @param qcs
 *   Array of WMO codes of the QC data to delete.  If NULL, all QC data
 *   associated to id_data will be deleted.
 * @param qcs_size
 *   Number of items in the qcs array.
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err dba_db_qc_remove(dba_db db, int id_context, dba_varcode id_var, dba_varcode* qcs, int qcs_size);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
