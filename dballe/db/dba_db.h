#ifndef DBA_DB_H
#define DBA_DB_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/dba_record.h>
#include <dballe/core/dba_var.h>

/** @file
 * @ingroup db
 *
 * Functions used to connect to Dballe and insert, query and delete data.
 */

/**
 * DB-ALLe connection
 */
typedef struct _dba_db* dba_db;

/**
 * Handle identifying a dballe cursor
 */
typedef struct _dba_db_cursor* dba_db_cursor;


/**
 * Initialize the dballe subsystem.
 *
 * This function needs to be called just once at the beginning of the work.
 */
dba_err dba_db_init();

/**
 * Shutdown the dballe subsystem.
 *
 * This function needs to be called just once at the end of the work.
 */
void dba_db_shutdown();

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
 *   The error indicator for the function
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
 *   The error indicator for the function
 */
dba_err dba_db_reset(dba_db db, const char* repinfo_file);

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
 *   The error indicator for the function
 */
dba_err dba_db_check_rep_cod(dba_db db, int rep_cod, int* valid);

/**
 * Start a query on the anagraphic archive
 *
 * @param db
 *   The dballe session id
 * @param cur
 *   The dba_db_cursor variable that will hold the resulting dba_db_cursor that can
 *   be used to get the result values (@see dba_ana_cursor_next)
 * @param count
 *   The count of items in the anagraphic archive, returned by the function
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_ana_query(dba_db db, dba_db_cursor* cur, int* count);

/**
 * Get a new item from the results of an anagraphic query
 *
 * @param cur
 *   The cursor returned by dba_ana_query
 * @param rec
 *   The record where to store the values
 * @param is_last
 *   Variable that will be set to true if the element returned is the last one
 *   in the sequence, else to false.
 * @return
 *   The error indicator for the function.  The error code DBA_ERR_NOTFOUND is
 *   used when there are no more results to get.
 *
 * @note
 *   Do not forget to call dba_db_cursor_delete after you have finished retrieving
 *   the query data.
 */
dba_err dba_db_ana_cursor_next(dba_db_cursor cur, dba_record rec, int* is_last);

/**
 * Insert a record into the database.
 *
 * In a record with the same phisical situation already exists, it is
 * overwritten
 *
 * @param db
 *   The dballe session id
 * @param rec
 *   The record to insert
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_insert(dba_db db, dba_record rec);

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
 * @param update_pseudoana
 *   If true, then the pseudoana informations are overwritten using information
 *   from rec; else data from rec is written into pseudoana only if there is no
 *   suitable existing anagraphical data for it.
 * @retval ana_id
 *   ID of the pseudoana record for the entry just inserted.  NULL can be used
 *   if the caller is not interested in this value.
 * @retval context_id
 *   ID of the context record for the entry just inserted.  NULL can be used
 *   if the caller is not interested in this value.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_db_insert_or_replace(dba_db db, dba_record rec, int can_replace, int update_pseudoana, int* ana_id, int* context_id);

/**
 * Insert a record into the database
 *
 * In a record with the same phisical situation already exists, the function
 * fails.
 *
 * @param db
 *   The dballe session id
 * @param rec
 *   The record to insert
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_insert_new(dba_db db, dba_record rec);

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
 *   "parameter output/input"
 * @retval cur
 *   The dba_db_cursor variable that will hold the resulting dba_db_cursor that can
 *   be used to get the result values (@see dba_db_cursor_next)
 * @retval count
 *   The number of values returned by the query
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_query(dba_db db, dba_record rec, dba_db_cursor* cur, int* count);

/**
 * Get a new item from the results of a query
 *
 * @param cur
 *   The cursor returned by dba_query
 * @param rec
 *   The record where to store the values
 * @retval var
 *   The variable read by this fetch
 * @retval is_last
 *   Variable that will be set to true if the element returned is the last one
 *   in the sequence, else to false.
 * @return
 *   The error indicator for the function.  The error code DBA_ERR_NOTFOUND is
 *   used when there are no more results to get.
 *
 * @note
 *   Do not forget to call dba_db_cursor_delete after you have finished retrieving
 *   the query data.
 */
dba_err dba_db_cursor_next(dba_db_cursor cur, dba_record rec, dba_varcode* var, int* cursor_id, int* is_last);

/**
 * Release a dba_db_cursor
 *
 * @param cur
 *   The cursor to delete
 */
void dba_db_cursor_delete(dba_db_cursor cur);

/**
 * Remove data from the database
 *
 * @param db
 *   The dballe session id
 * @param rec
 *   The record with the query data (see technical specifications, par. 1.6.4
 *   "parameter output/input") to select the items to be deleted
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_remove(dba_db db, dba_record rec);

/**
 * Query QC data
 *
 * @param db
 *   The dballe session id
 * @param id_data
 *   The database ID of the variable to which the QC data to query are referred
 * @param qcs
 *   The WMO codes of the QC values requested.  If it is NULL, then all values
 *   are returned.
 * @param qcs_size
 *   Number of elements in qcs
 * @param qc
 *   The dba_record that will hold the resulting QC data
 * @retval count
 *   Number of QC items returned in qc
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_qc_query(dba_db db, int id_context, dba_varcode id_var, dba_varcode* qcs, int qcs_size, dba_record qc, int* count);

/**
 * Insert a new QC value into the database.
 *
 * @param db
 *   The dballe session id
 * @param id_data
 *   The database id of the data item related to the QC values to insert
 * @param qc
 *   The QC data to be added
 * @param can_replace
 *   If true, then existing data can be rewritten, else data can only be added.
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_qc_insert_or_replace(dba_db db, int id_context, dba_varcode id_var, dba_record qc, int can_replace);

/**
 * Insert a new QC value into the database.
 *
 * If the same QC value exists for the same data, it is
 * overwritten
 *
 * @param db
 *   The dballe session id
 * @param id_data
 *   The database id of the data item related to the QC values to insert
 * @param qc
 *   The QC data to be added
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_qc_insert(dba_db db, int id_context, dba_varcode id_var, dba_record qc);

/**
 * Insert a new QC value into the database.
 *
 * If the same QC value exists for the same data, the function fails.
 *
 * @param db
 *   The dballe session id
 * @param id_data
 *   The database id of the data item related to the QC values to insert
 * @param qc
 *   The QC data to be added
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_qc_insert_new(dba_db db, int id_context, dba_varcode id_var, dba_record qc);

/**
 * Delete QC data for the variable `var' in record `rec' (coming from a previous
 * dba_query)
 *
 * @param db
 *   The dballe session id
 * @param id_data
 *   The database id of the data item related to the QC values to insert
 * @param qcs
 *   Array of WMO codes of the QC data to delete.  If NULL, all QC data
 *   associated to id_data will be deleted.
 * @param qcs_size
 *   Number of items in the qcs array.
 * @return
 *   The error indicator for the function
 */
dba_err dba_db_qc_remove(dba_db db, int id_context, dba_varcode id_var, dba_varcode* qcs, int qcs_size);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
