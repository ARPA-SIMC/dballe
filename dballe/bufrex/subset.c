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

#include <config.h>

#include "subset.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

dba_err bufrex_subset_create(dba_vartable btable, bufrex_subset* subset)
{
	*subset = (bufrex_subset)calloc(1, sizeof(struct _bufrex_subset));
	if (*subset == NULL)
		return dba_error_alloc("allocating new storage for a subsection of decoded BUFR/CREX data");
	(*subset)->btable = btable;
	return dba_error_ok();
}

void bufrex_subset_delete(bufrex_subset subset)
{
	bufrex_subset_reset(subset);

	if (subset->vars)
	{
		free(subset->vars);
		subset->vars = NULL;
		subset->vars_alloclen = 0;
	}

	free(subset);
}

void bufrex_subset_reset(bufrex_subset subset)
{
	int i;

	/* Preserve vars and vars_alloclen so that allocated memory can be reused */
	for (i = 0; i < subset->vars_count; i++)
		dba_var_delete(subset->vars[i]);
	subset->vars_count = 0;
}

void bufrex_subset_truncate(bufrex_subset subset, size_t size)
{
	int i;
	/* Preserve vars and vars_alloclen so that allocated memory can be reused */
	for (i = size; i < subset->vars_count; ++i)
		dba_var_delete(subset->vars[i]);
	subset->vars_count = size;
}

dba_err bufrex_subset_store_variable(bufrex_subset subset, dba_var var)
{
	/* Check if we need to enlarge the buffer size */
	if (subset->vars_count == subset->vars_alloclen)
	{
		/* Enlarge the buffer size */
		if (subset->vars == NULL)
		{
			subset->vars_alloclen = 32;
			if ((subset->vars = (dba_var*)malloc(subset->vars_alloclen * sizeof(dba_var))) == NULL)
				return dba_error_alloc("allocating memory for decoded message variables");
		} else {
			dba_var* newbuf;

			/* Grow by doubling the allocated space */
			subset->vars_alloclen <<= 1;

			if ((newbuf = (dba_var*)realloc(subset->vars, subset->vars_alloclen * sizeof(dba_var))) == NULL)
				return dba_error_alloc("allocating more memory for message data");
			subset->vars = newbuf;
		}
	}

	subset->vars[subset->vars_count++] = var;
	return dba_error_ok();
}

dba_err bufrex_subset_store_variable_var(bufrex_subset subset, dba_varcode code, dba_var val)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(subset->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_create(info, &var));
	if (val != NULL)
		DBA_RUN_OR_RETURN(dba_var_copy_val(var, val));
	return bufrex_subset_store_variable(subset, var);
}

dba_err bufrex_subset_store_variable_i(bufrex_subset subset, dba_varcode code, int val)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(subset->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_createi(info, val, &var));
	return bufrex_subset_store_variable(subset, var);
}

dba_err bufrex_subset_store_variable_d(bufrex_subset subset, dba_varcode code, double val)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(subset->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_created(info, val, &var));
	return bufrex_subset_store_variable(subset, var);
}

dba_err bufrex_subset_store_variable_c(bufrex_subset subset, dba_varcode code, const char* val)
{
	if (val == NULL || val[0] == 0)
	{
		return bufrex_subset_store_variable_undef(subset, code);
	} else {
		dba_var var;
		dba_varinfo info;
		DBA_RUN_OR_RETURN(dba_vartable_query(subset->btable, code, &info));
		DBA_RUN_OR_RETURN(dba_var_createc(info, val, &var));
		return bufrex_subset_store_variable(subset, var);
	}
}

dba_err bufrex_subset_store_variable_undef(bufrex_subset subset, dba_varcode code)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(subset->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_create(info, &var));
	return bufrex_subset_store_variable(subset, var);
}


dba_err bufrex_subset_add_attr(bufrex_subset subset, dba_var attr)
{
	if (subset->vars_count == 0)
		return dba_error_consistency("checking that some variable was previously appended");
	return dba_var_seta(subset->vars[subset->vars_count - 1], attr);
}

dba_err bufrex_subset_add_attrs(bufrex_subset subset, dba_var var)
{
	if (subset->vars_count == 0)
		return dba_error_consistency("checking that some variable was previously appended");
	return dba_var_copy_attrs(subset->vars[subset->vars_count - 1], var);
}

