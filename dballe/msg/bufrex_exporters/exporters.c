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

#include "exporters.h"

extern struct _bufrex_exporter bufrex_exporter_generic;
extern struct _bufrex_exporter bufrex_exporter_synop_0_1;
extern struct _bufrex_exporter bufrex_exporter_synop_0_1high;
extern struct _bufrex_exporter bufrex_exporter_synop_0_3;
extern struct _bufrex_exporter bufrex_exporter_sea_1_9;
extern struct _bufrex_exporter bufrex_exporter_sea_1_11;
extern struct _bufrex_exporter bufrex_exporter_sea_1_13;
extern struct _bufrex_exporter bufrex_exporter_sea_1_19;
extern struct _bufrex_exporter bufrex_exporter_sea_1_21;
extern struct _bufrex_exporter bufrex_exporter_pilot_2_91;
extern struct _bufrex_exporter bufrex_exporter_temp_2_101;
extern struct _bufrex_exporter bufrex_exporter_temp_2_102;
extern struct _bufrex_exporter bufrex_exporter_flight_4_142;
extern struct _bufrex_exporter bufrex_exporter_flight_4_144;
extern struct _bufrex_exporter bufrex_exporter_acars_4_145;
extern struct _bufrex_exporter bufrex_exporter_metar_0_140;
extern struct _bufrex_exporter bufrex_exporter_pollution_8_102;

static bufrex_exporter exporters[] = {
	&bufrex_exporter_generic,
	&bufrex_exporter_synop_0_1,
	&bufrex_exporter_synop_0_3,
	&bufrex_exporter_sea_1_9,
	&bufrex_exporter_sea_1_11,
	&bufrex_exporter_sea_1_13,
	&bufrex_exporter_sea_1_19,
	&bufrex_exporter_sea_1_21,
	&bufrex_exporter_pilot_2_91,
	&bufrex_exporter_temp_2_101,
	&bufrex_exporter_temp_2_102,
	&bufrex_exporter_flight_4_142,
	&bufrex_exporter_flight_4_144,
	&bufrex_exporter_acars_4_145,
	&bufrex_exporter_metar_0_140,
	&bufrex_exporter_pollution_8_102,
	0
};

dba_err bufrex_infer_type_subtype(dba_msg msg, int* type, int* subtype, int* localsubtype)
{
	bufrex_exporter exp = NULL;
	switch (msg->type)
	{
		case MSG_GENERIC:	exp = &bufrex_exporter_generic;			break;
		case MSG_SYNOP: {
			dba_var var = dba_msg_get_st_type_var(msg);
			if (var == NULL)
				exp = &bufrex_exporter_synop_0_1;
			else if (dba_var_value(var)[0] == '1')
				exp = &bufrex_exporter_synop_0_1;
			else
				exp = &bufrex_exporter_synop_0_3;
			break;
		}
		case MSG_PILOT:		exp = &bufrex_exporter_pilot_2_91;		break;
		case MSG_TEMP:		exp = &bufrex_exporter_temp_2_101;		break;
		case MSG_TEMP_SHIP:	exp = &bufrex_exporter_temp_2_102;		break;
		case MSG_AIREP:		exp = &bufrex_exporter_flight_4_142;	break;
		case MSG_AMDAR:		exp = &bufrex_exporter_flight_4_144;	break;
		case MSG_ACARS:		exp = &bufrex_exporter_acars_4_145;		break;
		case MSG_SHIP: {
			dba_var var = dba_msg_get_st_type_var(msg);
			if (var == NULL)
				exp = &bufrex_exporter_sea_1_11;
			else if (dba_var_value(var)[0] == '1')
				exp = &bufrex_exporter_sea_1_11;
			else
				exp = &bufrex_exporter_sea_1_13;
			break;
		}
		case MSG_BUOY:		exp = &bufrex_exporter_sea_1_21;		break;
		case MSG_METAR:		exp = &bufrex_exporter_metar_0_140;		break;
		case MSG_POLLUTION:	exp = &bufrex_exporter_pollution_8_102;	break;
		case MSG_SAT:	exp = &bufrex_exporter_generic;			break;
	}
	*type = exp->type;
	*subtype = exp->subtype;
	*localsubtype = exp->localsubtype;
	return dba_error_ok();
}

dba_err bufrex_get_exporter(dba_msg src, int type, int subtype, int localsubtype, bufrex_exporter* exp)
{
	int i;
	for (i = 0; exporters[i] != NULL; i++)
	{
		/* fprintf(stderr, "TRY %d %d %d for %d %d %d\n", exporters[i]->type, exporters[i]->subtype, exporters[i]->localsubtype, type, subtype, localsubtype); */
		if (exporters[i]->type        == type &&
			exporters[i]->subtype     == subtype &&
			exporters[i]->localsubtype     == localsubtype)
		{
			if (type == 0 && localsubtype == 1)
				// Template ambiguity workaround: if we are handling a synop
				// that has geopotential in the ana level, then it's a
				// high-level station and we need to fetch the alternate output
				// template for the same type and subtype.
				if (dba_msg_get_isobaric_surface_var(src) != NULL)
					*exp = &bufrex_exporter_synop_0_1high;
				else
					*exp = exporters[i];
			else
				*exp = exporters[i];
			/*return exporters[i]->exporter(src, dst);*/
			return dba_error_ok();
		}
	}
	*exp = &bufrex_exporter_generic;
	return dba_error_ok();
	/*
	return dba_error_notfound("Exporter for %s messages to BUFREX type %d subtype %d", 
					dba_msg_type_name(src->type), type, subtype);
	*/
}

dba_err bufrex_standard_datadesc_func(bufrex_exporter exp, dba_msg src, bufrex_msg dst)
{
	int i;
	/* Init the bufrex_msg data descriptor chain */
	for (i = 0; exp->ddesc[i] != 0; i++)
	{
		/* Skip encoding of attributes for CREX */
		if (dst->encoding_type == BUFREX_CREX && exp->ddesc[i] == DBA_VAR(2, 22, 0))
			break;
		DBA_RUN_OR_RETURN(bufrex_msg_append_datadesc(dst, exp->ddesc[i]));
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
