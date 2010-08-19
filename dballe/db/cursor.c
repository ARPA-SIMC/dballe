/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <regex.h>

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
	{
		SQLEndTran(SQL_HANDLE_DBC, cur->db->od_conn, SQL_COMMIT);
		SQLFreeHandle(SQL_HANDLE_STMT, cur->stm);
	}
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

static dba_err parse_varcode(const char* str, regmatch_t pos, dba_varcode* code)
{
	dba_varcode res;
	/* Parse the varcode */
	if (str[pos.rm_so] == 'B')
		res = DBA_STRING_TO_VAR(str + pos.rm_so + 1);
	else
	    res = dba_varcode_alias_resolve_substring(str + pos.rm_so, pos.rm_eo - pos.rm_so);

	if (res == 0)
		return dba_error_consistency("cannot resolve the variable code or alias in \"%.*s\"", pos.rm_eo - pos.rm_so, str + pos.rm_so);

	*code = res;
	return dba_error_ok();
}

static dba_err parse_value(const char* str, regmatch_t pos, dba_varinfo info, char* value)
{
	/* Parse the value */
	const char* s = str + pos.rm_so;
	int len = pos.rm_eo - pos.rm_so;
	if (info->is_string)
	{
		/* Copy the string, escaping quotes */
		int i = 0, j = 0;

		value[j++] = '\'';
		for (; i < len && j < 253; ++i, ++j)
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
			return dba_error_consistency("value in \"%.*s\" must be a number", len, s);
		DBA_RUN_OR_RETURN(dba_var_created(info, dval, &tmpvar));
		strncpy(value, dba_var_value(tmpvar), 255);
		value[254] = 0;
		dba_var_delete(tmpvar);
	}
	return dba_error_ok();
}


