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

#include <dballe/db/cursor.h>
#include <dballe/db/internals.h>
#include <dballe/db/repinfo.h>
#include <dballe/core/aliases.h>

#include <sql.h>
#include <sqlext.h>

#include <stdlib.h>
#include <string.h>
#if 0
#include <dballe/db/repinfo.h>
#include <dballe/db/pseudoana.h>
#include <dballe/db/context.h>
#include <dballe/db/data.h>
#include <dballe/db/attr.h>
#include <dballe/core/dba_record.h>
#include <dballe/core/dba_var.h>
#include <dballe/core/csv.h>
#include <dballe/core/verbose.h>
#include <dballe/core/aliases.h>

#include <config.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <stdio.h>
#include <stdarg.h>

#include <assert.h>
#endif



dba_err dba_db_cursor_create(dba_db db, dba_db_cursor* cur)
{
	dba_err err;

	if ((*cur = (dba_db_cursor)malloc(sizeof(struct _dba_db_cursor))) == NULL)
		return dba_error_alloc("trying to allocate a new dba_db_cursor object");
	(*cur)->db = db;
	(*cur)->stm = NULL;
	(*cur)->query = NULL;
	(*cur)->where = NULL;

	DBA_RUN_OR_GOTO(fail, dba_db_statement_create(db, &((*cur)->stm)));
	DBA_RUN_OR_GOTO(fail, dba_querybuf_create(2048, &((*cur)->query)));
	DBA_RUN_OR_GOTO(fail, dba_querybuf_create(1024, &((*cur)->where)));
	return dba_error_ok();

fail:
	dba_db_cursor_delete(*cur);
	*cur = 0;
	return err;
}

void dba_db_cursor_delete(dba_db_cursor cur)
{
	if (cur->stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, cur->stm);
	if (cur->query != NULL)
		dba_querybuf_delete(cur->query);
	if (cur->where != NULL)
		dba_querybuf_delete(cur->where);
	free(cur);
}

int dba_db_cursor_remaining(dba_db_cursor cur)
{
	return cur->count;
}

static dba_err decode_data_filter(const char* filter, dba_varinfo* info, const char** op, const char** val)
{
	static char operator[5];
	static char value[255];
	size_t len = strcspn(filter, "<=>");
	const char* s = filter + len;
	dba_varcode code;

	if (filter[0] == 0)
		return dba_error_consistency("filter is the empty string");

	/* Parse the varcode */
	if (filter[0] == 'B')
		code = DBA_STRING_TO_VAR(filter + 1);
	else
	    code = dba_varcode_alias_resolve_substring(filter, len);
	
	if (code == 0)
		return dba_error_consistency("cannot resolve the variable code or alias in \"%s\" (len %d)", filter, len);

	/* Query informations for the varcode */
	DBA_RUN_OR_RETURN(dba_varinfo_query_local(code, info));

	/* Parse the operator */
	len = strspn(s, "<=>");
	if (len == 0 || len > 4)
		return dba_error_consistency("invalid operator found in \"%s\"", filter);

	memcpy(operator, s, len);
	operator[len] = 0;
	*op = operator;

	/* Parse the value */
	s += len;
	if ((*info)->is_string)
	{
		/* Copy the string, escaping quotes */
		int i = 0, j = 0;
		value[j++] = '\'';
		for (; s[i] && j < 253; ++i, ++j)
		{
			if (s[i] == '\'')
				value[j++] = '\\';
			value[j] = s[i];
		}
		value[j++] = '\'';
		value[j] = 0;
	}
	else
	{
		dba_var tmpvar;
		double dval;
		if (sscanf(s, "%lf", &dval) != 1)
			return dba_error_consistency("value in \"%s\" must be a number", filter);
		DBA_RUN_OR_RETURN(dba_var_created(*info, dval, &tmpvar));
		strncpy(value, dba_var_value(tmpvar), 255);
		value[254] = 0;
		dba_var_delete(tmpvar);
	}
	*val = value;
	return dba_error_ok();
}


