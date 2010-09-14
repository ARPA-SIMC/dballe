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

using namespace wreport;
using namespace std;

namespace tut {

struct bufr_decoder_shar
{
	bufr_decoder_shar()
	{
	}

	~bufr_decoder_shar()
	{
	}
};
TESTGRP(bufr_decoder);

#if 0
#define ensure_has_33007(msg, subset, var, val) _ensure_has_33007(__FILE__, __LINE__, msg, subset, var, val)
void _ensure_has_33007(const char* file, int line, bufrex_msg msg, int subset, int var, int val)
{
	dba_var attr;
	INNER_CHECKED(dba_var_enqa(msg->subsets[subset]->vars[var], WR_VAR(0, 33, 7), &attr));
	inner_ensure(attr != NULL);

	int ival;
	INNER_CHECKED(dba_var_enqi(attr, &ival));
	inner_ensure_equals(ival, val);
}
#endif

typedef tests::MsgTester<BufrBulletin> MsgTester;

template<> template<>
void to::test<1>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 21);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 35u);

			ensure_varcode_equals(s[9].code(), WR_VAR(0, 5, 2));
			ensure_equals(s[9].enqd(), 68.27);
			ensure_varcode_equals(s[10].code(), WR_VAR(0, 6, 2));
			ensure_equals(s[10].enqd(),  9.68);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/bufr1");
}

template<> template<>
void to::test<2>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 21);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 35u);

			ensure_varcode_equals(s[9].code(), WR_VAR(0, 5, 2));
			ensure_equals(s[9].enqd(), 43.02);
			ensure_varcode_equals(s[10].code(), WR_VAR(0, 6, 2));
			ensure_equals(s[10].enqd(), -12.45);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/bufr2");
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
	test.set(WR_VAR(0, 5, 2), 54.10);
	test.set(WR_VAR(0, 6, 2), 12.10);

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
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 1);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 52u);

			ensure_varcode_equals(s[27].code(), WR_VAR(0, 20, 13));
			ensure_equals(s[27].enqd(), 250.0);
			ensure_varcode_equals(s[34].code(), WR_VAR(0, 20, 13));
			ensure_equals(s[34].enqd(), 320.0);
			ensure_varcode_equals(s[38].code(), WR_VAR(0, 20, 13));
			ensure_equals(s[38].enqd(), 620.0);
			ensure_varcode_equals(s[42].code(), WR_VAR(0, 20, 13));
			ensure_equals(s[42].enqd(), 920.0);
			ensure_varcode_equals(s[46].code(), WR_VAR(0, 20, 13));
			ensure(s[46].value() == NULL);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs0-1.22.bufr");
}

template<> template<>
void to::test<5>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 3);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 52u);

			ensure_varcode_equals(s[28].code(), WR_VAR(0, 20, 12));
			ensure_equals(s[28].enqd(), 37.0);
			ensure_varcode_equals(s[29].code(), WR_VAR(0, 20, 12));
			ensure_equals(s[29].enqd(), 22.0);
			ensure_varcode_equals(s[30].code(), WR_VAR(0, 20, 12));
			ensure_equals(s[30].enqd(), 60.0);
			ensure_varcode_equals(s[33].code(), WR_VAR(0, 20, 12));
			ensure_equals(s[33].enqd(),  7.0);
			ensure_varcode_equals(s[37].code(), WR_VAR(0, 20, 12));
			ensure_equals(s[37].enqd(),  5.0);
			ensure_varcode_equals(s[41].code(), WR_VAR(0, 20, 12));
			ensure(s[41].value() == NULL);
			ensure_varcode_equals(s[45].code(), WR_VAR(0, 20, 12));
			ensure(s[45].value() == NULL);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs0-3.504.bufr");
}

template<> template<>
void to::test<6>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 9);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 37u);

			ensure_varcode_equals(s[0].code(), WR_VAR(0, 1, 11));
			ensure_equals(string(s[0].enqc()), "DFPC");

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs1-9.2.bufr");
}

template<> template<>
void to::test<7>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 11);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 37u);

			ensure_varcode_equals(s[33].code(), WR_VAR(0, 10, 197));
			ensure_equals(s[33].enqd(), 46.0);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs1-11.16.bufr");
}

template<> template<>
void to::test<8>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 13);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 37u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs1-13.36.bufr");
}

template<> template<>
void to::test<9>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 19);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 37u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs1-19.3.bufr");
}

template<> template<>
void to::test<10>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 21);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 35u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/synop-old-buoy.bufr");
}

template<> template<>
void to::test<11>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 2);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 101);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 619u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[1].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[1].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs2-101.16.bufr");
}

template<> template<>
void to::test<12>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 2);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 102);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 403u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs2-102.1.bufr");
}

template<> template<>
void to::test<13>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 2);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 91);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 127u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs2-91.2.bufr");
}

template<> template<>
void to::test<14>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 4);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 142);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 21u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/airep-old-4-142.bufr");
}

template<> template<>
void to::test<15>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 4);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 144);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 21u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs4-144.4.bufr");
}

template<> template<>
void to::test<16>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 4);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 145);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 31u);

			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
		}
	} test;

	test.run("bufr/obs4-145.4.bufr");
}

template<> template<>
void to::test<17>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 3);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 3);
			ensure_equals(msg.subsets.size(), 180);

			ensure_equals(msg.subset(0).size(), 127u);
			ensure_equals(msg.subset(1).size(), 127u);
			ensure_equals(msg.subset(2).size(), 127u);
			ensure_equals(msg.subset(179).size(), 127u);
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/obs3-3.1.bufr");
}

template<> template<>
void to::test<18>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 3);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 56);
			ensure_equals(msg.subsets.size(), 35u);

			ensure_equals(msg.subset(0).size(), 225u);
			ensure_equals(msg.subset(1).size(), 225u);
			ensure_equals(msg.subset(2).size(), 225u);
			ensure_equals(msg.subset(34).size(), 225u);

			const Subset& s = msg.subset(0);
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/obs3-56.2.bufr");
}

template<> template<>
void to::test<19>()
{
	//bufrex_msg msgr = read_test_msg_header_raw("bufr/ed4.bufr", BUFR);
	//ensureBufrexRawEquals(test, msgr);

	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 8);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 171);
			ensure_equals(msg.subsets.size(), 128u);

			ensure_equals(msg.subset(0).size(), 26u);
			ensure_equals(msg.subset(1).size(), 26u);
			ensure_equals(msg.subset(2).size(), 26u);
			ensure_equals(msg.subset(127).size(), 26u);
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/ed4.bufr");
}

template<> template<>
void to::test<20>()
{
	//bufrex_msg msgr = read_test_msg_header_raw("bufr/ed4date.bufr", BUFR);
	//ensureBufrexRawEquals(test, msgr);

	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 8);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 171);
			ensure_equals(msg.subsets.size(), 128u);

			ensure_equals(msg.rep_year, 2000);
			ensure_equals(msg.rep_month, 1);
			ensure_equals(msg.rep_day, 2);
			ensure_equals(msg.rep_hour, 7);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 26u);
			ensure_equals(msg.subset(1).size(), 26u);
			ensure_equals(msg.subset(2).size(), 26u);
			ensure_equals(msg.subset(127).size(), 26u);
		}
	} test;

	test.run("bufr/ed4date.bufr");
}

template<> template<>
void to::test<21>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 2);
			ensure_equals(msg.type, 6);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1u);

			ensure_equals(msg.rep_year, 2007);
			ensure_equals(msg.rep_month, 8);
			ensure_equals(msg.rep_day, 13);
			ensure_equals(msg.rep_hour, 18);
			ensure_equals(msg.rep_minute, 30);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 4606u);
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/ed2radar.bufr");
}

/*
 * In this case, the ECMWF table has 12 bits for BUFR in Kelvin (up to 409.6)
 * but 3 digits for CREX in Celsius (up to 99.0).  This means that BUFR can
 * encode values too big to fit in CREX, and when it happens dba_var range
 * checks kick in and abort decoding.
 */
