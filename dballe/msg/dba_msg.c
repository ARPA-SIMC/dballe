#include "dba_msg.h"

#include <stdlib.h>
#include <string.h>

dba_err dba_msg_create(dba_msg* msg)
{
	dba_msg res = (dba_msg)calloc(1, sizeof(struct _dba_msg));
	if (res == NULL)
		return dba_error_alloc("allocating new dba_msg");

	res->type = MSG_GENERIC;
	res->data_count = 0;
	res->data_alloc = 0;
	res->data = NULL;

	*msg = res;
	
	return dba_error_ok();
}

/* Enlarge the number of data that can be held my msg, so that it is at least 1
 * (possibly much more) more than it is now */
static dba_err dba_msg_enlarge(dba_msg msg)
{
	const int new_size = msg->data_alloc == 0 ? 10 : msg->data_alloc * 2;

	if (msg->data == NULL)
	{
		msg->data = (dba_msg_level*)calloc(new_size, sizeof(dba_msg_level));
		if (msg->data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg");
	} else {
		int i;
		dba_msg_level* new_data;
		new_data = (dba_msg_level*)realloc(msg->data, new_size * sizeof(dba_msg_level));
		if (new_data == NULL)
			return dba_error_alloc("enlarging the size of a dba_msg");
		msg->data = new_data;
		
		for (i = msg->data_count; i < new_size; i++)
			msg->data[i] = NULL;
	}
	msg->data_alloc = new_size;

	return dba_error_ok();
}

static dba_err dba_msg_add_level_nocopy(dba_msg msg, dba_msg_level lev)
{
	int pos;

	/* Enlarge the buffer if needed */
	if (msg->data_count == msg->data_alloc)
		DBA_RUN_OR_RETURN(dba_msg_enlarge(msg));

	/* Insertionsort.  Crude, but our datasets should be too small for an
	 * RB-Tree to be worth */

	for (pos = msg->data_count; pos > 0; pos--)
		if (dba_msg_level_compare(msg->data[pos - 1], lev) > 0)
			msg->data[pos] = msg->data[pos - 1];
		else
			break;
	msg->data[pos] = lev;

	msg->data_count++;

	return dba_error_ok();
}

static dba_err dba_msg_add_level(dba_msg msg, dba_msg_level lev)
{
	dba_err err;
	dba_msg_level copy = NULL;
	DBA_RUN_OR_RETURN(dba_msg_level_copy(lev, &copy));
	DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(msg, copy));
	return dba_error_ok();

fail:
	if (copy)
		dba_msg_level_delete(copy);
	return err;
}

dba_err dba_msg_set_nocopy(dba_msg msg, dba_var var, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_msg_level lev = dba_msg_find_level(msg, ltype, l1, l2);

	if (lev == NULL)
	{
		/* Create the level if missing */

		DBA_RUN_OR_RETURN(dba_msg_level_create(&lev, ltype, l1, l2));
		DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(msg, lev));
	}

	return dba_msg_level_set_nocopy(lev, var, pind, p1, p2);
	
fail:
	if (lev != NULL)
		dba_msg_level_delete(lev);
	return err;
}

dba_err dba_msg_set_nocopy_by_id(dba_msg msg, dba_var var, int id)
{
	dba_msg_var v = &dba_msg_vartable[id];
	return dba_msg_set_nocopy(msg, var, v->ltype, v->l1, v->l2, v->pind, v->p1, v->p2);
}

dba_err dba_msg_set_by_id(dba_msg msg, dba_var var, int id)
{
	dba_msg_var v = &dba_msg_vartable[id];
	dba_err err;
	dba_var copy = NULL;

	/* Make a copy of the variable, to give it to dba_msg_add_nocopy */
	DBA_RUN_OR_RETURN(dba_var_create_local(v->code, &copy));
	/* Use copy_val to ensure we get the variable code we want */
	DBA_RUN_OR_GOTO(fail, dba_var_copy_val(copy, var));

	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, copy, v->ltype, v->l1, v->l2, v->pind, v->p1, v->p2));

	return dba_error_ok();

fail:
	if (copy != NULL)
		dba_var_delete(copy);
	return err;
}

