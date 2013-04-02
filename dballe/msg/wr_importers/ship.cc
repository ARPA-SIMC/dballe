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
                    default: return MSG_SHIP;
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
        // Icing and ice
        // TODO: check levels and time ranges
        case WR_VAR(0, 20, 31):
        case WR_VAR(0, 20, 32):
        case WR_VAR(0, 20, 33):
        case WR_VAR(0, 20, 34):
        case WR_VAR(0, 20, 35):
        case WR_VAR(0, 20, 36):
        case WR_VAR(0, 20, 37):
        case WR_VAR(0, 20, 38): msg->set(var, var.code(), Level(1), Trange::instant()); break;

        // Ship marine data
        // TODO: check levels and time ranges
        case WR_VAR(0,  2, 38): msg->set(var, var.code(), Level::ana(), Trange::ana()); break;
        case WR_VAR(0,  2, 39): msg->set(var, var.code(), Level::ana(), Trange::ana()); break;
        // TODO: check levels and time ranges
        case WR_VAR(0, 22, 42):
        case WR_VAR(0, 22, 43): ctx.set_water_temperature(var); break;

        // Waves
        // TODO: check levels and time ranges
        case WR_VAR(0, 22,  1):
        case WR_VAR(0, 22, 11):
        case WR_VAR(0, 22, 21):
        case WR_VAR(0, 22,  2):
        case WR_VAR(0, 22, 12):
        case WR_VAR(0, 22, 22): msg->set(var, var.code(), Level(1), Trange::instant()); break;
            break;

        // D03023 swell waves (2 grups)
        case WR_VAR(0, 22,  3): // Direction of swell waves
        case WR_VAR(0, 22, 13): // Period of swell waves
        case WR_VAR(0, 22, 23): // Height of swell waves
            ctx.set_swell_waves(var);
            break;

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
