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
#include <wreport/vartable.h>
#include <cstring>
#include <cstdlib>

using namespace wreport;
using namespace std;

namespace tut {

struct vartable_shar
{
	vartable_shar()
	{
	}

	~vartable_shar()
	{
	}
};
TESTGRP(vartable);

/* Test querying CREX tables */
template<> template<>
void to::test<1>()
{
	const char* testdatadir = getenv("WREPORT_TESTDATA");
	tests::LocalEnv le("WREPORT_TABLES", testdatadir ? testdatadir : ".");

	const Vartable* table = Vartable::get("test-crex-table");

	try {
		table->query(WR_VAR(0, 2, 99));
	} catch (error_notfound& e) {
		ensure_contains(e.what(), "002099");
	}

	Varinfo info = table->query(WR_VAR(0, 1, 6));
	ensure_equals(info->var, WR_VAR(0, 1, 6));
	ensure_equals(string(info->desc), string("AIRCRAFT FLIGHT NUMBER"));
	ensure_equals(string(info->unit), string("CCITTIA5"));
	ensure_equals(info->scale, 0) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 8);
	ensure(info->is_string());

	info = table->query(WR_VAR(0, 2, 114));
	ensure_equals(info->var, WR_VAR(0, 2, 114));
	ensure_equals(strcmp(info->desc, "ANTENNA EFFECTIVE SURFACE AREA"), 0);
	ensure_equals(strcmp(info->unit, "M**2"), 0);
	ensure_equals(info->scale, 0) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 5);
	ensure(!info->is_string());

	info = table->query(WR_VAR(0, 2, 153));
	ensure_equals(info->var, WR_VAR(0, 2, 153));
	ensure_equals(strcmp(info->desc, "SATELLITE CHANNEL CENTRE FREQUENCY"), 0);
	ensure_equals(strcmp(info->unit, "Hz"), 0);
	ensure_equals(info->scale, -8) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 8);
	ensure(!info->is_string());
}

/* Test querying BUFR tables */
template<> template<>
void to::test<2>()
{
	/* dba_err err; */
	const char* testdatadir = getenv("WREPORT_TESTDATA");
	tests::LocalEnv le("WREPORT_TABLES", testdatadir ? testdatadir : ".");

	const Vartable* table = Vartable::get("test-bufr-table");

	try {
		table->query(WR_VAR(0, 2, 99));
	} catch (error_notfound& e) {
		ensure(string(e.what()).find("002099") != -1);
	}

	Varinfo info = table->query(WR_VAR(0, 1, 6));
	ensure_equals(info->var, WR_VAR(0, 1, 6));
	ensure_equals(string(info->desc), string("AIRCRAFT FLIGHT NUMBER"));
	ensure_equals(string(info->unit), string("CCITTIA5"));
	ensure_equals(info->scale, 0) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 8);
	ensure_equals(info->bit_len, 64);
	ensure(info->is_string());

	info = table->query(WR_VAR(0, 2, 114));
	ensure_equals(info->var, WR_VAR(0, 2, 114));
	ensure_equals(strcmp(info->desc, "ANTENNA EFFECTIVE SURFACE AREA"), 0);
	ensure_equals(strcmp(info->unit, "M**2"), 0);
	ensure_equals(info->scale, 0) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 5);
	ensure_equals(info->bit_len, 15);
	ensure_equals(info->imin, 0);
	ensure_equals(info->imax, 32766);
	ensure(!info->is_string());

	info = table->query(WR_VAR(0, 11, 35));
	ensure_equals(info->var, WR_VAR(0, 11, 35));
	ensure_equals(strcmp(info->desc, "VERTICAL GUST ACCELERATION"), 0);
	ensure_equals(strcmp(info->unit, "M/S**2"), 0);
	ensure_equals(info->scale, 2) ;
	ensure_equals(info->bit_ref, -8192);
	ensure_equals(info->bit_len, 14);
	ensure_equals(info->len, 5);
	ensure(!info->is_string());

	info = table->query(WR_VAR(0, 7, 31));
	ensure_equals(info->var, WR_VAR(0, 7, 31));
	ensure_equals(string(info->desc), string("HEIGHT OF BAROMETER ABOVE MEAN SEA LEVEL"));
	ensure_equals(string(info->unit), "M");
	ensure_equals(info->scale, 1) ;
	ensure_equals(info->bit_ref, -4000);
	ensure_equals(info->bit_len, 17);
	ensure_equals(info->len, 6);
	ensure(!info->is_string());
}

/* Test reading BUFR edition 4 tables */
template<> template<>
void to::test<3>()
{
	const Vartable* table = Vartable::get("B0000000000098013102");

	try {
		table->query(WR_VAR(0, 2, 99));
	} catch (error_notfound& e) {
		ensure(string(e.what()).find("002099") != -1);
	}

	Varinfo info = table->query(WR_VAR(0, 1, 6));
	ensure_equals(info->var, WR_VAR(0, 1, 6));
	ensure_equals(string(info->desc), string("AIRCRAFT FLIGHT NUMBER"));
	ensure_equals(string(info->unit), string("CCITTIA5"));
	ensure_equals(info->scale, 0) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 8);
	ensure_equals(info->bit_len, 64);
	ensure(info->is_string());

	info = table->query(WR_VAR(0, 2, 114));
	ensure_equals(info->var, WR_VAR(0, 2, 114));
	ensure_equals(strcmp(info->desc, "ANTENNA EFFECTIVE SURFACE AREA"), 0);
	ensure_equals(strcmp(info->unit, "M**2"), 0);
	ensure_equals(info->scale, 0) ;
	ensure_equals(info->ref, 0);
	ensure_equals(info->len, 5);
	ensure_equals(info->bit_len, 15);
	ensure_equals(info->imin, 0);
	ensure_equals(info->imax, 32766);
	ensure(!info->is_string());

	info = table->query(WR_VAR(0, 11, 35));
	ensure_equals(info->var, WR_VAR(0, 11, 35));
	ensure_equals(strcmp(info->desc, "VERTICAL GUST ACCELERATION"), 0);
	ensure_equals(strcmp(info->unit, "M/S**2"), 0);
	ensure_equals(info->scale, 2) ;
	ensure_equals(info->bit_ref, -8192);
	ensure_equals(info->bit_len, 14);
	ensure_equals(info->len, 5);
	ensure(!info->is_string());
}

/* Test reading WMO standard tables */
template<> template<>
void to::test<4>()
{
	const Vartable* table = NULL;
	table = Vartable::get("B0000000000000012000");
	table = Vartable::get("B0000000000000013000");
	table = Vartable::get("B0000000000000014000");
}

}

/* vim:set ts=4 sw=4: */