dba_err dba_msg_set(dba_msg msg, dba_var var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var copy = NULL;

	/* Make a copy of the variable, to give it to dba_msg_add_nocopy */
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &copy));
	/* Use copy_val to ensure we get the variable code we want */
	DBA_RUN_OR_GOTO(fail, dba_var_copy_val(copy, var));

	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, copy, ltype, l1, l2, pind, p1, p2));

	return dba_error_ok();

fail:
	if (copy != NULL)
		dba_var_delete(copy);
	return err;
}

dba_err dba_msg_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var var = NULL;
	dba_var attr = NULL;
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &var));
	DBA_RUN_OR_GOTO(fail, dba_var_seti(var, val));
	if (conf != -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_seti(attr, conf));
		DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, attr));
		attr = NULL;
	}
	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	return dba_error_ok();
	
fail:
	if (var != NULL)
		dba_var_delete(var);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

dba_err dba_msg_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var var = NULL;
	dba_var attr = NULL;
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &var));
	DBA_RUN_OR_GOTO(fail, dba_var_setd(var, val));
	if (conf != -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_seti(attr, conf));
		DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, attr));
		attr = NULL;
	}
	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	return dba_error_ok();
	
fail:
	if (var != NULL)
		dba_var_delete(var);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

dba_err dba_msg_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_err err;
	dba_var var = NULL;
	dba_var attr = NULL;
	DBA_RUN_OR_RETURN(dba_var_create_local(code, &var));
	DBA_RUN_OR_GOTO(fail, dba_var_setc(var, val));
	if (conf != -1)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_seti(attr, conf));
		DBA_RUN_OR_GOTO(fail, dba_var_seta_nocopy(var, attr));
		attr = NULL;
	}
	DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	return dba_error_ok();
	
fail:
	if (var != NULL)
		dba_var_delete(var);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

dba_msg_level dba_msg_find_level(dba_msg msg, int ltype, int l1, int l2)
{
	int begin, end;

	/* Binary search */
	begin = -1, end = msg->data_count;
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (dba_msg_level_compare2(msg->data[cur], ltype, l1, l2) > 0)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || dba_msg_level_compare2(msg->data[begin], ltype, l1, l2) != 0)
		return NULL;
	else
		return msg->data[begin];
}

dba_msg_datum dba_msg_find(dba_msg msg, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2)
{
	dba_msg_level lev = dba_msg_find_level(msg, ltype, l1, l2);
	if (lev == NULL)
		return NULL;
	return dba_msg_level_find(lev, code, pind, p1, p2);
}

dba_msg_datum dba_msg_find_by_id(dba_msg msg, int id)
{
	dba_msg_var v = &dba_msg_vartable[id];
	return dba_msg_find(msg, v->code, v->ltype, v->l1, v->l2, v->pind, v->p1, v->p2);
}

dba_msg_type dba_msg_get_type(dba_msg msg)
{
	return msg->type;
}

const char* dba_msg_type_name(dba_msg_type type)
{
	switch (type)
	{
		case MSG_GENERIC: return "generic";
		case MSG_SYNOP: return "synop";
		case MSG_PILOT: return "pilot";
		case MSG_TEMP: return "temp";
		case MSG_TEMP_SHIP: return "temp_ship";
		case MSG_AIREP: return "airep";
		case MSG_AMDAR: return "amdar";
		case MSG_ACARS: return "acars";
		case MSG_SHIP: return "ship";
		case MSG_BUOY: return "buoy";
		case MSG_METAR: return "metar";
	}
	return "(unknown)";
}

dba_msg_type dba_msg_type_from_repcod(int repcod)
{
	switch (repcod)
	{
		case 1:  return MSG_SYNOP;
		case 2:  return MSG_METAR;
		case 10: return MSG_SHIP;
		case 9:  return MSG_BUOY;
		case 12: return MSG_AIREP;
		case 13: return MSG_AMDAR;
		case 14: return MSG_ACARS;
		case 4:  return MSG_PILOT;
		case 3:  return MSG_TEMP;		
		case 11: return MSG_TEMP_SHIP;
		case 255:
		default: return MSG_GENERIC;
	}
}

