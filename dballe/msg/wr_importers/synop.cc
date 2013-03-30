/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "base.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

namespace {

static const Level lev_ground(1);
static const Level lev_std_wind(103, 10*1000);
static const Trange tr_std_wind(200, 0, 600);
static const Trange tr_std_wind_max10m(205, 0, 600);

class SynopImporter : public WMOImporter
{
protected:
    CloudContext clouds;
    LevelContext level;
    TimerangeContext trange;
    ContextChooser ctx;

    void peek_var(const Var& var);
    void import_var(const Var& var);

public:
    SynopImporter(const msg::Importer::Options& opts)
        : WMOImporter(opts), ctx(level, trange) {}
    virtual ~SynopImporter() {}

    virtual void init()
    {
        WMOImporter::init();
        clouds.init();
        level.init();
        trange.init();
        ctx.init(*msg, opts.simplified);
    }

    virtual void run()
    {
        for (pos = 0; pos < subset->size(); ++pos)
        {
            const Var& var = (*subset)[pos];
            if (WR_VAR_F(var.code()) != 0) continue;
            if (WR_VAR_X(var.code()) < 10) peek_var(var);
            if (var.isset()) import_var(var);
        }
    }

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.type)
        {
            case 0: return MSG_SYNOP;
            default: return MSG_GENERIC; break;
        }
    }
};

void SynopImporter::peek_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  4,  4):
        case WR_VAR(0,  4, 24):
        case WR_VAR(0,  4, 25):
        case WR_VAR(0,  8, 21): trange.peek_var(var, pos); break;
        case WR_VAR(0,  8,  2): clouds.on_vss(*subset, pos); break;
        case WR_VAR(0,  7,  4):
        case WR_VAR(0,  7, 31):
        case WR_VAR(0,  7, 32): level.peek_var(var); break;
    }
}

void SynopImporter::import_var(const Var& var)
{
    switch (var.code())
    {
/* Context items */
        case WR_VAR(0,  8,  2):
            /* Store original VS value as a measured value */
            msg->set(var, WR_VAR(0, 8, 2), clouds.level, Trange::instant());
            break;
/* Fixed surface station identification, time, horizontal and vertical
 * coordinates (complete) */
        case WR_VAR(0,  7,  1):
        case WR_VAR(0,  7, 30): msg->set_height_station_var(var); break;
        /* case WR_VAR(0,  7,  4): DBA_RUN_OR_RETURN(dba_msg_set_isobaric_surface_var(msg, var)); break; */
        case WR_VAR(0,  7, 31):
            /* Store also in the ana level, so that if the
             * pressure later is missing we still have
             * access to the value */
            msg->set_height_baro_var(var);
            break;

/* Pressure data (complete) */
        case WR_VAR(0, 10,  4): ctx.set_baro_sensor(var, DBA_MSG_PRESS); break;
        case WR_VAR(0, 10, 51): ctx.set_baro_sensor(var, DBA_MSG_PRESS_MSL); break;
        case WR_VAR(0, 10, 61): ctx.set_baro_sensor(var, DBA_MSG_PRESS_3H); break;
        case WR_VAR(0, 10, 62): ctx.set_baro_sensor(var, DBA_MSG_PRESS_24H); break;
        case WR_VAR(0, 10, 63): ctx.set_baro_sensor(var, DBA_MSG_PRESS_TEND); break;
        case WR_VAR(0, 10,  3):
        case WR_VAR(0, 10,  8):
        case WR_VAR(0, 10,  9): ctx.set_pressure(var); break;

        /* Legacy bits */
/* Basic synoptic "instantaneous" data */

/* Temperature and humidity data (complete) */
        case WR_VAR(0, 12,   4):
        case WR_VAR(0, 12, 101): ctx.set_gen_sensor(var, DBA_MSG_TEMP_2M); break;
        case WR_VAR(0, 12,   6):
        case WR_VAR(0, 12, 103): ctx.set_gen_sensor(var, DBA_MSG_DEWPOINT_2M); break;
        case WR_VAR(0, 13,   3): ctx.set_gen_sensor(var, DBA_MSG_HUMIDITY); break;

/* Visibility data (complete) */
        case WR_VAR(0, 20,  1): ctx.set_gen_sensor(var, DBA_MSG_VISIBILITY); break;

/* Precipitation past 24h (complete) */
        case WR_VAR(0, 13, 19): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC1); break;
        case WR_VAR(0, 13, 20): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC3); break;
        case WR_VAR(0, 13, 21): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC6); break;
        case WR_VAR(0, 13, 22): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC12); break;
        case WR_VAR(0, 13, 23): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC24); break;

/* Cloud data */
        case WR_VAR(0, 20, 10): msg->set_cloud_n_var(var); break;

/* Individual cloud layers or masses (complete) */
/* Clouds with bases below station level (complete) */
/* Direction of cloud drift (complete) */
        case WR_VAR(0, 20, 11):
        case WR_VAR(0, 20, 13):
        case WR_VAR(0, 20, 17):
        case WR_VAR(0, 20, 54): msg->set(var, var.code(), clouds.level, Trange::instant()); break;
        case WR_VAR(0, 20, 12): // CH CL CM
            msg->set(var, WR_VAR(0, 20, 12), clouds.clcmch(), Trange::instant());
            break;
/* Direction and elevation of cloud (complete) */
        case WR_VAR(0, 5, 21): msg->set(var, WR_VAR(0, 5, 21), Level::cloud(262, 0), Trange::instant()); break;
        case WR_VAR(0, 7, 21): msg->set(var, WR_VAR(0, 7, 21), Level::cloud(262, 0), Trange::instant()); break;
        /* Cloud type is handled by the generic cloud type handler */

/* State of ground, snow depth, ground minimum temperature (complete) */
        case WR_VAR(0, 20,  62): msg->set_state_ground_var(var); break;
        case WR_VAR(0, 13,  13): msg->set_tot_snow_var(var); break;
        case WR_VAR(0, 12, 113): msg->set(var, WR_VAR(0, 12, 121), Level(1), Trange(3, 0, 43200)); break;

/* Basic synoptic "period" data */

/* Present and past weather (complete) */
        case WR_VAR(0, 20,  3): msg->set_pres_wtr_var(var); break;
        case WR_VAR(0, 20,  4): ctx.set_past_weather(var, DBA_MSG_PAST_WTR1_6H); break;
        case WR_VAR(0, 20,  5): ctx.set_past_weather(var, DBA_MSG_PAST_WTR2_6H); break;

/* Sunshine data (complete) */
        case WR_VAR(0, 14, 31): msg->set(var, WR_VAR(0, 14, 31), Level(1), Trange(1, 0, abs(trange.time_period))); break;

/* Precipitation measurement (complete) */
        case WR_VAR(0, 13, 11): ctx.set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1), Trange(1, 0, abs(trange.time_period))); break;

/* Extreme temperature data */
        case WR_VAR(0, 12, 111):
            ctx.set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(2, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;
        case WR_VAR(0, 12, 112):
            ctx.set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(3, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;

/* Wind data (complete) */
        case WR_VAR(0, 2, 2): msg->set_wind_inst_var(var); break;

        /* Note B/C 1.10.5.3.2 Calm shall be reported by
         * setting wind direction to 0 and wind speed to 0.
         * Variable shall be reported by setting wind direction
         * to 0 and wind speed to a positive value, not a
         * missing value indicator.
         */
        case WR_VAR(0, 11,  1):
        case WR_VAR(0, 11, 11):
            if (trange.time_sig != MISSING_TIME_SIG && trange.time_sig != 2)
                    error_consistency::throwf("Found unsupported time significance %d for wind direction", trange.time_sig);
            ctx.set_gen_sensor(var, DBA_MSG_WIND_DIR, lev_std_wind, tr_std_wind);
            break;
        case WR_VAR(0, 11,  2):
        case WR_VAR(0, 11, 12):
            if (trange.time_sig != MISSING_TIME_SIG && trange.time_sig != 2)
                error_consistency::throwf("Found unsupported time significance %d for wind speed", trange.time_sig);
            ctx.set_gen_sensor(var, DBA_MSG_WIND_SPEED, lev_std_wind, tr_std_wind);
            break;
        case WR_VAR(0, 11, 43): ctx.set_gen_sensor(var, DBA_MSG_WIND_GUST_MAX_DIR, lev_std_wind, tr_std_wind_max10m, false, true); break;
        case WR_VAR(0, 11, 41): ctx.set_gen_sensor(var, DBA_MSG_WIND_GUST_MAX_SPEED, lev_std_wind, tr_std_wind_max10m, false, true); break;

/* Evaporation data */
        case WR_VAR(0, 2, 4): msg->set(var, WR_VAR(0, 2, 4), Level(1), Trange::instant()); break;
        case WR_VAR(0, 13, 33):
            if (trange.time_period == MISSING_INT)
                msg->set(var, WR_VAR(0, 13, 33), Level(1), Trange(1));
            else
                msg->set(var, WR_VAR(0, 13, 33), Level(1), Trange(1, 0, abs(trange.time_period)));
            break;

/* Radiation data */
        case WR_VAR(0, 14, 2):
            throw error_unimplemented("wow, a synop with radiation info, please give it to Enrico");

/* Temperature change */
        case WR_VAR(0, 12, 49):
            throw error_unimplemented("wow, a synop with temperature change info, please give it to Enrico");

        case WR_VAR(0, 22, 42): msg->set_water_temp_var(var); break;
        case WR_VAR(0, 12,  5): msg->set_wet_temp_2m_var(var); break;
        case WR_VAR(0, 10,197): msg->set_height_anem_var(var); break;

        default: WMOImporter::import_var(var); break;
    }
}

} // anonynmous namespace

std::auto_ptr<Importer> Importer::createSynop(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new SynopImporter(opts));
}


} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