static dba_err decode_data_filter(const char* filter, dba_varinfo* info, const char** op, const char** val, const char** val1)
{
	static regex_t* re_normal = NULL;
	static regex_t* re_between = NULL;
	regmatch_t matches[4];

	static char operator[5];
	static char value[255];
	static char value1[255];
#if 0
	size_t len = strcspn(filter, "<=>");
	const char* s = filter + len;
#endif
	dba_varcode code;
	int res;

	/* Compile the regular expression if it has not yet been done */
	if (re_normal == NULL)
	{
		if ((re_normal = (regex_t*)malloc(sizeof(regex_t))) == NULL)
			return dba_error_alloc("building the regular expression to match normal filters");
		if ((res = regcomp(re_normal, "^([^<=>]+)([<=>]+)([^<=>]+)$", REG_EXTENDED)) != 0)
			return dba_error_regexp(res, re_normal, "compiling regular expression to match normal filters");
	}
	if (re_between == NULL)
	{
		if ((re_between = (regex_t*)malloc(sizeof(regex_t))) == NULL)
			return dba_error_alloc("building the regular expression to match 'between' filters");
		if ((res = regcomp(re_between, "^([^<=>]+)<=([^<=>]+)<=([^<=>]+)$", REG_EXTENDED)) != 0)
			return dba_error_regexp(res, re_between, "compiling regular expression to match 'between' filters");
	}

	res = regexec(re_normal, filter, 4, matches, 0);
	if (res != 0 && res != REG_NOMATCH)
		return dba_error_regexp(res, re_normal, "Trying to parse '%s' as a 'normal' filter", filter);
	if (res == 0)
	{
		int len;
		/* We have a normal filter */

		/* Parse the varcode */
		DBA_RUN_OR_RETURN(parse_varcode(filter, matches[1], &code));
		/* Query informations for the varcode */
		DBA_RUN_OR_RETURN(dba_varinfo_query_local(code, info));
		
		/* Parse the operator */
		len = matches[2].rm_eo - matches[2].rm_so;
		if (len > 4)
			return dba_error_consistency("operator %.*s is not valid", len, filter + matches[2].rm_so);
		memcpy(operator, filter + matches[2].rm_so, len);
		operator[len] = 0;
		if (strcmp(operator, "!=") == 0)
			*op = "<>";
		else if (strcmp(operator, "==") == 0)
			*op = "=";
		else
			*op = operator;

		/* Parse the value */
		DBA_RUN_OR_RETURN(parse_value(filter, matches[3], *info, value));
		*val = value;
		*val1 = NULL;
	}
	else
	{
		res = regexec(re_between, filter, 4, matches, 0);
		if (res == REG_NOMATCH)
			return dba_error_consistency("%s is not a valid filter", filter);
		if (res != 0)
			return dba_error_regexp(res, re_normal, "Trying to parse '%s' as a 'between' filter", filter);
		if (res == 0)
		{
			/* We have a between filter */

			/* Parse the varcode */
			DBA_RUN_OR_RETURN(parse_varcode(filter, matches[2], &code));
			/* Query informations for the varcode */
			DBA_RUN_OR_RETURN(dba_varinfo_query_local(code, info));
			/* No need to parse the operator */
			operator[0] = 0;
			*op = operator;
			/* Parse the values */
			DBA_RUN_OR_RETURN(parse_value(filter, matches[1], *info, value));
			DBA_RUN_OR_RETURN(parse_value(filter, matches[3], *info, value1));
			*val = value;
			*val1 = value1;
		}
	}

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
					else if (strncmp(s, "nosort", 6) == 0)
						cur->modifiers |= DBA_DB_MODIFIER_UNSORTED;
					else if (strncmp(s, "stream", 6) == 0)
						cur->modifiers |= DBA_DB_MODIFIER_STREAM;
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
		cur->select_wanted |= DBA_DB_FROM_PA;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.lat"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_lat), sizeof(cur->out_lat), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.lon"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_lon), sizeof(cur->out_lon), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_IDENT)
	{
		cur->from_wanted |= DBA_DB_FROM_PA;
		cur->select_wanted |= DBA_DB_FROM_PA;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.ident"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_CHAR, &(cur->out_ident), sizeof(cur->out_ident), &(cur->out_ident_ind));
	}
	if (cur->wanted & DBA_DB_WANT_LEVEL)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		cur->select_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.ltype1"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_ltype1), sizeof(cur->out_ltype1), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.l1"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_l1), sizeof(cur->out_l1), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.ltype2"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_ltype2), sizeof(cur->out_ltype2), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.l2"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_l2), sizeof(cur->out_l2), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_TIMERANGE)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		cur->select_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.ptype"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_pind), sizeof(cur->out_pind), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.p1"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_p1), sizeof(cur->out_p1), NULL);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.p2"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_p2), sizeof(cur->out_p2), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_DATETIME)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		cur->select_wanted |= DBA_DB_FROM_C;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.datetime"));
		SQLBindCol(cur->stm, cur->output_seq++, SQL_C_TYPE_TIMESTAMP, &(cur->out_datetime), sizeof(cur->out_datetime), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_REPCOD)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
		cur->select_wanted |= DBA_DB_FROM_D;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id_report"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_rep_cod), sizeof(cur->out_rep_cod), NULL);
	}
	if (cur->wanted & DBA_DB_WANT_VAR_NAME || cur->wanted & DBA_DB_WANT_VAR_VALUE)
	{
		cur->from_wanted |= DBA_DB_FROM_D;
		cur->select_wanted |= DBA_DB_FROM_D;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "d.id_var"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_idvar), sizeof(cur->out_idvar), NULL);

		if (cur->wanted & DBA_DB_WANT_VAR_VALUE)
		{
			cur->from_wanted |= DBA_DB_FROM_D;
			cur->select_wanted |= DBA_DB_FROM_D;
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "d.value"));
			SQLBindCol(cur->stm, cur->output_seq++, SQL_C_CHAR, &(cur->out_value), sizeof(cur->out_value), NULL);
		}
	}

	/* If querybest is used, then we need ri.prio here so that GROUP BY can use it */
	if (cur->modifiers & DBA_DB_MODIFIER_BEST)
	{
		cur->from_wanted |= DBA_DB_FROM_RI;
		cur->select_wanted |= DBA_DB_FROM_RI;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "ri.prio"));
		SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_priority), sizeof(cur->out_context_id), NULL);
	}

	/* For these parameters we can try to be opportunistic and avoid extra joins */
	if (cur->wanted & DBA_DB_WANT_ANA_ID)
	{
		if (cur->select_wanted & DBA_DB_FROM_PA)
		{
			/* Try pa first */
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.id"));
			SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_ana_id), sizeof(cur->out_ana_id), NULL);
		} else if (cur->select_wanted & DBA_DB_FROM_C) {
			/* Then c */
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id_ana"));
			SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_ana_id), sizeof(cur->out_ana_id), NULL);
		} else {
			/* If we don't have anything to reuse, get it from pa */
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "pa.id"));
			SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_ana_id), sizeof(cur->out_ana_id), NULL);
			cur->from_wanted |= DBA_DB_FROM_PA;
			cur->select_wanted |= DBA_DB_FROM_PA;
		}
	}

	if (cur->wanted & DBA_DB_WANT_CONTEXT_ID)
	{
		if (cur->select_wanted & DBA_DB_FROM_C)
		{
			/* Try c first */
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id"));
			SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_context_id), sizeof(cur->out_context_id), NULL);
		} else if (cur->select_wanted & DBA_DB_FROM_D) {
			/* Then c */
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "d.id_context"));
			SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_context_id), sizeof(cur->out_context_id), NULL);
		} else {
			/* If we don't have anything to reuse, get it from c */
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query, "c.id"));
			SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, &(cur->out_context_id), sizeof(cur->out_context_id), NULL);
			cur->from_wanted |= DBA_DB_FROM_C;
		}
	}

	return dba_error_ok();
}

static dba_err add_int(
		dba_db_cursor cur,
		dba_record query,
		DBALLE_SQL_C_SINT_TYPE* out,
		dba_keyword key,
		const char* sql,
		int needed_from)
{
	const char* val;
	if ((val = dba_record_key_peek_value(query, key)) != NULL) {
		*out = strtol(val, 0, 10);
		//TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, sql));
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, out, 0, 0);
		cur->from_wanted |= needed_from;
	}
	return dba_error_ok();
}

// Normalise longitude values to the [-180..180[ interval
static inline int normalon(int lon)
{
	return ((lon + 18000000) % 36000000) - 18000000;
}

static dba_err add_repinfo_where(dba_db_cursor cur, dba_querybuf buf, dba_record query, const char* colname)
{
	const char* val;
#define ADD_INT(key, sql, needed) do { \
	if ((val = dba_record_key_peek_value(query, key)) != NULL) { \
		int ival = strtol(val, 0, 10); \
		/*TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);*/ \
		DBA_RUN_OR_RETURN(dba_querybuf_append_listf(buf, sql, colname, ival)); \
		cur->from_wanted |= needed; \
	} } while (0)
	
	ADD_INT(DBA_KEY_PRIORITY, "%s.prio=%d", DBA_DB_FROM_RI);
	ADD_INT(DBA_KEY_PRIOMIN, "%s.prio>=%d", DBA_DB_FROM_RI);
	ADD_INT(DBA_KEY_PRIOMAX, "%s.prio<=%d", DBA_DB_FROM_RI);
#undef ADD_INT
	return dba_error_ok();
}

/*
 * Create the WHERE part of the query
 */
static dba_err make_where(dba_db_cursor cur, dba_record query)
{
#define ADD_INT(field, key, sql, needed) DBA_RUN_OR_RETURN(add_int(cur, query, field, key, sql, needed))
	const char* val;

	DBA_RUN_OR_RETURN(dba_db_need_repinfo(cur->db));
	DBA_RUN_OR_RETURN(dba_querybuf_start_list(cur->where, " AND "));

//	fprintf(stderr, "A1 '%s'\n", dba_querybuf_get(cur->where));

	ADD_INT(&cur->sel_ana_id, DBA_KEY_ANA_ID, "pa.id=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_latmin, DBA_KEY_LAT, "pa.lat=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_latmin, DBA_KEY_LATMIN, "pa.lat>=?", DBA_DB_FROM_PA);
	ADD_INT(&cur->sel_latmax, DBA_KEY_LATMAX, "pa.lat<=?", DBA_DB_FROM_PA);
	//ADD_INT(&cur->sel_lonmin, DBA_KEY_LON, "pa.lon=?", DBA_DB_FROM_PA);
	if ((val = dba_record_key_peek_value(query, DBA_KEY_LON)) != NULL)
	{
		cur->sel_lonmin = normalon(strtol(val, 0, 10));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "pa.lon=?"));
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &cur->sel_lonmin, 0, 0);
		cur->from_wanted |= DBA_DB_FROM_PA;
	}
	if (dba_record_key_peek_value(query, DBA_KEY_LONMIN) != NULL &&
	    dba_record_key_peek_value(query, DBA_KEY_LONMAX) != NULL)
	{
		int lonmin, lonmax, ival;
		DBA_RUN_OR_RETURN(dba_record_key_enqi(query, DBA_KEY_LONMIN, &lonmin, &ival));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(query, DBA_KEY_LONMAX, &lonmax, &ival));
		cur->sel_lonmin = normalon(lonmin);
		cur->sel_lonmax = normalon(lonmax);
		if (lonmin < lonmax)
		{
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "pa.lon>=? AND pa.lon<=?"));
		} else {
			DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "((pa.lon>=? AND pa.lon<=18000000) OR (pa.lon>=-18000000 AND pa.lon<=?))"));
		}
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &cur->sel_lonmin, 0, 0);
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &cur->sel_lonmax, 0, 0);
		cur->from_wanted |= DBA_DB_FROM_PA;
	} else if (dba_record_key_peek_value(query, DBA_KEY_LONMIN) != NULL) {
		return dba_error_consistency("'lonmin' query parameter was specified without 'lonmax'");
	} else if (dba_record_key_peek_value(query, DBA_KEY_LONMAX) != NULL) {
		return dba_error_consistency("'lonmax' query parameter was specified without 'lonmin'");
	}

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
				cur->sel_dtmin.year = minvalues[0];
				cur->sel_dtmin.month = minvalues[1];
				cur->sel_dtmin.day = minvalues[2];
				cur->sel_dtmin.hour = minvalues[3];
				cur->sel_dtmin.minute = minvalues[4];
				cur->sel_dtmin.second = minvalues[5];
				cur->sel_dtmin.fraction = 0;
				DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime=?"));
				TRACE("found exact time: adding AND c.datetime=?. val is %04d-%02d-%02d %02d:%02d:%02d\n",
						minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
				if (cur->db->server_type == POSTGRES || cur->db->server_type == SQLITE)
					SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &(cur->sel_dtmin), 0, 0);
				else
					SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_DATETIME, 0, 0, &(cur->sel_dtmin), 0, 0);
				cur->from_wanted |= DBA_DB_FROM_C;
			}
			else
			{
				if (minvalues[0] != -1)
				{
					/* Add constraint on the minimum date interval */
					cur->sel_dtmin.year = minvalues[0];
					cur->sel_dtmin.month = minvalues[1];
					cur->sel_dtmin.day = minvalues[2];
					cur->sel_dtmin.hour = minvalues[3];
					cur->sel_dtmin.minute = minvalues[4];
					cur->sel_dtmin.second = minvalues[5];
					cur->sel_dtmin.fraction = 0;
					DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime>=?"));
					TRACE("found min time: adding AND c.datetime>=?. val is %04d-%02d-%02d %02d:%02d:%02d\n",
						minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
					if (cur->db->server_type == POSTGRES || cur->db->server_type == SQLITE)
						SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &cur->sel_dtmin, 0, 0);
					else
						SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_DATETIME, 0, 0, &cur->sel_dtmin, 0, 0);
					cur->from_wanted |= DBA_DB_FROM_C;
				}
				if (maxvalues[0] != -1)
				{
					cur->sel_dtmax.year = maxvalues[0];
					cur->sel_dtmax.month = maxvalues[1];
					cur->sel_dtmax.day = maxvalues[2];
					cur->sel_dtmax.hour = maxvalues[3];
					cur->sel_dtmax.minute = maxvalues[4];
					cur->sel_dtmax.second = maxvalues[5];
					cur->sel_dtmax.fraction = 0;
					DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime<=?"));
					TRACE("found max time: adding AND c.datetime<=?. val is %04d-%02d-%02d %02d:%02d:%02d\n",
						minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
					if (cur->db->server_type == POSTGRES || cur->db->server_type == SQLITE)
						SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &cur->sel_dtmax, 0, 0);
					else
						SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_DATETIME, 0, 0, &cur->sel_dtmax, 0, 0);
					cur->from_wanted |= DBA_DB_FROM_C;
				}
			}
		}

		if (dba_record_key_peek_value(query, DBA_KEY_CONTEXT_ID) != NULL ||
		    minvalues[0] == 1000 || maxvalues[0] == 1000)
			cur->accept_from_ana_context = 1;

		if (cur->modifiers & DBA_DB_MODIFIER_NOANAEXTRA)
			cur->accept_from_ana_context = 0;
	}

//	fprintf(stderr, "A3 '%s'\n", dba_querybuf_get(cur->where));

	ADD_INT(&cur->sel_ltype1, DBA_KEY_LEVELTYPE1, "c.ltype1=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_l1, DBA_KEY_L1, "c.l1=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_ltype2, DBA_KEY_LEVELTYPE2, "c.ltype2=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_l2, DBA_KEY_L2, "c.l2=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_pind, DBA_KEY_PINDICATOR, "c.ptype=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_p1, DBA_KEY_P1, "c.p1=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_p2, DBA_KEY_P2, "c.p2=?", DBA_DB_FROM_C);
	ADD_INT(&cur->sel_context_id, DBA_KEY_CONTEXT_ID, "c.id = ?", DBA_DB_FROM_C);

	/* rep_memo has priority over rep_cod */
	if ((val = dba_record_key_peek_value(query, DBA_KEY_REP_MEMO)) != NULL)
	{
		int src_val;
		DBA_RUN_OR_RETURN(dba_db_repinfo_get_id(cur->db->repinfo, val, &src_val));
		cur->sel_rep_cod = src_val;
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.id_report=?"));
		TRACE("found rep_memo %s: adding AND c.id_report = ?. val is %d\n", val, cur->sel_rep_cod);
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(cur->sel_rep_cod), 0, 0);
		cur->from_wanted |= DBA_DB_FROM_C;
	} else
		ADD_INT(&cur->sel_rep_cod, DBA_KEY_REP_COD, "c.id_report=?", DBA_DB_FROM_C);


	if ((val = dba_record_key_peek_value(query, DBA_KEY_VAR)) != NULL)
	{
		cur->sel_b = dba_descriptor_code(val);
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "d.id_var=?"));
		TRACE("found b: adding AND d.id_var = ?. val is %d %s\n", cur->sel_b, val);
		SQLBindParameter(cur->stm, cur->input_seq++, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &cur->sel_b, 0, 0);
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

	DBA_RUN_OR_RETURN(add_repinfo_where(cur, cur->where, query, "ri"));

	if ((val = dba_record_var_peek_value(query, DBA_VAR(0, 1, 1))) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "dblo.value="));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->where, val));
		cur->from_wanted |= DBA_DB_FROM_DBLO;
	}
	if ((val = dba_record_var_peek_value(query, DBA_VAR(0, 1, 2))) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "dsta.value="));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->where, val));
		cur->from_wanted |= DBA_DB_FROM_DSTA;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_ANA_FILTER)) != NULL)
	{
		dba_varinfo info;
		const char *op, *value, *value1;
		DBA_RUN_OR_RETURN(decode_data_filter(val, &info, &op, &value, &value1));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "dana.id_var="));
		if (value1 == NULL)
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND dana.value%s%s",
					info->var, op, value));
		else
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND dana.value BETWEEN %s AND %s",
					info->var, value, value1));
		cur->from_wanted |= DBA_DB_FROM_DANA;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_DATA_FILTER)) != NULL)
	{
		dba_varinfo info;
		const char *op, *value, *value1;
		DBA_RUN_OR_RETURN(decode_data_filter(val, &info, &op, &value, &value1));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "ddf.id_var="));
		if (value1 == NULL)
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND ddf.value%s%s",
					info->var, op, value));
		else
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND ddf.value BETWEEN %s AND %s",
					info->var, value, value1));
		cur->from_wanted |= DBA_DB_FROM_DDF;
	}

	if ((val = dba_record_key_peek_value(query, DBA_KEY_ATTR_FILTER)) != NULL)
	{
		dba_varinfo info;
		const char *op, *value, *value1;
		DBA_RUN_OR_RETURN(decode_data_filter(val, &info, &op, &value, &value1));
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "adf.type="));
		if (value1 == NULL)
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND adf.value%s%s",
					info->var, op, value));
		else
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->where, "%d AND adf.value BETWEEN %s AND %s",
					info->var, op, value, value1));
		cur->from_wanted |= DBA_DB_FROM_ADF;
	}

//	fprintf(stderr, "A15 '%s'\n", dba_querybuf_get(cur->where));

	return dba_error_ok();
#undef ADD_INT
}

