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
#include <wreport/error.h>
#include <string.h> /* strdup */

using namespace wreport;
using namespace std;

namespace tut {

struct error_shar
{
	error_shar()
	{
	}

	~error_shar()
	{
	}
};
TESTGRP(error);

// Test notfound
template<> template<>
void to::test<1>()
{
	try {
		throw error_notfound("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_NOTFOUND);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_notfound::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_NOTFOUND);
		ensure_equals(string(e.what()), "42");
	}
}

// Test type
template<> template<>
void to::test<2>()
{
	try {
		throw error_type("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_TYPE);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_type::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_TYPE);
		ensure_equals(string(e.what()), "42");
	}
}

// Test alloc
template<> template<>
void to::test<3>()
{
	try {
		throw error_alloc("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_ALLOC);
		ensure_equals(string(e.what()), "foo");
	}
}

// Test handles
template<> template<>
void to::test<4>()
{
	try {
		throw error_handles("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_HANDLES);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_handles::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_HANDLES);
		ensure_equals(string(e.what()), "42");
	}
}

// Test toolong
template<> template<>
void to::test<5>()
{
	try {
		throw error_toolong("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_TOOLONG);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_toolong::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_TOOLONG);
		ensure_equals(string(e.what()), "42");
	}
}

// Test system
template<> template<>
void to::test<6>()
{
	try {
		throw error_system("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_SYSTEM);
		ensure_equals(string(e.what()).substr(0, 5), "foo: ");
	}

	try {
		error_system::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_SYSTEM);
		ensure_equals(string(e.what()).substr(0, 4), "42: ");
	}
}

// Test consistency
template<> template<>
void to::test<7>()
{
	try {
		throw error_consistency("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_CONSISTENCY);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_consistency::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_CONSISTENCY);
		ensure_equals(string(e.what()), "42");
	}
}

// Test parse
template<> template<>
void to::test<8>()
{
	try {
		throw error_parse("file", 42, "foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_PARSE);
		ensure_equals(string(e.what()), "file:42: foo");
	}

	try {
		error_parse::throwf("file", 42, "%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_PARSE);
		ensure_equals(string(e.what()), "file:42: 42");
	}
}

// Test regexp
template<> template<>
void to::test<9>()
{
	// TODO: setup a test case involving a regexp
}

// Test unimplemented
template<> template<>
void to::test<10>()
{
	try {
		throw error_unimplemented("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_UNIMPLEMENTED);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_unimplemented::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_UNIMPLEMENTED);
		ensure_equals(string(e.what()), "42");
	}
}

// Test domain
template<> template<>
void to::test<11>()
{
	try {
		throw error_domain("foo");
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_DOMAIN);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_domain::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), WR_ERR_DOMAIN);
		ensure_equals(string(e.what()), "42");
	}
}

}

/* vim:set ts=4 sw=4: */
