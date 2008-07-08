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

#include "msgapi.h"
#include <dballe/core/aliases.h>
#include <dballe/core/verbose.h>
#include <dballe/msg/file.h>
#include <cstdlib>
#include <cstring>
#include <strings.h>


using namespace std;

namespace dballef {


MsgAPI::MsgAPI(const char* fname, const char* mode, const char* type)
	: file(0), state(0), msgs(0), curmsgidx(0), iter_l(-1), iter_d(-1)
{
	if (strchr(mode, 'r') != NULL)
	{
		set_permissions("read", "read", "read");
	} else if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL) {
		set_permissions("write", "add", "add");
	}
	dba_encoding etype = (dba_encoding)-1;
	if (strcasecmp(type, "BUFR") == 0)
		etype = BUFR;
	else if (strcasecmp(type, "CREX") == 0)
		etype = CREX;
	else if (strcasecmp(type, "AOF") == 0)
		etype = AOF;
	else if (strcasecmp(type, "AUTO") == 0)
		etype = (dba_encoding)-1;
	else
		checked(dba_error_consistency("\"%s\" is not one of the supported message types", type));

	checked(dba_file_create(etype, fname, mode, &file));

	readNextMessage();
}

MsgAPI::~MsgAPI()
{
	if (file)
		dba_file_delete(file);
}

dba_msg MsgAPI::curmsg()
{
	if (msgs && curmsgidx < msgs->len)
		return msgs->msgs[curmsgidx];
	else
		return NULL;
}

bool MsgAPI::readNextMessage()
{
	if (state & STATE_EOF)
		return false;

	int found;
	state = 0;
	curmsgidx = 0;
	if (msgs)
	{
		dba_msgs_delete(msgs);
		msgs = 0;
	}
	checked(dba_file_read_msgs(file, &msgs, &found));
	if (found)
		return true;

	state |= STATE_EOF;
	return false;
}

void MsgAPI::scopa(const char* repinfofile)
{
	if (!(perms & PERM_DATA_WRITE))
		checked(dba_error_consistency(
			"scopa must be run with the database open in data write mode"));

	// FIXME: In theory, nothing to do
	// FIXME: In practice, we could reset all buffered data and ftruncate the file
}

int MsgAPI::quantesono()
{
	if (state & STATE_QUANTESONO)
		readNextMessage();
	if (state & STATE_EOF)
		return 0;
	state |= STATE_QUANTESONO;
		
	return 1;
}

void MsgAPI::elencamele()
{
	if ((state & STATE_QUANTESONO) == 0)
		checked(dba_error_consistency("elencamele called without a previous quantesono"));

	dba_record_clear(output);

	dba_msg msg = curmsg();
	if (!msg) return;

	dba_msg_level level = dba_msg_find_level(msg, 257, 0, 0, 0);
	if (!level) return;

	checked(dba_record_set_ana_context(output));
	checked(dba_record_key_seti(output, DBA_KEY_MOBILE, 0));
	checked(dba_record_key_seti(output, DBA_KEY_REP_COD, dba_msg_repcod_from_type(msg->type)));

	for (int l = 0; l < level->data_count; ++l)
	{
		dba_msg_datum d = level->data[l];
		if (!d->var) continue;
		switch (dba_var_code(d->var))
		{
			case DBA_VAR(0, 5,   1): checked(dba_record_key_set(output, DBA_KEY_LAT, d->var)); break;
			case DBA_VAR(0, 6,   1): checked(dba_record_key_set(output, DBA_KEY_LON, d->var)); break;
			case DBA_VAR(0, 1,  11):
				checked(dba_record_key_set(output, DBA_KEY_IDENT, d->var));
				checked(dba_record_key_seti(output, DBA_KEY_MOBILE, 1));
				break;
			case DBA_VAR(0, 1, 192): checked(dba_record_key_set(output, DBA_KEY_ANA_ID, d->var)); break;
			default:
				checked(dba_record_var_set_direct(output, d->var));
		}
	}
}

bool MsgAPI::incrementMsgIters()
{
	if (iter_l == -1)
	{
		iter_l = 0;
		iter_d = -1;
	}

	dba_msg msg = curmsg();
	if (iter_l >= msg->data_count)
		return false;

	dba_msg_level level = msg->data[iter_l];
	if (iter_d < level->data_count - 1)
	{
		++iter_d;
	} else {
		++iter_l;
		iter_d = 0;
	}
	// Skip the pseudoana layer
	if (level->ltype1 == 257)
	{
		++iter_l;
		iter_d = 0;
	}
	if (iter_l >= msg->data_count)
		return false;

	return true;
}

int MsgAPI::voglioquesto()
{
	if (state & STATE_VOGLIOQUESTO)
		readNextMessage();
	if (state & STATE_EOF)
		return 0;
	state |= STATE_VOGLIOQUESTO;
		
	iter_l = iter_d = -1;

	dba_msg msg = curmsg();
	if (!msg) return 0;

	int count = 0;
	for (int l = 0; l < msg->data_count; ++l)
	{
		dba_msg_level level = msg->data[l];
		if (level->ltype1 == 257) continue;
		count += level->data_count;
	}
	return count;
}

