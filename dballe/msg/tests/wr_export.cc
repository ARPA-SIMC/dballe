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

#include <test-utils-msg.h>
#include <dballe/msg/wr_codec.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/context.h>
#include <wreport/bulletin.h>
#include <wibble/string.h>
#include <set>
#include <cstring>

using namespace dballe;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct wr_export_shar
{
    wr_export_shar()
    {
    }

    ~wr_export_shar()
    {
    }
};
TESTGRP(wr_export);

// Common machinery for import -> export -> reimport tests
template<class B>
struct ReimportTest
{
    struct Hook
    {
        virtual ~Hook() {}
        virtual void clean_first(Msgs&) {};
        virtual void clean_second(Msgs&) {};
    };

    // Strip attributes from all variables in a Msgs
    struct StripAttrsHook : public Hook
    {
        bool first, second;
        StripAttrsHook(bool first=true, bool second=true)
            : first(first), second(second) {}
        void do_strip(Msgs& msgs)
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
                        while (const Var* a = v.next_attr())
                            v.unseta(a->code());
                    }
                }
            }
        }
        virtual void clean_first(Msgs& msgs) { if (first) do_strip(msgs); };
        virtual void clean_second(Msgs& msgs) { if (second) do_strip(msgs); };
    };

    // Round variables to account for a passage through legacy vars
    struct RoundLegacyVarsHook : public Hook
    {
        const Vartable* table;
        bool first, second;
        RoundLegacyVarsHook(bool first=true, bool second=true)
            : table(NULL), first(first), second(second)
        {
            table = Vartable::get("B0000000000000014000");
        }
        void do_round(Msgs& msgs)
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
                }
            }
        }
        virtual void clean_first(Msgs& msgs) { if (first) do_round(msgs); };
        virtual void clean_second(Msgs& msgs) { if (second) do_round(msgs); };
    };

    // Round variables to account for a passage through legacy vars
    struct RemoveSynopWMOOnlyVarsHook : public Hook
    {
        bool first, second;
        RemoveSynopWMOOnlyVarsHook(bool first=true, bool second=true)
            : first(first), second(second) {}
        void do_remove(Msgs& msgs)
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
                        c.remove(WR_VAR(0, 10, 60)); /// 24h pressure change
                    if (c.data.empty())
                        ci = m.data.erase(ci);
                    else
                        ++ci;
                }
            }
        }
        virtual void clean_first(Msgs& msgs) { if (first) do_remove(msgs); };
        virtual void clean_second(Msgs& msgs) { if (second) do_remove(msgs); };
    };

    // Remove ground level with missing length of statistical processing, that
    // cannot be encoded in ECMWF templates
    struct RemoveSynopWMOOddprecHook : public Hook
    {
        bool first, second;
        RemoveSynopWMOOddprecHook(bool first=true, bool second=true)
            : first(first), second(second) {}
        void do_remove(Msgs& msgs)
        {
            for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
            {
                Msg& m = **mi;
                m.remove_context(Level(1), Trange(1, 0));
            }
        }
        virtual void clean_first(Msgs& msgs) { if (first) do_remove(msgs); };
        virtual void clean_second(Msgs& msgs) { if (second) do_remove(msgs); };
    };

    // Truncate station name to its canonical length
    struct TruncStName : public Hook
    {
        TruncStName() {}
        void do_trunc(Msgs& msgs)
        {
            for (Msgs::iterator mi = msgs.begin(); mi != msgs.end(); ++mi)
            {
                Msg& m = **mi;
                if (msg::Context* c = m.edit_context(Level::ana(), Trange::ana()))
                    if (const Var* orig = c->find(WR_VAR(0, 1, 19)))
                        if (const char* val = orig->value())
                        {
                            char buf[20];
                            strncpy(buf, val, 20);
                            c->set(Var(orig->info(), buf));
                        }
            }
        }
        virtual void clean_first(Msgs& msgs) { do_trunc(msgs); };
        virtual void clean_second(Msgs& msgs) { do_trunc(msgs); };
    };

    // Preround geopotential with a B10003->B10008->B10009->B10008->B10003 round trip
    struct PreroundGeopotentialHook : public Hook
    {
        PreroundGeopotentialHook() {}
        void do_preround(Msgs& msgs)
        {
            const Vartable* table = Vartable::get("B0000000000000014000");
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
        virtual void clean_first(Msgs& msgs) { do_preround(msgs); };
        virtual void clean_second(Msgs& msgs) { do_preround(msgs); };
    };

    string fname;
    Encoding type;
    auto_ptr<Msgs> msgs1;
    auto_ptr<Msgs> msgs2;
    msg::Importer::Options input_opts;
    msg::Exporter::Options output_opts;
    vector<Hook*> hooks;

    void clear_hooks()
    {
        for (typename vector<Hook*>::iterator i = hooks.begin();
                i != hooks.end(); ++i)
            delete *i;
        hooks.clear();
    }

    void dump(const std::string& tag, const Msgs& msgs, const std::string& desc="message")
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
    void dump(const std::string& tag, const Bulletin& bul, const std::string& desc="message")
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
    void dump(const std::string& tag, const Rawmsg& msg, const std::string& desc="message")
    {
        string fname = "/tmp/" + tag + ".raw";
        FILE* out = fopen(fname.c_str(), "w");
        fwrite(msg.data(), msg.size(), 1, out);
        fclose(out);
        cerr << desc << " saved in " << fname << endl;
    }

    ReimportTest(const std::string& fname, Encoding type=BUFR)
        : fname(fname), type(type)
    {
    }
    ~ReimportTest()
    {
        clear_hooks();
    }

    void do_test(const dballe::tests::Location& loc, const char* tname1, const char* tname2=NULL)
    {
        std::auto_ptr<msg::Importer> importer(msg::Importer::create(type, input_opts));

        // Import
        msgs1 = inner_read_msgs_opts(fname.c_str(), type, input_opts);
        inner_ensure(msgs1->size() > 0);

        // Run hooks
        for (typename vector<Hook*>::iterator i = hooks.begin(); i != hooks.end(); ++i)
            (*i)->clean_first(*msgs1);

        // Export
        B bulletin;
        try {
            output_opts.template_name = tname1;
            std::auto_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
            exporter->to_bulletin(*msgs1, bulletin);
        } catch (std::exception& e) {
            dump("bul1", bulletin);
            dump("msg1", *msgs1);
            throw tut::failure(loc.msg(e.what()));
        }

        // Encode
        Rawmsg rawmsg;
        try {
            bulletin.encode(rawmsg);
            //exporter->to_rawmsg(*msgs1, rawmsg);
        } catch (std::exception& e) {
            dump("bul1", bulletin);
            dump("msg1", *msgs1);
            throw tut::failure(loc.msg(e.what()));
        }

        // Import again
        msgs2.reset(new Msgs);
        try {
            importer->from_rawmsg(rawmsg, *msgs2);
        } catch (std::exception& e) {
            dump("msg1", *msgs1);
            dump("msg", rawmsg);
            throw tut::failure(loc.msg(e.what()));
        }

        auto_ptr<Msgs> msgs3;
        if (tname2)
        {
            // Export
            B bulletin;
            try {
                output_opts.template_name = tname2;
                std::auto_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
                exporter->to_bulletin(*msgs2, bulletin);
            } catch (std::exception& e) {
                dump("bul2", bulletin);
                dump("msg2", *msgs1);
                throw tut::failure(loc.msg(e.what()));
            }

            // Encode
            Rawmsg rawmsg;
            try {
                bulletin.encode(rawmsg);
                //exporter->to_rawmsg(*msgs1, rawmsg);
            } catch (std::exception& e) {
                dump("bul2", bulletin);
                dump("msg2", *msgs1);
                throw tut::failure(loc.msg(e.what()));
            }

            // Import again
            msgs3.reset(new Msgs);
            try {
                importer->from_rawmsg(rawmsg, *msgs3);
            } catch (std::exception& e) {
                dump("msg2", *msgs2);
                dump("raw2", rawmsg);
                throw tut::failure(loc.msg(e.what()));
            }
        } else
            msgs3 = msgs2;

        // Run hooks
        for (typename vector<Hook*>::iterator i = hooks.begin(); i != hooks.end(); ++i)
            (*i)->clean_second(*msgs3);

        // Compare
        int diffs = msgs1->diff(*msgs3, stdout);
        if (diffs)
        {
            dump("msg1", *msgs1);
            if (msgs2.get())
                dump("msg2", *msgs2);
            dump("msg3", *msgs3);
            dump("msg", rawmsg);
        }
        inner_ensure_equals(diffs, 0);
    }
};
typedef ReimportTest<BufrBulletin> BufrReimportTest;
typedef ReimportTest<CrexBulletin> CrexReimportTest;
#define run_test(obj, ...) obj.do_test(wibble::tests::Location(__FILE__, __LINE__, (obj.fname + " " #__VA_ARGS__).c_str()), __VA_ARGS__)