static dba_err resolve_dependencies(dba_db_cursor cur)
{
	if (cur->wanted & DBA_DB_WANT_COORDS)
	{
		cur->from_wanted |= DBA_DB_FROM_PA;
	}
	if (cur->wanted & DBA_DB_WANT_IDENT)
	{
		cur->from_wanted |= DBA_DB_FROM_PA;
	}
	if (cur->wanted & DBA_DB_WANT_LEVEL)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
	}
	if (cur->wanted & DBA_DB_WANT_TIMERANGE)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
	}
	if (cur->wanted & DBA_DB_WANT_DATETIME)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
	}
	if (cur->wanted & DBA_DB_WANT_REPCOD)
	{
		cur->from_wanted |= DBA_DB_FROM_C;
	}
	if (cur->wanted & DBA_DB_WANT_VAR_NAME || cur->wanted & DBA_DB_WANT_VAR_VALUE)
	{
		cur->from_wanted |= DBA_DB_FROM_D;
	}

	/* If querybest is used, then we need ri.prio here so that GROUP BY can use it */
	if (cur->modifiers & DBA_DB_MODIFIER_BEST)
	{
		cur->from_wanted |= DBA_DB_FROM_RI;
	}

	/* For these parameters we can try to be opportunistic and avoid extra joins */
	if (cur->wanted & DBA_DB_WANT_ANA_ID)
	{
		if (!(cur->from_wanted & DBA_DB_FROM_PA) && cur->from_wanted & DBA_DB_FROM_C) {
		} else {
			cur->from_wanted |= DBA_DB_FROM_PA;
		}
	}

	if (cur->wanted & DBA_DB_WANT_CONTEXT_ID)
	{
		if (!(cur->from_wanted & DBA_DB_FROM_C) && cur->from_wanted & DBA_DB_FROM_D) {
		} else {
			cur->from_wanted |= DBA_DB_FROM_C;
		}
	}

	/* Enforce join dependencies */
	if (cur->from_wanted & (DBA_DB_FROM_DBLO | DBA_DB_FROM_DSTA | DBA_DB_FROM_DANA))
		cur->from_wanted |= DBA_DB_FROM_CBS;
	if (cur->from_wanted & (DBA_DB_FROM_DDF))
		cur->from_wanted |= DBA_DB_FROM_C;
	if (cur->from_wanted & (DBA_DB_FROM_ADF))
		cur->from_wanted |= (DBA_DB_FROM_C | DBA_DB_FROM_D);
	if (cur->from_wanted & DBA_DB_FROM_PA && cur->from_wanted & DBA_DB_FROM_D)
		cur->from_wanted |= DBA_DB_FROM_C;
	if (cur->from_wanted & (DBA_DB_FROM_CBS))
		cur->from_wanted |= DBA_DB_FROM_C;

	/* Always join with context if we need to weed out the extra ana data */
	if (cur->modifiers & DBA_DB_MODIFIER_NOANAEXTRA)
		cur->from_wanted |= DBA_DB_FROM_C;

	return dba_error_ok();
}

static dba_err add_other_froms(dba_db_cursor cur, unsigned int base)
{
	/* Remove the base table from the things to add */
	unsigned int wanted = cur->from_wanted & ~base;

	if (wanted & DBA_DB_FROM_PA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN pseudoana pa ON c.id_ana = pa.id "));

	if (wanted & DBA_DB_FROM_C)
		switch (base)
		{
			case DBA_DB_FROM_PA:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN context c ON c.id_ana=pa.id "));
				break;
			case DBA_DB_FROM_D:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN context c ON c.id=d.id_context "));
				break;
			case DBA_DB_FROM_RI:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN context c ON c.id_report=ri.id "));
				break;
			default:
				return dba_error_consistency("requested to add a JOIN on context on the unsupported base %d", base);
		}

	if (wanted & DBA_DB_FROM_CBS)
		switch (base)
		{
			case DBA_DB_FROM_PA:
				/*
				 * If we are here, it means that no rep_cod or rep_memo has
				 * been specified, and either height or ana_filter have been
				 * asked.  This means that we cannot know what network should
				 * be used.
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
							" JOIN context cbs ON pa.id=cbs.id_ana"
							" AND cbs.id_report=254"
							" AND cbs.datetime={ts '1000-01-01 00:00:00.0'}"
							" AND cbs.ltype=257 AND cbs.l1=0 AND cbs.l2=0"
							" AND cbs.ptype=0 AND cbs.p1=0 AND cbs.p2=0 "));
				break;
				*/
				return dba_error_consistency("please specify rep_cod or rep_memo among the query parameters, otherwise the query is ambiguous in this case");
			case DBA_DB_FROM_C:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
							" JOIN context cbs ON c.id_ana=cbs.id_ana"
							" AND cbs.id_report=c.id_report"
							" AND cbs.datetime={ts '1000-01-01 00:00:00.0'}"
							" AND cbs.ltype1=257 AND cbs.l1=0 AND cbs.ltype2=0 AND cbs.l2=0"
							" AND cbs.ptype=0 AND cbs.p1=0 AND cbs.p2=0 "));
				break;
			default:
				return dba_error_consistency("requested to add a JOIN on anagraphical context on the unsupported base %d", base);
		}

	if (wanted & DBA_DB_FROM_D)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN data d ON d.id_context=c.id "));

	if (wanted & DBA_DB_FROM_RI)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "JOIN repinfo ri ON ri.id=c.id_report "));

	if (wanted & DBA_DB_FROM_DBLO)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data dblo ON dblo.id_context=cbs.id AND dblo.id_var=257 "));
	if (wanted & DBA_DB_FROM_DSTA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data dsta ON dsta.id_context=cbs.id AND dsta.id_var=258 "));

	if (wanted & DBA_DB_FROM_DANA)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data dana ON dana.id_context=cbs.id "));

	if (wanted & DBA_DB_FROM_DDF)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN data ddf ON ddf.id_context=c.id "));

	if (wanted & DBA_DB_FROM_ADF)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					"JOIN attr adf ON adf.id_context=c.id AND adf.id_var=d.id_var "));

	return dba_error_ok();
}

#if 0
static dba_err rowcount(dba_db db, const char* table, DBALLE_SQL_C_SINT_TYPE* count)
{
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	char buf[100];
	int len, res;

    /* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	/* Bind count directly in the output  */
	SQLBindCol(stm, 1, DBALLE_SQL_C_SINT, count, sizeof(*count), NULL);

	len = snprintf(buf, 100, "SELECT COUNT(*) FROM %s", table);
	res = SQLExecDirect(stm, (unsigned char*)buf, len);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
				"Counting the elements of table %s", table);
		goto cleanup;
	}

	/* Get the result */
	if (SQLFetch(stm) == SQL_NO_DATA)
	{
		err = dba_error_consistency("no results from database when querying row count of table %s", table);
		goto cleanup;
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err setstmtattr(SQLHSTMT stm, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len, const char* context)
{
	int res = SQLSetStmtAttr(stm, attr, val, len);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, context);
	return dba_error_ok();
}
#endif

