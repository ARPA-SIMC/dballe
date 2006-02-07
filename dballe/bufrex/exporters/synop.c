#include <dballe/msg/dba_msg.h>

#include "exporters.h"

static dba_err exporter(dba_msg src, bufrex_raw dst, int type);

bufrex_exporter bufrex_exporter_synop_0_1 = {
	/* Category */
	0,
	/* Subcategory */
	1,
	/* dba_msg type it can convert from */
	MSG_SYNOP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  7,  5),
		DBA_VAR(0, 13, 23),
		DBA_VAR(0, 13, 13),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 49),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1, 49),
		DBA_VAR(0, 33,  7),
		0
	},
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

bufrex_exporter bufrex_exporter_synop_0_3 = {
	/* Category */
	0,
	/* Subcategory */
	3,
	/* dba_msg type it can convert from */
	MSG_SYNOP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  7,  5),
		DBA_VAR(0, 13, 23),
		DBA_VAR(0, 13, 13),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 49),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1, 49),
		DBA_VAR(0, 33,  7),
		0
	},
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

struct template {
	dba_varcode code;
	int var;
};

static struct template tpl[] = {
/*  0 */ { DBA_VAR(0,  1,  1), DBA_MSG_BLOCK },
/*  1 */ { DBA_VAR(0,  1,  2), DBA_MSG_STATION },
/*  2 */ { DBA_VAR(0,  2,  1), DBA_MSG_ST_TYPE },
/*  3 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR },
/*  4 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH },
/*  5 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY },
/*  6 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR },
/*  7 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE },
/*  8 */ { DBA_VAR(0,  5,  1), DBA_MSG_LATITUDE },
/*  9 */ { DBA_VAR(0,  6,  1), DBA_MSG_LONGITUDE },
/* 10 */ { DBA_VAR(0,  7,  1), DBA_MSG_HEIGHT },
/* 11 */ { DBA_VAR(0, 10,  4), DBA_MSG_PRESS },
/* 12 */ { DBA_VAR(0, 10, 51), DBA_MSG_PRESS_MSL },
/* 13 */ { DBA_VAR(0, 10, 61), DBA_MSG_PRESS_3H },
/* 14 */ { DBA_VAR(0, 10, 63), DBA_MSG_PRESS_TEND },
/* 15 */ { DBA_VAR(0, 11, 11), DBA_MSG_WIND_DIR },
/* 16 */ { DBA_VAR(0, 11, 12), DBA_MSG_WIND_SPEED },
/* 17 */ { DBA_VAR(0, 12,  4), DBA_MSG_TEMP_2M },
/* 18 */ { DBA_VAR(0, 12,  6), DBA_MSG_DEWPOINT_2M },
/* 19 */ { DBA_VAR(0, 13,  3), DBA_MSG_HUMIDITY },
/* 20 */ { DBA_VAR(0, 20,  1), DBA_MSG_VISIBILITY },
/* 21 */ { DBA_VAR(0, 20,  3), DBA_MSG_PRES_WTR },
/* 22 */ { DBA_VAR(0, 20,  4), DBA_MSG_PAST_WTR1 },
/* 23 */ { DBA_VAR(0, 20,  5), DBA_MSG_PAST_WTR2 },
/* 24 */ { DBA_VAR(0, 20, 10), DBA_MSG_CLOUD_N },
/* 25 */ { DBA_VAR(0,  8,  2), -1 },
/* 26 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_NH },
/* 27 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_HH },
/* 28 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CL },
/* 29 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CM },
/* 30 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CH },
/* 31 */ { DBA_VAR(0,  8,  2), -1 },
/* 32 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_N1 },
/* 33 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_C1 },
/* 34 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_H1 },
/* 35 */ { DBA_VAR(0,  8,  2), -1 },
/* 36 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_N2 },
/* 37 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_C2 },
/* 38 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_H2 },
/* 39 */ { DBA_VAR(0,  8,  2), -1 },
/* 40 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_N3 },
/* 41 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_C3 },
/* 42 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_H3 },
/* 43 */ { DBA_VAR(0,  8,  2), -1 },
/* 44 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_N4 },
/* 45 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_C4 },
/* 46 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_H4 },
/* 47 */ { DBA_VAR(0, 13, 23), DBA_MSG_TOT_PREC24 },
/* 48 */ { DBA_VAR(0, 13, 13), DBA_MSG_TOT_SNOW },
};

static dba_err exporter(dba_msg src, bufrex_raw dst, int type)
{
	int i;
	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case 25:
			case 31:
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, tpl[i].code, 1));
				break;
			case 35:
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, tpl[i].code, 2));
				break;
			case 39:
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, tpl[i].code, 3));
				break;
			case 43:
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, tpl[i].code, 4));
				break;
			default: {
				dba_msg_datum d = dba_msg_find_by_id(src, tpl[i].var);
				if (d != NULL)
					DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, tpl[i].code, d->var));
				else
					DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, tpl[i].code));
				break;
			}
		}
	}

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_fixed_dpb(dst, 49));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, DBA_VAR(0, 1, 31), ORIG_CENTRE_ID));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, DBA_VAR(0, 1, 32), ORIG_APP_ID));
		DBA_RUN_OR_RETURN(bufrex_raw_append_fixed_attrs(dst, 49, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
