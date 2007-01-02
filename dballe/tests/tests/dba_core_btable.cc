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
#include <dballe/core/vartable.h>

namespace tut {
using namespace tut_dballe;

struct dba_core_btable_shar
{
	dba_core_btable_shar()
	{
	}

	~dba_core_btable_shar()
	{
	}
};
TESTGRP(dba_core_btable);

/* Test varcode encoding functions */
template<> template<>
void to::test<1>()
{
	gen_ensure_equals(DBA_VAR(0, 0, 0), 0);
	gen_ensure_equals(DBA_VAR(0, 0, 255), 0xff);
	gen_ensure_equals(DBA_VAR(0, 1, 0), 0x100);
	gen_ensure_equals(DBA_VAR(0, 63, 0), 0x3f00);
	gen_ensure_equals(DBA_VAR(0, 63, 255), 0x3fff);
	gen_ensure_equals(DBA_VAR(1, 0, 0), 0x4000);
	gen_ensure_equals(DBA_VAR(2, 0, 255), 0x80ff);
	gen_ensure_equals(DBA_VAR(3, 1, 0), 0xc100);
	gen_ensure_equals(DBA_VAR(1, 63, 0), 0x7f00);
	gen_ensure_equals(DBA_VAR(2, 63, 255), 0xbfff);
	gen_ensure_equals(DBA_VAR(3, 63, 255), 0xffff);

	gen_ensure_equals(DBA_STRING_TO_VAR("12345"), DBA_VAR(0, 12, 345));
	gen_ensure_equals(DBA_STRING_TO_VAR("00345"), DBA_VAR(0, 0, 345));
	gen_ensure_equals(DBA_STRING_TO_VAR("00000"), DBA_VAR(0, 0, 0));
	gen_ensure_equals(DBA_STRING_TO_VAR("63255"), DBA_VAR(0, 63, 255));

	gen_ensure_equals(dba_descriptor_code("B12345"), DBA_VAR(0, 12, 345));
	gen_ensure_equals(dba_descriptor_code("R00345"), DBA_VAR(1, 0, 345));
	gen_ensure_equals(dba_descriptor_code("C00000"), DBA_VAR(2, 0, 0));
	gen_ensure_equals(dba_descriptor_code("D63255"), DBA_VAR(3, 63, 255));
	gen_ensure_equals(dba_descriptor_code("012345"), DBA_VAR(0, 12, 345));
	gen_ensure_equals(dba_descriptor_code("100345"), DBA_VAR(1, 0, 345));
	gen_ensure_equals(dba_descriptor_code("200000"), DBA_VAR(2, 0, 0));
	gen_ensure_equals(dba_descriptor_code("363255"), DBA_VAR(3, 63, 255));
}

/* Test varcode alteration functions */
template<> template<>
void to::test<2>()
{
	dba_alteration a = DBA_ALT(1, 2);
	gen_ensure_equals(DBA_ALT_WIDTH(a), 1);
	gen_ensure_equals(DBA_ALT_SCALE(a), 2);

	a = DBA_ALT(-1, -2);
	gen_ensure_equals(DBA_ALT_WIDTH(a), -1);
	gen_ensure_equals(DBA_ALT_SCALE(a), -2);
}

/* Test querying CREX tables */
template<> template<>
void to::test<3>()
{
	/* dba_err err; */
	LocalEnv le("DBA_TABLES", ".");

	dba_vartable table;
	dba_varinfo info;

	CHECKED(dba_vartable_create("test-crex-table", &table));

	gen_ensure_equals(dba_vartable_query(table, DBA_VAR(0, 2, 99), &info), DBA_ERROR);

	CHECKED(dba_vartable_query(table, DBA_VAR(0, 1, 6), &info));
	gen_ensure_equals(info->var, DBA_VAR(0, 1, 6));
	gen_ensure_equals(strcmp(info->desc, "AIRCRAFT FLIGHT NUMBER"), 0);
	gen_ensure_equals(strcmp(info->unit, "CHARACTER"), 0);
	gen_ensure_equals(info->scale, 0) ;
	gen_ensure_equals(info->ref, 0);
	gen_ensure_equals(info->len, 8);
	gen_ensure(info->is_string);

	CHECKED(dba_vartable_query(table, DBA_VAR(0, 2, 114), &info));
	gen_ensure_equals(info->var, DBA_VAR(0, 2, 114));
	gen_ensure_equals(strcmp(info->desc, "ANTENNA EFFECTIVE SURFACE AREA"), 0);
	gen_ensure_equals(strcmp(info->unit, "M**2"), 0);
	gen_ensure_equals(info->scale, 0) ;
	gen_ensure_equals(info->ref, 0);
	gen_ensure_equals(info->len, 5);
	gen_ensure(!info->is_string);

	CHECKED(dba_vartable_query(table, DBA_VAR(0, 2, 153), &info));
	gen_ensure_equals(info->var, DBA_VAR(0, 2, 153));
	gen_ensure_equals(strcmp(info->desc, "SATELLITE CHANNEL CENTRE FREQUENCY"), 0);
	gen_ensure_equals(strcmp(info->unit, "Hz"), 0);
	gen_ensure_equals(info->scale, -8) ;
	gen_ensure_equals(info->ref, 0);
	gen_ensure_equals(info->len, 8);
	gen_ensure(!info->is_string);
}

/* Test querying BUFR tables */
template<> template<>
void to::test<4>()
{
	/* dba_err err; */
	LocalEnv le("DBA_TABLES", ".");

	dba_vartable table;
	dba_varinfo info;

	CHECKED(dba_vartable_create("test-bufr-table", &table));

	gen_ensure_equals(dba_vartable_query(table, DBA_VAR(0, 2, 99), &info), DBA_ERROR);

	CHECKED(dba_vartable_query(table, DBA_VAR(0, 1, 6), &info));
	gen_ensure_equals(info->var, DBA_VAR(0, 1, 6));
	gen_ensure_equals(strcmp(info->desc, "AIRCRAFT FLIGHT NUMBER"), 0);
	gen_ensure_equals(strcmp(info->unit, "CCITTIA5"), 0);
	gen_ensure_equals(info->scale, 0) ;
	gen_ensure_equals(info->ref, 0);
	gen_ensure_equals(info->bit_len, 64);
	gen_ensure_equals(info->len, 8);
	gen_ensure(info->is_string);

	CHECKED(dba_vartable_query(table, DBA_VAR(0, 2, 114), &info));
	gen_ensure_equals(info->var, DBA_VAR(0, 2, 114));
	gen_ensure_equals(strcmp(info->desc, "ANTENNA EFFECTIVE SURFACE AREA"), 0);
	gen_ensure_equals(strcmp(info->unit, "M**2"), 0);
	gen_ensure_equals(info->scale, 0) ;
	gen_ensure_equals(info->ref, 0);
	gen_ensure_equals(info->bit_len, 15);
	gen_ensure_equals(info->len, 5);
	gen_ensure(!info->is_string);

	CHECKED(dba_vartable_query(table, DBA_VAR(0, 11, 35), &info));
	gen_ensure_equals(info->var, DBA_VAR(0, 11, 35));
	gen_ensure_equals(strcmp(info->desc, "VERTICAL GUST ACCELERATION"), 0);
	gen_ensure_equals(strcmp(info->unit, "M/S**2"), 0);
	gen_ensure_equals(info->scale, 2) ;
	gen_ensure_equals(info->bit_ref, -8192);
	gen_ensure_equals(info->bit_len, 14);
	gen_ensure_equals(info->len, 5);
	gen_ensure(!info->is_string);
}

}

/* vim:set ts=4 sw=4: */
