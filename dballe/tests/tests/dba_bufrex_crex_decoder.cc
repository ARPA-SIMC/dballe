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

#include <extra/test-utils.h>
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
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 0;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 49;
	test.subset(0).set(DBA_VAR(0, 5, 1), 48.22);
	test.subset(0).set(DBA_VAR(0, 6, 1),  9.92);

	bufrex_msg msg = read_test_msg_raw("crex/test-synop0.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<2>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 0;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 21;
	test.subset(0).set(DBA_VAR(0, 5, 1), 53.55);
	test.subset(0).set(DBA_VAR(0, 6, 1), 13.20);

	bufrex_msg msg = read_test_msg_raw("crex/test-synop1.crex", CREX);
	/* {
		dba_var* vars;
		int count;
		CHECKED(bufrex_msg_get_vars(msg, &vars, &count));
		for (int i = 0; i < count; i++)
			dba_var_print(vars[i], stderr);
	} */
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	/*
	bufrex_msg_print(msg, stderr);
	bufrex_msg_print(msg1, stderr);
	*/
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<3>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 0;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 49;
	test.subset(0).set(DBA_VAR(0, 5, 1), 47.83);
	test.subset(0).set(DBA_VAR(0, 6, 1), 10.87);

	bufrex_msg msg = read_test_msg_raw("crex/test-synop2.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<4>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 0;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 27;
	test.subset(0).set(DBA_VAR(0, 5, 1), 61.85);
	test.subset(0).set(DBA_VAR(0, 6, 1), 24.80);

	bufrex_msg msg = read_test_msg_raw("crex/test-synop3.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<5>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 1;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 32;
	test.subset(0).set(DBA_VAR(0, 5, 2), 68.27);
	test.subset(0).set(DBA_VAR(0, 6, 2),  9.68);

	bufrex_msg msg = read_test_msg_raw("crex/test-mare0.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<6>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 1;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 32;
	test.subset(0).set(DBA_VAR(0, 5, 2),  43.02);
	test.subset(0).set(DBA_VAR(0, 6, 2), -12.45);

	bufrex_msg msg = read_test_msg_raw("crex/test-mare1.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<7>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 1;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 39;
	test.subset(0).set(DBA_VAR(0, 5, 2), 33.90);
	test.subset(0).set(DBA_VAR(0, 6, 2), 29.00);

	bufrex_msg msg = read_test_msg_raw("crex/test-mare2.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<8>()
{
	TestBufrexMsg test;
	test.edition = 1;
	test.cat = 2;
	test.subcat = 0;
	test.subsets = 1;
	test.subset(0).vars = 550;
	test.subset(0).set(DBA_VAR(0, 5, 1), 55.75);
	test.subset(0).set(DBA_VAR(0, 6, 1), 12.52);

	bufrex_msg msg = read_test_msg_raw("crex/test-temp0.crex", CREX);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

}

/* vim:set ts=4 sw=4: */
