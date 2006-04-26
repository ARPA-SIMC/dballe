#ifndef DBALLE_DB_PSEUDOANA_H
#define DBALLE_DB_PSEUDOANA_H

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
 * Precompiled query to insert a value in pseudoana
 */
struct _dba_db_pseudoana
{
	struct _dba_db* db;
	SQLHSTMT sfstm;
	SQLHSTMT smstm;
	SQLHSTMT istm;
	SQLHSTMT ustm;

	int id;
	int lat;
	int lon;
	char ident[64];
	SQLINTEGER ident_ind;
};
typedef struct _dba_db_pseudoana* dba_db_pseudoana;


dba_err dba_db_pseudoana_create(dba_db db, dba_db_pseudoana* ins);
void dba_db_pseudoana_delete(dba_db_pseudoana ins);
void dba_db_pseudoana_set_ident(dba_db_pseudoana ins, const char* ident);
dba_err dba_db_pseudoana_get_id(dba_db_pseudoana ins, int *id);
dba_err dba_db_pseudoana_insert(dba_db_pseudoana ins, int *id);
dba_err dba_db_pseudoana_update(dba_db_pseudoana ins);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
