#include "tests.h"
#include "wr_codec.h"
#include "msg.h"
#include <wreport/notes.h>
#include <wreport/bulletin.h>

using namespace std;
using namespace wreport;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("empty", []() {
            // Try encoding and decoding an empty generic message
            unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR);
            unique_ptr<msg::Exporter> exporter = msg::Exporter::create(File::BUFR);

            Messages msgs;
            msgs.append(unique_ptr<Message>(new Msg));

            // Export msg as a generic message
            BinaryMessage raw(File::BUFR);
            raw.data = wcallchecked(exporter->to_binary(msgs));

            // Parse it back
            Messages msgs1 = wcallchecked(importer->from_binary(raw));

            // Check that the data are the same
            notes::Collect c(cerr);
            int diffs = msgs.diff(msgs1);
            if (diffs) dballe::tests::track_different_msgs(msgs, msgs1, "genericempty");
            wassert(actual(diffs) == 0);
        });
        add_method("known", []() {
            // Try encoding and decoding a generic message
            unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR);
            unique_ptr<msg::Exporter> exporter = msg::Exporter::create(File::BUFR);

            unique_ptr<Msg> msg(new Msg);

            /* Fill up msg */
            msg->set_press(			15,	45);
            msg->set_height_anem(	15,	45);
            msg->set_tot_snow(		15,	45);
            msg->set_visibility(		15,	45);
            msg->set_pres_wtr(		 5,	45);
            msg->set_metar_wtr(		 5,	45);
            msg->set_water_temp(		15,	45);
            msg->set_past_wtr1_3h(	 2,	45);
            msg->set_past_wtr2_3h(	 2,	45);
            msg->set_press_tend(		 5,	45);
            msg->set_tot_prec24(		15,	45);
            msg->set_press_3h(		15,	45);
            msg->set_press_msl(		15,	45);
            msg->set_qnh(			15,	45);
            msg->set_temp_2m(		15,	45);
            msg->set_wet_temp_2m(	15,	45);
            msg->set_dewpoint_2m(	15,	45);
            msg->set_humidity(		15,	45);
            msg->set_wind_dir(		15,	45);
            msg->set_wind_speed(		15,	45);
            msg->set_ex_ccw_wind(	15,	45);
            msg->set_ex_cw_wind(		15,	45);
            msg->set_wind_gust_max_speed(	15,	45);
            msg->set_cloud_n(		3,		45);
            msg->set_cloud_nh(		10,	45);
            msg->set_cloud_hh(		3,		45);
            msg->set_cloud_cl(		3,		45);
            msg->set_cloud_cm(		3,		45);
            msg->set_cloud_ch(		3,		45);
            msg->set_cloud_n1(		3,		45);
            msg->set_cloud_c1(		3,		45);
            msg->set_cloud_h1(		3,		45);
            msg->set_cloud_n2(		3,		45);
            msg->set_cloud_c2(		3,		45);
            msg->set_cloud_h2(		3,		45);
            msg->set_cloud_n3(		3,		45);
            msg->set_cloud_c3(		3,		45);
            msg->set_cloud_h3(		3,		45);
            msg->set_cloud_n4(		3,		45);
            msg->set_cloud_c4(		3,		45);
            msg->set_cloud_h4(		3,		45);
            msg->set_block(			3,		45);
            msg->set_station(		3,		45);
            msg->set_flight_reg_no(	"pippo", 45);
            msg->set_ident(			"cippo", 45);
            msg->set_st_dir(			3,		45);
            msg->set_st_speed(		3,		45);
            msg->set_st_name(		"ciop", 45);
            msg->set_st_name_icao(	"cip", 45);
            msg->set_st_type(		1,		45);
            msg->set_wind_inst(		3,		45);
            msg->set_temp_precision(	1.23,	45);
            msg->set_sonde_type(		3,		45);
            msg->set_sonde_method(	3,		45);
            msg->set_navsys(			3,		45);
            msg->set_data_relay(		3,		45);
            msg->set_flight_roll(	3,		45);
            msg->set_latlon_spec(	3,		45);
            msg->set_datetime(Datetime(3, 3, 3, 3, 3, 0));
            msg->seti(WR_VAR(0, 4, 1), 3, 45, Level(), Trange());
            msg->seti(WR_VAR(0, 4, 2), 3, 45, Level(), Trange());
            msg->seti(WR_VAR(0, 4, 3), 3, 45, Level(), Trange());
            msg->seti(WR_VAR(0, 4, 4), 3, 45, Level(), Trange());
            msg->seti(WR_VAR(0, 4, 5), 3, 45, Level(), Trange());
            msg->set_latitude(		3,		45);
            msg->set_longitude(		3,		45);
            msg->set_height_station(3,		45);
            msg->set_height_baro(	3,		45);
            msg->set_flight_phase(	3,		45);
            msg->set_timesig(		3,		45);
            //CHECKED(dba_msg_set_flight_press(	msg, 3,		45));

            Messages msgs;
            msgs.append(move(msg));

            /* Export msg as a generic message */
            BinaryMessage raw(File::BUFR);
            raw.data = wcallchecked(exporter->to_binary(msgs));

            //FILE* out = fopen("/tmp/zaza.bufr", "wb");
            //fwrite(raw.data.data(), raw.data.size(), 1, out);
            //fclose(out);

            /* Parse it back */
            Messages msgs1 = wcallchecked(importer->from_binary(raw));

            /* Check that the data are the same */
            notes::Collect c(cerr);
            int diffs = msgs.diff(msgs1);
            if (diffs) dballe::tests::track_different_msgs(msgs, msgs1, "generic2");
            wassert(actual(diffs) == 0);
        });
        add_method("attrs", []() {
            // Check that attributes are properly exported
            unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR);
            unique_ptr<msg::Exporter> exporter = msg::Exporter::create(File::BUFR);

            /* Create a new message */
            unique_ptr<Msg> msg(new Msg);
            msg->type = MSG_GENERIC;

            // Set some metadata
            msg->set_datetime(Datetime(2006, 1, 19, 14, 50));
            msg->set_latitude(50.0);
            msg->set_longitude(12.0);

            /* Create a variable to add to the message */
            unique_ptr<Var> var = newvar(WR_VAR(0, 12, 101), 270.15);

            /* Add some attributes to the variable */
            var->seta(newvar(WR_VAR(0, 33, 2), 1));
            var->seta(newvar(WR_VAR(0, 33, 3), 2));
            var->seta(newvar(WR_VAR(0, 33, 5), 3));

            /* Add the variable to the message */
            msg->set(move(var), Level(1), Trange::instant());

            /* Create a second variable to add to the message */
            var = newvar(WR_VAR(0, 12, 102), 272.0);

            /* Add some attributes to the variable */
            var->seta(newvar(WR_VAR(0, 33, 3), 1));
            var->seta(newvar(WR_VAR(0, 33, 5), 2));

            /* Add the variable to the message */
            msg->set(move(var), Level(1), Trange::instant());

            Messages msgs;
            msgs.append(move(msg));

            // Encode the message
            BinaryMessage raw(File::BUFR);
            raw.data = wcallchecked(exporter->to_binary(msgs));

            // Decode the message
            Messages msgs1 = wcallchecked(importer->from_binary(raw));

            // Check that the data are the same
            notes::Collect c(cerr);
            int diffs = msgs.diff(msgs1);
            if (diffs) dballe::tests::track_different_msgs(msgs, msgs1, "genericattr");
            wassert(actual(diffs) == 0);
        });
        add_method("doublememo", []() {
            // Test a bug in which B01194 ([SIM] Report mnemonic) appears twice

            // Import a synop message
            Messages msgs = read_msgs("bufr/obs0-1.22.bufr", File::BUFR);
            wassert(actual(msgs.size()) > 0);

            // Convert it to generic, with a 'ship' rep_memo
            Msg::downcast(msgs[0]).type = MSG_GENERIC;
            Msg::downcast(msgs[0]).set_rep_memo("ship");

            // Export it
            unique_ptr<msg::Exporter> exporter = msg::Exporter::create(File::BUFR);
            unique_ptr<Bulletin> bulletin = exporter->to_bulletin(msgs);

            // Ensure that B01194 only appears once
            wassert(actual(bulletin->subsets.size()) == 1u);
            unsigned count = 0;
            for (std::vector<wreport::Var>::const_iterator i = bulletin->subsets[0].begin(); i != bulletin->subsets[0].end(); ++i)
            {
                if (i->code() == WR_VAR(0, 1, 194))
                    ++count;
            }
            wassert(actual(count) == 1u);
        });
    }
} test("msg_wr_codec_generic");

}
