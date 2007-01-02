#include <dballe++/error.h>
#include <dballe/core/test-utils-core.h>

using namespace std;

namespace tut {
using namespace tut_dballe;

struct error_shar {
};

TESTGRP( error );

using namespace dballe;

// Check if the appropriate exceptions are thrown for the appropriate errors
template<> template<>
void to::test<1>()
{
	try {
		checked(DBA_OK);
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_notfound("foo"));
	} catch (dballe::exception::NotFound& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_type("foo"));
	} catch (dballe::exception::Type& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_alloc("foo"));
	} catch (dballe::exception::Alloc& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_generic1(DBA_ERR_ODBC, "foo", "foo"));
	} catch (dballe::exception::ODBC& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_handles("foo"));
	} catch (dballe::exception::Handles& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_toolong("foo"));
	} catch (dballe::exception::TooLong& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_system("foo"));
	} catch (dballe::exception::System& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_consistency("foo"));
	} catch (dballe::exception::Consistency& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_parse("foo", 0, "foo"));
	} catch (dballe::exception::Parse& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_generic1(DBA_ERR_WRITE, "foo"));
	} catch (dballe::exception::Write& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_generic1(DBA_ERR_REGEX, "foo"));
	} catch (dballe::exception::Regex& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}

	try {
		checked(dba_error_unimplemented("foo"));
	} catch (dballe::exception::Unimplemented& e) {
		;
	} catch (...) {
		gen_ensure(false);
	}
}

}

// vim:set ts=4 sw=4:
