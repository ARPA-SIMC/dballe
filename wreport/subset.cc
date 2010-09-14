/*
 * wreport/subset - Data subset for BUFR and CREX messages
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

#include <config.h>

#include "subset.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

namespace wreport {

Subset::Subset(const Vartable* btable) : btable(btable) {}
Subset::~Subset() {}

void Subset::store_variable(const Var& var)
{
	push_back(var);
}

void Subset::store_variable(Varcode code, const Var& var)
{
	Varinfo info = btable->query(code);
	push_back(Var(info, var));
}

void Subset::store_variable_i(Varcode code, int val)
{
	Varinfo info = btable->query(code);
	push_back(Var(info, val));
}

void Subset::store_variable_d(Varcode code, double val)
{
	Varinfo info = btable->query(code);
	push_back(Var(info, val));
}

void Subset::store_variable_c(Varcode code, const char* val)
{
	Varinfo info = btable->query(code);
	push_back(Var(info, val));
}

void Subset::store_variable_undef(Varcode code)
{
	Varinfo info = btable->query(code);
	push_back(Var(info));
}

#if 0
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
		while (src < subset->vars_count && WR_VAR_F(dba_var_code(subset->vars[src])) != 0)
			++src;

#if 0
		dba_varcode code = dba_var_code(subset->vars[i]);
		/* Skip over special data like delayed repetition counts */
		if (WR_VAR_F(code) != 0 ||
		    (WR_VAR_F(code) == 0 && WR_VAR_X(code) == 31))
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
	bufrex_subset_store_variable_i(subset, WR_VAR(0, 31, 2), 0);
	repcount_idx = subset->vars_count - 1;
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < subset->vars_count && size > 0; i++)
	{
		dba_var var_attr;

#if 0
		/* Skip over special data like delayed repetition counts */
		if (WR_VAR_F(dba_var_code(subset->vars[i])) != 0)
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
#endif

unsigned Subset::diff(const Subset& s2, FILE* out) const
{
	// Compare btables
	if (btable->id() != s2.btable->id())
	{
		fprintf(out, "B tables differ (first is %s, second is %s)\n",
				btable->id().c_str(), s2.btable->id().c_str());
		return 1;
	}

	// Compare vars
	if (size() != s2.size())
	{
		fprintf(out, "Number of variables differ (first is %zd, second is %zd)\n",
				size(), s2.size());
		return 1;
	}
	for (size_t i = 0; i < size(); ++i)
	{
		unsigned diff = (*this)[i].diff(s2[i], out);
		if (diff > 0) return diff;
	}
	return 0;
}

}

/* vim:set ts=4 sw=4: */