static dba_err init_modifiers(dba_db_cursor cur, dba_record query)
{
	const char* val;

	/* Decode query modifiers */
	if ((val = dba_record_key_peek_value(query, DBA_KEY_QUERY)) != NULL)
	{
		const char* s = val;
		while (*s)
		{
			size_t len = strcspn(s, ",");
			int got = 1;
			switch (len)
			{
				case 0:
					/* If it's an empty token, skip it */
					break;
				case 4:
					/* "best": if more values exist in a point, get only the
					   best one */
					if (strncmp(s, "best", 4) == 0)
					{
						cur->modifiers |= DBA_DB_MODIFIER_BEST;
						cur->from_wanted |= DBA_DB_FROM_D;
						cur->from_wanted |= DBA_DB_FROM_RI;
					}
					else
						got = 0;
					break;
				case 6:
					/* "bigana": optimize with date first */
					if (strncmp(s, "bigana", 6) == 0)
						cur->modifiers |= DBA_DB_MODIFIER_BIGANA;
					else
						got = 0;
					break;
				default:
					got = 0;
					break;
			}

			/* Check that we parsed it correctly */
			if (!got)
				return dba_error_consistency("Query modifier \"%.*s\" is not recognized", len, s);

			/* Move to the next token */
			s += len;
			if (*s == ',')
				++s;
		}
	}

	return dba_error_ok();
}

/* Prepare SELECT Part and see what needs to be available in the FROM part */
static dba_err make_select(dba_db_cursor cur)
{
	DBA_RUN_OR_RETURN(dba_querybuf_start_list(cur->query, ", "));
	if (cur->wanted & DBA_DB_WANT_COORDS)
	{
		cur->from_wanted |= DBA_DB_FROM_PA;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.lat"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_lat), sizeof(cur->out_lat), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.lon"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_lon), sizeof(cur->out_lon), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_IDENT)
	{
		cur->from_wanted |= DBA_DB_FROM_PA;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.ident"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_CHAR, &(cur->out_ident), sizeof(cur->out_ident), &(cur->out_ident_ind));
	}
	if (cur->wanted & DBA_DB_WANT_LEVEL)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.ltype"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_ltype), sizeof(cur->out_ltype), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.l1"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_l1), sizeof(cur->out_l1), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.l2"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_l2), sizeof(cur->out_l2), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_TIMERANGE)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.ptype"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_pind), sizeof(cur->out_pind), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.p1"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_p1), sizeof(cur->out_p1), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.p2"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_p2), sizeof(cur->out_p2), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_DATETIME)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.datetime"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_CHAR, &(cur->out_datetime), sizeof(cur->out_datetime), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_REPCOD)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id_report"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_rep_cod), sizeof(cur->out_rep_cod), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_VAR_NAME)
	{
		cur->from_wanted |= DBA_DB_FROM_D;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "d.id_var"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_idvar), sizeof(cur->out_idvar), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_VAR_VALUE)
	{
		cur->from_wanted |= DBA_DB_FROM_D;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "d.value"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_CHAR, &(cur->out_value), sizeof(cur->out_value), NULL);
	}

	/* If querybest is used, then we need ri.prio here so that GROUP BY can use it */
	if (cur->modifiers & DBA_DB_MODIFIER_BEST)
	{
		cur->from_wanted |= DBA_DB_FROM_RI;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "ri.prio"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_priority), sizeof(cur->out_context_id), NULL);
	}

	/* For these parameters we can try to be opportunistic and avoid extra joins */
	if (cur->wanted & DBA_DB_WANT_ANA_ID)
	{
		if (!(cur->from_wanted & DBA_DB_FROM_PA) && cur->from_wanted & DBA_DB_FROM_C) {
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id_ana"));
			SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_ana_id), sizeof(cur->out_ana_id), NULL);
		} else {
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.id"));
			SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_ana_id), sizeof(cur->out_ana_id), NULL);
			cur->from_wanted |= DBA_DB_FROM_PA;
		}
	}
	if (cur->wanted & DBA_DB_WANT_CONTEXT_ID)
	{
		if (!(cur->from_wanted & DBA_DB_FROM_C) && cur->from_wanted & DBA_DB_FROM_D) {
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "d.id_context"));
			SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_context_id), sizeof(cur->out_context_id), NULL);
		} else {
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id"));
			SQLBindCol(cur->stm, cur->output_seq++, SQL_C_SLONG, &(cur->out_context_id), sizeof(cur->out_context_id), NULL);
			cur->from_wanted |= DBA_DB_FROM_C;
		}
	}

	return dba_error_ok();
}

static dba_err add_int(
		dba_db_cursor cur,
		dba_record query,
		int* out,
		dba_keyword key,
		const char* sql,
		int needed_from)
{
	const char* val;
	if ((val = dba_record_key_peek_value(query, key)) != NULL) {
		*out = strtol(val, 0, 10);
		//TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, sql));
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, out, 0, 0);
		cur->from_wanted |= needed_from;
	}
	return dba_error_ok();
}

static dba_err make_where(dba_db_cursor cur, dba_record query)
{
#define ADD_INT(field, key, sql, needed) DBA_RUN_OR_RETURN(add_int(cur, query, field, key, sql, needed))
	const char* val;

	DBA_RUN_OR_RETURN(dba_querybuf_start_list(cur->where, " AND "));

//	fprintf(stderr, "A1 '%s'\n", dba_querybuf_get(cur->where));

	ADD_INT(&cur->sel_ana_id, DBA_KEY_ANA_ID, "pa.id=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_latmin, DBA_KEY_LAT, "pa.lat=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_latmin, DBA_KEY_LATMIN, "pa.lat>=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_latmax, DBA_KEY_LATMAX, "pa.lat<=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_lonmin, DBA_KEY_LON, "pa.lon=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_lonmin, DBA_KEY_LONMIN, "pa.lon>=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_lonmax, DBA_KEY_LONMAX, "pa.lon<=?", DBA_DB_FROM_PA);

//	fprintf(stderr, "A2 '%s'\n", dba_querybuf_get(cur->where));

	if ((val = dba_record_key_peek_value(query, DBA_KEY_MOBILE)) != NULL)
	{
		if (val[0] == '0')
		{
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "pa.ident IS NULL"));
			TRACE("found fixed/mobile: adding AND pa.ident IS NULL.\n");
		} else {
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "NOT (pa.ident IS NULL)"));
			TRACE("found fixed/mobile: adding AND NOT (pa.ident IS NULL)\n");
		}
		cur->from_wanted |= DBA_DB_FROM_PA;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_IDENT)) != NULL)
	{
		strncpy(cur->sel_ident, val, 64);
		cur->sel_ident[63] = 0;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "pa.ident=?"));
		TRACE("found ident: adding AND pa.ident = ?.  val is %s\n", cur->sel_ident);
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)cur->sel_ident, 0, 0);
		cur->from_wanted |= DBA_DB_FROM_PA;
	}

	/* Set the time extremes */
	{
		int minvalues[6], maxvalues[6];
		DBA_RUN_OR_RETURN(dba_record_parse_date_extremes(query, minvalues, maxvalues));
		
		if (minvalues[0] != -1 || maxvalues[0] != -1)
		{
			if (memcmp(minvalues, maxvalues, 6 * sizeof(int)) == 0)
			{
				/* Add constraint on the exact date interval */
				snprintf(cur->sel_dtmin, 25, "%04d-%02d-%02d %02d:%02d:%02d",
						minvalues[0], minvalues[1], minvalues[2],
						minvalues[3], minvalues[4], minvalues[5]);
				DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime=?"));
				TRACE("found exact time: adding AND c.datetime = ?.  val is %s\n", cur->sel_dtmin);
				SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)cur->sel_dtmin, 0, 0);
				cur->from_wanted |= DBA_DB_FROM_C;
			}
			else
			{
				if (minvalues[0] != -1)
				{
					/* Add constraint on the minimum date interval */
					snprintf(cur->sel_dtmin, 25, "%04d-%02d-%02d %02d:%02d:%02d",
							minvalues[0], minvalues[1], minvalues[2],
							minvalues[3], minvalues[4], minvalues[5]);
					DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime>=?"));
					TRACE("found min time interval: adding AND c.datetime >= ?.  val is %s\n", cur->sel_dtmin);
					SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)cur->sel_dtmin, 0, 0);
					cur->from_wanted |= DBA_DB_FROM_C;
				}
				if (maxvalues[0] != -1)
				{
					snprintf(cur->sel_dtmax, 25, "%04d-%02d-%02d %02d:%02d:%02d",
							maxvalues[0], maxvalues[1], maxvalues[2],
							maxvalues[3], maxvalues[4], maxvalues[5]);
					DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime<=?"));
					TRACE("found max time interval: adding AND c.datetime <= ?.  val is %s\n", cur->sel_dtmax);
					SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)cur->sel_dtmax, 0, 0);
					cur->from_wanted |= DBA_DB_FROM_C;
				}
			}
		}

		if (minvalues[0] == 1000 || maxvalues[0] == 1000)
			cur->want_ana_context = 1;
	}

