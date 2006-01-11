#include <dballe/msg/dba_msg.h>
#include <dballe/conv/dba_conv.h>
#include <dballe/bufrex/bufrex_raw.h>

dba_err bufrex_copy_to_flight(dba_msg msg, bufrex_raw raw)
{
	int i;
	double press = -1;

	switch (raw->subtype)
	{
		case 142: msg->type = MSG_AIREP; break;
		case 144: msg->type = MSG_AMDAR; break;
		case 145: msg->type = MSG_ACARS; break;
		default: msg->type = MSG_GENERIC; break;
	}
	
	for (i = 0; i < raw->vars_count; i++)
	{
		dba_var var = raw->vars[i];

		if (dba_var_value(var) == NULL)
			continue;
		switch (dba_var_code(var))
		{
			case DBA_VAR(0,  1,  6): DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg, var)); break;
			case DBA_VAR(0,  1,  8): DBA_RUN_OR_RETURN(dba_msg_set_flight_reg_no_var(msg, var)); break;
			case DBA_VAR(0,  2, 61): DBA_RUN_OR_RETURN(dba_msg_set_navsys_var(msg, var)); break;
			case DBA_VAR(0,  2, 62): DBA_RUN_OR_RETURN(dba_msg_set_data_relay_var(msg, var)); break;
			case DBA_VAR(0,  2,  2): DBA_RUN_OR_RETURN(dba_msg_set_wind_inst_var(msg, var)); break;
			case DBA_VAR(0,  2,  5): DBA_RUN_OR_RETURN(dba_msg_set_temp_precision_var(msg, var)); break;
			case DBA_VAR(0,  2, 70): DBA_RUN_OR_RETURN(dba_msg_set_latlon_spec_var(msg, var)); break;
			case DBA_VAR(0,  2, 63): DBA_RUN_OR_RETURN(dba_msg_set_flight_roll_var(msg, var)); break;
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
			case DBA_VAR(0,  8,  4): DBA_RUN_OR_RETURN(dba_msg_set_flight_phase_var(msg, var)); break;
			case DBA_VAR(0,  8, 21): DBA_RUN_OR_RETURN(dba_msg_set_timesig_var(msg, var)); break;
			case DBA_VAR(0,  7,  2): {
				DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var));
				DBA_RUN_OR_RETURN(dba_var_enqd(var, &press));
				DBA_RUN_OR_RETURN(dba_convert_icao_to_press(press, &press));
				break;
			}
			case DBA_VAR(0,  7,  4): {
				DBA_RUN_OR_RETURN(dba_var_enqd(var, &press));
				DBA_RUN_OR_RETURN(dba_msg_set_press_var(msg, var));
				break;
			}
			case DBA_VAR(0, 11,  1):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11,  1), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 11,  2):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11,  2), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 11, 31):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 31), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 11, 32):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 32), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 11, 33):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 33), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 11, 34):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 34), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 11, 35):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 35), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 12,  1):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12,  1), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 12,  3):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12,  3), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 13,  3):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13,  3), 100, press, 0, 0, 0, 0));
				break;
			case DBA_VAR(0, 20, 41):
				if (press == -1) return dba_error_consistency("pressure not found in incoming message");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 20, 41), 100, press, 0, 0, 0, 0));
				break;
		}
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
