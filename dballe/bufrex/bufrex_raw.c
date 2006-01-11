#include <config.h>

#include "bufrex_opcode.h"
#include "bufrex_raw.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


dba_err bufrex_raw_create(bufrex_raw* msg, bufrex_type type)
{
	*msg = (bufrex_raw)calloc(1, sizeof(struct _bufrex_raw));
	if (*msg == NULL)
		return dba_error_alloc("allocating new storage for decoded BUFR/CREX data");
    (*msg)->datadesc_last = &((*msg)->datadesc);
	(*msg)->encoding_type = type;
	return dba_error_ok();
}

void bufrex_raw_reset(bufrex_raw msg)
{
	int i;

	/* Preserve vars and vars_alloclen so that allocated memory can be reused */
	for (i = 0; i < msg->vars_count; i++)
		dba_var_delete(msg->vars[i]);
	msg->vars_count = 0;

	if (msg->datadesc != NULL)
	{
		bufrex_opcode_delete(&(msg->datadesc));
		msg->datadesc_last = &(msg->datadesc);
	}

	msg->type = 0;
	msg->subtype = 0;
}

void bufrex_raw_delete(bufrex_raw msg)
{
	bufrex_raw_reset(msg);

	if (msg->vars)
	{
		free(msg->vars);
		msg->vars = NULL;
		msg->vars_alloclen = 0;
	}

	free(msg);
}

dba_err bufrex_raw_get_table_id(bufrex_raw msg, const char** id)
{
	if (msg->btable == NULL)
		*id = NULL;
	else
		*id = dba_vartable_id(msg->btable);
	return dba_error_ok();
}

dba_err bufrex_raw_load_tables(bufrex_raw msg)
{
	char id[30];
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR:
			sprintf(id, "B00000%03d%03d%02d%02d",
					0,
					msg->opt.bufr.origin,
					msg->opt.bufr.master_table,
					msg->opt.bufr.local_table);
			break;
		case BUFREX_CREX:
			sprintf(id, "B%02d%02d%02d",
					msg->opt.crex.master_table,
					msg->edition,
					msg->opt.crex.table);
			break;
	}

	DBA_RUN_OR_RETURN(dba_vartable_create(id, &(msg->btable)));
	/* TRACE(" -> loaded B table %s\n", id); */

	id[0] = 'D';
	DBA_RUN_OR_RETURN(bufrex_dtable_create(id, &(msg->dtable)));
	/* TRACE(" -> loaded D table %s\n", id); */

	return dba_error_ok();
}

dba_err bufrex_raw_query_btable(bufrex_raw msg, dba_varcode code, dba_varinfo* info)
{
	return dba_vartable_query(msg->btable, code, info);
}

dba_err bufrex_raw_query_dtable(bufrex_raw msg, dba_varcode code, struct _bufrex_opcode** res)
{
	return bufrex_dtable_query(msg->dtable, code, res);
}


void bufrex_raw_reset_datadesc(bufrex_raw msg)
{
	if (msg->datadesc != NULL)
	{
		bufrex_opcode_delete(&(msg->datadesc));
		msg->datadesc_last = &(msg->datadesc);
	}
}

dba_err bufrex_raw_get_datadesc(bufrex_raw msg, bufrex_opcode* res)
{
	*res = NULL;
	return bufrex_opcode_prepend(res, msg->datadesc);
}

dba_err bufrex_raw_append_datadesc(bufrex_raw msg, dba_varcode varcode)
{
	DBA_RUN_OR_RETURN(bufrex_opcode_append(msg->datadesc_last, varcode));
	msg->datadesc_last = &((*(msg->datadesc_last))->next);
	return dba_error_ok();
}

dba_err bufrex_raw_store_variable(bufrex_raw msg, dba_var var)
{
	/* Check if we need to enlarge the buffer size */
	if (msg->vars_count == msg->vars_alloclen)
	{
		/* Enlarge the buffer size */
		if (msg->vars == NULL)
		{
			msg->vars_alloclen = 32;
			if ((msg->vars = (dba_var*)malloc(msg->vars_alloclen * sizeof(dba_var))) == NULL)
				return dba_error_alloc("allocating memory for decoded message variables");
		} else {
			dba_var* newbuf;

			/* Grow by doubling the allocated space */
			msg->vars_alloclen <<= 1;

			if ((newbuf = (dba_var*)realloc(msg->vars, msg->vars_alloclen * sizeof(dba_var))) == NULL)
				return dba_error_alloc("allocating more memory for message data");
			msg->vars = newbuf;
		}
	}

	msg->vars[msg->vars_count++] = var;
	return dba_error_ok();
}

dba_err bufrex_raw_store_variable_var(bufrex_raw msg, dba_varcode code, dba_var val)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(msg->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_create(info, &var));
	DBA_RUN_OR_RETURN(dba_var_copy_val(var, val));
	return bufrex_raw_store_variable(msg, var);
}