// Test plain re-export of all our BUFR test files
template<> template<>
void to::test<1>()
{
    // note: These were blacklisted:
    //      "bufr/obs3-3.1.bufr",
    //      "bufr/obs3-56.2.bufr",
    //      "bufr/test-buoy1.bufr", 
    //      "bufr/test-soil1.bufr", 
    set<string> bl_crex;
    // CREX tables do not contain the entries needed to encode pollution
    // messages
    bl_crex.insert("bufr/ed4.bufr");

    const char** files = dballe::tests::bufr_files;
    vector<string> fails;
    int i;
    for (i = 0; files[i] != NULL; i++)
    {
        try {
            auto_ptr<Msgs> msgs = read_msgs(files[i], BUFR);
            ensure(msgs->size() > 0);

            std::auto_ptr<msg::Exporter> exporter;

            exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
            wreport::BufrBulletin bbulletin;
            exporter->to_bulletin(*msgs, bbulletin);

            if (bl_crex.find(files[i]) == bl_crex.end())
            {
                exporter = msg::Exporter::create(CREX/*, const Options& opts=Options()*/);
                wreport::CrexBulletin cbulletin;
                exporter->to_bulletin(*msgs, cbulletin);
            }
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

// Test plain re-export of all our CREX test files
template<> template<>
void to::test<2>()
{
    const char** files = dballe::tests::crex_files;

    vector<string> fails;
    int i;
    for (i = 0; files[i] != NULL; i++)
    {
        try {
            auto_ptr<Msgs> msgs = read_msgs(files[i], CREX);
            ensure(msgs->size() > 0);

            std::auto_ptr<msg::Exporter> exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
            wreport::BufrBulletin bbulletin;
            exporter->to_bulletin(*msgs, bbulletin);

            exporter = msg::Exporter::create(CREX/*, const Options& opts=Options()*/);
            wreport::CrexBulletin cbulletin;
            exporter->to_bulletin(*msgs, cbulletin);
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

// Export a well known TEMP which used to fail
template<> template<>
void to::test<3>()
{
    auto_ptr<Msgs> msgs = read_msgs_csv("csv/temp1.csv");
    ensure(msgs->size() > 0);

    // Replace with packed levels because comparison later happens against
    // packed levels
    {
        auto_ptr<Msg> msg(new Msg);
        msg->sounding_pack_levels(*(*msgs)[0]);
        msgs.reset(new Msgs);
        msgs->acquire(msg);
    }

    // Export to BUFR
    std::auto_ptr<msg::Exporter> bufr_exporter(msg::Exporter::create(BUFR/*, const Options& opts=Options()*/));
    wreport::BufrBulletin bbulletin;
    bufr_exporter->to_bulletin(*msgs, bbulletin);

    // Import and check the differences
    {
        std::auto_ptr<msg::Importer> bufr_importer(msg::Importer::create(BUFR/*, const Options& opts=Options()*/));
        Msgs msgs1;
        bufr_importer->from_bulletin(bbulletin, msgs1);
        ensure_equals(msgs->diff(msgs1, stderr), 0);
    }

    // Export to CREX
    std::auto_ptr<msg::Exporter> crex_exporter(msg::Exporter::create(CREX/*, const Options& opts=Options()*/));
    wreport::CrexBulletin cbulletin;
    crex_exporter->to_bulletin(*msgs, cbulletin);

    // Import and check the differences
    {
        std::auto_ptr<msg::Importer> crex_importer(msg::Importer::create(CREX/*, const Options& opts=Options()*/));
        Msgs msgs1;
        crex_importer->from_bulletin(cbulletin, msgs1);
        ensure_equals(msgs->diff(msgs1, stderr), 0);
    }
}

// Re-export test for old style synops
template<> template<>
void to::test<5>()
{
    {
        BufrReimportTest test("bufr/obs0-1.22.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
        run_test(test, "synop-wmo");
        test.clear_hooks();
        run_test(test, "synop-ecmwf");
    }
    {
        BufrReimportTest test("bufr/obs0-1.11188.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
        test.hooks.push_back(new BufrReimportTest::PreroundGeopotentialHook());
        run_test(test, "synop-wmo");
        test.clear_hooks();
        run_test(test, "synop-ecmwf");
    }
    {
        BufrReimportTest test("bufr/obs0-3.504.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
        run_test(test, "synop-wmo");
        test.clear_hooks();
        run_test(test, "synop-ecmwf");
    }
//    {
//        BufrReimportTest test("bufr/synop-old-buoy.bufr");
//        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
//        test.output_opts.template_name = "synop-wmo";
//        run_test(test, "auto");
//        test.output_opts.template_name = "synop-ecmwf";
//        test.clear_hooks();
//        run_test(test, "old");
//    }
}

// Re-export test for new style synops
template<> template<>
void to::test<6>()
{
    {
        BufrReimportTest test("bufr/synop-cloudbelow.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-evapo.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-groundtemp.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-longname.bufr");
        test.hooks.push_back(new BufrReimportTest::TruncStName());
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::TruncStName());
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-oddgust.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-oddprec.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOddprecHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-strayvs.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
    {
        BufrReimportTest test("bufr/synop-sunshine.bufr");
        run_test(test, "synop-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RoundLegacyVarsHook(true, true));
        test.hooks.push_back(new BufrReimportTest::RemoveSynopWMOOnlyVarsHook(true, true));
        run_test(test, "synop-ecmwf", "synop-wmo");
    }
}

// Re-export test for old style temps
template<> template<>
void to::test<7>()
{
    {
        BufrReimportTest test("bufr/obs2-101.16.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
        run_test(test, "temp-wmo", "temp-ecmwf");
        test.clear_hooks();
        run_test(test, "temp-ecmwf");
    }
    {
        BufrReimportTest test("bufr/obs2-102.1.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
        run_test(test, "temp-wmo", "temp-ecmwf");
        test.clear_hooks();
        run_test(test, "temp-ecmwf");
    }
    {
        BufrReimportTest test("bufr/obs2-91.2.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, false));
        run_test(test, "temp-wmo", "temp-ecmwf");
        test.clear_hooks();
        run_test(test, "temp-ecmwf");
    }
}

// Re-export test for new style temps
template<> template<>
void to::test<8>()
{
    {
        BufrReimportTest test("bufr/temp-gts1.bufr");
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
    {
        BufrReimportTest test("bufr/temp-gts2.bufr");
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
    {
        BufrReimportTest test("bufr/temp-gts3.bufr");
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
    //{
    //    BufrReimportTest test("bufr/temp-2-255.bufr");
    //    test.hooks.push_back(new BufrReimportTest::TruncStName());
    //    test.output_opts.template_name = "temp-wmo";
    //    run_test(test, "auto");
    //    test.output_opts.template_name = "temp-ecmwf";
    //    test.clear_hooks();
    //    run_test(test, "old");
    //}
    //{ // Geopotential gets changed during conversions
    //    BufrReimportTest test("bufr/temp-bad1.bufr");
    //    test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
    //    test.output_opts.template_name = "temp-wmo";
    //    run_test(test, "auto");
    //    test.output_opts.template_name = "temp-ecmwf";
    //    test.clear_hooks();
    //    run_test(test, "old");
    //}
    //{ // Geopotential gets changed during conversions
    //    BufrReimportTest test("bufr/temp-bad2.bufr");
    //    test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
    //    test.output_opts.template_name = "temp-wmo";
    //    run_test(test, "auto");
    //    test.output_opts.template_name = "temp-ecmwf";
    //    test.clear_hooks();
    //    run_test(test, "old");
    //}
    {
        BufrReimportTest test("bufr/temp-bad3.bufr");
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
    {
        BufrReimportTest test("bufr/temp-bad4.bufr");
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
    {
        BufrReimportTest test("bufr/temp-bad5.bufr");
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
    {
        BufrReimportTest test("bufr/tempforecast.bufr");
        run_test(test, "temp-wmo");
        test.clear_hooks();
        test.hooks.push_back(new BufrReimportTest::StripAttrsHook(true, true));
        run_test(test, "temp-ecmwf", "temp-wmo");
    }
}

// Re-export to BUFR and see the differences
template<> template<>
void to::test<9>()
{
    const char** files = dballe::tests::bufr_files;
    vector<string> fails;
    int i;
    std::auto_ptr<msg::Exporter> exporter;
    exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
    std::auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR/*, opts*/);

    for (i = 0; files[i] != NULL; i++)
    {
        try {
            // Import
            auto_ptr<Msgs> msgs = read_msgs(files[i], BUFR);
            ensure(msgs->size() > 0);

            // Export
            wreport::BufrBulletin bbulletin;
            exporter->to_bulletin(*msgs, bbulletin);

            // Import again
            Msgs msgs1;
            importer->from_bulletin(bbulletin, msgs1);

            // Compare
            int diffs = msgs->diff(msgs1, stdout);
            if (diffs)
            {
                FILE* out1 = fopen("/tmp/msg1.txt", "w");
                FILE* out2 = fopen("/tmp/msg2.txt", "w");
                msgs->print(out1);
                msgs1.print(out2);
                fclose(out1);
                fclose(out2);
            }
            ensure_equals(diffs, 0);
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

#if 0
template<> template<>
void to::test<1>()
{
    auto_ptr<Msgs> msgs = read_msgs("crex/test-synop0.crex", CREX);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    IS(block, 10); IS(station, 837); IS(st_type, 1);
    IS(year, 2004); IS(month, 11); IS(day, 30); IS(hour, 12); IS(minute, 0);
    IS(latitude, 48.22); IS(longitude, 9.92);
    IS(height, 550.0); UN(height_baro);
    IS(press, 94340.0); IS(press_msl, 100940.0); IS(press_tend, 7.0);
    IS(wind_dir, 80.0); IS(wind_speed, 6.0);
    IS(temp_2m, 276.15); IS(dewpoint_2m, 273.85); UN(humidity);
    IS(visibility, 5000.0); IS(pres_wtr, 10); IS(past_wtr1, 2); IS(past_wtr2, 2);
    IS(cloud_n, 100); IS(cloud_nh, 8); IS(cloud_hh, 450.0);
    IS(cloud_cl, 35); IS(cloud_cm, 61); IS(cloud_ch, 60);
    IS(cloud_n1, 8); IS(cloud_c1, 6); IS(cloud_h1, 350.0);
    UN(cloud_n2); UN(cloud_c2); UN(cloud_h2);
    UN(cloud_n3); UN(cloud_c3); UN(cloud_h3);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    UN(tot_prec24); UN(tot_snow);
}

template<> template<>
void to::test<2>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/obs0-1.22.bufr", BUFR);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    IS(block, 60); IS(station, 150); IS(st_type, 1);
    IS(year, 2004); IS(month, 11); IS(day, 30); IS(hour, 12); IS(minute, 0);
    IS(latitude, 33.88); IS(longitude, -5.53);
    IS(height, 560.0); UN(height_baro);
    IS(press, 94190.0); IS(press_msl, 100540.0); IS(press_3h, -180.0); IS(press_tend, 8.0);
    IS(wind_dir, 80.0); IS(wind_speed, 4.0);
    IS(temp_2m, 289.2); IS(dewpoint_2m, 285.7); UN(humidity);
    IS(visibility, 8000.0); IS(pres_wtr, 2); IS(past_wtr1, 6); IS(past_wtr2, 2);
    IS(cloud_n, 100); IS(cloud_nh, 8); IS(cloud_hh, 250.0);
    IS(cloud_cl, 39); IS(cloud_cm, 61); IS(cloud_ch, 60);
    IS(cloud_n1, 2); IS(cloud_c1, 8); IS(cloud_h1, 320.0);
    IS(cloud_n2, 5); IS(cloud_c2, 8); IS(cloud_h2, 620.0);
    IS(cloud_n3, 2); IS(cloud_c3, 9); IS(cloud_h3, 920.0);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    IS(tot_prec12, 0.5); UN(tot_snow);
}

template<> template<>
void to::test<3>()
{
    msg::Importer::Options opts;
    opts.simplified = true;
    auto_ptr<Msgs> msgs = read_msgs_opts("bufr/synop-cloudbelow.bufr", BUFR, opts);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    //msg.print(stderr);

    IS(block, 11); IS(station, 406); IS(st_type, 1);
    IS(year, 2009); IS(month, 12); IS(day, 3); IS(hour, 15); IS(minute, 0);
    IS(latitude, 50.07361); IS(longitude, 12.40333);
    IS(height, 483.0); IS(height_baro, 490.0);
    IS(press, 95090.0); IS(press_msl, 101060.0); IS(press_3h, -110.0); IS(press_tend, 6.0);
    IS(wind_dir, 0.0); IS(wind_speed, 1.0);
    IS(temp_2m, 273.05); IS(dewpoint_2m, 271.35); IS(humidity, 88.0);
    IS(visibility, 14000.0); IS(pres_wtr, 508); IS(past_wtr1, 10); IS(past_wtr2, 10);
    IS(cloud_n, 38); IS(cloud_nh, 0); IS(cloud_hh, 6000.0);
    IS(cloud_cl, 30); IS(cloud_cm, 20); IS(cloud_ch, 12);
    IS(cloud_n1, 3); IS(cloud_c1, 0); IS(cloud_h1, 6000.0);
    UN(cloud_n2); UN(cloud_c2); UN(cloud_h2);
    UN(cloud_n3); UN(cloud_c3); UN(cloud_h3);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    UN(tot_prec24); UN(tot_snow);
}

template<> template<>
void to::test<4>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/synop-cloudbelow.bufr", BUFR);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    //msg.print(stderr);

    IS(block, 11); IS(station, 406); IS(st_type, 1);
    IS(year, 2009); IS(month, 12); IS(day, 3); IS(hour, 15); IS(minute, 0);
    IS(latitude, 50.07361); IS(longitude, 12.40333);
    IS(height, 483.0); IS(height_baro, 490.0);
    IS(press, 95090.0); IS(press_msl, 101060.0); IS(press_3h, -110.0); IS(press_tend, 6.0);
    IS(wind_dir, 0.0); IS(wind_speed, 1.0);
    IS(temp_2m, 273.05); IS(dewpoint_2m, 271.35); IS(humidity, 88.0);
    IS(visibility, 14000.0); IS(pres_wtr, 508); IS(past_wtr1, 10); IS(past_wtr2, 10);
    IS(cloud_n, 38); IS(cloud_nh, 0); IS(cloud_hh, 6000.0);
    IS(cloud_cl, 30); IS(cloud_cm, 20); IS(cloud_ch, 12);
    IS(cloud_n1, 3); IS(cloud_c1, 0); IS(cloud_h1, 6000.0);
    UN(cloud_n2); UN(cloud_c2); UN(cloud_h2);
    UN(cloud_n3); UN(cloud_c3); UN(cloud_h3);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    UN(tot_prec24); UN(tot_snow);
}

static void relax_bufrex_msg(bufrex_msg b)
{
    int sounding_workarounds = 
        b->type == 2 && (b->localsubtype == 91 || b->localsubtype == 101 || b->localsubtype == 102);

    if (b->rep_year < 100)
        b->rep_year += 2000;
    for (size_t i = 0; i < b->subsets_count; ++i)
    {
        // See what is the index of the first attribute
        size_t first_attr = 0;
        for (; first_attr < b->subsets[i]->vars_count; ++first_attr)
            if (dba_var_code(b->subsets[i]->vars[first_attr]) == DBA_VAR(0, 33, 7))
                break;
    
        // See what is the index of the data present indicator
        size_t first_dds = 0;
        for (; first_dds < b->subsets[i]->vars_count; ++first_dds)
            if (dba_var_code(b->subsets[i]->vars[first_dds]) == DBA_VAR(0, 31, 31))
                break;

        // FIXME: I still haven't figured out a good way of comparing the
        // trailing confidence intervals for temps, so I get rid of them and
        // their leading delayed replication factor 
        if (sounding_workarounds)
            bufrex_subset_truncate(b->subsets[i], first_attr - 1);

        for (size_t j = 0; j < b->subsets[i]->vars_count; ++j)
        {
            switch (dba_var_code(b->subsets[i]->vars[j]))
            {
                case DBA_VAR(0, 8, 2):
                    // Vertical significances tend to lose attrs
                    dba_var_clear_attrs(b->subsets[i]->vars[j]);
                    if (b->type == 0 || b->type == 1)
                    {
                        // And they are meaningless for SYNOPs
                        dba_var_seti(b->subsets[i]->vars[j], 1);
                        // Also drop their confidence intervals queued for encoding
                        if (first_attr + j < b->subsets[i]->vars_count)
                            dba_var_unset(b->subsets[i]->vars[first_attr + j]);
                    }
                    // For temps, we also need to unset the data present indicators
                    if (sounding_workarounds)
                        if (first_dds + j < b->subsets[i]->vars_count)
                            dba_var_unset(b->subsets[i]->vars[first_dds + j]);
                    break;
                case DBA_VAR(0, 31, 1):
                    // Delayed replication factors' confidence intervals are
                    // SICK and we don't support them
                    dba_var_clear_attrs(b->subsets[i]->vars[j]);
                    if (first_attr + j < b->subsets[i]->vars_count)
                        dba_var_unset(b->subsets[i]->vars[first_attr + j]);
                    if (sounding_workarounds)
                        if (first_dds + j < b->subsets[i]->vars_count)
                            dba_var_unset(b->subsets[i]->vars[first_dds + j]);
                    break;
                case DBA_VAR(0, 1,  31):
                case DBA_VAR(0, 1,  32):
                case DBA_VAR(0, 1, 201):
                    // Generating centre and application do change
                    dba_var_seti(b->subsets[i]->vars[j], 1);
                    break;
                case DBA_VAR(0, 7,  32):
                    // Some pollution stations don't transmit the sensor
                    // height, but when it happens we use a default
                    if (b->type == 8)
                        dba_var_unset(b->subsets[i]->vars[j]);
                    break;
            }

            if (dba_var_value(b->subsets[i]->vars[j]) == NULL)
            {
                // Remove confidence intervals for unset variables
                if (first_attr + j < b->subsets[i]->vars_count)
                    dba_var_unset(b->subsets[i]->vars[first_attr + j]);
                // For temps, we also need to unset the data present indicators
                if (sounding_workarounds)
                    if (first_dds + j < b->subsets[i]->vars_count)
                        dba_var_unset(b->subsets[i]->vars[first_dds + j]);
            }
        }
    }
    // Some temp ship D table entries are not found in CREX D tables, so we
    // replace them with their expansion.  As a result, we cannot compare the
    // data descriptor sections of temp ship and we throw them away here.
    if (b->type == 2 && b->localsubtype == 102)
        bufrex_msg_reset_datadesc(b);

}

/* Test going from dba_msg to BUFR and back */
template<> template<>
void to::test<2>()
{
    const char* files[] = {
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
        NULL
    };

    for (int i = 0; files[i] != NULL; i++)
    {
        test_tag(files[i]);

        // Read the test message in a bufrex_raw
        bufrex_msg braw1 = read_test_msg_raw(files[i], BUFR);

        // Save category as a reference for reencoding
        int type = braw1->type;
        int subtype = braw1->subtype;
        int localsubtype = braw1->localsubtype;

        // Finish converting in a dba_msg
        dba_msgs msgs1;
        CHECKED(bufrex_msg_to_dba_msgs(braw1, &msgs1));

        /// Test if reencoded bufr_msg matches

        // Reencode the message to BUFR using the same template
        bufrex_msg b1 = read_test_msg_raw(files[i], BUFR);
        bufrex_msg b2;
        CHECKED(bufrex_msg_create(BUFREX_BUFR, &b2));
        b2->type = b1->type;
        b2->subtype = b1->subtype;
        b2->localsubtype = b1->localsubtype;
        //fprintf(stderr, "Read %s %d %d %d\n", files[i], b1->type, b1->subtype, b1->localsubtype);
        b2->edition = b1->edition;
        b2->opt.bufr.centre = b1->opt.bufr.centre;
        b2->opt.bufr.subcentre = b1->opt.bufr.subcentre;
        b2->opt.bufr.master_table = b1->opt.bufr.master_table;
        b2->opt.bufr.local_table = b1->opt.bufr.local_table;
        CHECKED(bufrex_msg_load_tables(b2));
        CHECKED(bufrex_msg_from_dba_msgs(b2, msgs1));

        // FIXME: relax checks a bit
        relax_bufrex_msg(b1);
        relax_bufrex_msg(b2);

        // Compare b1 and b2
        int bdiffs = 0;
        // Our metar message sample is different than the official template
        // and test-airep1 uses the wrong varcode for originating application
        // and test-temp1 uses a nonstandard template
        #if 0
        if ((b1->type != 0 || b1->subtype != 255 || b1->localsubtype != 140)
                && string(files[i]) != "bufr/test-airep1.bufr"
                && string(files[i]) != "bufr/test-temp1.bufr")
            bufrex_msg_diff(b1, b2, &bdiffs, stderr);
        if (bdiffs > 0)
        {
            FILE* out1 = fopen("/tmp/bufrexmsg1.txt", "wt");
            FILE* out2 = fopen("/tmp/bufrexmsg2.txt", "wt");
            bufrex_msg_print(b1, out1);
            bufrex_msg_print(b2, out2);
            fclose(out1);
            fclose(out2);
            fprintf(stderr, "Message dumps have been written to /tmp/bufrexmsg1.txt and /tmp/bufrexmsg2.txt\n");
        }
        #endif
        gen_ensure_equals(bdiffs, 0);

        bufrex_msg_delete(b1);
        bufrex_msg_delete(b2);


        /// Test if reencoded dba_msg match
        if (strcmp(files[i], "bufr/ed4.bufr") != 0)
        {
            // Reencode the dba_msg in another dba_rawmsg
            dba_rawmsg raw2;
            CHECKED(bufrex_encode_bufr(msgs1, type, subtype, localsubtype, &raw2));

            // Parse the second dba_rawmsg
            dba_msgs msgs2;
            CHECKED(bufrex_decode_bufr(raw2, &msgs2));

            // Compare the two dba_msg
            int diffs = 0;
            dba_msgs_diff(msgs1, msgs2, &diffs, stderr);

            if (diffs != 0)
            {
                FILE* outraw1 = fopen("/tmp/raw1.txt", "w");
                bufrex_msg_print(braw1, outraw1);
                fclose(outraw1);

                FILE* outraw2 = fopen("/tmp/raw2.txt", "w");
                bufrex_msg braw2;
                CHECKED(bufrex_msg_create(BUFREX_BUFR, &braw2));
                braw2->edition = 3;
                braw2->type = type;
                braw2->subtype = subtype;
                braw2->localsubtype = localsubtype;
                braw2->opt.bufr.centre = 98;
                braw2->opt.bufr.subcentre = 0;
                braw2->opt.bufr.master_table = 6;
                braw2->opt.bufr.local_table = 1;
                CHECKED(bufrex_msg_load_tables(braw2));
                CHECKED(bufrex_msg_from_dba_msgs(braw2, msgs1));
                bufrex_msg_print(braw2, outraw2);
                fclose(outraw2);
                bufrex_msg_delete(braw2);

                FILE* out1 = fopen("/tmp/msg1.txt", "w");
                FILE* out2 = fopen("/tmp/msg2.txt", "w");
                    
                dba_msgs_print(msgs1, out1);
                dba_msgs_print(msgs2, out2);
                fclose(out1);
                fclose(out2);
            }

            gen_ensure_equals(diffs, 0);

            //cerr << files[i] << ": ok" << endl;

            dba_msgs_delete(msgs1);
            dba_msgs_delete(msgs2);
            bufrex_msg_delete(braw1);
            dba_rawmsg_delete(raw2);
        }
    }
    test_untag();
}

/* Test going from CREX to dba_msg and back */
template<> template<>
void to::test<3>()
{
    const char* files[] = {
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

    for (int i = 0; files[i] != NULL; i++)
    {
        test_tag(files[i]);

        // Read the test message in a bufrex_raw
        bufrex_msg braw1 = read_test_msg_raw(files[i], CREX);

        // Save category as a reference for reencoding
        int type = braw1->type;
        int subtype = braw1->subtype;
        int localsubtype = braw1->localsubtype;
        if (localsubtype == 0)
            type = 0;
        
        // Finish converting in a dba_msg
        dba_msgs msgs1;
        CHECKED(bufrex_msg_to_dba_msgs(braw1, &msgs1));

        // Reencode the dba_msg in another dba_rawmsg
        dba_rawmsg raw2;
        CHECKED(bufrex_encode_crex(msgs1, type, localsubtype, &raw2));

        // Parse the second dba_rawmsg
        dba_msgs msgs2;
        CHECKED(bufrex_decode_crex(raw2, &msgs2));

        /*
        if (string(files[i]).find("mare2") != string::npos)
        {
            dba_msg_print(msg1, stderr);
            dba_msg_print(msg2, stderr);
        }
        */

        // Compare the two dba_msg
        int diffs = 0;
        dba_msgs_diff(msgs1, msgs2, &diffs, stderr);
        gen_ensure_equals(diffs, 0);

        dba_msgs_delete(msgs1);
        dba_msgs_delete(msgs2);
        bufrex_msg_delete(braw1);
        dba_rawmsg_delete(raw2);
    }
    test_untag();
}

/* Check that a BUFR from a synop high-level station correctly reports isobaric
 * surface and geopotential */
template<> template<>
void to::test<4>()
{
    dba_msgs msgs = read_test_msg("bufr/obs0-1.11188.bufr", BUFR);
    dba_msg src = msgs->msgs[0];
    dba_var var;

    //gen_ensure((var = dba_msg_get_isobaric_surface_var(src)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    gen_ensure((var = dba_msg_get_geopotential_var(src)) != NULL);
    gen_ensure(dba_var_value(var) != NULL);

    dba_msgs_delete(msgs);
}

/* Test import of environment BUFR4 messages */
template<> template<>
void to::test<5>()
{
    dba_msgs msgs = read_test_msg("bufr/ed4.bufr", BUFR);
    dba_msg src = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    gen_ensure(dba_var_value(var) != NULL);
    CHECKED(dba_var_enqd(var, &val));
    gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<6>()
{
    dba_msgs msgs = read_test_msg("bufr/ed4-compr-string.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<7>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-cloudbelow.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<8>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-groundtemp.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<9>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-sunshine.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    // Check the context information for the wind data
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 1), 103, 10000, 0, 0, 0, -600, 600)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 140);
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 41), 103, 10000, 0, 0, 205, -600, 600)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 15.4);
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 41), 103, 10000, 0, 0, 205, -10800, 10800)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 15.4);

    dba_msgs_delete(msgs);
}

/* Test import of a WMO GTS synop message with a stray vertical significance */
template<> template<>
void to::test<10>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-strayvs.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<11>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-evapo.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);
    gen_ensure_equals(msgs->len, 14);

    msg = msgs->msgs[4];
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<12>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-oddprec.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);
    gen_ensure_equals(msgs->len, 1);

    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<13>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-oddgust.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);
    gen_ensure_equals(msgs->len, 26);

    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS temp messages */
template<> template<>
void to::test<14>()
{
    dba_msgs msgs = read_test_msg("bufr/temp-gts1.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_TEMP);
    gen_ensure_equals(msgs->len, 1);

    // Ensure we decoded all the sounding levels
    int pres_lev_count = 0;
    for (int i = 0; i < msg->data_count; ++i)
        if (msg->data[i]->ltype1 == 100)
            ++pres_lev_count;
    gen_ensure_equals(pres_lev_count, 56);

    // Ensure we got the wind shear section
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 61), 100, 35560, 0, 0, 254, 0, 0)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 13.1);
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 62), 100, 35560, 0, 0, 254, 0, 0)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 5.6);

    // Ensure the extended vertical significances are put in the right
    // level, since they appear before the pressure context
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 8, 42), 100, 100000, 0, 0, 254, 0, 0)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 65536);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS temp messages */
template<> template<>
void to::test<15>()
{
    dba_msgs msgs = read_test_msg("bufr/temp-gts2.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_TEMP);
    gen_ensure_equals(msgs->len, 6);

    // Ensure we decoded all the sounding levels
    int pres_lev_count = 0;
    for (int i = 0; i < msg->data_count; ++i)
        if (msg->data[i]->ltype1 == 100)
            ++pres_lev_count;
    gen_ensure_equals(pres_lev_count, 45);


    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS temp messages */
template<> template<>
void to::test<16>()
{
    dba_msgs msgs = read_test_msg("bufr/temp-gts3.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_TEMP);
    gen_ensure_equals(msgs->len, 1);

    // Ensure we decoded all the sounding levels
    int pres_lev_count = 0;
    for (int i = 0; i < msg->data_count; ++i)
        if (msg->data[i]->ltype1 == 100)
            ++pres_lev_count;
    gen_ensure_equals(pres_lev_count, 26);

    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */
#endif

}

/* vim:set ts=4 sw=4: */
