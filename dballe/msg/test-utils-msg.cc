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

#include "test-utils-msg.h"
#include "codec.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace std;

namespace dballe {
namespace tests {

auto_ptr<Msgs> _read_msgs(const wibble::tests::Location& loc, const char* filename, Encoding type)
{
    std::auto_ptr<Rawmsg> raw = read_rawmsg(filename, type);
    std::auto_ptr<msg::Importer> importer = msg::Importer::create(type);
    std::auto_ptr<Msgs> msgs(new Msgs);
    importer->import(*raw, *msgs);
    return msgs;
}

void track_different_msgs(const Msg& msg1, const Msg& msg2, const std::string& prefix)
{
	string fname1 = "/tmp/test-" + prefix + "1.bufr";
	string fname2 = "/tmp/test-" + prefix + "2.bufr";
	FILE* out1 = fopen(fname1.c_str(), "w");
	FILE* out2 = fopen(fname2.c_str(), "w");
	msg1.print(out1);
	msg2.print(out2);
	fclose(out1);
	fclose(out2);
	cerr << "Wrote mismatching messages to " << fname1 << " and " << fname2 << endl;
}

void track_different_msgs(const Msgs& msgs1, const Msgs& msgs2, const std::string& prefix)
{
	string fname1 = "/tmp/test-" + prefix + "1.bufr";
	string fname2 = "/tmp/test-" + prefix + "2.bufr";
	FILE* out1 = fopen(fname1.c_str(), "w");
	FILE* out2 = fopen(fname2.c_str(), "w");
	msgs1.print(out1);
	msgs2.print(out2);
	fclose(out1);
	fclose(out2);
	cerr << "Wrote mismatching messages to " << fname1 << " and " << fname2 << endl;
}

#if 0
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
	DBA_VAR(0, 10,  60),
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

		int found;
		int ltype1, l1, ltype2, l2, pind, p1, p2;
		// Since I just filled, I'm sure that the values are there
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE1, &ltype1, &found));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L1, &l1, &found));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE2, &ltype2, &found));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L2, &l2, &found));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_PINDICATOR, &pind, &found));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P1, &p1, &found));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P2, &p2, &found));

		dba_var var;
		DBA_RUN_OR_RETURN(dba_var_create_local(generator_varcodes[rnd(0, sizeof(generator_varcodes) / sizeof(dba_varcode))], &var));
		DBA_RUN_OR_RETURN(dba_var_setc(var, string(dba_var_info(var)->len, '8').c_str()));
		DBA_RUN_OR_RETURN(dba_msg_set_nocopy(msg, var, ltype1, l1, ltype2, l2, pind, p1, p2));
	}
	return dba_error_ok();
}


dba_var my_want_var(const char* file, int line, dba_msg msg, int id, const char* idname)
{
	dba_var var = dba_msg_find_by_id(msg, id);
	if (var == NULL)
	{
		std::stringstream ss;
		ss << "message does not contain the value " << idname;
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
	return var;
}

dba_var my_want_var_at(const char* file, int line, dba_msg msg, dba_varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
{
	dba_var var = dba_msg_find(msg, code, ltype1, l1, ltype2, l2, pind, p1, p2);

	if (var == NULL)
	{
		char varname[10];
		snprintf(varname, 10, "B%02d%03d", DBA_VAR_X(code), DBA_VAR_Y(code));
		std::stringstream ss;
		ss << "message does not contain the value " << varname << " at "
		   << "lev(" << ltype1 << "," << l1 << "," << ltype2 << "," << l2 << ")"
		   << "tr(" << pind << "," << p1 << "," << p2 << ")";
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
	return var;
}


void my_ensure_msg_undef(const char* file, int line, dba_msg msg, int id, const char* idname)
{
	dba_var var = dba_msg_find_by_id(msg, id);
	if (var != NULL && dba_var_value(var) != NULL)
	{
		std::stringstream ss;
		ss << "message has " << idname << " set to " << dba_var_value(var) << " instead of being undefined";
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
}
#endif

}
}

// vim:set ts=4 sw=4:
