/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>		/* malloc, strtod, getenv */
#include <string.h>		/* strncmp */
#include <ctype.h>		/* isspace */
#include <math.h>		/* rint */
#include <assert.h>		/* assert */

#include "var.h"
#include "fast.h"
#include "conv.h"

struct _dba_var_attr
{
	dba_var var;
	struct _dba_var_attr* next;
};
typedef struct _dba_var_attr* dba_var_attr;

struct _dba_var
{
	dba_varinfo info;
	char* value;
	dba_var_attr attrs;
};

static const double scales[] = {
	1.0,
	10.0,
	100.0,
	1000.0,
	10000.0,
	100000.0,
	1000000.0,
	10000000.0,
	100000000.0,
	1000000000.0,
	10000000000.0,
	100000000000.0,
	1000000000000.0,
	10000000000000.0,
	100000000000000.0,
	1000000000000000.0,
	10000000000000000.0,
};

/* Decode a double value from its integer representation and varinfo encoding
 * informations */
double dba_var_decode_int(int val, dba_varinfo info)
{
	if (info->scale > 0)
		return (val - info->ref) / scales[info->scale];
	else if (info->scale < 0)
		return (val - info->ref) * scales[-info->scale];
	else
		return val - info->ref;
}

/* Encode a double value from its integer representation and varinfo encoding
 * informations */
int dba_var_encode_int(double fval, dba_varinfo info)
{
	if (info->scale > 0)
		return (int)rint((fval + info->ref) * scales[info->scale]);
	else if (info->scale < 0)
		return (int)rint((fval + info->ref) / scales[-info->scale]);
	else
		return (int)rint(fval + info->ref);
}

dba_err dba_var_create(dba_varinfo info, dba_var* var)
{
	if ((*var = (dba_var)malloc(sizeof(struct _dba_var))) == NULL)
		return dba_error_alloc("creating new DBA variable");
	(*var)->info = info;
	(*var)->value = NULL;
	(*var)->attrs = NULL;
	return dba_error_ok();
}

dba_err dba_var_createi(dba_varinfo info, int val, dba_var* var)
{
	dba_err err;
	DBA_RUN_OR_RETURN(dba_var_create(info, var));
	DBA_RUN_OR_GOTO(fail, dba_var_seti(*var, val));
	return dba_error_ok();
fail:
	dba_var_delete(*var);
	*var = NULL;
	return err;
}

dba_err dba_var_created(dba_varinfo info, double val, dba_var* var)
{
	dba_err err;
	DBA_RUN_OR_RETURN(dba_var_create(info, var));
	DBA_RUN_OR_GOTO(fail, dba_var_setd(*var, val));
	return dba_error_ok();
fail:
	dba_var_delete(*var);
	*var = NULL;
	return err;
}

dba_err dba_var_createc(dba_varinfo info, const char* val, dba_var* var)
{
	dba_err err;
	DBA_RUN_OR_RETURN(dba_var_create(info, var));
	DBA_RUN_OR_GOTO(fail, dba_var_setc(*var, val));
	return dba_error_ok();
fail:
	dba_var_delete(*var);
	*var = NULL;
	return err;
}

dba_err dba_var_create_local(dba_varcode code, dba_var* var)
{
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_varinfo_query_local(code, &info));
	return dba_var_create(info, var);
}

dba_err dba_var_copy(dba_var source, dba_var* dest)
{
	dba_err err;
	dba_varinfo newinfo = NULL;

	*dest = NULL;
	/* Make a copy in case source uses a singleuse varinfo */
	DBA_RUN_OR_GOTO(fail, dba_varinfo_copy(source->info, &newinfo));
	DBA_RUN_OR_GOTO(fail, dba_var_create(newinfo, dest));

	/* Copy the value */
	if (source->value != NULL)
		DBA_RUN_OR_GOTO(fail, dba_var_setc(*dest, source->value));
	else
		(*dest)->value = NULL;

	/* Copy the attributes */
	DBA_RUN_OR_GOTO(fail, dba_var_copy_attrs(*dest, source));

	return dba_error_ok();

fail:
	if (*dest != NULL)
	{
		dba_var_delete(*dest);
		*dest = NULL;
	}
	if (newinfo != NULL)
		dba_varinfo_delete(newinfo);
	return err;
}

