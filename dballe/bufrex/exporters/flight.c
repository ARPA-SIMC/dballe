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

#include <dballe/conv/conv.h>
#include <dballe/msg/dba_msg.h>

#include "exporters.h"

static dba_err exporter(dba_msg src, bufrex_raw dst, int type);
static dba_err exporter_acars(dba_msg src, bufrex_raw dst, int type);

bufrex_exporter bufrex_exporter_flight_4_142 = {
	/* Category */
	4,
	/* Subcategory */
	142,
	/* dba_msg type it can convert from */
	MSG_AIREP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3, 11,  1),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 18),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1, 18),
		DBA_VAR(0, 33,  7),
		0
	},
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

bufrex_exporter bufrex_exporter_flight_4_144 = {
	/* Category */
	4,
	/* Subcategory */
	144,
	/* dba_msg type it can convert from */
	MSG_AMDAR,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3, 11,  1),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 18),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1, 18),
		DBA_VAR(0, 33,  7),
		0
	},
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

bufrex_exporter bufrex_exporter_acars_4_145 = {
	/* Category */
	4,
	/* Subcategory */
	145,
	/* dba_msg type it can convert from */
	MSG_ACARS,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(0,  1,  6),
		DBA_VAR(0,  1,  8),
		DBA_VAR(0,  2, 61),
		DBA_VAR(0,  2, 62),
		DBA_VAR(0,  2,  2),
		DBA_VAR(0,  2,  5),
		DBA_VAR(0,  2, 70),
		DBA_VAR(0,  2, 63),
		DBA_VAR(0,  2,  1),
		DBA_VAR(0,  4,  1),
		DBA_VAR(0,  4,  2),
		DBA_VAR(0,  4,  3),
		DBA_VAR(0,  4,  4),
		DBA_VAR(0,  4,  5),
		DBA_VAR(0,  5,  2),
		DBA_VAR(0,  6,  2),
		DBA_VAR(0,  8,  4),
		DBA_VAR(0,  7,  4),
		DBA_VAR(0,  8, 21),
		DBA_VAR(0, 11,  1),
		DBA_VAR(0, 11,  2),
		DBA_VAR(0, 11, 31),
		DBA_VAR(0, 11, 34),
		DBA_VAR(0, 11, 35),
		DBA_VAR(0, 12,  1),
		DBA_VAR(0, 12,  3),
		DBA_VAR(0, 13,  3),
		DBA_VAR(0, 20, 41),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 28),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0, 12,  1),
		DBA_VAR(1,  1, 28),
		DBA_VAR(0, 33,  7),
		0
	},
	/* Exporter function */
	(bufrex_exporter_func)exporter_acars,
};



struct template {
	dba_varcode code;
	int var;
	dba_varcode msgcode;
};

static struct template tpl_gen[] = {
/*  0 */ { DBA_VAR(0,  1,  6), DBA_MSG_IDENT,			0 },
/*  1 */ { DBA_VAR(0,  2, 61), DBA_MSG_NAVSYS,			0 },
/*  2 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR,			0 },
/*  3 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH,			0 },
/*  4 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY,				0 },
/*  5 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR,			0 },
/*  6 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE,			0 },
/*  7 */ { DBA_VAR(0,  5,  1), DBA_MSG_LATITUDE,		0 },
/*  8 */ { DBA_VAR(0,  6,  1), DBA_MSG_LONGITUDE,		0 },
/*  9 */ { DBA_VAR(0,  8,  4), DBA_MSG_FLIGHT_PHASE,	0 },
/* 10 */ { DBA_VAR(0,  7,  2), DBA_MSG_HEIGHT,			0 },
/* 11 */ { DBA_VAR(0, 12,  1), -1,			DBA_VAR(0, 12,  1) },	/* TEMPERATURE/DRY-BULB TEMPERATURE */
/* 12 */ { DBA_VAR(0, 11,  1), -1,			DBA_VAR(0, 11,  1) },	/* WIND DIRECTION */
/* 13 */ { DBA_VAR(0, 11,  2), -1,			DBA_VAR(0, 11,  2) },	/* WIND SPEED */
/* 14 */ { DBA_VAR(0, 11, 31), -1,			DBA_VAR(0, 11, 31) },	/* DEGREE OF TURBULENCE */
/* 15 */ { DBA_VAR(0, 11, 32), -1,			DBA_VAR(0, 11, 32) },	/* HEIGHT OF BASE OF TURBULENCE */
/* 16 */ { DBA_VAR(0, 11, 33), -1,			DBA_VAR(0, 11, 33) },	/* HEIGHT OF TOP OF TURBULENCE */
/* 17 */ { DBA_VAR(0, 20, 41), -1,			DBA_VAR(0, 20, 41) },	/* AIRFRAME ICING */
};

