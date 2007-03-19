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

#ifndef DBALLE_DB_ODBC_WORKAROUNDS_H
#define DBALLE_DB_ODBC_WORKAROUNDS_H

#include <bits/wordsize.h>

/** @file
 * @ingroup db
 *
 * The ODBC specification is imperfect with regards to integer sizes on 64bit
 * platforms, and different ODBC drivers are currently interpreting it
 * differently.
 */

#if __WORDSIZE == 64
#define DBALLE_SQL_C_SINT_TYPE long
#define DBALLE_SQL_C_UINT_TYPE unsigned long
#define DBALLE_SQL_C_SINT SQL_C_SBIGINT
#define DBALLE_SQL_C_UINT SQL_C_UBIGINT
#else
#define DBALLE_SQL_C_SINT_TYPE long
#define DBALLE_SQL_C_UINT_TYPE unsigned long
#define DBALLE_SQL_C_SINT SQL_C_SLONG
#define DBALLE_SQL_C_UINT SQL_C_ULONG
#endif

#endif
