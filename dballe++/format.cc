#include <dballe++/var.h>

#include <dballe/msg/formatter.h>

using namespace std;

namespace dballe {

std::string describeLevel(int ltype, int l1, int l2)
{
	char* desc;

	checked(dba_formatter_describe_level(ltype, l1, l2, &desc));

	std::string res(desc);
	free(desc);
	return res;
}

std::string describeTrange(int ptype, int p1, int p2)
{
	char* desc;

	checked(dba_formatter_describe_trange(ptype, p1, p2, &desc));

	std::string res(desc);
	free(desc);
	return res;
}

}

#ifdef DBALLEPP_COMPILE_TESTSUITE

#include <tests/test-utils.h>

namespace tut {

struct format_shar {
};

TESTGRP( format );

using namespace dballe;

template<> template<>
void to::test<1>()
{
	for (int i = 0; i < 258; ++i)
		gen_ensure(!describeLevel(i, 0, 0).empty());
}

template<> template<>
void to::test<2>()
{
	for (int i = 0; i < 256; ++i)
		gen_ensure(!describeTrange(i, 0, 0).empty());
}

}

#endif
