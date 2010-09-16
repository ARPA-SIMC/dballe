/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "dballe/msg/msg.h"
#include "dballe/bufrex/msg.h"
#include <string.h>

dba_err bufrex_copy_to_sat(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	dba_err err = DBA_OK;
	int i;
	int l1 = -1;
	int id_satid = 0, id_sensorid = 0;
	dba_var scanline = NULL, fov = NULL, framecount = NULL, wavenum = NULL, chqf = NULL;
	dba_var copy = NULL;
	char ident[10];

	switch (raw->localsubtype)
	{
#if 0
		case 142: msg->type = MSG_AIREP; break;
		case 144: msg->type = MSG_AMDAR; break;
		case 145: msg->type = MSG_ACARS; break;
#endif
		default: msg->type = MSG_SAT; break;
	}
	
	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
			continue;
		switch (dba_var_code(var))
		{
		  	// 001007 SATELLITE IDENTIFIER(CODE TABLE   1007): 3
			case WR_VAR(0, 1,  7): DBA_RUN_OR_RETURN(dba_var_enqi(var, &id_satid)); break;
			// 002048 SATELLITE SENSOR INDICATOR(CODE TABLE   2048): 0
			case WR_VAR(0, 2, 48): DBA_RUN_OR_RETURN(dba_var_enqi(var, &id_sensorid)); break;
		    // -> Ident

			// 005040 ORBIT NUMBER(NUMERIC): 1
			case WR_VAR(0, 5, 40):
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, WR_VAR(0, 5, 40), 8, 0, 0, 0, 254, 0, 0));
				break;
		    // -> Data

#if 0
	-- Ignored
			025075 SATELLITE ANTENNA CORRECTION VERSION NUMBER(NUMERIC): (undef)
#endif

			// 005041 SCAN LINE NUMBER(NUMERIC): 211
			case WR_VAR(0, 5, 41): scanline = var; break;
			// 005043 FIELD OF VIEW NUMBER(NUMERIC): 0
			case WR_VAR(0, 5, 43): fov = var; break;
			// 025070 MAJOR FRAME COUNT(NUMERIC): (undef)
			case WR_VAR(0, 25, 70): framecount = var; break;
			// -> Attrs

#if 0
	-- Ignored
			033030 SCAN LINE STATUS FLAGS FOR ATOVS(FLAG TABLE  33030): 0
			033031 SCAN LINE QUALITY FLAGS FOR ATOVS(FLAG TABLE  33031): 262144
#endif
			case WR_VAR(0,  4,  1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case WR_VAR(0,  4,  2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case WR_VAR(0,  4,  3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case WR_VAR(0,  4,  4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case WR_VAR(0,  4,  5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			// TODO: 004006 SECOND(SECOND): 22.522000 does not fit in normal second
			case WR_VAR(0,  5,  1):
			case WR_VAR(0,  5,  2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case WR_VAR(0,  6,  1):
			case WR_VAR(0,  6,  2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			// TODO: 007001 HEIGHT OF STATION(M): 830900.000000 does not fit in normal height
			//case WR_VAR(0,  7,  1): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;

			// 007024 SATELLITE ZENITH ANGLE(DEGREE): 56.830000
			case WR_VAR(0,  7, 24): 
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, WR_VAR(0, 7, 24), 8, 0, 0, 0, 254, 0, 0));
				break;
			// 005021 BEARING OR AZIMUTH(DEGREE TRUE): 28.650000
			case WR_VAR(0,  5, 21): 
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, WR_VAR(0, 5, 21), 8, 0, 0, 0, 254, 0, 0));
				break;
			// 007025 SOLAR ZENITH ANGLE(DEGREE): 74.240000
			case WR_VAR(0,  7, 25): 
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, WR_VAR(0, 7, 25), 8, 0, 0, 0, 254, 0, 0));
				break;
			// 005022 SOLAR AZIMUTH(DEGREE TRUE): 273.710000
			case WR_VAR(0,  5, 22): 
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, WR_VAR(0, 5, 22), 8, 0, 0, 0, 254, 0, 0));
				break;
			// -> data

			// TODO: 033033 FIELD OF VIEW QUALITY FLAGS FOR ATOVS(FLAG TABLE  33033): (undef)
			// -> ?																		 

#if 0																			  
	-- Ignored
			002151 RADIOMETER IDENTIFIER(CODE TABLE   2151): 0
			012064 INSTRUMENT TEMPERATURE(K): (undef)
			002151 RADIOMETER IDENTIFIER(CODE TABLE   2151): 3
			012064 INSTRUMENT TEMPERATURE(K): (undef)
			002151 RADIOMETER IDENTIFIER(CODE TABLE   2151): 4
			012064 INSTRUMENT TEMPERATURE(K): (undef)
			002151 RADIOMETER IDENTIFIER(CODE TABLE   2151): 6
			012064 INSTRUMENT TEMPERATURE(K): (undef)
#endif
			// 002150 TOVS/ATOVS/HIRS INSTRUMENT CHANNEL NUMBER(CODE TABLE   2150): 1
			case WR_VAR(0, 2, 150): DBA_RUN_OR_RETURN(dba_var_enqi(var, &l1)); break;
			// -> level (8, x, 0)

			// 025076 LOG-10 OF (TEMP-RAD CENTRAL WAVENUMBER) FOR ATOVS(LOGM-1): 4.825342
			case WR_VAR(0, 25, 76): wavenum = var; break;
			// -> attr
#if 0																			  
	-- Ignored
			025077 BANDWIDTH CORRECTION COEFFICIENT 1 FOR ATOVS(NUMERIC): -0.000250
			025078 BANDWIDTH CORRECTION COEFFICIENT 2 FOR ATOVS(NUMERIC): 0.999990
#endif
			// 033032 CHANNEL QUALITY FLAGS FOR ATOVS(FLAG TABLE  33032): 0
			case WR_VAR(0, 33, 32): chqf = var; break;
			// -> attr

			// 012063 BRIGHTNESS TEMPERATURE(K): 241.100000
			case WR_VAR(0, 12, 63): {
				if (l1 == -1) return dba_error_consistency("channel number not found in incoming message");
				DBA_RUN_OR_RETURN(dba_var_copy(var, &copy));
				if (scanline != NULL)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, scanline));
				if (fov != NULL)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, fov));
				if (framecount != NULL)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, framecount));
				if (wavenum != NULL)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, wavenum));
				if (chqf != NULL)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, chqf));
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set(msg, copy, WR_VAR(0, 12, 63), 8, l1, 0, 0, 254, 0, 0));
				dba_var_delete(copy);
				copy = NULL;
				break;
			}
			// -> data
		}
	}

	snprintf(ident, 6, "%03d%02d", id_satid, id_sensorid);
	DBA_RUN_OR_RETURN(dba_msg_set_ident(msg, ident, -1));
cleanup:
	if (copy != NULL) dba_var_delete(copy);

	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
