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
