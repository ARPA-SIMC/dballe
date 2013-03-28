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
#include <dballe/msg/context.h>
#include <wreport/bulletin.h>
#include <wreport/conv.h>
#include <wreport/notes.h>
#include <wibble/string.h>

#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>

using namespace wibble;
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
    try {
        std::auto_ptr<Rawmsg> raw = read_rawmsg(filename, type);
        std::auto_ptr<msg::Importer> importer = msg::Importer::create(type, opts);
        std::auto_ptr<Msgs> msgs(new Msgs);
        importer->from_rawmsg(*raw, *msgs);
        return msgs;
    } catch (std::exception& e) {
        throw tut::failure(loc.msg(string("cannot read ") + filename + ": " + e.what()));
    }
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

void _export_msgs(const Location& loc, const Msgs& in, Bulletin& out, const std::string& tag, const dballe::msg::Exporter::Options& opts)
{
    try {
        Encoding type = BUFR;
        if (string(out.encoding_name()) == "CREX")
            type = CREX;
        std::auto_ptr<msg::Exporter> exporter(msg::Exporter::create(type, opts));
        exporter->to_bulletin(in, out);
    } catch (std::exception& e) {
        dballe::tests::dump("bul-" + tag, in);
        dballe::tests::dump("msg-" + tag, out);
        throw tut::failure(loc.msg("exporting to bulletin (" + tag + "): " + e.what()));
    }
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
    dump(prefix + "1", msgs1, "first message");
    dump(prefix + "2", msgs2, "second message");
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

void dump(const std::string& tag, const Msg& msg, const std::string& desc)
{
    string fname = "/tmp/" + tag + ".txt";
    FILE* out = fopen(fname.c_str(), "w");
    try {
        msg.print(out);
    } catch (std::exception& e) {
        fprintf(out, "Dump interrupted: %s\n", e.what());
    }
    fclose(out);
    cerr << desc << " saved in " << fname << endl;
}

void dump(const std::string& tag, const Msgs& msgs, const std::string& desc)
{
    string fname = "/tmp/" + tag + ".txt";
    FILE* out = fopen(fname.c_str(), "w");
    try {
        msgs.print(out);
    } catch (std::exception& e) {
        fprintf(out, "Dump interrupted: %s\n", e.what());
    }
    fclose(out);
    cerr << desc << " saved in " << fname << endl;
}

void dump(const std::string& tag, const Bulletin& bul, const std::string& desc)
{
    string fname = "/tmp/" + tag + ".txt";
    FILE* out = fopen(fname.c_str(), "w");
    try {
        bul.print(out);
    } catch (std::exception& e) {
        fprintf(out, "Dump interrupted: %s\n", e.what());
    }
    fclose(out);

    string fname1 = "/tmp/" + tag + "-st.txt";
    out = fopen(fname1.c_str(), "w");
    try {
        bul.print_structured(out);
    } catch (std::exception& e) {
        fprintf(out, "Dump interrupted: %s\n", e.what());
    }
    fclose(out);
    cerr << desc << " saved in " << fname << " and " << fname1 << endl;
}

void dump(const std::string& tag, const Rawmsg& msg, const std::string& desc)
{
    string fname = "/tmp/" + tag + ".raw";
    FILE* out = fopen(fname.c_str(), "w");
    fwrite(msg.data(), msg.size(), 1, out);
    fclose(out);
    cerr << desc << " saved in " << fname << endl;
}

void dump(const std::string& tag, const std::string& msg, const std::string& desc)
{
    string fname = "/tmp/" + tag + ".txt";
    FILE* out = fopen(fname.c_str(), "w");
    fwrite(msg.data(), msg.size(), 1, out);
    fclose(out);
    cerr << desc << " saved in " << fname << endl;
}

namespace tweaks {

void StripAttrs::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            for (vector<wreport::Var*>::iterator vi = c.data.begin(); vi != c.data.end(); ++vi)
            {
                Var& v = **vi;
                for (vector<wreport::Varcode>::const_iterator i = codes.begin();
                        i != codes.end(); ++i)
                    v.unseta(*i);
            }
        }
    }
}

StripQCAttrs::StripQCAttrs()
{
    codes.push_back(WR_VAR(0, 33, 7));
}

StripContextAttrs::StripContextAttrs()
{
    codes.push_back(WR_VAR(0, 7,  31));
    codes.push_back(WR_VAR(0, 7,  32));
    codes.push_back(WR_VAR(0, 4, 194));
}

void StripSubstituteAttrs::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            for (vector<wreport::Var*>::iterator vi = c.data.begin(); vi != c.data.end(); ++vi)
            {
                Var& v = **vi;
                v.unseta(v.code());
            }
        }
    }
}

void StripVars::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); )
        {
            msg::Context& c = **ci;
            for (vector<wreport::Varcode>::const_iterator i = codes.begin();
                    i != codes.end(); ++i)
                c.remove(*i);
            if (c.data.empty())
                ci = m.data.erase(ci);
            else
                ++ci;
        }
    }
}

RoundLegacyVars::RoundLegacyVars() : table(NULL)
{
    table = Vartable::get("B0000000000000014000");
}

void RoundLegacyVars::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            if (Var* var = c.edit(WR_VAR(0, 12, 101)))
            {
                Var var1(table->query(WR_VAR(0, 12, 1)), *var);
                var->set(var1);
            }
            if (Var* var = c.edit(WR_VAR(0, 12, 103)))
            {
                Var var1(table->query(WR_VAR(0, 12, 3)), *var);
                var->set(var1);
            }
            if (Var* var = c.edit(WR_VAR(0,  7,  30)))
            {
                Var var1(table->query(WR_VAR(0, 7, 1)), *var);
                var->set(var1);
            }
        }
    }
}

void RemoveSynopWMOOnlyVars::tweak(Msgs& msgs)
{
    int seen_tprec_trange = MISSING_INT;
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        // Remove all 'cloud drift' levels
        for (int i = 1; m.remove_context(Level::cloud(260, i), Trange::instant()); ++i)
            ;
        // Remove all 'cloud elevation' levels
        for (int i = 1; m.remove_context(Level::cloud(261, i), Trange::instant()); ++i)
            ;
        // Remove all 'cloud direction and elevation' levels
        for (int i = 1; m.remove_context(Level::cloud(262, i), Trange::instant()); ++i)
            ;
        // Remove all 'cloud below' levels
        for (int i = 1; m.remove_context(Level::cloud(263, i), Trange::instant()); ++i)
            ;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); )
        {
            msg::Context& c = **ci;
            c.remove(WR_VAR(0, 20, 62)); // State of the ground (with/without snow)
            c.remove(WR_VAR(0, 14, 31)); // Total sunshine
            c.remove(WR_VAR(0, 13, 33)); // Evaporation/evapotranspiration
            c.remove(WR_VAR(0, 12,121)); // Ground minimum temperature
            c.remove(WR_VAR(0, 11, 43)); // Maximum wind gust
            c.remove(WR_VAR(0, 11, 41)); // Maximum wind gust
            c.remove(WR_VAR(0, 10,  8)); // Geopotential
            c.remove(WR_VAR(0,  7, 31)); // Height of barometer
            c.remove(WR_VAR(0,  2,  4)); // Type of instrumentation for evaporation measurement
            c.remove(WR_VAR(0,  2,  2)); // Type of instrumentation for wind measurement
            c.remove(WR_VAR(0,  1, 19)); // Long station or site name
            if (c.find(WR_VAR(0, 13, 11)))
            {
                // Keep only one total precipitation measurement common
                // to all subsets
                if (seen_tprec_trange == MISSING_INT)
                    seen_tprec_trange = c.trange.p2;
                else if (c.trange.p2 != seen_tprec_trange)
                    c.remove(WR_VAR(0, 13, 11));
            }
            if (c.trange == Trange(4, 0, 86400))
                c.remove(WR_VAR(0, 10, 60)); // 24h pressure change
            if (c.trange.pind == 2 || c.trange.pind == 3)
                c.remove(WR_VAR(0, 12, 101)); // min and max temperature
            if (c.data.empty())
                ci = m.data.erase(ci);
            else
                ++ci;
        }
    }
}

void RemoveTempWMOOnlyVars::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); )
        {
            msg::Context& c = **ci;
            c.remove(WR_VAR(0, 22, 43)); // Sea/water temperature
            c.remove(WR_VAR(0, 11, 62)); // Absolute wind shear layer above
            c.remove(WR_VAR(0, 11, 61)); // Absolute wind shear layer below
            c.remove(WR_VAR(0,  7, 31)); // Height of barometer above MSL
            c.remove(WR_VAR(0,  7,  7)); // Height (of release above MSL)
            c.remove(WR_VAR(0,  6, 15)); // Longitude displacement
            c.remove(WR_VAR(0,  5, 15)); // Latitude displacement
            c.remove(WR_VAR(0,  4, 86)); // Long time period or displacement
            c.remove(WR_VAR(0,  4,  6)); // Second
            c.remove(WR_VAR(0,  2, 14)); // Tracking technique / status of system
            c.remove(WR_VAR(0,  2, 13)); // Solar and infrared radiation correction
            c.remove(WR_VAR(0,  2, 12)); // Radiosonde computational method
            c.remove(WR_VAR(0,  2,  3)); // Type of measuring equipment

            if (c.data.empty())
                ci = m.data.erase(ci);
            else
                ++ci;
        }
    }
}

RemoveOddTempTemplateOnlyVars::RemoveOddTempTemplateOnlyVars()
{
    codes.push_back(WR_VAR(0, 2, 12)); // Radiosonde computational method
}

