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

#include "config.h"
#include <dballe/bufrex/bufrex_msg.h>
#include <dballe/bufrex/bufrex_subset.h>
#include <dballe/msg/dba_msgs.h>
#include "exporters/exporters.h"

extern dba_err bufrex_copy_to_generic(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_synop(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_metar(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_temp(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_pilot(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_flight(dba_msg msg, bufrex_msg raw, bufrex_subset sset);

dba_err bufrex_msg_to_dba_msgs(bufrex_msg raw, dba_msgs* msgs)
{
	dba_err err = DBA_OK;
	dba_msgs res = NULL;
	dba_msg msg = NULL;
	int i;

	DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&res));

	for (i = 0; i < raw->subsets_count; ++i)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_msg_create(&msg));

		switch (raw->type)
		{
			case 0:
			case 1:
				if (raw->subtype == 140)
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_metar(msg, raw, raw->subsets[i]));
				else
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_synop(msg, raw, raw->subsets[i]));
				break;
			case 2:
				if (raw->subtype == 91 || raw->subtype == 92)
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_pilot(msg, raw, raw->subsets[i]));
				else
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_temp(msg, raw, raw->subsets[i]));
				break;
			case 4: DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_flight(msg, raw, raw->subsets[i])); break;
			default: DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_generic(msg, raw, raw->subsets[i])); break;
		}

		DBA_RUN_OR_GOTO(cleanup, dba_msgs_append_acquire(res, msg));
		msg = NULL;
	}

	*msgs = res;
	res = NULL;

cleanup:
	if (msg != NULL)
		dba_msg_delete(msg);
	if (res != NULL)
		dba_msgs_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_msg_from_dba_msg(bufrex_msg raw, dba_msg msg)
{
	bufrex_exporter exp;
	bufrex_subset subset;

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(bufrex_get_exporter(msg, raw->type, raw->subtype, &exp));

	/* Init the bufrex_msg data descriptor chain */
	DBA_RUN_OR_RETURN(exp->datadesc(exp, msg, raw));

	/* Import the message into the first subset */
	DBA_RUN_OR_RETURN(bufrex_msg_get_subset(raw, 0, &subset));

	/* Fill up the bufrex_msg with variables from msg */
	DBA_RUN_OR_RETURN(exp->exporter(msg, raw, subset, raw->encoding_type == BUFREX_BUFR ? 0 : 1));

	/* Fill in the nominal datetime informations */
	{
		dba_var var;
		if ((var = dba_msg_get_year_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_year)));
		if ((var = dba_msg_get_month_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_month)));
		if ((var = dba_msg_get_day_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_day)));
		if ((var = dba_msg_get_hour_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_hour)));
		if ((var = dba_msg_get_minute_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_minute)));
	}
	
	return dba_error_ok();
}

dba_err bufrex_msg_from_dba_msgs(bufrex_msg raw, dba_msgs msgs)
{
	bufrex_exporter exp;
	int i;

	if (msgs->len == 0)
		return dba_error_consistency("tried to export an empty dba_msgs");

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(bufrex_get_exporter(msgs->msgs[0], raw->type, raw->subtype, &exp));

	/* Init the bufrex_msg data descriptor chain */
	DBA_RUN_OR_RETURN(exp->datadesc(exp, msgs->msgs[0], raw));

	/* Import each message into its own subset */
	for (i = 0; i < msgs->len; ++i)
	{
		bufrex_subset subset;
		DBA_RUN_OR_RETURN(bufrex_msg_get_subset(raw, i, &subset));

		/* Fill up the bufrex_msg with variables from msg */
		DBA_RUN_OR_RETURN(exp->exporter(msgs->msgs[i], raw, subset, raw->encoding_type == BUFREX_BUFR ? 0 : 1));
	}

	/* Fill in the nominal datetime informations */
	{
		dba_var var;
		if ((var = dba_msg_get_year_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_year)));
		if ((var = dba_msg_get_month_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_month)));
		if ((var = dba_msg_get_day_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_day)));
		if ((var = dba_msg_get_hour_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_hour)));
		if ((var = dba_msg_get_minute_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_minute)));
	}
	
	return dba_error_ok();
}
/* vim:set ts=4 sw=4: */