/* FIXME: this is a temporary solution giving an approximate row count only:
 * insert/delete/update queries run between the count and the select will
 * change the size of the result set */
static dba_err getcount(dba_db_cursor cur, dba_record query, unsigned int wanted, unsigned int modifiers, DBALLE_SQL_C_SINT_TYPE* count)
{
	/* Reset the cursor to start a new query */
	dba_querybuf_reset(cur->query);
	dba_querybuf_reset(cur->where);
	cur->wanted = wanted;
	cur->select_wanted = 0;
	cur->from_wanted = 0;
	cur->input_seq = 1;
	cur->output_seq = 1;
	cur->accept_from_ana_context = 0;

	/* Scan query modifiers */
	cur->modifiers = modifiers;
	DBA_RUN_OR_RETURN(init_modifiers(cur, query));

	DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "SELECT "));

#if 0
	if (cur->modifiers & DBA_DB_MODIFIER_DISTINCT)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "COUNT(DISTINCT *) "));
	else
#endif
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "COUNT(*) "));
	SQLBindCol(cur->stm, cur->output_seq++, DBALLE_SQL_C_SINT, count, sizeof(*count), NULL);

	/* Prepare WHERE part and see what needs to be available in the FROM part */
	DBA_RUN_OR_RETURN(make_where(cur, query));

	/* Solve dependencies among the various parts of the query */
	DBA_RUN_OR_RETURN(resolve_dependencies(cur));

	/* Ignore anagraphical context unless explicitly requested */
	if (cur->from_wanted & DBA_DB_FROM_C && !cur->accept_from_ana_context)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime>={ts '1001-01-01 00:00:00.0'}"));
		TRACE("ignoring anagraphical context as it has not been explicitly requested: adding AND c.datetime >= {ts '1001-01-01 00:00:00.0'}\n");
	}

	/* Create the FROM part with everything that is needed */
	if (cur->from_wanted & DBA_DB_FROM_C)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM context c "));
		add_other_froms(cur, DBA_DB_FROM_C);
	} else if (cur->from_wanted & DBA_DB_FROM_PA) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM pseudoana pa "));
		add_other_froms(cur, DBA_DB_FROM_PA);
	} else if (cur->from_wanted & DBA_DB_FROM_D) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM data d "));
		add_other_froms(cur, DBA_DB_FROM_D);
	} else if (cur->from_wanted & DBA_DB_FROM_RI) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM repinfo ri "));
		add_other_froms(cur, DBA_DB_FROM_RI);
	}

	/* Append the WHERE part that we prepared previously */
	if (dba_querybuf_size(cur->where) > 0)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "WHERE "));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, dba_querybuf_get(cur->where)));
	}

	if (cur->modifiers & DBA_DB_MODIFIER_BEST)
		{
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					" AND ri.prio=(SELECT MAX(sri.prio) FROM repinfo sri JOIN context sc ON sri.id=sc.id_report JOIN data sd ON sc.id=sd.id_context WHERE "));
				DBA_RUN_OR_RETURN(dba_querybuf_start_list(cur->query, " AND "));
				DBA_RUN_OR_RETURN(add_repinfo_where(cur, cur->query, query, "sri"));
				DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query,
					"sc.id_ana=c.id_ana AND sc.ltype1=c.ltype1 AND sc.l1=c.l1 AND sc.ltype2=c.ltype2 AND sc.l2=c.l2 AND sc.ptype=c.ptype AND sc.p1=c.p1 AND sc.p2=c.p2 AND sc.datetime=c.datetime AND sd.id_var=d.id_var) "));
		}

	TRACE("Performing query: %s\n", dba_querybuf_get(cur->query));
	/* fprintf(stderr, "Performing query: %s\n", dba_querybuf_get(cur->query)); */

	/* Perform the query */
	{
		int res = SQLExecDirect(cur->stm, (unsigned char*)dba_querybuf_get(cur->query), dba_querybuf_size(cur->query));
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, cur->stm, "performing DBALLE query \"%s\"", dba_querybuf_get(cur->query));
	}

	if (SQLFetch(cur->stm) == SQL_NO_DATA)
		return dba_error_consistency("no results when trying to get the row count");

	{
		int res = SQLCloseCursor(cur->stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, cur->stm, "closing cursor after getting approximate row count");
	}

	return dba_error_ok();
}

static dba_err add_to_orderby(dba_querybuf query, const char* fields, int* first)
{
	if (*first) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(query, " ORDER BY "));
		*first = 0;
	} else
		DBA_RUN_OR_RETURN(dba_querybuf_append(query, ", "));
	return dba_querybuf_append(query, fields);
}

