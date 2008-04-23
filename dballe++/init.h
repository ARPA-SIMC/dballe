#ifndef DBALLE_CPP_INIT_H
#define DBALLE_CPP_INIT_H

/*
 * init - Take care of initialising the DB-All.e library
 *
 * Copyright (C) 2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

namespace dballe {

/**
 * Keep one object of this class alive for as long as you want to use DB-All.e.
 *
 * Normally, you just create one in your main:
 * \code
 * int main(int argc, const char* argv[])
 * {
 *     dballe::DballeInit dballeInit;
 *
 *     ...
 *
 *     return 0;
 * }
 * \endcode
 */
struct DballeInit
{
	DballeInit();
	~DballeInit();
};

}

#endif
