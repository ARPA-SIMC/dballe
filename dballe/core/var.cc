/*
 * dballe/var - DB-All.e specialisation of wreport variable
 *
 * Copyright (C) 2005,2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "var.h"
#include <wreport/vartable.h>

using namespace wreport;
using namespace std;

namespace dballe {

const Vartable* local = NULL;

wreport::Varinfo varinfo(wreport::Varcode code)
{
	if (local == NULL)
		local = Vartable::get("dballe");
	return local->query(code);
}

}

/* vim:set ts=4 sw=4: */
