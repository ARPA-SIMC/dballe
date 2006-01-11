#include <dballe/dba_marshal.h>

#include <dballe/aof/aof_decoder.h>
#include <dballe/bufrex/bufrex.h>

dba_err dba_marshal_decode(dba_rawmsg rmsg, dba_msg *msg)
{
	switch (rmsg->encoding)
	{
		case BUFR: DBA_RUN_OR_RETURN(bufrex_decode_bufr(rmsg, msg)); break;
		case CREX: DBA_RUN_OR_RETURN(bufrex_decode_crex(rmsg, msg)); break;
		case AOF: DBA_RUN_OR_RETURN(aof_decoder_decode(rmsg, msg)); break;
	}
	return dba_error_ok();
}

dba_err dba_marshal_encode(dba_msg msg, dba_encoding type, dba_rawmsg *rmsg)
{
	switch (type)
	{
		case BUFR: DBA_RUN_OR_RETURN(bufrex_encode_bufr(msg, 0, 0, rmsg)); break;
		case CREX: DBA_RUN_OR_RETURN(bufrex_encode_crex(msg, 0, 0, rmsg)); break;
		case AOF: return dba_error_unimplemented("exporting to AOF"); break;
	}
	return dba_error_ok();
}


/* vim:set ts=4 sw=4: */
