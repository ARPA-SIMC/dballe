#include "level.h"

#include <stdlib.h>
#include <string.h>

dba_err dba_msg_level_create(dba_msg_level* l, int ltype, int l1, int l2)
{
	dba_msg_level res = (dba_msg_level)calloc(1, sizeof(struct _dba_msg_level));
	if (res == NULL)
		return dba_error_alloc("allocating new dba_msg_level");

	res->ltype = ltype;
	res->l1 = l1;
	res->l2 = l2;
	res->data_count = 0;
	res->data_alloc = 0;
	res->data = NULL;

	*l = res;
	
	return dba_error_ok();
}

/* Enlarge the number of data that can be held by l, so that it is at least 1
 * (possibly much more) more than it is now */
static dba_err dba_msg_level_enlarge(dba_msg_level l)
{
	const int new_size = l->data_alloc == 0 ? 4 : l->data_alloc * 2;

	if (l->data == NULL)
	{
		l->data = (dba_msg_datum*)calloc(new_size, sizeof(dba_msg_datum));
		if (l->data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg_level");
	} else {
		int i;
		dba_msg_datum* new_data;
		new_data = (dba_msg_datum*)realloc(l->data, new_size * sizeof(dba_msg_datum));
		if (new_data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg_level");
		l->data = new_data;
		
		for (i = l->data_count; i < new_size; i++)
			l->data[i] = NULL;
	}
	l->data_alloc = new_size;

	return dba_error_ok();
}

dba_err dba_msg_level_copy(dba_msg_level src, dba_msg_level* dst)
{
	dba_err err = DBA_OK;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_level_create(dst, src->ltype, src->l1, src->l2));
	
	/* Allocate enough space to copy the items */
	while ((*dst)->data_alloc < src->data_count)
		DBA_RUN_OR_GOTO(fail, dba_msg_level_enlarge(*dst));
	
	for (i = 0; i < src->data_count; i++)
	{
		DBA_RUN_OR_GOTO(fail, dba_msg_datum_copy(src->data[i], &((*dst)->data[i])));
		++(*dst)->data_count;
	}

	return dba_error_ok();

fail:
	dba_msg_level_delete(*dst);
	*dst = NULL;
	return err;
}

void dba_msg_level_delete(dba_msg_level l)
{
	if (l->data_alloc)
	{
		int i;
		for (i = 0; i < l->data_count; i++)
			dba_msg_datum_delete(l->data[i]);
		free(l->data);
	}
	free(l);
}

int dba_msg_level_compare(const dba_msg_level l1, const dba_msg_level l2)
{
	if (l1->ltype != l2->ltype)
		return l1->ltype - l2->ltype;
	if (l1->l1 != l2->l1)
		return l1->l1 - l2->l1;
	return l1->l2 - l2->l2;
}

int dba_msg_level_compare2(const dba_msg_level l, int ltype, int l1, int l2)
{
	if (l->ltype != ltype)
		return l->ltype - ltype;
	if (l->l1 != l1)
		return l->l1 - l1;
	return l->l2 - l2;
}

dba_err dba_msg_level_set_nocopy(dba_msg_level l, dba_var var, int pind, int p1, int p2)
{
	dba_msg_datum datum = dba_msg_level_find(l, dba_var_code(var), pind, p1, p2);

	if (datum != NULL)
	{
		/* Replace the value */
		dba_var_delete(datum->var);
		datum->var = var;
	} else {
		/* Add the value */
		int pos;

		/* Enlarge the buffer if needed */
		if (l->data_count == l->data_alloc)
			DBA_RUN_OR_RETURN(dba_msg_level_enlarge(l));

		DBA_RUN_OR_RETURN(dba_msg_datum_create(&datum, pind, p1, p2));
		datum->var = var;

		/* Insertionsort.  Crude, but our datasets should be too small for an
		 * RB-Tree to be worth */
		for (pos = l->data_count; pos > 0; pos--)
			if (dba_msg_datum_compare(l->data[pos - 1], datum) > 0)
				l->data[pos] = l->data[pos - 1];
			else
				break;
		l->data[pos] = datum;

		l->data_count++;
	}

	return dba_error_ok();
}

dba_msg_datum dba_msg_level_find(dba_msg_level l, dba_varcode code, int pind, int p1, int p2)
{
	int begin, end;

	/* Binary search */
	begin = -1, end = l->data_count;
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (dba_msg_datum_compare2(l->data[cur], code, pind, p1, p2) > 0)
			end = cur;
		else
			begin = cur;
	}

	if (begin == -1 || dba_msg_datum_compare2(l->data[begin], code, pind, p1, p2) != 0)
		return NULL;
	else
		return l->data[begin];
}

void dba_msg_level_print(dba_msg_level l, FILE* out)
{
	if (l->data_count > 0)
	{
		int i;
		fprintf(out, "Level %d,%d,%d, %d vars:\n", l->ltype, l->l1, l->l2, l->data_count);
		for (i = 0; i < l->data_count; i++)
		{
			fprintf(out, "   p %d,%d,%d ", l->data[i]->pind, l->data[i]->p1, l->data[i]->p2);
			dba_var_print(l->data[i]->var, out);
		}
	} else
		fprintf(out, "Level %d,%d,%d exists but is empty.\n", l->ltype, l->l1, l->l2);
}

static void datum_summary(dba_msg_datum d, FILE* out)
{
	dba_varcode v = dba_var_code(d->var);
	dba_varinfo info = dba_var_info(d->var);
	fprintf(out, "%d%02d%03d[%s],p(%d,%d,%d)",
			DBA_VAR_F(v), DBA_VAR_X(v), DBA_VAR_Y(v),
			info->desc,
			d->pind, d->p1, d->p2);
}

void dba_msg_level_diff(dba_msg_level l1, dba_msg_level l2, int* diffs, FILE* out)
{
	int i1 = 0, i2 = 0;
	if (l1->ltype != l2->ltype || l1->l1 != l2->l1 || l1->l2 != l2->l2)
	{
		fprintf(out, "the levels are different (first is %d,%d,%d, second is %d,%d,%d)\n",
				l1->ltype, l1->l1, l1->l2, l2->ltype, l2->l1, l2->l2);
		(*diffs)++;
		return;
	}
	
	while (i1 < l1->data_count || i2 < l2->data_count)
	{
		if (i1 == l1->data_count)
		{
			fprintf(out, "Variable "); datum_summary(l2->data[i2], out);
			fprintf(out, " exists only in the second message\n");
			++i2;
			++*diffs;
		} else if (i2 == l2->data_count) {
			fprintf(out, "Variable "); datum_summary(l1->data[i1], out);
			fprintf(out, " exists only in the first message\n");
			++i1;
			++*diffs;
		} else {
			int cmp = dba_msg_datum_compare(l1->data[i1], l2->data[i2]);
			if (cmp == 0)
			{
				dba_var_diff(l1->data[i1]->var, l2->data[i2]->var, diffs, out);
				++i1;
				++i2;
			} else if (cmp < 0) {
				if (dba_var_value(l1->data[i1]->var) != NULL)
				{
					fprintf(out, "Variable "); datum_summary(l1->data[i1], out);
					fprintf(out, " exists only in the first message\n");
					++*diffs;
				}
				++i1;
			} else {
				if (dba_var_value(l2->data[i2]->var) != NULL)
				{
					fprintf(out, "Variable "); datum_summary(l2->data[i2], out);
					fprintf(out, " exists only in the second message\n");
					++*diffs;
				}
				++i2;
			}
		}
	}
}

/* vim:set ts=4 sw=4: */
