#include <dballe/dba_file.h>
#include <dballe/marshal.h>
#include <dballe/io/dba_rawfile.h>
#include <dballe/io/readers.h>
#include <dballe/io/writers.h>
#include <dballe/aof/decoder.h>
#include <dballe/bufrex/bufrex.h>

#include <stdlib.h>

struct _dba_file
{
	dba_encoding type;
	dba_rawfile rawfile;
	dba_file_reader reader;
	dba_file_writer writer;
};

dba_err dba_file_create(dba_file* file, dba_encoding type, const char* name, const char* mode)
{
	dba_err err;
	dba_file res = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (res == NULL)
		return dba_error_alloc("Allocating a new file reader");
	res->type = type;

	DBA_RUN_OR_GOTO(fail, dba_rawfile_create(&(res->rawfile), name, mode));

	switch (type)
	{
		case BUFR:
			DBA_RUN_OR_GOTO(fail, dba_file_reader_create_bufr(&(res->reader), res->rawfile));
			DBA_RUN_OR_GOTO(fail, dba_file_writer_create_bufr(&(res->writer), res->rawfile));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(fail, dba_file_reader_create_crex(&(res->reader), res->rawfile));
			DBA_RUN_OR_GOTO(fail, dba_file_writer_create_crex(&(res->writer), res->rawfile));
			break;
		case AOF:
			DBA_RUN_OR_GOTO(fail, dba_file_reader_create_aof(&(res->reader), res->rawfile));
			DBA_RUN_OR_GOTO(fail, dba_file_writer_create_aof(&(res->writer), res->rawfile));
			break;
	}

	*file = (dba_file)res;
	return dba_error_ok();

fail:
	if (res->rawfile != NULL)
		dba_rawfile_delete(res->rawfile);
	if (res->reader != NULL)
		dba_file_reader_delete(res->reader);
	free(res);
	*file = NULL;
	return err;
}

void dba_file_delete(dba_file file)
{
	if (file->rawfile != NULL)
		dba_rawfile_delete(file->rawfile);
	if (file->reader != NULL)
		dba_file_reader_delete(file->reader);
	free(file);
}

dba_encoding dba_file_get_type(dba_file file)
{
	return file->type;
}

dba_err dba_file_read_raw(dba_file file, dba_rawmsg msg, int* found)
{
	return dba_file_reader_read(file->reader, msg, found);
}

dba_err dba_file_read(dba_file file, dba_msg* msg, int* found)
{
	dba_err err = DBA_OK;
	dba_rawmsg rm = NULL;
	
	DBA_RUN_OR_RETURN(dba_rawmsg_create(&rm));
	DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, rm, found));
	if (*found)
		/* Parse the message */
		DBA_RUN_OR_GOTO(cleanup, dba_marshal_decode(rm, msg));
	else
		*msg = NULL;

cleanup:
	if (rm != NULL)
		dba_rawmsg_delete(rm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_file_write_raw(dba_file file, dba_rawmsg msg)
{
	return dba_file_writer_write_raw(file->writer, msg);
}

dba_err dba_file_write(dba_file file, dba_msg msg, int cat, int subcat)
{
	dba_err err = DBA_OK;
	dba_rawmsg raw = NULL;

	switch (file->type)
	{
		case BUFR:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_bufr(msg, cat, subcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write_raw(file, raw));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_crex(msg, cat, subcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write_raw(file, raw));
			break;
		case AOF: 
			err = dba_error_unimplemented("export to AOF format");
			goto cleanup;
	}

cleanup:
	if (raw != NULL)
		dba_rawmsg_delete(raw);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
