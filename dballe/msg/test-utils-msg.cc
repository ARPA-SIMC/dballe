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

#include "test-utils-msg.h"

#include <dballe/msg/aof_codec.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace tut_dballe {

dba_msgs _read_test_msg(const char* file, int line, const char* filename, dba_encoding type)
{
	if (type == AOF)
	{
		// Read the sample message
		dba_rawmsg rawmsg = _read_rawmsg(file, line, filename, type);

		// Decode the sample message
		dba_msgs msgs;
		INNER_CHECKED(aof_codec_decode(rawmsg, &msgs));

		dba_rawmsg_delete(rawmsg);
		return msgs;
	} else {
		bufrex_msg bufrex = _read_test_msg_raw(file, line, filename, type);

		// Parse the decoded message into a synop
		dba_msgs msgs;
		INNER_CHECKED(bufrex_msg_to_dba_msgs(bufrex, &msgs));

		bufrex_msg_delete(bufrex);
		return msgs;
	}
}

const static dba_varcode generator_varcodes[] = {
	DBA_VAR(0,  1,   1),
	DBA_VAR(0,  1,   2),
	DBA_VAR(0,  1,   8),
	DBA_VAR(0,  1,  11),
	DBA_VAR(0,  1,  12),
	DBA_VAR(0,  1,  13),
	DBA_VAR(0,  2,   1),
	DBA_VAR(0,  2,   2),
	DBA_VAR(0,  2,   5),
	DBA_VAR(0,  2,  11),
	DBA_VAR(0,  2,  12),
	DBA_VAR(0,  2,  61),
	DBA_VAR(0,  2,  62),
	DBA_VAR(0,  2,  63),
	DBA_VAR(0,  2,  70),
	DBA_VAR(0,  4,   1),
	DBA_VAR(0,  4,   2),
	DBA_VAR(0,  4,   3),
	DBA_VAR(0,  4,   4),
	DBA_VAR(0,  4,   5),
	DBA_VAR(0,  5,   1),
	DBA_VAR(0,  6,   1),
	DBA_VAR(0,  7,   1),
	DBA_VAR(0,  7,   2),
	DBA_VAR(0,  7,  31),
	DBA_VAR(0,  8,   1),
	DBA_VAR(0,  8,   4),
	DBA_VAR(0,  8,  21),
	DBA_VAR(0, 10,   3),
	DBA_VAR(0, 10,   4),
	DBA_VAR(0, 10,  51),
	DBA_VAR(0, 10,  61),
	DBA_VAR(0, 10,  63),
	DBA_VAR(0, 10, 197),
	DBA_VAR(0, 11,   1),
	DBA_VAR(0, 11,   2),
	DBA_VAR(0, 11,   3),
	DBA_VAR(0, 11,   4),
};


dba_err msg_generator::fill_message(dba_msg msg, bool mobile)
{
	dba_record rec;
	DBA_RUN_OR_RETURN(dba_record_create(&rec));

	DBA_RUN_OR_RETURN(fill_pseudoana(rec, mobile));
	DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg,		dba_record_key_peek(rec, DBA_KEY_LAT)));
	DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg,	dba_record_key_peek(rec, DBA_KEY_LON)));
	/* DBA_RUN_OR_RETURN(dba_msg_set_name_var(msg,			dba_record_key_peek(rec, DBA_KEY_NAME))); */
	if (mobile)
	{
		DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg,	dba_record_key_peek(rec, DBA_KEY_IDENT)));
	} else {
		//DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg,	dba_record_key_peek(rec, DBA_KEY_BLOCK)));
		//DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg,	dba_record_key_peek(rec, DBA_KEY_STATION)));
	}

	DBA_RUN_OR_RETURN(fill_context(rec));
	DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg,		dba_record_key_peek(rec, DBA_KEY_YEAR)));
	DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg,	dba_record_key_peek(rec, DBA_KEY_MONTH)));
	DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg,		dba_record_key_peek(rec, DBA_KEY_DAY)));
	DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg,		dba_record_key_peek(rec, DBA_KEY_HOUR)));
	DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg,	dba_record_key_peek(rec, DBA_KEY_MIN)));

	for (int i = 0; i < rnd(4, 20); i++)
	{
		DBA_RUN_OR_RETURN(fill_context(rec));

		int ltype, l1, l2, pind, p1, p2;
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE, &ltype));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L1, &l1));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L2, &l2));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_PINDICATOR, &pind));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P1, &p1));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P2, &p2));

		dba_var var;
		DBA_RUN_OR_RETURN(dba_var_create_local(generator_varcodes[rnd(0, sizeof(generator_varcodes) / sizeof(dba_varcode))], &var));
		DBA_RUN_OR_RETURN(dba_var_setc(var, string(dba_var_info(var)->len, '8').c_str()));
		DBA_RUN_OR_RETURN(dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	}
	return dba_error_ok();
}

void track_different_msgs(dba_msg msg1, dba_msg msg2, const std::string& prefix)
{
	string fname1 = "/tmp/test-" + prefix + "1.bufr";
	string fname2 = "/tmp/test-" + prefix + "2.bufr";
	FILE* out1 = fopen(fname1.c_str(), "w");
	FILE* out2 = fopen(fname2.c_str(), "w");
	dba_msg_print(msg1, out1);
	dba_msg_print(msg2, out2);
	fclose(out1);
	fclose(out2);
	cerr << "Wrote mismatching messages to " << fname1 << " and " << fname2 << endl;
}

void track_different_msgs(dba_msgs msgs1, dba_msgs msgs2, const std::string& prefix)
{
	string fname1 = "/tmp/test-" + prefix + "1.bufr";
	string fname2 = "/tmp/test-" + prefix + "2.bufr";
	FILE* out1 = fopen(fname1.c_str(), "w");
	FILE* out2 = fopen(fname2.c_str(), "w");
	dba_msgs_print(msgs1, out1);
	dba_msgs_print(msgs2, out2);
	fclose(out1);
	fclose(out2);
	cerr << "Wrote mismatching messages to " << fname1 << " and " << fname2 << endl;
}

dba_var my_want_var(const char* file, int line, dba_msg msg, int id, const char* idname)
{
	dba_msg_datum d = dba_msg_find_by_id(msg, id);
	if (d == NULL)
	{
		std::stringstream ss;
		ss << "message does not contain the value " << idname;
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
	if (d->var == NULL)
	{
		std::stringstream ss;
		ss << "message has a NULL variable in the datum for value " << idname;
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
	return d->var;
}

dba_var my_want_var_at(const char* file, int line, dba_msg msg, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_msg_datum d = dba_msg_find(msg, code, ltype, l1, l2, pind, p1, p2);

	if (d == NULL)
	{
		char varname[10];
		snprintf(varname, 10, "B%02d%03d", DBA_VAR_X(code), DBA_VAR_Y(code));
		std::stringstream ss;
		ss << "message does not contain the value " << varname << " at "
		   << "lev(" << ltype << "," << l1 << "," << l2 << ")"
		   << "tr(" << pind << "," << p1 << "," << p2 << ")";
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
	if (d->var == NULL)
	{
		char varname[10];
		snprintf(varname, 10, "B%02d%03d", DBA_VAR_X(code), DBA_VAR_Y(code));
		std::stringstream ss;
		ss << "message has a NULL variable in the datum for value " << varname << " at "
		   << "lev(" << ltype << "," << l1 << "," << l2 << ")"
		   << "tr(" << pind << "," << p1 << "," << p2 << ")";
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
	return d->var;
}


void my_ensure_msg_undef(const char* file, int line, dba_msg msg, int id, const char* idname)
{
	dba_msg_datum d = dba_msg_find_by_id(msg, id);
	if (d != NULL && d->var != NULL && dba_var_value(d->var) != NULL)
	{
		std::stringstream ss;
		ss << "message has " << idname << " set to " << dba_var_value(d->var) << " instead of being undefined";
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
}

}

// vim:set ts=4 sw=4:
