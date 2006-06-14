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

#include <tests/test-utils.h>
#include <string.h> /* strdup */

namespace tut {
using namespace tut_dballe;

struct dba_bufrex_crex_decoder_shar
{
	dba_bufrex_crex_decoder_shar()
	{
	}

	~dba_bufrex_crex_decoder_shar()
	{
	}
};
TESTGRP(dba_bufrex_crex_decoder);

template<> template<>
void to::test<1>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(0);
	test.setSubcat(0);
	test.setVars(49);
	test.set(DBA_VAR(0, 5, 1), 48.22);
	test.set(DBA_VAR(0, 6, 1),  9.92);

	bufrex_raw msg = read_test_msg_raw("crex/test-synop0.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<2>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(0);
	test.setSubcat(0);
	test.setVars(21);
	test.set(DBA_VAR(0, 5, 1), 53.55);
	test.set(DBA_VAR(0, 6, 1), 13.20);

	bufrex_raw msg = read_test_msg_raw("crex/test-synop1.crex", CREX);
	/* {
		dba_var* vars;
		int count;
		CHECKED(bufrex_raw_get_vars(msg, &vars, &count));
		for (int i = 0; i < count; i++)
			dba_var_print(vars[i], stderr);
	} */
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	/*
	bufrex_raw_print(msg, stderr);
	bufrex_raw_print(msg1, stderr);
	*/
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<3>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(0);
	test.setSubcat(0);
	test.setVars(49);
	test.set(DBA_VAR(0, 5, 1), 47.83);
	test.set(DBA_VAR(0, 6, 1), 10.87);

	bufrex_raw msg = read_test_msg_raw("crex/test-synop2.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<4>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(0);
	test.setSubcat(0);
	test.setVars(27);
	test.set(DBA_VAR(0, 5, 1), 61.85);
	test.set(DBA_VAR(0, 6, 1), 24.80);

	bufrex_raw msg = read_test_msg_raw("crex/test-synop3.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<5>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(1);
	test.setSubcat(0);
	test.setVars(32);
	test.set(DBA_VAR(0, 5, 2), 68.27);
	test.set(DBA_VAR(0, 6, 2),  9.68);

	bufrex_raw msg = read_test_msg_raw("crex/test-mare0.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<6>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(1);
	test.setSubcat(0);
	test.setVars(32);
	test.set(DBA_VAR(0, 5, 2),  43.02);
	test.set(DBA_VAR(0, 6, 2), -12.45);

	bufrex_raw msg = read_test_msg_raw("crex/test-mare1.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<7>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(1);
	test.setSubcat(0);
	test.setVars(39);
	test.set(DBA_VAR(0, 5, 2), 33.90);
	test.set(DBA_VAR(0, 6, 2), 29.00);

	bufrex_raw msg = read_test_msg_raw("crex/test-mare2.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<8>()
{
	TestBufrexRaw test;
	test.setEdition(103);
	test.setCat(2);
	test.setSubcat(0);
	test.setVars(550);
	test.set(DBA_VAR(0, 5, 1), 55.75);
	test.set(DBA_VAR(0, 6, 1), 12.52);

	bufrex_raw msg = read_test_msg_raw("crex/test-temp0.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

}

/* vim:set ts=4 sw=4: */
