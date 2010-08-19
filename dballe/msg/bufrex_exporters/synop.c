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

static dba_err synop_datadesc_func(bufrex_exporter exp, dba_msg src, bufrex_msg dst);

static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type);
static dba_err exporterhigh(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type);

struct _bufrex_exporter bufrex_exporter_synop_0_1 = {
	/* Category */
	0,
	/* Subcategory */
	255,
	/* Local subcategory */
	1,
	/* dba_msg type it can convert from */
	MSG_SYNOP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  7,  5),
		DBA_VAR(0, 13, 11),
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
	/* Datadesc function */
	synop_datadesc_func,
	/* Exporter function */
	(bufrex_exporter_func)exporter,
};

struct _bufrex_exporter bufrex_exporter_synop_0_1high = {
	/* Category */
	0,
	/* Subcategory */
	255,
	/* Local subcategory */
	1,
	/* dba_msg type it can convert from */
	MSG_SYNOP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  7,  7),
		DBA_VAR(0, 13, 11),
		DBA_VAR(0, 13, 13),
		DBA_VAR(2, 22,  0),
		DBA_VAR(1,  1, 34),
		DBA_VAR(0, 31, 31),
		DBA_VAR(0,  1, 31),
		DBA_VAR(0,  1, 32),
		DBA_VAR(1,  1, 34),
		DBA_VAR(0, 33,  7),
		0
	},
	/* Datadesc function */
	synop_datadesc_func,
	/* Exporter function */
	(bufrex_exporter_func)exporterhigh,
};

struct _bufrex_exporter bufrex_exporter_synop_0_3 = {
	/* Category */
	0,
	/* Subcategory */
	255,
	/* Local subcategory */
	3,
	/* dba_msg type it can convert from */
	MSG_SYNOP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  7,  5),
		DBA_VAR(0, 13, 11),
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
	/* Datadesc function */
	synop_datadesc_func,
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
/* 47 */ { DBA_VAR(0, 13, 22), -1 /* DBA_MSG_TOT_PREC12 */ },
/* 48 */ { DBA_VAR(0, 13, 13), DBA_MSG_TOT_SNOW },
};

static struct template tplhigh[] = {
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
/* 12 */ { DBA_VAR(0,  7,  4), DBA_MSG_ISOBARIC_SURFACE },
/* 13 */ { DBA_VAR(0, 10,  3), DBA_MSG_GEOPOTENTIAL },
/* 14 */ { DBA_VAR(0, 10, 61), DBA_MSG_PRESS_3H },
/* 15 */ { DBA_VAR(0, 10, 63), DBA_MSG_PRESS_TEND },
/* 16 */ { DBA_VAR(0, 11, 11), DBA_MSG_WIND_DIR },
/* 17 */ { DBA_VAR(0, 11, 12), DBA_MSG_WIND_SPEED },
/* 18 */ { DBA_VAR(0, 12,  4), DBA_MSG_TEMP_2M },
/* 19 */ { DBA_VAR(0, 12,  6), DBA_MSG_DEWPOINT_2M },
/* 20 */ { DBA_VAR(0, 13,  3), DBA_MSG_HUMIDITY },
/* 21 */ { DBA_VAR(0, 20,  1), DBA_MSG_VISIBILITY },
/* 22 */ { DBA_VAR(0, 20,  3), DBA_MSG_PRES_WTR },
/* 23 */ { DBA_VAR(0, 20,  4), DBA_MSG_PAST_WTR1 },
/* 24 */ { DBA_VAR(0, 20,  5), DBA_MSG_PAST_WTR2 },
/* 25 */ { DBA_VAR(0, 20, 10), DBA_MSG_CLOUD_N },
/* 26 */ { DBA_VAR(0,  8,  2), -1 },
/* 27 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_NH },
/* 28 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_HH },
/* 29 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CL },
/* 30 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CM },
/* 31 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CH },
/* 32 */ { DBA_VAR(0, 13, 22), -1 /* DBA_MSG_TOT_PREC12 */ },
/* 33 */ { DBA_VAR(0, 13, 13), DBA_MSG_TOT_SNOW },
};


