#include <tests/test-utils.h>
#include <dballe/core/fast.h>

namespace tut {
using namespace tut_dballe;

struct dba_core_shar
{
	dba_core_shar()
	{
	}

	~dba_core_shar()
	{
	}
};
TESTGRP(dba_core);


// Test variable creation
template<> template<>
void to::test<1>()
{
	gen_ensure_equals(string(itoa(1, 3)), string("1"));
	gen_ensure_equals(string(itoa(100, 3)), string("100"));
	gen_ensure_equals(string(itoa(1000, 3)), string("000"));
	gen_ensure_equals(string(itoa(1234567890, 10)), string("1234567890"));
	gen_ensure_equals(string(itoa(45, 2)), string("45"));
	gen_ensure_equals(string(itoa(-1, 2)), string("-1"));
	gen_ensure_equals(string(itoa(-11000000, 7)), string("1000000"));
	gen_ensure_equals(string(itoa(-11000000, 8)), string("-11000000"));
}
	
}

/* vim:set ts=4 sw=4: */
