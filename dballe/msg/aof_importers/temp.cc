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

#include "common.h"
#include <wreport/conv.h>

using namespace wreport;

namespace dballe {
namespace msg {

void AOFImporter::read_temp(const uint32_t* obs, int obs_len, Msg& msg)
{
	int nlev = (obs_len - 20) / 8;
	int i;

	/* DBA_RUN_OR_RETURN(dba_msg_sounding_resize_obs(msg, nlev)); */

	/* 07 Code type */
	switch (OBS(7))
	{
		case 35:
			msg.type = MSG_TEMP;
			parse_st_block_station(obs, msg);
			parse_altitude(obs, msg);
			break;
		case 36:
			msg.type = MSG_TEMP_SHIP;
			parse_st_ident(obs, msg);
			break;
		default:
			msg.type = MSG_GENERIC;
			parse_st_ident(obs, msg);
			break;
	}

	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_dir, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_speed, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_type, 0));

	/* 08 Latitude */
	/* 09 Longitude */
	/* 10 Observation date */
	/* 12 Exact time of observation */
	parse_lat_lon_datetime(obs, msg);

	for (i = 0; i < nlev; i++)
	{
		int os = 20 + i*8;
		// Pressure
		double press = (double)OBS(os + 0) * 10;
		int vss = (OBS(os + 6) >> 12) & 0x1ff;

		if (OBS(os + 0) == AOF_UNDEF)
			error_consistency::throwf("pressure indication not found for level %d", i);
		else
			msg.setd(WR_VAR(0, 10, 4),
						OBS(os + 0) * 10, get_conf6(OBS(os + 6) & 0x3f),
						Level(100, press), Trange::instant());
					
		// Vertical sounding significance
		vss = convert_AOFVSS_to_BUFR08042(vss);
		msg.seti(WR_VAR(0, 8, 42), vss, -1, Level(100, press), Trange::instant());

		// Wind direction
		if (OBS(os + 1) != AOF_UNDEF)
			msg.setd(WR_VAR(0, 11, 1),
						OBS(os + 1), get_conf6((OBS(os + 6) >> 6) & 0x3f),
						Level(100, press), Trange::instant());
		// Wind speed
		if (OBS(os + 2) != AOF_UNDEF)
			msg.setd(WR_VAR(0, 11, 2),
						OBS(os + 2), get_conf6(OBS(os + 7) & 0x3f),
						Level(100, press), Trange::instant());
		// Air temperature
		if (OBS(os + 3) != AOF_UNDEF)
			msg.setd(WR_VAR(0, 12, 101),
						totemp(OBS(os + 3)), get_conf6((OBS(os + 7) >> 6) & 0x3f),
                        Level(100, press), Trange::instant());
		// Dew point temperature
		if (OBS(os + 4) != AOF_UNDEF)
			msg.setd(WR_VAR(0, 12, 103),
						totemp(OBS(os + 4)), get_conf6((OBS(os + 7) >> 12) & 0x3f),
                        Level(100, press), Trange::instant());
		// Height + 1000
		if (OBS(os + 5) != AOF_UNDEF)
		{
			// Convert height to geopotential
#if 0
			dba_msg_datum d;
			double dval;
			fprintf(stderr, "HEIGHT: %d -> %f\n", OBS(os + 5) - 1000, ((double)OBS(os + 5) - 1000)*9.80665);
#endif
			// Rounding the converted height->geopotential to preserve the
			// correct amount of significant digits
			msg.setd(WR_VAR(0, 10, 8),
						round(((double)OBS(os + 5) - 1000)*9.80665/10)*10, get_conf6((OBS(os + 7) >> 18) & 0x3f),
                        Level(100, press), Trange::instant());
#if 0
			d = dba_msg_find(msg, WR_VAR(0, 10, 3), 100, press, 0, 0, 0, 0);
			if (d == NULL)
				fprintf(stderr, "NO, QUESTO NO!\n");
			DBA_RUN_OR_RETURN(dba_var_enqd(d->var, &dval));
			fprintf(stderr, "HEIGHT: %d -> %f (was %f)\n", OBS(os + 5) - 1000, dval, round(((double)OBS(os + 5) - 1000)*9.80665/10)*10);
#endif
		}
	}
}

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
