/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/msg/context.h>
#include <dballe/msg/vars.h>

#include <stdlib.h>
#include <string.h>

dba_err dba_msg_context_create(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2, dba_msg_context* l)
{
	dba_msg_context res = (dba_msg_context)calloc(1, sizeof(struct _dba_msg_context));
	if (res == NULL)
		return dba_error_alloc("allocating new dba_msg_context");

	res->ltype1 = ltype1;
	res->l1 = l1;
	res->ltype2 = ltype2;
	res->l2 = l2;
	res->pind = pind;
	res->p1 = p1;
	res->p2 = p2;
	res->data_count = 0;
	res->data_alloc = 0;
	res->data = NULL;

	*l = res;
	
	return dba_error_ok();
}

/* Enlarge the number of data that can be held by l, so that it is at least 1
 * (possibly much more) more than it is now */
static dba_err dba_msg_context_enlarge(dba_msg_context l)
{
	const int new_size = l->data_alloc == 0 ? 4 : l->data_alloc * 2;

	if (l->data == NULL)
	{
		l->data = (dba_var*)calloc(new_size, sizeof(dba_var));
		if (l->data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg_context");
	} else {
		int i;
		dba_var* new_data;
		new_data = (dba_var*)realloc(l->data, new_size * sizeof(dba_var));
		if (new_data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg_context");
		l->data = new_data;
		
		for (i = l->data_count; i < new_size; i++)
			l->data[i] = NULL;
	}
	l->data_alloc = new_size;

	return dba_error_ok();
}

dba_err dba_msg_context_copy(dba_msg_context src, dba_msg_context* dst)
{
	dba_err err = DBA_OK;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_context_create(src->ltype1, src->l1, src->ltype2, src->l2, src->pind, src->p1, src->p2, dst));
	
	/* Allocate enough space to copy the items */
	while ((*dst)->data_alloc < src->data_count)
		DBA_RUN_OR_GOTO(fail, dba_msg_context_enlarge(*dst));
	
	for (i = 0; i < src->data_count; i++)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_copy(src->data[i], &((*dst)->data[i])));
		++(*dst)->data_count;
	}

	return dba_error_ok();

fail:
	dba_msg_context_delete(*dst);
	*dst = NULL;
	return err;
}

void dba_msg_context_delete(dba_msg_context l)
{
	if (l->data_alloc)
	{
		int i;
		for (i = 0; i < l->data_count; i++)
			dba_var_delete(l->data[i]);
		free(l->data);
	}
	free(l);
}

int dba_msg_context_compare(const dba_msg_context l1, const dba_msg_context l2)
{
	int res;
	if ((res = l1->ltype1 - l2->ltype1)) return res;
	if ((res = l1->l1 - l2->l1)) return res;
	if ((res = l1->ltype2 - l2->ltype2)) return res;
	if ((res = l1->l2 - l2->l2)) return res;
	if ((res = l1->pind - l2->pind)) return res;
	if ((res = l1->p1 - l2->p1)) return res;
	return l1->p2 - l2->p2;
}

int dba_msg_context_compare2(const dba_msg_context l, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	int res;
	if ((res = l->ltype1 - ltype1)) return res;
	if ((res = l->l1 - l1)) return res;
	if ((res = l->ltype2 - ltype2)) return res;
	if ((res = l->l2 - l2)) return res;
	if ((res = l->pind - pind)) return res;
	if ((res = l->p1 - p1)) return res;
	return l->p2 - p2;
}

/*
 * Return the index of the var `code' in l, or -1 if it was not found
 */
static int dba_msg_context_find_index(dba_msg_context l, dba_varcode code)
{
	/* Binary search */
	int low = 0, high = l->data_count - 1;
	while (low <= high)
	{
		int middle = low + (high - low)/2;
		int cmp = (int)code - (int)dba_var_code(l->data[middle]);
		if (cmp < 0)
			high = middle - 1;
		else if (cmp > 0)
			low = middle + 1;
		else
			return middle;
	}

	return -1;
}

dba_var dba_msg_context_find_vsig(dba_msg_context ctx)
{
	dba_var res = NULL;

	// Check if we have the right context information
	if ((ctx->ltype1 != 100 && ctx->ltype1 != 102) || ctx->pind != 254 || ctx->p1 != 0 || ctx->p2 != 0)
		return NULL;

	// Look for VSS variable
	res = dba_msg_context_find(ctx, DBA_VAR(0, 8, 1));
	if (res == NULL) return NULL;

	// Ensure it is not undefined
	if (dba_var_value(res) == NULL) return NULL;

	// Finally return it
	return res;
}