void dba_var_clear_attrs(dba_var var)
{
	while (var->attrs != NULL)
	{
		dba_var_attr next = var->attrs->next;
		dba_var_delete(var->attrs->var);
		free(var->attrs);
		var->attrs = next;
	}
}

void dba_var_delete(dba_var var)
{
	if (var->value != NULL)
		free(var->value);
	if (VARINFO_IS_SINGLEUSE(var->info))
		dba_varinfo_delete(var->info);
	dba_var_clear_attrs(var);
	free(var);
}

dba_varcode dba_var_code(dba_var var)
{
	return var->info->var;
}

dba_varinfo dba_var_info(dba_var var)
{
	return var->info;
}

const char* dba_var_value(dba_var var)
{
	return var->value;
}

dba_var_attr_iterator dba_var_attr_iterate(dba_var var)
{
	return var->attrs;
}

dba_var_attr_iterator dba_var_attr_iterator_next(dba_var_attr_iterator iter)
{
	if (iter == NULL)
		return NULL;
	return iter->next;
}

dba_var dba_var_attr_iterator_attr(dba_var_attr_iterator iter)
{
	if (iter == NULL)
		return NULL;
	else
		return iter->var;
}

dba_err dba_var_enqa(dba_var var, dba_varcode code, dba_var* attr)
{
	dba_var_attr cur;
	for (cur = var->attrs; cur != NULL; cur = cur->next)
		if (dba_var_code(cur->var) == code)
		{
			*attr = cur->var;
			return dba_error_ok();
		}
	*attr = NULL;
	return dba_error_ok();
}

dba_err dba_var_seta(dba_var var, dba_var attr)
{
	dba_err err = DBA_OK;
	dba_var copy = NULL;

	DBA_RUN_OR_RETURN(dba_var_copy(attr, &copy));
	DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, copy));

	return dba_error_ok();

fail:
	if (copy != NULL)
		dba_var_delete(copy);
	return err;
}

dba_err dba_var_seta_nocopy(dba_var var, dba_var attr)
{
	dba_var_attr cur;
	for (cur = var->attrs; cur != NULL; cur = cur->next)
		if (dba_var_code(cur->var) == dba_var_code(attr))
		{
			/* If found, replace the old value with this one */
			dba_var_delete(cur->var);
			cur->var = attr;
			return dba_error_ok();
		}

	/* Else, create it fresh and prepend it to the list */
	if ((cur = (dba_var_attr)malloc(sizeof(struct _dba_var_attr))) == NULL)
		return dba_error_alloc("creating new dba variable attribute");
	cur->var = attr;
	cur->next = var->attrs;
	var->attrs = cur;
	
	return dba_error_ok();
}

dba_err dba_var_unseta(dba_var var, dba_varcode code)
{
	dba_var_attr cur;

	/* Handle no attrs */
	if (var->attrs == NULL)
		return dba_error_ok();

	/* Handle the head */
	if (dba_var_code(var->attrs->var) == code)
	{
		dba_var_attr next = var->attrs->next;
		dba_var_delete(var->attrs->var);
		free(var->attrs);
		var->attrs = next;
		return dba_error_ok();
	}

	/* Handle the tail */
	for (cur = var->attrs; cur->next != NULL; cur = cur->next)
		if (dba_var_code(cur->next->var) == code)
		{
			dba_var_attr next = cur->next->next;
			dba_var_delete(cur->next->var);
			free(cur->next);
			cur->next = next;
			return dba_error_ok();
		}
	
	return dba_error_ok();
}

dba_err dba_var_enqi(dba_var var, int* val)
{
	if (var->value == NULL)
		return dba_error_notfound("B%02d%03d (%s) is not defined",
					DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);

	/* Ensure that we're working with a numeric value */
	if (VARINFO_IS_STRING(var->info))
		return dba_error_type("\"B%02d%03d\" (%s) is of type string and cannot be accessed as an integer",
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);

	*val = strtol(var->value, 0, 10);
	return dba_error_ok();
}

dba_err dba_var_enqd(dba_var var, double* val)
{
	if (var->value == NULL)
		return dba_error_notfound("B%02d%03d (%s) is not defined",
					DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);

	/* Ensure that we're working with a numeric value */
	if (VARINFO_IS_STRING(var->info))
		return dba_error_type("\"B%02d%03d\" (%s) is of type string and cannot be accessed as a floating-point",
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);

	*val = dba_var_decode_int(strtol(var->value, 0, 10), var->info);
	return dba_error_ok();
}

