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

#define MISSING_SENSOR_H -10000

class MetarImporter : public WMOImporter
{
protected:
    double height_sensor;

    void peek_var(const Var& var);
    void import_var(const Var& var);

    void set_gen_sensor(const Var& var, Varcode code, const Level& defaultLevel, const Trange& trange)
    {
        if (height_sensor == MISSING_SENSOR_H || defaultLevel == Level(103, height_sensor * 1000))
            msg->set(var, code, defaultLevel, trange);
        else if (opts.simplified)
        {
            Var var1(var);
            var1.seta(newvar(WR_VAR(0, 7, 32), height_sensor));
            msg->set(var1, code, defaultLevel, trange);
        } else
            msg->set(var, code, Level(103, height_sensor * 1000), trange);
    }

    void set_gen_sensor(const Var& var, int shortcut)
    {
        const MsgVarShortcut& v = shortcutTable[shortcut];
        set_gen_sensor(var, v.code, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
    }

public:
    MetarImporter(const msg::Importer::Options& opts) : WMOImporter(opts) {}
    virtual ~MetarImporter() {}

    virtual void init()
    {
        WMOImporter::init();
        height_sensor = MISSING_SENSOR_H;
    }

    virtual void run()
    {
        for (pos = 0; pos < subset->size(); ++pos)
        {
            const Var& var = (*subset)[pos];
            if (WR_VAR_F(var.code()) != 0) continue;
            if (WR_VAR_X(var.code()) < 10)
                peek_var(var);
            if (var.isset())
                import_var(var);
        }
    }

    MsgType scanType(const Bulletin& bulletin) const { return MSG_METAR; }
};

std::unique_ptr<Importer> Importer::createMetar(const msg::Importer::Options& opts)
{
    return unique_ptr<Importer>(new MetarImporter(opts));
}

void MetarImporter::peek_var(const Var& var)
{
    switch (var.code())
    {
        // Context items
        case WR_VAR(0,  7,  6):
            if (var.isset())
                height_sensor = var.enqi();
            else
                height_sensor = MISSING_SENSOR_H;
            break;
    }
}

void MetarImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  7,  1): msg->set_height_station_var(var); break;
        case WR_VAR(0, 11,  1): set_gen_sensor(var, DBA_MSG_WIND_DIR); break;
        case WR_VAR(0, 11, 16): set_gen_sensor(var, DBA_MSG_EX_CCW_WIND); break;
        case WR_VAR(0, 11, 17): set_gen_sensor(var, DBA_MSG_EX_CW_WIND); break;
        case WR_VAR(0, 11,  2): set_gen_sensor(var, DBA_MSG_WIND_SPEED); break;
        case WR_VAR(0, 11, 41): set_gen_sensor(var, DBA_MSG_WIND_SPEED); break;
        case WR_VAR(0, 12,  1): set_gen_sensor(var, DBA_MSG_TEMP_2M); break;
        case WR_VAR(0, 12,  3): set_gen_sensor(var, DBA_MSG_DEWPOINT_2M); break;
        case WR_VAR(0, 10, 52): set_gen_sensor(var, DBA_MSG_QNH); break;
        case WR_VAR(0, 20,  9): set_gen_sensor(var, DBA_MSG_METAR_WTR); break;
        default:
            WMOImporter::import_var(var);
            break;
    }
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
