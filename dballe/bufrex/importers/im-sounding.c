#include <dballe/msg/dba_msg.h>
#include <dballe/bufrex/bufrex_raw.h>

dba_err bufrex_copy_to_sounding(dba_msg msg, bufrex_raw raw)
{
	int i;
	int cloud_type_count = 0;
	double press = -1;

	switch (raw->subtype)
	{
		case 0:
			/* Guess looking at the variables */
			if (raw->vars_count > 1 && dba_var_code(raw->vars[0]) == DBA_VAR(0, 1, 11))
				msg->type = MSG_TEMP_SHIP;
			else if (raw->vars_count > 13 && dba_var_code(raw->vars[12]) == DBA_VAR(0, 31, 1))
				msg->type = MSG_PILOT;
			else
				msg->type = MSG_TEMP;
			break;
		case 91: msg->type = MSG_PILOT; break;
		case 101: msg->type = MSG_TEMP; break;
		case 92:
		case 102: msg->type = MSG_TEMP_SHIP; break;
		default: msg->type = MSG_GENERIC; break;
	}

	for (i = 0; i < raw->vars_count; i++)
	{
		dba_var var = raw->vars[i];

		if (dba_var_value(var) == NULL)
			continue;

		switch (dba_var_code(var))
		{
			case DBA_VAR(0,  1,  1): DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg, var)); break;
			case DBA_VAR(0,  1,  2): DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg, var)); break;
			case DBA_VAR(0,  1, 11): DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg, var)); break;
			case DBA_VAR(0,  1, 12): DBA_RUN_OR_RETURN(dba_msg_set_st_dir_var(msg, var)); break;
			case DBA_VAR(0,  1, 13): DBA_RUN_OR_RETURN(dba_msg_set_st_speed_var(msg, var)); break;
			case DBA_VAR(0,  2, 11): DBA_RUN_OR_RETURN(dba_msg_set_sonde_type_var(msg, var)); break;
			case DBA_VAR(0,  2, 12): DBA_RUN_OR_RETURN(dba_msg_set_sonde_method_var(msg, var)); break;
			case DBA_VAR(0,  4,  1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,  2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,  3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,  4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,  5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  5,  1): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  5,  2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  1): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  7,  1): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;

			case DBA_VAR(0, 20, 10): DBA_RUN_OR_RETURN(dba_msg_set_cloud_n_var(msg, var)); break;
			case DBA_VAR(0, 20, 11): DBA_RUN_OR_RETURN(dba_msg_set_cloud_nh_var(msg, var)); break;
			case DBA_VAR(0, 20, 13): DBA_RUN_OR_RETURN(dba_msg_set_cloud_hh_var(msg, var)); break;
			case DBA_VAR(0, 20, 12):
				switch (cloud_type_count++)
				{
					case 0: DBA_RUN_OR_RETURN(dba_msg_set_cloud_cl_var(msg, var)); break;
					case 1: DBA_RUN_OR_RETURN(dba_msg_set_cloud_cm_var(msg, var)); break;
					case 2: DBA_RUN_OR_RETURN(dba_msg_set_cloud_ch_var(msg, var)); break;
				}
				break;
/*
			case DBA_VAR(0, 31,  1):
		    {
				int size;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &size));
				DBA_RUN_OR_RETURN(dba_msg_sounding_resize_obs(so, size));
				obs = -1;
				break;
			}
*/
			case DBA_VAR(0,  7,  4):
				DBA_RUN_OR_RETURN(dba_var_enqd(var, &press));
				break;
			case DBA_VAR(0,  8,  1): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0,  8, 1), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 10,  3): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 3), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 12,  1): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 1), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 12,  2): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 2), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 12,  3): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 3), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 11,  1): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 1), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 11,  2): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 2), 100, press, 0, 0, 0, 0)); break;
			/*
			default:
				fprintf(stderr, "Unhandled variable: "); dba_var_print(var, stderr);
				break;
			*/
		}
	}

	return dba_error_ok();
}
/* vim:set ts=4 sw=4: */
