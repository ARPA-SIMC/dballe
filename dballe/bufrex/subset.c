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

dba_err bufrex_subset_apply_attributes(bufrex_subset subset)
{
	int i = 0;
	int cur_dpb = -1;
	int cur_attr = -1;

	/* First step: scan the variable list and note the index of the first data
	 * present bitmap and the first quality attribute */
	for ( ; i < subset->vars_count; i++)
		if (dba_var_code(subset->vars[i]) == DBA_VAR(0, 31, 31))
		{
			cur_dpb = i;
			break;
		}
	for ( ; i < subset->vars_count; i++)
	{
		dba_varcode code = dba_var_code(subset->vars[i]);
		if (DBA_VAR_F(code) == 0 && DBA_VAR_X(code) == 33)
		{
			cur_attr = i;
			break;
		}
	}

	/* Nothing to do if the data is not present */
	if (cur_dpb == -1 || cur_attr == -1)
		return dba_error_ok();
	

	/* Second step: iterate through the three lists applying the changes */
	for (i = 0; i < subset->vars_count &&
			cur_dpb < subset->vars_count &&
			dba_var_code(subset->vars[cur_dpb]) == DBA_VAR(0, 31, 31) &&
			cur_attr < subset->vars_count &&
			DBA_VAR_F(dba_var_code(subset->vars[cur_attr])) == 0 &&
			DBA_VAR_X(dba_var_code(subset->vars[cur_attr])) == 33; i++)
	{
		dba_var attr;
		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(subset->vars[i])) != 0)
			continue;
		/* Skip over variables that don't have a 0 in the data present bitmap */
		if (dba_var_value(subset->vars[cur_dpb++]) == NULL)
			continue;
		attr = subset->vars[cur_attr++];
		/* Skip over attributes with NULL values */
		if (dba_var_value(attr) == NULL)
			continue;
		DBA_RUN_OR_RETURN(dba_var_seta(subset->vars[i], attr));
	}
	return dba_error_ok();
}

dba_err bufrex_subset_append_dpb(bufrex_subset subset, int size, dba_varcode attr)
{
	int i;

	/* Add repetition count */
	bufrex_subset_store_variable_i(subset, DBA_VAR(0, 31, 2), size);
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < subset->vars_count && size > 0; i++)
	{
		dba_var test_var;

#if 0
		dba_varcode code = dba_var_code(subset->vars[i]);
		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(code) != 0 ||
		    (DBA_VAR_F(code) == 0 && DBA_VAR_X(code) == 31))
			continue;
#endif

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_RETURN(dba_var_enqa(subset->vars[i], attr, &test_var));
		if (test_var != NULL)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(subset, DBA_VAR(0, 31, 31), 0));
		else
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(subset, DBA_VAR(0, 31, 31)));
		size--;
	}
	return dba_error_ok();
}

dba_err bufrex_subset_append_fixed_dpb(bufrex_subset subset, int size)
{
	int i;

	for (i = 0; i < size; i++)
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(subset, DBA_VAR(0, 31, 31), 0));

	return dba_error_ok();
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

dba_err bufrex_subset_append_fixed_attrs(bufrex_subset subset, int size, dba_varcode attr)
{
	int i;
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < subset->vars_count && size > 0; i++)
	{
		dba_var var_attr;

		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(subset->vars[i])) != 0)
			continue;

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_RETURN(dba_var_enqa(subset->vars[i], attr, &var_attr));
		if (var_attr != NULL)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(subset, attr, var_attr));
		else
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(subset, attr));
		size--;
	}

	return dba_error_ok();
}

void bufrex_subset_diff(bufrex_subset s1, bufrex_subset s2, int* diffs, FILE* out)
{
	// TODO: btable;

	if (s1->vars_count != s2->vars_count)
	{
		fprintf(out, "Number of variables differ (first is %d, second is %d)\n",
				s1->vars_count, s2->vars_count);
		++*diffs;
	} else {
		int i;
		for (i = 0; i < s1->vars_count; ++i)
			dba_var_diff(s1->vars[i], s2->vars[i], diffs, out);
	}
}

/* vim:set ts=4 sw=4: */
