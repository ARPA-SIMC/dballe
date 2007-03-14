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

#include "test-utils-db.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace tut_dballe {

dba_err create_dba_db(dba_db* db)
{
/*
	const char* uname = getenv("DBA_USER");
	if (uname == NULL)
	{
		struct passwd *pwd = getpwuid(getuid());
		uname = pwd == NULL ? "test" : pwd->pw_name;
	}
*/
	return dba_db_create("test", "" , "", db);
}

}
