#ifndef CONVERSION_H
#define CONVERSION_H

#include <dballe/dba_file.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/msg/dba_msg.h>


struct conversion_info
{
	dba_encoding outType;
	void* outAction;
	void* outActionData;
};

dba_err convert_bufr_message(dba_rawmsg msg, bufrex_raw braw, dba_msg decoded, void* data);
dba_err convert_crex_message(dba_rawmsg msg, bufrex_raw braw, dba_msg decoded, void* data);
dba_err convert_aof_message(dba_rawmsg msg, dba_msg decoded, void* data);


#endif
