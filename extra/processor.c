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

#include "processor.h"

#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/aof_codec.h>

#include <string.h>
#include <stdlib.h>

extern int op_verbose;

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

dba_err match_common(dba_rawmsg rmsg, dba_msgs msgs, struct grep_t* grepdata, int* match)
{
	if (msgs == NULL && grepdata->parsable)
	{
		*match = 0;
		return dba_error_ok();
	}

	if (msgs != NULL && grepdata->unparsable)
	{
		*match = 0;
		return dba_error_ok();
	}

	*match = 1;
	return dba_error_ok();
}

dba_err match_bufrex(dba_rawmsg rmsg, bufrex_msg rm, dba_msgs msgs, struct grep_t* grepdata, int* match)
{
	DBA_RUN_OR_RETURN(match_common(rmsg, msgs, grepdata, match));
	if (*match == 0)
		return dba_error_ok();

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

dba_err match_bufr(dba_rawmsg rmsg, bufrex_msg rm, dba_msgs msgs, struct grep_t* grepdata, int* match)
{
	DBA_RUN_OR_RETURN(match_bufrex(rmsg, rm, msgs, grepdata, match));

	if (*match == 0)
		return dba_error_ok();

	*match = 1;
	return dba_error_ok();
}

dba_err match_crex(dba_rawmsg rmsg, bufrex_msg rm, dba_msgs msgs, struct grep_t* grepdata, int* match)
{
	DBA_RUN_OR_RETURN(match_bufrex(rmsg, rm, msgs, grepdata, match));

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

dba_err match_aof(dba_rawmsg rmsg, dba_msgs msgs, struct grep_t* grepdata, int* match)
{
	int category, subcategory;
	DBA_RUN_OR_RETURN(aof_codec_get_category(rmsg, &category, &subcategory));

	DBA_RUN_OR_RETURN(match_common(rmsg, msgs, grepdata, match));
	if (*match == 0)
		return dba_error_ok();

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

dba_err process_input(
		dba_file file,
		dba_rawmsg rmsg,
		struct grep_t* grepdata,
		action action, void* data)
{
	dba_err err = DBA_OK;
	int print_errors = (grepdata == NULL || !grepdata->unparsable);
	dba_msgs parsed = NULL;
	bufrex_msg br = NULL;
	int match = 1;

	switch (rmsg->encoding)
	{
		case BUFR:
			DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&parsed));
			DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(BUFREX_BUFR, &br));
			
			if ( !(bufrex_msg_decode(br, rmsg) == DBA_OK &&
				   bufrex_msg_to_dba_msgs(br, &parsed) == DBA_OK) && print_errors)
				print_parse_error("BUFR", rmsg);
			if (grepdata != NULL) DBA_RUN_OR_GOTO(cleanup, match_bufr(rmsg, br, parsed, grepdata, &match));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&parsed));
			DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(BUFREX_CREX, &br));
			
			if ( !(bufrex_msg_decode(br, rmsg) == DBA_OK &&
				   bufrex_msg_to_dba_msgs(br, &parsed) == DBA_OK) && print_errors)
				print_parse_error("CREX", rmsg);
			if (grepdata != NULL) DBA_RUN_OR_GOTO(cleanup, match_crex(rmsg, br, parsed, grepdata, &match));
			break;
		case AOF:
			if (aof_codec_decode(rmsg, &parsed) != DBA_OK && print_errors)
				print_parse_error("AOF", rmsg);
			if (grepdata != NULL) DBA_RUN_OR_GOTO(cleanup, match_aof(rmsg, parsed, grepdata, &match));
			break;
	}

	if (!match)
		goto cleanup;

	DBA_RUN_OR_GOTO(cleanup, action(rmsg, br, parsed, data));

cleanup:
	if (parsed != NULL)
		dba_msgs_delete(parsed);
	if (br != NULL)
		bufrex_msg_delete(br);
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
		DBA_RUN_OR_RETURN(dba_file_create(type, name, "r", &file));

		DBA_RUN_OR_RETURN(dba_file_read(file, rmsg, &found));

		while (found)
		{
			++index;

			if (op_verbose)
				fprintf(stderr, "Reading message #%d...\n", index);

			if (grepdata->index[0] == 0 || match_index(index, grepdata->index))
			{
				rmsg->index = index;
				DBA_RUN_OR_RETURN(process_input(file, rmsg, grepdata, action, data));
			}

			DBA_RUN_OR_RETURN(dba_file_read(file, rmsg, &found));
		}

		dba_file_delete(file);
	} while ((name = poptGetArg(optCon)) != NULL);

	dba_rawmsg_delete(rmsg);

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
