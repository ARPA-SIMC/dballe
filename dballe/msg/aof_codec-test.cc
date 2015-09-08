#include "tests.h"
#include "aof_codec.h"
#include "msg.h"
#include "context.h"
#include <wreport/codetables.h>
#include <wreport/notes.h>
#include <math.h>

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

using dballe::tests::TestCodec;

msg::AOFImporter importer;

void strip_attributes(Messages& msgs)
{
    for (auto& i: msgs)
    {
        Msg& msg = Msg::downcast(i);
        for (vector<msg::Context*>::iterator j = msg.data.begin(); j != msg.data.end(); ++j)
            for (vector<Var*>::iterator k = (*j)->data.begin(); k != (*j)->data.end(); ++k)
                (*k)->clear_attrs();
    }
}

void propagate_if_missing(int varid, const Msg& src, Msg& dst)
{
    const Var* var = src.find_by_id(varid);
    if (var == NULL || !var->isset()) return;
    dst.set_by_id(*var, varid);
}

void normalise_encoding_quirks(Messages& amsgs, Messages& bmsgs)
{
    size_t len = amsgs.size();
    if (len == 0) return;
    if (bmsgs.size() == 0) return;

    // Message-wide tweaks
    if (Msg::downcast(amsgs[0]).type == MSG_PILOT)
    {
        dballe::tests::tweaks::StripVars stripper;
        stripper.codes.push_back(WR_VAR(0, 11, 61));
        stripper.codes.push_back(WR_VAR(0, 11, 62));
        stripper.tweak(amsgs);
        stripper.tweak(bmsgs);
    }

    if (len > bmsgs.size()) len = bmsgs.size();
    for (size_t msgidx = 0; msgidx < len; ++msgidx)
    {
        Msg& amsg = Msg::downcast(amsgs[msgidx]);
        Msg& bmsg = Msg::downcast(bmsgs[msgidx]);

        for (size_t i = 0; i < bmsg.data.size(); ++i)
        {
            msg::Context& ctx = *bmsg.data[i];
            for (size_t j = 0; j < ctx.data.size(); ++j)
            {
                int qc_is_undef = 0;
                Var& var = *ctx.data[j];

                // Recode BUFR attributes to match the AOF 2-bit values
                for (const Var* attr = var.next_attr(); attr != NULL; attr = attr->next_attr())
                {
                    if (attr->code() == WR_VAR(0, 33, 7))
                    {
                        if (!attr->isset())
                        {
                            qc_is_undef = 1;
                        }
                        else
                        {
                            int val = attr->enqi();
                            // Recode val using one of the value in the 4 steps of AOF
                            if (val > 75)
                                val = 76;
                            else if (val > 50)
                                val = 51;
                            else if (val > 25)
                                val = 26;
                            else
                                val = 0;
                            // Cast away const. This whole function is a hack,
                            // therefore we can. HARRR!
                            ((Var*)attr)->seti(val);
                        }
                    }
                }
                if (qc_is_undef)
                    var.unseta(WR_VAR(0, 33, 7));

                // Propagate Vertical Significances
                if (var.code() == WR_VAR(0, 8, 2))
                    amsg.set(var, WR_VAR(0, 8, 2), ctx.level, ctx.trange);
            }
        }
        
        Var* var;

        if ((var = bmsg.edit_by_id(DBA_MSG_BLOCK)) != NULL)
            var->clear_attrs();
        if ((var = bmsg.edit_by_id(DBA_MSG_STATION)) != NULL)
            var->clear_attrs();
        if ((var = bmsg.edit_by_id(DBA_MSG_ST_TYPE)) != NULL)
            var->clear_attrs();
        if ((var = bmsg.edit_by_id(DBA_MSG_IDENT)) != NULL)
            var->clear_attrs();
        if ((var = bmsg.edit_by_id(DBA_MSG_FLIGHT_PHASE)) != NULL)
            var->clear_attrs();

        if ((var = bmsg.edit_by_id(DBA_MSG_CLOUD_CL)) != NULL &&
                var->enqi() == 62 && amsg.get_cloud_cl_var() == NULL)
            amsg.set_cloud_cl_var(*var);

        if ((var = bmsg.edit_by_id(DBA_MSG_CLOUD_CM)) != NULL &&
                var->enqi() == 61 && amsg.get_cloud_cm_var() == NULL)
            amsg.set_cloud_cm_var(*var);

        if ((var = bmsg.edit_by_id(DBA_MSG_CLOUD_CH)) != NULL &&
                var->enqi() == 60 && amsg.get_cloud_ch_var() == NULL)
            amsg.set_cloud_ch_var(*var);

        propagate_if_missing(DBA_MSG_HEIGHT_ANEM, bmsg, amsg);
        propagate_if_missing(DBA_MSG_NAVSYS, bmsg, amsg);

        // In AOF, only synops and ships can encode direction and speed
        if (amsg.type != MSG_SHIP && amsg.type != MSG_SYNOP)
        {
            propagate_if_missing(DBA_MSG_ST_DIR, bmsg, amsg);
            propagate_if_missing(DBA_MSG_ST_SPEED, bmsg, amsg);
        }

        propagate_if_missing(DBA_MSG_PRESS_TEND, bmsg, amsg);

        // AOF AMDAR has pressure indication, BUFR AMDAR has only height
        if (amsg.type == MSG_AMDAR)
        {
            // dba_var p = dba_msg_get_flight_press_var(amsg);
            for (size_t i = 0; i < amsg.data.size(); ++i)
            {
                msg::Context& c = *amsg.data[i];
                if (c.level.ltype1 == 100 && c.trange == Trange::instant())
                    if (const Var* var = c.find(WR_VAR(0, 10, 4)))
                    {
                        bmsg.set(*var, WR_VAR(0, 10, 4), c.level, c.trange);
                        break;
                    }
            }
#if 0
            dba_var h = dba_msg_get_height_var(bmsg);
            if (p && h)
            {
                double press, height;
                CHECKED(dba_var_enqd(p, &press));
                CHECKED(dba_var_enqd(h, &height));
                dba_msg_level l = dba_msg_find_level(bmsg, 103, (int)height, 0);
                if (l)
                {
                    l->ltype = 100;
                    l->l1 = (int)(press/100);
                    CHECKED(dba_msg_set_flight_press_var(bmsg, p));
                }
            }
#endif
        }

        if (amsg.type == MSG_TEMP)
        {
            propagate_if_missing(DBA_MSG_SONDE_TYPE, bmsg, amsg);
            propagate_if_missing(DBA_MSG_SONDE_METHOD, bmsg, amsg);
        }

        if (amsg.type == MSG_TEMP_SHIP)
        {
            propagate_if_missing(DBA_MSG_HEIGHT_STATION, bmsg, amsg);
        }

        if (amsg.type == MSG_TEMP || amsg.type == MSG_TEMP_SHIP)
        {
            propagate_if_missing(DBA_MSG_CLOUD_N, bmsg, amsg);
            propagate_if_missing(DBA_MSG_CLOUD_NH, bmsg, amsg);
            propagate_if_missing(DBA_MSG_CLOUD_HH, bmsg, amsg);
            propagate_if_missing(DBA_MSG_CLOUD_CL, bmsg, amsg);
            propagate_if_missing(DBA_MSG_CLOUD_CM, bmsg, amsg);
            propagate_if_missing(DBA_MSG_CLOUD_CH, bmsg, amsg);

#define FIX_GEOPOTENTIAL
#ifdef FIX_GEOPOTENTIAL
            // Convert the geopotentials back to heights in a dirty way, but it
            // should make the comparison more meaningful.  It catches this case:
            //  - AOF has height, that gets multiplied by 9.80665
            //    25667 becomes 251707
            //  - BUFR stores geopotential, without the last digit
            //    251707 becomes 251710
            //  - However, if we go back to heights, the precision should be
            //    preserved
            //    251710 / 9.80664 becomes 25667 as it was
            for (size_t i = 0; i < amsg.data.size(); ++i)
            {
                msg::Context& c = *amsg.data[i];
                if (Var* var = c.edit(WR_VAR(0, 10, 3)))
                    var->setd(var->enqd() / 9.80665);
            }
            for (size_t i = 0; i < bmsg.data.size(); ++i)
            {
                msg::Context& c = *bmsg.data[i];
                if (Var* var = c.edit(WR_VAR(0, 10, 3)))
                    var->setd(var->enqd() / 9.80665);
            }

            // Decoding BUFR temp messages copies data from the surface level into
            // more classical places: compensate by copying the same data in the
            // AOF file
            propagate_if_missing(DBA_MSG_PRESS, bmsg, amsg);
            propagate_if_missing(DBA_MSG_TEMP_2M, bmsg, amsg);
            propagate_if_missing(DBA_MSG_DEWPOINT_2M, bmsg, amsg);
            propagate_if_missing(DBA_MSG_WIND_DIR, bmsg, amsg);
            propagate_if_missing(DBA_MSG_WIND_SPEED, bmsg, amsg);
#endif
        }

        // Remove attributes from all vertical sounding significances
        for (size_t i = 0; i < bmsg.data.size(); ++i)
        {
            msg::Context& c = *bmsg.data[i];
            if (Var* var = c.edit(WR_VAR(0, 8, 42)))
            {
                var->clear_attrs();
                // Remove SIGHUM that is added by ECMWF template conversions
                int val = var->enqi();
                val &= ~BUFR08042::SIGHUM;
                var->seti(val);
            }
        }
    }
}


class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test simple decoding
        add_method("simple", []() {
            const char** files = dballe::tests::aof_files;
            for (size_t i = 0; files[i] != NULL; i++)
            {
                try {
                    BinaryMessage raw = read_rawmsg(files[i], File::AOF);

                    /* Parse it */
                    Messages msgs = importer.from_binary(raw);
                    wassert(actual(msgs.empty()).isfalse());
                } catch (std::exception& e) {
                    throw TestFailed(string("[") + files[i] + "] " + e.what());
                }
            }
        });

        // Compare decoding results with BUFR sample data
        add_method("compare_bufr", []() {
            string files[] = {
                "aof/obs1-14.63",		// OK
                "aof/obs1-21.1",		// OK
                "aof/obs1-24.2104",		// OK
        //		"aof/obs1-24.34",		// Data are in fact slightly different
        //		"aof/obs2-144.2198",	// Data have different precision in BUFR and AOF, but otherwise match
        //		"aof/obs2-244.0",		// BUFR counterpart missing for this message
                "aof/obs4-165.2027",	// OK
        //		"aof/obs5-35.61",		// Data are in fact slightly different
        //		"aof/obs5-36.30",		// Data are in fact slightly different
                "aof/obs6-32.1573",		// OK
        //		"aof/obs6-32.0",		// BUFR conterpart missing for this message
                "",
            };

            for (size_t i = 0; !files[i].empty(); i++)
            {
                try {
                    Messages amsgs = read_msgs((files[i] + ".aof").c_str(), File::AOF);
                    Messages bmsgs = read_msgs((files[i] + ".bufr").c_str(), File::BUFR);
                    normalise_encoding_quirks(amsgs, bmsgs);

                    // Compare the two dba_msg
                    notes::Collect c(cerr);
                    int diffs = amsgs.diff(bmsgs);
                    if (diffs)
                    {
                        dballe::tests::track_different_msgs(amsgs, bmsgs, "aof");
                    }
                    wassert(actual(diffs) == 0);
                } catch (std::exception& e) {
                    throw TestFailed(string(files[i]) + ": " + e.what());
                }
            }
        });

        // Reencode to BUFR and compare
        add_method("reencode_bufr", []() {
            std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(File::BUFR));
            std::unique_ptr<msg::Importer> importer(msg::Importer::create(File::BUFR));
            const char** files = dballe::tests::aof_files;
            // Note: missingclouds and brokenamdar were not in the original file list before it got unified
            for (size_t i = 0; files[i] != NULL; i++)
            {
                try {
                    // Read
                    Messages amsgs = read_msgs(files[i], File::AOF);

                    // Reencode to BUFR
                    BinaryMessage raw(File::BUFR);
                    raw.data = exporter->to_binary(amsgs);

                    // Decode again
                    Messages bmsgs = importer->from_binary(raw);

                    normalise_encoding_quirks(amsgs, bmsgs);

                    // Compare the two dba_msg
                    notes::Collect c(cerr);
                    int diffs = amsgs.diff(bmsgs);
                    if (diffs) 
                    {
                        dballe::tests::track_different_msgs(amsgs, bmsgs, "aof-bufr");
                        dballe::tests::dump("aof-bufr", raw, "AOF reencoded to BUFR");
                    }
                    wassert(actual(diffs) == 0);
                } catch (std::exception& e) {
                    throw TestFailed(string(files[i]) + ": " + e.what());
                }

            }
        });

        // Reencode to CREX and compare
        add_method("reencode_crex", []() {
#if 0
            const char* files[] = {
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
                NULL,
            };

            for (size_t i = 0; files[i] != NULL; i++)
            {
                test_tag(files[i]);

                dba_msgs amsgs = read_test_msg(files[i], AOF);
                
                dba_rawmsg raw;
                CHECKED(dba_marshal_encode(amsgs, CREX, &raw));

                dba_msgs bmsgs;
                CHECKED(dba_marshal_decode(raw, &bmsgs));

                strip_attributes(amsgs);
                normalise_encoding_quirks(amsgs, bmsgs);
                roundtemps(amsgs);
                roundtemps(bmsgs);

                // Compare the two dba_msg
                int diffs = 0;
                dba_msgs_diff(amsgs, bmsgs, &diffs, stderr);
                if (diffs) track_different_msgs(amsgs, bmsgs, "aof-crex");
                gen_wassert(actual(diffs) == 0);

                dba_msgs_delete(amsgs);
                dba_msgs_delete(bmsgs);
                dba_rawmsg_delete(raw);
            }
            test_untag();
#endif
        });

        // Compare no-dew-point AOF plane reports with those with dew point
        add_method("dewpoint", []() {
            string prefix = "aof/aof_";
            const char* files[] = {
                "-2-144.aof",
                "-2-244.aof",
                NULL,
            };

            for (size_t i = 0; files[i] != NULL; i++)
            {
                try {
                    Messages amsgs1 = read_msgs(string(prefix + "27" + files[i]).c_str(), File::AOF);
                    Messages amsgs2 = read_msgs(string(prefix + "28" + files[i]).c_str(), File::AOF);

                    // Compare the two dba_msg
                    notes::Collect c(cerr);
                    int diffs = amsgs1.diff(amsgs2);
                    if (diffs) dballe::tests::track_different_msgs(amsgs1, amsgs2, "aof-2728");
                    wassert(actual(diffs) == 0);
                } catch (std::exception& e) {
                    throw TestFailed(prefix + "2x" + files[i] + ": " + e.what());
                }
            }
        });

        // Ensure that missing values in existing synop optional sections are caught
        // correctly
        add_method("missing", []() {
            Messages msgs = read_msgs("aof/missing-cloud-h.aof", File::AOF);
            wassert(actual(msgs.size()) == 1);

            const Msg& msg = Msg::downcast(msgs[0]);
            wassert(actual(msg.get_cloud_h1_var()).isfalse());
        });

        // Verify decoding of confidence intervals in optional ship group
        add_method("ship_confidence", []() {
            Messages msgs = read_msgs("aof/confship.aof", File::AOF);
            wassert(actual(msgs.size()) == 1);

            const Msg& msg = Msg::downcast(msgs[0]);
            const Var* var = msg.get_st_dir_var();
            wassert(actual(var).istrue());

            const Var* attr = var->enqa(WR_VAR(0, 33, 7));
            wassert(actual(attr).istrue());

            wassert(actual(attr->enqi()) == 51);
        });
    }
} test("msg_aof_codec");

}