int dba_msg_repcod_from_type(dba_msg_type type)
{
	switch (type)
	{
		case MSG_SYNOP:		return 1;
		case MSG_METAR:		return 2;
		case MSG_SHIP:		return 10;
		case MSG_BUOY:		return 9;
		case MSG_AIREP:		return 12;
		case MSG_AMDAR:		return 13;
		case MSG_ACARS:		return 14;
		case MSG_PILOT:		return 4;
		case MSG_TEMP:		return 3;
		case MSG_TEMP_SHIP:	return 11;
		case MSG_GENERIC:
		default:			return 255;
	}
}


dba_err dba_msg_sounding_pack_levels(dba_msg msg, dba_msg* dst)
{
	dba_err err;
	dba_msg res = NULL;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&res));
	res->type = msg->type;

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		dba_msg_datum d;
		int j;

        if (lev->ltype != 100 ||
				(d = dba_msg_level_find(lev, DBA_VAR(0, 8, 1), 0, 0, 0)) == NULL ||
				dba_var_value(d->var) == NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level(res, msg->data[i]));
			continue;
		}

		for (j = 0; j < lev->data_count; j++)
		{
			dba_var copy;
			DBA_RUN_OR_GOTO(fail, dba_var_copy(lev->data[j]->var, &copy));
			DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(res, copy, lev->ltype, lev->l1, 0, d->pind, d->p1, d->p2));
		}
	}

	*dst = res;

	return dba_error_ok();

fail:
	*dst = 0;
	if (res != NULL)
		dba_msg_delete(res);
	return err;
}

dba_err dba_msg_sounding_unpack_levels(dba_msg msg, dba_msg* dst)
{
	const int VSIG_MISSING = 1;
	const int VSIG_SIGWIND = 2;		/* 6 */
	const int VSIG_SIGTEMP = 4;		/* 5 */
	const int VSIG_MAXWIND = 8;		/* 4 */
	const int VSIG_TROPOPAUSE = 16;	/* 3 */
	const int VSIG_STANDARD = 32;	/* 2 */
	const int VSIG_SURFACE = 64;	/* 1 */

	dba_err err;
	dba_msg res = NULL;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&res));
	res->type = msg->type;

	for (i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		dba_msg_level copy;
		dba_msg_datum d;
		int vsig;
        if (lev->ltype != 100 ||
				(d = dba_msg_level_find(lev, DBA_VAR(0, 8, 1), 0, 0, 0)) == NULL ||
				dba_var_value(d->var) == NULL)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level(res, msg->data[i]));
			continue;
		}

		DBA_RUN_OR_GOTO(fail, dba_var_enqi(d->var, &vsig));
		if (vsig & VSIG_MISSING)
		{
			/* If there is no vsig, then we consider it a normal level */
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level(res, msg->data[i]));
			continue;
		}

		/* DBA_RUN_OR_GOTO(fail, dba_var_enqi(msg->data[i].var_press, &press)); */

		/* TODO: delete the dba_msg_datum that do not belong in that level */

		if (vsig & VSIG_SIGWIND)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_level_copy(lev, &copy));
			copy->l2 = 6;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(res, copy));
		}
		if (vsig & VSIG_SIGTEMP)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_level_copy(lev, &copy));
			copy->l2 = 5;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(res, copy));
		}
		if (vsig & VSIG_MAXWIND)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_level_copy(lev, &copy));
			copy->l2 = 4;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(res, copy));
		}
		if (vsig & VSIG_TROPOPAUSE)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_level_copy(lev, &copy));
			copy->l2 = 3;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(res, copy));
		}
		if (vsig & VSIG_STANDARD)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_level_copy(lev, &copy));
			copy->l2 = 2;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(res, copy));
		}
		if (vsig & VSIG_SURFACE)
		{
			DBA_RUN_OR_GOTO(fail, dba_msg_level_copy(lev, &copy));
			copy->l2 = 1;
			DBA_RUN_OR_GOTO(fail, dba_msg_add_level_nocopy(res, copy));
		}
	}

	*dst = res;

	return dba_error_ok();

fail:
	*dst = 0;
	if (res != NULL)
		dba_msg_delete(res);
	return err;
}

