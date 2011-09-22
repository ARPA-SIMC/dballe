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
#include "msgs.h"
#include <wreport/bulletin.h>

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

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
