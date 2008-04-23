#include <dballe++/var.h>

#include <dballe/msg/formatter.h>

#include <cstdlib>

using namespace std;

namespace dballe {

std::string describeLevel(int ltype1, int l1, int ltype2, int l2)
{
	char* desc;

	checked(dba_formatter_describe_level_or_layer(ltype1, l1, ltype2, l2, &desc));

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

// vim:set ts=4 sw=4:
