#include <dballe/msg/dba_msg.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <math.h>

static inline dba_var get(bufrex_raw raw, int idx, dba_varcode code)
{
	dba_var res = raw->vars[idx];
	if (dba_var_code(res) != code)
		return NULL;
	if (dba_var_value(res) == NULL)
		return NULL;
	return res;
}

#define GET(idx1, code) \
	(var = get(raw, idx1 - 1, DBA_STRING_TO_VAR(code + 1))) != NULL

dba_err bufrex_copy_to_pilot(dba_msg msg, bufrex_raw raw)
{
	int i;
	dba_var var;

	msg->type = MSG_PILOT;

	if (GET( 1, "B01001")) DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg, var));
	if (GET( 2, "B01002")) DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg, var));
	if (GET( 3, "B02011")) DBA_RUN_OR_RETURN(dba_msg_set_sonde_type_var(msg, var));
	if (GET( 4, "B02012")) DBA_RUN_OR_RETURN(dba_msg_set_sonde_method_var(msg, var));
	if (GET( 5, "B04001")) DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var));
	if (GET( 6, "B04002")) DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var));
	if (GET( 7, "B04003")) DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var));
	if (GET( 8, "B04004")) DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var));
	if (GET( 9, "B04005")) DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var));
	if (GET(10, "B05001")) DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var));
	if (GET(11, "B06001")) DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var));
	if (GET(12, "B07001")) DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var));

	for (i = 14; i < raw->vars_count && dba_var_code(raw->vars[i-1]) == DBA_VAR(0, 7, 4); i += 5)
	{
		int ltype = -1, l1 = -1;

		if (GET(i, "B07004"))
		{
			double press;
			DBA_RUN_OR_RETURN(dba_var_enqd(var, &press));
			ltype = 100;
			l1 = press / 100;
			DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 4), ltype, l1, 0, 0, 0, 0));
		}
		if (GET(i + 2, "B10003"))
		{
			double geopot;
			DBA_RUN_OR_RETURN(dba_var_enqd(var, &geopot));
			if (ltype == -1)
			{
				ltype = 103;
				l1 = lround((double)geopot / 9.80665);
			}
			DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 3), ltype, l1, 0, 0, 0, 0));
		}
		if (ltype == -1)
			return dba_error_notfound("looking for pressure or height in a BUFR/CREX PILOT message");

		if (GET(i+1, "B08001")) DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0,  8, 1), ltype, l1, 0, 0, 0, 0));
		if (GET(i+3, "B11001")) DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 1), ltype, l1, 0, 0, 0, 0));
		if (GET(i+4, "B11002")) DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 2), ltype, l1, 0, 0, 0, 0));
	}

	return dba_error_ok();
}
/* vim:set ts=4 sw=4: */