template<> template<>
void to::test<22>()
{
	/*
	TestBufrexMsg test;
	test.edition = 2;
	test.cat = 6;
	test.subcat = 255;
	test.localsubcat = 0;
	test.subsets = 1;
	*/

	//bufrex_msg msg = read_test_msg_header_raw("bufr/crex-has-few-digits.bufr", BUFR);
	/*
	ensureBufrexRawEquals(test, msg);
	ensure_equals(msg->rep_year, 2007);
	ensure_equals(msg->rep_month, 8);
	ensure_equals(msg->rep_day, 13);
	ensure_equals(msg->rep_hour, 18);
	ensure_equals(msg->rep_minute, 30);
	ensure_equals(msg->rep_second, 0);

	test.subset(0).vars = 4606;
	*/

	//bufrex_msg msg1 = reencode_test(msg);
	//ensureBufrexRawEquals(test, msg1);

	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			/*
			ensure_equals(msg.edition, 2);
			ensure_equals(msg.type, 6);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1u);

			ensure_equals(msg.rep_year, 2007);
			ensure_equals(msg.rep_month, 8);
			ensure_equals(msg.rep_day, 13);
			ensure_equals(msg.rep_hour, 18);
			ensure_equals(msg.rep_minute, 30);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 4606u);

			const Subset& s = msg.subset(0);

			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/crex-has-few-digits.bufr");
}

// Buoy who could not look up a D table
template<> template<>
void to::test<23>()
{
	/*
	TestBufrexMsg test;
	test.edition = 2;
	test.cat = 6;
	test.subcat = 255;
	test.localsubcat = 0;
	test.subsets = 1;
	*/

	//bufrex_msg msg = read_test_msg_header_raw("bufr/test-buoy1.bufr", BUFR);
	/*
	ensureBufrexRawEquals(test, msg);
	ensure_equals(msg->rep_year, 2007);
	ensure_equals(msg->rep_month, 8);
	ensure_equals(msg->rep_day, 13);
	ensure_equals(msg->rep_hour, 18);
	ensure_equals(msg->rep_minute, 30);
	ensure_equals(msg->rep_second, 0);

	test.subset(0).vars = 4606;
	*/

	//bufrex_msg msg1 = reencode_test(msg);
	//ensureBufrexRawEquals(test, msg1);

	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			/*
			ensure_equals(msg.edition, 2);
			ensure_equals(msg.type, 6);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1u);

			ensure_equals(msg.rep_year, 2007);
			ensure_equals(msg.rep_month, 8);
			ensure_equals(msg.rep_day, 13);
			ensure_equals(msg.rep_hour, 18);
			ensure_equals(msg.rep_minute, 30);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 4606u);

			const Subset& s = msg.subset(0);

			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/test-buoy1.bufr");
}

// Soil temperature message
template<> template<>
void to::test<24>()
{
	/*
	TestBufrexMsg test;
	test.edition = 2;
	test.cat = 6;
	test.subcat = 255;
	test.localsubcat = 0;
	test.subsets = 1;
	*/

	//bufrex_msg msg = read_test_msg_header_raw("bufr/test-soil1.bufr", BUFR);
	/*
	ensureBufrexRawEquals(test, msg);
	ensure_equals(msg->rep_year, 2007);
	ensure_equals(msg->rep_month, 8);
	ensure_equals(msg->rep_day, 13);
	ensure_equals(msg->rep_hour, 18);
	ensure_equals(msg->rep_minute, 30);
	ensure_equals(msg->rep_second, 0);

	test.subset(0).vars = 4606;
	*/

	//bufrex_msg msg1 = reencode_test(msg);
	//ensureBufrexRawEquals(test, msg1);

	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			/*
			ensure_equals(msg.edition, 2);
			ensure_equals(msg.type, 6);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1u);

			ensure_equals(msg.rep_year, 2007);
			ensure_equals(msg.rep_month, 8);
			ensure_equals(msg.rep_day, 13);
			ensure_equals(msg.rep_hour, 18);
			ensure_equals(msg.rep_minute, 30);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 4606u);

			const Subset& s = msg.subset(0);

			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/test-soil1.bufr");
}

// BUFR4 with compressed strings
template<> template<>
void to::test<25>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 2);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 5u);

			ensure_equals(msg.rep_year, 2009);
			ensure_equals(msg.rep_month, 12);
			ensure_equals(msg.rep_day, 3);
			ensure_equals(msg.rep_hour, 3);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 115u);
			ensure_equals(msg.subset(1).size(), 115u);
			ensure_equals(msg.subset(2).size(), 115u);
			ensure_equals(msg.subset(3).size(), 115u);
			ensure_equals(msg.subset(4).size(), 115u);

			const Subset& s = msg.subset(0);

			/*
			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/ed4-compr-string.bufr");

}

// BUFR4 which gives a parse error
template<> template<>
void to::test<26>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 1);
			ensure_equals(msg.localsubtype, 255);
			ensure_equals(msg.subsets.size(), 5u);

			ensure_equals(msg.rep_year, 2009);
			ensure_equals(msg.rep_month, 12);
			ensure_equals(msg.rep_day, 3);
			ensure_equals(msg.rep_hour, 3);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 107u);
			ensure_equals(msg.subset(1).size(), 107u);
			ensure_equals(msg.subset(2).size(), 107u);
			ensure_equals(msg.subset(3).size(), 107u);
			ensure_equals(msg.subset(4).size(), 107u);

			const Subset& s = msg.subset(0);

			/*
			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/ed4-parseerror1.bufr");
}

// BUFR4 which does not give a parse error but looks empty
template<> template<>
void to::test<27>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 1);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 7u);

			ensure_equals(msg.rep_year, 2009);
			ensure_equals(msg.rep_month, 12);
			ensure_equals(msg.rep_day, 3);
			ensure_equals(msg.rep_hour, 3);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 120u);
			ensure_equals(msg.subset(1).size(), 120u);
			ensure_equals(msg.subset(2).size(), 120u);
			ensure_equals(msg.subset(3).size(), 120u);
			ensure_equals(msg.subset(4).size(), 120u);
			ensure_equals(msg.subset(5).size(), 120u);
			ensure_equals(msg.subset(6).size(), 120u);

			const Subset& s = msg.subset(0);

			/*
			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/ed4-empty.bufr");
}

// GTS temp message
template<> template<>
void to::test<28>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			/*
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 1);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 7u);

			ensure_equals(msg.rep_year, 2009);
			ensure_equals(msg.rep_month, 12);
			ensure_equals(msg.rep_day, 3);
			ensure_equals(msg.rep_hour, 3);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 120u);
			ensure_equals(msg.subset(1).size(), 120u);
			ensure_equals(msg.subset(2).size(), 120u);
			ensure_equals(msg.subset(3).size(), 120u);
			ensure_equals(msg.subset(4).size(), 120u);
			ensure_equals(msg.subset(5).size(), 120u);
			ensure_equals(msg.subset(6).size(), 120u);
			ensure_equals(msg.subset(7).size(), 120u);

			const Subset& s = msg.subset(0);
			*/

			/*
			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/C05060.bufr");
}

// Custom ARPA temp forecast message saved as ARPA generic
template<> template<>
void to::test<29>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			/*
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 1);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 7u);

			ensure_equals(msg.rep_year, 2009);
			ensure_equals(msg.rep_month, 12);
			ensure_equals(msg.rep_day, 3);
			ensure_equals(msg.rep_hour, 3);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 120u);
			ensure_equals(msg.subset(1).size(), 120u);
			ensure_equals(msg.subset(2).size(), 120u);
			ensure_equals(msg.subset(3).size(), 120u);
			ensure_equals(msg.subset(4).size(), 120u);
			ensure_equals(msg.subset(5).size(), 120u);
			ensure_equals(msg.subset(6).size(), 120u);
			ensure_equals(msg.subset(7).size(), 120u);

			const Subset& s = msg.subset(0);
			*/

			/*
			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/tempforecast.bufr");
}

// C23000 modifier
template<> template<>
void to::test<30>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			/*
			ensure_equals(msg.edition, 4);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 1);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 7u);

			ensure_equals(msg.rep_year, 2009);
			ensure_equals(msg.rep_month, 12);
			ensure_equals(msg.rep_day, 3);
			ensure_equals(msg.rep_hour, 3);
			ensure_equals(msg.rep_minute, 0);
			ensure_equals(msg.rep_second, 0);

			ensure_equals(msg.subset(0).size(), 120u);
			ensure_equals(msg.subset(1).size(), 120u);
			ensure_equals(msg.subset(2).size(), 120u);
			ensure_equals(msg.subset(3).size(), 120u);
			ensure_equals(msg.subset(4).size(), 120u);
			ensure_equals(msg.subset(5).size(), 120u);
			ensure_equals(msg.subset(6).size(), 120u);
			ensure_equals(msg.subset(7).size(), 120u);

			const Subset& s = msg.subset(0);
			*/

			/*
			// FIXME Does it have this?
			ensure(s[0].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[0].enqa(WR_VAR(0, 33, 7))->enqi(), 70);

			ensure(s[5].enqa(WR_VAR(0, 33, 7)) != NULL);
			ensure_equals(s[5].enqa(WR_VAR(0, 33, 7))->enqi(), 70);
			*/
		}
	} test;

	// FIXME: recoding might not work
	test.run("bufr/C23000.bufr");
}

}

/* vim:set ts=4 sw=4: */
