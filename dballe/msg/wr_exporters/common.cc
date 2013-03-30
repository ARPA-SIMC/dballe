/*
 * dballe/wr_exporter/common - Common infrastructure for wreport exporters
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "common.h"
#include <wreport/subset.h>
#include <dballe/msg/context.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

void ExporterModule::init(wreport::Subset& subset)
{
    this->subset = &subset;
    c_surface_instant = 0;
    c_ana = 0;
}

void ExporterModule::scan_context(const msg::Context& c)
{
    switch (c.level.ltype1)
    {
        case 1:
            if (c.trange.pind == 254)
                c_surface_instant = &c;
            break;
        case 257: c_ana = &c; break;
    }
}

void ExporterModule::add(Varcode code, const msg::Context* ctx, int shortcut) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->find_by_id(shortcut))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add(Varcode code, const msg::Context* ctx, Varcode srccode) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->find(srccode))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add(Varcode code, const msg::Context* ctx) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->find(code))
        subset->store_variable(*var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add(wreport::Varcode code, const wreport::Var* var) const
{
    if (var)
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

int ExporterModule::get_hour()
{
    if (!c_ana) return MISSING_INT;
    const Var* v_hour = c_ana->find_by_id(DBA_MSG_HOUR);
    return v_hour ? v_hour->enqi() : MISSING_INT;
}

void ExporterModule::add_year_to_minute()
{
    add(WR_VAR(0,  4,  1), c_ana);
    add(WR_VAR(0,  4,  2), c_ana);
    add(WR_VAR(0,  4,  3), c_ana);
    add(WR_VAR(0,  4,  4), c_ana);
    add(WR_VAR(0,  4,  5), c_ana);
}

void ExporterModule::add_latlon_coarse()
{
    add(WR_VAR(0,  5,  2), c_ana, DBA_MSG_LATITUDE);
    add(WR_VAR(0,  6,  2), c_ana, DBA_MSG_LONGITUDE);
}

void ExporterModule::add_latlon_high()
{
    add(WR_VAR(0,  5,  1), c_ana, DBA_MSG_LATITUDE);
    add(WR_VAR(0,  6,  1), c_ana, DBA_MSG_LONGITUDE);
}

void ExporterModule::add_station_height()
{
    add(WR_VAR(0,  7, 30), c_ana);
    add(WR_VAR(0,  7, 31), c_ana);
}

void ExporterModule::add_station_name(wreport::Varcode code)
{
    // Append an undefined station name
    subset->store_variable_undef(code);

    if (!c_ana) return;

    if (const wreport::Var* var = c_ana->find_by_id(DBA_MSG_ST_NAME))
    {
        // Add the value with truncation
        if (var->value())
            (*subset)[subset->size() - 1].setc_truncate(var->value());
    }
}

void ExporterModule::add_D01090()
{
    add(WR_VAR(0,  1,  1), c_ana);
    add(WR_VAR(0,  1,  2), c_ana);
    add_station_name(WR_VAR(0,  1, 15));
    add(WR_VAR(0,  2,  1), c_ana);
    add_year_to_minute();
    add_latlon_high();
    add_station_height();
}

void ExporterModule::add_ecmwf_synop_head()
{
    add(WR_VAR(0,  1,  1), c_ana);
    add(WR_VAR(0,  1,  2), c_ana);
    add(WR_VAR(0,  2,  1), c_ana);
}

void ExporterModule::add_ship_head()
{
    add(WR_VAR(0,  1, 11), c_ana);
    add(WR_VAR(0,  1, 12), c_surface_instant);
    add(WR_VAR(0,  1, 13), c_surface_instant);
    add(WR_VAR(0,  2,  1), c_ana);
}

void ExporterModule::add_D01093()
{
    add_ship_head();
    add_year_to_minute();
    add_latlon_coarse();
    add_station_height();
}

void CommonSynopExporter::init(wreport::Subset& subset)
{
    ExporterModule::init(subset);
    c_geopotential = 0;
    v_press = 0;
    v_pressmsl = 0;
    v_pchange3 = 0;
    v_pchange24 = 0;
    v_ptend = 0;
    v_geopotential = 0;
}

void CommonSynopExporter::scan_context(const msg::Context& c)
{
    ExporterModule::scan_context(c);
    switch (c.level.ltype1)
    {
        case 1:
            switch (c.trange.pind)
            {
                case 4:
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS_3H))
                        switch (c.trange.p2)
                        {
                            case  3*3600: v_pchange3 = v; break;
                            case 24*3600: v_pchange24 = v; break;
                        }
                    break;
                case 205:
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS_TEND))
                        v_ptend = v;
                    break;
                case 254:
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS))
                        v_press = v;
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS_MSL))
                        v_pressmsl = v;
                    break;
            }
            break;
        case 100:
            // Look for geopotential
            if (const Var* v = c.find(WR_VAR(0, 10, 8)))
            {
                c_geopotential = &c;
                v_geopotential = v;
            }
            break;
        case 101:
        case 102:
            switch (c.trange.pind)
            {
                case 4:
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS_3H))
                        switch (c.trange.p2)
                        {
                            case  3*3600: v_pchange3 = v; break;
                            case 24*3600: v_pchange24 = v; break;
                        }
                    break;
                case 205:
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS_TEND))
                        v_ptend = v;
                    break;
                case 254:
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS))
                        v_press = v;
                    if (const Var* v = c.find_by_id(DBA_MSG_PRESS_MSL))
                        v_pressmsl = v;
                    break;
            }
            break;
    }
};

void CommonSynopExporter::add_D02001()
{
    add(WR_VAR(0, 10,  4), v_press);
    add(WR_VAR(0, 10, 51), v_pressmsl);
    add(WR_VAR(0, 10, 61), v_pchange3);
    add(WR_VAR(0, 10, 63), v_ptend);
}

void CommonSynopExporter::add_D02031()
{
    add_D02001();
    add(WR_VAR(0, 10, 62), v_pchange24);
    add_pressure();
    add_geopotential(WR_VAR(0, 10,  9));
}

void CommonSynopExporter::add_pressure()
{
    if (c_geopotential)
        subset->store_variable_d(WR_VAR(0, 7, 4), c_geopotential->level.l1);
    else
        subset->store_variable_undef(WR_VAR(0,  7,  4));
}

void CommonSynopExporter::add_geopotential(wreport::Varcode code)
{
    add(code, v_geopotential);
}

}
}
}