//	fprintf(stderr, "A3 '%s'\n", dba_querybuf_get(cur->where));

	ADD_INT(&cur->sel_ltype, DBA_KEY_LEVELTYPE, "c.ltype=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_l1, DBA_KEY_L1, "c.l1=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_l2, DBA_KEY_L2, "c.l2=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_pind, DBA_KEY_PINDICATOR, "c.ptype=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_p1, DBA_KEY_P1, "c.p1=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_p2, DBA_KEY_P2, "c.p2=?", DBA_DB_FROM_C);
	// FIXME ADD_INT(&cur->sel_context_id, DBA_KEY_CONTEXT_ID, "c.id = ?", DBA_DB_FROM_C);

	/* rep_memo has priority over rep_cod */
	if ((val = dba_record_key_peek_value(query, DBA_KEY_REP_MEMO)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_db_repinfo_get_id(cur->db->repinfo, val, &(cur->sel_rep_cod)));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.id_report=?"));
		TRACE("found rep_memo %s: adding AND c.id_report = ?. val is %d\n", val, cur->sel_rep_cod);
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->sel_rep_cod), 0, 0);
		cur->from_wanted |= DBA_DB_FROM_C;
	} else
		ADD_INT(&cur->sel_rep_cod, DBA_KEY_REP_COD, "c.id_report=?", DBA_DB_FROM_C);


	if ((val = dba_record_key_peek_value(query, DBA_KEY_VAR)) != NULL)
	{
		cur->sel_b = dba_descriptor_code(val);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "d.id_var=?"));
		TRACE("found b: adding AND d.id_var = ?. val is %d %s\n", cur->sel_b, val);
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &cur->sel_b, 0, 0);
		cur->from_wanted |= DBA_DB_FROM_D;
	}
	if ((val = dba_record_key_peek_value(query, DBA_KEY_VARLIST)) != NULL)
	{
		size_t pos;
		size_t len;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "d.id_var IN ("));
		for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
		{
			dba_varcode code = DBA_STRING_TO_VAR(val + pos + 1);
			if (pos == 0)
				DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d", code));
			else
				DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, ",%d", code));
		}
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->where, ")"));
		TRACE("found blist: adding AND d.id_var IN (%s)\n", val);
		cur->from_wanted |= DBA_DB_FROM_D;
	}

	ADD_INT(&cur->sel_priority, DBA_KEY_PRIORITY, "ri.prio=?", DBA_DB_FROM_RI);
	ADD_INT(&cur->sel_priomin, DBA_KEY_PRIOMIN, "ri.prio>=?", DBA_DB_FROM_RI);
	ADD_INT(&cur->sel_priomax, DBA_KEY_PRIOMAX, "ri.prio<=?", DBA_DB_FROM_RI);

	if ((val = dba_record_key_peek_value(query, DBA_KEY_BLOCK)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "dblo.value="));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->where, val));
		cur->from_wanted |= DBA_DB_FROM_DBLO;
	}
	if ((val = dba_record_key_peek_value(query, DBA_KEY_STATION)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "dsta.value="));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->where, val));
		cur->from_wanted |= DBA_DB_FROM_DSTA;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_ANA_FILTER)) != NULL)
	{
		dba_varinfo info;
		const char* op;
		const char* value;
		DBA_RUN_OR_RETURN(decode_data_filter(val, &info, &op, &value));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "dana.id_var="));
		DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND dana.value%s%s",
				info->var, op, value));
		cur->from_wanted |= DBA_DB_FROM_DANA;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_DATA_FILTER)) != NULL)
	{
		dba_varinfo info;
		const char* op;
		const char* value;
		DBA_RUN_OR_RETURN(decode_data_filter(val, &info, &op, &value));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "ddf.id_var="));
		DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND ddf.value%s%s",
				info->var, op, value));
		cur->from_wanted |= DBA_DB_FROM_DDF;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_ATTR_FILTER)) != NULL)
	{
		dba_varinfo info;
		const char* op;
		const char* value;
		DBA_RUN_OR_RETURN(decode_data_filter(val, &info, &op, &value));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "adf.type="));
		DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND adf.value%s%s",
				info->var, op, value));
		cur->from_wanted |= DBA_DB_FROM_ADF;
	}

//	fprintf(stderr, "A15 '%s'\n", dba_querybuf_get(cur->where));

	return dba_error_ok();
#undef ADD_INT
}

static dba_err add_other_froms(dba_db_cursor cur, unsigned int base)
{
	/* Remove the base table from the things to add */
	unsigned int wanted = cur->from_wanted & ~base;

	if (wanted & DBA_DB_FROM_PA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN pseudoana AS pa ON c.id_ana = pa.id "));

	if (wanted & DBA_DB_FROM_C)
		switch (base)
		{
			case DBA_DB_FROM_PA:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN context AS c ON c.id_ana=pa.id "));
				break;
			case DBA_DB_FROM_D:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN context AS c ON c.id=d.id_context "));
				break;
			case DBA_DB_FROM_RI:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN context AS c ON c.id_report=ri.id "));
				break;
			default:
				return dba_error_consistency("requested to add a JOIN on context on the unsupported base %d", base);
		}

	if (wanted & DBA_DB_FROM_CBS)
		switch (base)
		{
			case DBA_DB_FROM_PA:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
							" JOIN context AS cbs ON pa.id=cbs.id_ana"
							" AND cbs.id_report=254"
							" AND cbs.datetime='1000-01-01 00:00:00'"
							" AND cbs.ltype=257 AND cbs.l1=0 AND cbs.l2=0"
							" AND cbs.ptype=0 AND cbs.p1=0 AND cbs.p2=0 "));
				break;
			case DBA_DB_FROM_C:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
							" JOIN context AS cbs ON c.id_ana=cbs.id_ana"
							" AND cbs.id_report=254"
							" AND cbs.datetime='1000-01-01 00:00:00'"
							" AND cbs.ltype=257 AND cbs.l1=0 AND cbs.l2=0"
							" AND cbs.ptype=0 AND cbs.p1=0 AND cbs.p2=0 "));
				break;
			default:
				return dba_error_consistency("requested to add a JOIN on anagraphical context on the unsupported base %d", base);
		}

	if (wanted & DBA_DB_FROM_D)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN data AS d ON d.id_context=c.id "));

	if (wanted & DBA_DB_FROM_RI)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN repinfo AS ri ON ri.id=c.id_report "));

	if (wanted & DBA_DB_FROM_DBLO)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data AS dblo ON dblo.id_context=cbs.id AND dblo.id_var=257 "));
	if (wanted & DBA_DB_FROM_DSTA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data AS dsta ON dsta.id_context=cbs.id AND dsta.id_var=258 "));

	if (wanted & DBA_DB_FROM_DANA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data AS dana ON dana.id_context=cbs.id "));

	if (wanted & DBA_DB_FROM_DDF)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data AS ddf ON ddf.id_context=c.id "));

	if (wanted & DBA_DB_FROM_ADF)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN attr AS adf ON adf.id_context=c.id AND adf.id_var=d.id "));

	return dba_error_ok();
}

dba_err dba_db_cursor_query(dba_db_cursor cur, dba_record query, unsigned int wanted, unsigned int modifiers)
{
	const char* val;

	/* Reset the cursor to start a new query */
	dba_querybuf_reset(cur->query);
	dba_querybuf_reset(cur->where);
	cur->wanted = wanted;
	cur->from_wanted = 0;
	cur->input_seq = 1;
	cur->output_seq = 1;
	cur->want_ana_context = 0;

	/* Scan query modifiers */
	cur->modifiers = modifiers;
	DBA_RUN_OR_RETURN(init_modifiers(cur, query));

	DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "SELECT "));
	if (cur->modifiers & DBA_DB_MODIFIER_DISTINCT)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "DISTINCT "));
	if (cur->modifiers & DBA_DB_MODIFIER_BIGANA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "straight_join "));

	/* Prepare WHERE part and see what needs to be available in the FROM part */
	DBA_RUN_OR_RETURN(make_where(cur, query));

	/* Prepare SELECT Part and see what needs to be available in the FROM part.
	 * We do this after creating the WHERE part, so that we can add
	 * more opportunistic extra values (see the end of make_select) */
	DBA_RUN_OR_RETURN(make_select(cur));

	/* Enforce join dependencies */
	if (cur->from_wanted & (DBA_DB_FROM_DBLO | DBA_DB_FROM_DSTA | DBA_DB_FROM_DANA))
		cur->from_wanted |= DBA_DB_FROM_CBS;
	if (cur->from_wanted & (DBA_DB_FROM_DDF))
		cur->from_wanted |= DBA_DB_FROM_C;
	if (cur->from_wanted & (DBA_DB_FROM_ADF))
		cur->from_wanted |= (DBA_DB_FROM_C | DBA_DB_FROM_D);

	/* Ignore anagraphical context unless explicitly requested */
	if (cur->from_wanted & DBA_DB_FROM_C && !cur->want_ana_context)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime>='1001-01-01 00:00:00'"));
		TRACE("ignoring anagraphical context as it has not been explicitly requested: adding AND c.datetime >= '1002-01-01 00:00:00'\n");
	}

	/* Create the FROM part with everything that is needed */
	if (cur->from_wanted & DBA_DB_FROM_C)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM context AS c "));
		add_other_froms(cur, DBA_DB_FROM_C);
	} else if (cur->from_wanted & DBA_DB_FROM_PA) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM pseudoana AS pa "));
		add_other_froms(cur, DBA_DB_FROM_PA);
	} else if (cur->from_wanted & DBA_DB_FROM_D) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM data AS d "));
		add_other_froms(cur, DBA_DB_FROM_D);
	} else if (cur->from_wanted & DBA_DB_FROM_RI) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM repinfo AS ri "));
		add_other_froms(cur, DBA_DB_FROM_RI);
	}

	/* Append the WHERE part that we prepared previously */
	if (dba_querybuf_size(cur->where) > 0)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "WHERE "));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, dba_querybuf_get(cur->where)));
	}

	/* Append GROUP BY and ORDER BY as needed */
	if (cur->modifiers & DBA_DB_MODIFIER_BEST)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
			" GROUP BY d.id_var, c.id_ana, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, c.datetime "
			"HAVING ri.prio=MAX(ri.prio) "
			"ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2"));
