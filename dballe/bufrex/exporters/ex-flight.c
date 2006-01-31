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

static struct template tpl[] = {
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

static dba_err exporter(dba_msg src, bufrex_raw dst, int type)
{
	int i;
	double press;
	/* Get the pressure to identify the level layer where the airplane is */
	dba_msg_datum p = dba_msg_find_by_id(src, DBA_MSG_HEIGHT);
	if (p == NULL)
		return dba_error_notfound("looking for airplane height in AIREP message");
	DBA_RUN_OR_RETURN(dba_var_enqd(p->var, &press));
	DBA_RUN_OR_RETURN(dba_convert_icao_to_press(press, &press));

	/* Fill up the message */
	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		dba_msg_datum d;

		if (tpl[i].var != -1)
			d = dba_msg_find_by_id(src, tpl[i].var);
		else
			d = dba_msg_find(src, tpl[i].msgcode, 100, press, 0, 0, 0, 0);

		if (d != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, tpl[i].code, d->var));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, tpl[i].code));
	}

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
	int i;
	double press;
	/* Get the pressure to identify the level layer where the airplane is */
	dba_msg_datum p = dba_msg_find_by_id(src, DBA_MSG_FLIGHT_PRESS);
	if (p == NULL)
		return dba_error_notfound("looking for airplane pressure in ACARS message");
	DBA_RUN_OR_RETURN(dba_var_enqd(p->var, &press));

	/* Fill up the message */
	for (i = 0; i < sizeof(tpl_acars)/sizeof(struct template); i++)
	{
		dba_msg_datum d;

		if (tpl_acars[i].var != -1)
			d = dba_msg_find_by_id(src, tpl_acars[i].var);
		else
			d = dba_msg_find(src, tpl_acars[i].msgcode, 100, press, 0, 0, 0, 0);

		if (d != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, tpl_acars[i].code, d->var));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, tpl_acars[i].code));
	}

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
