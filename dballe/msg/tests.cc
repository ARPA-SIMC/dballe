#include "tests.h"
#include "codec.h"
#include "wr_codec.h"
#include <dballe/core/csv.h>
#include "context.h"
#include "msg.h"
#include <wreport/bulletin.h>
#include <wreport/vartable.h>
#include <wreport/tableinfo.h>
#include <wreport/conv.h>
#include <wreport/notes.h>
#include <cstring>
#include <cmath>
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

Messages read_msgs(const char* filename, File::Encoding type, const msg::ImporterOptions& opts)
{
    BinaryMessage raw = wcallchecked(read_rawmsg(filename, type));
    std::unique_ptr<msg::Importer> importer = msg::Importer::create(type, opts);
    return importer->from_binary(raw);
}

Messages read_msgs_csv(const char* filename)
{
    std::string fname = datafile(filename);
    CSVReader reader(fname);

    Messages msgs = msg::messages_from_csv(reader);
    if (msgs.empty())
    {
        std::stringstream ss;
        ss << "cannot find the start of CSV message in " << fname;
        throw TestFailed(ss.str());
    }
    return msgs;
}

unique_ptr<Bulletin> export_msgs(File::Encoding enctype, const Messages& in, const std::string& tag, const dballe::msg::ExporterOptions& opts)
{
    try {
        std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(enctype, opts));
        return exporter->to_bulletin(in);
    } catch (std::exception& e) {
        dballe::tests::dump("bul-" + tag, in);
        //dballe::tests::dump("msg-" + tag, out);
        throw TestFailed("cannot export to bulletin (" + tag + "): " + e.what());
    }
}

void track_different_msgs(const Message& msg1, const Message& msg2, const std::string& prefix)
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

void track_different_msgs(const Messages& msgs1, const Messages& msgs2, const std::string& prefix)
{
    dump(prefix + "1", msgs1, "first message");
    dump(prefix + "2", msgs2, "second message");
}

void ActualMessage::is_undef(int shortcut) const
{
    const Var* var = Msg::downcast(_actual).find_by_id(shortcut);
    if (!var || !var->isset()) return;
    std::stringstream ss;
    ss << "value is " << var->enqc() << " instead of being undefined";
    throw TestFailed(ss.str());
}

const Var& want_var(const Message& msg, int shortcut)
{
    const Var* var = Msg::downcast(msg).find_by_id(shortcut);
    if (!var)
        throw TestFailed("value is missing");
    if (!var->isset())
        throw TestFailed("value is present but undefined");
    return *var;
}

const Var& want_var(const Message& msg, wreport::Varcode code, const dballe::Level& lev, const dballe::Trange& tr)
{
    const Var* var = msg.get(code, lev, tr);
    if (!var)
        throw TestFailed("value is missing");
    if (!var->isset())
        throw TestFailed("value is present but undefined");
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

void dump(const std::string& tag, const Messages& msgs, const std::string& desc)
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

void dump(const std::string& tag, const BinaryMessage& msg, const std::string& desc)
{
    string fname = "/tmp/" + tag + ".raw";
    FILE* out = fopen(fname.c_str(), "w");
    fwrite(msg.data.data(), msg.data.size(), 1, out);
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

MessageTweakers::~MessageTweakers()
{
    for (vector<MessageTweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
        delete *i;
}

void MessageTweakers::add(MessageTweaker* tweak)
{
    tweaks.push_back(tweak);
}

void MessageTweakers::apply(Messages& msgs)
{
    for (vector<MessageTweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
        (*i)->tweak(msgs);
}

namespace tweaks {

void StripAttrs::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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

void StripSubstituteAttrs::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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

void StripVars::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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

StripDatetimeVars::StripDatetimeVars()
{
    codes.push_back(WR_VAR(0, 4, 1));
    codes.push_back(WR_VAR(0, 4, 2));
    codes.push_back(WR_VAR(0, 4, 3));
    codes.push_back(WR_VAR(0, 4, 4));
    codes.push_back(WR_VAR(0, 4, 5));
    codes.push_back(WR_VAR(0, 4, 6));
}

RoundLegacyVars::RoundLegacyVars() : table(NULL)
{
    table = Vartable::get_bufr(BufrTableID(0, 0, 0, 14, 0));
}

void RoundLegacyVars::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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

void RemoveSynopWMOOnlyVars::tweak(Messages& msgs)
{
    int seen_tprec_trange = MISSING_INT;
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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
            c.remove(WR_VAR(0, 14,  2)); // Radiation data
            c.remove(WR_VAR(0, 14,  4)); // Radiation data
            c.remove(WR_VAR(0, 14, 16)); // Radiation data
            c.remove(WR_VAR(0, 14, 28)); // Radiation data
            c.remove(WR_VAR(0, 14, 29)); // Radiation data
            c.remove(WR_VAR(0, 14, 30)); // Radiation data
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

void RemoveTempWMOOnlyVars::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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

void RemoveSynopWMOOddprec::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
        m.remove_context(Level(1), Trange(1, 0));
    }
}

void TruncStName::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
        if (msg::Context* c = m.edit_context(Level(), Trange()))
            if (const Var* orig = c->find(WR_VAR(0, 1, 19)))
                if (orig->isset())
                {
                    const char* val = orig->enqc();
                    char buf[21];
                    strncpy(buf, val, 20);
                    buf[19] = '>';
                    buf[20] = 0;
                    c->set(Var(orig->info(), buf));
                }
    }
}

RoundGeopotential::RoundGeopotential()
{
    table = Vartable::get_bufr(BufrTableID(0, 0, 0, 14, 0));
}
void RoundGeopotential::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
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

HeightToGeopotential::HeightToGeopotential()
{
    table = Vartable::get_bufr(BufrTableID(0, 0, 0, 14, 0));
}
void HeightToGeopotential::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            if (c.level.ltype1 != 102) continue;
            Var var(table->query(WR_VAR(0, 10, 8)), round(c.level.l1 * 9.807 / 10) * 10);
            c.set(var);
        }
    }
}

void RoundVSS::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
        for (vector<msg::Context*>::iterator ci = m.data.begin(); ci != m.data.end(); ++ci)
        {
            msg::Context& c = **ci;
            if (Var* orig = c.edit(WR_VAR(0, 8, 42)))
                orig->seti(convert_BUFR08001_to_BUFR08042(convert_BUFR08042_to_BUFR08001(orig->enqi())));
        }
    }
}

RemoveContext::RemoveContext(const Level& lev, const Trange& tr)
    : lev(lev), tr(tr)
{
}
void RemoveContext::tweak(Messages& msgs)
{
    for (auto& mi: msgs)
    {
        Msg& m = Msg::downcast(mi);
        m.remove_context(lev, tr);
    }
}

}

TestMessage::TestMessage(File::Encoding type, const std::string& name)
    : name(name), type(type), raw(type)
{
}

TestMessage::~TestMessage()
{
    delete bulletin;
}

void TestMessage::read_from_file(const std::string& fname, const msg::ImporterOptions& input_opts)
{
    read_from_raw(read_rawmsg(fname.c_str(), type), input_opts);
}

void TestMessage::read_from_raw(const BinaryMessage& msg, const msg::ImporterOptions& input_opts)
{
    std::unique_ptr<msg::Importer> importer(msg::Importer::create(type, input_opts));
    raw = msg;
    switch (type)
    {
        case File::BUFR: bulletin = BufrBulletin::decode(raw.data).release(); break;
        case File::CREX: bulletin = CrexBulletin::decode(raw.data).release(); break;
        default: throw wreport::error_unimplemented("Unsupported message type");
    }
    msgs = importer->from_binary(raw);
}

void TestMessage::read_from_msgs(const Messages& _msgs, const msg::ExporterOptions& export_opts)
{
    // Export
    std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(type, export_opts));
    msgs = _msgs;
    delete bulletin;
    bulletin = exporter->to_bulletin(msgs).release();
    raw.data = bulletin->encode();
}

void TestMessage::dump() const
{
    string basename = "/tmp/" + name;

    string fname = basename + ".encoded";
    FILE* out = fopen(fname.c_str(), "w");
    fwrite(raw.data.data(), raw.data.size(), 1, out);
    fclose(out);
    cerr << name << " encoded version saved in " << fname << endl;

    if (bulletin)
    {
        fname = basename + ".bulletin";
        FILE* out = fopen(fname.c_str(), "w");
        bulletin->print_structured(out);
        fclose(out);
        cerr << name << " bulletin saved in " << fname << endl;
    }

    fname = basename + ".messages";
    out = fopen(fname.c_str(), "w");
    msgs.print(out);
    fclose(out);
    cerr << name << " interpreted saved in " << fname << endl;
}

TestCodec::TestCodec(const std::string& fname, File::Encoding type)
    : fname(fname), type(type)
{
}

void TestCodec::configure_ecmwf_to_wmo_tweaks()
{
    after_convert_import.add(new tweaks::StripQCAttrs);
    after_convert_import.add(new tweaks::StripDatetimeVars);
}

