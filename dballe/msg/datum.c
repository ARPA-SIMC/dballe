#include "datum.h"

#include <stdlib.h>

dba_err dba_msg_datum_create(dba_msg_datum* d, int pind, int p1, int p2)
{
	dba_msg_datum res = (dba_msg_datum)calloc(1, sizeof(struct _dba_msg_datum));
	*d = res;
	if (res == NULL)
		return dba_error_alloc("allocating new dba_msg_datum");

	res->pind = pind;
	res->p1 = p1;
	res->p2 = p2;

	return dba_error_ok();
}

dba_err dba_msg_datum_copy(dba_msg_datum src, dba_msg_datum* dst)
{
	dba_err err;

	DBA_RUN_OR_RETURN(dba_msg_datum_create(dst, src->pind, src->p1, src->p2));
	DBA_RUN_OR_GOTO(fail, dba_var_copy(src->var, &((*dst)->var)));

	return dba_error_ok();

fail:
	dba_msg_datum_delete(*dst);
	*dst = NULL;
	return err;
}

void dba_msg_datum_delete(dba_msg_datum d)
{
	if (d->var != NULL)
		dba_var_delete(d->var);
	free(d);
}

int dba_msg_datum_compare(const dba_msg_datum d1, const dba_msg_datum d2)
{
	if (d1->pind != d2->pind)
		return d1->pind - d2->pind;
	if (d1->p1 != d2->p1)
		return d1->p1 - d2->p1;
	if (d1->p2 != d2->p2)
		return d1->p2 - d2->p2;

	return dba_var_code(d1->var) - dba_var_code(d2->var);
}

int dba_msg_datum_compare2(dba_msg_datum d, dba_varcode code, int pind, int p1, int p2)
{
	if (d->pind != pind)
		return d->pind - pind;
	if (d->p1 != p1)
		return d->p1 - p1;
	if (d->p2 != p2)
		return d->p2 - p2;

	return dba_var_code(d->var) - code;
}

/* vim:set ts=4 sw=4: */