void dba_msg_print(dba_msg msg, FILE* out)
{
	fprintf(out, "%s message\n", dba_msg_type_name(msg->type));

	switch (msg->type)
	{
		case MSG_PILOT:
		case MSG_TEMP:
		case MSG_TEMP_SHIP:
			if (msg->data_count > 0)
			{
				int i;
				fprintf(out, "%d levels:\n", msg->data_count);
				for (i = 0; i < msg->data_count; i++)
				{
					dba_msg_datum d;
					if (msg->data[i]->ltype == 100 &&
							(d = dba_msg_level_find(msg->data[i], DBA_VAR(0, 8, 1), 0, 0, 0)) != NULL &&
							dba_var_value(d->var) != NULL)
					{
						int vsig = strtol(dba_var_value(d->var), 0, 10);
						const int VSIG_EXTRA = 128;
						const int VSIG_SURFACE = 64;
						const int VSIG_STANDARD = 32;
						const int VSIG_TROPOPAUSE = 16;
						const int VSIG_MAXWIND = 8;
						const int VSIG_SIGTEMP = 4;
						const int VSIG_SIGWIND = 2;
						const int VSIG_MISSING = 1;

						fprintf(out, "Sounding #%d (level %d -", i + 1, vsig);
						if (vsig & VSIG_EXTRA)
							fprintf(out, " extra");
						if (vsig & VSIG_SURFACE)
							fprintf(out, " surface");
						if (vsig & VSIG_STANDARD)
							fprintf(out, " standard");
						if (vsig & VSIG_TROPOPAUSE)
							fprintf(out, " tropopause");
						if (vsig & VSIG_MAXWIND)
							fprintf(out, " maxwind");
						if (vsig & VSIG_SIGTEMP)
							fprintf(out, " sigtemp");
						if (vsig & VSIG_SIGWIND)
							fprintf(out, " sigwind");
						if (vsig & VSIG_MISSING)
							fprintf(out, " missing");
						fprintf(out, ") ");
					}
					dba_msg_level_print(msg->data[i], out);
				}
			}
			break;
		default:
			if (msg->data_count > 0)
			{
				int i;
				fprintf(out, "%d levels:\n", msg->data_count);
				for (i = 0; i < msg->data_count; i++)
					dba_msg_level_print(msg->data[i], out);
			}
			break;
	}
}

static void level_summary(dba_msg_level l, FILE* out)
{
	fprintf(out, "l(%d,%d,%d)", l->ltype, l->l1, l->l2);
}

void dba_msg_diff(dba_msg msg1, dba_msg msg2, int* diffs, FILE* out)
{
	int i1 = 0, i2 = 0;
	if (msg1->type != msg2->type)
	{
		fprintf(out, "the messages have different type (first is %s (%d), second is %s (%d))\n",
				dba_msg_type_name(msg1->type), msg1->type, dba_msg_type_name(msg2->type), msg2->type);
		(*diffs)++;
	}
	
	while (i1 < msg1->data_count || i2 < msg2->data_count)
	{
		if (i1 == msg1->data_count)
		{
			fprintf(out, "Level "); level_summary(msg2->data[i2], out);
			fprintf(out, " exists only in the second message\n");
			++i2;
			++*diffs;
		} else if (i2 == msg2->data_count) {
			fprintf(out, "Level "); level_summary(msg1->data[i1], out);
			fprintf(out, " exists only in the first message\n");
			++i1;
			++*diffs;
		} else {
			int cmp = dba_msg_level_compare(msg1->data[i1], msg2->data[i2]);
			if (cmp == 0)
			{
				dba_msg_level_diff(msg1->data[i1], msg2->data[i2], diffs, out);
				++i1;
				++i2;
			} else if (cmp < 0) {
				if (msg1->data[i1]->data_count != 0)
				{
					fprintf(out, "Level "); level_summary(msg1->data[i1], out);
					fprintf(out, " exists only in the first message\n");
					++*diffs;
				}
				++i1;
			} else {
				if (msg2->data[i2]->data_count != 0)
				{
					fprintf(out, "Level "); level_summary(msg2->data[i2], out);
					fprintf(out, " exists only in the second message\n");
					++*diffs;
				}
				++i2;
			}
		}
	}
}

void dba_msg_delete(dba_msg m)
{
	if (m->data_alloc)
	{
		int i;
		for (i = 0; i < m->data_count; i++)
			dba_msg_level_delete(m->data[i]);
		free(m->data);
	}
	free(m);
}


/* vim:set ts=4 sw=4: */