dba_err bufrex_raw_store_variable_i(bufrex_raw msg, dba_varcode code, int val)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(msg->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_createi(info, &var, val));
	return bufrex_raw_store_variable(msg, var);
}

dba_err bufrex_raw_store_variable_d(bufrex_raw msg, dba_varcode code, double val)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(msg->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_created(info, &var, val));
	return bufrex_raw_store_variable(msg, var);
}

dba_err bufrex_raw_store_variable_c(bufrex_raw msg, dba_varcode code, const char* val)
{
	if (val == NULL || val[0] == 0)
	{
		return bufrex_raw_store_variable_undef(msg, code);
	} else {
		dba_var var;
		dba_varinfo info;
		DBA_RUN_OR_RETURN(dba_vartable_query(msg->btable, code, &info));
		DBA_RUN_OR_RETURN(dba_var_createc(info, &var, val));
		return bufrex_raw_store_variable(msg, var);
	}
}

dba_err bufrex_raw_store_variable_undef(bufrex_raw msg, dba_varcode code)
{
	dba_var var;
	dba_varinfo info;
	DBA_RUN_OR_RETURN(dba_vartable_query(msg->btable, code, &info));
	DBA_RUN_OR_RETURN(dba_var_create(info, &var));
	return bufrex_raw_store_variable(msg, var);
}

dba_err bufrex_raw_add_attrs(bufrex_raw msg, dba_var var)
{
	if (msg->vars_count == 0)
		return dba_error_consistency("checking that some variable was previously appended");
	return dba_var_copy_attrs(msg->vars[msg->vars_count - 1], var);
}

dba_err bufrex_raw_apply_attributes(bufrex_raw msg)
{
	int i = 0;
	int cur_dpb = -1;
	int cur_attr = -1;

	/* First step: scan the variable list and note the index of the first data
	 * present bitmap and the first quality attribute */
	for ( ; i < msg->vars_count; i++)
		if (dba_var_code(msg->vars[i]) == DBA_VAR(0, 31, 31))
		{
			cur_dpb = i;
			break;
		}
	for ( ; i < msg->vars_count; i++)
	{
		dba_varcode code = dba_var_code(msg->vars[i]);
		if (DBA_VAR_F(code) == 0 && DBA_VAR_X(code) == 33)
		{
			cur_attr = i;
			break;
		}
	}

	/* Nothing to do if the data is not present */
	if (cur_dpb == -1 || cur_attr == -1)
		return dba_error_ok();
	

	/* Second step: iterate through the three lists applying the changes */
	for (i = 0; i < msg->vars_count &&
			cur_dpb < msg->vars_count &&
			dba_var_code(msg->vars[cur_dpb]) == DBA_VAR(0, 31, 31) &&
			cur_attr < msg->vars_count &&
			DBA_VAR_F(dba_var_code(msg->vars[cur_attr])) == 0 &&
			DBA_VAR_X(dba_var_code(msg->vars[cur_attr])) == 33; i++)
	{
		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(msg->vars[i])) != 0)
			continue;
		/* Skip over variables that don't have a 0 in the data present bitmap */
		if (dba_var_value(msg->vars[cur_dpb++]) == NULL)
			continue;
		DBA_RUN_OR_RETURN(dba_var_seta(msg->vars[i], msg->vars[cur_attr++]));
	}
	return dba_error_ok();
}

dba_err bufrex_raw_append_dpb(bufrex_raw msg, int size, dba_varcode attr)
{
	int i;

	/* Add repetition count */
	bufrex_raw_store_variable_i(msg, DBA_VAR(0, 31, 2), size);
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < msg->vars_count && size > 0; i++)
	{
		dba_var test_var;

		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(msg->vars[i])) != 0)
			continue;

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_RETURN(dba_var_enqa(msg->vars[i], attr, &test_var));
		if (test_var != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(msg, DBA_VAR(0, 31, 31), 0));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(msg, DBA_VAR(0, 31, 31)));
		size--;
	}
	return dba_error_ok();
}

dba_err bufrex_raw_append_fixed_dpb(bufrex_raw msg, int size)
{
	int i;

	for (i = 0; i < size; i++)
		DBA_RUN_OR_RETURN(bufrex_raw_store_variable_i(msg, DBA_VAR(0, 31, 31), 0));

	return dba_error_ok();
}

dba_err bufrex_raw_append_attrs(bufrex_raw msg, int size, dba_varcode attr)
{
	int i;
	int repcount_idx;
	int added = 0;

	/* Add delayed repetition count with an initial value of 0, and mark its position */
	bufrex_raw_store_variable_i(msg, DBA_VAR(0, 31, 2), 0);
	repcount_idx = msg->vars_count - 1;
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < msg->vars_count && size > 0; i++)
	{
		dba_var var_attr;

		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(msg->vars[i])) != 0)
			continue;

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_RETURN(dba_var_enqa(msg->vars[i], attr, &var_attr));
		if (var_attr != NULL)
		{
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(msg, attr, var_attr));
			added++;
		}
		size--;
	}

	/* Set the repetition count with the number of variables we added */
	DBA_RUN_OR_RETURN(dba_var_seti(msg->vars[repcount_idx], added));

	return dba_error_ok();
}