static dba_err exporter(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type)
{
	int i;
	dba_varcode prectype = DBA_VAR(0, 13, 23);
	bufrex_opcode op;
	dba_var var;

	/* Check what precipitation type we are supposed to use */
	for (op = bmsg->datadesc; op != NULL; op = op->next)
	{
		if (DBA_VAR_F(op->val) == 0 && DBA_VAR_X(op->val) == 13 &&
				DBA_VAR_Y(op->val) >= 19 && DBA_VAR_Y(op->val) <= 23)
		{
			prectype = op->val;
			break;
		}
	}

	for (i = 0; i < sizeof(tpl)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case 25:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 258, 0, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			case 31:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 259, 1, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			case 35:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 259, 2, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			case 39:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 259, 3, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			case 43:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 259, 4, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tpl[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tpl[i].code));
				break;
			case 47:
				switch (prectype)
				{
					case DBA_VAR(0, 13, 23): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC24); break;
					case DBA_VAR(0, 13, 22): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC12); break;
					case DBA_VAR(0, 13, 21): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC6); break;
					case DBA_VAR(0, 13, 20): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC3); break;
					case DBA_VAR(0, 13, 19): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC1); break;
					default: var = NULL;
				}
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, prectype, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, prectype));
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
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_dpb(dst, 49));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 31), ORIG_CENTRE_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 32), ORIG_APP_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_attrs(dst, 49, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

static dba_err exporterhigh(dba_msg src, bufrex_msg bmsg, bufrex_subset dst, int type)
{
	int i;
	dba_varcode prectype = DBA_VAR(0, 13, 23);
	bufrex_opcode op;
	dba_var var;

	/* Check what precipitation type we are supposed to use */
	for (op = bmsg->datadesc; op != NULL; op = op->next)
	{
		if (DBA_VAR_F(op->val) == 0 && DBA_VAR_X(op->val) == 13 &&
				DBA_VAR_Y(op->val) >= 19 && DBA_VAR_Y(op->val) <= 23)
		{
			prectype = op->val;
			break;
		}
	}

	for (i = 0; i < sizeof(tplhigh)/sizeof(struct template); i++)
	{
		switch (i)
		{
			case 26:
				var = dba_msg_find(src, DBA_VAR(0, 8, 2), 256, 0, 258, 0, 254, 0, 0);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tplhigh[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tplhigh[i].code));
				break;
			case 32:
				switch (prectype)
				{
					case DBA_VAR(0, 13, 23): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC24); break;
					case DBA_VAR(0, 13, 22): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC12); break;
					case DBA_VAR(0, 13, 21): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC6); break;
					case DBA_VAR(0, 13, 20): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC3); break;
					case DBA_VAR(0, 13, 19): var = dba_msg_find_by_id(src, DBA_MSG_TOT_PREC1); break;
					default: var = NULL;
				}
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, prectype, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, prectype));
				break;
			default:
				var = dba_msg_find_by_id(src, tplhigh[i].var);
				if (var != NULL)
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_var(dst, tplhigh[i].code, var));
				else
					DBA_RUN_OR_RETURN(bufrex_subset_store_variable_undef(dst, tplhigh[i].code));
				break;
		}
	}

	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_dpb(dst, 34));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 31), ORIG_CENTRE_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_store_variable_i(dst, DBA_VAR(0, 1, 32), ORIG_APP_ID));
		DBA_RUN_OR_RETURN(bufrex_subset_append_fixed_attrs(dst, 34, DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}


static dba_err synop_datadesc_func(bufrex_exporter exp, dba_msg src, bufrex_msg dst)
{
	int i;
	/* Init the bufrex_msg data descriptor chain */
	for (i = 0; exp->ddesc[i] != 0; i++)
	{
		/* Skip encoding of attributes for CREX */
		if (dst->encoding_type == BUFREX_CREX && exp->ddesc[i] == DBA_VAR(2, 22, 0))
			break;
		/* Use the best kind of precipitation found in the message to encode */
		if (exp->ddesc[i] == DBA_VAR(0, 13, 11))
		{
			dba_varcode code = DBA_VAR(0, 13, 23);
			if (dba_msg_find_by_id(src, DBA_MSG_TOT_PREC24) != NULL)
				code = DBA_VAR(0, 13, 23);
			else if (dba_msg_find_by_id(src, DBA_MSG_TOT_PREC12) != NULL)
				code = DBA_VAR(0, 13, 22);
			else if (dba_msg_find_by_id(src, DBA_MSG_TOT_PREC6) != NULL)
				code = DBA_VAR(0, 13, 21);
			else if (dba_msg_find_by_id(src, DBA_MSG_TOT_PREC3) != NULL)
				code = DBA_VAR(0, 13, 20);
			else if (dba_msg_find_by_id(src, DBA_MSG_TOT_PREC1) != NULL)
				code = DBA_VAR(0, 13, 19);
			DBA_RUN_OR_RETURN(bufrex_msg_append_datadesc(dst, code));
		} else
			DBA_RUN_OR_RETURN(bufrex_msg_append_datadesc(dst, exp->ddesc[i]));
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