static dba_err bufrex_subset_append_c_with_dpb(bufrex_subset subset, dba_varcode ccode, int count, const char* bitmap)
{
	dba_err err = DBA_OK;
	dba_varinfo info = NULL;
	dba_var var = NULL;

	/* Create a single use varinfo to store the bitmap */
	DBA_RUN_OR_GOTO(cleanup, dba_varinfo_create_singleuse(ccode, &info));
	strcpy(info->desc, "DATA PRESENT BITMAP");
	strcpy(info->unit, "CCITTIA5");
	strcpy(info->bufr_unit, "CCITTIA5");
	info->len = count;
	info->bit_len = info->len * 8;

	/* Create the dba_var with the bitmap */
	DBA_RUN_OR_GOTO(cleanup, dba_var_createc(info, bitmap, &var));
	info = NULL;

	/* Store the variable in the subset */
	DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable(subset, var));
	var = NULL;

cleanup:
	if (info != NULL) dba_varinfo_delete(info);
	if (var != NULL) dba_var_delete(var);

	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_subset_append_dpb(bufrex_subset subset, dba_varcode ccode, int size, dba_varcode attr, int* count)
{
	dba_err err = DBA_OK;
	char* bitmap = (char*)malloc(size + 1);
	int src, dst;
	*count = 0;

	if (bitmap == 0)
		return dba_error_alloc("Allocating space for building a data present bitmap");

	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (src = 0, dst = 0; src < subset->vars_count && dst < size; dst++)
	{
		dba_var test_var;

		/* Skip extra, special vars */
		while (src < subset->vars_count && DBA_VAR_F(dba_var_code(subset->vars[src])) != 0)
			++src;

#if 0
		dba_varcode code = dba_var_code(subset->vars[i]);
		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(code) != 0 ||
		    (DBA_VAR_F(code) == 0 && DBA_VAR_X(code) == 31))
			continue;
#endif

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_GOTO(cleanup, dba_var_enqa(subset->vars[src], attr, &test_var));

		if (test_var == NULL)
			bitmap[dst] = '-';
		else
		{
			bitmap[dst] = '+';
			++*count;
		}
	}
	bitmap[size] = 0;

	/* Append the bitmap to the message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_subset_append_c_with_dpb(subset, ccode, size, bitmap));

cleanup:
	if (bitmap != NULL) free(bitmap);

	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_subset_append_fixed_dpb(bufrex_subset subset, dba_varcode ccode, int size)
{
	dba_err err = DBA_OK;
	char* bitmap = (char*)malloc(size + 1);

	memset(bitmap, '+', size);
	bitmap[size] = 0;

	/* Append the bitmap to the message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_subset_append_c_with_dpb(subset, ccode, size, bitmap));

cleanup:
	if (bitmap != NULL) free(bitmap);

	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_subset_append_attrs(bufrex_subset subset, int size, dba_varcode attr)
{
	int i;
	int repcount_idx;
	int added = 0;

	/* Add delayed repetition count with an initial value of 0, and mark its position */
	bufrex_subset_store_variable_i(subset, DBA_VAR(0, 31, 2), 0);
	repcount_idx = subset->vars_count - 1;
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < subset->vars_count && size > 0; i++)
	{
		dba_var var_attr;

#if 0
		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(subset->vars[i])) != 0)
			continue;
#endif

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_RETURN(dba_var_enqa(subset->vars[i], attr, &var_attr));
		if (var_attr != NULL)
		{
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(subset, attr, var_attr));
			added++;
		}
		size--;
	}

	/* Set the repetition count with the number of variables we added */
	DBA_RUN_OR_RETURN(dba_var_seti(subset->vars[repcount_idx], added));

	return dba_error_ok();
}

void bufrex_subset_diff(bufrex_subset s1, bufrex_subset s2, int* diffs, FILE* out)
{
	// TODO: btable;

	if (s1->vars_count != s2->vars_count)
	{
		fprintf(out, "Number of variables differ (first is %zd, second is %zd)\n",
				s1->vars_count, s2->vars_count);
		++*diffs;
	} else {
		int i;
		for (i = 0; i < s1->vars_count; ++i)
			dba_var_diff(s1->vars[i], s2->vars[i], diffs, out);
	}
}

/* vim:set ts=4 sw=4: */
