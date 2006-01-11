#include <tests/test-utils.h>
#include <dballe/core/dba_var.h>

namespace tut {
using namespace tut_dballe;

struct dba_core_var_shar
{
	dba_core_var_shar()
	{
	}

	~dba_core_var_shar()
	{
	}
};
TESTGRP(dba_core_var);


// Test variable creation
template<> template<>
void to::test<1>()
{
	dba_var var;
	dba_varinfo info;

	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 6, 1), &info));

	CHECKED(dba_var_create(info, &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure_equals(dba_var_value(var), (const char*)0);
	dba_var_delete(var);

	CHECKED(dba_var_createi(info, &var, 123));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure(dba_var_value(var) != 0);
	gen_ensure_var_equals(var, 123);
	dba_var_delete(var);

	CHECKED(dba_var_created(info, &var, 123.456));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure(dba_var_value(var) != 0);
	gen_ensure_var_equals(var, 123.456);
	dba_var_delete(var);

	CHECKED(dba_var_createc(info, &var, "123"));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure(dba_var_value(var) != 0);
	gen_ensure_var_equals(var, "123");
	dba_var_delete(var);

	CHECKED(dba_var_create_local(DBA_VAR(0, 6, 1), &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_value(var), (const char*)0);
	dba_var_delete(var);
}	

// Get and set values
template<> template<>
void to::test<2>()
{

}

// Test variable copy
template<> template<>
void to::test<3>()
{
	dba_var var = NULL, var1 = NULL, attr = NULL;
	
	CHECKED(dba_var_create_local(DBA_VAR(0, 6, 1), &var));
	gen_ensure(var != NULL);
	CHECKED(dba_var_seti(var, 234));

	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
	gen_ensure(var != NULL);
	CHECKED(dba_var_seti(attr, 75));
	CHECKED(dba_var_seta_nocopy(var, attr));

	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 2), &attr));
	gen_ensure(var != NULL);
	CHECKED(dba_var_seti(attr, 45));
	CHECKED(dba_var_seta_nocopy(var, attr));

	CHECKED(dba_var_enqa(var, DBA_VAR(0, 33, 7), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 75);

	CHECKED(dba_var_enqa(var, DBA_VAR(0, 33, 2), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 45);


	CHECKED(dba_var_copy(var, &var1));
	gen_ensure(var1 != NULL);
	gen_ensure_var_equals(var1, 234);

	CHECKED(dba_var_enqa(var1, DBA_VAR(0, 33, 7), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 75);

	CHECKED(dba_var_enqa(var1, DBA_VAR(0, 33, 2), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 45);


	dba_var_delete(var);
	dba_var_delete(var1);
}
	
}

/* vim:set ts=4 sw=4: */
