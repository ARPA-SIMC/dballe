#include "processor.h"

#include <dballe/bufrex/bufrex.h>
#include <dballe/aof/aof_decoder.h>

#include <string.h>
#include <stdlib.h>

static int match_index(int idx, const char* expr)
{
	size_t pos;
	size_t len;
	for (pos = 0; (len = strcspn(expr + pos, ",")) > 0; pos += len + 1)
	{
		int start, end;
		int found = sscanf(expr + pos, "%d-%d", &start, &end);
		switch (found)
		{
			case 1:
				if (start == idx)
					return 1;
				break;
			case 2: 
				if (start <= idx && idx <= end)
					return 1;
				break;
			default:
				fprintf(stderr, "Cannot parse index string %s\n", expr);
				return 0;
		}
	}
	return 0;
}

dba_err match_bufrex(dba_rawmsg rmsg, bufrex_raw rm, dba_msg msg, struct grep_t* grepdata, int* match)
{
	if (grepdata->category != -1)
		if (grepdata->category != rm->type)
		{
			*match = 0;
			return dba_error_ok();
		}

	if (grepdata->subcategory != -1)
		if (grepdata->subcategory != rm->subtype)
		{
			*match = 0;
			return dba_error_ok();
		}

	*match = 1;
	return dba_error_ok();
}

dba_err match_bufr(dba_rawmsg rmsg, bufrex_raw rm, dba_msg msg, struct grep_t* grepdata, int* match)
{
	DBA_RUN_OR_RETURN(match_bufrex(rmsg, rm, msg, grepdata, match));

	if (*match == 0)
		return dba_error_ok();

	*match = 1;
	return dba_error_ok();
}

dba_err match_crex(dba_rawmsg rmsg, bufrex_raw rm, dba_msg msg, struct grep_t* grepdata, int* match)
{
	DBA_RUN_OR_RETURN(match_bufrex(rmsg, rm, msg, grepdata, match));

	if (*match == 0)
		return dba_error_ok();

#if 0
	if (grepdata->checkdigit != -1)
	{
		int checkdigit;
		DBA_RUN_OR_RETURN(crex_message_has_check_digit(msg, &checkdigit));
		if (grepdata->checkdigit != checkdigit)
		{
			*match = 0;
			return dba_error_ok();
		}
	}
#endif

	*match = 1;
	return dba_error_ok();
}

dba_err match_aof(dba_rawmsg rmsg, dba_msg msg, struct grep_t* grepdata, int* match)
{
	int category, subcategory;
	DBA_RUN_OR_RETURN(aof_decoder_get_category(rmsg, &category, &subcategory));

	if (grepdata->category != -1)
	{
		if (grepdata->category != category)
		{
			*match = 0;
			return dba_error_ok();
		}
	}

	if (grepdata->subcategory != -1)
	{
		if (grepdata->subcategory != subcategory)
		{
			*match = 0;
			return dba_error_ok();
		}
	}

	*match = 1;
	return dba_error_ok();
}

static void print_parse_error(const char* type, dba_rawmsg msg)
{
	dba_error_info einfo = NULL;
	const char* details;

	/* Save error state, because the dba_rawmsg functions change it */
	dba_error_state_get(&einfo);

	dba_error_state_set(einfo);
	dba_error_state_delete(&einfo);

	fprintf(stderr, "Cannot parse %s message #%d: error %d (%s) while %s",
			type, msg->index, dba_error_get_code(), dba_error_get_message(), dba_error_get_context());
	details = dba_error_get_details();
	if (details != NULL)
		fprintf(stderr, " (details: %s)", details);
	fprintf(stderr, " at offset %d.\n", msg->offset);
}

