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
#include <wreport/aliases.h>

using namespace wreport;

namespace tut {

struct aliases_shar
{
	aliases_shar()
	{
	}

	~aliases_shar()
	{
	}
};
TESTGRP(aliases);


// Test variable creation
template<> template<>
void to::test<1>()
{
	ensure_equals(varcode_alias_resolve("block"), WR_VAR(0, 1, 1));
	ensure_equals(varcode_alias_resolve("station"), WR_VAR(0, 1,  2));
	ensure_equals(varcode_alias_resolve("height"), WR_VAR(0, 7,  1));
	ensure_equals(varcode_alias_resolve("heightbaro"), WR_VAR(0, 7, 31));
	ensure_equals(varcode_alias_resolve("name"), WR_VAR(0, 1, 19));
	ensure_equals(varcode_alias_resolve("cippolippo"), 0);
}
	
}

/* vim:set ts=4 sw=4: */
