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

#include "common.h"

namespace dballe {
namespace msg {

void AOFImporter::read_dribu(const uint32_t* obs, int obs_len, Msg& msg)
{
	msg.type = MSG_BUOY;

	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_dir, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_speed, 0));
	msg.set_st_type(0, -1);

	/* 08 Latitude */
	/* 09 Longitude */
	/* 10 Observation date */
	/* 12 Exact time of observation */
	parse_lat_lon_datetime(obs, msg);

	/* 13,14 station ID */
	parse_st_ident(obs, msg);

	if (OBS(20) != AOF_UNDEF)
		msg.set_press_msl((double)OBS(20) * 10, get_conf6(OBS(25)));

	if (OBS(21) != AOF_UNDEF)
		msg.set_wind_dir(OBS(21), get_conf6(OBS(26) & 0x3f));
	if (OBS(22) != AOF_UNDEF)
		msg.set_wind_speed(OBS(22), get_conf6((OBS(26) >> 6) & 0x3f));

	if (OBS(23) != AOF_UNDEF)
		msg.set_temp_2m(totemp(OBS(23)), get_conf6((OBS(26) >> 12) & 0x3f));

	if (OBS(24) != AOF_UNDEF)
		msg.set_water_temp((double)OBS(24) / 10.0, get_conf6((OBS(26) >> 18) & 0x3f));
}

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
