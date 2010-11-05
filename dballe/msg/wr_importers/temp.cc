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

#include "base.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <dballe/msg/context.h>

using namespace wreport;
using namespace std;

#define MISSING_PRESS -1.0

namespace dballe {
namespace msg {
namespace wr {

class TempImporter : public WMOImporter
{
protected:
    double press;
    const Var* press_var;
    double surface_press;
    const Var* surface_press_var;

    void import_var(const Var& var);

public:
    TempImporter(const msg::Importer::Options& opts) : WMOImporter(opts) {}
    virtual ~TempImporter() {}

    virtual void init()
    {
        WMOImporter::init();

        press = MISSING_PRESS;
        press_var = NULL;
        surface_press = MISSING_PRESS;
        surface_press_var = NULL;
    }

    virtual void run()
    {
        for (pos = 0; pos < subset->size(); ++pos)
        {
                const Var& var = (*subset)[pos];
                if (WR_VAR_F(var.code()) != 0) continue;
                if (var.value() != NULL)
                        import_var(var);
        }

        /* Extract surface data from the surface level */
        if (surface_press != -1)
        {
                // Pressure is taken from a saved variable referencing to the original
                // pressure data in the message, to preserve data attributes
                if (surface_press_var && surface_press_var->value() && msg->get_press_var())
                        msg->set_press_var(*surface_press_var);

                const Context* sfc = msg->find_context(Level(100, surface_press), Trange::instant());
                if (sfc != NULL)
                {
                        const Var* var = sfc->find(WR_VAR(0, 12, 1));
                        if (var && msg->get_temp_2m_var())
                                msg->set_temp_2m_var(*var);

                        var = sfc->find(WR_VAR(0, 12, 3));
                        if (var && msg->get_dewpoint_2m_var())
                                msg->set_dewpoint_2m_var(*var);

                        var = sfc->find(WR_VAR(0, 11, 1));
                        if (var && !msg->get_wind_dir_var())
                                msg->set_wind_dir_var(*var);

                        var = sfc->find(WR_VAR(0, 11, 2));
                        if (var && !msg->get_wind_speed_var())
                                msg->set_wind_speed_var(*var);
                }
        }
    }

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.subtype)
        {
            case 4: return MSG_TEMP;
            case 255:
                switch (bulletin.localsubtype)
                {
                    case 0: {
                        /* Guess looking at the variables */
                        if (bulletin.subsets.empty())
                                throw error_consistency("trying to import a SYNOP message with no data subset");
                        const Subset& subset = bulletin.subsets[0];
                        if (subset.size() > 1 && subset[0].code() == WR_VAR(0, 1, 11))
                            return MSG_TEMP_SHIP;
                        else
                            return MSG_TEMP;
                    }
                    case 101: return MSG_TEMP;
                    case 92:
                    case 102: return MSG_TEMP_SHIP;
                }
        }
        return MSG_TEMP;
    }
};

std::auto_ptr<Importer> Importer::createTemp(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new TempImporter(opts));
}

void TempImporter::import_var(const Var& var)
{
    switch (var.code())
    {
/* Identification of launch site and instrumentation */
        case WR_VAR(0,  2,  3): msg->set_meas_equip_type_var(var); break;
        case WR_VAR(0,  2, 11): msg->set_sonde_type_var(var); break;
        case WR_VAR(0,  2, 12): msg->set_sonde_method_var(var); break;
        case WR_VAR(0,  2, 13): msg->set_sonde_correction_var(var); break;
        case WR_VAR(0,  2, 14): msg->set_sonde_tracking_var(var); break;
/* Date/time of launch */
        case WR_VAR(0,  8, 21):
            if (var.enqi() != 18)
                    error_consistency::throwf("TEMP time significance is %d instead of 18", var.enqi());
            break;
/* Horizontal and vertical coordinates of launch site */
        case WR_VAR(0,  7,  1): msg->set_height_var(var); break;
        case WR_VAR(0,  7, 30): msg->set_height_var(var); break;
        case WR_VAR(0,  7, 31): msg->set_height_baro_var(var); break;
        case WR_VAR(0,  7,  7): msg->set_height_release_var(var); break;
        case WR_VAR(0, 33, 24): msg->set_station_height_quality_var(var); break;
/* Cloud information reported with vertical soundings */
        case WR_VAR(0,  8,  2): msg->set(var, WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        case WR_VAR(0, 20, 10): msg->set_cloud_n_var(var); break;
        case WR_VAR(0, 20, 11): msg->set_cloud_nh_var(var); break;
        case WR_VAR(0, 20, 13): msg->set_cloud_hh_var(var); break;
        case WR_VAR(0, 20, 12): { // CH CL CM
            int l2 = 1;
            if (pos > 0 && (*subset)[pos - 1].code() == WR_VAR(0, 20, 12))
            {
                ++l2;
                if (pos > 1 && (*subset)[pos - 2].code() == WR_VAR(0, 20, 12))
                    ++l2;
            }
            msg->set(var, WR_VAR(0, 20, 12), Level::cloud(258, l2), Trange::instant());
            break;
        }
/* Temperature, dew-point and wind data at pressure levels */
        // Long time period or displacement (since launch time)
        case WR_VAR(0,  4, 86): msg->set(var, WR_VAR(0, 4, 86), Level(100, press), Trange::instant()); break;
        // Extended vertical sounding significance
        case WR_VAR(0,  8, 42): {
                if (pos == subset->size() - 1) throw error_consistency("B08042 found at end of message");
                if ((*subset)[pos + 1].code() == WR_VAR(0, 7, 4))
                {
                    // Pressure is reported later, we need to look ahead to compute the right level
                    press_var = &((*subset)[pos + 1]);
                    press = press_var->enqd();
                }
                msg->set(var, WR_VAR(0, 8, 42), Level(100, press), Trange::instant());
                break;
        }
        // Pressure
        case WR_VAR(0,  7,  4):
                press = var.enqd();
                press_var = &var;
                msg->set(var, WR_VAR(0, 10, 4), Level(100, press), Trange::instant());
                break;
        // Vertical sounding significance
        case WR_VAR(0,  8,  1):
                msg->set(var, WR_VAR(0,  8, 1), Level(100, press), Trange::instant());
                if (var.enqi() & 64)
                {
                        surface_press = press;
                        surface_press_var = press_var;
                }
                break;
        // Geopotential
        case WR_VAR(0, 10,  3): msg->set(var, WR_VAR(0, 10, 8), Level(100, press), Trange::instant()); break;
        case WR_VAR(0, 10,  8): msg->set(var, WR_VAR(0, 10, 8), Level(100, press), Trange::instant()); break;
        // Latitude displacement
        case WR_VAR(0,  5, 15): msg->set(var, WR_VAR(0, 5, 15), Level(100, press), Trange::instant()); break;
        // Longitude displacement
        case WR_VAR(0,  6, 15): msg->set(var, WR_VAR(0, 6, 15), Level(100, press), Trange::instant()); break;
        // Dry bulb temperature
        case WR_VAR(0, 12,  1): msg->set(var, WR_VAR(0, 12, 101), Level(100, press), Trange::instant()); break;
        case WR_VAR(0, 12, 101): msg->set(var, WR_VAR(0, 12, 101), Level(100, press), Trange::instant()); break;
        // Wet bulb temperature
        case WR_VAR(0, 12,  2): msg->set(var, WR_VAR(0, 12, 2), Level(100, press), Trange::instant()); break;
        // Dew point temperature
        case WR_VAR(0, 12,  3): msg->set(var, WR_VAR(0, 12, 103), Level(100, press), Trange::instant()); break;
        case WR_VAR(0, 12, 103): msg->set(var, WR_VAR(0, 12, 103), Level(100, press), Trange::instant()); break;
        // Wind direction
        case WR_VAR(0, 11,  1): msg->set(var, WR_VAR(0, 11, 1), Level(100, press), Trange::instant()); break;
        // Wind speed
        case WR_VAR(0, 11,  2): msg->set(var, WR_VAR(0, 11, 2), Level(100, press), Trange::instant()); break;
/* Wind shear data at a pressure level */
        case WR_VAR(0, 11, 61): msg->set(var, WR_VAR(0, 11, 61), Level(100, press), Trange::instant()); break;
        case WR_VAR(0, 11, 62): msg->set(var, WR_VAR(0, 11, 62), Level(100, press), Trange::instant()); break;
        default:
                WMOImporter::import_var(var);
                break;
    }
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
