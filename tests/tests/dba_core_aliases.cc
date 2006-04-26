#include <tests/test-utils.h>
#include <dballe/core/aliases.h>

namespace tut {
using namespace tut_dballe;

struct dba_core_aliases_shar
{
	dba_core_aliases_shar()
	{
	}

	~dba_core_aliases_shar()
	{
	}
};
TESTGRP(dba_core_aliases);


// Test variable creation
template<> template<>
void to::test<1>()
{
	gen_ensure_equals(dba_varcode_alias_resolve("block"), DBA_VAR(0, 1, 1));
	gen_ensure_equals(dba_varcode_alias_resolve("station"), DBA_VAR(0, 1,  2));
	gen_ensure_equals(dba_varcode_alias_resolve("height"), DBA_VAR(0, 7,  1));
	gen_ensure_equals(dba_varcode_alias_resolve("heightbaro"), DBA_VAR(0, 7, 31));
	gen_ensure_equals(dba_varcode_alias_resolve("name"), DBA_VAR(0, 1, 19));
	gen_ensure_equals(dba_varcode_alias_resolve("cippolippo"), 0);
}
	
}

/* vim:set ts=4 sw=4: */
