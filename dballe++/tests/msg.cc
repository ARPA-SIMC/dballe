#include <dballe++/msg.h>
#include <dballe/core/test-utils-core.h>

using namespace std;

namespace tut {
using namespace tut_dballe;

struct msg_shar {
};

TESTGRP( msg );

using namespace dballe;

template<> template<>
void to::test<1>()
{
}

}

// vim:set ts=4 sw=4:
