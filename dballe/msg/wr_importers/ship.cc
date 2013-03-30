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

class ShipImporter : public SynopBaseImporter
{
protected:
    virtual void import_var(const Var& var);

public:
    ShipImporter(const msg::Importer::Options& opts)
        : SynopBaseImporter(opts) {}
    virtual ~ShipImporter() {}

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.type)
        {
            case 1:
                switch (bulletin.localsubtype)
                {
                    case 21: return MSG_BUOY;
                    case 9:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 19: return MSG_SHIP;
                    case 0: {
                        // Guess looking at the variables
                        if (bulletin.subsets.empty())
                            throw error_consistency("trying to import a SYNOP message with no data subset");
                        const Subset& subset = bulletin.subsets[0];
                        if (subset.size() > 1 && subset[0].code() == WR_VAR(0, 1, 5))
                            return MSG_BUOY;
                        else
                            return MSG_SHIP;
                    }
                    default: return MSG_GENERIC;
                }
                break;
            default: return MSG_GENERIC; break;
        }
    }
};

void ShipImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        // Cloud data
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
        case WR_VAR(0, 11, 11): ctx.set_wind(var, DBA_MSG_WIND_DIR); break;
        case WR_VAR(0, 11,  2):
        case WR_VAR(0, 11, 12): ctx.set_wind(var, DBA_MSG_WIND_SPEED); break;
        case WR_VAR(0, 11, 43): ctx.set_wind_max(var, DBA_MSG_WIND_GUST_MAX_DIR); break;
        case WR_VAR(0, 11, 41): ctx.set_wind_max(var, DBA_MSG_WIND_GUST_MAX_SPEED); break;

        case WR_VAR(0, 22, 42): msg->set_water_temp_var(var); break;
        case WR_VAR(0, 12,  5): msg->set_wet_temp_2m_var(var); break;
        case WR_VAR(0, 10,197): msg->set_height_anem_var(var); break;

        default: SynopBaseImporter::import_var(var); break;
    }
}

} // anonynmous namespace

std::auto_ptr<Importer> Importer::createShip(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new ShipImporter(opts));
}


} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
