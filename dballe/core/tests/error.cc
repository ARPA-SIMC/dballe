/*
 * DB-ALLe - Archive for punctual meteorological data
 *
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

#include <test-utils-core.h>
#include <dballe/core/error.h>
#include <string.h> /* strdup */

using namespace dballe;
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

static void cb_count(void* data)
{
	++(*(int*)data);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
}

// Basic error functions and callbacks
template<> template<>
void to::test<1>()
{
	int allcount = 0;
	int somecount = 0;
	int alloccount = 0;
	
	dba_error_set_callback(DBA_ERR_NONE, cb_count, &allcount);
	dba_error_set_callback(DBA_ERR_NONE, cb_count, &somecount);
	dba_error_set_callback(DBA_ERR_ALLOC, cb_count, &alloccount);

	ensure_equals(dba_error_ok(), DBA_OK);
	ensure_equals(dba_error_get_code(), DBA_ERR_NONE);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);

	ensure_equals(dba_error_notfound("booh"), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_NOTFOUND);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);

	ensure_equals(dba_error_type("dummy error message %f", 3.14), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_TYPE);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);

	ensure_equals(dba_error_handles("dummy error message %d", 314), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_HANDLES);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);

	ensure_equals(dba_error_toolong("dummy error message %s", "3.14"), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_TOOLONG);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);

	dba_error_remove_callback(DBA_ERR_NONE, cb_count, &somecount);

	ensure_equals(alloccount, 0);
	ensure_equals(dba_error_alloc("alloc error"), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_ALLOC);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);
	ensure_equals(alloccount, 1);

	ensure_equals(dba_error_system("dummy error message %s", "3.14"), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_SYSTEM);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure(dba_error_get_details() != NULL);

	ensure_equals(dba_error_consistency("dummy error message %s", "3.14"), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_CONSISTENCY);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure_equals(dba_error_get_details(), (const char*)0);

	ensure_equals(dba_error_parse("pippo.txt", 3, "dummy error message"), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_PARSE);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure(dba_error_get_details() != NULL);

	ensure_equals(dba_error_generic0(DBA_ERR_NOTFOUND, strdup("dummy error message"), strdup("this is just a dummy error message")), DBA_ERROR);
	ensure_equals(dba_error_get_code(), DBA_ERR_NOTFOUND);
	ensure(dba_error_get_message() != NULL);
	ensure(dba_error_get_context() != NULL);
	ensure(dba_error_get_details() != NULL);

	ensure_equals(allcount, 9);
	ensure_equals(somecount, 4);
	ensure_equals(alloccount, 1);

	dba_error_remove_callback(DBA_ERR_NONE, cb_count, &allcount);
	dba_error_remove_callback(DBA_ERR_ALLOC, cb_count, &alloccount);
}

// Test notfound
template<> template<>
void to::test<2>()
{
	try {
		throw error_notfound("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_NOTFOUND);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_notfound::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_NOTFOUND);
		ensure_equals(string(e.what()), "42");
	}
}

// Test type
template<> template<>
void to::test<3>()
{
	try {
		throw error_type("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_TYPE);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_type::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_TYPE);
		ensure_equals(string(e.what()), "42");
	}
}

// Test alloc
template<> template<>
void to::test<4>()
{
	try {
		throw error_alloc("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_ALLOC);
		ensure_equals(string(e.what()), "foo");
	}
}

// Test handles
template<> template<>
void to::test<5>()
{
	try {
		throw error_handles("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_HANDLES);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_handles::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_HANDLES);
		ensure_equals(string(e.what()), "42");
	}
}

// Test toolong
template<> template<>
void to::test<6>()
{
	try {
		throw error_toolong("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_TOOLONG);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_toolong::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_TOOLONG);
		ensure_equals(string(e.what()), "42");
	}
}

// Test system
template<> template<>
void to::test<7>()
{
	try {
		throw error_system("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_SYSTEM);
		ensure_equals(string(e.what()).substr(0, 5), "foo: ");
	}

	try {
		error_system::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_SYSTEM);
		ensure_equals(string(e.what()).substr(0, 4), "42: ");
	}
}

// Test consistency
template<> template<>
void to::test<8>()
{
	try {
		throw error_consistency("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_CONSISTENCY);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_consistency::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_CONSISTENCY);
		ensure_equals(string(e.what()), "42");
	}
}

// Test parse
template<> template<>
void to::test<9>()
{
	try {
		throw error_parse("file", 42, "foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_PARSE);
		ensure_equals(string(e.what()), "file:42: foo");
	}

	try {
		error_parse::throwf("file", 42, "%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_PARSE);
		ensure_equals(string(e.what()), "file:42: 42");
	}
}

// Test regexp
template<> template<>
void to::test<10>()
{
	// TODO: setup a test case involving a regexp
}

// Test unimplemented
template<> template<>
void to::test<11>()
{
	try {
		throw error_unimplemented("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_UNIMPLEMENTED);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_unimplemented::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_UNIMPLEMENTED);
		ensure_equals(string(e.what()), "42");
	}
}

// Test domain
template<> template<>
void to::test<12>()
{
	try {
		throw error_domain("foo");
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_DOMAIN);
		ensure_equals(string(e.what()), "foo");
	}

	try {
		error_domain::throwf("%d", 42);
	} catch (error& e) {
		ensure_equals(e.code(), DBA_ERR_DOMAIN);
		ensure_equals(string(e.what()), "42");
	}
}

}

/* vim:set ts=4 sw=4: */
