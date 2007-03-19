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

#ifndef DBALLE_DB_CONTEXT_H
#define DBALLE_DB_CONTEXT_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 *
 * Context table management used by the db module.
 */

#include <dballe/db/internals.h>

struct _dba_db;
	
/**
 * Precompiled query to manipulate the context table
 */
struct _dba_db_context
{
	/** dba_db this dba_db_context is part of */
	struct _dba_db* db;
	/** Precompiled select statement */
	SQLHSTMT sstm;
	/** Precompiled select data statement */
	SQLHSTMT sdstm;
	/** Precompiled insert statement */
	SQLHSTMT istm;
	/** Precompiled delete statement */
	SQLHSTMT dstm;

	/** Context ID SQL parameter */
	DBALLE_SQL_C_SINT_TYPE id;

	/** Pseudoana ID SQL parameter */
	DBALLE_SQL_C_SINT_TYPE id_ana;
	/** Report ID SQL parameter */
	DBALLE_SQL_C_SINT_TYPE id_report;
	/** Date SQL parameter */
	SQL_TIMESTAMP_STRUCT date;
	/** Level type SQL parameter */
	DBALLE_SQL_C_SINT_TYPE ltype;
	/** Level L1 SQL parameter */
	DBALLE_SQL_C_SINT_TYPE l1;
	/** Level L2 SQL parameter */
	DBALLE_SQL_C_SINT_TYPE l2;
	/** Time range type SQL parameter */
	DBALLE_SQL_C_SINT_TYPE pind;
	/** Time range P1 SQL parameter */
	DBALLE_SQL_C_SINT_TYPE p1;
	/** Time range P2 SQL parameter */
	DBALLE_SQL_C_SINT_TYPE p2;
};
/** @copydoc _dba_db_context */
typedef struct _dba_db_context* dba_db_context;

/**
 * Create a new dba_db_context
 * 
 * @param db
 *   The ::dba_db this ::dba_db_context will access
 * @retval ins
 *   The newly created ::dba_db_context (it will need to be deallocated wth dba_db_context_delete())
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_context_create(dba_db db, dba_db_context* ins);

/**
 * Deletes a dba_db_context
 *
 * @param ins
 *   The ::dba_db_context to delete
 */
void dba_db_context_delete(dba_db_context ins);

/**
 * Get the context id for the context data previously set in ins.
 *
 * @param ins
 *   The dba_db_context structure, with parameters filled in for the query
 * @retval id
 *   The database ID, or -1 if no existing context entry matches the given values
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_context_get_id(dba_db_context ins, int *id);

/**
 * Get context information given a context ID
 *
 * @param ins
 *   ::dba_db_context to query
 * @param id
 *   ID of the context to query
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_context_get_data(dba_db_context ins, int id);

/**
 * Get the context id for a pseudoana info context.
 *
 * @param ins
 *   The dba_db_context structure, with id_ana and id_report filled in for the
 *   query.  If id_report is filled with -1, it gets replaced with the report
 *   code for pseudoana information.
 * @retval id
 *   The database ID
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_context_obtain_ana(dba_db_context ins, int *id);

/**
 * Insert a new context in the database
 *
 * @param ins
 *   The dba_db_context structure with all the input fields filled in.
 * @retval id
 *   The ID of the newly inserted context
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_context_insert(dba_db_context ins, int *id);

/**
 * Remove a context record
 *
 * @param ins
 *   The dba_db_context structure, with id filled with the id of the context to
 *   remove.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_context_remove(dba_db_context ins);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
