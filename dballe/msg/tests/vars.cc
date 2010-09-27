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

#include <test-utils-msg.h>
#include <dballe/core/defs.h>
#include <dballe/msg/vars.h>

using namespace dballe;
using namespace std;

namespace tut {

struct msg_vars_shar
{
};
TESTGRP(msg_vars);

// Test variable alias resolution
template<> template<>
void to::test<1>()
{
    // First
    ensure_equals(resolve_var("block"), DBA_MSG_BLOCK);
    ensure_equals(resolve_var_substring("blocks", 5), DBA_MSG_BLOCK);

    // Last
	ensure_equals(resolve_var("tot_prec1"), DBA_MSG_TOT_PREC1);

    // Inbetween
	ensure_equals(resolve_var("cloud_h4"), DBA_MSG_CLOUD_H4);
	ensure_equals(resolve_var("st_type"), DBA_MSG_ST_TYPE);
	ensure_equals(resolve_var("tot_snow"), DBA_MSG_TOT_SNOW);
}

}

/* vim:set ts=4 sw=4: */
