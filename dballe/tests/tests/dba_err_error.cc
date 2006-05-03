#include <tests/test-utils.h>
#include <dballe/err/dba_error.h>
#include <string.h> /* strdup */

namespace tut {
using namespace tut_dballe;

struct dba_err_error_shar
{
	dba_err_error_shar()
	{
	}

	~dba_err_error_shar()
	{
	}
};
TESTGRP(dba_err_error);

static void cb_count(void* data)
{
	++(*(int*)data);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
}

// Basic error functions and callbacks
template<> template<>
void to::test<1>()
{
	int allcount = 0;
	int somecount = 0;
	int alloccount = 0;
	
	dba_error_set_callback(DBA_ERR_NONE, cb_count, &allcount);
	dba_error_set_callback(DBA_ERR_NONE, cb_count, &somecount);
	dba_error_set_callback(DBA_ERR_ALLOC, cb_count, &alloccount);

	gen_ensure_equals(dba_error_ok(), DBA_OK);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_NONE);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);

	gen_ensure_equals(dba_error_notfound("booh"), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_NOTFOUND);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);

	gen_ensure_equals(dba_error_type("dummy error message %f", 3.14), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_TYPE);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);

	gen_ensure_equals(dba_error_handles("dummy error message %d", 314), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_HANDLES);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);

	gen_ensure_equals(dba_error_toolong("dummy error message %s", "3.14"), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_TOOLONG);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);

	dba_error_remove_callback(DBA_ERR_NONE, cb_count, &somecount);

	gen_ensure_equals(alloccount, 0);
	gen_ensure_equals(dba_error_alloc("alloc error"), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_ALLOC);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);
	gen_ensure_equals(alloccount, 1);

	gen_ensure_equals(dba_error_system("dummy error message %s", "3.14"), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_SYSTEM);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure(dba_error_get_details() != NULL);

	gen_ensure_equals(dba_error_consistency("dummy error message %s", "3.14"), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_CONSISTENCY);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure_equals(dba_error_get_details(), (const char*)0);

	gen_ensure_equals(dba_error_parse("pippo.txt", 3, "dummy error message"), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_PARSE);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure(dba_error_get_details() != NULL);

	gen_ensure_equals(dba_error_generic0(DBA_ERR_NOTFOUND, strdup("dummy error message"), strdup("this is just a dummy error message")), DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), DBA_ERR_NOTFOUND);
	gen_ensure(dba_error_get_message() != NULL);
	gen_ensure(dba_error_get_context() != NULL);
	gen_ensure(dba_error_get_details() != NULL);

	gen_ensure_equals(allcount, 9);
	gen_ensure_equals(somecount, 4);
	gen_ensure_equals(alloccount, 1);

	dba_error_remove_callback(DBA_ERR_NONE, cb_count, &allcount);
	dba_error_remove_callback(DBA_ERR_ALLOC, cb_count, &alloccount);
}

}

/* vim:set ts=4 sw=4: */
