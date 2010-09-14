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
#include <wreport/opcode.h>

using namespace wreport;
using namespace std;

namespace tut {

struct opcode_shar
{
	opcode_shar()
	{
	}

	~opcode_shar()
	{
	}
};
TESTGRP(opcode);

#define CHECK_CHAIN(chain, string) do { \
		bufrex_opcode cur = chain; \
		const char* scur = string; \
		for (; cur != NULL; cur = cur->next, scur++) \
			gen_ensure_equals(cur->val, *scur); \
		gen_ensure_equals(*scur, 0); \
	} while (0)

#define PRINT_CHAIN(chain) do { \
		fputs("CHAIN "#chain ": ", stdout); \
		bufrex_opcode cur = chain; \
		for (; cur != NULL; cur = cur->next) \
			putc(cur->val, stdout); \
		putc('\n', stdout); \
	} while (0)

template<> template<>
void to::test<1>()
{
	vector<Varcode> ch0_vec;
	ch0_vec.push_back('A');
	ch0_vec.push_back('n');
	ch0_vec.push_back('t');

	Opcodes ch0(ch0_vec);
	ensure_equals(ch0.head(), 'A');
	ensure_equals(ch0.next().head(), 'n');
	ensure_equals(ch0.next().next().head(), 't');
	ensure_equals(ch0.next().next().next().head(), 0);

#if 0 // TODO when implementing subchains
	CHECKED(bufrex_opcode_pop_n(&ch0, &ch3, 3));
	/*
	PRINT_CHAIN(ch0);
	PRINT_CHAIN(ch1);
	PRINT_CHAIN(ch2);
	PRINT_CHAIN(ch3);
	*/
	CHECK_CHAIN(ch0, "ntani");
	CHECK_CHAIN(ch1, "A");
	CHECK_CHAIN(ch2, "A");
	CHECK_CHAIN(ch3, "ntA");

	CHECKED(bufrex_opcode_join(&ch2, ch0));
	/*
	PRINT_CHAIN(ch0);
	PRINT_CHAIN(ch1);
	PRINT_CHAIN(ch2);
	PRINT_CHAIN(ch3);
	*/
	CHECK_CHAIN(ch2, "Antani");
	CHECK_CHAIN(ch0, "ntani");

	bufrex_opcode_delete(&ch2);
	bufrex_opcode_delete(&ch3);
#endif

#if 0
	/* Set the location where the tables can be found */
	setenv("DBA_TABLES", "../tables", 1);

	/* Test queries */
	{
		bufrex_decoder decoder;

		CHECKED(crex_decoder_create(&decoder));

		CHECKED(crex_decoder_start_from_file(decoder, "crex/test-acar.crex"));
		while ((err = crex_decoder_next_message(decoder)) == DBA_OK)
			;
		if (dba_error_get_code() != DBA_ERR_NOTFOUND)
		{
			print_dba_error();
			fail("decoding failed");
		}

/*
		crex_decoder_decode_file(decoder, "crex/test-amdar.crex");
		crex_decoder_decode_file(decoder, "crex/test-buoy-baddigit.crex");
		crex_decoder_decode_file(decoder, "crex/test-buoy.crex");
		crex_decoder_decode_file(decoder, "crex/test-satob.crex");
		crex_decoder_decode_file(decoder, "crex/test-synop.crex");
		crex_decoder_decode_file(decoder, "crex/test-synop-ship.crex");
		crex_decoder_decode_file(decoder, "crex/test-temp.crex");
*/

		crex_decoder_delete(decoder);

#if 0
		
		bufrex_dtable table;
		bufrex_opcode chain;
		bufrex_opcode cur;

		CHECKED(bufrex_dtable_create("test-crex-d-table", &table));

		/* Try querying a nonexisting item */
		fail_unless(bufrex_dtable_query(table, WR_VAR(3, 0, 9), &chain) == DBA_ERROR);
		fail_unless(chain == NULL);

		/* Now query an existing item */
		CHECKED(bufrex_dtable_query(table, WR_VAR(3, 35, 6), &chain));
		fail_unless(chain != NULL);
		cur = chain;

		fail_unless(cur->val == WR_VAR(1, 8, 21));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(1, 4, 4));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(1, 8, 21));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(1, 4, 4));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(1, 35, 0));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(1, 1, 3));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(1, 35, 11));

		bufrex_opcode_delete(&chain);
		fail_unless(chain == NULL);

		/* Then query the last item */
		CHECKED(bufrex_dtable_query(table, WR_VAR(3, 35, 10), &chain));
		fail_unless(chain != NULL);
		cur = chain;

		fail_unless(cur->val == WR_VAR(3, 35, 2));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(3, 35, 3));
		cur = chain->next; fail_unless(cur != NULL);
		fail_unless(cur->val == WR_VAR(3, 35, 7));

		bufrex_opcode_delete(&chain);
		fail_unless(chain == NULL);

		bufrex_dtable_delete(table);
#endif
	}
#endif
}

}

// vim:set ts=4 sw=4:
