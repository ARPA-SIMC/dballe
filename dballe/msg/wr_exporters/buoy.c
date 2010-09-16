/*
 * DB-ALLe - Archive for punctual meteorological data
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

#include "exporters.h"
#include <stdlib.h>

static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type);

struct _bufrex_exporter bufrex_exporter_sea_1_21 = {
	/* Category */
	1,
	/* Subcategory */
	255,
	/* Local subcategory */
	21,
	/* dba_msg type it can convert from */
	MSG_BUOY,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  8,    3),
		DBA_VAR(2, 22,    0),
		DBA_VAR(1,  1,   32),
		DBA_VAR(0, 31,   31),
		DBA_VAR(0,  1,   31),
		DBA_VAR(0,  1,  201),
		DBA_VAR(1,  1,   32),
		DBA_VAR(0, 33,    7),
		0
	},
	/* Datadesc function */
	bufrex_standard_datadesc_func,
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

struct template {
	dba_varcode code;
	int var;
};

static struct template tpl[] = {
/*  0 */ { DBA_VAR(0,  1,  5), -1 },
/*  1 */ { DBA_VAR(0,  1, 12), DBA_MSG_ST_DIR },
/*  2 */ { DBA_VAR(0,  1, 13), DBA_MSG_ST_SPEED },
/*  3 */ { DBA_VAR(0,  2,  1), DBA_MSG_ST_TYPE },
/*  4 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR },
/*  5 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH },
/*  6 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY },
/*  7 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR },
/*  8 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE },
/*  9 */ { DBA_VAR(0,  5,  2), DBA_MSG_LATITUDE },
/* 10 */ { DBA_VAR(0,  6,  2), DBA_MSG_LONGITUDE },
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
/* 31 */ { DBA_VAR(0, 22, 42), DBA_MSG_WATER_TEMP },
};

static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type)
{
	dba_var var;
	int i;
	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case 0:
				var = dba_msg_get_ident_var(src);
				const char* val = var == NULL ? NULL : dba_var_value(var);
				if (val != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, strtol(val, 0, 10)));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_add_attrs(dst, var));
				break;
			case 25:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 258, 0, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			default:
				var = dba_msg_find_by_id(src, tpl[i].var);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
		}
	}

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_dpb(dst, DBA_VAR(2, 22, 0), 32));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1,  31), ORIG_CENTRE_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 201), ORIG_APP_ID));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