#define WANTS(mask) ((cur->from_wanted & (mask)) == (mask))
	else if (WANTS(DBA_DB_FROM_C | DBA_DB_FROM_RI))
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
			" ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, ri.prio"));
#undef WANTS
	else if (cur->from_wanted & DBA_DB_FROM_C)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
			" ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2"));
	else if (cur->from_wanted & DBA_DB_FROM_PA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " ORDER BY pa.id"));

	/* Append LIMIT if requested */
	if ((val = dba_record_key_peek_value(query, DBA_KEY_LIMIT)) != NULL)
		DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->query, " LIMIT %s", val));

	TRACE("Performing query: %s\n", dba_querybuf_get(cur->query));

	/* Perform the query */
	{
		int res = SQLExecDirect(cur->stm, (unsigned char*)dba_querybuf_get(cur->query), dba_querybuf_size(cur->query));
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, cur->stm, "performing DBALLE query \"%s\"", dba_querybuf_get(cur->query));
	}

	/* Get the number of affected rows */
	{
		SQLINTEGER rowcount;
		int res = SQLRowCount(cur->stm, &rowcount);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, cur->stm, "getting row count");
		cur->count = rowcount;
	}

	/* Retrieve results will happen in dba_db_cursor_next() */

	/* Done.  No need to deallocate the statement, it will be done by
	 * dba_db_cursor_delete */
	return dba_error_ok();
}

