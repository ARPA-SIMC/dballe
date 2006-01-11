#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <dballe/dba_file.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/msg/dba_msg.h>
#include <popt.h>

struct grep_t
{
	int category;
	int subcategory;
	int checkdigit;
	int unparsable;
	const char* index;
};

typedef dba_err (*bufrex_action)(dba_rawmsg msg, bufrex_raw braw, dba_msg decoded, void* data);
typedef dba_err (*aof_action)(dba_rawmsg msg, dba_msg decoded, void* data);
/*
typedef dba_err (*crex_action)(crex_message msg, int idx, void* data);
typedef dba_err (*bufr_action)(bufr_message msg, int idx, void* data);
typedef dba_err (*aof_action)(aof_message msg, int idx, dba_msg decoded, void* data);
*/

dba_err process_all(
		poptContext optCon,
		dba_encoding type,
		struct grep_t* grepdata,
		void* action, void* data);


#endif
