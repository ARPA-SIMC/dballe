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

#ifndef CPLUSPLUS_DBALLE_ERROR_H
#define CPLUSPLUS_DBALLE_ERROR_H

#include <dballe/err/dba_error.h>

#include <string>

class DBAException
{
public:
	const char* what() { return dba_error_get_message(); }

	std::string message() { return dba_error_get_message(); }
	std::string context() { return dba_error_get_context() ?: ""; }
	std::string details() { return dba_error_get_details() ?: ""; }

	std::string fulldesc() {
		std::string res = message() + " while " + context();
		const char* details = dba_error_get_details();
		if (details != NULL)
			res += std::string(".  Details:\n%s") + details;
		return res;
	}
};

#define DBA_RUN_OR_THROW(...) do { \
		if ((__VA_ARGS__) != DBA_OK) \
			throw DBAException(); \
	} while (0)

#endif
