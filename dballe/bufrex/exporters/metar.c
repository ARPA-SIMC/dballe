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

#include <dballe/msg/dba_msg.h>

#include "exporters.h"

static dba_err exporter(dba_msg src, bufrex_subset dst, int type);

bufrex_exporter bufrex_exporter_metar_0_140 = {
	/* Category */
	0,
	/* Subcategory */
	140,
	/* dba_msg type it can convert from */
	MSG_SYNOP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  7, 11),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 21),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1, 21),
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
/*  0 */ { DBA_VAR(0,  1, 63), DBA_MSG_ST_NAME_ICAO },
/*  1 */ { DBA_VAR(0,  2,  1), DBA_MSG_ST_TYPE },
/*  2 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR },
/*  3 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH },
/*  4 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY },
/*  5 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR },
/*  6 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE },
/*  7 */ { DBA_VAR(0,  5,  2), DBA_MSG_LATITUDE },
/*  8 */ { DBA_VAR(0,  6,  2), DBA_MSG_LONGITUDE },
/*  9 */ { DBA_VAR(0,  7,  1), DBA_MSG_HEIGHT },
/* 10 */ { DBA_VAR(0,  7,  6), -1 },
/* 11 */ { DBA_VAR(0, 11,  1), DBA_MSG_WIND_DIR },
/* 12 */ { DBA_VAR(0, 11, 16), DBA_MSG_EX_CCW_WIND },
/* 13 */ { DBA_VAR(0, 11, 17), DBA_MSG_EX_CW_WIND },
/* 14 */ { DBA_VAR(0, 11,  2), DBA_MSG_WIND_SPEED },
/* 15 */ { DBA_VAR(0, 11, 41), DBA_MSG_WIND_MAX },
/* 16 */ { DBA_VAR(0,  7,  6), -1 },
/* 15 */ { DBA_VAR(0, 12,  1), DBA_MSG_TEMP_2M },
/* 16 */ { DBA_VAR(0, 12,  3), DBA_MSG_DEWPOINT_2M },
/* 17 */ { DBA_VAR(0, 10, 52), DBA_MSG_QNH },
/* 18 */ { DBA_VAR(0, 20,  9), DBA_MSG_METAR_WTR },
};

static dba_err exporter(dba_msg src, bufrex_subset dst, int type)
{
	int i;
	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case 10:
				DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, 10));
				break;
			case 16:
				DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, 2));
				break;
			default: {
				dba_msg_datum d = dba_msg_find_by_id(src, tpl[i].var);
				if (d != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, d->var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			}
		}
	}

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_dpb(dst, 21));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 31), ORIG_CENTRE_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 32), ORIG_APP_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_attrs(dst, 21, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
