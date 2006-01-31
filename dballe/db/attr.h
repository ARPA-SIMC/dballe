#ifndef DBALLE_DB_ATTR_H
#define DBALLE_DB_ATTR_H

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
 * Precompiled query to insert a value in attr
 */
struct _dba_db_attr
{
	struct _dba_db* db;
	SQLHSTMT istm;
	SQLHSTMT rstm;

	int id_context;
	dba_varcode id_var;
	dba_varcode type;
	char value[255];
	SQLINTEGER value_ind;
};
typedef struct _dba_db_attr* dba_db_attr;


dba_err dba_db_attr_create(dba_db db, dba_db_attr* ins);
void dba_db_attr_delete(dba_db_attr ins);
void dba_db_attr_set(dba_db_attr ins, dba_var var);
void dba_db_attr_set_value(dba_db_attr ins, const char* value);
dba_err dba_db_attr_insert(dba_db_attr ins, int replace);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
