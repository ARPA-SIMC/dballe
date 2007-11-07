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

#define _GNU_SOURCE
#include "dballe/msg/msg.h"
#include "dballe/core/conv.h"
#include "dballe/bufrex/msg.h"
#include <math.h>

dba_err bufrex_copy_to_pollution(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	dba_err err = DBA_OK;
	int i;
	// Above ground is ltype = 105, l1 = metri (se undef, cosa metto? Chiedo a
	// Stortini per un default?)
	int l1 = -1;
	int p1 = -1;
	int valtype = 0;
	int curdecscale = 0;
	int decscale = 0;
	double value = 0;
	dba_var attr_conf = NULL;
	dba_var attr_cas = NULL;
	dba_var attr_pmc = NULL;
	dba_var finalvar = NULL;

	msg->type = MSG_POLLUTION;
	
	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
			continue;
		switch (dba_var_code(var))
		{
			/* For this parameter you can give up to 32 characters as a station
			 * name. */
			case DBA_VAR(0,  1,  19): DBA_RUN_OR_RETURN(dba_msg_set_st_name_var(msg, var)); break;
			/* Airbase local code -- Up to 7 characters reflecting the local
			 * station code supplied with the observations. If not given then
			 * leave blank. */
			case DBA_VAR(0,  1, 212): DBA_RUN_OR_RETURN(dba_msg_set_poll_lcode_var(msg, var)); break;
			/* Airbase station code -- 7 character code supplied with AirBase
			 * observations (see Ref 1, II.1.4, page 23). If not supplied then
			 * leave blank.*/
			case DBA_VAR(0,  1, 213): DBA_RUN_OR_RETURN(dba_msg_set_poll_scode_var(msg, var)); break;
			/* GEMS code -- 6 character code suggested at RAQ Paris meeting,
			 * December 2006. First 2 characters to be country code (using
			 * ISO 3166-1-alpha-2 code), next 4 characters to be unique station
			 * number within national boundary for each station (numbering to
			 * be defined and maintained by each GEMS RAQ partner responsible
			 * for collecting observations within each national boundar
			 * invovled)*/
			case DBA_VAR(0,  1, 214): DBA_RUN_OR_RETURN(dba_msg_set_poll_gemscode_var(msg, var)); break;
			/*
			 * Dominant emission source influencing the air pollution
			 * concentrations at the station (based on Ref 1, II.2.2,  page 28)
			 * Possible values are:
			 * 0     traffic
			 * 1     industrial
			 * 2     background
			 * 3-6   reserved (do not use)
			 * 7     missing (or unknown)
			 */
			case DBA_VAR(0,  1, 215): DBA_RUN_OR_RETURN(dba_msg_set_poll_source_var(msg, var)); break;
			/*
			 * Type of area in which station is located (based on Ref 1, II.2.1, page 27)
			 * Possible values are:
			 * 0     urban
			 * 1     suburban
			 * 2     rural
			 * 3-6   reserved (do not use)
			 * 7     missing (or unknown)
			 */
			case DBA_VAR(0,  1, 216): DBA_RUN_OR_RETURN(dba_msg_set_poll_atype_var(msg, var)); break;
			/*
			 * Type of terrain in which the station is located (based on table in Ref 1, II.1.12, page 26)
			 * Possible values are:
			 * 0     mountain
			 * 1     valley
			 * 2     seaside
			 * 3     lakeside
			 * 4     plain
			 * 5     hilly terrain
			 * 6-14  reserved (do not use)
			 * 15    missing (or unknown)
			 */
			case DBA_VAR(0,  1, 217): DBA_RUN_OR_RETURN(dba_msg_set_poll_ttype_var(msg, var)); break;
			/*
			 * Date and time of observation in UTC. Should be the time of the
			 * observation, i.e. time at the end of the averaging
			 * period. Minute and second can be set to zero if that level of
			 * precision is not required.
			 */
			case DBA_VAR(0,  4,   1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,   2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,   3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,   4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,   5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  4,   6): DBA_RUN_OR_RETURN(dba_msg_set_second_var(msg, var)); break;
			/* Latitude of station (-90.0 to 90.0, up to 5 decimal place precision) */
			case DBA_VAR(0,  5,   1):
			case DBA_VAR(0,  5,   2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			/* Longitude of station (-180.0 to 180.0, up to 5 decimal place precision) */
			case DBA_VAR(0,  6,   1):
			case DBA_VAR(0,  6,   2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			/*
			 * Height of station above mean sea level and height of sensing
			 * instrument above local ground. Both in metres. If not known than
			 * can be coded as missing value.
			 */
			case DBA_VAR(0,  7,  30): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			case DBA_VAR(0,  7,  31):
				if (dba_var_value(var) == NULL)
				{
					/* Default to 3 metres above ground */
					l1 = 3;
				} else {
					DBA_RUN_OR_RETURN(dba_var_enqi(var, &l1));
				}
				break;
			/* Signifies that observation is an average over a certain time period. Value set to 2. */
			case DBA_VAR(0,  8,  21): {
				if (dba_var_value(var) == NULL)
					/* Since this is supposed to always be 2, if it is missing
					 * we just use the default */
					break;
				int val;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &val));
				if (val != 2)
					return dba_error_consistency("time significance is %d instead of 2", val);
				break;
			}
			/*
			 * Time period over which the average has been taken (in minutes),
			 * e.g. -60 for average over the previous hour.  The period is
			 * relative to the date/time of the observation.
			 */
			case DBA_VAR(0,  4,  25):
				if (dba_var_value(var) == NULL)
					/* Leave it to the default */
					break;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &p1));
				// Convert from minutes to seconds
				p1 = p1*60;
				break;
			/*
			 * VAL stands for validation and signifies that this parameter has
			 * not yet reached operational status at WMO.
			 * Identifier for species observed.
			 * Possible values are:
			 * Code    Constituent                        CAS Registry Number (if applicable)
			 * 0       Ozone (O3)                         10028-15-6
			 * 1       Water vapour (H2O)                 7732-18-5
			 * 2       Methane (CH4)                      74-82-8
			 * 3       Carbon dioxide (CO2)               37210-16-5
			 * 4       Carbon monoxide (CO)               630-08-0
			 * 5       Nitrogen dioxide (NO2)             10102-44-0
			 * 6       Nitrous oxide (N2O)                10024-97-2
			 * 7       Formaldehyde (HCHO)                50-00-0
			 * 8       Sulphur dioxide (SO2)              7446-09-5
			 * 9-24    reserved
			 * 25      Particulate matter < 1.0 microns
			 * 26      Particulate matter < 2.5 microns
			 * 27      Particulate matter < 10 microns
			 * 28      Aerosols (generic)
			 * 29      Smoke (generic)
			 * 30      Crustal material (generic)
			 * 31      Volcanic ash
			 * 32-200  reserved
			 * 201-254 reserved for local use
			 * 255     missing
			 * We may have to propose some new entries to WMO if this does not
			 * cover the range of constituents of the air quality observations.
			 * N.B. Do not code this as missing. This is a key piece of
			 * information in identifying the observed quantity.
			 *
			 * Our target BUFR entries are:
			 *  015192 [SIM] NO Concentration     Does not fit in above table: not exported
			 *  015193 [SIM] NO2 Concentration    5
			 *  015194 [SIM] O3 Concentration     0
			 *  015195 [SIM] PM10 Concentration   27
			 */
			case DBA_VAR(0,  8,  43): {
				if (dba_var_value(var) == NULL)
					return dba_error_consistency("message cannot be imported because the constituent type is missing");
				int val;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &val));
				switch (val)
				{
					case  0: valtype = DBA_VAR(0, 15, 194); break;
					case  5: valtype = DBA_VAR(0, 15, 193); break;
					case 27: valtype = DBA_VAR(0, 15, 195); break;
					default:
						return dba_error_consistency("cannot import constituent %d as there is no mapping for it", val);
				}
				break;
			}
			/*
			 * Chemical Abstracts Service (CAS) Registry number of constituent,
			 * if applicable. This parameter is optional and can be coded as a
			 * blank character string.
			 */
			case DBA_VAR(0,  8,  44): if (dba_var_value(var) != NULL) attr_cas = var; break;
			/*
			 * If parameter 008043 is coded as 25, 26 or 27, then this
			 * parameter can be used to further categorise the nature of the
			 * particulate matter.
			 *
			 * Possible values are:
			 * 0        Particulate matter (all types)
			 * 1        NO3(-)
			 * 2        NH4(+)
			 * 3        Na(+)
			 * 4        Cl(-)
			 * 5        Ca(2+)
			 * 6        Mg(2+)
			 * 7        K(+)
			 * 8        SO4(2-)
			 * 9-200    reserved
			 * 201-254  reserved for local use
			 * 255      missing
			 */
			case DBA_VAR(0,  8,  45): if (dba_var_value(var) != NULL) attr_pmc = var; break;
			/*
			 * A recent feature (still pre-operational at WMO) is the
			 * introduction of scaled quanities specifically to deal with
			 * quantities which may exhibit a large dynamic range. In order to
			 * be able to cover a large dynamic range whilst conserving
			 * precision some specific scale quantities have been introduced
			 * which have an associated decimal scaling factor. The
			 * example_airbase2bufr.f90 code gives an example of a method to
			 * calculate the decimal scaling factor.
			 */
			case DBA_VAR(0,  8,  90):
				/* Someone seemed to have thought that C fields in BUFR data
				 * section were not crazy enough, and went on reimplementing
				 * them using B fields.  So we have to reimplement the same
				 * logic here.
				 */
				if (dba_var_value(var) == NULL)
					/* If the value is missing, we reset decimal scaling */
					curdecscale = 0;
				else {
					DBA_RUN_OR_RETURN(dba_var_enqi(var, &curdecscale));
					
					/*
					 * Sadly, someone seems to have decided that "all 8 bits to
					 * 1" missing data is the same as encoding -127, so they
					 * encode "all 8 bits to 0 with a reference value of -127"
					 */
					if (curdecscale == -127)
						curdecscale = 0;
				}
				break;
			/*
			 * This is the most suitable parameter (that I can find) in BUFR to
			 * represent the concentration of pollutants.  The units are
			 * kg/m**3. The example_airbase2bufr.f90 code gives an example of a
			 * method to calculate the decimal scaling factor and scaled mass
			 * density from the observed concentration. To do the backwards
			 * calculation, concentration (kg/m**3)  = scaled mass density *
			 * 10**(decimal scaling factor)
			 */
			case DBA_VAR(0, 15,  23):
				if (dba_var_value(var) == NULL)
					return dba_error_consistency("message cannot be imported because the constituent type is missing");
				DBA_RUN_OR_RETURN(dba_var_enqd(var, &value));
				decscale = curdecscale;
				break;
			/*
			 * Parameter to give a qualitative measure of the quality of the
			 * observation. Set at the discretion of the encoder given any
			 * information they have either directly from the observation data
			 * set or otherwise.
			 * Possible values are:
			 * 0    Data not suspect
			 * 1    Data slightly suspect
			 * 2    Data highly suspect
			 * 3    Data considered unfit for use
			 * 4-6  Reserved
			 * 7    Quality information not given
			 */
			case DBA_VAR(0, 33,   3): if (dba_var_value(var) != NULL) attr_conf = var; break;
		}
	}

	/* Use default level and time range if the message did not report it */
	if (l1 == -1)
		l1 = 3;
	if (p1 == -1)
		p1 = -3600;
	
	/* Create the final variable */
	DBA_RUN_OR_RETURN(dba_var_create_local(valtype, &finalvar));

	/* Scale the value and set it */
	value = value * exp10(decscale);
	DBA_RUN_OR_GOTO(cleanup, dba_var_setd(finalvar, value));

	/* Add the attributes */
	if (attr_conf)
		DBA_RUN_OR_GOTO(cleanup, dba_var_seta(finalvar, attr_conf));
	if (attr_cas)
		DBA_RUN_OR_GOTO(cleanup, dba_var_seta(finalvar, attr_cas));
	if (attr_pmc)
		DBA_RUN_OR_GOTO(cleanup, dba_var_seta(finalvar, attr_pmc));

	/* Store it into the dba_msg */
	DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, finalvar, 105, l1, 0, 3, p1, 0));
	finalvar = NULL;

cleanup:
	if (finalvar != NULL)
		dba_var_delete(finalvar);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