dba_err dba_msg_context_set_nocopy(dba_msg_context l, dba_var var)
{
	dba_varcode code = dba_var_code(var);
	int idx = dba_msg_context_find_index(l, code);

	if (idx != -1)
	{
		/* Replace the variable */
		dba_var_delete(l->data[idx]);
		l->data[idx] = var;
	}
	else
	{
		/* Add the value */

		/* Enlarge the buffer if needed */
		if (l->data_count == l->data_alloc)
			DBA_RUN_OR_RETURN(dba_msg_context_enlarge(l));

		/* Insertionsort.  Crude, but our datasets should be too small for an
		 * RB-Tree to be worth */
		for (idx = l->data_count; idx > 0; idx--)
			if (dba_var_code(l->data[idx - 1]) > code)
				l->data[idx] = l->data[idx - 1];
			else
				break;
		l->data[idx] = var;

		l->data_count++;
	}

	return dba_error_ok();
}

dba_var dba_msg_context_find(dba_msg_context l, dba_varcode code)
{
	int idx = dba_msg_context_find_index(l, code);
	return (idx == -1) ? NULL : l->data[idx];
}

dba_var dba_msg_context_find_by_id(dba_msg_context l, int id)
{
	return dba_msg_context_find(l, dba_msg_vartable[id].code);
}

void dba_msg_context_print(dba_msg_context l, FILE* out)
{
	fprintf(out, "Level %d,%d, %d,%d  tr %d,%d,%d ", l->ltype1, l->l1, l->ltype2, l->l2, l->pind, l->p1, l->p2);

	if (l->data_count > 0)
	{
		int i;
		fprintf(out, " %d vars:\n", l->data_count);
		for (i = 0; i < l->data_count; i++)
			dba_var_print(l->data[i], out);
	} else
		fprintf(out, "exists but is empty.\n");
}

static void var_summary(dba_var var, FILE* out)
{
	dba_varcode v = dba_var_code(var);
	dba_varinfo info = dba_var_info(var);
	fprintf(out, "%d%02d%03d[%s]",
			DBA_VAR_F(v), DBA_VAR_X(v), DBA_VAR_Y(v),
			info->desc);
}

void dba_msg_context_diff(dba_msg_context l1, dba_msg_context l2, int* diffs, FILE* out)
{
	int i1 = 0, i2 = 0;
	if (l1->ltype1 != l2->ltype1 || l1->l1 != l2->l1
	 || l1->ltype2 != l2->ltype2 || l1->l2 != l2->l2
	 || l1->pind != l2->pind || l1->p1 != l2->p1 || l1->p2 != l2->p2)
	{
		fprintf(out, "the contexts are different (first is %d,%d, %d,%d, %d,%d,%d second is %d,%d, %d,%d, %d,%d,%d)\n",
				l1->ltype1, l1->l1, l1->ltype2, l1->l2, l1->pind, l1->p1, l1->p2,
				l2->ltype1, l2->l1, l2->ltype2, l2->l2, l2->pind, l2->p1, l2->p2);
		(*diffs)++;
		return;
	}
	
	while (i1 < l1->data_count || i2 < l2->data_count)
	{
		if (i1 == l1->data_count)
		{
			fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", l2->ltype1, l2->l1, l2->ltype2, l2->l2, l2->pind, l2->p1, l2->p2); var_summary(l2->data[i2], out);
			fprintf(out, " exists only in the second message\n");
			++i2;
			++*diffs;
		} else if (i2 == l2->data_count) {
			fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", l1->ltype1, l1->l1, l1->ltype2, l1->l2, l1->pind, l1->p1, l1->p2); var_summary(l2->data[i2], out);
			fprintf(out, " exists only in the first message\n");
			++i1;
			++*diffs;
		} else {
			int cmp = (int)dba_var_code(l1->data[i1]) - (int)dba_var_code(l2->data[i2]);
			if (cmp == 0)
			{
				dba_var_diff(l1->data[i1], l2->data[i2], diffs, out);
				++i1;
				++i2;
			} else if (cmp < 0) {
				if (dba_var_value(l1->data[i1]) != NULL)
				{
					fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", l1->ltype1, l1->l1, l1->ltype2, l1->l2, l1->pind, l1->p1, l1->p2); var_summary(l1->data[i1], out);
					fprintf(out, " exists only in the first message\n");
					++*diffs;
				}
				++i1;
			} else {
				if (dba_var_value(l2->data[i2]) != NULL)
				{
					fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", l2->ltype1, l2->l1, l2->ltype2, l2->l2, l2->pind, l2->p1, l2->p2); var_summary(l2->data[i2], out);
					fprintf(out, " exists only in the second message\n");
					++*diffs;
				}
				++i2;
			}
		}
	}
}

/* vim:set ts=4 sw=4: */