dba_err dba_var_enqc(dba_var var, const char** val)
{
	if (var->value == NULL)
		return dba_error_notfound("B%02d%03d (%s) is not defined",
					DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);

	*val = var->value;
	return dba_error_ok();
}

dba_err dba_var_seti(dba_var var, int val)
{
	/* Ensure that we're working with a numeric value */
	if (VARINFO_IS_STRING(var->info))
		return dba_error_type("\"B%02d%03d\" is of type string and cannot be accessed as an integer",
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var));

	/* Guard against overflows */
	if (val < var->info->imin || val > var->info->imax)
	{
		DBA_RUN_OR_RETURN(dba_var_unset(var));
		return dba_error_toolong("Value %i is outside the range [%i,%i] for B%02d%03d (%s)",
				val, var->info->imin, var->info->imax,
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);
	}
	
	/* Set the value */
	if (var->value == NULL &&
		(var->value = (char*)malloc(var->info->len + 2)) == NULL)
		return dba_error_alloc("allocating space for dba_var value");

	/* We add 1 to the length to cope with the '-' sign */
	strcpy(var->value, itoa(val, var->info->len + 1));
	/*snprintf(var->value, var->info->len + 2, "%d", val);*/
	return dba_error_ok();
}

dba_err dba_var_setd(dba_var var, double val)
{
	/* Ensure that we're working with a numeric value */
	if (VARINFO_IS_STRING(var->info))
		return dba_error_type("\"B%02d%03d\" is of type string and cannot be accessed as a floating-point",
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var));

	/* Guard against NaNs */
	if (isnan(val))
	{
		DBA_RUN_OR_RETURN(dba_var_unset(var));
		return dba_error_toolong("Value %g is outside the range [%g,%g] for B%02d%03d (%s)",
				val, var->info->dmin, var->info->dmax,
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);
	}
	
	/* Guard against overflows */
	if (val < var->info->dmin || val > var->info->dmax)
	{
		DBA_RUN_OR_RETURN(dba_var_unset(var));
		return dba_error_toolong("Value %g is outside the range [%g,%g] for B%02d%03d (%s)",
				val, var->info->dmin, var->info->dmax,
				DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc);
	}

	/* Set the value */
	if (var->value == NULL && 
		(var->value = (char*)malloc(var->info->len + 2)) == NULL)
		return dba_error_alloc("allocating space for dba_var value");

	strcpy(var->value, itoa(dba_var_encode_int(val, var->info), var->info->len + 1));
	/*snprintf(var->value, var->info->len + 2, "%ld", (long)dba_var_encode_int(val, var->info));*/
	return dba_error_ok();
}

dba_err dba_var_setc(dba_var var, const char* val)
{
	int len;

	/* Set the value */
	if (var->value == NULL &&
		(var->value = (char*)malloc(var->info->len + 2)) == NULL)
		return dba_error_alloc("allocating space for dba_var value");

	/* Guard against overflows */
	len = strlen(val);
	/* Tweak the length to account for the extra leading '-' allowed for
	 * negative numeric values */
	if (!VARINFO_IS_STRING(var->info) && val[0] == '-')
		--len;
	if (len > var->info->len)
	{
		DBA_RUN_OR_RETURN(dba_var_unset(var));
		return dba_error_toolong("Value \"%s\" is too long for B%02d%03d (%s): maximum length is %d",
				val, DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var), var->info->desc, var->info->len);
	}

	strncpy(var->value, val, var->info->len + 1);
	var->value[var->info->len + 1] = 0;
	return dba_error_ok();
}

dba_err dba_var_unset(dba_var var)
{
	/* Set the value */
	if (var->value != NULL)
		free(var->value);
	var->value = NULL;
	return dba_error_ok();
}

dba_err dba_var_copy_val(dba_var dest, dba_var orig)
{
	dba_err err;
	dba_varinfo destinfo = dba_var_info(dest);

	if (dba_var_value(orig) == NULL)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_var_unset(dest));
	} else {
		if (VARINFO_IS_STRING(destinfo))
		{
			DBA_RUN_OR_GOTO(cleanup, dba_var_setc(dest, dba_var_value(orig)));
		} else {
			dba_varinfo originfo = dba_var_info(orig);
			double val;
			double converted;

			/* Get the value to convert */
			DBA_RUN_OR_GOTO(cleanup, dba_var_enqd(orig, &val));

			/* Convert the value */
			DBA_RUN_OR_GOTO(cleanup, dba_convert_units(originfo->unit, destinfo->unit, val, &converted));

			/* Set the new value */
			DBA_RUN_OR_GOTO(cleanup, dba_var_setd(dest, converted));
		}
	}

	/* Copy the attributes */
	dba_var_clear_attrs(dest);
	DBA_RUN_OR_GOTO(cleanup, dba_var_copy_attrs(dest, orig));

	return dba_error_ok();

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_var_copy_attrs(dba_var dest, dba_var src)
{
	dba_var_attr cur;
	for (cur = src->attrs; cur != NULL; cur = cur->next)
		DBA_RUN_OR_RETURN(dba_var_seta(dest, cur->var));
	return dba_error_ok();
}

dba_err dba_var_convert(dba_var orig, dba_varinfo info, dba_var* conv)
{
	dba_err err;

	DBA_RUN_OR_RETURN(dba_var_create(info, conv));
	DBA_RUN_OR_GOTO(fail, dba_var_copy_val(*conv, orig));
	
	return dba_error_ok();

fail:
	dba_var_delete(*conv);
	*conv = NULL;
	return err;
}

void dba_var_print(dba_var var, FILE* out)
{
	dba_var_attr cur;

	fprintf(out, "%d%02d%03d %-.64s(%s): ",
			DBA_VAR_F(var->info->var), DBA_VAR_X(var->info->var), DBA_VAR_Y(var->info->var),
			var->info->desc, var->info->unit);

	if (var->value == NULL)
		fprintf(out, "(undef)\n");
	else if (VARINFO_IS_STRING(var->info) || var->info->scale == 0)
		fprintf(out, "%s\n", var->value);
	else
	{
		double val;
		dba_var_enqd(var, &val);
		fprintf(out, "%.*f\n", var->info->scale > 0 ? var->info->scale : 0, val);
	}

	for (cur = var->attrs; cur != NULL; cur = cur->next)
	{
		fputs("           ", out);
		dba_var_print(cur->var, out);
	}
}

int dba_var_equals(const dba_var var1, const dba_var var2)
{
	if (var1 == NULL && var2 == NULL)
		return 1;
	if (var1 == NULL || var2 == NULL)
		return 0;
	if (dba_var_code(var1) != dba_var_code(var2))
		return 0;
	if (var1->value == NULL && var2->value == NULL)
		return 1;
	if (var1->value == NULL || var2->value == NULL)
		return 0;
	if (VARINFO_IS_STRING(var1->info) || var1->info->scale == 0)
	{
		if (strcmp(var1->value, var2->value) != 0)
			return 0;
	}
	else
	{
		double val1, val2;
		dba_var_enqd(var1, &val1);
		dba_var_enqd(var2, &val2);
		if (val1 != val2)
			return 0;
	}

	if ((var1->attrs != 0) != (var2->attrs != 0))
		return 0;

	dba_var_attr cur;
	int count1 = 0, count2 = 0;

	for (cur = var1->attrs; cur != NULL; cur = cur->next)
		++count1;
	for (cur = var2->attrs; cur != NULL; cur = cur->next)
		++count2;

	if (count1 != count2)
		return 0;
	else {
		/* Check attributes */
		for (cur = var1->attrs; cur != NULL; cur = cur->next)
		{
			dba_var_attr cur2;
			int found = 0;

			/* We cannot call enqa because it would tamper the error status */
			for (cur2 = var2->attrs; cur2 != NULL; cur2 = cur2->next)
				if (dba_var_code(cur2->var) == dba_var_code(cur->var))
				{
					found = 1;
					if (!dba_var_equals(cur->var, cur2->var))
						return 0;
					break;
				}

			if (!found)
				return 0;
		}
	}
	return 1;
}

void dba_var_diff(dba_var var1, dba_var var2, int* diffs, FILE* out)
{
	if (var1 == NULL && var2 == NULL)
		return;
	if (var1 == NULL)
	{
		fprintf(out, "[%d%02d%03d %s] first var is NULL, second is %s\n",
			DBA_VAR_F(var2->info->var), DBA_VAR_X(var2->info->var), DBA_VAR_Y(var2->info->var), var2->info->desc, var2->value);
		(*diffs)++;
		return;
	}
	else if (var2 == NULL)
	{
		fprintf(out, "[%d%02d%03d %s] second var is NULL, first is %s\n",
			DBA_VAR_F(var1->info->var), DBA_VAR_X(var1->info->var), DBA_VAR_Y(var1->info->var), var1->info->desc, var1->value);
		(*diffs)++;
		return;
	}
	if (dba_var_code(var1) != dba_var_code(var2))
	{
		fprintf(out, "varcodes differ: first is %d%02d%03d'%s', second is %d%02d%03d'%s'\n",
			DBA_VAR_F(var1->info->var), DBA_VAR_X(var1->info->var), DBA_VAR_Y(var1->info->var), var1->info->desc,
			DBA_VAR_F(var2->info->var), DBA_VAR_X(var2->info->var), DBA_VAR_Y(var2->info->var), var2->info->desc);
		(*diffs)++;
		return;
	}
	if (var1->value == NULL && var2->value == NULL)
		return;
	if (var1->value == NULL)
	{
		fprintf(out, "[%d%02d%03d %s] first value is NULL, second value is %s\n",
			DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)), var1->info->desc,
			var2->value);
		(*diffs)++;
		return;
	}
	if (var2->value == NULL)
	{
		fprintf(out, "[%d%02d%03d %s] first value is %s, second value is NULL\n",
			DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)), var1->info->desc,
			var1->value);
		(*diffs)++;
		return;
	}
	if (VARINFO_IS_STRING(var1->info) || var1->info->scale == 0)
	{
		if (strcmp(var1->value, var2->value) != 0)
		{
			fprintf(out, "[%d%02d%03d %s] values differ: first is %s, second is %s\n",
				DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)), var1->info->desc,
				var1->value, var2->value);
			(*diffs)++;
			return;
		}
	}
	else
	{
		double val1, val2;
		dba_var_enqd(var1, &val1);
		dba_var_enqd(var2, &val2);
		if (val1 != val2)
		{
			fprintf(out, "[%d%02d%03d %s] values differ: first is %f, second is %f\n",
				DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)), var1->info->desc,
				val1, val2);
			(*diffs)++;
			return;
		}
	}

	if ((var1->attrs != 0) != (var2->attrs != 0))
	{
		if (var1->attrs)
		{
			fprintf(out, "[%d%02d%03d %s] attributes differ: first has attributes, second does not\n",
				DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)),
				var1->info->desc);
			(*diffs)++;
		}
		else
		{
			fprintf(out, "[%d%02d%03d %s] attributes differ: first does not have attributes, second does\n",
				DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)),
				var1->info->desc);
			(*diffs)++;
		}
	} else {
		dba_var_attr cur;
		int count1 = 0, count2 = 0;

		for (cur = var1->attrs; cur != NULL; cur = cur->next)
			++count1;
		for (cur = var2->attrs; cur != NULL; cur = cur->next)
			++count2;

		if (count1 != count2)
		{
			fprintf(out, "[%d%02d%03d %s] attributes differ: first has %d, second has %d\n",
				DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)),
				var1->info->desc, count1, count2);
			(*diffs) += abs(count1 - count2);
		} else {
			/* Check attributes */
			for (cur = var1->attrs; cur != NULL; cur = cur->next)
			{
				dba_var_attr cur2;
				int found = 0;

				/* We cannot call enqa because it would tamper the error status */
				for (cur2 = var2->attrs; cur2 != NULL; cur2 = cur2->next)
					if (dba_var_code(cur2->var) == dba_var_code(cur->var))
					{
						found = 1;
						dba_var_diff(cur->var, cur2->var, diffs, out);
						break;
					}

				if (!found)
				{
					fprintf(out, "[%d%02d%03d %s] attributes differ: attribute %d%02d%03d exists only on first\n",
						DBA_VAR_F(dba_var_code(var1)), DBA_VAR_X(dba_var_code(var1)), DBA_VAR_Y(dba_var_code(var1)),
						var1->info->desc, 
						DBA_VAR_F(dba_var_code(cur->var)), DBA_VAR_X(dba_var_code(cur->var)), DBA_VAR_Y(dba_var_code(cur->var)));
					(*diffs)++;
				}
			}
		}
	}
}

/* vim:set ts=4 sw=4: */
