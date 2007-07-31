/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-bufrex.h>
#include <dballe/bufrex/dtable.h>

namespace tut {
using namespace tut_dballe;

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
	const char* testdatadir = getenv("DBA_TESTDATA");
	LocalEnv le("DBA_TABLES", testdatadir ? testdatadir : ".");

	bufrex_dtable table;
	bufrex_opcode chain;
	bufrex_opcode cur;

	CHECKED(bufrex_dtable_create("test-crex-d-table", &table));

	/* Try querying a nonexisting item */
	gen_ensure_equals(bufrex_dtable_query(table, DBA_VAR(3, 0, 9), &chain), DBA_ERROR);
	chain = NULL;

	/* Now query an existing item */
	CHECKED(bufrex_dtable_query(table, DBA_VAR(3, 35, 6), &chain));
	gen_ensure(chain != NULL);
	cur = chain;

	/*fprintf(stderr, "VAL: %d %02d %03d\n", DBA_VAR_F(cur->val), DBA_VAR_X(cur->val), DBA_VAR_Y(cur->val));*/
	gen_ensure_equals(cur->val, DBA_VAR(0, 8, 21));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 4, 4));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 8, 21));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 4, 4));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 35, 0));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 1, 3));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 35, 11));

	bufrex_opcode_delete(&chain);
	gen_ensure_equals(chain, (bufrex_opcode)0);

	/* Then query the last item */
	CHECKED(bufrex_dtable_query(table, DBA_VAR(3, 35, 10), &chain));
	gen_ensure(chain != NULL);
	cur = chain;

	gen_ensure_equals(cur->val, DBA_VAR(3, 35, 2));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(3, 35, 3));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(3, 35, 7));

	bufrex_opcode_delete(&chain);
	gen_ensure_equals(chain, (bufrex_opcode)0);
}

// Try reading a BUFR edition 4 table
template<> template<>
void to::test<2>()
{
	bufrex_dtable table;
	bufrex_opcode chain;
	bufrex_opcode cur;

	CHECKED(bufrex_dtable_create("D0000000000098013102", &table));

	/* Try querying a nonexisting item */
	gen_ensure_equals(bufrex_dtable_query(table, DBA_VAR(3, 0, 9), &chain), DBA_ERROR);
	chain = NULL;

	/* Now query an existing item */
	CHECKED(bufrex_dtable_query(table, DBA_VAR(3, 1, 24), &chain));
	gen_ensure(chain != NULL);
	cur = chain;

	/*fprintf(stderr, "VAL: %d %02d %03d\n", DBA_VAR_F(cur->val), DBA_VAR_X(cur->val), DBA_VAR_Y(cur->val));*/
	gen_ensure_equals(cur->val, DBA_VAR(0, 5, 2));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 6, 2));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 7, 1));

	bufrex_opcode_delete(&chain);
	gen_ensure_equals(chain, (bufrex_opcode)0);

	/* Then query the last item */
	CHECKED(bufrex_dtable_query(table, DBA_VAR(3, 21, 28), &chain));
	gen_ensure(chain != NULL);
	cur = chain;

	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 118));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(2,  2, 129));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(2,  1, 132));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0,  2, 112));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(2,  1,   0));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(2,  1, 131));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0,  2, 111));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(2,  1,   0));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(2,  2,   0));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0,  2, 104));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 123));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 106));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 107));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 114));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 115));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 116));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0,  8,  18));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 21, 117));

	bufrex_opcode_delete(&chain);
	gen_ensure_equals(chain, (bufrex_opcode)0);
}
}

/* vim:set ts=4 sw=4: */
