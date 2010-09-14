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

struct crex_decoder_shar
{
	crex_decoder_shar()
	{
	}

	~crex_decoder_shar()
	{
	}
};
TESTGRP(crex_decoder);

typedef tests::MsgTester<CrexBulletin> MsgTester;

template<> template<>
void to::test<1>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 49u);

			ensure_varcode_equals(s[8].code(), WR_VAR(0, 5, 1));
			ensure_equals(s[8].enqd(), 48.22);
			ensure_varcode_equals(s[9].code(), WR_VAR(0, 6, 1));
			ensure_equals(s[9].enqd(), 9.92);
			ensure_varcode_equals(s[17].code(), WR_VAR(0, 12, 4));
			ensure_equals(s[17].enqd(), 3.0);
			ensure_varcode_equals(s[18].code(), WR_VAR(0, 12, 6));
			ensure_equals(s[18].enqd(), 0.7);
		}
	} test;

	test.run("crex/test-synop0.crex");
}

template<> template<>
void to::test<2>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 21u);

			ensure_varcode_equals(s[8].code(), WR_VAR(0, 5, 1));
			ensure_equals(s[8].enqd(), 53.55);
			ensure_varcode_equals(s[9].code(), WR_VAR(0, 6, 1));
			ensure_equals(s[9].enqd(), 13.20);
		}
	} test;

	test.run("crex/test-synop1.crex");
}

template<> template<>
void to::test<3>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 49u);

			ensure_varcode_equals(s[8].code(), WR_VAR(0, 5, 1));
			ensure_equals(s[8].enqd(), 47.83);
			ensure_varcode_equals(s[9].code(), WR_VAR(0, 6, 1));
			ensure_equals(s[9].enqd(), 10.87);
		}
	} test;

	test.run("crex/test-synop2.crex");
}

template<> template<>
void to::test<4>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 27u);

			ensure_varcode_equals(s[8].code(), WR_VAR(0, 5, 1));
			ensure_equals(s[8].enqd(), 61.85);
			ensure_varcode_equals(s[9].code(), WR_VAR(0, 6, 1));
			ensure_equals(s[9].enqd(), 24.80);
		}
	} test;

	test.run("crex/test-synop3.crex");
}

template<> template<>
void to::test<5>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 32u);

			ensure_varcode_equals(s[9].code(), WR_VAR(0, 5, 2));
			ensure_equals(s[9].enqd(), 68.27);
			ensure_varcode_equals(s[10].code(), WR_VAR(0, 6, 2));
			ensure_equals(s[10].enqd(),  9.68);
		}
	} test;

	test.run("crex/test-mare0.crex");
}

template<> template<>
void to::test<6>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 32u);

			ensure_varcode_equals(s[9].code(), WR_VAR(0, 5, 2));
			ensure_equals(s[9].enqd(),  43.02);
			ensure_varcode_equals(s[10].code(), WR_VAR(0, 6, 2));
			ensure_equals(s[10].enqd(), -12.45);
		}
	} test;

	test.run("crex/test-mare1.crex");
}

template<> template<>
void to::test<7>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 1);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 39u);

			ensure_varcode_equals(s[9].code(), WR_VAR(0, 5, 2));
			ensure_equals(s[9].enqd(), 33.90);
			ensure_varcode_equals(s[10].code(), WR_VAR(0, 6, 2));
			ensure_equals(s[10].enqd(), 29.00);
		}
	} test;

	test.run("crex/test-mare2.crex");
}

template<> template<>
void to::test<8>()
{
	struct Tester : public MsgTester {
		void test(const CrexBulletin& msg)
		{
			ensure_equals(msg.edition, 1);
			ensure_equals(msg.type, 2);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 550u);

			ensure_varcode_equals(s[9].code(), WR_VAR(0, 5, 1));
			ensure_equals(s[9].enqd(), 55.75);
			ensure_varcode_equals(s[10].code(), WR_VAR(0, 6, 1));
			ensure_equals(s[10].enqd(), 12.52);
		}
	} test;

	test.run("crex/test-temp0.crex");
}

}

/* vim:set ts=4 sw=4: */
