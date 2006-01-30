#include <dballe/core/dba_record.h>

#include "exporters.h"

static dba_err exporter(dba_msg src, bufrex_raw dst, int type);

bufrex_exporter bufrex_exporter_generic = {
	/* Category */
	255,
	/* Subcategory */
	0,
	/* dba_msg type it can convert from */
	MSG_GENERIC,
	/* Data descriptor section */
	(dba_varcode[]){ 0 },
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

static dba_err exporter(dba_msg src, bufrex_raw dst, int type)
{
	dba_err err = DBA_OK;
	dba_var copy = NULL;
	dba_var attr_copy = NULL;
	int i, j;
	int ltype = -1, l1 = -1, l2 = -1, pind = -1, p1 = -1, p2 = -1;

	for (i = 0; i < src->data_count; i++)
	{
		dba_msg_level lev = src->data[i];

		for (j = 0; j < lev->data_count; j++)
		{
			dba_var_attr_iterator iter;
			dba_msg_datum d = lev->data[j];
			if (dba_var_value(d->var) == NULL)
				continue;

			/* Update the context in the message, if needed */
			if (ltype != lev->ltype)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable_i(dst, DBA_VAR(0, 7, 192), lev->ltype));
				ltype = lev->ltype;
			}
			if (l1 != lev->l1)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable_i(dst, DBA_VAR(0, 7, 193), lev->l1));
				l1 = lev->l1;
			}
			if (l2 != lev->l2)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable_i(dst, DBA_VAR(0, 7, 194), lev->l2));
				l2 = lev->l2;
			}
			if (pind != d->pind)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable_i(dst, DBA_VAR(0, 4, 192), d->pind));
				pind = d->pind;
			}
			if (p1 != d->p1)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable_i(dst, DBA_VAR(0, 4, 193), d->p1));
				p1 = d->p1;
			}
			if (p2 != d->p2)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable_i(dst, DBA_VAR(0, 4, 194), d->p2));
				p2 = d->p2;
			}

			/* Store the variable */
			DBA_RUN_OR_GOTO(cleanup, dba_var_copy(d->var, &copy));
			DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable(dst, copy));

			/* Store the attributes */
			for (iter = dba_var_attr_iterate(copy);
					iter != NULL;
					iter = dba_var_attr_iterator_next(iter))
			{
				dba_var attr = dba_var_attr_iterator_attr(iter);
				DBA_RUN_OR_GOTO(cleanup, dba_var_copy(attr, &attr_copy));
				DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable(dst, attr_copy));
				attr_copy = NULL;
			}
			
			copy = NULL;
		}
	}

cleanup:
	if (copy != NULL)
		dba_var_delete(copy);
	if (attr_copy != NULL)
		dba_var_delete(attr_copy);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