static dba_err export_common(dba_msg src, struct template* tpl, int tpl_count, bufrex_raw dst, int type)
{
	int ltype = -1, l1 = -1;
	dba_msg_datum d;
	int i;
	
	/* Get the pressure to identify the level layer where the airplane is */
	if ((d = dba_msg_find_by_id(src, DBA_MSG_FLIGHT_PRESS)) != NULL)
	{
		double press;
		DBA_RUN_OR_RETURN(dba_var_enqd(d->var, &press));
		if (dba_msg_find_level(src, 100, press/100, 0) != NULL)
		{
			ltype = 100;
			l1 = press / 100;
			/* fprintf(stderr, "BUFR Press float: %f int: %d encoded: %s\n", press, l1, dba_var_value(d->var)); */
		}
	}
	if ((d = dba_msg_find_by_id(src, DBA_MSG_HEIGHT)) != NULL && ltype == -1)
	{
		double height;
		DBA_RUN_OR_RETURN(dba_var_enqd(d->var, &height));
		ltype = 103;
		l1 = height;
	}

	if (ltype == -1)
		return dba_error_notfound("looking for airplane pressure or height in flight message");

	/* Fill up the message */
	for (i = 0; i < tpl_count; i++)
	{
		dba_msg_datum d;

		if (tpl[i].var != -1)
			d = dba_msg_find_by_id(src, tpl[i].var);
		else
			d = dba_msg_find(src, tpl[i].msgcode, ltype, l1, 0, 0, 0, 0);

		if (d != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, tpl[i].code, d->var));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, tpl[i].code));
	}

	return dba_error_ok();
}

static dba_err exporter(dba_msg src, bufrex_raw dst, int type)
{
	DBA_RUN_OR_RETURN(export_common(src, tpl_gen, sizeof(tpl_gen)/sizeof(struct template), dst, type));

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_fixed_dpb(dst, 18));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 32)));
		DBA_RUN_OR_RETURN(bufrex_raw_append_fixed_attrs(dst, 18, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

static struct template tpl_acars[] = {
/*  0 */ { DBA_VAR(0,  1,  6), DBA_MSG_IDENT,			0 },
/*  1 */ { DBA_VAR(0,  1,  8), DBA_MSG_FLIGHT_REG_NO,	0 },
/*  2 */ { DBA_VAR(0,  2, 61), DBA_MSG_NAVSYS,			0 },
/*  3 */ { DBA_VAR(0,  2, 62), DBA_MSG_DATA_RELAY,		0 },
/*  4 */ { DBA_VAR(0,  2,  2), DBA_MSG_WIND_INST,		0 },
/*  5 */ { DBA_VAR(0,  2,  5), DBA_MSG_TEMP_PRECISION,	0 },
/*  6 */ { DBA_VAR(0,  2, 70), DBA_MSG_LATLON_SPEC,		0 },
/*  7 */ { DBA_VAR(0,  2, 63), DBA_MSG_FLIGHT_ROLL,		0 },
/*  8 */ { DBA_VAR(0,  2,  1), DBA_MSG_ST_TYPE,			0 },
/*  9 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR,			0 },
/* 10 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH,			0 },
/* 11 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY,				0 },
/* 12 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR,			0 },
/* 13 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE,			0 },
/* 14 */ { DBA_VAR(0,  5,  2), DBA_MSG_LATITUDE,		0 },
/* 15 */ { DBA_VAR(0,  6,  2), DBA_MSG_LONGITUDE,		0 },
/* 16 */ { DBA_VAR(0,  8,  4), DBA_MSG_FLIGHT_PHASE,	0 },
/* 17 */ { DBA_VAR(0,  7,  4), DBA_MSG_FLIGHT_PRESS,	0 },
/* 18 */ { DBA_VAR(0,  8, 21), DBA_MSG_TIMESIG,			0 },
/* 19 */ { DBA_VAR(0, 11,  1), -1,			DBA_VAR(0, 11,  1) },	/* WIND DIRECTION */
/* 20 */ { DBA_VAR(0, 11,  2), -1,			DBA_VAR(0, 11,  2) },	/* WIND SPEED */
/* 21 */ { DBA_VAR(0, 11, 31), -1,			DBA_VAR(0, 11, 31) },	/* DEGREE OF TURBULENCE */
/* 22 */ { DBA_VAR(0, 11, 34), -1,			DBA_VAR(0, 11, 34) },	/* VERTICAL GUST VELOCITY */
/* 23 */ { DBA_VAR(0, 11, 35), -1,			DBA_VAR(0, 11, 35) },	/* VERTICAL GUST ACCELERATION */
/* 24 */ { DBA_VAR(0, 12,  1), -1,			DBA_VAR(0, 12,  1) },	/* TEMPERATURE/DRY-BULB TEMPERATURE */
/* 25 */ { DBA_VAR(0, 12,  3), -1,			DBA_VAR(0, 12,  3) },	/* DEW-POINT TEMPERATURE */
/* 26 */ { DBA_VAR(0, 13,  3), -1,			DBA_VAR(0, 13,  3) },	/* RELATIVE HUMIDITY */
/* 27 */ { DBA_VAR(0, 20, 41), -1,			DBA_VAR(0, 20, 41) },	/* AIRFRAME ICING */
};

static dba_err exporter_acars(dba_msg src, bufrex_raw dst, int type)
{
	DBA_RUN_OR_RETURN(export_common(src, tpl_acars, sizeof(tpl_acars)/sizeof(struct template), dst, type));

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_fixed_dpb(dst, 28));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 12, 1)));
		DBA_RUN_OR_RETURN(bufrex_raw_append_fixed_attrs(dst, 28, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
