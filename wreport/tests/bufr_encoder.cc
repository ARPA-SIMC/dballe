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
#include <string.h>

using namespace wreport;
using namespace std;

namespace tut {

struct bufr_encoder_shar
{
	bufr_encoder_shar()
	{
	}

	~bufr_encoder_shar()
	{
	}
};
TESTGRP(bufr_encoder);

bool memfind(const std::string& rmsg, const char* str, size_t len)
{
	for (size_t i = 0; true; ++i)
	{
		if (i + len >= rmsg.size()) return false;
		if (memcmp((const char*)rmsg.data() + i, str, len) == 0)
			return true;
	}
}

typedef tests::MsgTester<BufrBulletin> MsgTester;

template<> template<>
void to::test<1>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 1u);

			ensure_varcode_equals(s[0].code(), WR_VAR(0, 0, 13));
			ensure_equals(string(s[0].enqc()), "abcdefg");

			/* Ensure that the decoded strings are zero-padded */
			ensure(memcmp(s[0].value(), "abcdefg\0\0\0\0\0\0\0", 7+7) == 0);
		}
	} test;

	BufrBulletin msg;

	/* Initialise common message bits */
	msg.edition = 3;            // BUFR ed.4
	msg.type = 0;               // Template 8.255.171
	msg.subtype = 255;
	msg.localsubtype = 0;
	msg.centre = 98;
	msg.subcentre = 0;
	msg.master_table = 12;
	msg.local_table = 1;
	msg.compression = 1;
	msg.rep_year = 2008;
	msg.rep_month = 5;
	msg.rep_day = 3;
	msg.rep_hour = 12;
	msg.rep_minute = 30;
	msg.rep_second = 0;

	// Load encoding tables
	msg.load_tables();

	// Fill up the data descriptor section
	msg.datadesc.push_back(WR_VAR(0,  0,  13));

	// Get the working subset
	Subset& s = msg.obtain_subset(0);

	// Set a text variable
	s.store_variable_c(WR_VAR(0, 0, 13), "12345678901234567890");

	// Set it to a shorter text, to see if the encoder encodes the trailing garbage
	s[0].setc("abcdefg");

	// Run tests on the original
	test.run("orig", msg);

	// Encode
	string rmsg;
	msg.encode(rmsg);

	// Ensure that the encoded strings are space-padded
	ensure(memfind(rmsg, "abcdefg       ", 14));

	// Decode the message and retest
	BufrBulletin msg1;
	msg1.decode(rmsg);
	test.run("reencoded", msg1);
}

// Encode a BUFR with an optional section
template<> template<>
void to::test<2>()
{
	struct Tester : public MsgTester {
		void test(const BufrBulletin& msg)
		{
			ensure_equals(msg.edition, 3);
			ensure_equals(msg.type, 0);
			ensure_equals(msg.subtype, 255);
			ensure_equals(msg.localsubtype, 0);
			ensure_equals(msg.subsets.size(), 1);

			const Subset& s = msg.subset(0);
			ensure_equals(s.size(), 1u);

			ensure_varcode_equals(s[0].code(), WR_VAR(0, 0, 13));
			ensure_equals(string(s[0].enqc()), "abcdefg");

			/* Ensure that the decoded strings are zero-padded */
			ensure(memcmp(s[0].value(), "abcdefg\0\0\0\0\0\0\0", 7+7) == 0);
		}
	} test;

	BufrBulletin msg;

	// Initialise common message bits
	msg.edition = 3;            // BUFR ed.4
	msg.type = 0;               // Template 8.255.171
	msg.subtype = 255;
	msg.localsubtype = 0;
	msg.centre = 98;
	msg.subcentre = 0;
	msg.master_table = 12;
	msg.local_table = 1;
	msg.compression = 1;
	msg.optional_section_length = 5;
	msg.optional_section = new char[5];
	strcpy(msg.optional_section, "Ciao");
	msg.rep_year = 2008;
	msg.rep_month = 5;
	msg.rep_day = 3;
	msg.rep_hour = 12;
	msg.rep_minute = 30;
	msg.rep_second = 0;

	// Load encoding tables
	msg.load_tables();

	// Fill up the data descriptor section
	msg.datadesc.push_back(WR_VAR(0, 0, 13));

	// Get the working subset
	Subset& s = msg.obtain_subset(0);

	// Set a text variable
	s.store_variable_c(WR_VAR(0, 0, 13), "12345678901234567890");

	// Set it to a shorter text, to see if the encoder encodes the trailing garbage
	s[0].setc("abcdefg");

	// Run tests on the original
	test.run("orig", msg);

	// Encode
	string rmsg;
	msg.encode(rmsg);

	// Ensure that the encoded strings are space-padded
	ensure(memfind(rmsg, "abcdefg       ", 14));

	// Decode the message and retest
	BufrBulletin msg1;
	msg1.decode(rmsg);

	// Check that the optional section has been padded
	ensure_equals(msg1.optional_section_length, 6);
	ensure_equals(memcmp(msg1.optional_section, "Ciao\0", 6), 0);

	test.run("reencoded", msg1);
}

// Test variable ranges during encoding
template<> template<>
void to::test<3>()
{
	BufrBulletin msg;

	// Initialise common message bits
	msg.edition = 3;            // BUFR ed.4
	msg.type = 0;               // Template 8.255.171
	msg.subtype = 255;
	msg.localsubtype = 0;
	msg.centre = 98;
	msg.subcentre = 0;
	msg.master_table = 12;
	msg.local_table = 1;
	msg.compression = 0;
	msg.rep_year = 2008;
	msg.rep_month = 5;
	msg.rep_day = 3;
	msg.rep_hour = 12;
	msg.rep_minute = 30;
	msg.rep_second = 0;

	// Load encoding tables
	msg.load_tables();

	// Fill up the data descriptor section
	msg.datadesc.push_back(WR_VAR(0, 1, 1));

	/* Get the working subset */
	Subset& s = msg.obtain_subset(0);

	/* Set the test variable */
	//CHECKED(bufrex_subset_store_variable_d(s, WR_VAR(0, 1, 1), -1.0));
	/* Now it errors here, because the range check is appropriately strict */
	try {
		s.store_variable_d(WR_VAR(0, 1, 1), -1.0);
		ensure(false);
	} catch (error_domain& e) {
		ensure_contains(e.what(), "B01001");

	}

#if 0
	/* Encode gives error because of overflow */
	dba_rawmsg rmsg = NULL;
	dba_err err = bufrex_msg_encode(msg, &rmsg);
	ensure(err == DBA_ERROR);
#endif
}



}

/* vim:set ts=4 sw=4: */
