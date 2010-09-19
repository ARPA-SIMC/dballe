/*
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
#include "dballe/msg/context.h"

static dba_err exporter91(dba_msg msg, bufrex_msg bmsg, bufrex_subset dst, int type);

struct _bufrex_exporter bufrex_exporter_pilot_2_91 = {
	/* Category */
	2,
	/* Subcategory */
	255,
	/* Local subcategory */
	91,
	/* dba_msg type it can convert from */
	MSG_PILOT,
	/* Data descriptor section */
	(dba_varcode[]){
		WR_VAR(3,  1,  1),
		WR_VAR(0,  2, 11),
		WR_VAR(0,  2, 12),
		WR_VAR(3,  1, 11),
		WR_VAR(3,  1, 12),
		WR_VAR(3,  1, 22),
		WR_VAR(1,  5,  0),
		WR_VAR(0, 31,  1),
		WR_VAR(0,  7,  4),
		WR_VAR(0,  8,  1),
		WR_VAR(0, 10,  3),
		WR_VAR(0, 11,  1),
		WR_VAR(0, 11,  2),
		WR_VAR(2, 22,  0),
		WR_VAR(1,  1,  0),
		WR_VAR(0, 31,  2),
		WR_VAR(0, 31, 31),
		WR_VAR(0,  1, 31),
		WR_VAR(0,  1, 32),
		WR_VAR(1,  1,  0),
		WR_VAR(0, 31,  2),
		WR_VAR(0, 33,  7),
		0,
	},
	/* Datadesc function */
	bufrex_standard_datadesc_func,
	/* Exporter function */
	(bufrex_exporter_func)exporter91,
};

struct template {
	dba_varcode code;
	int var;
};

static dba_err run_template(dba_msg msg, bufrex_subset dst, struct template* tpl, int tpl_count)
{
	int i;

	/* Fill up the message */
	for (i = 0; i < tpl_count; i++)
		if (tpl[i].var < 0)
		{
			/* Special handling for vertical sounding significance */
			if (tpl[i].code == WR_VAR(0, 8, 2))
				DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, -tpl[i].var));
		}
		else
		{
			dba_var var = dba_msg_find_by_id(msg, tpl[i].var);
			if (var != NULL)
				DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
			else
				DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
		}

	return dba_error_ok();
}

static struct template tpl91[] = {
/*  0 */ { WR_VAR(0,  1,  1), DBA_MSG_BLOCK,		 },
/*  1 */ { WR_VAR(0,  1,  2), DBA_MSG_STATION,		 },
/*  2 */ { WR_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE,	 },
/*  3 */ { WR_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD, },
/*  4 */ { WR_VAR(0,  4,  1), DBA_MSG_YEAR,		 },
/*  5 */ { WR_VAR(0,  4,  2), DBA_MSG_MONTH,		 },
/*  6 */ { WR_VAR(0,  4,  3), DBA_MSG_DAY,			 },
/*  7 */ { WR_VAR(0,  4,  4), DBA_MSG_HOUR,		 },
/*  8 */ { WR_VAR(0,  4,  5), DBA_MSG_MINUTE,		 },
/*  9 */ { WR_VAR(0,  5,  1), DBA_MSG_LATITUDE,	 },
/* 10 */ { WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE,	 },
/* 11 */ { WR_VAR(0,  7,  1), DBA_MSG_HEIGHT,		 },
};
static dba_err exporter91(dba_msg msg, bufrex_msg bmsg, bufrex_subset dst, int type)
{
	const int tplsize = sizeof(tpl91)/sizeof(struct template);
	dba_var var_levcount;
	int lev_no = 0;
	int i;

	/* Fill up the message */
	DBA_RUN_OR_RETURN(run_template(msg, dst, tpl91, tplsize));
	DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 31,  1)));
	var_levcount = dst->vars[dst->vars_count - 1];

	/* Iterate backwards as we need to add levels in decreasing pressure order */
	for (i = msg->data_count - 1; i >= 0; --i)
	{
		dba_msg_context ctx = msg->data[i];
		dba_var vss = dba_msg_context_find_vsig(ctx);
		dba_var var;

		/* Skip levels without vertical sounding significance */
		if (vss == NULL) continue;

		/* Add pressure */
		if ((var = dba_msg_context_find(ctx, WR_VAR(0, 10, 4))) != NULL)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, WR_VAR(0, 7, 4), var));
		else if (ctx->ltype1 == 100)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_d(dst, WR_VAR(0, 7, 4), ctx->l1));
		else
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 7, 4)));

		/* Add vertical sounding significance */
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, WR_VAR(0, 8, 1), vss));

		/* Add geopotential */
		if ((var = dba_msg_context_find(ctx, WR_VAR(0, 10, 3))) != NULL)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, WR_VAR(0, 10, 3), var));
		else if (ctx->ltype1 == 102)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_d(dst, WR_VAR(0, 10, 3), (double)ctx->l1 * 9.80665));
		else
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 10, 3)));

		/* Add wind direction */
		if ((var = dba_msg_context_find(ctx, WR_VAR(0, 11, 1))) != NULL)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, WR_VAR(0, 11, 1), var));
		else
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 11, 1)));

		/* Add wind speed */
		if ((var = dba_msg_context_find(ctx, WR_VAR(0, 11, 2))) != NULL)
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, WR_VAR(0, 11, 2), var));
		else
			DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 11, 2)));

		lev_no++;
	}
	
	DBA_RUN_OR_RETURN(dba_var_seti(var_levcount, lev_no));

	if (type == 0)
	{
		int count;
		DBA_RUN_OR_RETURN(bufrex_subset_append_dpb(dst, WR_VAR(2, 22, 0), tplsize + 1 + lev_no * 5, WR_VAR(0, 33, 7), &count));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, WR_VAR(0, 1, 32)));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, WR_VAR(0, 31, 2), count));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
