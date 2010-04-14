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
#include <dballe/bufrex/msg.h>
#include <dballe/msg/file.h>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <strings.h>


using namespace std;

namespace dballef {


MsgAPI::MsgAPI(const char* fname, const char* mode, const char* type)
	: file(0), state(0), msgs(0), wmsg(0), wvar(0), curmsgidx(0), iter_l(-1), iter_d(-1),
		cached_cat(0), cached_subcat(0), cached_lcat(0)
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

	if (strchr(mode, 'r') != NULL)
		readNextMessage();
}

MsgAPI::~MsgAPI()
{
	if (perms & (PERM_DATA_WRITE | PERM_DATA_ADD))
	{
		if (wmsg)
			flushSubset();
		if (msgs)
			flushMessage();
	} else {
		if (wmsg)
			dba_msg_delete(wmsg);
		if (msgs)
			dba_msgs_delete(msgs);
	}
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
	checked(dba_record_key_setc(output, DBA_KEY_REP_MEMO, dba_msg_repmemo_from_type(msg->type)));

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
			case DBA_VAR(0, 1, 194): checked(dba_record_key_set(output, DBA_KEY_REP_MEMO, d->var)); break;
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
	// No not skip the pseudoana layer
	// if (level->ltype1 == 257)
	// {
	// 	++iter_l;
	// 	iter_d = 0;
	// }
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
		//if (level->ltype1 == 257) continue;
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
		checked(dba_record_key_setc(output, DBA_KEY_REP_MEMO, dba_msg_repmemo_from_type(msg->type)));

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
				case DBA_VAR(0, 1, 194): checked(dba_record_key_set(output, DBA_KEY_REP_MEMO, d->var)); break;
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

void MsgAPI::flushSubset()
{
	if (wmsg)
	{
		checked(dba_msgs_append_acquire(msgs, wmsg));
		wmsg = 0;
		wvar = 0;
	}
}

void MsgAPI::flushMessage()
{
	if (msgs)
	{
		flushSubset();
		checked(dba_file_write_msgs(file, msgs, cached_cat, cached_subcat, cached_lcat));
		dba_msgs_delete(msgs);
		msgs = 0;
	}
}

void MsgAPI::prendilo()
{
	if (perms & PERM_DATA_RO)
		checked(dba_error_consistency(
			"prendilo cannot be called with the file open in read mode"));

	if (!msgs)
		checked(dba_msgs_create(&msgs));
	if (!wmsg)
		checked(dba_msg_create(&wmsg));

	// Store record metainfo
	const char* sval;
	int ival, found;
	double dval;
	checked(dba_record_key_enqc(input, DBA_KEY_REP_MEMO, &sval));
	if (sval) 
	{
		checked(dba_msg_set_rep_memo(wmsg, sval, -1));
		wmsg->type = dba_msg_type_from_repmemo(sval);
	}
	checked(dba_record_key_enqi(input, DBA_KEY_ANA_ID, &ival, &found));
	if (found) checked(dba_msg_seti(wmsg, DBA_VAR(0, 1, 192), ival, -1, 257, 0, 0, 0, 0, 0, 0));
	checked(dba_record_key_enqc(input, DBA_KEY_IDENT, &sval));
	if (sval) checked(dba_msg_set_ident(wmsg, sval, -1));
	checked(dba_record_key_enqd(input, DBA_KEY_LAT, &dval, &found));
	if (found) checked(dba_msg_set_latitude(wmsg, dval, -1));
	checked(dba_record_key_enqd(input, DBA_KEY_LON, &dval, &found));
	if (found) checked(dba_msg_set_longitude(wmsg, dval, -1));
	checked(dba_record_key_enqi(input, DBA_KEY_YEAR, &ival, &found));
	if (found) checked(dba_msg_set_year(wmsg, ival, -1));
	checked(dba_record_key_enqi(input, DBA_KEY_MONTH, &ival, &found));
	if (found) checked(dba_msg_set_month(wmsg, ival, -1));
	checked(dba_record_key_enqi(input, DBA_KEY_DAY, &ival, &found));
	if (found) checked(dba_msg_set_day(wmsg, ival, -1));
	checked(dba_record_key_enqi(input, DBA_KEY_HOUR, &ival, &found));
	if (found) checked(dba_msg_set_hour(wmsg, ival, -1));
	checked(dba_record_key_enqi(input, DBA_KEY_MIN, &ival, &found));
	if (found) checked(dba_msg_set_minute(wmsg, ival, -1));
	checked(dba_record_key_enqi(input, DBA_KEY_SEC, &ival, &found));
	if (found) checked(dba_msg_set_second(wmsg, ival, -1));

	if (dba_record_cursor c = dba_record_iterate_first(input))
	{
		int ltype1, l1, ltype2, l2, pind, p1, p2;
		checked(dba_record_key_enqi(input, DBA_KEY_LEVELTYPE1, &ltype1, &found));
		if (!found) checked(dba_error_consistency("leveltype1 is not set"));
		checked(dba_record_key_enqi(input, DBA_KEY_L1, &l1, &found));
		if (!found) checked(dba_error_consistency("l1 is not set"));
		checked(dba_record_key_enqi(input, DBA_KEY_LEVELTYPE2, &ltype2, &found));
		if (!found) ltype2 = 0;
		checked(dba_record_key_enqi(input, DBA_KEY_L2, &l2, &found));
		if (!found) l2 = 0;
		checked(dba_record_key_enqi(input, DBA_KEY_PINDICATOR, &pind, &found));
		if (!found) checked(dba_error_consistency("pindicator is not set"));
		checked(dba_record_key_enqi(input, DBA_KEY_P1, &p1, &found));
		if (!found) checked(dba_error_consistency("p1 is not set"));
		checked(dba_record_key_enqi(input, DBA_KEY_P2, &p2, &found));
		if (!found) checked(dba_error_consistency("p2 is not set"));
		
		for ( ; c; c = dba_record_iterate_next(input, c))
		{
			dba_var v;
			checked(dba_var_copy(dba_record_cursor_variable(c), &v));
			// FIXME: if the next one throws, we leak v
			checked(dba_msg_set_nocopy(wmsg, v, ltype1, l1, ltype2, l2, pind, p1, p2));
			if (last_set_code != 0)
			{
				if (dba_var_code(v) == last_set_code)
					wvar = v;
			}
			else
				wvar = v;
		}
	}

	const char* query;
	checked(dba_record_key_enqc(input, DBA_KEY_QUERY, &query));
	if (query != NULL)
	{
		if (strcasecmp(query, "subset") == 0)
		{
			flushSubset();
		} else if (strncasecmp(query, "message", 7) == 0) {
			// Check that message is followed by spaces or end of string
			const char* s = query + 7;
			if (*s != 0 && !isblank(*s))
				checked(dba_error_consistency("Query type \"%s\" is not among the supported values", query));
			// Skip the spaces after message
			while (*s != 0 && isblank(*s))
				++s;

			if (*s)
				// If a template is specified, open a new message with that template
				checked(bufrex_msg_parse_template(s, &cached_cat, &cached_subcat, &cached_lcat));
			else
				// Else, open a new message with template guessing
				cached_cat = cached_subcat = cached_lcat = 0;

			flushMessage();
		} else
			checked(dba_error_consistency("Query type \"%s\" is not among the supported values", query));

		// Uset query after using it: it needs to be explicitly set every time
		checked(dba_record_key_unset(input, DBA_KEY_QUERY));
	}
}

void MsgAPI::dimenticami()
{
	checked(dba_error_consistency(
		"dimenticami does not make sense when writing messages"));
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
	if (perms & PERM_ATTR_RO)
		checked(dba_error_consistency(
			"critica cannot be called with the database open in attribute readonly mode"));
	if (wvar == 0)
		checked(dba_error_consistency(
			"critica has been called without a previous prendilo"));

	for (dba_record_cursor c = dba_record_iterate_first(qcinput);
			c; c = dba_record_iterate_next(qcinput, c))
		checked(dba_var_seta(wvar, dba_record_cursor_variable(c)));

	dba_record_clear(qcinput);
}

void MsgAPI::scusa()
{
	checked(dba_error_consistency(
		"scusa does not make sense when writing messages"));
}

}

/* vim:set ts=4 sw=4: */
