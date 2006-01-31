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
	int parsable;
	const char* index;
};

typedef dba_err (*action)(dba_rawmsg msg, bufrex_raw braw, dba_msg decoded, void* data);

dba_err process_all(
		poptContext optCon,
		dba_encoding type,
		struct grep_t* grepdata,
		action action, void* data);


#endif