dba_err dba_db_cursor_query(dba_db_cursor cur, dba_record query, unsigned int wanted, unsigned int modifiers)
{
	const char* val;
	int limit = -1;

	/* Scan query modifiers */
	cur->modifiers = modifiers;
	DBA_RUN_OR_RETURN(init_modifiers(cur, query));

	if (cur->db->server_type == ORACLE && !(cur->modifiers & DBA_DB_MODIFIER_STREAM))
	{
		DBALLE_SQL_C_SINT_TYPE count;
		DBA_RUN_OR_RETURN(getcount(cur, query, wanted, modifiers, &count));
		cur->count = count;
	}

	/* Reset the cursor to start a new query */
	dba_querybuf_reset(cur->query);
	dba_querybuf_reset(cur->where);
	cur->wanted = wanted;
	cur->select_wanted = 0;
	cur->from_wanted = 0;
	cur->input_seq = 1;
	cur->output_seq = 1;
	cur->accept_from_ana_context = 0;

	if ((val = dba_record_key_peek_value(query, DBA_KEY_LIMIT)) != NULL)
		limit = strtoul(val, NULL, 10);

	DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "SELECT "));
	if (cur->modifiers & DBA_DB_MODIFIER_DISTINCT)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "DISTINCT "));
	if (cur->modifiers & DBA_DB_MODIFIER_BIGANA && cur->db->server_type == MYSQL)
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "straight_join "));

	/* Prepare WHERE part and see what needs to be available in the FROM part */
	DBA_RUN_OR_RETURN(make_where(cur, query));

	/* Prepare SELECT Part and see what needs to be available in the FROM part.
	 * We do this after creating the WHERE part, so that we can add
	 * more opportunistic extra values (see the end of make_select) */
	DBA_RUN_OR_RETURN(make_select(cur));

	/* Solve dependencies among the various parts of the query */
	DBA_RUN_OR_RETURN(resolve_dependencies(cur));

	/* Ignore anagraphical context unless explicitly requested */
	if (cur->from_wanted & DBA_DB_FROM_C && !cur->accept_from_ana_context)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->where, "c.datetime>={ts '1001-01-01 00:00:00.0'}"));
		TRACE("ignoring anagraphical context as it has not been explicitly requested: adding AND c.datetime >= {ts '1001-01-01 00:00:00.0'}\n");
	}

	/* Create the FROM part with everything that is needed */
	if (cur->from_wanted & DBA_DB_FROM_C)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM context c "));
		add_other_froms(cur, DBA_DB_FROM_C);
	} else if (cur->from_wanted & DBA_DB_FROM_PA) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM pseudoana pa "));
		add_other_froms(cur, DBA_DB_FROM_PA);
	} else if (cur->from_wanted & DBA_DB_FROM_D) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM data d "));
		add_other_froms(cur, DBA_DB_FROM_D);
	} else if (cur->from_wanted & DBA_DB_FROM_RI) {
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, " FROM repinfo ri "));
		add_other_froms(cur, DBA_DB_FROM_RI);
	}

	/* Append the WHERE part that we prepared previously */
	if (dba_querybuf_size(cur->where) > 0)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, "WHERE "));
		DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query, dba_querybuf_get(cur->where)));
	}

	if (cur->modifiers & DBA_DB_MODIFIER_BEST)
		switch (cur->db->server_type)
		{
			case ORACLE:
				if (limit != -1)
					return dba_error_unimplemented("best-value queries with result limit are not implemented for Oracle");
				/* Continue to the query */
			default:
				DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
					" AND ri.prio=(SELECT MAX(sri.prio) FROM repinfo sri JOIN context sc ON sri.id=sc.id_report JOIN data sd ON sc.id=sd.id_context WHERE "));
				DBA_RUN_OR_RETURN(dba_querybuf_start_list(cur->query, " AND "));
				DBA_RUN_OR_RETURN(add_repinfo_where(cur, cur->query, query, "sri"));
				DBA_RUN_OR_RETURN(dba_querybuf_append_list(cur->query,
					"sc.id_ana=c.id_ana AND sc.ltype1=c.ltype1 AND sc.l1=c.l1 AND sc.ltype2=c.ltype2 AND sc.l2=c.l2 AND sc.ptype=c.ptype AND sc.p1=c.p1 AND sc.p2=c.p2 AND sc.datetime=c.datetime AND sd.id_var=d.id_var) "));
				break;
		}

	/* Append ORDER BY as needed */
	if (!(cur->modifiers & DBA_DB_MODIFIER_UNSORTED))
	{
		int first = 1;
		if (limit != -1 && cur->db->server_type == ORACLE)
			return dba_error_unimplemented("sorted queries with result limit are not implemented for Oracle");

		if (cur->modifiers & DBA_DB_MODIFIER_BEST) {
			DBA_RUN_OR_RETURN(dba_querybuf_append(cur->query,
				"ORDER BY c.id_ana, c.datetime, c.ltype1, c.l1, c.ltype2, c.l2, c.ptype, c.p1, c.p2"));
		} else if (cur->select_wanted & DBA_DB_FROM_C) {
			if (cur->wanted & DBA_DB_WANT_ANA_ID)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "c.id_ana", &first));
			if (cur->modifiers & DBA_DB_MODIFIER_SORT_FOR_EXPORT)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "c.id_report", &first));
			if (cur->wanted & DBA_DB_WANT_DATETIME)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "c.datetime", &first));
			if (cur->wanted & DBA_DB_WANT_LEVEL)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "c.ltype1, c.l1, c.ltype2, c.l2", &first));
			if (cur->wanted & DBA_DB_WANT_TIMERANGE)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "c.ptype, c.p1, c.p2", &first));
			if (!(cur->modifiers & DBA_DB_MODIFIER_SORT_FOR_EXPORT) && (cur->wanted & DBA_DB_WANT_REPCOD))
			{
				if (cur->select_wanted & DBA_DB_FROM_RI)
					DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "ri.prio", &first));
				else 
					DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "c.id_report", &first));
			}
		} else if (cur->select_wanted & DBA_DB_FROM_PA) {
			if (cur->wanted & DBA_DB_WANT_ANA_ID)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "pa.id", &first));
			if (cur->wanted & DBA_DB_WANT_IDENT)
				DBA_RUN_OR_RETURN(add_to_orderby(cur->query, "pa.ident", &first));
		}
	}

	/* Append LIMIT if requested */
	if (limit != -1)
	{
		if (cur->db->server_type == ORACLE)
		{
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->query, " AND rownum <= %d", limit));
		} else {
			DBA_RUN_OR_RETURN(dba_querybuf_appendf(cur->query, " LIMIT %d", limit));
		}
	}

	TRACE("Performing query: %s\n", dba_querybuf_get(cur->query));

	if (cur->modifiers & DBA_DB_MODIFIER_STREAM && cur->db->server_type != ORACLE)
	{
		int res = SQLSetStmtAttr(cur->stm, SQL_ATTR_CURSOR_TYPE, 
				(SQLPOINTER)SQL_CURSOR_FORWARD_ONLY, SQL_IS_INTEGER);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, cur->stm, "setting SQL_CURSOR_FORWARD_ONLY on DBALLE query");
	}

