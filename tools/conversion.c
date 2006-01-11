#include "conversion.h"
#include "processor.h"

#include <dballe/bufrex/bufrex.h>
/* #include <dballe/aof/aof_encoder.h> */
#include <dballe/dba_file.h>

static dba_err process_dba_msg(dba_msg msg, dba_file file, int type, int subtype)
{
	dba_encoding ftype = dba_file_get_type(file);

	switch (ftype)
	{
		case BUFR:
		{
			dba_rawmsg raw;
			DBA_RUN_OR_RETURN(bufrex_encode_bufr(msg, type, subtype, &raw));
			DBA_RUN_OR_RETURN(dba_file_write_raw(file, raw));
			dba_rawmsg_delete(raw);
			break;
		}
		case CREX:
		{
			dba_rawmsg raw;
			DBA_RUN_OR_RETURN(bufrex_encode_crex(msg, type, subtype, &raw));
			DBA_RUN_OR_RETURN(dba_file_write_raw(file, raw));
			dba_rawmsg_delete(raw);
			break;
		}
		case AOF:
		{
			return dba_error_unimplemented("encoding of AOF messages");
			/*
			dba_rawmsg rmsg;
			DBA_RUN_OR_RETURN(dba_rawmsg_create(&rmsg));
			DBA_RUN_OR_RETURN(aof_encoder_encode(m, rmsg));
			DBA_RUN_OR_RETURN(((bufrex_action)data->outAction)(rmsg, m, data->outActionData));
			dba_rawmsg_delete(rmsg);
			break;
			*/
		}
	}
	return dba_error_ok();
}

dba_err convert_bufr_message(dba_rawmsg msg, bufrex_raw braw, dba_msg decoded, void* data)
{
	dba_file file = (dba_file)data;

	DBA_RUN_OR_RETURN(process_dba_msg(decoded, file, braw->subtype == 0 ? 0 : braw->type, braw->subtype));

	return dba_error_ok();
}

dba_err convert_crex_message(dba_rawmsg msg, bufrex_raw braw, dba_msg decoded, void* data)
{
	dba_file file = (dba_file)data;

	DBA_RUN_OR_RETURN(process_dba_msg(decoded, file, braw->subtype == 0 ? 0 : braw->type, braw->subtype));

	return dba_error_ok();
}

dba_err convert_aof_message(dba_rawmsg msg, dba_msg decoded, void* data)
{
	dba_file file = (dba_file)data;

	DBA_RUN_OR_RETURN(process_dba_msg(decoded, file, 0, 0));

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
