/*
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

using namespace wreport;
using namespace std;

namespace dballe {
namespace tests {

const char* bufr_files[] = {
    "bufr/obs0-1.22.bufr", 
    "bufr/obs0-1.11188.bufr",
    "bufr/obs0-3.504.bufr", 
    "bufr/obs1-9.2.bufr", 
    "bufr/obs1-11.16.bufr", 
    "bufr/obs1-13.36.bufr", 
    "bufr/obs1-19.3.bufr", 
    "bufr/synop-old-buoy.bufr", 
    "bufr/obs1-140.454.bufr", 
    "bufr/obs2-101.16.bufr", 
    "bufr/obs2-102.1.bufr", 
    "bufr/obs2-91.2.bufr", 
//      "bufr/obs3-3.1.bufr",
//      "bufr/obs3-56.2.bufr",
    "bufr/airep-old-4-142.bufr", 
    "bufr/obs4-142.1.bufr", 
    "bufr/obs4-144.4.bufr", 
    "bufr/obs4-145.4.bufr", 
    "bufr/obs255-255.0.bufr", 
    "bufr/synop3new.bufr", 
    "bufr/test-airep1.bufr",
    "bufr/test-temp1.bufr", 
//      "bufr/test-buoy1.bufr", 
//      "bufr/test-soil1.bufr", 
    "bufr/ed4.bufr", 
    "bufr/ed4-compr-string.bufr",
    "bufr/ed4-parseerror1.bufr",
    "bufr/ed4-empty.bufr",
    "bufr/C05060.bufr",
    "bufr/tempforecast.bufr",
    "bufr/temp-2-255.bufr",
    NULL
};

const char* crex_files[] = {
    "crex/test-mare0.crex",
    "crex/test-mare1.crex",
    "crex/test-mare2.crex",
    "crex/test-synop0.crex",
    "crex/test-synop1.crex",
    "crex/test-synop2.crex",
    "crex/test-synop3.crex",
    "crex/test-temp0.crex",
    NULL
};

const char* aof_files[] = {
	"aof/obs1-11.0.aof",
	"aof/obs1-14.63.aof",
	"aof/obs1-21.1.aof",
	"aof/obs1-24.2104.aof",
	"aof/obs1-24.34.aof",
	"aof/obs2-144.2198.aof",
	"aof/obs2-244.0.aof",
	"aof/obs2-244.1.aof",
	"aof/obs4-165.2027.aof",
	"aof/obs5-35.61.aof",
	"aof/obs5-36.30.aof",
	"aof/obs6-32.1573.aof",
	"aof/obs6-32.0.aof",
	"aof/aof_27-2-144.aof",
	"aof/aof_28-2-144.aof",
	"aof/aof_27-2-244.aof",
	"aof/aof_28-2-244.aof",
	"aof/missing-cloud-h.aof",
	"aof/brokenamdar.aof",
	"aof/aof-undersealevel.aof",
	NULL,
};

auto_ptr<Msgs> _read_msgs(const wibble::tests::Location& loc, const char* filename, Encoding type, const msg::Importer::Options& opts)
{
    std::auto_ptr<Rawmsg> raw = read_rawmsg(filename, type);
    std::auto_ptr<msg::Importer> importer = msg::Importer::create(type, opts);
    std::auto_ptr<Msgs> msgs(new Msgs);
    importer->from_rawmsg(*raw, *msgs);
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

void _ensure_msg_undef(const wibble::tests::Location& loc, const Msg& msg, int shortcut)
{
	const Var* var = msg.find_by_id(shortcut);
	if (var && var->value())
	{
		std::stringstream ss;
		ss << "value is " << var->value() << " instead of being undefined";
		throw tut::failure(loc.msg(ss.str()));
	}
}

const Var& _want_var(const Location& loc, const Msg& msg, int shortcut)
{
	const Var* var = msg.find_by_id(shortcut);
	if (!var)
		throw tut::failure(loc.msg("value is missing"));
	if (!var->value())
		throw tut::failure(loc.msg("value is present but undefined"));
	return *var;
}

const Var& _want_var(const Location& loc, const Msg& msg, wreport::Varcode code, const dballe::Level& lev, const dballe::Trange& tr)
{
	const Var* var = msg.find(code, lev, tr);
	if (!var)
		throw tut::failure(loc.msg("value is missing"));
	if (!var->value())
		throw tut::failure(loc.msg("value is present but undefined"));
	return *var;
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



#endif

}
}

// vim:set ts=4 sw=4:
