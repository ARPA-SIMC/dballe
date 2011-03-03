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
#include <dballe/core/csv.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>

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
    "bufr/C23000.bufr",
    "bufr/tempforecast.bufr",
    "bufr/temp-2-255.bufr",
    "bufr/synop-cloudbelow.bufr",
    "bufr/synop-evapo.bufr",
    "bufr/synop-groundtemp.bufr",
    "bufr/synop-longname.bufr",
    "bufr/synop-oddgust.bufr",
    "bufr/synop-oddprec.bufr",
    "bufr/synop-old-buoy.bufr",
    "bufr/synop-strayvs.bufr",
    "bufr/synop-sunshine.bufr",
    "bufr/temp-bad1.bufr",
    "bufr/temp-bad2.bufr",
    "bufr/temp-bad3.bufr",
    "bufr/temp-bad4.bufr",
    "bufr/temp-bad5.bufr",
    "bufr/temp-gts1.bufr",
    "bufr/temp-gts2.bufr",
    "bufr/temp-gts3.bufr",
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

std::auto_ptr<Msgs> _read_msgs_csv(const Location& loc, const char* filename)
{
    std::string fname = datafile(filename);
    ifstream in(fname.c_str());
    IstreamCSVReader reader(in);

    auto_ptr<Msgs> msgs(new Msgs);
    if (!msgs->from_csv(reader))
    {
        std::stringstream ss;
        ss << "cannot find the start of CSV message in " << fname;
        throw tut::failure(loc.msg(ss.str()));
    }
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

}
}

// vim:set ts=4 sw=4:
