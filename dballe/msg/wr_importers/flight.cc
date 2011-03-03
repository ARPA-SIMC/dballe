/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

class FlightImporter : public WMOImporter
{
protected:
    Level lev;

    void import_var(const Var& var);

public:
    FlightImporter(const msg::Importer::Options& opts) : WMOImporter(opts) {}
    virtual ~FlightImporter() {}

    virtual void init()
    {
        WMOImporter::init();
        lev = Level();
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
    }

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.localsubtype)
        {
            case 142: return MSG_AIREP;
            case 144: return MSG_AMDAR;
            case 145: return MSG_ACARS;
            default: return MSG_GENERIC;
        }
    }
};

std::auto_ptr<Importer> Importer::createFlight(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new FlightImporter(opts));
}

void FlightImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  2, 61): msg->set_navsys_var(var); break;
        case WR_VAR(0,  2, 62): msg->set_data_relay_var(var); break;
        case WR_VAR(0,  2,  2): msg->set_wind_inst_var(var); break;
        case WR_VAR(0,  2,  5): msg->set_temp_precision_var(var); break;
        case WR_VAR(0,  2, 70): msg->set_latlon_spec_var(var); break;
        case WR_VAR(0,  2, 63): msg->set_flight_roll_var(var); break;
        case WR_VAR(0,  2,  1): msg->set_st_type_var(var); break;
        case WR_VAR(0,  8,  4): msg->set_flight_phase_var(var); break;
        case WR_VAR(0,  8, 21): msg->set_timesig_var(var); break;
        case WR_VAR(0,  7,  2):
            lev = Level(102, var.enqd());
            msg->set(var, WR_VAR(0,  7, 30), lev, Trange::instant());
            break;
        case WR_VAR(0,  7,  4):
            lev = Level(100, var.enqd());
            msg->set(var, WR_VAR(0, 10,  4), lev, Trange::instant());
            break;
        case WR_VAR(0, 11,  1): msg->set(var, WR_VAR(0, 11,   1), lev, Trange::instant()); break;
        case WR_VAR(0, 11,  2): msg->set(var, WR_VAR(0, 11,   2), lev, Trange::instant()); break;
        case WR_VAR(0, 11, 31): msg->set(var, WR_VAR(0, 11,  31), lev, Trange::instant()); break;
        case WR_VAR(0, 11, 32): msg->set(var, WR_VAR(0, 11,  32), lev, Trange::instant()); break;
        case WR_VAR(0, 11, 33): msg->set(var, WR_VAR(0, 11,  33), lev, Trange::instant()); break;
        case WR_VAR(0, 11, 34): msg->set(var, WR_VAR(0, 11,  34), lev, Trange::instant()); break;
        case WR_VAR(0, 11, 35): msg->set(var, WR_VAR(0, 11,  35), lev, Trange::instant()); break;
        case WR_VAR(0, 12,  1): msg->set(var, WR_VAR(0, 12, 101), lev, Trange::instant()); break;
        case WR_VAR(0, 12,  3): msg->set(var, WR_VAR(0, 12, 103), lev, Trange::instant()); break;
        case WR_VAR(0, 13,  3): msg->set(var, WR_VAR(0, 13,   3), lev, Trange::instant()); break;
        case WR_VAR(0, 20, 41): msg->set(var, WR_VAR(0, 20,  41), lev, Trange::instant()); break;
        default:
            WMOImporter::import_var(var);
            break;
    }
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe


/* vim:set ts=4 sw=4: */
