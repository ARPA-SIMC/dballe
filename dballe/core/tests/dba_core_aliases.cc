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

#include <extra/test-utils-core.h>
#include <dballe/core/aliases.h>

namespace tut {
using namespace tut_dballe;

struct dba_core_aliases_shar
{
	dba_core_aliases_shar()
	{
	}

	~dba_core_aliases_shar()
	{
	}
};
TESTGRP(dba_core_aliases);


// Test variable creation
template<> template<>
void to::test<1>()
{
	gen_ensure_equals(dba_varcode_alias_resolve("block"), DBA_VAR(0, 1, 1));
	gen_ensure_equals(dba_varcode_alias_resolve("station"), DBA_VAR(0, 1,  2));
	gen_ensure_equals(dba_varcode_alias_resolve("height"), DBA_VAR(0, 7,  1));
	gen_ensure_equals(dba_varcode_alias_resolve("heightbaro"), DBA_VAR(0, 7, 31));
	gen_ensure_equals(dba_varcode_alias_resolve("name"), DBA_VAR(0, 1, 19));
	gen_ensure_equals(dba_varcode_alias_resolve("cippolippo"), 0);
}
	
}

/* vim:set ts=4 sw=4: */
