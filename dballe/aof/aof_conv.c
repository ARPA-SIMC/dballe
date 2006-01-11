#include <dballe/aof/aof_conv.h>

static dba_err aof_copy_from_buoy(dba_msg_buoy buoy, aof_message msg)
{
	 const char* cval;
	 int ival;
	 double dval;

#if 0
	 *** To be redesigned 
	 DBA_RUN_OR_RETURN(dba_message_set_category(msg, 4));
	 DBA_RUN_OR_RETURN(dba_message_set_subcategory(msg, 165));

	 DBA_RUN_OR_RETURN(dba_enqd(buoy->context, "lat", &dval));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_d(msg, DBA_VAR(0,  5,   2), dval));
	 DBA_RUN_OR_RETURN(dba_enqd(buoy->context, "lon", &dval));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_d(msg, DBA_VAR(0,  6,   2), dval));

	 DBA_RUN_OR_RETURN(dba_enqi(buoy->context, "year", &ival));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_i(msg, DBA_VAR(0,  4,   1), ival));
	 DBA_RUN_OR_RETURN(dba_enqi(buoy->context, "month", &ival));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_i(msg, DBA_VAR(0,  4,   2), ival));
	 DBA_RUN_OR_RETURN(dba_enqi(buoy->context, "day", &ival));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_i(msg, DBA_VAR(0,  4,   3), ival));
	 DBA_RUN_OR_RETURN(dba_enqi(buoy->context, "hour", &ival));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_i(msg, DBA_VAR(0,  4,   4), ival));
	 DBA_RUN_OR_RETURN(dba_enqi(buoy->context, "min", &ival));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_i(msg, DBA_VAR(0,  4,   5), ival));

	 if ((cval = dba_record_enqk(buoy->context, "ident")) != NULL)
		  DBA_RUN_OR_RETURN(aof_message_store_variable_c(msg, DBA_VAR(0, 1, 5), cval));
	 else
		  DBA_RUN_OR_RETURN(aof_message_store_variable_undef(msg, DBA_VAR(0, 1, 5)));

	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 10, 51), buoy->var_press_msl));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 11, 11), buoy->var_wind_dir));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 11, 12), buoy->var_wind_speed));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 12,  4), buoy->var_temp));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 22, 42), buoy->var_water_temp));
#endif
	 return dba_error_ok();
}

static dba_err aof_copy_to_buoy(dba_msg_buoy buoy, aof_message msg)
{
	 dba_var* vars;
	 int vars_count;

#if 0
	 *** To be redesigned 

	 DBA_RUN_OR_RETURN(aof_message_get_vars(msg, &vars, &vars_count));

	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "lat", vars[1]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "lon", vars[2]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "year", vars[3]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "month", vars[4]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "day", vars[5]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "hour", vars[6]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "min", vars[7]));
	 DBA_RUN_OR_RETURN(dba_setvar(buoy->context, "ident", vars[8]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(buoy->var_press, vars[9]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(buoy->var_wind_dir, vars[10]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(buoy->var_wind_speed, vars[11]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(buoy->var_temp, vars[12]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(buoy->var_water_temp, vars[13]));
#endif

	 return dba_error_ok();
}

static dba_err aof_copy_from_synop(dba_msg_synop synop, aof_message msg)
{
#if 0
	 *** To be redesigned 

	 DBA_RUN_OR_RETURN(dba_message_set_category(msg, 1));
	 DBA_RUN_OR_RETURN(dba_message_set_subcategory(msg, 11));

	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  5,  2), synop->var_latitude));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  6,  2), synop->var_longitude));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  4,  1), synop->var_year));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  4,  2), synop->var_month));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  4,  3), synop->var_day));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  4,  4), synop->var_hour));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0,  4,  5), synop->var_minute));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_undef(msg, DBA_VAR(0, 1, 5)));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 10, 51), synop->var_press_msl));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 11, 11), synop->var_wind_dir));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 11, 12), synop->var_wind_speed));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 12,  4), synop->var_temp));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 12,  6), synop->var_dewpoint));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 10, 63), synop->var_press_tend));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 13,  3), synop->var_humidity));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 20,  1), synop->var_visibility));

	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 20,  3), synop->var_pres_wtr));
	 DBA_RUN_OR_RETURN(aof_message_store_variable_var(msg, DBA_VAR(0, 20,  4), synop->var_past_wtr1));
