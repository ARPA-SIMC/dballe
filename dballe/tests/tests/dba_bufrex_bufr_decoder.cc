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
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(21);
	test.setVars(98);
	test.set(DBA_VAR(0, 5, 2), 68.27);
	test.set(DBA_VAR(0, 6, 2),  9.68);

	bufrex_raw msg = read_test_msg_raw("bufr/bufr1", BUFR);
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
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(21);
	test.setVars(98);
	test.set(DBA_VAR(0, 5, 2),  43.02);
	test.set(DBA_VAR(0, 6, 2), -12.45);

	bufrex_raw msg = read_test_msg_raw("bufr/bufr2", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
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

	bufrex_raw msg = read_test_msg_raw("bufr/bufr3", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg, BUFR);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
#endif
}

template<> template<>
void to::test<4>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(0);
	test.setSubcat(1);
	test.setVars(149);
	test.set(DBA_VAR(0, 20, 13), 250.0);
	test.set(DBA_VAR(0, 20, 13), 320.0);
	test.set(DBA_VAR(0, 20, 13), 620.0);
	test.set(DBA_VAR(0, 20, 13), 920.0);
	test.setUndef(DBA_VAR(0, 20, 13));

	bufrex_raw msg = read_test_msg_raw("bufr/obs0-1.22.bufr", BUFR);
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
	test.setEdition(3);
	test.setCat(0);
	test.setSubcat(3);
	test.setVars(149);
	test.set(DBA_VAR(0, 20, 12), 37.0);
	test.set(DBA_VAR(0, 20, 12), 22.0);
	test.set(DBA_VAR(0, 20, 12), 60.0);
	test.set(DBA_VAR(0, 20, 12), 7.0);
	test.set(DBA_VAR(0, 20, 12), 5.0);
	test.setUndef(DBA_VAR(0, 20, 12));
	test.setUndef(DBA_VAR(0, 20, 12));

	bufrex_raw msg = read_test_msg_raw("bufr/obs0-3.504.bufr", BUFR);
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
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(9);
	test.setVars(104);
	test.set(DBA_VAR(0,  1,  11), "DFPC");

	bufrex_raw msg = read_test_msg_raw("bufr/obs1-9.2.bufr", BUFR);
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
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(11);
	test.setVars(104);
	test.set(DBA_VAR(0, 10, 197), 46.0);

	bufrex_raw msg = read_test_msg_raw("bufr/obs1-11.16.bufr", BUFR);
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
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(13);
	test.setVars(104);

	bufrex_raw msg = read_test_msg_raw("bufr/obs1-13.36.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<9>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(19);
	test.setVars(104);

	bufrex_raw msg = read_test_msg_raw("bufr/obs1-19.3.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<10>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(1);
	test.setSubcat(21);
	test.setVars(98);

	bufrex_raw msg = read_test_msg_raw("bufr/obs1-21.1.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<11>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(2);
	test.setSubcat(101);
	test.setVars(1655);

	bufrex_raw msg = read_test_msg_raw("bufr/obs2-101.16.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<12>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(2);
	test.setSubcat(102);
	test.setVars(1082);

	bufrex_raw msg = read_test_msg_raw("bufr/obs2-102.1.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<13>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(2);
	test.setSubcat(91);
	test.setVars(349);

	bufrex_raw msg = read_test_msg_raw("bufr/obs2-91.2.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<14>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(4);
	test.setSubcat(142);
	test.setVars(56);

	bufrex_raw msg = read_test_msg_raw("bufr/obs4-142.13803.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<15>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(4);
	test.setSubcat(144);
	test.setVars(56);

	bufrex_raw msg = read_test_msg_raw("bufr/obs4-144.4.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

template<> template<>
void to::test<16>()
{
	TestBufrexRaw test;
	test.setEdition(3);
	test.setCat(4);
	test.setSubcat(145);
	test.setVars(86);

	bufrex_raw msg = read_test_msg_raw("bufr/obs4-145.4.bufr", BUFR);
	ensureBufrexRawEquals(test, msg);

	bufrex_raw msg1 = reencode_test(msg);
	ensureBufrexRawEquals(test, msg1);

	bufrex_raw_delete(msg);
	bufrex_raw_delete(msg1);
}

}

/* vim:set ts=4 sw=4: */