void TestCodec::do_compare(const TestMessage& msg1, const TestMessage& msg2)
{
    // Compare msgs
    {
        stringstream str;
        notes::Collect c(str);
        int diffs = msg1.msgs.diff(msg2.msgs);
        if (diffs)
        {
            msg1.dump();
            msg2.dump();
            dballe::tests::dump("diffs", str.str(), "details of differences");
            std::stringstream ss;
            ss << "found " << diffs << " differences";
            throw TestFailed(ss.str());
        }
    }

    // Compare bulletins
    try {
        if (msg2.bulletin->subsets.size() != (unsigned)expected_subsets)
        {
            std::stringstream ss;
            ss << "Number of subsets differ from expected: " << msg2.bulletin->subsets.size() << " != " << expected_subsets;
            throw TestFailed(ss.str());
        }
        if (msg2.bulletin->subsets[0].size() < (unsigned)expected_min_vars)
        {
            std::stringstream ss;
            ss << "Number of items in first subset is too small: " << msg2.bulletin->subsets[0].size() << " < " << expected_min_vars;
            throw TestFailed(ss.str());
        }
    } catch (...) {
        msg1.dump();
        msg2.dump();
        throw;
    }
}

void TestCodec::run_reimport()
{
    // Import
    if (verbose) cerr << "Importing " << fname << " with options '" << input_opts.to_string() << "'" << endl;
    TestMessage orig(type, "orig");
    try {
        orig.read_from_file(fname, input_opts);
    } catch (std::exception& e) {
        throw TestFailed(string("cannot decode ") + fname + ": " + e.what());
    }

    wassert(actual(orig.msgs.size()) > 0);

    // Run tweaks
    after_reimport_import.apply(orig.msgs);

    // Export
    if (verbose) cerr << "Exporting with options '" << output_opts.to_string() << "'" << endl;
    TestMessage exported(type, "exported");

    if (!expected_template.empty())
    {
        std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
        msg::WRExporter* exp = dynamic_cast<msg::WRExporter*>(exporter.get());
        auto tpl = exp->infer_template(orig.msgs);
        wassert(actual(tpl->name()) == expected_template);
    }
    wassert(exported.read_from_msgs(orig.msgs, output_opts));

    // Import again
    if (verbose) cerr << "Reimporting with options '" << input_opts.to_string() << "'" << endl;
    TestMessage final(type, "final");
    wassert(final.read_from_raw(exported.raw, input_opts));

    // Run tweaks
    after_reimport_reimport.apply(orig.msgs);
    after_reimport_reimport.apply(final.msgs);

    try {
        wassert(do_compare(orig, final));
    } catch (...) {
        dballe::tests::dump("reexported", exported.raw);
        throw;
    }

    if (expected_data_category != MISSING_INT)
        wassert(actual((unsigned)final.bulletin->data_category) == expected_data_category);
    if (expected_data_subcategory != MISSING_INT)
        wassert(actual((unsigned)final.bulletin->data_subcategory) == expected_data_subcategory);
    if (expected_data_subcategory_local != MISSING_INT)
        wassert(actual((unsigned)final.bulletin->data_subcategory_local) == expected_data_subcategory_local);
}

void TestCodec::run_convert(const std::string& tplname)
{
    // Import
    if (verbose) cerr << "Importing " << fname << " with options " << input_opts.to_string() << endl;
    TestMessage orig(type, "orig");
    try {
        orig.read_from_file(fname, input_opts);
    } catch (std::exception& e) {
        throw TestFailed(string("cannot decode ") + fname + ": " + e.what());
    }

    wassert(actual(orig.msgs.size()) > 0);

    // Run tweaks
    after_convert_import.apply(orig.msgs);

    // Export
    if (verbose) cerr << "Exporting with template " << tplname << " and options " << output_opts.to_string() << endl;
    msg::ExporterOptions output_opts = this->output_opts;
    output_opts.template_name = tplname;
    TestMessage exported(type, "exported");
    try {
        exported.read_from_msgs(orig.msgs, output_opts);
    } catch (std::exception& e) {
        //dballe::tests::dump("bul1", *exported);
        //balle::tests::dump("msg1", *msgs1);
        throw TestFailed(string("cannot export: ") + e.what());
    }

    // Import again
    TestMessage reimported(type, "reimported");
    try {
        reimported.read_from_raw(exported.raw, input_opts);
    } catch (std::exception& e) {
        //dballe::tests::dump("msg1", *msgs1);
        //dballe::tests::dump("msg", rawmsg);
        throw TestFailed(string("importing from exported rawmsg: ") + e.what());
    }

    // Run tweaks
    after_convert_reimport.apply(orig.msgs);
    after_convert_reimport.apply(reimported.msgs);

    try {
        wassert(do_compare(orig, reimported));
    } catch (...) {
        dballe::tests::dump("converted", exported.raw);
        throw;
    }
}

}
}
