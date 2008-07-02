/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "dbapi.h"
#include <dballe/core/aliases.h>
#include <dballe/core/verbose.h>
#include <dballe/db/internals.h>
#include <cstdlib>

/*
#include <f77.h>
#include <float.h>
#include <limits.h>

#include <stdio.h>	// snprintf
#include <string.h>	// strncpy
#include <math.h>
*/

using namespace std;

namespace dballef {


DbAPI::DbAPI(dba_db& db, const char* anaflag, const char* dataflag, const char* attrflag)
	: db(db), ana_cur(0), query_cur(0)
{
	set_permissions(anaflag, dataflag, attrflag);
}

DbAPI::~DbAPI()
{
	if (ana_cur)
		dba_db_cursor_delete(ana_cur);
	if (query_cur)
		dba_db_cursor_delete(query_cur);
}

void DbAPI::scopa(const char* repinfofile)
{
	if (!(perms & PERM_DATA_WRITE))
		checked(dba_error_consistency(
			"scopa must be run with the database open in data write mode"));

	checked(dba_db_reset(db, repinfofile));
}

int DbAPI::quantesono()
{
	if (ana_cur != NULL)
	{
		dba_db_cursor_delete(ana_cur);
		ana_cur = NULL;
	}

	if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
	{
		dba_verbose(DBA_VERB_DB_INPUT,
				"invoking dba_db_ana_query(db, <input>).  <input> is:\n");
		dba_record_print(input, DBA_VERBOSE_STREAM);
	}

	int count;
	checked(dba_db_ana_query(db, input, &ana_cur, &count));
	return count;
}

void DbAPI::elencamele()
{
	if (ana_cur == NULL)
		checked(dba_error_consistency("elencamele called without a previous quantesono"));

	int has_data;
	checked(dba_db_cursor_next(ana_cur, &has_data));

	dba_record_clear(output);
	if (!has_data)
	{
		dba_db_cursor_delete(ana_cur);
		ana_cur = NULL;
	} else
		checked(dba_db_cursor_to_record(ana_cur, output));
}

int DbAPI::voglioquesto()
{
	if (query_cur != NULL)
	{
		dba_db_cursor_delete(query_cur);
		query_cur = NULL;
	}

	if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
	{
		dba_verbose(DBA_VERB_DB_INPUT,
				"invoking dba_query(db, <input>).  <input> is:\n");
		dba_record_print(input, DBA_VERBOSE_STREAM);
	}

	int count;
	checked(dba_db_query(db, input, &query_cur, &count));
	return count;
}

const char* DbAPI::dammelo()
{
	if (query_cur == NULL)
		checked(dba_error_consistency("dammelo called without a previous voglioquesto"));

	/* Reset qc record iterator, so that ancora will not return
	 * leftover QC values from a previous query */
	qc_iter = 0;

	int has_data;
	checked(dba_db_cursor_next(query_cur, &has_data));
	if (!has_data)
	{
		dba_db_cursor_delete(query_cur);
		query_cur = NULL;
		dba_record_clear(output);
		return 0;
	} else {
		dba_record_clear(output);
		checked(dba_db_cursor_to_record(query_cur, output));
		const char* varstr;
		checked(dba_record_key_enqc(output, DBA_KEY_VAR, &varstr));

		/* Set context id and variable name on qcinput so that
		 * attribute functions will refer to the last variable read */
		checked(dba_record_key_seti(qcinput, DBA_KEY_CONTEXT_ID,
							query_cur->out_context_id));
		checked(dba_record_key_setc(qcinput, DBA_KEY_VAR_RELATED, varstr));
		return varstr;
	}
}

void DbAPI::prendilo()
{
	if (perms & PERM_DATA_RO)
		checked(dba_error_consistency(
			"idba_prendilo cannot be called with the database open in data readonly mode"));

	if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
	{
		dba_verbose(DBA_VERB_DB_INPUT,
				"invoking dba_insert_or_replace(db, <input>, %d, %d).  <input> is:\n",
				perms & PERM_DATA_WRITE ? 1 : 0,
				perms & PERM_ANA_WRITE ? 1 : 0);
		dba_record_print(input, DBA_VERBOSE_STREAM);
	}

	int ana_id, context_id;
	checked(dba_db_insert(
				db, input,
				perms & PERM_DATA_WRITE ? 1 : 0,
				perms & PERM_ANA_WRITE ? 1 : 0,
				&ana_id, &context_id));

	/* Set the values in the output */
	checked(dba_record_key_seti(output, DBA_KEY_ANA_ID, ana_id));
	checked(dba_record_key_seti(output, DBA_KEY_CONTEXT_ID, context_id));

	/* Set context id and variable name on qcinput so that
	 * attribute functions will refer to what has been written */
	checked(dba_record_key_seti(qcinput, DBA_KEY_CONTEXT_ID, context_id));

	/* If there was only one variable in the input, we can pass it on as a
	 * default for attribute handling routines; otherwise we unset to mark
	 * the ambiguity */
	dba_record_cursor cur;
	dba_var var = NULL;
	if ((cur = dba_record_iterate_first(input)) != NULL &&
			dba_record_iterate_next(input, cur) == NULL)
		var = dba_record_cursor_variable(cur);
	
	if (var != NULL)
	{
		dba_varcode code = dba_var_code(var);
		char varname[8];
		snprintf(varname, 7, "B%02d%03d", DBA_VAR_X(code), DBA_VAR_Y(code));
		checked(dba_record_key_setc(qcinput, DBA_KEY_VAR_RELATED, varname));
	}
	else
		checked(dba_record_key_unset(qcinput, DBA_KEY_VAR_RELATED));
}

void DbAPI::dimenticami()
{
	if (! (perms & PERM_DATA_WRITE))
		checked(dba_error_consistency(
			"dimenticami must be called with the database open in data write mode"));

	checked(dba_db_remove(db, input));
}

int DbAPI::voglioancora()
{
	int id_context;
	dba_varcode id_var;

	/* Retrieve the ID of the data to query */
	get_referred_data_id(&id_context, &id_var);

	dba_varcode* arr = NULL;
	size_t arr_len = 0;

	try {
		/* Retrieve the varcodes of the wanted QC values */
		read_qc_list(&arr, &arr_len);

		/* Do QC query */
		int qc_count;
		checked(dba_db_qc_query(db, id_context, id_var, 
					arr == NULL ? NULL : arr,
					arr == NULL ? 0 : arr_len,
					qcoutput, &qc_count));
		qc_iter = dba_record_iterate_first(qcoutput);

		clear_qcinput();
		free(arr);

		return qc_count;
	} catch (...) {
		if (arr != NULL)
			free(arr);
		throw;
	}
}

void DbAPI::critica()
{
	if (perms & PERM_ATTR_RO)
		checked(dba_error_consistency(
			"critica cannot be called with the database open in attribute readonly mode"));

	int id_context;
	dba_varcode id_var;
	get_referred_data_id(&id_context, &id_var);

	checked(dba_db_qc_insert_or_replace(
				db, id_context, id_var, qcinput,
				perms & PERM_ATTR_WRITE ? 1 : 0));

	clear_qcinput();
}

void DbAPI::scusa()
{
	if (! (perms & PERM_ATTR_WRITE))
		checked(dba_error_consistency(
			"scusa must be called with the database open in attribute write mode"));

	int id_context;
	dba_varcode id_var;
	get_referred_data_id(&id_context, &id_var);

	dba_varcode* arr = NULL;
	size_t arr_len = 0;
	try {
		/* Retrieve the varcodes of the wanted QC values */
		read_qc_list(&arr, &arr_len);

		// If arr is still 0, then dba_qc_delete deletes all QC values
		checked(dba_db_qc_remove(
					db, id_context, id_var,
					arr == NULL ? NULL : arr,
					arr == NULL ? 0 : arr_len));

		clear_qcinput();
		free(arr);
	} catch (...) {
		if (arr != NULL)
			free(arr);
		throw;
	}
}

}

/* vim:set ts=4 sw=4: */
