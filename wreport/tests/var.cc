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
#include <wreport/var.h>
#include <wreport/vartable.h>
#include <math.h>

using namespace wreport;
using namespace std;

namespace tut {

struct var_shar
{
	var_shar()
	{
	}

	~var_shar()
	{
	}
};
TESTGRP(var);


// Test variable creation
template<> template<>
void to::test<1>()
{
	const Vartable* table = Vartable::get("B0000000000000014000");
	Varinfo info = table->query(WR_VAR(0, 6, 1));

	{
		Var var(info);
		ensure_equals(var.code(), WR_VAR(0, 6, 1));
		ensure_equals(var.info()->var, WR_VAR(0, 6, 1));
		ensure_equals(var.value(), (const char*)0);
	}

	{
		Var var(info, 123);
		ensure_equals(var.code(), WR_VAR(0, 6, 1));
		ensure_equals(var.info()->var, WR_VAR(0, 6, 1));
		ensure(var.value() != 0);
		ensure_var_equals(var, 123);
		var.seti(-123);
		ensure_var_equals(var, -123);
	}

	{
		Var var(info, 123.456);
		ensure_equals(var.code(), WR_VAR(0, 6, 1));
		ensure_equals(var.info()->var, WR_VAR(0, 6, 1));
		ensure(var.value() != 0);
		ensure_var_equals(var, 123.456);
		var.setd(-123.456);
		ensure_var_equals(var, -123.456);
	}

	{
		Var var(info, "123");
		ensure_equals(var.code(), WR_VAR(0, 6, 1));
		ensure_equals(var.info()->var, WR_VAR(0, 6, 1));
		ensure(var.value() != 0);
		ensure_var_equals(var, "123");
	}

#if 0
	{
		Var var(WR_VAR(0, 6, 1));
		ensure_equals(var.code(), WR_VAR(0, 6, 1));
		ensure_equals(var.value(), (const char*)0);
	}
#endif
}	

// Get and set values
template<> template<>
void to::test<2>()
{

}

// Test variable copy
template<> template<>
void to::test<3>()
{
	const Vartable* table = Vartable::get("B0000000000000014000");

	Var var(table->query(WR_VAR(0, 6, 1)));
	var.seti(234);
	
	var.seta(auto_ptr<Var>(new Var(table->query(WR_VAR(0, 33,  7)), 75)));
	var.seta(auto_ptr<Var>(new Var(table->query(WR_VAR(0, 33, 15)), 45)));

	ensure(var.enqa(WR_VAR(0, 33, 7)) != NULL);
	ensure_var_equals(*var.enqa(WR_VAR(0, 33, 7)), 75);

	ensure(var.enqa(WR_VAR(0, 33, 15)) != NULL);
	ensure_var_equals(*var.enqa(WR_VAR(0, 33, 15)), 45);


	Var var1 = var;
	ensure_var_equals(var1, 234);
	ensure(var == var1);
	ensure(var1 == var);

	ensure(var1.enqa(WR_VAR(0, 33, 7)) != NULL);
	ensure_var_equals(*var1.enqa(WR_VAR(0, 33, 7)), 75);

	ensure(var1.enqa(WR_VAR(0, 33, 15)) != NULL);
	ensure_var_equals(*var1.enqa(WR_VAR(0, 33, 15)), 45);

	// Fiddle with the attribute and make sure dba_var_equals notices
	var.seta(auto_ptr<Var>(new Var(table->query(WR_VAR(0, 33,  7)), 10)));
	ensure(var != var1);
	ensure(var1 != var);
}

// Test missing checks
template<> template<>
void to::test<4>()
{
	const Vartable* table = Vartable::get("B0000000000000014000");
	Var var(table->query(WR_VAR(0, 12, 103)));

	try {
		var.setd(logf(0));
		ensure(false);
	} catch (error_domain& e) {
		ensure(var.value() == NULL);
	}

	try {
		var.setd(logf(0)/logf(0));
		ensure(false);
	} catch (error_domain& e) {
		ensure(var.value() == NULL);
	}
}

// Test ranges
template<> template<>
void to::test<5>()
{
	const Vartable* table = Vartable::get("B0000000000000014000");
	Var var(table->query(WR_VAR(0, 21, 143)));

	var.setd(1.0);
	ensure_equals(var.enqd(), 1.0);

	var.setd(-1.0);
	ensure_equals(var.enqd(), -1.0);
}

// Test attributes
template<> template<>
void to::test<6>()
{
	const Vartable* table = Vartable::get("B0000000000000014000");
	Var var(table->query(WR_VAR(0, 21, 143)));

	// No attrs at the beginning
	ensure(var.enqa(WR_VAR(0, 33, 7)) == NULL);

	// Set an attr
	var.seta(auto_ptr<Var>(new Var(table->query(WR_VAR(0, 33, 7)), 42)));

	// Query it back
	ensure(var.enqa(WR_VAR(0, 33, 7)) != NULL);
	ensure_var_equals(*var.enqa(WR_VAR(0, 33, 7)), 42);

	// Unset it
	var.unseta(WR_VAR(0, 33, 7));

	// Query it back: it should be NULL
	ensure(var.enqa(WR_VAR(0, 33, 7)) == NULL);
}

}

/* vim:set ts=4 sw=4: */
