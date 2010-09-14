/*
 * wreport/test-utils-wreport - Unit test utilities
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

#include <wibble/tests.h>
#include <wreport/varinfo.h>
#include <wreport/bulletin.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstdlib>

namespace wreport {
struct Var;

namespace tests {

#define ensure_contains(x, y) wreport::tests::impl_ensure_contains(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_contains(x, y) wreport::tests::impl_ensure_contains(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
static inline void impl_ensure_contains(const wibble::tests::Location& loc, const std::string& haystack, const std::string& needle)
{
	if( haystack.find(needle) == std::string::npos )
	{
		std::stringstream ss;
		ss << "'" << haystack << "' does not contain '" << needle << "'";
		throw tut::failure(loc.msg(ss.str()));
	}
}

#define ensure_not_contains(x, y) arki::tests::impl_ensure_not_contains(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_not_contains(x, y) arki::tests::impl_ensure_not_contains(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
static inline void impl_ensure_not_contains(const wibble::tests::Location& loc, const std::string& haystack, const std::string& needle)
{
	if( haystack.find(needle) != std::string::npos )
	{
		std::stringstream ss;
		ss << "'" << haystack << "' must not contain '" << needle << "'";
		throw tut::failure(loc.msg(ss.str()));
	}
}

#define ensure_varcode_equals(x, y) wreport::tests::_ensure_varcode_equals(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_varcode_equals(x, y) wreport::tests::_ensure_varcode_equals(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
void _ensure_varcode_equals(const wibble::tests::Location& loc, wreport::Varcode actual, wreport::Varcode expected);

#define ensure_var_equals(x, y) wreport::tests::_ensure_var_equals(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_var_equals(x, y) wreport::tests::_ensure_var_equals(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, int val);
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, double val);
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, const std::string& val);

#define ensure_var_undef(x) wreport::tests::_ensure_var_undef(wibble::tests::Location(__FILE__, __LINE__, #x " is undef"), (x))
#define inner_ensure_var_undef(x) wreport::tests::_ensure_var_undef(wibble::tests::Location(loc, __FILE__, __LINE__, #x " is undef"), (x))
void _ensure_var_undef(const wibble::tests::Location& loc, const Var& var);


/// Return the pathname of a test file
std::string datafile(const std::string& fname);

/**
 * Read the entire contents of a test file into a string
 *
 * The file name will be resolved through datafile
 */
std::string slurpfile(const std::string& name);


template<typename BULLETIN>
struct MsgTester
{
	virtual ~MsgTester() {}
	virtual void test(const BULLETIN& msg) = 0;
	void operator()(const std::string& name, const BULLETIN& msg)
	{
		try {
			test(msg);
		} catch (tut::failure& f) {
			throw tut::failure("[" + name + "]" + f.what());
		}
	}

	void run(const char* name)
	{
		// Read the whole contents of the test file
		std::string raw1 = slurpfile(name);

		// Decode the original contents
		BULLETIN msg1;
		msg1.decode(raw1, name);
		(*this)("orig", msg1);

		// Encode it again
		std::string raw;
		msg1.encode(raw);

		// Decode our encoder's output
		BULLETIN msg2;
		msg2.decode(raw, name);

		// Test the decoded version
		(*this)("reencoded", msg2);

		// Ensure the two are the same
		ensure_equals(msg1.diff(msg2, stderr), 0);
	}

	void run(const char* tag, const BULLETIN& msg)
	{
		(*this)(tag, msg);
	}
};

/* Test environment */
class LocalEnv
{
	std::string key;
	std::string oldVal;
public:
	LocalEnv(const std::string& key, const std::string& val)
		: key(key)
	{
		const char* v = getenv(key.c_str());
		oldVal = v == NULL ? "" : v;
		setenv(key.c_str(), val.c_str(), 1);
	}
	~LocalEnv()
	{
		setenv(key.c_str(), oldVal.c_str(), 1);
	}
};

} // namespace tests
} // namespace wreport

// vim:set ts=4 sw=4:
