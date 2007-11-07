/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006,2007  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type);

struct _bufrex_exporter bufrex_exporter_pollution_8_102 = {
	/* Category */
	8,
	/* Subcategory */
	255,
	/* Local subcategory */
	171,
	/* dba_msg type it can convert from */
	MSG_POLLUTION,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(0,  1,  19),
		DBA_VAR(0,  1, 212),
		DBA_VAR(0,  1, 213),
		DBA_VAR(0,  1, 214),
		DBA_VAR(0,  1, 215),
		DBA_VAR(0,  1, 216),
		DBA_VAR(0,  1, 217),
		DBA_VAR(0,  4,   1),
		DBA_VAR(0,  4,   2),
		DBA_VAR(0,  4,   3),
		DBA_VAR(0,  4,   4),
		DBA_VAR(0,  4,   5),
		DBA_VAR(0,  4,   6),
		DBA_VAR(0,  5,   1),
		DBA_VAR(0,  6,   1),
		DBA_VAR(0,  7,  30),
		DBA_VAR(0,  7,  32),
		DBA_VAR(0,  8,  21),
		DBA_VAR(0,  4,  25),
		DBA_VAR(0,  8,  43),
		DBA_VAR(0,  8,  44),
		DBA_VAR(0,  8,  45),
		DBA_VAR(0,  8,  90),
		DBA_VAR(0, 15,  23),
		DBA_VAR(0,  8,  90),
		DBA_VAR(0, 33,   3),
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
/*  0 */ { DBA_VAR(0,  1,  19), DBA_MSG_ST_NAME },
/*  1 */ { DBA_VAR(0,  1, 212), DBA_MSG_POLL_LCODE },
/*  2 */ { DBA_VAR(0,  1, 213), DBA_MSG_POLL_SCODE },
/*  3 */ { DBA_VAR(0,  1, 214), DBA_MSG_POLL_GEMSCODE },
/*  4 */ { DBA_VAR(0,  1, 215), DBA_MSG_POLL_SOURCE },
/*  5 */ { DBA_VAR(0,  1, 216), DBA_MSG_POLL_ATYPE },
/*  6 */ { DBA_VAR(0,  1, 217), DBA_MSG_POLL_TTYPE },
/*  7 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR },
/*  8 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH },
/*  9 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY },
/* 10 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR },
/* 11 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE },
/* 12 */ { DBA_VAR(0,  4,  6), DBA_MSG_SECOND },
/* 13 */ { DBA_VAR(0,  5,  1), DBA_MSG_LATITUDE },
/* 14 */ { DBA_VAR(0,  6,  1), DBA_MSG_LONGITUDE },
/* 15 */ { DBA_VAR(0,  7, 30), DBA_MSG_HEIGHT },
/* 16 */ { DBA_VAR(0,  7, 32), -1 },
/* 17 */ { DBA_VAR(0,  8, 21), -1 },
/* 18 */ { DBA_VAR(0,  4, 25), -1 },
/* 19 */ { DBA_VAR(0,  8, 43), -1 },
/* 20 */ { DBA_VAR(0,  8, 44), -1 },
/* 21 */ { DBA_VAR(0,  8, 45), -1 },
/* 22 */ { DBA_VAR(0,  8, 90), -1 },
/* 23 */ { DBA_VAR(0, 15, 23), -1 },
/* 24 */ { DBA_VAR(0,  8, 90), -1 },
/* 25 */ { DBA_VAR(0, 33,  3), -1 },
};


static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type)
{
	int i, li, di;
	dba_var var = NULL;
	dba_var attr_conf = NULL;
	dba_var attr_cas = NULL;
	dba_var attr_pmc = NULL;
	int l1 = -1, p1 = -1;
	int constituent = -1;

	dba_msg_print(src, stderr);

	// Get the variable out of msg
	for (li = 0; li < src->data_count; ++li)
	{
		if (src->data[li]->ltype != 105) continue;
		for (di = 0; di < src->data[li]->data_count; ++di)
		{
			dba_msg_datum d = src->data[li]->data[di];
			if (d->pind != 3) continue;
			dba_varcode code = dba_var_code(d->var);
			if (code < DBA_VAR(0, 15, 193) || code > DBA_VAR(0, 15, 195)) continue;
			if (var != NULL)
				return dba_error_consistency("found more than one variable to export in one template");
			var = d->var;
			l1 = src->data[li]->l1;
			p1 = d->p1;
		}
	}

	if (var == NULL)
		return dba_error_consistency("found no variable to export");

	// Extract the various attributes
	DBA_RUN_OR_RETURN(dba_var_enqa(var, DBA_VAR(0, 33,   3), &attr_conf));
	DBA_RUN_OR_RETURN(dba_var_enqa(var, DBA_VAR(0,  8,  44), &attr_cas));
	DBA_RUN_OR_RETURN(dba_var_enqa(var, DBA_VAR(0,  8,  45), &attr_pmc));

	// Compute the constituent type
	switch (dba_var_code(var))
	{
		case DBA_VAR(0, 15, 193): constituent =  5; break;
		case DBA_VAR(0, 15, 194): constituent =  0; break;
		case DBA_VAR(0, 15, 195): constituent = 27; break;
		default:
			return dba_error_consistency("found unknown variable type when getting constituent type");
	}

	// Compute the decimal scaling factor


	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case 16: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, l1)); break;
			case 17: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, 2)); break;
			case 18: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, p1/60)); break;
			case 19: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, tpl[i].code, constituent)); break;
			case 20: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, attr_cas)); break;
			case 21: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, attr_pmc)); break;
			case 22: DBA_VAR(0,  8,  90); break;
			case 23: DBA_VAR(0, 15,  23); break;
			case 24: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code)); break;
			case 25: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, attr_conf)); break;
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

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
