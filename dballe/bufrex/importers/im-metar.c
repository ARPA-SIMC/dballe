#include <dballe/msg/dba_msg.h>
#include <dballe/bufrex/bufrex_raw.h>

dba_err bufrex_copy_to_metar(dba_msg msg, bufrex_raw raw)
{
	int i;
	int height_above_count = 0;
	int height = -1;

	msg->type = MSG_METAR;
	
	for (i = 0; i < raw->vars_count; i++)
	{
		dba_var var = raw->vars[i];

		if (dba_var_value(var) == NULL)
			continue;
		switch (dba_var_code(var))
		{
			case DBA_VAR(0,  1, 63): DBA_RUN_OR_RETURN(dba_msg_set_st_name_icao_var(msg, var)); break;
			case DBA_VAR(0,  2,  1): DBA_RUN_OR_RETURN(dba_msg_set_st_type_var(msg, var)); break;
			case DBA_VAR(0,  4,  1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,  2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,  3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,  4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,  5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  5,  1):
			case DBA_VAR(0,  5,  2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  1):
			case DBA_VAR(0,  6,  2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  7,  1): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			case DBA_VAR(0,  7,  6):
				switch (height_above_count++)
				{
					case 0:
						DBA_RUN_OR_RETURN(dba_var_enqi(var, &height));
						if (height != 10)
							return dba_error_consistency("checking that wind level is 10M");
						break;
					case 1:
						DBA_RUN_OR_RETURN(dba_var_enqi(var, &height));
						if (height != 2)
							return dba_error_consistency("checking that temperature level is 2M");
						break;
				}
				break;
			case DBA_VAR(0, 11,  1): DBA_RUN_OR_RETURN(dba_msg_set_wind_dir_var(msg, var)); break;
			case DBA_VAR(0, 11, 16): DBA_RUN_OR_RETURN(dba_msg_set_ex_ccw_wind_var(msg, var)); break;
			case DBA_VAR(0, 11, 17): DBA_RUN_OR_RETURN(dba_msg_set_ex_cw_wind_var(msg, var)); break;
			case DBA_VAR(0, 11,  2): DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, var)); break;
			case DBA_VAR(0, 11, 41): DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, var)); break;
			case DBA_VAR(0, 12,  1): DBA_RUN_OR_RETURN(dba_msg_set_temp_2m_var(msg, var)); break;
			case DBA_VAR(0, 12,  3): DBA_RUN_OR_RETURN(dba_msg_set_dewpoint_2m_var(msg, var)); break;
			case DBA_VAR(0, 10, 52): DBA_RUN_OR_RETURN(dba_msg_set_qnh_var(msg, var)); break;
			case DBA_VAR(0, 20,  9): DBA_RUN_OR_RETURN(dba_msg_set_metar_wtr_var(msg, var)); break;
		}
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