void RemoveSynopWMOOddprec::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        m.remove_context(Level(1), Trange(1, 0));
    }
}

void TruncStName::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        if (msg::Context* c = m.edit_context(Level::ana(), Trange::ana()))
            if (const Var* orig = c->find(WR_VAR(0, 1, 19)))
                if (const char* val = orig->value())
                {
                    char buf[21];
                    strncpy(buf, val, 20);
                    buf[20] = 0;
                    c->set(Var(orig->info(), buf));
                }
    }
}

RoundGeopotential::RoundGeopotential()
{
    table = Vartable::get("B0000000000000014000");
}
void RoundGeopotential::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            if (Var* orig = c.edit(WR_VAR(0, 10, 8)))
            {
                // Convert to B10009 (new GTS TEMP templates)
                Var var2(table->query(WR_VAR(0, 10, 9)), *orig);
                // Convert to B10008 (used for geopotential by DB-All.e)
                Var var3(table->query(WR_VAR(0, 10, 8)), var2);
                // Convert back to B10003
                Var var4(table->query(WR_VAR(0, 10, 3)), var3);
                orig->set(var4);
            }
        }
    }
}

void RoundVSS::tweak(Msgs& msgs)
{
    for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
    {
        Msg& m = **mi;
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            if (Var* orig = c.edit(WR_VAR(0, 8, 42)))
                orig->seti(convert_BUFR08001_to_BUFR08042(convert_BUFR08042_to_BUFR08001(orig->enqi())));
        }
    }
}

}

TestMessage::TestMessage(Encoding type, const std::string& name)
    : name(name), type(type), bulletin(0)
{
    switch (type)
    {
        case BUFR: bulletin = BufrBulletin::create().release(); break;
        case CREX: bulletin = CrexBulletin::create().release(); break;
        default: throw wreport::error_unimplemented("Unsupported message type");
    }
}

TestMessage::~TestMessage()
{
    if (bulletin) delete bulletin;
}

void TestMessage::read_from_file(const std::string& fname, const msg::Importer::Options& input_opts)
{
    auto_ptr<Rawmsg> src = read_rawmsg(fname.c_str(), type);
    read_from_raw(*src, input_opts);
}

void TestMessage::read_from_raw(const Rawmsg& msg, const msg::Importer::Options& input_opts)
{
    std::auto_ptr<msg::Importer> importer(msg::Importer::create(type, input_opts));
    raw = msg;
    bulletin->decode(raw);
    importer->from_rawmsg(raw, msgs);
}

void TestMessage::read_from_msgs(const Msgs& _msgs, const msg::Exporter::Options& export_opts)
{
    // Export
    std::auto_ptr<msg::Exporter> exporter(msg::Exporter::create(type, export_opts));

    msgs = _msgs;
    exporter->to_bulletin(msgs, *bulletin);
    bulletin->encode(raw);
}

TestCodec::TestCodec(const std::string& fname, Encoding type)
    : fname(fname), type(type), verbose(false), expected_subsets(1), expected_min_vars(1)
{
}

void TestCodec::do_compare(const dballe::tests::Location& loc, const TestMessage& msg1, const TestMessage& msg2)
{
    // Compare msgs
    {
        stringstream str;
        notes::Collect c(str);
        int diffs = msg1.msgs.diff(msg2.msgs);
        if (diffs)
        {
            dballe::tests::dump("msg1", msg1.msgs);
            dballe::tests::dump("msg2", msg2.msgs);
            dballe::tests::dump("msg", msg2.raw);
            dballe::tests::dump("diffs", str.str(), "details of differences");
            throw tut::failure(loc.msg(str::fmtf("found %d differences", diffs)));
        }
    }

    // Compare bulletins
    try {
        if (msg2.bulletin->subsets.size() != (unsigned)expected_subsets)
            throw tut::failure(loc.msg(str::fmtf("Number of subsets differ from expected: %zd != %d\n", msg2.bulletin->subsets.size(), expected_subsets)));
        if (msg2.bulletin->subsets[0].size() < (unsigned)expected_min_vars)
            throw tut::failure(loc.msg(str::fmtf("Number of items in first subset is too small: %zd < %d\n", msg2.bulletin->subsets[0].size(), expected_min_vars)));
    } catch (...) {
        dballe::tests::dump("bull1", *msg1.bulletin);
        dballe::tests::dump("bull2", *msg2.bulletin);
        dballe::tests::dump("msg", msg2.raw);
        throw;
    }
}

void TestCodec::run_reimport(const dballe::tests::Location& loc)
{
    if (verbose) cerr << "Running test " << loc.locstr() << endl;

    // Import
    if (verbose) cerr << "Importing " << fname << " " << input_opts.to_string() << endl;
    TestMessage orig(type, "orig");
    try {
        orig.read_from_file(fname, input_opts);
    } catch (std::exception& e) {
        throw tut::failure(loc.msg(string("cannot decode ") + fname + ": " + e.what()));
    }

    inner_ensure(orig.msgs.size() > 0);

#if 0
    // Run tweaks
    for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
    {
        if (verbose) cerr << "Running tweak " << (*i)->desc() << endl;
        (*i)->tweak(*msgs1);
    }
    if (do_ecmwf_tweaks)
        for (typename vector<Tweaker*>::iterator i = ecmwf_tweaks.begin(); i != ecmwf_tweaks.end(); ++i)
        {
            if (verbose) cerr << "Running ecmwf tweak " << (*i)->desc() << endl;
            (*i)->tweak(*msgs1);
        }
    if (do_wmo_tweaks)
        for (typename vector<Tweaker*>::iterator i = wmo_tweaks.begin(); i != wmo_tweaks.end(); ++i)
        {
            if (verbose) cerr << "Running wmo tweak " << (*i)->desc() << endl;
            (*i)->tweak(*msgs1);
        }
#endif

    // Export
    if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
    TestMessage exported(type, "exported");
    try {
        exported.read_from_msgs(orig.msgs, output_opts);
    } catch (std::exception& e) {
        //dballe::tests::dump("bul1", *exported);
        //balle::tests::dump("msg1", *msgs1);
        throw tut::failure(loc.msg(string("cannot export: ") + e.what()));
    }

    // Import again
    TestMessage final(type, "final");
    try {
        final.read_from_raw(exported.raw, input_opts);
    } catch (std::exception& e) {
        //dballe::tests::dump("msg1", *msgs1);
        //dballe::tests::dump("msg", rawmsg);
        throw tut::failure(loc.msg(string("importing from exported rawmsg: ") + e.what()));
    }

#if 0
    if (do_ignore_context_attrs)
    {
        StripContextAttrs sca;
        sca.tweak(*msgs1);
    }
    if (do_round_geopotential)
    {
        RoundGeopotential rg;
        rg.tweak(*msgs1);
        rg.tweak(*msgs3);
    }
#endif
    do_compare(loc, orig, final);
}

void TestCodec::run_convert(const dballe::tests::Location& loc, const std::string& tplname)
{
    if (verbose) cerr << "Running test " << loc.locstr() << endl;

    // Import
    if (verbose) cerr << "Importing " << fname << " " << input_opts.to_string() << endl;
    TestMessage orig(type, "orig");
    try {
        orig.read_from_file(fname, input_opts);
    } catch (std::exception& e) {
        throw tut::failure(loc.msg(string("cannot decode ") + fname + ": " + e.what()));
    }

    inner_ensure(orig.msgs.size() > 0);

#if 0
    // Run tweaks
    for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
    {
        if (verbose) cerr << "Running tweak " << (*i)->desc() << endl;
        (*i)->tweak(*msgs1);
    }
    if (do_ecmwf_tweaks)
        for (typename vector<Tweaker*>::iterator i = ecmwf_tweaks.begin(); i != ecmwf_tweaks.end(); ++i)
        {
            if (verbose) cerr << "Running ecmwf tweak " << (*i)->desc() << endl;
            (*i)->tweak(*msgs1);
        }
    if (do_wmo_tweaks)
        for (typename vector<Tweaker*>::iterator i = wmo_tweaks.begin(); i != wmo_tweaks.end(); ++i)
        {
            if (verbose) cerr << "Running wmo tweak " << (*i)->desc() << endl;
            (*i)->tweak(*msgs1);
        }
#endif

    // Export
    if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
    msg::Exporter::Options output_opts = this->output_opts;
    output_opts.template_name = tplname;
    TestMessage exported(type, "exported");
    try {
        exported.read_from_msgs(orig.msgs, output_opts);
    } catch (std::exception& e) {
        //dballe::tests::dump("bul1", *exported);
        //balle::tests::dump("msg1", *msgs1);
        throw tut::failure(loc.msg(string("cannot export: ") + e.what()));
    }

    // Import again
    TestMessage final(type, "final");
    try {
        final.read_from_raw(exported.raw, input_opts);
    } catch (std::exception& e) {
        //dballe::tests::dump("msg1", *msgs1);
        //dballe::tests::dump("msg", rawmsg);
        throw tut::failure(loc.msg(string("importing from exported rawmsg: ") + e.what()));
    }

#if 0
    if (do_ignore_context_attrs)
    {
        StripContextAttrs sca;
        sca.tweak(*msgs1);
    }
    if (do_round_geopotential)
    {
        RoundGeopotential rg;
        rg.tweak(*msgs1);
        rg.tweak(*msgs3);
    }
#endif
    do_compare(loc, orig, final);
}

}
}

// vim:set ts=4 sw=4:
