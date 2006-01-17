#include "exporters.h"

static dba_err exporter91(dba_msg msg, bufrex_raw dst, int type);
static dba_err exporter101(dba_msg msg, bufrex_raw dst, int type);
static dba_err exporter102(dba_msg msg, bufrex_raw dst, int type);


bufrex_exporter bufrex_exporter_sounding_2_91 = {
	/* Category */
	2,
	/* Subcategory */
	91,
	/* dba_msg type it can convert from */
	MSG_TEMP,
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

bufrex_exporter bufrex_exporter_sounding_2_101 = {
	/* Category */
	2,
	/* Subcategory */
	101,
	/* dba_msg type it can convert from */
	MSG_TEMP,
	/* Data descriptor section */
	(dba_varcode[]){
		DBA_VAR(3,  9,  7),
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
	(bufrex_exporter_func)exporter101,
};

bufrex_exporter bufrex_exporter_sounding_2_102 = {
	/* Category */
	2,
	/* Subcategory */
	102,
	/* dba_msg type it can convert from */
	MSG_TEMP_SHIP,
	/* Data descriptor section */
	/* FIXME: some expansions are not in CREX D tables yet, so we need to use
	 * the expanded version here.  The code needs to be changed when newer CREX
	 * D tables are available */
	(dba_varcode[]){
 	    /* DBA_VAR(3,  9, 196), Replaced with expansion: */
			DBA_VAR(3, 1,  3),
			DBA_VAR(0, 2, 11),
			DBA_VAR(0, 2, 12),
			DBA_VAR(3, 1, 11),
			DBA_VAR(3, 1, 12),
			DBA_VAR(3, 1, 23),
			DBA_VAR(0, 7,  1),
		DBA_VAR(3,  2,  4),
		DBA_VAR(1,  1,  0),
		DBA_VAR(0, 31,  1),
		DBA_VAR(3,  3, 14),
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
	(bufrex_exporter_func)exporter102,
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

static int count_levels(dba_msg msg)
{
	int lev_no = 0;
	int i;

	/* Count the number of levels */
	for (i = 0; i < msg->data_count; i++)
		if (msg->data[i]->ltype == 100 &&
				dba_msg_level_find(msg->data[i], DBA_VAR(0, 8, 1), 0, 0, 0) != NULL)
			lev_no++;

	return lev_no;
}

static dba_err add_sounding_levels(dba_msg msg, bufrex_raw dst, dba_varcode* tpl, int tpl_count)
{
	int i;
	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		double press = lev->l1;
		dba_msg_datum d;
		int j;

		if (lev->ltype != 100 ||
				(d = dba_msg_level_find(lev, DBA_VAR(0, 8, 1), 0, 0, 0)) == NULL)
			continue;

		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_d(dst, DBA_VAR(0,  7,  4), press));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, DBA_VAR(0,  8,  1), d->var));

		for (j = 0; j < tpl_count; j++)
			if ((d = dba_msg_level_find(lev, tpl[j], 0, 0, 0)) != NULL)
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(dst, tpl[j], d->var));
			else
				DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, tpl[j]));
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
static dba_varcode levtpl91[] = { DBA_VAR(0, 10, 3), DBA_VAR(0, 11, 1), DBA_VAR(0, 11, 2) };