dba_err process_bufr_input(
		dba_file file,
		dba_rawmsg rmsg,
		struct grep_t* grepdata,
		action action, void* data)
{
	dba_err err = DBA_OK;
	dba_msg parsed = NULL;
	bufrex_raw br = NULL;
	int print_errors = (grepdata == NULL || !grepdata->unparsable);
	int match;
	dba_err parse_result;

	DBA_RUN_OR_RETURN(bufrex_raw_create(&br, BUFREX_BUFR));
	
	parse_result = bufrex_raw_decode(br, rmsg);
	if (parse_result != DBA_OK && print_errors)
		print_parse_error("BUFR", rmsg);
	parse_result = bufrex_raw_to_msg(br, &parsed);
	if (parse_result != DBA_OK && print_errors)
		print_parse_error("BUFR", rmsg);

	if (parse_result == DBA_OK || (grepdata != NULL && grepdata->unparsable))
	{
		if (grepdata)
			DBA_RUN_OR_GOTO(cleanup, match_bufr(rmsg, br, parsed, grepdata, &match));
		else
			match = 1;

		if (match)
			DBA_RUN_OR_GOTO(cleanup, action(rmsg, br, parsed, data));
	} else
		err = parse_result;
	
cleanup:
	if (parsed != NULL)
		dba_msg_delete(parsed);
	if (br != NULL)
		bufrex_raw_delete(br);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err process_crex_input(
		dba_file file,
		dba_rawmsg rmsg,
		struct grep_t* grepdata,
		action action,
		void* data)
{
	dba_err err = DBA_OK;
	dba_msg parsed = NULL;
	bufrex_raw br = NULL;
	int print_errors = (grepdata == NULL || !grepdata->unparsable);
	int match;
	dba_err parse_result;

	DBA_RUN_OR_RETURN(bufrex_raw_create(&br, BUFREX_CREX));
	
	parse_result = bufrex_raw_decode(br, rmsg);
	if (parse_result != DBA_OK && print_errors)
		print_parse_error("CREX", rmsg);
	parse_result = bufrex_raw_to_msg(br, &parsed);
	if (parse_result != DBA_OK && print_errors)
		print_parse_error("CREX", rmsg);

	if (parse_result == DBA_OK || (grepdata != NULL && grepdata->unparsable))
	{
		if (grepdata)
			DBA_RUN_OR_RETURN(match_crex(rmsg, br, parsed, grepdata, &match));
		else
			match = 1;

		if (match)
			DBA_RUN_OR_GOTO(cleanup, action(rmsg, br, parsed, data));
	} else
		err = parse_result;

cleanup:
	if (parsed != NULL)
		dba_msg_delete(parsed);
	if (br != NULL)
		bufrex_raw_delete(br);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err process_aof_input(
		dba_file file,
		dba_rawmsg rmsg,
		struct grep_t* grepdata,
		action action,
		void* data)
{
	dba_err err = DBA_OK;
	int print_errors = (grepdata == NULL || !grepdata->unparsable);
	int match;
	dba_msg parsed = NULL;
	dba_err parse_result = aof_decoder_decode(rmsg, &parsed);

	if (parse_result != DBA_OK && print_errors)
		print_parse_error("AOF", rmsg);

	if (parse_result == DBA_OK || (grepdata != NULL && grepdata->unparsable))
	{
		if (grepdata)
			DBA_RUN_OR_GOTO(cleanup, match_aof(rmsg, parsed, grepdata, &match));
		else
			match = 1;

		if (match)
			DBA_RUN_OR_GOTO(cleanup, action(rmsg, NULL, parsed, data));
	} else
		err = parse_result;

cleanup:
	if (parsed != NULL)
		dba_msg_delete(parsed);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err process_all(
		poptContext optCon,
		dba_encoding type,
		struct grep_t* grepdata,
		action action, void* data)
{
	const char* name = poptGetArg(optCon);
	dba_rawmsg rmsg;
	int index = 0;

	if (name == NULL)
		name = "(stdin)";

	DBA_RUN_OR_RETURN(dba_rawmsg_create(&rmsg));

	do
	{
		dba_file file;
		int found;
		DBA_RUN_OR_RETURN(dba_file_create(&file, type, name, "r"));

		DBA_RUN_OR_RETURN(dba_file_read_raw(file, rmsg, &found));

		while (found)
		{
			++index;

			if (grepdata->index[0] == 0 || match_index(index, grepdata->index))
			{
				rmsg->index = index;
				switch (type)
				{
					case CREX:
						DBA_RUN_OR_RETURN(process_crex_input(file, rmsg, grepdata, action, data));
						break;
					case BUFR:
						DBA_RUN_OR_RETURN(process_bufr_input(file, rmsg, grepdata, action, data));
						break;
					case AOF:
						DBA_RUN_OR_RETURN(process_aof_input(file, rmsg, grepdata, action, data));
						break;
				}
			}
			DBA_RUN_OR_RETURN(dba_file_read_raw(file, rmsg, &found));
		}

		dba_file_delete(file);
	} while ((name = poptGetArg(optCon)) != NULL);

	dba_rawmsg_delete(rmsg);

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
