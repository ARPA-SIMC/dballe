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

static dba_err exporter91(dba_msg msg, bufrex_raw dst, int type);

bufrex_exporter bufrex_exporter_pilot_2_91 = {
	/* Category */
	2,
	/* Subcategory */
	91,
	/* dba_msg type it can convert from */
	MSG_PILOT,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  1,  1),
		DBA_VAR(0,  2, 11),
		DBA_VAR(0,  2, 12),
		DBA_VAR(3,  1, 11),
		DBA_VAR(3,  1, 12),
		DBA_VAR(3,  1, 22),
		DBA_VAR(1,  5,  0),
		DBA_VAR(0, 31,  1),
		DBA_VAR(0,  7,  4),
		DBA_VAR(0,  8,  1),
		DBA_VAR(0, 10,  3),
		DBA_VAR(0, 11,  1),
		DBA_VAR(0, 11,  2),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1,  0),
		DBA_VAR(0, 31,  2),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1,  0),
		DBA_VAR(0, 31,  2),
		DBA_VAR(0, 33,  7),
		0,
	},
	/* Exporter function */
	(bufrex_exporter_func)exporter91,
};

struct template {
	dba_varcode code;
	int var;
};

static dba_err run_template(dba_msg msg, bufrex_raw dst, struct template* tpl, int tpl_count)
{
	int i;

	/* Fill up the message */
	for (i = 0; i < tpl_count; i++)
		if (tpl[i].var < 0)
		{
			/* Special handling for vertical sounding significance */
			if (tpl[i].code == DBA_VAR(0, 8, 2))
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, tpl[i].code, -tpl[i].var));
		}
		else
		{
			dba_msg_datum d = dba_msg_find_by_id(msg, tpl[i].var);

			if (d != NULL)
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, tpl[i].code, d->var));
			else
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, tpl[i].code));
		}

	return dba_error_ok();
}

static struct template tpl91[] = {
/*  0 */ { DBA_VAR(0,  1,  1), DBA_MSG_BLOCK,		 },
/*  1 */ { DBA_VAR(0,  1,  2), DBA_MSG_STATION,		 },
/*  2 */ { DBA_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE,	 },
/*  3 */ { DBA_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD, },
/*  4 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR,		 },
/*  5 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH,		 },
/*  6 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY,			 },
/*  7 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR,		 },
/*  8 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE,		 },
/*  9 */ { DBA_VAR(0,  5,  1), DBA_MSG_LATITUDE,	 },
/* 10 */ { DBA_VAR(0,  6,  1), DBA_MSG_LONGITUDE,	 },
/* 11 */ { DBA_VAR(0,  7,  1), DBA_MSG_HEIGHT,		 },
};
static dba_err exporter91(dba_msg msg, bufrex_raw dst, int type)
{
	const int tplsize = sizeof(tpl91)/sizeof(struct template);
	dba_var var_levcount;
	int lev_no = 0;
	int i;

	/* Fill up the message */
	DBA_RUN_OR_RETURN(run_template(msg, dst, tpl91, tplsize));
	DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 31,  1)));
	var_levcount = dst->vars[dst->vars_count - 1];

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		dba_msg_datum d, d1;

		if ((lev->ltype != 100 && lev->ltype != 103) ||
			(d = dba_msg_level_find(lev, DBA_VAR(0, 8, 1), 0, 0, 0)) == NULL)
			continue;

		if ((d1 = dba_msg_level_find(lev, DBA_VAR(0, 10, 4), 0, 0, 0)) != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, DBA_VAR(0, 7, 4), d1->var));
		else if (lev->ltype == 100)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_d(dst, DBA_VAR(0, 7, 4), lev->l1 * 100));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 7, 4)));

		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, DBA_VAR(0, 8, 1), d->var));

		if ((d = dba_msg_level_find(lev, DBA_VAR(0, 10, 3), 0, 0, 0)) != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, DBA_VAR(0, 10, 3), d->var));
		else if (lev->ltype == 103)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_d(dst, DBA_VAR(0, 10, 3), (double)lev->l1 * 9.80665));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 10, 3)));

		if ((d = dba_msg_level_find(lev, DBA_VAR(0, 11, 1), 0, 0, 0)) != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, DBA_VAR(0, 11, 1), d->var));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 11, 1)));

		if ((d = dba_msg_level_find(lev, DBA_VAR(0, 11, 2), 0, 0, 0)) != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, DBA_VAR(0, 11, 2), d->var));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 11, 2)));

		lev_no++;
	}
	
	DBA_RUN_OR_RETURN(dba_var_seti(var_levcount, lev_no));

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_dpb(dst, tplsize + 1 + lev_no * 5, DBA_VAR(0, 33, 7)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 32)));
		DBA_RUN_OR_RETURN(bufrex_raw_append_attrs(dst, tplsize + 1 + lev_no * 5, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
