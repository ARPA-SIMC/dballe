#ifndef DBALLE_TESTS_WR_EXPORT_H
#define DBALLE_TESTS_WR_EXPORT_H

namespace {

// Common machinery for import -> export -> reimport tests
template<class B>
struct ReimportTest
{
    typedef dballe::tests::MessageTweaker Tweaker;

    string fname;
    Encoding type;
    unique_ptr<Msgs> msgs1;
    unique_ptr<Msgs> msgs2;
    unique_ptr<Bulletin> exported;
    msg::Importer::Options input_opts;
    msg::Exporter::Options output_opts;
    vector<Tweaker*> tweaks;
    vector<Tweaker*> ecmwf_tweaks;
    vector<Tweaker*> wmo_tweaks;
    bool do_ecmwf_tweaks;
    bool do_wmo_tweaks;
    bool do_ignore_context_attrs;
    bool do_round_geopotential;
    bool verbose;

    void clear_tweaks()
    {
        for (typename vector<Tweaker*>::iterator i = tweaks.begin();
                i != tweaks.end(); ++i)
            delete *i;
        tweaks.clear();
        for (typename vector<Tweaker*>::iterator i = ecmwf_tweaks.begin();
                i != ecmwf_tweaks.end(); ++i)
            delete *i;
        ecmwf_tweaks.clear();
        for (typename vector<Tweaker*>::iterator i = wmo_tweaks.begin();
                i != wmo_tweaks.end(); ++i)
            delete *i;
        wmo_tweaks.clear();
    }

    ReimportTest(const std::string& fname, Encoding type=BUFR)
        : fname(fname), type(type), do_ecmwf_tweaks(false), do_wmo_tweaks(false),
          do_ignore_context_attrs(false), do_round_geopotential(false), verbose(false)
    {
        ecmwf_tweaks.push_back(new StripQCAttrs());
        wmo_tweaks.push_back(new RoundLegacyVars());
    }
    ~ReimportTest()
    {
        clear_tweaks();
    }

    void do_test(const dballe::tests::Location& loc, const char* tname1, const char* tname2=NULL)
    {
        if (verbose) cerr << "Running test " << loc.locstr() << endl;

        std::unique_ptr<msg::Importer> importer(msg::Importer::create(type, input_opts));

        // Import
        if (verbose) cerr << "Importing " << fname << " " << input_opts.to_string() << endl;
        msgs1 = inner_read_msgs_opts(fname.c_str(), type, input_opts);
        inner_ensure(msgs1->size() > 0);

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

        // Export
        exported.reset(B::create().release());
        try {
            if (tname1 != NULL)
                output_opts.template_name = tname1;
            else
                output_opts.template_name.clear();
            if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
            std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
            exporter->to_bulletin(*msgs1, *exported);
        } catch (std::exception& e) {
            dballe::tests::dump("bul1", *exported);
            dballe::tests::dump("msg1", *msgs1);
            throw tut::failure(loc.msg(string("exporting to bulletin (first template): ") + e.what()));
        }

        // Encode
        Rawmsg rawmsg;
        try {
            exported->encode(rawmsg);
            //exporter->to_rawmsg(*msgs1, rawmsg);
        } catch (std::exception& e) {
            dballe::tests::dump("bul1", *exported);
            dballe::tests::dump("msg1", *msgs1);
            throw tut::failure(loc.msg(string("encoding to rawmsg (first template): ") + e.what()));
        }

        // Import again
        if (verbose) cerr << "Reimporting " << input_opts.to_string() << endl;
        msgs2.reset(new Msgs);
        try {
            importer->from_rawmsg(rawmsg, *msgs2);
        } catch (std::exception& e) {
            dballe::tests::dump("msg1", *msgs1);
            dballe::tests::dump("msg", rawmsg);
            throw tut::failure(loc.msg(string("importing from rawmsg (first template): ") + e.what()));
        }

        unique_ptr<Msgs> msgs3;
        if (tname2)
        {
            // Export
            unique_ptr<Bulletin> bulletin(B::create());
            try {
                output_opts.template_name = tname2;
                if (verbose) cerr << "Reexporting " << output_opts.to_string() << endl;
                std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
                exporter->to_bulletin(*msgs2, *bulletin);
            } catch (std::exception& e) {
                dballe::tests::dump("bul2", *bulletin);
                dballe::tests::dump("msg2", *msgs1);
                throw tut::failure(loc.msg(string("exporting to bulletin (second template): ") + e.what()));
            }

            // Encode
            rawmsg.clear();
            try {
                bulletin->encode(rawmsg);
                //exporter->to_rawmsg(*msgs1, rawmsg);
            } catch (std::exception& e) {
                dballe::tests::dump("bul2", *bulletin);
                dballe::tests::dump("msg2", *msgs1);
                throw tut::failure(loc.msg(string("encoding to rawmsg (second template): ") + e.what()));
            }

            // Import again
            msgs3.reset(new Msgs);
            try {
                if (verbose) cerr << "Reimporting " << input_opts.to_string() << endl;
                importer->from_rawmsg(rawmsg, *msgs3);
            } catch (std::exception& e) {
                dballe::tests::dump("msg2", *msgs2);
                dballe::tests::dump("raw2", rawmsg);
                throw tut::failure(loc.msg(string("importing from rawmsg (first template): ") + e.what()));
            }
        } else
            msgs3 = move(msgs2);

#if 0
        // Run tweaks
        for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
            (*i)->clean_second(*msgs3);
#endif
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

        // Compare
        stringstream str;
        notes::Collect c(str);
        int diffs = msgs1->diff(*msgs3);
        if (diffs)
        {
            dballe::tests::dump("msg1", *msgs1);
            if (msgs2.get())
                dballe::tests::dump("msg2", *msgs2);
            dballe::tests::dump("msg3", *msgs3);
            dballe::tests::dump("msg", rawmsg);
            dballe::tests::dump("diffs", str.str(), "details of differences");
            throw tut::failure(loc.msg(str::fmtf("found %d differences", diffs)));
        }
    }

#define inner_do_test(name, ...) do_test(wibble::tests::Location(loc, __FILE__, __LINE__, name), __VA_ARGS__)
    void do_ecmwf(const dballe::tests::Location& loc, const char* template_type="synop")
    {
        string ecmwf_template_name = string(template_type) + "-ecmwf";
        string wmo_template_name = string(template_type) + "-wmo";
        do_wmo_tweaks = false;

        input_opts.simplified = true;
        do_round_geopotential = false;

        do_ecmwf_tweaks = false;
        inner_do_test("simp-ecmwf-through-auto", NULL);
        inner_do_test("simp-ecmwf-through-ecmwf", ecmwf_template_name.c_str());

        do_ecmwf_tweaks = true;
        do_round_geopotential = true;
        inner_do_test("simp-ecmwf-through-wmo", wmo_template_name.c_str());

        input_opts.simplified = false;
        do_round_geopotential = false;

        do_ecmwf_tweaks = false;
        inner_do_test("real-ecmwf-through-auto", NULL);
        inner_do_test("real-ecmwf-through-ecmwf", ecmwf_template_name.c_str());

        do_ecmwf_tweaks = true;
        do_round_geopotential = true;
        inner_do_test("real-ecmwf-through-wmo", wmo_template_name.c_str());
    }
    void do_wmo(const dballe::tests::Location& loc, const char* template_type="synop")
    {
        string ecmwf_template_name = string(template_type) + "-ecmwf";
        string wmo_template_name = string(template_type) + "-wmo";
        do_ecmwf_tweaks = false;

        input_opts.simplified = true;
        do_round_geopotential = false;

        do_wmo_tweaks = false;
        inner_do_test("simp-wmo-through-auto", NULL);
        inner_do_test("simp-wmo-through-wmo", wmo_template_name.c_str());

        do_wmo_tweaks = true;
        do_ignore_context_attrs = true;
        do_round_geopotential = true;
        inner_do_test("simp-wmo-through-ecmwf", ecmwf_template_name.c_str());
        do_ignore_context_attrs = false;

        input_opts.simplified = false;
        do_round_geopotential = false;

        do_wmo_tweaks = false;
        inner_do_test("real-wmo-through-auto", NULL);
        inner_do_test("real-wmo-through-wmo", wmo_template_name.c_str());

        // There doesn't seem much sense testing this at the moment
        //do_tweaks = true;
        //inner_do_test("real-wmo-through-ecmwf", ecmwf_template_name.c_str());
    }
#undef inner_do_test
};
typedef ReimportTest<BufrBulletin> BufrReimportTest;
typedef ReimportTest<CrexBulletin> CrexReimportTest;
#define run_test(obj, meth, ...) obj.meth(wibble::tests::Location(__FILE__, __LINE__, (obj.fname + " " #__VA_ARGS__).c_str()), __VA_ARGS__)

}

#endif
