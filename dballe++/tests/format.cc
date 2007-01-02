#include <dballe++/format.h>
#include <dballe/core/test-utils-core.h>

using namespace std;

namespace tut {
using namespace tut_dballe;

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

// vim:set ts=4 sw=4:
