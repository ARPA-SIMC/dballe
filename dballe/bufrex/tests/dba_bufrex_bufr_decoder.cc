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

namespace tut {
using namespace tut_dballe;

struct dba_bufrex_bufr_decoder_shar
{
	dba_bufrex_bufr_decoder_shar()
	{
	}

	~dba_bufrex_bufr_decoder_shar()
	{
	}
};
TESTGRP(dba_bufrex_bufr_decoder);

template<> template<>
void to::test<1>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 1;
	test.subcat = 21;
	test.subsets = 1;
	test.subset(0).vars = 98;
	test.subset(0).set(DBA_VAR(0, 5, 2), 68.27);
	test.subset(0).set(DBA_VAR(0, 6, 2),  9.68);

	bufrex_msg msg = read_test_msg_raw("bufr/bufr1", BUFR);
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
	test.edition = 3;
	test.cat = 1;
	test.subcat = 21;
	test.subsets = 1;
	test.subset(0).vars = 98;
	test.subset(0).set(DBA_VAR(0, 5, 2),  43.02);
	test.subset(0).set(DBA_VAR(0, 6, 2), -12.45);

	bufrex_msg msg = read_test_msg_raw("bufr/bufr2", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<3>()
{
#if 0
	*** Disabled because this test data uses a template that we do not support

	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(12);
	test.setVars(119);
	test.set(DBA_VAR(0, 5, 2), 54.10);
	test.set(DBA_VAR(0, 6, 2), 12.10);

	bufrex_msg msg = read_test_msg_raw("bufr/bufr3", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg, BUFR);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
#endif
}

template<> template<>
void to::test<4>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 0;
	test.subcat = 1;
	test.subsets = 1;
	test.subset(0).vars = 149;
	test.subset(0).set(DBA_VAR(0, 20, 13), 250.0);
	test.subset(0).set(DBA_VAR(0, 20, 13), 320.0);
	test.subset(0).set(DBA_VAR(0, 20, 13), 620.0);
	test.subset(0).set(DBA_VAR(0, 20, 13), 920.0);
	test.subset(0).setUndef(DBA_VAR(0, 20, 13));

	bufrex_msg msg = read_test_msg_raw("bufr/obs0-1.22.bufr", BUFR);
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
	test.edition = 3;
	test.cat = 0;
	test.subcat = 3;
	test.subsets = 1;
	test.subset(0).vars = 149;
	test.subset(0).set(DBA_VAR(0, 20, 12), 37.0);
	test.subset(0).set(DBA_VAR(0, 20, 12), 22.0);
	test.subset(0).set(DBA_VAR(0, 20, 12), 60.0);
	test.subset(0).set(DBA_VAR(0, 20, 12), 7.0);
	test.subset(0).set(DBA_VAR(0, 20, 12), 5.0);
	test.subset(0).setUndef(DBA_VAR(0, 20, 12));
	test.subset(0).setUndef(DBA_VAR(0, 20, 12));

	bufrex_msg msg = read_test_msg_raw("bufr/obs0-3.504.bufr", BUFR);
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
	test.edition = 3;
	test.cat = 1;
	test.subcat = 9;
	test.subsets = 1;
	test.subset(0).vars = 104;
	test.subset(0).set(DBA_VAR(0,  1,  11), "DFPC");

	bufrex_msg msg = read_test_msg_raw("bufr/obs1-9.2.bufr", BUFR);
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
	test.edition = 3;
	test.cat = 1;
	test.subcat = 11;
	test.subsets = 1;
	test.subset(0).vars = 104;
	test.subset(0).set(DBA_VAR(0, 10, 197), 46.0);

	bufrex_msg msg = read_test_msg_raw("bufr/obs1-11.16.bufr", BUFR);
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
	test.edition = 3;
	test.cat = 1;
	test.subcat = 13;
	test.subsets = 1;
	test.subset(0).vars = 104;

	bufrex_msg msg = read_test_msg_raw("bufr/obs1-13.36.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<9>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 1;
	test.subcat = 19;
	test.subsets = 1;
	test.subset(0).vars = 104;

	bufrex_msg msg = read_test_msg_raw("bufr/obs1-19.3.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<10>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 1;
	test.subcat = 21;
	test.subsets = 1;
	test.subset(0).vars = 98;

	bufrex_msg msg = read_test_msg_raw("bufr/obs1-21.1.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<11>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 2;
	test.subcat = 101;
	test.subsets = 1;
	test.subset(0).vars = 1655;

	bufrex_msg msg = read_test_msg_raw("bufr/obs2-101.16.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<12>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 2;
	test.subcat = 102;
	test.subsets = 1;
	test.subset(0).vars = 1082;

	bufrex_msg msg = read_test_msg_raw("bufr/obs2-102.1.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<13>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 2;
	test.subcat = 91;
	test.subsets = 1;
	test.subset(0).vars = 349;

	bufrex_msg msg = read_test_msg_raw("bufr/obs2-91.2.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<14>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 4;
	test.subcat = 142;
	test.subsets = 1;
	test.subset(0).vars = 56;

	bufrex_msg msg = read_test_msg_raw("bufr/obs4-142.13803.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<15>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 4;
	test.subcat = 144;
	test.subsets = 1;
	test.subset(0).vars = 56;

	bufrex_msg msg = read_test_msg_raw("bufr/obs4-144.4.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<16>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 4;
	test.subcat = 145;
	test.subsets = 1;
	test.subset(0).vars = 86;

	bufrex_msg msg = read_test_msg_raw("bufr/obs4-145.4.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<17>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 3;
	test.subcat = 3;
	test.subsets = 180;
	test.subset(0).vars = 127;
	test.subset(1).vars = 127;
	test.subset(2).vars = 127;
	test.subset(179).vars = 127;

	bufrex_msg msg = read_test_msg_raw("bufr/obs3-3.1.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

template<> template<>
void to::test<18>()
{
	TestBufrexMsg test;
	test.edition = 3;
	test.cat = 3;
	test.subcat = 56;
	test.subsets = 35;
	test.subset(0).vars = 225;
	test.subset(1).vars = 225;
	test.subset(2).vars = 225;
	test.subset(34).vars = 225;

	bufrex_msg msg = read_test_msg_raw("bufr/obs3-56.2.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_msg msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_msg_delete(msg);
	bufrex_msg_delete(msg1);
}

}

/* vim:set ts=4 sw=4: */