const char* MsgAPI::dammelo()
{
	if ((state & STATE_VOGLIOQUESTO) == 0)
		checked(dba_error_consistency("dammelo called without a previous voglioquesto"));

	dba_record_clear(output);

	dba_msg msg = curmsg();
	if (!msg) return 0;

	if (!incrementMsgIters())
		return 0;

	// Set metainfo from msg ana layer
	if (dba_msg_level level = dba_msg_find_level(msg, 257, 0, 0, 0))
	{
		checked(dba_record_key_seti(output, DBA_KEY_MOBILE, 0));
		checked(dba_record_key_seti(output, DBA_KEY_REP_COD, dba_msg_repcod_from_type(msg->type)));

		for (int l = 0; l < level->data_count; ++l)
		{
			dba_msg_datum d = level->data[l];
			if (!d->var) continue;
			switch (dba_var_code(d->var))
			{
				case DBA_VAR(0, 5,   1): checked(dba_record_key_set(output, DBA_KEY_LAT, d->var)); break;
				case DBA_VAR(0, 6,   1): checked(dba_record_key_set(output, DBA_KEY_LON, d->var)); break;
				case DBA_VAR(0, 4,   1): checked(dba_record_key_set(output, DBA_KEY_YEAR, d->var)); break;
				case DBA_VAR(0, 4,   2): checked(dba_record_key_set(output, DBA_KEY_MONTH, d->var)); break;
				case DBA_VAR(0, 4,   3): checked(dba_record_key_set(output, DBA_KEY_DAY, d->var)); break;
				case DBA_VAR(0, 4,   4): checked(dba_record_key_set(output, DBA_KEY_HOUR, d->var)); break;
				case DBA_VAR(0, 4,   5): checked(dba_record_key_set(output, DBA_KEY_MIN, d->var)); break;
				case DBA_VAR(0, 4,   6): checked(dba_record_key_set(output, DBA_KEY_SEC, d->var)); break;
				case DBA_VAR(0, 1,  11):
					checked(dba_record_key_set(output, DBA_KEY_IDENT, d->var));
					checked(dba_record_key_seti(output, DBA_KEY_MOBILE, 1));
					break;
				case DBA_VAR(0, 1, 192): checked(dba_record_key_set(output, DBA_KEY_ANA_ID, d->var)); break;
				default:
					checked(dba_record_var_set_direct(output, d->var));
			}
		}
	}

	dba_msg_level level = msg->data[iter_l];
	checked(dba_record_key_seti(output, DBA_KEY_LEVELTYPE1, level->ltype1));
	checked(dba_record_key_seti(output, DBA_KEY_L1, level->l1));
	checked(dba_record_key_seti(output, DBA_KEY_LEVELTYPE2, level->ltype2));
	checked(dba_record_key_seti(output, DBA_KEY_L2, level->l2));

	dba_msg_datum datum = level->data[iter_d];
	checked(dba_record_key_seti(output, DBA_KEY_PINDICATOR, datum->pind));
	checked(dba_record_key_seti(output, DBA_KEY_P1, datum->p1));
	checked(dba_record_key_seti(output, DBA_KEY_P2, datum->p2));

	char vname[10];
	dba_varcode code = dba_var_code(datum->var);
	snprintf(vname, 10, "B%02d%03d", DBA_VAR_X(code), DBA_VAR_Y(code));
	checked(dba_record_key_setc(output, DBA_KEY_VAR, vname));
	const char* res;
	checked(dba_record_key_enqc(output, DBA_KEY_VAR, &res));

	checked(dba_record_var_set_direct(output, datum->var));

	return res;
}

void MsgAPI::prendilo()
{
#if 0
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
#endif
}

void MsgAPI::dimenticami()
{
#if 0
	if (! (perms & PERM_DATA_WRITE))
		checked(dba_error_consistency(
			"dimenticami must be called with the database open in data write mode"));

	checked(dba_db_remove(db, input));
#endif
}

int MsgAPI::voglioancora()
{
	dba_msg msg = curmsg();
	if (msg == 0 || iter_l == -1 || iter_d == -1)
		checked(dba_error_consistency("voglioancora called before dammelo"));

	if (iter_l >= msg->data_count) return 0;
	dba_msg_level level = msg->data[iter_l];

	if (iter_d >= level->data_count) return 0;
	dba_var var = level->data[iter_d]->var;
	if (!var) return 0;

	dba_record_clear(qcoutput);

	int count = 0;
	for (dba_var_attr_iterator i = dba_var_attr_iterate(var); i; i = dba_var_attr_iterator_next(i))
	{
		dba_record_var_set_direct(qcoutput, dba_var_attr_iterator_attr(i));
		++count;
	}

	qc_iter = dba_record_iterate_first(qcoutput);

	return count;
}

void MsgAPI::critica()
{
#if 0
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
#endif
}

void MsgAPI::scusa()
{
#if 0
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
#endif
}

}

/* vim:set ts=4 sw=4: */