dba_err bufrex_raw_append_fixed_attrs(bufrex_raw msg, int size, dba_varcode attr)
{
	int i;
	
	/* Scan first 'size' variables checking for the presence of 'attr' */
	for (i = 0; i < msg->vars_count && size > 0; i++)
	{
		dba_var var_attr;

		/* Skip over special data like delayed repetition counts */
		if (DBA_VAR_F(dba_var_code(msg->vars[i])) != 0)
			continue;

		/* Check if the variable has the attribute we want */
		DBA_RUN_OR_RETURN(dba_var_enqa(msg->vars[i], attr, &var_attr));
		if (var_attr != NULL)
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_var(msg, attr, var_attr));
		else
			DBA_RUN_OR_RETURN(bufrex_raw_store_variable_undef(msg, attr));
		size--;
	}

	return dba_error_ok();
}

dba_err bufrex_raw_decode(bufrex_raw msg, dba_rawmsg raw)
{
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: return bufr_decoder_decode(raw, msg);
		case BUFREX_CREX: return crex_decoder_decode(raw, msg);
	}
	return dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
}

dba_err bufrex_raw_encode(bufrex_raw msg, dba_rawmsg* raw)
{
	dba_err err;

	DBA_RUN_OR_RETURN(dba_rawmsg_create(raw));
	
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: DBA_RUN_OR_GOTO(fail, bufr_encoder_encode(msg, *raw)); break;
		case BUFREX_CREX: DBA_RUN_OR_GOTO(fail, crex_encoder_encode(msg, *raw)); break;
		default:
			err = dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
			goto fail;
	}

	return dba_error_ok();

fail:
	if (*raw != NULL)
		dba_rawmsg_delete(*raw);
	return err;
}

void bufrex_raw_print(bufrex_raw msg, FILE* out)
{
	bufrex_opcode cur;
	int i;
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: fprintf(out, "BUFR o%d m%d l%d", msg->opt.bufr.origin, msg->opt.bufr.master_table, msg->opt.bufr.local_table); break;
		case BUFREX_CREX: fprintf(out, "CREX T00%02d%02d", msg->opt.crex.master_table, msg->opt.crex.table); break;
	}
	fprintf(out, " type %d subtype %d edition %d table %s alloc %d, %d vars.\n",
			msg->type, msg->subtype, msg->edition, msg->btable == NULL ? NULL : dba_vartable_id(msg->btable),
			msg->vars_alloclen, msg->vars_count);
	fprintf(out, "Data descriptors:");
	for (cur = msg->datadesc; cur != NULL; cur = cur->next)
		fprintf(out, " %d%02d%03d", DBA_VAR_F(cur->val), DBA_VAR_X(cur->val), DBA_VAR_Y(cur->val));
	putc('\n', out);
	for (i = 0; i < msg->vars_count; i++)
		dba_var_print(msg->vars[i], out);
}

#if 0
#include <dballe/dba_check.h>

#ifdef HAVE_CHECK

#include <string.h> /* strcmp */

void test_bufrex_raw()
{
	/* dba_err err; */
	int val, val1;
	dba_var* vars;
	bufrex_raw msg;

	CHECKED(bufrex_raw_create(&msg));

	/* Resetting things should not fail */
	bufrex_raw_reset(msg);

	/* The message should be properly empty */
	CHECKED(bufrex_raw_get_category(msg, &val, &val1));
	fail_unless(val == 0);
	fail_unless(val1 == 0);
	CHECKED(bufrex_raw_get_vars(msg, &vars, &val));
	fail_unless(vars == 0);
	fail_unless(val == 0);

	CHECKED(bufrex_raw_set_category(msg, 42, 24));
	CHECKED(bufrex_raw_get_category(msg, &val, &val1));
	fail_unless(val == 42);
	fail_unless(val1 == 24);

	{
		dba_varinfo info;
		dba_var v;
		CHECKED(dba_varinfo_query_local(DBA_VAR(0, 1, 2), &info));
		CHECKED(dba_var_createi(info, &v, 7));
		CHECKED(bufrex_raw_store_variable(msg, v));
	}

	CHECKED(bufrex_raw_get_vars(msg, &vars, &val));
	fail_unless(vars != 0);
	fail_unless(vars[0] != 0);
	fail_unless(val == 1);
	fail_unless(dba_var_code(vars[0]) == DBA_VAR(0, 1, 2));
	fail_unless(strcmp(dba_var_value(vars[0]), "7") == 0);

	bufrex_raw_delete(msg);
}

#endif
#endif

/* vim:set ts=4 sw=4: */
