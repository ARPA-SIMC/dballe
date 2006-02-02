#include "config.h"
#include <dballe/bufrex/bufrex_raw.h>
#include "exporters/exporters.h"

extern dba_err bufrex_copy_to_generic(dba_msg msg, bufrex_raw raw);
extern dba_err bufrex_copy_to_synop(dba_msg msg, bufrex_raw raw);
extern dba_err bufrex_copy_to_metar(dba_msg msg, bufrex_raw raw);
extern dba_err bufrex_copy_to_temp(dba_msg msg, bufrex_raw raw);
extern dba_err bufrex_copy_to_pilot(dba_msg msg, bufrex_raw raw);
extern dba_err bufrex_copy_to_flight(dba_msg msg, bufrex_raw raw);

extern bufrex_exporter bufrex_exporter_generic;
extern bufrex_exporter bufrex_exporter_synop_0_1;
extern bufrex_exporter bufrex_exporter_synop_0_3;
extern bufrex_exporter bufrex_exporter_sea_1_9;
extern bufrex_exporter bufrex_exporter_sea_1_11;
extern bufrex_exporter bufrex_exporter_sea_1_13;
extern bufrex_exporter bufrex_exporter_sea_1_19;
extern bufrex_exporter bufrex_exporter_sea_1_21;
extern bufrex_exporter bufrex_exporter_pilot_2_91;
extern bufrex_exporter bufrex_exporter_temp_2_101;
extern bufrex_exporter bufrex_exporter_temp_2_102;
extern bufrex_exporter bufrex_exporter_flight_4_142;
extern bufrex_exporter bufrex_exporter_flight_4_144;
extern bufrex_exporter bufrex_exporter_acars_4_145;
extern bufrex_exporter bufrex_exporter_metar_0_140;

static bufrex_exporter* exporters[] = {
	&bufrex_exporter_generic,
	&bufrex_exporter_synop_0_1,
	&bufrex_exporter_synop_0_3,
	&bufrex_exporter_sea_1_9,
	&bufrex_exporter_sea_1_11,
	&bufrex_exporter_sea_1_13,
	&bufrex_exporter_sea_1_19,
	&bufrex_exporter_sea_1_21,
	&bufrex_exporter_pilot_2_91,
	&bufrex_exporter_temp_2_101,
	&bufrex_exporter_temp_2_102,
	&bufrex_exporter_flight_4_142,
	&bufrex_exporter_flight_4_144,
	&bufrex_exporter_acars_4_145,
	&bufrex_exporter_metar_0_140,
	0
};

dba_err bufrex_infer_type_subtype(dba_msg msg, int* type, int* subtype)
{
	bufrex_exporter* exp = NULL;
	switch (msg->type)
	{
		case MSG_GENERIC:	exp = &bufrex_exporter_generic;			break;
		case MSG_SYNOP: {
			dba_var var = dba_msg_get_st_type_var(msg);
			if (var == NULL)
				exp = &bufrex_exporter_synop_0_1;
			else if (dba_var_value(var)[0] == '1')
				exp = &bufrex_exporter_synop_0_1;
			else
				exp = &bufrex_exporter_synop_0_3;
			break;
		}
		case MSG_PILOT:		exp = &bufrex_exporter_pilot_2_91;		break;
		case MSG_TEMP:		exp = &bufrex_exporter_temp_2_101;		break;
		case MSG_TEMP_SHIP:	exp = &bufrex_exporter_temp_2_102;		break;
		case MSG_AIREP:		exp = &bufrex_exporter_flight_4_142;	break;
		case MSG_AMDAR:		exp = &bufrex_exporter_flight_4_144;	break;
		case MSG_ACARS:		exp = &bufrex_exporter_acars_4_145;		break;
		case MSG_SHIP: {
			dba_var var = dba_msg_get_st_type_var(msg);
			if (var == NULL)
				exp = &bufrex_exporter_sea_1_11;
			else if (dba_var_value(var)[0] == '1')
				exp = &bufrex_exporter_sea_1_11;
			else
				exp = &bufrex_exporter_sea_1_13;
			break;
		}
		case MSG_BUOY:		exp = &bufrex_exporter_sea_1_21;		break;
		case MSG_METAR:		exp = &bufrex_exporter_metar_0_140;		break;
	}
	*type = exp->type;
	*subtype = exp->subtype;
	return dba_error_ok();
}

static dba_err get_exporter(dba_msg src, int type, int subtype, bufrex_exporter** exp)
{
	int i;
	for (i = 0; exporters[i] != NULL; i++)
	{
		if (exporters[i]->type        == type &&
			exporters[i]->subtype     == subtype)
		{
			*exp = exporters[i];
			/*return exporters[i]->exporter(src, dst);*/
			return dba_error_ok();
		}
	}
	*exp = &bufrex_exporter_generic;
	return dba_error_ok();
	/*
	return dba_error_notfound("Exporter for %s messages to BUFREX type %d subtype %d", 
					dba_msg_type_name(src->type), type, subtype);
	*/
}

dba_err bufrex_raw_to_msg(bufrex_raw raw, dba_msg* msg)
{
	dba_err err;
	dba_msg res;

	DBA_RUN_OR_RETURN(dba_msg_create(&res));

	switch (raw->type)
	{
		case 0:
		case 1:
			if (raw->subtype == 140)
				DBA_RUN_OR_GOTO(failed, bufrex_copy_to_metar(res, raw));
			else
				DBA_RUN_OR_GOTO(failed, bufrex_copy_to_synop(res, raw));
			break;
		case 2:
			if (raw->subtype == 91 || raw->subtype == 92)
				DBA_RUN_OR_GOTO(failed, bufrex_copy_to_pilot(res, raw));
			else
				DBA_RUN_OR_GOTO(failed, bufrex_copy_to_temp(res, raw));
			break;
		case 4: DBA_RUN_OR_GOTO(failed, bufrex_copy_to_flight(res, raw)); break;
		default: DBA_RUN_OR_GOTO(failed, bufrex_copy_to_generic(res, raw)); break;
	}

	*msg = res;

	return dba_error_ok();

failed:
	dba_msg_delete(res);
	*msg = NULL;
	return err;
}

dba_err bufrex_raw_from_msg(bufrex_raw raw, dba_msg msg)
{
	bufrex_exporter* exp;
	int i;

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(get_exporter(msg, raw->type, raw->subtype, &exp));

	/* Init the bufrex_raw data descriptor chain */
	for (i = 0; exp->ddesc[i] != 0; i++)
	{
		/* Skip encoding of attributes for CREX */
		if (raw->encoding_type == BUFREX_CREX && exp->ddesc[i] == DBA_VAR(2, 22, 0))
			break;

		DBA_RUN_OR_RETURN(bufrex_raw_append_datadesc(raw, exp->ddesc[i]));
	}

	/* Fill up the bufrex_raw with variables from msg */
	DBA_RUN_OR_RETURN(exp->exporter(msg, raw, raw->encoding_type == BUFREX_BUFR ? 0 : 1));

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
/* vim:set ts=4 sw=4: */
