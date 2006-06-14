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
 * Pseudoana table management used by the db module, but not
 * exported as official API.
 */

#include <dballe/db/internals.h>

struct _dba_db;
	
/**
 * Precompiled query to insert a value in context
 */
struct _dba_db_context
{
	struct _dba_db* db;
	SQLHSTMT sstm;
	SQLHSTMT istm;

	int id;

	int id_ana;
	int id_report;
	char date[25];
	SQLINTEGER date_ind;
	int ltype;
	int l1;
	int l2;
	int pind;
	int p1;
	int p2;
};
typedef struct _dba_db_context* dba_db_context;


dba_err dba_db_context_create(dba_db db, dba_db_context* ins);
void dba_db_context_delete(dba_db_context ins);

/**
 * Get the context id for the context data previously set in ins.
 *
 * @param ins
 *   The dba_db_context structure, with parameters filled in for the query
 * @retval id
 *   The database ID, or -1 if no existing context entry matches the given values
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_context_get_id(dba_db_context ins, int *id);

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
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_db_context_obtain_ana(dba_db_context ins, int *id);

dba_err dba_db_context_insert(dba_db_context ins, int *id);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
