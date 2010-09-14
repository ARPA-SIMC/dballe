/*
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

#include <test-utils-wreport.h>
#include <wreport/dtable.h>

using namespace wreport;
using namespace std;

namespace tut {

struct dtable_shar
{
	dtable_shar()
	{
	}

	~dtable_shar()
	{
	}
};
TESTGRP(dtable);

// Test basic queries
template<> template<>
void to::test<1>()
{
	const char* testdatadir = getenv("WREPORT_TESTDATA");
	tests::LocalEnv le("WREPORT_TABLES", testdatadir ? testdatadir : ".");

	const DTable* table = DTable::get("test-crex-d-table");

	/* Try querying a nonexisting item */
	try {
		table->query(WR_VAR(3, 0, 9));
	} catch (error_notfound& e) {
		ensure_contains(e.what(), "300009");
	}

	/* Query the first item */
	Opcodes chain = table->query(WR_VAR(3, 0, 2));
	ensure_equals(chain.size(), 2u);
	ensure_equals(chain.head(), WR_VAR(0, 0, 2));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 0, 3));
	chain = chain.next();
	ensure_equals(chain.head(), 0);
	ensure_equals(chain.size(), 0);

	/* Now query an existing item */
	chain = table->query(WR_VAR(3, 35, 6));
	ensure_equals(chain.size(), 7u);

	ensure_equals(chain.head(), WR_VAR(0, 8, 21));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 4, 4));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 8, 21));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 4, 4));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 35, 0));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 1, 3));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(0, 35, 11));
	chain = chain.next();
	ensure_equals(chain.head(), 0);
	chain = chain.next();
	ensure_equals(chain.head(), 0);
	ensure_equals(chain.size(), 0);

	/* Then query the last item */
	chain = table->query(WR_VAR(3, 35, 10));
	ensure_equals(chain.size(), 3u);

	ensure_equals(chain.head(), WR_VAR(3, 35, 2));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(3, 35, 3));
	chain = chain.next();
	ensure_equals(chain.head(), WR_VAR(3, 35, 7));
	chain = chain.next();
	ensure_equals(chain.head(), 0);
	ensure_equals(chain.size(), 0);
}

// Try reading a BUFR edition 4 table
template<> template<>
void to::test<2>()
{
	const DTable* table = DTable::get("D0000000000098013102");

	/* Try querying a nonexisting item */
	try {
		table->query(WR_VAR(3, 0, 9));
	} catch (error_notfound& e) {
		ensure_contains(e.what(), "300009");
	}

	/* Now query an existing item */
	Opcodes chain = table->query(WR_VAR(3, 1, 24));

	ensure_varcode_equals(chain.head(), WR_VAR(0, 5, 2)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 6, 2)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 7, 1)); chain = chain.next();
	ensure_varcode_equals(chain.head(), 0); chain = chain.next();
	ensure_varcode_equals(chain.head(), 0);
	ensure_equals(chain.size(), 0);
	/*fprintf(stderr, "VAL: %d %02d %03d\n", WR_VAR_F(cur->val), WR_VAR_X(cur->val), WR_VAR_Y(cur->val));*/

	/* Then query the last item */
	chain = table->query(WR_VAR(3, 21, 28));
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 118)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(2,  2, 129)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(2,  1, 132)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0,  2, 112)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(2,  1,   0)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(2,  1, 131)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0,  2, 111)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(2,  1,   0)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(2,  2,   0)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0,  2, 104)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 123)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 106)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 107)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 114)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 115)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 116)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0,  8,  18)); chain = chain.next();
	ensure_varcode_equals(chain.head(), WR_VAR(0, 21, 117)); chain = chain.next();
	ensure_varcode_equals(chain.head(), 0); chain = chain.next();
	ensure_varcode_equals(chain.head(), 0);
	ensure_equals(chain.size(), 0);
}
}

/* vim:set ts=4 sw=4: */
