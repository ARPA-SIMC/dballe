/*
 * wreport/test-utils-wreport - Unit test utilities
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "test-utils-wreport.h"

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace std;

namespace wreport {
namespace tests {

std::string datafile(const std::string& fname)
{
	const char* testdatadirenv = getenv("WREPORT_TESTDATA");
	std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
	return testdatadir + "/" + fname;
}

std::string slurpfile(const std::string& name)
{
	string fname = datafile(name);
	string res;

	FILE* fd = fopen(fname.c_str(), "rb");
	if (fd == NULL)
		error_system::throwf("opening %s", fname.c_str());
		
	/* Read the entire file contents */
	while (!feof(fd))
	{
		char c;
		if (fread(&c, 1, 1, fd) == 1)
			res += c;
	}

	fclose(fd);

	return res;
}

void _ensure_varcode_equals(const wibble::tests::Location& loc, Varcode actual, Varcode expected)
{
	if( expected != actual )
	{
		char buf[40];
		snprintf(buf, 40, "expected %01d%02d%03d actual %01d%02d%03d",
				WR_VAR_F(expected), WR_VAR_X(expected), WR_VAR_Y(expected),
				WR_VAR_F(actual), WR_VAR_X(actual), WR_VAR_Y(actual));
		throw tut::failure(loc.msg(buf));
	}
}

void _ensure_var_undef(const wibble::tests::Location& loc, const Var& var)
{
	inner_ensure_equals(var.value(), (const char*)0);
}
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, int val)
{
	inner_ensure_equals(var.enqi(), val);
}
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, double val)
{
	inner_ensure_equals(var.enqd(), val);
}
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, const string& val)
{
	inner_ensure_equals(string(var.enqc()), val);
}

} // namespace tests
} // namespace wreport

// vim:set ts=4 sw=4:
