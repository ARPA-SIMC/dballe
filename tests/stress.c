#include <dballe/dba_file.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex.h>

static void print_dba_error()
{
	const char* details = dba_error_get_details();
	fprintf(stderr, "Error %d (%s) while %s",
			dba_error_get_code(),
			dba_error_get_message(),
			dba_error_get_context());
	if (details == NULL)
		fputc('\n', stderr);
	else
		fprintf(stderr, ".  Details:\n%s\n", details);
}

int main(int argc, const char* argv[])
{
	dba_err err = DBA_OK;
	dba_file filein;
	dba_msg msg;
	int found;
	int idx;

	if (argc > 1)
		DBA_RUN_OR_GOTO(fail, dba_file_create(&filein, CREX, argv[1], "r"));
	else
		DBA_RUN_OR_GOTO(fail, dba_file_create(&filein, CREX, "(stdin)", "r"));

	DBA_RUN_OR_GOTO(fail, dba_file_read(filein, &msg, &found));
	for (idx = 1; found; idx++)
	{
		dba_rawmsg raw;
		dba_msg msg1;
		int diffs = 0;

		DBA_RUN_OR_RETURN(bufrex_encode_bufr(msg, 0, 1, &raw));
		DBA_RUN_OR_GOTO(fail, bufrex_decode_bufr(raw, &msg));
		dba_rawmsg_delete(raw);

		dba_msg_diff(msg, msg1, &diffs, stderr);
		if (diffs != 0)
			fprintf(stderr, "#%d: %d differences found\n", idx, diffs);

		dba_msg_delete(msg);
		dba_msg_delete(msg1);

		DBA_RUN_OR_GOTO(fail, dba_file_read(filein, &msg, &found));
	}

	dba_file_delete(filein);

	return 0;

fail:
	print_dba_error();
	return 1;
}

/* vim:set ts=4 sw=4: */