#endif

	 return dba_error_ok();
}

static dba_err aof_copy_to_synop(dba_msg_synop synop, aof_message msg)
{
	 dba_var* vars;
	 int vars_count;

#if 0
	 *** To be redesigned 

	 DBA_RUN_OR_RETURN(aof_message_get_vars(msg, &vars, &vars_count));

	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_latitude, vars[1]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_longitude, vars[2]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_year, vars[3]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_month, vars[4]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_day, vars[5]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_hour, vars[6]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_minute, vars[7]));
	 //DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_ident, vars[8]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_press, vars[9]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_wind_dir, vars[10]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_wind_speed, vars[11]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_temp, vars[12]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_dewpoint, vars[13]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_press_tend, vars[14]));
	 //DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_water_temp, vars[15]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_humidity, vars[16]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_visibility, vars[17]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_pres_wtr, vars[18]));
	 DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_past_wtr1, vars[19]));
#endif

	 return dba_error_ok();
}

dba_err aof_from_buoy(dba_msg_buoy buoy, aof_message* aof)
{
	 dba_err err;
	 DBA_RUN_OR_RETURN(aof_message_create(aof));
	 DBA_RUN_OR_GOTO(fail, aof_copy_from_buoy(buoy, (aof_message)*aof));
	 return dba_error_ok();
fail:
	 aof_message_delete(*aof);
	 *aof = NULL;
	 return err;
}

dba_err aof_to_buoy(dba_msg_buoy* buoy, aof_message aof)
{
   dba_err err;
   DBA_RUN_OR_RETURN(dba_msg_buoy_create(buoy));
   DBA_RUN_OR_GOTO(fail, aof_copy_to_buoy(*buoy, (aof_message)aof));
   return dba_error_ok();
fail:
   dba_msg_buoy_delete(*buoy);
   *buoy = NULL;
   return err;
}

dba_err aof_from_synop(dba_msg_synop synop, aof_message* aof)
{
	 dba_err err;
	 DBA_RUN_OR_RETURN(aof_message_create(aof));
	 DBA_RUN_OR_GOTO(fail, aof_copy_from_synop(synop, (aof_message)*aof));
	 return dba_error_ok();
fail:
	 aof_message_delete(*aof);
	 *aof = NULL;
	 return err;
}

dba_err aof_to_synop(dba_msg_synop* synop, aof_message aof)
{
   dba_err err;
   DBA_RUN_OR_RETURN(dba_msg_synop_create(synop));
   DBA_RUN_OR_GOTO(fail, aof_copy_to_synop(*synop, (aof_message)aof));
   return dba_error_ok();
fail:
   dba_msg_synop_delete(*synop);
   *synop = NULL;
   return err;
}


dba_err aof_to_msg(dba_msg* msg, aof_message amsg)
{
	int cat, subcat;

	DBA_RUN_OR_RETURN(dba_message_get_category(amsg, &cat));
	DBA_RUN_OR_RETURN(dba_message_get_subcategory(amsg, &subcat));

	switch (cat * 1000 + subcat)
	{
		case 1011:
		{
			dba_msg_synop synop;
			DBA_RUN_OR_RETURN(aof_to_synop(&synop, amsg));
			*msg = (dba_msg)synop;
		}
		case 4165:
		{
			dba_msg_buoy buoy;
			DBA_RUN_OR_RETURN(aof_to_buoy(&buoy, amsg));
			*msg = (dba_msg)buoy;
		}
		default:
			return dba_error_unimplemented("interpreting AOF message with category %d:%d", cat, subcat);
	}

	return dba_error_ok();
}

dba_err aof_from_msg(dba_msg msg, aof_message* amsg)
{
	switch (msg->type)
	{
		case SYNOP: DBA_RUN_OR_RETURN(aof_from_synop((dba_msg_synop)msg, amsg)); break;
		case BUOY: DBA_RUN_OR_RETURN(aof_from_buoy((dba_msg_buoy)msg, amsg)); break;
		default:
			return dba_error_unimplemented("interpreting message with category %d into an AOF",
					  msg->type);
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=5: */
