#ifndef DBA_MSG_DATUM_H
#define DBA_MSG_DATUM_H

/** @file
 * @ingroup msg
 *
 * Store a dba_var together with time range metadata.
 */

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/dba_var.h>

struct _dba_msg_datum
{
	dba_var var;
	int pind, p1, p2;
};
typedef struct _dba_msg_datum* dba_msg_datum;

dba_err dba_msg_datum_create(dba_msg_datum* d, int pind, int p1, int p2);
dba_err dba_msg_datum_copy(dba_msg_datum src, dba_msg_datum* dst);
void dba_msg_datum_delete(dba_msg_datum d);
int dba_msg_datum_compare(const dba_msg_datum d1, const dba_msg_datum d2);
int dba_msg_datum_compare2(dba_msg_datum d, dba_varcode code, int pind, int p1, int p2);


#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