static struct template tpl101[] = {
/*  0 */ { DBA_VAR(0,  1,  1), DBA_MSG_BLOCK		},
/*  1 */ { DBA_VAR(0,  1,  2), DBA_MSG_STATION		},
/*  2 */ { DBA_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE	},
/*  3 */ { DBA_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD	},
/*  4 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR			},
/*  5 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH		},
/*  6 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY			},
/*  7 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR			},
/*  8 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE		},
/*  9 */ { DBA_VAR(0,  5,  1), DBA_MSG_LATITUDE		},
/* 10 */ { DBA_VAR(0,  6,  1), DBA_MSG_LONGITUDE	},
/* 11 */ { DBA_VAR(0,  7,  1), DBA_MSG_HEIGHT		},
/* 12 */ { DBA_VAR(0, 20, 10), DBA_MSG_CLOUD_N		},
/* 13 */ { DBA_VAR(0,  8,  2), -1					},
/* 14 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_NH		},
/* 15 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_HH		},
/* 16 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CL		},
/* 17 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CM		},
/* 18 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CH		},
};
static dba_varcode levtpl101[] = {
	DBA_VAR(0, 10, 3),
	DBA_VAR(0, 12, 1),
	DBA_VAR(0, 12, 3),
	DBA_VAR(0, 11, 1),
	DBA_VAR(0, 11, 2),
};

static struct template tpl102[] = {
/*  0 */ { DBA_VAR(0,  1, 11), DBA_MSG_IDENT		},
/*  1 */ { DBA_VAR(0,  1, 12), DBA_MSG_ST_DIR		},
/*  2 */ { DBA_VAR(0,  1, 13), DBA_MSG_ST_SPEED		},
/*  3 */ { DBA_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE	},
/*  4 */ { DBA_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD	},
/*  5 */ { DBA_VAR(0,  4,  1), DBA_MSG_YEAR			},
/*  6 */ { DBA_VAR(0,  4,  2), DBA_MSG_MONTH		},
/*  7 */ { DBA_VAR(0,  4,  3), DBA_MSG_DAY			},
/*  8 */ { DBA_VAR(0,  4,  4), DBA_MSG_HOUR			},
/*  9 */ { DBA_VAR(0,  4,  5), DBA_MSG_MINUTE		},
/* 10 */ { DBA_VAR(0,  5,  2), DBA_MSG_LATITUDE		},
/* 11 */ { DBA_VAR(0,  6,  2), DBA_MSG_LONGITUDE	},
/* 12 */ { DBA_VAR(0,  7,  1), DBA_MSG_HEIGHT		},
/* 13 */ { DBA_VAR(0, 20, 10), DBA_MSG_CLOUD_N		},
/* 14 */ { DBA_VAR(0,  8,  2), -1					},
/* 15 */ { DBA_VAR(0, 20, 11), DBA_MSG_CLOUD_NH		},
/* 16 */ { DBA_VAR(0, 20, 13), DBA_MSG_CLOUD_HH		},
/* 17 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CL		},
/* 18 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CM		},
/* 19 */ { DBA_VAR(0, 20, 12), DBA_MSG_CLOUD_CH		},
};
static dba_varcode levtpl102[] = {
	DBA_VAR(0, 10, 3),
	DBA_VAR(0, 12, 1),
	DBA_VAR(0, 12, 3),
	DBA_VAR(0, 11, 1),
	DBA_VAR(0, 11, 2),
};

static dba_err exporter91(dba_msg msg, bufrex_raw dst, int type)
{
	const int tplsize = sizeof(tpl91)/sizeof(struct template);
	const int levsize = sizeof(levtpl91)/sizeof(dba_varcode);
	int lev_no = count_levels(msg);

	/* Fill up the message */
	DBA_RUN_OR_RETURN(run_template(msg, dst, tpl91, tplsize));
	DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, DBA_VAR(0, 31,  1), lev_no));
	DBA_RUN_OR_RETURN(add_sounding_levels(msg, dst, levtpl91, levsize));
	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_dpb(dst, tplsize + 1 + lev_no * (levsize + 2), DBA_VAR(0, 33, 7)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 32)));
		DBA_RUN_OR_RETURN(bufrex_raw_append_attrs(dst, tplsize + 1 + lev_no * (levsize + 2), DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

static dba_err exporter101(dba_msg msg, bufrex_raw dst, int type)
{
	const int tplsize = sizeof(tpl101)/sizeof(struct template);
	const int levsize = sizeof(levtpl101)/sizeof(dba_varcode);
	int lev_no = count_levels(msg);

	/* Fill up the message */
	DBA_RUN_OR_RETURN(run_template(msg, dst, tpl101, tplsize));
	DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, DBA_VAR(0, 31,  1), lev_no));
	DBA_RUN_OR_RETURN(add_sounding_levels(msg, dst, levtpl101, levsize));
	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_dpb(dst, tplsize + 1 + lev_no * (levsize + 2), DBA_VAR(0, 33, 7)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 32)));
		DBA_RUN_OR_RETURN(bufrex_raw_append_attrs(dst, tplsize + 1 + lev_no * (levsize + 2), DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

static dba_err exporter102(dba_msg msg, bufrex_raw dst, int type)
{
	const int tplsize = sizeof(tpl102)/sizeof(struct template);
	const int levsize = sizeof(levtpl102)/sizeof(dba_varcode);
	int lev_no = count_levels(msg);

	/* Fill up the message */
	DBA_RUN_OR_RETURN(run_template(msg, dst, tpl102, tplsize));
	DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(dst, DBA_VAR(0, 31,  1), lev_no));
	DBA_RUN_OR_RETURN(add_sounding_levels(msg, dst, levtpl102, levsize));
	if (type == 0)
	{
		DBA_RUN_OR_RETURN(bufrex_raw_append_dpb(dst, tplsize + 1 + lev_no * (levsize + 2), DBA_VAR(0, 33, 7)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 31)));
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(dst, DBA_VAR(0, 1, 32)));
		DBA_RUN_OR_RETURN(bufrex_raw_append_attrs(dst, tplsize + 1 + lev_no * (levsize + 2), DBA_VAR(0, 33, 7)));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