static dba_err dba_ana_add_extra(dba_db_cursor cur, dba_record rec)
{
	/* Extra variables to add:
	 *
	 * HEIGHT,      B07001  1793
	 * HEIGHT_BARO, B07031  1823
	 * ST_NAME,     B01019   275
	 * BLOCK,       B01001   257
	 * STATION,     B01002   258
	*/
	const char* query =
		"SELECT d.id_var, d.value"
		"  FROM context AS c, data AS d"
		" WHERE c.id = d.id_context AND c.id_ana = ?"
		"   AND c.datetime = '1000-01-01 00:00:00'"
		"   AND c.id_report = 254"
		"   AND c.ltype = 257 AND c.l1 = 0 AND c.l2 = 0"
		"   AND c.ptype = 0 AND c.p1 = 0 AND c.p2 = 0"
		"   AND d.id_var IN (257, 258, 275, 1793, 1823)";

	dba_err err = DBA_OK;
	SQLHSTMT stm;
	int res;
	dba_db db = cur->db;
	dba_varcode out_code;
	char out_val[256];
	SQLINTEGER out_val_ind;

	/* Allocate statement handle */
	DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

	/* Bind input fields */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->out_ana_id), 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, SQL_C_USHORT, &out_code, sizeof(out_code), 0);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_val, sizeof(out_val), &out_val_ind);

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE ANA extra query \"%s\"", query);
		goto cleanup;
	}

	/* Get the results and save them in the record */
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		switch (out_code)
		{
			case DBA_VAR(0, 1,  1): // BLOCK
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_BLOCK, out_val));
				break;
			case DBA_VAR(0, 1,  2): // STATION
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_STATION, out_val));
				break;
			default:
				DBA_RUN_OR_GOTO(cleanup, dba_record_var_setc(rec, out_code, out_val));
				break;
		}
	}

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_cursor_next(dba_db_cursor cur)
{
	/* Fetch new data */
	if (SQLFetch(cur->stm) == SQL_NO_DATA)
		return dba_error_notfound("retrieving a SQL query result (probably there are no more results to retrieve)");

	--cur->count;

	return dba_error_ok();
}

dba_err dba_db_cursor_to_record(dba_db_cursor cur, dba_record rec)
{
	/* Empty the record from old data */
	/* See if it works without: in theory if the caller does a record_clear
	 * before the query, all the values coming out of dba_db_cursor_next should
	 * just overwrite the previous ones, as the range of output parameters does
	 * not change */
	/* dba_record_clear(rec); */

	if (cur->from_wanted & DBA_DB_FROM_PA)
	{
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_ANA_ID, cur->out_ana_id));
		if (cur->wanted & DBA_DB_WANT_COORDS)
		{
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LAT, cur->out_lat));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LON, cur->out_lon));
		}
		if (cur->wanted & DBA_DB_WANT_IDENT)
		{
			if (cur->out_ident_ind != SQL_NULL_DATA && cur->out_ident[0] != 0)
			{
				DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_IDENT, cur->out_ident));
				DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
			} else {
				dba_record_key_unset(rec, DBA_KEY_IDENT);
				DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));
			}
		}
	}
	if (cur->from_wanted & DBA_DB_FROM_C)
	{
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_CONTEXT_ID, cur->out_context_id));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_REP_COD, cur->out_rep_cod));

		/* If PA was not wanted, we can still get the ana_id */
		if (!(cur->from_wanted & DBA_DB_FROM_PA))
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_ANA_ID, cur->out_ana_id));

		if (cur->wanted & DBA_DB_WANT_LEVEL)
		{
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE, cur->out_ltype));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L1, cur->out_l1));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L2, cur->out_l2));
		}

		if (cur->wanted & DBA_DB_WANT_TIMERANGE)
		{
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_PINDICATOR, cur->out_pind));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P1, cur->out_p1));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P2, cur->out_p2));
		}

		if (cur->wanted & DBA_DB_WANT_DATETIME)
		{
			/*fprintf(stderr, "SETTING %s to %d\n", #var,  _db_cursor[cur].out_##var); */
			int year, mon, day, hour, min, sec;
			if (sscanf(cur->out_datetime,
						"%04d-%02d-%02d %02d:%02d:%02d", &year, &mon, &day, &hour, &min, &sec) != 6)
				return dba_error_consistency("parsing datetime string \"%s\"", cur->out_datetime);

			DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_DATETIME, cur->out_datetime));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_YEAR, year));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MONTH, mon));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_DAY, day));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_HOUR, hour));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MIN, min));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_SEC, sec));
		}
	}
	if (cur->from_wanted & DBA_DB_FROM_D)
	{
		if (!(cur->from_wanted & DBA_DB_FROM_C))
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_CONTEXT_ID, cur->out_context_id));

		if (cur->wanted & DBA_DB_WANT_VAR_NAME)
		{
			char bname[7];
			snprintf(bname, 7, "B%02d%03d",
					DBA_VAR_X(cur->out_idvar),
					DBA_VAR_Y(cur->out_idvar));
			DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_VAR, bname));
		}

		if (cur->wanted & DBA_DB_WANT_VAR_VALUE)
			DBA_RUN_OR_RETURN(dba_record_var_setc(rec, cur->out_idvar, cur->out_value));
	}

	if (cur->from_wanted & (DBA_DB_FROM_RI | DBA_DB_FROM_C))
	{
		if (!(cur->from_wanted & DBA_DB_FROM_C))
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_REP_COD, cur->out_rep_cod));

		if (cur->wanted & DBA_DB_WANT_REPCOD)
		{
			dba_db_repinfo_cache c = dba_db_repinfo_get_by_id(cur->db->repinfo, cur->out_rep_cod);
			if (c != NULL)
			{
				DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_REP_MEMO, c->memo));
				DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_PRIORITY, c->prio));
			}
		}
	}

	if (cur->modifiers & DBA_DB_MODIFIER_ANAEXTRA)
		DBA_RUN_OR_RETURN(dba_ana_add_extra(cur, rec));

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