#if 0
	//DBA_RUN_OR_RETURN(setstmtattr(cur->stm, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_STATIC, SQL_IS_INTEGER, "Setting SQL_CURSOR_STATIC"));
	DBA_RUN_OR_RETURN(setstmtattr(cur->stm, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER)SQL_SCROLLABLE, SQL_IS_INTEGER, "Setting SQL_SCROLLABLE"));
	DBA_RUN_OR_RETURN(setstmtattr(cur->stm, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, SQL_IS_INTEGER, "Setting SQL_CURSOR_DYNAMIC"));

#endif
	//fprintf(stderr, "********************** 0 ************\n");
	//fprintf(stderr, "** Q %s\n", dba_querybuf_get(cur->query));

	/* Perform the query */
	{
		int res = SQLExecDirect(cur->stm, (unsigned char*)dba_querybuf_get(cur->query), dba_querybuf_size(cur->query));
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, cur->stm, "performing DBALLE query \"%s\"", dba_querybuf_get(cur->query));
	}

	if (cur->db->server_type != ORACLE)
	{
		/* Get the number of affected rows */
		SQLLEN rowcount;
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
	dba_err err = DBA_OK;
	const char* query;
	SQLHSTMT stm;
	int res;
	dba_db db = cur->db;
	dba_varcode out_code;
	char out_val[256];
	SQLLEN out_val_ind;
	DBALLE_SQL_C_SINT_TYPE out_rep_cod;

#define BASE_QUERY \
		"SELECT d.id_var, d.value, ri.id, ri.prio" \
		"  FROM context c, data d, repinfo ri" \
		" WHERE c.id = d.id_context AND ri.id = c.id_report AND c.id_ana = ?" \
		"   AND c.datetime = {ts '1000-01-01 00:00:00.0'}" \
		"   AND c.ltype1 = 257 AND c.l1 = 0 AND c.ltype2 = 0 AND c.l2 = 0" \
		"   AND c.ptype = 0 AND c.p1 = 0 AND c.p2 = 0"

	switch (cur->db->server_type)
	{
		case MYSQL:
			query = BASE_QUERY
				" GROUP BY d.id_var,ri.id "
				"HAVING ri.prio=MAX(ri.prio)";
			break;
		default:
			query = BASE_QUERY
				" AND ri.prio=("
				"  SELECT MAX(sri.prio) FROM repinfo sri"
				"    JOIN context sc ON sri.id=sc.id_report"
				"    JOIN data sd ON sc.id=sd.id_context"
				"  WHERE sc.id_ana=c.id_ana"
				"    AND sc.ltype1=c.ltype1 AND sc.l1=c.l1 AND sc.ltype2=c.ltype2 AND sc.l2=c.l2"
				"    AND sc.ptype=c.ptype AND sc.p1=c.p1 AND sc.p2=c.p2"
				"    AND sc.datetime=c.datetime AND sd.id_var=d.id_var)";
			break;
	}
#undef BASE_QUERY

	/* Allocate statement handle */
	DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

	/* Bind input fields */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(cur->out_ana_id), 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, SQL_C_USHORT, &out_code, sizeof(out_code), 0);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_val, sizeof(out_val), &out_val_ind);
	SQLBindCol(stm, 3, DBALLE_SQL_C_SINT, &out_rep_cod, sizeof(out_rep_cod), NULL);

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
		DBA_RUN_OR_GOTO(cleanup, dba_record_var_setc(rec, out_code, out_val));
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_REP_COD, out_rep_cod));
	}

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_cursor_next(dba_db_cursor cur, int* has_data)
{
	/* Fetch new data */
	*has_data = (SQLFetch(cur->stm) != SQL_NO_DATA);

	if (cur->count != -1)
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
	DBA_RUN_OR_RETURN(dba_db_need_repinfo(cur->db));

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
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE1, cur->out_ltype1));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L1, cur->out_l1));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE2, cur->out_ltype2));
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
			/*
			int year, mon, day, hour, min, sec;
			if (sscanf(cur->out_datetime,
						"%04d-%02d-%02d %02d:%02d:%02d", &year, &mon, &day, &hour, &min, &sec) != 6)
				return dba_error_consistency("parsing datetime string \"%s\"", cur->out_datetime);
			*/
			//DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_DATETIME, cur->out_datetime));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_YEAR, cur->out_datetime.year));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MONTH, cur->out_datetime.month));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_DAY, cur->out_datetime.day));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_HOUR, cur->out_datetime.hour));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MIN, cur->out_datetime.minute));
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_SEC, cur->out_datetime.second));
		}
	}
	if (cur->from_wanted & DBA_DB_FROM_D)
	{
		if (!(cur->from_wanted & DBA_DB_FROM_C))
			DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_CONTEXT_ID, cur->out_context_id));

		if (cur->wanted & DBA_DB_WANT_VAR_NAME || cur->wanted & DBA_DB_WANT_VAR_VALUE)
		{
			char bname[7];
			snprintf(bname, 7, "B%02ld%03ld",
					DBA_VAR_X(cur->out_idvar),
					DBA_VAR_Y(cur->out_idvar));
			DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_VAR, bname));

			if (cur->wanted & DBA_DB_WANT_VAR_VALUE)
				DBA_RUN_OR_RETURN(dba_record_var_setc(rec, cur->out_idvar, cur->out_value));
		}
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
