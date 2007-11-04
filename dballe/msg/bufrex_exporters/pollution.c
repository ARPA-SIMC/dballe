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

struct _bufrex_exporter bufrex_exporter_pollution_13_102 = {
	/* Category */
	13,
	/* Subcategory */
	102,
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
/*  0 */ { DBA_VAR(0,  1,  19), -1 },
/*  1 */ { DBA_VAR(0,  1, 212), -1 },
/*  2 */ { DBA_VAR(0,  1, 213), -1 },
/*  3 */ { DBA_VAR(0,  1, 214), -1 },
/*  4 */ { DBA_VAR(0,  1, 215), -1 },
/*  5 */ { DBA_VAR(0,  1, 216), -1 },
/*  6 */ { DBA_VAR(0,  1, 217), -1 },
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
	int i;
	bufrex_opcode op;

	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case  0: DBA_VAR(0,  1,  19); break;
			case  1: DBA_VAR(0,  1, 212); break;
			case  2: DBA_VAR(0,  1, 213); break;
			case  3: DBA_VAR(0,  1, 214); break;
			case  4: DBA_VAR(0,  1, 215); break;
			case  5: DBA_VAR(0,  1, 216); break;
			case  6: DBA_VAR(0,  1, 217); break;
			case 16: DBA_VAR(0,  7,  32); break;
			case 17: DBA_VAR(0,  8,  21); break;
			case 18: DBA_VAR(0,  4,  25); break;
			case 19: DBA_VAR(0,  8,  43); break;
			case 20: DBA_VAR(0,  8,  44); break;
			case 21: DBA_VAR(0,  8,  45); break;
			case 22: DBA_VAR(0,  8,  90); break;
			case 23: DBA_VAR(0, 15,  23); break;
			case 24: DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code)); break;
			case 25: DBA_VAR(0, 33,   3); break;
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
