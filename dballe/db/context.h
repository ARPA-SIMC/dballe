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
dba_err dba_db_context_get_id(dba_db_context ins, int *id);
dba_err dba_db_context_insert(dba_db_context ins, int *id);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
