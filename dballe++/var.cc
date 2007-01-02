#include <dballe++/var.h>

using namespace std;

namespace dballe {

Varinfo Varinfo::create(dba_varcode code)
{
	dba_varinfo info;
	checked(dba_varinfo_query_local(code, &info));
	return Varinfo(info);
}

}

#ifdef DBALLEPP_COMPILE_TESTSUITE

#include <tests/test-utils.h>

namespace tut {

struct var_shar {
};

TESTGRP( var );

using namespace dballe;

template<> template<>
void to::test<1>()
{
	Varinfo info = Varinfo::create(DBA_VAR(0, 1, 1));
	gen_ensure_equals(info.var(), DBA_VAR(0, 1, 1));
	gen_ensure_equals(string(info.desc()), string("WMO BLOCK NUMBER"));
	gen_ensure_equals(string(info.unit()), string("NUMERIC"));
	gen_ensure_equals(info.scale(), 0);
	gen_ensure_equals(info.ref(), 0);
	gen_ensure_equals(info.len(), 2);
	gen_ensure_equals(info.is_string(), false);
}

template<> template<>
void to::test<2>()
{
	Var var(DBA_VAR(0, 1, 1));
	gen_ensure_equals(var.code(), DBA_VAR(0, 1, 1));
	gen_ensure_equals(var.raw(), (const char*)NULL);
	var.set(10);
	gen_ensure_equals(var.enqi(), 10);
}

}

#endif
