/*
 * dballe/wr_importers/base - Base infrastructure for wreport importers
 *
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
#include "msg/msgs.h"
#include <wreport/bulletin.h>
#include <iostream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

std::auto_ptr<Importer> Importer::createSat(const msg::Importer::Options&) { throw error_unimplemented("WB sat Importers"); }

void WMOImporter::import_var(const Var& var)
{
	switch (var.code())
	{
// General bulletin metadata
		case WR_VAR(0,  1,  1): msg->set_block_var(var); break;
		case WR_VAR(0,  1,  2): msg->set_station_var(var); break;
		case WR_VAR(0,  1,  5):
		case WR_VAR(0,  1,  6): msg->set_ident_var(var); break;
		case WR_VAR(0,  1, 11): msg->set_ident_var(var); break;
		case WR_VAR(0,  1, 12): msg->set_st_dir_var(var); break;
		case WR_VAR(0,  1, 13): msg->set_st_speed_var(var); break;
		case WR_VAR(0,  1, 63): msg->set_st_name_icao_var(var); break;
		case WR_VAR(0,  2,  1): msg->set_st_type_var(var); break;
		case WR_VAR(0,  1, 15): msg->set_st_name_var(var); break;
		case WR_VAR(0,  4,  1): msg->set_year_var(var); break;
		case WR_VAR(0,  4,  2): msg->set_month_var(var); break;
		case WR_VAR(0,  4,  3): msg->set_day_var(var); break;
		case WR_VAR(0,  4,  4): msg->set_hour_var(var); break;
		case WR_VAR(0,  4,  5): msg->set_minute_var(var); break;
		case WR_VAR(0,  4,  6): msg->set_second_var(var); break;
		case WR_VAR(0,  5,  1):
		case WR_VAR(0,  5,  2): msg->set_latitude_var(var); break;
		case WR_VAR(0,  6,  1):
		case WR_VAR(0,  6,  2): msg->set_longitude_var(var); break;
	}
}

void CloudContext::init()
{
    level = Level::cloud(MISSING_INT, MISSING_INT);
}

void CloudContext::on_vss(const wreport::Subset& subset, unsigned pos)
{
    /* Vertical significance */
    if (pos == 0) throw error_consistency("B08002 found at beginning of message");
    Varcode prev = subset[pos - 1].code();

    if (prev == WR_VAR(0, 20, 10))
    {
        // Normal cloud data
        level.ltype2 = 258;
        level.l2 = 0;
        return;
    }

    if (pos == subset.size() - 1) throw error_consistency("B08002 found at end of message");
    Varcode next = subset[pos + 1].code();

    switch (next)
    {
        case WR_VAR(0, 20, 11): {
            if (pos >= subset.size() - 3)
                throw error_consistency("B08002 followed by B20011 found less than 3 places before end of message");
            Varcode next2 = subset[pos + 3].code();
            if (next2 == WR_VAR(0, 20, 14))
            {
                // Clouds with bases below station level
                if (level.ltype2 != 263)
                {
                    level.ltype2 = 263;
                    level.l2 = 1;
                } else {
                    ++level.l2;
                }
            } else {
                /* Individual cloud groups */
                if (level.ltype2 != 259)
                {
                    level.ltype2 = 259;
                    level.l2 = 1;
                } else {
                    ++level.l2;
                }
            }
            break;
        } case WR_VAR(0, 20, 54):
            // Direction of cloud drift
            if (level.ltype2 != 260)
            {
                level.ltype2 = 260;
                level.l2 = 1;
            } else {
                ++level.l2;
            }
            break;
        default:
            break;
#if 0
            /* Vertical significance */
            if (pos == 0) throw error_consistency("B08002 found at beginning of message");
            if (pos == subset->size() - 1) throw error_consistency("B08002 found at end of message");
            Varcode prev = (*subset)[pos - 1].code();
            Varcode next = (*subset)[pos + 1].code();

            } else if (var.value() == NULL) {
                level.ltype2 = 0;
            } else {
                /* Unless we can detect known buggy situations, raise an error */
                if (next != WR_VAR(0, 20, 62))
                    error_consistency::throwf("Vertical significance %d found in unrecognised context", var.enqi());
            }
            break;
#endif
    }
}

const Level& CloudContext::clcmch()
{
    if (level.ltype2 == 258)
        ++level.l2;
    return level;
}

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
